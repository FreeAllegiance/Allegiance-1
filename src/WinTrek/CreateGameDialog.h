#pragma once

#include "pch.h"

// internal structs for core/server
struct CoreInfo
{
    char Name[c_cbName];
    bool bOfficial;
    DWORD dwBit;
    StaticCoreInfo mStatic;
};
struct ServerInfo
{
    ServerCoreInfo mStatic;
    int ping;
    bool bOfficial;
    bool IsFull()
    {
        return (mStatic.iCurGames >= mStatic.iMaxGames);
    }
    bool HandleCore(CoreInfo *pcore)
    {
        return (mStatic.dwCoreMask & pcore->dwBit);
    }
};

class CreateGameDialogPopup :
    public IPopup,
    public EventTargetContainer<CreateGameDialogPopup>,
    public IItemEvent::Sink
{
    class ServerItemPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        CreateGameDialogPopup* m_pparent;
    public:
        ServerItemPainter(const TVector<int>& viColumns, CreateGameDialogPopup* pparent) :
            m_viColumns(viColumns), m_pparent(pparent) {};
        int GetXSize()
        {
            return m_viColumns[4];
        }

        int GetYSize()
        {
            return 14;
        }
        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            ServerInfo *pserver = (ServerInfo*)pitemArg;
            CoreInfo *pcore = m_pparent->GetSelectedCore();

            TRef<IEngineFont> pfont = TrekResources::SmallFont();
            Color             color = Color::White();

            if (bSelected) {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(0, 0, 1)*0.7f
                );
            }
            if (pserver->bOfficial)
                DrawIcon(psurface, 0, GetYSize() / 2, "iconscorescountbmp");

            if (pcore) if (!pserver->HandleCore(pcore)) color = Color::White()*0.5f;

            char cbTemp[256];
            WinRect rectClipOld = psurface->GetClipRect();

            psurface->SetClipRect(WinRect(m_viColumns[0], 0, m_viColumns[1], GetYSize())); // clip name to fit in column
            wsprintf(cbTemp, "%s", pserver->mStatic.szName);
            psurface->DrawString(pfont, color,
                WinPoint(m_viColumns[0] + 1, 1),
                cbTemp);
            psurface->RestoreClipRect(rectClipOld);

            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(m_viColumns[1], 0, m_viColumns[2], GetYSize())); // clip name to fit in column
            wsprintf(cbTemp, "%s", pserver->mStatic.szLocation);
            psurface->DrawString(pfont, color,
                WinPoint(m_viColumns[1] + 1, 1),
                cbTemp);
            psurface->RestoreClipRect(rectClipOld);


            wsprintf(cbTemp, "%d/%d", pserver->mStatic.iCurGames, pserver->mStatic.iMaxGames);
            psurface->DrawString(pfont, pserver->IsFull() ? Color::Red() : color,
                WinPoint(m_viColumns[2] + 1, 1),
                cbTemp);

            wsprintf(cbTemp, "%d", pserver->ping);
            color = Color(1, 1, 0); // yellow
            if (pserver->ping<100) color = Color::Green();
            if (pserver->ping>250) color = Color::Red();
            if (pserver->ping == NA)
            {
                color = Color::Gray();
                strcpy(cbTemp, "?");
            }
            psurface->DrawString(pfont, color,
                WinPoint(m_viColumns[3] + 1, 1),
                cbTemp);
            // line under
            psurface->FillRect(WinRect(0, GetYSize() - 1, GetXSize(), GetYSize()), Color::Gray());
        }
        int DrawIcon(Surface* psurface, int nXLeft, int nYCenter, const char* iconName)
        {
            TRef<Image> pimage = GetModeler()->LoadImage(iconName, true);

            WinPoint pntIcon(
                nXLeft,
                nYCenter - (int)pimage->GetBounds().GetRect().YSize() / 2
            );
            psurface->BitBlt(pntIcon, pimage->GetSurface());

            return nXLeft + (int)pimage->GetBounds().GetRect().XSize() + 2;
        }
    };
    class CoreItemPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        CreateGameDialogPopup* m_pparent;
    public:
        CoreItemPainter(const TVector<int>& viColumns, CreateGameDialogPopup* pparent) :
            m_viColumns(viColumns), m_pparent(pparent) {};
        int GetXSize()
        {
            return m_viColumns[1];
        }

        int GetYSize()
        {
            return 14;
        }
        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            CoreInfo *pcore = (CoreInfo*)pitemArg;
            TRef<IEngineFont> pfont = TrekResources::SmallFont();
            Color             color = Color::White();
            ServerInfo *pserver = m_pparent->GetSelectedServer();

            if (bSelected) {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(0, 0, 1)
                );
            }
            char cbTemp[256];

            if (pserver) if (!pserver->HandleCore(pcore)) color = Color::White()*0.5f;

            if (pcore->bOfficial)
                DrawIcon(psurface, 0, GetYSize() / 2, "iconscorescountbmp");

            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(m_viColumns[0], 0, m_viColumns[1], GetYSize())); // clip name to fit in column

            wsprintf(cbTemp, "%s", pcore->Name);
            psurface->DrawString(pfont, color,
                WinPoint(m_viColumns[0] + 1, 1),
                cbTemp);
            psurface->RestoreClipRect(rectClipOld);

        }
        int DrawIcon(Surface* psurface, int nXLeft, int nYCenter, const char* iconName)
        {
            TRef<Image> pimage = GetModeler()->LoadImage(iconName, true);

            WinPoint pntIcon(
                nXLeft,
                nYCenter - (int)pimage->GetBounds().GetRect().YSize() / 2
            );
            psurface->BitBlt(pntIcon, pimage->GetSurface());

            return nXLeft + (int)pimage->GetBounds().GetRect().XSize() + 2;
        }
    };


private:
    TRef<Pane> m_ppane;
    TRef<ButtonPane> m_pbuttonOK;
    TRef<ButtonPane> m_pbuttonCancel;
    TRef<EditPane>	 m_peditGameName;

    TRef<ListPane>   m_plistPaneServers;
    TRef<ListPane>   m_plistPaneCores;
    TVector<int>     m_viServerColumns;
    TVector<int>     m_viCoreColumns;

    TRef<IItemEvent::Source>   m_peventServers;
    TRef<TEvent<ItemID>::Sink> m_psinkServers;
    TRef<IItemEvent::Source>   m_peventCores;
    TRef<TEvent<ItemID>::Sink> m_psinkCores;

    TRef<IKeyboardInput> m_pkeyboardInputOldFocus;
    //data
    int m_cCores;
    CoreInfo *m_pCores;
    int m_cServers;
    ServerInfo *m_pServers;

public:
    // ctor
    CreateGameDialogPopup(TRef<INameSpace> pns) :
        m_cCores(0),
        m_pCores(NULL),
        m_cServers(0),
        m_pServers(NULL)
    {
        CastTo(m_ppane, pns->FindMember("createDialog"));
        CastTo(m_pbuttonOK, pns->FindMember("createOkButtonPane"));
        CastTo(m_pbuttonCancel, pns->FindMember("createCancelButtonPane"));
        CastTo(m_peditGameName, (Pane*)pns->FindMember("createEditPane"));

        TRef<IObject> pserverColumns;
        TRef<IObject> pcoreColumns;

        CastTo(m_plistPaneServers, (Pane*)pns->FindMember("serverListPane"));
        CastTo(m_plistPaneCores, (Pane*)pns->FindMember("coreListPane"));
        CastTo(pserverColumns, pns->FindMember("serverColumns"));
        CastTo(pcoreColumns, pns->FindMember("coreColumns"));

        ParseIntVector(pserverColumns, m_viServerColumns);
        ParseIntVector(pcoreColumns, m_viCoreColumns);

        m_plistPaneServers->SetItemPainter(new ServerItemPainter(m_viServerColumns, this));
        m_plistPaneCores->SetItemPainter(new CoreItemPainter(m_viCoreColumns, this));

        AddEventTarget(&CreateGameDialogPopup::OnButtonOK, m_pbuttonOK->GetEventSource());
        AddEventTarget(&CreateGameDialogPopup::OnButtonCancel, m_pbuttonCancel->GetEventSource());

    }
    // construct data
    void FreeData()
    {
        if (m_cCores) delete[] m_pCores;
        m_cCores = 0;
        m_pCores = NULL;
        if (m_cServers) delete[] m_pServers;
        m_cServers = 0;
        m_pServers = NULL;
    }
    void OnServersList(int cCores, StaticCoreInfo *pcores, int cServers, ServerCoreInfo *pservers)
    {
        FreeData();
        m_cCores = cCores;
        m_cServers = cServers;
        if (cCores)
        {
            m_pCores = new CoreInfo[cCores]; //Fix memory leak -Imago 8/2/09
            for (int i = 0; i < cCores; i++)
            {
                memcpy(&(m_pCores[i].mStatic), &(pcores[i]), sizeof(StaticCoreInfo));
                m_pCores[i].bOfficial = trekClient.CfgIsOfficialCore(m_pCores[i].mStatic.cbIGCFile);
                m_pCores[i].dwBit = 1 << i;
                strcpy(m_pCores[i].Name, trekClient.CfgGetCoreName(m_pCores[i].mStatic.cbIGCFile));
            }
        }
        if (cServers)
        {
            m_pServers = new ServerInfo[cServers]; //Fix memory leak -Imago 8/2/09
            for (int i = 0; i < cServers; i++)
            {
                memcpy(&(m_pServers[i].mStatic), &(pservers[i]), sizeof(ServerCoreInfo));
                m_pServers[i].ping = -1;
                m_pServers[i].bOfficial = trekClient.CfgIsOfficialServer(m_pServers[i].mStatic.szName, m_pServers[i].mStatic.szRemoteAddress);
            }
        }
    }

    // logic
    ServerInfo *GetSelectedServer()
    {
        return (ServerInfo *)m_plistPaneServers->GetSelection();
    }
    CoreInfo *GetSelectedCore()
    {
        return (CoreInfo *)m_plistPaneCores->GetSelection();
    }
    // logic for ok button
    void RefreshOkButton()
    {
        ServerInfo *pserver = GetSelectedServer();
        CoreInfo *pcore = GetSelectedCore();
        if (pserver)
            if (!pserver->IsFull())
                if (pcore)
                {
                    m_pbuttonOK->SetHidden(!pserver->HandleCore(pcore));
                    return;
                }
        m_pbuttonOK->SetHidden(true);
    }
    // logic for server list
    void OnSelectServer(ServerInfo* pserver)
    {
        CoreInfo *pcore = (CoreInfo *)m_plistPaneCores->GetSelection();
        m_plistPaneCores->ForceRefresh();
        if (pcore && pserver)
        {
            if (!pserver->HandleCore(pcore))
                // this server doesnt support current core so unselect core
                m_plistPaneCores->SetSelection(NULL);
        }
        RefreshOkButton();
    }
    // logic for core list
    void OnSelectCore(CoreInfo* pcore)
    {
        ServerInfo *pserver = (ServerInfo *)m_plistPaneServers->GetSelection();
        m_plistPaneServers->ForceRefresh();
        if (pserver && pcore)
        {
            if (!pserver->HandleCore(pcore))
                // this core not supported on current server so unselect server
                m_plistPaneServers->SetSelection(NULL);

        }
        RefreshOkButton();
    }
    // events
    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        if (pevent == m_peventServers) {
            ServerInfo* p = (ServerInfo*)pitem;
            OnSelectServer(p);
        }
        if (pevent == m_peventCores) {
            CoreInfo* p = (CoreInfo*)pitem;
            OnSelectCore(p);
        }
        return true;
    }
    // IPopUp
    virtual void OnClose()
    {
        if (m_pkeyboardInputOldFocus)
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);

        m_pkeyboardInputOldFocus = NULL;

        m_peventServers->RemoveSink(m_psinkServers);
        m_peventCores->RemoveSink(m_psinkCores);
        IPopup::OnClose();
    }
    Pane* GetPane()
    {
        return m_ppane;
    }
    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        // we need to make sure we get OnChar calls to pass on to the edit box
        fForceTranslate = true;
        return false;
    }

    bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    {
        if (ks.vk == VK_ESCAPE)
        {
            OnButtonCancel();
            return true;
        }
        else
            return ((IKeyboardInput*)m_peditGameName)->OnChar(pprovider, ks);
    }
    virtual void SetContainer(IPopupContainer* pcontainer)
    {
        TRef<TListListWrapper<ServerInfo*> > plistServers = new TListListWrapper<ServerInfo*>();
        TRef<TListListWrapper<CoreInfo*> > plistCores = new TListListWrapper<CoreInfo*>();

        for (int i = 0; i<m_cCores; i++)	plistCores->PushEnd(&(m_pCores[i]));
        for (int i = 0; i<m_cServers; i++) plistServers->PushEnd(&(m_pServers[i]));

        m_plistPaneServers->SetList(plistServers);
        m_plistPaneCores->SetList(plistCores);

        m_peventServers = m_plistPaneServers->GetSelectionEventSource();
        m_peventServers->AddSink(m_psinkServers = new IItemEvent::Delegate(this));
        m_peventCores = m_plistPaneCores->GetSelectionEventSource();
        m_peventCores->AddSink(m_psinkCores = new IItemEvent::Delegate(this));

        RefreshOkButton();
        // KGJV fix: remove trailing rank "(#)" from default game name
        ZString szPlayerName = ZString(trekClient.GetNameLogonZoneServer());
        int leftParen = szPlayerName.ReverseFind('(', 0);
        if (leftParen > 1)
            szPlayerName = szPlayerName.Left(leftParen);

        m_peditGameName->SetString(szPlayerName + ZString("'s game"));

        m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
        GetEngineWindow()->SetFocus(m_peditGameName);

        IPopup::SetContainer(pcontainer);
    }
    bool OnButtonOK()
    {
        if (m_peditGameName->GetString().IsEmpty())
        {
            TRef<IMessageBox> pmsgBox =
                CreateMessageBox("Error : Game name is empty");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            return true;
        }
        if (m_peditGameName->GetString().GetLength() >= c_cbGameName)
        {
            TRef<IMessageBox> pmsgBox =
                CreateMessageBox("Error : Game name is too long");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            return true;
        }

        if (m_ppopupOwner) {
            m_ppopupOwner->ClosePopup(this);
        }
        else {
            m_pcontainer->ClosePopup(this);
        }
        // create the game
        //1. get the name, server and core
        ServerInfo *pserver = (ServerInfo*)m_plistPaneServers->GetSelection();
        CoreInfo *pcore = (CoreInfo*)m_plistPaneCores->GetSelection();
        const ZString pgamename = m_peditGameName->GetString();
        //2. send the create message
        trekClient.CreateMissionReq(pserver->mStatic.szName, pserver->mStatic.szRemoteAddress, pcore->mStatic.cbIGCFile, pgamename);
        return true;
    }
    bool OnButtonCancel()
    {
        if (m_ppopupOwner) {
            m_ppopupOwner->ClosePopup(this);
        }
        else {
            m_pcontainer->ClosePopup(this);
        }
        return true;
    }
};