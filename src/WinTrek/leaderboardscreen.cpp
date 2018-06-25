#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// LeaderBoard Screen
//
//////////////////////////////////////////////////////////////////////////////

extern bool    g_bDisableZoneClub;

class LeaderBoardScreen :
    public Screen,
    public EventTargetContainer<LeaderBoardScreen>,
    public IItemEvent::Sink,
    public TEvent<ZString>::Sink,
    public TrekClientEventSink
{
public:
    TRef<INameSpace>            m_pns;
    TRef<Pane>                  m_ppane;
    TRef<ButtonPane>            m_pbuttonBack;
    TRef<ButtonPane>            m_pbuttonPlayerProfile;
    TRef<ButtonPane>            m_pbuttonPlayerProfile2;
    TRef<ButtonPane>            m_pbuttonSquadProfile;
    TRef<ButtonPane>            m_pbuttonSquads;
    TRef<ButtonPane>            m_pbuttonWeb;
    TRef<TEvent<ZString>::Sink> m_psinkSearchEdit;
    TRef<ButtonPane>            m_pbuttonSearch;
    TRef<ButtonPane>            m_pbuttonFindMe;
    TRef<ButtonPane>            m_pbuttonTop;
    TRef<ButtonPane>            m_pbuttonPageUp;
    TRef<ButtonPane>            m_pbuttonPageDown;
    TRef<EditPane>              m_peditPane;
    TRef<ButtonBarPane>         m_pbuttonbarStats;
    TRef<ListPane>              m_plistTopPlayer;
    TRef<ListPane>              m_plistPaneStats;
    TRef<WrapImage>             m_pwrapImageCiv;
    TRef<ComboPane>             m_pcomboCiv;
    TRef<IItemEvent::Sink>      m_psinkStats;
    TRef<IItemEvent::Source>    m_peventStats;

    TRef<TEvent<ZString>::Source> m_peventSearchEdit;
    TRef<IKeyboardInput>        m_pkeyboardInputOldFocus;
    
    TVector<int>                m_viStatsColumns;

    enum { cLinesPerScreen = 28 };

    ZString                     m_strBasis;
    int                         m_idBasis;
    LeaderBoardEntry            m_vEntries[cLinesPerScreen * 3];
    LeaderBoardEntry            m_entryNumber1; // the highest or lowest ranked player
    int                         m_cEntries;
    int                         m_nOffset;
    CivID                       m_civId;

    static LeaderBoardScreen*   s_pLeaderBoard;

    enum StatsColumn
    {
        colPosition,
        colPlayerName,
        colRank,
        colOverallRating,
        colCombatRating,
        colGamesPlayed,
        colTimePlayed,
        colCommanderWins,
        colWins,
        colLosses,
        colKills,
        colDeaths,
        colBaseKills,
        colBaseCaptures,
        maxCol
    };

    class PlayerPainter : public ItemPainter
    {
        const TVector<int>& m_viColumns;
        LeaderBoardScreen* m_pparent;

    public:

        PlayerPainter(const TVector<int>& viColumns, LeaderBoardScreen* pparent)
            : m_viColumns(viColumns), m_pparent(pparent) {};

        int GetXSize()
        {
            return m_viColumns[maxCol - 1];
        }

        int GetYSize()
        {
            return 12;
        }

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            if (pitemArg == NULL)
                return;

            if (bSelected) 
            {
                psurface->FillRect(
                    WinRect(0, 0, GetXSize(), GetYSize()),
                    Color(1, 0, 0)
                );
            }

            PaintText(pitemArg, psurface, 0.85f * Color::White());
        }


        int GetColOffset(StatsColumn col)
        {
            assert(col >= 0 && col < maxCol);

            if (col == 0)
                return 0;
            else
                return m_viColumns[col - 1];
        }

        void PaintText(ItemID pitemArg, Surface* psurface, Color color)
        {
            LeaderBoardEntry* pplayer = (LeaderBoardEntry*)pitemArg;

            if (pplayer == NULL)
                return;

            // draw the player's position            

            if (m_pparent->FindSortColumn() != colPlayerName)
            {
                psurface->DrawString(
                    TrekResources::SmallFont(),
                    color,
                    WinPoint(GetColOffset(colPosition) + 2, 0),
                    pplayer->nPosition
                );
            }

            // draw the player's name
            WinRect rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(GetColOffset(colPlayerName) + 2, 0), WinPoint(GetColOffset(colOverallRating), GetYSize()))); // clip name to fit in column
            
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colPlayerName) + 2, 0),
                pplayer->CharacterName
            );

            psurface->RestoreClipRect(rectClipOld);
            

            // draw the player rank
            rectClipOld = psurface->GetClipRect();
            psurface->SetClipRect(WinRect(WinPoint(GetColOffset(colRank) + 2, 0), WinPoint(GetColOffset(colOverallRating), GetYSize()))); // clip rank to fit in column

            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colRank) + 2, 0),
                trekClient.LookupRankName(pplayer->rank, m_pparent->m_civId)
                );

            psurface->RestoreClipRect(rectClipOld);

            // draw the player's ratings
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colOverallRating) + 5, 0),
                (int)pplayer->fTotalScore
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colCombatRating) + 3, 0),
                pplayer->rating
                );
            int nHoursPlayed = pplayer->nMinutesPlayed / 60;
            int nMinutesPlayed = pplayer->nMinutesPlayed - nHoursPlayed * 60;
            
            ZString strTimePlayed = ZString(nHoursPlayed) + 
                (nHoursPlayed > 999 ? 
                    "h" : 
                    ((nMinutesPlayed < 10) ? ":0" : ":") + ZString(nMinutesPlayed));
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colTimePlayed) + 3, 0),
                strTimePlayed
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colCommanderWins) + 3, 0),
                pplayer->cTotalCommanderWins
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colGamesPlayed) + 3, 0),
                pplayer->cTotalGamesPlayed
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colWins) + 3, 0),
                pplayer->cTotalWins
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colLosses) + 3, 0),
                pplayer->cTotalLosses
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colKills) + 3, 0),
                pplayer->cTotalKills
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colDeaths) + 3, 0),
                pplayer->cTotalDeaths
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colBaseKills) + 3, 0),
                pplayer->cTotalBaseKills
                );
            psurface->DrawString(
                TrekResources::SmallFont(),
                color,
                WinPoint(GetColOffset(colBaseCaptures) + 3, 0),
                pplayer->cTotalBaseCaptures
                );
        }
    };

    class TopPlayerPainter : public PlayerPainter
    {
    public:

        TopPlayerPainter(const TVector<int>& viColumns, LeaderBoardScreen* pparent)
            : PlayerPainter(viColumns, pparent) {};

        void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
        {
            PaintText(pitemArg, psurface, Color::White());
        }
    };

public:
    LeaderBoardScreen(Modeler* pmodeler, const ZString& strCharacter) :
      m_nOffset(0)
    {
        TRef<IObject> pobjStatsColumns;
        TRef<INameSpace> pnsLeaderBoardScreenData = GetModeler()->CreateNameSpace("leaderboardscreendata");
        pnsLeaderBoardScreenData->AddMember("civImage", (Value*)(m_pwrapImageCiv = new WrapImage(Image::GetEmpty())));

        m_pns = pmodeler->GetNameSpace("LeaderBoardscreen");
        CastTo(m_ppane, m_pns->FindMember("screen"));

        CastTo(m_pbuttonBack, m_pns->FindMember("backButtonPane"));
        CastTo(m_pbuttonPlayerProfile, m_pns->FindMember("playerProfileButtonPane"));
        CastTo(m_pbuttonPlayerProfile2, m_pns->FindMember("playerProfile2ButtonPane"));
        CastTo(m_pbuttonSquadProfile, m_pns->FindMember("squadProfileButtonPane"));
        CastTo(m_pbuttonSquads, m_pns->FindMember("squadsButtonPane"));
        CastTo(m_pbuttonWeb, m_pns->FindMember("webButtonPane"));

        CastTo(m_pcomboCiv, m_pns->FindMember("civFilterComboPane"));
        
        CastTo(m_peditPane, (Pane*)m_pns->FindMember("searchEditPane"));
        CastTo(m_pbuttonSearch, m_pns->FindMember("searchButtonPane"));
        CastTo(m_pbuttonFindMe, m_pns->FindMember("findMeButtonPane"));

        CastTo(m_pbuttonbarStats, m_pns->FindMember("statsListHeader"));
        CastTo(m_plistPaneStats, (Pane*)m_pns->FindMember("statsListPane"));
        CastTo(m_plistTopPlayer, (Pane*)m_pns->FindMember("topListPane"));
        CastTo(pobjStatsColumns, m_pns->FindMember("statsColumns"));

        CastTo(m_pbuttonTop, m_pns->FindMember("topButtonPane"));
        CastTo(m_pbuttonPageUp, m_pns->FindMember("pageUpButtonPane"));
        CastTo(m_pbuttonPageDown, m_pns->FindMember("pageDownButtonPane"));


        //
        // Buttons
        //
        
		// mdvalley: Pointer. Class.
        AddEventTarget(&LeaderBoardScreen::OnButtonBack, m_pbuttonBack->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonFindMe, m_pbuttonFindMe->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonPlayerProfile, m_pbuttonPlayerProfile->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonZoneWeb, m_pbuttonWeb->GetEventSource());

        AddEventTarget(&LeaderBoardScreen::OnButtonPlayerProfile, m_pbuttonPlayerProfile2->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonSquadProfile, m_pbuttonSquadProfile->GetEventSource());
        
        m_pbuttonSquadProfile->SetEnabled(!g_bDisableZoneClub);
        m_pbuttonSquads->SetEnabled(!g_bDisableZoneClub);
        
        AddEventTarget(&LeaderBoardScreen::OnButtonSquadProfile, m_pbuttonSquads->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonTop, m_pbuttonTop->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonPageUp, m_pbuttonPageUp->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonPageUp, m_pbuttonPageUp->GetDoubleClickEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonPageDown, m_pbuttonPageDown->GetEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonPageDown, m_pbuttonPageDown->GetDoubleClickEventSource());
        AddEventTarget(&LeaderBoardScreen::OnButtonBarStats, m_pbuttonbarStats->GetEventSource());


        //
        // civ combo
        //

        FillCombo(m_pcomboCiv, "CivNames");
        AddEventTarget(&LeaderBoardScreen::OnCivChange, m_pcomboCiv->GetEventSource());


        //
        // search box
        //

        m_peditPane->SetTextColor(Color::Black());
        m_peventSearchEdit = m_peditPane->GetChangeEvent();
// syntax error ??
        m_peventSearchEdit->AddSink(m_psinkSearchEdit = new TEvent<ZString>/*>*/::Delegate(this));
        m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
        GetEngineWindow()->SetFocus();
        GetEngineWindow()->SetFocus(m_peditPane);
        
        m_strBasis = strCharacter.IsEmpty() ? trekClient.GetNameLogonClubServer() : strCharacter;
        m_idBasis = strCharacter.IsEmpty() ? -2 : -1;

        AddEventTarget(&LeaderBoardScreen::OnButtonSearch, m_pbuttonSearch->GetEventSource());
        m_cEntries = 0;

        s_pLeaderBoard = this;

        //
        // Player Stats
        //

        ParseIntVector(pobjStatsColumns, m_viStatsColumns);
        m_plistPaneStats->SetItemPainter(new PlayerPainter(m_viStatsColumns, this));
        m_plistTopPlayer->SetItemPainter(new TopPlayerPainter(m_viStatsColumns, this));
        m_peventStats = m_plistPaneStats->GetSelectionEventSource();
        m_peventStats->AddSink(m_psinkStats = new IItemEvent::Delegate(this));

        m_pbuttonbarStats->SetChecked(colOverallRating, true);

        OnCivChange(0);
    } 

    ~LeaderBoardScreen()
    {
        m_peventSearchEdit->RemoveSink(m_psinkSearchEdit);
        m_peventStats->RemoveSink(m_psinkStats);

        GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
        GetModeler()->UnloadNameSpace(m_pns);

        s_pLeaderBoard = NULL;
    }

    static void ForwardLeaderBoardMessage(FEDMESSAGE * pLeaderBoardMessage)
    {
        if (s_pLeaderBoard)
        {
            s_pLeaderBoard->HandleLeaderBoardMessage(pLeaderBoardMessage);
        }
    }

    void FillCombo(ComboPane* pcombo, const char* szContentName)
    {
        IObjectList* plist; 
        
        CastTo(plist, m_pns->FindMember(szContentName));

        plist->GetFirst();

        int index = 0;
        while (plist->GetCurrent() != NULL) {
            pcombo->AddItem(GetString(plist->GetCurrent()), index);
            plist->GetNext();
            ++index;
        }
    }

    float FindFloatValue(int index, const char* szTableName)
    {
        IObjectList* plist; 
        
        CastTo(plist, m_pns->FindMember(szTableName));

        plist->GetFirst();

        while (index > 0) {
            plist->GetNext();
            --index;
        }

        return GetNumber(plist->GetCurrent());
    }

    int FindIntValue(int index, const char* szTableName)
    {
        return (int)FindFloatValue(index, szTableName);
    }

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
    }

    bool OnButtonSearch()
    {
        ZString str = m_peditPane->GetString();

        if (!str.IsEmpty())
        {
            m_strBasis = str;
            m_idBasis = NA;
            ClearListBoxes();
            UpdateData();
        }
        m_peditPane->SetString(ZString());

        return true;
    }

    void UpdateButtons()
    {
        m_pbuttonPlayerProfile->SetEnabled(m_cEntries > 0);
        m_pbuttonPlayerProfile2->SetEnabled(m_cEntries > 0);
        m_pbuttonSquadProfile->SetEnabled(!g_bDisableZoneClub && m_cEntries > 0);
        m_pbuttonSquads->SetEnabled(!g_bDisableZoneClub && m_cEntries > 0);

        m_pbuttonPageUp->SetEnabled(CanPageUp());
        m_pbuttonPageDown->SetEnabled(CanPageDown());
    }

    void ClearListBoxes()
    {
        m_cEntries = 0;
        UpdateListBoxes();
    }

    bool OnEvent(TEvent<ZString>::Source* pevent, ZString str)
    {
        if (!str.IsEmpty())
        {
            m_strBasis = str;
            m_idBasis = NA;
            ClearListBoxes();
            UpdateData();
        }
        m_peditPane->SetString(ZString());

        return true;
    }

    bool OnCivChange(int index)
    {
        int civID = FindIntValue(index, "CivIDs");
        
        m_pwrapImageCiv->SetImage(GetModeler()->LoadImage(FindStringValue(index, "CivBackgrounds"), true));

        ClearListBoxes();
        UpdateData();
        return true;
    }

    bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
    {
        if (pitem)
        {
            // set the basis to be the selected character
            m_strBasis = ((LeaderBoardEntry*)pitem)->CharacterName;
            m_idBasis = ((LeaderBoardEntry*)pitem)->idCharacter;
        }

        return true;
    }

    bool OnButtonBack()
    {
        GetWindow()->screen(ScreenIDZoneClubScreen);

        return true;
    }

    bool OnButtonFindMe()
    {
        m_strBasis = trekClient.m_szClubCharName;
        m_idBasis = NA;
        ClearListBoxes();
        UpdateData();
        return true;
    }

    bool OnButtonPlayerProfile()
    {
        if (m_cEntries > 0 && !m_strBasis.IsEmpty())
        {
            GetWindow()->CharInfoScreenForPlayer(m_idBasis);
        }

        return true;
    }

    bool OnButtonSquadProfile()
    {
        if (m_cEntries > 0 && !m_strBasis.IsEmpty())
        {
            GetWindow()->SquadScreenForPlayer(PCC(m_strBasis), m_idBasis, NULL);
        }

        return true;
    }

    bool OnButtonZoneWeb()
    {
        // note: users can also access web page from the introscreen
        GetWindow()->ShowWebPage();
        return true;
    }

    bool OnButtonGames()
    {

        return true;
    }

    bool OnButtonTop()
    {
        m_strBasis = "";
        m_idBasis = NA;
        ClearListBoxes();
        UpdateData();
        return true;
    }

    int CalcPageUpBasis()
    {
        if (m_cEntries <= 0)
            return m_nOffset;
        else
            return std::max(0, m_nOffset - cLinesPerScreen + cLinesPerScreen/2); // rounding trick
    }

    bool CanPageUp()
    {
        return (CalcPageUpBasis() < m_nOffset);
    }

    bool OnButtonPageUp()
    {
        if (CanPageUp())
        {
            int indexBasis = CalcPageUpBasis();
            m_strBasis = m_vEntries[indexBasis].CharacterName;
            m_idBasis = m_vEntries[indexBasis].idCharacter;
            UpdateListBoxes();
            UpdateData();
        }

        return true;
    }

    int CalcPageDownBasis()
    {
        if (m_cEntries <= 0)
            return m_nOffset;
        else
            return std::min(m_cEntries - 1, m_nOffset + cLinesPerScreen + cLinesPerScreen/2); 
    }

    bool CanPageDown()
    {
        return (CalcPageDownBasis() >= m_nOffset + cLinesPerScreen);
    }

    bool OnButtonPageDown()
    {
        if (CanPageDown())
        {
            int indexBasis = CalcPageDownBasis();
            m_strBasis = m_vEntries[indexBasis].CharacterName;
            m_idBasis = m_vEntries[indexBasis].idCharacter;
            UpdateListBoxes();
            UpdateData();
        }

        return true;
    }

    StatsColumn FindSortColumn()
    {
        for (int i = 0; i <= maxCol; i++)
        {
            if (m_pbuttonbarStats->GetChecked(i) 
                || m_pbuttonbarStats->GetChecked2(i)
                )
            {
                return (StatsColumn)i;
            }
        }

        ZError("unreached");
        return colPlayerName;
    };

    bool OnButtonBarStats(int iButton)
    {
        // if they click on the position button, ignore the click.
        if (iButton == colPosition)
            return true;

        bool bReverse = false;//m_pbuttonbarStats->GetChecked(iButton);
        ItemID pselection = m_plistPaneStats->GetSelection();

        // do radio-button behavior
        for (int i = 0; i < maxCol; i++)
        {
            m_pbuttonbarStats->SetChecked(i, false);
            m_pbuttonbarStats->SetChecked2(i, false);
        }
        
        if (bReverse)
            m_pbuttonbarStats->SetChecked2(iButton, true);
        else
            m_pbuttonbarStats->SetChecked(iButton, true);

        ClearListBoxes();
        UpdateData();

        return true;
    }

    LeaderBoardSort ColumnToSort(StatsColumn col)
    {
        switch (col)
        {
        case colPlayerName:
            return lbSortName;

        case colRank:
            return lbSortRank;

        case colOverallRating:
            return lbSortScore;

        case colCombatRating:
            return lbSortRating;

        case colTimePlayed:
            return lbSortTimePlayed;

        case colCommanderWins:
            return lbSortCommanderWins;

        case colGamesPlayed:
            return lbSortGamesPlayed;

        case colWins:
            return lbSortWins;

        case colLosses:
            return lbSortLosses;

        case colKills:
            return lbSortKills;

        case colDeaths:
            return lbSortDeaths;

        case colBaseKills:
            return lbSortBaseKills;

        case colBaseCaptures:
            return lbSortBaseCaptures;

        case colPosition:
        default:
            ZError("invalid column");
            return lbSortScore;
        };
    }

    void UpdateData()
    {
        StatsColumn sortCol = FindSortColumn();

        BEGIN_PFM_CREATE(trekClient.m_fmClub, pfmLeaderBoardQuery, C, LEADER_BOARD_QUERY)
        END_PFM_CREATE
        if ((m_idBasis != NA && m_idBasis != -2) || m_strBasis.IsEmpty())
            pfmLeaderBoardQuery->szBasis[0] = '\0';
        else
            strcpy(pfmLeaderBoardQuery->szBasis, m_strBasis);
        pfmLeaderBoardQuery->idBasis = m_idBasis;
        pfmLeaderBoardQuery->civid = m_civId = FindIntValue(m_pcomboCiv->GetSelection(), "CivIDs");        
        pfmLeaderBoardQuery->sort = ColumnToSort(sortCol);
        trekClient.SendClubMessages();
    };

    int OffsetOfCharacter(ZString szCharacterName)
    {
        return OffsetOfCharacter(szCharacterName, m_vEntries, m_cEntries);
    }

    int OffsetOfCharacter(const char* szCharacterName, LeaderBoardEntry* vEntries, int cEntries)
    {
        for (int i = 0; i < cEntries; i++)
        {
            if (_stricmp(vEntries[i].CharacterName, szCharacterName) == 0)
            {
                return i;
            }
        }

        return -1;
    }

    int OffsetOfCharacterPrefix(const char* szCharacterPrefix, LeaderBoardEntry* vEntries, int cEntries)
    {
        for (int i = 0; i < cEntries; i++)
        {
            int nLength = strlen(szCharacterPrefix);
            if (_strnicmp(vEntries[i].CharacterName, szCharacterPrefix, nLength) == 0)
            {
                return i;
            }
        }

        return -1;
    }

    void HandleLeaderBoardMessage(FEDMESSAGE *pLeaderBoardMessage)
    {
        
        switch (pLeaderBoardMessage->fmid) 
        {
            case FM_S_LEADER_BOARD_QUERY_FAIL:
            {
                CASTPFM(pfmLeaderBoardQueryFail, S, LEADER_BOARD_QUERY_FAIL, pLeaderBoardMessage);

                if (pfmLeaderBoardQueryFail->szBasis[0] == '\0')
                {
                    TRef<IMessageBox> pmsgBox = CreateMessageBox(
                        "No characters were found for the chosen faction.");
                    GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                }
                else
                {
                    TRef<IMessageBox> pmsgBox = CreateMessageBox(
                        "Could not find ranking info for \'" 
                         + ZString(pfmLeaderBoardQueryFail->szBasis) + "\'");
                    GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                }
                break;
            }
            break;

            case FM_S_LEADER_BOARD_LIST:
            {
                CASTPFM(pfmLeaderBoardList, S, LEADER_BOARD_LIST, pLeaderBoardMessage);

                ZAssert(pfmLeaderBoardList->cEntries > 0 
                    && pfmLeaderBoardList->cEntries <= cLinesPerScreen * 3);
                
                LeaderBoardEntry* vEntriesNew = (LeaderBoardEntry*)FM_VAR_REF(pfmLeaderBoardList, ventryPlayers);

                int indexBasis;

                // if we are getting the top players, set the basis to be 
                // the first player in the list.  
                if (m_strBasis.IsEmpty())
                {
                    m_strBasis = vEntriesNew[0].CharacterName;
                    indexBasis = 0;
                }
                else
                    indexBasis = OffsetOfCharacter(m_strBasis, vEntriesNew, pfmLeaderBoardList->cEntries);

                // if this was the first search, allow the top players in the list to have been returned
                if (m_idBasis == -2 && indexBasis == -1 && pfmLeaderBoardList->cEntries > 0)
                {
                    m_strBasis = vEntriesNew[0].CharacterName;
                    indexBasis = 0;
                }

                // if we are searching by name, allow a prefix
                if (m_idBasis == -1 && indexBasis == -1)
                {
                    indexBasis = OffsetOfCharacterPrefix(m_strBasis, vEntriesNew, pfmLeaderBoardList->cEntries);
                }

                if (indexBasis != -1)
                {
                    m_cEntries = pfmLeaderBoardList->cEntries;
                    m_entryNumber1 = pfmLeaderBoardList->entryNumber1;
                    memcpy(m_vEntries, vEntriesNew, sizeof(LeaderBoardEntry) * m_cEntries);

                    // find the basis's character id
                    m_idBasis = vEntriesNew[indexBasis].idCharacter;
                    m_strBasis = vEntriesNew[indexBasis].CharacterName;

                    UpdateListBoxes();
                }
            }
            break;
        }
    }

    void UpdateListBoxes()
    {
        TRef<TListListWrapper<LeaderBoardEntry*> > plistTopPlayer = new TListListWrapper<LeaderBoardEntry*>();
        if (m_cEntries > 0)
            plistTopPlayer->PushEnd(&m_entryNumber1);
        m_plistTopPlayer->SetList(plistTopPlayer);

        TRef<TListListWrapper<LeaderBoardEntry*> > plistStats = new TListListWrapper<LeaderBoardEntry*>();
        if (m_cEntries > 0)
        {
            // figure out the offset in the list at which to start
            int nOffsetBasis = OffsetOfCharacter(m_strBasis);
            m_nOffset = std::max(0, std::min(m_cEntries - cLinesPerScreen, 
                nOffsetBasis - (cLinesPerScreen-1)/2));

            for (int i = 0; i < cLinesPerScreen && i + m_nOffset < m_cEntries; ++i)
            {
                plistStats->PushEnd(&m_vEntries[i + m_nOffset]);
            }

            m_plistPaneStats->SetList(plistStats);

            m_plistPaneStats->SetSelection(&m_vEntries[nOffsetBasis]);
        }
        else
            m_plistPaneStats->SetList(plistStats);

        UpdateButtons();
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

LeaderBoardScreen*   LeaderBoardScreen::s_pLeaderBoard = NULL;

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateLeaderBoardScreen(Modeler* pmodeler, const ZString strCharacter)
{
    return new LeaderBoardScreen(pmodeler, strCharacter);
}




void ForwardLeaderBoardMessage(FEDMESSAGE * pLeaderBoardMessage)
{
    LeaderBoardScreen::ForwardLeaderBoardMessage(pLeaderBoardMessage);
}

