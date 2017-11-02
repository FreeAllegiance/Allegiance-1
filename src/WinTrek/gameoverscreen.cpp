#include "pch.h"
#include "badwords.h"

//////////////////////////////////////////////////////////////////////////////
//
// GameOver Screen
//
//////////////////////////////////////////////////////////////////////////////

class GameOverScreen :
    public Screen,
    public EventTargetContainer<GameOverScreen>,
    public IItemEvent::Sink,
    public TEvent<ZString>::Sink,
    public TrekClientEventSink
{
public:
    TRef<Pane>                  m_ppane;
    TRef<ButtonPane>            m_pbuttonBack;
	// KGJV-save stats button
	TRef<ButtonPane>            m_pbuttonSave;
    TRef<TEvent<ZString>::Sink> m_psinkChatEdit;
    TRef<ButtonPane>            m_pbuttonSend;
    TRef<ChatListPane>          m_pchatPane;
    TRef<StringPane>            m_ptextTitle;
    TRef<StringPane>            m_ptextReason;
    TRef<StringPane>            m_ptextCountdown;
    TRef<StringPane>            m_ptextGameCounted;
    TRef<EditPane>              m_peditPane;
    TRef<ButtonBarPane>         m_pbuttonbarStats;
    TRef<ButtonBarPane>         m_pbuttonbarStatsSide;
    TRef<ListPane>              m_plistPaneStats;
    TRef<ListPane>              m_plistPaneStatsSide;

    TRef<IItemEvent::Sink>      m_psinkStats;
    TRef<IItemEvent::Sink>      m_psinkStatsSide;
    TRef<IItemEvent::Source>    m_peventStats;
    TRef<IItemEvent::Source>    m_peventStatsSide;

    TRef<TEvent<ZString>::Source> m_peventChatEdit;
    TRef<IKeyboardInput>        m_pkeyboardInputOldFocus;
    
    TVector<int>                m_viStatsColumns;
    TVector<int>                m_viStatsSideColumns;

    TRef<TListListWrapper<PlayerEndgameInfo*> > m_plistStats;
    TRef<TListListWrapper<SideEndgameInfo*> > m_plistStatsSide;
    
    TRef<List>                                  m_plistSortedStats;
    TRef<List>                                  m_plistSortedStatsSide;

            
    TRef<ModifiableNumber>      m_pnumberSquadGame;

    int m_nPlayerSort;
    bool m_bPlayerSortReverse;

    int m_nSideSort;
    bool m_bSideSortReverse;

    class PlayerPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        GameOverScreen* m_pparent;

    public:

        PlayerPainter(const TVector<int>& viColumns, GameOverScreen* pparent)
            : m_viColumns(viColumns), m_pparent(pparent) {};

        int GetXSize()
        {
            return m_viColumns[9];
        }

        int GetYSize()
        {
            return 14;
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            PlayerEndgameInfo* pplayer = (PlayerEndgameInfo*)pitemArg;

            if (bSelected) 
            {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(1, 0, 0)
                );
            }

            if (pplayer == NULL)
                return;

            int nPosition = m_pparent->m_plistSortedStats->GetIndex(pitemArg);

            // draw the player's position            

            psurface->DrawString(
                TrekResources::SmallFont(),
                (nPosition > 0) ? (0.75 * Color::White()) : Color::White(),
                WinPoint(2, 0),
                (nPosition + 1)
            );

            // draw the player's name, bold if they were the leader, dimmed if the game did not count
            
            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[0] + 4, 0), WinPoint(m_viColumns[1], GetYSize()))); // clip name to fit in column
            psurface->DrawString(
                pplayer->scoring.GetCommander() ? TrekResources::SmallBoldFont() : TrekResources::SmallFont(),
                trekClient.GetEndgameSideColor(pplayer->sideId),
                WinPoint(m_viColumns[0] + 4, 0),
                pplayer->characterName
            );
            psurface->RestoreClipRect(rectClipOld);
            
            // draw the player rank
			// TE: Retrieve the PlayerInfo
			PlayerInfo* pinfo = trekClient.FindPlayer(pplayer->characterName);
			RankID PlayerRankID = pplayer->stats.GetRank();

			// Grab their rank if the player is still ingame
			if (pinfo)
				PlayerRankID = pinfo->GetPersistScore(NA).GetRank();

            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[1] + 4, 0), WinPoint(m_viColumns[2], GetYSize()))); // clip rank to fit in column
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[1] + 4, 0),
                trekClient.LookupRankName(PlayerRankID, trekClient.GetEndgameSideCiv(pplayer->sideId)) // TE: Modified this to pull rank name properly
                ); 
            psurface->RestoreClipRect(rectClipOld);

            // draw the player's ratings
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[2] + 7, 0),
                pplayer->scoring.GetRecentBaseKills()
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[3] + 5, 0),
                pplayer->scoring.GetRecentBaseCaptures()
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[4] + 5, 0),
                (int)pplayer->scoring.GetRecentScore()
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[5] + 5, 0),
                pplayer->scoring.GetRecentAssists()
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[6] + 5, 0),
                pplayer->scoring.GetRecentKills()
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[7] + 9, 0),
                trekClient.GetGameoverEjectPods() 
                    ? pplayer->scoring.GetRecentEjections() 
                    : pplayer->scoring.GetRecentDeaths()
                );

            int nTime = (int)pplayer->scoring.GetRecentTimePlayed();
            int nHours = nTime/(60 * 60);
            int nMinutes = nTime/60 - nHours * 60;
            int nSeconds = nTime - nMinutes * 60 - nHours * 60 * 60;

            ZString strPlayTime = ZString(nHours) 
                + ((nMinutes > 9) ? ":" : ":0") + ZString(nMinutes)
                ;//+ ((nSeconds > 9) ? ":" : ":0") + ZString(nSeconds);

            psurface->DrawString(
                TrekResources::SmallFont(),
                (pplayer->scoring.GetGameCounted() && trekClient.GetScoresCounted()) ? Color::White() : Color::Red(),
                WinPoint(m_viColumns[8] + 5, 0),
                strPlayTime
                );
        }
    };

    class SidePainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        GameOverScreen* m_pparent;

    public:

        SidePainter(const TVector<int>& viColumns, GameOverScreen* pparent)
            : m_viColumns(viColumns), m_pparent(pparent) {};

        int GetXSize()
        {
            return m_viColumns[7];
        }

        int GetYSize()
        {
            return 14;
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            SideEndgameInfo* pside = (SideEndgameInfo*)pitemArg;

            if (bSelected) 
            {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(1, 0, 0)
                );
            }

            if (pside == NULL)
                return;

            int nPosition = m_pparent->m_plistSortedStatsSide->GetIndex(pitemArg);

            // draw the side's position            

            psurface->DrawString(
                TrekResources::SmallFont(),
                (nPosition > 0) ? (0.75 * Color::White()) : Color::White(),
                WinPoint(2, 0),
                (nPosition + 1)
            );

            // draw the side's name
            
            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(m_viColumns[0] + 2, 0), WinPoint(m_viColumns[1], GetYSize()))); // clip name to fit in column
            psurface->DrawString(
                TrekResources::SmallFont(),
                pside->color,
                WinPoint(m_viColumns[0] + 2, 0),
                CensorBadWords (pside->sideName)
            );
            psurface->RestoreClipRect(rectClipOld);
   
            // draw the side's ratings
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[1] + 2, 0),
                pside->cBaseKills
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[2] + 2, 0),
                pside->cBaseCaptures
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[3] + 2, 0),
                pside->cFlags
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[4] + 2, 0),
                pside->cArtifacts
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[5] + 2, 0),
                pside->cKills
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                Color::White(),
                WinPoint(m_viColumns[6] + 2, 0),
                pside->cDeaths
                );
        }
    };


public:
    GameOverScreen(Modeler* pmodeler)
    {
        TRef<IObject> pobjStatsColumns;
        TRef<IObject> pobjStatsSideColumns;
        
        bool bDeaths = !trekClient.GetGameoverEjectPods();

        TRef<INameSpace> pnsData = pmodeler->CreateNameSpace("gameoverscreendata", pmodeler->GetNameSpace("effect"));
        pnsData->AddMember("SquadGame", m_pnumberSquadGame = new ModifiableNumber(0));
        pnsData->AddMember("headerName", new StringValue(bDeaths ? "headerendplayersbmp" : "headerendplayersejectsbmp"));

        TRef<INameSpace> pns = pmodeler->GetNameSpace("gameoverscreen");

        CastTo(m_ppane, pns->FindMember("screen"));


        CastTo(m_pbuttonBack, pns->FindMember("backButtonPane"));
        CastTo(m_pbuttonSave, pns->FindMember("saveButtonPane")); // KGJV- save stats button

        CastTo(m_peditPane,         (Pane*)pns->FindMember("chatEditPane"        ));
        CastTo(m_pbuttonSend,              pns->FindMember("sendButtonPane"      ));
        CastTo(m_pchatPane,         (Pane*)pns->FindMember("chatText"            ));
        CastTo(m_ptextTitle,        (Pane*)pns->FindMember("textTitle"           ));
        CastTo(m_ptextReason,       (Pane*)pns->FindMember("textReason"          ));
        CastTo(m_ptextCountdown,    (Pane*)pns->FindMember("textCountdown"       ));
        CastTo(m_ptextGameCounted,  (Pane*)pns->FindMember("textGameCounted"     ));
        CastTo(m_pbuttonbarStats,          pns->FindMember("statsListHeader"     ));
        CastTo(m_pbuttonbarStatsSide,      pns->FindMember("statsSideListHeader" ));
        CastTo(m_plistPaneStats,    (Pane*)pns->FindMember("statsListPane"       ));
        CastTo(m_plistPaneStatsSide,(Pane*)pns->FindMember("statsSideListPane"   ));
        CastTo(pobjStatsColumns,           pns->FindMember("statsColumns"        ));
        CastTo(pobjStatsSideColumns,       pns->FindMember("statsSideColumns"    ));
        
        
        
        if (trekClient.MyMission()->GetMissionParams().bSquadGame)
            m_pnumberSquadGame->SetValue(1);

        //
        // back button
        // 

        AddEventTarget(&GameOverScreen::OnButtonBack, m_pbuttonBack->GetEventSource());

		// KGJV
		// save button
		AddEventTarget(&GameOverScreen::OnButtonSave, m_pbuttonSave->GetEventSource());

        //
        // Text messages
        // 
        
        if (trekClient.GetWonLastGame())
        {
            m_ptextTitle->SetString(ZString("You are victorious!"));
        }
        else if (trekClient.GetLostLastGame())
        {
            m_ptextTitle->SetString(ZString("You have been defeated."));
        }
        else
        {
            m_ptextTitle->SetString(ZString(""));
        }

        //
        // Player Stats
        //

        m_plistStats = new TListListWrapper<PlayerEndgameInfo*>();
        ParseIntVector(pobjStatsColumns, m_viStatsColumns);
        m_plistPaneStats->SetItemPainter(new PlayerPainter(m_viStatsColumns, this));
        m_peventStats = m_plistPaneStats->GetSelectionEventSource();
        m_peventStats->AddSink(m_psinkStats = new IItemEvent::Delegate(this));

        AddEventTarget(&GameOverScreen::OnButtonBarStats, m_pbuttonbarStats->GetEventSource());
        // WLP - 2005 - changed default to sort by score instead of players - magic number 7(kills) goes to 5(score)
        // OnButtonBarStats(7);
        OnButtonBarStats(5); // WLP sort with low scores on top
        OnButtonBarStats(5); // WLP toggle sort with high scores on top
        //
        // Side Stats
        //

        m_plistStatsSide = new TListListWrapper<SideEndgameInfo*>();
        ParseIntVector(pobjStatsSideColumns, m_viStatsSideColumns);
        m_plistPaneStatsSide->SetItemPainter(new SidePainter(m_viStatsSideColumns, this));
        m_peventStatsSide = m_plistPaneStatsSide->GetSelectionEventSource();
        m_peventStats->AddSink(m_psinkStatsSide = new IItemEvent::Delegate(this));

        AddEventTarget(&GameOverScreen::OnButtonBarStatsSide, m_pbuttonbarStatsSide->GetEventSource());
        OnButtonBarStatsSide(6);

        //
        // Chat 
        //

        GetWindow()->SetChatListPane(m_pchatPane);

        m_peventChatEdit = m_peditPane->GetChangeEvent();
        m_peventChatEdit->AddSink(m_psinkChatEdit = new TEvent<ZString>::Delegate(this));
        m_pkeyboardInputOldFocus = GetWindow()->GetFocus();
        GetWindow()->SetFocus();
        GetWindow()->SetFocus(m_peditPane);

        AddEventTarget(&GameOverScreen::OnButtonSend, m_pbuttonSend->GetEventSource());

        GetWindow()->SetLobbyChatTarget(CHAT_EVERYONE);

        pmodeler->UnloadNameSpace(pns);


		//Imago set AFK #117 7/10
		if (trekClient.MyPlayerInfo()->IsTeamLeader() && trekClient.GetPlayerInfo ()->IsReady())
		{
			trekClient.GetPlayerInfo ()->SetReady(false);
			trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
			BEGIN_PFM_CREATE(trekClient.m_fm, pfmReady, CS, PLAYER_READY)
			END_PFM_CREATE
			pfmReady->fReady = false;
			pfmReady->shipID = trekClient.GetShipID();
		}
		//
		
		OnGameoverStats();
        OnGameoverPlayers();
    } 

    ~GameOverScreen()
    {
        m_peventChatEdit->RemoveSink(m_psinkChatEdit);
        m_peventStats->RemoveSink(m_psinkStats);
        m_peventStatsSide->RemoveSink(m_psinkStatsSide);

        GetWindow()->SetFocus(m_pkeyboardInputOldFocus);
        GetWindow()->SetChatListPane(NULL);
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
        else
        {
            trekClient.SendChat(trekClient.GetShip(), CHAT_EVERYONE, NA,
                                NA, (const char*)strChat);
        }
    }

    bool OnButtonSend()
    {
        ZString str = m_peditPane->GetString();

        if (!str.IsEmpty())
            SendChat(str);
        m_peditPane->SetString(ZString());

        return true;
    }

    bool OnEvent(TEvent<ZString>::Source* pevent, ZString str)
    {
        if (!str.IsEmpty())
            SendChat(str);
        m_peditPane->SetString(ZString());

        return true;
    }

    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        return true;
    }

	// KGJV- SAVE STATS TO XML FILE
	bool OnButtonSave()
	{
		// 1. construct file name with date/time
		SYSTEMTIME sysTimeForName;
		GetLocalTime(&sysTimeForName);

		ZString statsFileName = "scores-";
		statsFileName += sysTimeForName.wYear;
		statsFileName += "-";
		statsFileName += sysTimeForName.wMonth;
		statsFileName += "-";
		statsFileName += sysTimeForName.wDay;
		statsFileName += "_";
		statsFileName += sysTimeForName.wHour;
		statsFileName += ".";
		statsFileName += sysTimeForName.wMinute;
		statsFileName += ".xml";

		// 2. open the file
		FILE* outputFile;
		outputFile = fopen(statsFileName,"wt");
		if (!outputFile)
		{
			statsFileName = "error opening file "+statsFileName;
			trekClient.SendChat(trekClient.MyPlayerInfo()->GetShip(),
				CHAT_INDIVIDUAL,
				trekClient.MyPlayerInfo()->ShipID(),
				NA,
				(const char*)statsFileName);			
			return true;
		}

		// 3. save the results
		// stats are stored (unsorted) in: 
		//    m_plistStats - players stats
		//    m_plistStatsSide - side stats
		// and are stored sorted in
		//	m_plistSortedStats
		//	m_plistSortedStatsSide
		// we save unsorted for now
		
		// store current date/time in a ZString
		char tmpbuf[128];
		ZString dateName;
		_strdate_s( tmpbuf, 128 );
		dateName = tmpbuf;
	    _strtime_s( tmpbuf, 128 );
		dateName += " ";
		dateName += tmpbuf;
		ZString s;

		// generate XML (hand made XML, fiesty!)
		s = "<xml>\n";	fwrite((const char *)s,1,s.GetLength(),outputFile);
		// game info
		s = "<GameInfo>\n";
		s += "  <Date>"; s += dateName; s += "</Date>\n";
		s += "  <Server>"; s+= trekClient.MyMission()->GetMissionDef().szServerName; s += "</Server>\n";
		s += "  <Core>"; s+= trekClient.MyMission()->GetMissionDef().misparms.szIGCStaticFile; s += "</Core>\n";
		s += "  <GameOverReason>"; s += m_ptextReason->GetString(); s += "</GameOverReason>\n";
		s += "</GameInfo>\n"; fwrite((const char *)s,1,s.GetLength(),outputFile);
		// iterate thru teams
		s = "<Teams>\n"; fwrite((const char *)s,1,s.GetLength(),outputFile);
		TListListWrapper<SideEndgameInfo*>::Iterator iterSides(*m_plistStatsSide);
		SideEndgameInfo* si; int n = 0;
		while (!iterSides.End()) 
        {
			s = "  <Team>\n";
			si = iterSides.Value();
			s += "    <TeamNumber>"; s += n++; s += "</TeamNumber>\n";
			s += "    <Name>"; s += si->sideName; s += "</Name>\n";
			s += "    <Faction>"; s += trekClient.GetCore()->GetCivilization(si->civID)->GetName(); s += "</Faction>\n";
			s += "  </Team>\n";
			fwrite((const char *)s,1,s.GetLength(),outputFile);

			iterSides.Next();
		}
		s = "</Teams>\n"; fwrite((const char *)s,1,s.GetLength(),outputFile);
		// end - iterate thru teams

		// iterate thru players
		s = "<PlayersScore>\n";	fwrite((const char *)s,1,s.GetLength(),outputFile);
	
		TListListWrapper<PlayerEndgameInfo*>::Iterator iterPlayers(*m_plistStats);
		PlayerEndgameInfo* pi;
		while (!iterPlayers.End()) 
        {
			s = "  <PlayerScore>\n";
            pi = iterPlayers.Value();
			s += "    <Name>"; s += pi->characterName; s += "</Name>\n";
			s += "    <TeamNumber>"; s += pi->sideId; s += "</TeamNumber>\n";
			s += "    <IsCommander>"; s += pi->scoring.GetCommander()? "true" : "false" ; s += "</IsCommander>\n";
			s += "    <Rank>"; s += pi->scoring.GetRank(); s += "</Rank>\n";
			s += "    <Assists>"; s += pi->scoring.GetRecentAssists(); s += "</Assists>\n";
			s += "    <BaseCaptures>"; s += pi->scoring.GetRecentBaseCaptures(); s += "</BaseCaptures>\n";
			s += "    <BaseKills>"; s += pi->scoring.GetRecentBaseKills(); s += "</BaseKills>\n";
			s += "    <Deaths>"; s += pi->scoring.GetRecentDeaths(); s += "</Deaths>\n";
			s += "    <Ejections>"; s += pi->scoring.GetRecentEjections(); s += "</Ejections>\n";
			s += "    <Kills>"; s += pi->scoring.GetRecentKills(); s += "</Kills>\n";
			s += "    <Score>"; s += pi->scoring.GetRecentScore(); s += "</Score>\n";
			s += "    <TimePlayed>"; s += pi->scoring.GetRecentTimePlayed(); s += "</TimePlayed>\n";
			s += "  </PlayerScore>\n";
			fwrite((const char *)s,1,s.GetLength(),outputFile);
            iterPlayers.Next();
        }
		s = "</PlayersScore>\n"; fwrite((const char *)s,1,s.GetLength(),outputFile);
		// end - iterate thru players

		s = "</xml>\n";	fwrite((const char *)s,1,s.GetLength(),outputFile);

		fclose(outputFile);

		// 4. inform player
		statsFileName = "scores saved to "+statsFileName;
		trekClient.SendChat(trekClient.MyPlayerInfo()->GetShip(),
			CHAT_INDIVIDUAL,
			trekClient.MyPlayerInfo()->ShipID(),
			NA,
			(const char*)statsFileName);			
		return true;
	}

    bool OnButtonBack()
    {
        if (trekClient.MyMission()->GetStage() == STAGE_OVER)
        {
            GetWindow()->StartQuitMission();
        }
        else
        {
            // set ourselves to ready and go back to the team lobby screen
            // REVIEW: This does not make much sense with the silly away from keyboard thing
            /*trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmReady, CS, PLAYER_READY)
            END_PFM_CREATE
            pfmReady->fReady = true;
            pfmReady->shipID = trekClient.GetShipID();
            */
            GetWindow()->screen(ScreenIDTeamScreen);
        }

        return true;
    }

    virtual void OnFrame()
    {
        UpdateCountdownText();
    }

    void UpdateCountdownText()
    {
        STAGE stage = trekClient.MyMission()->GetStage();

        if (stage == STAGE_STARTING
            && trekClient.GetSideID() != SIDE_TEAMLOBBY)
        {
            GetWindow()->screen(ScreenIDTeamScreen);
        }
        else if (stage == STAGE_STARTING
            || (trekClient.MyMission()->GetMissionParams().bAutoRestart && stage == STAGE_NOTSTARTED))
        {
            // note: have the timer lag by 1 second to give users the familiar countdown feel
            int nTimeLeft = max(0, int(trekClient.MyMission()->GetMissionParams().timeStart - Time::Now()) + 1);

            int nMinutesLeft = nTimeLeft/60;
            int nSecondsLeft = nTimeLeft - nMinutesLeft * 60;

            if (nMinutesLeft > 0)
                m_ptextCountdown->SetString(ZString(nMinutesLeft) + 
                    ((nSecondsLeft > 9) ? ":" : ":0") + ZString(nSecondsLeft));
            else
                m_ptextCountdown->SetString(ZString(nSecondsLeft));
        }
        else
        {
            m_ptextCountdown->SetString("");
        }
    }

    virtual void OnMissionStarted(MissionInfo* pMissionDef)
    {
        // if this is our mission and we are not on the lobby side...
        if (pMissionDef == trekClient.MyMission() && trekClient.GetSideID() != SIDE_TEAMLOBBY)
        {
            // switch back the the lobby screen.
            GetWindow()->screen(ScreenIDTeamScreen);
            
            GetWindow()->SetWaitCursor();
            TRef<IMessageBox> pmsgBox = CreateMessageBox("The game is starting...", NULL, false, false, 1.0f);
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
    }

    virtual void OnGameoverStats()
    {
        m_plistStatsSide->SetEmpty();

        for (SideID iSideIndex = 0; iSideIndex < trekClient.GetNumEndgameSides(); ++iSideIndex)
        {
            m_plistStatsSide->PushEnd(trekClient.GetSideEndgameInfo(iSideIndex));
        }
        
        m_ptextReason->SetString (CensorBadWords (trekClient.GetGameoverMessage()));
        RefreshSides();
    };

    virtual void OnGameoverPlayers()
    {
        m_plistStats->SetEmpty();

        for (int iPlayerIndex = 0; iPlayerIndex < trekClient.GetNumEndgamePlayers(); ++iPlayerIndex)
        {
            m_plistStats->PushEnd(trekClient.GetEndgamePlayerInfo(iPlayerIndex));
        }
        
		// BT - STEAM - Scores always count.
        /*if (!trekClient.GetGameCounted() || !trekClient.GetScoresCounted())
        {
            if (trekClient.GetWonLastGame())
                m_ptextGameCounted->SetString("Win not counted");
            else if (trekClient.GetLostLastGame())
                m_ptextGameCounted->SetString("Loss not counted");
            else
                m_ptextGameCounted->SetString("");
        }
        else*/
            m_ptextGameCounted->SetString("");

        RefreshPlayers();
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

    static bool SortName(ItemID id1, ItemID id2)
    {
        return _stricmp(((PlayerEndgameInfo*)id1)->characterName, ((PlayerEndgameInfo*)id2)->characterName) > 0;
    }

    static bool SortRank(ItemID id1, ItemID id2)
    {
        PlayerEndgameInfo* pplayer1 = ((PlayerEndgameInfo*)id1);
        PlayerEndgameInfo* pplayer2 = ((PlayerEndgameInfo*)id2);
        
        RankID rank1 = pplayer1->scoring.GetRank();
        RankID rank2 = pplayer2->scoring.GetRank();
        if (rank1 == rank2)
            return trekClient.GetEndgameSideCiv(pplayer1->sideId) > trekClient.GetEndgameSideCiv(pplayer2->sideId);
        else
            return rank1 > rank2;
    }

    static bool SortBaseKills(ItemID id1, ItemID id2)
    {
        return ((PlayerEndgameInfo*)id1)->scoring.GetRecentBaseKills() > ((PlayerEndgameInfo*)id2)->scoring.GetRecentBaseKills();
    }

    static bool SortBaseCaptures(ItemID id1, ItemID id2)
    {
        return ((PlayerEndgameInfo*)id1)->scoring.GetRecentBaseCaptures() > ((PlayerEndgameInfo*)id2)->scoring.GetRecentBaseCaptures();
    }

    static bool SortRecentScore(ItemID id1, ItemID id2)
    {
        return ((PlayerEndgameInfo*)id1)->scoring.GetRecentScore() > ((PlayerEndgameInfo*)id2)->scoring.GetRecentScore();
    }

    static bool SortRecentAssists(ItemID id1, ItemID id2)
    {
        return ((PlayerEndgameInfo*)id1)->scoring.GetRecentAssists() > ((PlayerEndgameInfo*)id2)->scoring.GetRecentAssists();
    }

    static bool SortKills(ItemID id1, ItemID id2)
    {
        return ((PlayerEndgameInfo*)id1)->scoring.GetRecentKills() > ((PlayerEndgameInfo*)id2)->scoring.GetRecentKills();
    }

    static bool SortDeaths(ItemID id1, ItemID id2)
    {
        GameOverScoreObject& gso1 = ((PlayerEndgameInfo*)id1)->scoring;
        GameOverScoreObject& gso2 = ((PlayerEndgameInfo*)id2)->scoring;

        if (trekClient.GetGameoverEjectPods())
            return gso1.GetRecentEjections() > gso2.GetRecentEjections();
        else
            return gso1.GetRecentDeaths() > gso2.GetRecentDeaths();
    }

    static bool SortTimePlayed(ItemID id1, ItemID id2)
    {
        return ((PlayerEndgameInfo*)id1)->scoring.GetRecentTimePlayed() 
            > ((PlayerEndgameInfo*)id2)->scoring.GetRecentTimePlayed();
    }

    static bool SortSideName(ItemID id1, ItemID id2)
    {
        return _stricmp(((SideEndgameInfo*)id1)->sideName, ((SideEndgameInfo*)id2)->sideName) > 0;
    }
        
    static bool SortSideBaseKills(ItemID id1, ItemID id2)
    {
        return ((SideEndgameInfo*)id1)->cBaseKills > ((SideEndgameInfo*)id2)->cBaseKills;
    }

    static bool SortSideBaseCaptures(ItemID id1, ItemID id2)
    {
        return ((SideEndgameInfo*)id1)->cBaseCaptures > ((SideEndgameInfo*)id2)->cBaseCaptures;
    }

    static bool SortSideFlags(ItemID id1, ItemID id2)
    {
        return ((SideEndgameInfo*)id1)->cFlags > ((SideEndgameInfo*)id2)->cFlags;
    }

    static bool SortSideArtifacts(ItemID id1, ItemID id2)
    {
        return ((SideEndgameInfo*)id1)->cArtifacts > ((SideEndgameInfo*)id2)->cArtifacts;
    }

    static bool SortSideKills(ItemID id1, ItemID id2)
    {
        return ((SideEndgameInfo*)id1)->cKills > ((SideEndgameInfo*)id2)->cKills;
    }

    static bool SortSideDeaths(ItemID id1, ItemID id2)
    {
        return ((SideEndgameInfo*)id1)->cDeaths > ((SideEndgameInfo*)id2)->cDeaths;
    }

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

    bool OnButtonBarStatsSide(int iButton)
    {
        // if they click on the position button, treat it as if they clicked on
        // the button defining the current sort order.
        if (iButton == 0)
        {
            iButton = 1;

            for (int i = 2; i < 8; i++)
            {
                if (m_pbuttonbarStatsSide->GetChecked(i) 
                    || m_pbuttonbarStatsSide->GetChecked2(i)
                    )
                {
                    iButton = i;
                }
            }
        }

        bool bIsChecked = m_pbuttonbarStatsSide->GetChecked2(iButton);

        // do radio-button behavior
        for (int i = 0; i < 8; i++)
        {
            m_pbuttonbarStatsSide->SetChecked(i, false);
            m_pbuttonbarStatsSide->SetChecked2(i, false);
        }
        
        if (bIsChecked)
            m_pbuttonbarStatsSide->SetChecked(iButton, true);
        else
            m_pbuttonbarStatsSide->SetChecked2(iButton, true);

        m_nSideSort = iButton;
        m_bSideSortReverse = bIsChecked;

        RefreshSides();

        return true;
    }

    void RefreshSides()
    {
        switch (m_nSideSort)
        {
        case 1:
            m_plistSortedStatsSide = SortingList(m_plistStatsSide, SortSideName, m_bSideSortReverse);
            break;

        case 2:
            m_plistSortedStatsSide = SortingList(m_plistStatsSide, SortSideBaseKills, m_bSideSortReverse);
            break;

        case 3:
            m_plistSortedStatsSide = SortingList(m_plistStatsSide, SortSideBaseCaptures, m_bSideSortReverse);
            break;

        case 4:
            m_plistSortedStatsSide = SortingList(m_plistStatsSide, SortSideFlags, m_bSideSortReverse);
            break;

        case 5:
            m_plistSortedStatsSide = SortingList(m_plistStatsSide, SortSideArtifacts, m_bSideSortReverse);
            break;

        case 6:
            m_plistSortedStatsSide = SortingList(m_plistStatsSide, SortSideKills, m_bSideSortReverse);
            break;

        case 7:
            m_plistSortedStatsSide = SortingList(m_plistStatsSide, SortSideDeaths, m_bSideSortReverse);
            break;

        default:
            assert(false);
            m_plistSortedStatsSide = m_plistStatsSide;
            break;
        }

        m_plistPaneStatsSide->SetList(m_plistSortedStatsSide);
    }

    bool OnButtonBarStats(int iButton)
    {
        // if they click on the position button, treat it as if they clicked on
        // the button defining the current sort order.
        if (iButton == 0)
        {
            iButton = 1;

            for (int i = 2; i < 10; i++)
            {
                if (m_pbuttonbarStats->GetChecked(i) 
                    || m_pbuttonbarStats->GetChecked2(i)
                    )
                {
                    iButton = i;
                }
            }
        }

        bool bIsChecked = m_pbuttonbarStats->GetChecked2(iButton);

        // do radio-button behavior
        for (int i = 0; i < 10; i++)
        {
            m_pbuttonbarStats->SetChecked(i, false);
            m_pbuttonbarStats->SetChecked2(i, false);
        }
        
        if (bIsChecked)
            m_pbuttonbarStats->SetChecked(iButton, true);
        else
            m_pbuttonbarStats->SetChecked2(iButton, true);

        m_nPlayerSort = iButton;
        m_bPlayerSortReverse = bIsChecked;
        
        RefreshPlayers();

        return true;
    };

    void RefreshPlayers()
    {
        switch (m_nPlayerSort)
        {
        case 1:
            m_plistSortedStats = SortingList(m_plistStats, SortName, m_bPlayerSortReverse);
            break;

        case 2:
            m_plistSortedStats = SortingList(m_plistStats, SortRank, m_bPlayerSortReverse);
            break;

        case 3:
            m_plistSortedStats = SortingList(m_plistStats, SortBaseKills, m_bPlayerSortReverse);
            break;

        case 4:
            m_plistSortedStats = SortingList(m_plistStats, SortBaseCaptures, m_bPlayerSortReverse);
            break;

        case 5:
            m_plistSortedStats = SortingList(m_plistStats, SortRecentScore, m_bPlayerSortReverse);
            break;

        case 6:
            m_plistSortedStats = SortingList(m_plistStats, SortRecentAssists, m_bPlayerSortReverse);
            break;

        case 7:
            m_plistSortedStats = SortingList(m_plistStats, SortKills, m_bPlayerSortReverse);
            break;

        case 8:
            m_plistSortedStats = SortingList(m_plistStats, SortDeaths, m_bPlayerSortReverse);
            break;

        case 9:
            m_plistSortedStats = SortingList(m_plistStats, SortTimePlayed, m_bPlayerSortReverse);
            break;

        default:
            assert(false);
            m_plistSortedStats = m_plistStats;
            break;
        }

        m_plistPaneStats->SetList(m_plistSortedStats);
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
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateGameOverScreen(Modeler* pmodeler)
{
    return new GameOverScreen(pmodeler);
}
