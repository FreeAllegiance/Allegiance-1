/*-------------------------------------------------------------------------
 * FSMission.cpp
 *
 * FedSrv mission handling stuff
 *
 * Owner:
 *
 * Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/

#include "pch.h"

//CFSMission statics
ListFSMission CFSMission::s_list;
int           CFSMission::s_iMissionID = 1; // just for igc--this too wil go away when we have single exe

const DWORD CFSMission::c_sbtPlayer = 0x00000001;
const DWORD CFSMission::c_sbtLeader = 0x00000002;
const float c_flUpdateTimeInterval = 10.0f;

static const char* sideNames[c_cSidesMax] =
  {
    "Team 1",
    "Team 2",
    "Team 3",
    "Team 4",
    "Team 5",
    "Team 6"
  };

void CFSMission::InitSide(SideID sideID)
{
/*                    { "Crusaders",
                      "Unity",
                      "Survivors",
                      "Protectorate",
                      "Colossal Mining Corp",
                      "Midnight Runners" };
*/
  assert(sideID >= 0 && sideID < c_cSidesMax);

  strcpy(m_misdef.rgszName[sideID], sideNames[sideID]);

  /*
  if (m_misdef.misparms.rgCivID[sideID] == NA)
    m_misdef.misparms.rgCivID[sideID] = PickNewCiv(sideID, m_misdef.rgCivID);
  assert (m_misdef.misparms.rgCivID[sideID] != NA);
  */
  assert (m_misdef.misparms.rgCivID[sideID] != NA);

  //m_misdef.rgCivID        [sideID] = m_misdef.misparms.rgCivID[sideID];
  m_misdef.rgShipIDLeaders[sideID] = NA;
  m_misdef.rgcPlayers     [sideID] = 0;
  m_misdef.rgfAutoAccept  [sideID] = true;
  m_misdef.rgfReady       [sideID] = false;
  m_misdef.rgfForceReady  [sideID] = false;
  m_misdef.rgfActive      [sideID] = true;
  m_misdef.rgfAllies	  [sideID] = NA; // #ALLY
  m_rgMoney[sideID] = 0;
}

static int InitializeCivList(const CivilizationListIGC*    pcivs,
                             CivID                         civIDs[])
{
    int n = 0;
    for (CivilizationLinkIGC*   pcl = pcivs->first(); (pcl != NULL); pcl = pcl->next())
    {
        CivID   civID = pcl->data()->GetObjectID();

        //Hack to prevent bonus civs from showing up
        //if (civID < 300)
            civIDs[n++] = civID;
    }
    return n;
}

CFSMission::CFSMission(
    const MissionParams& misparms,
    char * szDesc,
    IMissionSite * psiteMission,
    IIgcSite* psiteIGC,
    CAdditionalAGCParamData * paagcParamData,
    const char* pszStoryText
    ) :
    m_pMission(::CreateMission()), // IGC CreateMission
    m_psiteMission(psiteMission),
    m_psiteIGC(psiteIGC),
    m_fLobbyDirty(true),
    m_timeLastLobbyMissionInfo(Time::Now() - c_flUpdateTimeInterval), // fudge factor--want to
    m_nFrame(0),
    m_bShouldDelete(false),
    m_pszReason(NULL),
    m_flGameDuration(0.0f),
    m_pttbmNewSetting(NULL),
    m_pttbmAltered(NULL),
    m_strStoryText(pszStoryText)
{
  // Init mission def
  ZeroMemory(&m_misdef, sizeof(m_misdef));
  BEGIN_PFM_CREATE_PREALLOC(g.fm, &m_misdef, S, MISSIONDEF)
  END_PFM_CREATE
  m_misdef.misparms = misparms;

  //m_misdef.misparms.fStartingMoney = 0.0f;
  //m_misdef.misparms.iGoalTerritoryPercentage = 75;  //NYI
  //m_misdef.misparms.iRandomEncounters = 2;  //NYI

  // KGJV fix: fill in server friendly name so clients can display it
  strcpy_s(m_misdef.szServerName,sizeof(m_misdef.szServerName),g.strLocalAddress);

  psiteMission->Create(this);

  assert(!g.strLobbyServer.IsEmpty() == !!g.fmLobby.IsConnected());

  // keep track of whether this is a lobbied and/or club game
  m_misdef.misparms.bLobbiedGame = g.fmLobby.IsConnected();
#if !defined(ALLSRV_STANDALONE)
  m_misdef.misparms.bClubGame = true;
  // -KGJV only set core if not defined in game params
  if (m_misdef.misparms.szIGCStaticFile[0] == '\0')
  {
	strcpy(m_misdef.misparms.szIGCStaticFile, IGC_ENCRYPT_CORE_FILENAME);
  }
#else // !defined(ALLSRV_STANDALONE)
  m_misdef.misparms.bClubGame = false;
  // -KGJV only set core if not defined in game params
  if (m_misdef.misparms.szIGCStaticFile[0] == '\0')
  {
  strcpy(m_misdef.misparms.szIGCStaticFile, IGC_STATIC_CORE_FILENAME);
  }
  // hardcode this cap in one more place to make it harder to work around.
  m_misdef.misparms.nTotalMaxPlayersPerGame = min(c_cMaxPlayersPerGame, misparms.nTotalMaxPlayersPerGame);
#endif // !defined(ALLSRV_STANDALONE)

  // if this game is an auto-start game, set the start time appropriately
  if (m_misdef.misparms.bAutoRestart)
    m_misdef.misparms.timeStart = Time::Now() + m_misdef.misparms.fStartCountdown;

  // set Mission Name here too so that AGC Admin tool reports the params while mission stage is STAGE_NOTSTARTED
  m_pMission->SetMissionParams(&m_misdef.misparms);
  m_pMission->Initialize(g.timeNow, psiteIGC);
  m_pMission->SetPrivateData((DWORD) this); // so we can get back to a CFSMission from an ImissionIGC
  m_pgrpMission = g.fm.CreateGroup(m_misdef.misparms.strGameName);

  if (szDesc)
  {
    assert(lstrlen(szDesc) < sizeof(m_misdef.szDescription));
    lstrcpy(m_misdef.szDescription, szDesc);
  }
  else
  {
    // use the description in the mission params
    strncpy(m_misdef.szDescription, m_misdef.misparms.strGameName, sizeof(m_misdef.szDescription));
    m_misdef.szDescription[sizeof(m_misdef.szDescription)-1] = '\0';
  }
  m_misdef.iSideMissionOwner = NA; // will turn into the person who created this when they do a SideChange
  m_misdef.fAutoAcceptLeaders = true;
  m_misdef.fInProgress = false;
  m_misdef.stage = STAGE_NOTSTARTED;
#if !defined(ALLSRV_STANDALONE)
  extern void DoDecrypt(int size, char* pdata);
  m_misdef.misparms.verIGCcore = LoadIGCStaticCore(m_misdef.misparms.szIGCStaticFile, m_pMission, false,  DoDecrypt);
#else // !defined(ALLSRV_STANDALONE)
  m_misdef.misparms.verIGCcore = LoadIGCStaticCore(m_misdef.misparms.szIGCStaticFile, m_pMission, false,  NULL);
#endif // !defined(ALLSRV_STANDALONE)
  m_misdef.dwCookie = NULL;

  const CivilizationListIGC*  pcivs = m_pMission->GetCivilizations();

  const int cCivsMax = 20;
  assert (pcivs->n() < cCivsMax);

  //Get random initial civs for each side, minimizing duplication vs. pre-existing sides
  CivID civIDs[cCivsMax];
  int n = InitializeCivList(pcivs, civIDs);
  int iSide = 0;

  for (iSide = 0; iSide < c_cSidesMax; iSide++)
  {
    if (NA == m_misdef.misparms.rgCivID[iSide])
    {
        if (n == 1)
        {
            m_misdef.misparms.rgCivID[iSide] = civIDs[0];
            n = InitializeCivList(pcivs, civIDs);
        }
        else
        {
            int index = randomInt(0, --n);
            m_misdef.misparms.rgCivID[iSide] = civIDs[index];

            if (index != n)
            {
                civIDs[index] = civIDs[n];
            }
        }
    }
    InitSide(iSide);
  }

  // Build the mission
  m_pMission->SetMissionID(s_iMissionID++);
  char szRealTeams[] = ": All real teams";
  char szBuff[sizeof(m_misdef.misparms.strGameName) + sizeof(szRealTeams)];
  wsprintf(szBuff, "%s%s", m_misdef.misparms.strGameName, szRealTeams);
  m_pgrpSidesReal = g.fm.CreateGroup(szBuff);

  GetSystemTime(&m_stStartTime);

  m_pMission->UpdateSides(Time::Now(), &m_misdef.misparms, m_misdef.rgszName);

  s_list.last(this);            // add us to the list of missions

  // handle initation list, if any
  m_nInvitationListID = m_misdef.misparms.nInvitationListID;
  if (RequiresInvitation())
  {
    // Invitations will NOT be added to the right right now, but will be added later, asynchronously after they come back from sql.
    GetInvitations(m_nInvitationListID, GetMissionID());
  }

  if (paagcParamData)
  {
    TechTreeBitMask ttbmBlank;
    ttbmBlank.ClearAll();

    for (iSide = 0; iSide < c_cSidesMax; iSide++)
    {
      // detect admin made changes
      if (ttbmBlank != paagcParamData->m_ttbmAltered[iSide])
      {
          MakeOverrideTechBits();
          m_pttbmAltered[iSide] = paagcParamData->m_ttbmAltered[iSide];
          m_pttbmNewSetting[iSide] = paagcParamData->m_ttbmNewSetting[iSide];
      }

      // detect admin made changes
      if(paagcParamData->m_szTeamName[iSide][0] != '\0')
        SetSideName(iSide, paagcParamData->m_szTeamName[iSide]);
    }
  }

  //
  // Fire off an AGC event that a game was created
  //
  LPCSTR pszGame = GetMissionDef()->misparms.strGameName;
  long idGame = GetIGCMission() ? GetIGCMission()->GetMissionID() : -1;

  LPCSTR pszContext = GetIGCMission() ? GetIGCMission()->GetContextName() : NULL;

  _AGCModule.TriggerContextEvent(NULL, EventID_GameCreated, pszContext,
    pszGame, idGame, -1, -1, 0);
}


CFSMission::~CFSMission()
{
  //
  // Fire off an AGC event that a game was destroyed
  //
  LPCSTR pszGame = GetMissionDef()->misparms.strGameName;
  long idGame = GetIGCMission() ? GetIGCMission()->GetMissionID() : -1;

  LPCSTR pszContext = GetIGCMission() ? GetIGCMission()->GetContextName() : NULL;

  _AGCModule.TriggerContextEvent(NULL, EventID_GameDestroyed, pszContext,
    pszGame, idGame, -1, -1, 0);

  // Destroy any existing AGC game associated with the IGC mission
  GetAGCGlobal()->RemoveAGCObject(m_pMission, true);

  Vacate();

  // We don't want anymore mission related messages from any players still here
  ShipLinkIGC * pShiplink = m_pMission->GetShips()->first();
  while (pShiplink)
  {
    CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
    pShiplink = pShiplink->next();
    assert (pfsShip->IsPlayer());
    RemovePlayerFromMission(pfsShip->GetPlayer(), QSR_ServerShutdown);
  }

  //
  // free all container elements
  //
  for (std::vector<CFSCluster*>::iterator i(m_pFSClusters.begin()); i != m_pFSClusters.end(); ++i)
    delete static_cast<CFSCluster*>(*i);

  for (std::vector<CFSSide*>::iterator _i(m_pFSSides.begin()); _i != m_pFSSides.end(); ++_i)
    delete static_cast<CFSSide*>(*_i);

  if(m_pttbmNewSetting)
    delete[] m_pttbmNewSetting;

  if(m_pttbmAltered)
    delete[] m_pttbmAltered;

  // And the mission is definitely over at this point, so the lobby and all clients should forget about it
  SetStage(STAGE_TERMINATE);
  g.fm.DeleteGroup(m_pgrpMission);

  if (g.fmLobby.IsConnected())
  {
    BEGIN_PFM_CREATE(g.fmLobby, pfmMissionGone, LS, MISSION_GONE)
    END_PFM_CREATE
    pfmMissionGone->dwCookie = GetCookie();
    g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
  }

  // remove us from our list of missions
  // mdvalley: plinkFSMis needs defining out here
  LinkFSMission * plinkFSMis;
  for (plinkFSMis = s_list.first(); plinkFSMis; plinkFSMis = plinkFSMis->next())
  {
    CFSMission * pfsMission = plinkFSMis->data();
    if (this == pfsMission)
    {
      delete plinkFSMis;
      break;
    }
  }
  assert (plinkFSMis);      // better have found it
  m_pMission->Terminate();
  delete m_pMission;
  m_psiteMission->Destroy(this);
  g.fm.DeleteGroup(m_pgrpSidesReal);

#if defined(ALLSRV_STANDALONE)
// Possibly shutdown the standalone server if no more games

#if !defined(SRV_CHILD)
  // KGJV #114 - if lobbied then dont shutdown if create game allowed on this server
    bool bSupposedToConnectToLobby = !(FEDSRV_GUID != g.fm.GetHostApplicationGuid());
	if ( (0 == s_list.n()) && (bSupposedToConnectToLobby ? (g.cStaticCoreInfo==0) : true)) {

		// Disconnect from the lobby server
		DisconnectFromLobby();
		g.strLobbyServer.SetEmpty();

		// Shutdown the server if no sessions exist
		if (0 == CAdminSession::GetSessionCount())
			PostThreadMessage(g.idReceiveThread, WM_QUIT, 0, 0);
	}

#else
		// Disconnect from the lobby server
		DisconnectFromLobby();
		g.strLobbyServer.SetEmpty();
		PostThreadMessage(g.idReceiveThread, WM_QUIT, 0, 0);
#endif
#endif // defined(ALLSRV_STANDALONE)
  // kill any pending ballots
  while (!m_ballots.IsEmpty())
    delete m_ballots.PopFront();
}


/*-------------------------------------------------------------------------
 * AddPlayerToMission
 *-------------------------------------------------------------------------
 * Purpose:
 *    Set a player on a mission in the team lobby.
 *    This must be called INSTEAD OF IGC's SetSide() for the player's new side.
 */

void CFSMission::AddPlayerToMission(CFSPlayer * pfsPlayer)
{
  debugf("Player %s, ship=%d joined mission=%x\n",
          pfsPlayer->GetName(), pfsPlayer->GetShipID(),
          GetCookie());

  assert (pfsPlayer->GetPlayerScoreObject());

  OldPlayerLink*  popl = GetOldPlayerLink(pfsPlayer->GetName());
  if (popl)
  {
      pfsPlayer->SetBannedSideMask(popl->data().bannedSideMask);
	  // mdvalley: Pull the side flown for
	  pfsPlayer->SetLastSide(popl->data().lastSide);

      PlayerScoreObject * ppso = &(popl->data().pso);

      pfsPlayer->GetIGCShip()->SetExperience(0.0f);
      pfsPlayer->GetIGCShip()->SetKills(ppso->GetKills());
      pfsPlayer->GetIGCShip()->SetDeaths(ppso->GetDeaths());
      pfsPlayer->GetIGCShip()->SetEjections(ppso->GetEjections());
      pfsPlayer->SetPlayerScoreObject(ppso);

      if (popl->data().pclusterLifepod)
          pfsPlayer->SetLifepod(popl->data().pclusterLifepod,
                                popl->data().positionLifepod);

      //delete popl;
  }
  else
  {
      pfsPlayer->GetIGCShip()->SetKills(0);
      pfsPlayer->GetIGCShip()->SetDeaths(0);
      pfsPlayer->GetIGCShip()->SetEjections(0);
  }

  g.fm.AddConnectionToGroup(GetGroupMission(), pfsPlayer->GetConnection());
  SendLobbyMissionInfo(pfsPlayer);

  pfsPlayer->SetSide(this, GetIGCMission()->GetSide(SIDE_TEAMLOBBY));
  pfsPlayer->GetIGCShip()->CreateDamageTrack();

  SendPlayerInfo(NULL, pfsPlayer, this);
  assert(0 == g.fm.CbUsedSpaceInOutbox()); // everything should've been sent

  // tell the lobby that this character is now in this mission
  if (g.fmLobby.IsConnected())
  {
    assert(GetCookie()); // we can't be sending messages w/ cookies unless we have a real cookie

	 // BT - 9/11/2010 ACSS - Supports authentication check of the CD Key.
	char szAddress[16];
	g.fm.GetIPAddress(*pfsPlayer->GetConnection(), szAddress);

    BEGIN_PFM_CREATE(g.fmLobby, pfmPlayerJoined, S, PLAYER_JOINED)
      FM_VAR_PARM(pfsPlayer->GetName(), CB_ZTS)
      FM_VAR_PARM(pfsPlayer->GetCDKey(), CB_ZTS)
	  FM_VAR_PARM(szAddress, CB_ZTS) // BT - 9/11/2010 ACSS - Supports authentication check of the CD Key.
    END_PFM_CREATE
    pfmPlayerJoined->dwMissionCookie = GetCookie();
    g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);

#if defined(SRV_CHILD)
	// Imago 6/25/08
	if (GetGroupMission()->GetCountConnections() == 0 && GetGameDuration() < 15.0f && g.dwArgCookie > 99) { // <-- set that to the pump cycle
		char strFilename[10] = "\0";
		sprintf(strFilename,"%d.pid",GetCurrentProcessId());
		UTL::SaveFile(strFilename,"1",2,NULL,false); // the creation of this file signals the parent
		// tell the lobby about the mission
		BEGIN_PFM_CREATE(g.fmLobby, pfmNewMission, S, NEW_MISSION)
		END_PFM_CREATE
		pfmNewMission->dwIGCMissionID = GetIGCMission()->GetMissionID();
	}

#endif
		g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
  }

  m_bShouldDelete = false;
  SetLobbyIsDirty();
}


/*-------------------------------------------------------------------------
 * RemovePlayerFromMission
 *-------------------------------------------------------------------------
 * Purpose:
 *    Removes a player in the team lobby from the mission.
 *    This must be called INSTEAD OF IGC's SetSide() for the player's new side.
 */

void CFSMission::RemovePlayerFromMission(CFSPlayer * pfsPlayer, QuitSideReason reason, const char* szMessageParam)
{
  debugf("Player %s, ship=%d quiting mission=%x, reason=%d\n",
          pfsPlayer->GetName(), pfsPlayer->GetShipID(),
          GetCookie(), reason);

  // move them to the lobby first
  PlayerScoreObject*    ppso = pfsPlayer->GetPlayerScoreObject();
  if (ppso->Connected())
  {
    pfsPlayer->GetPlayerScoreObject()->Disconnect(g.timeNow);
  }

  //Cache the player's old score object
  SaveAsOldPlayer(pfsPlayer, QSRIsBoot(reason));

  if (pfsPlayer->GetSide()->GetObjectID() != SIDE_TEAMLOBBY)
    RemovePlayerFromSide(pfsPlayer, reason, szMessageParam);

  // Since they're quiting, then they're not requesting a position anywhere anymore
  RemoveJoinRequest(pfsPlayer, NULL);

  BEGIN_PFM_CREATE(g.fm, pfmQuitMission, CS, QUIT_MISSION)
    FM_VAR_PARM(szMessageParam, CB_ZTS)
  END_PFM_CREATE
  pfmQuitMission->shipID       = pfsPlayer->GetShipID();
  pfmQuitMission->reason       = reason;

  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

  // if they are quitting the mission, wait to remove them until we have sent
  // the side change.
  pfsPlayer->GetIGCShip()->DeleteDamageTrack();
  pfsPlayer->SetSide(NULL, NULL);

  // Let's just be super paranoid and make sure they're not in any of the mission's groups
  // TODO: Verify that they're already gone in debug builds
  g.fm.DeleteConnectionFromGroup(GetGroupMission(),   pfsPlayer->GetConnection());
  g.fm.DeleteConnectionFromGroup(GetGroupLobbySide(), pfsPlayer->GetConnection());
  g.fm.DeleteConnectionFromGroup(GetGroupRealSides(), pfsPlayer->GetConnection());

  // tell the lobby that this character is no longer in this mission
  if (g.fmLobby.IsConnected() && GetCookie())
  {
    BEGIN_PFM_CREATE(g.fmLobby, pfmPlayerQuit, S, PLAYER_QUIT)
      FM_VAR_PARM(pfsPlayer->GetName(), CB_ZTS)
    END_PFM_CREATE
    pfmPlayerQuit->dwMissionCookie = GetCookie();
    g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
  }

  // KGJV #114 changed logic to remove empty games
  if (!HasPlayers(NULL, true)          && // no one left in the mission
      //!m_misdef.misparms.bAllowEmptyTeams && // game doesn't allow empty sides
      //m_misdef.misparms.bClubGame && // not a standalone server - KGJV #114
      //(IMPLIES( GetMissionDef()->misparms.bObjectModelCreated, GetStage() != STAGE_NOTSTARTED )) // not an admin created game that has started
	  !GetMissionDef()->misparms.bObjectModelCreated
     )
  {
    m_bShouldDelete = true;
  }
  else
  {
    SetLobbyIsDirty();
  }
}


/*-------------------------------------------------------------------------
 * AddPlayerToSide
 *-------------------------------------------------------------------------
 * Purpose:
 *    Set a player from the team lobby to a side, to maintain team/mission leader status
 *    This must be called INSTEAD OF IGC's SetSide() for the player's new side.
 *
 * Side Effects:
 *    Maintain team/mission leader status
 */

void CFSMission::AddPlayerToSide(CFSPlayer * pfsPlayer, IsideIGC * pside)
{
  assert (pside);
  ShipID    shipid  = pfsPlayer->GetShipID();
  SideID    sideid  = pside->GetObjectID();
  bool      fTeamLeader   = false;
  bool      fMissionOwner = false;
  ImissionIGC * pMission = GetIGCMission();

  // w0dk4 join-drop bug fix (allow more time when joining)
  pfsPlayer->SetJustJoined(true);

  assert (sideid != SIDE_TEAMLOBBY);

  CFSSide*  pfsSide = CFSSide::FromIGC(pside);

  if (pfsPlayer->GetSide() == NULL || pfsPlayer->GetMission() != this)
  {
    assert(false);
    if (pfsPlayer->GetMission())
      RemovePlayerFromMission(pfsPlayer, QSR_Quit);
    pfsPlayer->GetMission()->AddPlayerToMission(pfsPlayer);
  }
  else if (pfsPlayer->GetSide()->GetObjectID() != SIDE_TEAMLOBBY)
    RemovePlayerFromSide(pfsPlayer, QSR_SwitchingSides);

  // mdvalley: Set side last flown for
  pfsPlayer->SetLastSide(sideid);

  debugf("Player %s, ship=%d joined side %d, mission=%x\n",
          pfsPlayer->GetName(), pfsPlayer->GetShipID(),
          sideid, GetCookie());

  {
      OldPlayerLink*  popl = GetOldPlayerLink(pfsPlayer->GetName());
      if (popl)
      {
          delete popl;
      }
  }

  // Since they're changing sides, then they're not requesting a position anywhere anymore
  RemoveJoinRequest(pfsPlayer, pside);

  // check to see if we need a mission leader...
  if (m_misdef.iSideMissionOwner < 0)
    fMissionOwner = true;
  else
  {
    IsideIGC * pside = pMission->GetSide(m_misdef.iSideMissionOwner);

    if (!HasPlayers(pside, true)) // no one on the mission leader's side
      fMissionOwner = true;
  }

  assert(pside->GetActiveF()); // shouldn't be joining an inactive team
  m_misdef.rgcPlayers[sideid]++;

  // Set their stuff appropriate for this side
  assert(pfsPlayer->GetMoney() == 0);

  //pfsPlayer->GetIGCShip()->SetWingID(1); // TE: Default wing is Attack(1)  //Imago removed 6/10 #91
  if (!HasPlayers(pside, true)) // we have a new team leader
  {
    fTeamLeader = true;
    assert (m_misdef.rgShipIDLeaders[sideid] == NA);

    SetLeaderID(sideid, shipid);

    if (fMissionOwner) // we have a new mission leader too
    {
      fMissionOwner = true;
      m_misdef.iSideMissionOwner = sideid;
    }
  }

  // announce the guy that we were called for. Make sure nothing got wacky
  assert(IMPLIES(fMissionOwner, fTeamLeader));

  assert(IFF(pside->GetActiveF(),
      (STAGE_NOTSTARTED == GetStage()) || (HasPlayers(pside, true) || m_misdef.misparms.bAllowEmptyTeams)));

  BEGIN_PFM_CREATE(g.fm, pfmJoinSide, S, JOIN_SIDE)
  END_PFM_CREATE
  pfmJoinSide->shipID = shipid;
  pfmJoinSide->sideID = sideid;

  if (fTeamLeader)
  {
    BEGIN_PFM_CREATE(g.fm, pfmSetTeamLeader, CS, SET_TEAM_LEADER)
    END_PFM_CREATE
    pfmSetTeamLeader->shipID = shipid;
    pfmSetTeamLeader->sideID = sideid;

    // put them on the command wing
    pfsPlayer->GetIGCShip()->SetWingID(0);
  }

  if (fMissionOwner)
  {
    BEGIN_PFM_CREATE(g.fm, pfmSetMissionOwner, CS, SET_MISSION_OWNER)
    END_PFM_CREATE
    pfmSetMissionOwner->shipID = shipid;
    pfmSetMissionOwner->sideID = sideid;
  }

  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

  pfsPlayer->SetTreasureObjectID(NA);         //NYI shouldn't we also set money here?
  pfsPlayer->SetSide(this, pside);
  // pfsPlayer->SetReady(true); Imago commented out so afk not reset

  	//Imago 6/10 we must set server-side default wing to NA! #91
  if (pfsPlayer->GetIGCShip()->GetWingID() != NA) {
	BEGIN_PFM_CREATE(g.fm, pfmSetWingID, CS, SET_WINGID)
	END_PFM_CREATE
	pfmSetWingID->wingID = pfsPlayer->GetIGCShip()->GetWingID();
	pfmSetWingID->shipID = shipid;
	pfmSetWingID->bCommanded = true;
	g.fm.SendMessages(pfsSide->GetGroup(), FM_GUARANTEED, FM_FLUSH);
  }

  if (fTeamLeader)
  {
    if (m_misdef.misparms.bLockTeamSettings)
    {
      // if this is a squad game, set the squad to the invited squad.
      if (m_misdef.misparms.bSquadGame)
      {
        const char* pzSquadName = CFSSide::FromIGC(pside)->GetInvitedSquadName();

        if(pzSquadName)
        {
          SquadMembershipLink* pSquadLink = pfsPlayer->GetSquadMembershipList()->first();
          for (; pSquadLink; pSquadLink = pSquadLink->next())
          {
            if (_stricmp(pSquadLink->data()->GetName(), pzSquadName) == 0)
            {
              SetSideSquad(sideid, pSquadLink->data()->GetID());
              break;
            }
          }
        }
      }
    }
    // set the squad if this is a squad game
    else if (m_misdef.misparms.bSquadGame)
      SetSideSquad(sideid, pfsPlayer->GetPreferredSquadToLead());
    // otherwise reset the team name
    else
      SetSideName(sideid, sideNames[sideid]);
  }

  if (STAGE_STARTED != GetStage() && STAGE_STARTING != GetStage())
  {
    g.fm.SetDefaultRecipient((CFMRecipient*) pfsPlayer->GetConnection(),
                             FM_GUARANTEED);

    //Tell the player wing ID for every other person on the side
    for (ShipLinkIGC*   psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
    {
        IshipIGC*   ps = psl->data();
        if (ps != pfsPlayer->GetIGCShip())
        {
            ShipID   shipID = ps->GetObjectID();

            BEGIN_PFM_CREATE(g.fm, pfmSetWingID, CS, SET_WINGID)
            END_PFM_CREATE
            pfmSetWingID->wingID = ps->GetWingID();
            pfmSetWingID->shipID = shipID;
            pfmSetWingID->bCommanded = true;
        }
    }

    g.fm.SendMessages(NULL, FM_GUARANTEED, FM_FLUSH); // default recipient

    if (STAGE_NOTSTARTED == GetStage())
      CheckForSideAllReady(pside);
  }
  else
  {
    // set them to auto-donate to the team leader
    CFSPlayer* pfsLeader = GetLeader(pside->GetObjectID());
    pfsPlayer->SetAutoDonate((pfsPlayer != pfsLeader) ? pfsLeader : NULL, 0, true);

    // tell them their starting sector so they can display the mission briefing
    bool bGenerateCivBriefing = m_strStoryText.IsEmpty();
    const char* szBriefingText =  bGenerateCivBriefing
        ? GetBase(pside)->GetCluster()->GetName() : m_strStoryText;
    BEGIN_PFM_CREATE(g.fm, pfmSetBriefingText, S, SET_BRIEFING_TEXT)
      FM_VAR_PARM(szBriefingText, CB_ZTS)
    END_PFM_CREATE
    pfmSetBriefingText->fGenerateCivBriefing = bGenerateCivBriefing;

    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

    // tell them about the world
    SendMissionInfo(pfsPlayer, pside);

    //  tell them about the players who have flags AND WINGS - IMAGO #91 7/10
    {
      const ShipListIGC*    pships = m_pMission->GetShips();
      for (ShipLinkIGC* psl = pships->first(); (psl != NULL); psl = psl->next())
      {
        IshipIGC*   pship = psl->data();

		if (pship != pfsPlayer->GetIGCShip() && pship->GetPilotType() >= c_ptPlayer)
        {
            ShipID   shipID = pship->GetObjectID(); 
            BEGIN_PFM_CREATE(g.fm, pfmSetWingID, CS, SET_WINGID) 
            END_PFM_CREATE
            pfmSetWingID->wingID = pship->GetWingID(); // AND WINGS - IMAGO #91 7/10
            pfmSetWingID->shipID = shipID;
            pfmSetWingID->bCommanded = true;
        }

        SideID  sidFlag = pship->GetFlag();
        if (sidFlag != NA)
        {
          BEGIN_PFM_CREATE(g.fm, pfmGain, S, GAIN_FLAG)
          END_PFM_CREATE
          pfmGain->sideidFlag = sidFlag;
          pfmGain->shipidRecipient = pship->GetObjectID();
		  pfmGain->bIsTreasureDocked = false; // KGJV #118
        }
      }
    }

    assert (pfsPlayer->GetIGCShip()->GetDeaths() == pfsPlayer->GetPlayerScoreObject()->GetDeaths());
    assert (pfsPlayer->GetIGCShip()->GetEjections() == pfsPlayer->GetPlayerScoreObject()->GetEjections());
    assert (pfsPlayer->GetIGCShip()->GetChildShips()->n() == 0);
    if (pfsPlayer->GetLifepodCluster() && (STAGE_STARTED == GetStage()))
    {
        //Flush the send buffer to clear out the flag information.
        g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

        //Oopsie ... joined in their underwear ... make em' walk home
        IhullTypeIGC*   pht = pfsPlayer->GetSide()->GetCivilization()->GetLifepod();
        pfsPlayer->GetIGCShip()->SetBaseHullType(pht);
        pfsPlayer->ShipStatusHullChange(pht);

        pfsPlayer->GetIGCShip()->SetPosition(pfsPlayer->GetLifepodPosition());
        pfsPlayer->GetIGCShip()->SetVelocity(Vector::GetZero());

        Orientation o;
        o.Reset();
        pfsPlayer->GetIGCShip()->SetOrientation(o);

        pfsPlayer->GetIGCShip()->SetCluster(pfsPlayer->GetLifepodCluster());

        pfsPlayer->SetLifepod(NULL, Vector::GetZero());
    }
    else
    {
        IstationIGC * pstation = GetBase(pside);

        pfsPlayer->GetIGCShip()->SetStation(pstation);
        pfsPlayer->ShipStatusStart(pstation);
    }

    if (STAGE_STARTED == GetStage())
    {
      pfsPlayer->GetPlayerScoreObject()->Connect(g.timeNow);

      // let them know that the game has started
      BEGIN_PFM_CREATE(g.fm, pfmEnterGame, S, ENTER_GAME)
      END_PFM_CREATE


      if (m_misdef.misparms.IsProsperityGame())
      {
        //Ship down the win the game bucket for all sides
        for (SideLinkIGC*   psl = pMission->GetSides()->first(); (psl != NULL); psl = psl->next())
        {
            BucketLinkIGC* pbl = psl->data()->GetBuckets()->last();
            {
              assert (pbl != NULL);
              IbucketIGC* pbucket = pbl->data();
              assert ((pbucket->GetBucketType() == OT_development) &&
                      (pbucket->GetBuyable()->GetObjectID() == c_didTeamMoney));

              BEGIN_PFM_CREATE(g.fm, pfmBucketStatus, S, BUCKET_STATUS)
              END_PFM_CREATE
              pfmBucketStatus->timeTotal  = pbucket->GetTime();
              pfmBucketStatus->moneyTotal = pbucket->GetMoney();
              pfmBucketStatus->iBucket    = pbucket->GetObjectID();
              pfmBucketStatus->sideID     = psl->data()->GetObjectID();
            }
        }
      }

      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

    }
  }
}


/*-------------------------------------------------------------------------
 * RemovePlayerFromSide
 *-------------------------------------------------------------------------
 * Purpose:
 *    Removes a player from a side to the team lobby, to maintain team/mission leader status
 *    This must be called INSTEAD OF IGC's SetSide() for the player's new side.
 *
 * Side Effects:
 *    Maintain team/mission leader status
 */

void CFSMission::RemovePlayerFromSide(CFSPlayer * pfsPlayer, QuitSideReason reason, const char* szMessageParam)
{
  IsideIGC * psideOld = pfsPlayer->GetSide();
  SideID sideidOld = psideOld->GetObjectID();
  pfsPlayer->SetDPGroup(NULL, false);

  ShipID    shipid  = pfsPlayer->GetShipID();
  SquadID squadID = psideOld->GetSquadID();
  IshipIGC * pshipPlayer = pfsPlayer->GetIGCShip();
  ImissionIGC * pMission = GetIGCMission();
  bool bTurnOnAutoaccept = false;
  bool bWasTeamLeader = false;

  pfsPlayer->GetPlayerScoreObject()->Disconnect(g.timeNow);

  // mdvalley: Set last side to NOAT if the game is not running
  if(GetStage() != STAGE_STARTED)
	  pfsPlayer->SetLastSide(SIDE_TEAMLOBBY);

  SideID   sidFlag = pshipPlayer->GetFlag();
  if (sidFlag != NA)
  {
    const Vector&   p = pfsPlayer->GetIGCShip()->GetPosition();
    float           lm = m_pMission->GetFloatConstant(c_fcidLensMultiplier);
    float           r = 1.5f * m_pMission->GetFloatConstant(c_fcidRadiusUniverse);
    if (p.x*p.x + p.y*p.y + p.z*p.z/(lm*lm) > r*r)
    {
      ((FedSrvSiteBase*)m_psiteIGC)->RespawnFlag(sidFlag, NULL); // mmf 11/07 added second argument pside (which is set to null in this case)
    }
    else
    {
      DataTreasureIGC dt;
      dt.treasureCode = c_tcFlag;
      dt.treasureID = sidFlag;
      dt.amount = 0;
      dt.createNow = false;
      dt.lifespan = 3600.0f * 24.0f * 10.0f;      //10 days
      CreateTreasure(g.timeNow, pfsPlayer->GetIGCShip(), &dt, pfsPlayer->GetIGCShip()->GetPosition(), 100.0f);
    }
  }

  debugf("Player %s, ship=%d quitting side %d, mission=%x\n",
          pfsPlayer->GetName(), pfsPlayer->GetShipID(),
          pfsPlayer->GetSide() ? pfsPlayer->GetSide()->GetObjectID() : NA,
          GetCookie());

  pfsPlayer->ShipStatusExit();

  //They are leaving their team ... promote
  {
    const ShipListIGC*  pshipsChildren = pfsPlayer->GetIGCShip()->GetChildShips();
    if (pshipsChildren->first())
    {
        IclusterIGC*        pcluster = pfsPlayer->GetIGCShip()->GetCluster();
        IshipIGC*   pship = pshipsChildren->first()->data();
        pship->Promote();
        ((CFSShip*)(pship->GetPrivateData()))->ShipStatusRecalculate();

        BEGIN_PFM_CREATE(g.fm, pfmPromote, S, PROMOTE)
        END_PFM_CREATE
        pfmPromote->shipidPromoted = pship->GetObjectID();

        if (pcluster)
            g.fm.SendMessages(GetGroupSectorFlying(pcluster), FM_GUARANTEED, FM_FLUSH); //GetGroupRealSides(),
        else
            g.fm.SendMessages(CFSSide::FromIGC(psideOld)->GetGroup(), FM_GUARANTEED, FM_FLUSH); //GetGroupRealSides(),
    }
  }

  m_misdef.rgcPlayers[sideidOld]--;

  CFSPlayer*    pfsNewLeader = NULL;
  if (m_misdef.rgShipIDLeaders[sideidOld] == shipid)
  {
    //The person leaving the team was the team leader ...
    bWasTeamLeader = true;
    SideID iSideNewOwner = sideidOld; // side of person getting promoted, which is same team if anyone left
    ShipID shipidNewOwner = NA;
    bool fMissionOwner = false;

    int cPlayers = GetCountOfPlayers(psideOld, true);

    if (cPlayers == 1)
    {
      //There was one player -- the soon to be gone team leader

      if (STAGE_NOTSTARTED == m_misdef.stage)
      {
        //the game hasn't started yet ... accept everyone on the request list
        BEGIN_PFM_CREATE(g.fm, pfmAutoAccept, CS, AUTO_ACCEPT)
        END_PFM_CREATE
        pfmAutoAccept->iSide = sideidOld;
        pfmAutoAccept->fAutoAccept = true;
        g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
        bTurnOnAutoaccept = true;
      }
      else
      {
        // otherwise, no one accepted them to the team so I suppose they've
        // been implicitly rejected.
        RejectSideJoinRequests(psideOld);
      }
    }

    if (cPlayers > 1) // Other people still on the side--side count not adjusted yet
    {
      const ShipListIGC * plistShip = psideOld->GetShips();

      if (STAGE_STARTED == m_misdef.stage)
      {
          //Leader left the game in the middle ... pick a leader based on who has the
          //most people donating to them
          IshipIGC* pshipNewLeader = PickNewLeader(plistShip, pshipPlayer, 0);
          if (pshipNewLeader)
          {
              pfsNewLeader = ((CFSShip*)(pshipNewLeader->GetPrivateData()))->GetPlayer();
              shipidNewOwner = pshipNewLeader->GetObjectID();
          }
      }

      if (pfsNewLeader == NULL)
      {
        // Look for someone else on the team to be team leader
        // (there must be one), favoring someone who can lead the current squad.
        for (ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
        {
          if (plinkShip->data() != pshipPlayer && ISPLAYER(plinkShip->data()))
          {
            CFSPlayer* pfsPlayerTemp = ((CFSShip*)(plinkShip->data()->GetPrivateData()))->GetPlayer();

            if (pfsPlayerTemp->GetCanLeadSquad(squadID))
            {
              pfsNewLeader = pfsPlayerTemp;
              shipidNewOwner = plinkShip->data()->GetObjectID();
              break;
            }
            else if (!pfsNewLeader)
            {
              pfsNewLeader = pfsPlayerTemp;
              shipidNewOwner = plinkShip->data()->GetObjectID();
            }
          }
        }
        assert(pfsNewLeader);
      }

      SetLeaderID(iSideNewOwner, shipidNewOwner);

      if (m_misdef.iSideMissionOwner == sideidOld)
      {
        fMissionOwner = true;
      }
    }
    else // no one left on the team
    {
      // Then it better be auto-accept again
      SetLeaderID(sideidOld, NA);

      // also, better set the squad to NA if the game hasn't started
      // (but leave the squad for scoring purposes if it has started)
      if (STAGE_NOTSTARTED == m_misdef.stage)
        SetSideSquad(sideidOld, NA);

      if (m_misdef.iSideMissionOwner == sideidOld)
      {
        m_misdef.iSideMissionOwner = NA;

        // Now gotta look for another side to be mission owner
        const SideListIGC * plistSide = m_pMission->GetSides();
        for (SideLinkIGC * plinkSide = plistSide->first(); plinkSide; plinkSide = plinkSide->next())
        {
          IsideIGC * pside = plinkSide->data();
          if ((psideOld != pside) && HasPlayers(pside, true)) // found one
          {
            iSideNewOwner = pside->GetObjectID();
            shipidNewOwner = GetLeader(iSideNewOwner)->GetShipID();
            fMissionOwner = true;
            assert(m_misdef.rgShipIDLeaders[iSideNewOwner] == shipidNewOwner);
            m_misdef.iSideMissionOwner = iSideNewOwner;
            break;
          }
        }

        // if we don't have a new mission owner, unlock the lobby and the sides
        if (m_misdef.iSideMissionOwner == NA)
        {
            if (m_misdef.misparms.bLockLobby)
            {
                SetLockLobby(false);
                BEGIN_PFM_CREATE(g.fm, pfmLockLobby, CS, LOCK_LOBBY)
                END_PFM_CREATE
                pfmLockLobby->fLock = false;
            }

            if (m_misdef.misparms.bLockSides)
            {
                SetLockSides(false);
                BEGIN_PFM_CREATE(g.fm, pfmLockSides, CS, LOCK_SIDES)
                END_PFM_CREATE
                pfmLockSides->fLock = false;
            }

			// mmf reset skill level on an empty game
			// Note using current values in skilllevels.mdl which could change
			// mmf revist
			// This works (when they try and connect) but it does not let the clients know things were reset so it does not update their gui
			// AFAIK there is no corresponding message we can send for this like there is above for LOCK_SIDES, etc
			// mmf don't reset skill level if there is a server skill level setting for this server (KGJV added this feature keying off registry)
			// mmf since there isn't an easy way to check for Min/MaxRank# entries in registry as we don't know the game number
			//     assume if game is locked open we don't want to change the min and max rank
			if (!(m_misdef.misparms.bLockGameOpen) && !m_misdef.misparms.bObjectModelCreated) {//#203 Turkey 6/11
				m_misdef.misparms.iMinRank = -1;
				m_misdef.misparms.iMaxRank = 1000;
			}
        }

        // No one's left in this side
        SetLeaderID(sideidOld, NA);
      }
    }

    if (shipidNewOwner != NA) // somebody's getting promoted
    {
		if (shipidNewOwner > 0) {
			//set command wing Imago 6/10
			BEGIN_PFM_CREATE(g.fm, pfmSetWingID, CS, SET_WINGID)
			END_PFM_CREATE
			pfmSetWingID->wingID = 0;
			pfmSetWingID->shipID = shipidNewOwner;
			pfmSetWingID->bCommanded = true;
			g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
			GetLeader(iSideNewOwner)->GetIGCShip()->SetWingID(0);
		}

	// announce new leader
      BEGIN_PFM_CREATE(g.fm, pfmSetTeamLeader, CS, SET_TEAM_LEADER)
      END_PFM_CREATE
      pfmSetTeamLeader->shipID = shipidNewOwner;
      pfmSetTeamLeader->sideID = iSideNewOwner;

      if (fMissionOwner)
      {
        BEGIN_PFM_CREATE(g.fm, pfmSetMissionOwner, CS, SET_MISSION_OWNER)
        END_PFM_CREATE
        pfmSetMissionOwner->shipID = shipidNewOwner;
        pfmSetMissionOwner->sideID = iSideNewOwner;
      }
    }
  }



  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

  Money payday;
  if (STAGE_STARTED == m_misdef.stage || STAGE_STARTING == m_misdef.stage)
  {
    //The player was on a side and the game was going
    if (pfsNewLeader && (pfsNewLeader->GetIGCShip()->GetAutoDonate() == pshipPlayer))
        pfsNewLeader->SetAutoDonate(NULL, 0, false);

    //Anyone who was autodonating to this player is no longer autodonating to this player
    for (ShipLinkIGC*   psl = psideOld->GetShips()->first(); (psl != NULL); psl = psl->next())
    {
        if ((psl->data()->GetAutoDonate() == pshipPlayer) &&
            ((pfsNewLeader == NULL) || (psl->data() != pfsNewLeader->GetIGCShip())))
        {
            ((CFSShip*)(psl->data()->GetPrivateData()))->GetPlayer()->SetAutoDonate(pfsNewLeader, 0, false);
        }
    }

    g.fm.SendMessages(CFSSide::FromIGC(psideOld)->GetGroup(), FM_GUARANTEED, FM_FLUSH);

    payday = pfsPlayer->GetMoney() + pfsPlayer->GetIGCShip()->GetValue();

    assert(STAGE_STARTING != m_misdef.stage || payday == 0);
  }
  else
    payday = 0;

  pfsPlayer->SetMoney(0);

  // announce the guy that we were called for. Make sure nothing got wacky
  BEGIN_PFM_CREATE(g.fm, pfmSideChange, CS, QUIT_SIDE)
    FM_VAR_PARM(szMessageParam, CB_ZTS)
  END_PFM_CREATE
  pfmSideChange->shipID = shipid;
  pfmSideChange->reason = reason;

  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

  pfsPlayer->Reset(false);
  if (STAGE_STARTED != m_misdef.stage)
      pfsPlayer->GetIGCShip()->SetExperience(1.0f);

  pfsPlayer->SetSide(this, pMission->GetSide(SIDE_TEAMLOBBY));
  // pfsPlayer->SetReady(true); Imago commented out so afk not reset
  GiveSideMoney(psideOld, payday);

  bool fDeactivate = (STAGE_NOTSTARTED != m_misdef.stage) && !HasPlayers(psideOld, false);
  if (fDeactivate)
    DeactivateSide(psideOld);             // deactivate the side _after_ the side change
  else
    CheckForSideAllReady(psideOld);       // its ready state may have changed

  // if the game has not started and the leader quit, make sure the squad memberships work
  if (bWasTeamLeader && STAGE_NOTSTARTED == m_misdef.stage && squadID != NA)
    MaintainSquadLeadership(sideidOld);

  if (bTurnOnAutoaccept)
    SetAutoAccept(psideOld, true);

  if (QSRIsBoot(reason) && (sideidOld >= 0))
  {
    unsigned char bannedSideMask = pfsPlayer->GetBannedSideMask() | SideMask(sideidOld);
    pfsPlayer->SetBannedSideMask(bannedSideMask);
  }
}


void CFSMission::AddInvitation(SideID sid, char * szPlayerName)
{
   if(sid >= c_cSidesMax)
     return;

   m_pFSSides[sid]->AddInvitation(szPlayerName);
}



/*-------------------------------------------------------------------------
 * IsInvited
 *-------------------------------------------------------------------------
 */
bool CFSMission::IsInvited(CFSPlayer * pPlayer)
{
    assert(RequiresInvitation());

    for (std::vector<CFSSide*>::iterator _i(m_pFSSides.begin()); _i != m_pFSSides.end(); ++_i)
    {
      if(static_cast<CFSSide*>(*_i)->IsInvited(pPlayer))
        return true;
    }
    return false;
}


/*-------------------------------------------------------------------------
 * AddBallot
 *-------------------------------------------------------------------------
 */
void CFSMission::AddBallot(Ballot * pBallot)
{
  m_ballots.PushEnd(pBallot);
}


/*-------------------------------------------------------------------------
 * TallyVote
 *-------------------------------------------------------------------------
 */
void CFSMission::TallyVote(CFSPlayer* pfsPlayer, BallotID ballotID, bool bVote)
{
  for (BallotList::Iterator iter(m_ballots); !iter.End(); iter.Next())
  {
    if (iter.Value()->GetBallotID() == ballotID)
    {
      iter.Value()->CastVote(pfsPlayer, bVote);
      break;
    }
  }
}

/*-------------------------------------------------------------------------
 * HasBallots  mmf/KGJV 09/07 allow only one ballot of each type at a time
 *-------------------------------------------------------------------------
 */
bool CFSMission::HasBallots(BallotType iType)
{
    for (BallotList::Iterator iter(m_ballots); !iter.End();)
    {
        if (iter.Value()->GetType() == iType) return true;
        iter.Next();
    }
   return false;
}

/*-------------------------------------------------------------------------
 * RemovePlayerByName
 *-------------------------------------------------------------------------
 * Purpose:
 *    Removes all players with the given character name from this mission,
 *    if they are a member.
 *
 * Parameters:
 *    character name to boot and the reason for booting them.
 *
 * Returns:
 *    true if a player was booted
 */
bool CFSMission::RemovePlayerByName(const char* szCharacterName, QuitSideReason reason, const char* szMessageParam)
{
  ShipLinkIGC * pShiplink = GetIGCMission()->GetShips()->first();

  while (pShiplink)
  {
    CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();

    if (pfsShip->IsPlayer())
    {
      CFSPlayer * pfsPlayer = pfsShip->GetPlayer();

      if (_stricmp(pfsPlayer->GetName(), szCharacterName) == 0)
      {
        RemovePlayerFromMission(pfsShip->GetPlayer(), reason, szMessageParam);
        debugf("Booted duplicate character %s from mission %x.\n",
            pfsPlayer->GetName(), GetCookie());
        break;
      }
    }

    pShiplink = pShiplink->next();
  }

  return pShiplink != NULL;
}


/*-------------------------------------------------------------------------
 * RemovePlayerByCDKey
 *-------------------------------------------------------------------------
 * Purpose:
 *    Removes all players with the given CD Key from this mission,
 *    if they are a member.
 *
 * Parameters:
 *    character CD Key to boot and the reason for booting them.
 *
 * Returns:
 *    true if a player was booted
 */
bool CFSMission::RemovePlayerByCDKey(const char* szCDKey, QuitSideReason reason, const char* szMessageParam)
{
  ShipLinkIGC * pShiplink = GetIGCMission()->GetShips()->first();

  while (pShiplink)
  {
    CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();

    if (pfsShip->IsPlayer())
    {
      CFSPlayer * pfsPlayer = pfsShip->GetPlayer();

      if (_stricmp(pfsPlayer->GetCDKey(), szCDKey) == 0)
      {
        RemovePlayerFromMission(pfsShip->GetPlayer(), reason, szMessageParam);
        debugf("Booted character %s with duplicate CD Key %s from mission %x.\n",
            pfsPlayer->GetName(), pfsPlayer->GetCDKey(), GetCookie());
        break;
      }
    }

    pShiplink = pShiplink->next();
  }

  return pShiplink != NULL;
}


/*-------------------------------------------------------------------------
 * GetCountOfPlayers
 *-------------------------------------------------------------------------
 * Purpose:
 *    Count up number of human players on side, or mission (when pside==NULL)
 *
 * Parameters:
 *    side to check, NULL for whole mission
 *
 * Returns:
 *    count of players
 */
int CFSMission::GetCountOfPlayers(IsideIGC * pside, bool bCountGhosts)
{
  int cPlayers = 0;
  const ShipListIGC * plistShip = pside ? pside->GetShips() : m_pMission->GetShips();
  for (ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
  {
    if (ISPLAYER(plinkShip->data()) && (bCountGhosts || !plinkShip->data()->IsGhost()))
      cPlayers++;
  }
  return cPlayers;
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
int CFSMission::GetSideRankSum(IsideIGC * pside, bool bCountGhosts)
{
  int iRankSum = 0;
  int iTempRank = 0;
  const ShipListIGC * plistShip = pside ? pside->GetShips() : m_pMission->GetShips();
  CFSPlayer* pfsPlayer = NULL;
  for (ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
  {
    if (ISPLAYER(plinkShip->data()) && (bCountGhosts || !plinkShip->data()->IsGhost()))
	{
		pfsPlayer = ((CFSShip*)(plinkShip->data()->GetPrivateData()))->GetPlayer();
		iTempRank = pfsPlayer->GetPersistPlayerScore(NA)->GetRank();
		iRankSum += (iTempRank < 1) ? 1 : iTempRank;
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
int CFSMission::GetRankThreshold()
{
	int iHighestRank = 1;
	int iAverageRank = 0;

	const ShipListIGC * plistShip = m_pMission->GetShips();
	CFSPlayer* pfsPlayer = NULL;
	int iTempRank = 0;
	for (ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
	{
		CFSShip* pfsShip = ((CFSShip *) (plinkShip->data())->GetPrivateData());
		if (pfsShip)
		{
			if (pfsShip->IsPlayer())
			{
				iTempRank = pfsShip->GetPlayer()->GetPersistPlayerScore(NA)->GetRank();
				if (iTempRank > iHighestRank)
					iHighestRank = iTempRank;

				iAverageRank += iTempRank;
			}
		}
	}

	iAverageRank = iAverageRank / plistShip->n();
	int iThreshold = iHighestRank + ((iAverageRank * iAverageRank) / (iHighestRank + iAverageRank));
	return iThreshold;
}

/*-------------------------------------------------------------------------
 * HasPlayers
 *-------------------------------------------------------------------------
 * Purpose:
 *    Like GetCountOfPlayers, but just determines whether there's anyone on
 *        a side, so slightly faster
 *
 * Parameters:
 *    side, or NULL for mission
 *
 * Returns:
 *    whether the side has any players
 */
bool CFSMission::HasPlayers(IsideIGC * pside, bool bCountGhosts)
{
  const ShipListIGC * plistShip = pside ? pside->GetShips() : m_pMission->GetShips();
  for (ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
  {
    if (ISPLAYER(plinkShip->data()) && (bCountGhosts || !plinkShip->data()->IsGhost()))
      return true;
  }
  return false;
}

IstationIGC * CFSMission::GetBase(IsideIGC * pside)
{
  if (pside)
  {
    for (StationLinkIGC*    psl = pside->GetStations()->first(); (psl != NULL); psl = psl->next())
    {
        IstationIGC*    pstation = psl->data();

        if (pstation->GetStationType()->HasCapability(c_sabmRestart))
            return pstation;
    }
  }

  return NULL;
}

//
// This probably should be in CFSSide
//
CFSPlayer * CFSMission::GetLeader(SideID sid)
{
  assert (NA != sid);
  if (sid == SIDE_TEAMLOBBY) // lobby side never has a leader
      return NULL;
  ShipID shipid = m_misdef.rgShipIDLeaders[sid];
  return NA == shipid ? NULL : CFSShip::GetShipFromID(shipid)->GetPlayer();
}

//
// This probably should be in CFSSide
//
void    CFSMission::SetLeaderID(SideID sideID, ShipID shipID)
{
    if (m_misdef.rgShipIDLeaders[sideID] != NA)
    {
        CFSShip*    pfsShip = CFSShip::GetShipFromID(m_misdef.rgShipIDLeaders[sideID]);
        assert (pfsShip);
        assert (pfsShip->IsPlayer());
        pfsShip->GetPlayerScoreObject()->StopCommand(m_pMission->GetLastUpdate());
    }

    m_misdef.rgShipIDLeaders[sideID] = shipID;

    if (shipID != NA)
    {
        CFSShip*    pfsShip = CFSShip::GetShipFromID(shipID);
        assert (pfsShip);
        assert (pfsShip->IsPlayer());
        pfsShip->GetPlayerScoreObject()->StartCommand(m_pMission->GetLastUpdate());
    }
}

void CFSMission::SetLeader(CFSPlayer * pfsPlayer)
{
  assert(pfsPlayer);

  SideID sid = pfsPlayer->GetSide()->GetObjectID();
  ShipID shipId = pfsPlayer->GetShipID();

  assert(0 <= sid && sid < m_misdef.misparms.nTeams);

  CFSPlayer * pfsOldLeader = GetLeader(sid);
  assert(pfsOldLeader);

  if (pfsOldLeader->GetIGCShip()->GetWingID() == 0)
  {
    //pfsOldLeader->GetIGCShip()->SetWingID(1);	// TE: Old commander goes to Attack wing //Imago #91 removed
	  /*
    BEGIN_PFM_CREATE(g.fm, pfmSetWingID, CS, SET_WINGID)
    END_PFM_CREATE
    pfmSetWingID->wingID = 1; // TE: Old comm goes to Attack wing  
    pfmSetWingID->shipID = pfsOldLeader->GetShipID();
    pfmSetWingID->bCommanded = true;
	*/
  }

  // put them on the command wing
  pfsPlayer->GetIGCShip()->SetWingID(0);

  BEGIN_PFM_CREATE(g.fm, pfmSetWingID, CS, SET_WINGID)
  END_PFM_CREATE
  pfmSetWingID->wingID = 0;
  pfmSetWingID->shipID = shipId;
  pfmSetWingID->bCommanded = true;
  g.fm.SendMessages(CFSSide::FromIGC(pfsPlayer->GetSide())->GetGroup(), FM_GUARANTEED, FM_FLUSH);

  // change leaders
  SetLeaderID(sid, shipId);

  BEGIN_PFM_CREATE(g.fm, pfmSetTeamLeader, CS, SET_TEAM_LEADER)
  END_PFM_CREATE
  pfmSetTeamLeader->shipID = shipId;
  pfmSetTeamLeader->sideID = sid;

  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
}

/*-------------------------------------------------------------------------
 * SetSideName
 *-------------------------------------------------------------------------
 * Purpose:
 *    Set the name of a side and notifies clients
 *
 * Parameters:
 *    The side whose name should changed and the new name
 */
void CFSMission::SetSideName(SideID sid, const char* szName)
{
  assert(sid >= 0 && sid < c_cSidesMax);
  assert(strlen(szName) < c_cbName);
  IsideIGC* pside = m_pMission->GetSide(sid);
  if (!pside)
    return;

  ZString strNameOld(m_misdef.rgszName[sid]);
  if (szName != m_misdef.rgszName[sid])
  {
    strncpy(m_misdef.rgszName[sid], szName, c_cbName - 1);
    m_misdef.rgszName[sid][c_cbName-1] = '\0';
    m_pMission->GetSide(sid)->SetName(m_misdef.rgszName[sid]);
  }

  // tell the clients about the new side info
  BEGIN_PFM_CREATE(g.fm, pfmSetTeamInfo, CS, SET_TEAM_INFO)
  END_PFM_CREATE
  pfmSetTeamInfo->sideID = sid;
  pfmSetTeamInfo->squadID = pside->GetSquadID();
  strcpy(pfmSetTeamInfo->SideName, m_misdef.rgszName[sid]);
  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

  // Fire an AGCEvent to indicate the Team info change
  LPCSTR pszGame = GetMissionDef()->misparms.strGameName;
  long idGame = GetIGCMission() ? GetIGCMission()->GetMissionID() : -1;
  long idTeam = GetIGCMission()->GetSide(sid)->GetUniqueID();

  LPCSTR pszContext = GetIGCMission() ? GetIGCMission()->GetContextName() : NULL;

  _AGCModule.TriggerContextEvent(NULL, EventID_TeamInfoChange, pszContext,
    strNameOld, idTeam, -1, -1, 3,
    "GameID"     , VT_I4   , idGame,
    "GameName"   , VT_LPSTR, pszGame,
    "NewTeamName", VT_LPSTR, szName);
};

/*-------------------------------------------------------------------------
 * SetSideSquad
 *-------------------------------------------------------------------------
 * Purpose:
 *    Set the squad for a side, booting players who are not on that squad,
 *    changing the team name, and notifing players of the team's new squad.
 *
 * Parameters:
 *    The side whose squad should changed and the id of the new squad
 */
void CFSMission::SetSideSquad(SideID sid, SquadID squadID)
{
  assert(sid >= 0 && sid < c_cSidesMax);
  IsideIGC* pside = m_pMission->GetSide(sid);

  // if the squad doesn't need to be changed, don't touch it.
  if (squadID == pside->GetSquadID())
    return;

  pside->SetSquadID(squadID);

  // loop through the players and boot anyone who is not a member of this squad
  if (squadID != NA)
  {
    ShipLinkIGC*   pslNext;
    for (ShipLinkIGC* psl = pside->GetShips()->first(); (psl != NULL); psl = pslNext)
    {
      pslNext = psl->next();

      CFSShip* pcs = (CFSShip *)(psl->data()->GetPrivateData());
      if (pcs->IsPlayer())
      {
        CFSPlayer* pfsPlayer = pcs->GetPlayer();
        if (!pfsPlayer->GetIsMemberOfSquad(squadID))
        {
          RemovePlayerFromSide(pfsPlayer, QSR_SquadChange);
        }
      }
    }
  }

  const char* szSquadName = NULL;

  if (m_misdef.misparms.bLockTeamSettings)
  {
      // don't change the name
      szSquadName = m_misdef.rgszName[sid];
  }
  else
  {
      CFSPlayer* pfsLeader = GetLeader(sid);
      if (squadID != NA)
      {
        assert(pfsLeader);
        for (SquadMembershipLink* pSquadLink = pfsLeader->GetSquadMembershipList()->first();
          pSquadLink != NULL; pSquadLink = pSquadLink->next())
        {
          if (pSquadLink->data()->GetID() == squadID)
            szSquadName = pSquadLink->data()->GetName();
        }
      }
      assert((squadID == NA) == (szSquadName == NULL));
  }

  // use SetSideName to send the new squadID along with the new name
  SetSideName(sid, szSquadName ? szSquadName : sideNames[sid]);

  SetLobbyIsDirty();
};

/*-------------------------------------------------------------------------
 * MaintainSquadLeadership
 *-------------------------------------------------------------------------
 * Purpose:
 * Makes sure the team has a squand and the current leader can lead the team
 * with the current squad.  If not, it tries to fix this first by assigning
 * a new squad and possibly a new leader.
 *
 * Parameters:
 *    The side to check
 */
void CFSMission::MaintainSquadLeadership(SideID sid)
{
  IsideIGC* pside = m_pMission->GetSide(sid);
  SquadID squadID = pside->GetSquadID();
  CFSPlayer* pfsLeader = GetLeader(sid);

  // don't try anything if the team settings are locked.
  if (m_misdef.misparms.bLockTeamSettings)
    return;

  if (pfsLeader && (squadID == NA || !pfsLeader->GetCanLeadSquad(squadID)))
  {
    // see which squad the current leader would prefer to lead
    squadID = pfsLeader->GetPreferredSquadToLead();

    // failing that, try to find a new leader
    if (squadID == NA)
    {
      ShipLinkIGC*   pslNext;
      for (ShipLinkIGC* psl = pside->GetShips()->first(); psl != NULL && squadID == NA; psl = pslNext)
      {
        pslNext = psl->next();

        CFSShip* pcs = (CFSShip *)(psl->data()->GetPrivateData());
        if (pcs->IsPlayer())
        {
          CFSPlayer* pfsPlayer = pcs->GetPlayer();
          squadID = pfsPlayer->GetPreferredSquadToLead();
          pfsLeader = pfsPlayer;
        }

        if (squadID != NA)
        {
          // switch to the new leader who is qualified to lead
          SetLeader(pfsLeader);
        }
      }
    }

    // set the team to its new squad
    SetSideSquad(sid, squadID);

    // if we couldn't find a squad leader, boot everyone on the team
    if (squadID == NA)
    {
      ShipLinkIGC*   pslNext;
      for (ShipLinkIGC* psl = pside->GetShips()->first(); psl != NULL && squadID == NA; psl = pslNext)
      {
        pslNext = psl->next();

        CFSShip* pcs = (CFSShip *)(psl->data()->GetPrivateData());
        if (pcs->IsPlayer())
          RemovePlayerFromSide(pcs->GetPlayer(), QSR_SquadChange);
      }
    }
  }
}

/*-------------------------------------------------------------------------
 * SetStage
 *-------------------------------------------------------------------------
 * Purpose:
 *    Tell the mission what stage it's in, and tell the clients
 *
 * Parameters:
 *    new stage
 */
void CFSMission::SetStage(STAGE stage)
{
  if (m_misdef.stage != stage)
  {
    assert(m_misdef.stage < stage || stage == STAGE_NOTSTARTED); // it's a one-way road (sort of)
    debugf("SetStage, stage=%d, mission=%x\n", stage, GetCookie());
    m_misdef.stage = stage;
    m_misdef.fInProgress = stage == STAGE_STARTED;

    BEGIN_PFM_CREATE(g.fm, pfmMissionStage, S, MISSION_STAGE)
    END_PFM_CREATE
    pfmMissionStage->stage = GetStage();
    g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

    m_pMission->SetMissionStage(stage);

    SetLobbyIsDirty();
  }
}


/*-------------------------------------------------------------------------
 * GiveSideMoney
 *-------------------------------------------------------------------------
 * Purpose:
 *    Give a side some money, divided equally among all players
 */
void CFSMission::GiveSideMoney(IsideIGC * pside, Money money)
{
  assert (money >= 0);
  if (money != 0)
  {
      SideID sideID = pside->GetObjectID();
      m_rgMoney[sideID] += money;

      DoPayday(pside);
  }
}


/*-------------------------------------------------------------------------
 * SetMissionParams
 *-------------------------------------------------------------------------
 * Purpose:
 *    Called when a client changes the mission parameters
 */
void CFSMission::SetMissionParams(const MissionParams & misparmsNew)
{
  MissionParams misparms = misparmsNew;
  assert(GetStage() == STAGE_NOTSTARTED);
  assert(misparms.Invalid() == NULL);

  // don't even think about changing the IGC static stuff.
  misparms.verIGCcore = m_misdef.misparms.verIGCcore;
  strncpy(misparms.szIGCStaticFile, m_misdef.misparms.szIGCStaticFile, c_cbFileName);

  // make sure it's properly marked as a club or non-club game too.
  #if !defined(ALLSRV_STANDALONE)
    misparms.bClubGame = true;
  #else // !defined(ALLSRV_STANDALONE)
    misparms.bClubGame = false;
  #endif // !defined(ALLSRV_STANDALONE)

	// BT - STEAM - Scores always count in steam!
  // TE: Enforce LockSides = on if ScoresCount mmf change to MaxImbalance
  //if (misparms.bScoresCount) 
	 // misparms.iMaxImbalance = 0x7ffe;

  int numTeamsOld = m_misdef.misparms.nTeams;

  if (numTeamsOld > misparms.nTeams)
  {
    // destroy the extra teams
    for (SideID sideID = numTeamsOld - 1; sideID >= misparms.nTeams; sideID--)
    {
      // kick everyone off of the extra team
      IsideIGC* pside = m_pMission->GetSide(sideID);
      while (pside->GetShips()->first())
      {
        CFSShip* pship = (CFSShip*)(pside->GetShips()->first()->data()->GetPrivateData());
        assert(pship->IsPlayer());
        RemovePlayerFromSide(
          pship->GetPlayer(),
          QSR_SideDestroyed
          );
      }
    }
  }

  bool bSquadChange = m_misdef.misparms.bSquadGame != misparms.bSquadGame;

  m_misdef.misparms = misparms;

  // set Mission Name here too so that AGC Admin tool reports the params while mission stage is STAGE_NOTSTARTED
  m_pMission->SetMissionParams(&misparms);

  if (numTeamsOld < misparms.nTeams)
  {
    // add any new teams
    for (SideID sideID = numTeamsOld; sideID < misparms.nTeams; sideID++)
    {
      InitSide(sideID);
    }
  }

  m_pMission->UpdateSides(Time::Now(), &m_misdef.misparms, m_misdef.rgszName);
  UpdateAlliances(NA,NA); // #ALLY - some teams may have been removed, revalidate the alliances

  // see if that changed the ready state of anyone.
  for (SideLinkIGC* l = m_pMission->GetSides()->first(); l; l = l->next())
  {
    CheckForSideAllReady(l->data());
  }

  // boot any players who are no longer the right rank to play in this game
  {
    const ShipListIGC*      pships = m_pMission->GetShips();
    ShipLinkIGC* pshipLinkNext;
    for (ShipLinkIGC* pshipLink = pships->first();
         (pshipLink != NULL);
         pshipLink = pshipLinkNext)
    {
      pshipLinkNext = pshipLink->next();
      CFSPlayer* pfsPlayer = ((CFSShip*)(pshipLink->data()->GetPrivateData()))->GetPlayer();

      RankID rank = pfsPlayer->GetPersistPlayerScore(NA)->GetRank();
	  // mmf also added check here for special players
	  if ((misparms.iMinRank > rank || misparms.iMaxRank < rank) && !pfsPlayer->CanCheat() && !UTL::PrivilegedUser(pfsPlayer->GetName(),-1)) //Imago 6/10
      {
        RemovePlayerFromMission(pfsPlayer, QSR_RankLimits);
      }
    }
  }

  // if the squad setting has changed, update the new squad state
  if (bSquadChange)
  {
    for (SideID sideID = 0; sideID < misparms.nTeams; ++sideID)
    {
      if (misparms.bSquadGame)
        MaintainSquadLeadership(sideID);
      else
        SetSideSquad(sideID, NA);
    }
  }
}


/*-------------------------------------------------------------------------
 * ChangeCountdown
 *-------------------------------------------------------------------------
 * Purpose:
 *    Called when we want to change the countdown to starting the game
 */
void CFSMission::DelayCountdown(float fDelayLength)
{
  if (GetStage() == STAGE_NOTSTARTED || GetStage() == STAGE_STARTING)
  {
      // delay the game start time as appropriate.
      m_misdef.misparms.timeStart += fDelayLength;

      // forward the new start time to everyone
      BEGIN_PFM_CREATE(g.fm, pfmStartTime, S, SET_START_TIME)
      END_PFM_CREATE
      pfmStartTime->timeStart = m_misdef.misparms.timeStart;
      g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
  }
}


/*-------------------------------------------------------------------------
 * StartCountdown
 *-------------------------------------------------------------------------
 * Purpose:
 *    Called when we want to start the countdown to starting the game
 */
void CFSMission::StartCountdown(float fCountdownLength)
{
  if (GetStage() == STAGE_NOTSTARTED || GetStage() == STAGE_STARTING)
  {
      // set (or reset) the game start time as appropriate.
      m_misdef.misparms.timeStart = Time::Now() + fCountdownLength;

      // forward the new start time to everyone
      BEGIN_PFM_CREATE(g.fm, pfmStartTime, S, SET_START_TIME)
      END_PFM_CREATE
      pfmStartTime->timeStart = m_misdef.misparms.timeStart;
      g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
  }

  if (GetStage() == STAGE_NOTSTARTED)
  {
    // create the mission
    {

      for (SideLinkIGC* l = m_pMission->GetSides()->first(); l; l = l->next())
      {
          IsideIGC* pside = l->data();
          if (m_misdef.misparms.bAllowDevelopments)
              pside->SetGlobalAttributeSet(pside->GetCivilization()->GetBaseAttributes());
          else
              pside->ResetGlobalAttributeSet();
      }
    }
    m_pMission->GenerateMission(Time::Now(), &m_misdef.misparms, m_pttbmAltered, m_pttbmNewSetting);

    // tell each client their starting sector so they can display the mission briefing
    {
      for (SideLinkIGC* l = m_pMission->GetSides()->first(); l; l = l->next())
      {
        IsideIGC * pside = l->data();

        // tell them their starting sector so they can display the mission briefing
        bool bGenerateCivBriefing = m_strStoryText.IsEmpty();
        const char* szBriefingText =  bGenerateCivBriefing
            ? GetBase(pside)->GetCluster()->GetName() : m_strStoryText;
        BEGIN_PFM_CREATE(g.fm, pfmSetBriefingText, S, SET_BRIEFING_TEXT)
          FM_VAR_PARM(szBriefingText, CB_ZTS)
        END_PFM_CREATE
        pfmSetBriefingText->fGenerateCivBriefing = bGenerateCivBriefing;

        g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
      }
    }

    // let the clients know that the game is starting
    SetStage(STAGE_STARTING);

    // create the dplay groups for the clusters
    {
      const ClusterListIGC * pclist = m_pMission->GetClusters();
      for (ClusterLinkIGC * l = pclist->first(); (l != NULL); l = l->next())
      {
          IclusterIGC * pcluster = l->data();
          CreateDPGroups(pcluster);
      }
    }

    // set up each of the sides
    {
      const MissionParams*  pmp = m_pMission->GetMissionParams();

      float moneyStart = m_pMission->GetFloatConstant(c_fcidStartingMoney);

      for (SideLinkIGC* l = m_pMission->GetSides()->first(); l; l = l->next())
      {
        IsideIGC * pside = l->data();
        CFSPlayer* pfsLeader = GetLeader(pside->GetObjectID());

        float   moneyMultiplier = pmp->fStartingMoney + pside->GetCivilization()->GetBonusMoney();
        m_rgMoney[pside->GetObjectID()] = moneyMultiplier > 0.0f
                                          ? ((Money)(moneyStart * moneyMultiplier))
                                          : ((Money)0);

        //Set the leader to NULL first (so he is a legitimate autodonate target)
        if (pfsLeader)
            pfsLeader->SetAutoDonate(NULL, 0, false);

        // set up the team's money, and set up everyone to autodonate to the team leader
        const ShipListIGC*      pships = pside->GetShips();
        for (ShipLinkIGC* pshipLink = pships->first();
             (pshipLink != NULL);
             pshipLink = pshipLink->next())
        {
          CFSShip* pfsShip = ((CFSShip*)(pshipLink->data()->GetPrivateData()));
          pfsShip->SetMoney(0);
          pfsShip->GetPlayer()->SetTreasureObjectID(NA);

          // w0dk4 join-drop bug fix (allow more time when joining)
		  // w0dk4 - set join parameter here
		  pfsShip->GetPlayer()->SetJustJoined(true);

          if ((pfsShip != pfsLeader) && pfsShip->IsPlayer())
              pfsShip->GetPlayer()->SetAutoDonate(pfsLeader, 0, false);
        }

        g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);

        if (HasPlayers(pside, true))
        {
          // tell them about the world
          SendMissionInfo(NULL, pside);
        }
        else
        {
          // don't penalize a squad who is not in the game
          if (pside->GetSquadID() != NA)
            SetSideSquad(pside->GetObjectID(), NA);
          DeactivateSide(pside);
        }

        const StationListIGC*   pstations = pside->GetStations();
        assert (pstations->n() >= 1);

        const int       c_stationsMax = 20;
        IstationIGC*    stations[c_stationsMax];

        int             nStations = 0;
        {
            for (StationLinkIGC*    psl = pstations->first(); (psl != NULL); psl = psl->next())
            {
                IstationIGC*    pstation = psl->data();
                if (pstation->GetStationType()->HasCapability(c_sabmRestart))
                {
                    assert (nStations < c_stationsMax);
                    stations[nStations++] = pstation;
                }
            }
        }
        assert (nStations >= 1);

        IstationIGC*            pstation = stations[0];
        assert (pstation);
        if (nStations == 1)
        {
            //Start all ships at that station
			// mdvalley: 2005 needs pshipLink defined
            for (ShipLinkIGC* pshipLink = pships->first();
                 (pshipLink != NULL);
                 pshipLink = pshipLink->next())
            {
              pshipLink->data()->SetStation(pstation);
              ((CFSShip*)(pshipLink->data()->GetPrivateData()))->ShipStatusStart(pstation);
            }
        }
        else
		{
            //Multiple stations for a side ... try to divy things up evenly without breaking wings up

            //We need at least this many/station,
            int     minPerStation = pships->n() / (2 * nStations);
            //and can't have more than this (assuming all the other stations have the min)
            int     maxPerStation = pships->n() - minPerStation * (nStations - 1);

            //Count the number of players on each wing
            int     players[c_widMax] = {0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0};

            int     assignment[c_widMax] = {-2, -2, -2, -2, -2,
                                            -2, -2, -2, -2, -2};

            int     unassignedWings = 0;

            {
				// mdvalley: gotta define it each time now
                for (ShipLinkIGC* pshipLink = pships->first();
                     (pshipLink != NULL);
                     pshipLink = pshipLink->next())
                {
                    IshipIGC*   pship = pshipLink->data();
                    WingID      wid = pship->GetWingID();
                    assert (wid >= 0);
                    assert (wid < c_widMax);

                    if (players[wid]++ == 0)
                        unassignedWings++;
                }
            }

            int       population[c_stationsMax];
            {
                for (int i = 0; (i < nStations); i++)
                    population[i] = 0;
            }

            while (unassignedWings-- > 0)
            {
                //Find the largest unassigned wing
                int widMax = -1;
                {
                    for (int i = 0; (i < c_widMax); i++)
                    {
                        if ((assignment[i] == -2) && ((widMax == -1) || (players[i] >= players[widMax])))
                            widMax = i;
                    }
                }
                assert (widMax != -1);

                //Find the least populated station
                int stationMin = 0;
                {
                    for (int i = 1; (i < nStations); i++)
                    {
                        if (population[i] < population[stationMin])
                            stationMin = i;
                    }
                }

                if (population[stationMin] + players[widMax] <= maxPerStation)
                {
                    population[stationMin] += players[widMax];
                    assignment[widMax] = stationMin;
                }
                else
                    assignment[widMax] = -1;
            }

            {
				// mdvalley: define it again!
                for (ShipLinkIGC* pshipLink = pships->first();
                     (pshipLink != NULL);
                     pshipLink = pshipLink->next())
                {
                    IshipIGC*   pship = pshipLink->data();
                    WingID      wid = pship->GetWingID();
                    assert (wid >= 0);
                    assert (wid < c_widMax);

                    int   stationMin;
                    if (assignment[wid] == -1)
                    {
                        stationMin = 0;
                        {
                            for (int i = 1; (i < nStations); i++)
                            {
                                if (population[i] < population[stationMin])
                                    stationMin = i;
                            }
                        }

                        population[stationMin]++;
                    }
                    else
                        stationMin = assignment[wid];

                    assert (stationMin >= 0);
                    IstationIGC*    pstation = stations[stationMin];

                    pshipLink->data()->SetStation(pstation);
                    ((CFSShip*)(pshipLink->data()->GetPrivateData()))->ShipStatusStart(pstation);
                }
            }
        }


		// KGJV #62 - Eliminate all of the inactive teams bases if bAllowEmptyTeams
		if (pmp->bAllowEmptyTeams && !pside->GetActiveF())
		{
		  while (StationLinkIGC* psl = pside->GetStations()->first())
		  {
			  IstationIGC*   pstation = psl->data();
			  debugf("removing station %s, cause bAllowEmptyTeams & Inactive side\n",pstation->GetName());
			  m_psiteIGC->KillStationEvent(pstation,NULL,0);
		  }
		}

		else if (pmp->bAllowDevelopments)
		{
            //Create miners for each side in the sector with their home starbase
			if (pmp->bAllowEmptyTeams ? pside->GetActiveF() : true) // KGJV #62 only for active sides when empty teams allowed
            for (DroneTypeLinkIGC* pdl = m_pMission->GetDroneTypes()->first(); (pdl != NULL); pdl = pdl->next())
            {
              IdroneTypeIGC*  pdt = pdl->data();
              if ((pdt->GetPilotType() == c_ptMiner) && pstation->CanBuy(pdt))
              {
                for (int nMiner = 0; nMiner < m_misdef.misparms.nInitialMinersPerTeam; nMiner++)
                {
                  CFSDrone * pfsDrone = CreateStockDrone(pdt, pside);

                  if (pfsDrone)
                  {
                      pfsDrone->GetIGCShip()->SetStation(pstation);  //start them at a station just like everyone else
                  }
                }
                break;
              }
            }
        }
      }
    }
  }
}

/*-------------------------------------------------------------------------
 * StartGame
 *-------------------------------------------------------------------------
 * Purpose:
 *    Called when we're ready to set the ships loose and actually start flying around
 */
void CFSMission::StartGame()
{
  if (GetStage() == STAGE_NOTSTARTED)
  {
      // transition through the STAGE_STARTING state for consistancy
      StartCountdown(0.0f);
  }

  if (GetStage() == STAGE_STARTING)
  {
    SetStage(STAGE_STARTED);
    {
        //Connect all players score objects to their side score objects
        for (ShipLinkIGC*   psl = m_pMission->GetShips()->first(); (psl != NULL); psl = psl->next())
        {
            CFSShip*    pfsship = (CFSShip*)(psl->data()->GetPrivateData());
            IsideIGC*   pside = psl->data()->GetSide();
            if (pside->GetObjectID() >= 0)
            {
                pfsship->GetPlayerScoreObject()->Connect(g.timeNow);
            }
        }
    }

    m_timeNextPayday = g.timeNow + m_pMission->GetFloatConstant(c_fcidSecondsBetweenPaydays);

    m_psideWon = NULL;
    m_pszReason = NULL;
    m_bDraw = false;
    m_pMission->EnterGame();

	//Clear out old players to avoid them rejoining the game if they have dropped from a team
	m_oldPlayers.purge();

    // let everyone know that the game has started
    BEGIN_PFM_CREATE(g.fm, pfmEnterGame, S, ENTER_GAME)
    END_PFM_CREATE
    g.fm.SendMessages(GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

    LPCSTR pszContext = GetIGCMission() ? GetIGCMission()->GetContextName() : NULL;

    GetSite()->SendChat(NULL, CHAT_EVERYONE, NA, NA, "The game has started.");
    // TE, Modify GameStarted AGCEvent to include MissionID.
    //Imago / Sgt_Baker: Include alliances info: <allyid>:<team ids>,<allyid>:<team ids>...  NYI 7/10/09
    	//Note:  Alliances may change in-game (NYI), create your own AGC Event! AllsrvEventID_AlliancesChanged
	_AGCModule.TriggerContextEvent(NULL, AllsrvEventID_GameStarted, pszContext,
		GetIGCMission()->GetMissionParams()->strGameName, GetMissionID(), -1, -1, 0);
    // Changed "" and -1 to MissionName and MissionID
	// _AGCModule.TriggerContextEvent(NULL, AllsrvEventID_GameStarted, pszContext, "", -1, -1, -1, 0);
    DoPayday();
  }
}

void CFSMission::DoPayday(void)
{
    //Paydays for the various sides
    const SideListIGC*  psides = GetIGCMission()->GetSides();
    for (SideLinkIGC*   l = psides->first();
         (l != NULL);
         l = l->next())
    {
        DoPayday(l->data());
    }
}

void CFSMission::DoPayday(IsideIGC* pside)
{
    SideID  sideID = pside->GetObjectID();

    int np = GetCountOfPlayers(pside, true);
    if (np != 0)
    {
        int delta = m_rgMoney[sideID] / np;
        if (delta > 0)
        {
            BEGIN_PFM_CREATE(g.fm, pfmPayday, S, PAYDAY)
            END_PFM_CREATE
            pfmPayday->dMoney = Money(delta);
            g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);

            m_rgMoney[sideID] -= delta * np;

            const ShipListIGC * plistShip = pside->GetShips();
            for(ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
            {
                CFSShip * pfsShip = (CFSShip *) plinkShip->data()->GetPrivateData();
                if (pfsShip->IsPlayer())
                {
                    CFSPlayer*  pfsDonate = pfsShip->GetPlayer()->GetAutoDonate();
                    if (pfsDonate)
                        pfsDonate->SetMoney(pfsDonate->GetMoney() + delta);
                    else
                        pfsShip->SetMoney(pfsShip->GetMoney() + delta);
                }
            }
        }
    }
}

void CFSMission::DoTick(Time timeNow)
{
    assert (GetStage() == STAGE_STARTED);

    ImissionIGC*            pm = GetIGCMission();
    const MissionParams*    pmp = pm->GetMissionParams();

    // check for the timeout winning condition
    {
        if (pmp->IsCountdownGame())
        {
            if ((timeNow - pmp->timeStart) > pmp->GetCountDownTime())
            {
                //End the game ... side with the most kills wins (ties -> fewest deaths, ties -> most money)
                IsideIGC*   psideWin = NULL;

                const char* pszClue;

                if (pmp->IsConquestGame())
                {
                    pszClue = "conquest";

                    int maxFlags = 0;
                    for (SideLinkIGC*   psl = pm->GetSides()->first(); (psl != NULL); psl = psl->next())
                    {
                        IsideIGC*   pside = psl->data();
                        int nFlags = 0;
                        for (StationLinkIGC*    l = pside->GetStations()->first(); (l != NULL); l = l->next())
                        {
                            if (l->data()->GetBaseStationType()->HasCapability(c_sabmFlag))
                                nFlags++;
                        }

                        if (nFlags > maxFlags)
                        {
                            maxFlags = nFlags;
                            psideWin = pside;
                        }
                        else if (nFlags == maxFlags)
                        {
                            psideWin = NULL;
                        }
                    }
                }

                if ((psideWin == NULL) && pmp->IsTerritoryGame())
                {
                    pszClue = "territory";

                    unsigned char maxSectors = 0;
                    for (SideLinkIGC*   psl = pm->GetSides()->first(); (psl != NULL); psl = psl->next())
                    {
                        IsideIGC*   pside = psl->data();
                        int nSectors = pside->GetTerritoryCount();

                        if (nSectors > maxSectors)
                        {
                            maxSectors = nSectors;
                            psideWin = pside;
                        }
                        else if (nSectors == maxSectors)
                        {
                            psideWin = NULL;
                        }
                    }
                }

                if ((psideWin == NULL) && pmp->IsProsperityGame())
                {
                    pszClue = "prosperity";

                    DWORD maxTime = 0;
                    for (SideLinkIGC*   psl = pm->GetSides()->first(); (psl != NULL); psl = psl->next())
                    {
                        IsideIGC*   pside = psl->data();

                        IbucketIGC* pbucket = pside->GetBuckets()->last()->data();
                        assert ((pbucket->GetBucketType() == OT_development) &&
                                (pbucket->GetBuyable()->GetObjectID() == c_didTeamMoney));

                        DWORD   time = pbucket->GetTime();

                        if (time > maxTime)
                        {
                            maxTime = time;
                            psideWin = pside;
                        }
                        else if (time == maxTime)
                        {
                            psideWin = NULL;
                        }
                    }
                }

                if ((psideWin == NULL) && pmp->IsArtifactsGame())
                {
                    pszClue = "artifacts";

                    short   maxFlags = 0;
                    for (SideLinkIGC*   psl = pm->GetSides()->first(); (psl != NULL); psl = psl->next())
                    {
                        IsideIGC*   pside = psl->data();

                        short   nFlags = pside->GetArtifacts();

                        if (nFlags > maxFlags)
                        {
                            maxFlags = nFlags;
                            psideWin = pside;
                        }
                        else if (nFlags == maxFlags)
                        {
                            psideWin = NULL;
                        }
                    }
                }

                if ((psideWin == NULL) && pmp->IsFlagsGame())
                {
                    pszClue = "flags";

                    short   maxFlags = 0;
                    for (SideLinkIGC*   psl = pm->GetSides()->first(); (psl != NULL); psl = psl->next())
                    {
                        IsideIGC*   pside = psl->data();

                        short   nFlags = pside->GetFlags();

                        if (nFlags > maxFlags)
                        {
                            maxFlags = nFlags;
                            psideWin = pside;
                        }
                        else if (nFlags == maxFlags)
                        {
                            psideWin = NULL;
                        }
                    }
                }

                if (psideWin == NULL)
                {
                    pszClue = "kills";

                    int     nKills = -1;
                    int     nDeaths = INT_MAX;
                    for (SideLinkIGC*   psl = pm->GetSides()->first(); (psl != NULL); psl = psl->next())
                    {
                        IsideIGC*   pside = psl->data();
                        int         k = pside->GetKills();
                        int         d = pside->GetDeaths();

                        if ((k > nKills) ||
                            ((k == nKills) && (d < nDeaths)))
                        {
                            nKills = k;
                            nDeaths = d;
                            psideWin = pside;
                        }
                        else if ((k == nKills) && (d == nDeaths))
                        {
                            psideWin = NULL;
                        }
                    }
                }

                if (psideWin)
                {
                    static char szReason[100];
                    sprintf(szReason, "The clock has stopped. The winner: %s by %s", psideWin->GetName(), pszClue);
                    GameOver(psideWin, szReason);
                }
                else
                {
                    GameOver(NULL, "The clock has stopped. The game is a draw.");
                }
            }
        }
    }

    if (m_psideWon || m_bDraw)
    {
      ProcessGameOver();
      return;
    }

    if (timeNow >= m_timeNextPayday)
    {
        float dtPayday = m_pMission->GetFloatConstant(c_fcidSecondsBetweenPaydays);
        float dt = dtPayday + (timeNow - m_timeNextPayday);

        m_timeNextPayday = m_timeNextPayday + dtPayday;
        if (pmp->bAllowDevelopments)
        {
            //Get the average pot size for all teams
            float moneyIncome = m_pMission->GetFloatConstant(c_fcidIncome);

            const SideListIGC*  psides = GetIGCMission()->GetSides();
            {
                for (SideLinkIGC*   l = psides->first();
                     (l != NULL);
                     l = l->next())
                {
                    IsideIGC*   pside = l->data();

                    SideID      sideID = pside->GetObjectID();

                    //Calculate side income
                    m_rgMoney[sideID] += (Money)(moneyIncome * (1.0f + pside->GetCivilization()->GetIncomeMoney()));

                    Money   income = 0;
                    for (StationLinkIGC*    psl = pside->GetStations()->first();
                         (psl != NULL);
                         psl = psl->next())
                    {
                        income += psl->data()->GetStationType()->GetIncome();
                    }

                    m_rgMoney[sideID] += income;
                }
            }

            //Paydays for the various sides
            DoPayday();
        }

        {
            //Newly created objects for the various sectors

            for (ClusterLinkIGC*   l = GetIGCMission()->GetClusters()->first();
                 (l != NULL);
                 l = l->next())
            {
                IclusterIGC*   pcluster = l->data();
                bool    bHome = pcluster->GetHomeSector();
                float   treasures = pcluster->GetPendingTreasures() +
                                    dt * (bHome
                                          ? pmp->nPlayerSectorTreasureRate
                                          : pmp->nNeutralSectorTreasureRate);
                if (treasures >= 1.0f)
                {
                    //Create the appropriate treasures for the
                    int n = int(treasures);
                    treasures -= float(n);

                    short   tsi = bHome ? pmp->tsiPlayerRegenerate : pmp->tsiNeutralRegenerate;
                    for (int i = n; (i > 0); i--)
                    {
                        GetIGCMission()->GenerateTreasure(timeNow, pcluster, tsi);
                    }
                }

                pcluster->SetPendingTreasures(treasures);
            }
        }
    }

    // update any pending ballots
    for (BallotList::Iterator iter(m_ballots); !iter.End();)
    {
      if (iter.Value()->Update(timeNow))
        iter.Remove();
      else
        iter.Next();
    }
}



/*-------------------------------------------------------------------------
 * Vacate
 *-------------------------------------------------------------------------
   Purpose:
      Kicks everyone out
      Moves all players to lifepods in the null cluster and terminates drones
 */
void CFSMission::Vacate(void)
{
  const ShipListIGC * pShips = m_pMission->GetShips();
  CFSShip * pfsShip = NULL;
  ShipLinkIGC * pShiplink = NULL;

  pShiplink = pShips->first();
  while (pShiplink)
  {
    CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
    pShiplink = pShiplink->next();
    if (!pfsShip->IsPlayer())
      delete pfsShip->GetDrone();
    else
    {
      pfsShip->Reset(true);
    }
  }
}

void CFSMission::SaveAsOldPlayer(CFSPlayer* pfsplayer, bool bBooted)
{
  OldPlayerLink*  popl = GetOldPlayerLink(pfsplayer->GetName());

  if (!popl)
  {
    popl = new OldPlayerLink;
    m_oldPlayers.last(popl);
  }

  OldPlayerInfo&  opi = popl->data();

  strcpy(opi.name, pfsplayer->GetName());

  PlayerScoreObject*  ppso = pfsplayer->GetPlayerScoreObject();
  opi.pso = *ppso;
  opi.sideID = pfsplayer->GetSide()->GetObjectID();

  // mdvalley: save side flown for
  opi.lastSide = pfsplayer->GetLastSide();

  if ((GetStage() != STAGE_STARTED) ||
      (m_pMission->GetMissionParams()->bAllowDefections))
  {
    opi.bannedSideMask = pfsplayer->GetBannedSideMask();
    if (bBooted)
    {
      if (opi.sideID < 0)
      {
        opi.bannedSideMask = 0xff;
      }
      else
      {
        opi.bannedSideMask |= SideMask(opi.sideID);
      }
    }
  }
  else
  {
    //No defections
    if (bBooted)
      opi.bannedSideMask = 0xff;
    else if (opi.sideID >= 0)
      opi.bannedSideMask = ~SideMask(opi.sideID); // #ALLYTD: kgvj - remove allied teams from mask?  IMAGO turned off defections 7/8/09
    else
      opi.bannedSideMask = pfsplayer->GetBannedSideMask();
  }

  opi.characterID = pfsplayer->GetCharacterID();

  IclusterIGC*    pcluster = pfsplayer->GetIGCShip()->GetCluster();
  if (pcluster)
  {
    opi.pclusterLifepod = pcluster;
    opi.positionLifepod = pfsplayer->GetIGCShip()->GetSourceShip()->GetPosition();
  }
}

/*-------------------------------------------------------------------------
 * GameOver
 *-------------------------------------------------------------------------
   Purpose:
      Let someone forcibly end the game, even though no victory condition was met

   Parameters:
      The winning side
 */
void CFSMission::GameOver(IsideIGC * psideWin, const char* pszReason)
{
  m_psideWon = psideWin;
  m_pszReason = pszReason;
  m_bDraw = m_psideWon == NULL;

  // TE: Safely retrieve the team's ID and name
  const ObjectID iTeamObjectID = (psideWin) ? psideWin->GetObjectID() : -1;
  const char* pszTeamName = (psideWin) ? psideWin->GetName() : "";

  LPCSTR pszContext = GetIGCMission() ? GetIGCMission()->GetContextName() : NULL;

  // the game will actually end when we get around to checking whether a team has won
  // TE, Modify GameEnded AGCEvent to include MissionName and MissionID.
  _AGCModule.TriggerContextEvent(NULL, AllsrvEventID_GameEnded, pszContext,
      GetIGCMission()->GetMissionParams()->strGameName, GetMissionID(), -1, -1, 3, // changed "" to MissionName and -1 to MissionID
      "Reason", VT_LPSTR, pszReason,
	  "WinningTeamID", VT_I4, iTeamObjectID,	 // TE: Added winning teamID
	  "WinningTeamName", VT_LPSTR, pszTeamName); // TE: Added winning teamName

}


/*-------------------------------------------------------------------------
 * MakeOverrideTechBits
 *-------------------------------------------------------------------------
 */
void CFSMission::MakeOverrideTechBits()
{
    if (m_pttbmAltered == NULL)
    {
      m_pttbmAltered = new TechTreeBitMask[c_cSidesMax];
      for (int i = 0; i < c_cSidesMax; ++i)
        (m_pttbmAltered)[i].ClearAll();
    }

    if (m_pttbmNewSetting == NULL)
    {
      m_pttbmNewSetting = new TechTreeBitMask[c_cSidesMax];
      for (int i = 0; i < c_cSidesMax; ++i)
        (m_pttbmNewSetting)[i].ClearAll();
    }
}


/*-------------------------------------------------------------------------
 * RecordGameResults
 *-------------------------------------------------------------------------
   Purpose:
     Records the results of the game to the database.
 */
void CFSMission::RecordGameResults()
{
  #if !defined(ALLSRV_STANDALONE)

    // Create the database update query
    CQGameResults*  pquery = new CQGameResults(NULL); // don't need call-back notification
    CQGameResultsData* pqd = pquery->GetData();

    // Populate the query parameters
    strncpy(pqd->szGameID     , GetIGCMission()->GetContextName()      , sizeofArray(pqd->szGameID));
    strncpy(pqd->szName       , m_misdef.misparms.strGameName          , sizeofArray(pqd->szName));
    strncpy(pqd->szWinningTeam, m_psideWon ? m_psideWon->GetName() : "", sizeofArray(pqd->szWinningTeam));
    // make SURE they're NULL-terminated
    pqd->szGameID[sizeofArray(pqd->szGameID) - 1] = '\0';
    pqd->szName[sizeofArray(pqd->szName) - 1] = '\0';
    pqd->szWinningTeam[sizeofArray(pqd->szWinningTeam) - 1] = '\0';

    pqd->nWinningTeamID       = m_psideWon ? m_psideWon->GetObjectID() : NA;
    pqd->bIsGoalConquest      = m_misdef.misparms.IsConquestGame();
    pqd->bIsGoalCountdown     = m_misdef.misparms.IsCountdownGame();
    pqd->bIsGoalTeamKills     = m_misdef.misparms.IsDeathMatchGame();
    pqd->bIsGoalProsperity    = m_misdef.misparms.IsProsperityGame();
    pqd->bIsGoalArtifacts     = m_misdef.misparms.IsArtifactsGame();
    pqd->bIsGoalFlags         = m_misdef.misparms.IsFlagsGame();
    pqd->nGoalConquest        = m_misdef.misparms.iGoalConquestPercentage;
    pqd->nGoalCountdown       = m_misdef.misparms.dtGameLength;
    pqd->nGoalTeamKills       = m_misdef.misparms.nGoalTeamKills;
    pqd->fGoalProsperity      = m_misdef.misparms.fGoalTeamMoney;
    pqd->nGoalArtifacts       = m_misdef.misparms.nGoalArtifactsCount;
    pqd->nGoalFlags           = m_misdef.misparms.nGoalFlagsCount;
    pqd->nDuration            = GetGameDuration();

    // Post the query for async completion
    g.sql.PostQuery(pquery);

#endif // !defined(ALLSRV_STANDALONE)

	// BT - STEAM - Making this run for all game over scenarios.

    // Iterate through each team of the game
    const SideListIGC* pSides = GetIGCMission()->GetSides();
    for (SideLinkIGC* itSide = pSides->first(); itSide; itSide = itSide->next())
    {
      IsideIGC* pside = itSide->data();
      assert(pside);

      // Record the team results
      RecordTeamResults(pside);
    }

	// BT - STEAM - players who are not currently connected to the mission do not have initialized CSteamAchievement objects
	// available, so skipping this one for now. Players who disconnect before the end of the game will not get any stats recorded.

    // Iterate through each player that left the game before it ended
    //for (OldPlayerLink* itOld = m_oldPlayers.first(); itOld; itOld = itOld->next())
    //{
    //  OldPlayerInfo& opi = itOld->data();

    //  // Record the player results (if they played on a real side)
    //  if (opi.sideID != SIDE_TEAMLOBBY)
    //    RecordPlayerResults(opi.name, &opi.pso, opi.sideID);
    //}


}


/*-------------------------------------------------------------------------
 * RecordTeamResults
 *-------------------------------------------------------------------------
   Purpose:
     Records the results of the team to the database.
 */
void CFSMission::RecordTeamResults(IsideIGC* pside)
{
  #if !defined(ALLSRV_STANDALONE)

    // Create the database update query
    CQTeamResults*  pquery = new CQTeamResults(NULL);
    CQTeamResultsData* pqd = pquery->GetData();

    // Populate the query parameters
    strncpy(pqd->szGameID           , GetIGCMission()->GetContextName(), sizeofArray(pqd->szGameID));
    strncpy(pqd->szName             , pside->GetName()                 , sizeofArray(pqd->szName));
    // make SURE they're NULL-terminated
    pqd->szGameID[sizeofArray(pqd->szGameID) - 1] = '\0';
    pqd->szName[sizeofArray(pqd->szName) - 1] = '\0';

	// KGJV- make sure there is a trailing zero
	for (int i=0;i<sizeofArray(pqd->szTechs);i++) pqd->szTechs[i]=0;
    pside->GetTechs().ToString(pqd->szTechs, sizeofArray(pqd->szTechs));
	pqd->szTechs[sizeofArray(pqd->szTechs)-1]=0;

    pqd->nCivID                     = pside->GetCivilization()->GetObjectID();
    pqd->nTeamID                    = pside->GetObjectID();
    pqd->cPlayerKills               = pside->GetKills();
    pqd->cBaseKills                 = pside->GetBaseKills();
    pqd->cBaseCaptures              = pside->GetBaseCaptures();
    pqd->cDeaths                    = pside->GetDeaths();
    pqd->cEjections                 = pside->GetEjections();
    pqd->cFlags                     = pside->GetFlags();
    pqd->cArtifacts                 = pside->GetArtifacts();
    pqd->nConquestPercent           = pside->GetConquestPercent();
    pqd->nProsperityPercentBought   = pside->GetProsperityPercentBought();
    pqd->nProsperityPercentComplete = pside->GetProsperityPercentComplete();
    if (pside->GetActiveF())
      pqd->nTimeEndured             = GetGameDuration();
    else
      pqd->nTimeEndured             = pside->GetTimeEndured();

    // Post the query for async completion
    g.sql.PostQuery(pquery);

#endif // !defined(ALLSRV_STANDALONE)

	// BT - STEAM - Making this run for all game over scenarios.

    // Iterate through each player of the team
    const ShipListIGC* pShips = pside->GetShips();
    for (ShipLinkIGC* itShip = pShips->first(); itShip; itShip = itShip->next())
    {
      IshipIGC* pship = itShip->data();
      assert(pship);

      // Filter-out non-player ships
      if (ISPLAYER(pship))
      {
        // Get the player score object
        CFSPlayer*    pfsPlayer = ((CFSShip*)(pship->GetPrivateData()))->GetPlayer();
        PlayerScoreObject* ppso = pfsPlayer->GetPlayerScoreObject();

        // Record the player results
        RecordPlayerResults(pship, pfsPlayer, pside->GetObjectID());
      }
    }
}


/*-------------------------------------------------------------------------
 * RecordPlayerResults
 *-------------------------------------------------------------------------
   Purpose:
     Records the results of the player to the database.
 */
void CFSMission::RecordPlayerResults(IshipIGC* pship, CFSPlayer *player, SideID sid)
{
	PlayerScoreObject*  ppso = player->GetPlayerScoreObject();
	//Xynth I didn't need this change today but I don't think it hurts and will likely be needed down the road
	const char * pszName = pship->GetName();

  #if !defined(ALLSRV_STANDALONE)

    // Create the database update query
    CQPlayerResults*  pquery = new CQPlayerResults(NULL);
    CQPlayerResultsData* pqd = pquery->GetData();

    // Populate the query parameters
    strncpy(pqd->szGameID   , GetIGCMission()->GetContextName(), sizeofArray(pqd->szGameID));
    strncpy(pqd->szName     , pszName                          , sizeofArray(pqd->szName)  );
    // make SURE they're NULL-terminated
    pqd->szGameID[sizeofArray(pqd->szGameID) - 1] = '\0';
    pqd->szName[sizeofArray(pqd->szName) - 1] = '\0';

    pqd->nTeamID            = sid;
    pqd->cPlayerKills       = ppso->GetPlayerKills();
    pqd->cBuilderKills      = ppso->GetBuilderKills();
    pqd->cLayerKills        = ppso->GetLayerKills();
    pqd->cMinerKills        = ppso->GetMinerKills();
    pqd->cBaseKills         = ppso->GetBaseKills();
    pqd->cBaseCaptures      = ppso->GetBaseCaptures();
    pqd->cPilotBaseKills    = ppso->GetPilotBaseKills();
    pqd->cPilotBaseCaptures = ppso->GetPilotBaseCaptures();
    pqd->cDeaths            = ppso->GetDeaths();
    pqd->cEjections         = ppso->GetEjections();
    pqd->cRescues           = ppso->GetRescues();
    pqd->cFlags             = ppso->GetFlags();
    pqd->cArtifacts         = ppso->GetArtifacts();
    pqd->cTechsRecovered    = ppso->GetTechsRecovered();
    pqd->cAlephsSpotted     = ppso->GetWarpsSpotted();
    pqd->cAsteroidsSpotted  = ppso->GetAsteroidsSpotted();
    pqd->fCombatRating      = ppso->GetCombatRating();
    pqd->fScore             = ppso->GetScore();
    pqd->nTimePlayed        = ppso->GetTimePlayed();

    // spew the player results that we're writing...
    debugf("Writing player results: %s %s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %g %g %d\n",
      pqd->szGameID,
      pqd->szName,
      pqd->nTeamID,
      pqd->cPlayerKills,
      pqd->cBuilderKills,
      pqd->cLayerKills,
      pqd->cMinerKills,
      pqd->cBaseKills,
      pqd->cBaseCaptures,
      pqd->cPilotBaseKills,
      pqd->cPilotBaseCaptures,
      pqd->cDeaths,
      pqd->cEjections,
      pqd->cRescues,
      pqd->cFlags,
      pqd->cArtifacts,
      pqd->cTechsRecovered,
      pqd->cAlephsSpotted,
      pqd->cAsteroidsSpotted,
      (double)pqd->fCombatRating,
      (double)pqd->fScore,
      pqd->nTimePlayed
      );


    // Post the query for async completion
    g.sql.PostQuery(pquery);

#else

	// BT - STEAM
	CSteamAchievements *pSteamAchievements = player->GetSteamAchievements();
	IshipIGC *pIship = player->GetIGCShip();
	pSteamAchievements->AwardBetaParticipation();
	
	pSteamAchievements->UpdateLeaderboard(ppso);
	pSteamAchievements->AddUserStats(ppso, pIship);	

	pSteamAchievements->SaveStats();


  #endif // !defined(ALLSRV_STANDALONE)
}


/*-------------------------------------------------------------------------
 * QueueGameoverMessage
 *-------------------------------------------------------------------------
   Purpose:
     Tell a player or group of players that they are out of the game.

   Parameters:
      The winning side is in m_psideWon
 */
void CFSMission::QueueGameoverMessage()
{
  // queue the general game over stats and side stats

  BEGIN_PFM_CREATE(g.fm, pfmGameOver, S, GAME_OVER)
    FM_VAR_PARM((const char*)(m_pszReason ? m_pszReason : "Your side has been eliminated."), CB_ZTS)
  END_PFM_CREATE

  for (SideID sideID = 0; sideID < c_cSidesMax; sideID++)
  {
    IsideIGC* pside = m_pMission->GetSide(sideID);

    strcpy(pfmGameOver->rgSides[sideID].sideName, pside ? pside->GetName() : "<bug>");
    pfmGameOver->rgSides[sideID].civID = pside ? pside->GetCivilization()->GetObjectID() : -1;
    pfmGameOver->rgSides[sideID].color = pside ? pside->GetColor() : Color(0.5, 0.5, 0.5);
    pfmGameOver->rgSides[sideID].cKills = pside ? pside->GetKills() : 0;
    pfmGameOver->rgSides[sideID].cDeaths = pside ? pside->GetDeaths() : 0;
    pfmGameOver->rgSides[sideID].cEjections = pside ? pside->GetEjections() : 0;
    pfmGameOver->rgSides[sideID].cBaseKills = pside ? pside->GetBaseKills() : 0;
    pfmGameOver->rgSides[sideID].cBaseCaptures = pside ? pside->GetBaseCaptures() : 0;
    pfmGameOver->rgSides[sideID].cFlags = pside ? pside->GetFlags() : 0;
    pfmGameOver->rgSides[sideID].cArtifacts = pside ? pside->GetArtifacts() : 0;
  };

  pfmGameOver->nNumSides = m_pMission->GetSides()->n();
  pfmGameOver->iSideWinner = m_psideWon ? m_psideWon->GetObjectID() : NA;
  pfmGameOver->bEjectPods = m_misdef.misparms.bEjectPods;


  // queue the player stats
  const ShipListIGC * plistShip = m_pMission->GetShips();
  int     nNumPlayers = plistShip->n() + m_oldPlayers.n();      //Worst case estimate
  size_t  playerlistSize = nNumPlayers * sizeof(PlayerEndgameInfo);
  PlayerEndgameInfo* playerlist = (PlayerEndgameInfo*)alloca(playerlistSize);

  // get the current stats for all players
  int nPlayerIndex = 0;
  for (ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
  {
    if (ISPLAYER(plinkShip->data()))
    {
      CFSPlayer* pfsPlayer = ((CFSShip *)(plinkShip->data()->GetPrivateData()))->GetPlayer();

      PlayerScoreObject*    ppso = pfsPlayer->GetPlayerScoreObject();
      if (ppso->GetTimePlayed() > 0.0f && pfsPlayer->GetSide()->GetObjectID() != SIDE_TEAMLOBBY)
      {
          strcpy(playerlist[nPlayerIndex].characterName, pfsPlayer->GetName());
          playerlist[nPlayerIndex].scoring.Set(pfsPlayer->GetPlayerScoreObject());
          playerlist[nPlayerIndex].stats = ppso->GetPersist();
          playerlist[nPlayerIndex].sideId = pfsPlayer->GetSide()->GetObjectID();

          nPlayerIndex++;
      }
    }
  }

  for (OldPlayerLink*   popl = m_oldPlayers.first(); (popl != NULL); popl = popl->next())
  {
      OldPlayerInfo&  opi = popl->data();

      if (opi.pso.GetTimePlayed() != 0.0f && opi.sideID != SIDE_TEAMLOBBY)
      {
          strcpy(playerlist[nPlayerIndex].characterName, opi.name);
          playerlist[nPlayerIndex].scoring.Set(&(opi.pso));
          playerlist[nPlayerIndex].stats = opi.pso.GetPersist();
          playerlist[nPlayerIndex].sideId = opi.sideID;

          nPlayerIndex++;
      }
  }
  assert(nPlayerIndex <= nNumPlayers);

  // send the players in chunks of no more than 50 players at a time
  const int nMaxPlayersPerMsg = 50;
  while (nPlayerIndex > 0)
  {
      int nPlayers = min(nPlayerIndex, nMaxPlayersPerMsg);
      nPlayerIndex -= nPlayers;

      BEGIN_PFM_CREATE(g.fm, pfmGameOver, S, GAME_OVER_PLAYERS)
        FM_VAR_PARM(playerlist + nPlayerIndex, nPlayers * sizeof(PlayerEndgameInfo))
      END_PFM_CREATE
  }
}

static float    GetExpMult(float totalExp, float sideExp, int nSides)
{
    const float minExpMult = 0.1f;

    if ((nSides < 2) || (sideExp <= 0.0f))
        return 1.0f;

    float   avgExp = (totalExp - sideExp) / float(nSides - 1);
    if (sideExp <= avgExp)
        return 1.0f;

    float m = avgExp / sideExp;

    return (m < minExpMult) ? minExpMult : m;
}

/*-------------------------------------------------------------------------
 * ProcessGameOver
 *-------------------------------------------------------------------------
   Purpose:
      Wrap things up when the game is actually over, not to be confused with
      when the mission is over, which doesn't happen until everyone in it leaves.

   Parameters:
      The winning side is in m_psideWon
 */
void CFSMission::ProcessGameOver()
{
  const ShipListIGC * pShips = m_pMission->GetShips();
  ShipLinkIGC * pShiplink;

  m_flGameDuration = g.timeNow - m_misdef.misparms.timeStart;

  //Xynth Determine Rank Ratio and load it into PlayerScoreObjects
  {
	  int sideCount = 0;
	  ObjectID sideID[3];
	  int rank[3];
	  for (SideLinkIGC* psl = m_pMission->GetSides()->first(); psl != NULL && sideCount < 3; psl = psl->next())
	  {
		  IsideIGC* pside = psl->data();
		  sideID[sideCount] = pside->GetObjectID();
		  rank[sideCount] = GetSideRankSum(pside,false);
		  sideCount++;		  
	  }
	  if (sideCount == 2) //if more than 2 sides, just use the default 1.0 ratio
	  {
		  for (int i = 0; i < 2; i++)
		  {
			  if (rank[i] == 0) //avoid divide by zero
				  rank[i] = 1;
		  }
		  float ratio1 = float(rank[0]) / rank[1];
		  float ratio0 = float(rank[1]) / rank[0];
		  for (pShiplink = pShips->first(); pShiplink; pShiplink = pShiplink->next())
		  {
			  CFSShip * pfsShip = (CFSShip *)pShiplink->data()->GetPrivateData();
			  if (pfsShip->IsPlayer())
			  {
				  PlayerScoreObject*  ppso = pfsShip->GetPlayerScoreObject();
				  if (pfsShip->GetSide()->GetObjectID() == sideID[0])
					  ppso->SetRankRatio(ratio0);
				  else
					  ppso->SetRankRatio(ratio1);
			  }
		  }
	  }

  }
  
  //Calculate scores for all players and get a running average of time played.
  float                 totalExperience                = 0.0f;
  float                 sideExperience[c_cSidesMax]    = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  float                 dtPlayed[c_cSidesMax]    = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  float                 scoreEarned[c_cSidesMax] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
  PlayerScoreObject*    commander[c_cSidesMax]   = {NULL, NULL, NULL, NULL, NULL, NULL};
  {
      for (pShiplink = pShips->first(); pShiplink; pShiplink = pShiplink->next())
      {
        CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
        if (pfsShip->IsPlayer())
        {
			PlayerScoreObject*  ppso = pfsShip->GetPlayerScoreObject();
			
			if (ppso->Connected())
            {

                IsideIGC*   pside = pfsShip->GetSide();

				ppso->Disconnect(g.timeNow);

                bool  bCount = (ppso->GetTimePlayed() > 180.0f) && !m_bDraw;

                ppso->EndGame(m_pMission,
                              (pside == m_psideWon || m_psideWon->AlliedSides(m_psideWon,pside)) && bCount && (ppso->GetTimePlayed() > GetGameDuration() / 2.0f),
                              (pside != m_psideWon && !m_psideWon->AlliedSides(m_psideWon,pside)) && bCount);

                SideID  sid = pside->GetObjectID();


                if (sid >= 0)
                {
                    float   e = ppso->GetTimePlayed() * ppso->GetPersist().GetScore();

                    totalExperience += e;
                    sideExperience[sid] += e;

                    scoreEarned[sid] += ppso->GetScore();
                    dtPlayed[sid] += ppso->GetTimePlayed();
                }

                if ((commander[sid] == NULL) ||
                    (commander[sid]->GetTimeCommanded() < ppso->GetTimeCommanded()))
                {
                    commander[sid] = ppso;
                }

            }
        }
      }
  }

  {
    SideID  sidWin = m_psideWon ? m_psideWon->GetObjectID() : NA;
    for (OldPlayerLink* popl = m_oldPlayers.first(); (popl != NULL); popl = popl->next())
	{
        OldPlayerInfo & opi = popl->data();
        bool  bCount = (opi.pso.GetTimePlayed() > 180.0f);

         opi.pso.EndGame(m_pMission,
                      (opi.sideID == sidWin || m_psideWon->AlliedSides(m_psideWon,m_pMission->GetSide(opi.sideID))) && bCount && (opi.pso.GetTimePlayed() > GetGameDuration() / 2.0f),
                      (opi.sideID != sidWin && !m_psideWon->AlliedSides(m_psideWon,m_pMission->GetSide(opi.sideID))) && bCount);

      if (opi.sideID >= 0)
        {
            float   e = opi.pso.GetTimePlayed() * opi.pso.GetPersist().GetScore();

            totalExperience += e;
            sideExperience[opi.sideID] += e;

            scoreEarned[opi.sideID] += opi.pso.GetScore();
            dtPlayed[opi.sideID] += opi.pso.GetTimePlayed();


            if ((commander[opi.sideID] == NULL) ||
                (commander[opi.sideID]->GetTimeCommanded() < opi.pso.GetTimeCommanded()))
            {
                commander[opi.sideID] = &(opi.pso);
            }
        }
    }
  }

  //Get average exp/team
  {
      for (pShiplink = pShips->first(); pShiplink; pShiplink = pShiplink->next())
      {
        CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
        if (pfsShip->IsPlayer())
        {
            IsideIGC*   pside = pfsShip->GetSide();
            if (pside && pside->GetObjectID() != SIDE_TEAMLOBBY)
            {
                SideID  sid = pside->GetObjectID();
                float   f = GetExpMult(totalExperience, sideExperience[sid], m_pMission->GetSides()->n());
                if (f < 1.0f)
                {
                    PlayerScoreObject*  ppso = pfsShip->GetPlayerScoreObject();
                    ppso->SetScore(ppso->GetScore() * f);
                }
            }
        }
      }

      for (OldPlayerLink* popl = m_oldPlayers.first(); (popl != NULL); popl = popl->next())
      {
        OldPlayerInfo & opi = popl->data();

        if (opi.sideID != SIDE_TEAMLOBBY)
        {
          SideID  sid = opi.sideID;
          float   f = GetExpMult(totalExperience, sideExperience[sid], m_pMission->GetSides()->n());
          if (f < 1.0f)
          {
            opi.pso.SetScore(opi.pso.GetScore() * f);
          }
        }
      }
  }

  // award points for commanding
  {
      for (SideLinkIGC* psl = m_pMission->GetSides()->first(); (psl != NULL); psl = psl->next())
      {
          IsideIGC* pside = psl->data();
          SideID    sideID = pside->GetObjectID();

          PlayerScoreObject*  ppso = commander[sideID];
          if (ppso)
          {
            assert (dtPlayed[sideID] >= 0.0f);

            //Average points earned per player minute
            float   commandScore = scoreEarned[sideID] / dtPlayed[sideID];
            {
                float   f = GetExpMult(totalExperience, sideExperience[sideID], m_pMission->GetSides()->n());
                if (f < 1.0f)
                {
                    commandScore *= f;
                }
            }

            ppso->SetCommanderScore(commandScore * ppso->GetTimePlayed());
          }
      }
  }

  //Save player scores
  {
      for (pShiplink = pShips->first(); pShiplink; pShiplink = pShiplink->next())
      {
        CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
        if (pfsShip->IsPlayer())
        {
            IsideIGC*   pside = pfsShip->GetSide();
            if (pside && pside->GetObjectID() != SIDE_TEAMLOBBY)
            {
                PlayerScoreObject*  ppso = pfsShip->GetPlayerScoreObject();
                SetCharStats(pfsShip->GetPlayer()->GetCharacterID(), pfsShip->GetPlayer(), pside, *ppso, this);
            }
        }
      }
  }

  {
    for (OldPlayerLink* popl = m_oldPlayers.first(); (popl != NULL); popl = popl->next())
    {
      OldPlayerInfo & opi = popl->data();

      if (opi.sideID != SIDE_TEAMLOBBY)
        SetCharStats(opi.characterID, NULL, GetIGCMission()->GetSide(opi.sideID), opi.pso, this);
    }
  }

  // if this was a squads game, record the wins and losses for the squads
  if (m_misdef.misparms.bSquadGame && m_misdef.misparms.bScoresCount && m_psideWon
      && m_misdef.misparms.nTeams == 2
      && m_pMission->GetSide(0)->GetSquadID() != m_pMission->GetSide(1)->GetSquadID())
  {
    RecordSquadGame(m_pMission->GetSides(), m_psideWon);
  }

  // Record the Game Results
  //if (m_misdef.misparms.bScoresCount) // Only if scores count
  //{
  // BT - STEAM - Scores always count. 
    RecordGameResults();
  //}

  // Queue the GameOver message to all connected users
  g.fm.SetDefaultRecipient(GetGroupMission(), FM_GUARANTEED);
  QueueGameoverMessage();
  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
  {
    for (pShiplink = pShips->first(); pShiplink; pShiplink = pShiplink->next())
    {
      CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
      if (pfsShip->IsPlayer())
      {
        pfsShip->GetPlayer()->SetLifepod(NULL, Vector::GetZero());

        PlayerScoreObject*  ppso = pfsShip->GetPlayerScoreObject();
        ppso->Reset(true);
      }
    }
  }

  // Purge the old players list
  m_oldPlayers.purge();

  // clear the clusters and destroy the drones
  Vacate();

  // kill any pending votes
  while (!m_ballots.IsEmpty())
    delete m_ballots.PopFront();

  /*GetSite()->SendChatf(NULL, CHAT_EVERYONE, NA,
                       NA, "The game is over. %s are victorious!.",
                       m_psideWon->GetName());
  */

/*  Imago commented out so afk not reset
  // set all of the players to unready
  for (pShiplink = pShips->first(); pShiplink; pShiplink = pShiplink->next())
  {
    CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();

    assert(pfsShip->IsPlayer());

    // Review: since ready has changed to this cheezy "away from keyboard" thing,
    // set everyone as not away from keyboard.
    if (pfsShip->GetSide()->GetObjectID() != SIDE_TEAMLOBBY)
        pfsShip->GetPlayer()->SetReady(true);
  }
*/
  // Restart the game if the server is not paused.
  bool bRestartable = !g.fPaused && m_misdef.misparms.bAllowRestart;
  #if defined(ALLSRV_STANDALONE)
    // HACK: for training missions, end the game and don't let it restart.
    if (m_misdef.misparms.nTotalMaxPlayersPerGame == 1)
        bRestartable = false;
  #endif

  SetStage(bRestartable ? STAGE_NOTSTARTED : STAGE_OVER); // set to STAGE_OVER if game should not restart.

  // set all of the sides to active and not forced ready
  for (SideID sideID = 0; sideID < m_misdef.misparms.nTeams; sideID++)
  {
    IsideIGC* pside = m_pMission->GetSide(sideID);

    // break up teams that no longer have the leadership they need
    if (pside->GetSquadID() != NA)
      MaintainSquadLeadership(sideID);

    m_misdef.rgfReady       [sideID] = false;
    m_misdef.rgfForceReady  [sideID] = false;
    m_misdef.rgfActive      [sideID] = true;
    m_rgMoney               [sideID] = 0;
    pside->SetActiveF(true);
	// KGJV #62 - reset AET
	bool bAET = m_misdef.misparms.bAllowEmptyTeams;
	if (!m_misdef.misparms.bObjectModelCreated)
		m_misdef.misparms.bAllowEmptyTeams = false;
	// send new AET value if changed
	if (bAET)
	{
		BEGIN_PFM_CREATE(g.fm, pfmSideInactive, CS, SIDE_INACTIVE)
		END_PFM_CREATE
		pfmSideInactive->sideID = 0; // doesnt matter which side but 0 is always valid
		pfmSideInactive->bActive = true;
		pfmSideInactive->bChangeAET = true;
		pfmSideInactive->bAET = false;
		g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
	}
    CheckForSideAllReady(pside);
  }

  // reset the clusters, etc.
  for (SideLinkIGC*   psl = m_pMission->GetSides()->first(); (psl != NULL); psl = psl->next())
      psl->data()->Reset();
  m_pMission->ResetMission();

  LPCSTR pszContext = GetIGCMission() ? GetIGCMission()->GetContextName() : NULL;
  // TE, Modify GameOver AGCEvent to include MissionID.
  _AGCModule.TriggerContextEvent(NULL, AllsrvEventID_GameOver, pszContext,
    GetIGCMission()->GetMissionParams()->strGameName, GetMissionID(),
   -1, -1, 0); // // Modified "" and -1 to MissionName and MissionID
  // old event
  //_AGCModule.TriggerContextEvent(NULL, AllsrvEventID_GameOver, pszContext,
  //  "", -1, -1, -1, 0);

  // if the game is an auto-restart game, reset the start time.
  if (bRestartable && m_misdef.misparms.bAutoRestart)
  {
    m_misdef.misparms.timeStart = Time::Now() + m_misdef.misparms.fRestartCountdown;

    // forward the new start time to everyone
    BEGIN_PFM_CREATE(g.fm, pfmStartTime, S, SET_START_TIME)
    END_PFM_CREATE
    pfmStartTime->timeStart = m_misdef.misparms.timeStart;
    g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
  }
}


/*-------------------------------------------------------------------------
 * CheckForVictory
 *-------------------------------------------------------------------------
 * Purpose:
 *    See if a side has won and handle it if they have
 */
IsideIGC*   CFSMission::CheckForVictoryByStationBuild(IsideIGC* psideTest)
{
    return CheckForVictoryByStationKill(NULL, NULL);
}

IsideIGC*   CFSMission::CheckForVictoryByStationCapture(IsideIGC* psideTest, IsideIGC* psideOld)
{
    return CheckForVictoryByStationKill(NULL, psideOld);
}

IsideIGC*   CFSMission::CheckForVictoryByStationKill(IstationIGC* pstationKilled, IsideIGC* psideOld)
{
  IsideIGC*   psideWon = NULL;
  const MissionParams* pmp = m_pMission->GetMissionParams();
  // Imago account for allies 7/17/09
  bool bAllies = false;

  if ( (STAGE_STARTED == GetStage()) || ((STAGE_STARTING == GetStage()) && pmp->bAllowEmptyTeams) && (m_psideWon == NULL))
  {
    const SideListIGC*  psides = m_pMission->GetSides();


    bool                 bChange = false;
    if (pmp->IsConquestGame())
    {

        int nStationsTotal = 0;
        assert (c_cSidesMax == 6);
        int nStationsPerSide[c_cSidesMax] = {0, 0, 0, 0, 0, 0};

        {
            for (StationLinkIGC*    psl = m_pMission->GetStations()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                IstationIGC*    pstation = psl->data();
                if ((pstation != pstationKilled) && pstation->GetBaseStationType()->HasCapability(c_sabmFlag))
                {
                    nStationsTotal++;
                    nStationsPerSide[pstation->GetSide()->GetObjectID()]++;
                }
            }
        }

		// KGJV #62 count active teams
		int nActiveTeams = 0;

        if (nStationsTotal != 0)
        {
            const SideListIGC*  psides = m_pMission->GetSides();
			

            for (SideLinkIGC* l = psides->first(); (l != NULL); l = l->next())
            {
                IsideIGC*       pside = l->data();
				// KGJV #62 count active team
				if (pside->GetActiveF()) nActiveTeams++;
				if (pside->GetAllies() != NA) bAllies = true;

                unsigned char   conquest = (unsigned char)(100 * nStationsPerSide[pside->GetObjectID()] / nStationsTotal);

                if (conquest != pside->GetConquestPercent())
                {
                    bChange = true;
                    pside->SetConquestPercent(conquest);

                    if (conquest >= pmp->iGoalConquestPercentage)
                        psideWon = pside;
                }
            }
			// KGJV #62 - no winner if bAllowEmptyTeam and one active team is left - Imago: allies overrides to prevent "issues"?
			if (pmp->bAllowEmptyTeams && (nActiveTeams == 1))
				psideWon = NULL;
        }
    }

    if (pmp->IsTerritoryGame() && (psideWon == NULL))
	{
        assert (c_cSidesMax == 6);
        unsigned char nTerritoriesPerSide[c_cSidesMax] = {0, 0, 0, 0, 0, 0};

        const ClusterListIGC*   pclusters = m_pMission->GetClusters();

        for (ClusterLinkIGC*    pcl = pclusters->first(); (pcl != NULL); pcl = pcl->next())
        {
            IclusterIGC*    pcluster = pcl->data();

            IsideIGC*       psideOwner = NULL;
            StationLinkIGC*    psl;
            for (psl = pcluster->GetStations()->first(); (psl != NULL); psl = psl->next())
            {
                if (psl->data() != pstationKilled)
                {
                    IsideIGC*   pside = psl->data()->GetSide();
                    if (psideOwner == NULL)
                        psideOwner = pside;
                    else if (psideOwner != pside)
                        break;
                }
            }

            if (psideOwner && (psl == NULL))
                nTerritoriesPerSide[psideOwner->GetObjectID()]++;
        }

        unsigned char nThreashold = (unsigned char)((50 + pclusters->n() * pmp->iGoalTerritoryPercentage) / 100);

        for (SideLinkIGC* l = psides->first(); (l != NULL); l = l->next())
        {
            IsideIGC*       pside = l->data();
            SideID          sideID = pside->GetObjectID();

            if (nTerritoriesPerSide[sideID] != pside->GetTerritoryCount())
            {
                bChange = true;
                pside->SetTerritoryCount(nTerritoriesPerSide[sideID]);

                if (nTerritoriesPerSide[sideID] >= nThreashold)
                    psideWon = pside;
            }
        }
    }

    //Broadcast the victory rankings to all sides
    if (bChange)
    {
        BEGIN_PFM_CREATE(g.fm, pfmGameState, S, GAME_STATE)
        END_PFM_CREATE
        SideID  sid = 0;
        for (SideLinkIGC*   l = psides->first(); (l != NULL); l = l->next())
        {
            assert (sid == l->data()->GetObjectID());
            pfmGameState->conquest[sid] = l->data()->GetConquestPercent();
            pfmGameState->territory[sid] = l->data()->GetTerritoryCount();
            pfmGameState->nFlags[sid] = l->data()->GetFlags();
            pfmGameState->nArtifacts[sid++] = l->data()->GetArtifacts();
        }
        g.fm.SendMessages(GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);
    }

    //Elimiation of all restart stations of all other sides is always a win
    if ((psideWon == NULL) && psideOld)
    {
        StationLinkIGC*    psl;
        for (psl = psideOld->GetStations()->first();
             (psl != NULL);
             psl = psl->next())
        {
            IstationIGC* pstation = psl->data();
            if ((pstation != pstationKilled) && pstation->GetStationType()->HasCapability(c_sabmRestart))
                break;
        }
        if (psl == NULL)
        {
            //NYI last station gone ... need to deactivate side
            DeactivateSide(psideOld);
        }
    }
  }


  if (bAllies) { //Imago 7/21/09
	bool bAllAllies = true;
	IsideIGC*   allywinside = NULL;
	//lets get a list of allied sideIDs  Imago ALLY 7/17/09
	for (SideLinkIGC* psidelink = m_pMission->GetSides()->first();
		(psidelink != NULL);
		psidelink = psidelink->next())
	{
		IsideIGC* otherside = psidelink->data();
		//the only active sides are all allied...game is over.
		if (!otherside->GetActiveF()) continue;
		for (SideLinkIGC* psl = m_pMission->GetSides()->first();
			(psl != NULL);
			psl = psl->next())
		{
			IsideIGC*   thisside = psl->data();
			if (!thisside->GetActiveF()) continue;
			if (!thisside->AlliedSides(otherside,thisside) && otherside != thisside)
				bAllAllies = false;
			else 
				allywinside = thisside;
		}
	}
	psideWon = (bAllAllies) ? allywinside : psideWon;
  }


  return psideWon;
}

IsideIGC*   CFSMission::CheckForVictoryByKills(IsideIGC*    psideTest)
{
  if ((STAGE_STARTED == GetStage()) && (m_psideWon == NULL))
  {
    const MissionParams* pmp = m_pMission->GetMissionParams();
    if (pmp->IsDeathMatchGame())
    {
        if (psideTest->GetKills() >= pmp->nGoalTeamKills)
        {
            return psideTest;
        }
    }
  }

  return NULL;
}


IsideIGC* CFSMission::CheckForVictoryByInactiveSides(bool& bAllSidesInactive)
{
  bAllSidesInactive = true;

  IsideIGC* pSideWin = NULL;
  const SideListIGC * plistSide = m_pMission->GetSides();
  for (SideLinkIGC * plinkSide = plistSide->first(); plinkSide; plinkSide = plinkSide->next())
  {
    IsideIGC * pside = plinkSide->data();
    if (HasPlayers(pside, false) && pside->GetActiveF()) // found one
    {
      bAllSidesInactive = false;

	  if (pSideWin)
      {
        pSideWin = NULL;
        break;
      }
      else
        pSideWin = pside;
    }
  }

  #if defined(ALLSRV_STANDALONE)
    // HACK: for training missions, don't end the game before it starts just
    // because a side is inactive.
    if (m_misdef.misparms.nTotalMaxPlayersPerGame == 1 && GetStage() == STAGE_STARTING)
    {
      pSideWin = NULL;
    }
  #endif

  // HACK: for testing purposes, don't end the game before it's started if
  // everyone still playing can cheat.
  if (pSideWin && GetStage() == STAGE_STARTING)
  {
    bool bFoundNormalPlayer = false;

    const ShipListIGC*    pships = m_pMission->GetShips();
    for (ShipLinkIGC* psl = pships->first(); (psl != NULL); psl = psl->next())
    {
      CFSShip* pfsShip = (CFSShip *)(psl->data()->GetPrivateData());

      if (pfsShip->IsPlayer() && !pfsShip->GetPlayer()->CanCheat())
      {
		  //imago 7/9/09 REMOVE REVIEW TEST
//#ifdef DEBUG
//		bFoundNormalPlayer = false;
//#else
        bFoundNormalPlayer = true;
//#endif
        break;
      }
    }

    if (!bFoundNormalPlayer)
      pSideWin = NULL;
  }

  // KGJV #62 - handling of AllowEmptyTeams
  // don't end game during STAGE_STARTING if bAllowEmptyTeams
  if ((GetStage() == STAGE_STARTING) && m_misdef.misparms.bAllowEmptyTeams)
  {
      return NULL; //Imago 7/21/09
  }

	bool bAllAllies = true;
	IsideIGC*   allywinside = NULL;
	//lets get a list of allied sideIDs  Imago Ally 7/17/09
	for (SideLinkIGC* psidelink = m_pMission->GetSides()->first();
		(psidelink != NULL);
		psidelink = psidelink->next())
	{
		IsideIGC* otherside = psidelink->data();
		//the only active sides are all allied...game is over.
		if (!otherside->GetActiveF()) continue;
		for (SideLinkIGC* psl = m_pMission->GetSides()->first();
			(psl != NULL);
			psl = psl->next())
		{
			IsideIGC*   thisside = psl->data();
			if (!thisside->GetActiveF()) continue;
			if (!thisside->AlliedSides(otherside,thisside) && otherside != thisside)
				bAllAllies = false;
			else 
				allywinside = thisside;
		}
	}
	pSideWin = (bAllAllies) ? allywinside : pSideWin;

  return pSideWin;
}

//#ALLYTD: Fix CTF for alliances
IsideIGC*   CFSMission::CheckForVictoryByFlags(IsideIGC*    psideTest, SideID sidFlag)
{
  if ((STAGE_STARTED == GetStage()) && (m_psideWon == NULL))
  {
    const MissionParams* pmp = m_pMission->GetMissionParams();
    if (pmp->IsArtifactsGame() || pmp->IsFlagsGame())
    {
        BEGIN_PFM_CREATE(g.fm, pfmGameState, S, GAME_STATE)
        END_PFM_CREATE
        SideID  sid = 0;
        for (SideLinkIGC*   l = m_pMission->GetSides()->first(); (l != NULL); l = l->next())
        {
            assert (sid == l->data()->GetObjectID());
            pfmGameState->conquest[sid] = l->data()->GetConquestPercent();
            pfmGameState->territory[sid] = l->data()->GetTerritoryCount();
            pfmGameState->nFlags[sid] = l->data()->GetFlags();
            pfmGameState->nArtifacts[sid++] = l->data()->GetArtifacts();
        }
        g.fm.SendMessages(GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

        if (sidFlag != SIDE_TEAMLOBBY)
        {
            if (psideTest->GetFlags() >= pmp->nGoalFlagsCount)
            {
                return psideTest;
            }
        }
        else
        {
            if (psideTest->GetArtifacts() >= pmp->nGoalArtifactsCount)
            {
                return psideTest;
            }
        }
    }
  }

  return NULL;
}


/*-------------------------------------------------------------------------
 * CreateDPGroups
 *-------------------------------------------------------------------------
 * Purpose:
 *    create the groups for the cluster, and attach it to the IGC cluster
 *
 * Side Effects:
 *    These groups must be cleaned up manually before destroying the cluster!
 */
void CFSMission::CreateDPGroups(IclusterIGC * pcluster)
{
  ClusterGroups * pcg = new ClusterGroups;
  char szDocked[] = "Everyone docked in sector ";
  char szFlying[] = "Everyone flying in sector ";
  char szBuff[max(sizeof(szDocked), sizeof(szFlying)) + c_cbName + 1];
  wsprintf(szBuff, "%s%s", szDocked, pcluster->GetName());
  pcg->pgrpClusterDocked = g.fm.CreateGroup(szBuff);
  wsprintf(szBuff, "%s%s", szDocked, pcluster->GetName());
  pcg->pgrpClusterFlying = g.fm.CreateGroup(szBuff);

  ((CFSCluster*)pcluster->GetPrivateData())->SetClusterGroups(pcg);
}


/*-------------------------------------------------------------------------
 * SendLobbyMissionInfo
 *-------------------------------------------------------------------------
 * Purpose:
 *    Send the player only the data they need while in the lobby.  This
 *    does not include things like station and sector info.
 *
 * Parameters:
 *    Who the junk goes to
 */
void CFSMission::SendLobbyMissionInfo(CFSPlayer * pfsPlayer)
{
  ImissionIGC * pMission = GetIGCMission();

  g.fm.SetDefaultRecipient((CFMRecipient*)(pfsPlayer->GetConnection()),
                           FM_GUARANTEED);

  // tell them about the current mission info
  g.fm.QueueExistingMsg(GetMissionDef());

  // tell them that they have been accepted
  BEGIN_PFM_CREATE(g.fm, pfmJoinedMission, S, JOINED_MISSION)
  END_PFM_CREATE
  pfmJoinedMission->shipID = pfsPlayer->GetShipID();
  pfmJoinedMission->dwCookie = GetCookie();
  // Export sides
  const SideListIGC * pstlist = pMission->GetSides();
  SideLinkIGC * pstlink;
  for (pstlink = pstlist->first(); pstlink; pstlink = pstlink->next())
    ExportObj(pstlink->data(), OT_side, NULL);

  g.fm.SendMessages(NULL, FM_GUARANTEED, FM_FLUSH);

  // tell them about all of the players
  const ShipListIGC * pshiplist = pMission->GetShips();
  for (ShipLinkIGC * pshiplink = pshiplist->first();
       pshiplink;
       pshiplink = pshiplink->next())
  {
    IshipIGC*   pship = pshiplink->data();
    CFSShip * pfsShip = (CFSShip *)(pship->GetPrivateData());

    SendPlayerInfo(pfsPlayer, pfsShip, this, false);
  }

  g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
}

/*-------------------------------------------------------------------------
 * QueueLobbyMissionInfo
 *-------------------------------------------------------------------------
 * Purpose:
 *    Queue up a shorter form of the mission def which is used for players
 *    who have not yet chosen a game.
 *
 * Parameters:
 */
void CFSMission::QueueLobbyMissionInfo()
{
  assert(g.fmLobby.IsConnected());

  SquadID rgSquadIDs[c_cSidesMax];
  int nSquadCount = 0;

  // fill in the squad info for any squads that are playing
  {
    const SideListIGC * plistSide = m_pMission->GetSides();
    for (SideLinkIGC * plinkSide = plistSide->first(); plinkSide; plinkSide = plinkSide->next())
    {
      SquadID squadID = plinkSide->data()->GetSquadID();

      if (squadID != NA && 0 != plinkSide->data()->GetShips()->n()
          && (STAGE_STARTED != GetStage() || plinkSide->data()->GetActiveF()))
      {
        rgSquadIDs[nSquadCount] = squadID;
        ++nSquadCount;
      }
    }
  }

  // mdvalley: Discover listening port
  DWORD dwPort = 0;

#if defined(SRV_CHILD) //Imago: revisit
  HRESULT hr = g.fm.GetListeningPort(&dwPort);
  if(FAILED(hr))
  {
	  debugf("Cannot discover listening port.\n");
  }
#endif

  char szAddr[16]= "XXX-YYY-ZZZ-TTT"; // KGJV #114 IMAGO REVIEW IPv6!!!!
  ZVersionInfo vi; ZString zInfo = (LPCSTR)vi.GetFileVersionString(); //Imago 7/10 #62
  // KGJV: added sending m_misdef.misparms.szIGCStaticFile to lobby
  BEGIN_PFM_CREATE(g.fmLobby, pfmLobbyMissionInfo, LS, LOBBYMISSIONINFO)
    FM_VAR_PARM(m_misdef.misparms.strGameName, CB_ZTS)
    FM_VAR_PARM(nSquadCount ? rgSquadIDs : NULL, nSquadCount * sizeof(SquadID))
    FM_VAR_PARM((PCC)m_strDetailsFiles, CB_ZTS)
	FM_VAR_PARM(m_misdef.misparms.szIGCStaticFile,CB_ZTS)
	FM_VAR_PARM((PCC)(g.strLocalAddress),CB_ZTS) // KGJV #114 - ServerName
	FM_VAR_PARM(szAddr,16)                       // KGJV #114 - ServerAddr - placeholder here, lobby will fill it /Revisit, this can be Oct'ed and sent non variable
	FM_VAR_PARM(PCC(UTL::GetPrivilegedUsers(-1)),CB_ZTS) //Imago 6/10
	FM_VAR_PARM(PCC(zInfo),CB_ZTS) //Imago 7/10
  END_PFM_CREATE
  pfmLobbyMissionInfo->dwPort = dwPort;
  pfmLobbyMissionInfo->dwCookie = GetCookie();

  debugf("sending lobby our mission info for %x listening on port %d\n",pfmLobbyMissionInfo->dwCookie,dwPort);
  // adjust the clock time to be an offset from the current time (the lobby server will fix this)
  pfmLobbyMissionInfo->dwStartTime = m_misdef.misparms.timeStart.clock() - Time::Now().clock();

  // fill in the information on number of players and slots available
  pfmLobbyMissionInfo->fGuaranteedSlotsAvailable       = false;
  pfmLobbyMissionInfo->fAnySlotsAvailable              = false;
  pfmLobbyMissionInfo->nNumPlayers                     = GetCountOfPlayers(NULL, false);
  pfmLobbyMissionInfo->nNumNoatPlayers                 = GetCountOfPlayers(GetIGCMission()->GetSide(SIDE_TEAMLOBBY), false); //Imago #169

  // if there might be a chance a player can join, check the teams to see if they can
  if ((m_misdef.misparms.bAllowJoiners || STAGE_NOTSTARTED == GetStage())
      && !m_misdef.misparms.bLockLobby && STAGE_OVER != GetStage())
  {
    const SideListIGC * plistSide = m_pMission->GetSides();
    for (SideLinkIGC * plinkSide = plistSide->first(); plinkSide; plinkSide = plinkSide->next())
    {
      IsideIGC * pside = plinkSide->data();
      int nNumPlayersOnSide = GetCountOfPlayers(pside, false);
      int nAvailablePositions = m_misdef.misparms.nMaxPlayersPerTeam - nNumPlayersOnSide;

      // if it is possible to join this side...
      if ((STAGE_NOTSTARTED == GetStage() || (GetBase(pside) && pside->GetActiveF()))
          && nAvailablePositions && STAGE_OVER != GetStage())
      {
        // track that
        pfmLobbyMissionInfo->fAnySlotsAvailable = true;

        // if a player is guaranteed to get in, track that too.
        if (GetAutoAccept(pside)
            || (m_misdef.fAutoAcceptLeaders && !GetLeader(pside->GetObjectID())))
        {
          pfmLobbyMissionInfo->fGuaranteedSlotsAvailable = true;
        }
      }
    }
  }

  pfmLobbyMissionInfo->nTeams                   = m_misdef.misparms.nTeams;
  pfmLobbyMissionInfo->nMinRank                 = m_misdef.misparms.iMinRank;
  pfmLobbyMissionInfo->nMaxRank                 = m_misdef.misparms.iMaxRank;
  pfmLobbyMissionInfo->nMaxPlayersPerGame       = min(m_misdef.misparms.nTotalMaxPlayersPerGame,
                                                    m_misdef.misparms.nTeams
                                                        * m_misdef.misparms.nMaxPlayersPerTeam);
  pfmLobbyMissionInfo->nMinPlayersPerTeam       = m_misdef.misparms.nMinPlayersPerTeam;
  pfmLobbyMissionInfo->nMaxPlayersPerTeam       = m_misdef.misparms.nMaxPlayersPerTeam;
  pfmLobbyMissionInfo->fInProgress              = m_misdef.fInProgress;
  pfmLobbyMissionInfo->fCountdownStarted        = (GetStage() == STAGE_STARTING
                                                  || (GetStage() == STAGE_NOTSTARTED && m_misdef.misparms.bAutoRestart));
  pfmLobbyMissionInfo->fMSArena                 = m_misdef.misparms.bObjectModelCreated;
  pfmLobbyMissionInfo->fScoresCount             = m_misdef.misparms.bScoresCount;
  pfmLobbyMissionInfo->fInvulnerableStations    = m_misdef.misparms.bInvulnerableStations;
  pfmLobbyMissionInfo->fAllowDevelopments       = m_misdef.misparms.bAllowDevelopments;
  pfmLobbyMissionInfo->fLimitedLives            = m_misdef.misparms.iLives != 0x7fff;
  pfmLobbyMissionInfo->fConquest                = m_misdef.misparms.IsConquestGame();
  pfmLobbyMissionInfo->fDeathMatch              = m_misdef.misparms.IsDeathMatchGame();
  pfmLobbyMissionInfo->fCountdown               = m_misdef.misparms.IsCountdownGame();
  pfmLobbyMissionInfo->fProsperity              = m_misdef.misparms.IsProsperityGame();
  pfmLobbyMissionInfo->fArtifacts               = m_misdef.misparms.IsArtifactsGame();
  pfmLobbyMissionInfo->fFlags                   = m_misdef.misparms.IsFlagsGame();
  pfmLobbyMissionInfo->fTerritorial             = m_misdef.misparms.IsTerritoryGame();
  pfmLobbyMissionInfo->fSquadGame               = m_misdef.misparms.bSquadGame;
  pfmLobbyMissionInfo->fEjectPods               = m_misdef.misparms.bEjectPods;
}

/*-------------------------------------------------------------------------
 * SendMissionInfo
 *-------------------------------------------------------------------------
 * Purpose:
 *    Send the player all the data they need to play in the mission.
 *    This is *different* from the info that just defined the mission on the mission board
 *
 * Parameters:
 *    Who the junk goes to (if pfsPlayer is NULL, send it to the entire side)
 */
void CFSMission::SendMissionInfo(CFSPlayer * pfsPlayer, IsideIGC*   pside)
{
    ImissionIGC * pMission = GetIGCMission();

    g.fm.SetDefaultRecipient(pfsPlayer ?
                                (CFMRecipient*) pfsPlayer->GetConnection() :
                                (CFMRecipient*) CFSSide::FromIGC(pside)->GetGroup(),
                             FM_GUARANTEED);

    SideID  sideID = pside->GetObjectID();


    // Send all clusters, and what that side sees in them
    const ClusterListIGC * pclstlist = pMission->GetClusters();
    ClusterLinkIGC * pclstlink;
    for (pclstlink = pclstlist->first(); pclstlink; pclstlink = pclstlink->next())
    {
        IclusterIGC * pcluster = pclstlink->data();
        ExportObj(pcluster, OT_cluster, NULL);

        // Export alephs
        {
            const WarpListIGC * pwarplist = pcluster->GetWarps();
            WarpLinkIGC * pwarplink;
            for (pwarplink = pwarplist->first(); pwarplink; pwarplink = pwarplink->next())
            {
              if (pwarplink->data()->SeenBySide(pside))
                ExportObj(pwarplink->data(), OT_warp, NULL);
            }

            // Export aleph bombs (get all alephs first so all alephs have destinations)
            for (pwarplink = pwarplist->first(); pwarplink; pwarplink = pwarplink->next())
            {
                const WarpBombList* plist = pwarplink->data()->GetBombs();
                if (plist->first() && (pwarplink->data()->SeenBySide(pside)))
                for (WarpBombLink* p = plist->first(); (p != NULL); p = p->next())
                {
                    BEGIN_PFM_CREATE(g.fm, pfmWB, S, WARP_BOMB)
                    END_PFM_CREATE
                    pfmWB->timeExplosion = p->data().timeExplosion;
                    pfmWB->warpidBombed = pwarplink->data()->GetObjectID();
                    pfmWB->expendableidMissile = p->data().pmt->GetObjectID();
                }
            }
        }

        // Export planets
        const AsteroidListIGC * pmdllist = pcluster->GetAsteroids();
        AsteroidLinkIGC * pmdllink;
        for (pmdllink = pmdllist->first(); pmdllink; pmdllink = pmdllink->next())
        {
          if (pmdllink->data()->SeenBySide(pside))
          {
            ExportObj(pmdllink->data(), OT_asteroid, NULL);
            IbuildingEffectIGC* pbe = pmdllink->data()->GetBuildingEffect();
            if (pbe)
                ExportObj(pbe, OT_buildingEffect, NULL);
          }
        }

        // Export stations
        const StationListIGC * pstnlist = pcluster->GetStations();
        StationLinkIGC * pstnlink;
        for (pstnlink = pstnlist->first(); pstnlink; pstnlink = pstnlink->next())
        {
          if (pstnlink->data()->SeenBySide(pside))
            ExportObj(pstnlink->data(), OT_station, NULL);
        }

        // Export treasure
        const TreasureListIGC * ptlist = pcluster->GetTreasures();
        TreasureLinkIGC * ptlink;
        for (ptlink = ptlist->first(); ptlink; ptlink = ptlink->next())
        {
          if (ptlink->data()->SeenBySide(pside))
            ExportObj(ptlink->data(), OT_treasure, NULL);
        }

        // Export mines
        const MineListIGC * pmlist = pcluster->GetMines();
        MineLinkIGC * pmlink;
        for (pmlink = pmlist->first(); pmlink; pmlink = pmlink->next())
        {
          if (pmlink->data()->SeenBySide(pside))
            ExportObj(pmlink->data(), OT_mine, NULL);
        }

        // Export probes
        const ProbeListIGC * pplist = pcluster->GetProbes();
        ProbeLinkIGC * pplink;
        for (pplink = pplist->first(); pplink; pplink = pplink->next())
        {
          if (pplink->data()->SeenBySide(pside))
            ExportObj(pplink->data(), OT_probe, NULL);
        }
    }



    //For the player's side, update the money & completion state of all buckets
    BEGIN_PFM_CREATE(g.fm, pfmCreateBuckets, S, CREATE_BUCKETS)
    END_PFM_CREATE
    pfmCreateBuckets->ttbmDevelopments = pside->GetDevelopmentTechs();
    pfmCreateBuckets->ttbmInitial = pside->GetInitialTechs();

    {
        for (BucketLinkIGC* pbl = pside->GetBuckets()->first(); (pbl != NULL); pbl = pbl->next())
        {
          IbucketIGC* pbucket = pbl->data();
          {
              BEGIN_PFM_CREATE(g.fm, pfmBucketStatus, S, BUCKET_STATUS)
              END_PFM_CREATE
              pfmBucketStatus->timeTotal = pbucket->GetTime();
              pfmBucketStatus->moneyTotal = pbucket->GetMoney();
              pfmBucketStatus->iBucket    = pbucket->GetObjectID();
              pfmBucketStatus->sideID     = sideID;
          }
        }
    }

    //Tell the player the money & autodonate for every other person on the side
    for (ShipLinkIGC*   psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
    {
        IshipIGC*   ps = psl->data();
        if (!pfsPlayer || ps != pfsPlayer->GetIGCShip())
        {
            CFSShip* pcs = (CFSShip *)(ps->GetPrivateData());
            if (pcs->IsPlayer())
            {
                CFSPlayer*  pcp = pcs->GetPlayer();
                IshipIGC*   pshipDonate = ps->GetAutoDonate();

                ShipID   shipID = ps->GetObjectID();

                BEGIN_PFM_CREATE(g.fm, pfmDonate, S, AUTODONATE)
                END_PFM_CREATE
                pfmDonate->sidDonateTo = pshipDonate ? pshipDonate->GetObjectID() : NA;
                pfmDonate->sidDonateBy = shipID;
                pfmDonate->amount      = 0;

                BEGIN_PFM_CREATE(g.fm, pfmMoney, S, SET_MONEY)
                END_PFM_CREATE
                pfmMoney->shipID = shipID;
                pfmMoney->money = pcp->GetMoney();
            }
        }
    }

    // send the data on all known ships
    {

        const ShipListIGC * plistShip = m_pMission->GetShips();

        for (ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
        {
            IshipIGC* pship = plinkShip->data();
            if (pship->GetSide()->GetObjectID() >= 0)
            {
                CFSShip* pcs = (CFSShip *)(pship->GetPrivateData());
                ShipStatus* pss = pcs->GetShipStatus(sideID);

                //We know something about the ship, even if it is obsolete info
                BEGIN_PFM_CREATE(g.fm, pfmShipStatus, S, SHIP_STATUS)
                END_PFM_CREATE
                pfmShipStatus->shipID = pship->GetObjectID();
                pfmShipStatus->status = *pss;
            }
        }
    }
    {
        //Broadcast the victory rankings to all sides
        BEGIN_PFM_CREATE(g.fm, pfmGameState, S, GAME_STATE)
        END_PFM_CREATE
        SideID  sid = 0;
        for (SideLinkIGC*   l = m_pMission->GetSides()->first(); (l != NULL); l = l->next())
        {
            assert (sid == l->data()->GetObjectID());
            pfmGameState->conquest[sid] = l->data()->GetConquestPercent();
            pfmGameState->territory[sid] = l->data()->GetTerritoryCount();
            pfmGameState->nFlags[sid] = l->data()->GetFlags();
            pfmGameState->nArtifacts[sid++] = l->data()->GetArtifacts();
        }
    }

    g.fm.SendMessages(NULL, FM_GUARANTEED, FM_FLUSH); // default recipient

}


CFSMission * CFSMission::GetMission(DWORD dwCookie)
{
  for (LinkFSMission * plinkFSMis = s_list.first(); plinkFSMis; plinkFSMis = plinkFSMis->next())
  {
    CFSMission * pfsMission = plinkFSMis->data();
    if (pfsMission->GetCookie() == dwCookie)
      return pfsMission;
  }
  return NULL;
}


CFSMission * CFSMission::GetMissionFromIGCMissionID(DWORD dwIGCMissionID)
{
  for (LinkFSMission * plinkFSMis = s_list.first(); plinkFSMis; plinkFSMis = plinkFSMis->next())
  {
    CFSMission * pfsMission = plinkFSMis->data();
    if (pfsMission->m_pMission->GetMissionID() == dwIGCMissionID)
      return pfsMission;
  }
  return NULL;
}



/*-------------------------------------------------------------------------
 * FAllReady
 *-------------------------------------------------------------------------
 * Purpose:
 *    See if all teams are ready, and if so, tell them to start
 */
bool CFSMission::FAllReady()
{
    {
        //Not everyone is ready if sides are imbalanced
        int   minPlayers;
        int   maxPlayers;
		int   minTeamRank = 1000000; // TE: Added for rank balancing
		int   maxTeamRank = 1;       // TE: Added for rank balancing

        SideLinkIGC*   psl = m_pMission->GetSides()->first();
        assert (psl);
		// KGJV: fix initial values
        minPlayers = m_misdef.misparms.nMaxPlayersPerTeam; // or anything 'big' enough
		maxPlayers = 0;//psl->data()->GetShips()->n();
        while (true)
        {
            int n = psl->data()->GetShips()->n();

			// KGJV #62 AllowEmptyTeams
			// KGJV: fix to include 1st team
			bool bSkipTeam = false;
			if (m_misdef.misparms.bAllowEmptyTeams)
				if (n==0 && !m_misdef.rgfActive[psl->data()->GetObjectID()])
					bSkipTeam = true; // skip inactive & empty teams

			if (!bSkipTeam)
			{
            if (n < minPlayers)
                minPlayers = n;

            if (n > maxPlayers)
                maxPlayers = n;

			// TE: Remember highest/lowest rank
			int r = GetSideRankSum(psl->data(), false);

			if (r < minTeamRank)
				minTeamRank = r;

			if (r > maxTeamRank)
				maxTeamRank = r;
			}
			// KGJV: moved here to include 1st team in the loop
			psl = psl->next();
            if (psl == NULL)
                break;
        }

		int threshold = GetRankThreshold();

        if ((minPlayers < m_misdef.misparms.nMinPlayersPerTeam) ||
            (maxPlayers > m_misdef.misparms.nMaxPlayersPerTeam) ||
            (minPlayers + m_misdef.misparms.iMaxImbalance < maxPlayers) ||
			((m_misdef.misparms.iMaxImbalance == 0x7ffe) && (maxTeamRank - minTeamRank) > threshold))	// TE: Add check for rank balancing if it's on
			// mmf changed to MaxImbalance
            return false;
		
	return true;
    }

    SideID iSide = m_misdef.misparms.nTeams;

	// KGJV #62 AllowEmptyTeams - not active + AllowEmptyTeams = team ready
	while (iSide-- > 0 && (
		GetReady(iSide) ||
		(m_misdef.misparms.bAllowEmptyTeams && (!m_misdef.rgfActive[iSide]))
		));
    ;

    return iSide < 0;
}


/*-------------------------------------------------------------------------
 * PickNewSide
 *-------------------------------------------------------------------------
 * Purpose:
 *    pick an available side for a new player (may return SIDE_TEAMLOBBY)
 */
SideID CFSMission::PickNewSide(CFSPlayer* pfsPlayer, bool bAllowTeamLobby, unsigned char bannedSideMask)
{
  // KGJV - set everyone in lobby by default
  // return SIDE_TEAMLOBBY; // TE: Commented to allow balancing to work

  if (bAllowTeamLobby && (GetStage() > STAGE_NOTSTARTED))
    return SIDE_TEAMLOBBY;

  IsideIGC * psideFewestPlayers = NULL;
  IsideIGC * psideLowestRank = NULL;
  int nFewestPlayers = 2147483646; // TE: Records the #players on the smallest team; Initialize to very high number
  int nLowestRank = 2147483646;	// TE: Records the lowest rank of a team; Initialize to very high number
  int nNumPlayers = -1;
  int nRankSum = -1;

  // find the side with the most positions free that will automatically accept the player
  const SideListIGC * plistSide = m_pMission->GetSides();
  for (SideLinkIGC * plinkSide = plistSide->first(); plinkSide; plinkSide = plinkSide->next())
  {
    IsideIGC*   pside = plinkSide->data();
	// KGJV : fix for deactivated teams
	if (m_misdef.rgfActive[pside->GetObjectID()])
    if ((SideMask(pside) & bannedSideMask) == 0)
    {
      nNumPlayers = GetCountOfPlayers(pside, false); // TE: Retrieve #players of this side
      nRankSum = GetSideRankSum(pside, false);	// TE: Retrieve the total rank of this side

      if (CheckPositionRequest(pfsPlayer, pside) == NA)	// they are allowed to join
      // TE: Removed AutoAccept check (since it's disabled before this runs) and comm check
      {
       // Flag this team as the lowest-ranked team if they are
        if (nRankSum < nLowestRank)
        {
          psideLowestRank = pside;
          nLowestRank = nRankSum;
        }

        // Flag this team as the most-open team if it is
        if (nNumPlayers < nFewestPlayers)
        {
          psideFewestPlayers = pside;
          nFewestPlayers = nNumPlayers;
        }
      }
    }
  }

  // We now have the side with the lowest rank, and the side with the fewest players.

  // Default newside to lowest ranked side
  IsideIGC * psideNewSide = psideLowestRank;

  // TE: If player imbalance is on, enforce it as necessary
  // mmf added check for nImbalance of 7fffe for auto (balance)
  //
  int nImbalance = m_misdef.misparms.iMaxImbalance;
  if (nImbalance != 0x7fff && nImbalance != 0x7fff && psideNewSide != NULL)
  {
	  int nRankedTeamPlayers = GetCountOfPlayers(psideNewSide, false);

	  if (nRankedTeamPlayers + 1 > nFewestPlayers + nImbalance)
	  {
		  // Override rank choice with imbalance choice
		  psideNewSide = psideFewestPlayers;
	  }
  }
  // place suggest that side, or the team lobby if no side was found that would accept them
  return psideNewSide
         ? psideNewSide->GetObjectID()
         : SIDE_TEAMLOBBY;
}


/*-------------------------------------------------------------------------
 * PickNewCiv
 *-------------------------------------------------------------------------
 * Purpose:
 *    pick a civ for a new team, preferably one that is not in use
CivID   CFSMission::PickNewCiv(SideID nSides, CivID   rgCivs[])
{
    //Pick a random, legal, least used civ
    //Go over all civs and get the minimum count
    int nCivMin;
    int cCivMin = 0x7ffffff;

    const int   cCivsMax = 20;
    assert (m_pMission->GetCivilizations()->n() < cCivsMax);
    CivID   civIDs[cCivsMax];

    for (pcl = m_pMission->GetCivilizations()->first(); (pcl != NULL); pcl = pcl->next())
    {
        CivID   civID = pcl->data()->GetObjectID();

        //NYI hack to prevent special civs
        if (civID < 100)
        {
            int cCiv = 0;
            for (int i = nSides - 1; (i >= 0); i--)
            {
                if (civID == rgCivs[i])
                    cCiv++;
            }

            if (cCiv < cCivMin)
            {
                cCivMin = cCiv;
                civIDs[0] = civID;
                nCivMin = 0;
            }
            else if (cCiv == cCivMin)
            {
                civIDs[++nCivMin] = civID;
            }
        }
    }
    assert (cCivMin != 0x7ffffff);

    return civIDs[RandomInt(0, nCivMin)];
}
*/

DelPositionReqReason CFSMission::CheckPositionRequest(CFSPlayer * pfsPlayer, IsideIGC * pside)
{
  assert(pside);
  SideID sideID = pside->GetObjectID();
  const MissionParams*  pmp = m_pMission->GetMissionParams();
  IsideIGC* psideCurrent = pfsPlayer->GetSide();

  // mdvalley: Check for defects off, and side last flown not requested side.
  if ((!pmp->bAllowDefections) && (GetStage() == STAGE_STARTED)
	  && (sideID != pfsPlayer->GetLastSide()) && (pfsPlayer->GetLastSide() != SIDE_TEAMLOBBY))
	  return DPR_NoDefections;

  //Imago #12 6/10
  if ((!pmp->bAllowDefections) && (GetStage() == STAGE_STARTED) && (psideCurrent && psideCurrent->GetObjectID() != SIDE_TEAMLOBBY))
      return DPR_NoDefections;
    else if (pmp->bLockSides && sideID != SIDE_TEAMLOBBY) // TE: Added 2nd check to allow them to go to NOAT any time) --did you even try testing this? 
      return DPR_SidesLocked;

  if (sideID != SIDE_TEAMLOBBY)
  {
	 // KGJV fix: no rejoin to a deactivated team
    if (!m_misdef.rgfActive[sideID])
		return DPR_SideGone;

    if (GetStage() == STAGE_OVER)
      return DPR_ServerPaused;

    if (pfsPlayer->GetBannedSideMask() & SideMask(sideID))
      return DPR_Banned;

    if (RequiresInvitation() && !CFSSide::FromIGC(pside)->IsInvited(pfsPlayer))
      return DPR_PrivateGame;

    int nNumPlayers = GetCountOfPlayers(pside, false);
    int maxPlayers = pmp->nMaxPlayersPerTeam;

    //Can they join the chosen side?
    if ((STAGE_NOTSTARTED != GetStage()) && (pmp->iMaxImbalance != 0x7fff))
    {
      for (SideLinkIGC*  psl = m_pMission->GetSides()->first(); (psl != NULL); psl = psl->next())
      {
        if (psl->data()->GetActiveF())
        {
          int count = GetCountOfPlayers(psl->data(), false) + pmp->iMaxImbalance;
          if (count < maxPlayers)
            maxPlayers = count;
        }
      }
    }

    if (STAGE_NOTSTARTED != GetStage())
    {
      if (!GetBase(pside))
        return DPR_NoBase;
      else if (!pside->GetActiveF())
        return DPR_SideDefeated;
    }
	
	if (GetCountOfPlayers(pside, false) >= pmp->nMaxPlayersPerTeam)
		return DPR_TeamFull;
    else if ((nNumPlayers >= maxPlayers) &&//Xynth #166 7/2010 Add condition to let low rank players join stack
			 !((pfsPlayer->GetPersistPlayerScore(NA)->GetRank() <= g.MaxNewbRank) && (nNumPlayers < (maxPlayers + 2))))
			//If the player is low rank (<4) and there aren't more than 2 extra players, let him join (Imago made this configurable #174)
		return DPR_TeamBalance;

	// TE: Can they join chosen side based on rank? mmf changed to MaxImbalance
	if ((STAGE_NOTSTARTED != GetStage()) && (pmp->iMaxImbalance == 0x7ffe))
	{
		int nRequestedSideRank = GetSideRankSum(pside, false);
		int nHighestTeamRank = 0;
		int nLowestTeamRank = 2147483646;

		// Grab lowest and highest teamranks
		for (SideLinkIGC*  psl = m_pMission->GetSides()->first(); (psl != NULL); psl = psl->next())
		{
			IsideIGC*   pTempSide = psl->data();
			if (pTempSide->GetActiveF())
			{
				int nTempSideRank = GetSideRankSum(pTempSide, false);

				// Store lowest teamrank
				if (nTempSideRank < nLowestTeamRank)
					nLowestTeamRank = nTempSideRank;

				// Store highest teamrank
				if (nTempSideRank > nHighestTeamRank)
					nHighestTeamRank = nTempSideRank;
			}
		}
		// mmf go with simple method of only allowing new players on the side with the lowest rank
		if (nRequestedSideRank != nLowestTeamRank)
			return DPR_TeamBalance;

		// mmf disable this for now with if 0
		// TODO: revisit this
		//   Perhaps just select the team for them when they click join, so instead of rejecting
		//   them from the selected team stick them on the proper team.
		//
		//   The below is bugged in that if the difference between teams gets too large
		//   no one can join either side.  Also newbstack is too restrictive also frequently resulting
		//   in players ranked 5,6,7,8 not being able to join either side.
		//
		//   Fix the below.  Why? so instead of forcing them on a side they may not want to be
		//   on allow them on if the ranks are close enough.
#if 0
		int nPlayerRank = pfsPlayer->GetPersistPlayerScore(NA)->GetRank();
		int nRankThreshold = GetRankThreshold();

		// Prevent joining a team if it will put it over the threshold
		int nNewSideRank = nRequestedSideRank + nPlayerRank;
		if (nNewSideRank - nLowestTeamRank > nRankThreshold) {
			// mmf log this
			debugf("join blocked would unbalance sides Rank=%d, NewSideRank=%d, RequestedSideRank=%d, LowestTeamRank=%d, RankThreshold=%d\n",
				nPlayerRank,nNewSideRank,nRequestedSideRank,nLowestTeamRank,nRankThreshold);
			return DPR_TeamBalance;
		}

		// Prevent joining a team if a newbie is trying to newbstack
		if (nPlayerRank <= 8 && nRequestedSideRank == nLowestTeamRank)
		{
			int nDifference = nHighestTeamRank - nRequestedSideRank;
			if (nDifference > nRankThreshold) {
				// mmf log this
				debugf("join blocked (newbstack) Rank=%d, Difference=%d, RequestedSideRank=%d, HighestTeamRank=%d, RankThreshold=%d\n",
				nPlayerRank,nDifference,nRequestedSideRank,nHighestTeamRank,nRankThreshold);
				return DPR_TeamBalance;
			}
		}
#endif
	}

    if (m_misdef.misparms.bSquadGame)
    {
      if (nNumPlayers > 0)
      {
        if (!pfsPlayer->GetIsMemberOfSquad(pside->GetSquadID()))
          return DPR_WrongSquad;
      }
      else
      {
        if (pfsPlayer->GetPreferredSquadToLead() == NA)
          return DPR_CantLeadSquad;
      }
    }
  }

  return (DelPositionReqReason)NA;
}

void CFSMission::RequestPosition(CFSPlayer * pfsPlayer, IsideIGC * pside, bool bRejoin)
{
  assert(pside);
  SideID sideID = pside->GetObjectID();
  const MissionParams*  pmp = m_pMission->GetMissionParams();

  DelPositionReqReason reason = CheckPositionRequest(pfsPlayer, pside);

  if (reason != NA)
  {
    //deny the request
    BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
    END_PFM_CREATE
    pfmDelPosReq->shipID    = pfsPlayer->GetShipID();
    pfmDelPosReq->iSide     = sideID;
    pfmDelPosReq->reason    = reason;

    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
    return;
  }

  if (pfsPlayer->GetSide()->GetObjectID() != SIDE_TEAMLOBBY)
  {
    RemovePlayerFromSide(pfsPlayer, QSR_SwitchingSides);
  }

  if (sideID != SIDE_TEAMLOBBY)
  {
    if ((!GetLeader(sideID) && m_misdef.fAutoAcceptLeaders)
        || bRejoin
        || m_misdef.rgfAutoAccept[sideID])
    {
      AddPlayerToSide(pfsPlayer, pside);
    }
    else // need permission
    {
      BEGIN_PFM_CREATE(g.fm, pfmSPositionReq, S, POSITIONREQ)
      END_PFM_CREATE
      pfmSPositionReq->shipID    = pfsPlayer->GetShipID();
      pfmSPositionReq->iSide     = sideID;
      g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

      AddJoinRequest(pfsPlayer, pside);
    }
  }
}


/*-------------------------------------------------------------------------
 * VacateStation
 *-------------------------------------------------------------------------
 * Purpose:
 *    Kick everyone out of a station, except the people on the owning side
 */
void CFSMission::VacateStation(IstationIGC * pstation)
{
    assert (pstation->GetMission() == m_pMission);

    IsideIGC*   psideGhost = NULL;

    //Everyone goes (in the event of a capture: the person capturing has not docked yet)
    const ShipListIGC*  pships = pstation->GetShips();
    ShipLinkIGC*        pshiplink = pships->first();
    while (pshiplink)
    {
        IshipIGC*   pship = pshiplink->data();
        if (pship->GetParentShip() == NULL)
        {
            if (pship->IsGhost())
            {
                //Ghosts ... move the ghosts in the next pass
                psideGhost = pship->GetSide();
                pshiplink = pshiplink->next();
            }
            else
            {
                //Force any kids who haven't undocked to undock
                for (ShipLinkIGC*   psl = pship->GetChildShips()->first();
                   (psl != NULL);
                   psl = psl->next())
                {
                  psl->data()->SetStation(NULL);
                }

                pship->SetStation(NULL);

                CFSShip*    pfsship = (CFSShip*)(pship->GetPrivateData());
                if (pfsship->IsPlayer())
                    pfsship->GetPlayer()->ForceLoadoutChange();

                pshiplink = pships->first();
            }

        }
        else
            pshiplink = pshiplink->next();
    }

    if (psideGhost)
    {
        //Find an alternate base for the ghosts ... if they don't have one, don't worry: side
        //will be eliminated
        for (StationLinkIGC*    psl = psideGhost->GetStations()->first(); (psl != NULL); psl = psl->next())
        {
            IstationIGC*    ps = psl->data();
            if ((ps != pstation) && ps->GetBaseStationType()->HasCapability(c_sabmRestart))
            {
                ShipLinkIGC*        pshiplink = pships->first();
                while (pshiplink)
                {
                    IshipIGC*   pship = pshiplink->data();
                    pshiplink = pshiplink->next();

                    assert (pship->GetParentShip() == NULL);
                    assert (pship->IsGhost());
                    pship->SetStation(ps);
                }
                break;
            }
        }
    }
}


/*-------------------------------------------------------------------------
 * AddJoinRequest
 *-------------------------------------------------------------------------
 * Purpose:
 *    When a player joins a team that has auto-accept off (or other blockers,
 *    such as not having a civ yet), a player is added to the join request list.
 */
void CFSMission::AddJoinRequest(CFSPlayer * pfsPlayer, IsideIGC * pside)
{
  JoinRequest * pjr = new JoinRequest;
  pjr->pfsPlayer    = pfsPlayer;
  pjr->pSide        = pside;
  m_listJoinReq.last(pjr);

  // must go through the lobby to change missions
  assert(pfsPlayer->GetMission() == this);
}


/*-------------------------------------------------------------------------
 * NotifyPlayerBoot
 *-------------------------------------------------------------------------
 * Purpose:
 *    Mark a player as off the side, both the remote player and the local igc player
 *
 * Paremeters:
 *    dwSendBootTo as defined next to declaration of this function in fsmission.h
 */
void CFSMission::NotifyPlayerBoot(CFSPlayer * pfsPlayer, IsideIGC * pSide)
{
  // Forward the remove request to everyone
  BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
  END_PFM_CREATE
  pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
  pfmDelPosReq->iSide      = pSide->GetObjectID();
  pfmDelPosReq->reason     = DPR_Rejected;

  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
}


/*-------------------------------------------------------------------------
 * RejectSideJoinRequests
 *-------------------------------------------------------------------------
 * Purpose:
 *    Any pending requests on a side are rejected
 *
 * Side Effects:
 *    Those people are given the boot
 */
bool CFSMission::RejectSideJoinRequests(IsideIGC * pSide)
{
  bool fAny = false;
  // Note: almost the same as loop in SetAutoAccept--maybe should combine 'em
  LinkJoinReq*  plinkNext;
  for (LinkJoinReq* plinkJR = m_listJoinReq.first(); (plinkJR != NULL); plinkJR = plinkNext)
  {
    plinkNext = plinkJR->next();

    JoinRequest * pjr = plinkJR->data();
    if (pjr->pSide == pSide)
    {
      NotifyPlayerBoot(pjr->pfsPlayer, pjr->pSide);
      delete plinkJR;
      delete pjr;
      fAny = true;
    }
  }
  return fAny;
}


/*-------------------------------------------------------------------------
 * RemoveJoinRequest
 *-------------------------------------------------------------------------
 * Purpose:
 *    Remove all stored requests for specified player
 *    A player comes off the join list when they are added to a team, or log off
 *
 * Parameters:
 *    pfsPlayer: Player to remove, or NULL for all players
 *    psideDest: Side the player is going to
 *
 * Returns: whether there were any requests to remove
 */
bool CFSMission::RemoveJoinRequest(CFSPlayer * pfsPlayer, IsideIGC * psideDest)
{
  bool fAny = false;
  // Note: almost the same as loop in SetAutoAccept--maybe should combine 'em
  LinkJoinReq*  plinkNext;
  for (LinkJoinReq* plinkJR = m_listJoinReq.first(); (plinkJR != NULL); plinkJR = plinkNext)
  {
    plinkNext = plinkJR->next();

    JoinRequest * pjr = plinkJR->data();
    if (pjr->pfsPlayer == pfsPlayer || !pfsPlayer)
    {
      // Let's tell everyone that the request is gone if the player isn't going to a team
      if (!psideDest)
        NotifyPlayerBoot(pjr->pfsPlayer, pjr->pSide);

      delete plinkJR;
      delete pjr;
      fAny = true;
    }
  }
  return fAny;
}


/*-------------------------------------------------------------------------
 * SetAutoAccept
 *-------------------------------------------------------------------------
 * Purpose:
 *    Turn auto-accept on or off for a side.
 *    Slightly bastardized use: pass NULL for pside and false for fAccept to
 *      reject all pending requests on all sides
 *
 * Side Effects:
 *    If turning it on, any requests on the side are accepted.
 */
void CFSMission::SetAutoAccept(IsideIGC * pside, bool fAccept)
{
  assert(IMPLIES(!pside, !fAccept));
  SideID sideID = pside ? pside->GetObjectID() : NA;
  if (pside)
    m_misdef.rgfAutoAccept[sideID] = fAccept;
  // else the mission is going away altogether, so it doesn't matter

  if (pside && fAccept || !pside)
  {
    LinkJoinReq* plinkNext;
    for (LinkJoinReq* plinkJR = m_listJoinReq.first(); (plinkJR != NULL); plinkJR = plinkNext)
    {
      plinkNext = plinkJR->next();

      JoinRequest * pjr = plinkJR->data();
      if (pjr->pSide == pside || !pside)
      {
        delete plinkJR;
        // If anyone has more than one request in, then they'll get more than one side change.
        // That shouldn't happen.
        CFSPlayer* pfsPlayer = pjr->pfsPlayer;
        delete pjr;

        if (pside)
        {
          DelPositionReqReason reason = CheckPositionRequest(pfsPlayer, pside);  //Xynth #195 8/2010 Look to see if this request is going to fail
		  		
		  // should never add the player back to the list, since autoaccept is on
          RequestPosition(pfsPlayer, pside, false);
		  
		  if (reason != NA)  //Xynth #195 8/2010 If rejected do a formal request to join the lobby
			  RequestPosition(pfsPlayer, pfsPlayer->GetMission()->GetIGCMission()->GetSide(SIDE_TEAMLOBBY), false);
        }
      }
    }
  }
}


/*-------------------------------------------------------------------------
 * SetLockLobby
 *-------------------------------------------------------------------------
 * Purpose:
 *    Lock/unlock the lobby for a game.
 *
 * Notes:
 *    Does not send the lock lobby message to the clients here, so you have
 *    to do it from the call site.  Should we change this?
 */
void CFSMission::SetLockLobby(bool bLock)
{
  m_misdef.misparms.bLockLobby = bLock;
  m_pMission->SetMissionParams(&m_misdef.misparms);
  SetLobbyIsDirty();
}

/*-------------------------------------------------------------------------
 * SetLockSides
 *-------------------------------------------------------------------------
 * Purpose:
 *    Lock/unlock the Sides for a game.
 *
 * Notes:
 *    Does not send the lock Sides message to the clients here, so you have
 *    to do it from the call site.  Should we change this?
 */
void CFSMission::SetLockSides(bool bLock)
{
  m_misdef.misparms.bLockSides = bLock;
  m_pMission->SetMissionParams(&m_misdef.misparms);
}

/* mmf added this function to support #autobalance chat command
 *-------------------------------------------------------------------------
 * SetMaxTeamImbalance
 *-------------------------------------------------------------------------
 * Purpose:
 *    Set the MaxTeamImbalance game parameter
 *
 * Notes:
 *    0x7ffe is 'auto' balance, 0x7fff is N/A
 */
void CFSMission::SetMaxTeamImbalance(int imbalance)
{
  m_misdef.misparms.iMaxImbalance = imbalance;
  m_pMission->SetMissionParams(&m_misdef.misparms);
}

/*-------------------------------------------------------------------------
 * FlushSides
 *-------------------------------------------------------------------------
 * Purpose:
 *    Removes all players (except for commanders) from all sides and puts them inn the team lobby
 */
void CFSMission::FlushSides()
{
  assert(GetStage() == STAGE_NOTSTARTED);

  // turn on auto accept for all sides
  // KGJV: changed to turn on auto accept off
  {
    for (SideLinkIGC* psl = m_pMission->GetSides()->first(); psl != NULL; psl = psl->next())
    {
      if (GetAutoAccept(psl->data()))
      {
        SetAutoAccept(psl->data(), false);
        BEGIN_PFM_CREATE(g.fm, pfmAutoAccept, CS, AUTO_ACCEPT)
        END_PFM_CREATE
        pfmAutoAccept->iSide = psl->data()->GetObjectID();
        pfmAutoAccept->fAutoAccept = false;
        g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_DONT_FLUSH);
      }
    }
    g.fm.PurgeOutBox();
  }

  // move everyone who's not a team leader to the lobby side.
  {
    const ShipListIGC*      pships = m_pMission->GetShips();
    ShipLinkIGC* pshipLinkNext;
    for (ShipLinkIGC* pshipLink = pships->first();
         (pshipLink != NULL);
         pshipLink = pshipLinkNext)
    {
      pshipLinkNext = pshipLink->next();
      CFSPlayer* pfsPlayer = ((CFSShip*)(pshipLink->data()->GetPrivateData()))->GetPlayer();

      SideID sideID = pfsPlayer->GetSide()->GetObjectID();

      // clear their banned side mask (it simplifies life)
      pfsPlayer->SetBannedSideMask(0);

	  // if they are not already on the lobby side and not a team leader
      if (sideID != SIDE_TEAMLOBBY && GetLeader(sideID) != pfsPlayer)
      {
        RemovePlayerFromSide(pfsPlayer, QSR_FlushSides);
        // mdvalley: empty last side masks
        pfsPlayer->SetLastSide(SIDE_TEAMLOBBY);

      }
    }
  }

  g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
}

 /*-------------------------------------------------------------------------
 * TE: GetLockSides
 *-------------------------------------------------------------------------
 * Purpose:
 *    Retrieve the status of the LockSides game parameter
 */
bool CFSMission::GetLockSides()
{
  return m_misdef.misparms.bLockSides;
}

/*-------------------------------------------------------------------------
 * RandomizeSides (TE: Now works like "BalanceSides")
 *-------------------------------------------------------------------------
 * Purpose:
 *    [Assigns everyone in the mission to a random team except the team leaders] --MS
 *    Evenly distributes all non-afk players to all available teams --TE
 */
void CFSMission::RandomizeSides()
{
  assert(GetStage() == STAGE_NOTSTARTED);

  // turn on auto accept for all sides
  // KGJV: changed to turn on auto accept off
  {
    for (SideLinkIGC* psl = m_pMission->GetSides()->first(); psl != NULL; psl = psl->next())
    {
      if (GetAutoAccept(psl->data()))
      {
        SetAutoAccept(psl->data(), false);
        BEGIN_PFM_CREATE(g.fm, pfmAutoAccept, CS, AUTO_ACCEPT)
        END_PFM_CREATE
        pfmAutoAccept->iSide = psl->data()->GetObjectID();
        pfmAutoAccept->fAutoAccept = false;
        g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_DONT_FLUSH);
      }
    }
    g.fm.PurgeOutBox();
  }

  // move everyone who's not a team leader to the lobby side.
  {
    const ShipListIGC*      pships = m_pMission->GetShips();
    ShipLinkIGC* pshipLinkNext;
    for (ShipLinkIGC* pshipLink = pships->first();
         (pshipLink != NULL);
         pshipLink = pshipLinkNext)
    {
      pshipLinkNext = pshipLink->next();
      CFSPlayer* pfsPlayer = ((CFSShip*)(pshipLink->data()->GetPrivateData()))->GetPlayer();

      SideID sideID = pfsPlayer->GetSide()->GetObjectID();

      // clear their banned side mask (it simplifies life)
      pfsPlayer->SetBannedSideMask(0);

	  // if they are not already on the lobby side and not a team leader
      if (sideID != SIDE_TEAMLOBBY && GetLeader(sideID) != pfsPlayer)
      {
        RemovePlayerFromSide(pfsPlayer, QSR_BalanceSides);
        // mdvalley: empty last side masks
        pfsPlayer->SetLastSide(SIDE_TEAMLOBBY);

	  }	  	 	  
    }
  }
  
  //Xynth #13 8/2010  Need to remove all join requests.
  LinkJoinReq* plinkNext;
  for (LinkJoinReq* plinkJR = m_listJoinReq.first(); (plinkJR != NULL); plinkJR = plinkNext)
  {
      plinkNext = plinkJR->next();
      JoinRequest * pjr = plinkJR->data();
	  RemoveJoinRequest(pjr->pfsPlayer, pjr->pSide);
  }

  // TE: Assign players to sides
  IsideIGC* psideLobby = m_pMission->GetSide(SIDE_TEAMLOBBY);
  const ShipListIGC* pshipsLobby = psideLobby->GetShips();

  bool bPickTeamFailed = false;
  CFSPlayer* pshipHighestRank = NULL;
  CFSPlayer* pshipTemp = NULL;

  // this is O(n^2), but I think n is small enough and it's used rarely
  // enough that it won't matter.

  // Loop through highest to lowest rank, assigning them to the team with the lowest rank
  while (pshipsLobby->n() > 0 && !bPickTeamFailed)
  {
    // Pick player with highest rank from Lobby
    {
      ShipLinkIGC* pshipLinkNext;
      pshipTemp = NULL;
      pshipHighestRank = NULL;

      for (ShipLinkIGC* pshipLink = pshipsLobby->first();
          (pshipLink != NULL);
           pshipLink = pshipLinkNext)
      {
          // Get the current player
			pshipLinkNext = pshipLink->next();
			pshipTemp = ((CFSShip*)(pshipLink->data()->GetPrivateData()))->GetPlayer();

			// Grab the current player's rank
			RankID rank = pshipTemp->GetPersistPlayerScore(NA)->GetRank();

			// Ignore AFK players
			if (pshipTemp->GetReady())
			{
				if (pshipHighestRank == NULL)
					pshipHighestRank = pshipTemp;

				// If current player's rank is higher than our current, remember highest rank
				if (pshipTemp->GetPersistPlayerScore(NA)->GetRank() > pshipHighestRank->GetPersistPlayerScore(NA)->GetRank())
					pshipHighestRank = pshipTemp;
			}
        }

        // pshipHighestRank is now the highest-ranked non-AFK player on NOAT
		// or it's NULL, meaning we've gone through all non-AFK players.
		// If we didn't find any non-AFK players on NOAT, we're done!
		if (pshipHighestRank == NULL)
		{
			bPickTeamFailed = true;
			break;
      }
    }

    // try to find a side to assign them to.
    SideID sideID = PickNewSide(pshipHighestRank, true, pshipHighestRank->GetBannedSideMask());

    if (sideID == SIDE_TEAMLOBBY)
      bPickTeamFailed = true;
    else
      AddPlayerToSide(pshipHighestRank, m_pMission->GetSide(sideID));
  }

  {
    //SetLockSides(true);  // TE: Commented this. Randomize should not touch sides!
    //BEGIN_PFM_CREATE(g.fm, pfmLockSides, CS, LOCK_SIDES)
    //END_PFM_CREATE
    //pfmLockSides->fLock = true;
    g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
  }
}

void CFSMission::SetSideCiv(IsideIGC * pside, IcivilizationIGC * pciv)
{
  pside->SetCivilization(pciv);
  CivID civID = pciv->GetObjectID();
  //m_misdef.rgCivID[pside->GetObjectID()] = civID;

  //Go through all players on the team and adjust their persist score objects
  // Their persist score now becomes their current score, and other stuff is reset
  for (ShipLinkIGC* psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
  {
      IshipIGC*     pship = psl->data();
      CFSShip*      pfsship = (CFSShip*)(pship->GetPrivateData());

      PlayerScoreObject* ppso = pfsship->GetPlayerScoreObject();
      ppso->SetPersist(pfsship->GetPlayer()->GetPersistPlayerScore(civID));
  }
}
//*-------------------------------------------------------------------------
//* Set & Get Allies - #ALLY
//*-------------------------------------------------------------------------
 void CFSMission::SetSideAllies(SideID sideid, char Allies)
{
	m_misdef.rgfAllies[sideid] = Allies;
}
 char CFSMission::GetSideAllies(SideID sideid)
 {
	 return m_misdef.rgfAllies[sideid];
 }

//*-------------------------------------------------------------------------
//* SetSideActive - KGJV #62
//*-------------------------------------------------------------------------
 void CFSMission::SetSideActive(SideID sideid, bool bActive)
{
	m_misdef.rgfActive[sideid] = bActive;
}
 bool CFSMission::GetSideActive(SideID sideid)
 {
	 return (bool)m_misdef.rgfActive[sideid];
 }
/*-------------------------------------------------------------------------
 * DeactivateSide
 *-------------------------------------------------------------------------
 * Purpose:
 *    Mark a side as inactive and tell everyone about it.
 *    Once a side is inactive, no one can join it
 *
 * Parameters:
 *    side to deactivate
 */
void CFSMission::DeactivateSide(IsideIGC * pside)
{
   // KGJV #62
  //if (!m_misdef.misparms.bAllowEmptyTeams)
  //{
      assert(pside->GetMission() == m_pMission);
      debugf("DeactivateSide side=%d.\n", pside->GetObjectID());
      pside->SetActiveF(false);
      SideID sideid = pside->GetObjectID();
      m_misdef.rgfActive[sideid]  =
      m_misdef.rgfReady[sideid]   = false;
      pside->SetTimeEndured(max(0.0f, Time::Now() - m_misdef.misparms.timeStart));

      //Eliminate all of the side's drones
      const ShipListIGC*    pships = pside->GetShips();
      {
        ShipLinkIGC*    pslNext;
        for (ShipLinkIGC*   psl = pships->first(); (psl != NULL); psl = pslNext)
        {
            pslNext = psl->next();
            IshipIGC*   pship = psl->data();

            // BT - 2/4/2012 - Fixing clint's server crash bug: Game with one player, set to 1 life, eject pods off, launch scout, #resign causes server crash.
            if(pship == NULL)
                break;

            if (pship->GetBaseHullType())
            {
                {
                    const PartListIGC*  parts = pship->GetParts();
                    PartLinkIGC*    plink;
                    while (plink = parts->first())  //Not ==
                        plink->data()->Terminate();
                }
                pship->SetAmmo(0);
                pship->SetFuel(0.0f);
            }

            if (pship->GetPilotType() < c_ptPlayer)
            {
                m_psiteIGC->KillShipEvent(g.timeNow, pship, NULL, 0.0f, pship->GetPosition(), Vector::GetZero());
            }
            else
            {
                if ((pship->GetParentShip() == NULL) && (pship->GetCluster() != NULL))
                    m_psiteIGC->KillShipEvent(g.timeNow, pship, NULL, -1.0f, pship->GetPosition(), Vector::GetZero());  //"Negative damage" == force killed

                pship->Reset(false);
                ((CFSShip*)(pship->GetPrivateData()))->ShipStatusExit();
            }
        }
      }

      BEGIN_PFM_CREATE(g.fm, pfmSideInactive, CS, SIDE_INACTIVE) // KGJV #62
      END_PFM_CREATE
      pfmSideInactive->sideID = pside->GetObjectID();
	  pfmSideInactive->bActive = false; // KGJV #62
	  pfmSideInactive->bChangeAET = false; // KGJV #62
      g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
      GetSite()->SendChatf(NULL, CHAT_EVERYONE, NA, NA, "%s is no more.", pside->GetName());


      bool bAllSidesInactive;
      IsideIGC* psideWin = CheckForVictoryByInactiveSides(bAllSidesInactive);
      if (bAllSidesInactive)
      {
          // if the game has not started yet, start it so that a side can win.
          if (GetStage() == STAGE_STARTING)
            StartGame();

          GameOver(NULL, "All sides were destroyed");
      }
      else if (psideWin)
      {
          // if the game has not started yet, start it so that a side can win.
          if (GetStage() == STAGE_STARTING)
            StartGame();

          static char szReason[256];     //Make this static so we only need to keep a pointer to it around

		  //make the allies team name list Imago ALLY 7/13/09
		  if (psideWin->GetAllies() != NA) {
			  ZString strWinningTeams;

				//lets get a list of allied sideIDs
			    for (SideLinkIGC* psidelink = pside->GetMission()->GetSides()->first();
					(psidelink != NULL);
					psidelink = psidelink->next())
				{
					IsideIGC*   otherside = psidelink->data();
					//this side is ally...and not ours
					if (pside->AlliedSides(psideWin,otherside) && otherside != psideWin)
						strWinningTeams += " & " + ZString(otherside->GetName());
				}
				strWinningTeams = psideWin->GetName() + strWinningTeams;
				sprintf(szReason, "%s won by outlasting all other sides.", (PCC)strWinningTeams);
				GameOver(psideWin, szReason);
		  } else {
	          sprintf(szReason, "%s won by outlasting all other sides.", psideWin->GetName());
			  GameOver(psideWin, szReason);
		  }
      }
      else if ((m_psideWon == NULL) && pships->n())
      {
          //Games not over for anyone but this team (only ships on the team will be players)
          CFMGroup* pgroup = CFSSide::FromIGC(pside)->GetGroup();
          g.fm.SetDefaultRecipient(pgroup, FM_GUARANTEED);
          QueueGameoverMessage();
          g.fm.SendMessages(pgroup, FM_GUARANTEED, FM_FLUSH);
      }

 // }
}

/*-------------------------------------------------------------------------
 * SetForceReady
 *-------------------------------------------------------------------------
 * Purpose:
 *    Set a side's force ready bit
 *
 * Side Effects:
 *    Ready state of side may change
 */
void CFSMission::SetForceReady(SideID iSide, bool fForceReady)
{
  m_misdef.rgfForceReady[iSide] = fForceReady;
  if (fForceReady)
    SetReady(iSide, true);
  else
    CheckForSideAllReady(m_pMission->GetSide(iSide));
}


/*-------------------------------------------------------------------------
 * SetReady
 *-------------------------------------------------------------------------
 * Purpose:
 *    Mark a side as ready or not. No checking of player ready states,
 *    force ready bits, etc. is done here. That's why it's private
 *
 * Side Effects:
 *    If side is now ready, game may start
 */
void CFSMission::SetReady(SideID iSide, bool fReady)
{
  if (fReady != GetReady(iSide))
  {
    BEGIN_PFM_CREATE(g.fm, pfmTeamReady, S, TEAM_READY)
    END_PFM_CREATE
    pfmTeamReady->iSide     = iSide;
    pfmTeamReady->fReady    =
      m_misdef.rgfReady[iSide] = fReady; // dbl assignment
    g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
  }
}


/*-------------------------------------------------------------------------
 * PlayerReadyChange
 *-------------------------------------------------------------------------
 * Purpose:
 *    Notification from a player that their ready status changed. This may
 *    change the ready status of the side
 *
 * Side Effects:
 *    A side's ready status may change
 */
void CFSMission::PlayerReadyChange(CFSPlayer * pfsPlayer)
{
  if (pfsPlayer->GetSide()->GetObjectID() != SIDE_TEAMLOBBY)
    CheckForSideAllReady(pfsPlayer->GetSide());
}


/*-------------------------------------------------------------------------
 * CheckForSideAllReady
 *-------------------------------------------------------------------------
 * Purpose:
 *    See if everyone on a side is ready. Mark the side as such (if not force ready)
 */
void CFSMission::CheckForSideAllReady(IsideIGC * pside)
{
  SideID sideid = pside->GetObjectID();
  if (GetCountOfPlayers(pside, false) < m_misdef.misparms.nMinPlayersPerTeam)
    SetReady(sideid, false);
  else if (GetForceReady(pside->GetObjectID()))
    SetReady(sideid, true);
  else
  {
    bool fReady = true;
    const ShipListIGC * plistShip = pside->GetShips();
    for(ShipLinkIGC * plinkShip = plistShip->first(); plinkShip; plinkShip = plinkShip->next())
    {
      CFSShip * pfsShip = (CFSShip *) plinkShip->data()->GetPrivateData();
      if (pfsShip->IsPlayer())
      {
        CFSPlayer * pfsPlayer = pfsShip->GetPlayer();
        if (!pfsPlayer->GetReady())
        {
          fReady = false;
          break;
        }
      }
    }
    SetReady(sideid, fReady);
  }
}


/*-------------------------------------------------------------------------
 * CreateCluster()
 *-------------------------------------------------------------------------
 * Purpose:
 *    Make a FSCluster.
 */
void CFSMission::CreateCluster(IclusterIGC * pIclusterIGC)
{
  CFSCluster * pCFSCluster = new CFSCluster(pIclusterIGC);

  m_pFSClusters.push_back(pCFSCluster);
}


/*-------------------------------------------------------------------------
 * DeleteCluster()
 *-------------------------------------------------------------------------
 * Purpose:
 *    Destroy a FSCluster, given a pointer to an IclusterIGC.
 */
void CFSMission::DeleteCluster(IclusterIGC * pIclusterIGC)
{
  CFSCluster * pFSCluster = (CFSCluster *) pIclusterIGC->GetPrivateData();

  if (pFSCluster)
  {
    std::vector<CFSCluster*>::iterator i = std::find(m_pFSClusters.begin(), m_pFSClusters.end(), pFSCluster);

    assert(i != m_pFSClusters.end()); // assert that we found the pointer to delete

    delete static_cast<CFSCluster*>(*i);

    m_pFSClusters.erase(i);
  }
}



/*-------------------------------------------------------------------------
 * CreateSide()
 *-------------------------------------------------------------------------
 * Purpose:
 *    Make a FSSide.
 */
void CFSMission::CreateSide(IsideIGC * pIsideIGC)
{
  CFSSide * pCFSSide = new CFSSide(pIsideIGC, this);

  m_pFSSides.push_back(pCFSSide);
}


/*-------------------------------------------------------------------------
 * DeleteSide()
 *-------------------------------------------------------------------------
 * Purpose:
 *    Destroy a FSSide, given a pointer to an IsideIGC.
 */
void CFSMission::DeleteSide(IsideIGC * pIsideIGC)
{
  CFSSide * pFSSide = (CFSSide *) pIsideIGC->GetPrivateData();

  if (pFSSide)
  {
    std::vector<CFSSide*>::iterator i = std::find(m_pFSSides.begin(), m_pFSSides.end(), pFSSide);

    assert(i != m_pFSSides.end()); // assert that we found the pointer to delete

    delete (CFSSide*)(*i);

    m_pFSSides.erase(i);
  }
}

void CFSMission::UpdateLobby(Time now)
{
  if (m_fLobbyDirty && (now - m_timeLastLobbyMissionInfo >= c_flUpdateTimeInterval)
      && (GetCookie() != NULL || !g.fmLobby.IsConnected()))
  {
    m_fLobbyDirty = false;
    if (g.fmLobby.IsConnected())
    {
      QueueLobbyMissionInfo();
      g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
    }
    m_timeLastLobbyMissionInfo = now;

    // Let's overload this function and also update the session details
    SetSessionDetails();
  }
}

void CFSMission::SetLobbyIsDirty()
{
  m_fLobbyDirty = true;
  UpdateLobby(Time::Now());
}

// #ALLY
// CFSMission::UpdateAlliances
//   master logic for alliances
//   parameters:
//          create alliance between sideID and sideAlly
//      or remove all sideID alliances if sideAlly is NA
//      or validate current alliances if sideID is NA
//   check for consistency & validate alliances
//   broadcast to clients & igc layer
void CFSMission::UpdateAlliances(SideID sideID,SideID sideAlly)
{
	// filter bad/corrupted values
	// sideID: NA or a valid team ID
	if (sideID != NA)
		if (!((sideID>=0) && (sideID<c_cSidesMax))) return;
	// sideAlly: NA or a valid team ID
	if (sideAlly != NA)
		if (!((sideAlly>=0) && (sideAlly<c_cSidesMax))) return;

	// convert alliance groups to bitmasks
	int AlliesMasks[c_cSidesMax];
	for (SideID i = 0; i <  c_cSidesMax ; i++)
	{
		if (GetSideAllies(i) != NA)
		{
			AlliesMasks[i] = 1<<i;
			for (SideID j = 0; j <  c_cSidesMax ; j++)
			{
				if (GetSideAllies(j) == GetSideAllies(i))
					AlliesMasks[i] |= 1<<j;
			}
		}
		else
			AlliesMasks[i] = 0;
	}

	debugf("UpdateAlliances begin: %d %d %d %d %d %d\n",GetSideAllies(0),GetSideAllies(1),GetSideAllies(2),GetSideAllies(3),GetSideAllies(4),GetSideAllies(5));
	debugf("  m: %d %d %d %d %d %d\n",AlliesMasks[0],AlliesMasks[1],AlliesMasks[2],AlliesMasks[3],AlliesMasks[4],AlliesMasks[5]);
	debugf("  team %d ,sideAlly = %d\n",sideID,sideAlly);

	if (sideID != NA) // we got a valid parameter sideID
	{
		// update sideID AlliesMasks

		// if side is active and sideAlly parameter is a real team
		if (GetSideActive(sideID) && (sideAlly != NA))
		{
		   // new allies = current allies + sideAlly + itself
		   AlliesMasks[sideID] = AlliesMasks[sideID]|(1<<sideAlly)|(1<<sideID);  // update this side allies
		}
		else
			AlliesMasks[sideID]= 0;
	}
	debugf("  m1: %d %d %d %d %d %d\n",AlliesMasks[0],AlliesMasks[1],AlliesMasks[2],AlliesMasks[3],AlliesMasks[4],AlliesMasks[5]);

	SideID NumSides = GetCountSides();

	// propagate the alliances & count active sides
	int activesides = 0;
	for (SideID i = 0; i <  NumSides ; i++)
	{
		if (!GetSideActive(i)) // i is inactive, clear its alliances
		{
			AlliesMasks[i] = 0;
			continue;
		}
		activesides++;

		if (sideID == NA) continue;
		if (i==sideID) continue;

		if (AlliesMasks[sideID] & (1<<i)) // team sideID says it's allied with i
		{
			char merged = AlliesMasks[sideID] | AlliesMasks[i];  // so merge their alliances
			AlliesMasks[sideID] = merged;
			AlliesMasks[i] = merged;
		}
		else
			AlliesMasks[i] = AlliesMasks[i] & ~(1<<sideID); // if not then make sure team i isnt ally with team sideid
	}
	debugf("  m2: %d %d %d %d %d %d\n",AlliesMasks[0],AlliesMasks[1],AlliesMasks[2],AlliesMasks[3],AlliesMasks[4],AlliesMasks[5]);

	// build the final allied groups
	char Allies[c_cSidesMax]; for (SideID i = 0; i <  c_cSidesMax ; i++) Allies[i] = NA;

	if (activesides >=3) // need at least 3 active teams for alliances
	{
		// reset allies for solo teams  (remove them from their own allies mask)
		for (SideID i = 0; i <  NumSides ; i++)
		{
			if (AlliesMasks[i] == (1<<i)) AlliesMasks[i] = 0;
		}
		debugf("  m3: %d %d %d %d %d %d\n",AlliesMasks[0],AlliesMasks[1],AlliesMasks[2],AlliesMasks[3],AlliesMasks[4],AlliesMasks[5]);

		// propagate the alliance till it's stable
		while (true)
		{
			bool stop = true;
			for (SideID i = 0; i <  NumSides ; i++)
			{
				for (SideID j = 0; j <  NumSides ; j++)
				{
					int ai = AlliesMasks[i];
					int aj = AlliesMasks[j];
					if (ai & aj)                      // i & j have common allies
					{
						int merged = ai | aj;         // merge them
						if ((ai != merged) || (aj != merged))
						{
							stop = false;             // continue if something changed
							AlliesMasks[i] = merged;
							AlliesMasks[j] = merged;
						}
					}
				}
			}
			if (stop) break; // nothing changed after this pass, we stop
		}
		debugf("  m4: %d %d %d %d %d %d\n",AlliesMasks[0],AlliesMasks[1],AlliesMasks[2],AlliesMasks[3],AlliesMasks[4],AlliesMasks[5]);

		// create the different allied groups

		char groups[c_cAlliancesMax];
		for (int gr=0;gr<c_cAlliancesMax;gr++)  groups[gr]=0; // clear the groups

		int soloteams = 0; //count the solo teams found
		int numgroups = 0; //count the groups found

		for (SideID i = 0; i <  NumSides ; i++)
		{
			if (AlliesMasks[i] != 0) // this side has allies so find its group or build a new group
			{
				bool found_group = false;
				for (int g=0;g<c_cAlliancesMax;g++)
				{
					if (AlliesMasks[i] == groups[g])
					{
						Allies[i] = g;
						found_group = true; // we found its group
					}
				}
				if (!found_group)
				{
					// create a new group
					assert(numgroups<=c_cAlliancesMax) ; // shouldnt happen...
					groups[numgroups] = AlliesMasks[i];
					Allies[i] = numgroups;
					numgroups++;
				}
			}
			else
			{
				Allies[i] = NA; // this teams has no allies
				if (GetSideActive(i)) soloteams++; // and it's a solo team if it's active
			}
		}
		// validation: if no group at all OR no solo teams and only 1 group then invalid alliances
 		debugf("  final g=%d, st=%d\n",numgroups,soloteams);
		if ((numgroups==0) || ((soloteams==0) && (numgroups==1)))
		{
			// in that case, we clear all alliances
			for (SideID i = 0; i <  c_cSidesMax ; i++) Allies[i] = NA;
		}
	}// activesides >=3

	// here we have valid alliances (in Allies[]) so we save them, broadcast them to everyone and update the IGC sides

	BEGIN_PFM_CREATE(g.fm, pfmUpdateAlliances, S, CHANGE_ALLIANCES)
	END_PFM_CREATE
	for (SideID i = 0; i <  c_cSidesMax ; i++)
	{
		pfmUpdateAlliances->Allies[i] = Allies[i]; // the message
		SetSideAllies(i,Allies[i]);         // the server CFSMission
		IsideIGC* side = m_pMission->GetSide(i);
		if (side)
			side->SetAllies(Allies[i]);     // the server IGC sides
	}
	g.fm.SendMessages(GetGroupMission(), FM_GUARANTEED, FM_FLUSH);

	debugf("UpdateAlliances end: %d %d %d %d %d %d\n",GetSideAllies(0),GetSideAllies(1),GetSideAllies(2),GetSideAllies(3),GetSideAllies(4),GetSideAllies(5));

}
// #ALLY end


// do any steps needed to update the current ballot.  Returns true iff
// the ballot is no longer needed.
bool Ballot::Update(const Time& now)
{
  if (m_bCanceled)
    return true;

  if (m_timeExpiration < now || AllVotesAreIn())
  {
    if (HasPassed())
      OnPassed();
    else
      OnFailed();

    return true;
  }
  else
    return false;
}

// cancels the pending vote
void Ballot::Cancel()
{
  if (!m_bCanceled)
  {
    m_bCanceled = true;

    // REVIEW: should we send a cancelation message here?
  }
}

// records the given vote from a player
void Ballot::CastVote(CFSPlayer* pfsPlayer, bool bVote)
{
  // if they are in the list of valid voters
  if (m_vShips.Remove(pfsPlayer->GetShipID()) != -1)
  {
    SideID sideID = pfsPlayer->GetSide()->GetObjectID();

    assert(sideID >= 0);

    // if they are on a valid side, tally the vote
    if (sideID >= 0 && m_cAbstaining[sideID] > 0)
    {
      --m_cAbstaining[sideID];
      if (bVote)
        ++m_cInFavor[sideID];
      else
        ++m_cOpposed[sideID];
    }
  }
}

// gets the ID of this ballot
BallotID Ballot::GetBallotID()
{
  return m_ballotID;
}

// mmf/KGJV 09/07 allow only one ballot of each type at a time
// gets the type of this ballot
BallotType Ballot::GetType()
{
	return m_type;
}

// initializes the ballot for a given vote proposed by a player to their team
void Ballot::Init(CFSPlayer* pfsInitiator, const ZString& strProposalName, const ZString& strBallotText)
{
  assert(pfsInitiator);

  // store some misc. info about the vote
  float c_fVoteDuration = 30.0f;

  m_pgroup = CFSSide::FromIGC(pfsInitiator->GetSide())->GetGroup();
  m_pmission = pfsInitiator->GetMission();
  m_chattarget = CHAT_TEAM;
  m_groupID = pfsInitiator->GetSide()->GetObjectID();
  m_strProposal = strProposalName;
  m_ballotID = s_ballotIDNext++;
  m_timeExpiration = Time::Now() + c_fVoteDuration;
  m_bCanceled = false;

  // zero out the counts
  for (SideID sideID = 0; sideID < c_cSidesMax; ++sideID)
  {
    m_cAbstaining[sideID] = 0;
    m_cInFavor[sideID] = 0;
    m_cOpposed[sideID] = 0;
  }

  // add the players
  assert(pfsInitiator->GetSide());
  const ShipListIGC* shipsList = pfsInitiator->GetSide()->GetShips();
  m_vShips.Reserve(shipsList->n());
  SideID sideIDInitiator = m_groupID;

  for (ShipLinkIGC* shipLink = shipsList->first(); shipLink; shipLink = shipLink->next())
  {
    CFSShip * pfsShip = (CFSShip*)shipLink->data()->GetPrivateData();

    if (pfsShip->IsPlayer())
    {
      ++m_cAbstaining[sideIDInitiator];
      m_vShips.PushEnd(pfsShip->GetShipID());
    }
  }

  // tell the clients about the vote
  BEGIN_PFM_CREATE(g.fm, pfmBallot, S, BALLOT)
    FM_VAR_PARM((PCC)(strBallotText), CB_ZTS)
  END_PFM_CREATE

  pfmBallot->ballotID = m_ballotID;
  pfmBallot->timeExpiration = m_timeExpiration;
  pfmBallot->otInitiator = OT_ship;
  pfmBallot->oidInitiator = pfsInitiator->GetShipID();
  pfmBallot->bHideToLeader = m_bHideToLeader;  // KGJV #110

  g.fm.SendMessages(m_pgroup, FM_GUARANTEED, FM_FLUSH);

  // tally the vote for the initiator
  CastVote(pfsInitiator, true);
}

// initializes the ballot for a given vote proposed by a team to all other teams
void Ballot::Init(CFSSide* pfsideInitiator, const ZString& strProposalName, const ZString& strBallotText)
{
  assert(pfsideInitiator);

  // store some misc. info about the vote
  float c_fVoteDuration = 30.0f;

  m_pgroup = pfsideInitiator->GetMission()->GetGroupRealSides();
  m_pmission = pfsideInitiator->GetMission();
  m_chattarget = CHAT_EVERYONE;
  m_groupID = NA;
  m_strProposal = strProposalName;
  m_ballotID = s_ballotIDNext++;
  m_timeExpiration = Time::Now() + c_fVoteDuration;
  m_bCanceled = false;

  // zero out the counts
  for (SideID sideID = 0; sideID < c_cSidesMax; ++sideID)
  {
    m_cAbstaining[sideID] = 0;
    m_cInFavor[sideID] = 0;
    m_cOpposed[sideID] = 0;
  }

  // add the players
  const ShipListIGC* shipsList = pfsideInitiator->GetMission()->GetIGCMission()->GetShips();
  m_vShips.Reserve(shipsList->n());
  SideID sideIDInitiator = pfsideInitiator->GetSideIGC()->GetObjectID();

  assert(sideIDInitiator >= 0);
  for (ShipLinkIGC* shipLink = shipsList->first(); shipLink; shipLink = shipLink->next())
  {
    CFSShip * pfsShip = (CFSShip*)shipLink->data()->GetPrivateData();
    IsideIGC* pside = pfsShip->GetSide();
    SideID sideIDPlayer = pside->GetObjectID();

    if (pfsShip->IsPlayer() && pside->GetActiveF() && sideIDPlayer >= 0 && sideIDPlayer != sideIDInitiator)
    {
      ++m_cAbstaining[sideIDPlayer];
      m_vShips.PushEnd(pfsShip->GetShipID());
    }
  }

  // tell the clients about the vote
  BEGIN_PFM_CREATE(g.fm, pfmBallot, S, BALLOT)
    FM_VAR_PARM((PCC)(strBallotText), CB_ZTS)
  END_PFM_CREATE

  pfmBallot->ballotID = m_ballotID;
  pfmBallot->timeExpiration = m_timeExpiration;
  pfmBallot->otInitiator = OT_side;
  pfmBallot->oidInitiator = pfsideInitiator->GetSideIGC()->GetObjectID();
  pfmBallot->bHideToLeader = false; // KGJV #110

  g.fm.SendMessages(m_pgroup, FM_GUARANTEED, FM_FLUSH);
}

// gets a string describing the tally of votes
ZString Ballot::GetTallyString()
{
  ZString strMessage;

  bool bFirst = true;
  for (SideID sideID = 0; sideID < c_cSidesMax; ++sideID)
  {
    // if there were any votes on this side
    if (m_cAbstaining[sideID] + m_cInFavor[sideID] + m_cOpposed[sideID])
    {
      if (bFirst)
      {
        bFirst = false;
        strMessage += "(";
      }
      else
        strMessage += ", ";

      // only display the team name if more than one team is voting on the issue
      if (m_chattarget != CHAT_TEAM)
        strMessage += ZString(m_pmission->GetIGCMission()->GetSide(sideID)->GetName()) + ":" ;

      if (m_cInFavor[sideID])
        strMessage += " " + ZString(m_cInFavor[sideID]) + " for";
      if (m_cOpposed[sideID])
        strMessage += " " + ZString(m_cOpposed[sideID]) + " against";
      if (m_cAbstaining[sideID])
        strMessage += " " + ZString(m_cAbstaining[sideID]) + " abstained";
    }
  }

  if (!bFirst)
    strMessage += ")";

  return strMessage;
}

// performs the appropriate result when the vote passes
void Ballot::OnPassed()
{
  ZString strMessage = m_strProposal + " has passed.  " + GetTallyString();

  m_pmission->GetSite()->SendChat(NULL, m_chattarget, m_groupID, NA,
      strMessage, c_cidNone, NA, NA, NULL, true);
}

// performs the appropriate result when the vote passes
void Ballot::OnFailed()
{
  ZString strMessage = m_strProposal + " has been defeated.  " + GetTallyString();

  m_pmission->GetSite()->SendChat(NULL, m_chattarget, m_groupID, NA,
      strMessage, c_cidNone, NA, NA, NULL, true);
}

// tests whether the vote passes
bool Ballot::HasPassed()
{
  for (SideID sideID = 0; sideID < c_cSidesMax; ++sideID)
  {
    // if there were any votes on this side
    if (m_cAbstaining[sideID] + m_cInFavor[sideID] + m_cOpposed[sideID])
    {
      // if it didn't get a majority, it didn't pass.
      if (m_cInFavor[sideID] <= m_cOpposed[sideID])
      {
        return false;
      }
    }
  }

  return true;
}

// tests to see if we have received all of the votes
bool Ballot::AllVotesAreIn()
{
  for (SideID sideID = 0; sideID < c_cSidesMax; ++sideID)
  {
    if (m_cAbstaining[sideID] != 0)
      return false;
  }

  return true;
}

BallotID Ballot::s_ballotIDNext = 0;

// KGJV #110
// mutiny ballot to change commander
MutinyBallot::MutinyBallot(CFSPlayer* pfsInitiator)
{
  m_pside = pfsInitiator->GetSide();
  m_idInitiatorShip = pfsInitiator->GetShipID();
  m_bHideToLeader = true;
  m_type = BALLOT_MUTINY; // mmf/KGJV 09/07 allow only one ballot of each type at a time
  Init(pfsInitiator, pfsInitiator->GetName() + ZString("'s proposal to mutiny"), pfsInitiator->GetName() + ZString(" has proposed to munity.  "));
}

void MutinyBallot::OnPassed()
{
  Ballot::OnPassed();

  SideID    sideID = m_pside->GetObjectID();
  if (sideID >= 0 && STAGE_STARTED == m_pmission->GetStage())
  {
	  CFSShip*    pfssNewLeader = CFSShip::GetShipFromID(m_idInitiatorShip);
      if (pfssNewLeader && pfssNewLeader->IsPlayer() &&
		  pfssNewLeader->GetSide() == m_pside)
      {
		  CFSPlayer * pfspNewLeader = pfssNewLeader->GetPlayer();
		  // KGJV fix: changed whole logic
		  if (pfspNewLeader->GetSide() == m_pside)
		  {
				// get the old leader
				CFSPlayer * pfspOldLeader = m_pmission->GetLeader(sideID);
				assert(pfspOldLeader); // this should really never assert ...

				// if player who proposed is already leader, we do nothing (leader change between Ballot start and end)
				if (pfspOldLeader == pfssNewLeader) return;

				// set new leader to stop donating
				pfspNewLeader->SetAutoDonate(NULL,0,false);

				// get old leader money and set him to autodonate to new leader
				// this will transfert old leader money to new leader and cascade autodonation of everyone
				Money money = pfspOldLeader->GetMoney();
				pfspOldLeader->SetAutoDonate(pfspNewLeader,money,true);

				// but old leader will still see his money because of how client handle AUTODONATE msg
				// (search for "mutiny-note" in clintlib\appmsg.cpp)
				// so we send him the change here:
				BEGIN_PFM_CREATE(g.fm, pfmMoneyChange, S, MONEY_CHANGE)
				END_PFM_CREATE
				pfmMoneyChange->dMoney  = -money;
				pfmMoneyChange->sidTo   = pfspOldLeader->GetShipID();
				pfmMoneyChange->sidFrom = NA;
				g.fm.SendMessages(pfspOldLeader->GetConnection(), FM_GUARANTEED, FM_FLUSH);

				// set pfspNewLeader as new leader
				m_pmission->SetLeader(pfspNewLeader);
		  }
      }
   }
}

// a ballot used when a player suggests resigning
ResignBallot::ResignBallot(CFSPlayer* pfsInitiator)
{
  m_pside = pfsInitiator->GetSide();
  m_bHideToLeader = false; // KGJV #110
  m_type = BALLOT_RESIGN;  // mmf/KGJV 09/07 allow only one ballot of each type at a time
  Init(pfsInitiator, pfsInitiator->GetName() + ZString("'s proposal to resign"), pfsInitiator->GetName() + ZString(" has proposed resigning.  "));
}

void ResignBallot::OnPassed()
{
  Ballot::OnPassed();

  SideID    sideID = m_pside->GetObjectID();
  if (sideID >= 0 && STAGE_STARTED == m_pmission->GetStage())
  {
    m_pmission->DeactivateSide(m_pside);
  }
}

// a ballot used when a player suggests offering a draw
OfferDrawBallot::OfferDrawBallot(CFSPlayer* pfsInitiator)
{
  m_pfside = CFSSide::FromIGC(pfsInitiator->GetSide());
  m_bHideToLeader = false; // KGJV #110
  m_type = BALLOT_OFFERDRAW; // mmf/KGJV 09/07 allow only one ballot of each type at a time
  Init(pfsInitiator, pfsInitiator->GetName() + ZString("'s proposal to offer a draw"), pfsInitiator->GetName() + ZString(" has proposed offering a draw.  "));
}

void OfferDrawBallot::OnPassed()
{
  Ballot::OnPassed();
  m_pmission->GetSite()->SendChat(NULL, m_chattarget, m_groupID, NA,
      "A draw is being offered to the other teams.", c_cidNone, NA, NA, NULL, true);
  m_pmission->AddBallot(new AcceptDrawBallot(m_pfside));
}

// a ballot used when one team offers a draw
AcceptDrawBallot::AcceptDrawBallot(CFSSide* pfsideInitiator)
{
  m_bHideToLeader = false; // KGJV #110
  m_type = BALLOT_ACCEPTDRAW; // mmf/KGJV 09/07 allow only one ballot of each type at a time
  Init(pfsideInitiator, pfsideInitiator->GetSideIGC()->GetName() + ZString("'s offer of a draw"), pfsideInitiator->GetSideIGC()->GetName() + ZString(" has offered a draw.  "));
}

void AcceptDrawBallot::OnPassed()
{
  Ballot::OnPassed();
  m_pmission->GameOver(NULL, "The game was declared a draw");
}





