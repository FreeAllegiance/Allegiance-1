#include "pch.h"
#include "trekctrls.h"
#include "trekmdl.h"
#include "badwords.h"

//////////////////////////////////////////////////////////////////////////////
//
// ChatListItem
//
//////////////////////////////////////////////////////////////////////////////

class ChatListItem : public ListItem
{
private:

    long                m_lData;
    ChatInfo*           m_pchatInfo;
    TVector<ZString>    m_vMsgLines;
    WinPoint            m_ptLineSize;
    
public:

    ChatListItem(ChatInfo* pchatInfo, const WinPoint& ptLineSize) :
        m_lData((long)pchatInfo),
        m_pchatInfo(pchatInfo),
        m_ptLineSize(ptLineSize)
    {
        ZString strMsg = CensorBadWords (m_pchatInfo->GetMessage());
		IEngineFont* pfont = pchatInfo->IsFromLeader() ? TrekResources::SmallBoldFont() : TrekResources::SmallFont();
        int nStrLenLeft = strMsg.GetLength();
        int nStrLenLine;
        while ((nStrLenLine = pfont->GetMaxTextLength(strMsg, ptLineSize.X(), true))
                < nStrLenLeft)
        {
            int nStrLenWordBreak = nStrLenLine;
            while (nStrLenWordBreak > 2 && strMsg[nStrLenWordBreak] != ' ' && strMsg[nStrLenWordBreak-1] != ' ')
                nStrLenWordBreak--;
            if (nStrLenWordBreak != 2)
                nStrLenLine = nStrLenWordBreak;
            
            if (nStrLenLine <= 2)
            {
                // put a blank line without the character
                nStrLenLine = 3;
                m_vMsgLines.PushEnd("");
            }
            else
            {
                m_vMsgLines.PushEnd(strMsg.Left(nStrLenLine));
            }
            strMsg = "  " + strMsg.RightOf(nStrLenLine);
            nStrLenLeft -= nStrLenLine - 2;
            ZAssert(strMsg.GetLength() == nStrLenLeft);
        }
        m_vMsgLines.PushEnd(strMsg);
    }

    ~ChatListItem()
    {
        // debugf("~ChatListItem\n");
    }

    short GetItemHeight()
    {
        return m_vMsgLines.GetCount();
    }

    long GetItemData()
    {
        return m_lData;
    };

    bool Update()
    {
        return true;
    }

    void SetSortOrder(long lSortOrder)
    {
    }
    
    bool SetFilter(long lFilter)
    {
        return true;
    }

    void DrawItem(Surface* pSurface, const WinRect& rect, bool fSelected, int iFirstSlot)
    {
        IEngineFont* pfont = m_pchatInfo->IsFromLeader() ? TrekResources::SmallBoldFont() : TrekResources::SmallFont();
        WinPoint pt(rect.Min() + WinPoint(3,3));
        for (
            int i = iFirstSlot; 
            i < m_vMsgLines.GetCount() && pt.Y() < rect.YMax(); 
            i++
        ) {
            pSurface->DrawString(pfont, m_pchatInfo->GetColor(), pt, m_vMsgLines[i]);
            pt += WinPoint(0, m_ptLineSize.Y());
        }
    }

};

//////////////////////////////////////////////////////////////////////////////
//
// ChatListPaneImpl
//
//////////////////////////////////////////////////////////////////////////////

class ChatListPaneImpl :    public ChatListPane, 
                            public EventTargetContainer<ChatListPaneImpl>,
                            public TrekClientEventSink,
                            public IKeyboardInput
{
private:
    TRef<ListPaneOld>      m_pListPane;
    WinPoint               m_ptItemSize;
	WinPoint			   m_ptSize;		// #294 - Turkey
    TList<ChatTarget>      m_listChannels;
    TRef<IKeyboardInput>   m_keyboardDelegate;
    TRef<IAllegEventTarget>     m_targetAutoscrollOn;
    bool                   m_bAutoscroll;
    bool                   m_bPlayerChatsOnly;
    bool                   m_bIgnoreScrollingEvents;

public:


    ChatListPaneImpl(const WinPoint& ptSize):
        m_ptItemSize(ptSize.X(), 12), m_bAutoscroll(true), m_bIgnoreScrollingEvents(false), m_ptSize(ptSize) // #294 - Turkey
    {
        m_bPlayerChatsOnly = true;
        m_pListPane = ListPaneOld::Create(m_ptSize, 12, true, NULL), // #294 - Turkey
        InsertAtBottom(m_pListPane);
		//mdvalley: I hate C3867.
        AddEventTarget(&ChatListPaneImpl::OnListSelect, m_pListPane->GetEventSource());
        AddEventTarget(&ChatListPaneImpl::OnScroll, m_pListPane->GetScrollEvent());
        UpdateContents();
        m_keyboardDelegate = IKeyboardInput::CreateDelegate(this);
        GetWindow()->AddKeyboardInputFilter(m_keyboardDelegate);
    }

    ~ChatListPaneImpl()
    {
        GetWindow()->RemoveKeyboardInputFilter(m_keyboardDelegate);
        if (m_targetAutoscrollOn)
            m_targetAutoscrollOn->Disconnect();
    }

	void SetScrollbarHidden(bool isHidden)
	{
		m_pListPane->SetScrollbarHidden(isHidden);
	}


    void AutoscrollOff()
    {
        m_bAutoscroll = false;

        // set/reset a timer to turn it back on after 10 seconds
        if (m_targetAutoscrollOn)
            m_targetAutoscrollOn->Disconnect();

        m_targetAutoscrollOn = 
            new TEventTarget<ChatListPaneImpl>(this, &ChatListPaneImpl::OnAutoscrollTimeout, GetWindow(), 10);
    }

    void AutoscrollOn()
    {
        m_bAutoscroll = true;

        if (m_targetAutoscrollOn)
            m_targetAutoscrollOn->Disconnect();
    }

    bool OnAutoscrollTimeout()
    {
        m_pListPane->ScrollBottom();
        AutoscrollOn();

        return false;
    }

    bool OnListSelect(int nSel)
    {
        return true;
    }

    void OnClearChat()
    {
        m_pListPane->RemoveAll();
    }

    void OnChatMessageChange()
    {
        // save some info about the current state of the chat list
        int countOld = m_pListPane->GetCountItems();
        int posOld = m_pListPane->GetScrollPosition();

        UpdateContents();

        // if the number of items did not change, assume we were just doing 
        // something like marking a player as dead so keep the current scroll
        // position.  
        if (countOld == m_pListPane->GetCountItems())
            m_pListPane->SetScrollPosition(posOld);
    }

    bool ChatIsNonCriticalMessage(ChatInfo * pchatinfo)
    {
        return !pchatinfo->IsFromPlayer()
            && !pchatinfo->IsFromObjectModel()
            && pchatinfo->GetChatTarget() != CHAT_INDIVIDUAL_NOFILTER
            && m_bPlayerChatsOnly;
    }

    void OnDeleteChatMessage(ChatInfo* pchatInfo)
    {
        m_bIgnoreScrollingEvents = true;
        m_pListPane->RemoveItemByData((long)pchatInfo);
        m_bIgnoreScrollingEvents = false;
    }

    void OnNewChatMessage()
    {
        ChatInfo * pchatinfo = &(trekClient.m_chatList.last()->data());

        if (ChatIsNonCriticalMessage(pchatinfo))
        {
            trekClient.PostText(false, CensorBadWords (pchatinfo->GetMessage()));
        }
        else
        {
            AddChatItem(pchatinfo);
        }
    }

    void PageUp()
    {
        m_pListPane->ScrollPageUp();
        AutoscrollOff();
    }

    void PageDown()
    {
        m_pListPane->ScrollPageDown();
        
        // if we are at the bottom of the list, turn auto-scroll back on
        if (m_pListPane->IsItemVisibleByIdx(m_pListPane->GetCountItems() - 1))
            AutoscrollOn();
        else
            AutoscrollOff();
    }

    bool OnScroll()
    {
        if (!m_bIgnoreScrollingEvents)
        {
            // if we are at the bottom of the list, turn auto-scroll back on
            if (m_pListPane->IsItemVisibleByIdx(m_pListPane->GetCountItems() - 1))
                AutoscrollOn();
            else
                AutoscrollOff();
        }

        return true;
    }

    void UpdateContents()
    {
        m_pListPane->RemoveAll();

        for (ChatLink* link = trekClient.m_chatList.first(); link; link = link->next())
        {
            AddChatItem(&(link->data()));
        }

        m_pListPane->ScrollBottom();
        AutoscrollOn();
    }

    bool ChatPassesFilter(ChatInfo* pchatInfo)
    {
        if (ChatIsNonCriticalMessage(pchatInfo))
        {
            return false;
        }
        if (m_listChannels.IsEmpty())
        {
            return true;
        }
        else
        {
            TList<ChatTarget>::Iterator iter(m_listChannels);
            
            while (!iter.End()) 
            {
                if (iter.Value() == pchatInfo->GetChatTarget())
                    return true;
                iter.Next();
            }
        }

        return false;
    }

    void AddChatItem(ChatInfo* pchatInfo)
    {
        if (ChatPassesFilter(pchatInfo))
        {
            m_bIgnoreScrollingEvents = true;
            int idx = m_pListPane->AppendItem(new ChatListItem(pchatInfo, m_ptItemSize));
            
            if (m_bAutoscroll)
            {
                m_pListPane->SetSelItemByIdx(idx);
                m_pListPane->ScrollBottom();
            }
            m_bIgnoreScrollingEvents = false;
        }
    }

    void AddChannel(ChatTarget channel)
    {
        m_listChannels.PushEnd(channel);
        UpdateContents();
    }

    void RemoveChannel(ChatTarget channel)
    {
        m_listChannels.Remove(channel);
        UpdateContents();
    }

    void ResetChannels()
    {
        m_listChannels.SetEmpty();
        UpdateContents();
    }

    void UpdateLayout()
    {
        DefaultUpdateLayout();
    }

	// #294 - Turkey
	void SetChatLines(int lines)
	{
		m_ptSize.SetY(lines * m_ptItemSize.Y() + 8);
		m_pListPane->SetListSize(m_ptSize);
		UpdateContents();
	}

};


TRef<IObject> ChatListPaneFactory::Apply(ObjectStack& stack)
{
    TRef<PointValue>  ppointSize; CastTo(ppointSize, stack.Pop());

	TRef<Boolean> showChatPaneScrollbar = new Boolean(true);
	if(stack.GetCount() > 0)
		CastTo(showChatPaneScrollbar, stack.Pop());

	ChatListPaneImpl *chatPane = new ChatListPaneImpl(
                WinPoint(
                    (int)ppointSize->GetValue().X(),
                    (int)ppointSize->GetValue().Y()
                    )
                );    
	
	chatPane->SetScrollbarHidden(showChatPaneScrollbar->GetValue() == false);

	return (Pane*) chatPane;
}
