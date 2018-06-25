#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Squads Screen
//
//////////////////////////////////////////////////////////////////////////////
//
// Functionality ideas:
//
// Make Tab switch between the fields just like Enter does now.  Make ESC
// cancel create/edit.
//
// Allow the selected squad to remain selected when switching between tabs.
//
// OPTimization idea:
//
//  We could reduce the Clubserver/SQL server load not sending the squad page requests
//  so often, and simply using what is in our cache.
//

class SquadsScreen;
extern SquadsScreen * g_pSquadScreen; // messy but avoids .h file

const int c_cSquadsShown = 21;  

class SquadsScreen :
    public Screen,
    public EventTargetContainer<SquadsScreen>,
    public TEvent<ZString>::Sink,
    public TrekClientEventSink,
    public IItemEvent::Sink
{

private:

    TRef<IKeyboardInput>        m_pkeyboardInputOldFocus;

    TRef<Pane>          m_ppane;

    //
    // Normal buttons
    //
    TRef<ButtonPane>    m_pbuttonZoneClub;
    TRef<ButtonPane>    m_pbuttonPlayerProfile; 
    TRef<ButtonPane>    m_pbuttonLeaderboard; 
    TRef<ButtonPane>    m_pbuttonEditSquad;
    TRef<ButtonPane>    m_pbuttonReject;
    TRef<ButtonPane>    m_pbuttonAccept;
    TRef<ButtonPane>    m_pbuttonCommission;
    TRef<ButtonPane>    m_pbuttonDemote;
    TRef<ButtonPane>    m_pbuttonTransferControl;
    TRef<ButtonPane>    m_pbuttonRequestJoin;
    TRef<ButtonPane>    m_pbuttonCancelRequest;
    TRef<ButtonPane>    m_pbuttonQuitSquad;
    TRef<ButtonPane>    m_pbuttonCreateSquad;
    TRef<ButtonPane>    m_pbuttonNext; // goto next group of squads
    TRef<ButtonPane>    m_pbuttonPrev; // goto prev group of squads
    TRef<ButtonPane>    m_pbuttonTop; // goto top of squads list
    TRef<ButtonPane>    m_pbuttonLog;
    TRef<ButtonPane>    m_pbuttonApply; // apply edit changes
    TRef<ButtonPane>    m_pbuttonURL;
    TRef<ButtonPane>    m_pbuttonFindSquad;

    //
    // Icons
    //
    TRef<Image>         m_pimageOwner;
    TRef<Image>         m_pimageASL;
    TRef<Image>         m_pimagePending;
    TRef<Image>         m_pimageRejected;
    TRef<Image>         m_pimageMember;
    TRef<Image>         m_pimageBooted;
    TRef<Image>         m_pimageQuit;

    //
    // Tabs
    //
    enum FilterType
    {
      FILTER_ALL = 0,
      FILTER_MY,
      FILTER_DUDEX,
      FILTER_MAX
    };

    FilterType          m_filter;

    Color               m_colorTabText;
    Color               m_colorTabTextSelected;

    TVector<int>        m_viTabColumns;

    TRef<ButtonBarPane> m_ptabSquads;
    TRef<ImagePane>     m_ptabSquadsImage;

    TRef<StringValue>   m_pstringSquadTabAll;
    TRef<StringValue>   m_pstringSquadTabMy;
    TRef<ModifiableString> m_pstringSquadTabDudeX;

    //
    // Squads Listbox
    //
    TRef<ButtonBarPane> m_pbuttonbarSquadsHeader;

    TRef<ListPane>      m_plistPaneSquads;
    TRef<ScrollPane>    m_pscrollPaneSquads;
    TRef<IItemEvent::Source> m_peventSquads;
    TRef<TEvent<ItemID>::Sink> m_psinkSquads;

    TVector<int>        m_viColumns;
    TVector<unsigned, DefaultEquals> m_vuSorts; // used for "my" and dudex tabs
    static SQUAD_SORT_COLUMN s_column; // currently sorted column for squads


    Color               m_colorSquadSelectionBar;

    XSquads             m_listSquads[FILTER_MAX];  // core listbox data; one list for each tab 0 == all, 1 == my, 2 == dudex

    XSquads             m_listAllSquads;  // any squad made that was placed in tab 0 (the ALL tab)
    XZonePlayers        m_listAllPlayers;

    static TRef<IZoneSquad> s_pSelectedSquad; 
                      
    //
    // Squad details data
    // 
    TRef<EditPane>      m_peditPaneSquadName;
    TRef<EditPane>      m_peditPaneSquadDescription;
    TRef<EditPane>      m_peditPaneSquadURL;
    TRef<EditPane>      m_peditPaneFind;

    TRef<TEvent<ZString>::Source> m_peventNameEdit;
    TRef<TEvent<ZString>::Sink> m_psinkNameEdit;

    TRef<TEvent<ZString>::Source> m_peventDescEdit;
    TRef<TEvent<ZString>::Sink> m_psinkDescEdit;

    TRef<TEvent<ZString>::Source> m_peventURLEdit;
    TRef<TEvent<ZString>::Sink> m_psinkURLEdit;

    TRef<TEvent<ZString>::Source> m_peventFind;
    TRef<TEvent<ZString>::Sink> m_psinkFind;

    TRef<StringPane>    m_pstrpaneRank;
    TRef<StringPane>    m_pstrpaneRanking; 
    TRef<StringPane>    m_pstrpaneWinsAndLosses;
    TRef<StringPane>    m_pstrpaneInceptionDate;

    TRef<ComboPane>     m_pcomboCiv;

    //
    // Players Listbox
    //
    TRef<ButtonBarPane> m_pbuttonbarPlayersHeader;

    TRef<ListPane>      m_plistPanePlayers;

    TRef<IItemEvent::Source> m_peventPlayers;
//    TRef<IItemEvent::Source> m_peventPlayersDoubleClick;

    TRef<TEvent<ItemID>::Sink> m_psinkPlayers;
//    TRef<TEvent<ItemID>::Sink> m_psinkPlayersDoubleClick ;

    TVector<int>        m_viPlayersColumns;
    TVector<unsigned, DefaultEquals> m_vuPlayersSorts;

    Color               m_colorPlayerSelectionBar;

    static TRef<IZonePlayer> s_pSelectedPlayer;

    static TRef<IZonePlayer> s_pDudeXPlayer;

    TRef<IZoneSquad>  m_psquadLastDetails; // used only by ForwardSquadMessage

    TRef<IZoneSquad>  m_psquadPrePage; 

    TRef<IZoneSquad>  m_psquadBeingMade;  // NULL else a squad is being created

    XZonePlayers      m_listPlayers; // players in listbox

    int               m_nNextCreationID;

    int               m_nHighestRanking[SSC_MAX]; // highest as in closest to 1
    TRef<IZoneSquad>  m_psquadHighestRanking[SSC_MAX]; 

    int               m_nLowestRanking[SSC_MAX]; // lowest as in closest to infinity
    TRef<IZoneSquad>  m_psquadLowestRanking[SSC_MAX]; 

    int               m_nCurrentRanking; // used in determining which squads get shown; used with c_cSquadsShown

    TRef<IZoneSquad>  m_psquadLastSelected;

    ZString           m_strSquadToBeSelected;

    int               m_nTab; // Currently selected Tab; 0 == ALL; 1 == My; 2 == DudeX

    unsigned          m_nBestSearchMatch; // used for selecting correct squad of a serach result

    int*              m_pCivIds;
    int               m_cCivs;

    // remember a coupld bits so we know when a player has clicked in the edit box for the first
    // time since creation
    bool              m_bSquadEditNameReset;
    bool              m_bSquadEditDescReset;
    bool              m_bSquadEditURLReset;

    bool              m_bLeftScreen;

    bool              m_bSelectorNeedsAutoSet; // set to true if we clear the list of squads; it means: do we need to auto select a squad for user

    bool              m_bSelectionNotFromSquadClick; // using clicked on something other than a squad to cause a squad to be selected

    TRef<IMessageBox> m_pmsgBox;

    TRef<ModifiableString> m_pstringDescription;

    //
    // History Popup List (Ownership Log)
    //

    friend class HistoryDialogPopup;
    class HistoryDialogPopup : public IPopup, public EventTargetContainer<HistoryDialogPopup> 
    {
    private:
        TRef<Pane> m_ppane;
        TRef<ButtonPane> m_pbuttonOK;
        TRef<EditPane>   m_peditPane;
        TRef<StringListPane>  m_plistPaneHistory;

        SquadsScreen* m_pparent;

    public:
        
        HistoryDialogPopup(TRef<INameSpace> pns, SquadsScreen* pparent)
        {
            m_pparent = pparent;
            CastTo(m_ppane,                  pns->FindMember("historyDialog"        ));
            CastTo(m_pbuttonOK,              pns->FindMember("historyOkButtonPane"  ));
            CastTo(m_plistPaneHistory,(Pane*)pns->FindMember("historyListPane"      ));

			// mdvalley: I hate C3867.
            AddEventTarget(&SquadsScreen::HistoryDialogPopup::OnButtonOK, m_pbuttonOK->GetEventSource());
        }

        void SetHistory(TList<ZString> * pList)
        {
            m_plistPaneHistory->GetStringList()->SetEmpty();

            TList<ZString>::Iterator it(*pList);

            while(!it.End())
            {
               m_plistPaneHistory->GetStringList()->AddItem(it.Value());
               it.Next();
            }
        }


        ~HistoryDialogPopup()
        {
        }

        //
        // IPopup methods
        //

        Pane* GetPane()
        {
            return m_ppane;
        }

/*        virtual void SetContainer(IPopupContainer* pcontainer)
        {
//            GetWindow()->SetFocus(m_peditPane);

            IPopup::SetContainer(pcontainer);
        }
*/
        bool OnButtonOK()
        {
            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }

            return true;
        }

    };

    TRef<HistoryDialogPopup> m_pHistoryDialog;

    //
    // Other data
    //
    static TRef<IZonePlayer>      s_pActivePlayer; // points to the squad player that the interactive user controls

public:

    ~SquadsScreen()
    {
        OnLeavingScreen();
        
        g_pSquadScreen = NULL;

        m_peventSquads->RemoveSink(m_psinkSquads);

        m_peventPlayers->RemoveSink(m_psinkPlayers);

//        m_peventPlayersDoubleClick->RemoveSink(m_psinkPlayersDoubleClick);

        m_listSquads[FILTER_ALL].SetEmpty();
        m_listSquads[FILTER_MY].SetEmpty();
        m_listSquads[FILTER_DUDEX].SetEmpty();
        m_listPlayers.SetEmpty();

        m_plistPaneSquads->SetList(new EmptyList); // remove to avoid cyclic destruction
        m_plistPanePlayers->SetList(new EmptyList); // remove to avoid cyclic destruction

        m_peventNameEdit->RemoveSink(m_psinkNameEdit);
        m_peventDescEdit->RemoveSink(m_psinkDescEdit);
        m_peventURLEdit->RemoveSink(m_psinkURLEdit);
        m_peventFind->RemoveSink(m_psinkFind);

        if(m_pCivIds)
            delete[] m_pCivIds;
    }


    SquadsScreen(Modeler* pmodeler, const char * szNameDudeX, int idPlayerDudeX, const char * szSquad):
      m_nNextCreationID(0),
      m_nCurrentRanking(1),
      m_psquadBeingMade(NULL),
      m_psquadPrePage(NULL),
      m_pmsgBox(NULL),
      m_pCivIds(NULL),
      m_bLeftScreen(false),
      m_bSelectionNotFromSquadClick(false),
      m_filter(FILTER_ALL),
      m_bSelectorNeedsAutoSet(false),
      m_bSquadEditNameReset(false),
      m_bSquadEditDescReset(false),
      m_bSquadEditURLReset(false)
    {
        m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();

        s_column = SSC_SCORE;
        s_pSelectedSquad = NULL;
        s_pSelectedPlayer = NULL;
        s_pDudeXPlayer = NULL;

        int i;

        for (i = 0; i < SSC_MAX; ++i)
        {
            m_nHighestRanking[i] = 999999; // a smaller number is actually higher
            m_nLowestRanking[i] = -1;
            m_psquadHighestRanking[i] = NULL;
            m_psquadLowestRanking[i] = NULL;
        }

        TRef<IZonePlayer> pplayer = FindPlayerByID(trekClient.GetZoneClubID());
        if (pplayer == NULL)
          pplayer = CreateZonePlayer(trekClient.GetNameLogonClubServer(), trekClient.GetZoneClubID());
        s_pActivePlayer = pplayer;

        // default to "all" tab being checked
        m_nTab = FILTER_ALL;

        // if dude x is the useractive user, then we simply use the my tab instead of dudex
        if(s_pActivePlayer->GetID() == idPlayerDudeX)
        {
           m_nTab = FILTER_MY;
           m_strSquadToBeSelected = szSquad ? szSquad : "";
           szNameDudeX = NULL;
        }
        else
        {
          if(szNameDudeX)
          {
              m_nTab = FILTER_DUDEX;
              m_strSquadToBeSelected = szSquad ? szSquad : "";
              TRef<IZonePlayer> pplayer = FindPlayerByID(idPlayerDudeX);
              if (pplayer == NULL)
                pplayer = CreateZonePlayer(szNameDudeX, idPlayerDudeX);
              s_pDudeXPlayer = pplayer;
          }
          else
              s_pDudeXPlayer = NULL;
        }

        //
        // SquadScreen exports
        //
        TRef<INameSpace> pnsData = GetModeler()->CreateNameSpace("squadsscreendata");

        pnsData->AddMember("squadTabAllStr",   (Value*)(m_pstringSquadTabAll = new ModifiableString("All Squads")));
        pnsData->AddMember("squadTabMyStr",    (Value*)(m_pstringSquadTabMy = new ModifiableString("My Squads")));

        if (s_pDudeXPlayer)
          m_pstringSquadTabDudeX = new ModifiableString(s_pDudeXPlayer->GetName());
        else
          m_pstringSquadTabDudeX = new ModifiableString("");

        pnsData->AddMember("squadTabDudeXStr", (Value*)m_pstringSquadTabDudeX);

        pnsData->AddMember("descriptionStr",(Value*)(m_pstringDescription = new ModifiableString("")));

        //
        // SquadScreen Imports
        //
        TRef<INameSpace> pns = pmodeler->GetNameSpace("squadsscreen");
        CastTo(m_ppane, pns->FindMember("screen"));

        //
        // Normal buttons
        // 
        CastTo(m_pbuttonZoneClub, pns->FindMember("zoneclubButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonZoneClub, m_pbuttonZoneClub->GetEventSource());

        CastTo(m_pbuttonPlayerProfile, pns->FindMember("playerProfileButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonPlayerProfile, m_pbuttonPlayerProfile->GetEventSource());

        CastTo(m_pbuttonLeaderboard, pns->FindMember("leaderboardButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonLeaderboard, m_pbuttonLeaderboard->GetEventSource());

        CastTo(m_pbuttonPrev, pns->FindMember("prevButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonPrev, m_pbuttonPrev->GetEventSource());

        CastTo(m_pbuttonNext, pns->FindMember("nextButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonNext, m_pbuttonNext->GetEventSource());

        CastTo(m_pbuttonTop, pns->FindMember("topButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonTop, m_pbuttonTop->GetEventSource());

        CastTo(m_pbuttonEditSquad, pns->FindMember("editsquadButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonEditSquad, m_pbuttonEditSquad->GetEventSource());

        CastTo(m_pbuttonReject, pns->FindMember("rejectButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonReject, m_pbuttonReject->GetEventSource());

        CastTo(m_pbuttonAccept, pns->FindMember("acceptButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonAccept, m_pbuttonAccept->GetEventSource());

        CastTo(m_pbuttonCommission, pns->FindMember("commissionButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonCommission, m_pbuttonCommission->GetEventSource());

        CastTo(m_pbuttonDemote, pns->FindMember("demoteButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonDemote, m_pbuttonDemote->GetEventSource());

        CastTo(m_pbuttonTransferControl, pns->FindMember("transfercontrolButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonTransferControl, m_pbuttonTransferControl->GetEventSource());

        CastTo(m_pbuttonRequestJoin, pns->FindMember("requestjoinButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonRequestJoin, m_pbuttonRequestJoin->GetEventSource());

        CastTo(m_pbuttonCancelRequest, pns->FindMember("cancelrequestButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonCancelRequest, m_pbuttonCancelRequest->GetEventSource());

        CastTo(m_pbuttonQuitSquad, pns->FindMember("quitsquadButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonQuitSquad, m_pbuttonQuitSquad->GetEventSource());

        CastTo(m_pbuttonCreateSquad, pns->FindMember("createsquadButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonCreateSquad, m_pbuttonCreateSquad->GetEventSource());
        m_pbuttonCreateSquad->SetEnabled(true);

        CastTo(m_pbuttonLog, pns->FindMember("logButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonLog, m_pbuttonLog->GetEventSource());
        m_pbuttonLog->SetEnabled(false);

        CastTo(m_pbuttonApply, pns->FindMember("applyButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonApply, m_pbuttonApply->GetEventSource());
        m_pbuttonApply->SetEnabled(false);
		    m_pbuttonApply->SetHidden(true);

        CastTo(m_pbuttonURL, pns->FindMember("URLButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonJumpURL, m_pbuttonURL->GetEventSource());

        CastTo(m_pbuttonFindSquad, pns->FindMember("squadFindButtonPane"));
        AddEventTarget(&SquadsScreen::OnButtonFindSquad, m_pbuttonFindSquad->GetEventSource());

        //
        // Combo panes
        //
        CastTo(m_pcomboCiv, pns->FindMember("civComboPane"));
        m_pcomboCiv->SetEnabled(false);

        int cNames = FillCombo(pns, m_pcomboCiv, "CivNames");
        m_cCivs = cNames;
        m_pCivIds = new int[cNames];
        int cInts = FillIntegerArray(pns, m_pCivIds, "CivIDs");
        assert(cInts == cNames);

        //
        // Icons
        // 
        CastTo(m_pimageOwner,    (Value*) pns->FindMember("ownerImage"   ));
        CastTo(m_pimageASL,      (Value*) pns->FindMember("ASLImage"     ));
        CastTo(m_pimagePending,  (Value*) pns->FindMember("PendingImage" ));
        CastTo(m_pimageRejected, (Value*) pns->FindMember("RejectedImage"));
        CastTo(m_pimageMember,   (Value*) pns->FindMember("MemberImage"  ));
        CastTo(m_pimageBooted,   (Value*) pns->FindMember("BootedImage"  ));
        CastTo(m_pimageQuit,     (Value*) pns->FindMember("QuitImage"    ));

        //
        // Tab
        //
        TRef<IObject> pobjColumns;
        TRef<ColorValue> pColor;

        CastTo(m_ptabSquads,             pns->FindMember("squadTabPane"           ));
        CastTo(pobjColumns,              pns->FindMember("tabColumns"             ));

        CastTo(pColor,                   pns->FindMember("squadTabHeaderTextColor"));
        m_colorTabText =                 pColor->GetValue();

        CastTo(pColor,                   pns->FindMember("squadSelectedTabHeaderTextColor"));
        m_colorTabTextSelected =         pColor->GetValue();

        ParseIntVector(pobjColumns, m_viTabColumns);
		// mdvalley: needs pointer and class
        AddEventTarget(&SquadsScreen::_OnButtonBarTabs, m_ptabSquads->GetEventSource());

        m_ptabSquads->SetHidden(FILTER_DUDEX, true);

//        m_ptabSquads->SetChecked(m_nTab,  true);
//        m_ptabSquads->SetChecked2(m_nTab, false);

        //
        // Squads listbox
        //
        CastTo(m_plistPaneSquads, (Pane*)pns->FindMember("squadsListPane"     ));
        CastTo(m_pbuttonbarSquadsHeader, pns->FindMember("squadsListHeader"   ));
        CastTo(pobjColumns,              pns->FindMember("squadsColumns"      ));
        CastTo(pColor,                   pns->FindMember("squadSelectColor"   ));
        m_colorSquadSelectionBar =       pColor->GetValue();

        AddEventTarget(&SquadsScreen::_OnButtonBarSquads, m_pbuttonbarSquadsHeader->GetEventSource());

        ParseIntVector(pobjColumns, m_viColumns);
        m_peventSquads = m_plistPaneSquads->GetSelectionEventSource();
        m_peventSquads->AddSink(m_psinkSquads = new IItemEvent::Delegate(this));
        m_plistPaneSquads->SetItemPainter(new SquadsItemPainter(m_viColumns, this));

        m_pscrollPaneSquads = m_plistPaneSquads->GetScrollPane();
        m_pscrollPaneSquads->SetHidden(true);

        DisableAllOwnerButtons();
        DisableAllRegularButtons();

        //
        // Squad Details Area
        //
        CastTo(m_peditPaneSquadName, (Pane*)pns->FindMember("squadNameEditPane"));
        m_peditPaneSquadName->SetMaxLength(c_cbNameDB);
        AddEventTarget(&SquadsScreen::OnSquadNameClick, m_peditPaneSquadName->GetClickEvent());

        m_peventNameEdit = m_peditPaneSquadName->GetChangeEvent();
        m_peventNameEdit->AddSink(m_psinkNameEdit = new TEvent<ZString>::Delegate(this));

        CastTo(m_peditPaneSquadDescription, (Pane*)pns->FindMember("squadDescriptionEditPane"));
        m_peditPaneSquadDescription->SetMaxLength(c_cbSquadDescriptionDB); 
        AddEventTarget(&SquadsScreen::OnSquadDescriptionClick, m_peditPaneSquadDescription->GetClickEvent());

        m_peventDescEdit = m_peditPaneSquadDescription->GetChangeEvent();
        m_peventDescEdit->AddSink(m_psinkDescEdit = new TEvent<ZString>::Delegate(this));

        CastTo(m_peditPaneSquadURL, (Pane*)pns->FindMember("squadURLEditPane"));
        m_peditPaneSquadURL->SetMaxLength(c_cbURLDB);
        AddEventTarget(&SquadsScreen::OnSquadURLClick, m_peditPaneSquadURL->GetClickEvent());

        m_peventURLEdit = m_peditPaneSquadURL->GetChangeEvent();
        m_peventURLEdit->AddSink(m_psinkURLEdit = new TEvent<ZString>::Delegate(this));

        CastTo(m_peditPaneFind, (Pane*)pns->FindMember("squadFindPane"));
        m_peditPaneFind->SetMaxLength(c_cbNameDB);
        AddEventTarget(&SquadsScreen::OnFindClick, m_peditPaneFind->GetClickEvent());

        m_peventFind = m_peditPaneFind->GetChangeEvent();
        m_peventFind->AddSink(m_psinkFind = new TEvent<ZString>::Delegate(this));

        m_peditPaneSquadName->SetReadOnly();
        m_peditPaneSquadDescription->SetHidden(true);
        m_peditPaneSquadURL->SetReadOnly();

        CastTo(m_pstrpaneRanking,       (Pane*) pns->FindMember("rankingStringPane"));
        CastTo(m_pstrpaneRank,          (Pane*) pns->FindMember("rankStringPane"));
        CastTo(m_pstrpaneWinsAndLosses, (Pane*) pns->FindMember("winsAndLossesStringPane"));
        CastTo(m_pstrpaneInceptionDate, (Pane*) pns->FindMember("inceptionDateStringPane"));
                                         
        //
        // Players listbox
        //
        CastTo(m_plistPanePlayers, (Pane*)pns->FindMember("playersListPane"     ));
        CastTo(m_pbuttonbarPlayersHeader, pns->FindMember("playersListHeader"   ));
        CastTo(pobjColumns,               pns->FindMember("playersColumns"      ));
        CastTo(pColor,                    pns->FindMember("playerSelectColor"   ));
        m_colorPlayerSelectionBar =       pColor->GetValue();

        AddEventTarget(&SquadsScreen::OnButtonBarPlayers, m_pbuttonbarPlayersHeader->GetEventSource());

        ParseIntVector(pobjColumns, m_viPlayersColumns);
        m_peventPlayers = m_plistPanePlayers->GetSelectionEventSource();
        m_peventPlayers->AddSink(m_psinkPlayers = new IItemEvent::Delegate(this));
        m_plistPanePlayers->SetItemPainter(new PlayersItemPainter(m_viPlayersColumns, this));

        AddEventTarget(&SquadsScreen::OnDoubleClickPlayer, m_plistPanePlayers->GetDoubleClickEventSource());

//        m_peventPlayersDoubleClick = m_plistPanePlayers->GetDoubleClickEventSource();
//        m_peventPlayersDoubleClick->AddSink(m_psinkPlayersDoubleClick = new IItemEvent::Delegate(this));

        //
        // History Dialog
        //
        m_pHistoryDialog = new HistoryDialogPopup(pns, this);

        if (s_pDudeXPlayer)
        {
            m_pstringSquadTabDudeX->SetValue(s_pDudeXPlayer->GetName());
            m_ptabSquads->SetHidden(FILTER_DUDEX, false);
        }

        //
        // Unload namespace
        //
        
        pmodeler->UnloadNameSpace(pns);
        //pmodeler->UnloadNameSpace(pnsData);

        // check column with the default sort
        m_pbuttonbarSquadsHeader->SetChecked(s_column,  true);
        m_pbuttonbarSquadsHeader->SetChecked2(s_column, false);

        m_pbuttonbarPlayersHeader->SetChecked(2,  true); // 2 == status
        m_pbuttonbarPlayersHeader->SetChecked2(2, false);

        m_vuPlayersSorts.PushEnd(1); // 1 == rank;   player rank is the default secondary sort
        m_vuPlayersSorts.PushEnd(2); // 2 == status; player status is the default primary sort

        // Request a starting page of squads
        int nTemp = m_nTab;
        m_nTab = -1; // force switch
        ZString temp(m_strSquadToBeSelected);
        OnButtonBarTabs(nTemp);
        m_strSquadToBeSelected = temp; // quick fix hack

        //
        //  Temporary, throw some squads in there
        //

/*
        for (int i = 0; i < 30; ++i)
        {
          TRef<IZoneSquad> psquad = CreateZoneSquad();

          psquad->SetName("Marco's Squad");
          psquad->SetDescription("Right mix of Pro's and Newbies");
          psquad->SetURL("home.sprintmail.com/~mcurrie");
          psquad->SetInceptionDate("12/11/99");
          psquad->SetWins(55-i);
          psquad->SetLosses(3+i);
          psquad->SetRanking((i+1)*2-1);
          psquad->SetRank(IZoneSquad::Rank((i+1)%3+1));
          psquad->SetCivID((i+2)%4-1);
          psquad->SetScore((i+2)%4-1);

          psquad->AppendOwnershipLog("Jack Smith", "1/1/98");
          psquad->AppendOwnershipLog("Tom Daly", "3/19/98");

          int j;
          TRef<IZonePlayer> pplayer;

          for (j = 0; j < 35 - i; ++j)
          {
            pplayer = CreateZonePlayer();
            pplayer->Randomize();
            psquad->AddPlayer(*pplayer, IZonePlayer::DSTAT_MEMBER);
          }
          psquad->SetOwner(pplayer);

          AddSquadToScreen(psquad);

          psquad = CreateZoneSquad();

          psquad->SetName("Marco's Squad 2");
          psquad->SetDescription("Come meet your doom.");
          psquad->SetURL("www.dkeep.com/marco");
          psquad->SetInceptionDate("12/12/99");
          psquad->SetWins(50-i);
          psquad->SetLosses(7+i);
          psquad->SetRanking((i+1)*2);
          psquad->SetRank(IZoneSquad::Rank(i%3+1));
          psquad->SetCivID(i%4-1);
          psquad->SetScore(i%4-1);
          psquad->AppendOwnershipLog("Mary Wilson", "8/10/99");
          psquad->AppendOwnershipLog("Steve Smith", "10/11/99");
          psquad->AppendOwnershipLog("Jack Reed", "11/12/99");

          for (j = 0; j < 32 - i; ++j)
          {
            pplayer = CreateZonePlayer();
            pplayer->Randomize();
            psquad->AddPlayer(*pplayer, IZonePlayer::DSTAT_MEMBER);
          }
          psquad->SetOwner(pplayer);

          s_pActivePlayer = pplayer; // make the last player

          AddSquadToScreen(psquad);
        }
*/        
    }

    void OnLeavingScreen()
    {
        // ensure called only once
        if (m_bLeftScreen)
            return;
        m_bLeftScreen = true;

        if(m_pkeyboardInputOldFocus)
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
    }


    bool IsRankingOnScreen(int nRanking)
    {
        if (nRanking == -1)
            return false;

        if (m_nCurrentRanking <= 1+c_cSquadsShown/2)
            return nRanking <= c_cSquadsShown;

        return nRanking >= m_nCurrentRanking - c_cSquadsShown / 2 && 
               nRanking <= m_nCurrentRanking + c_cSquadsShown / 2;
    }

/*
    int FindRanking_ForMiddleSquad()
    {
        XSquadsIt it(m_listSquads[FILTER_ALL]);

        int nLowest = 99999999;
        int nHighest = -1;

        while(!it.End())
        {
          int nRank = it.Value()->GetRanking(s_column);

          if (nRank < nLowest)
          {
              nLowest = nRank;
          }
          if (nRank > nHighest)
          {
              nHighest = nRank;
          }

          it.Next();
        }

        if (nHighest != -1 && nLowest + c_cSquadsShown / 2 <= nHighest)
          return nLowest + c_cSquadsShown / 2;
        else
          return -1;
    }
*/

    /*-------------------------------------------------------------------------
     * FindSquadByID()
     *-------------------------------------------------------------------------
     * Returns:
     *    A squad given its id.
     */
    TRef<IZoneSquad> FindSquadByID(int nID)
    {
        XSquadsIt it(m_listAllSquads);

        while(!it.End())
        {
          if (it.Value()->GetID() == nID)
          {
              return it.Value();
          }
          it.Next();
        }
        return NULL;
    }

    /*-------------------------------------------------------------------------
     * FindSquadByCreationID()
     *-------------------------------------------------------------------------
     * Returns:
     *    A squad given its creation id.
     */
    TRef<IZoneSquad> FindSquadByCreationID(int nID)
    {
        XSquadsIt it(m_listAllSquads);

        while(!it.End())
        {
          if (it.Value()->GetCreationID() == nID)
          {
              return it.Value();
          }
          it.Next();
        }
        return NULL;
    }

    /*-------------------------------------------------------------------------
     * FindSquadByRanking()
     *-------------------------------------------------------------------------
     * Returns:
     *    A squad given its ranking.  
     *
     * Remarks:
     *    Rankings are unique at a given snap shot of time.
     */
    TRef<IZoneSquad> FindSquadByRanking(SQUAD_SORT_COLUMN column, int nRanking)
    {
        XSquadsIt it(m_listAllSquads);

        while(!it.End())
        {
          if (it.Value()->GetRanking(column) == nRanking)
          {
              return it.Value();
          }
          it.Next();
        }
        return NULL;
    }


    /*-------------------------------------------------------------------------
     * FindPlayerByID()
     *-------------------------------------------------------------------------
     * Returns:
     *    A member given his id.
     */
    TRef<IZonePlayer> FindPlayerByID(int nID)
    {
        XZonePlayersIt it(m_listAllPlayers);

        while(!it.End())
        {
          if (it.Value()->GetID() == nID)
          {
              return it.Value();
          }
          it.Next();
        }
        return NULL;
    }


    /*-------------------------------------------------------------------------
     * CreateZoneSquad()
     *-------------------------------------------------------------------------
     * Returns:
     *    A new squad, after putting in mother list.
     */
    TRef<IZoneSquad> CreateZoneSquad()
    {
        TRef<IZoneSquad> psquad = ::CreateZoneSquad();
        m_listAllSquads.PushEnd(psquad);
        return psquad;
    }

    TRef<IZonePlayer> CreateZonePlayer(const char *szName, int nID)
    {
        TRef<IZonePlayer> pPlayer = ::CreateZonePlayer(szName, nID);
        m_listAllPlayers.PushEnd(pPlayer);
        return pPlayer;
    }



    /*-------------------------------------------------------------------------
     * SEND_FOR_ACTIVE_MEMBER()
     *-------------------------------------------------------------------------
     *
     * Purpose:   Send a message that has just one paramter: nSquadID
     *
     *            The member is question is assumed to be the Interactive member.
     *
     *            This applies to the following messages:
     *
     *            SQUAD_QUIT, SQUAD_CANCEL_JOIN_REQUEST, SQUAD_MAKE_JOIN_REQUEST
     *            SQUAD_DETAILS_REQ,
     *-------------------------------------------------------------------------
     */
    #define SEND_FOR_ACTIVE_MEMBER(MessageType) \
    { \
        debugf("Sending %s; squad %d (%s)\n", #MessageType, s_pSelectedSquad->GetID(), s_pSelectedSquad->GetName()); \
        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmSquadMessage, C, MessageType) \
        END_PFM_CREATE \
        pfmSquadMessage->nSquadID = s_pSelectedSquad->GetID(); \
        trekClient.SendClubMessages(); \
    }

    /*-------------------------------------------------------------------------
     * SEND_FOR_SELECTED_MEMBER()
     *-------------------------------------------------------------------------
     *
     * Purpose:   Send a message that has just two paramters: nSquadID, nMemberID
     *
     *            This applies to the following messages:
     *
     *            SQUAD_REJECT_JOIN_REQUEST, SQUAD_ACCEPT_JOIN_REQUEST, 
     *            SQUAD_BOOT_MEMBER, SQUAD_PROMOTE_TO_ASL, SQUAD_DEMOTE_TO_MEMBER,
     *            SQUAD_TRANSFER_CONTROL
     *-------------------------------------------------------------------------
     */
    #define SEND_FOR_SELECTED_MEMBER(MessageType) \
    { \
        debugf("Sending %s; member %d (%s)\n; squad %d (%s)", #MessageType, s_pSelectedPlayer->GetID(), s_pSelectedPlayer->GetName(), s_pSelectedSquad->GetID(), s_pSelectedSquad->GetName()); \
        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmSquadMessage, C, MessageType) \
        END_PFM_CREATE \
        pfmSquadMessage->nMemberID = s_pSelectedPlayer->GetID(); \
        pfmSquadMessage->nSquadID  = s_pSelectedSquad->GetID(); \
        trekClient.SendClubMessages(); \
    }

    void ResetHighLowRanges()
    {
        //
        // reset highest/lowest
        //
        m_nHighestRanking[s_column] = 999999;
        m_nLowestRanking[s_column] = -1;
        m_psquadHighestRanking[s_column] = NULL;
        m_psquadLowestRanking[s_column] = NULL;

        TRef<IZoneSquad> psquad = NULL;

        if (m_nCurrentRanking != -1)
            psquad = FindSquadByRanking(s_column, m_nCurrentRanking);

        if (psquad)
        {
            UpdateHighLowRanges(psquad, m_nCurrentRanking, s_column);
        }
    }


    /*-------------------------------------------------------------------------
     * RequestPage()
     *-------------------------------------------------------------------------
     *
     * Purpose:     Request a list of squads
     *
     * Parameters:  psquad: the basis squad used to get info
     *-------------------------------------------------------------------------
     */
    void RequestPage(TRef<IZoneSquad> psquad, bool bNewColumn = false, bool bSwitchingToAllTab = false)
    {
        m_psquadPrePage = psquad;

        // Ask server for data
        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmNextPage, C, SQUAD_NEXT_PAGE_ALL)
        END_PFM_CREATE

        //  update s_column as needed
        if (bNewColumn || bSwitchingToAllTab)
        {
            if (psquad && psquad->GetRanking(s_column) != -1)
            {
                m_nCurrentRanking = psquad->GetRanking(s_column);
                // reset highest/lowest
                m_nHighestRanking[s_column] = psquad->GetRanking(s_column);
                m_nLowestRanking[s_column] = psquad->GetRanking(s_column);
                m_psquadHighestRanking[s_column] = psquad;
                m_psquadLowestRanking[s_column] = psquad;
            }
            else 
            {
                m_nCurrentRanking = -1; // we always set this to -1 when changing columns
                ResetHighLowRanges();
            }
        }

        pfmNextPage->column = s_column;

        if(psquad)
        {
            pfmNextPage->nSquadID = psquad->GetID();
        }
        else
        {
            m_nCurrentRanking = 1+c_cSquadsShown/2; // place in middle of screen
            pfmNextPage->nSquadID = -1;
        }
 
        debugf("Sending SQUAD_NEXT_PAGE_ALL; squad %d (%s)\n", pfmNextPage->nSquadID, FindSquadByID(pfmNextPage->nSquadID) ? FindSquadByID(pfmNextPage->nSquadID)->GetName() : "TOP"); 
        trekClient.SendClubMessages();

        DisplaySquadsInRange();
    }

    /*-------------------------------------------------------------------------
     * DisplaySquadsInRange()
     *-------------------------------------------------------------------------
     *
     * Purpose:     Look in cache and display the appropriate squads.
     *
     *-------------------------------------------------------------------------
     */
    void DisplaySquadsInRange(bool bShowEvenIfNotFull = false)
    {                        
        assert(m_nTab == FILTER_ALL);

        // clear screen
        m_listSquads[FILTER_ALL].SetEmpty(); // 0 == ALL squads tab
        m_bSelectorNeedsAutoSet = true;

        if(m_nCurrentRanking==-1)
            return;

        if(m_nCurrentRanking < 1/*c_cSquadsShown / 2*/)
            m_nCurrentRanking = 1+c_cSquadsShown / 2;

        if (!bShowEvenIfNotFull)
        {
            //
            // First see is cache has c_cSquadsShown number of squads that we can display
            // right now, if not then don't display any of them (due to visional glitches
            // when paging up).
            // 
            int nCount = 0;

            XSquadsIt it(m_listAllSquads);

            while(!it.End())
            {
              TRef<IZoneSquad> psquad = it.Value();

              if (IsRankingOnScreen(psquad->GetRanking(s_column)))
              {
                  nCount++;
              }
              it.Next();
            }

            if (nCount < c_cSquadsShown)  // if cache isn't complete so quit now
                return;
        }

        //
        // Since we know we have enough to show, let's show them
        //
        debugf("Ranking At Top: %d\n", m_nCurrentRanking);

        //
        //  Let's see if we already have some of the squads we need, if so add it now
        //
        XSquadsIt it(m_listAllSquads);

        while(!it.End())
        {
          TRef<IZoneSquad> psquad = it.Value();
                         
          int ranking = psquad->GetRanking(s_column);
          if (IsRankingOnScreen(ranking))
          {
              AddSquadToScreen(psquad, false);

              if ((psquad->GetRanking(s_column) == m_nCurrentRanking) || 
                  (m_strSquadToBeSelected.GetLength() > 0 && m_strSquadToBeSelected == psquad->GetName()))
              {
                 SelectSquad(*psquad);
              }

             //
             // update highest and lowest; this is also done in when decoding a FM_S_SQUAD_INFO message
             //
             UpdateHighLowRanges(psquad, ranking, s_column);
          }
          it.Next();
        }

        RefreshScreen();
    }

    /*-------------------------------------------------------------------------
     * DisplaySquadsInRangeForDudeX()
     *-------------------------------------------------------------------------
     *
     * Purpose:     Look in cache and display the appropriate squads.
     *
     *-------------------------------------------------------------------------
     */
    void DisplaySquadsInRangeForDudeX(int nTab, TRef<IZonePlayer> pplayer)
    {
        m_listSquads[nTab].SetEmpty();
        m_bSelectorNeedsAutoSet = true;

        XSquadsIt it(m_listAllSquads);

        while(!it.End())
        {
          TRef<IZoneSquad> psquad = it.Value();

          if(pplayer->GetStatus(*psquad) != IZonePlayer::DSTAT_NO_ASSOCIATION)
          {
              m_listSquads[nTab].PushEnd(psquad);

              if (m_strSquadToBeSelected.GetLength() > 0 && m_strSquadToBeSelected == psquad->GetName())
              {
                  m_strSquadToBeSelected = "";
                  SelectSquad(*psquad);
              }
          }

          it.Next();
        }
        RefreshScreen();
    }

    /*-------------------------------------------------------------------------
     * UpdateHighLowRanges()
     *-------------------------------------------------------------------------
     *
     * Purpose:   These ranges help us decide which psquad to use as a basis
     *            in our query for most squad info messages.
     *
     *-------------------------------------------------------------------------
     */
     void UpdateHighLowRanges(TRef<IZoneSquad> psquad, int nRanking, int column)
     {
         assert(nRanking != -1);

         if(nRanking < m_nHighestRanking[column])
         {
             m_nHighestRanking[column] = nRanking;
             m_psquadHighestRanking[column] = psquad;
         }

         if(nRanking > m_nLowestRanking[column])
         {
            m_nLowestRanking[column] = nRanking;
            m_psquadLowestRanking[column] = psquad;
         }
     }


    /*-------------------------------------------------------------------------
     * ForwardSquadMessage()
     *-------------------------------------------------------------------------
     *
     * Purpose:   Process server generated message for squad screen.
     *
     *-------------------------------------------------------------------------
     */
    void ForwardSquadMessage(FEDMESSAGE *pSquadMessage)
    {
        switch (pSquadMessage->fmid) 
        {
            case FM_S_SQUAD_INFO:
            {
                CASTPFM(pfmInfo, S, SQUAD_INFO, pSquadMessage);

                SQUAD_SORT_COLUMN column = (SQUAD_SORT_COLUMN)pfmInfo->column;

                TRef<IZoneSquad> psquad = NULL;

                // ensure rankings are unique; find any existing squads with this rank and reset it
                psquad = FindSquadByRanking(column, pfmInfo->nRanking);

                if(psquad && psquad->GetID() != pfmInfo->nSquadID)
                {
                    // check for Ranking discontinuity.  That happens if ranking has changed
                    // and the cache is out-of-date.
                    debugf("Ranking discontinuity detected in column %d, Ranking of %d\n", column, pfmInfo->nRanking);

                    ResetHighLowRanges();

                    psquad->SetRanking(column, -1);
                    // remove from screen
                    if(m_listSquads[FILTER_ALL].Remove(psquad))
                        RefreshScreen();
                    psquad = NULL;
                }

                if (psquad == NULL)
                    psquad = FindSquadByID(pfmInfo->nSquadID); // see if we already know about the squad sent from club server

                if (psquad == NULL)
                {
                    psquad = CreateZoneSquad();
                    psquad->SetID(pfmInfo->nSquadID);
                }

                char *szName = FM_VAR_REF(pfmInfo, szName);

                if(szName == NULL)
                  szName = "";

                psquad->SetName(szName);

                psquad->SetRanking(column, pfmInfo->nRanking);
                psquad->SetWins(pfmInfo->cWins);
                psquad->SetLosses(pfmInfo->cLosses);
                psquad->SetScore(pfmInfo->nScore);

                //
                // update highest and lowest; this is also done in DisplaySquadsInRange()
                //
                UpdateHighLowRanges(psquad, pfmInfo->nRanking, column);

                // 
                // Add squad to screen as needed
                //
                if (column != s_column || m_nTab != FILTER_ALL)
                    break;

                if (pfmInfo->bSearchResult)
                {
                    unsigned nSearchMatch = CompareStrings(szName, PCC(m_peditPaneFind->GetString()), m_peditPaneFind->GetString().GetLength());

                    if (m_nCurrentRanking == -1)
                        m_nCurrentRanking = pfmInfo->nRanking;

                    if (nSearchMatch < m_nBestSearchMatch)
                    {
                        m_nCurrentRanking = pfmInfo->nRanking;
                        m_nBestSearchMatch = nSearchMatch;
                        DisplaySquadsInRange(true);

                        // shouldn't happen, but let's be safe
                        if(!m_listSquads[FILTER_ALL].Find(psquad))
                        {
                            AddSquadToScreen(psquad);
                            debugf("Warning, Forcing squad to be shown (after search): %s\n", psquad->GetName());
                        }
                        SelectSquad(*psquad);
                    }
                    else
                    // if in range, show it
                    if (IsRankingOnScreen(pfmInfo->nRanking))
                        AddSquadToScreen(psquad, true);
                }
                else
                if (m_nCurrentRanking == -1)
                {
                    // after changing column sorts, make sure the same squad is selected
                    if (m_psquadPrePage == psquad)  
                    {
                        m_nCurrentRanking = std::max(1, psquad->GetRanking(s_column));
                        DisplaySquadsInRange();

                        // It's rare, but this can happen if user request lots of pages and switching between columns frequently
                        if(!m_listSquads[FILTER_ALL].Find(psquad))
                        {
                            debugf("Warning, pre-column change squad is not on screen when it should, resending Request for Page: %s\n", psquad->GetName());
                            ResetHighLowRanges();
                            m_nCurrentRanking = psquad->GetRanking(s_column);
                            RequestPage(psquad);
                        }
                        else
                        SelectSquad(*psquad);
                    }
                    else
                        RefreshScreen();
                }
                else
                {
                    // if in range, show it
                    if (IsRankingOnScreen(pfmInfo->nRanking))
                    {
                        AddSquadToScreen(psquad, false);

                        if ((pfmInfo->nRanking == m_nCurrentRanking && m_bSelectorNeedsAutoSet) || 
                            (m_strSquadToBeSelected.GetLength() > 0 && m_strSquadToBeSelected == psquad->GetName()))
                        {
                            SelectSquad(*psquad);
                            m_bSelectorNeedsAutoSet = false;
                        }
                    }

                    RefreshScreen();
                }
            }
            break;

            case FM_S_SQUAD_INFO_DUDEX:
            {
                CASTPFM(pfmInfo, S, SQUAD_INFO_DUDEX, pSquadMessage);

                TRef<IZoneSquad> psquad = FindSquadByID(pfmInfo->nSquadID); // see if we already know about the squad sent from club server

                if (psquad == NULL)
                {
                    psquad = CreateZoneSquad();
                    psquad->SetID(pfmInfo->nSquadID);
                }

                char *szName = FM_VAR_REF(pfmInfo, szName);

                if(szName)
                    psquad->SetName(szName);

                psquad->SetWins(pfmInfo->cWins);
                psquad->SetLosses(pfmInfo->cLosses);
                psquad->SetScore(pfmInfo->nScore);

                if (pfmInfo->bMyTab)
                {
                    AddSquadToScreen(psquad, true, FILTER_MY);
                    if (s_pActivePlayer->GetStatus(*psquad) == IZonePlayer::DSTAT_NO_ASSOCIATION) // if not already on team
                        psquad->SetStatus(*s_pActivePlayer, IZonePlayer::DSTAT_UNKNOWN); // assume member until told otherwise
                }
                else
                {
                    AddSquadToScreen(psquad, true, FILTER_DUDEX);
                    if(s_pDudeXPlayer && s_pDudeXPlayer->GetStatus(*psquad) == IZonePlayer::DSTAT_NO_ASSOCIATION) // if not already on team
                        psquad->SetStatus(*s_pDudeXPlayer, IZonePlayer::DSTAT_UNKNOWN); // assume member until told otherwise
                }

                if (m_strSquadToBeSelected.GetLength() > 0 && m_strSquadToBeSelected == psquad->GetName())
                {
                    SelectSquad(*psquad);
                    m_strSquadToBeSelected = "";
                }
            }
            break;

            case FM_S_SQUAD_DETAILS:
            {
                CASTPFM(pfmDetails, S, SQUAD_DETAILS, pSquadMessage);

                m_psquadLastDetails = FindSquadByID(pfmDetails->nSquadID);

                if (m_psquadLastDetails)
                {
                    m_psquadLastDetails->SetEmpty(); // clear membership cache, cause update is coming
                    if (s_pSelectedSquad == m_psquadLastDetails)
                        m_listPlayers.SetEmpty(); // clear list of players on screen
                    m_psquadLastDetails->SetDetailsReceived();

                    char * szDesc =             FM_VAR_REF(pfmDetails, szDescription);
                    char * szURL =              FM_VAR_REF(pfmDetails, szURL);
                    char * szInceptionDate =    FM_VAR_REF(pfmDetails, szInceptionDate);

                    if(szDesc)
                        m_psquadLastDetails->SetDescription(szDesc);
                    else
                        m_psquadLastDetails->SetDescription("");
                    if(szURL)
                        m_psquadLastDetails->SetURL(szURL);
                    else
                        m_psquadLastDetails->SetURL("");

                    m_psquadLastDetails->SetCivID(pfmDetails->civid);

                    if(szInceptionDate)
                        m_psquadLastDetails->SetInceptionDate(szInceptionDate);
                    else
                        m_psquadLastDetails->SetInceptionDate("");

                    RefreshScreen();
                }
                else {assert(0);} // received details about a squad that doesn't exist
            }
            break;

            case FM_S_SQUAD_DETAILS_PLAYER:  // always follows a FM_S_SQUAD_DETAILS message (or another FM_S_SQUAD_DETAILS_PLAYER)
            {
                if (!m_psquadLastDetails)
                {
                    assert(0); // client never received a FM_S_SQUAD_DETAILS message
                    break;
                }

                CASTPFM(pfmDetails, S, SQUAD_DETAILS_PLAYER, pSquadMessage);

/*                if (!IMemberBase::IsRankValid(pfmDetails->sqRank) ||
                    !IMemberBase::IsDetailedStatusValid(pfmDetails->detailedstatus))
                {
                    assert(0); // data sent from club is invalid
                    break;
                }
*/


                TRef<IZonePlayer> pmember = FindPlayerByID(pfmDetails->nMemberID);

                char * szName =       FM_VAR_REF(pfmDetails, szName);

                if (pmember == NULL)
                {
                    pmember = CreateZonePlayer(szName ? szName : "?", pfmDetails->nMemberID);
                }

                if (szName)
                    pmember->SetName(szName);

                char * szLastPlayed = FM_VAR_REF(pfmDetails, szLastPlayed);

                // set/refresh member data

                pmember->SetRank(pfmDetails->sqRank);

                //
                // Okay, now we actually add the player to the squad, depending on
                // his detailed status.
                //
                switch (pfmDetails->detailedstatus)
                {
                    case IZonePlayer::DSTAT_PENDING:
                    {
                        m_psquadLastDetails->AddJoinRequest(*pmember);
                    }
                    break;

                    case IZonePlayer::DSTAT_MEMBER:
                    {
                        m_psquadLastDetails->AddPlayer(*pmember, IZonePlayer::DSTAT_MEMBER);
                    }
                    break;

                    case IZonePlayer::DSTAT_ASL:
                    {
                        m_psquadLastDetails->AddPlayer(*pmember, IZonePlayer::DSTAT_ASL);
                    }
                    break;

                    case IZonePlayer::DSTAT_LEADER:
                    {
                        m_psquadLastDetails->AddPlayer(*pmember, IZonePlayer::DSTAT_LEADER);
                    }
                    break;

                    default:
                        assert(0); // don't know how to add this member to specified squad
                    break;
                }

                //
                // Now that member has been added to squad, we can set his date
                //
                if (szLastPlayed)
                    pmember->SetLastPlayedDate(*m_psquadLastDetails, szLastPlayed);
                else
                    pmember->SetLastPlayedDate(*m_psquadLastDetails, "        ");

                // this keeps selected player selected (despite the fact we recently cleared m_listPlayers)
                if (pmember == s_pSelectedPlayer && 
                    s_pSelectedSquad && pmember->IsMemberOf(*s_pSelectedSquad) &&
                    s_pSelectedPlayer)
                {
                    if (!m_listPlayers.Find(pmember)) // make sure he is in list
                        m_listPlayers.PushEnd(pmember);
                    SelectPlayer(*pmember);
                }

                RefreshScreen();
            }
            break;

            case FM_S_SQUAD_LOG_INFO:
            {
                CASTPFM(pfmLogInfo, S, SQUAD_LOG_INFO, pSquadMessage);

                TRef<IZoneSquad> psquad = FindSquadByID(pfmLogInfo->nSquadID);

                if (psquad)
                {
                    psquad->SetOwnershipLog(FM_VAR_REF(pfmLogInfo, LogInfo));
                    m_pHistoryDialog->SetHistory(psquad->GetOwnershipLog());
                }
                else 
                { // received log info about a squad that doesn't exist
                    assert(0);
                }
            }
            break;

            case FM_S_SQUAD_DELETED:
            {
                CASTPFM(pfmDeletedNotice, S, SQUAD_DELETED, pSquadMessage);

                TRef<IZoneSquad> psquad = FindSquadByID(pfmDeletedNotice->nSquadID);

                if (psquad)
                {
                    DeleteSquad(psquad);
                }
            }
            break;

            case FM_S_SQUAD_TEXT_MESSAGE:
            {
                CASTPFM(pfmTextMsg, S, SQUAD_TEXT_MESSAGE, pSquadMessage);
                char * szTextMsg = FM_VAR_REF(pfmTextMsg, szMsg);

                MessageBox(szTextMsg);
            }
            break;

            case FM_S_SQUAD_CREATE_ACK:
            {
                CASTPFM(pfmAck, S, SQUAD_CREATE_ACK, pSquadMessage);

                TRef<IZoneSquad> psquad = FindSquadByCreationID(pfmAck->nClientSquadID);

                if (psquad)
                {
                    if (pfmAck->bSuccessful)
                    {
                        // see if there is an existing squad with this id, if so, delete it
                        // this can happen is the federation database is out-of-sync
                        TRef<IZoneSquad> psquadOld = FindSquadByCreationID(pfmAck->nSquadID);
                        if (psquadOld)
                        {
                          DeleteSquad(psquadOld);
                        }

                        psquad->SetID(pfmAck->nSquadID);

                        if (m_nTab == FILTER_ALL)
                        {
                            // now that we now the server made the squad, let's displayed the true
                            // squads that are supposed to be displayed around this one, given the
                            // current sort
                            ResetHighLowRanges();
                            m_nCurrentRanking = -1;
                            RequestPage(psquad);
                        }
                    }
                    else
                    {
                        DeleteSquad(psquad);
                    }
                }
            }
            break;
        }
    }


    /*-------------------------------------------------------------------------
     * MessageBox()
     *-------------------------------------------------------------------------
     */
    void MessageBox(ZString zstr)
    {
            static TRef<IMessageBox> pmsgBox = NULL;
            if (pmsgBox && !GetWindow()->GetPopupContainer()->IsEmpty())
            {
                GetWindow()->GetPopupContainer()->ClosePopup(pmsgBox);
                pmsgBox = NULL;
            }
            debugf(PCC(zstr));
            pmsgBox = CreateMessageBox( zstr );
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    }



    /*-------------------------------------------------------------------------
     * RefreshScreen()
     *-------------------------------------------------------------------------
     */
    void RefreshScreen()
    {
        RefreshButtonStates();
        RefreshSquadsListBox();
        RefreshPlayersListBox();
        RefreshSquadDetailsArea();
    }


    /*-------------------------------------------------------------------------
     * DisableAllOwnerButtons()
     *-------------------------------------------------------------------------
     */
    void DisableAllOwnerButtons(bool bEvenDisableEditSquad = true)
    {
        m_pbuttonReject->SetEnabled(false);
        m_pbuttonAccept->SetEnabled(false);
        m_pbuttonCommission->SetEnabled(false);
        m_pbuttonDemote->SetEnabled(false);
        m_pbuttonTransferControl->SetEnabled(false);
        
        if (bEvenDisableEditSquad){
            m_pbuttonEditSquad->SetEnabled(false);
		}
    }

    /*-------------------------------------------------------------------------
     * DisableAllButtons()
     *-------------------------------------------------------------------------
     */
    void DisableAllRegularButtons(bool bEvenDisableApply = true)
    {
        m_pbuttonReject->SetEnabled(false);
        m_pbuttonAccept->SetEnabled(false);
        m_pbuttonCommission->SetEnabled(false);
        m_pbuttonDemote->SetEnabled(false);
        m_pbuttonTransferControl->SetEnabled(false);
        m_pbuttonRequestJoin->SetEnabled(false);
        m_pbuttonCancelRequest->SetEnabled(false);
        m_pbuttonQuitSquad->SetEnabled(false);

        if(bEvenDisableApply)
            m_pbuttonApply->SetEnabled(false);
    }


    /*-------------------------------------------------------------------------
     * RefreshButtonStates()
     *-------------------------------------------------------------------------
     * Purpose:
     *    Enable or disable the buttons for everyone (and owners) depending
     *    on who is the active user, which squad is selected, and which player
     *    is selected
     */
    void RefreshButtonStates()
    {
        m_pbuttonCreateSquad->SetEnabled(m_psquadBeingMade ? false : true);

        //
        // Just in case check for these
        //
        if (!s_pSelectedSquad || !s_pSelectedSquad->WereDetailsReceived())
        {
            DisableAllOwnerButtons();
            DisableAllRegularButtons(s_pSelectedSquad ? false : true);
            return;
        }

        //
        //  Set the buttons for regular people
        //
        m_pbuttonRequestJoin->SetEnabled(!s_pActivePlayer->IsMemberOf(*s_pSelectedSquad) && 
                                         !s_pActivePlayer->IsPendingFor(*s_pSelectedSquad));
        m_pbuttonCancelRequest->SetEnabled(s_pActivePlayer->IsPendingFor(*s_pSelectedSquad));
        m_pbuttonQuitSquad->SetEnabled(s_pActivePlayer->IsMemberOf(*s_pSelectedSquad));

        //
        //  Set buttons for Owners                                                                                 
        //
        if (s_pSelectedSquad->GetOwner() == s_pActivePlayer) 
        {
            if (s_pSelectedPlayer && s_pActivePlayer != s_pSelectedPlayer)
            {
                m_pbuttonReject->SetEnabled(s_pSelectedPlayer->IsMemberOf(*s_pSelectedSquad) || s_pSelectedPlayer->IsPendingFor(*s_pSelectedSquad));
                m_pbuttonAccept->SetEnabled(s_pSelectedPlayer->IsPendingFor(*s_pSelectedSquad));
                m_pbuttonCommission->SetEnabled(s_pSelectedPlayer->GetPosition(*s_pSelectedSquad) == IZonePlayer::POS_MEMBER);
                m_pbuttonDemote->SetEnabled(s_pSelectedPlayer->GetPosition(*s_pSelectedSquad) == IZonePlayer::POS_ASL);
                m_pbuttonEditSquad->SetEnabled(true);
                m_pbuttonTransferControl->SetEnabled(s_pSelectedPlayer->IsMemberOf(*s_pSelectedSquad));
            }
            else // interactiveGameDestroyed user is leader and has himself selected
            {
                DisableAllOwnerButtons(false);

                m_pbuttonEditSquad->SetEnabled(true);
				
            }
        }
        else // not owner, or owner that is selecting himself
        {
            DisableAllOwnerButtons();
        }
    }


    //////////////////////////////////////////////////////////////////////////////
    //
    // UI Events
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        if (pevent == m_peventSquads) 
        {
            OnSquadSelected(*((IZoneSquad*)pitem));
        }
        else
        if (pevent == m_peventPlayers) 
        {
            OnPlayerSelected(*((IZonePlayer*)pitem));
        }
//        else
//        if (pevent == m_peventPlayersDoubleClick)
//        {
//            OnDoubleClickPlayer(*((IZonePlayer*)pitem));
//        }

        return true;
    }

    bool OnEvent(TEvent<ZString>::Source* pevent, ZString str)
    {
        if (pevent == m_peventFind)
        {
            OnButtonFindSquad();
        }
        else
        if (pevent == m_peventNameEdit)
        {
            if (m_bSquadEditURLReset)
            {
                m_bSquadEditURLReset=false;
                m_peditPaneSquadURL->SetString("");
            }

            GetEngineWindow()->SetFocus(m_peditPaneSquadURL);
        }
        else
        if (pevent == m_peventDescEdit)
        {
            if (m_bSquadEditNameReset)
            {
                m_bSquadEditNameReset=false;
                m_peditPaneSquadName->SetString("");
                m_pbuttonApply->SetEnabled(true);
            }

            GetEngineWindow()->SetFocus(m_peditPaneSquadName);
        }
        else
        if (pevent == m_peventURLEdit)
        {
            if (m_bSquadEditDescReset)
            {
                m_bSquadEditDescReset=false;
                m_peditPaneSquadDescription->SetString("");
            }

            GetEngineWindow()->SetFocus(m_peditPaneSquadDescription);
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IKeyboardInput Methods
    //
    //////////////////////////////////////////////////////////////////////////////
/*
    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        if (ks.bDown) 
        {
            if (ks.vk == VK_RETURN || ks.vk == VK_ESCAPE) 
            {
                return true;
            } 
            else if (ks.vk == VK_TAB) 
            {
                return true;
            }
        }
        return GetWindow()->GetFocus()->OnKey(pprovider, ks, fForceTranslate);
    }
*/

    bool OnButtonTop()
    {
        if (m_nTab != FILTER_ALL)
            return true;

        UnselectSquad();

        // reset highest/lowest
        m_nHighestRanking[s_column] = 999999;
        m_nLowestRanking[s_column] = -1;
        m_psquadHighestRanking[s_column] = NULL;
        m_psquadLowestRanking[s_column] = NULL;

        m_nCurrentRanking = 1;

        debugf("Paging to Top, m_nCurrentRanking = %d \n", m_nCurrentRanking);

        RequestPage(NULL);

        return true;
    }

    bool OnButtonPrev()  // down arrow was pressed
    {
        UnselectSquad();

        int nOriginalCurrentRanking = m_nCurrentRanking;

        m_nCurrentRanking -= (c_cSquadsShown-1);

        if (m_nCurrentRanking < 1) 
            m_nCurrentRanking = 1;

        if (m_nCurrentRanking < m_nHighestRanking[s_column])
            m_nCurrentRanking = m_nHighestRanking[s_column];

        debugf("Paging Up, m_nCurrentRanking = %d \n", m_nCurrentRanking);

        TRef<IZoneSquad> psquad = FindSquadByRanking(s_column, m_nCurrentRanking);

        if (psquad == NULL)  // if we scrolled too fast and the high/low ranges are no longer valid
        {
            debugf("High Range was invalid, resetting it.\n");
            ResetHighLowRanges();
            m_nCurrentRanking = nOriginalCurrentRanking;
            psquad = FindSquadByRanking(s_column, m_nCurrentRanking);
        }

        RequestPage(psquad);

        return true;
    }

    bool OnButtonNext() // up arrow was pressed
    {
        UnselectSquad();

        int nOriginalCurrentRanking = m_nCurrentRanking;

        m_nCurrentRanking += (c_cSquadsShown-1);

        if (m_nCurrentRanking > m_nLowestRanking[s_column])
            m_nCurrentRanking  = m_nLowestRanking[s_column];

        debugf("Paging Down, m_nCurrentRanking = %d \n", m_nCurrentRanking);

        TRef<IZoneSquad> psquad = FindSquadByRanking(s_column, m_nCurrentRanking);

        if (psquad == NULL) // if we scrolled too fast and the high/low ranges are no longer valid
        {
            debugf("Low Range was invalid, resetting it.\n");
            ResetHighLowRanges();
            m_nCurrentRanking = nOriginalCurrentRanking;
            psquad = FindSquadByRanking(s_column, m_nCurrentRanking);
        }

        RequestPage(psquad);

        return true;
    }


    bool OnButtonZoneClub()
    {
        OnLeavingScreen();

#ifndef MARKCU1
        GetWindow()->screen(ScreenIDZoneClubScreen);
#endif

        return true;
    }

    bool OnButtonLeaderboard()
    {
        OnLeavingScreen();

#ifndef MARKCU1
        if (s_pDudeXPlayer)
            GetWindow()->LeaderBoardScreenForPlayer(s_pDudeXPlayer->GetName()); 
        else
            GetWindow()->LeaderBoardScreenForPlayer(s_pActivePlayer->GetName()); 
#endif

        return true;
    }

    bool OnButtonPlayerProfile()
    {
        OnLeavingScreen();

#ifndef MARKCU1

        if (s_pSelectedPlayer)
        {
            GetWindow()->CharInfoScreenForPlayer(s_pSelectedPlayer->GetID()); 
        }
        else
        {
          if (s_pDudeXPlayer && m_nTab == FILTER_DUDEX)
              GetWindow()->CharInfoScreenForPlayer(s_pDudeXPlayer->GetID()); 
          else
              GetWindow()->CharInfoScreenForPlayer(s_pActivePlayer->GetID()); 
        }
#endif

        return true;
    }

    bool OnButtonReject()
    {
        if(s_pSelectedPlayer->IsPendingFor(*s_pSelectedSquad))
        {
            SEND_FOR_SELECTED_MEMBER(SQUAD_REJECT_JOIN_REQUEST);
            s_pSelectedSquad->RejectPlayer(*s_pSelectedPlayer);
            MessageBox("Player Rejected");
        }
        else
        {
            SEND_FOR_SELECTED_MEMBER(SQUAD_BOOT_MEMBER);
            s_pSelectedSquad->BootPlayer(*s_pSelectedPlayer);
            MessageBox("Player Booted");
        }

        RefreshScreen();

        return true;
    }

    bool OnButtonAccept()
    {
        SEND_FOR_SELECTED_MEMBER(SQUAD_ACCEPT_JOIN_REQUEST);

        s_pSelectedSquad->AddPlayer(*s_pSelectedPlayer, IZonePlayer::DSTAT_MEMBER);

        RefreshScreen();

        MessageBox("Player Accepted");

        return true;
    }

    bool OnButtonCommission()
    {
        SEND_FOR_SELECTED_MEMBER(SQUAD_PROMOTE_TO_ASL);

        s_pSelectedPlayer->SetPosition(*s_pSelectedSquad, IZonePlayer::POS_ASL);

        RefreshScreen();

        MessageBox("Player Commissioned");

        return true;
    }

    bool OnButtonDemote()
    {
        SEND_FOR_SELECTED_MEMBER(SQUAD_DEMOTE_TO_MEMBER);

        s_pSelectedPlayer->SetPosition(*s_pSelectedSquad, IZonePlayer::POS_MEMBER);

        RefreshScreen();

        MessageBox("Player Demoted");

        return true;
    }


    bool OnButtonTransferControl()
    {
        SEND_FOR_SELECTED_MEMBER(SQUAD_TRANSFER_CONTROL);

        s_pSelectedSquad->SetStatus(*s_pSelectedPlayer, IZonePlayer::DSTAT_LEADER);

        RefreshScreen();

        MessageBox("Ownership Transfered");

        return true;
    }

    bool OnButtonRequestJoin()
    {
        SEND_FOR_ACTIVE_MEMBER(SQUAD_MAKE_JOIN_REQUEST);

        s_pSelectedSquad->AddJoinRequest(*s_pActivePlayer);

        RefreshScreen();

        MessageBox("Request Issued");

        return true;
    }

    bool OnButtonCancelRequest()
    {
        SEND_FOR_ACTIVE_MEMBER(SQUAD_CANCEL_JOIN_REQUEST);

        s_pSelectedSquad->CancelJoinRequest(*s_pActivePlayer);

        // remove squad from these tabs as needed
        m_listSquads[FILTER_MY].Remove(s_pSelectedSquad);
        if (s_pDudeXPlayer == s_pActivePlayer)
          m_listSquads[FILTER_DUDEX].Remove(s_pSelectedSquad);

        RefreshScreen();

        MessageBox("Request Canceled");

        return true;
    }

    bool OnButtonQuitSquad()
    {
        SEND_FOR_ACTIVE_MEMBER(SQUAD_QUIT);

        s_pSelectedSquad->QuitPlayer(*s_pActivePlayer);

        // remove squad from these tabs as needed
        m_listSquads[FILTER_MY].Remove(s_pSelectedSquad);
        if (s_pDudeXPlayer == s_pActivePlayer)
          m_listSquads[FILTER_DUDEX].Remove(s_pSelectedSquad);

        RefreshScreen();

        MessageBox("Done");

        return true;
    }

    bool OnButtonCreateSquad()
    {
        if (m_nTab == FILTER_DUDEX) // if dudex is selected, switch to "my" tab
        {
            OnButtonBarTabs(FILTER_MY);
        }
        else
        if (m_nTab == FILTER_ALL)
        {
            // make room for the new squad
            TRef<IZoneSquad> pdum;
            if(m_listSquads[FILTER_ALL].GetCount() >= c_cSquadsShown) // if screen is full
            {
                //  m_listSquads[FILTER_ALL].PopEnd(); // causes stack overflow
                m_listSquads[FILTER_ALL].Remove(m_listSquads[FILTER_ALL].GetEnd());
            }
        }

        TRef<IZoneSquad> psquad = CreateZoneSquad();

        psquad->SetCreationID(m_nNextCreationID++);

        psquad->SetName("Click Here to Enter Name");
        psquad->SetDescription("Enter Description Here");
        psquad->SetURL("Enter Squad Home Page Here");
        psquad->SetInceptionDate("1/1/00");

        assert(s_pActivePlayer);

        psquad->AddPlayer(*s_pActivePlayer, IZonePlayer::DSTAT_LEADER);

        AddSquadToScreen(psquad, true, m_nTab);

        m_bSquadEditNameReset = true;
        m_bSquadEditDescReset = true;
        m_bSquadEditURLReset = true;

        m_psquadBeingMade = psquad;

        SelectSquad(*psquad);

        return true;
    }

    bool OnButtonEditSquad()
    {
        // Make fields writable
        //m_peditPaneSquadName->SetReadOnly(false); // Name not editable
        m_peditPaneSquadDescription->SetHidden(false);
        m_pstringDescription->SetValue("");
        m_peditPaneSquadURL->SetReadOnly(false);

        m_pbuttonApply->SetEnabled(true);
        m_pbuttonCreateSquad->SetEnabled(false);
        m_pbuttonQuitSquad->SetEnabled(false);
        m_pcomboCiv->SetEnabled(true);

		    m_pbuttonApply->SetHidden(false);
		    m_pbuttonEditSquad->SetHidden(true);

            GetEngineWindow()->SetFocus(m_peditPaneSquadDescription);

        return true;
    }

    bool OnSquadNameClick()
    {
        if (m_bSquadEditNameReset)
            m_peditPaneSquadName->SetString("");

        m_bSquadEditNameReset = false;
        m_pbuttonApply->SetEnabled(true);

        GetEngineWindow()->SetFocus(m_peditPaneSquadName);
        return true;
    }

    bool OnSquadDescriptionClick()
    {
        if (m_bSquadEditDescReset)
            m_peditPaneSquadDescription->SetString("");

        m_bSquadEditDescReset = false;

        GetEngineWindow()->SetFocus(m_peditPaneSquadDescription);
        return true;
    }

    bool OnSquadURLClick() // This is not the Jump to URL function, see OnButtonURL() 
    {
        if (m_bSquadEditURLReset)
            m_peditPaneSquadURL->SetString("");

        m_bSquadEditURLReset = false;

        GetEngineWindow()->SetFocus(m_peditPaneSquadURL);
        return true;
    }

    bool OnFindClick() 
    {
        GetEngineWindow()->SetFocus(m_peditPaneFind);
        return true;
    }

    bool OnButtonLog()
    {
        if (s_pSelectedSquad)
        {
            SEND_FOR_ACTIVE_MEMBER(SQUAD_LOG_REQ);

            m_pHistoryDialog->SetHistory(s_pSelectedSquad->GetOwnershipLog());

            GetWindow()->GetPopupContainer()->OpenPopup(m_pHistoryDialog, false);
        }

        return true;
    }

    bool OnButtonApply()
    {
        TRef<IKeyboardInput> p = GetEngineWindow()->GetFocus();
        if(p)
            GetEngineWindow()->RemoveFocus(p);

        if(s_pSelectedSquad == m_psquadBeingMade // if creating (not editing)
           &&  (m_bSquadEditNameReset ||
                m_peditPaneSquadName->GetString().GetLength() == 0)
           )
        {
            MessageBox("You must enter the \"squad name\".");
            return true;
        }

        m_bSquadEditNameReset = false;
        m_bSquadEditURLReset = false;
        m_bSquadEditDescReset = false;

        s_pSelectedSquad->SetName(m_peditPaneSquadName->GetString());
        s_pSelectedSquad->SetURL(m_peditPaneSquadURL->GetString());
        s_pSelectedSquad->SetDescription(m_peditPaneSquadDescription->GetString());
        s_pSelectedSquad->SetCivID(m_pCivIds[m_pcomboCiv->GetSelection()]);
                                                  
        m_peditPaneSquadName->SetReadOnly();
        m_peditPaneSquadDescription->SetHidden(true);
        m_pstringDescription->SetValue(m_peditPaneSquadDescription->GetString());
        m_peditPaneSquadURL->SetReadOnly();

        if (s_pSelectedSquad == m_psquadBeingMade)
        {
            debugf("Sending SQUAD_CREATE: name: %s, descrip: %s, URL: %s", PCC(m_peditPaneSquadName->GetString()),
                                                                           PCC(m_peditPaneSquadURL->GetString()),
                                                                           PCC(m_peditPaneSquadDescription->GetString()));

            BEGIN_PFM_CREATE(trekClient.m_fmClub, pfm, C, SQUAD_CREATE)
                FM_VAR_PARM(PCC(m_peditPaneSquadName->GetString()), CB_ZTS)
                FM_VAR_PARM(PCC(m_peditPaneSquadDescription->GetString()), CB_ZTS)
                FM_VAR_PARM(PCC(m_peditPaneSquadURL->GetString()), CB_ZTS)
            END_PFM_CREATE
            pfm->nClientSquadID = m_psquadBeingMade->GetCreationID();
            pfm->civid = m_psquadBeingMade->GetCivID();

            m_psquadBeingMade = NULL;
        }
        else
        {
            debugf("Sending SQUAD_EDIT; descrip: %s, URL: %s",             PCC(m_peditPaneSquadURL->GetString()),
                                                                           PCC(m_peditPaneSquadDescription->GetString()));

            BEGIN_PFM_CREATE(trekClient.m_fmClub, pfm, C, SQUAD_EDIT)
                FM_VAR_PARM(PCC(m_peditPaneSquadDescription->GetString()), CB_ZTS)
                FM_VAR_PARM(PCC(m_peditPaneSquadURL->GetString()), CB_ZTS)
            END_PFM_CREATE

            pfm->nSquadID = s_pSelectedSquad->GetID();
            pfm->civid = s_pSelectedSquad->GetCivID();
        }

        trekClient.SendClubMessages(); 

        RefreshScreen();

        m_pbuttonApply->SetEnabled(false);

        m_pcomboCiv->SetEnabled(false);

        m_pbuttonApply->SetHidden(true);
        m_pbuttonEditSquad->SetHidden(false);

        return true;
    }

    bool OnButtonJumpURL() // not to be confused with OnSquadURLClick()
    {
        ShellExecute(NULL, NULL, m_peditPaneSquadURL->GetString(), NULL, NULL, SW_SHOWNORMAL);

        return true;
    }

    bool OnButtonFindSquad()
    {
        if (m_peditPaneFind->GetString().GetLength() == 0)
            return true;

        m_listSquads[FILTER_ALL].SetEmpty();

        if (m_nTab != FILTER_ALL)
        {
            OnButtonBarTabs(FILTER_ALL, true);
        }

        if (s_column != 1)
        {
            m_nCurrentRanking = -1; // we always set this to -1 when changing columns
            m_psquadPrePage = NULL;
            OnButtonBarSquads(1, true); // set sort by name
        }

        m_nBestSearchMatch = 99999999;

        ResetHighLowRanges();

        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmPage, C, SQUAD_PAGE_FIND)
                FM_VAR_PARM(PCC(m_peditPaneFind->GetString()), CB_ZTS)
        END_PFM_CREATE
        debugf("Sending SQUAD_PAGE_FIND; %s\n", PCC(m_peditPaneFind->GetString())); 
        trekClient.SendClubMessages();

        return true;
    }

    bool _OnButtonBarSquads(int nColumn)
    {
        return OnButtonBarSquads(nColumn);
    }

    /*-------------------------------------------------------------------------
     * OnButtonBarSquads()
     *-------------------------------------------------------------------------
     * Purpose:
     *    User has requested a particular sort for column.  
     */
    bool OnButtonBarSquads(int nColumn, bool bSkipPageRequest = false)
    {
        // if clicks on Ranking, do nothing
        if (nColumn == 0 || nColumn == SSC_HOURS)
            return true;

        // do radio-button behavior
        for  (int i = 0; i < m_viColumns.GetCount(); i++) 
        {
            if (i != nColumn) {
                m_pbuttonbarSquadsHeader->SetChecked(i, false);
                m_pbuttonbarSquadsHeader->SetChecked2(i, false);
            }
        }

/*        if (m_pbuttonbarSquadsHeader->GetChecked(nColumn)) 
        {
            m_pbuttonbarSquadsHeader->SetChecked(nColumn,  false);
            m_pbuttonbarSquadsHeader->SetChecked2(nColumn, true);
        } else 
        {
*/
            m_pbuttonbarSquadsHeader->SetChecked(nColumn,  true);
            m_pbuttonbarSquadsHeader->SetChecked2(nColumn, false);
//        }

        s_column = (SQUAD_SORT_COLUMN) nColumn;

        if (m_nTab == FILTER_ALL)
        {
            if(!bSkipPageRequest)
                RequestPage(s_pSelectedSquad, true);
        }

        //
        // add this to the stack of sorting criteria as priority one;
        // these are only used for "my" and dudex tabs
        //
        m_vuSorts.Remove((unsigned)nColumn); // remove it's previous priority
        m_vuSorts.PushEnd(nColumn); // set as priority one 

        RefreshSquadsListBox();

        return true;
    }


    bool _OnButtonBarTabs(int nColumn)
    {
       return OnButtonBarTabs(nColumn);
    }

    /*-------------------------------------------------------------------------
     * OnButtonBarTabs()
     *-------------------------------------------------------------------------
     * Purpose:
     *    User has selected a tab...put it in the foreground.
     */
    bool OnButtonBarTabs(int nColumn, bool bSkipPageRequest = false)
    {
        if (m_nTab == nColumn)
            return true;

        // Disable DudeX column if Nobody is DudeX
        if (s_pDudeXPlayer == NULL && nColumn == 2)
            return true;

        TRef<IZoneSquad> psquadPreRefresh = s_pSelectedSquad;

        UnselectSquad();
        m_nTab = nColumn;
        RefreshSquadsListBox();  // this makes the m_nTab change legit

        if(nColumn == FILTER_ALL) // "All" Tab was pressed
        {
            m_pscrollPaneSquads->SetHidden(true);
            m_pbuttonNext->SetEnabled(true);
            m_pbuttonPrev->SetEnabled(true);
            m_pbuttonTop->SetEnabled(true);
            
            //ResetHighLowRanges();
            //m_nCurrentRanking = s_pSelectedSquad ? s_pSelectedSquad->GetRanking(s_column) : -1;
            if(!bSkipPageRequest)
                RequestPage(psquadPreRefresh ? psquadPreRefresh : m_psquadLastSelected, false, true);
        }
        else
        if(nColumn == FILTER_MY) // "My" Tab was pressed
        {
            m_pscrollPaneSquads->SetHidden(false);
            m_pbuttonNext->SetEnabled(false);
            m_pbuttonPrev->SetEnabled(false);
            m_pbuttonTop->SetEnabled(false);

            if (psquadPreRefresh)
                m_strSquadToBeSelected = psquadPreRefresh->GetName();
            else 
            {
                if (m_psquadLastSelected)
                    m_strSquadToBeSelected = m_psquadLastSelected->GetName();
                else
                    m_strSquadToBeSelected= "";
            }

            DisplaySquadsInRangeForDudeX(FILTER_MY, s_pActivePlayer);

            BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmSquadMessage, C, SQUAD_NEXT_PAGE_MY) 
            END_PFM_CREATE 
            debugf("Sending SQUAD_NEXT_PAGE_MY\n"); 
            trekClient.SendClubMessages(); 
        }
        else if(nColumn == FILTER_DUDEX) // DudeX Tab was pressed
        {
            m_pscrollPaneSquads->SetHidden(false);
            m_pbuttonNext->SetEnabled(false);
            m_pbuttonPrev->SetEnabled(false);
            m_pbuttonTop->SetEnabled(false);

            if (psquadPreRefresh)
                m_strSquadToBeSelected = psquadPreRefresh->GetName();
            else 
            {
                if (m_psquadLastSelected)
                    m_strSquadToBeSelected = m_psquadLastSelected->GetName();
                else
                    m_strSquadToBeSelected= "";
            }

            DisplaySquadsInRangeForDudeX(FILTER_DUDEX, s_pDudeXPlayer);

            debugf("Sending SQUAD_NEXT_PAGE_DUDEX; member %d (%s)\n", s_pDudeXPlayer->GetID(), s_pDudeXPlayer->GetName()); 

            BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmSquadMessage, C, SQUAD_NEXT_PAGE_DUDEX) 
            END_PFM_CREATE 
            pfmSquadMessage->nMemberID = s_pDudeXPlayer->GetID(); 
            trekClient.SendClubMessages(); 
        }

        // do radio-button behavior
        for  (int i = 0; i < m_viTabColumns.GetCount(); i++) 
        {
            if (i != nColumn) {
                m_ptabSquads->SetChecked(i, false);
                m_ptabSquads->SetChecked2(i, false);
            }
        }

        if (m_ptabSquads->GetChecked(nColumn)) 
        {
            m_ptabSquads->SetChecked(nColumn,  false);
            m_ptabSquads->SetChecked2(nColumn, true);
        } else 
        {
            m_ptabSquads->SetChecked(nColumn,  true);
            m_ptabSquads->SetChecked2(nColumn, false);
        }

        //
        //  Switch the game list filter of squads
        //
        m_filter = (FilterType)nColumn;
        
        RefreshSquadsListBox();

        return true;
    }


    /*-------------------------------------------------------------------------
     * OnButtonBarPlayers()
     *-------------------------------------------------------------------------
     * Purpose:
     *    User has requested a particular sort.  Give this column (nColumn) the 
     *    highest priority for the sort.
     */
    bool OnButtonBarPlayers(int nColumn)
    {
        // do radio-button behavior
        for  (int i = 0; i < m_viPlayersColumns.GetCount(); i++) 
        {
            if (i != nColumn) {
                m_pbuttonbarPlayersHeader->SetChecked(i, false);
                m_pbuttonbarPlayersHeader->SetChecked2(i, false);
            }
        }

/*        if (m_pbuttonbarPlayersHeader->GetChecked(nColumn)) 
        {
            m_pbuttonbarPlayersHeader->SetChecked(nColumn,  false);
            m_pbuttonbarPlayersHeader->SetChecked2(nColumn, true);
        } else 
        {
        */
            m_pbuttonbarPlayersHeader->SetChecked(nColumn,  true);
            m_pbuttonbarPlayersHeader->SetChecked2(nColumn, false);
//        }

        //
        // add this to the stack of sorting criteria as priority one
        //
        m_vuPlayersSorts.Remove((unsigned)nColumn); // remove it's previous priority
        m_vuPlayersSorts.PushEnd(nColumn); // set as priority one 

        RefreshPlayersListBox();

        return true;
    }

    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::OnDoubleClickPlayer()
     *-------------------------------------------------------------------------
     *   Event for when the active user double clicks on a person in the 
     *   players list box
     */
    bool OnDoubleClickPlayer(/*IZonePlayer * pPlayer*/)
    {
        if (s_pSelectedPlayer==NULL)
            return true;
        s_pDudeXPlayer = s_pSelectedPlayer;//pPlayer;
        m_pstringSquadTabDudeX->SetValue(s_pDudeXPlayer->GetName());
        m_ptabSquads->SetHidden(FILTER_DUDEX, false);
        if (m_nTab == FILTER_DUDEX)
        {
            DisplaySquadsInRangeForDudeX(FILTER_DUDEX, s_pDudeXPlayer);

            debugf("Sending SQUAD_NEXT_PAGE_DUDEX; member %d (%s)\n", s_pActivePlayer->GetID(), s_pActivePlayer->GetName()); 

            BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmSquadMessage, C, SQUAD_NEXT_PAGE_DUDEX) 
            END_PFM_CREATE 
            pfmSquadMessage->nMemberID = s_pDudeXPlayer->GetID(); 
            trekClient.SendClubMessages(); 
        }

        RefreshScreen();
        return true;
    }

    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::AddSquadToScreen()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Add a squad to the squad list box   
     * 
     */
    void AddSquadToScreen(TRef<IZoneSquad> pSquad, bool bRefresh = true, int nTab = 0)
    {
      if(m_listSquads[nTab].Find(pSquad))
          return;

       m_listSquads[nTab].PushEnd(pSquad);

      if(bRefresh)
        RefreshScreen();
    }


    void UnselectSquad()
    {
         m_plistPaneSquads->SetSelection(NULL);
         IZoneSquad * pNULL = NULL;
         m_bSelectionNotFromSquadClick = true;
         OnSquadSelected(*(pNULL));
    }


    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::DeleteSquad()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Remove a squad from existence
     * 
     */
    void DeleteSquad(TRef<IZoneSquad> pSquad, bool bRefresh = true)
    {
      if (m_psquadLastSelected == pSquad)
        m_psquadLastSelected = NULL;

      pSquad->SetEmpty(); // remove all players from it

      if (s_pSelectedSquad == pSquad)
      {
          UnselectSquad();
      }

      // remove from mother list
      m_listAllSquads.Remove(pSquad);
      
      // remove from screen list(s)
      m_listSquads[FILTER_ALL].Remove(pSquad);
      m_listSquads[FILTER_MY].Remove(pSquad);
      m_listSquads[FILTER_DUDEX].Remove(pSquad);

      if(bRefresh)
        RefreshScreen();
    }


    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::IsSquadAssociatedWithPlayer()
     *-------------------------------------------------------------------------
     * Purpose:
     *   filter for tabs
     *
     * Returns:
     *   true iff squad is associated to player in any way
     */
    static bool IsSquadAssociatedWithPlayer(IZoneSquad* pSquad, IZonePlayer *pPlayer)
    {
        return pPlayer->GetStatus(*pSquad) != IZonePlayer::STAT_NO_ASSOCIATION;
    }

    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::IsSquadAssociatedWithActivePlayer()
     *-------------------------------------------------------------------------
     * Purpose:
     *   filter for My tab
     *
     * Returns:
     *   true iff this squad is not associated to user in any way?
     */
    static bool IsSquadAssociatedWithActivePlayer(ItemID pitem) 
    {
        return IsSquadAssociatedWithPlayer((IZoneSquad*)pitem, s_pActivePlayer);
    }

    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::IsSquadAssociatedWithDudeX()
     *-------------------------------------------------------------------------
     * Purpose:
     *   filter for Dude X tab
     *
     * Returns:
     *   true iff this squad is not associated to tab's Dude X in any way?
     */
    static bool IsSquadAssociatedWithSelectedPlayer(ItemID pitem) 
    {
        return IsSquadAssociatedWithPlayer((IZoneSquad*)pitem, s_pSelectedPlayer);
    }

/*    static bool ShouldSquadBeShown(ItemID pitem) 
    {
        assert(g_pSquadScreen);
        return g_pSquadScreen->m_ShouldSquadBeShown(pitem);
    }

    bool m_ShouldSquadBeShown(ItemID pitem)
    {
        if(m_nCurrentRanking==-1)
            return false;

        int ranking = ((IZoneSquad*)pitem)->GetRanking(s_column);

        if (ranking != -1 && ranking >= m_nCurrentRanking && ranking < m_nCurrentRanking+c_cSquadsShown)
        {
            return true;
        }
        return false;
    }
*/
    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::RefreshSquadsListBox()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Redraw the listbox for squad
     * 
     */
    void RefreshSquadsListBox()
    {
        List *plist = new ListDelegate(&m_listSquads[m_nTab]);


//        plist = new FilteredList<bool (*)(ItemID)>(plist, ShouldSquadBeShown);

        /*
        switch (m_filter)
        {
            case FILTER_MY:
                plist = new FilteredList<bool (*)(ItemID)>(plist, IsSquadAssociatedWithActivePlayer);
                break;

            case FILTER_DUDEX:
                plist = new FilteredList<bool (*)(ItemID)>(plist, IsSquadAssociatedWithSelectedPlayer);
                break;

            default: // FILTER_ALL is default
                break;
        }
        */

        plist = new SortedList<ItemIDCompareFunction>(plist, SquadRankingCompare);

        if (m_nTab == FILTER_MY || m_nTab == FILTER_DUDEX) 
        {
            for (int i = 0; i < m_vuSorts.GetCount(); i++)
            {
                switch (m_vuSorts[i])
                {
                case 1:
                    plist = new SortedList<ItemIDCompareFunction>(plist, SquadNameCompare);
                    break;

                case 2:
                    plist = new SortedList<ItemIDCompareFunction>(plist, ScoreCompare);
                    break;

                case 3:
                    plist = new SortedList<ItemIDCompareFunction>(plist, WinsCompare);
                    break;

                case 4:
                    plist = new SortedList<ItemIDCompareFunction>(plist, LossesCompare);
                    break;

                case 5:
                    plist = new SortedList<ItemIDCompareFunction>(plist, HoursCompare);
                    break;
                }
            }
            
            m_plistPaneSquads->SetList(plist);
        }
        else
        m_plistPaneSquads->SetList(plist);
    }

    /*-------------------------------------------------------------------------
     * CPlayerscreen::RefreshPlayersListBox()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Redraw the listbox for squad
     * 
     */
    void RefreshPlayersListBox()
    {
        if(!s_pSelectedSquad)
        {
            m_plistPanePlayers->SetList(new EmptyList);
            return;
        }

        s_pSelectedSquad->AddPlayersToList(m_listPlayers);

        List * plist = new ListDelegate(&m_listPlayers);

/*        if (!m_fShowConquest)
            plist = new FilteredList<bool (*)(ItemID)>(plist, IsNotConquest);

        if (!m_fShowScoresCount)
            plist = new FilteredList<bool (*)(ItemID)>(plist, IsNotScoresCount);
*/
        for (int i = 0; i < m_vuPlayersSorts.GetCount(); i++)
        {
            switch (m_vuPlayersSorts[i])
            {
            case 0:
                plist = new SortedList<ItemIDCompareFunction>(plist, PlayerNameCompare);
                break;

            case 1:
                plist = new SortedList<ItemIDCompareFunction>(plist, PlayerRankCompare);
                break;

            case 2:
                plist = new SortedList<ItemIDCompareFunction>(plist, PlayerStatusCompare);
                break;

            case 3:
                plist = new SortedList<ItemIDCompareFunction>(plist, PlayerLastDateCompare);
                break;
            }
        }

        m_plistPanePlayers->SetList(plist);
    }

    // how different are these strings?
    unsigned CompareStrings(const char * sz1, const char * sz2, int cCharacters)
    {
        unsigned sum = 0;
        for (int i = 0; i < cCharacters; ++i)
        {
            sum += abs(toupper(sz1[i]) - toupper(sz2[i])) + (cCharacters - i) * 256;
        }
        return sum;
    }

    /////////////////////////////////////////////////////////////////////////

    int FillCombo(INameSpace* pns, ComboPane* pcombo, const char* szContentName)
    {
        IObjectList* plist; 
        
        CastTo(plist, pns->FindMember(szContentName));

        plist->GetFirst();

        int index = 0;
        while (plist->GetCurrent() != NULL) {
            pcombo->AddItem(GetString(plist->GetCurrent()), index);
            plist->GetNext();
            ++index;
        }
        return index;
    }

    /////////////////////////////////////////////////////////////////////////

    int FillIntegerArray(INameSpace* pns, int * pInts, const char* szContentName)
    {
        IObjectList* plist; 
        
        CastTo(plist, pns->FindMember(szContentName));

        plist->GetFirst();

        int index = 0;
        while (plist->GetCurrent() != NULL) {
            pInts[index] = (int)GetNumber(plist->GetCurrent());
            plist->GetNext();
            ++index;
        }
        return index;
    }

    static char* CivID2String(CivID id)
    {
      char *szCivs[] = {
        "ANY",
        "Rix",
        "Iron league",
        "Belorian",
      };
      
      if (id < -1 || id > 3) return "Unknown";

      return szCivs[id+1];
    }

    static bool SquadRankingCompare(ItemID pitem1, ItemID pitem2)
    {
        IZoneSquad* psquad1 = (IZoneSquad*)pitem1;
        IZoneSquad* psquad2 = (IZoneSquad*)pitem2;

        return psquad1->GetRanking(s_column) > psquad2->GetRanking(s_column);
    }

    static bool SquadNameCompare(ItemID pitem1, ItemID pitem2)
    {
        IZoneSquad* psquad1 = (IZoneSquad*)pitem1;
        IZoneSquad* psquad2 = (IZoneSquad*)pitem2;
                                                           
        return _stricmp(psquad1->GetName(), psquad2->GetName()) > 0;
    }

    static bool ScoreCompare(ItemID pitem1, ItemID pitem2)
    {
        IZoneSquad* psquad1 = (IZoneSquad*)pitem1;
        IZoneSquad* psquad2 = (IZoneSquad*)pitem2;

        return psquad1->GetScore() < psquad2->GetScore();
    }

    static bool WinsCompare(ItemID pitem1, ItemID pitem2)
    {
        IZoneSquad* psquad1 = (IZoneSquad*)pitem1;
        IZoneSquad* psquad2 = (IZoneSquad*)pitem2;

        return psquad1->GetWins() < psquad2->GetWins();
    }

    static bool LossesCompare(ItemID pitem1, ItemID pitem2)
    {
        IZoneSquad* psquad1 = (IZoneSquad*)pitem1;
        IZoneSquad* psquad2 = (IZoneSquad*)pitem2;

        return psquad1->GetLosses() < psquad2->GetLosses();
    }

    static bool HoursCompare(ItemID pitem1, ItemID pitem2)
    {
        IZoneSquad* psquad1 = (IZoneSquad*)pitem1;
        IZoneSquad* psquad2 = (IZoneSquad*)pitem2;

        return s_pActivePlayer->GetHours(*psquad1) < s_pActivePlayer->GetHours(*psquad1);
    }

    static bool PlayerNameCompare(ItemID pitem1, ItemID pitem2)
    {
        IZonePlayer* pPlayer1 = (IZonePlayer*)pitem1;
        IZonePlayer* pPlayer2 = (IZonePlayer*)pitem2;
                                                           
        return _stricmp(pPlayer1->GetName(), pPlayer2->GetName()) > 0;
    }

    static bool PlayerStatusCompare(ItemID pitem1, ItemID pitem2)
    {
        IZonePlayer* pPlayer1 = (IZonePlayer*)pitem1;
        IZonePlayer* pPlayer2 = (IZonePlayer*)pitem2;
                                                           
        return pPlayer1->GetStatus(*s_pSelectedSquad) < pPlayer2->GetStatus(*s_pSelectedSquad);
    }

    static bool PlayerLastDateCompare(ItemID pitem1, ItemID pitem2)
    {
        if (!s_pSelectedSquad)
          return false;

        IZonePlayer* pPlayer1 = (IZonePlayer*)pitem1;
        IZonePlayer* pPlayer2 = (IZonePlayer*)pitem2;

        char * sz1 = pPlayer1->GetLastPlayedDate(*s_pSelectedSquad);
        char * sz2 = pPlayer2->GetLastPlayedDate(*s_pSelectedSquad);

        if (strlen(sz1) == 8 && strlen(sz2) == 8)  // help ensure proper format
        {
          // Generate points for each day; a point is sort of like a day.   It's not exactly equal to a
          // day because there is no need to be exact.  Besides being exact would be quite tricky (think about leap year).
          unsigned n1 = (sz1[6]-48) * (10*366) + (sz1[7]-48) * 366 + (sz1[0]==' '?0:(sz1[0]-48)) * (10*32) + (sz1[1]-48) * 32 + (sz1[3]==' '?0:(sz1[3]-48)) * 11 + (sz1[4]-48);
          unsigned n2 = (sz2[6]-48) * (10*366) + (sz2[7]-48) * 366 + (sz2[0]==' '?0:(sz2[0]-48)) * (10*32) + (sz2[1]-48) * 32 + (sz2[3]==' '?0:(sz2[3]-48)) * 11 + (sz2[4]-48);
          return n1 > n2;
        }
        else return false;
    }

    static bool PlayerRankCompare(ItemID pitem1, ItemID pitem2)
    {
        IZonePlayer* pPlayer1 = (IZonePlayer*)pitem1;
        IZonePlayer* pPlayer2 = (IZonePlayer*)pitem2;
                                                           
        return pPlayer1->GetRank() < pPlayer2->GetRank();
    }

    //
    // Make these public for Painter classes 
    //
    TRef<Image> GetIconImage_Owner()    { return m_pimageOwner; }
    TRef<Image> GetIconImage_ASL()      { return m_pimageASL; }
    TRef<Image> GetIconImage_Pending()  { return m_pimagePending; }
    TRef<Image> GetIconImage_Rejected() { return m_pimageRejected; }
    TRef<Image> GetIconImage_Member()   { return m_pimageMember; }
    TRef<Image> GetIconImage_Booted()   { return m_pimageBooted; }
    TRef<Image> GetIconImage_Quit()     { return m_pimageQuit; }
    TRef<IZonePlayer> GetActivePlayer() { return s_pActivePlayer; }
    TRef<IZoneSquad> GetSelectedSquad()     { return s_pSelectedSquad; }

    class SquadsItemPainter;
    friend SquadsItemPainter;

    //////////////////////////////////////////////////////////////////////////////
    //
    // SquadsItemPainter - within the Squads listbox
    //
    //////////////////////////////////////////////////////////////////////////////
    class SquadsItemPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        SquadsScreen * m_pSquadsScreen;

    public:

        SquadsItemPainter(const TVector<int>& viColumns, SquadsScreen * pMaker)
            : m_viColumns(viColumns),
              m_pSquadsScreen(pMaker)
          {};

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
            IZoneSquad* psquad = (IZoneSquad*)pitemArg;

            char cbTemp[256];

            if (bSelected)
            {
              //
              // Draw selection bar
              //
              psurface->FillRect(
                  WinRect(0, 0, GetXSize(), GetYSize()),
                  m_pSquadsScreen->GetSquadSelectionBarColor());

//              m_pSquadsScreen->OnSquadSelected(*psquad);
            }

            // TODO: make read in font
            TRef<IEngineFont> pfont = TrekResources::SmallFont();

            //
            // Display Ranking
            //
            if(psquad->GetRanking(SquadsScreen::s_column) != -1 && m_pSquadsScreen->m_nTab == FILTER_ALL)
            {
                wsprintf(cbTemp, "%i", psquad->GetRanking(SquadsScreen::s_column));
                psurface->DrawString(pfont, Color::White(), WinPoint(/*m_viColumns[-1] +*/ 2, 0), cbTemp);
            }

            //
            // Display Name
            //
            
			
			WinRect rectClipOld = psurface->GetClipRect();
			
			
			if (strcmp(PCC(psquad->GetName()), "Click Here to Enter Name") == 0)
                wsprintf(cbTemp, "New Squad");
            else
                wsprintf(cbTemp, "%s", psquad->GetName());

            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[0] + 18, 0), WinPoint(m_viColumns[1], GetYSize()))); // clip name to fit in column
            psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[0] + 18, 0), cbTemp);
			
			psurface->RestoreClipRect(rectClipOld);

            //
            // Display Score
            //
            wsprintf(cbTemp, "%i", psquad->GetScore());
            psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[1] + 2, 0), cbTemp);

            //
            // Display Wins
            //
            wsprintf(cbTemp, "%i", psquad->GetWins());
            psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[2] + 2, 0), cbTemp);

            //
            // Display Losses
            //
            wsprintf(cbTemp, "%i", psquad->GetLosses());
            psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[3] + 2, 0), cbTemp);

            //
            // Display Proper Icon
            //
/*
            switch (m_pSquadsScreen->GetActivePlayer()->GetStatus(*psquad))
            {
            case IZonePlayer::STAT_ACTIVE:

                    if (psquad->GetOwner() == m_pSquadsScreen->GetActivePlayer())
                    {
                        psurface->BitBlt(WinPoint(m_viColumns[1]-20, 1), m_pSquadsScreen->GetIconImage_Owner()->GetSurface());
                    }
                    else if(m_pSquadsScreen->GetActivePlayer()->GetPosition(*psquad) == IZonePlayer::POS_ASL)
                    {
                        psurface->BitBlt(WinPoint(m_viColumns[1]-20, 1), m_pSquadsScreen->GetIconImage_ASL()->GetSurface());
                    }
                    else // must be member
                    {
                        psurface->BitBlt(WinPoint(m_viColumns[1]-20, 1), m_pSquadsScreen->GetIconImage_Member()->GetSurface());
                    }
                    break;

                case IZonePlayer::STAT_PENDING:

                    psurface->BitBlt(WinPoint(m_viColumns[1]-20, 1), m_pSquadsScreen->GetIconImage_Pending()->GetSurface());
                    break;

                case IZonePlayer::STAT_REJECTED:

                    psurface->BitBlt(WinPoint(m_viColumns[1]-20, 1), m_pSquadsScreen->GetIconImage_Rejected()->GetSurface());
                    break;

                case IZonePlayer::STAT_BOOTED:

                    psurface->BitBlt(WinPoint(m_viColumns[1]-20, 1), m_pSquadsScreen->GetIconImage_Booted()->GetSurface());
                    break;

                case IZonePlayer::STAT_QUIT:

                    psurface->BitBlt(WinPoint(m_viColumns[1]-20, 1), m_pSquadsScreen->GetIconImage_Quit()->GetSurface());
                    break;

                default:
                    assert(1); // User is not associated with the squad so shouldn't be drawn!
                    break;
            }
*/
                                
            //
            // Display Hours
            //
//            sprintf(cbTemp, "%3.1f", m_pSquadsScreen->GetActivePlayer()->GetHours(*psquad));
//            psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[4] + 2, 0), cbTemp);
        }
    };                            


    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::RefreshSquadDetailsArea()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Redraw the active squad's details-related stuff.
     * 
     */
    void RefreshSquadDetailsArea()
    {
      if(!s_pSelectedSquad) 
      {
          m_peditPaneSquadName->SetString("");
          m_peditPaneSquadDescription->SetString("");
          m_peditPaneSquadURL->SetString("");         
          m_pstrpaneRanking->SetString("");
          m_pstrpaneRank->SetString("");
          m_pstrpaneWinsAndLosses->SetString("");
          m_pstrpaneInceptionDate->SetString("");
          m_pcomboCiv->SetSelection(0);
          return;
      }

      //
      // Refresh Edit Panes
      //
      m_peditPaneSquadName->SetString(s_pSelectedSquad->GetName());
      m_peditPaneSquadDescription->SetString(s_pSelectedSquad->GetDescription());
      if(m_peditPaneSquadDescription->IsHidden())
        m_pstringDescription->SetValue(s_pSelectedSquad->GetDescription());
      else
        m_pstringDescription->SetValue("");
      m_peditPaneSquadURL->SetString(s_pSelectedSquad->GetURL());         

      //
      // Refresh String Panes
      //
//      if (s_pSelectedSquad->GetRanking(s_column) != -1)
//        m_pstrpaneRanking->SetString(s_pSelectedSquad->GetRanking(s_column));
      m_pstrpaneRank->SetString(s_pSelectedSquad->GetRank());
      m_pstrpaneWinsAndLosses->SetString(ZString(s_pSelectedSquad->GetWins()) + "/" + 
                                         ZString(s_pSelectedSquad->GetLosses()));
      m_pstrpaneInceptionDate->SetString(s_pSelectedSquad->GetInceptionDate());

      for (int i = 0; i < m_cCivs; ++i)
          if (m_pCivIds[i] == s_pSelectedSquad->GetCivID())
          {
              m_pcomboCiv->SetSelection(i);
          }
    }


    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::SelectSquad()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Call this to select a squad.
     * 
     */
    void SelectSquad(IZoneSquad & squad)
    {
       if (m_listSquads[m_nTab].Find(&squad))
       {
          m_bSelectionNotFromSquadClick = true;
          m_plistPaneSquads->SetSelection(&squad);
       }
    }

    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::SelectPlayer()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Call this to select a Player.
     * 
     */
    void SelectPlayer(IZonePlayer & Player)
    {
       m_plistPanePlayers->SetSelection(&Player);
    }

    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::OnSquadSelected()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Call this when a particular squad is selected.
     * 
     */
    void OnSquadSelected(IZoneSquad & squad) 
    {
        if (!m_bSelectionNotFromSquadClick)
        {
            m_bSelectorNeedsAutoSet = false;
            m_strSquadToBeSelected = "";
        }

        m_bSelectionNotFromSquadClick = false;

        if(&squad != NULL)
            m_psquadLastSelected = &squad;

        if (s_pSelectedSquad == &squad) return;

        s_pSelectedSquad = &squad; // note s_pSelectedSquad can be NULL

        if (!s_pSelectedSquad)
            m_pbuttonLog->SetEnabled(false);
        else
            m_pbuttonLog->SetEnabled(true);

        if (s_pSelectedSquad == m_psquadBeingMade && s_pSelectedSquad != NULL)
        {
            // Make fields writable if just created...this is done like this because selection is delayed
            m_peditPaneSquadName->SetReadOnly(false);
            m_peditPaneSquadDescription->SetHidden(false);
            m_pstringDescription->SetValue("");
            m_peditPaneSquadURL->SetReadOnly(false);
            m_pbuttonApply->SetEnabled(!m_bSquadEditNameReset);
            m_pbuttonCreateSquad->SetEnabled(false);

            m_pcomboCiv->SetEnabled(true);

            m_pbuttonApply->SetHidden(false);
            m_pbuttonEditSquad->SetHidden(true);

        }
        else
        {
            if (m_psquadBeingMade) // if user was making a squad, cancel that squad
            {
                TRef<IZoneSquad> psquadDead = m_psquadBeingMade;
                m_psquadBeingMade = NULL;
                DeleteSquad(psquadDead, false);

                MessageBox("The squad you were creating was cancelled.\n\nThe Edit Profile Button becomes the SAVE Button during squad creation.\n\nTo complete squad creation, you must click on the SAVE button.");
            }

            m_peditPaneSquadName->SetReadOnly();
            m_peditPaneSquadDescription->SetHidden(true);
            m_pstringDescription->SetValue(m_peditPaneSquadDescription->GetString());
            m_peditPaneSquadURL->SetReadOnly();
            m_pbuttonApply->SetEnabled(false);
            m_pbuttonCreateSquad->SetEnabled(true);

            m_pcomboCiv->SetEnabled(false);

			      m_pbuttonApply->SetHidden(true);
			      m_pbuttonEditSquad->SetHidden(false);

            if (s_pSelectedSquad)
            {
                SEND_FOR_ACTIVE_MEMBER(SQUAD_DETAILS_REQ);
            }
        }

        m_listPlayers.SetEmpty();
        s_pSelectedPlayer = NULL;

        RefreshButtonStates();
//        RefreshSquadsListBox();
        RefreshPlayersListBox();
        RefreshSquadDetailsArea();
    }

    /*-------------------------------------------------------------------------
     * IZoneSquadScreen::PlayerSelectedEvent()
     *-------------------------------------------------------------------------
     * Purpose:
     *   Call this when a particular player is selected.
     * 
     */
    void OnPlayerSelected(IZonePlayer & player)
    {
      if (&player == s_pSelectedPlayer) return;

      s_pSelectedPlayer = &player;

      RefreshButtonStates();
    }

    class PlayersItemPainter;
    friend class PlayersItemPainter;
    //////////////////////////////////////////////////////////////////////////////
    //
    // PlayersItemPainter -- within the Players listbox
    //
    //////////////////////////////////////////////////////////////////////////////
    class PlayersItemPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        SquadsScreen * m_pSquadsScreen;

    public:

        PlayersItemPainter(const TVector<int>& viColumns, SquadsScreen * pMaker)
            : m_viColumns(viColumns),
              m_pSquadsScreen(pMaker)
          {};

        int GetXSize()
        {
            return m_viColumns[3];
        }

        int GetYSize()
        {
            return 14;
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            // if a squad isn't selected, then paint no players
            if(!m_pSquadsScreen->GetSelectedSquad())
                return;

            IZonePlayer* pplayer = (IZonePlayer*)pitemArg;

            IZonePlayer::DetailedStatus detailedstatus = pplayer->GetStatus(*(m_pSquadsScreen->GetSelectedSquad()));

            // if this player isn't on the selected squad then don't draw him
            if (detailedstatus == IZonePlayer::DSTAT_NO_ASSOCIATION ||
                detailedstatus == IZonePlayer::DSTAT_UNKNOWN)
                return;

            char cbTemp[256];        

            if (bSelected)
             {
              //
              // Draw selection bar
              //
              psurface->FillRect(
                  WinRect(0, 0, GetXSize(), GetYSize()),
                  m_pSquadsScreen->GetPlayerSelectionBarColor());

//              m_pSquadsScreen->OnPlayerSelected(*pplayer);
            }

            // TODO: make read in
            TRef<IEngineFont> pfont = TrekResources::SmallFont();

            //
            // Display Name
            //

            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(2, 0), WinPoint(m_viColumns[0], GetYSize()))); // clip name to fit in column
            wsprintf(cbTemp, "%s", pplayer->GetName());
            psurface->DrawString(pfont, Color::White(), WinPoint(/*m_viColumns[-1] + */2, 0), cbTemp);
            psurface->RestoreClipRect(rectClipOld);
            
            //
            // Display Rank
            //
            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[0] + 2, 0), WinPoint(m_viColumns[1], GetYSize()))); // clip rank to fit in column
            CivID civid = s_pSelectedSquad ? s_pSelectedSquad->GetCivID() : -1;
            wsprintf(cbTemp, "%s", PCC(trekClient.LookupRankName((RankID)pplayer->GetRank(), civid)));
            psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[0] + 2, 0), cbTemp);
            psurface->RestoreClipRect(rectClipOld);

            //
            // Display Status
            //
            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[1] + 2, 0), WinPoint(m_viColumns[2], GetYSize()))); // clip status to fit in column
            wsprintf(cbTemp, "%s", IZonePlayer::DetailedStatus2String(detailedstatus));
            psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[1] + 2, 0), cbTemp);
            psurface->RestoreClipRect(rectClipOld);

            //
            // Display Last Date Played
            //
            if(s_pSelectedSquad)
            {
              wsprintf(cbTemp, "%s", pplayer->GetLastPlayedDate(*s_pSelectedSquad));
              psurface->DrawString(pfont, Color::White(), WinPoint(m_viColumns[2] + 2, 0), cbTemp);
            }
        }
    };


    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

    Color & GetPlayerSelectionBarColor()
    {
       return m_colorPlayerSelectionBar;
    }

    Color & GetSquadSelectionBarColor()
    {
       return m_colorSquadSelectionBar;
    }
};


//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateSquadsScreen(Modeler* pmodeler, const char * szNameDudeX, int idZoneDudeX, const char * szSquad)
{
    g_pSquadScreen = new SquadsScreen(pmodeler, szNameDudeX, idZoneDudeX, szSquad);

    return g_pSquadScreen;
}


TRef<IZonePlayer> SquadsScreen::s_pActivePlayer; // points to the squad player that the interactive user controls
TRef<IZonePlayer> SquadsScreen::s_pSelectedPlayer;
TRef<IZonePlayer> SquadsScreen::s_pDudeXPlayer;

SQUAD_SORT_COLUMN SquadsScreen::s_column; // currently sorted column

TRef<IZoneSquad>  SquadsScreen::s_pSelectedSquad;


SquadsScreen * g_pSquadScreen = NULL;

void ForwardSquadMessage(FEDMESSAGE * pSquadMessage)
{
    if(g_pSquadScreen)
    {
        g_pSquadScreen->ForwardSquadMessage(pSquadMessage);
    }
}

