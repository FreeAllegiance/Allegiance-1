#include "pch.h"
#include "training.h"
#include "badwords.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

// KGJV #62
#include "mappreview.h"
//////////////////////////////////////////////////////////////////////////////
//
// Team Screen
//
//////////////////////////////////////////////////////////////////////////////

// KGJV #104 : 27 march 2006 - changes to remove the modal popup while waiting to join a team
// some 'extra checks' cleanup may be required after beta test
// search "KGJV #104" to find all changes

int g_civIDStart = -1;
extern bool g_bDisableNewCivs;
extern bool g_bAFKToggled; // Imago: Manual AFK toggle flag

class TeamScreen :
    public Screen,
    public IItemEvent::Sink,
    public IEventSink,
    public TEvent<ZString>::Sink,
    public EventTargetContainer<TeamScreen>,
    public TrekClientEventSink,
	public IMenuCommandSink // #ALLY RMB sink
{
private:
    TRef<Pane>       m_ppane;

	bool m_bRipChecked;
    SideID m_sideCurrent;
    SideID m_sideToJoin;
    SideID m_lastToJoinSend; // KGKV #104: last m_sideToJoin send to server (extra control).

	TRef<MapPreviewPane> m_ppaneMapPreview; // KGJV #62
    TRef<ButtonPane>	 m_pbuttonsTeam[6]; // KGJV #62
	TRef<List>           m_plistSides; // KGJV #62
	MissionInfo* m_pMission;

    int               m_secondsOld;

    TRef<WrapImage>             m_pwrapImageCiv;
    TRef<ModifiableNumber>      m_pnumberShowSquad;
    TRef<ModifiableNumber>      m_pnumberStatsCount;
    TRef<ModifiableNumber>      m_pnumberSquadStatsCount;
    TRef<ModifiableColorValue>  m_pcolorCiv;
    TRef<IKeyboardInput>        m_pkeyboardInputOldFocus;
    TRef<IItemEvent::Sink>      m_psinkTeams;
    TRef<IItemEvent::Sink>      m_psinkPlayers;
    TRef<TEvent<ZString>::Sink> m_psinkChatEdit;
    TRef<IEventSink>            m_peventSinkTimer;

    TRef<ComboPane> m_pcomboWing;

    TRef<EditPane> m_peditPane;

    TRef<ButtonPane> m_pbuttonBack;
    TRef<ButtonPane> m_pbuttonGameOver;
    TRef<ButtonPane> m_pbuttonStart;
    TRef<ButtonPane> m_pbuttonAwayFromKeyboard;
    TRef<ButtonPane> m_pbuttonTeamReady;
    TRef<ButtonPane> m_pbuttonAutoAccept;
    TRef<ButtonPane> m_pbuttonLockLobby;
    TRef<ButtonPane> m_pbuttonLockSides;
    TRef<ButtonPane> m_pbuttonRandomize;
    TRef<ButtonPane> m_pbuttonAccept;
    TRef<ButtonPane> m_pbuttonReject;
    TRef<ButtonPane> m_pbuttonJoin;
    TRef<ButtonPane> m_pbuttonSend;
    TRef<ButtonPane> m_pbuttonDetails;
    TRef<ButtonPane> m_pbuttonSettings;
    TRef<ButtonPane> m_pbuttonTeamSettings;
    TRef<ButtonPane> m_pbuttonMakeLeader;
	TRef<ButtonPane> m_pbuttonDiscord;

    TRef<ButtonBarPane> m_pbuttonbarPlayers;
    TRef<ButtonBarPane> m_pbuttonbarChat;

    TRef<ListPane>   m_plistPanePlayers;
    TRef<ListPane>   m_plistPaneTeams;

    TRef<ChatListPane> m_pchatPane;

    TRef<StringPane> m_ptextTitle;
    TRef<StringPane> m_ptextPrompt;
    TRef<StringPane> m_ptextStatus;
    TRef<StringPane> m_ptextStatus2;
    TRef<StringPane> m_ptextTimer;
    TRef<StringPane> m_ptextTimerType;

    TRef<IItemEvent::Source> m_peventPlayers;
    TRef<IItemEvent::Source> m_peventTeams;
    TRef<TEvent<ZString>::Source> m_peventChatEdit;

    TRef<IMessageBox> m_pmsgBox;

	// #ALLY - RMB menu
	TRef<IMenuCommandSink>     m_pmenuCommandSink;
	TRef<IMenu>                m_pmenu;

    float m_fColorIntensity;
    float m_fColorBrightness;

    bool m_vbUncheckedMessages[3];
    bool m_bDetailsChanged;
    bool m_bShowingRandomizeWarning;

    ChatTarget      m_chattargetChannel;

    enum PlayerSort
    {
        sortLeader,
        sortPlayerName,
        sortPlayerRank,
        sortPlayerStatus,
        sortPlayerWing
    } m_sortPlayers;
    bool m_bReverseSort;

    TVector<int>    m_viPlayerColumns;
    TVector<int>    m_viTeamColumns;

    bool            m_fQuiting;

    int             m_nCountdown;

	int CivilizationArray[30]; //Xynth #170 8/2010
	int numCivilizations;
	Time	lastRandCivUpdate;

    class TeamPainter : public ItemPainter
    {
        MissionInfo* m_pMission;
        SideID *m_pSideToJoin; // KGJV #104

        TRef<Image> m_pimageArrow;
        TRef<Image> m_pimageTab;
        TRef<Image> m_pimageSelectedTab;
		TRef<Image> m_pimageTabEdges[c_cAlliancesMax]; //#ALLY
        TRef<Image> m_pimageSelectedTabEdges[c_cAlliancesMax];//#ALLY


        float m_fColorIntensity;
        float m_fColorBrightness;

        int m_nHeight;
        const TVector<int>& m_viColumns;

    public:
        // KGJV #104 - new param pSideToJoin
        TeamPainter(MissionInfo* pMission, SideID *pSideToJoin, const TVector<int>& viColumns, int nHeight, float fColorIntensity, float fColorBrightness) : 
            m_pMission(pMission),
            m_pSideToJoin(pSideToJoin), // KGJV #104
            m_viColumns(viColumns), 
            m_nHeight(nHeight),
            m_fColorIntensity(fColorIntensity),
            m_fColorBrightness(fColorBrightness)
        {
            
            m_pimageArrow = GetModeler()->LoadImage(AWF_TEAMROOM_PLAYERS_TEAM_ARROW, true);
            //m_pimageTab = GetModeler()->LoadImage(AWF_TEAMROOM_TAB_MASK, true);

			// #ALLY 
			m_pimageTabEdges[0] = GetModeler()->LoadImage("green_edgebmp",true);
            m_pimageSelectedTabEdges[0]= GetModeler()->LoadImage("greensel_edgebmp",true);
            m_pimageTabEdges[1] = GetModeler()->LoadImage("orange_edgebmp",true);
            m_pimageSelectedTabEdges[1]= GetModeler()->LoadImage("orangesel_edgebmp",true);
            m_pimageTabEdges[2] = GetModeler()->LoadImage("pink_edgebmp",true);
            m_pimageSelectedTabEdges[2]= GetModeler()->LoadImage("pinksel_edgebmp",true);
            
            //m_pimageSelectedTab = GetModeler()->LoadImage(AWF_TEAMROOM_SELECTED_TAB_MASK, true);
        };

        int GetXSize()
        {
            return m_viColumns[4];
        }

        int GetYSize()
        { 
            return m_nHeight;
        }

        WinPoint GetJoinPoint()
        {
            //return coordinates for join button
            return WinPoint(m_viColumns[2], m_nHeight * 1/2 - 8);
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            SideInfo* pitem = (SideInfo*)pitemArg;

            Color color;
            IsideIGC* pside = NULL;
            
            if (trekClient.GetCore())
                pside = trekClient.GetCore()->GetSide(pitem->GetSideID());
            int pnumberside = pitem->GetSideID();
            
			// KGJV #114 - remove empty teams
			if (!m_pMission->SideActive(pitem->GetSideID()) && m_pMission->GetStage() != STAGE_STARTED)
				return;

            switch (pnumberside)
                {
                case 0:
                    m_pimageSelectedTab = GetModeler()->LoadImage("teamselyelbmp", true);
                    m_pimageTab = GetModeler()->LoadImage("teamyelbmp", true);
                    break;

                case 1:
                    m_pimageSelectedTab = GetModeler()->LoadImage("teamselbluebmp", true);
                    m_pimageTab = GetModeler()->LoadImage("teambluebmp", true);
                    break;

                case 2:
                    m_pimageSelectedTab = GetModeler()->LoadImage("teamselpurplebmp", true);
                    m_pimageTab = GetModeler()->LoadImage("teampurplebmp", true);
                    break;

                case 3:
                    m_pimageSelectedTab = GetModeler()->LoadImage("teamselgreenbmp", true);
                    m_pimageTab = GetModeler()->LoadImage("teamgreenbmp", true);
                    break;

                case 4:
                    m_pimageSelectedTab = GetModeler()->LoadImage("teamselredbmp", true);
                    m_pimageTab = GetModeler()->LoadImage("teamredbmp", true);
                    break;

                case 5:
                    m_pimageSelectedTab = GetModeler()->LoadImage("teamselwhitebmp", true);
                    m_pimageTab = GetModeler()->LoadImage("teamwhitebmp", true);
                    break;

                default:
                    m_pimageSelectedTab = GetModeler()->LoadImage("teamselgreybmp", true);
                    m_pimageTab = GetModeler()->LoadImage("teamgreybmp", true);
                    break;
                }
            

			// #ALLY - temporary UI
			Color AllianceColors[3] = { Color::Orange(), Color::Green(), Color::Red() };

			if (!pside)
                color = m_fColorBrightness * 0.75 * Color::White() * m_fColorIntensity + 0.5 * Color::White() * (1 - m_fColorIntensity);
            else
			{
                color = m_fColorBrightness * pside->GetColor() * m_fColorIntensity + 0.5 * Color::White() * (1 - m_fColorIntensity);
			}
            if (bSelected) 
            {
				//Imago 7/8/09 reordered draw order
				psurface->BitBlt(WinPoint(0,0), m_pimageSelectedTab->GetSurface());
                // draw the selected tab outline
				if (pside->GetAllies()!=NA) // #ALLY
					//psurface->FillRect(WinRect::Cast(m_pimageSelectedTab->GetBounds().GetRect()), AllianceColors[pside->GetAllies()]*0.75);
					psurface->BitBlt(WinPoint(0,0), m_pimageSelectedTabEdges[pside->GetAllies()]->GetSurface());
                
            }
            else
            {
				//Imago 7/8/09 reordered draw order
				psurface->BitBlt(WinPoint(0,0), m_pimageTab->GetSurface());
                // draw the selected tab outline
				if (pside->GetAllies()!=NA) // #ALLY
					//psurface->FillRect(WinRect::Cast(m_pimageTab->GetBounds().GetRect()), AllianceColors[pside->GetAllies()]*0.75);
					psurface->BitBlt(WinPoint(0,0), m_pimageTabEdges[pside->GetAllies()]->GetSurface());
                
            }

            // KGJV #104 - paint the arrow if we request to join this team
            if (*m_pSideToJoin == pnumberside)
            {
                psurface->BitBlt(WinPoint(-2,-4), m_pimageArrow->GetSurface());
            }

			// #ALLY - debug display
#if 0
			if (pitem->GetSideID() != SIDE_TEAMLOBBY)
			{
				ZString strAllies("Solo");
				if (m_pMission->SideAllies(pitem->GetSideID()) != NA)
						strAllies = "G"+ZString(m_pMission->SideAllies(pitem->GetSideID()));

				psurface->DrawString(
                    TrekResources::LargeBoldFont(),
                    Color::White(),
                    WinPoint(m_viColumns[0] + 55, m_nHeight * 1/2 - 8),
                    ZString(trekClient.GetCore()->GetSide(pitem->GetSideID())->GetAllies()) + strAllies
                );
			}
#endif
            // if this team is empty...
            if (m_pMission->SideNumPlayers(pitem->GetSideID()) == 0 
                && pitem->GetSideID() != SIDE_TEAMLOBBY)
            {
                // draw "available" or "destroyed"
                ZString strDescription;
                
                if (m_pMission->SideAvailablePositions(pitem->GetSideID()) > 0
                    && m_pMission->SideActive(pitem->GetSideID()))
                {
                    strDescription = "available";
                }
                else
                {
					strDescription = "destroyed";
                }

                psurface->DrawString(
                    TrekResources::SmallFont(),
                    Color::White(),
                    WinPoint(m_viColumns[1] - 15, m_nHeight * 1/2 - 8),
                    strDescription
                );
            }
            else
            {
				//	yp - Your_Persona Team total rank in lobby patch Aug-04-2006
				//  mmf - modified to not show if total is zero
				// Add up the sum of all the players ranks.
				const ShipListIGC* mp_ships = pside->GetShips();
				int teamTotalRank = 0;
				for (const ShipLinkIGC* lShip = mp_ships->first(); lShip; lShip = lShip->next())
				{
					IshipIGC* pship = lShip->data();
					PlayerInfo* pplayer = (PlayerInfo*)pship->GetPrivateData();		            
					// teamTotalRank += (pplayer->GetPersistScore(NA).GetRank() < 1) ? 1 : pplayer->GetPersistScore(NA).GetRank();	// TE: Modified this to add a minimum of 5 per player
					teamTotalRank += pplayer->GetPersistScore(NA).GetRank(); // mmf this is just for display purposes, undoing TE's above modification
				}
				// end yp

                // draw the team name
				int idy = (pitem->GetSideID() >= 0)?-4:0;  // KGJV #62 if faction name, move team name up 4px
                WinRect rectClipOld = psurface->GetClipRect();
                psurface->SetClipRect(WinRect(WinPoint(m_viColumns[0], 0), WinPoint(m_viColumns[1] - 4, GetYSize()))); // clip name to fit in column
                psurface->DrawString(
                    TrekResources::SmallFont(),
                    Color::White(),
                    WinPoint(m_viColumns[0], m_nHeight * 1/2 - 8 +idy), // KGJV #62 
                    CensorBadWords (m_pMission->SideName(pitem->GetSideID())).ToUpper()
                );
				// KGJV #62 - draw the faction name under team name
				// faction name is under team and shifted +4px to the right
				// Xynth #170 8/2010, Set text to random for random civs
				if (pitem->GetSideID() >= 0)
				{
					if (trekClient.GetCore()->GetSide(pitem->GetSideID())->GetRandomCivilization())
                psurface->DrawString(
                    TrekResources::SmallFont(),
                    Color::White(),
                    WinPoint(m_viColumns[0]+4, m_nHeight * 1/2 -2),
						"Random"
						);
					else
						psurface->DrawString(
						TrekResources::SmallFont(),
						Color::White(),
						WinPoint(m_viColumns[0]+4, m_nHeight * 1/2 -2),
                    trekClient.GetCore()->GetSide(pitem->GetSideID())->GetCivilization()->GetName()
                );
				}
                psurface->RestoreClipRect(rectClipOld);

                // draw the available positions
                char cbPositions[80];
                if (pitem->GetSideID() == SIDE_TEAMLOBBY)
                {
                    TRef<List> plistPlayers = pitem->GetMemberList();
					if (teamTotalRank > 0)
                        wsprintf(cbPositions, "(%d)[%d]", plistPlayers->GetCount(), teamTotalRank);//	yp - Your_Persona Team total rank in lobby patch Aug-04-2006
					else
						wsprintf(cbPositions, "(%d)", plistPlayers->GetCount());
                }
                else
                {
					if (teamTotalRank > 0)
						wsprintf(cbPositions, "(%d/%d)[%d]", //	yp - Your_Persona Team toal rank in lobby patch Aug-04-2006
							m_pMission->SideNumPlayers(pitem->GetSideID()),
							m_pMission->SideMaxPlayers(pitem->GetSideID()),
							teamTotalRank);//	yp - Your_Persona Team toal rank in lobby patch Aug-04-2006
					else
						wsprintf(cbPositions, "(%d/%d)",
							m_pMission->SideNumPlayers(pitem->GetSideID()),
							m_pMission->SideMaxPlayers(pitem->GetSideID()));
                }
                psurface->DrawString(
                    TrekResources::SmallFont(),
                    Color::White(),
                    WinPoint(m_viColumns[1], m_nHeight * 1/2 - 8), 
                    cbPositions
                );
            }
        }
    };

    class PlayerPainter : public ItemPainter
    {
        MissionInfo* m_pMission;
        const TVector<int>& m_viColumns;
        const SideID& m_sideCurrent;

    public:

        PlayerPainter(MissionInfo* pMission, const TVector<int>& viColumns, const SideID& sideCurrent)
            : m_viColumns(viColumns), m_sideCurrent(sideCurrent), m_pMission(pMission) {};

        int GetXSize()
        {
            return m_viColumns[5];
        }

        int GetYSize()
        {
            return 14;
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitemArg));

            if (bSelected) {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(100, 0, 0)
                );
            }

            if (pplayer == NULL)
                return;

            bool bRequesting = m_sideCurrent != NA && pplayer->SideID() != m_sideCurrent;
            Color color = (bRequesting && TeamScreen::s_bFlash) ? (0.75f * Color::White()) : Color::White();
            bool bOnLobbySide = pplayer->SideID() == SIDE_TEAMLOBBY;

            // draw the crown icon for leaders
        
            if (!bRequesting)
            {
                TRef<Image> pimageCheck = GetModeler()->LoadImage("lobbycheckbmp", true);   
                if (pplayer->IsTeamLeader())
                {
                    WinPoint pntCheck = WinPoint(m_viColumns[1] + 2,
                        (GetYSize() - (int)pimageCheck->GetBounds().GetRect().YSize())/2
                        );
                    psurface->BitBlt(pntCheck, pimageCheck->GetSurface());   
                }

                if (pplayer->IsMissionOwner())
                {
                    WinPoint pntCheck2 = WinPoint(m_viColumns[0] + 2,
                        (GetYSize() - (int)pimageCheck->GetBounds().GetRect().YSize())/2
                        );
                    psurface->BitBlt(pntCheck2, pimageCheck->GetSurface());
                }
            }              

            // draw the player name
            
            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(0, 0), WinPoint(m_viColumns[0], GetYSize()))); // clip name to fit in column
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(0, 0),
                pplayer->CharacterName()
            );
            psurface->RestoreClipRect(rectClipOld);


            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[2] + 2, 0), WinPoint(m_viColumns[3], GetYSize()))); // clip rank to fit in column
            // draw the player rank
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(m_viColumns[2] + 2, 0),
                ZString(" (") + ZString(pplayer->GetPersistScore(NA).GetRank()) + ZString(") ") + ZString(trekClient.LookupRankName(pplayer->GetPersistScore(NA).GetRank(), pplayer->GetCivID()))
            );
            psurface->RestoreClipRect(rectClipOld);


            // draw their current status

            ZString strStatus;

            if (bRequesting)
                strStatus = "Requesting";
            else if (!pplayer->IsReady())
                strStatus = "AFK";
            else if (m_pMission->InProgress())
                strStatus = bOnLobbySide ? "Not Playing" : "Playing";
            else
                strStatus = "Ready";

            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[3] + 2, 0), WinPoint(m_viColumns[4], GetYSize()))); // clip status to fit in column
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(m_viColumns[3] + 2, 0),
                strStatus
            );
            psurface->RestoreClipRect(rectClipOld);

			// if they are on our team, draw the player's wing
			if (pplayer->SideID() == trekClient.GetSideID() && !bOnLobbySide)
			{
				// BT - 7/15 - When the team screen is shown, miners have -1 for the wingID, which causes alleg to crash.
				int wingID = pplayer->GetShip()->GetWingID();
				if (wingID != NA)
				{
					psurface->DrawString(
						TrekResources::SmallFont(),
						color,
						WinPoint(m_viColumns[4] + 2, 0),
						c_pszWingName[wingID]
					);
				}
			}
        }
    };

    class TeamIconImage : public Image
    {
    private:

        TRef<Image>             m_pimageBkgnd;
        TRef<Image>             m_pimageCivIcon;

    public:

        TeamIconImage()
        {
            m_pimageBkgnd = Image::GetEmpty();
            m_pimageCivIcon = Image::GetEmpty();
        }

        void CalcBounds()
        {
            m_bounds = m_pimageBkgnd->GetBounds();
        }

        void SetSide(SideID sideId)
        {
            switch (sideId)
            {
            case 0:
                m_pimageBkgnd = GetModeler()->LoadImage("iconteamyellowbmp", true);
                break;

            case 1:
                m_pimageBkgnd = GetModeler()->LoadImage("iconteambluebmp", true);
                break;

            case 2:
                m_pimageBkgnd = GetModeler()->LoadImage("iconteampurplebmp", true);
                break;

            case 3:
                m_pimageBkgnd = GetModeler()->LoadImage("iconteamgreenbmp", true);
                break;

            case 4:
                m_pimageBkgnd = GetModeler()->LoadImage("iconteamredbmp", true);
                break;

            case 5:
                m_pimageBkgnd = GetModeler()->LoadImage("iconteamwhitebmp", true);
                break;
            }
        }

        void SetCivilization(IcivilizationIGC* pciv)
        {
            m_pimageCivIcon = GetModeler()->LoadImage(
                ZString("icon") + ZString(pciv->GetIconName()).ToLower() + ZString("bmp"), true
                );
        }

        void Render(Context* pcontext)
        {
            // draw the background
            Surface* psurfaceBackground = m_pimageBkgnd->GetSurface();
            if (psurfaceBackground)
                pcontext->DrawImage(psurfaceBackground);

            // draw the civ icon
            Surface* psurfaceCivIcon = m_pimageCivIcon->GetSurface();
            if (psurfaceCivIcon)
                pcontext->DrawImage(psurfaceCivIcon);
        }
    };

    class MissionBriefPopup : 
        public IPopup, 
        public EventTargetContainer<MissionBriefPopup>
    {
    private:
        TRef<Pane> m_ppane;
        TRef<ModifiableString> m_strTitle;
        TRef<ModifiableString> m_strBody;
		TRef<ModifiableString> m_strTips; //AEM 10.13.07
		TRef<ModifiableString> m_strString; //AEM 10.13.07
		TRef<ModifiableString> m_strMap; //AEM 10.13.07
        TRef<ModifiableNumber> m_numCountdown;
        TRef<TeamIconImage> m_pimageTeamIcon;

    public:

        ZString GetString(const ZString& strName, TRef<INameSpace> pns)
        {
            TRef<StringValue> pval;
            CastTo(pval, pns->FindMember(strName));
            return pval->GetValue();
        }

        ZString GetCivString(const ZString& strName, TRef<INameSpace> pns)
        {
            return GetString(trekClient.GetSide()->GetCivilization()->GetIconName() + strName, pns);
        }

        ZString GetCivListString()
        {
            TVector<CivID, DefaultEquals> vectCivs;

            for (SideID id = 0; id < trekClient.MyMission()->NumSides(); id++)
            {
                if (id != trekClient.GetSideID())
                {
                    // if we don't have the civ yet, add it to the list.
                    CivID civId = trekClient.GetCore()->GetSide(id)->GetCivilization()->GetObjectID();

                    if (-1 == vectCivs.Find(civId))
                    {
                        vectCivs.PushEnd(civId);
                    }
                }
            }

            assert(vectCivs.GetCount() >= 1);
            
            ZString strResult;

            for (int nCiv = 0; nCiv < vectCivs.GetCount() - 1; nCiv++)
            {
                if (!strResult.IsEmpty())
                    strResult += ", ";
                strResult += trekClient.GetCore()->GetCivilization(vectCivs[nCiv])->GetName();
            }

            if (vectCivs.GetCount() > 1)
                strResult += " and ";

            strResult += trekClient.GetCore()->GetCivilization(vectCivs.GetEnd())->GetName();

            return strResult;
        }

        ZString GetMissionBriefingBodyText(TRef<INameSpace> pns)
        {
            if (trekClient.GenerateCivBriefing())
                return GetCivMissionBriefing(pns);
            else
                return trekClient.GetBriefingText();
        }
		
		//AEM 10.13.07 Display the map name above the map preview on the briefing
		ZString GetMissionMapType(TRef<INameSpace> pns)
        {
            const MissionParams& missionparams = trekClient.MyMission()->GetMissionParams();
			if (missionparams.szCustomMapFile[0] == '\0') 
			{
				switch ( missionparams.mmMapType )
				{
					case 0:
						return "Single Ring";
					case 1:
						return "Double Ring";
					case 2:
						return "Pinwheel";
					case 3:
						return "Diamond Ring";
					case 4:
						return "Snowflake";
					case 5:
						return "Split Bases";
					case 6:
						return "Brawl";
					case 7:
						return "Big Ring";
					case 8:
						return "HiLo";
					case 9:
						return "HiHigher";
					case 10:
						return "Star";
					case 11:
						return "InsideOut";
					case 12:
						return "Grid";
					case 13:
						return "EastWest";
					case 14:
						return "LargeSplit";
					default:
						return "Unknown map name";
				}				
			} else 
			{
				int mapNum;
				for (int i = 0; i < trekClient.GetNumStaticMaps(); i++)
				{
					if (strcmp(missionparams.szCustomMapFile, trekClient.GetStaticMapInfo(i).cbIGCFile) == 0)
						mapNum = i;
				}
				return trekClient.GetStaticMapInfo(mapNum).cbFriendlyName;
			}
        }

        ZString GetCivMissionBriefing(TRef<INameSpace> pns)
        {
            ZString strResult;
            const int nMaxText = 2048;
            char cbTemp[nMaxText];
            
            ZString strMainText;
            const MissionParams& misparams = trekClient.MyMission()->GetMissionParams();

            if (misparams.IsNoWinningConditionsGame())
                strMainText = GetCivString("NoWin", pns);
            else 
                strMainText = GetCivString("Default", pns);

            _snprintf(cbTemp, nMaxText, strMainText, (const char*)CensorBadWords (trekClient.GetSide()->GetName()), (const char*)trekClient.GetBriefingText(), (const char*)GetCivListString());
            strResult = cbTemp;

            if (misparams.IsArtifactsGame() || misparams.IsFlagsGame() || misparams.IsCountdownGame() 
                || (misparams.IsConquestGame() && misparams.GetConquestPercentage() != 100)
                || misparams.IsDeathMatchGame() || misparams.IsProsperityGame())
            {
                strResult += GetCivString("SecondaryObjectives", pns);

                if (misparams.IsConquestGame() && misparams.GetConquestPercentage() != 100)
                {
                    _snprintf(cbTemp, nMaxText, GetCivString("Conquest", pns), misparams.GetConquestPercentage());
                    strResult += cbTemp;
                }

                if (misparams.IsArtifactsGame())
                {
                    _snprintf(cbTemp, nMaxText, GetCivString("Artifacts", pns), misparams.nGoalArtifactsCount);
                    strResult += cbTemp;
                }

                if (misparams.IsFlagsGame())
                {
                    _snprintf(cbTemp, nMaxText, GetCivString("Flags", pns), misparams.nGoalFlagsCount);
                    strResult += cbTemp;
                }

                if (misparams.IsCountdownGame())
                {
                    _snprintf(cbTemp, nMaxText, GetCivString("Countdown", pns), (int)(misparams.GetCountDownTime() / 60));
                    strResult += cbTemp;
                }

                if (misparams.IsDeathMatchGame())
                {
                    _snprintf(cbTemp, nMaxText, GetCivString("Deathmatch", pns), misparams.GetDeathMatchKillLimit());
                    strResult += cbTemp;
                }

                if (misparams.IsProsperityGame())
                {
                    _snprintf(cbTemp, nMaxText, GetCivString("Prosperity", pns), (int)(misparams.fGoalTeamMoney * trekClient.GetCore()->GetFloatConstant(c_fcidWinTheGameMoney)));
                    strResult += cbTemp;
                }
            }
            
            return strResult;
        }
        
        ZString GetMissionBriefingTitle(TRef<INameSpace> pns)
        {
            return GetCivString("Title", pns);
        }

		//AEM 10.13.07 Used to get "Objective" or similar message from the mdl
		ZString GetMissionBriefingString(TRef<INameSpace> pns)
        {
            return GetCivString("String", pns);
        }

		//AEM 10.13.07 Find a random tip to display for the Tip of the Game
		ZString GetMissionTip(TRef<INameSpace> pns)
        {
			ZString numTipsStr = GetString("numTips", pns);
			int numTipsInt = numTipsStr.GetInteger();
			srand(time(NULL));
			int randTip = rand() % numTipsInt + 1;
			ZString getTip = "Tip";
			getTip += randTip;
			return GetString(getTip, pns);
        }
        
        MissionBriefPopup()
        {
            TRef<INameSpace> pnsMissionBriefingData = GetModeler()->CreateNameSpace("missionbriefdata");
            pnsMissionBriefingData->AddMember("missionBriefingBodyText", m_strBody = new ModifiableString("<bug>"));
			pnsMissionBriefingData->AddMember("missionBriefingTipText", m_strTips = new ModifiableString("<bug>")); //AEM 10.13.07 Used to display Tip of the Game
			pnsMissionBriefingData->AddMember("missionBriefingStringText", m_strString = new ModifiableString("<bug>")); //AEM 10.13.07 Used to display "Objective" or equivalent message under the faction name
            pnsMissionBriefingData->AddMember("missionBriefingTitleText", m_strTitle = new ModifiableString("<bug>"));
			pnsMissionBriefingData->AddMember("missionBriefingMapType", m_strMap = new ModifiableString("<bug>")); //AEM 10.13.07 Get the map name to display
            pnsMissionBriefingData->AddMember("countdown", m_numCountdown = new ModifiableNumber(-1));
            pnsMissionBriefingData->AddMember("teamImage", (Value*)(m_pimageTeamIcon = new TeamIconImage()));
			
            m_pimageTeamIcon->SetSide(trekClient.GetSideID());
            m_pimageTeamIcon->SetCivilization(trekClient.GetSide()->GetCivilization());

            TRef<INameSpace> pns = GetModeler()->GetNameSpace("missionbrief");

            CastTo(m_ppane, pns->FindMember("MissionBriefDialog"));

			m_strString->SetValue(GetMissionBriefingString(pns)); //AEM 10.8.07
            m_strBody->SetValue(GetMissionBriefingBodyText(pns));
            m_strTitle->SetValue(GetMissionBriefingTitle(pns));
			m_strMap->SetValue(GetMissionMapType(pns)); //AEM 10.8.07
			m_strTips->SetValue(GetMissionTip(pns)); //AEM 10.8.07

            GetModeler()->UnloadNameSpace("missionbrief");
        }

        void SetCountdown(int nSeconds)
        {
            m_numCountdown->SetValue((float)nSeconds);
        }

        //
        // IPopup methods
        //

        Pane* GetPane()
        {
            return m_ppane;
        }
    };

    class TeamSettingsDialogPopup : 
        public IPopup, 
        public EventTargetContainer<TeamSettingsDialogPopup>,
        public TrekClientEventSink
    {
    private:
        TRef<Pane> m_ppane;
        TRef<ButtonPane> m_pbuttonOk;
        TRef<ButtonPane> m_pbuttonCancel;
        TRef<EditPane>  m_peditPaneTeamName;
        TRef<ComboPane> m_pcomboCiv;
        TRef<ComboPane> m_pcomboSquad;

        TeamScreen* m_pparent;

        TRef<IKeyboardInput> m_pkeyboardInputOldFocus;		
		int CivilizationArray[30]; //Xynth #170 8/2010
		int numCivilizations;
    public:

        TeamSettingsDialogPopup(TRef<INameSpace> pns, TeamScreen* pparent)
        {
            m_pparent = pparent;

            CastTo(m_ppane, pns->FindMember("TeamSettingsDialog"));
            CastTo(m_pbuttonOk, pns->FindMember("okButtonPane"));
            CastTo(m_pbuttonCancel, pns->FindMember("cancelButtonPane"));
            CastTo(m_peditPaneTeamName, (Pane*)pns->FindMember("teamNameEditPane"));
            CastTo(m_pcomboCiv,                pns->FindMember("civComboPane"));
            CastTo(m_pcomboSquad,              pns->FindMember("squadComboPane"));

			// mdvalley: Search for '&TeamScreen::' to see so many pointers copied in.
            AddEventTarget(&TeamScreen::TeamSettingsDialogPopup::OnButtonOk, m_pbuttonOk->GetEventSource());
            AddEventTarget(&TeamScreen::TeamSettingsDialogPopup::OnButtonCancel, m_pbuttonCancel->GetEventSource());

            m_peditPaneTeamName->SetMaxLength(c_cbName - 1);

            int index = g_civStart;
			numCivilizations = 0;			
            for (CivilizationLinkIGC* linkCiv = trekClient.GetCore()->GetCivilizations()->first(); 
                linkCiv != NULL; linkCiv = linkCiv->next())
            {
                if (index == 0) {
                    g_civIDStart = linkCiv->data()->GetObjectID();    
                }
                index--;
                if (trekClient.MyPlayerInfo()->GetShip()->GetPilotType() == c_ptCheatPlayer 
                    || linkCiv->data()->GetObjectID() < 300)        
				{
                    m_pcomboCiv->AddItem(linkCiv->data()->GetName(), linkCiv->data()->GetObjectID());
					numCivilizations++; //Xynth #170 8/2010 fill an array of faction IDs to pick one at random
					CivilizationArray[numCivilizations-1] = linkCiv->data()->GetObjectID();					
            }
            }

			//Xynth #170 removed selection until bugs can be worked out
			//m_pcomboCiv->AddItem("Random", RANDOM_ID); //Xynth #170 8/2010

            m_peditPaneTeamName->SetHidden(true);
        }

        //
        // IPopup methods
        //
        
        void OnAddMission(MissionInfo* pMissionDef)
        {
            // if this is my mission...
        
            if (pMissionDef == trekClient.MyMission())
            {
                // Update the buttons
                UpdateControls();

                // if the team settings are locked, dismiss the dialog without making changes
                if (pMissionDef->GetMissionParams().bLockTeamSettings)
                    OnButtonCancel();
            }
        }

        void OnDelPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParamParam) 
        {
            // if this is me...
            if (m_pparent->m_bTeamSettingsPopupOpen
                && trekClient.MyPlayerInfo()->ShipID() == pPlayerInfo->ShipID())
            {
                // close the popup
                OnButtonCancel();
                debugf("team settings popup closed because of del player\n");
            }
        }

        void OnTeamNameChange(MissionInfo* pMissionInfo, SideID sideID)
        {
            if (sideID == trekClient.GetSideID())
            {
                UpdateNameAndSquad();
            }
        }

        void UpdateControls()
        {
            m_peditPaneTeamName->SetHidden(trekClient.MyMission()->GetMissionParams().bSquadGame);
            m_pcomboSquad->SetHidden(!trekClient.MyMission()->GetMissionParams().bSquadGame);
            
        }

        void UpdateNameAndSquad()
        {
            m_peditPaneTeamName->SetString(trekClient.MyMission()->SideName(trekClient.GetSideID()));
            
            if (trekClient.GetSide()->GetSquadID() != NA)
                m_pcomboSquad->SetSelection(trekClient.GetSide()->GetSquadID());
        }

        virtual void OnClose()
        {
            if (m_pkeyboardInputOldFocus)
                GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
            m_pkeyboardInputOldFocus = NULL;
            m_pparent->m_bTeamSettingsPopupOpen = false;

            IPopup::OnClose();
            debugf("team settings popup closed\n");
        }

        virtual void SetContainer(IPopupContainer* pcontainer)
        {      
            m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
            IPopup::SetContainer(pcontainer);
            GetEngineWindow()->SetFocus(m_peditPaneTeamName);
            m_pparent->m_bTeamSettingsPopupOpen = true;

            // add the squads list
            m_pcomboSquad->ClearContents();
            
            for (TList<SquadMembership>::Iterator iterSquad(trekClient.GetSquadMemberships());
                !iterSquad.End(); iterSquad.Next())
            {
                if (iterSquad.Value().GetIsLeader() || iterSquad.Value().GetIsAssistantLeader()
                       || iterSquad.Value().GetID() == trekClient.GetSide()->GetSquadID())
                    m_pcomboSquad->AddItem(iterSquad.Value().GetName(), iterSquad.Value().GetID());
            }

            UpdateNameAndSquad();
            UpdateControls();
			if (trekClient.GetSide()->GetRandomCivilization())
				m_pcomboCiv->SetSelection(RANDOM_ID);
			else
            m_pcomboCiv->SetSelection(trekClient.GetSide()->GetCivilization()->GetObjectID());
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
                OnButtonOk();
                return true;
            }
            else
                return ((IKeyboardInput*)m_peditPaneTeamName)->OnChar(pprovider, ks);
        }

        Pane* GetPane()
        {
            return m_ppane;
        }
        
        bool OnButtonOk()
        {
            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }
            GetWindow()->RestoreCursor();

            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmSetTeamInfo, CS, SET_TEAM_INFO)
            END_PFM_CREATE
            pfmSetTeamInfo->sideID = trekClient.GetSideID();

            if (trekClient.MyMission()->GetMissionParams().bSquadGame)
            {
                pfmSetTeamInfo->squadID = m_pcomboSquad->GetSelection();
                pfmSetTeamInfo->SideName[0] = '\0';
            }
            else
            {
                pfmSetTeamInfo->squadID = NA;
                strcpy(pfmSetTeamInfo->SideName, m_peditPaneTeamName->GetString());
            }
			//Xynth #170 8/2010
			int civSelection, randIndex;
			bool bRandomCiv;
			if (m_pcomboCiv->GetSelection() == RANDOM_ID)
			{
				bRandomCiv = true;
				randIndex = randomInt(0,numCivilizations - 1); //random faction
				civSelection = CivilizationArray[randIndex];
			}
			else
			{
				civSelection = m_pcomboCiv->GetSelection();
				bRandomCiv = false;
			}


            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmChangeCiv, CS, CHANGE_TEAM_CIV)
            END_PFM_CREATE
            pfmChangeCiv->iSide = trekClient.GetSideID(); 
            pfmChangeCiv->civID = civSelection;
			pfmChangeCiv->random = bRandomCiv;

            return true;
        }

        bool OnButtonCancel()
        {
            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else if (m_pcontainer) {
                m_pcontainer->ClosePopup(this);
            }
            GetWindow()->RestoreCursor();

            return true;
        }
    };

    TRef<MissionBriefPopup>  m_pmissionBriefPopup;
    TRef<TeamSettingsDialogPopup>  m_pteamSettingsPopup;
    
    static int s_nLastSelectedChatTab;

public:
    bool m_bTeamSettingsPopupOpen;
    static bool s_bFlash;

    // filter out drones
    static bool IsHuman(ItemID pitem)
    {
        PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem));

        if (pplayer == NULL || trekClient.MyMission() == NULL)
            return false;

        return pplayer->IsHuman();
    }

public:
    TeamScreen(Modeler* pmodeler) :         
        m_pMission(trekClient.MyMission()), 
        m_fQuiting(false),
        m_sortPlayers(sortLeader),
        m_bReverseSort(false),
        m_bDetailsChanged(false),
        m_bTeamSettingsPopupOpen(false),
        m_bShowingRandomizeWarning(false),
        m_sideToJoin(NA),
        m_lastToJoinSend(NA),//KGJV #104
		m_bRipChecked(false) 
    {
        TRef<IObject> pobjPlayerColumns;
        TRef<IObject> pobjTeamColumns;
        TRef<Number>  pnumTeamColorIntensity;
		TRef<Number>  pnumTeamColorBrightness; //Imago 7/10 #15

        //
        // Teamscreen exports
        //

        TRef<INameSpace> pnsTeamScreenData = GetModeler()->CreateNameSpace("teamscreendata");
        pnsTeamScreenData->AddMember("civImage", (Value*)(m_pwrapImageCiv = new WrapImage(Image::GetEmpty())));
        pnsTeamScreenData->AddMember("showSquad", m_pnumberShowSquad = new ModifiableNumber(0));
        pnsTeamScreenData->AddMember("statsCount", m_pnumberStatsCount = new ModifiableNumber(0));
        pnsTeamScreenData->AddMember("squadStatsCount", m_pnumberSquadStatsCount = new ModifiableNumber(0));
        pnsTeamScreenData->AddMember("civColor", m_pcolorCiv = new ModifiableColorValue(Color::Black()));
		//KGJV added: core name & server name export 
		// so we can use strServerName and strCoreName in teamscreen.mdl
		const char* szServerName = trekClient.MyMission()->GetMissionDef().szServerName;
		const char* szServerAddr = trekClient.MyMission()->GetMissionDef().szServerAddr;
		pnsTeamScreenData->AddMember("strServerName", new StringValue(szServerName));
		const char* szCoreName = trekClient.MyMission()->GetMissionParams().szIGCStaticFile;
		pnsTeamScreenData->AddMember("strCoreName", new StringValue(trekClient.CfgGetCoreName(szCoreName)));

		//
        // Load the members from MDL
        //

        TRef<INameSpace> pns = pmodeler->GetNameSpace("teamscreen");

        CastTo(m_ppane,                    pns->FindMember("screen"              ));
        CastTo(pobjPlayerColumns,          pns->FindMember("playerColumns"       ));
        CastTo(pobjTeamColumns,            pns->FindMember("teamColumns"         ));
        CastTo(pnumTeamColorIntensity,     pns->FindMember("teamColorIntensity"  ));
        CastTo(pnumTeamColorBrightness,     pns->FindMember("teamColorBrightness"  ));
        CastTo(m_pbuttonBack,              pns->FindMember("backButtonPane"      ));
        CastTo(m_pbuttonGameOver,          pns->FindMember("gameOverButtonPane"  ));
        CastTo(m_pbuttonStart,             pns->FindMember("startButtonPane"      ));
        //CastTo(m_pbuttonReady,             pns->FindMember("readyButtonPane"     ));
        CastTo(m_pbuttonAwayFromKeyboard,  pns->FindMember("awayFromKeyboardButtonPane"));
        CastTo(m_pbuttonTeamReady,         pns->FindMember("sideReadyButtonPane" ));
        CastTo(m_pbuttonAutoAccept,        pns->FindMember("autoAcceptButtonPane"));
        CastTo(m_pbuttonLockLobby,         pns->FindMember("lockLobbyButtonPane"));
        CastTo(m_pbuttonLockSides,         pns->FindMember("lockSidesButtonPane"));
        CastTo(m_pbuttonRandomize,         pns->FindMember("randomizeButtonPane"));
        CastTo(m_pbuttonAccept,            pns->FindMember("acceptButtonPane"    ));
        CastTo(m_pbuttonReject,            pns->FindMember("rejectButtonPane"    ));
        CastTo(m_pbuttonJoin,              pns->FindMember("joinButtonPane"      ));
        CastTo(m_pbuttonbarPlayers,        pns->FindMember("playerListHeader"    ));
        CastTo(m_pbuttonbarChat,           pns->FindMember("chatHeader"          ));
        CastTo(m_pbuttonSend,              pns->FindMember("sendButtonPane"      ));
        CastTo(m_pbuttonDetails,           pns->FindMember("detailsButtonPane"   ));
        CastTo(m_pbuttonSettings,          pns->FindMember("settingsButtonPane"  ));
        CastTo(m_pbuttonTeamSettings,      pns->FindMember("teamSettingsButtonPane"  ));
        CastTo(m_pbuttonMakeLeader,        pns->FindMember("makeLeaderButtonPane"));
        CastTo(m_plistPanePlayers,  (Pane*)pns->FindMember("playerListPane"      ));
        CastTo(m_plistPaneTeams,    (Pane*)pns->FindMember("teamListPane"        ));
        CastTo(m_pcomboWing,               pns->FindMember("wingComboPane"       ));
        CastTo(m_peditPane,         (Pane*)pns->FindMember("chatEditPane"        ));
        CastTo(m_pchatPane,         (Pane*)pns->FindMember("chatText"            ));
        CastTo(m_ptextTitle,        (Pane*)pns->FindMember("textTitle"           ));
        CastTo(m_ptextPrompt,       (Pane*)pns->FindMember("textPrompt"          ));
        CastTo(m_ptextStatus,       (Pane*)pns->FindMember("textStatus"          ));
        CastTo(m_ptextStatus2,      (Pane*)pns->FindMember("textStatus2"         ));
        CastTo(m_ptextTimer,        (Pane*)pns->FindMember("textTimer"           ));
        CastTo(m_ptextTimerType,    (Pane*)pns->FindMember("textTimerType"       ));
		CastTo(m_pbuttonDiscord,           pns->FindMember("discordButtonPane"      ));

		// KGJV #62
		CastTo(m_ppaneMapPreview ,  (Pane*)pns->FindMember("mapPreviewPane"));

		// KGJV #62
		CastTo(m_pbuttonsTeam[0],pns->FindMember("activeTeamButtonPane1"));
		CastTo(m_pbuttonsTeam[1],pns->FindMember("activeTeamButtonPane2"));
		CastTo(m_pbuttonsTeam[2],pns->FindMember("activeTeamButtonPane3"));
		CastTo(m_pbuttonsTeam[3],pns->FindMember("activeTeamButtonPane4"));
		CastTo(m_pbuttonsTeam[4],pns->FindMember("activeTeamButtonPane5"));
		CastTo(m_pbuttonsTeam[5],pns->FindMember("activeTeamButtonPane6"));


        m_pteamSettingsPopup = new TeamSettingsDialogPopup(pns, this);

        pmodeler->UnloadNameSpace("teamscreen");

        GetWindow()->SetChatListPane(m_pchatPane);
        m_fColorIntensity = pnumTeamColorIntensity->GetValue();
        m_fColorBrightness = pnumTeamColorBrightness->GetValue(); //Imago #15 7/10

        //
        // Text messages
        // 
        
        UpdateTitleText();
        UpdatePromptText();
        UpdateStatusText();

        //
        // EditPane
        //

        m_peventChatEdit = m_peditPane->GetChangeEvent();
        m_peventChatEdit->AddSink(m_psinkChatEdit = new TEvent<ZString>::Delegate(this));
        m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
        GetEngineWindow()->SetFocus();
        GetEngineWindow()->SetFocus(m_peditPane);


        //
        // combo boxes
        //

        for (WingID wingID = 0; wingID < c_widMax; wingID++)
        {
            m_pcomboWing->AddItem(
                c_pszWingName[wingID],
                wingID                
            );
        }
		
		//Imago 7/10 #149 #91
		int wid = trekClient.GetSavedWingAssignment();
		if (m_pcomboWing->GetSelection() != 0 || m_pcomboWing->GetSelection() == NULL) {
			if (wid == NA) {
					wid = 1; //default a newbs wing to attack if they dont have one saved
					trekClient.SaveWingAssignment(wid); //sends it
			}
		}
		if (!trekClient.GetPlayerInfo()->IsTeamLeader()) { //server will set comm's wings... Imago 8/10
		m_pcomboWing->SetSelection(wid);
			trekClient.SetWing(wid);
		}
		//

		AddEventTarget(&TeamScreen::OnWingCombo, m_pcomboWing->GetEventSource());

        //
        // Buttons
        //

		// prevent allow allied rip from being checked if it was explicitly changed

		bool bAllies = false;
		for (SideID i = 0; i < trekClient.MyMission()->GetSideList()->GetCount() ; i++) {
			if (trekClient.MyMission()->SideAllies(i) != NA) {
				bAllies = true;
				break;
			}
		}

		if (bAllies && !trekClient.MyMission()->GetMissionParams().bAllowAlliedRip)
			m_bRipChecked = true;

		if (!bAllies)
			m_bRipChecked = false;

        AddEventTarget(&TeamScreen::OnButtonBack, m_pbuttonBack->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonGameOver, m_pbuttonGameOver->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonStart, m_pbuttonStart->GetEventSource());
        //AddEventTarget(OnButtonReady, m_pbuttonReady->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonAwayFromKeyboard, m_pbuttonAwayFromKeyboard->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonTeamReady, m_pbuttonTeamReady->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonAutoAccept, m_pbuttonAutoAccept->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonLockLobby, m_pbuttonLockLobby->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonLockSides, m_pbuttonLockSides->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonRandomize, m_pbuttonRandomize->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonAccept, m_pbuttonAccept->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonReject, m_pbuttonReject->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonJoin, m_pbuttonJoin->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonBarChat, m_pbuttonbarChat->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonBarPlayers, m_pbuttonbarPlayers->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonSend, m_pbuttonSend->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonDetails, m_pbuttonDetails->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonDetails, m_pbuttonSettings->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonTeamSettings, m_pbuttonTeamSettings->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonMakeLeader, m_pbuttonMakeLeader->GetEventSource());
        m_pbuttonGameOver->SetHidden(trekClient.GetNumEndgamePlayers() == 0);
		// KGJV #62
        AddEventTarget(&TeamScreen::OnButtonToggleTeam0, m_pbuttonsTeam[0]->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonToggleTeam1, m_pbuttonsTeam[1]->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonToggleTeam2, m_pbuttonsTeam[2]->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonToggleTeam3, m_pbuttonsTeam[3]->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonToggleTeam4, m_pbuttonsTeam[4]->GetEventSource());
        AddEventTarget(&TeamScreen::OnButtonToggleTeam5, m_pbuttonsTeam[5]->GetEventSource());

		AddEventTarget(&TeamScreen::OnButtonDiscord, m_pbuttonDiscord->GetEventSource());

        //
        // Lists
        //

        ParseIntVector(pobjPlayerColumns, m_viPlayerColumns);
        m_plistPanePlayers->SetItemPainter(new PlayerPainter(m_pMission, m_viPlayerColumns, m_sideCurrent));
        m_peventPlayers = m_plistPanePlayers->GetSelectionEventSource();
        m_peventPlayers->AddSink(m_psinkPlayers = new IItemEvent::Delegate(this));

        ParseIntVector(pobjTeamColumns, m_viTeamColumns);
        m_peventTeams = m_plistPaneTeams->GetSelectionEventSource();
        m_peventTeams->AddSink(m_psinkTeams = new IItemEvent::Delegate(this));
		// KGJV #62
		m_plistSides = new SortedList<ItemIDCompareFunction>(m_pMission->GetSideList(), SideCompare);
        m_plistPaneTeams->SetList(
            new ConcatinatedList(
                m_plistSides,// KGJV #62
                new SingletonList(m_pMission->GetSideInfo(SIDE_TEAMLOBBY))
                )
            );
        m_plistPaneTeams->UpdateLayout();
        m_plistPaneTeams->SetItemPainter(new TeamPainter(
            m_pMission,
            &m_sideToJoin,// KGJV #104
            m_viTeamColumns, 
            m_plistPaneTeams->YSize() / (c_cSidesMax + 1), 
            m_fColorIntensity,
            m_fColorBrightness
            ));
        
        m_plistPaneTeams->SetSelection(m_pMission->GetSideInfo(trekClient.GetSideID()));

            
        AddEventTarget(&TeamScreen::OnTeamDoubleClicked, m_plistPaneTeams->GetDoubleClickEventSource());
        AddEventTarget(&TeamScreen::OnTeamClicked, m_plistPaneTeams->GetSingleClickEventSource());

		// #ALLY
		//   RMB popup menu on team
		AddEventTarget(&TeamScreen::OnTeamRightClicked, m_plistPaneTeams->GetSingleRightClickEventSource());
		//   RMB sink
		m_pmenuCommandSink  = IMenuCommandSink::CreateDelegate(this);

        // WLP 2005 - added line below to trap click on player
        AddEventTarget(&TeamScreen::OnPlayerClicked, m_plistPanePlayers->GetSingleClickEventSource()); // WLP - click selects
		AddEventTarget(&TeamScreen::OnPlayerRightClicked, m_plistPanePlayers->GetSingleRightClickEventSource()); // yp
		AddEventTarget(&TeamScreen::OnPlayerDoubleRightClicked, m_plistPanePlayers->GetDoubleRightClickEventSource()); // yp
        //
        // Chat pane
        //

        if (trekClient.GetSideID() == SIDE_TEAMLOBBY )
        {
            m_pbuttonbarChat->SetSelection(0);
            OnButtonBarChat(0);
        }
        else
        {
            m_pbuttonbarChat->SetSelection(1);
            OnButtonBarChat(1);
        }

        UpdateButtonStates();

        m_peventSinkTimer = IEventSink::CreateDelegate(this);
        GetEngineWindow()->GetTimer()->AddSink(m_peventSinkTimer, 0.25f);
        
        memset(&m_vbUncheckedMessages, 0, sizeof(m_vbUncheckedMessages));

        if (s_nLastSelectedChatTab != NA && m_pbuttonbarChat->GetEnabled(s_nLastSelectedChatTab))
        {
            m_pbuttonbarChat->SetSelection(s_nLastSelectedChatTab);
            OnButtonBarChat(s_nLastSelectedChatTab);
        }

        s_nLastSelectedChatTab = NA;

		if (g_bQuickstart) {
#ifdef DEBUG //Imago 7/5/09 quick start /w new public game, insta launch is debug only
			m_sideCurrent = 0;
		   if (m_sideCurrent != trekClient.GetSideID() 
		            && m_sideCurrent != SIDE_TEAMLOBBY
		            && m_pMission->SideAvailablePositions(m_sideCurrent) > 0
					&& m_pMission->SideActive(m_sideCurrent))
		   {
	            // try to join the current side
	            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
	            BEGIN_PFM_CREATE(trekClient.m_fm, pfmPositionReq, C, POSITIONREQ)
	            END_PFM_CREATE
	            pfmPositionReq->iSide = m_sideCurrent; 

	            m_sideToJoin = m_sideCurrent;
	            m_lastToJoinSend = m_sideToJoin;
	            UpdateButtonStates();
	            UpdatePromptText();
		   }

			SendChat("fourteen");
#endif
            g_bQuickstart = false;
        }

        if (trekClient.MyMission()->GetMissionParams().bSquadGame)
            m_pnumberShowSquad->SetValue(1);
        else
            m_pnumberShowSquad->SetValue(0);

		// KGJV: changed logic, ScoresCount is not from parameters anymore
		/*
        if (trekClient.MyMission()->GetMissionParams().bScoresCount)
            m_pnumberStatsCount->SetValue(1);
        else
            m_pnumberStatsCount->SetValue(0);
		*/
		if (trekClient.CfgIsOfficialServer(szServerName,szServerAddr) &&
			trekClient.CfgIsOfficialCore(szCoreName))
			m_pnumberStatsCount->SetValue(1);
		else
			m_pnumberStatsCount->SetValue(0);

        UpdateSquadsStatsMessage();

    }

    ~TeamScreen()
    {
        if (m_bTeamSettingsPopupOpen)
        {
            GetWindow()->GetPopupContainer()->ClosePopup(m_pteamSettingsPopup);
            GetWindow()->RestoreCursor();
        }

        m_peventTeams->RemoveSink(m_psinkTeams);
        m_peventPlayers->RemoveSink(m_psinkPlayers);
        m_peventChatEdit->RemoveSink(m_psinkChatEdit);

        if (m_peventSinkTimer)
            GetEngineWindow()->GetTimer()->RemoveSink(m_peventSinkTimer);

        GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
        GetWindow()->SetChatListPane(NULL);
    }

    SideID GetCurrentSide()
    {
        return m_sideCurrent;
    }

    void SendChat(const ZString& strChat)
    {
        PlayerInfo* pplayer;
        if (trekClient.ParseShellCommand(strChat))
        {
            // nothing more to do
        }
        else if (_stricmp(strChat.LeftOf(":"), "admin") == 0)
        {
            trekClient.SendChat(trekClient.GetShip(), CHAT_ADMIN, NA,
                                NA, (const char*)strChat.RightOf(":"));
        }
        else if (NULL != (pplayer = trekClient.FindPlayer(strChat.LeftOf(":"))))
        {
            trekClient.SendChat(trekClient.GetShip(), CHAT_INDIVIDUAL, pplayer->ShipID(),
                                NA, (const char*)strChat.RightOf(":"));
        }

        // WLP 2005 - added this to send chat to a highlighted player
        //   This only works when highlighted – when highlight is off it works normal
        // Imago 7/10 - it's wasn't entirely true (had issues)  --^
        else if ( m_plistPanePlayers->GetSelection())  // if something is selected now - 
														//#11 7/10 Imago ^-- this hits on edge cases even when something is no logner selected 
		{
			
			//Imago #11 7/10
			ObjectID oRecip = GetWindow()->GetLobbyChatRecip();
			// convert selected player to a ship id we can use to chat with
			ShipID shipID_S = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
			PlayerInfo* pplayer_S = trekClient.FindPlayer(shipID_S);
			//if a valid player selection and not ourself (most cases)
			if (pplayer_S && (pplayer_S->ShipID() != trekClient.GetShipID()) && (pplayer_S->ShipID() != NA))
				trekClient.SendChat(trekClient.GetShip(), CHAT_INDIVIDUAL, pplayer_S->ShipID(), NA, (const char*)strChat);
			//if the LCT is set (edge cases)
			else if (oRecip != NA)
				trekClient.SendChat(trekClient.GetShip(), CHAT_INDIVIDUAL, oRecip, NA, (const char*)strChat);
			//otherwise (unknown cases)
			else
				OnPlayerClicked(); //eat it and reset

		}
        // WLP - end of added code for chat to selected player
        else
        {
            trekClient.SendChat(trekClient.GetShip(), m_chattargetChannel, NA,
                                NA, (const char*)strChat);
        }
    }
	// KGJV #62
	void SetHiddenTeamButtons(bool bHidden)
	{
		SideID i;
		for (i = 0; i < m_plistSides->GetCount() ; i++)
			m_pbuttonsTeam[i]->SetHidden(bHidden);
		for (;i < c_cSidesMax; i++)
			m_pbuttonsTeam[i]->SetHidden(true);

	}
	// KGJV #62
	void UpdateTeamButtons()
	{
        bool bCanChange = trekClient.GetPlayerInfo()->IsMissionOwner() 
                && !trekClient.MyMission()->WasObjectModelCreated() 
                && trekClient.MyMission()->GetStage() == STAGE_NOTSTARTED;

		SetHiddenTeamButtons(!bCanChange);
		for (SideID i = 0; i < m_plistSides->GetCount() ; i++)
		{
			m_pbuttonsTeam[i]->SetChecked(m_pMission->SideActive(i));
		}
		for (SideID i = 0; i < m_plistSides->GetCount() ; i++)
		{
			m_pbuttonsTeam[i]->SetEnabled(m_pMission->SideNumPlayers(i)==0);
		}
	}
    void UpdateButtonStates()
    {
        OnSelPlayer(m_plistPanePlayers->GetSelection());

        if (trekClient.GetPlayerInfo() != NULL)
        {
            /*m_pbuttonReady->SetEnabled(!m_pMission->InProgress() 
                && trekClient.GetSideID() != SIDE_TEAMLOBBY);
            m_pbuttonReady->SetChecked(m_pMission->InProgress() 
                || trekClient.GetPlayerInfo()->IsReady());
            */
            m_pbuttonAwayFromKeyboard->SetEnabled(!m_pMission->InProgress() 
                || trekClient.GetSideID() == SIDE_TEAMLOBBY);
            m_pbuttonAwayFromKeyboard->SetChecked(!trekClient.GetPlayerInfo()->IsReady()
                && (!m_pMission->InProgress() || trekClient.GetSideID() == SIDE_TEAMLOBBY));
            
            m_pbuttonTeamReady->SetHidden(!trekClient.GetPlayerInfo()->IsTeamLeader() 
                || m_sideCurrent != trekClient.GetSideID());
            m_pbuttonTeamReady->SetEnabled(!m_pMission->InProgress() 
                && trekClient.GetPlayerInfo()->IsTeamLeader());
            m_pbuttonTeamReady->SetChecked(m_pMission->InProgress()
                || m_pMission->SideForceReady(trekClient.GetSideID()));

            m_pbuttonAutoAccept->SetHidden(!trekClient.GetPlayerInfo()->IsTeamLeader() 
                || m_sideCurrent != trekClient.GetSideID());
            m_pbuttonAutoAccept->SetEnabled(!m_pMission->InProgress() 
                && trekClient.GetPlayerInfo()->IsTeamLeader() 
                && !m_pMission->GetMissionParams().bLockGameOpen);
            m_pbuttonAutoAccept->SetChecked(m_pMission->SideAutoAccept(trekClient.GetSideID()));

            m_pbuttonLockLobby->SetEnabled(trekClient.GetPlayerInfo()->IsMissionOwner()
                && !m_pMission->GetMissionParams().bLockGameOpen);
            m_pbuttonLockLobby->SetChecked(m_pMission->GetLockLobby());

            m_pbuttonLockSides->SetEnabled(trekClient.GetPlayerInfo()->IsMissionOwner()); // && !m_pMission->GetMissionParams().bScoresCount); // TE: Disable LockSides if ScoresCount mmf undo this change
            m_pbuttonLockSides->SetChecked(m_pMission->GetLockSides());

            m_pbuttonRandomize->SetEnabled(trekClient.GetPlayerInfo()->IsMissionOwner());
            m_pbuttonRandomize->SetHidden(!trekClient.GetPlayerInfo()->IsMissionOwner());

            m_pbuttonAccept->SetHidden(!trekClient.GetPlayerInfo()->IsTeamLeader()
                || m_sideCurrent != trekClient.GetSideID());
            m_pbuttonReject->SetHidden(!trekClient.GetPlayerInfo()->IsTeamLeader()
                || m_sideCurrent != trekClient.GetSideID());
            m_pbuttonMakeLeader->SetHidden(!trekClient.GetPlayerInfo()->IsTeamLeader()
                || m_sideCurrent != trekClient.GetSideID());
            

            bool bCanChangeSettings = trekClient.GetPlayerInfo()->IsMissionOwner() 
                && !trekClient.MyMission()->WasObjectModelCreated() 
                && trekClient.MyMission()->GetStage() == STAGE_NOTSTARTED;
            m_pbuttonDetails->SetHidden(bCanChangeSettings);
            m_pbuttonSettings->SetHidden(!bCanChangeSettings);

            m_pbuttonTeamSettings->SetHidden(!trekClient.GetPlayerInfo()->IsTeamLeader()
                || m_sideCurrent != trekClient.GetSideID());
            m_pbuttonTeamSettings->SetEnabled(!m_pMission->GetMissionParams().bLockTeamSettings); 
            
            //
            // WLP - mod to allow team chat button for noat - changed 2 lines to allow it
            //
            // m_pbuttonbarChat->SetHidden(1, trekClient.GetSideID() == SIDE_TEAMLOBBY);
            // m_pbuttonbarChat->SetEnabled(1, trekClient.GetSideID() != SIDE_TEAMLOBBY);

            m_pbuttonbarChat->SetHidden(1, false); // WLP 2005 - show button
            m_pbuttonbarChat->SetEnabled(1, true); // WLP 2005 - turn it on

            m_pbuttonbarChat->SetHidden(2, !trekClient.GetPlayerInfo()->IsTeamLeader());
            m_pbuttonbarChat->SetEnabled(2, trekClient.GetPlayerInfo()->IsTeamLeader());

			// KGJV #62
			UpdateTeamButtons();
			
        }
        else
        {
            //m_pbuttonReady->SetEnabled(false);
            m_pbuttonMakeLeader->SetHidden(true);
            m_pbuttonAwayFromKeyboard->SetEnabled(false);
            m_pbuttonTeamReady->SetEnabled(false);
            m_pbuttonAutoAccept->SetEnabled(false);
            m_pbuttonLockLobby->SetEnabled(false);
            m_pbuttonLockSides->SetEnabled(false);
            m_pbuttonRandomize->SetEnabled(false);
            m_pbuttonRandomize->SetHidden(true);
            m_pbuttonSettings->SetHidden(true);
            m_pbuttonTeamSettings->SetHidden(true);
            m_pbuttonAccept->SetHidden(true);
            m_pbuttonReject->SetHidden(true);
            m_pbuttonbarChat->SetHidden(1, true);
            m_pbuttonbarChat->SetHidden(2, true);

			// KGJV #62
            SetHiddenTeamButtons(true);
        }
        
        m_pcomboWing->SetHidden(m_sideCurrent != trekClient.GetSideID() 
            || m_sideCurrent == SIDE_TEAMLOBBY);

		m_pbuttonDiscord->SetHidden(m_sideCurrent != trekClient.GetSideID());

        m_pbuttonJoin->SetEnabled(
                ((trekClient.GetSideID() == SIDE_TEAMLOBBY) && m_sideCurrent != SIDE_TEAMLOBBY) // TE: Commented this from the brackets so you can always join NOAT: || !m_pMission->GetMissionParams().bLockSides
                || (m_sideCurrent == SIDE_TEAMLOBBY
                || m_pMission->SideAvailablePositions(m_sideCurrent) > 0
                    && m_pMission->SideActive(m_sideCurrent))
            && m_sideCurrent != trekClient.GetSideID());

        m_pbuttonStart->SetHidden(!trekClient.MyPlayerInfo()->IsMissionOwner());
    }

    void UpdateTitleText()
    {   
        ZString str = ZString(m_pMission->Name()).ToUpper();
        m_ptextTitle->SetString(((strcmp(str, "") != 0) ? CensorBadWords (m_pMission->Name()) : "THIS GAME"));
    }
    
    void UpdatePromptText()
    {
        // KGJV #104 - display join requests
        if (m_sideToJoin != NA)
            m_ptextPrompt->SetString(ZString(trekClient.GetShip()->GetName()).ToUpper() 
                + ZString(", YOU ARE REQUESTING TO JOIN A TEAM ")); // todo: append team name may be or not :)
        else
        if (trekClient.GetSideID() != NA && trekClient.GetSideID() != SIDE_TEAMLOBBY)
            m_ptextPrompt->SetString(ZString(trekClient.GetShip()->GetName()).ToUpper() 
                + ZString(", YOU ARE ON TEAM ") + CensorBadWords (ZString(trekClient.GetSide()->GetName())).ToUpper());
        else
            m_ptextPrompt->SetString(ZString(trekClient.GetShip()->GetName()).ToUpper() 
                + ZString(" -- you need to pick a team"));
    }
    void OnFrame()
    {
        UpdateCountdownText();

        // if we should show the mission briefing...
        if (trekClient.GetSideID() != SIDE_TEAMLOBBY 
            && trekClient.MyMission()->GetStage() == STAGE_STARTING)
        {
            if (!m_pmsgBox)
            {
                if (m_bTeamSettingsPopupOpen)
                {
                    GetWindow()->GetPopupContainer()->ClosePopup(m_pteamSettingsPopup);
                    GetWindow()->RestoreCursor();
                }

                if (!m_pmissionBriefPopup && GetCountdownTime() > 0 
                    && !trekClient.GetBriefingText().IsEmpty())
                {
                    m_pmissionBriefPopup = new MissionBriefPopup();
                    GetWindow()->GetPopupContainer()->OpenPopup(m_pmissionBriefPopup, false);
                    GetWindow()->SetWaitCursor();
                }

                if (m_pmissionBriefPopup) {
                    m_pmissionBriefPopup->SetCountdown(GetCountdownTime());
                }
            }
        }
        else
        {
            if (m_pmissionBriefPopup && !m_pmsgBox)
            {
                GetWindow()->GetPopupContainer()->ClosePopup(m_pmissionBriefPopup);
                m_pmissionBriefPopup = NULL;
                GetWindow()->RestoreCursor();
            }
        }
    }

    void UpdateStatusText()
    {

        if (m_pMission->InProgress())
        {
            m_ptextStatus->SetString("GAME IN PROGRESS");
            m_ptextStatus2->SetString("");
            m_pbuttonStart->SetEnabled(false);
        }
        else if (m_pMission->GetStage() == STAGE_STARTING 
            || (m_pMission->GetMissionParams().bAutoRestart && m_pMission->GetStage() == STAGE_NOTSTARTED))
        {
            m_ptextStatus->SetString("COUNTDOWN IN PROGRESS");
            m_ptextStatus2->SetString("");
            m_pbuttonStart->SetEnabled(false);
        }
        else if (m_pMission->GetStage() == STAGE_OVER)
        {
            m_ptextStatus->SetString("GAME CANCELED");
            m_ptextStatus2->SetString("");
            m_pbuttonStart->SetEnabled(false);
        }
        else
        {
            int   minPlayers;
            int   maxPlayers;

            SideLinkIGC*   psl = trekClient.m_pCoreIGC->GetSides()->first();
            assert (psl);
            IsideIGC*   psideMin;
            IsideIGC*   psideMax = psideMin = psl->data();
            minPlayers = maxPlayers = psl->data()->GetShips()->n();
            
			// TE: Balance code
			// Initialize variables
			IsideIGC*   psideMinRank;
			IsideIGC*   psideMaxRank = psideMinRank = psl->data();
			int minTeamRank = 1000000; // Set really high: 1 meellion dollars!!!
			int maxTeamRank = 1;
			int tempRank = 0;
			int threshold = 1;

			// If "Enforce Balance" is checked, find the highest and lowest-ranked teams... mmf changed from lock sides to mImbalance
			if (m_pMission->GetMissionParams().iMaxImbalance == 0x7ffe)
			{
				threshold = GetRankThreshold();

				// Loop through all teams
				while (true)
				{
					if (psl == NULL)
						break;

					// KGJV #62 - skip deactivated team
					if (psl->data()->GetActiveF())
					{
						// TE: Remember lowest TeamRank
						tempRank = GetSideRankSum(psl->data(), false);
						if (tempRank < minTeamRank)
						{
							psideMinRank = psl->data();
							minTeamRank = tempRank;
						}

						// TE: Remember highest TeamRank
						if (tempRank > maxTeamRank)
						{
							psideMaxRank = psl->data();
							maxTeamRank = tempRank;
						}

						int n = psl->data()->GetShips()->n();

						// TE: Remember smallest side
						if (n < minPlayers)
						{
							psideMin = psl->data();
							minPlayers = n;
						}

						// TE: Remember largest side
						if (n > maxPlayers)
						{
							psideMax = psl->data();
							maxPlayers = n;
						}
					}
					psl = psl->next();
				}
			}

			// mmf using the below SendChat intermittently crashes the server
			//SendChat(ZString("Max: ") + ZString(maxTeamRank) + ZString("; Min: ") + ZString(minTeamRank) + "; Diff: " + ZString(maxTeamRank - minTeamRank) + ZString("; Thresh: ") + ZString(threshold));
			// mmf debugging, these do show up in the client log of the debug build
			//debugf("maxTR: %d minTR: %d thresh: %d\n",maxTeamRank, minTeamRank, threshold);

			// This section hides/shows the "Launch" button
			// TE: Added || to check rank balancing mmf changed from locksides to MaxImbalance
            if ((minPlayers + m_pMission->MaxImbalance() < maxPlayers) ||
			((m_pMission->GetMissionParams().iMaxImbalance == 0x7ffe) && (maxTeamRank - minTeamRank > threshold)))
            {
                m_ptextStatus->SetString("TEAMS ARE UNBALANCED");
                m_ptextStatus2->SetString("");
                m_pbuttonStart->SetEnabled(false);
            }
            else
            {
                SideID  idBlockingSide = NA;
                const char* szBlockingReason = NULL;

                for (SideID id = 0; id < m_pMission->NumSides(); id++)
                {
                    const char* szReason = NULL;
					// KGJV #62 - logic changed for AllowEmptyTeams 
					if (m_pMission->SideNumPlayers(id) < ( !m_pMission->SideActive(id) ? 0 : m_pMission->MinPlayersPerTeam()))
                        szReason = "BELOW MINIMUM SIZE";
                    else if (m_pMission->SideNumPlayers(id) > m_pMission->MaxPlayersPerTeam())
                        szReason = "ABOVE MAXIMUM SIZE";
					// EmptyTeams not allowed so check SideReady
					else if (m_pMission->SideActive(id) && !m_pMission->SideReady(id))
						szReason = "NOT READY";
					// EmptyTeams allowed so check SideReady only if it has at least 1 player
					else if (m_pMission->SideActive(id) && (m_pMission->SideNumPlayers(id) > 0 ) && !m_pMission->SideReady(id))
						szReason = "NOT READY";

                    if (szReason)
                    {
                        if (szBlockingReason == NULL)
                        {
                            idBlockingSide = id;
                            szBlockingReason = szReason;
							debugf("%s szBlockingReason set to %s\n",trekClient.GetCore()->GetSide(id)->GetName(),szBlockingReason);
                        }
                        else
                        {
                            // waiting on more than one side
                            idBlockingSide = NA;
                            break;
                        }
                    }
                }

                if (szBlockingReason == NULL)
                {
                    m_ptextStatus->SetString("READY TO START");
                    m_ptextStatus2->SetString("");
                    m_pbuttonStart->SetEnabled(!m_pMission->GetMissionParams().bAutoStart);
                }
                else
                {
                    if (idBlockingSide == NA)
                    {
                        m_ptextStatus->SetString("ONE OR MORE TEAMS");
                        m_ptextStatus2->SetString(ZString("ARE ") + szBlockingReason);
                    }
                    else
                    {
                        m_ptextStatus->SetString(CensorBadWords (ZString(trekClient.GetCore()->GetSide(idBlockingSide)->GetName())).ToUpper());
                        m_ptextStatus2->SetString(ZString("IS ") + szBlockingReason);
                    }
                    m_pbuttonStart->SetEnabled(false);
                }
            }
        }
    }

	/*-------------------------------------------------------------------------
	 * TE: GetSideRankSum
	 *-------------------------------------------------------------------------
	 * Purpose:
	 *    Count up the sum of ranks for a specified side
	 * 
	 * Parameters:
	 *    side to check
	 * 
	 * Returns:
	 *    sum of all players' ranks on side
	 */
	int GetSideRankSum(IsideIGC * pside, bool bCountGhosts)
	{
	  int iRankSum = 0;
	  int iTempRank = 0;
	  SideInfo* pSideInfo = m_pMission->GetSideInfo(pside->GetObjectID());

	  IshipIGC* pShip = NULL;
	  PlayerInfo* pPlayer = NULL;
	  ShipList plistMembers;
	  plistMembers = pSideInfo->GetMembers();
	  if (plistMembers.GetCount() > 0)
	  {
		  ShipID iShipID = plistMembers.GetFront();
		  while (true)
		  {
			  if (!iShipID)
				  break;

			  pPlayer = trekClient.FindPlayer(iShipID);

			  if (pPlayer)
			  {
				  if (pPlayer->IsHuman())
				  {
					  iTempRank = pPlayer->GetPersistScore(NA).GetRank();
					  iRankSum += (iTempRank < 1) ? 1 : iTempRank;
				  }
			  }
			  iShipID = (ShipID)plistMembers.GetNext((ItemID)iShipID);
		  }

	  }
	  return iRankSum;
	}
	
	/*-------------------------------------------------------------------------
	 * TE: GetRankThreshold
	 *-------------------------------------------------------------------------
	 * Purpose:
	 *    Retrieves the threshold to be used for rank balancing
	 * 
	 * Returns:
	 *    The threshold to be used for rank balancing
	 */
	int GetRankThreshold()
	{
		int iHighestRank = 1;
		int iAverageRank = 0;
	
		ShipList plistMembers;
	
		SideInfo* pSide = NULL;
	    PlayerInfo* pPlayer = NULL;
		int iTempRank = 0;
		int iShipIndex = 0;
	
		// Loop through sides
		for (SideID iSideID = 0; iSideID < m_pMission->NumSides(); iSideID++)
		{
			pSide = m_pMission->GetSideInfo(iSideID);
	
			// Loop through ships
			plistMembers = pSide->GetMembers();
			if (plistMembers.GetCount() > 0)
			{
				ShipID iShipID = plistMembers.GetFront();
				while (true)
				{
					if (!iShipID)
						break;

					pPlayer = trekClient.FindPlayer(iShipID);
	
					if (pPlayer)
					{
						if (pPlayer->IsHuman())
						{
							iTempRank = pPlayer->GetPersistScore(NA).GetRank();
							if (iTempRank <= 0)
								iTempRank = 1;
		
							// If it's the highest rank, remember it
							if (iTempRank > iHighestRank)
								iHighestRank = iTempRank;
		
							// Add to average
							iAverageRank += iTempRank;
						}
					}
					iShipID = (ShipID)plistMembers.GetNext((ItemID)iShipID);
				}
			}
		}
	
		// Divide average by #players
		int Divisor = trekClient.MyMission()->NumPlayers();
		if (Divisor > 0)
			iAverageRank = iAverageRank / Divisor;
		else
			iAverageRank = 1;
	
		// Calculate threshold
		int iThreshold = iHighestRank + ((iAverageRank * iAverageRank) / (iHighestRank + iAverageRank));
	
		return iThreshold;
	}

    int GetCountdownTime()
    {
        return std::max(0, int(trekClient.MyMission()->GetMissionParams().timeStart - Time::Now()) + 1);
    }

    void SetCountdownTimer(int nTime)
    {
        int nHours = nTime/(60 * 60);
        int nMinutes = nTime/60 - nHours * 60;
        int nSeconds = nTime - nMinutes * 60 - nHours * 60 * 60;

        if (nHours > 0)
            m_ptextTimer->SetString(ZString(nHours) 
                + ((nMinutes > 9) ? ":" : ":0") + ZString(nMinutes)
                + ((nSeconds > 9) ? ":" : ":0") + ZString(nSeconds));
        else if (nMinutes > 0)
            m_ptextTimer->SetString(ZString(nMinutes) + 
                ((nSeconds > 9) ? ":" : ":0") + ZString(nSeconds));
        else
            m_ptextTimer->SetString(ZString(nSeconds));
    }

    void UpdateCountdownText()
    {
        if ((trekClient.MyMission()->GetStage() == STAGE_STARTING
                && trekClient.GetSideID() == SIDE_TEAMLOBBY)
            || (trekClient.MyMission()->GetMissionParams().bAutoRestart 
                && trekClient.MyMission()->GetStage() == STAGE_NOTSTARTED))
        {
            m_ptextTimerType->SetString("STARTING IN:");
            int nTimeLeft = GetCountdownTime();

            SetCountdownTimer(nTimeLeft);
        }
        else if (trekClient.MyMission()->GetStage() == STAGE_STARTED)
        {
            int nTimeElapsed = (int)(Time::Now() - trekClient.MyMission()->GetMissionParams().timeStart);

            if (trekClient.MyMission()->GetMissionParams().IsCountdownGame())
            {
                m_ptextTimerType->SetString("TIME LEFT:");
                int nTimeLeft = (int)(trekClient.MyMission()->GetMissionParams().GetCountDownTime() - nTimeElapsed);
                SetCountdownTimer(nTimeLeft);
            }
            else
            {
                m_ptextTimerType->SetString("TIME ELAPSED:");
                SetCountdownTimer(nTimeElapsed);
            }
        }
        else
        {
            m_ptextTimer->SetString("");
            m_ptextTimerType->SetString("");
        }
    }

    static bool SideCompare(ItemID pitem1, ItemID pitem2)
    {
        SideInfo* sideInfo1 = (SideInfo*)pitem1;
        SideInfo* sideInfo2 = (SideInfo*)pitem2;
        
        return sideInfo1->GetSideID() > sideInfo2->GetSideID();
    }

    static bool LeaderCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return pplayer2->IsTeamLeader() && !pplayer1->IsMissionOwner();
    }

    static bool PlayerNameCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
        return _stricmp(pplayer1->CharacterName(), pplayer2->CharacterName()) > 0;
    }

    static bool PlayerRankCompare(ItemID pitem1, ItemID pitem2)
    {
        PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
        PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
		// TE: Modified this line to get rank properly so sorting works as expected
        return pplayer1->GetPersistScore(NA).GetRank() > pplayer2->GetPersistScore(NA).GetRank();
    }

    class PlayerStatusCompare
    {
    public:
        SideID m_sideCurrent;
        bool m_bReverse;

        PlayerStatusCompare(SideID sideCurrent, bool bReverse)
            : m_sideCurrent(sideCurrent), m_bReverse(bReverse) {};

        bool operator () (ItemID pitem1, ItemID pitem2)
        {
            PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
            PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
            bool bResult;

            // sort first by requesting/member, then by ready/unready
            if (pplayer1->SideID() != m_sideCurrent && pplayer2->SideID() == m_sideCurrent)
                bResult = false;
            else if (pplayer1->SideID() == m_sideCurrent && pplayer2->SideID() != m_sideCurrent)
                bResult = true;
            else
                bResult = pplayer1->IsReady() && !pplayer2->IsReady();

            return m_bReverse ? !bResult : bResult;
        }
    };

    class PlayerWingCompare
    {
    public:
        SideID m_sideCurrent;
        bool m_bReverse;

        PlayerWingCompare(SideID sideCurrent, bool bReverse)
            : m_sideCurrent(sideCurrent), m_bReverse(bReverse) {};

        bool operator () (ItemID pitem1, ItemID pitem2)
        {
            PlayerInfo* pplayer1 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem1));
            PlayerInfo* pplayer2 = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem2));
        
            bool bResult;

            // sort first by requesting/member, then by ready/unready
            if (pplayer1->SideID() != m_sideCurrent && pplayer2->SideID() == m_sideCurrent)
                bResult = false;
            else if (pplayer1->SideID() == m_sideCurrent && pplayer2->SideID() != m_sideCurrent)
                bResult = true;
            else
                bResult = pplayer1->GetShip()->GetWingID() > pplayer2->GetShip()->GetWingID();

            return m_bReverse ? !bResult : bResult;
        }
    };

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
    // UI Event handlers
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
		//#8 Imago 7/10
		if (m_plistPanePlayers->GetSelection() < 0)
			GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA); 

        if (pevent == m_peventPlayers) {
            OnSelPlayer(pitem);
        } else if (pevent = m_peventTeams) {
            OnSelTeam(pitem);
        }
        UpdatePromptText();//KGJV #104 - extra debug
        return true;
    }

    bool OnDismissRandomizeMessageBox()
    {
        m_bShowingRandomizeWarning = false;

        return false;
    }

    bool OnTeamClicked()
    {
		GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA); //#8 Imago 7/10
        trekClient.PlaySoundEffect(mouseclickSound);

        return true;
    }

    bool OnTeamDoubleClicked()
    {
		GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA); //#8 Imago 7/10
        trekClient.PlaySoundEffect(mouseclickSound);
        OnButtonJoin();

        return true;
    }

	// #ALLY
	// RMB trigger
    bool OnTeamRightClicked()
	{
		GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA); //#8 Imago 7/10
		if (!trekClient.MyPlayerInfo()->IsMissionOwner()) return true;// only game owner can activate the team context menu

		// allies only for conquest untill KGJV decides to finish what he started. Imago 7/18/09
		if (trekClient.MyMission()->GoalArtifacts()	|| trekClient.MyMission()->GoalDeathMatch() || 
			trekClient.MyMission()->GoalFlags() 	|| trekClient.MyMission()->GoalProsperity() || 
			trekClient.MyMission()->GoalTerritory()) 
			return true; 


		debugf("RMB team: side current = %d\n",m_sideCurrent);
		// count active teams
		int activeteams = 0;
		for (SideID i = 0; i < m_plistSides->GetCount() ; i++)
		{
			if (m_pMission->SideActive(i)) activeteams++;
		}
		debugf("RMB team: active teams = %d\n",activeteams);
		if (activeteams < 3) return true; // no alliance if 2 or less teams
		
		// create the menu
		m_pmenu =
            CreateMenu(
                GetModeler(),
                TrekResources::SmallFont(),
                m_pmenuCommandSink
            );

		char allies = m_pMission->SideAllies(m_sideCurrent);
		if (allies!=NA)
			m_pmenu->AddMenuItem(0 , "Remove allies");
		else
		{
			bool groups[c_cAlliancesMax]; for(int g=0;g<c_cAlliancesMax;g++) groups[g]=false;
			// add solo teams
			for (SideID i = 0; i < m_plistSides->GetCount() ; i++)
			{
				if (i == m_sideCurrent) continue;
				if (!m_pMission->SideActive(i)) continue;
				if (m_pMission->SideAllies(i) != NA)
				{
					groups[m_pMission->SideAllies(i)] = true;
				}
				else
					m_pmenu->AddMenuItem(i+1 , ZString("Ally with Team ")+ZString(i+1));
			}
			// add groups
			for(int g=0;g<c_cAlliancesMax;g++)
			{
				if (groups[g]==false) continue;
				ZString groupname("");
				SideID oneteamofthegroup;
				for (SideID i = 0; i < m_plistSides->GetCount() ; i++)
				{
					if (m_pMission->SideAllies(i) == g)
					{
						oneteamofthegroup = i;
						if (groupname != "")
							groupname += ",";
						groupname += ZString(i+1);
					}
				}
				m_pmenu->AddMenuItem(oneteamofthegroup+1 , ZString("Ally with Teams ")+groupname);
			}
		}
        m_pmenu->AddMenuItem(NA , "Cancel");

		Point popupPosition = GetEngineWindow()->GetMousePosition();
		
		TRef<Pane> ppane = m_pmenu->GetPane();
		ppane->UpdateLayout();
		Point p = Point::Cast(ppane->GetSize());

		popupPosition.SetY(popupPosition.Y() - p.Y());  
        GetWindow()->GetPopupContainer()->OpenPopup(m_pmenu,	popupPosition);

		return true;
	}

	// menu handlers - made to process only team RMB commands (alliances)
	void OnMenuCommand(IMenuItem* pitem)
	{
		debugf("menu %s-%d\n",PCC(pitem->GetString()),pitem->GetID());
		int menucmd = pitem->GetID();
		CloseMenu();
		if (m_sideCurrent == SIDE_TEAMLOBBY) return;

		if (menucmd == NA) return; // NA = cancel menu

		SideID ally = NA;
		if (menucmd != 0) // 0 = "remove all allies" from current team, otherwise it's a sideid+1
			ally = menucmd-1;

		// send this to server and let it handle the changes
		trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
		BEGIN_PFM_CREATE(trekClient.m_fm, pfmChangeAlliance, C, CHANGE_ALLIANCE)
		END_PFM_CREATE
		pfmChangeAlliance->sideID = m_sideCurrent;
		pfmChangeAlliance->sideAlly = ally;

		//imago 7/6/09 ALLY force Defections on when allies
		bool bAllies = true;
		for (SideID i = 0; i < trekClient.MyMission()->GetSideList()->GetCount() ; i++) {
			if (trekClient.MyMission()->SideAllies(i) == NA && bAllies) 
				bAllies = false;
		}
		bool bDefections = ((ally != NA) || bAllies) ? true : false;
		bool bAllowAlliedRip = ((ally != NA) || bAllies) ? true : false; //7/9/09 imago ALLY
		bool bAllowAlliedViz = ((ally != NA) || bAllies) ? true : false; //7/17/09 imago ALLY
		trekClient.MyMission()->SetAllowAlliedRip(bAllowAlliedRip && !m_bRipChecked);
		trekClient.MyMission()->SetAllowAlliedViz(bAllowAlliedViz); //always on when allies (for now)
		trekClient.MyMission()->SetDefections(bDefections);

		//because imbalance impelemtation is now FUBAR even w/o allies... we'll disable it -Imago 8/1/09
		if ((ally != NA) || bAllies)
			trekClient.MyMission()->SetMaxImbalance(0x7fff);
		else
			trekClient.MyMission()->SetMaxImbalance(0x7ffe);

		trekClient.MyMission()->SetSideAllies(m_sideCurrent,ally);
	
		OnTeamAlliancesChange(m_pMission); //hack this in while we're here
	
		//make the button flash and settings stick
		BEGIN_PFM_CREATE_ALLOC(trekClient.m_fm, pfmMissionParams, CS, MISSIONPARAMS)
        END_PFM_CREATE
		pfmMissionParams->missionparams = trekClient.MyMission()->GetMissionDef().misparms;
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        trekClient.m_fm.QueueExistingMsg((FEDMESSAGE *)pfmMissionParams);
		PFM_DEALLOC(pfmMissionParams);

		//

	}

    void CloseMenu()
	{
		debugf("menu close\n");
        GetWindow()->GetPopupContainer()->ClosePopup(m_pmenu);
        m_pmenu = NULL;
    }

	// not used atm (but the sink chain is ready for future use, see IClientEventSink)
	// imago won't go so far as to adapt this into clintlib, for now we'll just hack 7/5/09
	// into the new right click
	void OnTeamAlliancesChange(MissionInfo* pMissionInfo)
	{
		bool bAllies = false;
		for (SideID i = 0; i < trekClient.MyMission()->GetSideList()->GetCount() ; i++) {
			if (trekClient.MyMission()->SideAllies(i) != NA) {
				bAllies = true;
				break;
			}
		}
		if (!bAllies) {
				trekClient.MyMission()->SetAllowAlliedRip(false);
				trekClient.MyMission()->SetDefections(false);
				trekClient.MyMission()->SetAllowAlliedViz(false);
				trekClient.MyMission()->SetMaxImbalance(0x7ffe);
		}

		//#ALLY: set comms afk (except the game owner) Imago 7/3/09 (KGJV)
		if (trekClient.MyPlayerInfo()->IsTeamLeader()       // wlp - is a comm
			&& !trekClient.MyPlayerInfo()->IsMissionOwner()	// wlp - not in control
			&& !m_pbuttonAwayFromKeyboard->GetChecked())    // mmf 9/07 don't do anything if afk already checked
		{
			m_pbuttonTeamReady->SetChecked(false) ; // wlp - turn off team ready
			OnButtonTeamReady();// wlp - send to server
			m_pbuttonAwayFromKeyboard->SetChecked(true) ;// wlp - set comm afk
			// g_bAFKToggled = false; // mmf set this otherwise if afk was on it would get turned off  // mmf 9/07 may not need this anymore given above change
			OnButtonAwayFromKeyboard() ;// wlp - tell the world ( server ) about it
		} ;
		debugf("got OnTeamAlliancesChange\n");	
	}
	//#ALLY -end

	bool OnPlayerRightClicked()
    {
		ItemID pitem = m_plistPanePlayers->GetSelection();

        if (pitem != NULL && trekClient.GetPlayerInfo())
        {
            PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem));
			GetWindow()->ShowPlayerContextMenu(pplayer);            
        }		
        return true;
    }

	bool OnPlayerDoubleRightClicked()
	{
		return true;
	}

    //
    // WLP 2005 - added OnPlayerClicked to toggle selected player on/off for highlighted chat routine
	// 7/10 - finally fixed the bugs (Imago)
    //
    bool OnPlayerClicked()
    {
        static ShipID CurrentPlayerSelection = NA; //Imago 7/10 0 vs NA bug

        ShipID shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());

        if ( shipID == CurrentPlayerSelection )  // WLP - This is second click - turn off highlight
        {
            CurrentPlayerSelection = NA ; //Imago 7/10 0 vs NA bug
            m_plistPanePlayers->SetSelection(NULL);
			GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA); //Imago 7/10 #8
        }
        else CurrentPlayerSelection = shipID ;  // WLP - this is first click - leave highlight on

        trekClient.PlaySoundEffect(mouseclickSound);

        return true;
    }
    // WLP 2005 - end of add OnPlayerClicked()

    bool OnSelPlayer(ItemID pitem)
    {
        bool bEnableAccept = false;
        bool bEnableReject = false; 
        bool bEnableMakeLeader = false;

        if (pitem != NULL && trekClient.GetPlayerInfo())
        {
            PlayerInfo* pplayer = trekClient.FindPlayer(IntItemIDWrapper<ShipID>(pitem));

			//Imago #8 7/10
			GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA);
			if (pplayer->ShipID() != trekClient.GetShipID())
				GetWindow()->SetLobbyChatTarget(CHAT_INDIVIDUAL,pplayer->ShipID());
			
            if (m_sideCurrent == trekClient.GetSideID() 
                && trekClient.GetPlayerInfo()->IsTeamLeader())
            {
                bEnableAccept = m_pMission->SideAvailablePositions(m_sideCurrent) > 0
                    && m_pMission->FindRequest(m_sideCurrent, pplayer->ShipID());
                bEnableReject = pplayer->ShipID() != trekClient.GetShipID()
                    && (!m_pMission->GetMissionParams().bLockTeamSettings
                        || pplayer->SideID() != trekClient.GetSideID());
                bEnableMakeLeader = pplayer->SideID() == trekClient.GetSideID()
                    && pplayer->ShipID() != trekClient.GetShipID();
            }
        }

        m_pbuttonAccept->SetEnabled(bEnableAccept);
        m_pbuttonReject->SetEnabled(bEnableReject);
        m_pbuttonMakeLeader->SetEnabled(bEnableMakeLeader);
        
        return true;
    }

    void UpdateCivBitmap()
    {
        IsideIGC* pside = trekClient.GetCore()->GetSide(m_sideCurrent);
        TRef<Image> pimage;

		if (pside && m_sideCurrent != SIDE_TEAMLOBBY && pside->GetShips()->n() > 0 && !pside->GetRandomCivilization()) { //Xynth #170 8/2010 use default for Random
            ZString str = ZString(pside->GetCivilization()->GetIconName()).ToLower() + "lobbybmp";

            m_pwrapImageCiv->SetImage(GetModeler()->LoadImage(str, false)); // KGJV 32B - consistency with defaultwatermark below
        } else {
            m_pwrapImageCiv->SetImage(GetModeler()->LoadImage("defaultwatermarkbmp", false));
        }
    }

    void OnTeamCivChange(MissionInfo *pMissionDef, SideID sideID, CivID civID)
    {
		//Imago #114 7/10
		if (trekClient.MyPlayerInfo()->IsTeamLeader() && !m_pbuttonAwayFromKeyboard->GetChecked() && trekClient.GetSideID() != sideID)
		{
			if (!trekClient.GetShip()->GetSide()->GetRandomCivilization())  //Xynth #170 8/2010
			{
			m_pbuttonTeamReady->SetChecked(false);
			OnButtonTeamReady();
			m_pbuttonAwayFromKeyboard->SetChecked(true);
			OnButtonAwayFromKeyboard() ;
		}
		}
		//

        UpdateCivBitmap();
		m_plistPaneTeams->ForceRefresh(); // KGJV #62 fix: force faction names to refresh
    }

    bool OnSelTeam(ItemID pitem)
    {
        SideInfo* sideinfo = (SideInfo*)pitem;

        if (pitem == NULL)
        {
            // default the selection to the lobby side -Imago #54 commented out
            //m_plistPaneTeams->SetSelection(m_pMission->GetSideInfo(SIDE_TEAMLOBBY)); 
        }
		else
		// KGJV #114 cant select destroyed/inactive & empty teams
		if (!m_pMission->SideActive(sideinfo->GetSideID()) && //destroyed/inactive
			m_pMission->SideNumPlayers(sideinfo->GetSideID()) == 0) //empty teams
		{
            // default the selection to the lobby side
            m_plistPaneTeams->SetSelection(m_pMission->GetSideInfo(SIDE_TEAMLOBBY));
		}
        else
        {


            m_sideCurrent   = sideinfo->GetSideID();
            IsideIGC* pside = trekClient.GetCore()->GetSide(m_sideCurrent);
            
            //
            // Update the background color
            //

            Color color;

            if (!pside) {
                color = Color::Black();
            } else {
                color = pside->GetColor();
            }

            color = m_fColorBrightness * color;
            color.SetAlpha(m_fColorIntensity);
            m_pcolorCiv->SetValue(color);
            

            //
            // Update the civ bitmap
            //

            UpdateCivBitmap();

            //
            // create the player list for this side
            //

            TRef<List> plistPlayers = new ConcatinatedList(
                trekClient.MyMission()->GetSideInfo(m_sideCurrent)->GetMemberList(),
                trekClient.MyMission()->GetSideInfo(m_sideCurrent)->GetRequestList()
                );

            // filter out the drones
            plistPlayers = new FilteredList<bool (*)(ItemID)>(plistPlayers, IsHuman);

            switch (m_sortPlayers)
            {
            case sortLeader:
                plistPlayers = SortingList(plistPlayers, LeaderCompare, m_bReverseSort);
                break;

            case sortPlayerName:
                plistPlayers = SortingList(plistPlayers, PlayerNameCompare, m_bReverseSort);
                break;

            case sortPlayerRank:
                plistPlayers = SortingList(plistPlayers, PlayerRankCompare, m_bReverseSort);
                break;

            case sortPlayerStatus:
                plistPlayers = new SortedList<PlayerStatusCompare>(
                    plistPlayers, PlayerStatusCompare(m_sideCurrent, m_bReverseSort));
                break;

            case sortPlayerWing:
                plistPlayers = new SortedList<PlayerWingCompare>(
                    plistPlayers, PlayerWingCompare(m_sideCurrent, m_bReverseSort));
                break;
            }

            m_plistPanePlayers->SetList(plistPlayers);
        }

        UpdateButtonStates();
        return true;
    }

    bool OnEvent(TEvent<ZString>::Source* pevent, ZString str)
    {
        if (!str.IsEmpty())
            SendChat(str);
        m_peditPane->SetString(ZString());

        return true;
    }

    bool OnEvent(IEventSource* pevent)
    {
        return OnTimer();
    }

    void OnNewChatMessage()
    {
		ChatInfo info = trekClient.m_chatList.last()->data();
        ChatTarget target = info.GetChatTarget();
        if (target != m_chattargetChannel && info.IsFromPlayer())	// TE: Added IsFromPlayer check to prevent unnecessary blinking
        {
            switch (target)
            {
            case CHAT_EVERYONE:
                m_vbUncheckedMessages[0] = true;
                break;

            case CHAT_TEAM:
                m_vbUncheckedMessages[1] = true;
                break;

            case CHAT_LEADERS:
                m_vbUncheckedMessages[2] = true;
                break;
            }
        }
    }

    bool OnTimer()
    {
        s_bFlash = !s_bFlash;

        for (int i = 0; i < 3; i++)
        {
            if (m_vbUncheckedMessages[i])
                m_pbuttonbarChat->SetChecked2(i, s_bFlash);
        }

        if (m_bDetailsChanged)
            m_pbuttonDetails->SetChecked(s_bFlash);

        SideInfo* psideinfo = trekClient.MyMission()->GetSideInfo(m_sideCurrent);
            
        if (psideinfo->GetRequests().GetCount() > 0)
            m_plistPanePlayers->ForceRefresh();
		//Xynth #170 8/10 Randomize random factions again and resend to all players.  This mitigates joiners
		//seeing the faction picked randomly
		if (!trekClient.MyMissionInProgress() && trekClient.MyPlayerInfo()->IsMissionOwner() && (abs(Time::Now() - lastRandCivUpdate)  > 5.0))//only the GC does this
		{
			lastRandCivUpdate = Time::Now();
			numCivilizations = 0;
			for (CivilizationLinkIGC* linkCiv = trekClient.GetCore()->GetCivilizations()->first(); 
				linkCiv != NULL; linkCiv = linkCiv->next())
			{                
				if (trekClient.MyPlayerInfo()->GetShip()->GetPilotType() == c_ptCheatPlayer 
					|| linkCiv->data()->GetObjectID() < 300)        
				{				
					numCivilizations++; //Xynth #170 8/2010 fill an array of faction IDs to pick one at random
					CivilizationArray[numCivilizations-1] = linkCiv->data()->GetObjectID();					
				}
			}
			//Loop through the side and randomize anyone w/ random selected
			for (SideLinkIGC* psidelink = trekClient.GetShip()->GetMission()->GetSides()->first();
						(psidelink != NULL);
						psidelink = psidelink->next())
			{
				IsideIGC*   pside = psidelink->data();
				if (pside->GetRandomCivilization())
				{
					bool bRandomCiv = true;
					int randIndex = randomInt(0,numCivilizations - 1); //random faction
					int civSelection = CivilizationArray[randIndex];
					trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
					BEGIN_PFM_CREATE(trekClient.m_fm, pfmChangeCiv, CS, CHANGE_TEAM_CIV)
					END_PFM_CREATE
					pfmChangeCiv->iSide = pside->GetObjectID(); 
					pfmChangeCiv->civID = civSelection;
					pfmChangeCiv->random = bRandomCiv;
				}
			}
		}
        return true;
    }

    bool OnCivChange(int index)
    {
        OnCivChosen(index);
        return true;
    }

    bool OnWingCombo(int index)
    {
		trekClient.SaveWingAssignment(index); // kolie 6/10
        trekClient.SetWing(index);
        return true;                
    }

    bool OnButtonBack()
    {
        assert(!m_pmsgBox);
        if (m_pmsgBox)
        {
            // dismiss the "waiting to be accepted..." popup
            GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
            GetWindow()->RestoreCursor();
        }
        m_pmsgBox = NULL;

        GetWindow()->StartQuitMission();

        return true;
    }

    bool OnButtonGameOver()
    {
        // KGJV #104 -
        // cancel any join request while looking at previous game scores
        if (m_lastToJoinSend != NA)
        {
            OnCancelRequest();
        }
        s_nLastSelectedChatTab = ChatTargetToButton(m_chattargetChannel);
        GetWindow()->screen(ScreenIDGameOverScreen);

        return true;
    }

    bool OnButtonStart()
    {
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmStartGame, C, START_GAME)
        END_PFM_CREATE
        return true;
    }
    /*
    bool OnButtonReady()
    {
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmReady, CS, PLAYER_READY)
        END_PFM_CREATE
        pfmReady->fReady = m_pbuttonReady->GetChecked();
        pfmReady->shipID = trekClient.GetShipID();
        UpdateStatusText();
        return true;
    }
    */
	bool OnButtonAwayFromKeyboard() // wlp 8/5/2006 modded to support comm afk during settings
	{
		trekClient.GetPlayerInfo ()->SetReady(!m_pbuttonAwayFromKeyboard->GetChecked()); // Imago - prevents sending duplicate player_ready msg when DoTrekUpdate
		trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
		BEGIN_PFM_CREATE(trekClient.m_fm, pfmReady, CS, PLAYER_READY)
		END_PFM_CREATE
		pfmReady->fReady = !m_pbuttonAwayFromKeyboard->GetChecked();
		pfmReady->shipID = trekClient.GetShipID();
		UpdateButtonStates(); // wlp - 8/5/2006 added to show the comm status afk during settings
		UpdateStatusText();
		g_bAFKToggled = (g_bAFKToggled) ? false : true; //Imago: Manual AFK toggle flag

		return true;
	}

    bool OnButtonTeamReady()
    {
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmReady, CS, FORCE_TEAM_READY)
        END_PFM_CREATE
        pfmReady->iSide = trekClient.GetSideID();
        pfmReady->fForceReady = m_pbuttonTeamReady->GetChecked();
        UpdateStatusText();

		// mmf 09/07 partial fix launching with comm showing status as afk, they can still click afk after clicking TeamReady and will show as afk
		// uncheck comm's afk if they click TeamReady
		if (trekClient.MyPlayerInfo()->IsTeamLeader()      // is a comm
			&& m_pbuttonAwayFromKeyboard->GetChecked())    // afk is checked
		{
			m_pbuttonAwayFromKeyboard->SetChecked(false) ; //  clear comm's afk
			g_bAFKToggled = true; // mmf : wlp added this, not sure if it is needed in this situation
			OnButtonAwayFromKeyboard();
		} ;
        // end mmf 09/07
		
		return true;        
    }

    bool OnButtonAutoAccept()
    {
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmAutoAccept, CS, AUTO_ACCEPT)
        END_PFM_CREATE
        pfmAutoAccept->fAutoAccept = m_pbuttonAutoAccept->GetChecked();
        pfmAutoAccept->iSide = trekClient.GetSideID();

        //The server knows all and sets the side to keep people honest
        //pfmAutoAccept->iSide = trekClient.GetSideID();
        return true;
    }

    bool OnButtonLockLobby()
    {
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmLockLobby, CS, LOCK_LOBBY)
        END_PFM_CREATE
        pfmLockLobby->fLock = m_pbuttonLockLobby->GetChecked();
        return true;
    }

    bool OnButtonLockSides()
    {
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmLockSides, CS, LOCK_SIDES)
        END_PFM_CREATE
        pfmLockSides->fLock = m_pbuttonLockSides->GetChecked();
        return true;
    }

    bool OnButtonRandomize()
    {
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmRandomize, C, RANDOMIZE_TEAMS)
        END_PFM_CREATE
        return true;
    }

    bool OnCancelRequest()
    {
        // KGJV #104: called when cancelling a request to join a team
        // assert to do: m_sideToJoin should be valid  and match last POSITIONREQ message
        //assert(m_lastToJoinSend == NA);
        debugf("TeamScreen::OnCancelRequest: %d\n",m_lastToJoinSend);
        if (m_pmsgBox)
        {
            GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
            GetWindow()->RestoreCursor();
        }

        if (m_lastToJoinSend == NA)  // shouldnt happen
        {
            debugf("TeamScreen::OnCancelRequest - m_lastToJoinSend == NA\n"); 
            return false;
        }

        GetWindow()->SetWaitCursor();
        m_pmsgBox = CreateMessageBox("Canceling request...", NULL, false, false, 1.0f);
        GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
        // KGJV #104
        // we'll clear this when server replies
        // m_sideToJoin = NA;

        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmDelPosReq, CS, DELPOSITIONREQ)
        END_PFM_CREATE
        pfmDelPosReq->shipID = trekClient.MyPlayerInfo()->ShipID();
        pfmDelPosReq->iSide = m_sideToJoin; // KGJV #104 - we cancel for sideToJoin
        pfmDelPosReq->reason = DPR_Canceled;
        m_lastToJoinSend = NA;  // KGJV #104
        return false;
    }

	bool OnButtonDiscord()
	{
		switch (m_sideCurrent)
		{
		case 0: // Yellow.
			GetWindow()->ShowWebPage("https://discord.gg/qv3cEa7");
			break;

		case 1: // Blue.
			GetWindow()->ShowWebPage("https://discord.gg/6JWrRdZ");
			break;

		case 2: // Purple.
			GetWindow()->ShowWebPage("https://discord.gg/wytSK2Q");
			break;

		case 3: // Green.
			GetWindow()->ShowWebPage("https://discord.gg/5QWGrSa");
			break;

		case 4: // Pink.
			GetWindow()->ShowWebPage("https://discord.gg/WrHj7AS");
			break;

		case 5: // Teal.
			GetWindow()->ShowWebPage("https://discord.gg/eUrSxek");
			break;

		default: // NOAT
			GetWindow()->ShowWebPage("https://discord.gg/43jn6Ab");
			break;
		}

		return true;
	}

    bool OnButtonJoin()
    {
        if (m_pmsgBox)
        {
            // clicks shouldn't be sneaking through the message box
            assert(false);
            return true;
        }

		// Imago - They are not AFK, "click the button for them"
		if (g_bAFKToggled) OnButtonAwayFromKeyboard();

        UpdatePromptText();//KGJV #104 - extra debug
        // KGJV - #104
        // already requesting to join here or another team, so we cancel the request
        if (m_sideToJoin != NA)
        {
            OnCancelRequest();
            UpdateButtonStates();
            UpdatePromptText();//KGJV #104 - extra debug
            return true;
         }
        
        // ZAssert(m_sideCurrent != trekClient.GetSideID());

        if (m_sideCurrent == SIDE_TEAMLOBBY
            && trekClient.GetSideID() != SIDE_TEAMLOBBY)
        {
            // quit the current side
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmPositionReq, CS, QUIT_SIDE)
            END_PFM_CREATE
            pfmPositionReq->shipID = trekClient.GetShipID(); 
            pfmPositionReq->reason = QSR_Quit; 

            m_sideToJoin = m_sideCurrent;

            m_pmsgBox = 
                CreateMessageBox(
                    "Quitting current team...",
                    NULL,
                    false
                );

            GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
        }
        else if (m_sideCurrent != trekClient.GetSideID() 
            && m_sideCurrent != SIDE_TEAMLOBBY
            && m_pMission->SideAvailablePositions(m_sideCurrent) > 0
            && m_pMission->SideActive(m_sideCurrent)
            )
        {
			
			//Imago 6/10 #91 - server will only set command now
			if (trekClient.GetShip()->GetWingID() != trekClient.GetSavedWingAssignment()) {// || (trekClient.GetShip()->GetWingID() != 0 || !trekClient.MyPlayerInfo()->IsTeamLeader()) ) {
				trekClient.SetWing(trekClient.GetSavedWingAssignment());
			}

            // try to join the current side
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmPositionReq, C, POSITIONREQ)
            END_PFM_CREATE
            pfmPositionReq->iSide = m_sideCurrent; 

            m_sideToJoin = m_sideCurrent;

            // KGJV #104 : remove modal popup
            //m_pmsgBox = 
            //    CreateMessageBox(
            //        "Waiting to be accepted to the new team...",
            //        NULL,
            //        false
            //    );

            //assert(m_lastToJoinSend == NA); // check for no previous unanswered or cancelled request
            m_lastToJoinSend = m_sideToJoin;
            UpdateButtonStates();
            UpdatePromptText();

            //TRef<Surface> psurface = GetModeler()->LoadSurface(AWF_TEAMROOM_QUIT_TEAM_BUTTON, true);
            //TRef<ButtonPane> pQuitTeamButton 
            //    = CreateTrekButton(CreateButtonFacePane(psurface, ButtonNormal), false, positiveButtonClickSound);
            //pQuitTeamButton->SetOffset(WinPoint(183, 170));
            //m_pmsgBox->GetPane()->InsertAtBottom(pQuitTeamButton);
            //AddEventTarget(&TeamScreen::OnCancelRequest, pQuitTeamButton->GetEventSource());
            //GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
        }

        return true;
    }

    bool OnButtonAccept()
    {
        ShipID shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());

        // WLP 2005 – remove highlight from player to prevent chat target
        m_plistPanePlayers->SetSelection(NULL); // WLP – remove as chat target
        GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA); //#8 Imago 7/10

        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmPosAck, C, POSITIONACK)
        END_PFM_CREATE
        pfmPosAck->shipID = shipID;
        pfmPosAck->fAccepted = true;
        pfmPosAck->iSide = trekClient.GetSideID();

        return true;
    }

    bool OnButtonReject()
    {
        ShipID shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
        PlayerInfo* pplayer = trekClient.FindPlayer(shipID);
        
        if (!pplayer)
        {
            // should never happen - the button should be disabled.
            assert(false);
            return true;
        }

        // if we are booting a player who is already on our team...
        if (pplayer->SideID() == trekClient.GetSideID())
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmQuitSide, CS, QUIT_SIDE)
            END_PFM_CREATE
            pfmQuitSide->shipID = shipID;
            pfmQuitSide->reason = QSR_LeaderBooted;
        }
        else
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmPosAck, C, POSITIONACK)
            END_PFM_CREATE
            pfmPosAck->shipID = shipID;
            pfmPosAck->fAccepted = false;
            pfmPosAck->iSide = trekClient.GetSideID();
        }

        return true;
    }

    void OnCivChosen(int civID)
    {
        if (trekClient.GetSideID() != SIDE_TEAMLOBBY
            && trekClient.MyPlayerInfo()->IsTeamLeader())
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmChangeCiv, CS, CHANGE_TEAM_CIV)
            END_PFM_CREATE
            pfmChangeCiv->iSide = trekClient.GetSideID(); 
            pfmChangeCiv->civID = civID;
			pfmChangeCiv->random = false;  //Xynth #170 8/10
        }
    }

    int ChatTargetToButton(ChatTarget chatTarget)
    {
        switch (chatTarget)
        {
        case CHAT_EVERYONE:
            return 0;

        case CHAT_TEAM:
            return 1;

        case CHAT_LEADERS:
            return 2;

        default:
            ZError("Unreached");
            return 0;
        }
    }

    bool OnButtonBarChat(int iButton)
    {
        switch (iButton)
        {
        case 0: // everyone
            m_chattargetChannel = CHAT_EVERYONE;
            m_pchatPane->ResetChannels();
            m_pchatPane->AddChannel(m_chattargetChannel);
            m_pchatPane->AddChannel(CHAT_INDIVIDUAL);
            m_pchatPane->AddChannel(CHAT_INDIVIDUAL_NOFILTER);
            m_pchatPane->AddChannel(CHAT_ADMIN);
            break;

        case 1: // team
            m_chattargetChannel = CHAT_TEAM;
            m_pchatPane->ResetChannels();
            m_pchatPane->AddChannel(m_chattargetChannel);
            m_pchatPane->AddChannel(CHAT_INDIVIDUAL);
            m_pchatPane->AddChannel(CHAT_INDIVIDUAL_NOFILTER);
            m_pchatPane->AddChannel(CHAT_ADMIN);
            break;

        case 2:
            if (trekClient.MyPlayerInfo()->IsTeamLeader())
            {
                m_chattargetChannel = CHAT_LEADERS;
                m_pchatPane->ResetChannels();
                m_pchatPane->AddChannel(m_chattargetChannel);
                m_pchatPane->AddChannel(CHAT_INDIVIDUAL);
                m_pchatPane->AddChannel(CHAT_INDIVIDUAL_NOFILTER);
                m_pchatPane->AddChannel(CHAT_ADMIN);
            }
            break;
        }
        m_vbUncheckedMessages[iButton] = false;
        m_pbuttonbarChat->SetChecked2(iButton, false);
        GetWindow()->SetLobbyChatTarget(m_chattargetChannel);

        return true;
    }

    bool OnButtonBarPlayers(int iButton)
    {
        PlayerSort sortOld = m_sortPlayers;

        switch (iButton)
        {
        case 0:
            m_sortPlayers = sortPlayerName;
            break;

        case 3:
            m_sortPlayers = sortPlayerRank;
            break;

        case 4:
            m_sortPlayers = sortPlayerStatus;
            break;

        case 5:
            m_sortPlayers = sortPlayerWing;
            break;
        }

        if (sortOld == m_sortPlayers)
            m_bReverseSort = !m_bReverseSort;
        else
            m_bReverseSort = false;

        OnSelTeam(m_pMission->GetSideInfo(m_sideCurrent));

        // do radio-button behavior
        for (int i = 0; i < 6; i++)
        {
            m_pbuttonbarPlayers->SetChecked(i, false);
            m_pbuttonbarPlayers->SetChecked2(i, false);
        }
        m_pbuttonbarPlayers->SetChecked(iButton, !m_bReverseSort);
        m_pbuttonbarPlayers->SetChecked2(iButton, m_bReverseSort);

        return true;
    }

    bool OnButtonSend()
    {
        ZString str = m_peditPane->GetString();

        if (!str.IsEmpty())
            SendChat(str);
        m_peditPane->SetString(ZString());

        return true;
    }

	bool OnButtonDetails()
	{
		// wlp 8/5/2006
		// code added to prevent game starting when the comm is doing settings
		//
		// do this only for the commander without game control
		//
		if (trekClient.MyPlayerInfo()->IsTeamLeader()       // wlp - is a comm
			&& !trekClient.MyPlayerInfo()->IsMissionOwner()	// wlp - not in control
			&& !m_pbuttonAwayFromKeyboard->GetChecked())    // mmf 9/07 don't do anything if afk already checked
		{
			m_pbuttonTeamReady->SetChecked(false) ; // wlp - turn off team ready
			OnButtonTeamReady();// wlp - send to server
			m_pbuttonAwayFromKeyboard->SetChecked(true) ;// wlp - set comm afk
			// g_bAFKToggled = false; // mmf set this otherwise if afk was on it would get turned off  // mmf 9/07 may not need this anymore given above change
			OnButtonAwayFromKeyboard() ;// wlp - tell the world ( server ) about it
		} ;
		// wlp 8/5/2006 - end of Afk added code

        // KGJV #104
        // cancel any join request while looking at setting
        if (m_lastToJoinSend != NA)
        {
            OnCancelRequest();
        }
		s_nLastSelectedChatTab = ChatTargetToButton(m_chattargetChannel);
		GetWindow()->screen(ScreenIDCreateMission);
		return true;
	}

	bool OnButtonTeamSettings()
	{
		// wlp 8/5/2006
		// code added to prevent game starting when the comm is doing settings
		//
		// do this only for the commander without game control
		//
		if (trekClient.MyPlayerInfo()->IsTeamLeader()       // wlp - is a comm
			&& !trekClient.MyPlayerInfo()->IsMissionOwner() // wlp - not in control
			&& !m_pbuttonAwayFromKeyboard->GetChecked())    // mmf 9/07 don't do anything if afk already checked
		{
			m_pbuttonTeamReady->SetChecked(false) ; // wlp - turn off team ready
			OnButtonTeamReady() ;// wlp - send to server
			m_pbuttonAwayFromKeyboard->SetChecked(true) ;// wlp - set comm afk
			// g_bAFKToggled = false; // mmf set this otherwise if afk was on it would get turned off // mmf 9/07 may not need this anymore given above change
			OnButtonAwayFromKeyboard() ;// wlp - tell the world ( server ) about it
		} ;
		// wlp 8/5/2006 - end of Afk added code
		GetWindow()->GetPopupContainer()->OpenPopup(m_pteamSettingsPopup,false);
		debugf("opening team settings.\n");
		return true;
	}

    bool OnButtonMakeLeader()
    {
        ShipID shipID = IntItemIDWrapper<ShipID>(m_plistPanePlayers->GetSelection());
        PlayerInfo* pplayer = trekClient.FindPlayer(shipID);
        
		// WLP 2005 - remove highlight from player to prevent chat target
 		m_plistPanePlayers->SetSelection(NULL); // WLP 2005 - remove as chat target
		GetWindow()->SetLobbyChatTarget(m_chattargetChannel, NA); //#8 Imago 7/10

        if (pplayer && pplayer->SideID() == trekClient.GetSideID())
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmSetTeamLeader, CS, SET_TEAM_LEADER)
            END_PFM_CREATE
            pfmSetTeamLeader->sideID = trekClient.GetSideID(); 
            pfmSetTeamLeader->shipID = shipID; 

			//Imago 6/10 #91 - server will only set command now
			if (trekClient.GetShip()->GetWingID() != trekClient.GetSavedWingAssignment()) {// || (trekClient.GetShip()->GetWingID() != 0 || !trekClient.MyPlayerInfo()->IsTeamLeader()) ) {
				trekClient.SetWing(trekClient.GetSavedWingAssignment());
			}
        }
        else
        {
            // button should be disabled in this case
            assert(false);
        }

        return true;
    }
	// KGJV #62
	bool OnButtonToggleTeam(SideID side)
	{
        trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
        BEGIN_PFM_CREATE(trekClient.m_fm, pfmSideInactive, CS, SIDE_INACTIVE)
        END_PFM_CREATE
        pfmSideInactive->sideID = side;
        pfmSideInactive->bActive = m_pbuttonsTeam[side]->GetChecked();
		pfmSideInactive->bChangeAET = false; // server will set this when replying
		debugf("toggle team %d\n",(int)side);
		return true;
	}
	bool OnButtonToggleTeam0()	{	return OnButtonToggleTeam(0); }
	bool OnButtonToggleTeam1()	{	return OnButtonToggleTeam(1); }
	bool OnButtonToggleTeam2()	{	return OnButtonToggleTeam(2); }
	bool OnButtonToggleTeam3()	{	return OnButtonToggleTeam(3); }
	bool OnButtonToggleTeam4()	{	return OnButtonToggleTeam(4); }
	bool OnButtonToggleTeam5()	{	return OnButtonToggleTeam(5); }
    //////////////////////////////////////////////////////////////////////////////
    //
    // Game Event handlers
    //
    //////////////////////////////////////////////////////////////////////////////

    void OnAddMission(MissionInfo* pMissionDef)
    {
        // if this is my mission...
        
        if (pMissionDef == trekClient.MyMission())
        {

            // if the side that we were looking at was nuked, switch back 
            // to our team.
            if (m_sideCurrent >= pMissionDef->GetMissionParams().nTeams)
                m_plistPaneTeams->SetSelection(m_pMission->GetSideInfo(trekClient.GetSideID()));

            if (trekClient.MyMission()->GetMissionParams().bSquadGame)
                m_pnumberShowSquad->SetValue(1);
            else
                m_pnumberShowSquad->SetValue(0);

			// KGJV : changed, see ctor
            //if (trekClient.MyMission()->GetMissionParams().bScoresCount)
            //    m_pnumberStatsCount->SetValue(1);
            //else
            //    m_pnumberStatsCount->SetValue(0);

            // Update the text
            UpdateStatusText();
            UpdatePromptText();
            UpdateTitleText();
            UpdateButtonStates();
            UpdateSquadsStatsMessage();
            m_bDetailsChanged = true;

			// KGJV #62
			m_ppaneMapPreview->ComputePreview(trekClient.MyMission()->GetMissionParams());
        }
    }

    void OnAddPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        // if this is the side I'm looking at, if we just upped the number of 
        // players we may need to change the civ watermark.
        UpdateCivBitmap();

		UpdateTeamButtons();// KGJV #62

        // if this is me...

        if (trekClient.MyPlayerInfo()->ShipID() == pPlayerInfo->ShipID())
		{
            debugf("TeamScreen::OnAddPlayer: sideID=%d, m_sideToJoin=%d, m_lastToJoinSend=%d \n",sideID,m_sideToJoin,m_lastToJoinSend); // KGJV #104
            if (g_civIDStart != -1 && sideID != SIDE_TEAMLOBBY) {
                OnCivChosen(g_civIDStart);
                g_civStart = -1;
            }

            if (pPlayerInfo->SideID() == m_sideToJoin)
            {
                m_sideToJoin = NA;
                m_lastToJoinSend = NA; // KGJV #104 - request fullfilled
                if (m_pmsgBox)
                {
                    // dismiss the "waiting to be accepted..." popup
                    GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
                    GetWindow()->RestoreCursor();
                }
                m_pmsgBox = NULL;
            }
            // KGJV #104
            // player was added to a team while requesting to join another
            // so cancel his request
            else if (m_sideToJoin != NA)
                if (pPlayerInfo->SideID() != SIDE_TEAMLOBBY) // joining NOAT never generates a request
                if (m_sideToJoin != SIDE_TEAMLOBBY) // joining NOAT never generates a request
                    OnCancelRequest();
            // if I'm in the team leader chat and am no longer a team leader, 
            // or if I'm in the team chat and no longer on a team, throw me out.
            //
            // WLP 2005 - commented the qualififiers out to always start everyone on (all) chat
            //
            // if ((m_chattargetChannel == CHAT_LEADERS)
            //    && (!trekClient.MyPlayerInfo()->IsTeamLeader())
            //    || (m_chattargetChannel == CHAT_TEAM)
            //    && (trekClient.GetSideID() == SIDE_TEAMLOBBY))
            {              
				m_pbuttonbarChat->SetSelection(0);
                OnButtonBarChat(0);
            }

            if (NA == m_sideToJoin || pPlayerInfo->SideID() == m_sideToJoin)
                m_plistPaneTeams->SetSelection(m_pMission->GetSideInfo(trekClient.GetSideID()));
		
            UpdateButtonStates();
            UpdatePromptText();
        }
        UpdateStatusText();
    }

    virtual void OnDelPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParamParam) 
    {
        // if this is me...
        if (trekClient.MyPlayerInfo()->ShipID() == pPlayerInfo->ShipID())
        {
            debugf("TeamScreen::OnDelPlayer: %d %d\n",sideID, reason); // KGJV #104
            ZString strMessage;

            // KGJV #104
            // at this point, we've been removed from a team
            // if we was requesting to join a team, abort the request if needed
            if (m_lastToJoinSend != NA)
            {
                if (m_sideToJoin != m_lastToJoinSend)
                    if( reason != QSR_SwitchingSides) // dont abort the join request if we were removed because we asked to switch team
                        OnCancelRequest();
            }
            switch (reason)
            {
            case QSR_LeaderBooted:
                strMessage = "You have been booted by your team leader!";
                break;

            case QSR_OwnerBooted:
                strMessage = "You have been booted by the mission owner!";
                break;

            case QSR_AdminBooted:
                strMessage = "You have been booted by a server administrator!";
                break;

            case QSR_ServerShutdown:
                break;

            case QSR_SquadChange:
                strMessage = "You have been booted because you are not a member of the new squad.";
                break;

            case QSR_SideDestroyed:
                strMessage = "Your side has been destroyed by a reduction in the number of teams.";
                break;

            case QSR_TeamSizeLimits:
                strMessage = "You have been booted to reduce your team's size.";
                break;

            case QSR_RandomizeSides:
                if (!m_bShowingRandomizeWarning)
                    strMessage = "You have been assigned to a random team.";
                break;

			// TE: Add Flush and Balance messages
			//     Piggyback the RandomizeWarning flag to make the messages appear properly
			case QSR_FlushSides:
				if (!m_bShowingRandomizeWarning)
					strMessage = "You have been removed from your team because the Game Commander flushed all teams.";
				break;

			case QSR_BalanceSides:
				if (!m_bShowingRandomizeWarning)
					strMessage = "You have been removed from your team and assigned to another because the teams have been balanced.";

            case QSR_Quit:
            case QSR_LinkDead:
            case QSR_DuplicateLocalLogon:
            case QSR_DuplicateRemoteLogon:
            case QSR_DuplicateCDKey:
			case QSR_BannedBySteam: // BT - STEAM
                // message box created by OnQuitSide
                break;

            case QSR_SwitchingSides:
                break;
            }

            if (!strMessage.IsEmpty())
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox(strMessage);

                if (reason == QSR_RandomizeSides || 
					reason == QSR_BalanceSides ||	// TE: Added Balance and Flush reasons for the popup
					reason == QSR_FlushSides)
                {
                    m_bShowingRandomizeWarning = true;
                    AddEventTarget(&TeamScreen::OnDismissRandomizeMessageBox, pmsgBox->GetEventSource());
                }

                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
        }
        else
        {
            UpdateButtonStates();
            UpdateStatusText();
        }
    }

    void OnPlayerStatusChange(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        if (trekClient.MyPlayerInfo()->ShipID() == pPlayerInfo->ShipID())
        {
            UpdateButtonStates();
        }
        UpdateStatusText();
    }

    void OnTeamAutoAcceptChange(MissionInfo* pMissionInfo, SideID sideID, bool fAutoAccept)
    {
        if (m_pMission == pMissionInfo && trekClient.GetSideID() == sideID)
        {
            UpdateButtonStates();
        }
    }

    void OnLockLobby(bool bLock)
    {
        UpdateButtonStates();
    }

    void OnLockSides(bool bLock)
    {
        UpdateButtonStates();
		UpdateStatusText();
    }

    void OnTeamForceReadyChange(MissionInfo* pMissionInfo, SideID sideID, bool fTeamForceReady)
    {
        if (m_pMission == pMissionInfo && trekClient.GetSideID() == sideID)
        {
            UpdateButtonStates();
        }
        UpdateStatusText();
    }

    void OnTeamReadyChange(MissionInfo* pMissionDef, SideID sideID, bool fTeamReady) 
    {
        UpdateStatusText();
    }

    virtual void OnEnterMission()
    {
        s_nLastSelectedChatTab = NA;
    }

    virtual void OnMissionCountdown(MissionInfo* pMissionDef)
    {
        UpdateStatusText();
        UpdateButtonStates();
    }

    virtual void OnMissionStarted(MissionInfo* pMissionDef)
    {
        // if this is our mission and we are not on the lobby side...
        if (pMissionDef == m_pMission && trekClient.GetSideID() != SIDE_TEAMLOBBY)
        {
            if (m_pmsgBox)
            {
                GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
                GetWindow()->RestoreCursor();
            }
            m_pmsgBox = CreateMessageBox("The game is starting...", NULL, false, false, 1.0f);
            GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
        }
        UpdateStatusText();
    }

    void OnDelRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason)
    {
        if (pPlayerInfo == trekClient.MyPlayerInfo())
        {
            debugf("TeamScreen::OnDelRequest: %d %d\n",sideID, reason); // KGJV #104
            if (m_pmsgBox)
            {
                GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
                GetWindow()->RestoreCursor();
            }
            m_pmsgBox = NULL;

            if (reason != DPR_Canceled)
            {
                // rejected !!
                ZString strReason;

                switch (reason)
                {
                case DPR_Rejected:
                    strReason = "You have not been accepted to the team.";
                    break;

                case DPR_NoBase:
                    strReason = "You can't join this team - it no longer owns a base.";
                    break;

                case DPR_TeamFull:
                    strReason = "You can't join this team - it's full.";
                    break;

                case DPR_TeamBalance:
                    strReason = "You can't join this team - it would unbalance the teams.";
                    break;

                case DPR_NoJoiners:
                    strReason = "You can't join this team - joiners are not allowed.";
                    break;

                case DPR_NoDefections:
                    strReason = "You can't join this team - defections are not allowed.";
                    break;

                case DPR_WrongSquad:
                    strReason = "You can't join this team - you are not a member of this squad.";
                    break;

                case DPR_OutOfLives:
                    strReason = "You can't join this team - you are out of lives.";
                    break;

                case DPR_Banned:
                    strReason = "You have been removed (i.e. booted) from this team by the commander."; // mmf 09/07 was "You have been banned from this team."
                    break;

                case DPR_SideGone:
                    strReason = "The team you were trying to join has been deleted.";
                    break;

                case DPR_PrivateGame:
                    strReason = "You are not on the invitation list for this team.";
                    break;

                case DPR_DuplicateLogin:
                    strReason = "Someone else is already using that name.";
                    break;

                case DPR_SidesLocked:
                    strReason = "The sides are currently locked.";
                    break;

                case DPR_SideDefeated:
                    strReason = "This side has been defeated - you will need "
                        "to wait until the game ends to join this side.";
                    break;

                case DPR_CantLeadSquad:
                    strReason = "You must be a squad leader or assistant squad "
                        "leader to lead a team in a squads game.";
                    break;

                case DPR_ServerPaused:
                    strReason = "The server is shutting down and is not accepting new players.";
                    break;

				case DPR_BotsOnTeam0Only:
					strReason = "Only pigs are allowed on the yellow team, and you don't have a curly tail.";
					break;

                default:
                    assert(false);
                case DPR_Other:
                    strReason = "You were not able to join the team.";
                    break;
                }

                TRef<IMessageBox> pmsgBox = CreateMessageBox(strReason);
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
            }
            //KGJV #104
            // we got a reject to join, clear m_sideToJoin
            m_sideToJoin = NA;
            m_lastToJoinSend = NA;
            UpdatePromptText();
        }
    }

    virtual void OnTeamNameChange(MissionInfo* pMissionDef, SideID sideID)
    {
        UpdateSquadsStatsMessage();
        UpdatePromptText();
        UpdateStatusText();
    }

    void OnMissionEnded(MissionInfo* pmission)
    {
        UpdateStatusText();
    }

    virtual void OnGameoverPlayers()
    {
        m_pbuttonGameOver->SetHidden(trekClient.GetNumEndgamePlayers() == 0);
    }

    virtual void UpdateSquadsStatsMessage()
    {
        if (!trekClient.MyMission()->GetMissionParams().bSquadGame)
            m_pnumberSquadStatsCount->SetValue(-1);
        else if (trekClient.MyMission()->GetMissionParams().bScoresCount
                && trekClient.MyMission()->GetMissionParams().nTeams == 2
                && trekClient.GetCore()->GetSide(0)->GetSquadID() 
                    != trekClient.GetCore()->GetSide(1)->GetSquadID())
            m_pnumberSquadStatsCount->SetValue(1);
        else
            m_pnumberSquadStatsCount->SetValue(0);
    }
	// KGJV #62
	void OnTeamInactive(MissionInfo* pMissionDef, SideID sideID)
	{
		m_plistPaneTeams->ForceRefresh();
        UpdateStatusText();
        UpdatePromptText();
        UpdateTitleText();
        UpdateButtonStates();
        UpdateSquadsStatsMessage();
	}
};

int TeamScreen::s_nLastSelectedChatTab = NA;
bool TeamScreen::s_bFlash = false;

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateTeamScreen(Modeler* pmodeler)
{
    return new TeamScreen(pmodeler);
}
