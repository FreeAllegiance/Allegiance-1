#include "pch.h"
#include "badwords.h"

#include <button.h>
#include <controls.h>

#include "CreateGameDialog.h"

//////////////////////////////////////////////////////////////////////////////
//
// Game Screen
//
//////////////////////////////////////////////////////////////////////////////

// sample data for UI testing
//OldServerInfo sampleServers[10] = {
//	{"Planet","USA", 4,20,80,true},
//	{"Strip","CAN", 2,10,140,true},
//	{"GPZ","EU", 2,10,40,true},
//	{"BServ","unknown", 1,3,350,true},
//	{"123456789012345678901234","123456789012345678901234", 999,999,2000,false},
//	{"","",0,0,0,false}
//	};
//OldCoreInfo sampleCores[10] = {
//	{"FZ 1.25","static_core",false},
//	{"AZ 1.25","zone_core",false},
//	{"DN 4.60","dn_000460",true},
//	{"GoD II 4","GoDII_04",true},
//	{"StarWars 1.03","sw_a103",false},
//	{"EoR 6a ","RTc006a",true},
//	{"RPS 5.5","rps55",true},
//	{"Warpcore","warpcore",false},
//	{"12","12",false},
//	//{"123456789012345678901234","123456789012",false},
//	{"",""}
//};


class GameScreen :
    public Screen,
    public EventTargetContainer<GameScreen>,
    public TrekClientEventSink,
    public IItemEvent::Sink,
    public PasswordDialogSink,
    public IHTTPSessionSink
{

    IHTTPSession *              m_pSession;
    ZString                     m_strPendingFileList;
    ZString                     m_strWaitForFile;

    int                         m_nRetriesLeft;
    enum { c_cRetries = 5 };

public:

    enum StatusColor
    {
        statusGreen,
        statusYellow,
        statusRed
    };

    enum BooleanFilterState
    {
        No,
        Yes,
        DontCare
    };

    static BooleanFilterState m_sFilterDevelopments;
    static BooleanFilterState m_sFilterLives;
    static BooleanFilterState m_sFilterScoresCount;
    static BooleanFilterState m_sFilterZone;

    static BooleanFilterState m_sFilterConquest;
    static BooleanFilterState m_sFilterDeathmatch;
    static BooleanFilterState m_sFilterProsperity;
    static BooleanFilterState m_sFilterCountdown;
    static BooleanFilterState m_sFilterArtifact;
    static BooleanFilterState m_sFilterFlags;
    static BooleanFilterState m_sFilterTerritorial;

    static ZString m_strGameNameSubstring;

    static TVector<unsigned, DefaultEquals> m_vuSorts;
    static TVector<bool, DefaultEquals>     m_vbReversedSorts;

private:

    TRef<Pane>       m_ppane;

    TRef<ButtonPane> m_pbuttonBack;
    TRef<ButtonPane> m_pbuttonJoin;
    TRef<ButtonPane> m_pbuttonDetails;
    TRef<ButtonPane> m_pbuttonNewGame;
    TRef<ButtonPane> m_pbuttonFilter;
    TRef<ButtonPane> m_pbuttonFindPlayer;

    TRef<ButtonBarPane> m_pbuttonbarGamesHeader;

    TRef<ModifiableNumber> m_pnumberPlayerCount;

    TRef<ListPane>   m_plistPaneGames;

    TRef<IItemEvent::Source>   m_peventGames;
    TRef<TEvent<ItemID>::Sink> m_psinkGames;

    TVector<int>    m_viColumns;

    ZString         m_strLastPassword;

    class GameItemPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        GameScreen* m_pparent;

    public:

        GameItemPainter(const TVector<int>& viColumns, GameScreen* pparent)
            : m_viColumns(viColumns), m_pparent(pparent) {};

        int GetXSize()
        {
            return m_viColumns[7]; //KGJV #114 was col8
        }

        int GetYSize()
        {
            return 28;
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            MissionInfo* game = (MissionInfo*)pitemArg;
            char cbTemp[256];

            if (bSelected) {
                psurface->FillRect(
                    WinRect(56, 5, GetXSize() - 2, GetYSize() - 4), //Imago wider #169
                    Color(1, 0, 0)
                );
            }

            TRef<IEngineFont> pfont = TrekResources::SmallFont();
            Color             color = Color::White();


            // draw the Zone Icon
            if (game->WasObjectModelCreated())// KGJV #114 && trekClient.GetIsZoneClub())
            {
                DrawIcon(psurface, m_viColumns[0] - 90, GetYSize()/2, "iconzonebmp");

                // draw the Squad Pointer, but shifted left to make room for the zone icon
                if (trekClient.HasPlayerSquad(game))
                    DrawIcon(psurface, m_viColumns[0] - 120, GetYSize()/2, "iconsquadminebmp");
                else if (game->GetMissionParams().bSquadGame)
                    DrawIcon(psurface, m_viColumns[0] - 120, GetYSize()/2, "iconsquadherebmp");
            }
            else
            {
                // draw the Squad Pointer
                if (trekClient.HasPlayerSquad(game))
                    DrawIcon(psurface, m_viColumns[0] - 100, GetYSize()/2, "iconsquadminebmp");
                else if (game->GetMissionParams().bSquadGame)
                    DrawIcon(psurface, m_viColumns[0] - 100, GetYSize()/2, "iconsquadherebmp");
            }

            // draw the state icon

            TRef<Image> pimageState;
            switch (GameScreen::MissionStatusColor(game))
            {
            case statusGreen:
                if (game->InProgress())
                    pimageState = GetModeler()->LoadImage("iconrunninggreenbmp", true);
                else
                    pimageState = GetModeler()->LoadImage("iconwaitinggreenbmp", true);
                break;

            case statusYellow:
                if (game->InProgress())
                    pimageState = GetModeler()->LoadImage("iconrunningyellowbmp", true);
                else
                    pimageState = GetModeler()->LoadImage("iconwaitingyellowbmp", true);
                break;

            default:
                ZAssert(false);
            case statusRed:
                if (game->InProgress())
                    pimageState = GetModeler()->LoadImage("iconrunningredbmp", true);
                else
                    pimageState = GetModeler()->LoadImage("iconwaitingredbmp", true);
                break;
            }

            WinPoint pntStateIcon(
                ((m_viColumns[0]+54)/2 - (int)pimageState->GetBounds().GetRect().XSize()/2),
                (GetYSize() - (int)pimageState->GetBounds().GetRect().YSize())/2
                );
            psurface->BitBlt(pntStateIcon, pimageState->GetSurface());


            // draw the mission name
            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[0] + 4, 0), WinPoint(m_viColumns[1], GetYSize()))); // clip name to fit in column

            if (game->Name()[0] != '\0')
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[0] + 4, 6), CensorBadWords (game->Name()));
            else
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[0] + 4, 6), "<unnamed>");

            psurface->RestoreClipRect(rectClipOld);

            // draw the mission time
            if (game->InProgress() || game->CountdownStarted())
            {
                int nSecondsPlayed = (int)(Time::Now() - game->GetMissionParams().timeStart);
                int nHours = nSecondsPlayed / (60 * 60);
                int nMinutes = nSecondsPlayed / 60 - nHours * 60;

                if (nSecondsPlayed < 0 && game->CountdownStarted())
                    wsprintf(cbTemp, "-%d:%02d", -nHours, -nMinutes);
                else if (nSecondsPlayed < -100 || nHours > 99)
                    wsprintf(cbTemp, "days");
                else if (nSecondsPlayed < 0)
                    wsprintf(cbTemp, "0:00");
                else
                    wsprintf(cbTemp, "%d:%02d", nHours, nMinutes);

                psurface->DrawString(pfont, color,
                    WinPoint(m_viColumns[2] - pfont->GetTextExtent(cbTemp).X() - 5, 6),
                    cbTemp);
            }

            // draw in the skill level
            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[2] + 4, 0), WinPoint(m_viColumns[3], GetYSize()))); // clip skill level to fit in column
            psurface->DrawString(pfont, color, WinPoint(m_viColumns[2] + 4, 6), m_pparent->FindSkillLevelName(game));
            psurface->RestoreClipRect(rectClipOld);

            // draw team info:
            //wsprintf(cbTemp, "%d", game->NumSides());
            //psurface->DrawString(pfont, color,
            //    WinPoint(m_viColumns[4] - pfont->GetTextExtent(cbTemp).X() - 5, 6),
            //    cbTemp);

			// KGJV #114 draw server name
			rectClipOld = psurface->GetClipRect();
			psurface->SetClipRect(WinRect(m_viColumns[3], 0, m_viColumns[4], GetYSize()));
            wsprintf(cbTemp, "%s", game->GetMissionDef().szServerName);
			psurface->DrawString(pfont, color,
                WinPoint(m_viColumns[3] +4, 6),
                cbTemp);
			psurface->RestoreClipRect(rectClipOld);

			// KGJV #114
            //wsprintf(cbTemp, "%d", game->MinPlayersPerTeam());
            //psurface->DrawString(pfont, color,
            //    WinPoint(m_viColumns[5] - pfont->GetTextExtent(cbTemp).X() - 5, 6),
            //    cbTemp);

            //wsprintf(cbTemp, "%d", game->MaxPlayersPerTeam());
            //psurface->DrawString(pfont, color,
            //    WinPoint(m_viColumns[6] - pfont->GetTextExtent(cbTemp).X() - 5, 6),
            //    cbTemp);
			// draw core name
			ZString sCorename = ZString(trekClient.CfgGetCoreName(game->GetIGCStaticFile()));
            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[4] + 4, 0), WinPoint(m_viColumns[5], GetYSize()))); // clip to fit in column
            psurface->DrawString(pfont, color, WinPoint(m_viColumns[4] + 4, 6), sCorename);
            psurface->RestoreClipRect(rectClipOld);

            // draw slot info
			wsprintf(cbTemp, "%d/%d", game->NumPlayers(), game->MaxPlayers());
			WinPoint myPoint = WinPoint(m_viColumns[6] - pfont->GetTextExtent(cbTemp).X() - 3, 3);
            psurface->DrawString(pfont, color,
                myPoint, // KGJV #114 was col7 //Imago #169
                cbTemp);

			// #169 Imago 7/10
			wsprintf(cbTemp, "Noat: %d", game->NumNoatPlayers());
			WinPoint myPoint2 = WinPoint(m_viColumns[6] - pfont->GetTextExtent(cbTemp).X() - 3, 12);
            psurface->DrawString(pfont,Color::Gray(),
                myPoint2, // KGJV #114 was col7 //Imago #169
                cbTemp);



            // find the game type...
            TRef<GameType> pGameType = GameType::FindType(game->GetMissionParams());

            if (pGameType != NULL)
            {
                // draw the game type's name
                psurface->DrawString(pfont, color, WinPoint(m_viColumns[6] + 4, 6), pGameType->GetName());//KGJV #114 was col7
            }
            else
            {
                // custom type - draw style icons
				// KGJV #114 was col7
                if (game->GoalConquest())
                    DrawIcon(psurface, m_viColumns[6] + 3, GetYSize()/2, "iconconquestbmp");
                if (game->GoalTerritory())
                    DrawIcon(psurface, m_viColumns[6] + 19, GetYSize()/2, "iconterritorialbmp");
                if (game->GoalProsperity())
                    DrawIcon(psurface, m_viColumns[6] + 35, GetYSize()/2, "iconprosperitybmp");
                if (game->GoalArtifacts())
                    DrawIcon(psurface, m_viColumns[6] + 51, GetYSize()/2, "iconartifactsbmp");
                if (game->GoalFlags())
                    DrawIcon(psurface, m_viColumns[6] + 67, GetYSize()/2, "iconflagsbmp");
                if (game->GoalDeathMatch())
                    DrawIcon(psurface, m_viColumns[6] + 83, GetYSize()/2, "icondeathmatchbmp");
                if (game->GoalCountdown())
                    DrawIcon(psurface, m_viColumns[6] + 99, GetYSize()/2, "iconcountdownbmp");
            }

            //if (game->ScoresCount())
            //    DrawIcon(psurface, m_viColumns[6] + 115, GetYSize()/2 - 1, "iconscorescountbmp");
			// KGJV #114 - ScoresCount symbol now mean Official game (= official server + official core)
			bool bOfficial = trekClient.CfgIsOfficialServer(game->GetMissionDef().szServerName,game->GetMissionDef().szServerAddr);
			bOfficial &= trekClient.CfgIsOfficialCore(game->GetIGCStaticFile());
            if (bOfficial)
                DrawIcon(psurface, m_viColumns[6] + 107, GetYSize()/2 - 1, "iconscorescountbmp"); //imago 107 was 115

            if (game->AllowDevelopments())
                DrawIcon(psurface, m_viColumns[6] + 107, GetYSize()/2 - 1, "icondevelopmentsbmp");
            if (game->LimitedLives())
                DrawIcon(psurface, m_viColumns[6] + 107, GetYSize()/2 - 1, "iconlivesbmp");
         }

        int DrawIcon(Surface* psurface, int nXLeft, int nYCenter, const char* iconName)
        {
            TRef<Image> pimage = GetModeler()->LoadImage(iconName, true);

            WinPoint pntIcon(
                nXLeft,
                nYCenter - (int)pimage->GetBounds().GetRect().YSize()/2
                );
            psurface->BitBlt(pntIcon, pimage->GetSurface());

            return nXLeft + (int)pimage->GetBounds().GetRect().XSize() + 2;
        }
    };

	//KGJV #114 //Rock moved to own file
    friend class CreateGameDialogPopup;

    friend class FilterDialogPopup;
    class FilterDialogPopup : public IPopup, public EventTargetContainer<FilterDialogPopup>
    {
    private:
        TRef<Pane> m_ppane;
        TRef<ButtonPane> m_pbuttonOK;
        TRef<ButtonPane> m_pbuttonCancel;

        TRef<ComboPane>  m_pcomboDevelopments;
        TRef<ComboPane>  m_pcomboLives;
        TRef<ComboPane>  m_pcomboScoresCount;
        TRef<ComboPane>  m_pcomboZone;

        TRef<ComboPane>  m_pcomboConquest;
        TRef<ComboPane>  m_pcomboDeathmatch;
        TRef<ComboPane>  m_pcomboProsperity;
        TRef<ComboPane>  m_pcomboCountdown;
        TRef<ComboPane>  m_pcomboArtifact;
        TRef<ComboPane>  m_pcomboFlags;
        TRef<ComboPane>  m_pcomboTerritorial;

        TRef<EditPane>   m_peditGameName;

        GameScreen* m_pparent;

        TRef<IKeyboardInput> m_pkeyboardInputOldFocus;

    public:

        FilterDialogPopup(TRef<INameSpace> pns, GameScreen* pparent)
        {
            m_pparent = pparent;

            CastTo(m_ppane,                 pns->FindMember("FilterDialog"));

            CastTo(m_pbuttonOK,             pns->FindMember("filterOkButtonPane"));
            CastTo(m_pbuttonCancel,         pns->FindMember("filterCancelButtonPane"));

            CastTo(m_pcomboDevelopments, pns->FindMember("developmentsComboPane"));
            CastTo(m_pcomboLives, pns->FindMember("livesComboPane"));
            CastTo(m_pcomboScoresCount, pns->FindMember("scoresCountComboPane"));
            CastTo(m_pcomboZone, pns->FindMember("zoneComboPane"));

            CastTo(m_pcomboConquest, pns->FindMember("conquestComboPane"));
            CastTo(m_pcomboDeathmatch, pns->FindMember("deathmatchComboPane"));
            CastTo(m_pcomboProsperity, pns->FindMember("prosperityComboPane"));
            CastTo(m_pcomboCountdown, pns->FindMember("countdownComboPane"));
            CastTo(m_pcomboArtifact, pns->FindMember("artifactComboPane"));
            CastTo(m_pcomboFlags, pns->FindMember("flagsComboPane"));
            CastTo(m_pcomboTerritorial, pns->FindMember("territorialComboPane"));

            CastTo(m_peditGameName, (Pane*)pns->FindMember("gameNameEditPane"));

            InitializeYesNoDontCare(m_pcomboDevelopments);
            InitializeYesNoDontCare(m_pcomboLives);
            InitializeYesNoDontCare(m_pcomboScoresCount);
            InitializeYesNoDontCare(m_pcomboZone);

            InitializeYesNoDontCare(m_pcomboConquest);
            InitializeYesNoDontCare(m_pcomboDeathmatch);
            InitializeYesNoDontCare(m_pcomboProsperity);
            InitializeYesNoDontCare(m_pcomboCountdown);
            InitializeYesNoDontCare(m_pcomboArtifact);
            InitializeYesNoDontCare(m_pcomboFlags);
            InitializeYesNoDontCare(m_pcomboTerritorial);

			// mdvalley: Pointers and classes.
            AddEventTarget(&GameScreen::FilterDialogPopup::OnButtonOK, m_pbuttonOK->GetEventSource());
            AddEventTarget(&GameScreen::FilterDialogPopup::OnButtonCancel, m_pbuttonCancel->GetEventSource());
        }

        void InitializeYesNoDontCare(ComboPane* pcombo)
        {
            pcombo->AddItem("Yes", Yes);
            pcombo->AddItem("No", No);
            pcombo->AddItem("DontCare", DontCare);
        }

        //
        // IPopup methods
        //

        virtual void OnClose()
        {
            if (m_pkeyboardInputOldFocus)
                GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);

            m_pkeyboardInputOldFocus = NULL;

            IPopup::OnClose();
        }

        virtual void SetContainer(IPopupContainer* pcontainer)
        {
            // initialize the check boxes
            m_pcomboDevelopments->SetSelection(m_pparent->m_sFilterDevelopments);
            m_pcomboLives->SetSelection(m_pparent->m_sFilterLives);
            m_pcomboScoresCount->SetSelection(m_pparent->m_sFilterScoresCount);
            m_pcomboZone->SetSelection(m_pparent->m_sFilterZone);

            m_pcomboConquest->SetSelection(m_pparent->m_sFilterConquest);
            m_pcomboDeathmatch->SetSelection(m_pparent->m_sFilterDeathmatch);
            m_pcomboProsperity->SetSelection(m_pparent->m_sFilterProsperity);
            m_pcomboCountdown->SetSelection(m_pparent->m_sFilterCountdown);
            m_pcomboArtifact->SetSelection(m_pparent->m_sFilterArtifact);
            m_pcomboFlags->SetSelection(m_pparent->m_sFilterFlags);
            m_pcomboTerritorial->SetSelection(m_pparent->m_sFilterTerritorial);

            m_peditGameName->SetString(m_pparent->m_strGameNameSubstring);

            m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
            GetEngineWindow()->SetFocus(m_peditGameName);

            IPopup::SetContainer(pcontainer);
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
            if (ks.vk == 13)
            {
                OnButtonOK();
                return true;
            }
            else
                return ((IKeyboardInput*)m_peditGameName)->OnChar(pprovider, ks);
        }

        bool OnButtonOK()
        {
            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }

            m_pparent->m_sFilterDevelopments = BooleanFilterState(m_pcomboDevelopments->GetSelection());
            m_pparent->m_sFilterLives = BooleanFilterState(m_pcomboLives->GetSelection());
            m_pparent->m_sFilterScoresCount = BooleanFilterState(m_pcomboScoresCount->GetSelection());
            m_pparent->m_sFilterZone = BooleanFilterState(m_pcomboZone->GetSelection());

            m_pparent->m_sFilterConquest = BooleanFilterState(m_pcomboConquest->GetSelection());
            m_pparent->m_sFilterDeathmatch = BooleanFilterState(m_pcomboDeathmatch->GetSelection());
            m_pparent->m_sFilterProsperity = BooleanFilterState(m_pcomboProsperity->GetSelection());
            m_pparent->m_sFilterCountdown = BooleanFilterState(m_pcomboCountdown->GetSelection());
            m_pparent->m_sFilterArtifact = BooleanFilterState(m_pcomboArtifact->GetSelection());
            m_pparent->m_sFilterFlags = BooleanFilterState(m_pcomboFlags->GetSelection());
            m_pparent->m_sFilterTerritorial = BooleanFilterState(m_pcomboTerritorial->GetSelection());

            m_pparent->m_strGameNameSubstring = m_peditGameName->GetString();

            m_pparent->UpdateGameList();

            return true;
        }

        bool OnButtonCancel()
        {
            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }

            return true;
        }
    };


    friend class FindDialogPopup;
    class FindDialogPopup : public IPopup, public EventTargetContainer<FindDialogPopup>
    {
    private:
        TRef<Pane> m_ppane;
        TRef<ButtonPane> m_pbuttonOK;
        TRef<ButtonPane> m_pbuttonCancel;
        TRef<EditPane> m_peditPane;

        GameScreen* m_pparent;
        TRef<IKeyboardInput> m_pkeyboardInputOldFocus;

    public:

        FindDialogPopup(TRef<INameSpace> pns, GameScreen* pparent)
        {
            m_pparent = pparent;

            CastTo(m_ppane,                 pns->FindMember("FindDialog"));

            CastTo(m_pbuttonOK,             pns->FindMember("findOkButtonPane"));
            CastTo(m_pbuttonCancel,         pns->FindMember("findCancelButtonPane"));
            CastTo(m_peditPane,      (Pane*)pns->FindMember("findPlayerEditPane"));

            AddEventTarget(&GameScreen::FindDialogPopup::OnButtonOK, m_pbuttonOK->GetEventSource());
            AddEventTarget(&GameScreen::FindDialogPopup::OnButtonCancel, m_pbuttonCancel->GetEventSource());
        }

        //
        // IPopup methods
        //

        Pane* GetPane()
        {
            return m_ppane;
        }

        virtual void OnClose()
        {
            if (m_pkeyboardInputOldFocus)
                GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);

            m_pkeyboardInputOldFocus = NULL;

            IPopup::OnClose();
        }

        virtual void SetContainer(IPopupContainer* pcontainer)
        {
            m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
            GetEngineWindow()->SetFocus(m_peditPane);

            IPopup::SetContainer(pcontainer);
        }

        bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
        {
            // we need to make sure we get OnChar calls to pass on to the edit box
            fForceTranslate = true;
            return false;
        }

        bool OnChar(IInputProvider* pprovider, const KeyState& ks)
        {
            if (ks.vk == 13)
            {
                OnButtonOK();
                return true;
            }
            else
                return ((IKeyboardInput*)m_peditPane)->OnChar(pprovider, ks);
        }

        bool OnButtonOK()
        {
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
            m_pkeyboardInputOldFocus = NULL;

            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }

            GetWindow()->SetWaitCursor();
            TRef<IMessageBox> pmsgBox =
                CreateMessageBox("Asking server to find player....", NULL, false, false, 1.0f);
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

            BEGIN_PFM_CREATE(trekClient.m_fmLobby, pfmFindPlayer, C, FIND_PLAYER)
                FM_VAR_PARM((const char*)m_peditPane->GetString(), CB_ZTS)
            END_PFM_CREATE
            trekClient.SendLobbyMessages();

            return true;
        }

        bool OnButtonCancel()
        {
            GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
            m_pkeyboardInputOldFocus = NULL;

            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }

            return true;
        }
    };


    TRef<FilterDialogPopup> m_pfilterDialog;
    TRef<FindDialogPopup> m_pfindDialog;
	TRef<CreateGameDialogPopup> m_pcreateDialog; // KGJV #114
    TRef<INameSpace>     m_pns;

    static bool IsZoneLobby()
    {
        return trekClient.GetIsZoneClub();
    };

public:
    GameScreen(Modeler* pmodeler)
        : m_filter(this), m_pSession(NULL), m_nRetriesLeft(c_cRetries)
    {
        trekClient.DisconnectClub();
        trekClient.SetIsLobbied(true);

        TRef<IObject> pobjColumns;


                // Export to MDL

        TRef<INameSpace> pnsGameScreen = pmodeler->CreateNameSpace("gamescreendata", pmodeler->GetNameSpace("effect"));

        pnsGameScreen->AddMember("playerCount",            m_pnumberPlayerCount           = new ModifiableNumber(0));

        // Load the members from MDL

        m_pns = pmodeler->GetNameSpace(IsZoneLobby() ? "zonegamescreen" : "gamescreen");

        CastTo(m_ppane,                 m_pns->FindMember("screen"           ));
        CastTo(m_pbuttonBack,           m_pns->FindMember("backButtonPane"   ));
        CastTo(m_pbuttonJoin,           m_pns->FindMember("joinButtonPane"   ));
        CastTo(m_pbuttonDetails,        m_pns->FindMember("detailsButtonPane"));
        CastTo(m_pbuttonNewGame,        m_pns->FindMember("newGameButtonPane"));
        CastTo(m_pbuttonFilter,         m_pns->FindMember("filterButtonPane" ));
        CastTo(m_pbuttonFindPlayer,     m_pns->FindMember("findPlayerButtonPane"));
        CastTo(m_plistPaneGames, (Pane*)m_pns->FindMember("gameListPane"     ));
        CastTo(m_pbuttonbarGamesHeader, m_pns->FindMember("gameListHeader"   ));
        CastTo(pobjColumns,             m_pns->FindMember("gameColumns"      ));

		m_pfilterDialog = new FilterDialogPopup(m_pns, this);
        m_pfindDialog = new FindDialogPopup(m_pns, this);
		// KGJV #114
		m_pcreateDialog = new CreateGameDialogPopup(m_pns);
        //
        // game lists
        //

        ParseIntVector(pobjColumns, m_viColumns);

        m_peventGames = m_plistPaneGames->GetSelectionEventSource();
        m_peventGames->AddSink(m_psinkGames = new IItemEvent::Delegate(this));

        AddEventTarget(&GameScreen::OnButtonJoin, m_plistPaneGames->GetDoubleClickEventSource());

        m_plistPaneGames->SetItemPainter(new GameItemPainter(m_viColumns, this));

        static bool bStaticsInitialized = false;

        if (!bStaticsInitialized)
        {
            ResetFilters();

            // WLP 2005 - removed the sorts from here - they are done in resetfilters now
            // m_vuSorts.PushEnd(0);
            // m_vbReversedSorts.PushEnd(false);

            bStaticsInitialized = true;
        }
        else
        {
            UpdateGameList();
        }

        RefreshButtonBarGames();

        //
        // buttons
        //

		// KGJV #114
        AddEventTarget(&GameScreen::OnButtonNewGame, m_pbuttonNewGame->GetEventSource());
        if (IsZoneLobby())
        {
            AddEventTarget(&GameScreen::OnButtonDetails, m_pbuttonDetails->GetEventSource());
        }
        AddEventTarget(&GameScreen::OnButtonFindPlayer, m_pbuttonFindPlayer->GetEventSource());
        AddEventTarget(&GameScreen::OnButtonBack, m_pbuttonBack->GetEventSource());
        AddEventTarget(&GameScreen::OnButtonJoin, m_pbuttonJoin->GetEventSource());
        AddEventTarget(&GameScreen::OnButtonFilter, m_pbuttonFilter->GetEventSource());

        AddEventTarget(&GameScreen::OnButtonBarGames, m_pbuttonbarGamesHeader->GetEventSource());

        // update the join button
        OnSelectMission((MissionInfo*)m_plistPaneGames->GetSelection());

        if (g_bQuickstart)
            OnButtonNewGame();

        AddEventTarget(&GameScreen::OnListChanged, trekClient.GetMissionList()->GetChangedEvent());
        AddEventTarget(&GameScreen::OnRefreshTimer, GetEngineWindow(), 5);

        // if we are not connected, pop up a dialog box and let the screen
        // draw itself while we are waiting.
        if (trekClient.m_fmLobby.IsConnected())
        {
            DoLobbyConnect();
        }
        else
        {
            GetWindow()->SetWaitCursor();
            TRef<IMessageBox> pmsgBox =
                CreateMessageBox("Connecting to lobby....", NULL, false, false);
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

            AddEventTarget(&GameScreen::DoLobbyConnect, GetEngineWindow(), 0.1f);
        }
    }

    ~GameScreen()
    {
        if (m_pSession)
        {
            delete m_pSession;
            m_pSession = NULL;
        }

        GetModeler()->UnloadNameSpace(IsZoneLobby() ? "zonegamescreen" : "gamescreen");
        m_peventGames->RemoveSink(m_psinkGames);
        trekClient.DisconnectLobby();
    }

    bool DoLobbyConnect()
    {
        // close the "connecting..." popup
        if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
            GetWindow()->GetPopupContainer()->ClosePopup(NULL);
        GetWindow()->RestoreCursor();

        // make sure we're connected
        HRESULT hr = trekClient.ConnectToLobby(NULL);
        if (FAILED(hr))
        {
            GetWindow()->screen(ScreenIDZoneClubScreen);
            TRef<IMessageBox> pmsgBox =
                CreateMessageBox("Failed to reconnect to the lobby.");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
        else
        {
            trekClient.RestoreSquadMemberships(trekClient.m_szLobbyCharName);
        }

        return false;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // UI Events
    //
    //////////////////////////////////////////////////////////////////////////////


    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        if (pevent == m_peventGames) {
            MissionInfo* pmission = (MissionInfo*)pitem;
            OnSelectMission(pmission);
        }
        return true;
    }

    bool OnRefreshTimer()
    {
        // refresh so the time in progress can update
        m_plistPaneGames->ForceRefresh();
        return true;
    }

    void OnSelectMission(MissionInfo* pmission)
    {
        bool bEnableJoin = false;
        bool bShowDetails = false;

        if (pmission)
        {
            if (pmission->GetStage() != STAGE_OVER)
                bEnableJoin = true;

            bShowDetails = !pmission->GetDetailsFiles().IsEmpty();
        }


        m_pbuttonJoin->SetEnabled(bEnableJoin);

        if (IsZoneLobby())
            m_pbuttonDetails->SetHidden(!bShowDetails);
    }

    void RefreshButtonBarGames()
    {
        // do radio-button behavior
        for  (int i = 0; i < 8; i++) // KGJV #114 
        {
            m_pbuttonbarGamesHeader->SetChecked(i, false);
            m_pbuttonbarGamesHeader->SetChecked2(i, false);
        }

        if (m_vbReversedSorts.GetEnd())
            m_pbuttonbarGamesHeader->SetChecked(m_vuSorts.GetEnd(), true);
        else
            m_pbuttonbarGamesHeader->SetChecked2(m_vuSorts.GetEnd(), true);
    }

    bool OnButtonBarGames(int nColumn)
    {
        bool bReverse;

        // do radio-button behavior
        for  (int i = 0; i < 8; i++) // KGJV #114
        {
            if (i != nColumn) {
                m_pbuttonbarGamesHeader->SetChecked(i, false);
                m_pbuttonbarGamesHeader->SetChecked2(i, false);
            }
        }

        if (m_pbuttonbarGamesHeader->GetChecked2(nColumn))
        {
            m_pbuttonbarGamesHeader->SetChecked(nColumn,  true);
            m_pbuttonbarGamesHeader->SetChecked2(nColumn, false);
            bReverse = true;
        }
        else
        {
            m_pbuttonbarGamesHeader->SetChecked(nColumn,  false);
            m_pbuttonbarGamesHeader->SetChecked2(nColumn, true);
            bReverse = false;
        }

        // add this to the stack of sorting criteria
        int nIndex = m_vuSorts.Find(nColumn);

        if (nIndex != -1)
        {
            m_vuSorts.Remove(nIndex);
            m_vbReversedSorts.Remove(nIndex);
        }
        m_vuSorts.PushEnd(nColumn);
        m_vbReversedSorts.PushEnd(bReverse);

        UpdateGameList();

        return true;
    }

    // resets all the filters (to off)
    void ResetFilters()
    {
        m_sFilterDevelopments = DontCare;
        m_sFilterLives = DontCare;
        m_sFilterScoresCount = DontCare;
        m_sFilterZone = DontCare;

        m_sFilterConquest = DontCare;
        m_sFilterDeathmatch = DontCare;
        m_sFilterProsperity = DontCare;
        m_sFilterCountdown = DontCare;
        m_sFilterArtifact = DontCare;
        m_sFilterFlags = DontCare;
        m_sFilterTerritorial = DontCare;

        m_strGameNameSubstring.SetEmpty();

        // WLP 2005 - I added these next 4 lines to force the default sort to the number of players

        m_vuSorts.SetEmpty();              // WLP 2005 - clear it out
        m_vbReversedSorts.SetEmpty();      // WLP 2005 - Clear it out

        m_vuSorts.PushEnd(6);              // WLP 2005 - number of players magic number KGJV #114 7->6
        m_vbReversedSorts.PushEnd(true);  // we do want the big numbers on top(reversed)

        UpdateGameList();
    }

    class GameScreenFilter
    {
        GameScreen* m_pparent;

    public:
        GameScreenFilter(GameScreen* pparent)
            : m_pparent(pparent) {};

        bool operator () (ItemID pitem)
        {
            MissionInfo* pgame = (MissionInfo*)pitem;

            if (m_pparent->m_sFilterDevelopments != DontCare
                && (m_pparent->m_sFilterDevelopments == Yes)
                    != pgame->AllowDevelopments())
                return false;

            if (m_pparent->m_sFilterLives != DontCare
                && (m_pparent->m_sFilterLives == Yes)
                    != pgame->LimitedLives())
                return false;

            if (m_pparent->m_sFilterScoresCount != DontCare
                && (m_pparent->m_sFilterScoresCount == Yes)
                    != pgame->ScoresCount())
                return false;

            if (m_pparent->m_sFilterZone != DontCare
                && (m_pparent->m_sFilterZone == Yes)
                    != pgame->WasObjectModelCreated())
                return false;


            if (m_pparent->m_sFilterConquest != DontCare
                && (m_pparent->m_sFilterConquest == Yes)
                    != pgame->GoalConquest())
                return false;

            if (m_pparent->m_sFilterDeathmatch != DontCare
                && (m_pparent->m_sFilterDeathmatch == Yes)
                    != pgame->GoalDeathMatch())
                return false;

            if (m_pparent->m_sFilterProsperity != DontCare
                && (m_pparent->m_sFilterProsperity == Yes)
                    != pgame->GoalProsperity())
                return false;

            if (m_pparent->m_sFilterCountdown != DontCare
                && (m_pparent->m_sFilterCountdown == Yes)
                    != pgame->GoalCountdown())
                return false;

            if (m_pparent->m_sFilterArtifact != DontCare
                && (m_pparent->m_sFilterArtifact == Yes)
                    != pgame->GoalArtifacts())
                return false;

            if (m_pparent->m_sFilterFlags != DontCare
                && (m_pparent->m_sFilterFlags == Yes)
                    != pgame->GoalFlags())
                return false;

            if (m_pparent->m_sFilterTerritorial != DontCare
                && (m_pparent->m_sFilterTerritorial == Yes)
                    != pgame->GoalTerritory())
                return false;

            if (!m_pparent->m_strGameNameSubstring.IsEmpty()
                && (ZString(pgame->Name()).ToLower().Find(m_pparent->m_strGameNameSubstring.ToLower()) == -1))
                return false;

            return true;
        }
    };
    GameScreenFilter m_filter;

    static StatusColor MissionStatusColor(MissionInfo* game)
    {
        if (!game->GetAnySlotsAreAvailable())
        {
            return statusRed;
        }
        /*else if (!game->GetGuaranteedSlotsAreAvailable())
        {
            return statusYellow;
        }*/
        else
        {
            return statusGreen;
        }
    }

    static bool StatusCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        StatusColor status1 = MissionStatusColor(pgame1);
        StatusColor status2 = MissionStatusColor(pgame2);

        bool bHasPlayerSquad1 = trekClient.HasPlayerSquad(pgame1);
        bool bHasPlayerSquad2 = trekClient.HasPlayerSquad(pgame2);

        if (bHasPlayerSquad1 != bHasPlayerSquad2)
            return bHasPlayerSquad1 && !bHasPlayerSquad2;
        else if (status1 != status2)
            return status1 > status2;
        else if (pgame1->InProgress() != pgame2->InProgress())
            return !pgame1->InProgress();
        else if (pgame1->WasObjectModelCreated() != pgame2->WasObjectModelCreated())
            return pgame1->WasObjectModelCreated() && !pgame2->WasObjectModelCreated();
        else
            return (pgame1->GetStage() == STAGE_STARTED)
                && (pgame2->GetStage() != STAGE_STARTED);
    }

    static bool NameCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return _stricmp(pgame1->Name(), pgame2->Name()) > 0;
    }

    static bool RunningTimeCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        if (!pgame1->InProgress())
            return false;
        else if (!pgame2->InProgress())
            return true;
        else
            return pgame1->GetMissionParams().timeStart < pgame2->GetMissionParams().timeStart;
    }

    static bool SkillCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return (pgame1->GetMaxRank() > pgame2->GetMaxRank())
            || (pgame1->GetMaxRank() == pgame2->GetMaxRank()
                && pgame1->GetMinRank() == pgame2->GetMinRank());
    }

    static bool TeamCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return pgame1->NumSides() > pgame2->NumSides();
    }

    static bool MinPlayerCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return pgame1->MinPlayersPerTeam() > pgame2->MinPlayersPerTeam();
    }

    static bool MaxPlayerCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return pgame1->MaxPlayersPerTeam() > pgame2->MaxPlayersPerTeam();
    }
	//Imago 7/8/09
    static bool NumPlayerCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return pgame1->NumPlayers() > pgame2->NumPlayers();
    }
	// KGJV #114
    static bool ServerCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;
		const char * n1 = pgame1->GetMissionDef().szServerName;
		const char * n2 = pgame2->GetMissionDef().szServerName;
        return _stricmp(n1,n2) > 0;
    }
	// KGJV #114
    static bool CoreCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;
		ZString n1 = trekClient.CfgGetCoreName(pgame1->GetIGCStaticFile());
		ZString n2 = trekClient.CfgGetCoreName(pgame2->GetIGCStaticFile());
        return _stricmp(PCC(n1),PCC(n2)) > 0;
    }

    static bool NumPlayersCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return pgame1->NumPlayers() > pgame2->NumPlayers();
    }

    static int GameTypeIndex(MissionInfo* pgame)
    {
        TList<TRef<GameType> >::Iterator gameTypesIter(GameType::GetGameTypes());

        int nIndex = 0;
        while (!gameTypesIter.End()
            && !gameTypesIter.Value()->IsGameType(pgame->GetMissionParams()))
        {
            nIndex++;
            gameTypesIter.Next();
        }

        return nIndex;
    }

    static bool SettingsCompare(ItemID pitem1, ItemID pitem2)
    {
        MissionInfo* pgame1 = (MissionInfo*)pitem1;
        MissionInfo* pgame2 = (MissionInfo*)pitem2;

        return GameTypeIndex(pgame1) < GameTypeIndex(pgame2);
    }

    class ReverseCompare
    {
    public:

        ItemIDCompareFunction m_func;

        ReverseCompare(ItemIDCompareFunction func)
            : m_func(func) {};

        bool operator () (ItemID id1, ItemID id2)
        {
            return m_func(id2, id1);
        };
    };

    List* SortingList(List* list, ItemIDCompareFunction func, bool bReverse)
    {
        if (bReverse)
        {
            return new SortedList<ReverseCompare>(list, ReverseCompare(func));
        }
        else
        {
            return new SortedList<ItemIDCompareFunction>(list, func);
        }
    }

    void UpdateGameList()
    {
        List* plist = trekClient.GetMissionList();

        plist = new FilteredList<GameScreenFilter>(plist, m_filter);

        for (int i = 0; i < m_vuSorts.GetCount(); i++)
        {
            switch (m_vuSorts[i])
            {
            case 0:
                plist = SortingList(plist, StatusCompare, m_vbReversedSorts[i]);
                break;

            case 1:
                plist = SortingList(plist, NameCompare, m_vbReversedSorts[i]);
                break;

            case 2:
                plist = SortingList(plist, RunningTimeCompare, m_vbReversedSorts[i]);
                break;

            case 3:
                plist = SortingList(plist, SkillCompare, m_vbReversedSorts[i]);;
                break;

			// KGJV #114
            case 4:
                //plist = SortingList(plist, TeamCompare, m_vbReversedSorts[i]);
				plist = SortingList(plist, ServerCompare, m_vbReversedSorts[i]);
                break;

			// KGJV #114
            case 5:
                //plist = SortingList(plist, MinPlayerCompare, m_vbReversedSorts[i]);
				plist = SortingList(plist, CoreCompare, m_vbReversedSorts[i]);
                break;

            //case 6:
            //    plist = SortingList(plist, MaxPlayerCompare, m_vbReversedSorts[i]);
            //    break;

            case 6:
                plist = SortingList(plist, NumPlayersCompare, m_vbReversedSorts[i]);
                break;

            case 7:
                plist = SortingList(plist, SettingsCompare, m_vbReversedSorts[i]);
                break;
            }
        }

        m_plistPaneGames->SetList(plist);
    }


    int GetCountInLobby(List* plist)
    {
        int count = plist->GetCount();
        int cPlayers = 0;


        for  (int i = 0; i < count; i++) {

            MissionInfo* game = (MissionInfo*)plist->GetItem(i);

            cPlayers = game->NumPlayers() + cPlayers;

        }

		if (g_bQuickstart) {
        // close the "connecting..." popup
       		if (GetWindow()->GetPopupContainer() && !GetWindow()->GetPopupContainer()->IsEmpty())
            	GetWindow()->GetPopupContainer()->ClosePopup(NULL);
       	 	GetWindow()->RestoreCursor();

			//imago 7/5/09
			//if there are players in the lobby join the most populated server...
			//OutputDebugString("GetCountInLobby(plist) returned "+ ZString(cPlayers) +"\n");
			if (cPlayers) {
				 plist = SortingList(plist, NumPlayerCompare, true);
				 MissionInfo* game = (MissionInfo*)plist->GetItem(0);
				 //.. it's not a newb server and not our own game we're actually trying to insta create
				 ZString szPlayerName = ZString(trekClient.GetNameLogonZoneServer());
				 int leftParen = szPlayerName.ReverseFind('(',0);
				 if (leftParen > 1)
					szPlayerName = szPlayerName.Left(leftParen);
	        	 szPlayerName += ZString("'s game"); //reuse exact matches via. kgjv's adaptation
				 if ( (ZString(game->Name()).Find("newbie") == -1) && 
					  (ZString(game->Name()).Find(szPlayerName) == -1) ) {
					 ZDebugOutput("Insta join: "+ ZString(game->Name()) + "\n");					 
					 g_bQuickstart = false; //we're done with all that!
					 JoinMission(game);
				 }

			}
			//no other players? ;(  ...if you build it they will come!
			//trekClient.ServerListReq(); //quickstart keeps following into the new callback...
		}
        return cPlayers;
    }

    bool OnListChanged()
    {
        m_pnumberPlayerCount->SetValue(GetCountInLobby(trekClient.GetMissionList()));
        return true;
    }

    bool OnButtonBack()
    {
        GetWindow()->screen(ScreenIDZoneClubScreen);
        return false;
    }

    bool OnButtonJoin()
    {
        MissionInfo* pmission = (MissionInfo*)m_plistPaneGames->GetSelection();

        JoinMission(pmission);

        return true;
    }

    bool OnButtonDetails()
    {
        MissionInfo* pmission = (MissionInfo*)m_plistPaneGames->GetSelection();

        if (!pmission->GetDetailsFiles().IsEmpty())
            BeginEventDetailsDownload(pmission->GetDetailsFiles());

        return true;
    }

    void JoinMission(MissionInfo * pMissionInfo, const char * szMissionPassword = "")
    {
        m_strLastPassword = szMissionPassword;

        if (pMissionInfo)
        {
            trekClient.JoinMission(pMissionInfo, szMissionPassword);
        }
    }

    bool OnButtonNewGame()
    {
		// KGJV #114
		// ask lobby for the core/server list
		// reply will call OnServersList
		trekClient.ServerListReq();
        //trekClient.CreateMissionReq();
        return true;
    }

    /*
    bool OnButtonDetails()
    {
        TRef<IMessageBox> pmsgBox =
            CreateMessageBox("NYI - part of the create game sceen work.");
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        return true;
    }
    */

    bool OnButtonFilter()
    {
        GetWindow()->GetPopupContainer()->OpenPopup(m_pfilterDialog, false);
        return true;
    }


    bool OnButtonFindPlayer()
    {
        GetWindow()->GetPopupContainer()->OpenPopup(m_pfindDialog, false);
        return true;
    }


    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Game Events
    //
    //////////////////////////////////////////////////////////////////////////////

	//KGJV #114
	void OnServersList(int cCores, char *Cores, int cServers, char *Servers)
	{
		if (!GetWindow()->GetPopupContainer()->IsEmpty())
            GetWindow()->GetPopupContainer()->ClosePopup(NULL);
        GetWindow()->RestoreCursor();

		StaticCoreInfo *pcores   = (StaticCoreInfo*)Cores;
		ServerCoreInfo *pservers = (ServerCoreInfo*)Servers;

#ifdef _DEBUG
		debugf("got OnServersList: %d, %d\n",cCores,cServers);
		for (int i=0; i<cCores; i++)
		{
			debugf("  core %s\n",pcores[i].cbIGCFile);
		}
		for (int i=0; i<cServers; i++)
		{
			debugf("  server %d\n",i);
			debugf("    name = %s\n",pservers[i].szName);
			debugf("    addr = %s\n",pservers[i].szRemoteAddress);
			debugf("    loca = %s\n",pservers[i].szLocation);
			debugf("    curg = %d\n",pservers[i].iCurGames);
			debugf("    magx = %d\n",pservers[i].iMaxGames);
			debugf("    mask = %x\n",pservers[i].dwCoreMask);
		}
#endif
		//Imago #78 7/5/09 only automatically create games on the #1 server & core 
		if (g_bQuickstart) {
			if(cServers !=0 && cCores !=0) {
				// KGJV's adaption for game name reused
				ZString szPlayerName = ZString(trekClient.GetNameLogonZoneServer());
				int leftParen = szPlayerName.ReverseFind('(',0);
				if (leftParen > 1)
					szPlayerName = szPlayerName.Left(leftParen);
	        	szPlayerName += ZString("'s game");
				//7/7/09 use official only -Imago
				for (int i=0; i<cServers; i++)
				{
					for (int i2=0; i2<cCores; i2++)
					{
						if (trekClient.CfgIsOfficialServer(pservers[i].szName,pservers[i].szRemoteAddress) &&
							trekClient.CfgIsOfficialCore(pcores[i2].cbIGCFile)) 
						{	
							ZDebugOutput("Insta new game: on "+ZString(pservers[i].szName,pservers[i].szName)+" core "+ pcores[i2].cbIGCFile+"\n");
							trekClient.CreateMissionReq(pservers[i].szName,
							pservers[i].szRemoteAddress,
							pcores[i2].cbIGCFile,szPlayerName);
							return;  //follow quickstart into teamscreen..preferably we're going on a server that's SRVLOG or DEBUG
						}
					}
				}
				g_bQuickstart = false; //fine, no official server? just let them 
				return;					//sit at the empty lobby list, maybe they will create a non official game 7/9/09
			} else {
				g_bQuickstart = false; //fine, can't create a game? (no servers or cores) just let them 
				return;					//sit at the empty lobby list
			}
		} else {
			//do the normal thing and raise the create game server/core dialog
			m_pcreateDialog->OnServersList(cCores, pcores, cServers, pservers);
			GetWindow()->GetPopupContainer()->OpenPopup(m_pcreateDialog, false);
		}
	}
    void OnMissionEnded(MissionInfo* pmission)
    {
        MissionInfo* pmissionSelected = (MissionInfo*)m_plistPaneGames->GetSelection();

        if (pmissionSelected == pmission)
        {
            // update the join button
            OnSelectMission(pmission);
        }
    }

    void OnFoundPlayer(MissionInfo* pMissionInfo)
    {
        if (!GetWindow()->GetPopupContainer()->IsEmpty())
            GetWindow()->GetPopupContainer()->ClosePopup(NULL);
        GetWindow()->RestoreCursor();

        if (pMissionInfo == NULL)
        {
            TRef<IMessageBox> pmsgBox =
                CreateMessageBox("The player was not found.");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
        else
        {
            ResetFilters();
            m_plistPaneGames->SetSelection(pMissionInfo);
            m_plistPaneGames->ScrollToItem(pMissionInfo);

            TRef<IMessageBox> pmsgBox =
                CreateMessageBox("Player found!");
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
    }

    void OnAddPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        if (pMissionInfo && pPlayerInfo == trekClient.MyPlayerInfo())
        {
            // go to the team lobby...
            GetWindow()->screen(ScreenIDTeamScreen);
        }
    }

    void OnDelRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason)
    {
        if (pPlayerInfo == trekClient.MyPlayerInfo())
        {
            // rejected !!  Bad password?
            if (!GetWindow()->GetPopupContainer()->IsEmpty())
                GetWindow()->GetPopupContainer()->ClosePopup(NULL);
            GetWindow()->RestoreCursor();

            bool bDisconnect = true;

            if (reason == DPR_BadPassword)
            {
                MissionInfo* pmissionSelected = (MissionInfo*)m_plistPaneGames->GetSelection();

                TRef<IPopup> ppopupPassword = CreatePasswordPopup(this, m_strLastPassword, pmissionSelected);
                GetWindow()->GetPopupContainer()->OpenPopup(ppopupPassword, false);
                bDisconnect = false;
            }
            else if (reason == DPR_NoMission)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The mission you were trying to join has ended.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_LobbyLocked)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The lobby for this mission is locked.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_InvalidRank)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("You are the wrong rank for this mission.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_OutOfLives)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("You have run out of lives in this mission.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_Banned)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("You have been removed (i.e. booted) from this game by the commander(s)."); // mmf 09/07 was "You have been banned from this mission."
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_GameFull)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("This server has reached its maximum number of players.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_PrivateGame)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The game you tried to join is private.  You do not have access to this game.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_DuplicateLogin)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("Someone else is already using that name in this game.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else if (reason == DPR_ServerPaused)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The server for this game is shutting down and is not accepting new users.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            else
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("You have not been accepted into the game.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }

            if (bDisconnect)
                trekClient.Disconnect();
        }
    }

    virtual void OnCancelPassword()
    {
        trekClient.Disconnect();
    }

    virtual void OnPassword(ZString strPassword)
    {
        MissionInfo* pmissionSelected = (MissionInfo*)m_plistPaneGames->GetSelection();
        JoinMission(pmissionSelected, strPassword);
    }

    // Gets the name for a given min and max skill level
    ZString FindSkillLevelName(MissionInfo* pmission)
    {
		// KG- keep compiler happy (C4838) 
        float fValues[2] = { (float)pmission->GetMinRank(), (float)pmission->GetMaxRank() };
        const char* vszNames[2] = { "SkillLevelMin", "SkillLevelMax" };

        return FindStringValue(FindClosestValue(fValues, vszNames, 2), "SkillLevelNames");
    };

    // extract the string with the given index from the given table in mdl
    // (used for skill level)
    ZString FindStringValue(int index, const char* szTableName)
    {
        IObjectList* plist;

        CastTo(plist, m_pns->FindMember(szTableName));

        plist->GetFirst();

        while (index > 0) {
            plist->GetNext();
            --index;
        }

        return GetString(plist->GetCurrent());
    };

    // find the index of the closest value in a set of mdl lists
    // (used for skill level)
    int FindClosestValue(float fValue[], const char* szTableName[], int nLists)
    {
        TVector<IObjectList*> plists;

        {
            for (int nList = 0; nList < nLists; nList++)
            {
                IObjectList* plist;
                CastTo(plist, m_pns->FindMember(szTableName[nList]));
                plists.PushEnd(plist);
                plists[nList]->GetFirst();
            }
        }

        int indexClosest = 0;
        float fDistanceClosest = 1e10f;

        int index = 0;
        while (plists[0]->GetCurrent() != NULL) {

            float fDistance = 0;

            for (int nList = 0; nList < nLists; nList++)
            {
                fDistance += fabs(fValue[nList] - GetNumber(plists[nList]->GetCurrent()));
                plists[nList]->GetNext();
            }

            if (fDistance < fDistanceClosest)
            {
                fDistanceClosest = fDistance;
                indexClosest = index;
            }

            ++index;
        }

        return indexClosest;
    };

    void OnLogonGameServer()
    {
        // wait for a join message.
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox =
            CreateMessageBox("Joining mission....", NULL, false, false);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    }

    void OnLogonGameServerFailed(bool bRetry, const char* szReason)
    {
        // tell the user why the logon failed
        TRef<IMessageBox> pmsgBox = CreateMessageBox(szReason);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    }

    void OnLogonLobby()
    {
        // display any pending logoff error messages.
        trekClient.FlushSessionLostMessage();
    }

    void OnLogonLobbyFailed(bool bRetry, const char* szReason)
    {
        // send the user back to the intro screen and tell them why it failed.
        GetWindow()->screen(ScreenIDIntroScreen);
        TRef<IMessageBox> pmsgBox = CreateMessageBox(szReason);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    }

    void OnFrame()
    {
        if (m_pSession)
        {
            // if the download has completed or an error occured...
            if (!m_pSession->ContinueDownload())
            {
                if (!m_strPendingFileList.IsEmpty())
                {
                    DownloadEventDetails(m_strPendingFileList);
                    m_strPendingFileList.SetEmpty();
                }
                else
                {
                    // close the session
                    delete m_pSession;
                    m_pSession = NULL;
                }
            }
        }
    }

    void DownloadEventDetails(const ZString& strFileList)
    {
        GetWindow()->SetWaitCursor();

        // Generate the file list

        ZString* vstrFileList = new ZString[2*strFileList.GetLength() + 1];
        PCC* vszFileList = new PCC[2*strFileList.GetLength() + 1];

        ZString strFileListRemaining = strFileList;
        int i = 0;
        while (!strFileListRemaining.IsEmpty())
        {
            int nIndex;

            // find and read in the first file name
            for (nIndex = 0; nIndex < strFileListRemaining.GetLength()
                && strFileListRemaining[nIndex] != ' '
                && strFileListRemaining[nIndex] != ','
                && strFileListRemaining[nIndex] != '"';
                ++nIndex)
            {};

            ZString strFile = strFileListRemaining.Left(nIndex);

            // skip past any remaining white space or commas
            for (; nIndex < strFileListRemaining.GetLength()
                && (strFileListRemaining[nIndex] == ' '
                  || strFileListRemaining[nIndex] == ','
                  || strFileListRemaining[nIndex] == '"');
                ++nIndex)
            {};

            strFileListRemaining = strFileListRemaining.RightOf(nIndex);

            // if we hit leading whitespace in the list, recover from that
            if (strFile.IsEmpty())
                continue;

            // add the file in question to the list of files to download
            // (we have two arrays so that the memory in the ZStrings sticks around)
            vstrFileList[i*2] = trekClient.GetCfgInfo().strZoneEventDetailsURL + strFile;
            vstrFileList[i*2 + 1] = strFile;

            vszFileList[i*2] = vstrFileList[i*2];
            vszFileList[i*2 + 1] = vstrFileList[i*2 + 1];

            // make sure we unload this so we can write to it.
            GetModeler()->UnloadNameSpace(strFile.LeftOf("."));
            m_strWaitForFile = strFile;

            i++;
        }
        vszFileList[i*2] = NULL;

        // start the download

        if (!m_pSession)
            m_pSession = CreateHTTPSession(this);

        m_pSession->InitiateDownload(vszFileList, trekClient.GetArtPath());


        // clean up

        delete [] vszFileList;
        delete [] vstrFileList;
    }

    void OnError(char * szErrorMessage) // on HTTP download error
    {
        // Errors are essentially ignored
        debugf("Error while trying to get a Zone Events description file: %s", szErrorMessage);

        TRef<IMessageBox> pmessagebox =
            CreateMessageBox("Failed to retrieve the event description.");
        GetWindow()->GetPopupContainer()->OpenPopup(pmessagebox);

        // don't try to do a pending transaction after an error
        m_strPendingFileList.SetEmpty();
    }

    bool IsFileValid(char * szFileName)
    {
        ZFile file(szFileName);

        int n = file.GetLength(); // -1 means error
        if (n != -1 && n != 0)
        {
            char * pData = new char[n+1];
            memcpy(pData, file.GetPointer(), n);
            pData[n] = 0;

            if (strstr(pData, "<html") != NULL
                || strstr(pData, "<HTML") != NULL
                || strstr(pData, "<Html") != NULL)
            {
                debugf("Found '<html' in %s; assumed failure.\n", szFileName);
                delete[] pData;
                return false;
            }
            else
            {
                delete[] pData;
                return true;
            }
        }
        else
        {
            debugf("File %s error while trying to load event details in the games list.\n", szFileName);
            return false;
        }
    }

    bool OnFileCompleted(char * szFileName)
    {
        debugf("Downloaded file: %s\n", szFileName);

        char szPath[MAX_PATH+20];
        strcpy(szPath, m_pSession->GetDownloadPath());
        strcat(szPath, szFileName);

        if (!IsFileValid(szPath))
        {
            debugf("Download failed.");

            if (m_nRetriesLeft)
            {
                // try again
                --m_nRetriesLeft;
                return false;
            }
            else
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("Unable download event info from the Zone.  Please try again later.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
        }
        else
        {
            if (szFileName == m_strWaitForFile)
            {
                GetWindow()->GetPopupContainer()->OpenPopup(
                            CreateMMLPopup(
                                GetModeler(),
                                m_strWaitForFile,
                                false
                            )
                        );
                GetWindow()->RestoreCursor();
            }
        }

        // reset the retry counter
        m_nRetriesLeft = c_cRetries;

        return true; // true means don't retry download
    }

    void BeginEventDetailsDownload(const ZString& strFileList)
    {
        assert(!strFileList.IsEmpty());

        if (m_pSession)
        {
            m_strPendingFileList = strFileList;
        }
        else
        {
            DownloadEventDetails(strFileList);
        }
    }
};

GameScreen::BooleanFilterState GameScreen::m_sFilterDevelopments;
GameScreen::BooleanFilterState GameScreen::m_sFilterLives;
GameScreen::BooleanFilterState GameScreen::m_sFilterScoresCount;
GameScreen::BooleanFilterState GameScreen::m_sFilterZone;

GameScreen::BooleanFilterState GameScreen::m_sFilterConquest;
GameScreen::BooleanFilterState GameScreen::m_sFilterDeathmatch;
GameScreen::BooleanFilterState GameScreen::m_sFilterProsperity;
GameScreen::BooleanFilterState GameScreen::m_sFilterCountdown;
GameScreen::BooleanFilterState GameScreen::m_sFilterArtifact;
GameScreen::BooleanFilterState GameScreen::m_sFilterFlags;
GameScreen::BooleanFilterState GameScreen::m_sFilterTerritorial;

ZString GameScreen::m_strGameNameSubstring;

TVector<unsigned, DefaultEquals> GameScreen::m_vuSorts;
TVector<bool, DefaultEquals>     GameScreen::m_vbReversedSorts;

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateGameScreen(Modeler* pmodeler)
{
    return new GameScreen(pmodeler);
}
