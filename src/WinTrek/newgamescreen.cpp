#include "pch.h"

//KGJV #62
#include "mappreview.h"
//////////////////////////////////////////////////////////////////////////////
//
// NewGame Screen
//
//////////////////////////////////////////////////////////////////////////////

class NewGameScreen :
    public Screen,
    public EventTargetContainer<NewGameScreen>,
    public TrekClientEventSink
{
private:
    TRef<Pane>           m_ppane;
    TRef<INameSpace>     m_pns;

    TRef<EditPane>       m_peditPaneGameName;
    TRef<EditPane>       m_peditPaneGamePassword;

    TRef<ButtonPane>     m_pbuttonBack;
    TRef<ButtonPane>     m_pbuttonCancel;
    TRef<ButtonPane>     m_pbuttonCreate;

    TRef<ButtonPane>     m_pbuttonEjectPods;
    TRef<ButtonPane>     m_pbuttonFriendlyFire;
    // TRef<ButtonPane>     m_pbuttonStatsCount; KGJV #62 removed statscount
    TRef<ButtonPane>     m_pbuttonDefections;
    TRef<ButtonPane>     m_pbuttonJoiners;
    TRef<ButtonPane>     m_pbuttonSquadGame;
    TRef<ButtonPane>     m_pbuttonInvulnerableStations;
    TRef<ButtonPane>     m_pbuttonDevelopment;
    TRef<ButtonPane>     m_pbuttonAllowShipyards;

	// KGJV #62
    TRef<ButtonPane>     m_pbuttonAllowTactical;
    TRef<ButtonPane>     m_pbuttonAllowExpansion;
    TRef<ButtonPane>     m_pbuttonAllowSupremacy;
    //TRef<ButtonPane>     m_pbuttonAllowEmptyTeams; Removed AET setting  can't change anyways (use the space for ALLY ripcord option) Imago
	TRef<ButtonPane>     m_pbuttonAllowAlliedRip;
    
	TRef<ComboPane>      m_pcomboMapSize;
    TRef<ComboPane>      m_pcomboTreasures;
	TRef<ComboPane>      m_pcomboAsteriods;
    TRef<ComboPane>      m_pcomboInitialMiners;
    TRef<ComboPane>      m_pcomboMaximumDrones;

	int					 m_icomboMapTypeCount;
	int					 m_icomboCustomMapsCount;
	TRef<ButtonPane>     m_pbuttonNextMap;
	TRef<ButtonPane>     m_pbuttonPrevMap;
	TRef<MapPreviewPane> m_pimageMapPreview;

	TRef<ButtonPane>     m_pbuttonShowMap;

    TRef<ComboPane>      m_pcomboGameType; 
    TRef<ComboPane>      m_pcomboTeamCount; 
    //TRef<StringPane>     m_ptextMinPlayers;
    TRef<StringPane>     m_ptextMaxPlayers;
    TRef<ComboPane>      m_pcomboMaxPlayers;
    TRef<ComboPane>      m_pcomboMaxImbalance;
    TRef<ComboPane>      m_pcomboSkillLevel;
    TRef<ComboPane>      m_pcomboMapType;
    TRef<ComboPane>      m_pcomboConnectivity;
    TRef<ComboPane>      m_pcomboLives;
    TRef<ComboPane>      m_pcomboResources;
    TRef<ComboPane>      m_pcomboTotalMoney;
    TRef<ComboPane>      m_pcomboStartingMoney;
    TRef<ComboPane>      m_pcomboCountdownTime;
    TRef<ComboPane>      m_pcomboDeathWatchKills;
    TRef<ComboPane>      m_pcomboProsperity;
    TRef<ComboPane>      m_pcomboArtifactCount;
    TRef<ComboPane>      m_pcomboFlagCount;
    TRef<ComboPane>      m_pcomboConquestBases;
    TRef<ComboPane>      m_pcomboTerritory;
    TRef<StringPane>     m_ptextCountdown;

    TRef<IKeyboardInput> m_pkeyboardInputOldFocus;
    TRef<Modeler>        m_pmodeler;

    TRef<ModifiableNumber> m_pnumberCanChooseMaxPlayers;
    TRef<ModifiableNumber> m_pnumberIsCustomMap;

    bool                 m_bIsZoneClub;
    bool                 m_bLockGameOpen;
    bool                 m_bIgnoreGameTypeComboChanges;
    bool                 m_fQuitting;

    int GetResourceLevel(const MissionParams& missionparams)
    {
        int ResourceCount = missionparams.nPlayerSectorMineableAsteroids
            + missionparams.nNeutralSectorMineableAsteroids
            + missionparams.nPlayerSectorSpecialAsteroids
            + missionparams.nNeutralSectorAsteroids;

        if (ResourceCount > 9)
            return 4;
        else if (ResourceCount > 6)
            return 3;
        else if (ResourceCount > 5)
            return 2;
        else if (ResourceCount > 2)
            return 1;
        else 
            return 0;
    }

    const MissionParams& GetBaseMissionParams()
    {
        return trekClient.MyMission()->GetMissionParams();
    }

    void RefreshMissionParams()
    {
        ApplyMissionParams(GetBaseMissionParams());
    }

    void ApplyMissionParams(const MissionParams& missionparams)
    {
        m_peditPaneGameName->SetString(missionparams.strGameName);
        m_peditPaneGamePassword->SetString(missionparams.strGamePassword);

        m_pbuttonEjectPods           ->SetChecked(missionparams.bEjectPods);
        m_pbuttonFriendlyFire        ->SetChecked(missionparams.bAllowFriendlyFire);
        //m_pbuttonStatsCount          ->SetChecked(missionparams.bScoresCount); KGJV #62
        m_pbuttonDefections          ->SetChecked(missionparams.bAllowDefections);
        m_pbuttonJoiners             ->SetChecked(missionparams.bAllowJoiners);
        m_pbuttonSquadGame           ->SetChecked(missionparams.bSquadGame);
        m_pbuttonInvulnerableStations->SetChecked(missionparams.bInvulnerableStations);
        m_pbuttonDevelopment         ->SetChecked(missionparams.bAllowDevelopments);
        m_pbuttonAllowShipyards      ->SetChecked(missionparams.bAllowShipyardPath);
		// KGJV #62
        m_pbuttonAllowTactical       ->SetChecked(missionparams.bAllowTacticalPath);
        m_pbuttonAllowExpansion      ->SetChecked(missionparams.bAllowExpansionPath);
        m_pbuttonAllowSupremacy      ->SetChecked(missionparams.bAllowSupremacyPath);
    //    m_pbuttonAllowEmptyTeams     ->SetChecked(missionparams.bAllowEmptyTeams);  //Imago ALLY 7/8/09
		m_pbuttonAllowAlliedRip      ->SetChecked(missionparams.bAllowAlliedRip);

        m_pbuttonShowMap             ->SetChecked(missionparams.bShowMap);

        m_pcomboTeamCount->SetSelection(FindClosestValue(missionparams.nTeams, "TeamCountValues"));
        m_pcomboMaxPlayers->SetSelection(FindClosestValue(missionparams.nMaxPlayersPerTeam, "MaxPlayersValues"));
        m_ptextMaxPlayers->SetString(ZString(missionparams.nMinPlayersPerTeam) 
            + " - " + ZString(missionparams.nMaxPlayersPerTeam));
        //m_ptextMinPlayers->SetString(ZString(missionparams.nMinPlayersPerTeam));

        m_pcomboMaxImbalance   ->SetSelection(FindClosestValue(missionparams.iMaxImbalance, "MaxImbalanceValues"));

		m_bIsZoneClub = missionparams.bClubGame; //!missionparams.bObjectModelCreated;// KGJV #114   missionparams.bClubGame;
        m_bLockGameOpen = missionparams.bLockGameOpen;
        
        if (m_bIsZoneClub) //pkk #203 - Ignore clients SkillLevel, if it's a ZG
		{
			const char* vszSkillLevelNames[2] = {"GameSkillLevelMin", "GameSkillLevelMax"};
			float vfSkillLevel[2] = { (float)missionparams.iMinRank, (float)missionparams.iMaxRank };
			m_pcomboSkillLevel     ->SetSelection(FindClosestValue(vfSkillLevel, vszSkillLevelNames, 2));
		}

        m_pcomboConnectivity   ->SetSelection(FindClosestValue(missionparams.iRandomEncounters, "ConnectivityValues"));
        m_pcomboLives          ->SetSelection(FindClosestValue(missionparams.iLives, "LivesValues"));

        if (missionparams.szCustomMapFile[0] == '\0')
        {
            m_pcomboMapType->SetSelection(FindClosestValue(missionparams.mmMapType, "MapTypeValues"));
            m_pnumberIsCustomMap->SetValue(0.0f);
        }
        else
        {
            int mapTypeIndex = FindMapType(missionparams.szCustomMapFile);
            if (mapTypeIndex == NA)
            {
                m_pnumberIsCustomMap->SetValue(1.0f);
            }
            else
            {
                m_pnumberIsCustomMap->SetValue(0.0f);
                m_pcomboMapType->SetSelection(CustomMapIDToItemID(mapTypeIndex));
            }
        }

        const char* vszResourceNames[4] = {"ResourcesNeutralMinable", 
            "ResourcesPlayerMinable", "ResourcesNeutralSpecial", "ResourcesPlayerSpecial"};
        float vfResourceLevels[4] = { 
            (float)missionparams.nNeutralSectorMineableAsteroids, 
            (float)missionparams.nPlayerSectorMineableAsteroids, 
            (float)missionparams.nNeutralSectorSpecialAsteroids, 
            (float)missionparams.nPlayerSectorSpecialAsteroids, 
        };
        m_pcomboResources      ->SetSelection(FindClosestValue(vfResourceLevels, vszResourceNames, 4));

        m_pcomboTotalMoney     ->SetSelection(FindClosestValue(missionparams.fHe3Density, "TotalMoneyValues"));
        m_pcomboStartingMoney  ->SetSelection(FindClosestValue(missionparams.fStartingMoney, "StartingMoneyValues"));
        m_pcomboCountdownTime  ->SetSelection(FindClosestValue(missionparams.dtGameLength / 60.0f, "CountdownTimeValues"));
        m_pcomboDeathWatchKills->SetSelection(FindClosestValue(missionparams.nGoalTeamKills, "DeathWatchKillsValues"));
        m_pcomboProsperity     ->SetSelection(FindClosestValue(missionparams.fGoalTeamMoney, "ProsperityTargetValues"));
        m_pcomboArtifactCount  ->SetSelection(FindClosestValue(missionparams.nGoalArtifactsCount, "ArtifactCountValues"));
        m_pcomboFlagCount      ->SetSelection(FindClosestValue(missionparams.nGoalFlagsCount, "FlagCountValues"));
        m_pcomboConquestBases  ->SetSelection(FindClosestValue(missionparams.iGoalConquestPercentage, "ConquestPercentValues"));
        m_pcomboTerritory      ->SetSelection(FindClosestValue(missionparams.iGoalTerritoryPercentage, "TerritoryValues"));

		// KGJV #62
		m_pcomboMapSize		   ->SetSelection(FindClosestValue(missionparams.iMapSize, "MapSizeValues"));

		const char* vszAsteriodsNames[2] = {"AsteriodsHomeValues","AsteriodsNeutralValues"};
        float vfAsteriodsValues[2] = { 
            (float)missionparams.nPlayerSectorAsteroids, 
            (float)missionparams.nNeutralSectorAsteroids,
        };
		m_pcomboAsteriods      ->SetSelection(FindClosestValue(vfAsteriodsValues,vszAsteriodsNames,2));

        const char* vszTreasureNames[4] = {
			"TreasureHomeValues","TreasureNeutralValues",
			"TreasureRateHomeValues","TreasureRateNeutralValues"
		};
        float vfTreasureValues[4] = { 
            (float)missionparams.nPlayerSectorTreasures, 
            (float)missionparams.nNeutralSectorTreasures,
			(float)missionparams.nPlayerSectorTreasureRate * 60.0f, 
            (float)missionparams.nNeutralSectorTreasureRate * 60.0f, 
        };
		m_pcomboTreasures      ->SetSelection(FindClosestValue(vfTreasureValues,vszTreasureNames,4));
		m_pcomboInitialMiners  ->SetSelection(FindClosestValue(missionparams.nInitialMinersPerTeam, "initialMinersValues"));
		m_pcomboMaximumDrones  ->SetSelection(FindClosestValue(missionparams.nMaxDronesPerTeam, "maxDronesValues"));
    }
public:
    ZString YesNo(bool b)
    {
        return b ? "yes" : "no";
    }

    ZString GetNthString(const char* szContentName, int index)
    {
        IObjectList* plist; 
    
        CastTo(plist, m_pns->FindMember(szContentName));

        plist->GetFirst();

        while (index > 0) {
            plist->GetNext();
            index--;
        }

        return GetString(plist->GetCurrent());
    }

    ZString GetLives(const MissionParams& missionparams)
    {
        if (missionparams.iLives == 32767) {
            return "Unlimited";    
        } else {
            return missionparams.iLives;
        }
    }

    ZString GetMapType(const MissionParams& missionparams)
    {
        if (missionparams.szCustomMapFile[0] == '\0') {
            return GetNthString("MapTypeNames", missionparams.mmMapType);
        } else {
            return "Custom Map " + ZString(missionparams.szCustomMapFile);
        }
    }

	// KGJV: added - return (closest) connectivity name
    ZString GetConnectivityString(const MissionParams& missionparams)
    {
		return GetNthString("ConnectivityNames", FindClosestValue(missionparams.iRandomEncounters, "ConnectivityValues"));
	}

	// KGJV: added - return (closest) total money setting
	ZString GetTotalMoneyString(const MissionParams& missionparams)
    {
		return GetNthString("TotalMoneyNames", FindClosestValue(missionparams.fHe3Density, "TotalMoneyValues"));
	}

	// mmf: added - return (closest) max team imbalance setting
	ZString GetMaxImbalanceString(const MissionParams& missionparams)
    {
		return GetNthString("MaxImbalanceNames", FindClosestValue(missionparams.iMaxImbalance, "MaxImbalanceValues"));
	}

    ZString GetResourceString(const MissionParams& missionparams)
    {
        const char* vszResourceNames[4] = {
            "ResourcesNeutralMinable", 
            "ResourcesPlayerMinable", 
            "ResourcesNeutralSpecial", 
            "ResourcesPlayerSpecial"
        };

        float vfResourceLevels[4] = { 
            (float)missionparams.nNeutralSectorMineableAsteroids, 
            (float)missionparams.nPlayerSectorMineableAsteroids, 
            (float)missionparams.nNeutralSectorSpecialAsteroids, 
            (float)missionparams.nPlayerSectorSpecialAsteroids, 
        };

        return GetNthString("ResourcesNames", FindClosestValue(vfResourceLevels, vszResourceNames, 4));
    }

    NewGameScreen(Modeler* pmodeler, ZString& str)
    {
        //
        // Open the namespace
        //

        m_pmodeler = pmodeler;
        OpenMDLFile();

        //
        // Initialize just enough to allow the mission parameters to be written out as a string
        //

        const MissionParams& mp = trekClient.MyMission()->GetMissionParams();

		// KGJV : reformated + official stuff added
		// 
		const char* szServerName = trekClient.MyMission()->GetMissionDef().szServerName;
		const char* szCoreName = mp.szIGCStaticFile;


        str =
			ZString("<Color|yellow><Font|medBoldVerdana>Game details<Font|smallFont><Color|white><p>")
            + "Name: "                  + ZString(mp.strGameName)         + "<p>"
            + "<p>Server: " +
			+ (trekClient.CfgIsOfficialServer(szServerName,trekClient.MyMission()->GetMissionDef().szServerAddr) ?
				ZString("<Color|green>") :
				ZString("<Color|white>")
			  )
			+ ZString(szServerName)           + "<Color|white><p>Core: "
			+ (trekClient.CfgIsOfficialCore(szCoreName) ?
				ZString("<Color|green>") :
				ZString("<Color|white>")
			  )
			+ trekClient.CfgGetCoreName(szCoreName)     + "<Color|white><p>" // KGJV: added
			+ "<p>"
			+ "Lives: "                 + GetLives(mp)                    + "<p>"
            + "Map Type: "              + GetMapType(mp)                  + "<p>"
			+ "Map Connectivity: "      + GetConnectivityString(mp)       + "<p>" // KGJV: added
            + "Resources: "             + GetResourceString(mp)           + "<p>"
			+ "Total money: "           + GetTotalMoneyString(mp)         + "<p>" // KGJV: added
            + "Scores Count: "          + YesNo(mp.bScoresCount)          + "<p>"
            + "Eject Pods: "            + YesNo(mp.bEjectPods)            + "<p>"
            + "Allow Friendly Fire: "   + YesNo(mp.bAllowFriendlyFire)    + "<p>"
			+ "Max Team Imbalance: "    + GetMaxImbalanceString(mp)       + "<p>" // mmf: added  note this does not update properly if updated after launch with #autobalance
            + "Allow Defections: "      + YesNo(mp.bAllowDefections)      + "<p>"
			+ "Allow Joiners: "         + YesNo(mp.bAllowJoiners)         + "<p>"
            + "Invulnerable Stations: " + YesNo(mp.bInvulnerableStations) + "<p>"
            + "Developments: "          + YesNo(mp.bAllowDevelopments)    + "<p>"
            + "Allow Shipyards: "       + YesNo(mp.bAllowShipyardPath)    + "<p>"
			// KGJV #62
            + "Allow Tactical: "        + YesNo(mp.bAllowTacticalPath)    + "<p>"
            + "Allow Expansion: "       + YesNo(mp.bAllowExpansionPath)   + "<p>"
            + "Allow Supremacy: "       + YesNo(mp.bAllowSupremacyPath)   + "<p>"
			+ "Allow Empty Teams: "     + YesNo(mp.bAllowEmptyTeams)      + "<p>"
			+ "Allow Allied Ripcord: "  + YesNo(mp.bAllowAlliedRip)       + "<p>" //imago 7/10/09 ALLY
			+ "Experimental: "          + YesNo(mp.bExperimental)         + "<p>" //imago 7/10/09
            + "<p>"
            ;
    }

    void OpenMDLFile()
    {
        TRef<INameSpace> pnsTeamScreenData = GetModeler()->CreateNameSpace("newgamescreendata");
        pnsTeamScreenData->AddMember("CanChooseMaxPlayers", m_pnumberCanChooseMaxPlayers = new ModifiableNumber(0));
        pnsTeamScreenData->AddMember("IsCustomMap", m_pnumberIsCustomMap = new ModifiableNumber(0));

        m_pns = m_pmodeler->GetNameSpace("newgamescreen");
    }

    NewGameScreen(Modeler* pmodeler)
    {
        //
        // Read the mdl file
        //

        m_pmodeler = pmodeler;
        OpenMDLFile();

        m_fQuitting = false;
        m_bIgnoreGameTypeComboChanges = false;
        CastTo(m_ppane, m_pns->FindMember("screen"));

        //
        // Buttons
        //


        CastTo(m_pbuttonCancel,         m_pns->FindMember("cancelButtonPane"));
        CastTo(m_pbuttonBack,         m_pns->FindMember("backButtonPane"));
		// mdvalley: Needs the class name and pointer now
        AddEventTarget(&NewGameScreen::OnButtonBack,   m_pbuttonBack->GetEventSource());
        CastTo(m_pbuttonCreate,       m_pns->FindMember("createButtonPane"));

        AddEventTarget(&NewGameScreen::OnButtonBack,   m_pbuttonCancel->GetEventSource());
        
        AddEventTarget(&NewGameScreen::OnButtonCreate, m_pbuttonCreate->GetEventSource());

        //
        // Edit controls
        //


        CastTo(m_peditPaneGameName, (Pane*)m_pns->FindMember("gameNameEditPane"));
        m_peditPaneGameName->SetMaxLength(c_cbGameName - 37);
        CastTo(m_peditPaneGamePassword, (Pane*)m_pns->FindMember("gamePasswordEditPane"));
        m_peditPaneGamePassword->SetMaxLength(c_cbGamePassword - 1);

        AddEventTarget(&NewGameScreen::OnGameNameClick, m_peditPaneGameName->GetClickEvent());
        AddEventTarget(&NewGameScreen::OnGamePasswordClick, m_peditPaneGamePassword->GetClickEvent());

        //
        // Checkboxes
        //

        CastTo(m_pbuttonEjectPods           , m_pns->FindMember("ejectPodsCheckboxPane"));
        CastTo(m_pbuttonFriendlyFire        , m_pns->FindMember("friendlyFireCheckboxPane"));
        //CastTo(m_pbuttonStatsCount          , m_pns->FindMember("statsCountCheckboxPane")); KGJV #62
        CastTo(m_pbuttonDefections          , m_pns->FindMember("defectionsCheckboxPane"));
        CastTo(m_pbuttonJoiners             , m_pns->FindMember("joinersCheckboxPane"));
        CastTo(m_pbuttonSquadGame           , m_pns->FindMember("squadGameCheckboxPane"));
        CastTo(m_pbuttonInvulnerableStations, m_pns->FindMember("invulnerableStationsCheckboxPane"));
        CastTo(m_pbuttonDevelopment         , m_pns->FindMember("developmentCheckboxPane"));
        CastTo(m_pbuttonAllowShipyards      , m_pns->FindMember("allowShipyardsCheckboxPane"));
		// KGJV #62
        CastTo(m_pbuttonAllowTactical       , m_pns->FindMember("allowTacticalCheckboxPane"));
        CastTo(m_pbuttonAllowExpansion      , m_pns->FindMember("allowExpansionCheckboxPane"));
        CastTo(m_pbuttonAllowSupremacy      , m_pns->FindMember("allowSupremacyCheckboxPane"));
        //CastTo(m_pbuttonAllowEmptyTeams     , m_pns->FindMember("allowEmptyTeamsCheckboxPane"));
		CastTo(m_pbuttonAllowAlliedRip     , m_pns->FindMember("allowAlliedRipCheckboxPane")); //imago 7/8/09 ALLY
		CastTo(m_pbuttonNextMap             , m_pns->FindMember("mapPreviewNextButtonPane"));
		CastTo(m_pbuttonPrevMap				, m_pns->FindMember("mapPreviewPrevButtonPane"));

		CastTo(m_pbuttonShowMap             , m_pns->FindMember("showMapCheckboxPane"));


        //
        // Combo boxes
        //

        CastTo(m_pcomboGameType           , m_pns->FindMember("gameTypeComboPane"));
        CastTo(m_pcomboTeamCount          , m_pns->FindMember("teamCountComboPane"));
        CastTo(m_pcomboMaxPlayers         , m_pns->FindMember("maxPlayersComboPane"));
        CastTo(m_ptextMaxPlayers          , m_pns->FindMember("maxPlayersStringPane"));
        //CastTo(m_ptextMinPlayers          , m_pns->FindMember("minPlayersStringPane"));
        CastTo(m_pcomboMaxImbalance       , m_pns->FindMember("maxImbalanceComboPane"));
        CastTo(m_pcomboSkillLevel         , m_pns->FindMember("skillLevelComboPane"));
        CastTo(m_pcomboMapType            , m_pns->FindMember("mapTypeComboPane"));
        CastTo(m_pcomboConnectivity       , m_pns->FindMember("connectivityComboPane"));
        CastTo(m_pcomboLives              , m_pns->FindMember("livesComboPane"));
        CastTo(m_pcomboResources          , m_pns->FindMember("resourcesComboPane"));
        CastTo(m_pcomboTotalMoney         , m_pns->FindMember("totalMoneyComboPane"));
        CastTo(m_pcomboStartingMoney      , m_pns->FindMember("startingMoneyComboPane"));
        CastTo(m_pcomboCountdownTime      , m_pns->FindMember("countdownTimeComboPane"));
        CastTo(m_pcomboDeathWatchKills    , m_pns->FindMember("deathWatchKillsComboPane"));
        CastTo(m_pcomboProsperity         , m_pns->FindMember("prosperityComboPane"));
        CastTo(m_pcomboArtifactCount      , m_pns->FindMember("artifactCountComboPane"));
        CastTo(m_pcomboFlagCount          , m_pns->FindMember("flagCountComboPane"));
        CastTo(m_pcomboConquestBases      , m_pns->FindMember("conquestBasesComboPane"));
        CastTo(m_pcomboTerritory          , m_pns->FindMember("territoryComboPane"));
		// KGJV #62
        CastTo(m_pcomboMapSize            , m_pns->FindMember("mapSizeComboPane"));
		CastTo(m_pcomboAsteriods          , m_pns->FindMember("asteriodsComboPane"));
        CastTo(m_pcomboTreasures          , m_pns->FindMember("treasuresComboPane"));
        CastTo(m_pcomboInitialMiners      , m_pns->FindMember("initialMinersCountComboPane"));
        CastTo(m_pcomboMaximumDrones      , m_pns->FindMember("maxDronesCountComboPane"));

		CastTo(m_pimageMapPreview		  , (Pane*)m_pns->FindMember("mapPreviewPane"));

        TList<TRef<GameType> >::Iterator gameTypesIter(GameType::GetGameTypes());
        int index = 0;
        while (!gameTypesIter.End()) 
        {
            m_pcomboGameType->AddItem(gameTypesIter.Value()->GetName(), index);
            gameTypesIter.Next();
            index++;
        }
        m_pcomboGameType->AddItem("Custom", index);

        MissionParams misparams = GetBaseMissionParams();
        FillCombo(m_pcomboMaxPlayers         , "MaxPlayersNames");

        FillCombo(m_pcomboTeamCount          , "TeamCountNames");
        FillCombo(m_pcomboMaxImbalance       , "MaxImbalanceNames");
        FillCombo(m_pcomboSkillLevel         , "GameSkillLevelNames");        
        FillCombo(m_pcomboConnectivity       , "ConnectivityNames");
        FillCombo(m_pcomboLives              , "LivesNames");
        FillCombo(m_pcomboResources          , "ResourcesNames");
        FillCombo(m_pcomboTotalMoney         , "TotalMoneyNames");
        FillCombo(m_pcomboStartingMoney      , "StartingMoneyNames");
        FillCombo(m_pcomboCountdownTime      , "CountdownTimeNames");
        FillCombo(m_pcomboDeathWatchKills    , "DeathWatchKillsNames");
        FillCombo(m_pcomboProsperity         , "ProsperityTargetNames");
        FillCombo(m_pcomboArtifactCount      , "ArtifactCountNames");
        FillCombo(m_pcomboFlagCount          , "FlagCountNames");
        FillCombo(m_pcomboConquestBases      , "ConquestPercentNames");
        FillCombo(m_pcomboTerritory          , "TerritoryNames");

		// KGJV #62: changed to store # maptype entries
        m_icomboMapTypeCount = FillCombo(m_pcomboMapType            , "MapTypeNames");
        AddCustomMapTypes();

		// KGJV #62
        FillCombo(m_pcomboMapSize            , "MapSizeNames");
		FillCombo(m_pcomboAsteriods			 , "AsteriodsNames");
        FillCombo(m_pcomboTreasures          , "TreasuresNames");
        FillCombo(m_pcomboInitialMiners      , "initialMinersNames");
        FillCombo(m_pcomboMaximumDrones      , "maxDronesNames");

        AddEventTarget(&NewGameScreen::OnPickGameType, m_pcomboGameType->GetEventSource());

        //
        // Countdown box
        //
        CastTo(m_ptextCountdown, (Pane*)m_pns->FindMember("textCountdown"));

        //
        // Add event hooks
        //

        AddEventTarget(&NewGameScreen::OnMaxPlayersChange, m_pcomboMaxPlayers->GetEventSource());
        AddEventTarget(&NewGameScreen::OnTeamCountChange, m_pcomboTeamCount->GetEventSource());

        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboLives->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboCountdownTime->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboDeathWatchKills->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboProsperity->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboArtifactCount->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboFlagCount->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboConquestBases->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedComboChange, m_pcomboTerritory->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedCheckboxChange, m_pbuttonInvulnerableStations->GetEventSource());
        AddEventTarget(&NewGameScreen::OnGameTypeRelatedCheckboxChange, m_pbuttonDevelopment->GetEventSource());
        //AddEventTarget(&NewGameScreen::OnGameTypeRelatedCheckboxChange, m_pbuttonStatsCount->GetEventSource()); KGJV #62

		// KGJV #62
		AddEventTarget(&NewGameScreen::OnButtonMapNext, m_pbuttonNextMap->GetEventSource());
		AddEventTarget(&NewGameScreen::OnButtonMapPrev, m_pbuttonPrevMap->GetEventSource());
		// new event hooks for controls that can trigger a map preview change
		AddEventTarget(&NewGameScreen::OnRefreshMapPreview, m_pcomboMapType->GetEventSource());
		AddEventTarget(&NewGameScreen::OnRefreshMapPreview, m_pcomboMapSize->GetEventSource());

        //
        // set the keyboard focus
        //

        m_pkeyboardInputOldFocus = GetWindow()->GetFocus();

        if (CanEdit())
            GetWindow()->SetFocus(m_peditPaneGameName);

        if (g_bQuickstart) {
            OnButtonCreate();
        }

        //
        // Unload the namespace so that everything will be reset the next time we
        // come to this screen.
        //

        RefreshMissionParams();
        UpdateButtonStates();
        UpdateGameType();
    }

    ~NewGameScreen()
    {
        m_pmodeler->UnloadNameSpace("newgamescreen");
        if (m_pkeyboardInputOldFocus) {
            GetWindow()->SetFocus(m_pkeyboardInputOldFocus);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Methods
    //
    //////////////////////////////////////////////////////////////////////////////

	// KGJV #62 ; changed to return the number of items filled
    int FillCombo(ComboPane* pcombo, const char* szContentName)
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
		return index;
    }

    void FillIntComboWithLimit(ComboPane* pcombo, float fLimit, const char* szContentName, const char* szTableName)
    {
        IObjectList* plist; 
        IObjectList* plistValue; 
        
        CastTo(plist, m_pns->FindMember(szContentName));
        CastTo(plistValue, m_pns->FindMember(szTableName));

        plist->GetFirst();
        plistValue->GetFirst();

        bool bEliminatedItem = false;

        int index = 0;
        while (plist->GetCurrent() != NULL) 
        {
            float fValue = GetNumber(plistValue->GetCurrent());

            if (fValue < fLimit)
                pcombo->AddItem(GetString(plist->GetCurrent()), (int)fValue);
            else 
                bEliminatedItem = true;

            plist->GetNext();
            plistValue->GetNext();
            ++index;
        }

        if (bEliminatedItem)
            pcombo->AddItem(ZString((int)fLimit), (int)fLimit);
    }

    float FindValue(int index, const char* szTableName)
    {
        IObjectList* plist; 
        
        CastTo(plist, m_pns->FindMember(szTableName));

        plist->GetFirst();

        while (index > 0) 
        {
            plist->GetNext();
            --index;
        }

        return GetNumber(plist->GetCurrent());
    }

    int FindClosestValue(float fValue, const char* szTableName)
    {
        IObjectList* plist; 
        
        CastTo(plist, m_pns->FindMember(szTableName));

        plist->GetFirst();

        int indexClosest = 0;
        float fDistanceClosest = 1e10f;

        int index = 0;
        while (plist->GetCurrent() != NULL) 
        {
            float fDistance = fabs(fValue - GetNumber(plist->GetCurrent()));

            if (fDistance < fDistanceClosest)
            {
                fDistanceClosest = fDistance;
                indexClosest = index;
            }

            plist->GetNext();
            ++index;
        }

        return indexClosest;
    }

    int FindClosestValue(int nValue, const char* szTableName)
    {
        return FindClosestValue((float)nValue, szTableName);
    }

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
    }

    void AddCustomMapTypes()
    {
		m_icomboCustomMapsCount = 0;
        for (int i = 0; i < trekClient.GetNumStaticMaps(); i++)
        {
            m_pcomboMapType->AddItem(trekClient.GetStaticMapInfo(i).cbFriendlyName, CustomMapIDToItemID(i));
			m_icomboCustomMapsCount++;
        }
    }

    int FindMapType(const char* szMapFile)
    {
        for (int i = 0; i < trekClient.GetNumStaticMaps(); i++)
        {
            if (strcmp(szMapFile, trekClient.GetStaticMapInfo(i).cbIGCFile) == 0)
                return i;
        }
        return NA;
    }

    int CustomMapIDToItemID(int mapID)
    {
        assert(mapID >= 0 && mapID < trekClient.GetNumStaticMaps());
        return -1 - mapID;
    }

    int ItemIDToCustomMapID(int itemID)
    {
        int mapID = -(itemID + 1);
        assert(mapID >= 0 && mapID < trekClient.GetNumStaticMaps());
        return mapID;
    }

    int ItemIDToMapType(int itemID)
    {
        if (itemID >= 0)
            return FindValue(itemID, "MapTypeValues");
        else
            return 0;
    }

    const char* ItemIDToMapFile(int itemID)
    {
        if (itemID >= 0)
            return "";
        else
            return trekClient.GetStaticMapInfo(ItemIDToCustomMapID(itemID)).cbIGCFile;
    }

    bool ValidateNumTeams(int cRequested, int itemID)
    {
        if (itemID >= 0)
            return true;
        else
            return trekClient.GetStaticMapInfo(ItemIDToCustomMapID(itemID)).nNumTeams == cRequested;
    }

    bool ValidateNumTeams(int cRequested, const char* szIGCFile)
    {
        int mapTypeIndex = FindMapType(szIGCFile);

        if (mapTypeIndex == NA)
            return true;
        else
            return ValidateNumTeams(cRequested, CustomMapIDToItemID(mapTypeIndex));
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Events
    //
    //////////////////////////////////////////////////////////////////////////////
	
	// KGJV #62 handler for settings change that affect mappreview
    bool OnRefreshMapPreview(int max)
	{
		debugf("mapid = %d\n",m_pcomboMapType->GetSelection());
		MissionParams mp;
        ReadControls(mp);
		m_pimageMapPreview->ComputePreview(mp);
		return true;
	}
	// cycling thru map types
	// point is to change m_pcomboMapType->GetSelection(i)
	//   i values: 0 <----> (M-1)                   if no cust maps
	//   i values: 0 <----> (M-1) -1 <-----> -(C-1) if cust maps
	// M = m_icomboMapTypeCount - # of std maps
	// C = m_icomboCustomMapsCount - # of custom maps (O if none)

	bool OnButtonMapNext()
	{
		int i = m_pcomboMapType->GetSelection();
		//debugf("next: %d ",i);

		if (i>=0 && i<m_icomboMapTypeCount-1) // we're between 0 <----> (M-1)
			i++;
		else 
			if (m_icomboCustomMapsCount>0) // there is cust maps
				if (i == m_icomboMapTypeCount-1) // are we at (M-1)
					i = CustomMapIDToItemID(0);  // so next is 1st cust map (-1)
				else
					if (i > CustomMapIDToItemID(m_icomboCustomMapsCount-1)) // are we a last cust map
						i--; // no so move to next custmap
					else i = 0; // yes to move to 1st std map
			else i = 0; // no custmap so next is 1st std map
				
		//debugf("to %d\n",i);
		m_pcomboMapType->SetSelection(i);
		return true;
	}
	bool OnButtonMapPrev()
	{
		int i = m_pcomboMapType->GetSelection();
		//debugf("prev: %d ",i);

		if (i==0) // 1st map so prev is last custom if any or last standard
			if (m_icomboCustomMapsCount>0)
				i = CustomMapIDToItemID(m_icomboCustomMapsCount-1);
			else
				i = m_icomboMapTypeCount-1;
		else
			if (i==-1) // 1st custom so prev is last standard
				i = m_icomboMapTypeCount-1;
			else // else dec if positive and inc if negative
				i += (i>0?-1:1);

		//debugf("to %d\n",i,m_pcomboMapType->GetSelection());
		m_pcomboMapType->SetSelection(i);
		return true;
	}

    bool OnMaxPlayersChange(int max)
    {
        //m_ptextMinPlayers->SetString(
        //    ZString(FindValue(m_pcomboMaxPlayers->GetSelection(), "MinPlayersValues"))
        //    );
        return true;
    }

    bool OnTeamCountChange(int max)
    {
        if (!m_bIsZoneClub)
        {
            m_ptextMaxPlayers->SetString(
                ZString(GetBaseMissionParams().nMinPlayersPerTeam) + " - " 
                    + ZString(
						std::min(
							100,
							GetBaseMissionParams().nTotalMaxPlayersPerGame
							/ (int)FindValue(m_pcomboTeamCount->GetSelection(), "TeamCountValues")))
                );
        }
		// KGJV #62
		MissionParams mp;
        ReadControls(mp);
		m_pimageMapPreview->ComputePreview(mp);
        return true;
    }

    bool OnGameNameClick()
    {
        GetWindow()->SetFocus(m_peditPaneGameName);
        return true;
    }

    bool OnGamePasswordClick()
    {
        GetWindow()->SetFocus(m_peditPaneGamePassword);
        return true;
    }

    bool OnButtonBack()
    {
        GetWindow()->screen(ScreenIDTeamScreen);
        return true;
    }

    bool OnPickGameType(int nType)
    {
        if (!m_bIgnoreGameTypeComboChanges)
        {
            TList<TRef<GameType> >::Iterator gameTypesIter(GameType::GetGameTypes());
            int index = 0;
            while (index < nType) 
            {
                assert(!gameTypesIter.End());
                index++;
                gameTypesIter.Next();
            }
        
            if (!gameTypesIter.End())
            {
                MissionParams misparams;
                ReadControls(misparams);
                gameTypesIter.Value()->Apply(misparams);
                ApplyMissionParams(misparams);
            }
        }

        return true;
    }

    bool OnGameTypeRelatedCheckboxChange()
    {
        UpdateGameType();
        return true;
    }

    bool OnGameTypeRelatedComboChange(int nChange)
    {
        UpdateGameType();
        return true;
    }

    void UpdateGameType()
    {
        MissionParams misparams;
        ReadControls(misparams);

        TList<TRef<GameType> >::Iterator gameTypesIter(GameType::GetGameTypes());

        int nComboBoxIndex = 0;
        while (!gameTypesIter.End() && !gameTypesIter.Value()->IsGameType(misparams))
        {
            nComboBoxIndex++;
            gameTypesIter.Next();
        }

        if (m_pcomboGameType->GetSelection() != nComboBoxIndex)
        {
            m_bIgnoreGameTypeComboChanges = true;
            m_pcomboGameType->SetSelection(nComboBoxIndex);
            m_bIgnoreGameTypeComboChanges = false;
        }
    }

    void ReadControls(MissionParams& misparams)
    {
        misparams = GetBaseMissionParams();

        strcpy(misparams.strGameName, m_peditPaneGameName->GetString());
        strcpy(misparams.strGamePassword, m_peditPaneGamePassword->GetString());

        misparams.bEjectPods = m_pbuttonEjectPods->GetChecked();
        misparams.bAllowFriendlyFire = m_pbuttonFriendlyFire->GetChecked();
        //misparams.bScoresCount = m_pbuttonStatsCount->GetChecked(); KGJV #62
        misparams.bAllowDefections = m_pbuttonDefections->GetChecked();
        misparams.bAllowJoiners = m_pbuttonJoiners->GetChecked();
        misparams.bSquadGame = m_pbuttonSquadGame->GetChecked();
        misparams.bInvulnerableStations = m_pbuttonInvulnerableStations->GetChecked();
        misparams.bAllowDevelopments = m_pbuttonDevelopment->GetChecked();
        misparams.bAllowShipyardPath = m_pbuttonAllowShipyards->GetChecked();
		// KGJV #62
		misparams.bAllowTacticalPath = m_pbuttonAllowTactical->GetChecked();
		misparams.bAllowExpansionPath = m_pbuttonAllowExpansion->GetChecked();
		misparams.bAllowSupremacyPath = m_pbuttonAllowSupremacy->GetChecked();
		//misparams.bAllowEmptyTeams = m_pbuttonAllowEmptyTeams->GetChecked();
		misparams.bAllowAlliedRip = m_pbuttonAllowAlliedRip->GetChecked(); //imago 7/8/09 ALLY
        misparams.bShowMap = m_pbuttonShowMap->GetChecked();

        misparams.nTeams = FindValue(m_pcomboTeamCount->GetSelection(), "TeamCountValues");
        
        if (m_bIsZoneClub)
        {
            misparams.nMinPlayersPerTeam = FindValue(m_pcomboMaxPlayers->GetSelection(), "MinPlayersValues");
            misparams.nMaxPlayersPerTeam = FindValue(m_pcomboMaxPlayers->GetSelection(), "MaxPlayersValues");
        }
        else
        {
            misparams.nMinPlayersPerTeam = 1;
            misparams.nMaxPlayersPerTeam = std::min(100,misparams.nTotalMaxPlayersPerGame / misparams.nTeams);// KGJV #114
        }

        misparams.iMaxImbalance = FindValue(m_pcomboMaxImbalance->GetSelection(), "MaxImbalanceValues");
        misparams.iMinRank = FindValue(m_pcomboSkillLevel->GetSelection(), "GameSkillLevelMin");
        misparams.iMaxRank = FindValue(m_pcomboSkillLevel->GetSelection(), "GameSkillLevelMax");
        misparams.mmMapType = ItemIDToMapType(m_pcomboMapType->GetSelection());
        strcpy(misparams.szCustomMapFile, ItemIDToMapFile(m_pcomboMapType->GetSelection()));
        misparams.iRandomEncounters = FindValue(m_pcomboConnectivity->GetSelection(), "ConnectivityValues");
        misparams.iLives = FindValue(m_pcomboLives->GetSelection(), "LivesValues");
        misparams.nNeutralSectorMineableAsteroids = FindValue(m_pcomboResources->GetSelection(), "ResourcesNeutralMinable");
        misparams.nPlayerSectorMineableAsteroids = FindValue(m_pcomboResources->GetSelection(), "ResourcesPlayerMinable");
        misparams.nNeutralSectorSpecialAsteroids = FindValue(m_pcomboResources->GetSelection(), "ResourcesNeutralSpecial");
        misparams.nPlayerSectorSpecialAsteroids = FindValue(m_pcomboResources->GetSelection(), "ResourcesPlayerSpecial");
        misparams.fHe3Density = FindValue(m_pcomboTotalMoney->GetSelection(), "TotalMoneyValues");
        misparams.fStartingMoney = FindValue(m_pcomboStartingMoney->GetSelection(), "StartingMoneyValues");
        misparams.dtGameLength = FindValue(m_pcomboCountdownTime->GetSelection(), "CountdownTimeValues") * 60.0f;
        misparams.nGoalTeamKills = FindValue(m_pcomboDeathWatchKills->GetSelection(), "DeathWatchKillsValues");
        misparams.fGoalTeamMoney = FindValue(m_pcomboProsperity->GetSelection(), "ProsperityTargetValues");
        misparams.nGoalArtifactsCount = FindValue(m_pcomboArtifactCount->GetSelection(), "ArtifactCountValues");
        misparams.nGoalFlagsCount = FindValue(m_pcomboFlagCount->GetSelection(), "FlagCountValues");
        misparams.iGoalConquestPercentage = FindValue(m_pcomboConquestBases->GetSelection(), "ConquestPercentValues");
        misparams.iGoalTerritoryPercentage = FindValue(m_pcomboTerritory->GetSelection(), "TerritoryValues");

		//KGJV #62
		misparams.iMapSize = FindValue(m_pcomboMapSize->GetSelection(), "MapSizeValues");
		misparams.nPlayerSectorAsteroids  = FindValue(m_pcomboAsteriods->GetSelection(), "AsteriodsHomeValues");
		misparams.nNeutralSectorAsteroids = FindValue(m_pcomboAsteriods->GetSelection(), "AsteriodsNeutralValues");
		misparams.nPlayerSectorTreasures = FindValue(m_pcomboTreasures->GetSelection(), "TreasureHomeValues");
		misparams.nPlayerSectorTreasureRate = FindValue(m_pcomboTreasures->GetSelection(), "TreasureRateHomeValues") / 60.0f;
		misparams.nNeutralSectorTreasures = FindValue(m_pcomboTreasures->GetSelection(), "TreasureNeutralValues");
		misparams.nNeutralSectorTreasureRate = FindValue(m_pcomboTreasures->GetSelection(), "TreasureRateNeutralValues") / 60.0f;
		misparams.nInitialMinersPerTeam = FindValue(m_pcomboInitialMiners->GetSelection(), "initialMinersValues");
		misparams.nMaxDronesPerTeam = FindValue(m_pcomboMaximumDrones->GetSelection(), "maxDronesValues");

		// mmf 10/07 Experimental game type.  Hard coded this to 5 as GameType is not 'filled' from newgamescreen.mdl like
		// the others.  The entries are built in VerifyGameTypeInitialization in gametypes.cpp
		 misparams.bExperimental = (m_pcomboGameType->GetSelection() == 5) ? true : false;
	}

    bool OnButtonCreate()
    {
        BEGIN_PFM_CREATE_ALLOC(trekClient.m_fm, pfmMissionParams, CS, MISSIONPARAMS)
        END_PFM_CREATE

        ReadControls(pfmMissionParams->missionparams);

        const char* pszReason = pfmMissionParams->missionparams.Invalid(true);

        // don't let the player choose a game which they can't play in.
		// mmf unless they are a Privileged user
        RankID rankOwner = trekClient.MyPlayerInfo()->GetPersistScore(NA).GetRank();
        if (!pszReason)
        {
			if ( (rankOwner < pfmMissionParams->missionparams.iMinRank) && !UTL::PrivilegedUser(trekClient.m_szCharName,trekClient.m_pMissionInfo->GetCookie())) //Imago 6/10 #2
            {
                pszReason = "Skill Level must be set low enough for you to play.";
            }
			else if ( (rankOwner > pfmMissionParams->missionparams.iMaxRank) && !UTL::PrivilegedUser(trekClient.m_szCharName,trekClient.m_pMissionInfo->GetCookie()) ) //Imago 6/10 #2
            {
                pszReason = "Skill Level must be set high enough for you to play.";
            }
            else if (pfmMissionParams->missionparams.bSquadGame)
            {
                bool bFoundSquad = false;

                for (TList<SquadMembership>::Iterator iterSquad(trekClient.GetSquadMemberships());
                    !iterSquad.End(); iterSquad.Next())
                {
                    if (iterSquad.Value().GetIsLeader() 
                        || iterSquad.Value().GetIsAssistantLeader())
                    {
                        bFoundSquad = true;
                        break;
                    }
                }

                if (!bFoundSquad)
                    pszReason = "You must be a squad leader or an assistant squad leader to create a squad game";
            }
            else if (!ValidateNumTeams(pfmMissionParams->missionparams.nTeams, pfmMissionParams->missionparams.szCustomMapFile))
            {
                int mapTypeIndex = FindMapType(pfmMissionParams->missionparams.szCustomMapFile);
                assert(mapTypeIndex != NA);
                const StaticMapInfo& mapinfo = trekClient.GetStaticMapInfo(mapTypeIndex);

                const char* szFormat = "The map type '%s' can only be played with %d teams";
                char* cbTemp = (char*)_alloca(strlen(szFormat) + c_cbName + 8 + 1);
                wsprintf(cbTemp, szFormat, mapinfo.cbFriendlyName, mapinfo.nNumTeams);
                pszReason = cbTemp;
            }
        }

        // KGJV fix: prevent all teams to be inactive when game owner change the number of teams
        if (!pszReason)
        {
            // if number of teams was reduced
            if (pfmMissionParams->missionparams.nTeams < GetBaseMissionParams().nTeams)
            {
                // then we make sure there is at least one activated team
                bool bAllInactive = true;
                for (int i=0;i<pfmMissionParams->missionparams.nTeams;i++)
                {
                    if (trekClient.MyMission()->SideActive(i))
                    {
                        bAllInactive = false;
                        break;
                    }
                }
                if (bAllInactive)
                {
                    pszReason = "You can't reduce the number of teams if all remaining teams would be inactive.";
                }
            }
        }

		// TE: If they set stats count, set lock sides. mmf changed to modify MaxImbalance to 'auto' setting
		// mmf note game settings Max Team Imbalance field is not updated until you accept changes
		// TE: NOTE!! This has been removed since the balancing is now handled from within the GameSettings
		// screen itself, and does not rely on settings chosen outside of the screen.
		/*if (pfmMissionParams->missionparams.bScoresCount)
			pfmMissionParams->missionparams.iMaxImbalance = 0x7ffe;*/

        if (pszReason)
        {
            GetWindow()->GetPopupContainer()->OpenPopup(CreateMessageBox(pszReason, NULL),
                                                        false);
        }
        else
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            trekClient.m_fm.QueueExistingMsg((FEDMESSAGE *)pfmMissionParams);
            GetWindow()->SetWaitCursor();
            GetWindow()->GetPopupContainer()->OpenPopup(
                CreateMessageBox("Changing mission parameters...", NULL, false, false, 1.0f),
                false
            );
            m_fQuitting = true;
        }
		//Imago 7/18/09 turn off allies if these are set ALLY 
		//Imago 7/31/09 fixed heap corruption trying to access pfmMissionParams while being deallocated
		//imago 7/6/09 ALLY force Defections on when allies
		bool bAllies = false;
		for (SideID i = 0; i < trekClient.MyMission()->GetSideList()->GetCount() ; i++) {
			if (trekClient.MyMission()->SideAllies(i) != NA)  {
				bAllies = true;
				break;
			}
		}
		if (bAllies && (pfmMissionParams->missionparams.IsArtifactsGame() || pfmMissionParams->missionparams.IsDeathMatchGame() ||
		pfmMissionParams->missionparams.IsProsperityGame() || pfmMissionParams->missionparams.IsFlagsGame() ||
		pfmMissionParams->missionparams.IsTerritoryGame() || pfmMissionParams->missionparams.nTeams <= 2)) {
			pfmMissionParams->missionparams.bAllowAlliedRip = false;
			pfmMissionParams->missionparams.bAllowAlliedViz = false;
			pfmMissionParams->missionparams.bAllowDefections = false;
			pfmMissionParams->missionparams.iMaxImbalance = 0x7ffe;
			PFM_DEALLOC(pfmMissionParams);
			for (SideID i = 0; i < trekClient.MyMission()->GetSideList()->GetCount() ; i++) { 
				trekClient.MyMission()->SetSideAllies(i,NA);
				trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
				BEGIN_PFM_CREATE(trekClient.m_fm, pfmChangeAlliance, C, CHANGE_ALLIANCE)
				END_PFM_CREATE
				pfmChangeAlliance->sideID = i;
				pfmChangeAlliance->sideAlly = NA;
	            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
	            trekClient.m_fm.QueueExistingMsg((FEDMESSAGE *)pfmChangeAlliance);
			}
		} else {
        	PFM_DEALLOC(pfmMissionParams);
		}
		return true;
    }

    bool CanEdit()
    {
        return trekClient.GetPlayerInfo()->IsMissionOwner() 
            && !(trekClient.MyMission()->WasObjectModelCreated() && m_bIsZoneClub)
            && trekClient.MyMission()->GetStage() == STAGE_NOTSTARTED;
    }

    void UpdateButtonStates()
    {
        bool bEnable = CanEdit();

        m_pnumberCanChooseMaxPlayers->SetValue((m_bIsZoneClub && bEnable) ? 1.0f : 0.0f);

        m_peditPaneGameName->SetReadOnly(!bEnable || !m_bIsZoneClub); // pkk - Gamename not editable in Zonegames
        m_peditPaneGamePassword->SetReadOnly(!bEnable || !m_bIsZoneClub);
        m_peditPaneGamePassword->SetType((bEnable && m_bIsZoneClub) ? EditPane::Normal : EditPane::Password);
        
        m_pbuttonEjectPods->SetEnabled(bEnable);
        m_pbuttonFriendlyFire->SetEnabled(bEnable);
        //m_pbuttonStatsCount->SetEnabled(bEnable); // TE: Show the StatsCount checkbox - KGJV #62 removed
		
		//imago 7/6/09 ALLY force Defections on when allies
		bool bAllies = false;
		for (SideID i = 0; i < trekClient.MyMission()->GetSideList()->GetCount() ; i++) {
			if (trekClient.MyMission()->SideAllies(i) != NA && !bAllies) 
				bAllies = true;
		}
		m_pbuttonDefections->SetEnabled(!bAllies && bEnable);
		//

        m_pbuttonJoiners->SetEnabled(bEnable);
        m_pbuttonSquadGame->SetEnabled(bEnable && m_bIsZoneClub);
        m_pbuttonInvulnerableStations->SetEnabled(bEnable);
        m_pbuttonDevelopment->SetEnabled(bEnable);
        m_pbuttonAllowShipyards->SetEnabled(bEnable);
		// KGJV #62
		m_pbuttonAllowTactical->SetEnabled(bEnable);
        m_pbuttonAllowExpansion->SetEnabled(bEnable);
        m_pbuttonAllowSupremacy->SetEnabled(bEnable);
		//m_pbuttonAllowEmptyTeams->SetEnabled(false);//bEnable && m_bIsZoneClub); // KGJV #62
		
		m_pbuttonAllowAlliedRip->SetEnabled(bAllies && bEnable); //imago 7/8/09 ALLY


		m_pbuttonShowMap->SetEnabled(bEnable);

        m_pcomboTeamCount->SetEnabled(bEnable);
        m_pcomboMaxPlayers->SetEnabled(bEnable && !m_bLockGameOpen);
        m_pcomboMaxImbalance->SetEnabled(!bAllies && bEnable); //because imbalance impelemtation is now FUBAR even w/o allies... we'll disable it -Imago 8/1/09
        m_pcomboSkillLevel->SetEnabled(bEnable && m_bIsZoneClub); // pkk #203 - Disable SkillLevel on ZG
        m_pcomboMapType->SetEnabled(bEnable);
        m_pcomboConnectivity->SetEnabled(bEnable);
        m_pcomboLives->SetEnabled(bEnable);
        m_pcomboResources->SetEnabled(bEnable);
        m_pcomboTotalMoney->SetEnabled(bEnable);
        m_pcomboStartingMoney->SetEnabled(bEnable);
        m_pcomboCountdownTime->SetEnabled(bEnable);
        m_pcomboDeathWatchKills->SetEnabled(bEnable);
        m_pcomboProsperity->SetEnabled(bEnable);
        m_pcomboArtifactCount->SetEnabled(bEnable);
        m_pcomboFlagCount->SetEnabled(bEnable);
        m_pcomboConquestBases->SetEnabled(bEnable);
        m_pcomboTerritory->SetEnabled(bEnable);
        m_pcomboGameType->SetEnabled(bEnable);
		// KGJV #62
		m_pcomboMapSize->SetEnabled(bEnable);
		m_pcomboAsteriods->SetEnabled(bEnable);
		m_pcomboTreasures->SetEnabled(bEnable);
		m_pcomboInitialMiners->SetEnabled(bEnable);
		m_pcomboMaximumDrones->SetEnabled(bEnable);
		m_pbuttonNextMap->SetHidden(!bEnable);
		m_pbuttonPrevMap->SetHidden(!bEnable);

        m_pbuttonCreate->SetEnabled(bEnable);
        m_pbuttonCreate->SetHidden(!bEnable);
        m_pbuttonCancel->SetHidden(!bEnable);
        m_pbuttonBack->SetHidden(bEnable);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Fed Message handlers
    //
    //////////////////////////////////////////////////////////////////////////////

    void OnAddPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        // if this is me...
        if (trekClient.MyPlayerInfo()->ShipID() == pPlayerInfo->ShipID())
        {
            // maybe I'm a new leader
            UpdateButtonStates();
        }
    }

    void OnPlayerStatusChange(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        if (trekClient.MyPlayerInfo()->ShipID() == pPlayerInfo->ShipID())
        {
            UpdateButtonStates();
        }
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
            int nTimeLeft = std::max(0, int(trekClient.MyMission()->GetMissionParams().timeStart - Time::Now()) + 1);

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

    virtual void OnMissionCountdown(MissionInfo* pMissionDef)
    {
        UpdateButtonStates();
    }

   
    void OnMissionStarted(MissionInfo* pMissionDef)
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

    void OnAddMission(MissionInfo* pMissionInfo)
    {
        if (pMissionInfo == trekClient.MyMission())
        {
            if (m_fQuitting)
                GetWindow()->screen(ScreenIDTeamScreen);
            else
                RefreshMissionParams();
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
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateNewGameScreen(Modeler* pmodeler)
{
    return new NewGameScreen(pmodeler);
}

TRef<IPopup> CreateMissionParametersPopup(Modeler* pmodeler)
{
    ZString str;
    TRef<NewGameScreen> pngs = new NewGameScreen(pmodeler, str);

    return
        CreateMMLPopup(
            pmodeler,
            str,
            true
        );
}
