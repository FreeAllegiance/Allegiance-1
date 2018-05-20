/*-------------------------------------------------------------------------
 * fedsrv\FedSrv.CPP
 *
 * The main module of the server. Handles all communication and game logic
 *
 * Owner:
 *
 * Copyright 1986-1999 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/


#include "pch.h"
#include <..\TCLib\CoInit.h>
#include <..\TCLib\AutoHandle.h>
#include <agc.h>
#include <zreg.h>

 // BT - STEAM
#include <inttypes.h>

// mmf log to file on SRVLOG define as well as _DEBUG
#ifdef _DEBUG
#define SRVLOG
#endif

#ifdef SRVLOG // mmf changed this from _DEBUG
    #include "FedSrvApp.h"
#endif

//appweb -Imago
static Mpr *mpr;
static MprMutex	*mutex;

#ifdef _DEBUG
	static MprLogModule *tMod;
#endif

#if !defined(ALLSRV_STANDALONE)
void DoDecrypt(int size, char* pdata)
{
    DWORD encrypt = 0;
    //Do a rolling XOR to demunge the data
    for (int i = 0; (i < size); i += 4)
    {
        DWORD*  p = (DWORD*)(pdata + i);

        encrypt = *p = *p ^ encrypt;
    }
}
static void DoEncrypt(int size, char* pdata)
{
    DWORD   encrypt = 0;

    //Do a rolling XOR to munge the data
    for (int i = 0; (i < size); i += 4)
    {
        DWORD*  p = (DWORD*)(pdata + i);

        DWORD   old = *p;

        *p = *p ^ encrypt;

        encrypt = old;
    }
}
#endif // !defined(ALLSRV_STANDALONE)

ALLOC_MSG_LIST;
Global g;
// KG- obsolete
//#include "queries.h"

#if defined(ALLSRV_STANDALONE)
TMap<ZString, int> m_CharMap;
  int GetNextCharacterID()
  {
    static int s_nID;
    return ++s_nID;
  }

  //Imago 6/10 #35 - Dropped player stats are reset
  int GetStateID(ZString CharName) {
	  int id = NA;
	  if (m_CharMap.Find(CharName,id)) {
		  return id;
	  }
	  id  = GetNextCharacterID();
	  m_CharMap.Set(CharName,id);
	  return id;
  }
#endif // defined(ALLSRV_STANDALONE)


//removed mothballed MSRG code - Imago
//moved tech bit stuff into igc mission

void LoadTechBits()
{
  #if !defined(ALLSRV_STANDALONE)
    SQL_GO(TechBitsAll);
    while (SQL_SUCCEEDED(SQL_GETROW(TechBitsAll)))
    {
      // database ensures all bit ids are unique
      assert(TechBitsAll_BitID < cTechs);
      SqlStrCpy(g.rgTechs[TechBitsAll_BitID], TechBitsAll_Name, sizeof(TechBitsAll_Name));
    }
  #endif // !defined(ALLSRV_STANDALONE)
}


void LoadRankInfo()
{

#if !defined(ALLSRV_STANDALONE)
  SQL_GO(GetRankCount);
  SQL_GETROW(GetRankCount);
  g.cRankInfo = GetRankCount_Count;

  if (g.cRankInfo > 0)
  {
    g.vRankInfo = new RankInfo[g.cRankInfo];

    SQL_GO(GetRanks);
    for (int i = 0; i < g.cRankInfo; i++)
    {
      ZVerify(SQL_SUCCEEDED(SQL_GETROW(GetRanks)));
      g.vRankInfo[i].civ = GetRanks_CivID;
      g.vRankInfo[i].rank = GetRanks_Rank;
      g.vRankInfo[i].requiredRanking = GetRanks_Requirement;
      strcpy(g.vRankInfo[i].RankName, (char*)GetRanks_Name);
    }
  }
#else //Imago: Changed to load rank_info from <artwork>/ranks.txt
    char        mapfilename[MAX_PATH + 1];
    HRESULT     hr = UTL::getFile("ranks", ".txt", mapfilename, true, true);
    FILE*       file = fopen(mapfilename, "rt"); // open in text mode
    if (file)
    {
      int nbranks;
      fscanf(file,"%d\n",&nbranks);
      g.cRankInfo = nbranks;
      g.vRankInfo = new RankInfo[g.cRankInfo];
      for (int i=0;i<nbranks;i++)
      {
         int rank;
         fscanf(file,"%d,%[^,\n]\n",&rank,g.vRankInfo[i].RankName);
         g.vRankInfo[i].rank = rank;
         g.vRankInfo[i].requiredRanking = 0;
         g.vRankInfo[i].civ = -1;
      }
      fclose(file);
   }
   else
      g.cRankInfo = 0;
#endif // !defined(ALLSRV_STANDALONE)
}

void UnloadRankInfo()
{
  delete [] g.vRankInfo;
  g.cRankInfo = 0;
}

//KGJV #114
// read <artpath>/cores.txt and fill in g.cStaticCoreInfo (size) and g.vStaticCoreInfo (data)
// this assumes cores.txt is well formated (no overflow control)
void LoadStaticCoreInfo()
{
    char        mapfilename[MAX_PATH + 1];
    HRESULT     hr = UTL::getFile("cores", ".txt", mapfilename, true, true);
    FILE*       file = fopen(mapfilename, "rt"); // open in text mode
	g.cStaticCoreInfo = 0;
	g.vStaticCoreInfo = NULL;
    if (file)
    {
		// 1st line = nb cores
		fscanf(file,"%d\n",&g.cStaticCoreInfo);
		g.vStaticCoreInfo = new StaticCoreInfo[g.cStaticCoreInfo];
		memset(g.vStaticCoreInfo,0,g.cStaticCoreInfo*sizeof(StaticCoreInfo));// mainly for debugging purpose
		for (int i=0;i<g.cStaticCoreInfo;i++)
		{
			// one core per line, no .igc extension, max size = c_cbFileName
			fscanf(file,"%[^\n]\n",g.vStaticCoreInfo[i].cbIGCFile);
		}
		fclose(file);
	}
}
void UnloadStaticCoreInfo()
{
	if (g.vStaticCoreInfo)
		delete [] g.vStaticCoreInfo;
	g.cStaticCoreInfo = 0;
}
// End of KGJV #114


void LoadStaticMapInfo()
{
#if !defined(ALLSRV_STANDALONE)
  SQL_GO(GetStaticMapCount);
  SQL_GETROW(GetStaticMapCount);
  g.cStaticMapInfo = GetStaticMapCount_Count;

  if (g.cStaticMapInfo > 0)
  {
    g.vStaticMapInfo = new StaticMapInfo[g.cStaticMapInfo];

    SQL_GO(GetStaticMaps);
    for (int i = 0; i < g.cStaticMapInfo; i++)
    {
      ZVerify(SQL_SUCCEEDED(SQL_GETROW(GetStaticMaps)));
      SQLSTRCPY(g.vStaticMapInfo[i].cbIGCFile, GetStaticMaps_FileName);
      SQLSTRCPY(g.vStaticMapInfo[i].cbFriendlyName, GetStaticMaps_MapName);
      g.vStaticMapInfo[i].nNumTeams = GetStaticMaps_NumTeams;
      assert(GetStaticMaps_NumTeams > 1 && GetStaticMaps_NumTeams <= c_cSidesMax);
    }
  }
#else
// --KGJV; FZ/FAZ version: load maps list from a file (<artwork>\maps.txt)
    char        mapfilename[MAX_PATH + 1];
    HRESULT     hr = UTL::getFile("maps", ".txt", mapfilename, true, true);
    FILE*       file = fopen(mapfilename, "rt"); // open in text mode
    if (file)
    {
		int nbmaps;
		fscanf(file,"%d\n",&nbmaps);
		g.cStaticMapInfo = nbmaps;
		g.vStaticMapInfo = new StaticMapInfo[g.cStaticMapInfo];
		for (int i=0;i<nbmaps;i++)
		{
			int nbteams;
			fscanf(file,"%d,%[^,],%[^,\n]\n",&nbteams,g.vStaticMapInfo[i].cbFriendlyName,g.vStaticMapInfo[i].cbIGCFile);
			g.vStaticMapInfo[i].nNumTeams = nbteams;
		}
		fclose(file);
	}
	else
		g.cStaticMapInfo = 0;
#endif
}

bool VerifyUserMap(const char* szMapFile, int nTeams)
{
  for (int i = 0; i < g.cStaticMapInfo; i++)
  {
    if (nTeams == g.vStaticMapInfo[i].nNumTeams
        && 0 == strcmp(szMapFile, g.vStaticMapInfo[i].cbIGCFile))
      return true;
  }

  return false;
}

void UnloadStaticMapInfo()
{
  delete [] g.vStaticMapInfo;
  g.cStaticMapInfo = 0;
}


/*-------------------------------------------------------------------------
 * AddNamedValue
 *-------------------------------------------------------------------------
  Purpose:
      Support function for SetSessionDetails to add a named value to a string

  Side Effects:
      modifies position pointer
 */
void AddNamedValue(char ** ppNext, const char * szName, const char * szValue)
{
  lstrcpy(*ppNext, szName);
  (*ppNext) += lstrlen(szName);
  *((*ppNext)++) = '\t';
  lstrcpy(*ppNext, szValue);
  (*ppNext) += lstrlen(szValue);
  *((*ppNext)++) = '\n';
}


/*-------------------------------------------------------------------------
 * SetSessionDetails
 *-------------------------------------------------------------------------
  Purpose:
      Generate a string of name/value pairs to attach to the session in FedMessaging

  Side Effects:
      Sets the session details in FedMessaging
 */
void SetSessionDetails()
{
  static char rgchBuf[10<<10]; // big buffer
  char * pNext = rgchBuf;
  static CTimer tt("setting session details", .01f);
  tt.Start();
  const ListFSMission * plistMission = CFSMission::GetMissions();
  for (LinkFSMission * plinkMission = plistMission->first(); plinkMission; plinkMission = plinkMission->next())
  {
    CFSMission * pfsMission = plinkMission->data();
    AddNamedValue(&pNext, "GAM", pfsMission->GetMissionDef()->misparms.strGameName);
    char szPlayers[5];
    _itoa(pfsMission->GetCountOfPlayers(NULL, false), szPlayers, 10);
    AddNamedValue(&pNext, "PLR", szPlayers);
    char szMaxPlayers[6]; // bugfix/stack corruption
    _itoa(pfsMission->GetMissionDef()->misparms.nTotalMaxPlayersPerGame, szMaxPlayers, 10);
    AddNamedValue(&pNext, "LIM", szMaxPlayers);
  }
  g.fm.SetSessionDetails(rgchBuf);
  tt.Stop();
}


/*-------------------------------------------------------------------------
 * UnloadDbCache
 *-------------------------------------------------------------------------
 * Purpose:
 *    Clear out everything we got from database that doesn't change between missions
 */
void UnloadDbCache()
{
  #if !defined(ALLSRV_STANDALONE)
    if (g.trekCore)
    {
      g.trekCore->Terminate();
      delete g.trekCore;
      g.trekCore = NULL;
    }
  #endif // !defined(ALLSRV_STANDALONE)
}


/*-------------------------------------------------------------------------
 * FedSrv_Terminate
 *-------------------------------------------------------------------------
 * Purpose:
 *    Cleanup before exiting
 *
 * Side Effects:
 *    Everything that was allocated is deallocated and no longer valid
 */
HRESULT pascal FedSrv_Terminate(void)
{
  if (g.hReceiveThread)
  {
    // wake up receive thread and wait for it to quit
    SetEvent(g.hKillReceiveEvent);
    WaitForSingleObject(g.hReceiveThread, INFINITE);

    CloseHandle(g.hReceiveThread);
    g.hReceiveThread = NULL;
  }

  if (g.hKillReceiveEvent)
  {
    CloseHandle(g.hKillReceiveEvent);
    g.hKillReceiveEvent = NULL;
  }

  if (g.hPlayerEvent)
  {
    CloseHandle(g.hPlayerEvent);
    g.hPlayerEvent = NULL;
  }

  //SWMRGDelete(&g.swmrg); Imago removed (again)

  // Turn everything off
  g.fm.Shutdown();
  DisconnectFromLobby();

  #if !defined(ALLSRV_STANDALONE)
    ShutDownSQL();
  #endif // !defined(ALLSRV_STANDALONE)
  timeEndPeriod(1);

  if (NULL != g.pServerCounters)
  {
      g.perfshare.FreeCounters(g.pServerCounters);
      g.pServerCounters = NULL;
  }
  g.perfshare.Terminate();  // this puts to rest the server and client instance perf counters

  _AGCModule.TriggerEvent(NULL, AllsrvEventID_Terminate, "", -1, -1, -1, 0);

#ifdef USEAUTH
  g.pzas = NULL;
#endif

  //clean up the pid file we dropped for parent -Imago 6/25/08
#if defined(SRV_CHILD)
	char strFilename[10] = "\0";
	sprintf(strFilename,"%d.pid",GetCurrentProcessId());
	DeleteFile(strFilename);
#endif


  return(S_OK);
}


/*-------------------------------------------------------------------------
 * FetchGlobalAttributeSet
 *-------------------------------------------------------------------------
 * Purpose:
 *    Fetch attributes from database for specified global attribute
 *
 * Parameters:
 *    gaid: global attribute to get
 *    gas:  out parameter of fetched settings
 *
 * Side Effects:
 *    Inner Database query
 */
void FetchGlobalAttributeSet(GlobalAttributeID gaid, GlobalAttributeSet& gas)
{
  #if !defined(ALLSRV_STANDALONE)
    gas.Initialize();
    if (gaid)
    {
      GlobalAttrs_ID = gaid;
      SQL_GO(GetGlobalAttrs);
      SQLRETURN sqlret = SQL_GETROW(GetGlobalAttrs);
      assert(SQL_SUCCEEDED(sqlret));
      #define DBTOGAS(attr) (1.0f + (float) (attr) / 100.0f)

      #define DBGSA(x)     gas.SetAttribute(c_ga##x,                  DBTOGAS(GlobalAttrs_##x));

      DBGSA(MaxSpeed);
      DBGSA(Thrust);
      DBGSA(TurnRate);
      DBGSA(TurnTorque);
      DBGSA(MaxArmorStation);
      DBGSA(ArmorRegenerationStation);
      DBGSA(MaxShieldStation);
      DBGSA(ShieldRegenerationStation);
      DBGSA(MaxArmorShip);
      DBGSA(MaxShieldShip);
      DBGSA(ShieldRegenerationShip);
      DBGSA(ScanRange);
      DBGSA(Signature);
      DBGSA(MaxEnergy);
      DBGSA(SpeedAmmo);
      DBGSA(LifespanEnergy);
      DBGSA(TurnRateMissile);
      DBGSA(MiningRate);
      DBGSA(MiningYield);
      DBGSA(RipcordTime);
      DBGSA(DamageGuns);
      DBGSA(DamageMissiles);
      DBGSA(DevelopmentCost);
      DBGSA(DevelopmentTime);
      DBGSA(MiningCapacity);
    }
  #endif // !defined(ALLSRV_STANDALONE)
}

bool IsAnotherPlayerUsingThisName(const char * playerName, ImissionIGC * pMission)
{
	ShipLinkIGC* psl;
	for (psl = pMission->GetShips()->first(); (psl != NULL); psl = psl->next())
	{
		CFSShip* pfsShip = (CFSShip *)(psl->data()->GetPrivateData());
		if (pfsShip->IsPlayer())
		{
			CFSPlayer* pfsPlayerOther = pfsShip->GetPlayer();

			if (_stricmp(playerName, pfsPlayerOther->GetName()) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

void JoinMission(CFSPlayer * pfsPlayer, DWORD dwCookie, const char * szPW)
{

	// BT - STEAM - Rearching some complaints of stats not being logged. It turns out that US East 1 wasn't in the official game sevrer list. Derp.
	// Leaving this here for now so I can debug some achievement stuff as well. 
	//CSteamID id(uint64(76561198028193684));
	//CSteamAchievements ach(id);
	//int currScore;
	//for (int i = 0; i < 20; i++)
	//{
	//	ach.GetStat(EStats::SUM_SCORE, &currScore);
	//	ach.SetStat(EStats::SUM_SCORE, 3 + currScore);
	//}
	//ach.SaveStats();

  // Get the mission in question
  CFSMission * pfsMission;
  if (NA == (int)dwCookie && CFSMission::GetMissions()->n() > 0)
    pfsMission = CFSMission::GetMissions()->first()->data();
  else
    pfsMission = CFSMission::GetMission(dwCookie);

  if (!pfsMission)
  {
    debugf("%s trying to join mission=%x that doesn't exist\n", pfsPlayer->GetName(), dwCookie);

    // tell them that they can't join
    BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
    END_PFM_CREATE
    pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
    pfmDelPosReq->iSide      = NA;
    pfmDelPosReq->reason     = DPR_NoMission;
    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
    return;
  }

  if (pfsMission->GetStage() == STAGE_OVER)
  {
    // don't let a client join a paused game
    BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
    END_PFM_CREATE
    pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
    pfmDelPosReq->iSide      = NA;
    pfmDelPosReq->reason     = DPR_ServerPaused;
    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
    return;
  }

  ImissionIGC * pMission = pfsMission->GetIGCMission();

  if (IsAnotherPlayerUsingThisName(pfsPlayer->GetName(), pMission) == true)
  {
	  for (int nameIndexCounter = 1; nameIndexCounter < 100; nameIndexCounter++)
	  {
		  ZString currentPlayerName = pfsPlayer->GetName();
		  ZString namePart = currentPlayerName;
		  ZString tagPart = "";
		  if (currentPlayerName.Find("@") > -1)
		  {
			  namePart = currentPlayerName.LeftOf("@");
			  tagPart = currentPlayerName.RightOf("@");
		  }

		  if (currentPlayerName.GetLength() > 23)
			  namePart = namePart.Left(namePart.GetLength() - 2);

		  namePart += nameIndexCounter;

		  if (tagPart.GetLength() > 0)
			  namePart += "@" + tagPart;

		  if (IsAnotherPlayerUsingThisName((PCC)namePart, pMission) == false)
		  {
			  pfsPlayer->SetName((char *)(PCC)namePart);
			  break;
		  }
	  }
  }


  if(UTL::PrivilegedUser(pfsPlayer->GetName(),-1)){ //Avalanche: Let admins bypass most checks. //Imago 6/10 #2
  // check to see if the mission is running and whether joiners are allowed
  
  OldPlayerLink*    popi = pfsMission->GetOldPlayerLink(pfsPlayer->GetName());
  const MissionParams*    pmp = pMission->GetMissionParams();
  SideID iSide=SIDE_TEAMLOBBY;
  unsigned char     bannedSideMask;
  //Check if previously part of the game.
  if (popi)
  {
	bannedSideMask = popi->data().bannedSideMask;
	//Place on lobby if banned from team.
    if ((popi->data().sideID < 0) || (bannedSideMask & SideMask(popi->data().sideID))
        || ((pfsMission->GetStage() == STAGE_STARTED) && pmp->bAllowDefections)
        || (popi->data().sideID >= pmp->nTeams))
    {
      iSide = SIDE_TEAMLOBBY;
    }
    else
    {
      iSide = popi->data().sideID;
    }
  }
  //Join the mission.
  pfsMission->AddPlayerToMission(pfsPlayer);
  IsideIGC* psideReq = pMission->GetSide(iSide);
  //Rejoin team
  if (psideReq->GetObjectID() != SIDE_TEAMLOBBY)
	    pfsMission->RequestPosition(pfsPlayer, psideReq, popi != NULL);
  return;
  }


if (pfsMission->RequiresInvitation())
  {
    if (!pfsMission->IsInvited(pfsPlayer))
    {
      // one of the check to make sure client cannot log into games that it doesn't have access to
      BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
      END_PFM_CREATE
      pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
      pfmDelPosReq->iSide      = NA;
      pfmDelPosReq->reason     = DPR_PrivateGame;
      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
      return;
    }
  }

  //ImissionIGC * pMission = pfsMission->GetIGCMission();

  #if defined(ALLSRV_STANDALONE)
  if (g.fm.GetHostApplicationGuid() == FEDSRV_STANDALONE_PRIVATE_GUID)
  {
      //Disallow if the name is already in use
      ShipLinkIGC* psl;
      for (psl = pMission->GetShips()->first(); (psl != NULL); psl = psl->next())
      {
          CFSShip* pfsShip = (CFSShip *)(psl->data()->GetPrivateData());
          if (pfsShip->IsPlayer())
          {
              CFSPlayer* pfsPlayerOther = pfsShip->GetPlayer();

              if (_stricmp(pfsPlayer->GetName(), pfsPlayerOther->GetName()) == 0)
              {
                  // one of the check to make sure client cannot log into games that it doesn't have access to
                  BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
                  END_PFM_CREATE
                  pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
                  pfmDelPosReq->iSide      = NA;
                  pfmDelPosReq->reason     = DPR_DuplicateLogin;
                  g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
                  return;
              }
          }
      }
  }
  else
  #endif // defined(ALLSRV_STANDALONE)
  {
#ifdef USEAUTH
  pfsMission->RemovePlayerByName(pfsPlayer->GetName(), QSR_DuplicateLocalLogon);
#endif
  }

  const MissionParams*    pmp = pMission->GetMissionParams();

  // if this mission has a player limit, check that.
  if (pmp->nTotalMaxPlayersPerGame != 0x7fff
      && pfsMission->GetCountOfPlayers(NULL, true) >= pmp->nTotalMaxPlayersPerGame)
  {
    BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
    END_PFM_CREATE
    pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
    pfmDelPosReq->iSide      = NA;
    pfmDelPosReq->reason     = DPR_GameFull;
    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
    return;
  }

  // make sure this client is invited to the game
  const char* szPassword = pmp->strGamePassword;
  if (szPassword[0] != '\0'
      && Strcmp(szPassword, szPW) != 0)
  {
    BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
    END_PFM_CREATE
    pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
    pfmDelPosReq->iSide      = NA;
    pfmDelPosReq->reason     = DPR_BadPassword;
    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
    return;
  }

  // mmf added check here for special users so they can join any game

  if (!pfsPlayer->CanCheat() && !UTL::PrivilegedUser(pfsPlayer->GetName(),-1)) //Imago 6/10 #2
  {
    // make sure the player meets the rank requirements for this mission
    RankID rank = pfsPlayer->GetPersistPlayerScore(NA)->GetRank();
    if (pfsMission->GetMissionDef()->misparms.iMinRank > rank
        || pfsMission->GetMissionDef()->misparms.iMaxRank < rank)
    {
      BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
      END_PFM_CREATE
      pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
      pfmDelPosReq->iSide      = NA;
      pfmDelPosReq->reason     = DPR_InvalidRank;
      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
      return;
    }
  }

  // check to see if the mission is running and whether joiners are allowed
  OldPlayerLink*    popi = pfsMission->GetOldPlayerLink(pfsPlayer->GetName());

  SideID iSide;
  unsigned char     bannedSideMask;
  if (popi)
  {
    if (popi->data().pso.GetDeaths() > pmp->iLives)
    {
      BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
      END_PFM_CREATE
      pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
      pfmDelPosReq->iSide      = NA;
      pfmDelPosReq->reason     = DPR_OutOfLives;
      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
      return;
    }


    //Has this person been banned from the game (or ... is there a team that
    //will accept him)
    bannedSideMask = popi->data().bannedSideMask;

    //So ... if there are two teams, the legal side mask == 1 << 2 - 1 = 3
    unsigned char   legalSideMask = SideMask(pMission->GetSides()->n()) - 1;

    if ((bannedSideMask & legalSideMask) == legalSideMask)
    {
      //No one wants this player
      BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
      END_PFM_CREATE
      pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
      pfmDelPosReq->iSide      = NA;
      pfmDelPosReq->reason     = DPR_Banned;
      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
      return;
    }

    if ((popi->data().sideID < 0) || (bannedSideMask & SideMask(popi->data().sideID))
        || ((pfsMission->GetStage() == STAGE_STARTED) && pmp->bAllowDefections)
        || (popi->data().sideID >= pmp->nTeams))
    {
      iSide = SIDE_TEAMLOBBY;
    }
    else
    {
      iSide = popi->data().sideID;
    }
  }
  else
  {
    if (((!pmp->bAllowJoiners) && (pfsMission->GetStage() != STAGE_NOTSTARTED))
      || pmp->bLockLobby)
    {
      BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
      END_PFM_CREATE
      pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
      pfmDelPosReq->iSide      = NA;
      pfmDelPosReq->reason     = pmp->bLockLobby ? DPR_LobbyLocked : DPR_NoJoiners;
      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
      return;
    }

    bannedSideMask = 0;

    if (pfsMission->GetStage() == STAGE_NOTSTARTED)
      iSide = NA;                 //Pick a random team
    else
      iSide = SIDE_TEAMLOBBY;
  }

  // assign a random side (if we can)
  if ((iSide == NA) || ((iSide == SIDE_TEAMLOBBY) && pmp->bLockSides))
  {
    iSide = SIDE_TEAMLOBBY;	// TE: Made all players join lobby when joining game
    assert (iSide != NA);
  }

  // put them on the team lobby side
  pfsMission->AddPlayerToMission(pfsPlayer);
  IsideIGC* psideReq = pMission->GetSide(iSide);

  if (!psideReq)
  {
    assert(false);
    psideReq = pMission->GetSide(SIDE_TEAMLOBBY);
  }

  if (psideReq->GetObjectID() != SIDE_TEAMLOBBY)
    pfsMission->RequestPosition(pfsPlayer, psideReq, popi != NULL);
}


/*-------------------------------------------------------------------------
 * GotInvitationList
 *-------------------------------------------------------------------------
  Purpose:
      Callback from completion of CQGetInvitationList query.

  Side Effects:
      Players/teams added to list
 */
// KG: eventually remove this
#ifndef ALLSRV_STANDALONE
void GotInvitationList(CQGetInvitationList * pquery)
{
  CQGetInvitationListData * pqd = pquery->GetData();
  CFSMission * pfsMission = CFSMission::GetMissionFromIGCMissionID(pqd->missionID);
  int cRows = 0;

  if (pfsMission) // if the mission is gone, forget about it
  {
    CQGetInvitationListData * pRows = pquery->GetOutputRows(&cRows); // all the rows are here
    while (cRows--)
    {
      pfsMission->AddInvitation(SideID(pRows->iTeam), pRows->szSubject);
    }
  }
}
#endif


/*-------------------------------------------------------------------------
 * GotCharSquads
 *-------------------------------------------------------------------------
  Purpose:
      Callback from completion of CQCharSquads query.

  Side Effects:
      Tell the players what squads they're in.
 */
// KG: eventually remove this
#ifndef ALLSRV_STANDALONE
void GotCharSquads(CQCharSquads * pquery)
{
  CQCharSquadsData * pqd = pquery->GetData();
  CFMConnection * pcnxn = g.fm.GetConnectionFromId(pqd->dwConnectionID);
  if (!pcnxn)
    return; // nothing we do for someone who's gone

  CFSPlayer * pfsPlayer = CFSPlayer::GetPlayerFromConnection(*pcnxn);
  if (!pfsPlayer)
    return; // nothing we do for someone who's going away

  int cRows;
  CQCharSquadsData * pRows = pquery->GetOutputRows(&cRows); // all the rows are here
  while (cRows--)
  {
    pfsPlayer->GetSquadMembershipList()->last(
      new SquadMembership(
        pRows->squadID,
        (const char*)pRows->szSquadName,
        0 == pRows->status,
        1 == pRows->status && 1 == pRows->detailedStatus
      ));
    pRows++;
  }

  // tell them which squads they are on
  int nNumSquads = pfsPlayer->GetSquadMembershipList()->n();

  SquadMembership* vsquadmembership =
      (SquadMembership*)_alloca(sizeof(SquadMembership) * nNumSquads + 1);

  SquadMembershipLink* pSquadLink = pfsPlayer->GetSquadMembershipList()->first();
  for (int iSquad = 0; iSquad < nNumSquads; ++iSquad, pSquadLink = pSquadLink->next())
  {
    vsquadmembership[iSquad] = *(pSquadLink->data());
  }

  BEGIN_PFM_CREATE(g.fm, pfmSquadMemberships, LS, SQUAD_MEMBERSHIPS)
      FM_VAR_PARM(nNumSquads ? vsquadmembership : NULL, sizeof(SquadMembership) * nNumSquads)
  END_PFM_CREATE
  pfmSquadMemberships->cSquadMemberships = nNumSquads;
  g.fm.SendMessages(pcnxn, FM_GUARANTEED, FM_FLUSH);

  if (pqd->fJoin) // we waited for the squads because it's a squad game
    JoinMission(pfsPlayer, pqd->dwCookie, pqd->szPassword);
}


#endif
/*-------------------------------------------------------------------------
 * GotLogonDetails
 *-------------------------------------------------------------------------
  Purpose:
      Callback from GetLogonStats query. Logon finishes here
 */
void GotLogonDetails(CQLogonStats * pquery)
{
  CQLogonStatsData * pqd = pquery->GetData();
  CFMConnection * pcnxn = g.fm.GetConnectionFromId(pqd->dwConnectionID);
  bool fJoinNow = true; // only false if it's a squad game
  if (!pcnxn)
    return; // nothing we do for someone who's gone

  CFSPlayer * pfsPlayer = CFSPlayer::GetPlayerFromConnection(*pcnxn);
  bool fValid = pqd->fValid;
  bool fRetry = pqd->fRetry;

  // Some debugging stuff to monitor the amount of startup infowe send everyone
  g.cMsgsToOthers = 0;
  g.cBytesToOthers = 0;
  float flDTime;
  int cMsgsOdometer;
  int cBytesOdometer;
  g.fm.CheckOdometer(flDTime, cMsgsOdometer, cBytesOdometer);

  if (fValid)  // doesn't matter whether we created a new ship, or continued an existing one
  {
    lstrcpy(pqd->szReason, pqd->szCharacterName);

	// BT - STEAM
	pcnxn->SetSteamID(pqd->steamID);

    if (NULL != g.pServerCounters)
    {
      g.pServerCounters->cLogins++;
      g.pServerCounters->cPlayersOnline++;
    }

    // Create the CFSPlayer object. We require that a player always have a ship, so create that first
    // Any ol' ship will do since the client doesn't see it anyway until they buy one
    // But it has to be free so they don't get a cash back bonus when they trade it in for a new ship
    DataShipIGC dataship;
    ZeroMemory(&dataship, sizeof(dataship));
    dataship.shipID = NA;
    dataship.sideID = NA;
    dataship.hullID = NA;
    //dataship.wingID = 1;

	// Imago use cheat 7/4/09 for testing,
	// this ensures it's off for regular builds.
    #ifdef  _TEST
    pqd->fCanCheat = true;
    #endif

	// If CDKey matches Pig CDKey then they can always cheat
	if (strcmp(pqd->szCDKey, g.szBotAuthenticationGuid) == 0) {
		pqd->fCanCheat = true;  
	}

    dataship.pilotType = pqd->fCanCheat ? c_ptCheatPlayer : c_ptPlayer;
    dataship.abmOrders = 0;
    dataship.baseObjectID = NA;
    lstrcpy(dataship.name, pqd->szReason);
    TRef<IshipIGC> pship = (IshipIGC *) g.trekCore->CreateObject(g.timeNow,
                          OT_ship, &dataship, sizeof(dataship));

	// If CDKey matches Pig CDKey then give them super powers
	if (strcmp(pqd->szCDKey, g.szBotAuthenticationGuid) == 0) {
		pship->SetWingmanBehaviour(c_wbbmStrafe | c_wbbmUseMissiles | c_wbbmUseMines | c_wbbmRunAt30Hull | c_wbbmInRangeAggressive);
	}

	// BT - STEAM
	CSteamID steamID;
	steamID.SetFromUint64(pqd->steamID);

    pfsPlayer = new CFSPlayer(pcnxn, pqd->characterID, pqd->szCDKey, pship, !! pqd->fCanCheat, steamID);
    assert(pfsPlayer);

	/* pkk May 6th: Disabled bandwidth patch
	// w0dk4 Bandwidth Patch
	// set bandwidth to standard value, bandwidth packet should arrive soon anyways
	pfsPlayer->SetBandwidth(2);*/

	if (pqd->rank == -1) {
		fValid = false;
		Strcpy(pqd->szReason,"Your callsign does not exist on this server.");
	} else {
		const CivilizationListIGC * pctlist = g.trekCore->GetCivilizations();
		PersistPlayerScoreObject * ppso = new PersistPlayerScoreObject(0.0f,0.0f,pqd->rank,NA);  //and NOAT
		pfsPlayer->GetPersistPlayerScoreObjectList()->last(ppso);
		for (CivilizationLinkIGC * pcl = pctlist->first(); (pcl != NULL); pcl = pcl->next())
		{
			PersistPlayerScoreObject * ppso = new PersistPlayerScoreObject(0.0f,0.0f,pqd->rank,pcl->data()->GetObjectID());
			pfsPlayer->GetPersistPlayerScoreObjectList()->last(ppso);
		}
	}

	// mmf/Imago remove rank from player name when NotNewb
	char newName[c_cbName];
	ZString strName = pship->GetName();
	int leftParen = strName.ReverseFind('(',0);
	if (pqd->rank > g.MaxNewbRank && leftParen>1) {
		Strncpy(newName,(PCC)strName,leftParen);
		Strncpy(newName+leftParen,"\0",1); // null terminate
		pship->SetName(newName);
	}

  } else { //no soup for you!
	if (NULL != g.pServerCounters) {
		g.pServerCounters->cLoginsFailed++;
	}
  }

  BEGIN_PFM_CREATE(g.fm, pfmLogonAck, S, LOGONACK)
    FM_VAR_PARM(pqd->szReason, CB_ZTS)
  END_PFM_CREATE
  pfmLogonAck->shipID          = fValid ? pfsPlayer->GetShipID() : 0;
  pfmLogonAck->fValidated      = fValid;
  pfmLogonAck->fRetry          = fRetry;
  pfmLogonAck->cookie          = fValid ? pfsPlayer->GetCookie() : 0;
  pfmLogonAck->timeServer      = g.timeNow;


  // -KGJV: changed for StaticMapInfo in all mode
  // -Imago: changed for RankInfo in all mode
  if (fValid)
  {

    // send the static map info for the maps on this server
    BEGIN_PFM_CREATE(g.fm, pfmStaticMapInfo, S, STATIC_MAP_INFO)
      FM_VAR_PARM(g.vStaticMapInfo, sizeof(StaticMapInfo) * g.cStaticMapInfo)
    END_PFM_CREATE

    // send the rank info for this server
    BEGIN_PFM_CREATE(g.fm, pfmRankInfo, S, RANK_INFO)
      FM_VAR_PARM(g.vRankInfo, sizeof(RankInfo) * g.cRankInfo)
    END_PFM_CREATE
    pfmRankInfo->cRanks = g.cRankInfo;

#if !defined(ALLSRV_STANDALONE)

    // Get their squad memberships, but whether we need it b4 joining depends on whether it's a squad game
    CQCharSquads * pqueryCharSquads = new CQCharSquads(GotCharSquads);
    CQCharSquadsData * pqdSquads = pqueryCharSquads->GetData();
    pqdSquads->characterID = pqd->characterID;
    pqdSquads->dwConnectionID = pqd->dwConnectionID;
    CFSMission * pfsMission = CFSMission::GetMission(pqd->dwCookie);
    if (pfsMission) // it MAY have gone away in the meantime
    {
      pqdSquads->fJoin = pfsMission->IsSquadGame();
      fJoinNow = !pqdSquads->fJoin; // we EITHER join now, or later, not both
      pqdSquads->dwCookie = pqd->dwCookie;
      lstrcpy(pqdSquads->szPassword, pqd->szPassword);
      g.sql.PostQuery(pqueryCharSquads);
    }
#endif
  }

  g.fm.SendMessages(pcnxn, FM_GUARANTEED, FM_FLUSH);
  g.fm.CheckOdometer(flDTime, cMsgsOdometer, cBytesOdometer);
  debugf("Player %s logon %s. Sent %d msgs and %dB to them in %1.3fs, "
         "and %d msgs and %dB to others in the process.\n",
         pqd->szCharacterName, fValid ? "succeeded" : "failed",
         cMsgsOdometer, cBytesOdometer, flDTime, g.cMsgsToOthers, g.cBytesToOthers);

  if (fValid && fJoinNow)
    JoinMission(pfsPlayer, pqd->dwCookie, pqd->szPassword);
}



/*-------------------------------------------------------------------------
 * SetCharStats
 *-------------------------------------------------------------------------
  Purpose:
      Upload character persisted stats

  Parameters:
      PersistPlayerScoreObject
 */
void SetCharStats(int                   characterID,
                  CFSPlayer*            pfsPlayer,
                  IsideIGC *            pSide,
                  PlayerScoreObject&    pso,
                  CFSMission*           pfsMission)
{
#if !defined(ALLSRV_STANDALONE)  // mmf moved this up from further down
	if (0.0f == pso.GetTimePlayed())
    {
      return;
    }

    assert(pso.GetScore() >= 0.0f);

    assert (pSide);
    CivID   civID = pSide->GetCivilization()->GetObjectID();

    PersistPlayerScoreObject& ppso = pso.GetPersist();
    RankID  newRank = ppso.GetRank();

    if (pfsMission->GetScoresCount())
    {
        float   score = ppso.GetScore() + pso.GetScore();
        int     iScore = int(score);

        for (int i = g.cRankInfo - 1; (i >= 0); i--)
        {
            RankInfo&   ri = g.vRankInfo[i];
            if ((ri.civ == civID) && (ri.requiredRanking < iScore))
            {
                newRank = ri.rank;
                break;
            }
        }

        // Update the persist player score object
        ppso.Set(score,
                 pso.GetCombatRating(),
                 newRank);

        // Save to the cached persist player score object as well
        if (pfsPlayer)
        {
            pfsPlayer->GetPersistPlayerScore(civID)->Set(score, pso.GetCombatRating(), newRank);

            //Update NA civ rank as well.
            PersistPlayerScoreObject* ppsoNA = pfsPlayer->GetPersistPlayerScore(NA);
            float   s = ppsoNA->GetScore() + pso.GetScore();
            int     is = int(s);

            for (int i = g.cRankInfo - 1; (i >= 0); i--)
            {
                RankInfo&   ri = g.vRankInfo[i];
                if ((ri.civ == NA) && (ri.requiredRanking < is))
                {
                    ppsoNA->Set(s, 0.0f, ri.rank);
                    break;
                }
            }
        }
    }

 // #if !defined(ALLSRV_STANDALONE) mmf moved this further up

    CQCharStats * pquery = new CQCharStats(NULL); // don't need call-back notification
    CQCharStatsData * pdq = pquery->GetData();

    pdq->CharacterID  = characterID;
    pdq->CivID        = civID;
    // All of these are for THIS GAME ONLY (except Rating, which is always overall)
    pdq->Rating             = RATING_INT2EXT(pso.GetCombatRating());
    pdq->WarpsSpotted       = pso.GetWarpsSpotted();
    pdq->AsteroidsSpotted   = pso.GetAsteroidsSpotted();
    pdq->TechsRecovered     = pso.GetTechsRecovered();
    pdq->MinerKills         = pso.GetMinerKills();
    pdq->BuilderKills       = pso.GetBuilderKills();
    pdq->LayerKills         = pso.GetLayerKills();
    pdq->PlayerKills        = pso.GetPlayerKills();
    pdq->BaseKills          = pso.GetBaseKills();
    pdq->BaseCaptures       = pso.GetBaseCaptures();
    pdq->Deaths             = pso.GetDeaths();
    pdq->PilotBaseKills     = pso.GetPilotBaseKills();
    pdq->PilotBaseCaptures  = pso.GetPilotBaseCaptures();
    pdq->Minutes            = pso.GetTimePlayed() / 60.0f;
    pdq->bWin               = pso.GetWinner();
    pdq->bLose              = pso.GetLoser();
    pdq->bWinCmd            = pso.GetCommandWinner();
    pdq->bLoseCmd           = pso.GetCommandLoser();
    pdq->Score              = pso.GetScore();
    pdq->bScoresCount       = pfsMission->GetScoresCount();
    pdq->RankOld            = newRank;

    debugf("SetCharStats: bScoresCount=%d, CharacterID=%d, CivID=%d, Rating=%d,\n",
        pdq->bScoresCount,pdq->CharacterID, pdq->CivID, pdq->Rating);
    debugf("    WarpsSpotted=%.2f, AsteroidsSpotted=%.2f, TechsRecovered=%.2f,\n",
        pdq->WarpsSpotted, pdq->AsteroidsSpotted, pdq->TechsRecovered);
    debugf("    MinerKills=%.2f, BuilderKills=%.2f, LayerKills=%.2f, PlayerKills=%.2f,\n",
        pdq->MinerKills, pdq->BuilderKills, pdq->LayerKills, pdq->PlayerKills);
    debugf("    BaseKills=%.2f, BaseCaptures=%.2f, Deaths=%d, PilotBaseKills=%d, BaseCaptures=%d,\n",
        pdq->BaseKills, pdq->BaseCaptures, pdq->Deaths, pdq->PilotBaseKills, pdq->PilotBaseCaptures);
    debugf("    Minutes=%d, bWin=%d, bLose=%d, bWinCmd=%d, bLoseCmd=%d, RankOld=%d, Score=%.2f.\n",
        pdq->Minutes, pdq->bWin, pdq->bLose, pdq->bWinCmd, pdq->bLoseCmd, pdq->RankOld, pdq->Score);

    /* stress test
    int i1 = 0;
    for (i1 = 0; i1 < 10000; i1++)
    {
      CQCharStats * pq = (CQCharStats *) pquery->Copy(NULL);
      g.sql.PostQuery(pq);
    }
    */

    g.sql.PostQuery(pquery);

  #endif // !defined(ALLSRV_STANDALONE)

#ifdef DEBUG
	// mmf have debug build of server log this
	// mmf converted debug statement from pdq structure
	//     note pfsPlayer can be NULL so added check for that
	if (0.0f == pso.GetTimePlayed())
    {   return;    }
    assert(pso.GetScore() >= 0.0f);
	assert(pSide);
    PersistPlayerScoreObject& ppso = pso.GetPersist();
    RankID  Rank = ppso.GetRank();
	if (pfsPlayer != NULL) debugf("PlayerName =%s, ",pfsPlayer->GetName());
	debugf("CharacterID=%d, Rank=%d, bScoresCount=%d, CivID=%d, Rating=%d,\n",
        characterID, Rank ,pfsMission->GetScoresCount(), pSide->GetCivilization()->GetObjectID(),
		RATING_INT2EXT(pso.GetCombatRating()));
    debugf("    WarpsSpotted=%.2f, AsteroidsSpotted=%.2f, TechsRecovered=%d,\n",
        pso.GetWarpsSpotted(), pso.GetAsteroidsSpotted(), pso.GetTechsRecovered());
    debugf("    MinerKills=%.2f, BuilderKills=%.2f, LayerKills=%.2f, PlayerKills=%.2f,\n",
        pso.GetMinerKills(), pso.GetBuilderKills(), pso.GetLayerKills(), pso.GetPlayerKills());
    debugf("    BaseKills=%.2f, BaseCaptures=%.2f, Deaths=%d, PilotBaseKills=%d, BaseCaptures=%d,\n",
        pso.GetBaseKills(), pso.GetBaseCaptures(), pso.GetDeaths(), pso.GetPilotBaseKills(), pso.GetPilotBaseCaptures());
    debugf("    HighValueTargetsSpotted=%d, Repair=%.2f,\n",
        pso.GetTargetsSpotted(), pso.GetRepair());
    debugf("    Minutes=%.2f, bWin=%d, bLose=%d, bWinCmd=%d, bLoseCmd=%d, Score=%.2f.\n",
        (pso.GetTimePlayed() / 60.0f), pso.GetWinner(), pso.GetLoser(), pso.GetCommandWinner(), pso.GetCommandLoser(),
		pso.GetScore());
#endif
}

/*-------------------------------------------------------------------------
 * RecordSquadGame
 *-------------------------------------------------------------------------
  Purpose:
      Upload the results of a squad game

  Parameters:
      List of sides in the game & the winning side
 */
void RecordSquadGame(const SideListIGC*  psides, IsideIGC* psideWon)
{
  #if !defined(ALLSRV_STANDALONE)
    CQReportSquadGame * pquery = new CQReportSquadGame(NULL);
    CQReportSquadGameData * pqd = pquery->GetData();

    pqd->squadIDWon = psideWon->GetSquadID();

    // fill in the losers
    // Blech - have to unroll the loop because we need to fill in different
    // variable names, and not just different indices
    SideLinkIGC* linkSide = psides->first();

    // loser 1
    {
        while (linkSide
          && (linkSide->data() == psideWon || linkSide->data()->GetSquadID() == NA))
        {
          linkSide = linkSide->next();
        }

        if (linkSide)
        {
          pqd->squadIDLost1 = linkSide->data()->GetSquadID();
          linkSide = linkSide->next();
        }
        else
          pqd->squadIDLost1 = NA;
    }

    // loser 2
    {
        while (linkSide
          && (linkSide->data() == psideWon || linkSide->data()->GetSquadID() == NA))
        {
          linkSide = linkSide->next();
        }

        if (linkSide)
        {
          pqd->squadIDLost2 = linkSide->data()->GetSquadID();
          linkSide = linkSide->next();
        }
        else
          pqd->squadIDLost2 = NA;
    }

    // loser 3
    {
        while (linkSide
          && (linkSide->data() == psideWon || linkSide->data()->GetSquadID() == NA))
        {
          linkSide = linkSide->next();
        }

        if (linkSide)
        {
          pqd->squadIDLost3 = linkSide->data()->GetSquadID();
          linkSide = linkSide->next();
        }
        else
          pqd->squadIDLost3 = NA;
    }

    // loser 4
    {
        while (linkSide
          && (linkSide->data() == psideWon || linkSide->data()->GetSquadID() == NA))
        {
          linkSide = linkSide->next();
        }

        if (linkSide)
        {
          pqd->squadIDLost4 = linkSide->data()->GetSquadID();
          linkSide = linkSide->next();
        }
        else
          pqd->squadIDLost4 = NA;
    }

    // loser 5
    {
        while (linkSide
          && (linkSide->data() == psideWon || linkSide->data()->GetSquadID() == NA))
        {
          linkSide = linkSide->next();
        }

        if (linkSide)
        {
          pqd->squadIDLost5 = linkSide->data()->GetSquadID();
          linkSide = linkSide->next();
        }
        else
          pqd->squadIDLost5 = NA;
    }

    assert(linkSide == NULL || linkSide->data() == psideWon && linkSide->next() == NULL);

    g.sql.PostQuery(pquery);
  #endif // !defined(ALLSRV_STANDALONE)
}


/*-------------------------------------------------------------------------
 * GetInvitations
 *-------------------------------------------------------------------------
 */
void GetInvitations(int nInvitationID, MissionID missionID)
{
#if !defined(ALLSRV_STANDALONE)
    CQGetInvitationList * pquery = new CQGetInvitationList(GotInvitationList);
    CQGetInvitationListData * pdq = pquery->GetData();

    pdq->listid = nInvitationID;
    pdq->missionID = missionID;

    g.sql.PostQuery(pquery);
#endif
}


/*-------------------------------------------------------------------------
 * LoadDbCache
 *-------------------------------------------------------------------------
 * Purpose:
 *     Loads everything from database that's (basically) static for duration
 *     Keeps both the igc data, and also snapshots of the messages so that they
 *         can be sent to clients without any processing
 *
 * Returns:
 *     nothing (yet)
 *
 * Side Effects:
 *     lots of memory allocated for cached tables. Freed on cleanup
 */
void LoadDbCache()
{
  #if !defined(ALLSRV_STANDALONE)
    SQLRETURN sqlret = 0;
    int i = 0;
    int nCount;

    g.trekCore = CreateMission(); // this isn't a *real* mission, just a gateway to static stuff
    g.trekCore->Initialize(Time::Now(), &g.siteFedSrv);
    g.trekCore->SetStaticCore(NULL);

    // There's a number of database sizes that are greater than the message sizes,
    // and I don't want to put casts all over the place, so...
    #pragma warning(disable : 4244)

    LoadTechBits();

    // Load all ship types
    SQL_GO(ShipsCount);
    SQL_GETROW(ShipsCount);

    // to preserve DB ordering of ship types, get the ids in database order
    ShipID* vnShipIdList = (ShipID*)_alloca(g_cShipTypes * sizeof(ShipID));
    nCount = 0;
    SQL_GO(ShipTypeIDs);
    while (SQL_SUCCEEDED(sqlret = SQL_GETROW(ShipTypeIDs)))
    {
        vnShipIdList[nCount] = ShipTypeIDs_ShipTypeID;
        ++nCount;
        assert(nCount <= g_cShipTypes);
    }

    // Load all projectiles
    SQL_GO(Projectiles);
    while (SQL_SUCCEEDED(SQL_GETROW(Projectiles)))
    {
      DataProjectileTypeIGC dpt;
      dpt.projectileTypeID = Projectiles_ProjectileID;
      dpt.power       = Projectiles_HitPointsInflict;
      dpt.blastPower  = Projectiles_BlastPower;
      dpt.blastRadius = Projectiles_BlastRadius;
      dpt.speed       = Projectiles_SpeedMax;
      dpt.absoluteF   = false != Projectiles_IsAbsoluteSpeed;
      dpt.modelName[0] = '\0';
      SQLSTRCPY(dpt.textureName, Projectiles_FileTexture);
      SQLSTRCPY(dpt.modelName,   Projectiles_FileModel);
      dpt.lifespan     = Projectiles_TimeDuration / 1000.0f;
      dpt.radius       = Projectiles_Size_cm / 100.0f; // centimeters to meters
      dpt.rotation     = Projectiles_RateRotation * pi / 180.0f;
      dpt.color.r      = Projectiles_Red   / 100.0f;
      dpt.color.g      = Projectiles_Green / 100.0f;
      dpt.color.b      = Projectiles_Blue  / 100.0f;
      dpt.color.a      = Projectiles_Alpha / 100.0f;
      dpt.damageType   = Projectiles_DamageType;
          ;
      dpt.bDirectional = !!Projectiles_IsDirectional;
      dpt.width = Projectiles_WidthOverHeight;
      dpt.ambientSound = Projectiles_AmbientSound;

      IprojectileTypeIGC * pprojtype = (IprojectileTypeIGC *)
          g.trekCore->CreateObject(g.timeNow, OT_projectileType, &dpt, sizeof(dpt));
      pprojtype->Release();
    }

    // Load weapons
    SQL_GO(Weapons);
    while (SQL_SUCCEEDED(SQL_GETROW(Weapons)))
    {
      DataWeaponTypeIGC  dwt;
      CopyMemory(dwt.modelName,   Weapons_FileModel,   sizeof(Weapons_FileModel));
      SET_PARTS(dwt, Weapons)
      dwt.dtimeReady        = (float) Weapons_dTimeReady / 1000.0f;
      dwt.dtimeBurst        = Weapons_dTimeBurstShots / 1000.0f;
      dwt.energyPerShot     = Weapons_EnergyPerShot;
      dwt.dispersion        = Weapons_Dispersion;
      dwt.projectileTypeID  = Weapons_ProjectileID1;
      dwt.cAmmoPerShot      = Weapons_cBulletsPerShot;
      dwt.activateSound     = Weapons_ActivateSound;
      dwt.singleShotSound   = Weapons_SingleShotSound;
      dwt.burstSound        = Weapons_BurstShotSound;

      IweaponIGC * pweapon  = (IweaponIGC *) g.trekCore->CreateObject(
          g.timeNow, OT_partType, &dwt, sizeof(dwt));
      pweapon->Release();
    }

    // Load Missiles
    SQL_GO(Missiles);
    while (SQL_SUCCEEDED(SQL_GETROW(Missiles)))
    {
      DataMissileTypeIGC missile;
      SET_EXPEND(missile, Missiles)

      missile.maxLock = Missiles_MaxLock;
      missile.chaffResistance = Missiles_Resistance;

      missile.acceleration        = Missiles_Acceleration;
      missile.turnRate            = RadiansFromDegrees(Missiles_TurnRate);
      missile.initialSpeed        = Missiles_InitialSpeed;
      //missile.armTime             = Missiles_ArmTime;
      missile.lockTime            = Missiles_LockTime;
      missile.readyTime           = Missiles_ReadyTime;
      missile.dispersion          = Missiles_Dispersion;
      missile.lockAngle           = RadiansFromDegrees(Missiles_LockAngle);
      missile.power               = Missiles_Power;
      missile.damageType          = Missiles_DamageType;

      missile.blastPower          = Missiles_BlastPower;
      missile.blastRadius         = Missiles_BlastRadius;
      missile.bDirectional        = !!Missiles_IsDirectional;
      missile.width               = Missiles_WidthOverHeight;
      missile.launchSound         = Missiles_LaunchSound;
      missile.ambientSound        = Missiles_FlightSound;

      ImissileIGC * pmissile = (ImissileIGC *) g.trekCore->CreateObject(g.timeNow, OT_missileType,
                                                &missile, sizeof(missile));
      pmissile->Release();
    }

    // Load Chaff
    SQL_GO(Chaff);
    while (SQL_SUCCEEDED(SQL_GETROW(Chaff)))
    {
      DataChaffTypeIGC chaff;
      SET_EXPEND(chaff, Chaff)

      chaff.chaffStrength = Chaff_Strength;

      IObject*   u = g.trekCore->CreateObject(g.timeNow, OT_chaffType,
                                              &chaff, sizeof(chaff));

      u->Release();
    }

    // Load shields
    SQL_GO(Shields);
    while (SQL_SUCCEEDED(SQL_GETROW(Shields)))
    {
      DataShieldTypeIGC  dst;
      SET_PARTS(dst, Shields)
      dst.rateRegen     = Shields_RegenRate;
      dst.maxStrength   = Shields_MaxHitPoints;

      dst.defenseType   = Shields_DefenseType;

      dst.activateSound = Shields_ActivateSound;
      dst.deactivateSound = Shields_DeactivateSound;

      IpartTypeIGC * pparttype = (IpartTypeIGC *) g.trekCore->CreateObject(
          g.timeNow, OT_partType, &dst, sizeof(dst));
      pparttype->Release();
    }

    SQL_GO(Cloaks);
    while (SQL_SUCCEEDED(SQL_GETROW(Cloaks)))
    {
      DataCloakTypeIGC  dct;
      SET_PARTS(dct, Cloaks)
      dct.energyConsumption   = Cloaks_EnergyConsumption;
      dct.maxCloaking         = Cloaks_MaxCloaking;
      dct.onRate              = Cloaks_OnRate;
      dct.offRate             = Cloaks_OffRate;
      dct.engageSound         = Cloaks_EngageSound;
      dct.disengageSound      = Cloaks_DisengageSound;

      IpartTypeIGC * pparttype = (IpartTypeIGC *) g.trekCore->CreateObject(
          g.timeNow, OT_partType, &dct, sizeof(dct));
      pparttype->Release();
    }

    // Load afterburners
    SQL_GO(Afterburners);
    while (SQL_SUCCEEDED(SQL_GETROW(Afterburners)))
    {
      DataAfterburnerTypeIGC dab;
      SET_PARTS(dab, Afterburners)
      dab.fuelConsumption   = Afterburners_RateBurn;
      dab.maxThrust         = Afterburners_ThrustMax;
      dab.onRate            = Afterburners_RateOn;
      dab.offRate           = Afterburners_RateOff;
      dab.interiorSound     = Afterburners_InteriorSound;
      dab.exteriorSound     = Afterburners_ExteriorSound;

      IpartTypeIGC * pparttype = (IpartTypeIGC *) g.trekCore->CreateObject(
          g.timeNow, OT_partType, &dab, sizeof(dab));
      pparttype->Release();
    }

    // Load packs
    SQL_GO(Ammo);
    while (SQL_SUCCEEDED(SQL_GETROW(Ammo)))
    {
      DataPackTypeIGC dpt;
      SET_PARTS(dpt, Ammo)
      dpt.amount      = Ammo_Qty;
      dpt.packType    = Ammo_AmmoType;

      IpartTypeIGC * pparttype = (IpartTypeIGC *) g.trekCore->CreateObject(
          g.timeNow, OT_partType, &dpt, sizeof(dpt));
      pparttype->Release();
    }

    SQL_GO(Mines);
    while (SQL_SUCCEEDED(SQL_GETROW(Mines)))
    {
      DataMineTypeIGC dmt;
      SET_EXPEND(dmt, Mines)

      dmt.endurance = float(Mines_MunitionCount) * Mines_Power;
      dmt.power = Mines_Power;
      dmt.radius = Mines_PlacementRadius;
      dmt.damageType = Mines_DamageType;

      IObject*   u = g.trekCore->CreateObject(g.timeNow, OT_mineType,
                                          &dmt, sizeof(dmt));
      u->Release();
    }

    SQL_GO(Probes);
    while (SQL_SUCCEEDED(SQL_GETROW(Probes)))
    {
      DataProbeTypeIGC dpt;
      SET_EXPEND(dpt, Probes)

      dpt.scannerRange = Probes_ScanRange;

      dpt.projectileTypeID = Probes_ProjectileTypeID;
      dpt.accuracy = float(Probes_accuracy) / 100.0f;
      dpt.dtimeBurst = float(Probes_dtimeBurst) / 1000.0f;
      dpt.dispersion = Probes_dispersion;
      dpt.ammo = Probes_ammo;
      dpt.ambientSound = Probes_AmbientSound;

      dpt.dtRipcord = (Probes_dtRipcord != 255)? float(Probes_dtRipcord) : -1.0f;

      IObject*   u = g.trekCore->CreateObject(g.timeNow, OT_probeType,
                                          &dpt, sizeof(dpt));
      u->Release();
    }

    // Load Magazines
    SQL_GO(Magazines);
    while (SQL_SUCCEEDED(SQL_GETROW(Magazines)))
    {
      DataLauncherTypeIGC magazine;
      //Other fields are not used (data is gotten from the missile type)
      magazine.partID           = Magazine_PartID;
      magazine.expendabletypeID = Magazine_ExpendableID;
      magazine.amount           = Magazine_Amount;
      magazine.launchCount      = Magazine_LaunchCount;
      magazine.successorPartID  = Magazine_SuccessorID;
      SQLSTRCPY(magazine.inventoryLineMDL, Magazine_InventoryLineMDL);

      IpartTypeIGC * pparttype = (IpartTypeIGC *) g.trekCore->CreateObject(
          g.timeNow, OT_partType, &magazine, sizeof(magazine));
      pparttype->Release();
    }

      for (i = 0; i < nCount; i++)
    {
      // Gotta get each ship as separate SQL_GO, because can't have nested SQL_GO's, and we get attachpoints for ship below
      ShipTypes_ShipTypeID = vnShipIdList[i];
      SQL_GO(ShipTypes);
      if (SQL_SUCCEEDED(sqlret = SQL_GETROW(ShipTypes)))
      {
        // Not all ship types exist, but we check 'em all anyway
        // First find out how many attachment points this ship has
        AttachPointsCount_ShipTypeID = ShipTypes_ShipTypeID;
        SQL_GO(AttachPointsCount);
        SQL_GETROW(AttachPointsCount);

        CB cbHull = sizeof(DataHullTypeIGC) + g_cAttachPoints * sizeof(HardpointData);
        DataHullTypeIGC * pdht = (DataHullTypeIGC *) GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, cbHull);

        SQLSTRCPY(pdht->modelName,   ShipTypes_ShipModelName);
        SQLSTRCPY(pdht->iconName,    ShipTypes_ShipIconName);
        SQLSTRCPY(pdht->name,        ShipTypes_Name);
        SQLSTRCPY(pdht->description, ShipTypes_Description);

        //SQLSTRCPY(pdht->pilotHUDName, ShipTypes_PilotHUD);
        //SQLSTRCPY(pdht->observerHUDName, ShipTypes_ObserverHUD);

  #define defaultPart(x) pdht->preferredPartsTypes[x-1] = ShipTypes_PreferredPart##x > 0 ? ShipTypes_PreferredPart##x : NA;

        defaultPart(1)
        defaultPart(2)
        defaultPart(3)
        defaultPart(4)
        defaultPart(5)
        defaultPart(6)
        defaultPart(7)
        defaultPart(8)
        defaultPart(9)
        defaultPart(10)
        defaultPart(11)
        defaultPart(12)
        defaultPart(13)
        defaultPart(14)

  #undef defaultPart

        pdht->hardpointOffset                = sizeof(DataHullTypeIGC);
        pdht->price                          = ShipTypes_Price;
        pdht->mass                           = ShipTypes_flWeight;
        pdht->hullID                         = ShipTypes_ShipTypeID;
        pdht->length                         = ShipTypes_flLength;
        pdht->speed                          = ShipTypes_SpeedMax;
        pdht->maxTurnRates[c_axisYaw]        = ShipTypes_flRateYaw    * pi / 180.0f;
        pdht->maxTurnRates[c_axisPitch]      = ShipTypes_flRatePitch  * pi / 180.0f;
        pdht->maxTurnRates[c_axisRoll]       = ShipTypes_flRateRoll   * pi / 180.0f;
        float           fYaw   = ShipTypes_flRateYaw   * ShipTypes_flRateYaw   / (2.0f * ShipTypes_flDriftYaw);
        float           fPitch = ShipTypes_flRatePitch * ShipTypes_flRatePitch / (2.0f * ShipTypes_flDriftPitch);
        float           fRoll  = ShipTypes_flRateRoll  * ShipTypes_flRateRoll  / (2.0f * ShipTypes_flDriftPitch);
        pdht->turnTorques[c_axisYaw]         = pdht->mass * fYaw   * pi / 180.0f;
        pdht->turnTorques[c_axisPitch]       = pdht->mass * fPitch * pi / 180.0f;
        pdht->turnTorques[c_axisRoll]        = pdht->mass * fRoll  * pi / 180.0f;
        pdht->thrust                         = pdht->mass * ShipTypes_flAcceleration;
        pdht->sideMultiplier                 = ShipTypes_flAccelSideMult;
        pdht->backMultiplier                 = ShipTypes_flAccelBackMult;
        pdht->signature                      = (float)ShipTypes_BaseSignature / 100.0f;
        pdht->scannerRange                   = ShipTypes_RangeScanner;
        pdht->hitPoints                      = ShipTypes_HitPoints;
        pdht->defenseType                    = ShipTypes_DefenseType;
        pdht->maxEnergy                      = ShipTypes_EnergyMax;
        pdht->habmCapabilities               = ShipTypes_Capabilities;
        pdht->rechargeRate                   = ShipTypes_RateRechargeEnergy;

        /*
        assert (ShipTypes_flRateYaw == ShipTypes_flRatePitch);
        assert (ShipTypes_flRateYaw == ShipTypes_flRateRoll);

        assert (ShipTypes_flDriftYaw == ShipTypes_flDriftPitch);
        assert (ShipTypes_flDriftYaw == ShipTypes_flDriftPitch);

        debugf("%s\n", pdht->name);
        debugf("\t%16.3f%16.3f\n", ShipTypes_flRateYaw,   fYaw);
        debugf("\t%16.3f%16.3f\n", ShipTypes_flRatePitch, fPitch);
        debugf("\t%16.3f%16.3f\n", ShipTypes_flRateRoll,  fRoll);
        */

        pdht->ecm                            = ShipTypes_ecm;
        pdht->ripcordSpeed                   = ShipTypes_ripcordSpeed * g.trekCore->GetFloatConstant(c_fcidRipcordTime);
        pdht->ripcordCost                    = ShipTypes_ripcordCost;

        pdht->maxFuel = float(ShipTypes_MaxFuel);
        pdht->maxAmmo = ShipTypes_MaxAmmo;

        // set all ships to the largest capacity necessary to hold anything, until we get the numbers from the database.
        pdht->pmEquipment[ET_ChaffLauncher] = ShipTypes_ChaffPartMask;
        pdht->pmEquipment[ET_Shield] = ShipTypes_ShieldPartMask;
        pdht->pmEquipment[ET_Afterburner] = ShipTypes_AfterburnerPartMask;
        pdht->pmEquipment[ET_Cloak] = ShipTypes_CloakPartMask;
        pdht->pmEquipment[ET_Magazine] = ShipTypes_MagazinePartMask;
        pdht->pmEquipment[ET_Dispenser] = ShipTypes_DispenserPartMask;

        pdht->capacityMagazine = ShipTypes_MagazineCapacity;
        pdht->capacityDispenser = ShipTypes_DispenserCapacity;
        pdht->capacityChaffLauncher = ShipTypes_ChaffCapacity;

        pdht->successorHullID = ShipTypes_SuccessorID;

        pdht->interiorSound = ShipTypes_InteriorSound;
        pdht->exteriorSound = ShipTypes_ExteriorSound;
        pdht->mainThrusterInteriorSound = ShipTypes_ThrustInteriorSound;
        pdht->mainThrusterExteriorSound = ShipTypes_ThrustExteriorSound;
        pdht->manuveringThrusterInteriorSound = ShipTypes_TurnInteriorSound;
        pdht->manuveringThrusterExteriorSound = ShipTypes_TurnExteriorSound;

        TechsListToBits(ShipTypes_TechBitsReqd, pdht->ttbmRequired);
        TechsListToBits(ShipTypes_TechBitsEffect, pdht->ttbmEffects);
        assert (pdht->ttbmEffects <= pdht->ttbmRequired);

        AttachPoints_ShipTypeID = ShipTypes_ShipTypeID;
        pdht->maxWeapons = 0;
        pdht->maxFixedWeapons = 0;

        pdht->timeToBuild = ShipTypes_TimeToBuild;
        pdht->groupID = ShipTypes_GroupID;

        SQL_GO(AttachPoints);
        {
          HardpointData * phpd = (HardpointData*)((char*)pdht + pdht->hardpointOffset);
          while (SQL_SUCCEEDED(sqlret = SQL_GETROW(AttachPoints)))
          {
            SQLSTRCPY(phpd->frameName, AttachPoints_FrameName);
            //SQLSTRCPY(phpd->hudName, AttachPoints_GunnerHUD);
            SQLSTRCPY(phpd->locationAbreviation, AttachPoints_LocationAbreviation);
            phpd->bFixed = (AttachPoints_PartTypeID == ET_Weapon);


            //Never more than 4 unmanned weapons (and they have to be listed
            //first)
            assert ((!phpd->bFixed) ||
                    (( (char*)phpd - ((char*)pdht + pdht->hardpointOffset) ) <
                     c_maxUnmannedWeapons * sizeof(HardpointData)));

            /*
            phpd->minDot = (AttachPoints_FieldOfFire < 270)
                           ? float(cos(0.5f * RadiansFromDegrees(float(AttachPoints_FieldOfFire))))
                           : -1.0f;
                           */

            phpd->interiorSound = AttachPoints_InteriorSound;
            phpd->turnSound = AttachPoints_TurnSound;
            phpd->partMask = AttachPoints_PartMask;

            pdht->maxWeapons++;
            if (phpd->bFixed)
              pdht->maxFixedWeapons++;

            phpd++;
          }
          //assert (iAttach == g.cAttachPoints);
        }

        /*
        {
            HullAbilityBitMask  habm = pdht->habmCapabilities;

            habm &= ~c_habmRemoteLeadIndicator;

            if (habm != pdht->habmCapabilities)
            {
                debugf("%-20s %8d %8d -> %8d\n", pdht->name, pdht->hullID, pdht->habmCapabilities, habm);
            }
        }
        */

        IhullTypeIGC * phulltype = (IhullTypeIGC *) g.trekCore->CreateObject(g.timeNow, OT_hullType,
                                                   pdht, cbHull);
        phulltype->Release();
      }
    }

    // Load Developments

    // to preserve DB ordering of developments, get the ids in database order
    SQL_GO(DevelopmentsCount);
    SQL_GETROW(DevelopmentsCount);
    DevelopmentID* vnDevelopmentIdList = (DevelopmentID*)_alloca(g_cDevelopments * sizeof(DevelopmentID));
    nCount = 0;
    SQL_GO(DevelopmentsIDs);
    while (SQL_SUCCEEDED(sqlret = SQL_GETROW(DevelopmentsIDs)))
    {
        vnDevelopmentIdList[nCount] = DevelopmentIDs_DevelopmentID;
        ++nCount;
        assert(nCount <= g_cDevelopments);
    }

    for (i = 0; i < nCount; i++)
    {
      // Gotta get each dev as separate SQL_GO, because can't have nested SQL_GO's, and we get other stuff below
      Developments_ID = vnDevelopmentIdList[i];
      SQL_GO(Developments);
      if (SQL_SUCCEEDED(sqlret = SQL_GETROW(Developments)))
      {
        DataDevelopmentIGC dd;
        dd.price = Developments_Price;
        SQLSTRCPY(dd.modelName, Developments_Bitmap);
        SQLSTRCPY(dd.name, Developments_Name);
        SQLSTRCPY(dd.iconName, Developments_IconName);
        SQLSTRCPY(dd.description, Developments_Description);
        TechsListToBits(Developments_TechBitsReqd, dd.ttbmRequired);
        TechsListToBits(Developments_TechBitsEffect, dd.ttbmEffects);
        dd.timeToBuild = Developments_SecondsToBuild;
        dd.completionSound = Developments_CompletedSound;
        dd.groupID = Developments_GroupID;
        assert (dd.timeToBuild > 0);
        dd.developmentID = Developments_ID;
        if (NA == Developments_GlobalAttribute)
          Developments_GlobalAttribute = 0; // IGC wants zero for no global attribute
        FetchGlobalAttributeSet(Developments_GlobalAttribute, dd.gas);
        IdevelopmentIGC * pdev = (IdevelopmentIGC *) g.trekCore->CreateObject(
            g.timeNow, OT_development, &dd, sizeof(dd));
        pdev->Release();
      }
    }
  /*
    // Load effects
    SQL_GO(EffectsCount);
    SQL_GETROW(EffectsCount);
    g.pargEffect = (FMD_S_EFFECT *)
        GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, g_cEffects * sizeof(g.pargEffect[0]));
    sqlret = SQL_GO(Effects);
    for(i = 0; SQL_SUCCEEDED(sqlret = SQL_GETROW(Effects)); i++)
    {
      FMD_S_EFFECT * pfmEffect = &g.pargEffect[i];
      BEGIN_PFM_CREATE_PREALLOC(g.fm, pfmEffect, S, EFFECT)
      END_PFM_CREATE
      SQLSTRCPY(pfmEffect->szFileSound,   Effect_FileSound);
      //SQLSTRCPY(pfmEffect->szDescription, Effect_Description); //NYI not used
      pfmEffect->soundID = Effect_ID;
      pfmEffect->fLooping = !!Effect_IsLooping;
    }
  */
    // Load Drones
    SQL_GO(Drones);
    while (SQL_SUCCEEDED(SQL_GETROW(Drones)))
    {
      DataDroneTypeIGC    ddt;

      ddt.price = Drones_Price;
      lstrcpy(ddt.modelName, g.trekCore->GetHullType(Drones_ShipTypeID)->GetModelName());
      ddt.iconName[0] = '\0';       //Icon never seen for drone types
      SQLSTRCPY(ddt.name, Drones_Name);
      SQLSTRCPY(ddt.description, Drones_Description);

      TechsListToBits(Drones_TechBitsReqd, ddt.ttbmRequired);
      TechsListToBits(Drones_TechBitsEffect, ddt.ttbmEffects);
      ddt.timeToBuild   = Drones_SecToBuild;
      //assert (ddt.timeToBuild > 0);
      ddt.pilotType     = (PilotType)Drones_DroneType;
      ddt.hullTypeID    = Drones_ShipTypeID;
      ddt.droneTypeID   = Drones_ID;

      ddt.shootSkill = 0.5f;    //NYI get from DB
      ddt.moveSkill = 0.5f;
      ddt.bravery = 0.5f;
      ddt.groupID = Drones_GroupID;
      ddt.etidLaid = Drones_ExpendableID;

      IdroneTypeIGC * pdronetype = (IdroneTypeIGC *) g.trekCore->CreateObject(
                                      g.timeNow, OT_droneType, &ddt, sizeof(ddt));
      pdronetype->Release();
    }

    // Load station types
    SQL_GO(StationTypes);
    while(SQL_SUCCEEDED(SQL_GETROW(StationTypes)))
    {
      DataStationTypeIGC  st;

      memset(&st, 0, sizeof(st)); //for boundschecker

      SQLSTRCPY(st.name, Stations_Name);
      SQLSTRCPY(st.description, Stations_Description);
      SQLSTRCPY(st.modelName, Stations_Model);
      SQLSTRCPY(st.iconName, Stations_IconName);
      st.textureName[0] = '\0';
      SQLSTRCPY(st.builderName, Stations_BuilderName);

      TechsListToBits(Stations_TBReqd, st.ttbmRequired);
      TechsListToBits(Stations_TBEffect, st.ttbmEffects);
      TechsListToBits(Stations_TBLocal, st.ttbmLocal);

      st.price              = Stations_Price;
      st.signature          = float(Stations_Signature) / 100.0f;
      st.maxArmorHitPoints  = Stations_HPArmor;
      st.defenseTypeArmor   = Stations_DTArmor;
      st.maxShieldHitPoints = Stations_HPShield;
      st.defenseTypeShield  = Stations_DTShield;
      st.armorRegeneration  = Stations_RateRegenArmor;
      st.shieldRegeneration = Stations_RateRegenShield;
      st.timeToBuild        = Stations_SecondsToBuild;
      assert (st.timeToBuild > 0);
      st.radius             = Stations_Radius;
      st.income             = Stations_Income;
      st.stationTypeID      = Stations_ID;
      st.sabmCapabilities   = Stations_Capabilities;
      st.scannerRange       = Stations_ScanRange;

      st.aabmBuild          = Stations_AsteroidDiscountMask;

      st.successorStationTypeID = Stations_UpgradeStationTypeID;
      st.classID            = Stations_ClassID;
      st.groupID            = Stations_GroupID;

      st.constructionDroneTypeID = Stations_ConstructionDroneTypeID;

      st.constructorNeedRockSound = Stations_BuildLocationSound;
      st.constructorUnderAttackSound = Stations_ConstructorAttackedSound;
      st.constructorDestroyedSound = Stations_ConstructorDestroyedSound;
      st.completionSound = Stations_CompletedSound;

      st.exteriorSound = Stations_ExteriorSound;
      st.interiorSound = Stations_InteriorSound;
      st.interiorAlertSound = Stations_InteriorAlertSound;

      st.underAttackSound = Stations_UnderAttackSound;
      st.criticalSound = Stations_CriticalSound;
      st.destroyedSound = Stations_DestroyedSound;
      st.capturedSound = Stations_CapturedSound;
      st.enemyDestroyedSound = Stations_EnemyDestroyedSound;
      st.enemyCapturedSound = Stations_EnemyCapturedSound;

      /*
      {
        StationAbilityBitMask   sabm = st.sabmCapabilities;
        sabm &= ~c_sabmRemoteLeadIndicator;

        if (sabm != st.sabmCapabilities)
            debugf("%-20s %8d %8d -> %8d\n", st.name, st.stationTypeID, st.sabmCapabilities, sabm);
      }
      */

      IObject* punk = g.trekCore->CreateObject(g.timeNow, OT_stationType,
              &st, sizeof(st));
      punk->Release();
    }

    // Load Civs
    SQL_GO(CivsCount);
    SQL_GETROW(CivsCount);
    for (i = 0; i < g_cCivs; i++)
    {
      // Gotta get each dev as separate SQL_GO, because can't have nested SQL_GO's, and we get other stuff below
      Civs_ID = i;
      SQL_GO(Civs);
      if (SQL_SUCCEEDED(sqlret = SQL_GETROW(Civs)))
      {
        DataCivilizationIGC c;

        SQLSTRCPY(c.name, Civs_Name);
        SQLSTRCPY(c.iconName, Civs_IconName);
        SQLSTRCPY(c.hudName, Civs_HUDName);
        c.lifepod        = Civs_DefaultShipType;

        c.initialStationTypeID = Civs_InitialStationTypeID;

        c.civilizationID = Civs_ID;
        c.bonusMoney = Civs_BonusMoney;
        c.incomeMoney = Civs_IncomeMoney;

        TechsListToBits(Civs_TechBits, c.ttbmBaseTechs);
        TechsListToBits(Civs_TechBitsNoDev, c.ttbmNoDevTechs);
        FetchGlobalAttributeSet(Civs_GlobalAttr, c.gasBaseAttributes);


        IcivilizationIGC * pciv = (IcivilizationIGC *) g.trekCore->CreateObject(
            g.timeNow, OT_civilization, &c, sizeof(c));
        pciv->Release();
      }
    }

    //Load TreasureChances
    SQL_GO(TreasureSets);
    while (SQL_SUCCEEDED(SQL_GETROW(TreasureSets)))
    {
        DataTreasureSetIGC    dts;

        SQLSTRCPY(dts.name, TreasureSets_Name);
        dts.treasureSetID = TreasureSets_ID;
        dts.bZoneOnly = (TreasureSets_IsZoneOnly != 0);
        dts.nTreasureData = 0;

        IbaseIGC* pb =  g.trekCore->CreateObject(g.timeNow, OT_treasureSet,
                                                 &dts, sizeof(DataTreasureSetIGC));
        pb->Release();
    }
    assert (g.trekCore->GetTreasureSets()->n() > 0);  // we should have at least 1

    SQL_GO(TreasureChance);
    while (SQL_SUCCEEDED(SQL_GETROW(TreasureChance)))
    {
      ItreasureSetIGC*  pts = g.trekCore->GetTreasureSet(TreasureChance_SetID);
      assert (pts);

      pts->AddTreasureData(TreasureChance_TreasureCode, TreasureChance_ObjectID, TreasureChance_Chance);
    }
    #pragma warning(default : 4244)
  #endif // !defined(ALLSRV_STANDALONE)
}

CFMGroup * GetGroupSectorDocked(IclusterIGC * pCluster)
{
  ClusterGroups * pcg = ((CFSCluster*) pCluster->GetPrivateData())->GetClusterGroups();
  return pcg->pgrpClusterDocked;
}

CFMGroup * GetGroupSectorFlying(IclusterIGC * pCluster)
{
  ClusterGroups * pcg = ((CFSCluster*) pCluster->GetPrivateData())->GetClusterGroups();
  return pcg->pgrpClusterFlying;
}

/*-------------------------------------------------------------------------
 * ExportObj
 *-------------------------------------------------------------------------
 * Purpose:
 *    export an igc object to the client
 *
 * Parameters:
 *    ppfmExport: If supplied, allocates the message there, to be freed by caller
 *
 * Side Effects:
 *    Export message queued up
 */
void ExportObj(IbaseIGC * pIGC, ObjectType ot, FMD_S_EXPORT ** ppfmExport)
{
  int cbExport = pIGC->Export(NULL);

  if (ppfmExport)
  {
    BEGIN_PFM_CREATE_ALLOC(g.fm, pfmExport, S, EXPORT)
      FM_VAR_PARM(NULL, cbExport)
    END_PFM_CREATE
    pfmExport->objecttype = ot;
    pIGC->Export(FM_VAR_REF(pfmExport, exportData));
    *ppfmExport = pfmExport;
  }
  else
  {
    BEGIN_PFM_CREATE(g.fm, pfmExport, S, EXPORT)
      FM_VAR_PARM(NULL, cbExport)
    END_PFM_CREATE
    pfmExport->objecttype = ot;
    pIGC->Export(FM_VAR_REF(pfmExport, exportData));
  }
}


/*-------------------------------------------------------------------------
 * SendPlayerInfo
 *-------------------------------------------------------------------------
 * Purpose:
 *    Tell someone about another player. In this case, the other "player" may be a drone
 *
 * Parameters:
 *    pfsPlayer: player GETTING the info, NULL = everyone
 *    pfsShip:   "player" (human or drone) being identified
 */
void SendPlayerInfo(CFSPlayer * pfsPlayer, CFSShip * pfsShip, CFSMission* pfsMission, bool bSend)
{
  const int cCivsMax = 20; // is this specified anywhere else? -- KGJV : 8 to 20, why 8 anyway ?
  int cCivs = 0;
  PersistPlayerScoreObject rgperplrScore[cCivsMax];
  if (pfsShip->IsPlayer()) // get the whole list
  {
    CFSPlayer * pfsPlayerSent = pfsShip->GetPlayer(); // the player being sent
    PersistPlayerScoreObjectList * pperplrscoList = pfsPlayerSent->GetPersistPlayerScoreObjectList();
    cCivs = pperplrscoList->n();
    assert(cCivsMax >= cCivs);
    PersistPlayerScoreObjectLink * pl = NULL;
    int i = 0;
    for (pl = pperplrscoList->first(); pl; pl = pl->next())
    {
      rgperplrScore[i++] = *pl->data();
    }
  }
  else // just get the one
  {
    cCivs = 1;
    rgperplrScore[0] = pfsShip->GetPlayerScoreObject()->GetPersist();
  }

  BEGIN_PFM_CREATE(g.fm, pfmPlayerInfo, S, PLAYERINFO)
    FM_VAR_PARM(rgperplrScore, cCivs * sizeof(rgperplrScore[0]))
  END_PFM_CREATE
  lstrcpy(pfmPlayerInfo->CharacterName, pfsShip->GetName());
  pfmPlayerInfo->fExperience      = pfsShip->GetIGCShip()->GetExperience();
  pfmPlayerInfo->nKills           = pfsShip->GetIGCShip()->GetKills();
  pfmPlayerInfo->nDeaths          = pfsShip->GetIGCShip()->GetDeaths();
  pfmPlayerInfo->nEjections       = pfsShip->GetIGCShip()->GetEjections();
  pfmPlayerInfo->dtidDrone        = pfsShip->IsPlayer()
                                   ? NA
                                   : ((CFSDrone*)pfsShip)->GetDroneTypeID();
  pfmPlayerInfo->pilotType = pfsShip->GetIGCShip()->GetPilotType();
  pfmPlayerInfo->abmOrders = pfsShip->GetIGCShip()->GetOrdersABM();
  {
      IbaseIGC* pbase = pfsShip->GetIGCShip()->GetBaseData();
      pfmPlayerInfo->baseObjectID = pbase
                                    ? pbase->GetObjectID() : NA;
  }

  IsideIGC* pside = pfsShip->GetIGCShip()->GetSide();
  if (pside)
  {
      assert (pfsMission);
      pfmPlayerInfo->iSide      = pside->GetObjectID();
  }
  else
  {
    //No side means no mission as far as the client can tell
      pfmPlayerInfo->iSide      = NA;
  }
  pfmPlayerInfo->shipID     = pfsShip->GetShipID();
  //pfmPlayerInfo->wingID     = pfsShip->GetIGCShip()->GetWingID();
  pfmPlayerInfo->rankID     = 0; //NYI
  pfmPlayerInfo->fReady     = pfsShip->IsPlayer() ? pfsShip->GetPlayer()->GetReady() : true; // drones always ready
  pfmPlayerInfo->money      = 0; //pfsShip->GetMoney() //Do not tell them how much money the player has ...  //ALLYTD imago VISIBILITY

  if (pfsShip->IsPlayer() && pfsMission && pside && pfmPlayerInfo->iSide != SIDE_TEAMLOBBY)
  {
      CFSPlayer* pfsPlayerNew = pfsShip->GetPlayer();

      pfmPlayerInfo->fTeamLeader = pfsMission->GetLeader(pfmPlayerInfo->iSide) == pfsPlayerNew;
      pfmPlayerInfo->fMissionOwner = pfsMission->GetOwner() == pfsPlayerNew;
  }
  else
  {
      pfmPlayerInfo->fTeamLeader = false;
      pfmPlayerInfo->fMissionOwner = false;
  }

  if (bSend)
  {
      CFMRecipient* prcp;
      if (pfsPlayer)
        prcp = pfsPlayer->GetConnection();
      else
        prcp = pfsMission->GetGroupMission();
      g.fm.SendMessages(prcp, FM_GUARANTEED, FM_FLUSH);
      debugf("Sending PlayerInfo for %s to %s(%u)\n", pfsShip->GetName(),
            prcp->GetName(), prcp->GetID());
  }
}


/*
void CheatSides(CFSShip * pfsShip, CFSMission * pfsMission)
{
  {
    static _CrtMemState* pstate = NULL;
    static _CrtMemState state;
    _CrtMemDumpAllObjectsSince(pstate);
    _CrtMemCheckpoint(&state);
    pstate = &state;
  }

  // show status on all sides
  SideID iSide;
  debugf("\n");
  const SideListIGC * plistSide = pfsMission->GetIGCMission()->GetSides();
  for (SideLinkIGC * plinkSide = plistSide->first; plinkSide; plinkSide = plinkSide->next())
  {
    IsideIGC * pside = plinkSide->data();
    iSide = pside->GetObjectID();
    int cMembers = pside->GetShips()->n();;
    if (pfsShip)
      SendChatMessage(NULL, CHAT_INDIVIDUAL, pfsShip, c_fCheatsOff,
          "Side %d-%s has %d stations and %d ships:", iSide,
          g.rgMissions[g.missionID].rgszName[iSide],
          g.trekCore->GetSide(iSide)->GetStations()->n(), cMembers);
    debugf("Side %d-%s has %d stations and %d players:\n", iSide,
        g.rgMissions[g.missionID].rgszName[iSide],
        g.trekCore->GetSide(iSide)->GetStations()->n(), cMembers);
    ShipID * rgShipIDMembers = g.rgpargShipIDMembers[g.missionID][iSide];

    int iMember;
    for (iMember = cMembers - 1; iMember >= 0; iMember--)
    {
      CFSShip * pfsshipT = CFSShip::GetShipFromID(rgShipIDMembers[iMember]);
      IshipIGC * pship = pfsshipT->GetIGCShip();
      IclusterIGC * pcluster = pfsshipT->GetCluster();
      IstationIGC * pstation = pfsshipT->GetStation();
      if (!pstation)
      {
        assert (pship);
        if (pcluster) // they're flying
        {
          Vector vPosition = pship->GetPosition();
          if (pfsShip)
            SendChatMessage(NULL, CHAT_INDIVIDUAL, pfsShip, c_fCheatsOff,
                "%s has %d HP, and is flying a %s at %d.%d.%d in sector %s",
                pship->GetName(), (int) (100.0f * pship->GetFraction()),
                pship->GetHullType()->GetName(), (int) vPosition.X(),
                (int) vPosition.Y(), (int) vPosition.Z(), pcluster->GetName());
          debugf("    %s has %d HP, and is flying a %s at %d.%d.%d in sector %s\n",
              pship->GetName(), (int) (100.0f * pship->GetFraction()),
              pship->GetHullType()->GetName(), (int) vPosition.X(),
              (int) vPosition.Y(), (int) vPosition.Z(), pcluster->GetName());
        }
        else // something weird--need to find out why this happens
        {
          if (pfsShip)
            SendChatMessage(NULL, CHAT_INDIVIDUAL, pfsShip, c_fCheatsOff,
                "%s has never left team room", pfsshipT->GetName());
          debugf("    %s has never left team room\n", pfsshipT->GetName());
        }
      }
      else // docked
      {
        assert (pcluster);
        if (pfsShip)
          SendChatMessage(NULL, CHAT_INDIVIDUAL, pfsShip, c_fCheatsOff,
              "  %s is docked at station %s in sector %s",
              pfsshipT->GetName(), pstation->GetName(), pcluster->GetName());
        debugf("  %s is docked at station %s in sector %s\n",
            pfsshipT->GetName(), pstation->GetName(), pcluster->GetName());
      }
    }
  }
}
*/

CFSDrone * CreateStockDrone(IdroneTypeIGC * pdronetype, IsideIGC* pSide, IbaseIGC* pbase)
{
    PilotType pt = pdronetype->GetPilotType();
    ImissionIGC*    pmission = pSide->GetMission();
    IshipIGC*   pship = CreateDrone(pmission,
                                    -2,
                                    pt,
                                    NULL,
                                    pdronetype->GetHullTypeID(),
                                    pSide,
                                    (pt == c_ptMiner) ? c_aabmMineHe3 : 0,
                                    pdronetype->GetShootSkill(),
                                    pdronetype->GetMoveSkill(),
                                    pdronetype->GetBravery());

    assert (pship);
    if (pbase)
        pship->SetBaseData(pbase);
    pship->CreateDamageTrack();

    CFSDrone * pfsDrone = new CFSDrone(pship);

    pfsDrone->SetDroneTypeID(pdronetype->GetObjectID());


    {
        char    bfr[c_cbName + 10];

        const char* pszName;
        if (pt == c_ptBuilder)
        {
            assert (pbase);
            assert (pbase->GetObjectType() == OT_stationType);
            pszName = ((IstationTypeIGC*)pbase)->GetBuilderName();
        }
        else
        {
            pszName = pdronetype->GetName();
        }
        assert (strlen(pszName) < c_cbName - 4);
        strcpy(bfr, pszName);

        _itoa(pship->GetObjectID(), bfr + strlen(bfr), 10);
        pship->SetName(bfr);
    }

    pfsDrone->SetDroneTypeID(pdronetype->GetObjectID());

    // Tell everyone about the new player
    SendPlayerInfo(NULL, pfsDrone, (CFSMission *)(pmission->GetPrivateData()));
    pfsDrone->ShipStatusStart(NULL);

    pfsDrone->SetLastDamageReport(pship->GetLastUpdate());

    return pfsDrone;
}

bool    LeaveShip(CFSShip* pfsShip, IshipIGC*  pshipParent)
{
    assert (pfsShip);
    assert (pfsShip->IsPlayer());
    assert (pshipParent);

    bool    bLeft = false;

    if (pshipParent->GetCluster() == NULL)
    {
        IshipIGC*   pship = pfsShip->GetIGCShip();

        assert (pship != pshipParent);
        assert (pship->GetParentShip() == pshipParent);
        assert (pship->GetCluster() == NULL);

        CFSShip*    pfsShipParent = (CFSShip*)(pshipParent->GetPrivateData());
        assert (pfsShipParent->IsPlayer());

        pship->SetParentShip(NULL);

        bLeft = true;

        IsideIGC*   pside = pship->GetSide();
        IhullTypeIGC*   pht = pside->GetCivilization()->GetLifepod();
        pship->SetBaseHullType(pht);

        ShipStatus* pss = pfsShip->GetShipStatus(pside->GetObjectID());
        pss->SetState(c_ssDocked);
		pss->SetStateTime(g.timeNow.clock());
        pss->SetParentID(NA);
        pss->SetHullID(pht->GetObjectID());
        assert (pship->GetStation());
        assert (pss->GetStationID() == pship->GetStation()->GetObjectID());

        BEGIN_PFM_CREATE(g.fm, pfmLeave, S, LEAVE_SHIP)
        END_PFM_CREATE

        pfmLeave->sidChild = pship->GetObjectID();
        g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
    }

    return bLeft;
}

bool    BoardShip(CFSShip* pfsShip, IshipIGC*  pshipBoard)
{
    assert (pfsShip);
    assert (pfsShip->IsPlayer());
    assert (pshipBoard);

    bool    bBoarded = false;

    IshipIGC*   pship = pfsShip->GetIGCShip();

    if (pship != pshipBoard)
    {
        IhullTypeIGC*   phtBoard = pshipBoard->GetBaseHullType();
        if (phtBoard)
        {
            IshipIGC*       pshipOldParent = pship->GetParentShip();
            CFSShip*        pfsShipBoard = (CFSShip*)(pshipBoard->GetPrivateData());
            IstationIGC*    pstationBoard = pshipBoard->GetStation();
            if (pfsShipBoard->IsPlayer() &&
                ((pshipBoard == pshipOldParent) ||      //Moving within the same ship
                 ((pship->GetCluster() == NULL) &&              //Boarding a ship from elsewhere
                  (pship->GetStation() != NULL) &&
                  (pshipBoard->GetCluster() == NULL) &&
                  (pstationBoard != NULL))))
            {
                Mount   turret;
                {
                    Mount   minMount = phtBoard->GetMaxFixedWeapons();
                    Mount   maxMount = phtBoard->GetMaxWeapons();
                    if (minMount == maxMount)
                        turret = NA;
                    else
                    {
                        Mount startMount = pship->GetTurretID();
                        if (startMount == NA)
                            startMount = maxMount - 1;
                        else
                            assert (pship->GetParentShip() != NULL);

                        turret = startMount;
                        while (true)
                        {
                            turret++;

                            if (turret >= maxMount)
                                turret = minMount;

                            //Is this position occupied?
                            ShipLinkIGC*   psl;
                            for (psl = pshipBoard->GetChildShips()->first();
                                 (psl != NULL);
                                 psl = psl->next())
                            {
                                if (psl->data()->GetTurretID() == turret)
                                    break;
                            }

							//Is this position equipped? Turkey 07/10 #145
							PartLinkIGC*   ppl;
                            for (ppl = pshipBoard->GetParts()->first();
                                 (ppl != NULL);
                                 ppl = ppl->next())
                            {
                                if (ppl->data()->GetMountID() == turret)
                                    break;
                            }

                            if (psl == NULL && ppl != NULL) //end #145
                            {
                                //Not occupied, is equipped
                                break;
                            }

                            if (turret == startMount)
                            {
                                //No available slots ... become an observer
                                turret = NA;
                                break;
                            }
                        }
                    }
                }

                //Prevent anyone from becoming an observer unless they can cheat
                if ((turret != NA) || pfsShip->GetPlayer()->CanCheat())
                {
                    {
                        //Anyone onboard pship now needs to fend for themselves
                        const ShipListIGC*  pchildren = pship->GetChildShips();

                        //If we have kids and we are boarding another ship, we must be at a station
                        assert ((pchildren->n() == 0) || (pship->GetCluster() == NULL));

                        ShipLinkIGC*        psl;

                        while (psl = pchildren->first())  //Intentional ==
                        {
                            LeaveShip((CFSShip*)(psl->data()->GetPrivateData()), pship);
                        }
                    }


                    IsideIGC*   pside = pship->GetSide();

                    {
                        //The player is about to lose some expensive hardware ... credit them for it
                        Money   refund = pship->GetValue();
                        assert(refund >= 0);

                        if (refund > 0)
                        {
                            pfsShip->SetMoney(pfsShip->GetMoney() + refund);

                            BEGIN_PFM_CREATE(g.fm, pfmMoney, S, MONEY_CHANGE)
                            END_PFM_CREATE

                            pfmMoney->sidTo = pfsShip->GetShipID();
                            pfmMoney->sidFrom = NA;
                            pfmMoney->dMoney = refund;

                            g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
                        }
                    }
                    pship->SetParentShip(pshipBoard);
                    pship->SetTurretID(turret);

                    bBoarded = true;

                    ShipStatus* pss = pfsShip->GetShipStatus(pside->GetObjectID());

                    pss->SetState(turret == NA ? c_ssObserver : c_ssTurret);
                    pss->SetParentID(pshipBoard->GetObjectID());
                    pss->SetHullID(NA);

                    if (pstationBoard)
                        pship->SetStation(pstationBoard);

                    pfsShipBoard->QueueLoadoutChange();

                    IclusterIGC* pcluster = pship->GetCluster();
                    if (pcluster)
                    {
                        assert (pshipOldParent == pshipBoard);
                        g.fm.SendMessages(GetGroupSectorFlying(pcluster), FM_GUARANTEED, FM_FLUSH);
                    }
                    else
                    {
                        g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
                    }
                }
            }
        }
    }

    return bBoarded;
}

/*-------------------------------------------------------------------------
 * CheatCreateDrones
 *-------------------------------------------------------------------------
 * Purpose:
 *    handles cheat code to create drones
 *
 * Parameters:
 *    ship issuing cheat, cheat string
 *
 * Returns:
 *    whether cheat string could be successfully evaluated
 */
bool CheatCreateDrones(CFSShip * pfsShip, ZString & strCheat)
{
  IclusterIGC*  pcluster = pfsShip->GetIGCShip()->GetCluster();
  if (!pcluster)
    return false;

  CFSMission * pfsMission = pfsShip->GetMission();
  // Syntax: CreateDrones [DroneType] [[# of drones] [DroneSide] [shipTypeID]]
  bool fIsCheat = true;
  ZString strDroneType(strCheat.GetToken());
  ZString strNumDrones(strCheat.GetToken());
  ZString strDroneSide(strCheat.GetToken());
  ZString strMessage(strCheat.GetToken());
  ZString strShipTypeID(strCheat.GetToken());

  if (strDroneType.IsEmpty())
      fIsCheat = false;
  else
  {
    short iBucket = 0;
    IhullTypeIGC * phull = NULL;
    PilotType ptPilotType;
    IdroneTypeIGC * pdronetype = NULL;
    IsideIGC* psideDrone;
    int iNumDrones;

    if (!lstrcmpi(strDroneType, "carrier"))
        ptPilotType = c_ptCarrier;
    else if (!lstrcmpi(strDroneType, "miner"))
        ptPilotType = c_ptMiner;
    else if (!lstrcmpi(strDroneType, "wingman"))
        ptPilotType = c_ptWingman;
    else
    {
        pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                         NA, "%s is not a valid drone type.  Aborting cheat code.", (PCC) strDroneType);
        fIsCheat = false;
    }

    if (fIsCheat)
    {
      // We have to look through the drone types and look for one of the correct type
      // If there's more than one, too bad, you get the first one we find
      const DroneTypeListIGC * pdronelist = pfsMission->GetIGCMission()->GetDroneTypes();
      DroneTypeLinkIGC * pdronelink;
      for (pdronelink = pdronelist->first(); pdronelink; pdronelink = pdronelink->next())
      {
        pdronetype = pdronelink->data();
        if (pdronetype->GetPilotType() == ptPilotType)
          break;
      }
      // If we couldn't find one, they don't get a drone?
      if (!pdronetype)
      {
        pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                         NA, "There are not any %s type drones defined. Aborting cheat code.", (const char*)strDroneType);
        fIsCheat = false;
      }
    }

    if (fIsCheat)
    {
      iNumDrones = strNumDrones.IsEmpty() ? 1 : atoi(strNumDrones);

      if (!strDroneSide.IsEmpty())
      {
        psideDrone = pfsMission->GetIGCMission()->GetSide(atoi(strDroneSide));
        if (!psideDrone)
          fIsCheat = false;
      }
      else
      {
        psideDrone = pfsShip->GetSide();
      }
    }

    if (fIsCheat)
    {

      {
        Vector p = Vector::GetZero();

        if (!pfsShip->GetStation())      // only if I am undocked... otherwise make them aroun 0,0,0
          p = pfsShip->GetIGCShip()->GetSourceShip()->GetPosition();

        for (int i=0; i < iNumDrones; i++)
        {
          p.x += random(-200.0f, 200.0f);
          p.y += random(-200.0f, 200.0f);
          p.z += random(-200.0f, 200.0f);
          CFSDrone* pdrone = CreateStockDrone(pdronetype, psideDrone);
          assert (pdrone);
          pdrone->GetIGCShip()->SetAmmo(0x7fff);
          pdrone->GetIGCShip()->SetPosition(p);
          pdrone->GetIGCShip()->SetCluster(pcluster);
          pdrone->ShipStatusLaunched();
        }
      }
    }
  }
  return fIsCheat;
}


#ifdef MOVE_TO_FEDMESSAGING
BOOL FAR PASCAL EnumPlayersCallback(DPID dpId, DWORD dwPlayerType,
        LPCDPNAME lpName, DWORD dwFlags, LPVOID lpContext)
{
  DataEnumPlayers * pdep = (DataEnumPlayers *) lpContext;
  CFSMission * pfsMission = pdep->pfsShip->GetMission();
  char rgchdpn[256]; // buffer for DPNAME
  char szIndent[15];
  assert (pdep->indent < sizeof(szIndent));
  szIndent[pdep->indent] = '\0';
  int i = pdep->indent;
  while (i--)
    szIndent[i] = ' ';
  if (DPPLAYERTYPE_GROUP == dwPlayerType)
  {
    DPNAME * pdpn = (DPNAME *) rgchdpn;
    DWORD dwSize = sizeof(rgchdpn);
    ZSucceeded(g.fm.GetDPlay()->GetGroupName(dpId, rgchdpn, &dwSize));
    if (pdep->pfsShip)
      pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pdep->pfsShip->GetIGCShip()->GetObjectID(),
                                       NA, "%s%s%s (%u): ",
                                       szIndent, pdpn->lpszShortNameA, pdpn->lpszLongNameA, dpId);

    debugf("%s%s%s (%u): ", szIndent, pdpn->lpszShortNameA, pdpn->lpszLongNameA, dpId);
    DataEnumPlayers depNew = *pdep;
    depNew.indent += 2;
    // First enumerate players
    ZSucceeded(g.fm.GetDPlay()->EnumGroupPlayers( dpId, NULL, EnumPlayersCallback, (LPVOID) &depNew, DPENUMPLAYERS_ALL));
    debugf("\n");
    // Then enumerate child groups
    ZSucceeded(g.fm.GetDPlay()->EnumGroupsInGroup(dpId, NULL, EnumPlayersCallback, (LPVOID) &depNew, DPENUMGROUPS_LOCAL));
  }
  else
  {
    CFSShip * pfsShip = NULL;
    DWORD dwSize = sizeof(CFSShip);
    ZSucceeded(g.fm.GetDPlay()->GetPlayerData(dpId, &pfsShip, &dwSize, DPGET_LOCAL));
    if (pdep->pfsShip)
      pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pdep->pfsShip->GetIGCShip()->GetObjectID(), NA, "%s%s", szIndent, pfsShip->GetName());
    debugf("%s, ", pfsShip->GetName());
  }

  return TRUE;
}

/*-------------------------------------------------------------------------
 * CheatDPGroups
 *-------------------------------------------------------------------------
 * Purpose:
 *    Cheat code to enumerate all our dpgroups. For debugging purposes only
 *
 * Parameters:
 *    player requesting
 */
void CheatDPGroups(CFSShip * pfsShip)
{
  DataEnumPlayers dep;
  dep.pfsShip = pfsShip;
  dep.indent = 0;
  g.fm.GetDPlay()->EnumGroups(NULL, EnumPlayersCallback, &dep, DPENUMGROUPS_LOCAL);
  if (pfsShip && pfsShip->IsPlayer())
  {
    CFSPlayer * pfsPlayer = pfsShip->GetPlayer();
    char rgchdpn[128];
    DPNAME * pdpn = (DPNAME *) rgchdpn;
    DWORD dwSize = sizeof(rgchdpn);
    DPID dpidGroup = pfsPlayer->GetGroup();
    ZSucceeded(g.fm.GetDPlay()->GetGroupName(dpidGroup, rgchdpn, &dwSize));
    pfsShip->GetMission()->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                                NA, "You're in group %s%s (%u)", pdpn->lpszShortNameA, pdpn->lpszLongNameA, dpidGroup);
  }
}
#endif // MOVE_TO_FEDMESSAGING


/*-------------------------------------------------------------------------
 * CheatJPD
 *-------------------------------------------------------------------------
 * Purpose:
 *    The Joel P Dehlin cheat to create a friendly and enemy ship in front of you
 *    First find a wingman drone type. We have to look through the drone
 *    types and look for one of the correct type. If there's more than one,
 *    too bad, you get the first one we find.
 */
void CheatJPD(CFSShip * pfsShip)
{
  IclusterIGC*  pcluster = pfsShip->GetIGCShip()->GetCluster();
  if (!pcluster) // do nothing if you try this while docked.
    return;

  IdroneTypeIGC * pdronetype = NULL;
  CFSMission * pfsMission = pfsShip->GetMission();
  const DroneTypeListIGC * pdronelist = pfsMission->GetIGCMission()->GetDroneTypes();
  DroneTypeLinkIGC * pdronelink;
  for (pdronelink = pdronelist->first(); pdronelink && !pdronetype; pdronelink = pdronelink->next())
  {
    IdroneTypeIGC * pdronetypeT = pdronelink->data();
    if (pdronetypeT->GetPilotType() == c_ptWingman)
      pdronetype = pdronetypeT;
  }
  // If we couldn't find one, no cheat?
  if (!pdronetype)
  {
    pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                     NA, "There are not any wingman type drones defined. Aborting cheat code.");
    return;
  }

  IshipIGC * pship = pfsShip->GetIGCShip();
  const Orientation& o = pship->GetOrientation();
  const float flDistanceFromCenter = 200.0f;
  const float flDistanceFromShip = 600.0f;
  Vector p = pship->GetPosition() - o.GetBackward() * flDistanceFromShip +
                                     o.GetRight()   * flDistanceFromCenter;


  // Here's the friendly
  CFSDrone * pfsDrone = CreateStockDrone(pdronetype, pship->GetSide());

  pfsDrone->GetIGCShip()->SetAmmo(0x7fff);
  pfsDrone->GetIGCShip()->SetPosition(p);
  pfsDrone->GetIGCShip()->SetCluster(pship->GetCluster());
  pfsDrone->ShipStatusLaunched();

  // Now we need to find an enemy side
  const SideListIGC * plistSide = pfsMission->GetIGCMission()->GetSides();
  IsideIGC * psideEnemy = NULL;
  IsideIGC * psideMe = pship->GetSide();
  for (SideLinkIGC * plinkSide = plistSide->first();
       plinkSide && NULL == psideEnemy;
       plinkSide = plinkSide->next())
  {
    IsideIGC * psideT = plinkSide->data();
    if (psideT != psideMe) // we found a side
      psideEnemy = psideT;
  }

  if (psideEnemy)
  {
    p += o.GetRight() * (-2 * flDistanceFromCenter); // go back mirrored across the forward verctor
    pfsDrone = CreateStockDrone(pdronetype, psideEnemy);
    pfsDrone->GetIGCShip()->SetAmmo(0x7fff);
    pfsDrone->GetIGCShip()->SetPosition(p);
    pfsDrone->GetIGCShip()->SetCluster(pship->GetCluster());
    pfsDrone->ShipStatusLaunched();
  }
  else
  {
    pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                     NA, "Cannot create enemy ship because an enemy side does not exist.");
  }
}


void LogTrapHack(AGCEventID idEvent, CFSPlayer* pfsPlayer,
  CFMConnection* pcnxnFrom, const char* pExpr, UINT nLine)
{
  // Determine the event parameters
  LPCSTR      pszContext = NULL;
  const char* pszSubject = NULL;
  AGCUniqueID idSource   = -1;
  if (pfsPlayer)
  {
    pszContext = pfsPlayer->GetIGCShip() ?
      pfsPlayer->GetIGCShip()->GetMission()->GetContextName() : NULL;
    pszSubject = pfsPlayer->GetName();
    idSource =
      static_cast<USHORT>(CAdminUser::DetermineID(pfsPlayer))
        | static_cast<DWORD>(AGC_AdminUser << 16);
  }

  // Perform special processing for each event type
  switch (idEvent)
  {
    case EventID_HackLog:
      assert(pfsPlayer);
      assert(pszSubject);
      assert(pcnxnFrom);
      retailf("HackLog %s: %s %d\n", pszSubject, pExpr, nLine);
      break;
    case EventID_HackBoot:
      assert(pfsPlayer);
      assert(pszSubject);
      assert(pcnxnFrom);
      retailf("HackBoot %s: %s %d\n", pszSubject, pExpr, nLine);
      g.fm.DeleteConnection(*pfsPlayer->GetConnection());
      break;
    case EventID_HackBootNoPlayer:
      assert(!pfsPlayer);
      assert(!pszSubject);
      assert(pcnxnFrom);
      retailf("HackBootNoPlayer %s %d\n", pExpr, nLine);
      g.fm.DeleteConnection(*pcnxnFrom);
      break;
    default:
      ZError("Unexpected AGCEventID passed to LogTrapHack.");
  }

  // Trigger the event
  _AGCModule.TriggerContextEvent(NULL, idEvent, pszContext,
    pszSubject, idSource, -1, -1, 2,
    "Expr", VT_LPSTR, pExpr,
    "Line", VT_I4   , nLine);
}


/*-------------------------------------------------------------------------
 * DoCheatCode
 *-------------------------------------------------------------------------
 * Purpose:
 *    Parse chat line, and do something
 *
 * Parameters:
 *    pfsChip:      The ship sending the cheat
 *    szMessage:    The cheat
 *
 * Returns:
 *    Whether message was indeed a cheat, and hence doesn't need to be sent as chat
 */
bool DoCheatCode(CFSShip * pfsShip, const char * szMessage)
{
  CFSMission * pfsMission = pfsShip->GetMission();
  bool fIsCheat = true;
  ZString strCheat(szMessage);
  ZString strToken = strCheat.GetToken();
  // debugf("!!! Cheat from %s: %s\n", pfsShip->GetName(), (PCC) strToken);

  if (!lstrcmpi(strToken, "bootme"))
  {
    Sleep(1000); // give the client a chance to get some messages into the receive queue--if they're flying that is
    LogTrapHack(EventID_HackBoot, pfsShip->GetPlayer(), pfsShip->GetPlayer()->GetConnection(), "bootme cheat code", __LINE__);
  }
  else if (!lstrcmpi(strToken, "spew"))
  {
    debugf("===> %s\n", (PCC) strCheat);
  }
  else if (!lstrcmpi(strToken, "stations"))
  {
    // show status on all stations
    pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                     NA, "Station ID: Name, Side (yours is %d), HP, Sector", pfsShip->GetSide()->GetObjectID());
    const StationListIGC * pstations = pfsMission->GetIGCMission()->GetStations();
    StationLinkIGC * pstationLink = NULL;
    for (pstationLink = pstations->first(); pstationLink; pstationLink = pstationLink->next())
    {
      IstationIGC * pStation = pstationLink->data();
      pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                       NA, "%d: %s%s, %d-%s, %d%%/%d%%HP, in %s",
          pStation->GetObjectID(), pStation->GetName(), (pStation->GetStationType()->GetCapabilities() & c_sabmFlag) ? " (flag)" : "",
          pStation->GetSide()->GetObjectID(), pStation->GetSide()->GetName(),
          (int)(100.0f * pStation->GetFraction()), (int)(100.0f * pStation->GetShieldFraction()), pStation->GetCluster()->GetName());
    }
  }
  else if (!lstrcmpi(strToken, "exile"))
  {
    ZString strPlayer(strCheat.GetToken());
    if (strPlayer.IsEmpty()) // must specify valid station
      fIsCheat = false;
    else
    {
      //Find a player with the given name
      for (ShipLinkIGC* psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
      {
          CFSShip*  pfsShip = (CFSShip*)(psl->data()->GetPrivateData());
          if (pfsShip->IsPlayer() && (lstrcmpi(strPlayer, psl->data()->GetName()) == 0))
          {
              // send them to the game lobby
              pfsMission->RemovePlayerFromMission(pfsShip->GetPlayer(), QSR_AdminBooted);
              break;
          }
      }
    }
  }
  else if (!lstrcmpi(strToken, "OhSayCanYouSee"))
  {
    IclusterIGC*    pcluster = pfsShip->GetIGCShip()->GetCluster();
    if (pcluster)
    {
        IsideIGC*   pside = pfsShip->GetIGCShip()->GetSide();

        for (ModelLinkIGC*  pml = pcluster->GetModels()->first(); (pml != NULL); pml = pml->next())
        {
            ImodelIGC*  pmodel = pml->data();
            if ((pmodel->GetAttributes() & c_mtStatic) != 0)
                pmodel->SetSideVisibility(pside, true);
        }
    }
    else
        fIsCheat = false;
  }
  /* // Imago moved TechBitFromToken into IGC 7/28/08
  else if (!lstrcmpi(strToken, "IfYouBuildIt"))
  {
    ZString strTech;
    if (pfsMission->GetStage() != STAGE_STARTED)
      pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                       NA, "Trying to steal a tech before the game even starts??? Sheesh--what a loser!");
    else
    {
      IsideIGC * pside = pfsShip->GetSide();
      SideID sideid = pside->GetObjectID();
	  ImissionIGC * pMission = pside->GetMission();
      while (!(strTech = strCheat.GetToken()).IsEmpty()) // get a new token, and proceed if non-empty
      {
		  int ibit = pMission->TechBitFromToken(strTech);
        if (NA != ibit)
        {
          TechTreeBitMask ttbm;
          ttbm.ClearAll();
          ttbm.SetBit(ibit);
          pside->ApplyDevelopmentTechs(ttbm);
        }
        else
        {
          pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                           NA, "Sorry, no tech found named %s.", (const char*)strTech);
        }
      }
    }
  }
  */
  else if (!lstrcmpi(strToken, "IfYouWishUponAStar"))
  {
    ZString strTech;
    if (pfsMission->GetStage() != STAGE_STARTED)
      pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                       NA, "Trying to steal a tech before the game even starts??? Sheesh--what a loser!");
    else
    {
      IsideIGC * pside = pfsShip->GetSide();
      SideID sideid = pside->GetObjectID();
      TechTreeBitMask ttbm;
      ttbm.SetAll();
      pside->ApplyDevelopmentTechs(ttbm);
    }
  }
  else if (!lstrcmpi(strToken, "fourteen"))
  {
    CFSMission * pfsMission = pfsShip->GetMission();
    if (STAGE_NOTSTARTED == pfsMission->GetStage() || STAGE_STARTING == pfsMission->GetStage())
      pfsMission->StartGame();
  }
  else if (!lstrcmpi(strToken, "fifteen"))
  {
    CFSMission * pfsMission = pfsShip->GetMission();
    if (STAGE_NOTSTARTED == pfsMission->GetStage() || STAGE_STARTING == pfsMission->GetStage())
      pfsMission->StartCountdown(15);
  }
  else if (!lstrcmpi(strToken, "ragnarok"))
  {
    // crash the server on debug builds (for testing purposes)
    assert(false);
  }
  else if (!lstrcmpi(strToken, "gimme"))
  {
    ZString strMoney(strCheat.GetToken());
    if (strMoney.IsEmpty()) // must specify valid station
      fIsCheat = false;
    else
    {
      BEGIN_PFM_CREATE(g.fm, pfmMoney, S, MONEY_CHANGE)
      END_PFM_CREATE

      pfmMoney->sidTo = pfsShip->GetShipID();
      pfmMoney->sidFrom = NA;
      pfmMoney->dMoney = std::max(-pfsShip->GetMoney(), atoi(strMoney));

      pfsShip->SetMoney(pfsShip->GetMoney() + pfmMoney->dMoney);
      g.fm.SendMessages(CFSSide::FromIGC(pfsShip->GetSide())->GetGroup(), FM_GUARANTEED, FM_FLUSH);
    }
  }
  else if (!lstrcmpi(strToken, "winwinwin"))
    pfsMission->GameOver(pfsShip->GetSide(), "By a cheat");
  else if (!lstrcmpi(strToken, "checkbank"))
    pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                     NA, "You have %d in your bank account.", pfsShip->GetMoney());

/*
  else if (!lstrcmpi(strToken, "sides"))
    CheatSides(pfsShip, pfsMission);
*/

  else if (!lstrcmpi(strToken, "capture"))
  {
    ZString strStation(strCheat.GetToken());
    if (strStation.IsEmpty()) // must specify valid station
      fIsCheat = false;
    else
    {
      StationID stationID = atoi(strStation);
      IstationIGC * pstation = pfsMission->GetIGCMission()->GetStation(stationID);
      if (pstation)
        pfsShip->CaptureStation(pstation);
      else
        pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                         NA, "Cannot capture station %d because it does not exist.", stationID);
    }
  }

  else if (!lstrcmpi(strToken, "createdrones"))
    fIsCheat = CheatCreateDrones(pfsShip, strCheat);

#ifdef OUT_OF_ORDER
  else if (!lstrcmpi(strToken, "dpgroups"))
    CheatDPGroups(pfsShip);
#endif // OUT_OF_ORDER

  else if (!lstrcmpi(strToken, "jpd"))
  {
    CheatJPD(pfsShip);
  }
  else if (!lstrcmpi(strToken, "fireaway"))
  {
    int cChats = 0;
    Time timeStart = Time::Now();
    while (Time::Now() - timeStart < 3.0) // let's send chat for thee seconds
    {
      pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                       NA, "firing away: %05d", cChats);
      cChats++;
    }
    printf("%d chats sent in 3 seconds.\n", cChats);
    pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsShip->GetIGCShip()->GetObjectID(),
                                     NA, "%d chats sent in 3 seconds.", cChats);

  }
#ifdef _TEST
  else if (!lstrcmpi(strToken, "testagcevents"))
  {
    // Do nothing if no one is listening
    AGCEventID idEvent = EventID_ChatMessage;
    HAGCLISTENERS hListeners = GetAGCGlobal()->EventListeners(idEvent, -1, -1, -1);
    if (!hListeners)
      return true;

    // Create an AGCVector (to test object persistence)
    IAGCVectorPtr spVector1, spVector2;
    ZSucceeded(GetAGCGlobal()->MakeAGCVector(&Vector(10, 20, 30), &spVector1));
    ZSucceeded(GetAGCGlobal()->MakeAGCVector(&Vector(100, 200, 300), &spVector2));

    // Get the current VariantDate
    DATE date = GetLocalVariantDateTime();

    // Create a CY object
    CY cy = {99, 12};

    LPCSTR pszContext = pfsShip->GetIGCShip() ?
      pfsShip->GetIGCShip()->GetMission()->GetContextName() : NULL;

    // Trigger a chat event with all types of parameters
    _AGCModule.TriggerContextEvent(hListeners, idEvent, pszContext,
      "TestAGCEvents cheat", -1, -1, -1, 19,
      "VT_LPWSTR"  , VT_LPWSTR  , L"I'm a wide string",
      "VT_BSTR"    , VT_BSTR    , (BSTR)CComBSTR(L"I'm a BSTR"),
      "VT_UNKNOWN" , VT_UNKNOWN , (IAGCVector*)spVector2,
      "VT_DISPATCH", VT_DISPATCH, (IAGCVector*)spVector1,
      "VT_BOOL"    , VT_BOOL    , VARIANT_TRUE,
      "VT_I1"      , VT_I1      , 0x69,
      "VT_I2"      , VT_I2      , 0xF00D,
      "VT_UI1"     , VT_UI1     , 0x96,
      "VT_UI2"     , VT_UI2     , 0xD00F,
      "VT_ERROR"   , VT_ERROR   , E_NOTIMPL,
      "VT_R4"      , VT_R4      , 1.56f,
      "VT_R8"      , VT_R8      , pi,
      "VT_CY"      , VT_CY      , cy,
      "VT_DATE"    , VT_DATE    , date,
      "Message"    , VT_LPSTR   , "This is just a test",
      "Type"       , VT_LPSTR   , "Cheat!",
      "Target"     , VT_LPSTR   , "AllSrv",
      "Source"     , VT_I4      , -1,
      "Source Name", VT_LPSTR   , "Cheater");
  }
#endif // defined(ALLOW_TESTAGCEVENTS_CHEAT) //Imago made _TEST on 8/10
  else
    fIsCheat = false;

  return fIsCheat;
}




/*-------------------------------------------------------------------------
 * HandleCOMChatEvents
 *-------------------------------------------------------------------------
 * Purpose:
 *    Generate an appropriate COM event.
 *
 * Parameters:
 *    szMessage:    the main message text
 *    szDest:       descriping who or what is receiving the message
 *    shipidFrom:   who sent the message or NA for HQ
 *    pfsMission:   the mission that the message was generated in
 *
 * Notes:
 *    This was broken off from ForwardChatMessage()
 *
 */
void HandleCOMChatEvents(const char *szMessage, const char *szDest, ShipID shipidFrom,
                         CFSMission * pfsMission, int iType, SoundID voiceOver, CommandID commandID)
{
  const char * szSource = "Admin";   // source of the chat message
  AGCUniqueID idSource = -1;
  AGCUniqueID idTeam   = -1;

  //
  // Get the GameID
  //
  AGCUniqueID idGame = pfsMission->GetIGCMission()->GetMissionID();


  //
  // Figure out source of the message
  //
  if (shipidFrom != NA)
  {
    CFSShip * pfsShipFrom = CFSShip::GetShipFromID(shipidFrom);
    if (pfsShipFrom)
    {
      IshipIGC *pShipIGC = pfsShipFrom->GetIGCShip();
      assert(pShipIGC);
      szSource = pShipIGC->GetName();
      idSource = -1;
      if (pfsShipFrom->IsPlayer())
        idSource = static_cast<USHORT>(CAdminUser::DetermineID(pfsShipFrom->GetPlayer())) | static_cast<DWORD>(AGC_AdminUser << 16);

      idTeam = pShipIGC->GetSide()->GetUniqueID();
    }
  }


  //
  // Determine the Event ID to be triggered
  //
  AGCEventID idEvent = (CHAT_ADMIN == iType) ?
    EventID_AdminPage : EventID_ChatMessage;

  //
  // Define the table of Chat Type strings
  //
  static char* s_szChatTypes[CHAT_MAX] =
  {
    "EVERYONE",              // CHAT_EVERYONE            in igc.h
    "LEADERS",               // CHAT_LEADERS             in igc.h
    "ADMIN",                 // CHAT_ADMIN               in igc.h
    "SHIP",                  // CHAT_SHIP                in igc.h
	"ALLIES",                // CHAT_ALLIES              in igc.h Imago 7/4/09 ALLY
    "TEAM",                  // CHAT_TEAM                in igc.h
    "INDIVIDUAL",            // CHAT_INDIVIDUAL          in igc.h
    "INDIVIDUAL_NOFILTER"    // CHAT_INDIVIDUAL_NOFILTER in igc.h
    "WING",                  // CHAT_WING                in igc.h
    "INDIVIDUAL_ECHO",       // CHAT_INDIVIDUAL_ECHO     in igc.h
    "ALL_SECTOR",            // CHAT_ALL_SECTOR          in igc.h
    "FRIENDLY_SECTOR",       // CHAT_FRIENDLY_SECTOR     in igc.h
    "GROUP",                 // CHAT_GROUP               in igc.h
    "GROUP_NOECHO",          // CHAT_GROUP_NOECHO        in igc.h
    "NOSELECTION",           // CHAT_NOSELECTION         in igc.h
  };

  // Ensure that addition/deletions to igc's enum ChatTarget are reflected here
  assert(sizeofArray(s_szChatTypes) == CHAT_MAX);
  assert(iType >= 0 && iType < sizeofArray(s_szChatTypes));
  LPCSTR pszType = s_szChatTypes[iType];

  LPCSTR pszContext = pfsMission->GetIGCMission() ?
    pfsMission->GetIGCMission()->GetContextName() : NULL;

  // Trigger the event if anyone is listening
  _AGCModule.TriggerContextEvent(NULL, idEvent, pszContext,
    szSource, idSource, idGame, idTeam, 7,
    "Message"      , VT_LPSTR, szMessage,
    "Type"         , VT_LPSTR, pszType,
    "Target"       , VT_LPSTR, szDest,
    "GameID"       , VT_I4   , idGame,
    "TeamID"       , VT_I4   , idTeam,
    "VoiceID"      , VT_I4   , voiceOver,
    "CommandID"    , VT_I4   , commandID);
}


bool CommandToDrone(CFSMission*         pfsMission,
                    ChatData*           pcd,
                    ObjectType          otTarget,
                    ObjectID            oidTarget,
                    const DataBuoyIGC*  pdb,
                    CFSShip*            pfsSender,
                    IshipIGC*           pshipTo,
                    ImodelIGC**         ppmodelTarget)
{
    bool    bAccepted = false;
	bool	bBarelyLegal = false; //milfs only 
	
	assert (pfsSender);

    IsideIGC*   pside = pshipTo->GetSide();
    if ((pcd->commandID != c_cidNone) &&
        (pfsSender->GetIGCShip()->GetSide() == pside))
    {
        pshipTo->SetRunawayCheckCooldown(120.0f); // don't run anytime soon while executing a player command

        IclusterIGC* pminerCluster = NULL;
        char pcReply[100] = "Affirmative.";
        if (*ppmodelTarget == NULL)
		{
            if (pdb)
            {
                if (pshipTo->GetPilotType() == c_ptMiner && pdb->type == c_buoyCluster)
                    pminerCluster = pfsMission->GetIGCMission()->GetCluster(pdb->clusterID);
                //Create a buoy for this chat message
                *ppmodelTarget = (ImodelIGC*)(pfsMission->GetIGCMission()->CreateObject(g.timeNow, OT_buoy, pdb, sizeof(*pdb)));
                assert(*ppmodelTarget);

                ((IbuoyIGC*)*ppmodelTarget)->AddConsumer();
            }
            else
            {
                *ppmodelTarget = pfsMission->GetIGCMission()->GetModel(otTarget, oidTarget);
				
				//Imago 7/10 keep drones from giving away info
				if (pshipTo->GetPilotType() == c_ptBuilder && otTarget == OT_asteroid && *ppmodelTarget == NULL) {
					//not gunna' work 'cause we dont have the asteroid anymore....;-(
					//const StationListIGC* psl = pfsMission->GetIGCMission()->GetAsteroid(oidTarget)->GetCluster()->GetStations();
					const StationListIGC* psl = pfsMission->GetIGCMission()->GetStations();
					StationLinkIGC * pstationLink = NULL;
					for (pstationLink = psl->first(); pstationLink; pstationLink = pstationLink->next()) {
						IstationIGC * pStation = pstationLink->data();
						if (pStation->GetSide() == pshipTo->GetSide()) //REVIEW ALLY VIZ
							continue;
						if (oidTarget == pStation->GetRoidID()) {
							DataBuoyIGC db;
							db.position = pStation->GetPosition();
							db.buoyID = pfsMission->GetIGCMission()->GenerateNewBuoyID();
							db.clusterID = pStation->GetCluster()->GetObjectID();
							db.type = (pStation->GetCluster() == pshipTo->GetCluster()) ? c_buoyWaypoint : c_buoyCluster;
							*ppmodelTarget = (ImodelIGC*)(pfsMission->GetIGCMission()->CreateObject(g.timeNow, OT_buoy, &db, sizeof(db)));
							assert (*ppmodelTarget);
							((IbuoyIGC*)*ppmodelTarget)->AddConsumer();
							bBarelyLegal = true; //GGW spring break
							break;
						}
					}
				}
				//
                else if (pshipTo->GetPilotType() == c_ptMiner && otTarget == OT_warp) {
                    IwarpIGC* pwarp = (IwarpIGC*)(*ppmodelTarget);
                    IwarpIGC* pothersideWarp = pwarp ? pwarp->GetDestination() : NULL;
                    if (pothersideWarp)
                        pminerCluster = pothersideWarp->GetCluster();
                }
            }

        }
        if ((pcd->commandID == c_cidDefault) && (*ppmodelTarget != NULL))
        {
            pcd->commandID = pshipTo->GetDefaultOrder(*ppmodelTarget);
        }

        if (pshipTo->LegalCommand(pcd->commandID, *ppmodelTarget) || bBarelyLegal) //Imago 7/10 drone build sphere rock info ughh
        {
            pshipTo->SetStayDocked(false);  //Xynth #48 8/2010 reset staydocked on any command
			bAccepted = true;
            pfsMission->GetSite()->SendChat(pshipTo, CHAT_INDIVIDUAL, pfsSender->GetIGCShip()->GetObjectID(),
                                            voAffirmativeSound, pcReply);

            if (pcd->commandID == c_cidJoin)
            {
                //Join the targets wing
                assert ((*ppmodelTarget)->GetObjectType() == OT_ship);

                WingID  wid = ((IshipIGC*)*ppmodelTarget)->GetWingID();

                pshipTo->SetWingID(wid);

                BEGIN_PFM_CREATE(g.fm, pfmWingChange, CS, SET_WINGID)
                END_PFM_CREATE
                pfmWingChange->shipID = pshipTo->GetObjectID();
                pfmWingChange->wingID = wid;
                pfmWingChange->bCommanded = true;
                g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
            }
            else
            {
                /*//Set both current and accepted commands
                pshipTo->SetCommand(c_cmdCurrent,
                                    *ppmodelTarget,
                                    pcd->commandID);*/
                pshipTo->SetCommand(c_cmdAccepted, //sets c_cmdCurrent as well
                                    *ppmodelTarget,
                                    pcd->commandID);
            }
        }
        else
        {
            /*
            pfsMission->GetSite()->SendChat(pshipTo, CHAT_INDIVIDUAL, pfsSender->GetIGCShip()->GetObjectID(),
                                            voNegativeSound, "Negative.");
                                            */
        }

        // Remember player commands by storing location/ID in c_cmdQueued
        if (pshipTo->GetPilotType() == c_ptMiner) {
            if ((*ppmodelTarget) &&
                (*ppmodelTarget)->GetObjectType() != OT_station &&
                (*ppmodelTarget)->GetObjectType() != OT_ship)
            {
                DataBuoyIGC         buoyData;
                buoyData.position = Vector(0.0f, 0.0f, 0.0f);
                buoyData.type = c_buoyWaypoint; // c_buoyCluster would be deleted as soon as we are in the cluster
                if (pminerCluster)
                    buoyData.clusterID = pminerCluster->GetObjectID();
                else
                    buoyData.clusterID = (*ppmodelTarget)->GetCluster()->GetObjectID();
                buoyData.buoyID = pfsMission->GetIGCMission()->GenerateNewBuoyID();
                ImodelIGC* buoy = (ImodelIGC*)(pfsMission->GetIGCMission()->CreateObject(g.timeNow, OT_buoy, &buoyData, sizeof(buoyData)));

                CommandID cid = c_cidGoto;
                if (pminerCluster)
                    cid = c_cidJoin;
                pshipTo->SetCommand(c_cmdQueued, buoy, cid);

                buoy->Release(); //SetCommand added the consumer needed
            }
            else {
                // Remember that we just got a player command -> c_cidGoto instead of None
                pshipTo->SetCommand(c_cmdQueued, NULL, c_cidGoto);
            }
        }
    }

    return bAccepted;
}

void ForwardChatMessageToWing(CFSMission*         pfsMission,
                              CFSShip*            pfsSender,
                              FEDMESSAGE*         pfm,
                              WingID              wid,
                              IshipIGC*           pshipSkip)
{

    //Send the message to everyone on the specified wing
    for (ShipLinkIGC*   psl = pfsSender->GetIGCShip()->GetSide()->GetShips()->first();
         (psl != NULL);
         psl = psl->next())
    {
        IshipIGC*   pship = psl->data();
        if ((pship->GetWingID() == wid) && (pship != pshipSkip))
        {
            CFSShip* pfsShipTo = (CFSShip*)(pship->GetPrivateData());

            if (pfsShipTo->IsPlayer())
            {
              g.fm.ForwardMessage(pfsShipTo->GetPlayer()->GetConnection(), pfm, FM_GUARANTEED);
            }
        }
    }
	// send allies if command wing and not a drone command HACK: we use shipskip as a boolean--imago 7/3/09
//	if (wid == 0 && !pshipSkip)
//		ForwardChatMessageToAllies(pfsMission,pfsSender,pfm);
}

void ForwardChatMessage(CFSMission*         pfsMission,
                        CFSShip*            pfsSender,
                        FEDMESSAGE*         pfm,
                        ChatData*           pcd,
                        const char*         pszMsg,
                        ObjectType          otTarget = NA,
                        ObjectID            oidTarget = NA,
                        const DataBuoyIGC*  pdb = NULL)
{
    if (((pcd->commandID == c_cidNone) && (pcd->voiceOver == NA) && ((pszMsg == NULL) || (*pszMsg == '\0'))) ||
        (pcd->commandID < c_cidNone) ||
        (pcd->commandID >= c_cidMax))
        return;     //Invalid chat message


    if (pfsSender &&
        pfsSender->IsPlayer() &&
        !pfsSender->GetPlayer()->DecrementChatBudget(pcd->chatTarget != CHAT_GROUP_NOECHO))
    {
        return;
    }

    char    szDestBfr[20];
    const char* pszDest = "UNKNOWN";

    switch (pcd->chatTarget)
    {
        case CHAT_EVERYONE:
        {
            pszDest = pfsMission->GetMissionDef()->misparms.strGameName;
            if (pcd->commandID == c_cidNone)
            {
                g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfm, FM_GUARANTEED);
            }
        }
        break;

        case CHAT_FRIENDLY_SECTOR:  // can happen only after game starts
        {
            assert (pfsSender); // if no ship, then how can send to sector?

            IclusterIGC* pcluster = pfsMission->GetIGCMission()->GetCluster(pcd->oidRecipient);

            if (pcluster)
            {
                pszDest = pcluster->GetName();

                //Send the message to everyone on the specified wing
                IsideIGC*   pside = pfsSender->GetIGCShip()->GetSide();

                for (ShipLinkIGC*   psl = pcluster->GetShips()->first();
                     (psl != NULL);
                     psl = psl->next())
                {
                    IshipIGC*   pship = psl->data();
                    if (pship->GetSide() == pside || pside->AlliedSides(pside,pship->GetSide())) //ALLY imago 7/10/09
                    {
                        CFSShip* pfsShipTo = (CFSShip*)(pship->GetPrivateData());

                        if (pfsShipTo->IsPlayer())
                        {
                            g.fm.ForwardMessage(pfsShipTo->GetPlayer()->GetConnection(), pfm, FM_GUARANTEED);
                        }
                        else
                        {
                            //Do not send to drones
                        }
                    }
                }
            }
        }
        break;

        case CHAT_ALL_SECTOR:       // can happen only after game starts
        {
            assert (pfsSender); // if no ship, then how can send to sector?

            IclusterIGC* pcluster = pfsMission->GetIGCMission()->GetCluster(pcd->oidRecipient);

            if (pcluster && (pcd->commandID == c_cidNone))
            {
                pszDest = pcluster->GetName();
                g.fm.ForwardMessage(GetGroupSectorDocked(pcluster), pfm, FM_GUARANTEED);
                g.fm.ForwardMessage(GetGroupSectorFlying(pcluster), pfm, FM_GUARANTEED);
            }
        }
        break;

		case CHAT_ALLIES:
        {
            pszDest = "ALLIES";
            IsideIGC*   pside = pfsMission->GetIGCMission()->GetSide(pcd->oidRecipient);
            if (pside && (pcd->commandID == c_cidNone))
            {
                g.fm.ForwardMessage(CFSSide::FromIGC(pside)->GetGroup(),
                                    pfm, FM_GUARANTEED);

				//imago fixed crash 7/13/09 removing FwdChatMsgToAllies ALLY
			    for (SideLinkIGC* psidelink = pfsMission->GetIGCMission()->GetSides()->first();
					(psidelink != NULL);
					psidelink = psidelink->next())
				{
					IsideIGC*   otherside = psidelink->data();
					if (pside->AlliedSides(pside,otherside) && pside != otherside)
                		g.fm.ForwardMessage(CFSSide::FromIGC(otherside)->GetGroup(),
                                    pfm, FM_GUARANTEED);
				}
			}
		break;
		}


        case CHAT_LEADERS:
        {
            pszDest = "LEADERS";

            int iSide = pfsMission->GetMissionDef()->misparms.nTeams;
            while (iSide-- > 0)
            {
              CFSPlayer* pfsLeader = pfsMission->GetLeader(iSide);

              if (pfsLeader && (pcd->commandID == c_cidNone))
                g.fm.ForwardMessage(pfsLeader->GetConnection(), pfm, FM_GUARANTEED);
            }
        }
        break;
        case CHAT_ADMIN:
        {
            pszDest = "ADMIN";

            //NYI
        }
        break;

        case CHAT_SHIP:
        {
            pszDest = "ship";

            IshipIGC*   pshipSource = pfsSender->GetIGCShip()->GetSourceShip();
            if (pshipSource != pfsSender->GetIGCShip())
            {
                CFSShip* pfsShipTo = (CFSShip*)(pshipSource->GetPrivateData());
                assert (pfsShipTo->IsPlayer());
                g.fm.ForwardMessage(pfsShipTo->GetPlayer()->GetConnection(), pfm, FM_GUARANTEED);
            }

            for (ShipLinkIGC*   psl = pshipSource->GetChildShips()->first();
                 (psl != NULL);
                 psl = psl->next())
            {
                IshipIGC*   pship = psl->data();
                if (pship != pfsSender->GetIGCShip())
                {
                    CFSShip* pfsShipTo = (CFSShip*)(pship->GetPrivateData());

                    assert (pfsShipTo->IsPlayer());
                    g.fm.ForwardMessage(pfsShipTo->GetPlayer()->GetConnection(), pfm, FM_GUARANTEED);
                }
            }

        }
        break;

        case CHAT_TEAM:
        {
            pszDest = pfsMission->GetMissionDef()->rgszName[pcd->oidRecipient];
            IsideIGC*   pside = pfsMission->GetIGCMission()->GetSide(pcd->oidRecipient);
            if (pside && ((pcd->commandID == c_cidNone) || (pfsSender == NULL) || (pside == pfsSender->GetSide())))
            {
                g.fm.ForwardMessage(CFSSide::FromIGC(pside)->GetGroup(),
                                    pfm, FM_GUARANTEED);
            }

        }
        break;

        case CHAT_GROUP:
        case CHAT_GROUP_NOECHO:
        {
            pcd->chatTarget = CHAT_INDIVIDUAL;
        }
        //No break;

        case CHAT_INDIVIDUAL_NOFILTER:
        case CHAT_INDIVIDUAL:
        {
            CFSShip * pfsShipTo = CFSShip::GetShipFromID(pcd->oidRecipient);
            if (pfsShipTo) // may have gone away since the client tried to send this
            {
                assert(pfsShipTo->GetIGCShip());
                pszDest = pfsShipTo->GetIGCShip()->GetName();

                if (pfsShipTo->IsPlayer())
                {
                    if ((pcd->commandID == c_cidNone) || (pfsSender == NULL) ||
                        (pfsShipTo->GetSide() == pfsSender->GetSide() || pfsShipTo->GetSide()->AlliedSides(pfsShipTo->GetSide(),pfsSender->GetSide()))) //ALLY imago 7/10/09
                    {
                        CFSPlayer *pfsPlayer = pfsShipTo->GetPlayer();
                        assert(pfsPlayer);
                        g.fm.ForwardMessage(pfsPlayer->GetConnection(), pfm, FM_GUARANTEED);
                    }
                }
                else
                {
                    ImodelIGC*  pmodel = NULL;

                    if (CommandToDrone(pfsMission, pcd, otTarget, oidTarget, pdb, pfsSender, pfsShipTo->GetIGCShip(), &pmodel))
                    {
                        //Also echo this command to the command wing, but mark individual echo so the
                        //client will know how to process it.
                        pcd->chatTarget = CHAT_INDIVIDUAL_ECHO;
                        ForwardChatMessageToWing(pfsMission, pfsSender, pfm, 0, pfsSender->GetIGCShip()); //Command

						//AEM/TE: - Forward drone orders to Charlie, Delta, and Echo wings
						ForwardChatMessageToWing(pfsMission, pfsSender, pfm, 7, pfsSender->GetIGCShip()); //Charlie
						ForwardChatMessageToWing(pfsMission, pfsSender, pfm, 8, pfsSender->GetIGCShip()); //Delta
						ForwardChatMessageToWing(pfsMission, pfsSender, pfm, 9, pfsSender->GetIGCShip()); //Echo

                    }

                    if (pdb && pmodel)
                    {
                        assert (pmodel->GetObjectType() == OT_buoy);

                        ((IbuoyIGC*)pmodel)->ReleaseConsumer();
                        pmodel->Release();
                    }
                }
            }

        }
        break;

        case CHAT_WING:
        {
            pszDest = szDestBfr;
            strcpy(szDestBfr, "Wing");
            _itoa(pcd->oidRecipient, &szDestBfr[4], 10);

            ForwardChatMessageToWing(pfsMission, pfsSender, pfm, pcd->oidRecipient, NULL);
        }
        break;
    }

    HandleCOMChatEvents(pszMsg, pszDest, pcd->sidSender, pfsMission, pcd->chatTarget, pcd->voiceOver, pcd->commandID);
}


/*-------------------------------------------------------------------------
 * CmpShipDeviation
 *-------------------------------------------------------------------------
 * Purpose:
 *    sort function for qsort for sorting ships based on deviation of
 *    what the actual position is and the current calculated position
 *    based on interpolation of inputs
 *
 * Parameters & Return:
 *    as defined by qsort. Both are actually IShipIGC*
 */
int __cdecl CmpShipDeviation (const void *elem1, const void *elem2)
{
  CFSShip * pship1 = * (CFSShip **) elem1;
  CFSShip * pship2 = * (CFSShip **) elem2;
  // we don't really expect exactly equal deviations, so don't worry about it
  // We want to sort in descending order to get the largest deviations first
  return (pship1->GetDeviation() < pship2->GetDeviation()) ? 1 : -1;
}

void    AddUpdate(Time          timeReference,
                  const Vector& positionReference,
                  CFSShip* pfsShip, ServerHeavyShipUpdate*   pHeavy)
{
    IshipIGC*   pship = pfsShip->GetIGCShip();

    pHeavy->shipID = pship->GetObjectID();

    if (pfsShip->IsPlayer())
    {
        //If it is a player ... we are adding its update because we received a ship update for the player
        const ClientShipUpdate& su = pfsShip->GetPlayer()->GetShipUpdate();

        pHeavy->time.Set(timeReference, su.time);
        pHeavy->position.Set(positionReference, su.position);

        pHeavy->controls    = su.controls;
        pHeavy->orientation = su.orientation;
        pHeavy->stateM      = su.stateM;
        pHeavy->turnRates   = su.turnRates;
        pHeavy->velocity    = su.velocity;

        pHeavy->power       = su.power;

        //The following is not part of the client update ...
        pship->ExportFractions(&(pHeavy->fractions));
    }
    else
    {
        //It is a drone ... get the data from IGC
        pship->ExportShipUpdate(timeReference, positionReference, pHeavy);
    }
}
/*-------------------------------------------------------------------------
 * SendShipUpdates
 *-------------------------------------------------------------------------
 * Purpose:
 *    Send ship updates to all clients in sector
 */
void SendShipUpdates(CFSMission * pfsMission)
{
    DWORD   nFrame = pfsMission->IncrementFrame();
    if (nFrame % 2 == 0)
        return;

    //Loop over sides and ships, queuing ship update messages as appropriate
    {
        ShipLinkIGC*  pshipsFirst = pfsMission->GetIGCMission()->GetShips()->first();
        for (SideLinkIGC*   psidelink = pfsMission->GetIGCMission()->GetSides()->first();
             (psidelink != NULL);
             psidelink = psidelink->next())
        {
            IsideIGC*   pside = psidelink->data();
            assert(0 == g.fm.CbUsedSpaceInOutbox());

            SideID  sid = pside->GetObjectID();

            for (ShipLinkIGC*   pshiplink = pshipsFirst;
                 (pshiplink != NULL);
                 pshiplink = pshiplink->next())
            {
                IshipIGC*   pship = pshiplink->data();
                //Don't send ship status for people in the lobby
                if (pship->GetSide()->GetObjectID() >= 0)
                {
                    CFSShip*    pfsShip = (CFSShip*)(pship->GetPrivateData());

                    ShipStatus* pss = pfsShip->GetShipStatus(sid);
                    ShipStatus* pssOld = pfsShip->GetOldShipStatus(sid);

                    if (*pss != *pssOld)
                    {
                        BEGIN_PFM_CREATE(g.fm, pfmShipStatus, S, SHIP_STATUS)
                        END_PFM_CREATE

                        pfmShipStatus->shipID = pship->GetObjectID();
                        pfmShipStatus->status = *pssOld = *pss;         //Also update the old status
                    }

					//Xynth Add a periodic check for achievements and IGC Score triggers
					
					if (ISPLAYER(pship))
					{
						CSteamAchievements *pSteamAchievements = pfsShip->GetPlayer()->GetSteamAchievements();
						PlayerScoreObject * pso = pfsShip->GetPlayerScoreObject();						
						if ((pship->GetAchievementMask() & c_achmNewRepair) > 0)
							pso->SetRepair(pship->GetRepair());

						pSteamAchievements->AwardIGCAchievements(pship->GetAchievementMask());
						pship->ClearAchievementMask();

					}

                }
            }

            g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
        }
    }

    const ClusterListIGC* pclstlist = pfsMission->GetIGCMission()->GetClusters();
    for (ClusterLinkIGC*    pcl = pfsMission->GetIGCMission()->GetClusters()->first(); (pcl != NULL); pcl = pcl->next())
    {
        IclusterIGC*        pcluster = pcl->data();
        const ShipListIGC*  pshiplist     = pcluster->GetShips();
        const PlayerList*   pplayerList = ((CFSCluster*)(pcluster->GetPrivateData()))->GetFlyingPlayers();

        if ((pplayerList->n() > 0) && (pshiplist->n() > 0))
        {
            //We will need to send out at least some ship updates ...
            //Given our bandwidth constraints & the population of the sector, figure out how many turret, heavy and light updates
            //we would be able to send assuming every ship is sending a ship/turret update every c_dsecUpdateClient

            assert (pshiplist->n() < c_cShipsPerSectorMax);

            static CFSShip*     rgShips[c_cShipsPerSectorMax];
            static CFSShip*     rgActiveTurrets[c_cShipsPerSectorMax];
            static CFSShip*     rgInactiveTurrets[c_cShipsPerSectorMax];

            // Iterate once to build the array of things for which we have updates (which probably is not the total list
            // of things in the cluster)
            int     cShipUpdates = 0;
            int     cActiveTurretUpdates = 0;
            int     cInactiveTurretUpdates = 0;

            int     cShips = 0;
            int     cTurrets = 0;

            for (ShipLinkIGC * pshiplink = pshiplist->first(); (pshiplink != NULL); pshiplink = pshiplink->next())
            {
                IshipIGC*   pship = pshiplink->data();
                CFSShip*    pfsShip = (CFSShip *) pship->GetPrivateData();

                assert (!pfsShip->GetHasUpdate());

                if (pfsShip->IsPlayer())
                {
                    CFSPlayer*      pfsPlayer = pfsShip->GetPlayer();
                    unsigned char   ucLastUpdate = pfsPlayer->GetLastUpdate();

                    if (pship->GetParentShip() == NULL)
                    {
                        cShips++;

                        if (ucLastUpdate == c_ucShipUpdate)
                        {
                            const ClientShipUpdate& su = pfsPlayer->GetShipUpdate();
                            float   dt = su.time - g.timeNow;

                            if ((dt >= -1.5f) && (dt <= 1.5f))
                            {
                                rgShips[cShipUpdates++] = pfsShip;
                                pfsShip->SetHasUpdate();
                            }
                        }
                    }
                    else if (pship->GetTurretID() != NA)
                    {
                        cTurrets++;

                        if (ucLastUpdate == c_ucActiveTurretUpdate)
                        {
                            const ClientActiveTurretUpdate& atu = pfsPlayer->GetActiveTurretUpdate();
                            float   dt = atu.time - g.timeNow;

                            if ((dt >= -1.5f) && (dt <= 1.5f))
                                rgActiveTurrets[cActiveTurretUpdates++] = pfsShip;
                        }
                        else if ((pship->GetStateM() & (oneWeaponIGC | allWeaponsIGC)) == 0)       //ucLastUpdate == c_ucInactiveTurretUpdate)
                            rgInactiveTurrets[cInactiveTurretUpdates++] = pfsShip;
                    }

                    pfsPlayer->ResetLastUpdate();
                }
                else
                {
                    cShips++;

                    //Generate an update for drones every third frame. The address of the ship is as good as anything
                    //else for a hash function to randomly distribute drones across various updates (ignoring the low
                    //byte of the address since those may be block aligned)
                    static const DWORD  c_updateFrequency = 2;
                    //if ( (((((DWORD)pship) >> 8) + nFrame) % c_updateFrequency) == 0)

                    //Make sure, for whatever reason, no ancient updates are sent from a drone
                    //this shouldn't happen but it could if the server gets hung for a moment.
                    float   dt = pfsShip->GetIGCShip()->GetLastUpdate() - g.timeNow;
                    if ((dt >= -1.5f) && (dt <= 1.5f))
                    {
                        //This lucky ship's turn ... generate an update for it.
                        rgShips[cShipUpdates++] = pfsShip;
                        pfsShip->SetHasUpdate();
                    }
                }
            }
            assert (cShips > 0);

            if (cShipUpdates + cActiveTurretUpdates + cInactiveTurretUpdates > 0)
            {


				/* pkk May 6th: Disabled bandwidth patch
				for (PlayerLink* pplayerLink = pplayerList->first(); (pplayerLink != NULL); pplayerLink = pplayerLink->next())
                {
                    CFSPlayer*  pfsPlayer = pplayerLink->data();*/



                //Calculate how many heavy ship & active turret updates we can afford to send and still stay within our
                //bandwidth constraints (where we can afford to send a max of 1500 bytes/second and the clients will send
                //one update each interval)
					/* pkk May 6th: Disabled bandwidth patch
					   Enable line below again */
					const int cbAvailable = int(1600.0f * c_dsecUpdateClient);

                //This is what we can possibly send and still remain within out constraints
				/* pkk May 6th: Disabled bandwidth patch
					// w0dk4 Bandwidth Patch
					const int cHeavyPerInterval  = pfsPlayer->GetBandwidth();
					const int cLightPerInterval  = pfsPlayer->GetBandwidth();
					const int cActivePerInterval = pfsPlayer->GetBandwidth();*/
					const int cHeavyPerInterval  = 2;
					const int cLightPerInterval  = 2;
					const int cActivePerInterval = 2;


					/* pkk May 6th: Disabled bandwidth patch
					   This was disabled */
                assert (cbAvailable > cHeavyPerInterval  * sizeof(ServerHeavyShipUpdate) +
                                      cLightPerInterval  * sizeof(ServerLightShipUpdate) +
                                      cActivePerInterval * sizeof(ServerActiveTurretUpdate));
					/*  End */

                int cLightUpdates;
                int cHeavyUpdates;
					/* pkk May 6th: Disabled bandwidth patch
					if (cShipUpdates <= cHeavyPerInterval)*/
					if (cShipUpdates <= 2)
                {
                    cHeavyUpdates = cShipUpdates;
                    cLightUpdates = 0;
                }
                else
                {
						/* pkk May 6th: Disabled bandwidth patch
						cHeavyUpdates = cHeavyPerInterval; */
						cHeavyUpdates = 2;
                    cLightUpdates = (cShipUpdates * cLightPerInterval) / cShips;

                    if (cHeavyUpdates + cLightUpdates > cShipUpdates)
                        cLightUpdates = cShipUpdates - cHeavyUpdates;
                }

                int cTurretUpdates;
					/* pkk May 6th: Disabled bandwidth patch
					if (cActiveTurretUpdates < cActivePerInterval)
					{

						// for every 3 unused turret updates (-> -36byte) increase heavy and lower light updates (-> +30byte)

						cLightUpdates -= ((cActivePerInterval-cActiveTurretUpdates)/3);
						if (cLightUpdates < 0)
							cLightUpdates = 0;
						cHeavyUpdates += ((cActivePerInterval-cActiveTurretUpdates)/3);
						cTurretUpdates = cActiveTurretUpdates;
					} else
						cTurretUpdates = cActivePerInterval;


					// w0dk4 Bandwidth Patch
					/* weird code, replaced with above
					*/
                if (cActiveTurretUpdates <= 1)
                {
                    if ((cLightUpdates > 0) && (cActiveTurretUpdates == 0))
                    {
                        //Special case ... no active turret updates so use the bandwidth to
                        //promote a light ship update to a heavy ship update
                        cHeavyUpdates++;
                        cLightUpdates--;
                    }
                    cTurretUpdates = cActiveTurretUpdates;
                }
                else
                {
                    cTurretUpdates = (cActiveTurretUpdates == cTurrets) ? 2 : 1;
					}
					/* pkk May 6th: Disabled bandwidth patch
					   Outcommented code ends here */

                //OK ... given our bandwidth limits ... we think we can send the following this frame
                //      cTurretUpdates      turret updates (active or inactive)
                //      cHeavyUpdates       heavy ship updates
                //      cLightUpdates       light ship updates
                //
                //without busting out limits/second
                if (cHeavyUpdates != cShipUpdates)
                {
                    //We can not send everything we'd like to send. Sort the list so that the people
                    //at the front of the list are those that really need to be sent right now
                    qsort(rgShips, cShipUpdates, sizeof(rgShips[0]), CmpShipDeviation);
                }

                {
                    //Adjust the deviation of the ships based on where they fall in the array
                    //A heavy update might get demoted to a light update (and a light to nothing)
                    //if a ship has a target (so that the target update can be sent as a heavy) ...
                    //go and treat the last heavy update as a
                    int effectiveHeavy = (cHeavyUpdates < cShipUpdates) ? cHeavyUpdates - 1 : cHeavyUpdates;
                    int effectiveLight = cHeavyUpdates +
                                         (cHeavyUpdates + cLightUpdates < cShipUpdates)
                                         ? (cLightUpdates - 1)
                                         : cLightUpdates;

                    for (int i = 0; (i < cShipUpdates); i++)
                    {
                        if (i < effectiveHeavy)
                            rgShips[i]->SetDeviation(0.0f);
                        else
                        {
                            float   r = random(0.0f, 0.25f);
                            rgShips[i]->SetDeviation(rgShips[i]->GetDeviation() +
                                                     random(0.0f, 0.25f) +
                                                     ((i < effectiveLight)
                                                      ? 0.70f
                                                      : 1.00f));
                        }
                    }
                }

                if (cTurretUpdates < cActiveTurretUpdates)
                {
                    //We can't send all of the active turret updates so sort the list to send the ones that need
                    //it the most
                    qsort(rgActiveTurrets, cActiveTurretUpdates, sizeof(rgActiveTurrets[0]), CmpShipDeviation);
                }
                {
                    //Adjust the deviation of the ships based on where they fall in the array
                    for (int i = 0; (i < cActiveTurretUpdates); i++)
                    {
                        if (i < cTurretUpdates)
                            rgActiveTurrets[i]->SetDeviation(0.0f);
                        else
                            rgActiveTurrets[i]->SetDeviation(rgActiveTurrets[i]->GetDeviation() + random(0.5f, 0.75f));
                    }
                }
                {
                    //Adjust the deviation of the ships based on where they fall in the array
                    for (int i = 0; (i < cInactiveTurretUpdates); i++)
                    {
                        rgInactiveTurrets[i]->SetDeviation(0.0f);
                    }
                }

                //Assemble the parts of the message that are not going to change
                // Use one static update message so not always calculating size and allocating/deallocating
                // This is kinda tricky because we manually munge the message, rather than rely on the macros
                #define CB_HSU_VARDATA(elems) ((elems) * sizeof(ServerHeavyShipUpdate))
                #define CB_LSU_VARDATA(elems) ((elems) * sizeof(ServerLightShipUpdate))
                #define CB_ATU_VARDATA(elems) ((elems) * sizeof(ServerActiveTurretUpdate))
                #define CB_ITU_VARDATA(elems) ((elems) * sizeof(ShipID))

                static char bfrHeavy[sizeof(FMD_S_HEAVY_SHIPS_UPDATE) +
                                     CB_HSU_VARDATA(c_cShipsPerSectorMax) +
                                     CB_ATU_VARDATA(c_cShipsPerSectorMax) +
                                     sizeof(FMD_S_LIGHT_SHIPS_UPDATE) +             //Add room for the lts ship updates to be appended to the end
                                     CB_LSU_VARDATA(c_cShipsPerSectorMax) +
                                     CB_ITU_VARDATA(c_cShipsPerSectorMax)];

                static FMD_S_HEAVY_SHIPS_UPDATE* pfmHeavy = (FMD_S_HEAVY_SHIPS_UPDATE*)bfrHeavy;
                pfmHeavy->fmid = FM_S_HEAVY_SHIPS_UPDATE;

                //Use now as the reference time
                pfmHeavy->timeReference = g.timeNow;

                //active turret updates always start immediately after the end of the message & we will have cTurretUpdates worth
                pfmHeavy->ibrgActiveTurretUpdates = sizeof(FMD_S_HEAVY_SHIPS_UPDATE);

                static char bfrLight[sizeof(FMD_S_LIGHT_SHIPS_UPDATE) +
                                     CB_LSU_VARDATA(c_cShipsPerSectorMax) +
                                     CB_ITU_VARDATA(c_cShipsPerSectorMax)];

                static FMD_S_LIGHT_SHIPS_UPDATE* pfmLight = (FMD_S_LIGHT_SHIPS_UPDATE*)bfrLight;
                pfmLight->fmid = FM_S_LIGHT_SHIPS_UPDATE;

                //inactive turret updates always start immediately after the end of the message & we will have cInactiveTurretUpdates worth
                ShipID*   pInactive = (ShipID*)(pfmLight + 1);
                pfmLight->ibrgInactiveTurretUpdates = sizeof(FMD_S_LIGHT_SHIPS_UPDATE);
                pfmLight->cbrgInactiveTurretUpdates = CB_ITU_VARDATA(cInactiveTurretUpdates);

                {
                    //The same inactive messages are sent to all players ... so fill this part of the message in now
                    for (int i = 0; (i < cInactiveTurretUpdates); i++)
                    {
                        assert (rgInactiveTurrets[i]->IsPlayer());

                        *(pInactive++) = rgInactiveTurrets[i]->GetShipID();
                    }
                }

                //light ship updates come after that
                pfmLight->ibrgLightShipUpdates = pfmLight->ibrgInactiveTurretUpdates + pfmLight->cbrgInactiveTurretUpdates;

                //Number of light updates depends on the recipients ... so do not fill in the following fields immediately
					/* pkk May 6th: Disabled bandwidth patch
					   Old code below */
					for (PlayerLink* pplayerLink = pplayerList->first(); (pplayerLink != NULL); pplayerLink = pplayerLink->next())
					{
						CFSPlayer*  pfsPlayer = pplayerLink->data();
					/* pkk May 6th: Disabled bandwidth patch
					   Old code ends */
					

					// w0dk4 old for-loop starts here
					// actually, we could rewrite the whole code because we no longer should pre-assemble the first
					// packet part for every player as bandwidth settings may change ...
					// but lets not mess with the code, it *should* be not that of a performance problem

                    IshipIGC*   pshipPlayer = pfsPlayer->GetIGCShip();

                    {
                        //Fill in the stuff like the hull,
                        IshipIGC*   pshipSource = pshipPlayer->GetSourceShip();
                        pshipSource->ExportFractions(&(pfmHeavy->fractions));
                    }

                    ServerActiveTurretUpdate*   pActive = (ServerActiveTurretUpdate*)(pfmHeavy + 1);
                    {
                        ServerActiveTurretUpdate*   pActiveMax = pActive + cTurretUpdates;

                        for (int i = 0; ((pActive < pActiveMax) && (i < cActiveTurretUpdates)); i++)
                        {
                            assert (i < cActiveTurretUpdates);
                            assert (rgActiveTurrets[i]->IsPlayer());
                            CFSShip*  pfsShip = rgActiveTurrets[i];
                            if (pfsShip != pfsPlayer)
                            {
                                //The only way a player could get onto this list is if it had a recent turret update
                                //so forward it intact
                                const ClientActiveTurretUpdate& atu = pfsShip->GetPlayer()->GetActiveTurretUpdate();

                                pActive->time.Set(g.timeNow, atu.time);
                                pActive->controls = atu.controls;
                                pActive->orientation = atu.orientation;

                                (pActive++)->shipID = pfsShip->GetShipID();
                            }
                        }
                    }
                    pfmHeavy->cbrgActiveTurretUpdates = short((DWORD)pActive - (DWORD)(pfmHeavy + 1));
                    pfmHeavy->ibrgHeavyShipUpdates = pfmHeavy->ibrgActiveTurretUpdates + pfmHeavy->cbrgActiveTurretUpdates;

                    //Find the target of the player so that, if it is a ship, we can send it as a heavy if possible
                    IshipIGC*   pshipTarget = NULL;
                    CFSShip*    pfsTarget;
                    {
                        pfmHeavy->bpTargetHull.SetChar(255);

                        ImodelIGC*  pmodelTarget = pshipPlayer->GetCommandTarget(c_cmdCurrent);
                        if (pmodelTarget && (pmodelTarget->GetCluster() == pcluster))
                        {
                            switch (pmodelTarget->GetObjectType())
                            {
                                case OT_ship:
                                {
                                    IshipIGC*   pship = ((IshipIGC*)pmodelTarget)->GetSourceShip();
                                    if (pship != pshipPlayer)
                                    {
                                        assert (pship->GetParentShip() == NULL);

                                        pfmHeavy->bpTargetHull = pship->GetFraction();
                                        {
                                            IshieldIGC* pshield = (IshieldIGC*)(pship->GetMountedPart(ET_Shield, 0));
                                            if (pshield)
                                                pfmHeavy->bpTargetShield = pshield->GetFraction();
                                        }

                                        pfsTarget = (CFSShip*)(pship->GetPrivateData());

                                        if (pfsTarget->GetHasUpdate())
                                            pshipTarget = pship;
                                    }
                                }
                                break;

                                case OT_station:
                                {
                                    IstationIGC*   pstation = (IstationIGC*)pmodelTarget;

                                    pfmHeavy->bpTargetHull = pstation->GetFraction();
                                    pfmHeavy->bpTargetShield = pstation->GetShieldFraction();
                                }
                                break;

                                case OT_probe:
                                case OT_asteroid:
                                case OT_missile:
                                {
                                    pfmHeavy->bpTargetHull = ((IdamageIGC*)pmodelTarget)->GetFraction();
                                }
                            }
                        }
                    }

                    //Reference position depends on the sender
                    {
                        IshipIGC*   pshipSource = pshipPlayer->GetSourceShip();
                        pfmHeavy->positionReference = (pshipSource->GetCluster() == pcluster)
                                                      ? pshipSource->GetPosition()
                                                      : Vector::GetZero();
                    }

                    ServerHeavyShipUpdate*      pHeavy = (ServerHeavyShipUpdate*)pActive;
                    ServerLightShipUpdate*      pLight = (ServerLightShipUpdate*)pInactive;
                    {
                        ServerHeavyShipUpdate*      pHeavyMax = pHeavy + cHeavyUpdates;
                        ServerLightShipUpdate*      pLightMax = pLight + cLightUpdates;

                        if (pshipTarget)
                        {
                            assert (cHeavyUpdates > 0);
                            AddUpdate(g.timeNow, pfmHeavy->positionReference,
                                      pfsTarget, pHeavy++);
                        }

                        for (int index = 0; (index < cShipUpdates); index++)
                        {
                            CFSShip*    pfsShip = rgShips[index];
                            if (pfsShip != pfsPlayer)
                            {
                                IshipIGC*   pship = pfsShip->GetIGCShip();
                                if (pship != pshipTarget)
                                {
                                    if (pHeavy < pHeavyMax)
                                    {
                                        //debugf("%dH ", pship->GetObjectID());
                                        AddUpdate(g.timeNow, pfmHeavy->positionReference, pfsShip, pHeavy++);
                                    }
                                    else if (pLight < pLightMax)
                                    {
                                        //debugf("%dL ", pship->GetObjectID());
                                        pship->ExportShipUpdate(pLight++);
                                    }
                                    else
                                        break;
                                }
                            }
                        }
                    }

                    pfmLight->cbrgLightShipUpdates = short((DWORD)pLight - (DWORD)pInactive);
                    pfmHeavy->cbrgHeavyShipUpdates = short((DWORD)pHeavy - (DWORD)pActive);

                    pfmLight->cbmsg = sizeof(FMD_S_LIGHT_SHIPS_UPDATE) + pfmLight->cbrgInactiveTurretUpdates + pfmLight->cbrgLightShipUpdates;
                    pfmHeavy->cbmsg = sizeof(FMD_S_HEAVY_SHIPS_UPDATE) + pfmHeavy->cbrgActiveTurretUpdates + pfmHeavy->cbrgHeavyShipUpdates;

                    //Send the heavy ship update whenever the player is flying since it has hitpoint information
                    if ((pfmHeavy->cbmsg != sizeof(FMD_S_HEAVY_SHIPS_UPDATE)) || (pshipPlayer->GetCluster() != NULL))
                    {
                        CB  cbMsg = pfmHeavy->cbmsg;
                        if (pfmLight->cbmsg != sizeof(FMD_S_LIGHT_SHIPS_UPDATE))
                        {
                            //Copy the light ship update to the end of the heavy ship update
                            memcpy(((char*)pfmHeavy) + cbMsg, pfmLight, pfmLight->cbmsg);
                            cbMsg += pfmLight->cbmsg;
                        }
                        g.fm.GenericSend(pfsPlayer->GetConnection(), pfmHeavy, cbMsg, FM_NOT_GUARANTEED);
                    }
                    else if (pfmLight->cbmsg != sizeof(FMD_S_LIGHT_SHIPS_UPDATE))
                    {
                        //We should be able to ignore this case ... we should never have a case where we are sending light updates
                        //without sending at least one heavy update. But ... leave it in in case this assumption changes.
                        g.fm.ForwardMessage(pfsPlayer->GetConnection(), pfmLight, FM_NOT_GUARANTEED);
                    }


					/*debugf("hvy/int: %d hvy updates: %d ship update: %d %d %d %d %d\n",
						cHeavyPerInterval,
						cHeavyUpdates,
                        Time::Now().clock(),
                        pfmLight->cbmsg - sizeof(FMD_S_LIGHT_SHIPS_UPDATE),
                        pfmHeavy->cbmsg - sizeof(FMD_S_HEAVY_SHIPS_UPDATE),
                        cShips, cShipUpdates);*/

                }

                for (int i = 0; (i < cShipUpdates); i++)
                {
                    assert (rgShips[i]->GetHasUpdate());
                    rgShips[i]->ClearHasUpdate();
                }
            }
        }
    }

    //
    // UPDATE LATENCY: Report the counters to perfmon.
    //
    if (NULL != g.pServerCounters)
    {
        g.pServerCounters->csecAvgUpdateLatency = CFSPlayer::GetAverageLatency();
        g.pServerCounters->csecMaxUpdateLatency = CFSPlayer::GetMaxLatency();
    }

    #undef SUVARDATASIZE
}

/*-------------------------------------------------------------------------
 * MoveShips
 *-------------------------------------------------------------------------
 * Purpose:
 *    Keeps ships in constant motion
 *    Detects scan range entrances and exits, and sends appropriate notifications
 *    Sends all ship updates
 */
DWORD MoveShips(CFSMission * pfsMission, Time timeNow, Time timeLastUpdate)
{
  static CTempTimer timerMoveShips("in MoveShips", .1f);
  timerMoveShips.Start();

  if (pfsMission->GetStage() == STAGE_STARTED)
  {
      static CTempTimer timerSendShipUpdates("in SendShipUpdates", .1f);
      timerSendShipUpdates.Start();
      SendShipUpdates(pfsMission);
      timerSendShipUpdates.Stop();
  }

  static CTempTimer timerMissionUpdate("in ImissionIGC->Update", .15f);
  timerMissionUpdate.Start();
  pfsMission->GetIGCMission()->Update(timeNow);
  timerMissionUpdate.Stop();

  timerMoveShips.Stop();
  return 0;
}

/*

TODO: remove

struct ArtUpdateCallbackInfo
{
    FedMessaging  * pfm;
    CFMConnection * pcnxnFrom;
};

// Check for new art--gets called for every artwork file changed since client last downloaded
// Our job here is simply to send one message to the client referred to in pData.
bool HandleChangedArtwork(FileChangeInfo* pfci, void* pData)
{
    static DPID dpidLastPlayer = 0;
    struct ArtUpdateCallbackInfo *pMyData = (struct ArtUpdateCallbackInfo *)pData;

    // if a different user then last time this was called,
    // then it's time to send FTP Site info
    if (dpidLastPlayer != pMyData->pcnxnFrom->GetID())
    {
        dpidLastPlayer = pMyData->pcnxnFrom->GetID();

        // All messages that follow are going to this player, and are guaranteed
        g.fm.SetDefaultRecipient(pMyData->pcnxnFrom, FM_GUARANTEED);

        FILETIME ftLastWriteTime = {0, 0}; // tell the client what the most recently downloaded thing is

        FileChangeInfo* pFileInfo = g.pArtDirMon->GetNewestFile();
            if (pFileInfo)
                ftLastWriteTime = pFileInfo->ftLastWriteTime;

        BEGIN_PFM_CREATE(g.fm, pfmServerInfo, S, ART_SERVER_INFO)
            FM_VAR_PARM(g.szFTPServer, CB_ZTS)
            FM_VAR_PARM(g.szFTPInitialDir, CB_ZTS)
            FM_VAR_PARM(g.szFTPAccount, CB_ZTS)
            FM_VAR_PARM(g.szFTPPassword, CB_ZTS)
        END_PFM_CREATE

        pfmServerInfo->ftLastArtUpdate = ftLastWriteTime;
    }

    // TODO: use default recipient
    BEGIN_PFM_CREATE(*pMyData->pfm, pfmArtUpdate, S, ART_UPDATE)
        FM_VAR_PARM(pfci->szFileName, CB_ZTS)
    END_PFM_CREATE
    pfmArtUpdate->cFileSize = pfci->cFileSize;

    return true; // return success
}
*/

/*-------------------------------------------------------------------------
 * CreateTreasure(IpartIGC* p, float dv)
 *-------------------------------------------------------------------------
 * Purpise:
 *    A part was either dropped or ejected from a dead ship.
 *    Create the treasure on the server and export its existence
 *    to all clients in the mission (NYI: we really should just send
 *    it to the players in the sector then inform other players when
 *    they enter the sector).
 *
 *    The part has a random velocity of v added to the velocity of its ship.
 */
ItreasureIGC*  CreateTreasure(Time now, IshipIGC* pship, DataTreasureIGC* pdt, const Vector& position, float dv)
{
    assert (pship);
    assert (dv > 1.0f);

    pdt->objectID = pship->GetMission()->GenerateNewTreasureID();

    pdt->p0 = position;

    Vector direction = Vector::RandomDirection();

    float   radius = pship->GetRadius() + 10.0f;
    pdt->p0 += radius * direction;

    pdt->v0 = direction * dv + pship->GetVelocity();

    IclusterIGC*    pcluster = pship->GetCluster();
    pdt->clusterID = pcluster->GetObjectID();

    pdt->time0 = now;

    ItreasureIGC* t = (ItreasureIGC *)
                      pship->GetMission()->CreateObject(now, OT_treasure,
                                                        pdt, sizeof(*pdt));

    //Note: bad form releasing a pointer before we return it but we know it will
    //stick around since it is in a cluster.
    assert (t);
    t->Release();

    return t;
}

 ItreasureIGC*  CreateTreasure(Time now, IshipIGC* pship, short amount, IpartTypeIGC* ppt, const Vector& position, float dv, float lifespan = 600.0f)
 {
    assert (pship);
    assert (ppt);
    assert (dv > 1.0f);

    DataTreasureIGC dt;
    dt.treasureCode = c_tcPart;
    dt.treasureID = ppt->GetObjectID();
    dt.lifespan = lifespan;
    dt.amount = amount;
    dt.createNow = false;

    return CreateTreasure(now, pship, &dt, position, dv);
}


ItreasureIGC*  CreateTreasure(Time now, IshipIGC* pship, IpartIGC* p, IpartTypeIGC* ppt, const Vector& position, float dv, float lifespan = 600.0f)
{
    assert (p);

    short amount;
    switch (p->GetObjectType())
    {
        case OT_magazine:
        case OT_dispenser:
        case OT_pack:
        {
            amount = p->GetAmount();
            if (amount == 0)
                return NULL;
        }
        break;

        default:
            amount = 0;
    }

    return CreateTreasure(now, pship, amount, ppt, position, dv, lifespan);
}



/*-------------------------------------------------------------------------
 * ExportStaticIGCObjs
 *-------------------------------------------------------------------------
  Purpose:
      Send the client all the static IGC objects. A default recipient must be already set.

  Not being used. Keep it around for a while just in case we want to resurrect it.
 */
void ExportStaticIGCObjs()
{
  //g.fm.SetPriority(c_mcpLogonPrelim);

  // Export station types
  {
    const StationTypeListIGC * pstlist = g.trekCore->GetStationTypes();
    StationTypeLinkIGC * pstlink = NULL;
    for (pstlink = pstlist->first(); pstlink; pstlink = pstlink->next())
      ExportObj(pstlink->data(), OT_stationType, NULL);
  }

  // Export projectiles
  {
    const ProjectileTypeListIGC * pptlist = g.trekCore->GetProjectileTypes();
    ProjectileTypeLinkIGC * pptlink = NULL;
    for (pptlink = pptlist->first(); pptlink; pptlink = pptlink->next())
      ExportObj(pptlink->data(), OT_projectileType, NULL);
  }

  // Export missile types
  {
    const ExpendableTypeListIGC* petlist = g.trekCore->GetExpendableTypes();
    ExpendableTypeLinkIGC* petlink;
    for (petlink = petlist->first(); petlink; petlink = petlink->next())
      ExportObj(petlink->data(), petlink->data()->GetObjectType(), NULL);
  }

  // Export parts
  {
    const PartTypeListIGC * pptlist = g.trekCore->GetPartTypes();
    PartTypeLinkIGC * pptlink = NULL;
    for (pptlink = pptlist->first(); pptlink; pptlink = pptlink->next())
      ExportObj(pptlink->data(), OT_partType, NULL);
  }

  // Export hull types
  {
    const HullTypeListIGC * phtlist = g.trekCore->GetHullTypes();
    HullTypeLinkIGC * phtlink = NULL;
    for (phtlink = phtlist->first(); phtlink; phtlink = phtlink->next())
      ExportObj(phtlink->data(), OT_hullType, NULL);
  }

  // Export developments
  {
    const DevelopmentListIGC * pdtlist = g.trekCore->GetDevelopments();
    DevelopmentLinkIGC * pdtlink = NULL;
    for (pdtlink = pdtlist->first(); pdtlink; pdtlink = pdtlink->next())
      ExportObj(pdtlink->data(), OT_development, NULL);
  }

  // Export drones types
  {
    const DroneTypeListIGC * pdtlist = g.trekCore->GetDroneTypes();
    DroneTypeLinkIGC * pdtlink = NULL;
    for (pdtlink = pdtlist->first(); pdtlink; pdtlink = pdtlink->next())
      ExportObj(pdtlink->data(), OT_droneType, NULL);
  }

  // Export civilizations
  {
    const CivilizationListIGC * pctlist = g.trekCore->GetCivilizations();
    CivilizationLinkIGC * pctlink = NULL;
    for (pctlink = pctlist->first(); pctlink; pctlink = pctlink->next())
      ExportObj(pctlink->data(), OT_civilization, NULL);
  }
}


int FedSrvSiteBase::OnMessageBox(FedMessaging * pthis, const char * strText, const char * strCaption, UINT nType)
{
  return OnMessageBox(strText, strCaption, nType);
}


int FedSrvSiteBase::OnMessageBox(const char * strText, const char * strCaption, UINT nType)
{
  printf("%s:\n%s\n\n", strCaption, strText);
  _AGCModule.TriggerEvent(NULL, AllsrvEventID_MessageBox, "", -1, -1, -1, 2,
      "Caption", VT_LPSTR, strCaption,
      "Message", VT_LPSTR, strText);
  return 0; // which is intentionally not actually a valid MessageBox return code, since we're not actually throwing up a message box
}


HRESULT FedSrvSiteBase::OnSysMessage(FedMessaging * pthis)
{
  if (g.pServerCounters)
  {
    g.pServerCounters->cSysMessagesIn++;
    g.pServerCounters->cSysMessagesInPerSecond++;
  }
  return S_OK;
}

// KG: eventually remove this - or notk, Imago 6/10
//#ifndef ALLSRV_STANDALONE
void FedSrvSiteBase::OnSQLErrorRecord(SSERRORINFO * perror, OLECHAR * postrError)
{
  // don't make the event an error event, because this may or may not be fatal.
  // But we certainly want to see them all in any case.
  _AGCModule.TriggerEvent(NULL, AllsrvEventID_DatabaseErrorSQL, "", -1, -1, -1, 5,
    "Message"      , VT_LPWSTR, perror->pwszMessage,
    "Procedure"    , VT_LPWSTR, perror->pwszProcedure,
    "Native"       , VT_I4,     perror->lNative,
    "Line"         , VT_I2,     perror->wLineNumber,
    "OleDB"        , VT_LPWSTR, postrError);
}


void FedSrvSiteBase::OnOLEDBErrorRecord(BSTR bstrDescription, GUID guid, DWORD dwHelpContext,
                                  BSTR bstrHelpFile,    BSTR bstrSource)
{
  // Convert the GUID to a string for the event parameter
  OLECHAR szGUID[48];
  StringFromGUID2(guid, szGUID, sizeofArray(szGUID));

  // These are always fatal, because we have no idea what we can do about it.
  _AGCModule.TriggerEvent(NULL, AllsrvEventID_DatabaseErrorOLEDB, "", -1, -1, -1, 5,
    "Desc",     VT_BSTR,  bstrDescription,
    "Guid",     VT_LPWSTR,szGUID,
    "HelpID",   VT_I4,    dwHelpContext,
    "HelpFile", VT_BSTR,  bstrHelpFile,
    "Source",   VT_BSTR,  bstrSource);
}
//#endif //ALLSRV_STANDALONE

HRESULT FedSrvSiteBase::OnNewConnection(FedMessaging * pthis, CFMConnection & cnxn)
{
  // we don't do anything until they login

  // mmf 10/08 Log ip address (code provided by Imago)
  char szRemote[64];
  pthis->GetIPAddress(cnxn, szRemote);
  debugf("New connection ID: %x IP: %s\n",cnxn.GetID(),szRemote);

  return S_OK;
}


HRESULT FedSrvSiteBase::OnDestroyConnection(FedMessaging * pthis, CFMConnection & cnxn)
{
  CFSPlayer * pfsPlayer = CFSPlayer::GetPlayerFromConnection(cnxn);
  if (pfsPlayer) // need to clean up after them if they didn't log off
  {
    PlaySound("AppGPFault", NULL, SND_ALIAS | SND_PURGE | SND_ASYNC);
    _AGCModule.TriggerEvent(NULL, AllsrvEventID_PlayerDropped, "", -1, -1, -1, 2,
        "PlayerName", VT_LPSTR, pfsPlayer->GetName(),
        "ConxID", VT_I4, cnxn.GetID()); // pkk - IP address
    if (NULL != g.pServerCounters)
    {
        g.pServerCounters->cTimeouts++;
        g.pServerCounters->cTimeoutsPerSecond++;
    }
    delete pfsPlayer;
  }
  return S_OK;
}


HRESULT FedSrvSiteBase::OnSessionLost(FedMessaging * pthis)
{
  _AGCModule.TriggerEvent(NULL, AllsrvEventID_SessionLost, "", -1, -1, -1, 1,
      "Time", VT_I4, Time::Now().clock());
  // todo: gracefully shut down
  return S_OK;
}


#ifndef NO_MSG_CRC
/*-------------------------------------------------------------------------
 * OnBadCRC
 *-------------------------------------------------------------------------
  Purpose:
      If someone sends a message with a bad crc, we have no choice but to nuke 'em

  Parameters:
      The message that failed crc

  Side Effects:
      Bye-bye to the offender
 */
void FedSrvSiteBase::OnBadCRC(FedMessaging * pthis, CFMConnection & cnxn, BYTE * pMsg, DWORD cbMsg)
{
  char buf[256];
  FEDMESSAGE * pfm = (FEDMESSAGE *) pMsg;
  wsprintf(buf, "HEY! We got a corrupt message!\nPlayer=%s(%d), "
         "cbmsg=%d, fmid=%d, total packet size=%d.\n"
         "Copy the above line to crashplayers.txt on \\\\zoneagga01. Going to drop player now.\n",
         cnxn.GetName(), cnxn.GetID(),
         cbMsg >= 2 ? pfm->cbmsg : 0, cbMsg >= 4 ? pfm->fmid : 0, cbMsg);

  OnMessageBox(buf, "Allegiance", 0);
  pthis->DeleteConnection(cnxn); // bye bye now
}
#endif


/*-------------------------------------------------------------------------
 * FedSrvSiteBase.OnMessageSent
 *-------------------------------------------------------------------------
  Purpose:
    callback to help us keep perfmon counters on data sent
 */
void FedSrvSiteBase::OnMessageSent(FedMessaging * pthis, CFMRecipient * precip, const void * pv, DWORD cb, FMGuaranteed fmg)
{
  int cConnections = precip->GetCountConnections();
  assert (cConnections >= 0);
  DWORD dwBytes = cb * cConnections;
  FEDMESSAGE * pfm = (FEDMESSAGE*)pv;
  if (FM_GUARANTEED == fmg)
  {
    // debugf("Guaranteed: %s (%u)\n", g_rgszMsgNames[pfm->fmid], cb);
    if (FM_CS_ORDER_CHANGE == pfm->fmid)
    {
      CASTPFM(pfmOrderChange, CS, ORDER_CHANGE, pfm);
#ifdef DEBUG
      // mmf only log this in debug build
      debugf("FM_CS_ORDER_CHANGE: shipID=%d, objectID=%d, objectType=%d, commandID=%d, command=%d.\n",
          pfmOrderChange->shipID, pfmOrderChange->objectID, pfmOrderChange->objectType,
          pfmOrderChange->commandID, pfmOrderChange->command);
#endif
	}

    g.pServerCounters->cPacketsOut          += cConnections;
    g.pServerCounters->cPacketsOutPerSecond += cConnections;
    g.pServerCounters->cBytesOut            += dwBytes;
    g.pServerCounters->cBytesOutPerSecond   += dwBytes;
/*
    g.pServerCounters->cPacketsOutGPerSecond += cConnections;
    g.pServerCounters->cBytesOutGPerSecond   += dwBytes;
*/
  }
  else if (FM_NOT_GUARANTEED == fmg)
  {
    // debugf("Unguaranteed: %s (%u)\n", g_rgszMsgNames[pfm->fmid], cb);

/*
    g.pServerCounters->cPacketsOutUPerSecond += cConnections;
    g.pServerCounters->cBytesOutUPerSecond   += dwBytes;
*/
  }
}


/*
void FedSrvSiteBase::OnMessageNAK(FedMessaging * pthis, DWORD dwTime, CFMRecipient * prcp)
{
  pthis->DeleteConnection(*prcp);
}
*/


#define TrapHackLog(x)          {if (!(x)) { LogTrapHack(EventID_HackLog    , pfsPlayer, &cnxnFrom, #x, __LINE__); break; }}
#define TrapHackBoot(x)         {if (!(x)) { LogTrapHack(EventID_HackBoot   , pfsPlayer, &cnxnFrom, #x, __LINE__); break; }}
// REVIEW: can we get any info from the connection, like IP address?
#define TrapHackBootNoPlayer(x) {if (!(x)) { LogTrapHack(EventID_HackBootNoPlayer, NULL, &cnxnFrom, #x, __LINE__); break; }}


static bool DisplaceProbe(ImodelIGC* pmodel, Vector* position, float r)
{

      Vector dxy = *position - pmodel->GetPosition();
      float l2 = dxy.LengthSquared();
      float rOffset = pmodel->GetRadius() + r;
      if ((l2 < rOffset * rOffset) && (l2 != 0.0f))
      {
          *position = pmodel->GetPosition() + dxy * (rOffset / (float)sqrt(l2));

          return true;
      }

      return false;
}

static void DisplaceProbe(IclusterIGC* pcluster, Vector* position, float r)
{
    {
      for (StationLinkIGC*   psl = pcluster->GetStations()->first(); (psl != NULL); psl = psl->next())
      {
          if (DisplaceProbe(psl->data(), position, r))
              return;
      }
    }
    {
      for (WarpLinkIGC*   psl = pcluster->GetWarps()->first(); (psl != NULL); psl = psl->next())
      {
          if (DisplaceProbe(psl->data(), position, r))
              return;
      }
    }
    	
	/* Xynth - Removing loop through Asteroids from the DisplaceProbe function
	** This was causing issues with trying to hide TP2 behind rocks 7/10
	{
      for (AsteroidLinkIGC*   psl = pcluster->GetAsteroids()->first(); (psl != NULL); psl = psl->next())
      {
          if (DisplaceProbe(psl->data(), position, r))
              return;
      }
    }*/
}



// BT - STEAM
const char m_lookUpTable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };

// BT - STEAM
// https://stackoverflow.com/questions/1235371/fastest-base-conversion-method
void GetPlayerHandleFromSteamID(CSteamID steamID, char *playerHandle)
{
	char* pWritePos = playerHandle;
	uint64 leftOver = steamID.ConvertToUint64();
	int counter = 0;
	while (leftOver)
	{
		int value62 = leftOver % 62;
		*pWritePos = m_lookUpTable[value62];
		pWritePos++;

		leftOver = floor(leftOver / 62);
		counter++;
	}
	*pWritePos = 0;

	// Reverse the string.
	for (int i = 0; i < counter / 2; i++)
	{
		char temp = playerHandle[i];
		playerHandle[i] = playerHandle[counter - i - 1];
		playerHandle[counter - i - 1] = temp;
	}
}

// BT - STEAM
CSteamID GetSteamIDFromPlayerHandle(char *playerHandle)
{
	int iterator = strlen(playerHandle);
	uint64 returnValue = 0;
	int multiplier = 62;

	for (int index = 0; index < strlen(playerHandle); index++)
	{
		for (int i = 0; i < sizeof(m_lookUpTable); i++)
		{
			if (m_lookUpTable[i] == playerHandle[index])
			{
				returnValue = (returnValue * multiplier) + i;
				break;
			}
		}

		//multiplier *= 62;
	}

	return CSteamID(returnValue);
}


// BT - STEAM 
static DWORD WINAPI LogonThread(LPVOID threadData)
{
	CSteamValidation *steamValidation = (CSteamValidation *) threadData;
	steamValidation->BeginSteamAuthentication();

	// BeginSteamAuthentication will block until either 30 seconds goes by, or it gets a login response from steam.
	// When it completes, it calls PostThreadMessage to complete the login.

	delete steamValidation;

	return 0;
}



/*-------------------------------------------------------------------------
 * OnAppMessage
 *-------------------------------------------------------------------------
 * Purpose:
 *    Process all incoming fedsrv messages. This is like the winproc
 *    of messaging. All handling of messages from clients is done here
 *
 * Side Effects:
 *    Lots
 */
HRESULT FedSrvSiteBase::OnAppMessage(FedMessaging * pthis, CFMConnection & cnxnFrom, FEDMESSAGE * pfm)
{
  CFSPlayer * pfsPlayer = NULL;
  CFSMission * pfsMission = NULL;
  ImissionIGC * pMission = NULL;
  HRESULT hr = S_OK;

  static CTempTimer timerOnAppMessage("in OnAppMessage", .02f);
  static CTempTimer ttPreHandler("in OnAppMessage (before we actually switch)", .01f);
  ttPreHandler.Start();
  timerOnAppMessage.Start();

  if (NULL != g.pServerCounters)
  {
    g.pServerCounters->cPacketsIn++;
    g.pServerCounters->cPacketsInPerSecond++;
    g.pServerCounters->cBytesIn += pfm->cbmsg;
    g.pServerCounters->cBytesInPerSecond += pfm->cbmsg;

    static Time timeLastQueueCheck = 0;
    if (g.timeNow - timeLastQueueCheck > 1.0f)
    {
      g.fm.GetSendQueue(&(g.pServerCounters->cOutboundQueueLength),
                        &(g.pServerCounters->cOutboundQueueSize));
      g.fm.GetReceiveQueue(&(g.pServerCounters->cInboundQueueLength),
                           &(g.pServerCounters->cInboundQueueSize));
      timeLastQueueCheck = g.timeNow;
    }
  }

  pfsPlayer = CFSPlayer::GetPlayerFromConnection(cnxnFrom);

  if (pfsPlayer)
  {
    assert (pfm->fmid != FM_C_LOGONREQ); // shouldn't have a pfsPlayer yet if logging on
    cnxnFrom.ResetAbsentCount();
    pfsMission = pfsPlayer->GetMission();
    if (pfsMission)
      pMission   = pfsMission->GetIGCMission();
  }

  ttPreHandler.Stop("...for message type %s from %s", g_rgszMsgNames[pfm->fmid],
                    pfsPlayer ? pfsPlayer->GetName() : "<unknown>");

  if (!pfsPlayer && (pfm->fmid != FM_C_LOGONREQ))
  {
    _AGCModule.TriggerEvent(NULL, AllsrvEventID_MsgFromUnknownPlayer, "", -1, -1, -1, 2,
        "DPNID", VT_I4, cnxnFrom.GetID(), // pkk - Returns DPNID (DirectPlay network identifier)
        "MsgName", VT_LPSTR, g_rgszMsgNames[pfm->fmid]); // pkk - This should return FedSrvMsgID text, but somehow it doesn't
  }
  else
  switch(pfm->fmid)
  {
    case FM_C_SHIP_UPDATE:
    {
        CASTPFM(pfmShipUpdate, C, SHIP_UPDATE, pfm);
        // if the cookies don't match, either they're cheating, or they haven't processed
        // a message that needs processing before their updates become valid again
        if (pfsPlayer->GetCookie() == pfmShipUpdate->cookie)
        {
            IshipIGC * pship = pfsPlayer->GetIGCShip();
            if (pship->GetCluster() && (pship->GetParentShip() == NULL))
            {
                assert (pship->GetBaseHullType());
                pfsPlayer->SetShipUpdate(pfmShipUpdate->shipupdate);
            }
        }
    }
    break;

    case FM_C_ACTIVE_TURRET_UPDATE:
    {
        CASTPFM(pfmATU, C, ACTIVE_TURRET_UPDATE, pfm);

        IshipIGC * pship = pfsPlayer->GetIGCShip();
        if (pship->GetCluster() &&
            (pship->GetParentShip() != NULL) &&
            (pship->GetTurretID() != NA))
        {
            pfsPlayer->SetActiveTurretUpdate(pfmATU->atu);
        }
    }
    break;

    case FM_C_INACTIVE_TURRET_UPDATE:
    {
        CASTPFM(pfmITU, C, INACTIVE_TURRET_UPDATE, pfm);

        IshipIGC * pship = pfsPlayer->GetIGCShip();
        if (pship->GetCluster() &&
            (pship->GetParentShip() != NULL) &&
            (pship->GetTurretID() != NA))
        {
            //Stop shooting
            pfsPlayer->SetInactiveTurretUpdate();
        }
    }
    break;

    case FM_CS_PING:
    {
      CASTPFM(pfmPing, CS, PING, pfm);
      pfmPing->timeServer = g.timeNow;
      //debugf("Ping from %s in at %u, ", pfsPlayer->GetName(), Time::Now ());
      //g.fm.SetPriority(FedMessaging::c_mcpDefault + 1); // TE: This was in R2 but not in DPlay8
      g.fm.ForwardMessage(pfsPlayer->GetConnection(), pfmPing, pfmPing->fmg);
      //g.fm.SetPriority(FedMessaging::c_mcpDefault); // TE: This was in R2 but not DPlay8
      //debugf("out at %u\n", Time::Now ());

      // Would be nice if this worked, but it always reports 500ms.
      //DWORD dwLatency = g.fm.GetLatency(&cnxnFrom);
      //debugf("Latency to %s=%d\n", cnxnFrom.GetName(), dwLatency);
      break;
    }

    case FM_CS_CHATMESSAGE:
    {
      bool  bForward = true;
      CASTPFM(pfmChat, CS, CHATMESSAGE, pfm);

      if (!pfsMission)
        break;

      TrapHackBoot(pfmChat->cbMessage >= 0);
      TrapHackBoot(pfmChat->ibMessage == sizeof(FMD_CS_CHATMESSAGE));

	  // BT - STEAM - Send the message up to the lobby for logging.
	  ZString targetName;
	  IshipIGC *targetShip;
	  uint64 sourceSteamID = 0;
	  uint64 targetSteamID = 0;

	  if (pfmChat->otTarget == -1)
	  {
		  switch (pfmChat->cd.chatTarget )
		  {
			  case CHAT_INDIVIDUAL:
				  
				  targetShip = pfsMission->GetIGCMission()->GetShip(pfmChat->cd.oidRecipient);
				  if (targetShip != nullptr && targetShip->GetPilotType() == c_ptPlayer)
				  {
					  targetName = targetShip->GetName();
					  CFSShip*  pfsShip = (CFSShip*)(targetShip->GetPrivateData());

					  if (pfsShip != nullptr)
					  {
						  CFSPlayer * pfsPlayer = pfsShip->GetPlayer();
						  if (pfsPlayer != nullptr)
						  {
							  const char * cdKey = pfsShip->GetPlayer()->GetCDKey();
							  if(cdKey != nullptr)
								  targetSteamID = strtoull(cdKey, NULL, NULL);
						  }
					  }
				  }
				  else
				  {
					  targetName = "Unknown";
				  }

				/*  for (ShipLinkIGC* psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
				  {
					  CFSShip*  pfsShip = (CFSShip*)(psl->data()->GetPrivateData());
					  if(pfsShip->GetObjectID
				  }

				  targetName = pfsMission->GetIGCMission()->GetModel(pfmChat->otTarget, pfmChat->oidTarget)->GetName();*/
				  break;

			  case CHAT_WING:
				  targetName = "Wing: " + pfsPlayer->GetIGCShip()->GetWingID();
				  break;

			  case CHAT_TEAM:
				  targetName = pfsPlayer->GetSide()->GetName();
				  break;

			  case CHAT_ALL_SECTOR:
			  case CHAT_FRIENDLY_SECTOR:
				  targetName = pfsPlayer->GetCluster()->GetName();
				  break;

			  case CHAT_LEADERS:
				  targetName = "Leaders";
				  break;

			  default:
				  targetName = "All";
		  }
	  }

		ZString senderName = pfsPlayer->GetName();
		sourceSteamID = strtoull(pfsPlayer->GetCDKey(), NULL, NULL);
		
		char szRemote[64];
		CFMConnection * connection = pfsPlayer->GetConnection();
		pthis->GetIPAddress(*connection, szRemote);
		debugf("chat from connection ID: %x IP: %s\n", connection->GetID(), szRemote);

		const FMD_S_MISSIONDEF *missionDef = pfsMission->GetMissionDef();
		ZString missionName = ZString(missionDef->misparms.strGameName);

		// BT - 10/17 - Fixing crash on chat logging when lobby is not connected.
		if (g.fmLobby.IsConnected() == true)
		{
			BEGIN_PFM_CREATE(g.fmLobby, pfmLogChat, S, LOG_CHAT_MESSAGE)
				FM_VAR_PARM((PCC)missionName, CB_ZTS)
				FM_VAR_PARM((PCC)senderName, CB_ZTS)
				FM_VAR_PARM((PCC)targetName, CB_ZTS)
				FM_VAR_PARM(FM_VAR_REF(pfmChat, Message), CB_ZTS)
				FM_VAR_PARM(szRemote, CB_ZTS)
				END_PFM_CREATE

			pfmLogChat->sourceSteamID = sourceSteamID;
			pfmLogChat->targetSteamID = targetSteamID;

			g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
		}

		// BT - STEAN - End of chat logging.

      if (pfmChat->cbMessage > 0)
	  {
#ifdef _TEST
        //If you are going to cheat ... make it a public cheat
        if (DoCheatCode(pfsPlayer, FM_VAR_REF(pfmChat, Message)))  //Imago 7/8/09 only link this in debug builds
        {
          if (!lstrcmpi(FM_VAR_REF(pfmChat, Message), "bootme"))
            break;
          bForward = false;
        }
        else
#endif
		{

			//// BT - STEAM - Enable a user that is a member of the MS Allegiance Steam Group to get a list of players and steam IDs 
			//// so that a moderator can kick or ban users from inside the game console.
			//if (pfmChat->cbMessage > strlen("#who") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#who", strlen("#who")) == 0)
			//{
			//	if (CSteamUserGroupStatus::IsMemberOfModeratorsGroup(pfsPlayer->GetCDKey()) == true)
			//	{
			//		for (ShipLinkIGC* psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
			//		{
			//			CFSShip*  pfsShip = (CFSShip*)(psl->data()->GetPrivateData());
			//			if (pfsShip->IsPlayer() == true)
			//			{
			//				char playerData[1024];

			//				uint64 steamUID = strtoull(pfsShip->GetPlayer()->GetCDKey(), NULL, 0);
			//				CSteamID steamID(steamUID);

			//				char playerHandle[256];
			//				GetPlayerHandleFromSteamID(steamID, playerHandle);

			//				sprintf(playerData, "%s - %s", pfsShip->GetPlayer()->GetName(), playerHandle);

			//				CSteamID testID = GetSteamIDFromPlayerHandle(playerHandle);

			//				pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsPlayer->GetIGCShip()->GetObjectID(),
			//					NA, playerData);
			//			}
			//		}
			//	}
			//}

			//// BT - STEAM
			//if (pfmChat->cbMessage > strlen("#kick") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#kick", strlen("#kick")) == 0)
			//{
			//	if (CSteamUserGroupStatus::IsMemberOfModeratorsGroup(pfsPlayer->GetCDKey()) == true)
			//	{
			//		char *kickPlayerHandle = FM_VAR_REF(pfmChat, Message) + strlen("#kick ");
			//		CSteamID targetSteamID = GetSteamIDFromPlayerHandle(kickPlayerHandle);
			//		char kickSteamID[1024];
			//		sprintf(kickSteamID, "%" PRIu64, targetSteamID.ConvertToUint64());


			//		for (ShipLinkIGC* psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
			//		{
			//			CFSShip*  pfsShip = (CFSShip*)(psl->data()->GetPrivateData());
			//			if (pfsShip->IsPlayer() == true && strcmp(pfsShip->GetPlayer()->GetCDKey(), kickSteamID) == 0)
			//			{
			//				pfsMission->RemovePlayerFromSide(pfsShip->GetPlayer(), QuitSideReason::QSR_SwitchingSides, "You have been removed from the team by an admin.");

			//				// TE: Fire AGCEvent informing of the kick
			//				LPCSTR pszContext = pfsMission->GetIGCMission()->GetContextName();
			//				_AGCModule.TriggerContextEvent(NULL, EventID_LeaveTeam, pszContext, pfsShip->GetName(),
			//					pfsShip->GetShipID(), -1, -1, 3, "GameID", VT_I4, pfsMission->GetMissionID(),
			//					"GameName", VT_LPSTR, pfsMission->GetIGCMission()->GetMissionParams()->strGameName,
			//					"zKicker", VT_LPSTR, pfsPlayer->GetName());
			//				// This is named zBooter so the parameter appears last in the alphabetical list
			//				// TE

			//				break;
			//			}
			//		}
			//	}
			//}


			//// BT - STEAM
			//if (pfmChat->cbMessage > strlen("#ban") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#ban", strlen("#ban")) == 0)
			//{
			//	if (CSteamUserGroupStatus::IsMemberOfModeratorsGroup(pfsPlayer->GetCDKey()) == true)
			//	{
			//		char *banPlayerHandle = FM_VAR_REF(pfmChat, Message) + strlen("#ban ");
			//		CSteamID targetSteamID = GetSteamIDFromPlayerHandle(banPlayerHandle);
			//		char banSteamID[1024];
			//		sprintf(banSteamID, "%" PRIu64, targetSteamID.ConvertToUint64());

			//		for (ShipLinkIGC* psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
			//		{
			//			CFSShip*  pfsShip = (CFSShip*)(psl->data()->GetPrivateData());
			//			if (pfsShip->IsPlayer() == true && strcmp(pfsShip->GetPlayer()->GetCDKey(), banSteamID) == 0)
			//			{
			//				pfsMission->RemovePlayerFromMission(pfsShip->GetPlayer(), QuitSideReason::QSR_AdminBooted, "You have been removed from the game by an admin.");

			//				// TE: Fire AGCEvent informing of the kick
			//				LPCSTR pszContext = pfsMission->GetIGCMission()->GetContextName();
			//				_AGCModule.TriggerContextEvent(NULL, EventID_LogoutGame, pszContext, pfsShip->GetName(),
			//					pfsShip->GetShipID(), -1, -1, 3, "GameID", VT_I4, pfsMission->GetMissionID(),
			//					"GameName", VT_LPSTR, pfsMission->GetIGCMission()->GetMissionParams()->strGameName,
			//					"zBanner", VT_LPSTR, pfsPlayer->GetName());
			//				// This is named zBooter so the parameter appears last in the alphabetical list
			//				// TE

			//				break;
			//			}
			//		}
			//	}
			//}

          //NYI hack to allow misison owner to boot people from the lobby
          if (pfmChat->cbMessage > 5 && pfsMission->GetOwner() == pfsPlayer)
          {
            if (_strnicmp(FM_VAR_REF(pfmChat, Message), "#ban ", 5) == 0)
            {
              //Find a player with the given name
              for (ShipLinkIGC* psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
              {
                CFSShip*  pfsShip = (CFSShip*)(psl->data()->GetPrivateData());
                if (pfsShip->IsPlayer() && (_stricmp(FM_VAR_REF(pfmChat, Message) + 5, psl->data()->GetName()) == 0))
                {
                  if (pfsShip->GetSide()->GetObjectID() < 0)
                  {
                    // send them to the game lobby
                    debugf("#ban: %s banned by %s\n", pfsShip->GetName(), pfsPlayer->GetName());
					// TE: Fire AGCEvent informing of the ban
					LPCSTR pszContext = pfsMission->GetIGCMission()->GetContextName();
					_AGCModule.TriggerContextEvent(NULL, EventID_LogoutGame, pszContext, pfsShip->GetName(),
						pfsShip->GetShipID(), -1, -1, 3, "GameID",   VT_I4,    pfsMission->GetMissionID(),
						"GameName", VT_LPSTR, pfsMission->GetIGCMission()->GetMissionParams()->strGameName,
						"zBooter", VT_LPSTR, pfsPlayer->GetName());
					// This is named zBooter so the parameter appears last in the alphabetical list
					// TE
                    pfsMission->RemovePlayerFromMission(pfsShip->GetPlayer(), QSR_OwnerBooted);

                    pfmChat->cd.chatTarget = CHAT_EVERYONE;
                    pfmChat->cd.oidRecipient = NA;
                    break;
                  }
                }
              }
              bForward = false;
            }
          }

          if ((pfsPlayer->GetSide()->GetObjectID() >= 0) && (pfsMission->GetStage() == STAGE_STARTED))
          {
              if (pfmChat->cbMessage > strlen("#resign") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#resign", strlen("#resign")) == 0)
              {
				// mmf/KGJV 09/07 allow only one ballot of each type at a time
				if (!pfsMission->HasBallots(BALLOT_RESIGN))
                  pfsMission->AddBallot(new ResignBallot(pfsPlayer));
				else pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsPlayer->GetIGCShip()->GetObjectID(),
					NA, "Resign vote already in progress...");
                bForward = false;
              }
              else if (pfmChat->cbMessage > strlen("#draw") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#draw", strlen("#draw")) == 0)
              {
				// mmf/KGJV 09/07 allow only one ballot of each type at a time
				if (!pfsMission->HasBallots(BALLOT_OFFERDRAW))
                  pfsMission->AddBallot(new OfferDrawBallot(pfsPlayer));
				else pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsPlayer->GetIGCShip()->GetObjectID(),
					NA, "Draw vote already in progress...");
                bForward = false;
              }
				// KGJV #110
               else if (pfmChat->cbMessage > strlen("#mutiny") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#mutiny", strlen("#mutiny")) == 0)
              {
				// leader cant mutiny
			    if (pfsMission->GetLeader(pfsPlayer->GetSide()->GetObjectID()) != pfsPlayer)
				  // mmf/KGJV 09/07 allow only one ballot of each type at a time
				  if (!pfsMission->HasBallots(BALLOT_MUTINY))
				    pfsMission->AddBallot(new MutinyBallot(pfsPlayer));
				  else pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsPlayer->GetIGCShip()->GetObjectID(),
					NA, "Mutiny vote already in progress...");
                bForward = false;
              }
			   // yp Your_Persona bug98 (toggle auto accept after game launch) aug-3-2006
			   // mmf changed this from GetOwner to GetLeader
			  else if(pfsMission->GetLeader(pfsPlayer->GetSide()->GetObjectID()) == pfsPlayer && pfmChat->cbMessage > strlen("#autoaccept") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#autoaccept", strlen("#autoaccept")) == 0)
              {
				pfsMission->SetAutoAccept(pfsPlayer->GetSide(),!pfsMission->GetAutoAccept(pfsPlayer->GetSide()));
                bForward = true;
				g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfsMission->GetMissionDef(), FM_GUARANTEED); //Imago7/10
			  }
			  // mmf added ability to change balance (MaxTeamImbalance) to 1 2 a (for auto) n (for N/A)
			  else if(pfsMission->GetOwner() == pfsPlayer && pfmChat->cbMessage > strlen("#autobalance  ") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#autobalance", strlen("#autobalance")) == 0)
              {
                // check all cases explicitly instead of doing a string manipulation to pull of the parameter
				if (_strnicmp(FM_VAR_REF(pfmChat, Message), "#autobalance 1", strlen("#autobalance 1")) == 0)
					pfsMission->SetMaxTeamImbalance(1);
				if (_strnicmp(FM_VAR_REF(pfmChat, Message), "#autobalance 2", strlen("#autobalance 2")) == 0)
					pfsMission->SetMaxTeamImbalance(2);
				if (_strnicmp(FM_VAR_REF(pfmChat, Message), "#autobalance a", strlen("#autobalance a")) == 0)
					pfsMission->SetMaxTeamImbalance(0x7ffe);
				if (_strnicmp(FM_VAR_REF(pfmChat, Message), "#autobalance n", strlen("#autobalance n")) == 0)
					pfsMission->SetMaxTeamImbalance(0x7fff);
                pfmChat->cd.chatTarget = CHAT_EVERYONE;
                pfmChat->cd.oidRecipient = NA;
				bForward = true;
				g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfsMission->GetMissionDef(), FM_GUARANTEED); //Imago #4 7/10
			  }
			  // mmf 03/22/07 added #list to display all the other # commands
			  //              keep this at the end of this else if
			  else if(pfmChat->cbMessage > strlen("#list") && _strnicmp(FM_VAR_REF(pfmChat, Message), "#list", strlen("#list")) == 0)
              {
			    // mmf send message to self
				// KGJV #110 added #mutiny
				pfsMission->GetSite()->SendChatf(NULL, CHAT_INDIVIDUAL_NOFILTER, pfsPlayer->GetIGCShip()->GetObjectID(),
					NA, "#draw | #resign | #autoaccept | #autobalance [1|2|a|n] | #ban | #mutiny");
				bForward = false;
			  }
          }
		 // KGJV: added '#start' (equiv to cheat code 'fifteen') for game owner
		 // beta feature only  imago made one less define 7/8/09 and fixed for non-debug

		  else
		  {
#ifdef _TEST
			  if (pfsMission->GetOwner() == pfsPlayer && (_strnicmp(FM_VAR_REF(pfmChat, Message), "#start", strlen("#start")) == 0))
			  {
			    CFSMission * pfsMission = pfsPlayer->GetMission();
				if (STAGE_NOTSTARTED == pfsMission->GetStage() || STAGE_STARTING == pfsMission->GetStage())
					pfsMission->StartCountdown(1);
			  }
#endif
		  }

        }
      }
      pfmChat->cd.sidSender = pfsPlayer->GetIGCShip()->GetObjectID();

      char* pszMsg = FM_VAR_REF(pfmChat, Message);

      // If the message starts with "ICQ,playername:" then it's an icq message
      // If the format doesn't match exactly, then we treat it as a regular chat
      #if !defined(ALLSRV_STANDALONE)
        if (pszMsg && (0 == _strnicmp("icq,", pszMsg, 4)))
        {
            char * szName = pszMsg + 4;
            char * szColon = szName;
            while (*szColon && (':' != *szColon)) //
              szColon++;


            if (*szColon)
            {
              *szColon = '\0';
              lstrcpyn((char *)GetICQID_CharName, szName, sizeof(GetICQID_CharName));

              SQL_GO(GetICQID);
              SQLRETURN sqlret = SQL_GETROW(GetICQID);
              if (*(++szColon) && SQL_SUCCESS == sqlret && GetICQID_ID != -1)
              {
                BEGIN_PFM_CREATE(g.fm, pfmICQChat, S, ICQ_CHAT_ACK)
                    FM_VAR_PARM(szColon, CB_ZTS)
                END_PFM_CREATE
                pfmICQChat->icqid = GetICQID_ID;
                g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

                bForward = false;
              }
            }
        }
      #endif // !defined(ALLSRV_STANDALONE)

      if (bForward)
          ForwardChatMessage(pfsMission, pfsPlayer, pfm,
                             &(pfmChat->cd),
                             pszMsg,
                             pfmChat->otTarget,
                             pfmChat->oidTarget,
                             NULL);
	}
    break;

    case FM_CS_CHATBUOY:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
          break;

      CASTPFM(pfmBuoy, CS, CHATBUOY, pfm);
      pfmBuoy->cd.sidSender = pfsPlayer->GetIGCShip()->GetObjectID();

      ForwardChatMessage(pfsMission, pfsPlayer, pfm,
                         &(pfmBuoy->cd),
                         FM_VAR_REF(pfmBuoy, Message),
                         OT_buoy,
                         NA,
                         &(pfmBuoy->db));
      break;
    }

    case FM_CS_SET_WINGID: //Imago modified to allow clients to set the starting wing. #91 6/10:
    {
      if (!pfsMission)
        break;

      IsideIGC* pside = pfsPlayer->GetSide();
      //if (pside->GetObjectID() < 0)
          //break;

      CASTPFM(pfmSW, CS, SET_WINGID, pfm);

      //TrapHackBoot(pfmSW->wingID >= 0);  //Imago commented 7/10 #91
      TrapHackBoot(pfmSW->wingID < c_widMax);

      CFSShip*  pfsShip = CFSShip::GetShipFromID(pfmSW->shipID);
      if (pfsShip)
      {
          if (pfsShip != pfsPlayer)
          {
              if ((pfsShip->GetSide() != pside) ||
                  (pfsPlayer != pfsMission->GetLeader(pside->GetObjectID())))
              {
                  break;
              }
              pfmSW->bCommanded = true;
          }
          else
              pfmSW->bCommanded = false;        //If sent by a player, it should already be set correctly on the client

          //pfmSW->shipID = pfsPlayer->GetShipID();
          g.fm.ForwardMessage(CFSSide::FromIGC(pside)->GetGroup(), pfmSW, FM_GUARANTEED);

          pfsShip->GetIGCShip()->SetWingID(pfmSW->wingID);
      }
    }
    break;

    case FM_C_VOTE:
    {
      // you must be on a valid team to vote.
      if (!pfsMission || pfsPlayer->GetSide()->GetObjectID() < 0)
        break;

      CASTPFM(pfmVote, C, VOTE, pfm);

      pfsMission->TallyVote(pfsPlayer, pfmVote->ballotID, pfmVote->bAgree);
    }
    break;

    case FM_CS_REQUEST_MONEY:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
          break;

      CASTPFM(pfmRequest, CS, REQUEST_MONEY, pfm);

      TrapHackBoot(pfmRequest->amount > 0);

      if (!pfsPlayer->DecrementChatBudget(true))
      {
        break;
      }

      IshipIGC* pshipAD = pfsPlayer->GetIGCShip()->GetAutoDonate();
      if ((!pshipAD) || (((CFSShip*)(pshipAD->GetPrivateData()))->GetMoney() < pfmRequest->amount))
      {
          //Not autodonating to anyone ... find the person on the team with the most money
          Money     moneyMax = pfmRequest->amount - 1;

          IsideIGC* pside = pfsPlayer->GetSide();
          for (ShipLinkIGC* psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
          {
              IshipIGC* pship = psl->data();
              if (pship != pfsPlayer->GetIGCShip())
              {
                  CFSShip*  pfsship = (CFSShip*)(pship->GetPrivateData());

                  if (pfsship->GetMoney() > moneyMax)
                  {
                      moneyMax = pfsship->GetMoney();
                      pshipAD = pship;
                  }
              }
          }
      }

      if (pshipAD)
      {
          pfmRequest->shipidRequest = pfsPlayer->GetShipID();

          CFSShip*  pfsAD = (CFSShip*)(pshipAD->GetPrivateData());
          g.fm.ForwardMessage(pfsAD->GetPlayer()->GetConnection(), pfmRequest, FM_GUARANTEED);
      }
    }
    break;

    case FM_CS_FIRE_MISSILE:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
          break;

      CASTPFM(pfmFireMissile, CS, FIRE_MISSILE, pfm);

      DataMissileIGC  dm;
      dm.pLauncher = pfsPlayer->GetIGCShip();

      TrapHackBoot(dm.pLauncher->GetParentShip() == NULL);

      ImagazineIGC*   pmagazine = (ImagazineIGC*)(dm.pLauncher->GetMountedPart(ET_Magazine, 0));
      if (pmagazine)
      {
          TrapHackBoot(pmagazine->GetAmount() > 0);

          //Speed hack ... use the player's cluster if appropriate and the cluster from the missile
          //message if not.
          IclusterIGC*  pcluster = pfsPlayer->GetCluster();
          if ((!pcluster) || (pcluster->GetObjectID() != pfmFireMissile->clusterID))
            pcluster = pMission->GetCluster(pfmFireMissile->clusterID);

          TrapHackBoot (pcluster);

          Time  timeFudge = pfmFireMissile->timeFired + 0.5f;
          pfmFireMissile->bDud = (timeFudge < pmagazine->GetTimeLoaded());

          {
              if (pfmFireMissile->bDud)
                  debugf("dud missile %d %d %d\n", timeFudge.clock(), pmagazine->GetTimeLoaded().clock());
          }

          //Ignore the IDs provided by the client and, instead, generate one ourselves.
          pfmFireMissile->launcherID = pfsPlayer->GetShipID();

          dm.pmissiletype = pmagazine->GetMissileType();
          assert (dm.pmissiletype);

          int                   iNumMissiles = pfmFireMissile->cbmissileLaunchData / sizeof(MissileLaunchData);
          MissileLaunchData*    pMissileLaunchData = (MissileLaunchData*) (FM_VAR_REF(pfmFireMissile, missileLaunchData));
          pfmFireMissile->missiletypeID = dm.pmissiletype->GetObjectID();

          const Orientation&    myOrientation = dm.pLauncher->GetOrientation();
          Vector                position      = dm.pLauncher->GetPosition() + (pmagazine->GetEmissionPt() * myOrientation);
          const Vector&         myVelocity    = dm.pLauncher->GetVelocity();
          Vector                velocity      = myVelocity - dm.pmissiletype->GetInitialSpeed() * myOrientation.GetBackward();

          if (pfmFireMissile->targetType == NA)
              dm.pTarget = NULL;
          else
          {
              dm.pTarget = pMission->GetModel(pfmFireMissile->targetType, pfmFireMissile->targetID);
              if (dm.pTarget &&
                  ((dm.pTarget->GetCluster() != pcluster) ||
                   !pfsPlayer->GetIGCShip()->CanSee(dm.pTarget)))
                  dm.pTarget = NULL;
          }

          dm.pCluster = pcluster;
          dm.lock = pfmFireMissile->lock;

          for (int i = 0; i < iNumMissiles; i++)
          {
              {
                    Vector  d = position - pMissileLaunchData[i].vecPosition;
                    float   d2 = d * d;
                   // if (d2 > 20.0f)  // mmf 10/07 raised threshold from 10 to 20, we are not doing much with this info atm anyway - Removed debugging - Imago 9/10
                        //debugf("missile position error %f %s\n", d2, pfsPlayer->GetName());
                    if ((d2 < 0.0f) || (d2 > 200.0f * 200.0f))
                        pMissileLaunchData[i].vecPosition = position;
              }
              {
                    Vector  d = velocity - pMissileLaunchData[i].vecVelocity;
                    float   d2 = d * d;
                    //if (d2 > 20.0f) // mmf 10/07 raised threshold from 10 to 20, we are not doing much with this info atm anyway - Removed debugging - Imago 9/10
                        //debugf("missile velocity error %f\n", d2);
                    if ((d2 < 0.0f) || (d2 > 100.0f * 100.0f))
                        pMissileLaunchData[i].vecVelocity = velocity;
              }
              {
                    float   d2 = pMissileLaunchData[i].vecForward * pMissileLaunchData[i].vecForward;
                    if ((d2 < 0.95f * 0.95f) || (d2 > 1.05f * 1.05f))
                        pMissileLaunchData[i].vecForward = myOrientation.GetForward();
              }

              dm.position   = pMissileLaunchData[i].vecPosition;
              dm.velocity   = pMissileLaunchData[i].vecVelocity;
              dm.forward    = pMissileLaunchData[i].vecForward;
              dm.missileID  = pMissileLaunchData[i].missileID = pMission->GenerateNewMissileID ();
              dm.bDud       = pfmFireMissile->bDud;
              ImissileIGC * m = (ImissileIGC*)(pMission->CreateObject(pfmFireMissile->timeFired + dm.pmissiletype->GetReadyTime(), OT_missile, &dm, sizeof(dm)));
              if (m)
                m->Release();
          }

          //Tell everyone else to fire
          g.fm.ForwardMessage(GetGroupSectorFlying(pcluster), pfmFireMissile, FM_GUARANTEED);

          pmagazine->SetTimeFired(pfmFireMissile->timeFired);
          pmagazine->SetLock(0.0f);
          short   amount = pmagazine->GetAmount() - iNumMissiles;
          pmagazine->SetAmount(amount);
      }
    }
    break;

	// KGJV #110 - changed logic
    case FM_C_AUTODONATE:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
          break;

      CASTPFM(pfmAutoDonate, C, AUTODONATE, pfm);

      TrapHackBoot(pfmAutoDonate >= 0);
      TrapHackBoot(pfsPlayer->GetMoney() >= pfmAutoDonate->amount);

      IsideIGC* pside = pfsPlayer->GetSide();
	  SideID    sideID = pside->GetObjectID();
	  if (pfmAutoDonate->amount==-1 && pfsMission->GetLeader(sideID)!=pfsPlayer)
		  TrapHackBoot(false); // only leader can toggle someone donation

      CFSPlayer*    pfsDonate = NULL;
      ShipID        shipID = pfsPlayer->GetShipID();
      if ((pfmAutoDonate->sidDonateTo != NA) && (pfmAutoDonate->sidDonateTo != shipID))
      {
          CFSShip* pfsDonateTo = CFSShip::GetShipFromID(pfmAutoDonate->sidDonateTo);
          if (pfsDonateTo && pfsDonateTo->IsPlayer() && (pside == pfsDonateTo->GetSide()))
          {
              pfsDonate = pfsDonateTo->GetPlayer();
          }
      }

	  // logic change here
	  // if amount is -1 then it's the leader that is toggling someone donation
	  // otherwise do like previous version (aka redonate to leader or set new leader)
	  CFSPlayer *pfsPlayerChanged = NULL;
	  if (pfmAutoDonate->amount == -1)
	  {
		  if (!pfsDonate) break; // in case target dropped/left
		  // toggle donation:
		  if (pfsDonate->GetAutoDonate() != NULL)
			pfsDonate->SetAutoDonate(NULL,0);
		  else
			pfsDonate->SetAutoDonate(pfsPlayer,0);
		  pfsPlayerChanged = pfsDonate;
	  }
	  else
	  {
		  pfsPlayer->SetAutoDonate(pfsDonate, pfmAutoDonate->amount);
  		  pfsPlayerChanged = pfsPlayer;
	  }
	  assert(pfsPlayerChanged);

      if (pMission->GetMissionStage() == STAGE_STARTED)
      {
          IshipIGC*     pshipNewDonate = pfsPlayerChanged->GetIGCShip()->GetAutoDonate();

          SideID        sideID = pside->GetObjectID();
          IshipIGC*     pshipLeader = pfsMission->GetLeader(sideID)->GetIGCShip();
          if (pshipLeader != pshipNewDonate)
          {
              const ShipListIGC*    pships = pside->GetShips();

              //Donated to someone other than the existing leader ... possible
              //leader ship change ... see if someone has more votes than the leader
              int   cOther = 0;
              int   cLeader = 0;
              {
                  for (ShipLinkIGC* psl = pships->first(); (psl != NULL); psl = psl->next())
                  {
                      IshipIGC* pshipAD = psl->data()->GetAutoDonate();
                      if (pshipAD)
                      {
                          if (pshipAD == pshipLeader)
                              cLeader++;
                          else
                              cOther++;
                      }
                  }
              }

              if (cLeader < cOther)
              {
                  //Leader has less than the possible votes against
                  //Possible change of command

                  //Who has more votes than the leader?
                  IshipIGC* pshipNewLeader = PickNewLeader(pships, pshipLeader, cLeader);
                  if (pshipNewLeader)
                  {
                      // if there is a human player still on their team with the given ship ID,
                      // make that new player the leader.
                      CFSShip*  pfsNewLeader = (CFSShip*)(pshipNewLeader->GetPrivateData());
                      pfsMission->SetLeader(pfsNewLeader->GetPlayer());
                  }
              }
          }
      }
    }
    break;

    case FM_C_FIRE_EXPENDABLE:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
          break;

      CASTPFM(pfmFireExpendable, C, FIRE_EXPENDABLE, pfm);

      IshipIGC*             pShip = pfsPlayer->GetIGCShip();
      assert (pShip);

      TrapHackBoot(pShip->GetParentShip() == NULL);

      IdispenserIGC*        pdispenser = (IdispenserIGC*)(pShip->GetMountedPart(pfmFireExpendable->et, 0));
      if (pdispenser)
      {
          TrapHackBoot(pdispenser->GetAmount() > 0);

          IexpendableTypeIGC*   pet = pdispenser->GetExpendableType();

          IclusterIGC*          pcluster = pShip->GetCluster();
          if (pcluster)
          {
              assert(0 == g.fm.CbUsedSpaceInOutbox());
              {
                  BEGIN_PFM_CREATE(g.fm, pfmSFE, S, FIRE_EXPENDABLE)
                  END_PFM_CREATE

                  pfmSFE->launcherID = pShip->GetObjectID();
                  pfmSFE->equipmentType = pfmFireExpendable->et;
              }

              const Vector&      myPosition      = pShip->GetPosition();
              const Vector&      myVelocity      = pShip->GetVelocity();
              const Orientation& myOrientation   = pShip->GetOrientation();
              ObjectType    type = pet->GetObjectType();
              if (type == OT_chaffType)
              {
                  //Drop the chaff "behind" the player's ship

                  DataChaffIGC   dataChaff;

                  dataChaff.time0 = g.timeNow;
                  dataChaff.p0 = myPosition + pShip->GetBaseHullType()->GetChaffPosition() * myOrientation; // TurkeyXIII 11/09 #94
                  dataChaff.v0 = myVelocity + myOrientation.GetUp() * 5.0f;

                  dataChaff.pcluster = pcluster;
                  dataChaff.pchafftype = (IchaffTypeIGC*)pet;

                  {
                      BEGIN_PFM_CREATE(g.fm, pfmCC, S, CREATE_CHAFF)
                      END_PFM_CREATE

                      pfmCC->p0 = dataChaff.p0;
                      pfmCC->v0 = dataChaff.v0;
                      pfmCC->time0 = dataChaff.time0;
                      pfmCC->etid = pet->GetObjectID();
                  }

                  IchaffIGC*  c = (IchaffIGC*)(pMission->CreateObject(g.timeNow,
                                                                    OT_chaff,
                                                                    &dataChaff,
                                                                    sizeof(dataChaff)));
                  assert (c != NULL);

                  //Confuse any missiles lauched at the player's ship
                  for (MissileLinkIGC*  pml = pcluster->GetMissiles()->first();
                       (pml != NULL);
                       pml = pml->next())
                  {
                      ImissileIGC*    pmissile = pml->data();
                      if (pmissile->GetTarget() == pShip)
                      {
                          //A missing aimed at me ... do the chaff work?
                          float   chaff = ((IchaffTypeIGC*)pet)->GetChaffStrength();
                          float   missile = pmissile->GetMissileType()->GetChaffResistance();

                          //The following is equivalent to random(0, chaff) > random(0, missile)
                          float   cm = chaff * missile;
                          float   f = (chaff > missile)
                                      ? (cm - 0.5f * missile * missile)
                                      : (0.5f * chaff * chaff);

                          if (random(0.0f, cm) <= f)
                          {
                              //Missile lost lock
                              pmissile->SetTarget(c);

                              BEGIN_PFM_CREATE(g.fm, pfmSpoof, S, MISSILE_SPOOFED)
                              END_PFM_CREATE
                              pfmSpoof->missileID = pmissile->GetObjectID();
                          }
                      }
                  }

                  BEGIN_PFM_CREATE(g.fm, pfmSpoof, S, END_SPOOFING)
                  END_PFM_CREATE

                  c->Release();
              }
              else
              {
                  float              speed2 = myVelocity.LengthSquared();
                  float              offset = (pet->GetRadius() + pShip->GetRadius()) + 5.0f;
                  Vector             displace = (speed2 < 1.0f)
                                                ? (myOrientation.GetBackward() * offset)
                                                : (myVelocity * (-offset / float(sqrt(speed2))));
                  Vector             position = myPosition + displace;

                  IsideIGC* pside = pShip->GetSide();

                  if (type == OT_mineType)
                  {
                      DataMineIGC  dm;
                      dm.pshipLauncher = pShip;
                      dm.psideLauncher = pside;

                      dm.mineID = pMission->GenerateNewMineID();

                      dm.time0 = pShip->GetLastUpdate() + 3.0f; //3 second delay
                      dm.p0 = position;
                      dm.pminetype = (ImineTypeIGC*)pet;
                      assert (dm.pminetype);

                      dm.pcluster = pcluster;
                      dm.exportF = false;

                      ImineIGC * m = (ImineIGC*)(pMission->CreateObject(
                                               g.timeNow, OT_mine, &dm, sizeof(dm)));

                      if (m)
                        m->Release();
                  }
                  else
                  {
                      assert (type == OT_probeType);

                      DataProbeIGC dp;
                      dp.pside = pShip->GetSide();
                      dp.pship = pShip;
                      dp.probeID = pMission->GenerateNewProbeID();

                      dp.time0 = pShip->GetLastUpdate();

                      // Potentially unsafe
                      DisplaceProbe(pcluster, &position, pet->GetRadius()); //Imago 6/10 I live on the edge of unsafe!

                      dp.p0 = position;
                      dp.pprobetype = (IprobeTypeIGC*)pet;
                      assert (dp.pprobetype);

                      dp.pcluster = pcluster;
                      dp.exportF = false;

                      dp.pmodelTarget = pShip->GetCommandTarget(c_cmdCurrent);

                      IprobeIGC * p = (IprobeIGC*)(pMission->CreateObject(
                                                   g.timeNow, OT_probe, &dp, sizeof(dp)));

                      if (p)
                        p->Release();
                  }
              }

              short   amount = pdispenser->GetAmount() - 1;
              assert (amount >= 0);
              pdispenser->SetAmount(amount);
              g.fm.SendMessages(GetGroupSectorFlying(pcluster), FM_GUARANTEED, FM_FLUSH);
          }
      }
    }
    break;

    case FM_CS_LOGOFF:
    {
      if (NULL != g.pServerCounters)
        g.pServerCounters->cLogoffs++;

      debugf("Logoff: %s(%u), was on side=%d mission=%x.\n",
          pfsPlayer->GetName(), pfsPlayer->GetConnection()->GetID(),
          (pfsPlayer->GetSide() ? pfsPlayer->GetSide()->GetObjectID() : NA),
          (pfsPlayer->GetMission() ? pfsPlayer->GetMission()->GetCookie() : NA));
      g.fm.ForwardMessage(pfsPlayer->GetConnection(), pfm, FM_GUARANTEED);

	  // BT - STEAM
	  SteamGameServer()->EndAuthSession(CSteamID(pfsPlayer->GetConnection()->GetSteamID()));

      delete pfsPlayer; //need to delete here, instead of in OnDestroyConnection, so we can tell when we get there whether it was a dropped connection
    }
    break;

    case FM_C_LOGONREQ:
    {
      CQLogonStats * pquery = new CQLogonStats(GotLogonDetails);
      CQLogonStatsData * pqd = pquery->GetData();
      if (NULL != g.pServerCounters)
        g.pServerCounters->cLoginAttempts++;

      CASTPFM(pfmLogon, C, LOGONREQ, pfm);
      char * szReason = "Sorry Charlie. Something \"weird\" happened."; // default failure message
      bool fRetry = false;

      // sanity check the message
      TrapHackBootNoPlayer(NULL != FM_VAR_REF(pfmLogon, CharacterName)
          && NULL != memchr(FM_VAR_REF(pfmLogon, CharacterName), 0, c_cbName));
      TrapHackBootNoPlayer(NULL == FM_VAR_REF(pfmLogon, MissionPassword)
          || NULL != memchr(FM_VAR_REF(pfmLogon, MissionPassword), 0, c_cbGamePassword));
      TrapHackBootNoPlayer(NULL != FM_VAR_REF(pfmLogon, CDKey)
          && NULL != memchr(FM_VAR_REF(pfmLogon, CDKey), 0, c_cbCDKey));

      if (pfmLogon->fedsrvVer == MSGVER)
      { // ok, they have the right version of the client
        // Now we need to look them up to make sure they logged in

        bool fValid = false; // whether we have a valid character
        LPBYTE pZoneTicket = (LPBYTE) FM_VAR_REF(pfmLogon, ZoneTicket);
        //#if !defined(ALLSRV_STANDALONE)
		#ifdef USEAUTH
          if (pZoneTicket) // it's all in the Zone Ticket
          {
            hr = g.pzas->DecryptTicket(pZoneTicket, pfmLogon->cbZoneTicket);
            // Since they came in through the lobby, their token MUST be valid, but since we're paranoid, let's check again
            switch (hr)
            {
              case ZT_NO_ERROR:
              {
                if (lstrcmpi(g.strAuthServer, g.pzas->GetAuthServer())) // you MUST use the auth server that we expect
                {
                  szReason = "Your account authentication did not go through the expected server.";
                  break;
                }
                bool fValidNow = false;
                fValid = g.pzas->HasToken(g.m_szToken, &fValidNow);
                if (!fValid)
                  szReason = "Your Allegiance Zone subscription has expired.";
                else if (!fValidNow)
                {
                  fValid = false; // not considered valid anymore
                  szReason = "This account is not authorized to play on the Allegiance Zone.";
                  fRetry = true;
                }
                break;
              }

              case ZT_E_BUFFER_TOO_SMALL:
                _AGCModule.TriggerEvent(NULL, AllsrvEventID_IncreaseTokensMax, "", -1, -1, -1, 0);
                break;

              case ZT_E_AUTH_INVALID_TICKET:
                _AGCModule.TriggerEvent(NULL, AllsrvEventID_InvalidZoneTicket, "", -1, -1, -1, 0);
                szReason = "Could not validate Zone ID.";
                break;

              default:
                _AGCModule.TriggerEvent(NULL, AllsrvEventID_DecryptTicketFailed, "", -1, -1, -1, 0);
            }
          }
          else
          {
              _AGCModule.TriggerEvent(NULL, AllsrvEventID_NoTicket, "", -1, -1, -1, 0);
              szReason = "No login credentials found.";
          }

          lstrcpy(pqd->szCharacterName, g.pzas->GetName());
          pqd->characterID = g.pzas->GetAccountID();

        #else // defined(USEAUTH) -- was !defined(ALLSRV_STANDALONE)
          if (0 < pfmLogon->cbCharacterName && pfmLogon->cbCharacterName <= c_cbName)
          {
            fValid = true;
            lstrcpy(pqd->szCharacterName, FM_VAR_REF(pfmLogon, CharacterName));
            pqd->characterID = GetStateID(pqd->szCharacterName); //Imago 6/10
            pqd->fCanCheat = false;
          }
          else
          {
            _AGCModule.TriggerEvent(NULL, AllsrvEventID_NoTicket, "", -1, -1, -1, 0);
            szReason = "No player name found.";
            lstrcpy(pqd->szCharacterName, "<invalid>");
          }
        #endif // defined(USEAUTH) -- was !defined(ALLSRV_STANDALONE)

        lstrcpy(pqd->szReason, szReason);
        pqd->fRetry = fRetry;
        pqd->dwCookie    = pfmLogon->dwCookie;
        if(FM_VAR_REF(pfmLogon, MissionPassword))
          lstrcpy(pqd->szPassword, FM_VAR_REF(pfmLogon, MissionPassword));
        else
          pqd->szPassword[0] = 0;
        lstrcpy(pqd->szCDKey, FM_VAR_REF(pfmLogon, CDKey));
        pqd->fValid = fValid;
        pqd->dwConnectionID = cnxnFrom.GetID();

		// BT - STEAM
		if (g.bEnforceSteamDRM == true)
		{
			Strcpy(pqd->szDrmHash, pfmLogon->drmHash);
			pqd->steamID = pfmLogon->steamID;
			int tokenCRC = MemoryCRC(pfmLogon->steamAuthTicket, pfmLogon->steamAuthTicketLength);

			// BT - STEAM - Enable anyone in the Microsoft Allegiance - Developer group to connect to the game server with any client.
			if (CSteamUserGroupStatus::IsMemberOfDevelopersGroup(pqd->steamID) == false)
			{
				DrmVerifier drmVerifier;
				if (drmVerifier.CheckDrm(tokenCRC, pqd->szDrmHash) == false)
				{
					debugf("steam DRM check failed: %s\n", pqd->szDrmHash);

					pqd->fValid = false;
					szReason = "Your client has failed a DRM check. Please go back to your Steam app and reverify your installation. This action has been logged.";

					pqd->fValid = false;
					pqd->fRetry = false;
					//pqd->characterID = pfmLogon->CharacterID;
					Strcpy(pqd->szReason, szReason);
					PostThreadMessage(g.idReceiveThread, wm_sql_querydone, (WPARAM)NULL, (LPARAM)pquery);
					break;
				}
			}
		}

		//BT - STEAM
		// If we are connected to the lobby and using Steam authentication, then we are first going to make an async call to steam to validate that the client
		// is correct, then, we are going to make an async call to the Lobby to get the player's rank, then the return from that call will finally trigger the 
		// SQL Done state on the pQuery and let the user login.

		if (g.bUseSteamAuthentication == true 
			// If the user's CDKey matches the botAuthenticationGUID from the registry, then this login is a bot, so we can bypass all 
			// the steam security.
			&& strcmp(pqd->szCDKey, g.szBotAuthenticationGuid) != 0)
		{
			memcpy(pqd->steamAuthTicket, pfmLogon->steamAuthTicket, pfmLogon->steamAuthTicketLength);
			pqd->steamAuthTicketLength = pfmLogon->steamAuthTicketLength;
			pqd->steamID = pfmLogon->steamID;

			CSteamValidation *steamValidation = new CSteamValidation(g.idReceiveThread, pqd->characterID, pquery, pqd);
			DWORD threadID;
			CreateThread(NULL, 0, LogonThread, steamValidation, 0, &threadID);
		}
		else
		{
			if (strlen(pqd->szPassword) == 0)
				strcpy(pqd->szPassword, "PORKPORKPORK1234");

			ZString responseText("Bypassed Login");

			if (g.fmLobby.IsConnected())
			{
				// Kick off the rank retrieval from the lobby. The response from that request will complete the client logon, and then the user will be allowed into the server.
				BEGIN_PFM_CREATE(g.fmLobby, pfmPlayerRank, LS, PLAYER_RANK)
					FM_VAR_PARM(PCC(pqd->szCharacterName), CB_ZTS)
					FM_VAR_PARM(PCC(responseText), CB_ZTS)
					FM_VAR_PARM(PCC(pqd->szPassword), CB_ZTS)
					FM_VAR_PARM(PCC(pqd->szCDKey), CB_ZTS)
					END_PFM_CREATE

				pfmPlayerRank->characterID = pqd->characterID;
				pfmPlayerRank->fCanCheat = pqd->fCanCheat;
				pfmPlayerRank->fRetry = pqd->fRetry;
				pfmPlayerRank->dwCookie = pqd->dwCookie;
				pfmPlayerRank->fValid = true;
				pfmPlayerRank->dwConnectionID = pqd->dwConnectionID;

				g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
			}
			else
			{
				CQLogonStats * logonDetailsQuery = new CQLogonStats(GotLogonDetails);
				CQLogonStatsData * logonDetailsQueryData = logonDetailsQuery->GetData();

				lstrcpy(logonDetailsQueryData->szCDKey, pqd->szCDKey);
				lstrcpy(logonDetailsQueryData->szCharacterName, pqd->szCharacterName);
				lstrcpy(logonDetailsQueryData->szPassword, pqd->szPassword);
				lstrcpy(logonDetailsQueryData->szReason, "Login Success.");

				logonDetailsQueryData->characterID = pqd->characterID;
				logonDetailsQueryData->fCanCheat = pqd->fCanCheat;
				logonDetailsQueryData->fRetry = pqd->fRetry;
				logonDetailsQueryData->dwCookie = pqd->dwCookie;
				logonDetailsQueryData->fValid = true;
				logonDetailsQueryData->dwConnectionID = pqd->dwConnectionID;

				logonDetailsQueryData->rank = 7;
				logonDetailsQueryData->sigma = 0;
				logonDetailsQueryData->mu = 0;
				logonDetailsQueryData->commandRank = 0;
				logonDetailsQueryData->commandSigma = 0;
				logonDetailsQueryData->commandMu = 0;

				PostThreadMessage(g.idReceiveThread, wm_sql_querydone, (WPARAM)NULL, (LPARAM)logonDetailsQuery);
			}
		}

		//DWORD threadID;
		//CreateThread(NULL, 0, LogonThread, &steamValidation, 0, &threadID);

		// at this point, FedSrv is done with the message, now it's up to CSteamValidation to get
		// The response from steam, then kick off the player rank message to the lobby
		// When the lobby returns the rank results, this query will finally be complete with all the appropriate data. 




		// BT - 12/21/2010 ACSS - Use a message to the lobby server to get the player's rank details instead of
		// direct calls to the lobby web service.
		/*BEGIN_PFM_CREATE(g.fmLobby, pfmPlayerRank, LS, PLAYER_RANK)
			FM_VAR_PARM(PCC(pqd->szCharacterName), CB_ZTS) 
			FM_VAR_PARM(PCC(pqd->szReason), CB_ZTS) 
			FM_VAR_PARM(PCC(pqd->szPassword), CB_ZTS) 
			FM_VAR_PARM(PCC(pqd->szCDKey), CB_ZTS) 
		END_PFM_CREATE

		pfmPlayerRank->characterID = pqd->characterID;
		pfmPlayerRank->fCanCheat = pqd->fCanCheat;
		pfmPlayerRank->fRetry = pqd->fRetry;
		pfmPlayerRank->dwCookie = pqd->dwCookie;
		pfmPlayerRank->fValid = pqd->fValid;
		pfmPlayerRank->dwConnectionID = pqd->dwConnectionID;

		g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);*/

		// Wait for a PLAYER_RANK message to come back from the lobby.


#if !defined(ALLSRV_STANDALONE)
        if (fValid)
          g.sql.PostQuery(pquery);
        else //pquery->OnDataReady() below is the else of this
#endif
		// imago commented below out, "mpr thread" will tell the main when true 7/5/08
		//pquery->DataReady();

      } // if correct messaging version
      else
      {
        if (NULL != g.pServerCounters)
          g.pServerCounters->cLoginsFailed++;
        char szVerError[350];

        if (pfmLogon->fedsrvVer < MSGVER)
          wsprintf(szVerError, "The server you are trying to play on has a newer version than you.  "
            "Please go play online to get the latest auto-update.  If this doesn't work, try deleting "
            "the file 'filelist.txt' from the install directory and restarting the application.");
        else /* if (pfmLogon->fedsrvVer > MSGVER) */
        {
#if defined(ALLSRV_STANDALONE)
          wsprintf(szVerError, "The server you are trying to play on is out-of-date.  Find another server "
            "or contact the server's owner and tell them to auto-update by connecting to the zone match-making "
            "service.  If that doesn't work, have the server's owner try deleting "
            "the file 'filelist.txt' from the install directory and restarting the server.");
#else
          wsprintf(szVerError, "The server you are trying to play on is out-of-date.  The Zone needs to update their game server(s).  Please try again later.");
#endif
        }
        assert(lstrlen(szVerError) < sizeof(szVerError));
        _AGCModule.TriggerEvent(NULL, AllsrvEventID_BadClientVersion, "", -1, -1, -1, 2,
            "ServerVer", VT_I4, MSGVER,
            "ClientVer", VT_I4, pfmLogon->fedsrvVer);
        BEGIN_PFM_CREATE(g.fm, pfmLogonAck, S, LOGONACK)
          FM_VAR_PARM(szVerError, CB_ZTS)
          FM_VAR_PARM(NULL, CB_ZTS)
        END_PFM_CREATE
        pfmLogonAck->fValidated = false;
        pfmLogonAck->fRetry = false;
        g.fm.SendMessages(&cnxnFrom, FM_GUARANTEED, FM_FLUSH);
      }

#ifdef DEBUG
      DWORD cMsgs, cbData;
      ZSucceeded(g.fm.GetSendQueue(&cMsgs, &cbData));
      debugf("GetSendQueue reports %d msgs, %d bytes.\n", cMsgs, cbData);
#endif

      break;
    }

    case FM_C_TREASURE_ACK:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
        break;

      if (pfsPlayer->GetTreasureObjectID() != NA)
      {
          IsideIGC* pside = pfsPlayer->GetSide();
          if (pside->GetObjectID() >= 0)
          {
              TrapHackBoot(pfsPlayer->GetIGCShip()->GetParentShip() == NULL);

              CASTPFM(pfmTA, C, TREASURE_ACK, pfm);

              ObjectID  oid = pfsPlayer->GetTreasureObjectID();
              IpartTypeIGC* ppt = pMission->GetPartType(oid);
              assert(ppt);
              short amount = pfsPlayer->GetTreasureAmount();

              if (pfmTA->mountID != c_mountNA)
              {
                  TrapHackBoot(pfsPlayer->GetIGCShip()->GetHullType()->CanMount(ppt, pfmTA->mountID));

                  IpartIGC* ppart = pfsPlayer->GetIGCShip()->GetMountedPart(ppt->GetEquipmentType(), pfmTA->mountID);
                  if (ppart)
                  {
                      TrapHackBoot(ppart->GetPartType() == ppt);
                      ppart->SetAmount(ppart->GetAmount() + amount);
                  }
                  else
                    pfsPlayer->GetIGCShip()->CreateAndAddPart(ppt, pfmTA->mountID, amount);

                  IclusterIGC*  pcluster = pfsPlayer->GetIGCShip()->GetCluster();
                  if (pcluster)
                  {
                      BEGIN_PFM_CREATE(g.fm, pfmAddPart, S, ADD_PART)
                      END_PFM_CREATE

                      pfmAddPart->shipID = pfsPlayer->GetShipID();
                      pfmAddPart->newPartData.partID = oid;
                      pfmAddPart->newPartData.mountID = pfmTA->mountID;
                      pfmAddPart->newPartData.amount = amount;
                      g.fm.SendMessages(GetGroupSectorFlying(pcluster), FM_GUARANTEED, FM_FLUSH);
                  }
              }
              else if (pfsPlayer->GetIGCShip()->GetCluster())
              {
                  CreateTreasure(g.timeNow, pfsPlayer->GetIGCShip(), amount, ppt,
                                 pfsPlayer->GetIGCShip()->GetPosition(), 25.0f, 120.0f);
              }
          }

          pfsPlayer->SetTreasureObjectID(NA);
      }
    }
    break;

    case FM_C_BUY_LOADOUT:
    {
      if (!pfsMission
          || (pfsPlayer->GetIGCShip()->GetCluster() != NULL)
          || (pfsPlayer->GetIGCShip()->GetStation() == NULL)
          || pfsPlayer->GetIGCShip()->IsGhost())
        break;

      IsideIGC* pside = pfsPlayer->GetSide();
      if (pside->GetObjectID() < 0)
          break;

      //Imago ALLY 8/10/09 turreted ship @ allied base fix
      if ((pfsPlayer->GetIGCShip()->GetStation()->GetSide() != pside) &&
          (pside->AlliedSides(pside,pfsPlayer->GetIGCShip()->GetStation()->GetSide())) &&
          (pfsPlayer->GetIGCShip()->GetParentShip() == NULL) && (pfsPlayer->GetIGCShip()->GetChildShips()->n() != 0))
      {
          AlliedRearm(pfsPlayer->GetIGCShip(),true);
          break;
      }

      TrapHackBoot(pfsPlayer->GetIGCShip()->GetParentShip() == NULL);

      CASTPFM(pfmBuyLoadout, C, BUY_LOADOUT, pfm);

      TrapHackBoot(pfmBuyLoadout->ibloadout == sizeof(FMD_C_BUY_LOADOUT));
      TrapHackBoot(pfmBuyLoadout->cbloadout >= sizeof(ShipLoadout));
      TrapHackBoot((pfmBuyLoadout->cbloadout - sizeof(ShipLoadout)) % sizeof(ExpandedPartData) >= 0);

      TrapHackBoot(pfmBuyLoadout->cbloadout <= c_cbLoadout);


      // refund the cost for the player's current ship
      Money cost = -pfsPlayer->GetIGCShip()->GetValue();

      IhullTypeIGC* phtOld = pfsPlayer->GetIGCShip()->GetBaseHullType();

      // buy what we can of the new ship
      bool bBoughtEverything = pfsPlayer->GetIGCShip()->PurchaseShipLoadout(pfmBuyLoadout->cbloadout,
                                                                            (const ShipLoadout*)(FM_VAR_REF(pfmBuyLoadout, loadout)));

      // add up the cost of the new ship
      cost += pfsPlayer->GetIGCShip()->GetValue();

      // spend the money, if any
      if (cost != 0)
      {
        TrapHackBoot(pfsPlayer->GetMoney() >= cost);
        pfsPlayer->SetMoney(pfsPlayer->GetMoney() - cost);

        //Debit the player's money to all other player's on the side
        //(the player has already deducted the change and will ignore this)
        BEGIN_PFM_CREATE(g.fm, pfmMoneyChange, S, MONEY_CHANGE)
        END_PFM_CREATE
        pfmMoneyChange->dMoney = -cost;
        pfmMoneyChange->sidTo = pfsPlayer->GetShipID();
        pfmMoneyChange->sidFrom = NA;

        g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
      }

      //Boot all observers from the ship
      const ShipListIGC*    pshipsChildren = pfsPlayer->GetIGCShip()->GetChildShips();
      IhullTypeIGC* phtNew = pfsPlayer->GetIGCShip()->GetBaseHullType();
      if (phtOld != phtNew)
      {
          ShipLinkIGC*  pslNext;
          for (ShipLinkIGC* psl = pshipsChildren->first(); (psl != NULL); psl = pslNext)
          {
              pslNext = psl->next();

              if (psl->data()->GetTurretID() < 0)
                LeaveShip(((CFSShip*)(psl->data()->GetPrivateData()))->GetPlayer(), pfsPlayer->GetIGCShip());
          }
      } 
	  else if (phtNew->GetMaxWeapons() > phtNew->GetMaxFixedWeapons()) //Turkey 07/10 #145
	  {
		  ShipLinkIGC*  pslNext;
          for (ShipLinkIGC* psl = pshipsChildren->first(); (psl != NULL); psl = pslNext)
          {
              pslNext = psl->next();

			  PartLinkIGC*   ppl;
              for (ppl = pfsPlayer->GetIGCShip()->GetParts()->first();
              (ppl != NULL);
              ppl = ppl->next())
              {
				  if (ppl->data()->GetMountID() == psl->data()->GetTurretID())
					  break;
              }

              if (ppl == NULL)
                LeaveShip(((CFSShip*)(psl->data()->GetPrivateData()))->GetPlayer(), pfsPlayer->GetIGCShip());
          }
	  }//end #145

      bool bLaunch = pfmBuyLoadout->fLaunch && bBoughtEverything;        

      // tell the client what happened
      BEGIN_PFM_CREATE(g.fm, pfmBuyLoadoutAck, S, BUY_LOADOUT_ACK)
        FM_VAR_PARM(NULL, pfsPlayer->GetIGCShip()->ExportShipLoadout(NULL))
      END_PFM_CREATE

      pfsPlayer->GetIGCShip()->ExportShipLoadout((ShipLoadout*)(FM_VAR_REF(pfmBuyLoadoutAck, loadout)));

      pfsPlayer->SaveDesiredLoadout();

      pfmBuyLoadoutAck->fBoughtEverything = bBoughtEverything;
      pfmBuyLoadoutAck->fLaunch = bLaunch;

      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

      //Tell the surviving kids about dad's new toys
      pfsPlayer->QueueLoadoutChange();

      {
          SideID    sid = pfsPlayer->GetSide()->GetObjectID();
          pfsPlayer->GetShipStatus(sid)->SetHullID(pfsPlayer->GetIGCShip()->GetBaseHullType()->GetObjectID());

          for (ShipLinkIGC* psl = pshipsChildren->first(); (psl != NULL); psl = psl->next())
          {
              CFSShip*  pfsChild = (CFSShip*)(psl->data()->GetPrivateData());

              //Since it might have changed ... reset the stored turret ID as well
              pfsChild->GetShipStatus(sid)->SetState(psl->data()->GetTurretID() == NA ? c_ssObserver : c_ssTurret);

              g.fm.SendMessages(pfsChild->GetPlayer()->GetConnection(), FM_GUARANTEED, FM_DONT_FLUSH);
          }
      }
      g.fm.PurgeOutBox();

      if (bLaunch)
      {
        //Move the children of this ship to space as well
        for (ShipLinkIGC*   psl = pshipsChildren->first();
             (psl != NULL);
             psl = psl->next())
        {
          psl->data()->SetStation(NULL);
        }
        pfsPlayer->GetIGCShip()->SetStation(NULL);
      }
    }
    break;

    case FM_CS_DROP_PART:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED) || (pfsPlayer->GetIGCShip()->GetSide()->GetObjectID() < 0))
          break;

        TrapHackBoot (pfsPlayer->GetIGCShip()->GetParentShip() == NULL);
        IclusterIGC*  pcluster = pfsPlayer->GetIGCShip()->GetCluster();
        if (pcluster)
        {
            TrapHackBoot (pfsPlayer->GetSide()->GetObjectID() >= 0);

            CASTPFM(pfmDropPart, CS, DROP_PART, pfm);

            TrapHackBoot ((pfmDropPart->et >= NA) && (pfmDropPart->et < ET_MAX));
            TrapHackBoot ((pfmDropPart->mount >= -c_maxCargo) && (pfmDropPart->mount < c_maxMountedWeapons));

            TrapHackBoot ((pfmDropPart->mount <= 0) || (pfmDropPart->et == ET_Weapon));
            TrapHackBoot ((pfmDropPart->mount < 0) || (pfmDropPart->et >= 0));

            IpartIGC*       ppart = pfsPlayer->GetIGCShip()->GetMountedPart(pfmDropPart->et, pfmDropPart->mount);
            if (ppart)
            {
                pfmDropPart->shipID = pfsPlayer->GetShipID();
                g.fm.ForwardMessage(GetGroupSectorFlying(pcluster), pfmDropPart, FM_GUARANTEED);

                ObjectType  type = ppart->GetObjectType();
                if (((type != OT_pack) && !IlauncherIGC::IsLauncher(type)) ||
                    (ppart->GetAmount() != 0))
                {
                    CreateTreasure(g.timeNow, pfsPlayer->GetIGCShip(), ppart, ppart->GetPartType(),
                                   pfsPlayer->GetIGCShip()->GetPosition(), 25.0f, 120.0f);
                }

                ppart->Terminate();
            }
        }
    }
    break;

    case FM_C_SUICIDE:
    {
      // Only allow if player can cheat or is a child ship
      if (!(pfsPlayer->GetIGCShip()->GetParentShip() || pfsPlayer->CanCheat()))
        break;

      // Mission must be started
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
        break;

      //if the client does this multiple times, make sure we don't barf
      if (pfsPlayer->GetIGCShip()->GetCluster()) // NOT the pfsPlayer's cluster
        pfsMission->GetSite()->KillShipEvent(g.timeNow, pfsPlayer->GetIGCShip(), NULL, 1.0f,
                                             pfsPlayer->GetIGCShip()->GetSourceShip()->GetPosition(), Vector::GetZero());

      break;
    }

    case FM_C_PROMOTE:
    {
        // Mission must be started
        if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED) || pfsPlayer->GetIGCShip()->IsGhost())
            break;

        CASTPFM(pfmPromoteC, C, PROMOTE, pfm);

        for (ShipLinkIGC*   psl = pfsPlayer->GetIGCShip()->GetChildShips()->first(); (psl != NULL); psl = psl->next())
        {
            IshipIGC*   pship = psl->data();
            if (pship->GetTurretID() == pfmPromoteC->mountidPromoted)
            {
                pship->Promote();
                ((CFSShip*)(pship->GetPrivateData()))->ShipStatusRecalculate();

                BEGIN_PFM_CREATE(g.fm, pfmPromote, S, PROMOTE)
                END_PFM_CREATE
                pfmPromote->shipidPromoted = pship->GetObjectID();

                IclusterIGC*    pcluster = pfsPlayer->GetIGCShip()->GetCluster();
                if (pcluster)
                    g.fm.SendMessages(GetGroupSectorFlying(pcluster), FM_GUARANTEED, FM_FLUSH); //GetGroupRealSides(),
                else
                    g.fm.SendMessages(CFSSide::FromIGC(pfsPlayer->GetSide())->GetGroup(), FM_GUARANTEED, FM_FLUSH);

                break;
            }
        }
    }
    break;

    case FM_C_BOARD_SHIP:
    {
      // Mission must be started
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED) || pfsPlayer->GetIGCShip()->IsGhost())
        break;

      CASTPFM(pfmBoardShip, C, BOARD_SHIP, pfm);

      if (pfmBoardShip->sidParent == NA)
      {
        IshipIGC*   pship = pfsPlayer->GetIGCShip()->GetParentShip();
        if (pship)
        {
            if (LeaveShip(pfsPlayer, pship))
                break;
        }
      }
      else
      {
          IshipIGC*   pship = pfsPlayer->GetIGCShip()->GetSide()->GetShip(pfmBoardShip->sidParent);
          if (pship && !pship->IsGhost())
          {
            if (BoardShip(pfsPlayer, pship))
                break;
          }
      }

      BEGIN_PFM_CREATE(g.fm, pfmBoardNack, S, BOARD_NACK)
      END_PFM_CREATE
      pfmBoardNack->sidRequestedParent;

      g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
    }
    break;

    case FM_C_VIEW_CLUSTER:
    {
        if (pfsPlayer->GetIGCShip()->GetStation())
        {
            CASTPFM(pfmViewCluster, C, VIEW_CLUSTER, pfm);

            IclusterIGC*    pcluster;
            if (pfmViewCluster->clusterID != NA)
            {
                pcluster = pMission->GetCluster(pfmViewCluster->clusterID);

                TrapHackBoot(pcluster);

                BEGIN_PFM_CREATE(g.fm, pfmVC, S, VIEW_CLUSTER)
                END_PFM_CREATE

                pfmVC->clusterID = pfmViewCluster->clusterID;

                pfmVC->bUsePosition = false;
                if (pfmViewCluster->otTarget != NA)
                {
                    ImodelIGC*  ptarget = pMission->GetModel(pfmViewCluster->otTarget, pfmViewCluster->oidTarget);
                    if (ptarget && (ptarget->GetCluster() == pcluster) && ptarget->SeenBySide(pfsPlayer->GetSide()))
                    {
                        pfmVC->bUsePosition = true;
                        pfmVC->position = ptarget->GetPosition();
                    }
                }

                g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
            }
            else
                pcluster = NULL;

            pfsPlayer->SetCluster(pcluster, true);
        }
    }
    break;

    case FM_CS_SWAP_PART:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED) || (pfsPlayer->GetIGCShip()->GetSide()->GetObjectID() < 0))
        break;

      TrapHackBoot (pfsPlayer->GetIGCShip()->GetParentShip() == NULL);

      IclusterIGC*  pcluster = pfsPlayer->GetIGCShip()->GetCluster();
      if (pcluster)
      {
          CASTPFM(pfmSwapPart, CS, SWAP_PART, pfm);

          assert (pfsPlayer->GetIGCShip()->GetBaseHullType());
          TrapHack (pfmSwapPart->mountNew >= -c_maxCargo);

          TrapHackBoot ((pfmSwapPart->etOld >= NA) && (pfmSwapPart->etOld < ET_MAX));
          TrapHackBoot ((pfmSwapPart->mountOld >= -c_maxCargo) && (pfmSwapPart->mountOld < c_maxMountedWeapons));

          TrapHackBoot ((pfmSwapPart->mountOld <= 0) || (pfmSwapPart->etOld == ET_Weapon));
          TrapHackBoot ((pfmSwapPart->mountOld < 0) || (pfmSwapPart->etOld >= 0));

          TrapHackBoot ((pfmSwapPart->mountNew >= -c_maxCargo) && (pfmSwapPart->mountNew < c_maxMountedWeapons));

          TrapHackBoot ((pfmSwapPart->mountNew <= 0) || (pfmSwapPart->etOld == ET_Weapon));
          TrapHackBoot ((pfmSwapPart->mountNew < 0) || (pfmSwapPart->etOld >= 0));

          IpartIGC*   ppart = pfsPlayer->GetIGCShip()->GetMountedPart(pfmSwapPart->etOld, pfmSwapPart->mountOld);
          if (ppart)
          {
              IpartIGC*   ppartNew = pfsPlayer->GetIGCShip()->GetMountedPart(pfmSwapPart->etOld, pfmSwapPart->mountNew);
              if (ppartNew)
              {
                  ppart->SetMountID(c_mountNA);
                  ppartNew->SetMountID(pfmSwapPart->mountOld);
              }

              ppart->SetMountID(pfmSwapPart->mountNew);

              pfmSwapPart->shipID = pfsPlayer->GetShipID();
              g.fm.ForwardMessage(GetGroupSectorFlying(pcluster), pfmSwapPart, FM_GUARANTEED);
          }
      }
      break;
    }

    case FM_CS_RELOAD:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED) || (pfsPlayer->GetIGCShip()->GetSide()->GetObjectID() < 0))
        break;

      TrapHackBoot (pfsPlayer->GetIGCShip()->GetParentShip() == NULL);

      CASTPFM(pfmReload, CS, RELOAD, pfm);

      TrapHackBoot(pfmReload->ibrgReloads == sizeof(FMD_CS_RELOAD));

      IclusterIGC*  pcluster = pfsPlayer->GetIGCShip()->GetCluster();
      if (pcluster && (pfsPlayer->GetIGCShip()->GetParts()->n() != 0))      //NYI: hack to handle the lifepod case ... need something more sophisticated
      {
          const PartListIGC * ppartlist = pfsPlayer->GetIGCShip()->GetParts();

          ReloadData*   prlNext = (ReloadData*)FM_VAR_REF(pfmReload, rgReloads);
          int           nReloads = pfmReload->cbrgReloads / sizeof(ReloadData);

          TrapHackBoot((nReloads > 0) && (nReloads <= 5));  //fuel, ammo, missile, chaff, dispenser

          ReloadData*   prlStop = prlNext + nReloads;

          bool          bBoot = false;

          while (prlNext < prlStop)
          {
              if ((prlNext->mount >= 0) || (prlNext->mount < -c_maxCargo))
              {
				  debugf("(prlNext->mount >= 0) || (prlNext->mount < -c_maxCargo)\n"); //imago 7/14/09
                  bBoot = true;
                  break;
              }

              IpartIGC*     ppart = pfsPlayer->GetIGCShip()->GetMountedPart(NA, prlNext->mount);
              if (!ppart)
              {
				  debugf("! ppart = pfsPlayer->GetIGCShip()->GetMountedPart(NA, prlNext->mount)\n"); //imago 7/14/09
                  bBoot = true;
                  break;
              }

              ObjectType    type = ppart->GetObjectType();
              if (type == OT_pack)
              {
                  IpackIGC* ppack = (IpackIGC*)ppart;
                  PackType  packtype = ppack->GetPackType();
                  short     amount = ppack->GetAmount();

                  if (prlNext->amountTransfered == NA)
                  {
                      ppack->Terminate();
                  }
                  else
                  {
                      if ((prlNext->amountTransfered < 0) || (prlNext->amountTransfered >= amount))
                      {
						  debugf("(prlNext->amountTransfered < 0) || (prlNext->amountTransfered >= amount)\n"); //Imago 7/14/09
                          bBoot = true;
                          break;
                      }

                      ppack->SetAmount(amount - prlNext->amountTransfered);
                      amount = prlNext->amountTransfered;
                  }

                  if (packtype == c_packAmmo)
                  {
                      pfsPlayer->GetIGCShip()->SetAmmo(pfsPlayer->GetIGCShip()->GetAmmo() + amount);

                      //disable all mounted weapons that use ammo
                      Mount   maxWeapons = pfsPlayer->GetIGCShip()->GetHullType()->GetMaxWeapons();
                      for (Mount i = 0; (i < maxWeapons); i++)
                      {
                          IweaponIGC* pw = (IweaponIGC*)(pfsPlayer->GetIGCShip()->GetMountedPart(ET_Weapon, i));
                          if (pw && (pw->GetAmmoPerShot() != 0))
                              pw->SetMountedFraction(0.0f);
                      }
                  }
                  else
                  {
                      assert (packtype == c_packFuel);
                      pfsPlayer->GetIGCShip()->SetFuel(pfsPlayer->GetIGCShip()->GetFuel() + float(amount));

                      IpartIGC* pa = pfsPlayer->GetIGCShip()->GetMountedPart(ET_Afterburner, 0);
                      if (pa)
                          pa->SetMountedFraction(0.0f);
                  }
              }
              else
              {
                  if (!IlauncherIGC::IsLauncher(type))
                  {
					  debugf("(!IlauncherIGC::IsLauncher(type))\n"); //Imago 7/13/09
                      bBoot = true;
                      break;
                  }

                  IlauncherIGC* plauncher = (IlauncherIGC*)ppart;
                  IlauncherIGC* plauncherMounted = (IlauncherIGC*)(pfsPlayer->GetIGCShip()->GetMountedPart(plauncher->GetEquipmentType(), 0));

                  if (prlNext->amountTransfered == NA)
                  {
                      if ((plauncherMounted != NULL) && (plauncherMounted->GetAmount() != 0))
                      {
                          bBoot = true;
                          break;
                      }

                      if (plauncherMounted)
                          plauncherMounted->Terminate();

                      plauncher->SetMountID(0);
                  }
                  else
                  {
                      short amount = plauncher->GetAmount();

                      if ((!plauncherMounted) ||
                          (prlNext->amountTransfered <= 0) ||
                          (prlNext->amountTransfered > amount))
                      {
						  debugf("((!plauncherMounted) || (prlNext->amountTransfered <= 0) || (prlNext->amountTransfered > amount))\n");
                          bBoot = true;
                          break;
                      }

                      if (amount == prlNext->amountTransfered)
                          plauncher->Terminate();
                      else
                          plauncher->SetAmount(amount - prlNext->amountTransfered);

                      plauncherMounted->SetAmount(plauncherMounted->GetAmount() + prlNext->amountTransfered);
                      plauncherMounted->SetMountedFraction(0.0f);
                      plauncherMounted->ResetTimeLoaded();
                  }
              }

              prlNext++;
          }

          if (bBoot)
          {
              TrapHackBoot(!bBoot);
          }
          else
          {
              pfmReload->shipID = pfsPlayer->GetShipID();
              g.fm.ForwardMessage(GetGroupSectorFlying(pcluster), pfmReload, FM_GUARANTEED);
          }
      }
    }
    break;

    case FM_C_POSITIONREQ:
    {
      if (!pfsMission)
        break;

      CASTPFM(pfmPositionReq, C, POSITIONREQ, pfm);
      SideID iSide = pfmPositionReq->iSide;
      // Choose a side if NA was specified (Pigs-only)
      TrapHackBoot(iSide >= SIDE_TEAMLOBBY && iSide < c_cSidesMax);

      if (NA == iSide)
        iSide = pfsMission->PickNewSide(pfsPlayer, false, 0);

	  IsideIGC* psideReq = pMission->GetSide(iSide);

	  if (pMission->GetMissionParams()->bDrones == true && iSide == 0 && strcmp(pfsPlayer->GetCDKey(), g.szBotAuthenticationGuid) != 0)
	  {
		  //deny the request: Only bots are allowed on this team.
		  BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
			  END_PFM_CREATE
			  pfmDelPosReq->shipID = pfsPlayer->GetShipID();
		  pfmDelPosReq->iSide = iSide;

		  pfmDelPosReq->reason = DPR_BotsOnTeam0Only;

		  g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
		  break;
	  }

      
	  if (psideReq)
	  {
		  pfsMission->RequestPosition(pfsPlayer, psideReq, false);
	  }
      else
      {
        //deny the request: side no longer exists
        BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
        END_PFM_CREATE
        pfmDelPosReq->shipID     = pfsPlayer->GetShipID();
        pfmDelPosReq->iSide      = iSide;

        pfmDelPosReq->reason = DPR_SideGone;

        g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
        break;
      }

      break;
    }

    case FM_C_POSITIONACK:
    {
      if (!pfsMission)
        break;

      CASTPFM(pfmPositionAck, C, POSITIONACK, pfm);
      CFSShip * pfssAck = CFSShip::GetShipFromID(pfmPositionAck->shipID);
      IsideIGC * pside = pfsPlayer->GetSide(); // leader's side

      TrapHackBoot(pfmPositionAck->iSide >= 0 && pfmPositionAck->iSide < c_cSidesMax);
      // better be on the same mission, on the same side, and be the team leader
      // (but we can't trap hack it because they may have been booted or
      // demoted against their will)
      if (pfmPositionAck->iSide != pside->GetObjectID()
          || pfmPositionAck->iSide == SIDE_TEAMLOBBY
          || pfsPlayer != pfsMission->GetLeader(pfmPositionAck->iSide))
        break;

      if (!pfssAck || pfssAck->GetMission() != pfsMission) // player already went away, so do nothing
        break;

      CFSPlayer * pfspAck = pfssAck->GetPlayer();

      if (pfmPositionAck->fAccepted)
      {
        // if they still want the position...
        if (pfsMission->CheckPositionRequest(pfssAck->GetPlayer(), pside) == NA
            && pfsMission->RemoveJoinRequest(pfssAck->GetPlayer(), pside))
        {
          // put them on the side
          pfsMission->AddPlayerToSide(pfspAck, pside);
        }
      }
      else
      {
        pfsMission->RemoveJoinRequest(pfspAck, NULL);
      }
      break;
    }

    case FM_CS_QUIT_MISSION:
    {
      if (!pfsMission)
        break;

	  // BT - STEAM - Enable moderators to ban players by context menu.
	  bool isModerator = CSteamUserGroupStatus::IsMemberOfModeratorsGroup(pfsPlayer->GetCDKey()) || CSteamUserGroupStatus::IsMemberOfDevelopersGroup(pfsPlayer->GetCDKey());


      IsideIGC* pside = pfsPlayer->GetSide(); // player's side
      assert (pside);
      SideID    sideID = pside->GetObjectID();
      if (sideID < 0 && sideID != SIDE_TEAMLOBBY && isModerator == false)
          break;

      CASTPFM(pfmQuitMission, CS, QUIT_MISSION, pfm);

      CFSShip * pfssAck = CFSShip::GetShipFromID(pfmQuitMission->shipID);
	  CFSPlayer * pfspAck = pfssAck->GetPlayer();

      if (!pfssAck) // player already went away, so do nothing
        break;
	  
	  // BT - STEAM - Enable moderators to ban players by context menu.
	  if (isModerator == true && pfspAck->IsPlayer())
	  {
		  char szMessageParam[1024];
		  if (FM_VAR_REF(pfmQuitMission, szMessageParam))
			  lstrcpy(szMessageParam, FM_VAR_REF(pfmQuitMission, szMessageParam));

		  pfsMission->RemovePlayerFromMission(pfspAck, pfmQuitMission->reason, szMessageParam);
		  break;
	  }

      // the player must be the team leader or be booting themselves
      // (but the client might mistakenly think it's the team leader)
      if ((pfsPlayer == pfssAck) ||
          ((pfsPlayer == pfsMission->GetLeader(sideID)) &&
           (pfssAck->GetSide() == pside)))
      {
        
        bool bIsBoot = (pfsPlayer->GetShipID() != pfmQuitMission->shipID);
		bool bIsAdmin = (pfsPlayer->GetShipID() != pfmQuitMission->shipID) && (pfmQuitMission->reason & QSR_SwitchingSides == QSR_AdminBooted);

        // make sure that they are requesting a reasonable reason.
        TrapHackBoot(pfmQuitMission->reason == (bIsBoot ? QSR_LeaderBooted : QSR_Quit));

        // send them to the game lobby
		if (!bIsBoot || !pfsMission->GetMissionDef()->misparms.bLockTeamSettings)
		{
		    // TE:  Fire AGCEvent informing of the boot
			 if (bIsBoot)
			 {
				LPCSTR pszContext = pfsMission->GetIGCMission() ? pfsMission->GetIGCMission()->GetContextName() : NULL;
				_AGCModule.TriggerContextEvent(NULL, EventID_LeaveTeam, pszContext,
					pfssAck->GetName(), pfssAck->GetShipID(), pside->GetObjectID(), pfsPlayer->GetCharacterID(), 4,
					"MissionID", VT_I4, pfsMission->GetMissionID(),
					"Team"    , VT_I4   , pside->GetObjectID(),
					"TeamName", VT_LPSTR, pside->GetName(),
					"zBooter", VT_LPSTR, pfsPlayer->GetName());
			 }
			// TE
			pfsMission->RemovePlayerFromMission(pfspAck, pfmQuitMission->reason);
		}
      }

      break;
    }

    case FM_CS_QUIT_SIDE:
    {
      if (!pfsMission)
        break;

	  // BT - STEAM - Enable moderators to ban players by context menu.
	  bool isModerator = CSteamUserGroupStatus::IsMemberOfModeratorsGroup(pfsPlayer->GetCDKey()) || CSteamUserGroupStatus::IsMemberOfDevelopersGroup(pfsPlayer->GetCDKey());

      IsideIGC* pside = pfsPlayer->GetSide(); // player's side
      assert (pside);
      SideID    sideID = pside->GetObjectID();
      if (sideID < 0 && isModerator == false)
        break;

      CASTPFM(pfmQuitSide, CS, QUIT_SIDE, pfm);

      CFSShip * pfssAck = CFSShip::GetShipFromID(pfmQuitSide->shipID);
	  
      if (!pfssAck) // player already went away, so do nothing
        break;

	  CFSPlayer * pfspAck = pfssAck->GetPlayer();

	  if (isModerator == true && pfssAck->IsPlayer())
	  {
		  char szMessageParam[1024];
		  if (FM_VAR_REF(pfmQuitSide, szMessageParam))
			  lstrcpy(szMessageParam, FM_VAR_REF(pfmQuitSide, szMessageParam));

		  pfsMission->RemovePlayerFromSide(pfspAck, pfmQuitSide->reason, szMessageParam);
		  break;
	  }

      TrapHackBoot(pfssAck->IsPlayer());

      // the player must be the team leader or be booting themselves
      // (but the client might mistakenly think it's the team leader)
      if ((pfsPlayer == pfssAck) ||
          ((pfsPlayer == pfsMission->GetLeader(sideID)) &&
           (pfssAck->GetSide() == pside)))
      {
        
        bool bIsBoot = (pfsPlayer->GetShipID() != pfmQuitSide->shipID);

        // make sure that they are requesting a reasonable reason.
        TrapHackBoot(pfmQuitSide->reason == (bIsBoot ? QSR_LeaderBooted : QSR_Quit));

		// TE: Commented this block since locked sides now handle balancing
		// mmf MaxBalance is now used instead of lock sides so put this back, not that lock sides is that useful
		// #12 Unable to leave a locked team (to NOAT) Imago 6/10 --^ lol
        if (!bIsBoot && pfsMission->GetMissionDef()->misparms.bLockSides && sideID == SIDE_TEAMLOBBY)
        {
          // turn it down because the sides are locked
          BEGIN_PFM_CREATE(g.fm, pfmDelPosReq, CS, DELPOSITIONREQ)
          END_PFM_CREATE
          pfmDelPosReq->shipID    = pfsPlayer->GetShipID();
          pfmDelPosReq->iSide     = sideID;
          pfmDelPosReq->reason    = DPR_SidesLocked;

          g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
        }
        else
        {
          // send them to the game lobby
          if (!bIsBoot || !pfsMission->GetMissionDef()->misparms.bLockTeamSettings)
          {
            pfsMission->RemovePlayerFromSide(pfspAck, pfmQuitSide->reason);

            if (bIsBoot)
            {
              // if they are banned from all sides kick them out of the game
              unsigned char bannedSideMask = pfspAck->GetBannedSideMask();
              unsigned char   legalSideMask = SideMask(pfsMission->GetCountSides()) - 1;
              if ((bannedSideMask & legalSideMask) == legalSideMask)
                pfsMission->RemovePlayerFromMission(pfspAck, pfmQuitSide->reason);
            }
          }
        } // TE: matches above comment mmf put this back
      }

      break;
    }

    case FM_CS_AUTO_ACCEPT:
    {
      if (!pfsMission)
        break;

      CASTPFM(pfmAutoAccept, CS, AUTO_ACCEPT, pfm);

      SideID    sid = pfsPlayer->GetSide()->GetObjectID();

      if (sid == SIDE_TEAMLOBBY)
        break;

      TrapHackBoot(sid >= 0 && sid < c_cSidesMax);

      // can't be sure that they were not booted or something similar
      if (pfsPlayer != pfsMission->GetLeader(sid))
        break;

      TrapHackBoot(pfmAutoAccept->iSide == sid);

      // don't accept this message if the server is locked open
      if (pfsMission->GetMissionDef()->misparms.bLockGameOpen)
        break;

      g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfmAutoAccept, FM_GUARANTEED);
      IsideIGC * pside = pMission->GetSide(pfmAutoAccept->iSide);
      pfsMission->SetAutoAccept(pside, pfmAutoAccept->fAutoAccept);
      break;
    }

    case FM_CS_LOCK_LOBBY:
    {
      if ((!pfsMission) || (pfsMission->GetOwner() != pfsPlayer))
        break;

      // don't accept this message if the server is locked open
      if (pfsMission->GetMissionDef()->misparms.bLockGameOpen)
        break;

      CASTPFM(pfmLockLobby, CS, LOCK_LOBBY, pfm);
      g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfmLockLobby, FM_GUARANTEED);
      pfsMission->SetLockLobby(pfmLockLobby->fLock);
      break;
    }

    case FM_CS_LOCK_SIDES:
    {
      if ((!pfsMission) || (pfsMission->GetOwner() != pfsPlayer))
        break;

      CASTPFM(pfmLockSides, CS, LOCK_SIDES, pfm);
      g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfmLockSides, FM_GUARANTEED);
      pfsMission->SetLockSides(pfmLockSides->fLock);
      break;
    }

    case FM_C_RANDOMIZE_TEAMS:
    {
      if ((!pfsMission) || (pfsMission->GetOwner() != pfsPlayer))
        break;

      if (pfsMission->GetStage() != STAGE_NOTSTARTED)
        break;

      // TE: Balance or Flush depending on the LockSides parameter mmf MaxBalance
	  if (pfsMission->GetMissionDef()->misparms.iMaxImbalance == 0x7ffe)
	  {
		// Balance the teams
		pfsMission->RandomizeSides();
	  }
	  else
	  {
		// Flush the teams
		pfsMission->FlushSides();
	  }

      break;
    }

    case FM_CS_PLAYER_READY:
    case FM_CS_FORCE_TEAM_READY:
    {
      if (!pfsMission)
        break;

      bool fIsForce    = FM_CS_FORCE_TEAM_READY == pfm->fmid;
      IsideIGC * pside = pfsPlayer->GetSide();
      int  iSide       = pside->GetObjectID();
      bool fReady      = false;
      bool fForceReady = false;
      FEDMESSAGE * pfmForward = NULL;

      if (fIsForce)
      {
        if (pfsPlayer != pfsMission->GetLeader(iSide))
          break;

        CASTPFM(pfmForceReady, CS, FORCE_TEAM_READY, pfm);
        TrapHackBoot(iSide >= 0 && iSide < c_cSidesMax);
        TrapHackBoot(pfmForceReady->iSide == iSide);

        // tell everyone on the team about the ready status change
        // we should do this b4 continuing, since the game might start, and they should get the ready change 1st
        g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfm, FM_GUARANTEED);

        pfsMission->SetForceReady(iSide, pfmForceReady->fForceReady);
        pfmForward = (FEDMESSAGE *) pfmForceReady;
      }
      else // regular player ready
      {
        CASTPFM(pfmPlayerReady, CS, PLAYER_READY, pfm);
        TrapHackBoot(pfmPlayerReady->shipID == pfsPlayer->GetShipID());

        fReady = pfmPlayerReady->fReady;
        pfsPlayer->SetReady(pfmPlayerReady->fReady);
        pfmForward = (FEDMESSAGE *) pfmPlayerReady;

        g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfm, FM_GUARANTEED);
      }

      break;
    }

    case FM_C_DOCKED:
    {
        if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
            break;

        IsideIGC*   pside = pfsPlayer->GetSide();
        if (pside->GetObjectID() < 0)
            break;

        if (pfsPlayer->GetIGCShip()->GetStation() != NULL)
        {
            CASTPFM(pfmDocked, C, DOCKED, pfm);

            IshipIGC*           pshipParent = pfsPlayer->GetIGCShip()->GetParentShip();
            if (pshipParent == NULL)
            {
                const ShipListIGC*  pshipsChildren = pfsPlayer->GetIGCShip()->GetChildShips();

                IstationIGC*    pstation;
                if ((pfmDocked->stationID == NA) && !pfsPlayer->GetIGCShip()->IsGhost())
                {
                    pstation = NULL;
                }
                else
                {
                    pstation = pfsMission->GetIGCMission()->GetStation(pfmDocked->stationID);

                    BEGIN_PFM_CREATE(g.fm, pfmAck, S, TELEPORT_ACK)
                    END_PFM_CREATE



                    if ((pstation == NULL) ||
                        ( (pstation->GetSide() != pside && !pside->AlliedSides(pside,pstation->GetSide()))) || //#ALLY REARM 7/8/09 7/9/09
                        (!pstation->GetStationType()->HasCapability(c_sabmRestart)))
                    {
                        //Not a legitimate station to start at ... ignore the request
                        pfmAck->stationID = NA;
                        pfmAck->bNewHull = false;

                        g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
                        break;
                    }

                    pfmAck->stationID = pfmDocked->stationID;
                    pfmAck->bNewHull = !pstation->CanBuy(pfsPlayer->GetIGCShip()->GetBaseHullType());
                    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

                    if (pfmAck->bNewHull)
                    {
                        //Switch to a lifepod
                        {
                            //Boot any kids
                            ShipLinkIGC*    psl;
                            while (psl = pshipsChildren->first()) //intentional
                                LeaveShip((CFSShip*)(psl->data()->GetPrivateData()), pfsPlayer->GetIGCShip());
                        }

                        //Credit the player with the appropriate money
                        Money refund = pfsPlayer->GetIGCShip()->GetValue();
                        assert(refund >= 0);

                        if (refund > 0)
                        {
                            BEGIN_PFM_CREATE(g.fm, pfmMoney, S, MONEY_CHANGE)
                            END_PFM_CREATE

                            pfmMoney->sidTo = pfsPlayer->GetShipID();
                            pfmMoney->sidFrom = NA;
                            pfmMoney->dMoney = refund;

                            pfsPlayer->SetMoney(pfsPlayer->GetMoney() + refund);

                            g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
                        }

                        //Sell off any parts
                        {
                            const PartListIGC*  ppartsList = pfsPlayer->GetIGCShip()->GetParts();
                            PartLinkIGC*        ppl;
                            while (ppl = ppartsList->first())       //Intentional
                                ppl->data()->Terminate();
                        }

                        //Switch to a lifepod
                        IhullTypeIGC*   pht = pside->GetCivilization()->GetLifepod();
                        pfsPlayer->GetIGCShip()->SetBaseHullType(pht);

                        ShipStatus* pss = pfsPlayer->GetShipStatus(pside->GetObjectID());
                        pss->SetHullID(pht->GetObjectID());
                    }
                }

                //Move the children of this ship to the station as well
                for (ShipLinkIGC*   psl = pshipsChildren->first();
                     (psl != NULL);
                     psl = psl->next())
                {
                    psl->data()->SetStation(pstation);
                }
                pfsPlayer->GetIGCShip()->SetStation(pstation);
            }
        }
    }
    break;

    case FM_C_BUCKET_DONATE:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
        break;

      CASTPFM(pfmBucketDonate, C, BUCKET_DONATE, pfm);

      IsideIGC*   pside = pfsPlayer->GetSide();
      IbucketIGC* pbucket = pside->GetBucket(pfmBucketDonate->iBucket);
      TrapHackBoot(pbucket);
      TrapHackBoot(pbucket->GetGroupID() >= 0);
      TrapHackBoot(pfsPlayer->GetMoney() >= pfmBucketDonate->moneyGiven);
      TrapHackBoot(pfmBucketDonate->moneyGiven >= 0);

      Money mnyRefund = pfmBucketDonate->moneyGiven;
      if (pside->CanBuy(pbucket))
      {
          Money mnySpent = pbucket->AddMoney(pfmBucketDonate->moneyGiven);

          assert(mnySpent <= mnyRefund); // ACTUAL $ spent must be no more than ATTEMPTED $ spent

          mnyRefund -= mnySpent;

          pfsPlayer->SetMoney(pfsPlayer->GetMoney() - mnySpent);

          BEGIN_PFM_CREATE(g.fm, pfmMoneyChange, S, MONEY_CHANGE)
          END_PFM_CREATE
          pfmMoneyChange->dMoney = -mnySpent;
          pfmMoneyChange->sidTo = pfsPlayer->GetShipID();
          pfmMoneyChange->sidFrom = pfmMoneyChange->sidTo;

          g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);

          if (pfsPlayer->GetMoney() < 0)
          {
            LPCSTR pszContext = pfsPlayer->GetIGCShip() ?
              pfsPlayer->GetIGCShip()->GetMission()->GetContextName() : NULL;

            _AGCModule.TriggerContextEvent(NULL, AllsrvEventID_MoneyError,
              pszContext, pfsPlayer->GetName(), -1, -1, -1, 3,
                "PlayerName", VT_LPSTR, pfsPlayer->GetName(),
                "MoneyGiven", VT_I4, pfmBucketDonate->moneyGiven,
                "MoneyLeft", VT_I4, pfsPlayer->GetMoney());
          }

          SideID sid = pside->GetObjectID();
          BEGIN_PFM_CREATE(g.fm, pfmBucketStatus, S, BUCKET_STATUS)
          END_PFM_CREATE
          pfmBucketStatus->timeTotal  = pbucket->GetTime();
          pfmBucketStatus->moneyTotal = pbucket->GetMoney();
          pfmBucketStatus->iBucket    = pfmBucketDonate->iBucket;
          pfmBucketStatus->sideID     = sid;

          //Everyone knowns about anyone's development of the game winning tech
          IbuyableIGC*  b = pbucket->GetBuyable();

          CFMGroup * pgrp = ((b->GetObjectType() == OT_development) && (b->GetObjectID() == c_didTeamMoney)) ?
                             pfsMission->GetGroupMission() :
                             CFSSide::FromIGC(pside)->GetGroup();
          g.fm.SendMessages(pgrp, FM_GUARANTEED, FM_FLUSH);
      }

      if (mnyRefund)
      {
        assert (mnyRefund > 0);
        BEGIN_PFM_CREATE(g.fm, pfmMoneyChange, S, MONEY_CHANGE)
        END_PFM_CREATE
        pfmMoneyChange->dMoney = mnyRefund;
        pfmMoneyChange->sidTo = pfsPlayer->GetShipID();
        pfmMoneyChange->sidFrom = NA;
        g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
      }

      break;
    }

    case FM_C_PLAYER_DONATE:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
        break;

      CASTPFM(pfmPlayerDonate, C, PLAYER_DONATE, pfm);

      TrapHackBoot(pfmPlayerDonate->moneyGiven > 0);
      TrapHackBoot(pfsPlayer->GetMoney() >= pfmPlayerDonate->moneyGiven);

      CFSShip * pfssTo = CFSShip::GetShipFromID(pfmPlayerDonate->shipID);
      CFMRecipient * prcp = NULL;
      if (pfmPlayerDonate->moneyGiven > 0)
      {
          BEGIN_PFM_CREATE(g.fm, pfmMoneyChange, S, MONEY_CHANGE)
          END_PFM_CREATE
          pfmMoneyChange->dMoney  = pfmPlayerDonate->moneyGiven;

		  bool bAllied = IsideIGC::AlliedSides(pfsPlayer->GetSide(),pfssTo->GetSide());
          if (pfssTo && ( pfsPlayer->GetSide() == pfssTo->GetSide()
			  || bAllied ) // Imago ALLY 7/6/09
			  && (pfsPlayer->GetMoney() >= pfmPlayerDonate->moneyGiven))
          {
            pfmMoneyChange->sidTo   = pfmPlayerDonate->shipID;
            pfmMoneyChange->sidFrom = pfsPlayer->GetShipID();

            pfsPlayer->SetMoney(pfsPlayer->GetMoney() - pfmMoneyChange->dMoney);
            pfssTo->SetMoney(pfssTo->GetMoney() + pfmMoneyChange->dMoney);

			if (bAllied) // Imago ALLY 7/6/09
				g.fm.SendMessages(CFSSide::FromIGC(pfssTo->GetSide())->GetGroup(), FM_GUARANTEED, FM_FLUSH);

			prcp = CFSSide::FromIGC(pfsPlayer->GetSide())->GetGroup();
          }
          else
          {
            debugf("%s tried to donate money to ship %d, which doesn't exist in mission.\n",
                pfsPlayer->GetName(), pfmPlayerDonate->shipID);

            assert (pfmPlayerDonate->moneyGiven > 0);
            pfmMoneyChange->sidTo = pfsPlayer->GetShipID();
            pfmMoneyChange->sidFrom = NA;
            prcp = pfsPlayer->GetConnection();
          }
          g.fm.SendMessages(prcp, FM_GUARANTEED, FM_FLUSH);


      }
    }
    break;

    case FM_CS_ORDER_CHANGE:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED))
        break;

      CASTPFM(pfmOC, CS, ORDER_CHANGE, pfm);

      IshipIGC*   pship = pfsPlayer->GetIGCShip();
      assert (pship);

	  IsideIGC*   pside = pship->GetSide();
	  if (pside->GetShip(pfmOC->shipID))  //Xynth #48 8/2010 This msg handler is normally used for 
	  {									 //ownship commands.  Tagging on miner commands from playerpane
      	  pship = pside->GetShip(pfmOC->shipID);		 
	  }

	  ImodelIGC*  pmodel;

	  //Spunky #250 #257
	  bool DoNotExecute = false;
	  // pkk #211 08/05/2010 Allow all drones stay docked
      if (pship->GetPilotType() < c_ptPlayer)
	  {
			DoNotExecute = true; //don't carry out the sent command later, since we are handling it here
			if (pship->GetStation() != NULL) //if docked..	
			{ 
				if (pfmOC->commandID==c_cidDoNothing) //stay docked
				{
					pship->SetStayDocked(true);
					pfsMission->GetSite()->SendChat(pship, CHAT_INDIVIDUAL, pfsPlayer->GetIGCShip()->GetObjectID(),
                                            voAffirmativeSound, "Affirmative.");
				}
			    else //launch
				{
					pship->SetStayDocked(false);
					pfsMission->GetSite()->SendChat(pship, CHAT_INDIVIDUAL, pfsPlayer->GetIGCShip()->GetObjectID(),
                                            voAffirmativeSound, "Affirmative. Preparing to launch.");
					pship->PickDefaultOrder(pship->GetStation()->GetCluster(), pship->GetStation()->GetPosition(), true);			  
				}
			}
			else //we are going back to base
			{
				//we find the nearest station server side because the ship's exact location is only available here when we are in a different sector
				if (pship->GetPilotType() != c_ptCarrier)
				{
					pmodel = FindTarget(pship, c_ttFriendly | c_ttStation | c_ttNearest| c_ttAnyCluster,
						NULL, NULL, NULL, NULL, c_sabmRepair);
				}
				else
				{
					// Find shipyard
					pmodel = FindTarget(pship, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
					NULL, NULL,NULL, NULL, c_sabmCapLand);
					if (pmodel == NULL) //No shipyard, carrier flees to nearest base for protection
						pmodel = FindTarget(pship, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
								NULL, NULL, NULL, NULL, c_sabmRepair);
				}
				pship->SetStayDocked(true);
                pship->SetCommand(c_cmdQueued, NULL, c_cidNone); //Don't remember any sector
				if (pmodel)
				{
					pfsMission->GetSite()->SendChat(pship, CHAT_INDIVIDUAL, pfsPlayer->GetIGCShip()->GetObjectID(),
                                            voAffirmativeSound, "Affirmative.");
											pfsMission->GetSite()->SendChat(pship, CHAT_TEAM, pship->GetSide()->GetObjectID(),
                                            NA, "Heading back to base. I'll sit tight.");
		
					pship->SetCommand(pfmOC->command, pmodel, pfmOC->commandID);
				}
			}
	  }//Xynth #48 Spunky #250 #257 End Changes

      if (pside->GetObjectID() >= 0  && !DoNotExecute)
      {
		pmodel = pMission->GetModel(pfmOC->objectType, pfmOC->objectID);
		pship->SetCommand(pfmOC->command, pmodel, pfmOC->commandID);
      }
	  
    }
    break;

    case FM_CS_CHANGE_TEAM_CIV:
    {
      if (!pfsMission)
        break;

      IsideIGC * pside = pfsPlayer->GetSide();
      SideID sid = pside->GetObjectID();

      if (sid == SIDE_TEAMLOBBY || pfsPlayer != pfsMission->GetLeader(sid))
        break;

      CASTPFM(pfmChangeTeamCiv, CS, CHANGE_TEAM_CIV, pfm);
      TrapHackBoot(NA != pfmChangeTeamCiv->civID);
      //TrapHackBoot(sid == pfmChangeTeamCiv->iSide);  Xynth Had to remove this for random civs #170

      if (pfsMission->GetStage() == STAGE_NOTSTARTED)
      {
        IcivilizationIGC * pciv = pMission->GetCivilization(pfmChangeTeamCiv->civID);
        TrapHackBoot(pciv);
        g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfmChangeTeamCiv, FM_GUARANTEED);
        pfsMission->SetSideCiv(pside, pciv);
      }
    }
    break;

	//Xynth #208 8/2010
	case FM_CS_HIGHLIGHT_CLUSTER:
	{
		CASTPFM(pfmHC, CS, HIGHLIGHT_CLUSTER, pfm);
		
		g.fm.ForwardMessage(CFSSide::FromIGC(pfsPlayer->GetSide())->GetGroup(),pfmHC, FM_GUARANTEED);
	}
	break;

	// KGJV #62
    case FM_CS_SIDE_INACTIVE:
    {
		TrapHackBoot(pfsMission);
		TrapHackBoot(pfsMission->GetOwner() == pfsPlayer);
		TrapHackBoot(pfsMission->GetStage() == STAGE_NOTSTARTED);
		TrapHackBoot(!pfsMission->GetMissionDef()->misparms.bObjectModelCreated);

		CASTPFM(pfmSideInactive, CS, SIDE_INACTIVE, pfm);
  		SideID sid = pfmSideInactive->sideID;

		//TrapHackBoot(!pfsMission->HasPlayers(pfsMission->GetIGCMission()->GetSide(sid),true));
		// only change activeflag if team is empty
		if (!pfsMission->HasPlayers(pfsMission->GetIGCMission()->GetSide(sid),true))
			pfsMission->SetSideActive(sid,pfmSideInactive->bActive); // update cached missdef
		else // otherwise revert
			pfmSideInactive->bActive = pfsMission->GetSideActive(sid);

		// here we update the AllowEmptyTeams (AET) game parameter if needed
		// 1.  get current AET
		bool bAET = pfsMission->GetAllowEmptyTeams();

		// 2. check if all sides are active
		bool bAllActive = true;
		for (SideID i = 0; i < pfsMission->GetCountSides() ; i++) // or pfsMission->GetIGCMission()->GetSides()->n()
		{
			if (!pfsMission->GetSideActive(i))
				bAllActive = false;
		}
		// 3. do we need to change AET ?
		bool bMustChangeAET = false;
		if (bAllActive && bAET) bMustChangeAET = true; // all active and AET is on, change it
		if (!bAllActive && !bAET) bMustChangeAET = true; // not all active and AET is off, change it

		// 4.  if needed, change AET and inform clients
		if (bMustChangeAET)
		{
			pfsMission->SetAllowEmptyTeams(!bAET); // toggle AET
			pfmSideInactive->bChangeAET = true;
			pfmSideInactive->bAET = pfsMission->GetAllowEmptyTeams();
		}
		else
			pfmSideInactive->bChangeAET = false;

		// 4. broadcast both the team activeF and the AET changes
        g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfmSideInactive, FM_GUARANTEED);//broadcast the change

		// #ALLY - also update alliances
		pfsMission->UpdateAlliances(sid,NA);
	}
	break;

    case FM_CS_SET_TEAM_INFO:
    {
      if (!pfsMission)
        break;

      if (pfsMission->GetStage() == STAGE_NOTSTARTED
            && !pfsMission->GetMissionDef()->misparms.bLockTeamSettings)
      {
        CASTPFM(pfmSetTeamInfo, CS, SET_TEAM_INFO, pfm);
        TrapHackBoot(pfmSetTeamInfo->sideID >= 0);
        TrapHackBoot(pfmSetTeamInfo->sideID < c_cSidesMax);
        TrapHackBoot(NULL != memchr(pfmSetTeamInfo->SideName, 0, c_cbName));
        TrapHackBoot(pfsPlayer->GetIsMemberOfSquad(pfmSetTeamInfo->squadID));

        // make sure we haven't run into any strange race conditions with team
        // leadership.
        if (pfmSetTeamInfo->sideID >= pfsMission->GetCountSides()
            || pfsMission->GetLeader(pfmSetTeamInfo->sideID) != pfsPlayer
            || pfsPlayer->GetSide()->GetObjectID() != pfmSetTeamInfo->sideID)
          break;

        if (pfsMission->GetMissionDef()->misparms.bSquadGame)
        {
          TrapHackBoot(pfsPlayer->GetCanLeadSquad(pfmSetTeamInfo->squadID)
            || pfsPlayer->GetSide()->GetSquadID() == pfmSetTeamInfo->squadID);

          if (pfmSetTeamInfo->squadID != NA)
            pfsMission->SetSideSquad(pfmSetTeamInfo->sideID, pfmSetTeamInfo->squadID);
        }
        else
        {
          if (pfmSetTeamInfo->squadID == NA)
            pfsMission->SetSideName(pfmSetTeamInfo->sideID, pfmSetTeamInfo->SideName);
        }
      }
    }
    break;

	// #ALLY
	case FM_C_CHANGE_ALLIANCE:
	{
      if (!pfsMission)
        break;

      if (pfsMission->GetStage() == STAGE_NOTSTARTED) // for now, we can't change alliances when game is running
	  {
		  CASTPFM(pfmAlliancesChange, C, CHANGE_ALLIANCE, pfm);

		  pfsMission->UpdateAlliances(pfmAlliancesChange->sideID,pfmAlliancesChange->sideAlly);
	  }
	}
	break;
	// #ALLY end

    case FM_CS_DELPOSITIONREQ:
    {
      if (!pfsMission)
        break;

      CASTPFM(pfmDelPositionReq, CS, DELPOSITIONREQ, pfm);

      ShipID shipID = pfsPlayer->GetShipID();

      if (pfmDelPositionReq->shipID == shipID)
      {
        TrapHackBoot(pfmDelPositionReq->reason == DPR_Canceled);
      }
      else
      {
        TrapHackBoot(pfmDelPositionReq->reason == DPR_Rejected);
      }

      TrapHackBoot(pfmDelPositionReq->iSide >= 0 && pfmDelPositionReq->iSide < c_cSidesMax);
      if (pfmDelPositionReq->iSide >= pfsMission->GetCountSides())
        break;

      if (pfmDelPositionReq->shipID != shipID
          && pfsPlayer != pfsMission->GetLeader(pfmDelPositionReq->iSide))
        break;

      // tell everyone on the team about the change
      g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfm, FM_GUARANTEED);

      // remove the request in question
      pfsMission->RemoveJoinRequest(pfsPlayer, pfsMission->GetIGCMission()->GetSide(SIDE_TEAMLOBBY));
    }
    break;

    case FM_CS_MISSIONPARAMS:
    {
      if (!pfsMission)
        break;

      CASTPFM(pfmMissionParams, CS, MISSIONPARAMS, pfm);

      // the player better be the mission owner for this mission
      if (pfsMission->GetOwner() != pfsPlayer)
        break;

      TrapHackBoot(pfmMissionParams->missionparams.Invalid(true) == NULL);
      pfmMissionParams->missionparams.bObjectModelCreated =
        !!pfsMission->GetMissionDef()->misparms.bObjectModelCreated;

      #if !defined(ALLSRV_STANDALONE)
        TrapHackBoot(pfmMissionParams->missionparams.bClubGame == true);
      #else // !defined(ALLSRV_STANDALONE)
        TrapHackBoot(pfmMissionParams->missionparams.bClubGame == false);
      #endif // !defined(ALLSRV_STANDALONE)

      // players can't change this stuff
      TrapHackBoot(pfmMissionParams->missionparams.bLockGameOpen
        == pfsMission->GetMissionDef()->misparms.bLockGameOpen);
      TrapHackBoot(pfmMissionParams->missionparams.nTotalMaxPlayersPerGame
        == pfsMission->GetMissionDef()->misparms.nTotalMaxPlayersPerGame);

	  //Imago 7/10 - only for default games
      if (pfsMission->GetMissionDef()->misparms.bLockGameOpen && !pfmMissionParams->missionparams.bObjectModelCreated)
      {
        TrapHackBoot(
          pfmMissionParams->missionparams.nMaxPlayersPerTeam
          == pfsMission->GetMissionDef()->misparms.nMaxPlayersPerTeam
          || pfmMissionParams->missionparams.nMaxPlayersPerTeam
            >= pfmMissionParams->missionparams.nTotalMaxPlayersPerGame
              / pfmMissionParams->missionparams.nTeams);
      }

      // if the mission has not started yet, change the settings
      if (pfsMission->GetStage() == STAGE_NOTSTARTED)
      {
        #if defined(ALLSRV_STANDALONE)
          // for a stand alone server, don't let the client try to change the password
          // (we can't assert this due to a race condition)
		  // KGJV #114 - only for bObjectModelCreated games (=DefaultGames)
		  if (pfmMissionParams->missionparams.bObjectModelCreated)
		  {
          strcpy(pfmMissionParams->missionparams.strGamePassword,
              pfsMission->GetMissionDef()->misparms.strGamePassword);
          strcpy(pfmMissionParams->missionparams.strGameName,
              pfsMission->GetMissionDef()->misparms.strGameName);
		  }
        #endif // defined(ALLSRV_STANDALONE)

        // change the settings
        pfsMission->SetMissionParams(pfmMissionParams->missionparams);

        // tell the people in the game about the new settings...
        g.fm.ForwardMessage(pfsMission->GetGroupMission(), pfsMission->GetMissionDef(), FM_GUARANTEED);

        // and give the lobby a new abstract of the same
        pfsMission->SetLobbyIsDirty();


        LPCSTR pszContext = pfsMission->GetIGCMission()->GetContextName();

        // Send an AGC event
        _AGCModule.TriggerContextEvent(NULL, EventID_GameStateChange, pszContext,
          pfsMission->GetMissionDef()->misparms.strGameName,
          pfsMission->GetMissionID(), -1, -1, 0);
      }
      else
      {
        // otherwise silently ignore the change.
        // REVIEW: do we need to try to make this race condition more visible?
      }
    }
    break;

    case FM_C_START_GAME:
    {
      // the player better be the mission owner for this mission
      TrapHackBoot(pfsMission);

	  // Let bots start the game even if they are not the mission owner.
	  if (strcmp(pfsPlayer->GetCDKey(), g.szBotAuthenticationGuid) != 0)
		TrapHackBoot(pfsMission->GetOwner() == pfsPlayer);

      // if the mission has not started yet but is ready to start, start the mission
      if (pfsMission->GetStage() == STAGE_NOTSTARTED)
      {
		  // If all sides are ready, or if the game starter is a bot, then start the game. 
        if (pfsMission->FAllReady() || strcmp(pfsPlayer->GetCDKey(), g.szBotAuthenticationGuid) == 0) 
        {
            // start the countdown
            pfsMission->StartCountdown(c_fMissionBriefingCountdown);
        }
      }
    }
    break;

    case FM_CS_SET_TEAM_LEADER:
    {
      if (!pfsMission)
        break;

      CASTPFM(pfmSetTeamLeader, CS, SET_TEAM_LEADER, pfm);

      // the sender better be the current team leader
      IsideIGC* psidePlayer = pfsPlayer->GetSide();
      SideID sid = psidePlayer->GetObjectID();
      TrapHackBoot(sid >= 0 && sid < c_cSidesMax);

      if (pfsPlayer == pfsMission->GetLeader(sid))
      {
          TrapHackBoot(sid == pfmSetTeamLeader->sideID);

          CFSShip*    pfssNewLeader = CFSShip::GetShipFromID(pfmSetTeamLeader->shipID);
          if (pfssNewLeader && pfssNewLeader->IsPlayer()
              && pfssNewLeader->GetSide() == psidePlayer)
          {
              CFSPlayer * pfspNewLeader = pfssNewLeader->GetPlayer();

              // if there is a human player still on their team with the given ship ID,
              // make that new player the leader.
              if (pfspNewLeader->GetSide() == psidePlayer)
                  pfsMission->SetLeader(pfspNewLeader);
          }
      }
    }
    break;

    case FM_C_RIPCORD_REQUEST:
    {
      if (!pfsMission || (pfsMission->GetStage() != STAGE_STARTED) || (pfsPlayer->GetIGCShip()->GetSide()->GetObjectID() < 0) ||
          pfsPlayer->GetIGCShip()->GetParentShip() ||
          (pfsPlayer->GetIGCShip()->GetCluster() == NULL) ||
          (pfsPlayer->GetIGCShip()->GetFlag() != NA) ||
          pfsPlayer->GetIGCShip()->GetBaseHullType()->HasCapability(c_habmNoRipcord))
      {
        break;
      }

      CASTPFM(pfmRipcord, C, RIPCORD_REQUEST, pfm);
      RequestRipcord(pfsPlayer->GetIGCShip(), pfsMission->GetIGCMission()->GetCluster(pfmRipcord->sidRipcord));
    }
    break;

	/* pkk May 6th: Disabled bandwidth patch
	// w0dk4 Bandwidth Patch
	case FM_C_BANDWIDTH:
    {
		CASTPFM(pfmBandwidth, C, BANDWIDTH, pfm);
		pfsPlayer->SetBandwidth(pfmBandwidth->value);

	}
	break;*/

	// w0dk4 player-pings feature
	// send ping data
	case FM_C_REQPINGDATA:
    {
		CFMConnection * cnxn = pfsPlayer->GetConnection();
		if(cnxn->PingQuery()) { // check if last ping request longer than 5 seconds ago

			// iterate through all connections and if player data available, go for it
			ListConnections::Iterator iterCnxn(*g.fm.GetConnections());
			while (!iterCnxn.End())
			{
			  CFMConnection & cnxn = *iterCnxn.Value();
			  iterCnxn.Next();

			  CFSPlayer * pfsTempPlayer = CFSPlayer::GetPlayerFromConnection(cnxn);

			  if(pfsTempPlayer) {

				  // now get all the data and queue message

				  BEGIN_PFM_CREATE(g.fm, pfmPingData, S, PINGDATA)
				  END_PFM_CREATE
				  pfmPingData->shipID = pfsTempPlayer->GetShipID();

				  DPN_CONNECTION_INFO Connection_Info;
				  Connection_Info.dwSize = sizeof(DPN_CONNECTION_INFO);
				  if(g.fm.GetConnectionInfo(&cnxn, Connection_Info) == S_OK) {
					pfmPingData->ping = Connection_Info.dwRoundTripLatencyMS;
					// calculate packet loss
					DWORD m_lastLost;
					DWORD m_lastPackets;
					cnxn.GetPacketLossData(&m_lastPackets,&m_lastLost);
					DWORD packets_now = Connection_Info.dwPacketsSentGuaranteed + Connection_Info.dwPacketsSentNonGuaranteed;
					DWORD lost_now = Connection_Info.dwPacketsRetried + Connection_Info.dwPacketsDropped;
					float fLossPercentage;
					if ((packets_now-m_lastPackets) > 0) // check for division by zero
						fLossPercentage = (float)((float)(lost_now - m_lastLost)/(float)(packets_now-m_lastPackets))*100;
					else
						fLossPercentage = 0.0;
					pfmPingData->loss = (unsigned int)fLossPercentage;

				  } else {
					  pfmPingData->ping = 0;
					  pfmPingData->loss = 0;
				  }


			  }

			}

			BEGIN_PFM_CREATE(g.fm, pfmPingDataOpenWindow, S, PINGDATA)
			END_PFM_CREATE
			// send open ping window code
			pfmPingDataOpenWindow->shipID = -1;
			pfmPingDataOpenWindow->ping = 1;

			g.fm.SendMessages(pfsPlayer->GetConnection(),FM_GUARANTEED, FM_FLUSH);

		} else {
			BEGIN_PFM_CREATE(g.fm, pfmPingData, S, PINGDATA)
			END_PFM_CREATE
			// send open unchanged ping window code
			pfmPingData->shipID = -1;
			pfmPingData->ping = 1;
			g.fm.SendMessages(pfsPlayer->GetConnection(),FM_GUARANTEED, FM_FLUSH);
		}
	}
	break;

    default:
    {
      // an unrecognized message was received.
      TrapHackBoot(false);
    }
    break;
  } // switch(pfm->fmid)

  // Don't let messages queue up cross incoming message boundaries
  // If you're gonna queue a message, send it--don't make me guess who it goes to or whether it needs to be guaranteed.
  assert(0 == g.fm.CbUsedSpaceInOutbox());
  //g.fm.SetPriority(c_mcpDefault);

  timerOnAppMessage.Stop("...for message type %s\n", g_rgszMsgNames[pfm->fmid]);

  return(S_OK);
}


/*-------------------------------------------------------------------------
 * CreateUniqueMutex
 *-------------------------------------------------------------------------
 * Purpose:
 *    Creates a mutex object, with a unique name, to indicate when the
 *    server is running.
 *
 */
HANDLE CreateUniqueMutex()
{
  // Create a NULL dacl to give "everyone" access
  SECURITY_ATTRIBUTES* psa = NULL;
  SECURITY_DESCRIPTOR   sd;
  SECURITY_ATTRIBUTES   sa = {sizeof(sa), &sd, false};
  if (IsWinNT())
  {
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, true, NULL, FALSE);
    psa = &sa;
  }

  // Create the mutex using the global name first
  HANDLE hMutex = ::CreateMutex(psa, false, szAllSrvRunningGlobal);
  if (!hMutex)
    hMutex = ::CreateMutex(psa, false, szAllSrvRunning);
  return hMutex;
}


/*-------------------------------------------------------------------------
 * CheckForNoShows
 *-------------------------------------------------------------------------
  Purpose:
      We create the mission before anyone is actually here (as directed by the lobby)
      But for whatever reason, if the person who requested the mission never shows up,
      we want to make sure the missions gets killed, especially since we now don't
      show the mission on the lobby for others to join until the creator gets in.

  Side Effects:
      Mission could be blown away
 */
void CheckForNoShows()
{
//KGJV #114 reactivated
//#if !defined(ALLSRV_STANDALONE)

  const ListFSMission * plistMission = CFSMission::GetMissions();
  for (LinkFSMission * plinkMission = plistMission->first(); plinkMission; plinkMission = plinkMission->next())
  {
    CFSMission * pfsMission = plinkMission->data();
#if !defined(SRV_CHILD)
	if (!pfsMission->GetMissionDef()->misparms.bObjectModelCreated &&  // Let admin games break the rules
		!strcmp(pfsMission->GetMissionDef()->misparms.szIGCStaticFile,IGC_ENCRYPT_CORE_FILENAME) && // KGJV: let custom core games break the rules too
        pfsMission->GetCountOfPlayers(NULL, true) == 0)
	{
#else
    if (pfsMission->GetCountOfPlayers(NULL, true) == 0 && g.dwArgCookie > 99)
	{
#endif
      // nuke it if it's been around more than 15 seconds
      SYSTEMTIME stNow;
      FILETIME ftNow;
      FILETIME ftStart;
      ::GetSystemTime(&stNow);
      SystemTimeToFileTime(&stNow, &ftNow);
      SystemTimeToFileTime(pfsMission->GetStartTime(), &ftStart);
      ULONGLONG ullStart;
      ULONGLONG ullNow;
      CopyMemory(&ullStart, &ftStart, sizeof(ullStart));
      CopyMemory(&ullNow,   &ftNow,   sizeof(ullNow));

      if ((ULONG) (ullNow - ullStart) > 100000000) // was 15 seconds, measaured in 100 nanoseconds
		  //made above timeout 200000000 less -Imago
      {
        debugf("No one showed up for mission %x, killing it.\n", pfsMission->GetCookie());
        delete pfsMission;
        break; // deleting 1 mission per check is plenty
      }
	}
  }
//#endif
}


void DisconnectFromLobby()
{
  // Disconnect from the lobby server
  if (g.fmLobby.IsConnected())
  {
    _AGCModule.TriggerEvent(NULL, AllsrvEventID_DisconnectingLobby,
      "", -1, -1, -1, 1,
      "LobbyServer", VT_LPSTR, (LPCTSTR)g.strLobbyServer);

    g.fmLobby.Shutdown();

    _AGCModule.TriggerEvent(NULL, AllsrvEventID_DisconnectedLobby,
      "", -1, -1, -1, 1,
      "LobbyServer", VT_LPSTR, (LPCTSTR)g.strLobbyServer);
  }
}


void ProcessNetworkMessages()
{
  static CTimer timerReceiveMessages("in ReceiveMessages()", .1f);
  static CTimer timerReceiveLobbyMessages("in lobby ReceiveMessages()", .1f);
  static CTimer timerClientRollCall("doing client roll call", .1f);

  // receive any messages in the queue
  timerReceiveMessages.Start();
  g.fm.ReceiveMessages();
  timerReceiveMessages.Stop();

  if (g.fmLobby.IsConnected())
  {
    timerReceiveLobbyMessages.Start();
    g.fmLobby.ReceiveMessages();
    timerReceiveLobbyMessages.Stop();
  }

  // Do a periodic roll call. If we haven't heard from anyone for two roll calls in a row, waste 'em
  // Also waste 'em if they have more that 100 messages queued up (which is hopelessly behind)
  static Time timeRollCall = Time::Now();
  if (g.timeNow.clock() - timeRollCall.clock() > (CLOCKS_PER_SEC*10)) // 10 second interval, w0dk4 changed to dynamic calculation based on constant in header file
  {
    timerClientRollCall.Start();
    ListConnections::Iterator iterCnxn(*g.fm.GetConnections());
    while (!iterCnxn.End())
    {
      CFMConnection & cnxn = *iterCnxn.Value();
      iterCnxn.Next(); // have to move iterator FIRST, because we might kill the current node


	  // w0dk4 player-pings feature
	  // Packet Loss Update
	  DWORD m_lastLost;
	  DWORD m_lastPackets;
	  cnxn.GetPacketLossData(&m_lastPackets,&m_lastLost);
	  DPN_CONNECTION_INFO Connection_Info;
	  Connection_Info.dwSize = sizeof(DPN_CONNECTION_INFO);
	  if(g.fm.GetConnectionInfo(&cnxn, Connection_Info) == S_OK) {
		m_lastLost = Connection_Info.dwPacketsDropped + Connection_Info.dwPacketsRetried;
		m_lastPackets = Connection_Info.dwPacketsSentNonGuaranteed + Connection_Info.dwPacketsSentGuaranteed;
		cnxn.SetPacketLossData(m_lastPackets,m_lastLost);
	  }
	  // end w0dk4

      DWORD cMsgs;
      DWORD cbMsgs;
      HRESULT hr = g.fm.GetConnectionSendQueue(&cnxn, &cMsgs, &cbMsgs);
      const DWORD cMaxQ = 100;  // mmf Imago this was 50 bumped it up to 150, wOkd4 set to 100

	  // w0dk4 join-drop bug fix (allow more time when joining)
	  int cMsgPayload;
	  CFSPlayer * pfsPlayer = CFSPlayer::GetPlayerFromConnection(cnxn);
	  if (pfsPlayer && pfsPlayer->GetJustJoined() == true) {
		  cMsgPayload = cMsgs;
		  cnxn.SetQueuePayload(cMsgPayload);
		  pfsPlayer->SetJustJoined(false);
	  } else {
		  cMsgPayload = cnxn.GetQueuePayload();
	  }
	  if (cMsgPayload > cMsgs) {
		  cMsgPayload = cMsgs;
		  cnxn.SetQueuePayload(cMsgPayload);
	  }
	  // end w0dk4

      if ((cnxn.IncAbsentCount() > 6) || // give them a ridiculous 1 minute of no messages
          FAILED(hr) ||
          ((cMsgs-cMsgPayload) > cMaxQ)) // dead--nuke 'em, w0dk4 added '- cMsgPayload'
      {
        char szBuff[128];
        int cb = wsprintf(szBuff, "Nuking %s(%u) because ", cnxn.GetName(), cnxn.GetID());

        if (FAILED(hr))
        {
          wsprintf(szBuff + cb, "GetConnectionSendQueue returned 0x%08x.\n", hr);
          debugf(szBuff);
        }
        else
        {
          DWORD dwHundredbpsG, dwmsLatencyG, dwHundredbpsU, dwmsLatencyU;
          hr = g.fm.GetLinkDetails(&cnxn, &dwHundredbpsG, &dwmsLatencyG, &dwHundredbpsU, &dwmsLatencyU);
          float kbpsG = (float) dwHundredbpsG / 10.0f;
          float kbpsU = (float) dwHundredbpsU / 10.0f;

          if (cMsgs > cMaxQ)
            wsprintf(szBuff + cb, "they had too many messages (%u) queued up to them.\n", cMsgs);
          else
            wsprintf(szBuff + cb, "they missed roll call, with messages (%u) queued up to them.\n", cMsgs);

          debugf(szBuff);
          debugf("DPlay's view of the link was: %.1fkbps, %ums.\n", kbpsG, dwmsLatencyG);
        }
        float flTimeDiff = ((float) (timeRollCall.clock() - cnxn.GetLastComplete())) / 1000.0f;
        debugf("The last completion was %.2f seconds ago.\n", flTimeDiff);

        g.fm.DeleteConnection(cnxn);
      }
    }
    timeRollCall = g.timeNow;
    timerClientRollCall.Stop();
    if (g.pServerCounters)
      g.pServerCounters->timeNetworkMessages =
          timerReceiveMessages.LastInterval() +
          timerReceiveLobbyMessages.LastInterval() +
          timerClientRollCall.LastInterval();
  }
  else if (g.pServerCounters)
    g.pServerCounters->timeNetworkMessages = timerReceiveMessages.LastInterval() + timerReceiveLobbyMessages.LastInterval();


  bool bSupposedToConnectToLobby;
  bool bServerStartedByUI = false;

#if defined(ALLSRV_STANDALONE)
    // If standalone, we download the cfg file right before connecting to the lobby
    // if started by command-line, then always try to connect to lobby
    // if not started by command-line (started by UI) then connect to lobby only for public games

  bServerStartedByUI = _Module.WasCOMStarted();
  bSupposedToConnectToLobby = !(FEDSRV_GUID != g.fm.GetHostApplicationGuid());  // public games
#else
    bSupposedToConnectToLobby = !g.strLobbyServer.IsEmpty();
#endif

	static int iTrys = 1;

	if (bSupposedToConnectToLobby) // are we supposed to be connected to a lobby?
  {
	// children always connect to the lobby, but if we get disconnected and we're empty, suicide! -Imago 6/25/08
#if defined(SRV_CHILD)
	  if(!g.fmLobby.IsConnected()){
		const ListFSMission * plistMission = CFSMission::GetMissions();
		for (LinkFSMission * plinkMission = plistMission->first(); plinkMission; plinkMission = plinkMission->next())
		{
			CFSMission * pfsMission = plinkMission->data();
			//NYI wind up another check?
			if (pfsMission->GetCountOfPlayers(NULL, false) == 0 && iTrys > 4) //added try again
				PostQuitMessage(0);
		}
	  }

#endif

    // roll call for lobby
    if (g.fmLobby.IsConnected())
    {
      static Time timeRollCallLobby = Time::Now();
		if (g.timeNow.clock() - timeRollCallLobby.clock() > 4000) // 20s interval
		  {
			BEGIN_PFM_CREATE(g.fmLobby, pfmHeartbeat, S, HEARTBEAT)
			END_PFM_CREATE
			g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
			timeRollCallLobby = g.timeNow;

#if !defined(SRV_PARENT) //Imago - no need to check missiond for the parent
			CheckForNoShows(); // kinda of a weird place to be calling it, but since we're already in a perdiodic timer...
#endif
		}
#if defined(SRV_PARENT) //Imago - resend lobby our login info, maybe keep CurGames as the heartbeat?
		static bool bNotFirstPump = false;
		static Time timeLastRelog = Time::Now();
		if ((g.timeNow.clock() - timeLastRelog.clock() > 30000) && (bNotFirstPump == true)) // 1m interval
		{
			_Module.InitPIDs();
			ZVersionInfo vi; ZString zInfo = (LPCSTR)vi.GetFileVersionString(); //Imago 7/10 #62
			// do this to update our current games count/availability
			BEGIN_PFM_CREATE(g.fmLobby, pfmLogon, S, LOGON_LOBBY)
			  FM_VAR_PARM(g.cStaticCoreInfo ? g.vStaticCoreInfo : NULL, g.cStaticCoreInfo * c_cbFileName) // KGJV #114
			  FM_VAR_PARM(PCC(g.strLocation), CB_ZTS) // KGJV #114
			  FM_VAR_PARM(PCC(UTL::GetPrivilegedUsers(-1)), CB_ZTS) // Imago #2 6/10
			  FM_VAR_PARM(PCC(zInfo), CB_ZTS) // Imago 62 7/10
			END_PFM_CREATE
			pfmLogon->cStaticCoreInfo = g.cStaticCoreInfo; // KGJV #114
			//Imago NYI: adjust max games based if we're under load
			pfmLogon->MaxGames = g.cMaxGames; // KGJV #114
			pfmLogon->verLobby = LOBBYVER_LS;
			pfmLogon->dwPort = _Module.GetOutgoingPort();
			pfmLogon->CurGames = _Module.GetChildCount();
			g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
			timeLastRelog = g.timeNow;
		}
		bNotFirstPump = true;
#endif

    } else { //lobby is not connected..
      static Time timeLastTry = Time::Now();

      // retry once a minute. This will block for a few seconds, so we don't want to do it too often
      if (g.timeNow.clock() - timeLastTry.clock() > 60000)
      {
#if defined(SRV_CHILD) //imago warn people in game about cnxn issues 6/26/06
		  CFSMission * pfsMission = CFSMission::GetMissions()->first()->data();
		  if (iTrys <= 3) {
			char szMessage[100];
			sprintf(szMessage,"Warning: Game lost connection with lobby, retrying (%i of 3)",iTrys);
			pfsMission->GetSite()->SendChat(NULL, CHAT_EVERYONE, NA, NA, szMessage, c_cidNone, NA, NA, NULL, true);
            ConnectToLobby(g.strLobbyServer);
		  } else {
			  bSupposedToConnectToLobby = false;
			  //give up
		  }
		  iTrys++;
#else
		  const ListFSMission * plistMission = CFSMission::GetMissions();
			bool fAnyRunning = false;
			for (LinkFSMission * plinkMission = plistMission->first(); !fAnyRunning && plinkMission; plinkMission = plinkMission->next())
			{
			  CFSMission * pfsMission = plinkMission->data();
			  if (pfsMission->GetStage() == STAGE_STARTED)
				fAnyRunning = true;
			}

			if (!fAnyRunning)
			  ConnectToLobby(NULL);
#endif


        timeLastTry = g.timeNow;
      } //retry connect
    } //connected
  } //supposed to connect
}


void UpdateMissions(Time timeStart, Time timePrevious)
{
  static CTimer timerIGC("updating all IGC missions", .1f);
  timerIGC.Start();
  static Time timLastStationUpdate(Time::Now());
  const float dtimStationUpdateInterval = 0.75f;

  const ListFSMission * plistMission = CFSMission::GetMissions();
  LinkFSMission * plinkMissionNext;
  for (LinkFSMission * plinkMission = plistMission->first(); plinkMission; plinkMission = plinkMissionNext)
  {
    plinkMissionNext = plinkMission->next();

    CFSMission * pfsMission = plinkMission->data();
    if (pfsMission->GetStage() < STAGE_OVER &&
        pfsMission->GetIGCMission() != g.trekCore) // trekCore is a mission, but not a real one
    {
      // if the game is an auto-restart game and needs to have the countdown started, do so
      float dtTimeTillStart = pfsMission->GetMissionDef()->misparms.timeStart - timeStart;
      float dtPreviousTimeTillStart = pfsMission->GetMissionDef()->misparms.timeStart - timePrevious;

      if (pfsMission->GetStage() == STAGE_NOTSTARTED)
      {
        if (pfsMission->GetMissionDef()->misparms.bAutoRestart)
        {
          if (dtTimeTillStart <= c_fMissionBriefingCountdown)
          {
            // arena style games don't have a minimum number of players per side
            if (pfsMission->GetMissionDef()->misparms.bAllowEmptyTeams)
            {
              pfsMission->StartCountdown(std::max(0.0f, dtTimeTillStart));
            }
            else
            {
              // Force all sides to ready to prevent an AFK from canceling the game
              for (SideID sideId = 0; sideId < pfsMission->GetMissionDef()->misparms.nTeams; sideId++)
                pfsMission->SetForceReady(sideId, true);

              if (pfsMission->FAllReady()) // a team was found to have less than the required players
                pfsMission->StartCountdown(std::max(0.0f, dtTimeTillStart));
              else
              {
                // let them completely run out of time before canceling the game
                if (dtTimeTillStart <= 0)
                {
                  pfsMission->SetStage(STAGE_OVER);
                 // Send chat to let everyone know the game is canceled
                  pfsMission->GetSite()->SendChat(NULL, CHAT_EVERYONE, NA, NA,
                      "Game canceled due to lack of players", c_cidNone, NA, NA, NULL, true);
                }
              }
            }
          }
          else if (dtTimeTillStart <= 2 * c_fMissionBriefingCountdown
              && dtPreviousTimeTillStart > 2 * c_fMissionBriefingCountdown)
          {
            // Send a warning chat to let everyone know the game is at risk of being canceled
            if (!pfsMission->GetMissionDef()->misparms.bAllowEmptyTeams
                && !pfsMission->FAllReady())
            {
              char szMessage[100];
              sprintf(szMessage,
                "Warning: Game will be canceled if all teams are not ready in %d seconds.",
                int(2 * c_fMissionBriefingCountdown));
              pfsMission->GetSite()->SendChat(NULL, CHAT_EVERYONE, NA, NA,
                szMessage, c_cidNone, NA, NA, NULL, true);
            }
          }
        }
        else if (pfsMission->GetMissionDef()->misparms.bAutoStart && pfsMission->FAllReady())
        {
          pfsMission->StartCountdown(pfsMission->GetMissionDef()->misparms.fStartCountdown);
        }
      }


      // if the countdown for a game has expired (and someone is in the game), start the game
      if (pfsMission->GetStage() == STAGE_STARTING
          && timeStart > pfsMission->GetMissionDef()->misparms.timeStart
          && (pfsMission->HasPlayers(NULL, true)
            || pfsMission->GetMissionDef()->misparms.bAllowEmptyTeams)
          )
      {
        pfsMission->StartGame();
      }

      // Then do periodic updates
      pfsMission->UpdateLobby(timeStart);
      MoveShips(pfsMission, timeStart, timePrevious);

      if (pfsMission->GetStage() == STAGE_STARTED)
      {
        if (timeStart - timLastStationUpdate > dtimStationUpdateInterval)
        {
          //Send station states to all of the docked players
          for (ShipLinkIGC*   psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
          {
            IshipIGC*       pship = psl->data();
            IstationIGC*    pstation = pship->GetStation();
            if (pstation != NULL)
            {
              CFSShip*    pfsship = (CFSShip*)(pship->GetPrivateData());
              if (pfsship->IsPlayer())
              {
                  // tell the client what happened
                  BEGIN_PFM_CREATE(g.fm, pfmStationsUpdate, S, STATIONS_UPDATE)
                    FM_VAR_PARM(NULL, sizeof(StationState))
                  END_PFM_CREATE

                  StationState* pss = (StationState*)(FM_VAR_REF(pfmStationsUpdate, rgStationStates));
                  pss->stationID        = pstation->GetObjectID();
                  pss->bpHullFraction   = pstation->GetFraction();
                  pss->bpShieldFraction = pstation->GetShieldFraction();

                  g.fm.SendMessages(pfsship->GetPlayer()->GetConnection(), FM_NOT_GUARANTEED, FM_FLUSH);
              }
            }
          }
        }

        //Do economic stuff
        pfsMission->DoTick(timeStart);
      }

      {
        for (ShipLinkIGC*   psl = pfsMission->GetIGCMission()->GetShips()->first(); (psl != NULL); psl = psl->next())
        {
            IshipIGC*       pship = psl->data();
            CFSShip*    pfsship = (CFSShip*)(pship->GetPrivateData());
            if (pfsship->IsPlayer())
                pfsship->GetPlayer()->IncrementChatBudget();
        }
      }
    }

    if (pfsMission->ShouldDelete())
        delete pfsMission;
  }

  timLastStationUpdate = timeStart;
  timerIGC.Stop();
  if (g.pServerCounters)
    g.pServerCounters->timeIGCWork= timerIGC.LastInterval();
}

#if defined(ALLSRV_STANDALONE)

void GetExePath(char * szEXEPath)
{
    ::GetModuleFileName(NULL, szEXEPath, MAX_PATH);
    char*   p = strrchr(szEXEPath, '\\');
    if (p)
    {
        p++;
        *p = 0; // erase filename
    }
    else
    {
        szEXEPath[0] = '\\';
        szEXEPath[1] = '\0';
    }
}

bool IsCFGFileValid(const char * szFileName)
{
    const char * c_szValidCfg  = "THIS IS A VALID CONFIG FILE";

    ZFile file(szFileName);

    int n = file.GetLength(); // -1 means error
    if (n != -1 && n != 0)
    {
        char* pData = new char[n+1];
        memcpy(pData, file.GetPointer(), n);
        pData[n] = 0;

        bool bValid = strstr(pData, c_szValidCfg) != NULL;
        delete [] pData;

        if (!bValid)
        {
            debugf("File %s is not a valid config file.\n", szFileName);
            return false;
        }
    }
    else
    {
         debugf("File %s error while trying to load downloaded config file.\n", szFileName);
         return false;
    }

    return true;
}


IHTTPSession * BeginConfigDownload()
{
  // Get the version information of the current module
  ZVersionInfo vi;
  ZString strBuild(vi.GetFileBuildNumber());

  char szConfig[MAX_PATH];
#ifdef _ALLEGIANCE_PROD_
  lstrcpy(szConfig, "http://allegiance.zaphop.com/allegiance-server.txt"); //Imago 6/10 // BT - STEAM
#else
  lstrcpy(szConfig, "http://allegiance.zaphop.com/allegiance-server.txt"); //Imago 6/10 // BT - STEAM
#endif

  CRegKey key;
  if (ERROR_SUCCESS == key.Open(HKEY_LOCAL_MACHINE, HKLM_FedSrv, KEY_READ))
  {
      ZString strConfig;
      if (SUCCEEDED(LoadRegString(key, "cfgfile", strConfig)))
      {
          // if reg value exists copy over default
          strcpy(szConfig, PCC(strConfig));
      }
  }

  // WLP 2006 - added next line of code to force server to report invalid config
  // if the new config is not found. It used to just use the old one without an error
  //
#if defined(SRV_PARENT) || defined(SRV_CHILD)  // parent/child setup allows for httpd failure from cfg file host...
  MoveFile(".\\temp.cfg",".\\last.cfg"); //still allow using the last known cfg, but always get a new one if we can -Imago 6/26/08
#else
  DeleteFile(".\\temp.cfg") ;  // WLP 2006 - yell invalid if not found
#endif

  if (ZString(szConfig).Find("http://") == -1)
  {
    CopyFile(szConfig, ".\\temp.cfg", FALSE);    // copy to current folder
    debugf("Using local config file due to registry setting for ConfigFile because \"http://\" was missing from it. %s\n", szConfig);
    return NULL; // true means already done
  }
  else
  {
    // Start the download of CFG file on secondary thread
    IHTTPSession * pHTTPSession = CreateHTTPSession(NULL);

    const char * szFileList[] = { szConfig, "temp.cfg", NULL };
    pHTTPSession->InitiateDownload(szFileList, "."); // . means current path
    return pHTTPSession;
  }
}



// returns true only if up-to-date
bool OnAutoUpdateEXEDownloadDone(const char * szFilelistCRC, int nFilelistCRC, int nFilelistSize, const char * szSite, const char * szDirectory, int nAutoUpdateCRC, const char * szEXEPath)
{
    char szPath[MAX_PATH + 16];
    strcpy(szPath, szEXEPath);
    strcat(szPath, "AutoUpdate.exe");

    if (nAutoUpdateCRC != FileCRC(szPath, NULL))
    {
        // report CRC mismatch error for AutoUpdate.EXE
        // todo fire event
        char * szMsg = "ERROR: AutoUpdate was aborted.  CRC for AutoUpdate.exe downloaded did not match CRC found in cfg file.  You will not be able to host a public game.\n";
        printf(szMsg);
        MessageBox(NULL, szMsg, "Allegiance Server", MB_SERVICE_NOTIFICATION);
        debugf("AutoUpdate.exe CRC mismatch\n");
        return false;
    }

    //
    // Decide what will be launched after the update (AllSrv or UI), and how it will be launched (as service or EXE)
    //
    char * szLaunchPostUpdate;

    if (_Module.WasCOMStarted())
    {
      szLaunchPostUpdate = "AllSrvUI.exe";
    }
    else
    {
      szLaunchPostUpdate = _Module.IsInstalledAsService() ? "S:AllSrv" : "AllSrv.exe";  // S: is short for "Start as Service"
    }

    char szCommandLine[MAX_PATH * 10];
    _snprintf (szCommandLine, sizeof(szCommandLine)-1, "%s %s %d \"%s\" \"%s\" \"%s\"", szLaunchPostUpdate, szFilelistCRC, nFilelistSize, szSite, szDirectory, PCC(g.strArtPath));

    printf("Launching AutoUpdate.exe ... ");
    // Start the AutoUpdate utility
    int nErr;
    if((nErr = (int)ShellExecute(0,
                         "Open",
                         szPath,
                         szCommandLine,
                         NULL,
                         SW_SHOWNORMAL)
                         ) <= 32)
    {
        // report launch error
        // todo fire event
        printf("failed.\nError launching AutoUpdate.exe (Code = %d).  You will not be able to host a public game.\n", nErr);
        MessageBox(NULL, "Error launching AutoUpdate.exe.  You will not be able to host a public game.", "Allegiance Server", MB_SERVICE_NOTIFICATION);
        debugf("Couldn't launch AutoUpdate.exe (Code = %d)\n", nErr);
        return false;
    }
    printf("succeeded.\n");

    // Exit the application
    PostQuitMessage(0);    // allow proper shutdown to occur so Shutdown event gets fired to AllSRvUI32
    //SetEvent(g.hKillReceiveEvent);
    return false;
}



bool DownloadAutoUpdateEXE(const char * szFilelistCRC, int nFilelistCRC, int nFilelistSize, const char * szSite, const char * szDirectory, int nAutoUpdateCRC, const char * szAutoUpdateURL)
{
    char szEXEPath[MAX_PATH];
    GetExePath(szEXEPath);

    char szPath[MAX_PATH];
    strcpy(szPath, szEXEPath);
    strcat(szPath, "AutoUpdate.exe");

    printf("Server is out-of-date.\n");

    if (nAutoUpdateCRC != FileCRC(szPath, NULL))
    {
      // Start the download of AutoUpdate.exe file
      IHTTPSession * pHTTPSession = CreateHTTPSession(NULL);

      const char * szFileList[] = { szAutoUpdateURL, "AutoUpdate.exe", NULL };
      pHTTPSession->InitiateDownload(szFileList, szEXEPath);

      debugf("Downloading AutoUpdate.exe file...");
      printf("Downloading AutoUpdate.exe file ... ");

      // we block as we download the AutoUpdate.exe file; should be okay since we are not connected to the zone.
      while(pHTTPSession->ContinueDownload())
      {
        Sleep(100);
      }

      debugf("Done\n");

      //
      // At this point we are done downloading AutoUpdate.exe file
      //

      bool bErrorOccurred = pHTTPSession->GetLastErrorMessage() ? true : false;

      char szErrorMsg[512];
      if (bErrorOccurred)
        _snprintf(szErrorMsg, sizeof(szErrorMsg), "\nError Connecting to Zone.  You will not be able to host a public game. AutoUpdate.exe download failed. \r\n%s", pHTTPSession->GetLastErrorMessage());

      delete pHTTPSession;

      if (bErrorOccurred)
      {
          // report AutoUpdate.exe download error
          // TODO: fire event
          printf("failed: %s\n", szErrorMsg);
          MessageBox(NULL, szErrorMsg, "Allegiance Server", MB_SERVICE_NOTIFICATION);
          debugf("%s\n", szErrorMsg);
          return false;
      }
      else
      printf("succeeded.\n");
    }

    return OnAutoUpdateEXEDownloadDone(szFilelistCRC, nFilelistCRC, nFilelistSize, szSite, szDirectory, nAutoUpdateCRC, szEXEPath);
}


bool OnCFGDownloadDone(const char * szConfig) // returns true iff up-to-date
{
    // Read the PublicLobby value
    char szLobby[_MAX_PATH * 2];

    GetPrivateProfileString("Allegiance", "PublicLobby", "",
      szLobby, sizeofArray(szLobby), szConfig);

    if (*szLobby == '\0')
      return false;
    g.strLobbyServer = szLobby;

    // mdvalley: get lobby port
    char szPort[8];

    GetPrivateProfileString("Allegiance", "LobbyServerPort", "2303",
      szPort, sizeofArray(szPort), szConfig);

    g.dwLobbyPort = atoi(szPort);

    char szFilelistCRC[50];


#ifdef USEAUTH
	// Extract auth server from code and update our auth subsystem to use it.
	// We share the client's field for this.  Probably not the right thing to do,
	// but it will help prevent mistakes as all clients/servers sharing a cfg will
	// then automatically share an auth server.

    char szAuthServer[128]; // Size borrowed from clintlib.cpp
    GetPrivateProfileString("Allegiance", "ZAuth", "auth.zone.com",
                            szAuthServer, sizeof(szAuthServer), szConfig);

    g.strAuthServer = szAuthServer;
#endif

    GetPrivateProfileString("AllSrvUI", "FilelistCRC", "0",
                            szFilelistCRC, sizeof(szFilelistCRC), szConfig);

    _strupr(szFilelistCRC);

    int nFilelistCRC = UTL::hextoi(szFilelistCRC);

    // If the CRC is zero (or doesn't exist) just accept it
    if (nFilelistCRC == 0)
      return true;

    char szEXEPath[MAX_PATH + 16];
    GetExePath(szEXEPath);

    char szPath[MAX_PATH + 16];
    strcpy(szPath, szEXEPath);
    strcat(szPath, "Filelist.txt");

    // Ask if they want to auto update, if determined necessary
    if (FileCRC(szPath, NULL) != nFilelistCRC)
    {
      char szStr[100];

      GetPrivateProfileString("AllSrvUI", "FilelistSize", "0",
                              szStr, sizeof(szStr), szConfig);

      int nFilelistSize = atoi(szStr);

      // If the filelist size is zero (or doesn't exist) just accept it
      if (nFilelistSize == 0)
        return true;

      char szSite[512];

      GetPrivateProfileString("AllSrvUI", "Site", "",
                              szSite, sizeof(szSite), szConfig);

      // If the site value is empty (or doesn't exist) just accept it
      if (szSite[0] == '\0')
        return true;

      char szDirectory[512];

      GetPrivateProfileString("AllSrvUI", "Directory", "",
                              szDirectory, sizeof(szDirectory), szConfig);

      // If the directory value is empty (or doesn't exist) just accept it
      if (szDirectory[0] == '\0')
        return true;

      char szAutoUpdateURL[512];

      GetPrivateProfileString("AllSrvUI", "AutoUpdateURL", "",
                              szAutoUpdateURL, sizeof(szAutoUpdateURL), szConfig);

      // If the URL value is empty (or doesn't exist) just accept it
      if (szAutoUpdateURL[0] == '\0')
        return true;

      char szAutoUpdateCRC[30];

      GetPrivateProfileString("AllSrvUI", "AutoUpdateCRC", "",
                              szAutoUpdateCRC, sizeof(szAutoUpdateCRC), szConfig);

      _strupr(szAutoUpdateCRC);

      int nAutoUpdateCRC = UTL::hextoi(szAutoUpdateCRC);

      // If the AutoUpdate values are empty (or don't exist) just accept them
      if (szAutoUpdateCRC[0] == '\0' || nAutoUpdateCRC == 0)
        return true;

      return DownloadAutoUpdateEXE(szFilelistCRC, nFilelistCRC, nFilelistSize, szSite, szDirectory, nAutoUpdateCRC, szAutoUpdateURL);
    }
    else
    {
      return true;
    }
}


// Imago changed to quietly use the last config if there was a download issue /w the parent/child
bool RetrieveCFGFile() // returns true if cfg was successfully retrieved
{
	bool bErrorOccurred = false;
  IHTTPSession * pHTTPSession = BeginConfigDownload();

  //
  // At this point pHTTPSession is NULL iff we already have the cfg file (like for local cfg files)
  //

  if (pHTTPSession)
  {
      debugf("Downloading cfg file...");

    // we block as we download the cfg file; should be okay since we are not connected to the zone.
    while(pHTTPSession->ContinueDownload())
    {
      Sleep(100);
    }

    debugf("Done\n");

    //
    // At this point we are done downloading cfg file
    //

    bErrorOccurred = pHTTPSession->GetLastErrorMessage() ? true : false;
    char szErrorMsg[512];
    if (bErrorOccurred)
      _snprintf(szErrorMsg, sizeof(szErrorMsg), "\nError Connecting to Zone.  You will not be able to host a public game. \r\n%s", pHTTPSession->GetLastErrorMessage());

    delete pHTTPSession;

    if (bErrorOccurred)
    {
        // report cfg file download error
        // TODO: fire event

        printf("%s\n", szErrorMsg);
        debugf("%s\n", szErrorMsg);
		//Imago 6/26/08 remove this later when parent/child setup is default for service mode installations
#if !defined(SRV_PARENT) && !defined(SRV_CHILD)
		if (!_Module.IsInstalledAsService())
          MessageBox(NULL, szErrorMsg, "Allegiance Server", MB_SERVICE_NOTIFICATION);
        return false;
#endif
    }
  }

  PathString pathCurrent(PathString::GetCurrentDirectory());
  PathString pathConfig;

  //imago 6/26/08 - keep things running if cfg file fetch has issues
#if defined(SRV_PARENT) || defined(SRV_CHILD)

  if (bErrorOccurred) {
	  pathConfig = pathCurrent + PathString(PathString("last.cfg").GetFilename());
	  if (!IsCFGFileValid(PCC(pathConfig))) {
		debugf("\nCouldn't find the previous CFG file\n");
		return false;
	  } else {
		  printf("Using last known configuration\n");
		  return OnCFGDownloadDone(PCC(pathConfig));
	  }
  } else {
	  pathConfig = pathCurrent + PathString(PathString("temp.cfg").GetFilename());
  }
#else
  //leave the behavior the the old way
  pathConfig = pathCurrent + PathString(PathString("temp.cfg").GetFilename());

#endif

  if (IsCFGFileValid(PCC(pathConfig)))
  {
    return OnCFGDownloadDone(PCC(pathConfig));
  }
  else
  {
    // report error
    // TODO: fire event
    debugf("\nInvalid CFG File.\n");
    char * szErrorMsg = "\nError Connecting to Zone.  You will not be able to host a public game. (CFG File is invalid.)\n";
    printf(szErrorMsg);
    if (!_Module.IsInstalledAsService())
      MessageBox(NULL, szErrorMsg, "Allegiance Server", MB_SERVICE_NOTIFICATION);
    return false;
  }
}

#endif // STANDALONE


/*-------------------------------------------------------------------------
 * AnnouncePause
 *-------------------------------------------------------------------------
  Purpose:
      Tell the lobby whether we're paused or not
 */
void AnnouncePause()
{
  BEGIN_PFM_CREATE(g.fmLobby, pfmPause, S, PAUSE)
  END_PFM_CREATE
  pfmPause->fPause = g.fPaused;

  AGCEventID idEvent = g.fPaused ? AllsrvEventID_Pause : AllsrvEventID_Continue;
  _AGCModule.TriggerEvent(NULL, idEvent, "", -1, -1, -1, 0);
  char * szMode = g.fPaused ? "paused." : "continued.";
  printf("Server %s.\n", szMode);
  g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
}


/*-------------------------------------------------------------------------
 * RestartPausedMissions
 *-------------------------------------------------------------------------
 * Purpose:
 *     Restart any paused missions when the service is unpaused.
 */
void RestartPausedMissions()
{
  const ListFSMission * plistMission = CFSMission::GetMissions();
  for (LinkFSMission * plinkMission = plistMission->first(); plinkMission; plinkMission = plinkMission->next())
  {
    CFSMission * pfsMission = plinkMission->data();
    if (pfsMission->GetStage() == STAGE_OVER && pfsMission->GetMissionDef()->misparms.bAllowRestart)
        pfsMission->SetStage(STAGE_NOTSTARTED);
  }
}


HRESULT ConnectToLobby(const char * pszLobby_OverrideCFG /* NULL == do not override */)
{
  // Disconnect from current lobby, if any
  DisconnectFromLobby();
  assert(!g.fmLobby.IsConnected());

  // find out what lobby to use and/or autoupdate info
  if (!pszLobby_OverrideCFG){
#if defined(ALLSRV_STANDALONE)
      if (!RetrieveCFGFile())
          return HRESULT_FROM_WIN32(ERROR_CRC);
#endif
  } else {
	  g.strLobbyServer = pszLobby_OverrideCFG;
  }
  printf("Connecting to lobby: %s ... ", PCC(g.strLobbyServer));
  _AGCModule.TriggerEvent(NULL, AllsrvEventID_Connecting, "", -1, -1, -1, 1,
    "LobbyServer", VT_LPSTR, PCC(g.strLobbyServer));
  HRESULT hr = g.fmLobby.JoinSession(
#if defined(ALLSRV_STANDALONE)
        FEDFREELOBBYSERVERS_GUID,
#else
        FEDLOBBYSERVERS_GUID,
#endif
		PCC(g.strLobbyServer), PCC(g.strLocalAddress), g.dwLobbyPort);		// mdvalley: forced to lobby port
  if (FAILED(hr))
  {
    printf("failed.\n", PCC(g.strLobbyServer));
    return hr;
  }

  printf("succeeded.\n", PCC(g.strLobbyServer));

  // Save the new LobbyServer value
  g.strLobbyServer = PCC(g.strLobbyServer);

  _AGCModule.TriggerEvent(NULL, AllsrvEventID_ConnectedLobby,
    "", -1, -1, -1, 1,
    "LobbyServer", VT_LPSTR, PCC(g.strLobbyServer));

    // mdvalley: Discover listening port
  DWORD dwPort = 0;						// Passing a port of 0 informs the lobby of the error.

  // mdvalley: Discover listening port
#if !defined(SRV_PARENT)
  hr = g.fm.GetListeningPort(&dwPort);
  if(FAILED(hr))
  {
	  debugf("Cannot discover listening port.\n");
	  //do something!
  }
#endif
#if defined(SRV_PARENT) // Imago - not passing a port allows us to do the redirect w/o effecting regular servers
  if (_Module.GetOutgoingPort() == 0) {
	  hr = g.fm.GetListeningPort(&dwPort);
	  if(FAILED(hr))
	  {
		  debugf("Cannot discover listening port.\n");
		  //do something!
	  } else {
		  _Module.SetOutgoingPort(dwPort);
	  }
  } else {
	  dwPort = _Module.GetOutgoingPort();
  }
#endif

  // Give lobby the version
  ZVersionInfo vi; ZString zInfo = (LPCSTR)vi.GetFileVersionString(); //Imago 7/10 #62
  BEGIN_PFM_CREATE(g.fmLobby, pfmLogon, S, LOGON_LOBBY)
	  FM_VAR_PARM(g.cStaticCoreInfo ? g.vStaticCoreInfo : NULL, g.cStaticCoreInfo * c_cbFileName) // KGJV #114
	  FM_VAR_PARM(PCC(g.strLocation), CB_ZTS) // KGJV #114
	  FM_VAR_PARM(PCC(UTL::GetPrivilegedUsers(-1)), CB_ZTS) // Imago #2 6/10
	  FM_VAR_PARM(PCC(zInfo), CB_ZTS) // Imago #2 6/10
  END_PFM_CREATE
  pfmLogon->cStaticCoreInfo = g.cStaticCoreInfo; // KGJV #114
  pfmLogon->MaxGames = g.cMaxGames; // KGJV #114
  pfmLogon->verLobby = LOBBYVER_LS;
  pfmLogon->dwPort = dwPort;
  //reconnected servers show appropriate game count - Imago 6/25/08
#if defined(SRV_PARENT)
  pfmLogon->CurGames = _Module.GetChildCount();
#else
  //keep the regular servers happy
  pfmLogon->CurGames = CFSMission::GetMissions()->n();
#endif

  g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);

//#if !defined(SRV_CHILD) imago: revisit
  // Make sure the lobby knows whether we're paused before we do anything else
  AnnouncePause();
//#endif

  // ok, now let's re-advertize our games
  const ListFSMission * plistMission = CFSMission::GetMissions();
  for (LinkFSMission * plinkMission = plistMission->first(); plinkMission; plinkMission = plinkMission->next())
  {
    CFSMission * pfsMission = plinkMission->data();
    BEGIN_PFM_CREATE(g.fmLobby, pfmNewMission, S, NEW_MISSION)
    END_PFM_CREATE
    pfmNewMission->dwIGCMissionID = pfsMission->GetIGCMission()->GetMissionID();
    // We DON'T have cookies for these anymore, so we don't want to be trying to use these w/ lobby until we get real cookie
    pfsMission->SetCookie(0);
  }
  hr = g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);

  //
  // For standalone, create default games as needed
  //
  if (SUCCEEDED(hr))
  {
	// KGJV - StartDefaultGames even for AZ
  // #if defined(ALLSRV_STANDALONE)
    // if we were started NOT from the COM object model, start any autostart games
  // KGJV - FZ servers starts default games only in service mode (ie not COMStarted).
	  //Imago - Use StartDefaultGames in a child process to accept a mission from the parent

#if defined(SRV_PARENT)
	if (!_Module.GetChildCount())
		StartDefaultGames();
#else
#if defined(ALLSRV_STANDALONE)
  if (!_Module.WasCOMStarted())
#endif
      StartDefaultGames();
#endif
  // #endif
  }

  return hr;
}


/*-------------------------------------------------------------------------
 * ProcessMsgPump
 *-------------------------------------------------------------------------
  Purpose:
      Process Window messages while we wait for the specified amount of time or specified events

  Parameters:
      time to wait (ms)
      count of events
      array of events

  Returns:
      wait object signled (WAIT_OBJECT_0 + n), or WAIT_TIMEOUT
 */
DWORD ProcessMsgPump(int dwSleep, DWORD nCount, LPHANDLE pHandles)
{
  static bool s_fWinNT = ::IsWinNT();
  Time timeBegin = Time::Now();
  DWORD dwSleep1 = dwSleep;
  DWORD dwEnd = timeBegin.clock() + dwSleep; // don't wanna use Time, because it forces use of floats
  static CTimer timerMsgPump("in message pump", 0.25f);
  static CTempTimer timerSingleMsg("handling single Windows msg", .01f);
  timerMsgPump.Start();
  DWORD dwWait = WAIT_TIMEOUT;
  do
  {
    // Wait until either we get a message or we ran out of time
    // You cannot run debug/test on Win95
    #ifdef DEBUG
      dwWait = MsgWaitForMultipleObjectsEx(nCount, pHandles, dwSleep, QS_ALLINPUT, MWMO_ALERTABLE);
    #else
      dwWait = MsgWaitForMultipleObjects(nCount, pHandles, false, dwSleep, QS_ALLINPUT);
    #endif

    if (WAIT_TIMEOUT == dwWait)
      break;

    // Process the message queue, if any messages were received
    if ((WAIT_OBJECT_0 + nCount) == dwWait)
    {
      static MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        // dispatch Windows Messages to allow for the admin tool's COM to work
        timerSingleMsg.Start();
        TranslateMessage(&msg);
        switch (msg.message)
        {
          case wm_sql_querydone:
          {
            CSQLQuery * pQuery = (CSQLQuery *) msg.lParam;
            pQuery->DataReady();
            break;
          }

          case wm_fs_pause:
            AnnouncePause();
            if (!g.fPaused)
              RestartPausedMissions();
            break;

          case WM_QUIT:
            _Module.StopAllsrv();
            break;

          default:
            DispatchMessage(&msg);
        }
        timerSingleMsg.Stop();
      }
    }

    dwSleep = dwEnd - Time::Now().clock();
  } while (dwSleep <= dwSleep1); //dwSleep can't be less than 0 (since it is a dword)

  timerMsgPump.Stop();
  if (g.pServerCounters)
    g.pServerCounters->timeMsgPump= timerMsgPump.LastInterval();

  return dwWait;
}


// KGJV- StartDefaultGames even for AZ
//#if defined(ALLSRV_STANDALONE)
/*-------------------------------------------------------------------------
 * StartDefaultGames
 *-------------------------------------------------------------------------
 * Purpose:
 *    Starts any games listed in the registry
 */
void StartDefaultGames()
{
  // if we happen to have games running, then no need to start the same games over again.
  const ListFSMission * plistMission = CFSMission::GetMissions();
  if(0 != plistMission->n())
    return;

  MissionParams mp;
  mp.Reset();
  // KGJV #92
  // clean MissionParams to fetch default values
  MissionParams mpClean;
  mpClean.Reset();

  // we access the registry here because these keys don't need to be saved as globals.
  HKEY hKey;
  DWORD dwType;
  DWORD dwValue;
  DWORD cbValue;
  if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, HKLM_FedSrv, 0, KEY_READ, &hKey))
  {
    // get the max players per game to use.from the registry
    cbValue = sizeof(dwValue);
	// MaxPlayersPerGame not used for AZ
#if defined(ALLSRV_STANDALONE)
    if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "MaxPlayersPerGame", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
      mp.nTotalMaxPlayersPerGame = std::min(c_cMaxPlayersPerGame, (short)dwValue);
    else
      mp.nTotalMaxPlayersPerGame = c_cMaxPlayersPerGame;
#endif

    // lock the games open, if requested.
    // KGJV #92 - do this per game (see below) instead of global
    //cbValue = sizeof(dwValue);
    //if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "LockOpen", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
    //  mp.bLockGameOpen = !!dwValue;

#if defined(ALLSRV_STANDALONE)
    mp.nMinPlayersPerTeam = 1;
    mp.nMaxPlayersPerTeam = (char)(std::min(100, (int)mp.nTotalMaxPlayersPerGame / (int)mp.nTeams));

	mp.bObjectModelCreated = true; // a little bit of a fib, but it makes it look normal
#else
	mp.bObjectModelCreated = false; // FAZ: default games are not zone games
#endif

    HRESULT hr = S_OK;
    int iGame = 1;

	// Imago: children latch onto their parents mission 6/25/08
#if defined(SRV_CHILD)
	// Set name & core from command-line arguments
	Strcpy(mp.strGameName, (PCC)g.strArgName);
	Strcpy(mp.szIGCStaticFile, (PCC)g.strArgCore);
	mp.bScoresCount = false;
	mp.iMaxImbalance = 0x7ffe;
	mp.bLockGameOpen = false;

	mp.bObjectModelCreated = (g.dwArgCookie < 99) ? true : false;

    // Create the new mission
    FedSrvSite * psiteFedSrv = new FedSrvSite();
    CFSMission * pfsMissionNew = new CFSMission(mp, NULL, psiteFedSrv, psiteFedSrv, NULL, NULL);

	if(g.dwArgCookie < 99) {
		// tell the lobby about the mission
		if (g.fmLobby.IsConnected())
		{
		  BEGIN_PFM_CREATE(g.fmLobby, pfmNewMission, S, NEW_MISSION)
		  END_PFM_CREATE
		  pfmNewMission->dwIGCMissionID = pfsMissionNew->GetIGCMission()->GetMissionID();

		  g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
		}
	} else {
		// Set the cookie to the connection request so the mission creator can auto-join
		pfsMissionNew->SetCookie(g.dwArgCookie);
	}

	debugf("Starting game \"%s\"(%s) with core: %s - minrank=%d - maxrank=%d\n", mp.strGameName, (mp.bLockGameOpen == 1) ? "LockOpen":"Not LockOpen",mp.szIGCStaticFile,mp.iMinRank,mp.iMaxRank);
#else
	// Imago: otherwise do the regular gig
	while (SUCCEEDED(hr))
    {
      char cbValueName[24];
      ZString strGameName;

      // look for a registry key Game#
      wsprintf(cbValueName, "Game%d", iGame);
      hr = LoadRegString(hKey, cbValueName, strGameName);

      if (SUCCEEDED(hr))
      {
        // copy the new name into the mission parameters
        strncpy(mp.strGameName, strGameName, c_cbGameName);
        mp.strGameName[c_cbGameName - 1] = '\0';
        assert(!mp.Invalid());

		// -Imago
       	// look for a core/map for Game#
        ZString strCore;
        wsprintf(cbValueName, "Core%d", iGame);
		LoadRegString(hKey, cbValueName, strCore);
		strncpy(mp.szIGCStaticFile, strCore, c_cbFileName);

		// -Imago
       	// look for a core/map for Map#
		// -KGJV : should we lock game settings when custom map specified. If so need 'GameParameters#' too ?
		// removed map# for now
		// BT - Readded for bots
		ZString strMap;
		wsprintf(cbValueName, "Map%d", iGame);
		LoadRegString(hKey, cbValueName, strMap);

		if (strlen(strMap) > 0 && stricmp("botbrawl", strMap) == 0)
		{
			mp.mmMapType = c_mmBrawl;
			mp.nGoalTeamKills = 100;
			mp.bAllowDevelopments = false;
			mp.bObjectModelCreated = true;
			mp.bClubGame = true;
			mp.iGoalConquestPercentage = 0;
			mp.bDrones = true;
			mp.iMaxImbalance = 0x7fff; // Imalance = N/A
		}

		

        // KGJV #92 - LockOpen# option  per game
        // dword value - 1 to lockopen
        wsprintf(cbValueName, "LockOpen%d", iGame);
        cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, cbValueName, NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            mp.bLockGameOpen = !!dwValue;
        else
            mp.bLockGameOpen = mpClean.bLockGameOpen; // fetch default value

        // KGJV #92 - MinRank# option  per game
        // string value (but numeric)
        ZString strRankParam;
        wsprintf(cbValueName, "MinRank%d", iGame);
        if (LoadRegString(hKey, cbValueName, strRankParam) == S_OK)
            sscanf(strRankParam,"%hd",&(mp.iMinRank));
        else
            mp.iMinRank = mpClean.iMinRank; // fetch default value

        // KGJV #92 - MinRank# option  per game
        // string value (but numeric)
        wsprintf(cbValueName, "MaxRank%d", iGame);
        if (LoadRegString(hKey, cbValueName, strRankParam) == S_OK)
            sscanf(strRankParam,"%hd",&(mp.iMaxRank));
        else
            mp.iMaxRank = mpClean.iMaxRank; // fetch default value

		// KGJV -09/2008, added: Password# game parameter
		ZString strPassword;
        wsprintf(cbValueName, "Password%d", iGame);
        if (LoadRegString(hKey, cbValueName, strPassword) == S_OK)
			strcpy_s(mp.strGamePassword,sizeof(mp.strGamePassword),PCC(strPassword));

        // KGJV #92 - display LockOpen/Min/Maxrank and removed map name
        printf("Starting game \"%s\"(%s) with core: %s - minrank=%d - maxrank=%d\n", mp.strGameName, (mp.bLockGameOpen == 1) ? "LockOpen":"Not LockOpen",mp.szIGCStaticFile,mp.iMinRank,mp.iMaxRank);

#if !defined(SRV_PARENT)
        // Create the new mission
        FedSrvSite * psiteFedSrv = new FedSrvSite();
        CFSMission * pfsMissionNew = new CFSMission(mp, NULL, psiteFedSrv, psiteFedSrv, NULL, NULL);

        // tell the lobby about the mission
        if (g.fmLobby.IsConnected())
        {
          BEGIN_PFM_CREATE(g.fmLobby, pfmNewMission, S, NEW_MISSION)
          END_PFM_CREATE
          pfmNewMission->dwIGCMissionID = pfsMissionNew->GetIGCMission()->GetMissionID();

          g.fmLobby.SendMessages(g.fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
        }
#else //start default games in their own process
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );
		char szCmd[255]; char szName[32]; char szFile[32];
		ZString strName = mp.strGameName;
		ZString strFile = mp.szIGCStaticFile;
		si.lpTitle = mp.strGameName;
		Strcpy(szName,(PCC)strName);
		Strcpy(szFile,(PCC)strFile);
		sprintf(szCmd,"AllSrv.exe \"%s\" %s %x",szName,szFile,iGame);

		// Create a NULL dacl to give "everyone" access
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa = {sizeof(sa), &sd, false};
		InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, true, NULL, FALSE);

		if( !CreateProcess(
			NULL,                   // No module name (use command line).
			szCmd,
			NULL,
			NULL,
			FALSE,                  // Set handle inheritance to FALSE.
			CREATE_NEW_PROCESS_GROUP|CREATE_NEW_CONSOLE, // we're destined to do amazing things
			NULL,                   // Use parent's environment block.
			NULL,                   // Use parent's starting directory.
			&si,                    // Pointer to STARTUPINFO structure.
			&pi )                   // Pointer to PROCESS_INFORMATION structure.
		)
		{
			debugf( "CreateProcess failed (%d).\n", GetLastError() );
		}

		_Module.AddPID(pi.dwProcessId);
		char strFilename[10] = "\0";
		sprintf(strFilename,"%d.pid",pi.dwProcessId);
		UTL::SaveFile(strFilename,"1",2,NULL,false);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		g.bRestarting = true;
		Sleep(1000);
#endif
      }
      ++iGame;
    }

#endif
    RegCloseKey(hKey);
#if defined(SRV_PARENT) //reload ourselves to free the mission handles
	if(g.bRestarting) {
		Sleep(1000);
		PostQuitMessage(0);
	}
#endif
  }
}
//#endif


/*-------------------------------------------------------------------------
 * ReceiveThread
 *-------------------------------------------------------------------------
 * Purpose:
 *    Receives messages for a mission, and does all periodic processing
 *
 * Parameters:
 *    pThreadParameter: Unused
 */
DWORD WINAPI ReceiveThread(LPVOID pThreadParameter)
{
  HANDLE rgeventHandles[2];
  ZVersionInfo vi;
  
  debugf("Running %s %s\n", (PCC) vi.GetInternalName(), (PCC) vi.GetStringValue("FileVersion"));

  debugf("Entering ReceiveThread\n");
  DWORD dwUpdateInterval = g.nUpdatesPerSecond <= 0 ? (1000 / 10) : (1000 / g.nUpdatesPerSecond); // milliseconds
  if (g.pServerCounters)
    g.pServerCounters->timeCycleTarget = dwUpdateInterval;

  printf("Preparing server...\n");
  // Enter this thread into the COM STA
  HRESULT hr = CoInitialize(NULL);
  ZSucceeded(hr);

  // Initialize the ATL module
  _Module.Init(g.hInst); // init COM and/or NT service stuff

  // Create the single instance of the CAdminServer object
  CComObject<CAdminServer>* pServer = NULL;
  ZSucceeded(pServer->CreateInstance(&pServer));

  // Add the CAdminServer instance to the GIT (the only ref we keep)
  ZSucceeded(GetAGCGlobal()->RegisterInterfaceInGlobal(pServer->GetUnknown(),
    IID_IUnknown, &g.dwServerGITCookie));

  // Create a uniquely-named mutex to indicate that the server is running
  TCHandle shMutexRunning = CreateUniqueMutex();

  // Register class objects
  _Module.RegisterCOMObjects();


  // Setup the database stuff and connect
  // BT - 9/17 - No longer used.
//#if !defined(ALLSRV_STANDALONE) Imago 6/10 commented and fixed up
  /*if (g.strSQLConfig.m_str) {
    hr = g.sql.Init(g.strSQLConfig.m_str, g.idReceiveThread, g.csqlSilentThreads, g.csqlNotifyThreads);

    if (FAILED(hr))
    {
      return hr;
    }
  }*/
//#endif

  UTL::SetArtPath(g.strArtPath);

  g.trekCore = CreateMission(); // this isn't a *real* mission, just a gateway to static stuff
  g.trekCore->Initialize (Time::Now(), &g.siteFedSrv);
  debugf("trekCore completed\n");
  // until the server loads the appropriate igc static file based on mission params
  // and only has one game per exe
  int   iStaticCoreVersion;
#if !defined(ALLSRV_STANDALONE)
  iStaticCoreVersion = LoadIGCStaticCore (IGC_ENCRYPT_CORE_FILENAME, g.trekCore, false, DoDecrypt);
#else // !defined(ALLSRV_STANDALONE)
  iStaticCoreVersion = LoadIGCStaticCore (IGC_STATIC_CORE_FILENAME, g.trekCore, false);
#endif // !defined(ALLSRV_STANDALONE)
  assert (iStaticCoreVersion != NA);
  debugf("iStaticCoreVersion = %d\n",iStaticCoreVersion);

  // KGJV #114
  // load cores list (must be done before 1st connection to a lobby)
#if !defined(SRV_CHILD) //dont let the lobby list us (works for now) -Imago
  LoadStaticCoreInfo();
#endif
  //
  // This is the first place where we can might connect to a lobby
  //

  //Imago - parents & children always host sessions /w public lobby guid
#if defined(SRV_PARENT) || defined(SRV_CHILD)
   pServer->put_PublicLobby(true);
#endif
#if !defined(SRV_PARENT) && !defined(SRV_CHILD)
#if defined(ALLSRV_STANDALONE)
  // standalone servers don't connect to lobby by default
  if (_Module.WasCOMStarted())
  {
    pServer->put_PublicLobby(false); // false == private games mode
  }
  else
  {
    g.strLobbyServer="?"; // this will be overrided by cfg file download; needs to be non-blank so ConnectToLobby() is called
    pServer->put_PublicLobby(true); // true means use public dplay session, and connect to lobby is g.strLobbyServer is not empty
  }
#else
  //non-standalone servers always should connect to a lobby
  pServer->put_PublicLobby(true); // true means use public dplay session, and connect to lobby is g.strLobbyServer is not empty
#endif // !defined(ALLSRV_STANDALONE)
#endif
  rgeventHandles[0] = g.hKillReceiveEvent;
  // rgeventHandles[1] = g.hPlayerEvent;

  // -Imago: changed for RankInfo in all modes
  LoadRankInfo();
  debugf("LoadRankInfo result: #ranks=%d\n",g.cRankInfo);

  // -KGJV: changed for StaticMapInfo in all modes
  debugf("calling LoadStaticMapInfo\n");
    LoadStaticMapInfo();
  debugf("LoadStaticMapInfo result: #maps=%d\n",g.cStaticMapInfo);

  CTimer timerReceiveIterations("between iterations in ReceiveThread loop", .25f);

  printf("Ready for clients.\n");

  // loop waiting for player events. If the kill event is signaled
  // the thread will exit
  timerReceiveIterations.Start();

  Time timeStartIteration = Time::Now();
  Time timeEndIteration = timeStartIteration;
  DWORD dwWait = WAIT_TIMEOUT;
  while (dwWait != WAIT_OBJECT_0)
  {
	// BT - STEAM
	SteamGameServer_RunCallbacks();

    timerReceiveIterations.Stop();
    timerReceiveIterations.Start();
    if (g.pServerCounters)
      g.pServerCounters->timeBetweenInnerLoops = timerReceiveIterations.LastInterval();
    timeEndIteration = timeStartIteration;
    timeStartIteration = Time::Now();

    g.timeNow = timeStartIteration;

    ProcessNetworkMessages();

    UpdateMissions(timeStartIteration, timeEndIteration);

    //Sleep an amount of time appropriate to total time will be dwUpdateInterval
    DWORD   dwIteration = (Time::Now().clock() - timeStartIteration.clock());
    DWORD   dwSleep = dwIteration < dwUpdateInterval
                      ? (dwUpdateInterval - dwIteration)
                      : 0;

    dwWait = ProcessMsgPump(dwSleep, 1, rgeventHandles);
  }

  HAGCLISTENERS hListeners = GetAGCGlobal()->EventListeners(
    EventID_ServerShutdown, -1, -1, -1);
  if (hListeners)
  {
    // Get the computer name
    char szName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cchName = sizeofArray(szName);
    GetComputerNameA(szName, &cchName);

    _AGCModule.TriggerEvent(hListeners, EventID_ServerShutdown, "", -1, -1, -1, 1,
      "Server", VT_LPSTR, szName);
  }

  printf("Shutting down server...\n");

  // BT - STEAM
  SteamGameServer_Shutdown();

  // Suspend creation of any objects
  CoSuspendClassObjects();

  // Revoke class objects
  _Module.RevokeCOMObjects();

  // Close the uniquely-named mutex to indicate that the server is not running
  shMutexRunning = NULL;

  // Get the CAdminServer instance from the GIT (to keep a ref on it)
  IUnknownPtr spunkServer;
  ZSucceeded(GetAGCGlobal()->GetInterfaceFromGlobal(g.dwServerGITCookie,
    IID_IUnknown, (void**)&spunkServer));

  // Remove the CAdminServer instance from the GIT
  ZSucceeded(GetAGCGlobal()->RevokeInterfaceFromGlobal(g.dwServerGITCookie));

  // Disconnect any other external references to the CAdminServer instance
  CoDisconnectObject(spunkServer, 0);

  // Release the CAdminServer instance
  spunkServer = NULL;

  UnloadRankInfo(); //Imago: LoadRankInfo in all modes

  // -KGJV: StaticMapInfo in all modes
  UnloadStaticMapInfo();
#if !defined(SRV_CHILD)
  // KGJV #114
  UnloadStaticCoreInfo();
#endif
  // destroy IGC stuff
  UnloadDbCache();

  // Terminate the ATL module
  _Module.Term();

  // Remove this thread from the COM STA
  CoUninitialize();

  ExitThread(0);
  return 0;
}

/*-------------------------------------------------------------------------
 * LoadSettings
 *-------------------------------------------------------------------------
 * Purpose:
 *    Load static server settings
 */
void LoadSettings()
{
    // Explicitly initialize the global data structure
    g.Init();

    // defaults for reg keys:
    // fTimeout,                     1              1-okay to timeout clients
    // fWantInt3,                    1              1-want to hit an Int3 on asserts
    // nUpdatesPerSecond,            10             the number of updates-per-second

    // Set default g.strArtPath to "Artwork" under the EXE directory
    TCHAR szModule[_MAX_PATH];
    TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
    GetModuleFileName(NULL, szModule, sizeofArray(szModule));
    _tsplitpath(szModule, szDrive, szDir, NULL, NULL);
    _makepath(szModule, szDrive, szDir, TEXT("Artwork\\"), NULL);
    g.strArtPath = szModule;


    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    //   If you update this section, please update srvconfig to allow support
    //   without resorting to regedit.
    //
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    HKEY hKey;
    DWORD dwType;
    DWORD dwValue;
    DWORD cbValue;
    if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, HKLM_FedSrv, 0, KEY_READ, &hKey))
    {
        if (FAILED(LoadRegString(hKey, "AuthServer", g.strAuthServer)))
          g.strAuthServer = "Pointweb01"; // default for all developers
		
		ZString zUsers;
		if (!FAILED(LoadRegString(hKey, "PrivilegedUsers", zUsers))) {
			ZVersionInfo vi; ZString zInfo = (LPCSTR)vi.GetCompanyName(); zInfo += (LPCSTR)vi.GetLegalCopyright();
			UTL::SetPrivilegedUsers((PCC)zUsers.Scramble(zInfo),-1); //Imago 6/10
		}

        dwValue=0;
        cbValue = sizeof(dwValue);
		// w0dk4 Bandwidth Patch - removed registry-check and set tickrate to 20 so client bandwidth settings are not fooled
		// g.nUpdatesPerSecond = 20;
		// mmf leave this in, server admins should have control of this even if clients are not getting the bandwidth
		//     they expect from the 'rough' data rate w0dk4 added
		if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "nUpdatesPerSecond", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            g.nUpdatesPerSecond = dwValue;
        else
            g.nUpdatesPerSecond = 10;


// mmf log to file on SRVLOG define as well as _DEBUG
#ifdef _DEBUG
#define SRVLOG
#endif

#ifdef SRVLOG // mmf changed this from _DEBUG
        dwValue=0;
        cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS != ::RegQueryValueEx(hKey, "nDebugMask", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            dwValue=FED_DEBUG_FILE;
        g_app.SetDebug(dwValue);
#endif

        // Standalone servers do not need a LobbyServer registry value because
        // they always get it from the CFG file.
        //#if !defined(ALLSRV_STANDALONE)  Imago 6/10
          LoadRegString(hKey, "LobbyServer", g.strLobbyServer);
        //#endif // !defined(ALLSRV_STANDALONE)

        // Use winsock-provided address for clients, unless overridden (e.g. dual nic)
		// KGJV #114 localaddress isnt used anymore by DP8 so we use it as public userfriendly server name
	    // KGJV #114 TODO: eventually rename 'LocalAdresss' to something more appropriate now, PublicName for instance.
        if (FAILED(LoadRegString(hKey, "LocalAddress", g.strLocalAddress)))
        {
          WSADATA WSAData;
          char szHostName[20]; // length of machine name--also long enough for ip address if that's what we get
          WSAStartup(MAKEWORD(1,1), &WSAData);
          gethostname(szHostName, sizeof(szHostName));
          WSACleanup();
          g.strLocalAddress = szHostName;
        }

		// KGJV #114
        if (FAILED(LoadRegString(hKey, "Location", g.strLocation)))
        {
			g.strLocation = "unknown";
		}

		// KGJV #114
		g.cMaxGames = 99; // default max game
        cbValue = sizeof(dwValue);
		RegQueryValueEx(hKey, "MaxGames", NULL, &dwType, (unsigned char*)&g.cMaxGames, &cbValue);

		// BT - STEAM - Enable a server to decide wheather or not it should allow only offical game clients to connect.
		g.bEnforceSteamDRM = false;
		DWORD dwEnforceSteamDRM;
		DWORD cbEnforceSteamDRM = sizeof(dwEnforceSteamDRM);
		RegQueryValueExA(hKey, "EnforceSteamDRM", NULL, &dwType, (unsigned char*)&dwEnforceSteamDRM, &cbEnforceSteamDRM);

		if (dwEnforceSteamDRM > 0)
			g.bEnforceSteamDRM = true;

		// BT - STEAM
		g.bUseSteamAuthentication = false;
		DWORD dwUseSteamAuthentication;
		DWORD cbUseSteamAuthentication = sizeof(dwUseSteamAuthentication);
		RegQueryValueExA(hKey, "UseSteamAuthentication", NULL, &dwType, (unsigned char*)&dwUseSteamAuthentication, &cbUseSteamAuthentication);

		if (dwUseSteamAuthentication > 0)
			g.bUseSteamAuthentication = true;

		// BT - STEAM
		g.szLeaderboardUpdateUrl[0] = '\0';
		DWORD cbLeaderboardUpdateUrl = sizeof(g.szLeaderboardUpdateUrl);
		RegQueryValueExA(hKey, "LeaderboardUpdateUrl", NULL, &dwType, (unsigned char*)g.szLeaderboardUpdateUrl, &cbLeaderboardUpdateUrl);

		// BT - STEAM
		g.szApiKey[0] = '\0';
		DWORD cbApiKey = sizeof(g.szApiKey);
		RegQueryValueExA(hKey, "ApiKey", NULL, &dwType, (unsigned char*)g.szApiKey, &cbApiKey);

		// BT - STEAM
		g.szBotAuthenticationGuid[0] = '\0';
		DWORD cbBotAuthenticationGuid = sizeof(g.szBotAuthenticationGuid);
		RegQueryValueExA(hKey, "BotAuthenticationGuid", NULL, &dwType, (unsigned char*)g.szBotAuthenticationGuid, &cbBotAuthenticationGuid);

		// If no BotAuthenticationGuid was defined, then set this to some random GUID to ensure that no one guesses the bot auth GUID.
		if (strlen(g.szBotAuthenticationGuid) == 0)
		{
			UUID newId;
			UuidCreate(&newId);
			CHAR* szUuid = NULL;
			::UuidToString(&newId, (RPC_CSTR*)&szUuid);
			char pork[1024];
			strcpy(g.szBotAuthenticationGuid, szUuid);
			::RpcStringFree((RPC_CSTR*)&szUuid);
			szUuid = NULL;
		}

		/* pkk May 6th: Disabled bandwidth patch
		// w0dk4 Bandwidth Patch
		g.cMaxBandwidth = 32;
		dwValue=0;
        cbValue = sizeof(dwValue);
		RegQueryValueEx(hKey, "MaxBandwidth", NULL, &dwType, (unsigned char*)&g.cMaxBandwidth, &cbValue);*/


		ZString strArtPath;
        if (SUCCEEDED(LoadRegString(hKey, "ArtPath", strArtPath)))
        {
          if (strArtPath.Right(1) != "\\")
            strArtPath += "\\";
          g.strArtPath = strArtPath;
        }

        dwValue=0;
        cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "fWantInt3", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            g.fWantInt3 = !!dwValue;
        else g.fWantInt3 = true;
		
        //Imago 7/10 DIE ASGS DIE
        dwValue=0;
        cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "MaxNewbRank", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            g.MaxNewbRank = dwValue;
        else g.MaxNewbRank = 4;
        //

        dwValue=0;
        cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "fProtocol", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            g.fProtocol = !!dwValue;
        else g.fProtocol = true;

        dwValue=0;
        cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "fDoublePrecision", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            g.fDoublePrecision = !!dwValue;
        else g.fDoublePrecision = true;

        dwValue=0;
        cbValue = sizeof(dwValue);
        if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "fTimeout", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
            g.fTimeOut = !!dwValue;
        else g.fTimeOut = true;

      //#if !defined(ALLSRV_STANDALONE) Imago revived 6/10 // BT - 9/17 - No longer used.
        //dwValue=0;
        //cbValue = sizeof(dwValue);
        //if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "SQLThreadsNotify", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
        //    g.csqlNotifyThreads = dwValue;
        //else g.csqlNotifyThreads = 3;

        //dwValue=0;
        //cbValue = sizeof(dwValue);
        //if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, "SQLThreadsSilent", NULL, &dwType, (unsigned char*)&dwValue, &cbValue))
        //    g.csqlSilentThreads = dwValue;
        //else g.csqlSilentThreads = 2;

        //if (FAILED(LoadRegString(hKey, "SQLConfig", g.strSQLConfig)))
        //  g.strSQLConfig.Empty(); // default for all developers
      //#endif // !defined(ALLSRV_STANDALONE)

        ::RegCloseKey(hKey);
    }
}

/*-------------------------------------------------------------------------
 * StringStartsWith ()
 *-------------------------------------------------------------------------
 * Purpose:
 *    find out if a string starts with another string
 *
 *  Parameters: string to search, and string to find
 */
 bool   StringStartsWith (char* src, char* find)
 {
    int     find_length = strlen (find);
    for (int i = 0; i < find_length; i++)
        if (tolower (src[i]) != tolower (find[i]))
            return false;
    return true;
 }

/*-------------------------------------------------------------------------
 * BuildIGCFiles ()
 *-------------------------------------------------------------------------
 * Purpose:
 *    Build IGC files from the database
 *
 *  Parameters: none
 */
void    BuildIGCFiles (char* szArtworkPath, bool bEncrypt = false, bool bMaps = true)
{
  #if !defined(ALLSRV_STANDALONE)
    // standardize the format of the path string
    int     iPathLastChar = strlen (szArtworkPath) - 1;
    if (szArtworkPath[iPathLastChar] == '\\')
        szArtworkPath[iPathLastChar] = 0;

    UTL::SetArtPath(szArtworkPath);

    // load the static data and write it out
    LoadDbCache();
    printf (bEncrypt ? "  Writing encrypt core..." : "  Writing static core...");

    // we always generate it as static_core, but then they can rename it if they want to create different cores
    if (DumpIGCStaticCore (bEncrypt ? IGC_ENCRYPT_CORE_FILENAME : IGC_STATIC_CORE_FILENAME, g.trekCore, c_maskStaticTypes, bEncrypt ? DoEncrypt : NULL))
        printf (" OK\n");
    else
    {
        printf (" FAILED\n");
        exit (1);
    }

    // There's a number of database sizes that are greater than the message sizes,
    // and I don't want to put casts all over the place, so...
    #pragma warning(push)
    #pragma warning(disable : 4244)

    if (bMaps)
    {
        // now loop over all of the map data...
        SQL_GO(MapsCount);
        SQL_GETROW(MapsCount);

        MapDescription* pMaps = new MapDescription[g_cMapsCount];
        MapFileSite*    pSite = new MapFileSite;

        SQL_GO(MapIDs);
        for (int i = 0; i < g_cMapsCount; i++)
        {
            if (SQL_SUCCEEDED(SQL_GETROW(MapIDs)))
            {
                pMaps[i].mapID = MapIDs_MapID;
                SQLSTRCPY (pMaps[i].szFileName, MapIDs_FileName);
            }
        }

        for (i = 0; i < g_cMapsCount; i++)
        {
            if (StringStartsWith (pMaps[i].szFileName, "template") || StringStartsWith (pMaps[i].szFileName, "t_"))
                continue;
            printf ("  Creating Map %s...\n", pMaps[i].szFileName);
            ImissionIGC*    pMission = CreateMission();
            pMission->Initialize(Time::Now(), pSite);
            int   iStaticCoreVersion = LoadIGCStaticCore (bEncrypt ? IGC_ENCRYPT_CORE_FILENAME : IGC_STATIC_CORE_FILENAME, pMission, false,  bEncrypt ? DoDecrypt : NULL);
            assert (iStaticCoreVersion != NA);

            SectorInfo_MapID = StationInstance_MapID = Asteroids_MapID = AlephInst_MapID = MineInstance_MapID = ProbeInstance_MapID = TreasureInstance_MapID = pMaps[i].mapID;

            //-----------------------------------------------------------------
            // clusters
            //-----------------------------------------------------------------
            printf ("    --------------------------------------------------\n");
            SQL_GO(SectorInfo);
            while(SQL_SUCCEEDED(SQL_GETROW(SectorInfo)))
            {
                DataClusterIGC obj;

                obj.starSeed = SectorInfo_starSeed;
                obj.lightDirection = (Vector (SectorInfo_LightX, SectorInfo_LightY, SectorInfo_LightZ)).Normalize ();
                obj.lightColor = SectorInfo_LightColor;
                obj.screenX = SectorInfo_ScreenX;
                obj.screenY = SectorInfo_ScreenY;
                obj.clusterID = SectorInfo_SectorID;
                obj.nDebris = SectorInfo_DebrisCount;
                obj.nStars = SectorInfo_StarsCount;
                SQLSTRCPY (obj.name, SectorInfo_Name);
                SQLSTRCPY (obj.posterName, SectorInfo_PosterName);
                SQLSTRCPY (obj.planetName, SectorInfo_PlanetName);
                obj.planetSinLatitude.SetChar (SectorInfo_planetSinLatitude);
                obj.planetLongitude.SetChar (SectorInfo_planetLongitude);
                obj.planetRadius = SectorInfo_planetRadius;
                obj.activeF = true;
                obj.bHomeSector = SectorInfo_IsHomeSector ? true : false;

                if (StringStartsWith (obj.name, "template") || StringStartsWith (obj.name, "t_"))
                    continue;

                printf ("    Creating Sector %s...\n", obj.name);

                IObject* punk = pMission->CreateObject(g.timeNow, OT_cluster, &obj, sizeof(obj));
                punk->Release();
            }

            //-----------------------------------------------------------------
            // alephs
            //-----------------------------------------------------------------
            // count the number of alephs so we can preallocate them
            printf ("    --------------------------------------------------\n");
            SQL_GO(AlephsCount);
            SQL_GETROW(AlephsCount);

            std::map<int, DataWarpIGC*>   alephMap;

            // walk through all the alephs in the database, storing each one into the map
            SQL_GO(Alephs);
            while(SQL_SUCCEEDED(SQL_GETROW(Alephs)))
            {
                DataWarpIGC*    pObj = new DataWarpIGC;

                pObj->warpDef.warpID = AlephInst_ID;
                pObj->warpDef.destinationID = AlephInst_TargetAlephID;
                pObj->warpDef.radius = AlephInst_Radius;
                SQLSTRCPY (pObj->warpDef.textureName, AlephInst_Texture);
                SQLSTRCPY (pObj->warpDef.iconName, AlephInst_PRIcon);

                pObj->position = Vector (AlephInst_xLocation, AlephInst_yLocation, AlephInst_zLocation);
                Vector  forward (AlephInst_xForward, AlephInst_yForward, AlephInst_zForward);
                forward.SetNormalize ();
                pObj->forward = forward;
                pObj->rotation = Rotation (forward, AlephInst_rRotation);
                pObj->signature = AlephInst_Signature;
                pObj->clusterID = AlephInst_SectorID;

                alephMap[AlephInst_ID] = pObj;
            }

            // iterate over the alephs in the map, setting the names and creating the objects
            std::map<int, DataWarpIGC*>::iterator alephMapIterator = alephMap.begin ();
            while (alephMapIterator != alephMap.end ())
            {
                // get a pointer to the paired aleph
                DataWarpIGC*    pPairedAleph = alephMap[alephMapIterator->second->warpDef.destinationID];
                // get the sector the paried aleph is in
                IclusterIGC*    pCluster = pMission->GetCluster (pPairedAleph->clusterID);
                strcpy (alephMapIterator->second->name, pCluster->GetName ());
                IObject* punk = pMission->CreateObject(g.timeNow, OT_warp, alephMapIterator->second, sizeof(DataWarpIGC));
                punk->Release();

                printf ("    Creating Aleph from %s to %s...\n", pMission->GetCluster (alephMapIterator->second->clusterID)->GetName (), pCluster->GetName ());
                alephMapIterator++;
            }

            // iterate over the alephs in the map deleting the pointers to DataWarpIGC structures
            alephMapIterator = alephMap.begin ();
            while (alephMapIterator != alephMap.end ())
            {
                delete alephMapIterator->second;
                alephMapIterator++;
            }

            //-----------------------------------------------------------------
            // asteroids
            //-----------------------------------------------------------------
            printf ("    --------------------------------------------------\n");
            SQL_GO(Asteroids);
            while(SQL_SUCCEEDED(SQL_GETROW(Asteroids)))
            {
                DataAsteroidIGC obj;

                // XXX TODO -> propogate this to the database
                obj.signature = Asteroids_Signature;
                obj.position = Vector (Asteroids_x, Asteroids_y, Asteroids_z);
                Vector  up,
                        forward,
                        axis;
                if ((Asteroids_xUp == NA) && (Asteroids_yUp == NA) && (Asteroids_zUp == NA))
                    up = Vector::RandomDirection ();
                else
                    up = (Vector (Asteroids_xUp, Asteroids_yUp, Asteroids_zUp)).Normalize ();

                if ((Asteroids_xUp == NA) && (Asteroids_yUp == NA) && (Asteroids_zUp == NA))
                    forward = Vector::RandomDirection ();
                else
                    forward = (Vector (Asteroids_xForward, Asteroids_yForward, Asteroids_zForward)).Normalize ();

                if ((Asteroids_xRotation == NA) && (Asteroids_yRotation == NA) && (Asteroids_zRotation == NA))
                {
                    axis = Vector::RandomDirection ();
                    if (Asteroids_rRotation == NA)
                        Asteroids_rRotation = random (0.0f, 1.0f) * pi;
                }
                else
                    axis = (Vector (Asteroids_xRotation, Asteroids_yRotation, Asteroids_zRotation)).Normalize ();

                obj.up = up;
                obj.forward = forward;
                obj.rotation = Rotation (axis, Asteroids_rRotation);

                obj.asteroidDef.ore = obj.asteroidDef.oreMax = Asteroids_Ore;
                obj.asteroidDef.aabmCapabilities = Asteroids_Abilities;
                obj.asteroidDef.asteroidID = Asteroids_AsteroidID;
                obj.asteroidDef.hitpoints = Asteroids_HitPoints;
                obj.asteroidDef.radius = Asteroids_Radius;
                SQLSTRCPY (obj.asteroidDef.modelName, Asteroids_Model);
                SQLSTRCPY (obj.asteroidDef.textureName, Asteroids_Texture);
                SQLSTRCPY (obj.asteroidDef.iconName, Asteroids_PRIcon);

                obj.clusterID = Asteroids_ClusterID;
                SQLSTRCPY (obj.name, Asteroids_Name);
                if (obj.name[0] == '#')
                    obj.name[0] = '\0';

                obj.fraction = 1.0f;

                if (StringStartsWith (obj.name, "template") || StringStartsWith (obj.name, "t_"))
                    continue;

                printf ("    Creating Asteroid %s...\n", obj.name);

                IObject* punk = pMission->CreateObject(g.timeNow, OT_asteroid, &obj, sizeof(obj));
                punk->Release();
            }

            //-----------------------------------------------------------------
            // stations
            //-----------------------------------------------------------------
            // generate the sides
            for (int j = 0; j < c_cSidesMax; j++)
            {
                DataSideIGC         obj;

                // XXX TODO -> propogate this to the database
                obj.nFlags = 0;
                obj.nArtifacts = 0;

                obj.sideID = j;
                obj.gasAttributes.Initialize();
                obj.civilizationID = pMission->GetCivilizations()->first()->data()->GetObjectID();
                obj.ttbmDevelopmentTechs.SetAll ();
                obj.conquest = 0;
                obj.territory = 0;
                obj.nKills = obj.nDeaths = obj.nEjections = obj.nFlags = obj.nArtifacts
                    = obj.nBaseKills = obj.nBaseCaptures = 0;
                obj.squadID = NA;

                IObject*    punk = pMission->CreateObject(g.timeNow, OT_side, &obj, sizeof(obj));
                punk->Release();
            }

            // generate the actual stations
            printf ("    --------------------------------------------------\n");
            SQL_GO(StationInstance);
            while(SQL_SUCCEEDED(SQL_GETROW(StationInstance)))
            {
                DataStationIGC obj;

                // XXX TODO -> propogate this to the database
                obj.position = Vector (StationInstance_xLocation, StationInstance_yLocation, StationInstance_zLocation);
                obj.up = (Vector (StationInstance_xUp, StationInstance_yUp, StationInstance_zUp)).Normalize ();
                obj.forward = (Vector (StationInstance_xForward, StationInstance_yForward, StationInstance_zForward)).Normalize ();
                obj.rotation = Rotation ((Vector (StationInstance_xRotation, StationInstance_yRotation, StationInstance_zRotation)).Normalize (), StationInstance_rRotation);
                obj.clusterID = StationInstance_SectorID;
                obj.sideID = StationInstance_SideID;
                obj.stationID = StationInstance_StationID;
                obj.stationTypeID = StationInstance_StationTypeID;
                obj.bpHull = 1.0f;
                obj.bpShield = 1.0f;
                SQLSTRCPY (obj.name, StationInstance_Name);

                if (StringStartsWith (obj.name, "template") || StringStartsWith (obj.name, "t_"))
                    continue;

                printf ("    Creating Station %s...\n", obj.name);

                IObject* punk = pMission->CreateObject(g.timeNow, OT_station, &obj, sizeof(obj));
                punk->Release();
            }

            //-----------------------------------------------------------------
            // probes
            //-----------------------------------------------------------------
            printf ("    --------------------------------------------------\n");
            SQL_GO(ProbeInstance);
            while(SQL_SUCCEEDED(SQL_GETROW(ProbeInstance)))
            {
                DataProbeExport obj;

                obj.p0 = Vector (ProbeInstance_xLocation, ProbeInstance_yLocation, ProbeInstance_zLocation);
                obj.time0 = 0;
                obj.exportF = true;
                obj.probeID = ProbeInstance_ProbeID;
                obj.probetypeID = ProbeInstance_ProbeTypeID;
                obj.clusterID = ProbeInstance_SectorID;
                obj.sideID = ProbeInstance_SideID;
                obj.shipID = NA;
                obj.otTarget = NA;
                obj.oidTarget = NA;
                obj.createNow = true;

                printf ("    Creating probe %d...\n", ProbeInstance_ProbeID);

                IprobeIGC* punk = static_cast<IprobeIGC*> (pMission->CreateObject (g.timeNow, OT_probe, &obj, sizeof(obj)));
                punk->SetCreateNow ();
                punk->Release();
            }

            //-----------------------------------------------------------------
            // mines
            //-----------------------------------------------------------------
            printf ("    --------------------------------------------------\n");
    /*
    struct  DataMineBase
    {
        Vector              p0;
        Time                time0;
        MineID              mineID;
        bool                exportF;
    };

    struct  DataMineIGC : public DataMineBase
    {
        ImineTypeIGC*       pminetype;
        IshipIGC*           pshipLauncher;
        IsideIGC*           psideLauncher;
        IclusterIGC*        pcluster;
    };
    struct DataMineExport : public DataMineBase
    {
        SectorID            clusterID;
        ExpendableTypeID    minetypeID;
        ShipID              launcherID;
        SideID              sideID;
        BytePercentage      fraction;
        bool                createNow;
    };

        DEF_STMT(MineInstance, "SELECT "
            "MineID, "
            "MineTypeID, "
            "SideID, SectorID, "
            "LocationX, LocationY, LocationZ "
            "FROM MineInstances "
            "WHERE MapID = ?"
            )
        SQL_INT2_PARM(MineInstance_MineID,                          SQL_OUT_PARM)
        SQL_INT2_PARM(MineInstance_MineTypeID,                      SQL_OUT_PARM)
        SQL_INT1_PARM(MineInstance_SideID,                          SQL_OUT_PARM)
        SQL_INT2_PARM(MineInstance_SectorID,                        SQL_OUT_PARM)
        SQL_FLT4_PARM(MineInstance_xLocation,                       SQL_OUT_PARM)
        SQL_FLT4_PARM(MineInstance_yLocation,                       SQL_OUT_PARM)
        SQL_FLT4_PARM(MineInstance_zLocation,                       SQL_OUT_PARM)
        SQL_INT2_PARM(MineInstance_MapID,                           SQL_IN_PARM)

    */
            SQL_GO(MineInstance);
            while(SQL_SUCCEEDED(SQL_GETROW(MineInstance)))
            {
                DataMineExport obj;

                obj.p0 = Vector (MineInstance_xLocation, MineInstance_yLocation, MineInstance_zLocation);
                obj.time0 = 0;
                obj.exportF = true;
                obj.mineID = MineInstance_MineID;
                obj.minetypeID = MineInstance_MineTypeID;
                obj.launcherID = NA;
                obj.clusterID = MineInstance_SectorID;
                obj.sideID = MineInstance_SideID;
                obj.fraction = 1.0f;
                obj.createNow = true;

                printf ("    Creating mine %d...\n", MineInstance_MineID);

                ImineIGC* punk = static_cast<ImineIGC*> (pMission->CreateObject (g.timeNow, OT_mine, &obj, sizeof(obj)));
                punk->SetCreateNow ();
                punk->Release();
            }

            //-----------------------------------------------------------------
            // treasures
            //-----------------------------------------------------------------
            printf ("    --------------------------------------------------\n");
    /*
    struct  DataTreasureIGC
    {
        Vector              p0;
        Vector              v0;
        float               lifespan;
        Time                time0;
        ObjectID            objectID;
        TreasureID          treasureID;
        SectorID            clusterID;
        short               amount;
        TreasureCode        treasureCode;
        bool                createNow;
    };

        DEF_STMT(TreasureInstance, "SELECT "
            "Lifespan, "
            "TreasureID, "
            "LocationX, LocationY, LocationZ, "
            "SectorID, "
            "Amount, TreasureCode "
            "FROM TreasureInstances "
            "WHERE MapID = ?"
            )
        SQL_FLT4_PARM(TreasureInstance_Lifespan,                    SQL_OUT_PARM)
        SQL_INT2_PARM(TreasureInstance_TreasureID,                  SQL_OUT_PARM)
        SQL_FLT4_PARM(TreasureInstance_xLocation,                   SQL_OUT_PARM)
        SQL_FLT4_PARM(TreasureInstance_yLocation,                   SQL_OUT_PARM)
        SQL_FLT4_PARM(TreasureInstance_zLocation,                   SQL_OUT_PARM)
        SQL_INT2_PARM(TreasureInstance_SectorID,                    SQL_OUT_PARM)
        SQL_INT2_PARM(TreasureInstance_Amount,                      SQL_OUT_PARM)
        SQL_INT2_PARM(TreasureInstance_TreasureCode,                SQL_OUT_PARM)
        SQL_INT2_PARM(TreasureInstance_MapID,                       SQL_IN_PARM)
    */
            SQL_GO(TreasureInstance);
            while(SQL_SUCCEEDED(SQL_GETROW(TreasureInstance)))
            {
                DataTreasureIGC obj;

                obj.lifespan = TreasureInstance_Lifespan;
                obj.objectID = -(pMission->GenerateNewTreasureID() + 2);    //NYI hack: need explicit, unique object ID
                obj.treasureID = TreasureInstance_TreasureID;
                obj.p0 = Vector (TreasureInstance_xLocation, TreasureInstance_yLocation, TreasureInstance_zLocation);
                obj.v0 = Vector (0.0f, 0.0f, 0.0f);
                obj.time0 = 0;
                obj.clusterID = TreasureInstance_SectorID;
                obj.amount = TreasureInstance_Amount;
                obj.treasureCode = TreasureInstance_TreasureCode;
                obj.createNow = true;

                printf ("    Creating treasure %d...\n", TreasureInstance_TreasureID);

                ItreasureIGC* punk = static_cast<ItreasureIGC*> (pMission->CreateObject (g.timeNow, OT_treasure, &obj, sizeof(obj)));
                punk->SetCreateNow ();
                punk->Release();
            }

            //-----------------------------------------------------------------
            printf ("  Writing %s.igc ...", pMaps[i].szFileName);
            if (DumpIGCFile (pMaps[i].szFileName, pMission, c_maskMapTypes))
                printf (" OK\n\n");
            else
            {
                printf (" FAILED\n\n");
                exit (1);
            }

            pMission->Terminate ();
            delete pMission;
        }
    }

    #pragma warning(pop)
  #endif // !defined(ALLSRV_STANDALONE)
}


/*-------------------------------------------------------------------------
 * RegisterAllSrv()
 *-------------------------------------------------------------------------
 * Purpose:
 *    Register AllSrv as an EXE or NT Service, and report any errors
 *
 *  Paramters:
 *    fReRegister:  are we registering after an autoupdate?
 *    fInstallAsService: TRUE if NT service, FALSE for EXE
 */
void RegisterAllSrv(BOOL fReRegister, BOOL fInstallAsService, int argc, char *argv[])
{
  if (fInstallAsService && IsWin9x())
  {
    printf("Cannot register as a service on this operating system.\n");
    return;
  }

  DWORD dwErrorCode(_Module.RegisterServer(fReRegister, TRUE, fInstallAsService, argc, argv));

  if (FAILED(dwErrorCode))
  {
    char szBuf[MAX_PATH];
    sprintf(szBuf, "Unable to register server [0x%08x].  %s is not properly installed.\n", dwErrorCode, argv[0]);
    PrintSystemErrorMessage(szBuf, dwErrorCode);
  }
  else
  {
    // signal success in terms of adding COM related registry stuff
    printf("Registry Update Successful.\n");
  }
}


/*-------------------------------------------------------------------------
 * UnregisterSvr()
 *-------------------------------------------------------------------------
 * Purpose:
 *    Unregister AllSrv as an EXE or NT Service, and report any errors
 *
 */
void UnregisterAllSrv(char *szServerName)
{
  DWORD dwErrorCode(_Module.UnregisterServer());

  if (FAILED(dwErrorCode))
  {
    char szBuf[MAX_PATH];
    sprintf(szBuf, "Unable to remove server [0x%08x].  %s is not properly uninstalled.\n", dwErrorCode, szServerName);
    PrintSystemErrorMessage(szBuf, dwErrorCode);
  }
  else
  {
    // signal success in terms of removing COM related registry stuff
    printf("Registry Update Successful.\n");
  }
}


////////////////////////////////////////////////////////////////////////
// main
//
// This is the entry point to the process, whether run as a service, or
// as an executable.  Since the NT service manager does not send us
// command line flags, the default case is to run as a service.
// Additional supported mutually-exclusive functions are installation
// (with the -install command line switch) de-installation (with the
// -remove switch) and running as a non-service executable (with the
// -debug switch).
//
// This function parses the command line arguments and invokes the
// selected mode of operation.  When running as an executable, blocking
// keyboard input is used to prevent the program from exiting prematurely.
//
int __cdecl main(int argc, char *argv[])
{
    // Load the global settings from the registry
    LoadSettings();

    // set the FPU to double precision, instead of long double
    // (this makes FP compares a little safer)
    if (g.fDoublePrecision)
      _controlfp(_PC_53, _MCW_PC);

    // seed the random number generator with the current time
    // (GetTickCount may be semi-predictable on server startup, so we add the
    // clock time to shake things up a bit)
    srand(GetTickCount() + time(NULL));

	// start the appweb service thread w/log Imago 7/3/08
	char *programName = mprGetBaseName(argv[0]);
	mutex = new MprMutex();
	mpr = new Mpr(programName);
#ifdef _DEBUG
    MprLogToFile *logger;
	tMod = new MprLogModule(programName);
	logger = new MprLogToFile();
	mpr->addListener(logger);
    mpr->setLogSpec("allsrv_appweb.log:9");
#endif
	mpr->setMaxPoolThreads(4);    //NYI make the 4 a constant becasue it ended up getting reused
	mpr->start(MPR_SERVICE_THREAD);


    // Enter this thread into the COM MTA
    TCCoInit init(COINIT_MULTITHREADED);
    ZSucceeded((HRESULT)init);
    if (init.Failed())
      return (HRESULT)init;

    // Set the process-wide COM security
    // This provides a NULL DACL which will allow access to everyone.
    PSECURITY_DESCRIPTOR psd = NULL;
    CSecurityDescriptor sd;
    if (IsWinNT())
    {
      sd.InitializeFromThreadToken();
      psd = sd;
    }
    HRESULT hr;
    hr = CoInitializeSecurity(psd, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_PKT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    ZSucceeded(hr);
    RETURN_FAILED(hr);

    // Initialize AGC
    ZSucceeded(hr = _Module.InitAGC()); //dies here if the "Event Log" service is not started -Imago
    RETURN_FAILED(hr);

    char szSvcName[64];
	strcpy(szSvcName, c_szSvcName);
#if defined(SRV_PARENT)
    SERVICE_TABLE_ENTRY rgSTE[] =
    {
        { szSvcName, (LPSERVICE_MAIN_FUNCTION) MPServiceMain },
        { NULL, NULL }
    };
#else
    SERVICE_TABLE_ENTRY rgSTE[] =
    {
        { szSvcName, (LPSERVICE_MAIN_FUNCTION)_ServiceMain },
        { NULL, NULL }
    };
#endif
    bool fShowUsage = false;

    g.hInst = GetModuleHandle(NULL);

    if (argc < 1)
    {
      //printf ("Hell has frozen over.\n"); // this should never happen
      // Terminate AGC
      _Module.TermAGC();
      return 0;
    }

    // if registering (after an AutoUpdate)--be sure to keep existing install data intact
    if (argc > 1 && (*argv[1]=='-' || *argv[1]=='/') && !lstrcmpi ("reregister", argv[1]+1))
    {
      RegisterAllSrv(TRUE, _Module.IsInstalledAsService(), argc, argv);
      // Terminate AGC
      _Module.TermAGC();
      return 0;
    }

    // Check for a weird/busted installation...this really can happen
    if (_Module.IsInstalledAsService() && !_Module.IsInServiceControlManager())
    {
      printf("Incomplete installation detected. Removing incomplete installation...\n");

      UnregisterAllSrv(argv[0]);
    }

	// BT - STEAM
	uint32 unIP = INADDR_ANY;
	uint32 steamAuthenticationPort = 8766;
	uint32 gamePort = 27115; // We will manage our own network communications with game clients, so this will not be used directly by us.
	uint32 masterUpdaterPort = 27116;
	EServerMode eMode = eServerModeAuthenticationAndSecure;
	const char *lobbyVersion = "1.0.0.4";

	if (SteamGameServer_Init(unIP, steamAuthenticationPort, gamePort, masterUpdaterPort, eMode, lobbyVersion) == false)
	{
		printf("failed on steam server init.\n");
		exit(-1);
	}

	// Only for SteamAPI, not SteamGameServerAPI.
	//SteamUtils()->SetWarningMessageHook(&SteamAPIDebugTextHook);
	SteamGameServer()->LogOnAnonymous();

    _Module.SetCOMStarted(argc == 2 && !lstrcmpi("-Embedding",argv[1]));

    #if defined(ALLSRV_STANDALONE)
    #ifndef DEBUG //Imago changed from _DEBUG to allow DEBUG in retail builds w/o requiring debug run-times
    if (argc == 1 && !_Module.IsInstalledAsService() && !_Module.WasCOMStarted())
    {
		//multi-process setup can only be debugged /w console windows - Imago
		//NYI: should be released as service module only
#if defined(SRV_PARENT) || defined(SRV_CHILD)
		_Module.RunAsExecutable();
#else
      printf("\nUse AllSrvUI.exe to run the Allegiance Server.\n");
      // Terminate AGC
      _Module.TermAGC();
#endif
      return 0;
    }
    #endif
    #endif

	// Imago run a single game on this process
#if defined(SRV_CHILD)
	if(argc > 3) {
		g.strArgName = ZString(argv[1]);
		g.strArgCore = ZString(argv[2]);
		g.dwArgCookie = strtoul(argv[3], NULL, 16);
		_Module.RunAsExecutable();
	} else {
		printf("Error: Must be loaded by AllSrvMP.exe\n");
		// Terminate AGC
		_Module.TermAGC();

		// Indicate success
		return 0;
	}
#else // run multiple games on a single process
    if (argc > 1 && (*argv[1]=='-' || *argv[1]=='/') && !_Module.WasCOMStarted())
    {
      BOOL fInstallAsService = !lstrcmpi("Service",argv[1]+1);

      if (fInstallAsService || !lstrcmpi("RegServer",argv[1]+1))
      {

        RegisterAllSrv(FALSE, fInstallAsService, argc, argv);

      }
      else if (!lstrcmpi("UnregServer",argv[1]+1) || !lstrcmpi("Remove",argv[1]+1))
      {
        UnregisterAllSrv(argv[0]);
      }
	  //Imago ressurects the import/export functions (as structured text, not MSSQL)
      else if (!lstrcmpi ("Core2Text", argv[1]+1))
      {
		  char szFilename[MAX_PATH + 1];
		  UTL::SetArtPath(g.strArtPath);

		  if ( (argc > 2) && (UTL::getFile(argv[2], ".igc", szFilename, true, true) == S_OK) ) {

			printf("Reading core binary from file %s%s.igc\n",UTL::artworkPath(),argv[2]);

			MissionParams mp;
			Strcpy(mp.szIGCStaticFile, argv[2]);
			g.trekCore = CreateMission();
			g.trekCore->Initialize(Time::Now(), &g.siteFedSrv);
			g.trekCore->SetMissionParams(&mp);

			// load the specified core as the one and only
			int iStaticCoreVersion = LoadIGCStaticCore (argv[2], g.trekCore, false);
			ImissionIGC* pMission = g.trekCore;

			//make a folder for our export
			ZString strDir = ZString(UTL::artworkPath()) + ZString(argv[2]);
			CreateDirectoryA((PCC)strDir,NULL);

			if (!(pMission->LoadTechBitsList()))
				printf("Warning: Failed to load Tech mask bit flags! (\\artwork\\%s\\BitsOfTechs.csv)\n",argv[2]);
			if (!(pMission->LoadPartsBitsList()))
				printf("Warning: Failed to load Part mask bit flags! (\\artwork\\%s\\BitsOfParts.csv)\n",argv[2]);

			printf("Writing core text to folder %s\n",(PCC)strDir);
			pMission->ExportStaticIGCObjs();
			printf("Complete!\n");
			pMission->Terminate();
			delete pMission;
		  } else {
            printf ("Please provide a binary core IGC file...\n");
		  }
      }
      else if (!lstrcmpi ("Map2Text", argv[1]+1))
      {
		  if (argc > 2) {
            //ExportIGCObjs(argv[2]);
			  printf("\n\n\n\nNYI\n");
			  fShowUsage = true;
		  } else {
            printf ("Please provide the base name of a map IGC file...\n");
			fShowUsage = true;
		  }
      }
     else if (!lstrcmpi ("Text2Map", argv[1]+1))
      {
		  if (argc > 2) {
            //ImportIGCObjs(argv[2]);
			  printf("\n\n\n\nNYI\n");
			  fShowUsage = true;
		  } else {
            printf ("Please provide the base name of a map IGC folder...\n");
			fShowUsage = true;
		  }
      }
     else if (!lstrcmpi ("Text2Core", argv[1]+1))
      {
		  char szFilename[MAX_PATH + 1];
		  UTL::SetArtPath(g.strArtPath);
		  ZString strFile = ZString(argv[2]) + "//Constants";

		  if ( (argc > 2) && (UTL::getFile((PCC)strFile, ".csv", szFilename, true, true) == S_OK) ) {

			printf("Reading core text from folder %s%s\n",UTL::artworkPath(),argv[2]);

			MissionParams mp;
			Strcpy(mp.szIGCStaticFile, argv[2]);
			g.trekCore = CreateMission();
			g.trekCore->Initialize(Time::Now(), &g.siteFedSrv);
			g.trekCore->SetMissionParams(&mp);
			g.trekCore->SetStaticCore(NULL);

			ImissionIGC* pMission = g.trekCore;
			pMission->ImportStaticIGCObjs();


			if (!(pMission->LoadTechBitsList()))
				printf("Warning: Failed to load Tech mask bit flags! (\\artwork\\%s\\BitsOfTechs.csv)\n",argv[2]);
			if (!(pMission->LoadPartsBitsList()))
				printf("Warning: Failed to load Part mask bit flags! (\\artwork\\%s\\BitsOfParts.csv)\n",argv[2]);

			printf("Writing core binary to file %s%s.igc\n",UTL::artworkPath(),argv[2]);
			strFile = "\\" + ZString(argv[2]) + "\\" + ZString(argv[2]);
			DumpIGCStaticCore((PCC)strFile, pMission, c_maskStaticTypes, NULL);
			printf("Complete!\n");
			pMission->Terminate();
			delete pMission;
		  } else {
            printf ("Please provide a text core IGC folder...\n");
		  }
      }
    #if !defined(ALLSRV_STANDALONE)
      else if (!lstrcmpi ("BuildIGCFiles", argv[1]+1))
      {
        if (argc > 2)
            BuildIGCFiles (argv[2], false, true);
        else
            printf ("Please provide a path for IGC files to get written to...\n");
      }
      else if (!lstrcmpi ("EncryptIGCFiles", argv[1]+1))
      {
        if (argc > 2)
            BuildIGCFiles (argv[2], true, true);
        else
            printf ("Please provide a path for IGC files to get written to...\n");
      }
      else if (!lstrcmpi ("BuildIGCFilesNM", argv[1]+1))
      {
        if (argc > 2)
            BuildIGCFiles (argv[2], false, false);
        else
            printf ("Please provide a path for IGC files to get written to...\n");
      }
      else if (!lstrcmpi ("EncryptIGCFilesNM", argv[1]+1))
      {
        if (argc > 2)
            BuildIGCFiles (argv[2], true, false);
        else
            printf ("Please provide a path for IGC files to get written to...\n");
      }
    #endif // !defined(ALLSRV_STANDALONE)
      else
      {
            // if unknown args
           fShowUsage = true;
      }
    }
    else
    {
      if (argc == 1 || _Module.WasCOMStarted())
      {
        if (_Module.IsInstalledAsService())
        {
          printf("Trying to run %s as an NT Service...\nType \"%s -RegServer\" to re-install as EXE.\n", argv[0], argv[0]);
          //start as service
          if (!StartServiceCtrlDispatcher(rgSTE))
            _AGCModule.TriggerEvent(NULL, AllsrvEventID_StartingService, "", -1, -1, -1, 0);

        }
        else
        {
          if (!_Module.IsInstalled())
          {
            printf("\nInstalling %s as an EXE...\n", argv[0]);
            RegisterAllSrv(FALSE, FALSE, argc, argv); // FALSE == register as EXE
          }
          _Module.RunAsExecutable();
        }
      }
      else // if unknown args
        fShowUsage = true;
    }

    if (fShowUsage)
    {
      ZVersionInfo vi;
      printf("%s\n%s\n\n",
        (LPCSTR)vi.GetFileDescription(), (LPCSTR)vi.GetLegalCopyright());

      if (!_Module.IsInstalled())
      {
        printf("\nThe server needs to be installed before running.\n\n", argv[0]);
      }
      //      ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8
      printf("usage is ONE of:\n\n");
      printf("AllSrv                             starts the server if registered as an exe\n");
      printf("AllSrv -RegServer                  install the server as an EXE\n");
    if (IsWinNT())
      printf("AllSrv -Service   [user pw]        install the server as an NT Service\n");
      printf("AllSrv -UnregServer                uninstall the server\n");
	  printf("AllSrv -Core2Text [core_name]      converts core igc to text\n");
	  printf("AllSrv -Map2Text  [map_name]       converts map igc to text\n");
	  printf("AllSrv -Text2Core [core_name]      converts text core to igc\n");
	  printf("AllSrv -Text2Map  [map_name]       converts text map igc\n");
    #if !defined(ALLSRV_STANDALONE)
      printf("AllSrv -BuildIGCFiles <artpath>    builds igc files from database\n");
      printf("                                   (see root makefile for required env vars).\n");
      printf("AllSrv -EncryptIGCFiles <artpath>  builds & encrypts igc files from database\n");
      printf("                                   (see root makefile for required env vars).\n");
      printf("AllSrv -BuildIGCFilesNM <artpath>  as above, but no maps\n");
      printf("                                   (see root makefile for required env vars).\n");
      printf("AllSrv -EncryptIGCFilesNM <artpath> as above, but no maps\n");
      printf("                                   (see root makefile for required env vars).\n");
    #endif // !defined(ALLSRV_STANDALONE)
    if (IsWinNT())
      printf("\nTo start this service (when installed as NT service): net start %s\n",c_szSvcName);
      //      ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8
    }
#endif // Imago

    // Terminate AGC
    _Module.TermAGC();

	//appweb
	mpr->stop(0);
	delete mutex;
#ifdef _DEBUG
	delete tMod;
    delete logger;
#endif
	mprMemClose();

	// Indicate success
    return 0;
}


HRESULT pascal FedSrv_Pause()
{
  if (!g.fPaused)
  {
    g.fPaused = true;
    PostThreadMessage(g.idReceiveThread, wm_fs_pause, 0, 0);
  }
  return S_OK;
}


HRESULT pascal FedSrv_Continue()
{
  if (g.fPaused)
  {
    g.fPaused = false;
    PostThreadMessage(g.idReceiveThread, wm_fs_pause, 0, 0);
  }
  return S_OK;
}


/*-------------------------------------------------------------------------
 * FedSrv_Init
 *-------------------------------------------------------------------------
 * Purpose:
 *    Saves instance handle and creates main window. Does lots of setup stuff
 *
 * Side Effects:
 *    Lots
 */
HRESULT pascal FedSrv_Init()

{
  int iCon = 0;
  HRESULT hr = S_OK;

  _AGCModule.TriggerEvent(NULL, AllsrvEventID_Initialize, "", -1, -1, -1, 0);

  // Set cur dir = exe dir
  HMODULE hmod = GetModuleHandle(NULL);
  assert(hmod);
  char path_buffer[_MAX_PATH];
  GetModuleFileName(hmod, path_buffer, sizeof(path_buffer));

  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];

  // just strip off the filename.extension
  _splitpath(path_buffer, drive, dir, NULL, NULL);
  _makepath(path_buffer, drive, dir, NULL, NULL);
  SetCurrentDirectory(path_buffer);

  LoadTechBits(); // only for the IfYouBuildIt cheat

  //bool fSWMRGInit = FSWMRGInitialize(&g.swmrg, "FedSrv"); //Imago removed (again)
  //ZAssert(fSWMRGInit);

  // Initialize COM library
  // ZSucceeded(CoInitialize(NULL));

  ZVerify(g.perfshare.Initialize());
  //
  // Allocate perfmon counters for the server
  //
  g.pServerCounters = (SERVER_COUNTERS *)g.perfshare.AllocateCounters(
                      "AllSrv",
                      "0",    // if there are ever multiple servers running, change this
                      sizeof(SERVER_COUNTERS));
  memset(g.pServerCounters, 0x00, sizeof(SERVER_COUNTERS));

#if defined(USEAUTH)//!defined(ALLSRV_STANDALONE)
    g.pzas = CreateZoneAuthServer();
    if (!g.pzas)
    {
#if !defined(ALLSRV_STANDALONE)
      _AGCModule.TriggerEvent(NULL, AllsrvEventID_ZoneAuthServer, "", -1, -1, -1, 0);
#endif // !defined(ALLSRV_STANDALONE)
      return E_FAIL;
    }
#endif // defined(USEAUTH), formerly !defined(ALLSRV_STANDALONE)


  // create event used by fm to signal a message has arrived
  // let's try NOT signaling on this event, since it may signal us too often
  // g.hPlayerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  // assert(g.hPlayerEvent);

  // create event used to signal that the receive thread should exit
  g.hKillReceiveEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  assert(g.hKillReceiveEvent);

  g.idReceiveThread   = 0; // id of receive thread

  // create thread to receive player messages and start things cranking
  g.hReceiveThread = CreateThread(NULL, 0, ReceiveThread, 0, 0, &g.idReceiveThread);
  assert(g.hReceiveThread);

  ZVerify (TIMERR_NOERROR == timeBeginPeriod(1));

  return hr;
}

// ********** Game Site Implementation ************
class ThingSiteImpl : public ThingSite
{
    public:
        ThingSiteImpl(ImodelIGC*    pmodel)
        :
            m_pmodel(pmodel)            //Do not AddRef() ... lifespan < lifespan of model
        {
            ModelAttributes ma = pmodel->GetAttributes();
            m_bSideVisibility = (ma & c_mtSeenBySide) != 0;
            m_bPredictable = (ma & c_mtPredictable) != 0;
        }
        virtual ~ThingSiteImpl(void)
        {
        }

        virtual void    Terminate(void)
        {
        }

        void SetCluster(ImodelIGC* pmodel, IclusterIGC* pcluster)
        {
            assert (pmodel);
            if (m_bSideVisibility)
            {
                if (!m_bPredictable)
                {
                    IsideIGC*   psideModel = pmodel->GetSide();

                    assert (psideModel);
                    for (SideLinkIGC*   l = pmodel->GetMission()->GetSides()->first();
                         (l != NULL);
                         l = l->next())
                    {
                        IsideIGC*   pside = l->data();

						//imago ALLY 7/23/09
						if (psideModel->GetMission()->GetMissionParams()->bAllowAlliedViz) {
							 //You always lose contact when a non-static, non-allied object switches sectors
							if (pside != psideModel && !pside->AlliedSides(pside,psideModel)) {
                            	m_rgSideVisibility[pside->GetObjectID()].fVisible(false);
                            	HideObject(pside, pmodel);
							}
						} else {
							 //You always lose contact when a non-static object switches sectors
							if (pside != psideModel) {
                            	m_rgSideVisibility[pside->GetObjectID()].fVisible(false);
                            	HideObject(pside, pmodel);
							}

						}
                    }
                }

                if (pcluster)
                    UpdateSideVisibility(pmodel, pcluster);
            }
        }

        static  bool    ShowObject(IsideIGC*  pside, ImodelIGC*   pmodel)
        {
            assert (pside);
            assert (pmodel);
            bool    bSent = false;

            ObjectType  type = pmodel->GetObjectType();
            if (pmodel->GetAttributes() & c_mtPredictable)
            {
                CFSMission* pfsMission = (CFSMission *)(pside->GetMission()->GetPrivateData());
                assert (pfsMission);

                //Ships are never predictable
                assert (type != OT_ship);

                if (pside->GetActiveF() &&
                    ((pfsMission->GetStage() == STAGE_STARTED) ||
                     (pfsMission->GetStage() == STAGE_STARTING)))
                {
                    SideID  sideID = pside->GetObjectID();

                    // this is called when a stream of messages to a different recipient is in process of being queued-up
                    // so we will use the secondary (smaller 1k) outbox so as to not interrupt the main message stream
                    g.fm.UseMainOutBox(false);
					g.fm.SetDefaultRecipient(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED);                  

                    //Tell everyone on the side about the newly visible static object
                    bSent = true;
                    ExportObj(pmodel, type, NULL);



                    //perform a hack here: if a aleph becomes visible, make its corresponding aleph visible
                    if (type == OT_warp)
                    {
                        IwarpIGC*   pwarpOther = ((IwarpIGC*)pmodel)->GetDestination();
                        assert (pwarpOther);
                        ExportObj(pwarpOther, OT_warp, NULL);

						if (!pside->GetMission()->GetMissionParams()->bAllowAlliedViz) //ALLY VISIBILITY Imago 7/11/09
	                        assert (!pwarpOther->SeenBySide(pside));

	                        {
	                            //Force the visibility of this warp wrt the side to true without going through
	                            //SetSideVisibility ... do it via a ugly forced upcast
	                            ThingSiteImpl*  pts = (ThingSiteImpl*)(pwarpOther->GetThingSite());

	                            SideVisibility& sv = pts->m_rgSideVisibility[sideID];
	                            sv.fVisible(true);
	                        }

                        {
                            for (WarpBombLink* p = ((IwarpIGC*)pmodel)->GetBombs()->first(); (p != NULL); p = p->next())
                            {
                                BEGIN_PFM_CREATE(g.fm, pfmWB, S, WARP_BOMB)
                                END_PFM_CREATE
                                pfmWB->timeExplosion = p->data().timeExplosion;
                                pfmWB->warpidBombed = pmodel->GetObjectID();
                                pfmWB->expendableidMissile = p->data().pmt->GetObjectID();
                            }
                        }

                        {
                            for (WarpBombLink* p = pwarpOther->GetBombs()->first(); (p != NULL); p = p->next())
                            {
                                BEGIN_PFM_CREATE(g.fm, pfmWB, S, WARP_BOMB)
                                END_PFM_CREATE
                                pfmWB->timeExplosion = p->data().timeExplosion;
                                pfmWB->warpidBombed = pwarpOther->GetObjectID();
                                pfmWB->expendableidMissile = p->data().pmt->GetObjectID();
                            }
                        }
					}
                    else if (type == OT_asteroid)
                    {
                        //if under construction, send the building effect
                        IbuildingEffectIGC* pbe = ((IasteroidIGC*)pmodel)->GetBuildingEffect();
                        if (pbe)
                            ExportObj(pbe, OT_buildingEffect, NULL);
                    }
				
                    g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
                }
            }
            else if (type == OT_ship)
            {
                CFSShip* pfsShip = (CFSShip *)(((IshipIGC*)pmodel)->GetPrivateData());
                assert (pfsShip);
                pfsShip->ShipStatusSpotted(pside);
				if (pside->GetMission()->GetMissionParams()->bAllowAlliedViz) //ALLY VISIBILITY Imago 7/11/09
				{
					//lets get a list of allied sideIDs
				    for (SideLinkIGC* psidelink = pside->GetMission()->GetSides()->first();
						(psidelink != NULL);
						psidelink = psidelink->next())
					{
						IsideIGC*   otherside = psidelink->data();
						//this side is ally...and not ours
						if (pside->AlliedSides(pside,otherside) && !otherside->AlliedSides(pmodel->GetSide(),otherside) && !pmodel->SeenBySide(otherside)) {
							pfsShip->ShipStatusSpotted(otherside); //ALLY imago VISIBILITY 7/8/09
						}
					}
				}
            }
            g.fm.UseMainOutBox(true);

            return bSent;
        }

        static void HideObject(IsideIGC*  pside, ImodelIGC*   pmodel)
        {
            // let the team know that they can no longer see this ship
            if (pmodel->GetObjectType() == OT_ship)
            {
                CFSShip*    pfsShip = (CFSShip *)(((IshipIGC*)pmodel)->GetPrivateData());
                assert (pfsShip);

				//dont hide allies ships, ever, fixed 7/13/09 was && (oops) fixed for real 7/23/09
				if (pside->GetMission()->GetMissionParams()->bAllowAlliedViz) {
					if (!pside->AlliedSides(pside,pmodel->GetSide()) && pside != pmodel->GetSide()) {
                		pfsShip->ShipStatusHidden(pside);
					}
				} else {
					pfsShip->ShipStatusHidden(pside);
				}
            }
        }

        virtual void    UpdateSideVisibility(ImodelIGC*     pmodel,
                                             IclusterIGC*   pcluster)
        {
            assert (pmodel == m_pmodel);

            //We can only update it if we have one
            if (m_bSideVisibility)
            {
                ClusterSite*    pcs = pcluster->GetClusterSite();

                for (SideLinkIGC*   l = pcluster->GetMission()->GetSides()->first();
                     (l != NULL);
                     l = l->next())
                {
                    IsideIGC*   pside = l->data();

                    SideID  sid = pside->GetObjectID();
                    assert (sid >= 0);
                    assert (sid < c_cSidesMax);

                    SideVisibility& sv = m_rgSideVisibility[sid];
                    bool seenBefore = sv.fVisible();

                    //Update the visibility for everything, including visible static objects, so currentEyed gets set for He3 amount, etc.
                        /*if (seenBefore && m_bPredictable) {
                            //no need to change anything - wrong, need to hide He3
                        }*/
                        if (pside == pmodel->GetSide()) {
                            //Always see object on your own side (& call ShowObject() to export new stations, etc.)
                            if (!seenBefore) {
                                sv.fVisible(true);
                                sv.CurrentEyed(true);
                                ShowObject(pside, pmodel);
                            }
						} 
                        else if (pside->AlliedSides(pside,pmodel->GetSide()) && pside->GetMission()->GetMissionParams()->bAllowAlliedViz) {
							//Allied side
                            if (!seenBefore) {
                                sv.fVisible(true);
                                sv.CurrentEyed(true);
                                ShowObject(pside, pmodel);
                            }
						} 
                        else if (sv.pLastSpotter() && sv.pLastSpotter()->InScannerRange(pmodel)) { // No need to check if sv.pLastSpotter is allied, as only those get set as sv.pLastSpotter
                            if (!seenBefore) {
                                sv.fVisible(true);
                                sv.CurrentEyed(true);
                                ShowObject(pside, pmodel);
                            }
                            else if (!sv.CurrentEyed()) //only can be the case for m_bPredictable
                                sv.CurrentEyed(true);
						}
                        else {
                            //Not trivially seen ... loop over everything that could see it
                            sv.fVisible(false);
							sv.CurrentEyed(false); //Xynth #100 7/2010
                            for (ScannerLinkIGC*   l = pcs->GetScanners(sid)->first();
                                 (l != NULL);
                                 l = l->next())
                            {
                                IscannerIGC*   s = l->data();
                                assert (s->GetCluster() == pcluster);

								// If the thing is in scanner range
								if (s->InScannerRange(pmodel)) {
                                    sv.fVisible(true);
                                    sv.CurrentEyed(true);

									bool bSent = false;
                                    //See it
                                    if (!seenBefore) {
                                        bSent = ShowObject(pside, pmodel);

                                        //Check for spotted high value targets and award achievement/score
                                        if (pmodel->GetObjectType() == OT_ship) {
                                            ObjectType t;
                                            IshipIGC* pSpottedShip = (IshipIGC*)pmodel;
                                            //debugf("Ship %s spotted by %s.\n", GetModelName(pmodel), GetModelName(s));
                                            if (pSpottedShip && pSpottedShip->GetHullType()->GetData()) { // Guard for exploding ship without data
                                                AssetMask spottedMask = GetAssetMask(pSpottedShip, pSpottedShip->GetHullType(), false);
                                                const short hvMask = c_amEnemyMiner | c_amEnemyBuilder | c_amEnemyCapital | c_amEnemyCarrier | c_amEnemyBomber | c_amEnemyAPC | c_amEnemyTeleportShip;
                                                if (spottedMask & hvMask) {
                                                    if (!pSpottedShip->RecentlySpotted()) {
                                                        if (s->GetObjectType() == OT_probe) {
                                                            IprobeIGC* pProbe = (IprobeIGC*)s;
                                                            IshipIGC* pProbingShip = pProbe->GetProbeLauncherShip();
                                                            if (pProbingShip && pProbingShip->GetPilotType() >= c_ptPlayer) {
                                                                CFSShip* pfsShip = (CFSShip*)(pProbingShip->GetPrivateData());
                                                                PlayerScoreObject * pso = pfsShip->GetPlayerScoreObject();
                                                                pso->AddTargetSpot();
                                                                if (pfsShip->IsPlayer())
                                                                    pfsShip->GetPlayer()->GetSteamAchievements()->AwardIGCAchievements(c_achmProbeSpot);
                                                            }
                                                        }
                                                        else if (s->GetObjectType() == OT_ship) {
                                                            IshipIGC* spottingShip = (IshipIGC*)s;
                                                            if (spottingShip->GetPilotType() >= c_ptPlayer) {
                                                                CFSShip* pfsShip = (CFSShip*)(spottingShip->GetPrivateData());
                                                                PlayerScoreObject * pso = pfsShip->GetPlayerScoreObject();
                                                                pso->AddTargetSpot();
                                                            }
                                                        }
                                                        //mark hv ship as previously spotted, no matter by whom
                                                        pSpottedShip->MarkPreviouslySpotted();
                                                    }
                                                }
                                            }
                                        }
                                    }
									
                                    //Show it to allies who don't see it
									if (pside->GetMission()->GetMissionParams()->bAllowAlliedViz) //ALLY should be SCAN Imago 7/11/09
									{
										//lets get a list of allied sideIDs
									    for (SideLinkIGC* psidelink = pside->GetMission()->GetSides()->first();
											(psidelink != NULL);
											psidelink = psidelink->next())
										{
											IsideIGC*   otherside = psidelink->data();
											//this spotter's side is ally...and not ours
											if (s->GetSide()->AlliedSides(s->GetSide(),otherside) && s->GetSide() != otherside && !pmodel->SeenBySide(otherside) && bSent) {
												//printf("show object to ally via allied scanner\n");
												
												SideVisibility& sv2 = m_rgSideVisibility[otherside->GetObjectID()];
												sv2.fVisible(true);
												sv2.CurrentEyed(true); //Xynth #100 7/2010
												bool bAllySent = ShowObject(otherside, pmodel);
                                    			sv2.pLastSpotter(s);	

												ObjectType otModel = pmodel->GetObjectType();
												ObjectType  otSpotter = s->GetObjectType();
		                                        if (((bAllySent) && otModel == OT_station || otModel == OT_warp ||
		                                            (otModel == OT_asteroid &&
		                                             (static_cast<IasteroidIGC*>(pmodel)->GetCapabilities() & ~c_aabmBuildable) != 0)))
												{

	                                           		ObjectType  otSpotter = s->GetObjectType();
		                                            BEGIN_PFM_CREATE(g.fm, pfmObjectSpotted, S, OBJECT_SPOTTED)
		                                            END_PFM_CREATE
		                                            pfmObjectSpotted->otObject = otModel;
		                                            pfmObjectSpotted->oidObject = pmodel->GetObjectID();
		                                            pfmObjectSpotted->otSpotter = otSpotter;
		                                            pfmObjectSpotted->oidSpotter = s->GetObjectID();														
													//printf("sending object spotted message to ally\n");
													g.fm.SendMessages(CFSSide::FromIGC(otherside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
												}
											}
										}
									}	

                                    //Keep track of who spotted it
                                    sv.pLastSpotter(s);

                                    if (bSent)
                                    {
                                        // if it's a station, aleph, special asteroid or helium asteroid...
                                        ObjectType otModel = pmodel->GetObjectType();									
                                        if ((otModel == OT_station || otModel == OT_warp ||
                                            (otModel == OT_asteroid &&
                                             (static_cast<IasteroidIGC*>(pmodel)->GetCapabilities() & ~c_aabmBuildable) != 0))) //Imago - Review - what about mineable?! 7/10
										{
                                            ObjectType  otSpotter = s->GetObjectType();
											//Imago #121
											if (otModel == OT_station) {
                                                pside->UpdateTerritory();
                                                pside->HandleNewEnemyCluster(pmodel->GetCluster());

												ObjectID roidID = static_cast<IstationIGC*>(pmodel)->GetRoidID();
												//asteroid to silently pop
												if (roidID != NA) {
													BEGIN_PFM_CREATE(g.fm, pfmAsteroidDestroyed, S, ASTEROID_DESTROYED)
													END_PFM_CREATE
													pfmAsteroidDestroyed->clusterID =  pmodel->GetCluster()->GetObjectID();
													pfmAsteroidDestroyed->asteroidID = roidID;
													pfmAsteroidDestroyed->explodeF = false;
													g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
												}
											}

                                            // tell the side who spotted it
                                            BEGIN_PFM_CREATE(g.fm, pfmObjectSpotted, S, OBJECT_SPOTTED)
                                            END_PFM_CREATE
                                            pfmObjectSpotted->otObject = otModel;
                                            pfmObjectSpotted->oidObject = pmodel->GetObjectID();
                                            pfmObjectSpotted->otSpotter = otSpotter;
                                            pfmObjectSpotted->oidSpotter = s->GetObjectID();
																						
                                            g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH); 

											CFSMission* pfsMission = (CFSMission *)(pside->GetMission()->GetPrivateData());																		
											
											if (otModel == OT_station && pmodel != nullptr)
											{
												IstationIGC* pstation = ((IstationIGC*)pmodel);
												const IstationTypeIGC *pStationType = pstation->GetStationType();

												if (pStationType != nullptr && 
													(
														(pstation->GetStationType()->GetClassID() == c_scGarrison) ||
														(pstation->GetStationType()->GetClassID() == c_scStarbase)
													))
												{
													if (s != nullptr && s->GetMission() != nullptr && s->GetObjectType() == OT_ship)
													{
														ObjectID spotterID = s->GetObjectID();
														ImodelIGC* pShipModel = s->GetMission()->GetModel(OT_ship, spotterID);

														if (pShipModel != nullptr)
														{
															IshipIGC* pShip = (IshipIGC*)pShipModel;
															pShip->SetAchievementMask(c_achmGarrSpot);
														}
													}													
												}
											}


                                            if ((otModel == OT_warp) || (otModel == OT_asteroid))
                                            {
                                                if (otSpotter == OT_ship)
                                                {
                                                    IshipIGC*           pship = (IshipIGC*)s;
                                                    CFSShip*            pfsship = (CFSShip*)(pship->GetPrivateData());
                                                    PlayerScoreObject*  ppso = pfsship->GetPlayerScoreObject();

                                                    if (otModel == OT_warp)
                                                        ppso->SpotWarp();
                                                    else
                                                    {
                                                        assert (otModel == OT_asteroid); 
                                                        ppso->SpotSpecialAsteroid(); //Review - Changes for visibility of static objects (#135) may now require roid to be checked for abilities! -Imago 8/10

                                                    }
                                                }
                                            }
                                        }
									}

                                    break;
                                }
                            }

                            if (!sv.fVisible() && seenBefore) {
                                // hide it
								sv.CurrentEyed(false); //Xynth #100 7/2010
                                sv.pLastSpotter(NULL);
                                if (m_bPredictable) //keep static objects visible (but not currentEyed)
                                    sv.fVisible(true);
                                else
                                    HideObject(pside, pmodel);
                            }
                        }                    
                }
            }
		}
        virtual bool    GetSideVisibility(IsideIGC* pside)
        {
            assert (pside != NULL);
            return m_bSideVisibility ? m_rgSideVisibility[pside->GetObjectID()].fVisible() : true;
        }

		//Xynth #225 9/10 GetCurrentEye was not implemented for server side use
		virtual bool  GetCurrentEye(IsideIGC* pside)
		{
			assert (pside != NULL);
			return m_bSideVisibility ? m_rgSideVisibility[pside->GetObjectID()].CurrentEyed() : false;
		}

        virtual void    SetSideVisibility(IsideIGC* pside,
                                          bool      fVisible)
        {
            assert (pside);
            if (m_bSideVisibility)
            {
                SideID  sid = pside->GetObjectID();
                SideVisibility& sv = m_rgSideVisibility[sid];

                if (fVisible != sv.fVisible())
                {
                    sv.fVisible(fVisible);
                    if (fVisible)
                        ShowObject(pside, m_pmodel);
                    else
                        HideObject(pside, m_pmodel);
                }
				sv.CurrentEyed(fVisible);  //Xynth #100 7/2010

                if (fVisible && m_pmodel->GetObjectType() == OT_station) { //Not usually going to happen
                    pside->UpdateTerritory(); 
                }
            }
        }

    private:
        SideVisibility  m_rgSideVisibility[c_cSidesMax];
        ImodelIGC*      m_pmodel;
        bool            m_bPredictable;
        bool            m_bSideVisibility;
};

class   ClusterSiteImpl : public ClusterSite
{
    public:
        ClusterSiteImpl(IclusterIGC * pcluster)
        :
            m_pcluster(pcluster)
        {
            CFSMission * pfsMission = (CFSMission *) pcluster->GetMission()->GetPrivateData();
        }

        virtual void                    AddScanner(SideID   sid, IscannerIGC* scannerNew)
        {
            assert (sid >= 0);
            assert (sid < c_cSidesMax);
            assert (scannerNew);

            AddIbaseIGC((BaseListIGC*)&(m_scanners[sid]), scannerNew);
        }
        virtual void                    DeleteScanner(SideID   sid, IscannerIGC* scannerOld)
        {
            assert (sid >= 0);
            assert (sid < c_cSidesMax);
            assert (scannerOld);

            DeleteIbaseIGC((BaseListIGC*)&(m_scanners[sid]), scannerOld);
        }
        virtual const ScannerListIGC*      GetScanners(SideID   sid) const
        {
            assert (sid >= 0);
            assert (sid < c_cSidesMax);

            return &(m_scanners[sid]);
        }

        virtual void Terminate()
        {
          ((CFSMission *)m_pcluster->GetMission()->GetPrivateData())->DeleteCluster(m_pcluster); // destroy FS cluster
          ClusterSite::Terminate();
        }

    private:
        IclusterIGC*    m_pcluster;
        ScannerListIGC  m_scanners[c_cSidesMax];
};

TRef<ThingSite> FedSrvSiteBase::CreateThingSite(ImodelIGC* pModel)
{
  return new ThingSiteImpl(pModel);
}

TRef<ClusterSite> FedSrvSiteBase::CreateClusterSite(IclusterIGC* pCluster)
{
  m_pfsMission->CreateCluster(pCluster);   // make a FS cluster
  return new ClusterSiteImpl(pCluster);
}

void    FedSrvSiteBase::UpgradeDrones(IsideIGC* pside)
{

    {
        //Upgrade any drones docked at the station
        for (StationLinkIGC*    pslStation = pside->GetStations()->first();
             (pslStation != NULL);
             pslStation = pslStation->next())
        {
            IstationIGC*    pstation = pslStation->data();

            for (ShipLinkIGC*   pslShip = pstation->GetShips()->first();
                 (pslShip != NULL);
                 pslShip = pslShip->next())
            {
                IshipIGC*   pship = pslShip->data();
                CFSShip*    pfsship = (CFSShip*)(pship->GetPrivateData());

                if (!pfsship->IsPlayer())
                    UpgradeShip(pship, pstation);
            }
        }
    }
}

void FedSrvSiteBase::DevelopmentCompleted(IbucketIGC * pbucket, IdevelopmentIGC * pdev, Time now)
{
  IsideIGC * pside = pbucket->GetSide();
  SideID sideid = pside->GetObjectID();

  pside->ApplyDevelopmentTechs(pdev->GetEffectTechs());

  BEGIN_PFM_CREATE(g.fm, pfmDevCompleted, S, DEV_COMPLETED)
  END_PFM_CREATE
  pfmDevCompleted->devId = pdev->GetObjectID();
  g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);

  if (!pdev->GetTechOnly())
      pside->ApplyGlobalAttributeSet(pdev->GetGlobalAttributeSet());

  if (pdev->GetObjectID() == c_didTeamMoney)
  {
      static char szReason[100];
      sprintf(szReason, "%s has proven the economic value of their outpost.", pside->GetName());
      m_pfsMission->GameOver(pside, szReason);
  }
}

void FedSrvSiteBase::SideDevelopmentTechChange(IsideIGC*    pside)
{
    assert (pside);

    if (m_pfsMission->GetStage() == STAGE_STARTED)
    {
        SideID sideid = pside->GetObjectID();
        const TechTreeBitMask&  ttbmDev = pside->GetDevelopmentTechs();

        BEGIN_PFM_CREATE(g.fm, pfmSideTechChange, S, SIDE_TECH_CHANGE)
        END_PFM_CREATE
        pfmSideTechChange->sideID           = sideid;
        pfmSideTechChange->ttbmDevelopments = ttbmDev;
        g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
    }

    for (StationLinkIGC*    pslStation = pside->GetStations()->first();
         (pslStation != NULL);
         pslStation = pslStation->next())
    {
        IstationIGC*        pstation = pslStation->data();

        IstationTypeIGC*    pstOld       = pstation->GetBaseStationType();
        IstationTypeIGC*    pstSuccessor = pstOld->GetSuccessorStationType(pside);
        if (pstOld != pstSuccessor)
        {
            pstation->SetName(pstSuccessor->GetName());
            pstation->SetBaseStationType(pstSuccessor);
        }
    }

    UpgradeDrones(pside);
}

void FedSrvSiteBase::SideGlobalAttributeChange(IsideIGC*    pside)
{
    assert (pside);

    BEGIN_PFM_CREATE(g.fm, pfmSideAttrChange, S, SIDE_ATTRIBUTE_CHANGE)
    END_PFM_CREATE
    pfmSideAttrChange->sideID        = pside->GetObjectID();
    pfmSideAttrChange->gasAttributes = pside->GetGlobalAttributeSet();
    g.fm.SendMessages(m_pfsMission->GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
}

void FedSrvSiteBase::HullTypeCompleted(IsideIGC* pside, IhullTypeIGC* pht)
{
    pside->AddToStockpile(pht);

    //NYI send messages
}

void FedSrvSiteBase::PartTypeCompleted(IsideIGC* pside, IpartTypeIGC* ppt)
{
    pside->AddToStockpile(ppt, IlauncherTypeIGC::IsLauncherType(ppt->GetEquipmentType())
                               ? ppt->GetAmount(NULL)
                               : 1);

    //NYI send messages
}

void FedSrvSiteBase::StationTypeCompleted(IbucketIGC * pbucket, IstationIGC* pstation, IstationTypeIGC * pstationtype, Time now)
{
    //NYI decide on the correct hack for the placement of space stations
    IsideIGC * pside = pbucket->GetSide();
    assert (pside);
    if (pside->GetActiveF())
    {
        //Hack alert
        CFSDrone * pfsDrone = CreateStockDrone(pstationtype->GetConstructionDroneType(), pstation->GetSide(), pstationtype);
        assert (pfsDrone);

        pfsDrone->GetIGCShip()->SetStation(pstation);   // start them at a station just like everyone else
        pfsDrone->GetIGCShip()->PickDefaultOrder(pstation->GetCluster(), pstation->GetPosition(), true);
        pfsDrone->GetIGCShip()->SetStation(NULL);       //but quickly undock them

        //Don't auto-build
        if (pfsDrone->GetIGCShip()->GetCommandID(c_cmdCurrent) == c_cidBuild) {
            ImodelIGC* targetRock = pfsDrone->GetIGCShip()->GetCommandTarget(c_cmdCurrent);
            // Queue doing nothing
            pfsDrone->GetIGCShip()->SetCommand(c_cmdAccepted, NULL, c_cidDoNothing);
            // move to the possible rock
            pfsDrone->GetIGCShip()->SetCommand(c_cmdPlan, targetRock, c_cidGoto);
            pfsDrone->GetIGCShip()->SetCommand(c_cmdCurrent, targetRock, c_cidGoto);
        }

        // request a rock.
        SendChat(pfsDrone->GetIGCShip(), CHAT_TEAM, pfsDrone->GetIGCShip()->GetSide()->GetObjectID(),
            pstationtype->GetConstructorNeedRockSound(), "New constructor requesting asteroid.");
    }
}

void FedSrvSiteBase::BuildStation(IasteroidIGC* pasteroid,
                              IsideIGC*         pside,
                              IstationTypeIGC*  pstationtype,
                              Time              now,
							  bool pbseensides[]) //Imago bool (&bseensides)[c_cSidesMax] #120 8/10
{
    //Allow for an upgrade to the station being available
    pstationtype = pstationtype->GetSuccessorStationType(pside);

    ImissionIGC*    pmission = m_pfsMission->GetIGCMission();
    IclusterIGC*    pcluster = pasteroid->GetCluster();

    DataStationIGC  ds;
    strcpy(ds.name, pstationtype->GetName());
    ds.clusterID  = pcluster->GetObjectID();
    ds.position   = pasteroid->GetPosition();
    ds.forward.x  = ds.forward.y = 0.0f;
    ds.forward.z  = 1.0f;
    ds.up.x       = ds.up.z = 0.0f;
    ds.up.y       = 1.0f;
    ds.rotation.axis(ds.forward);
    ds.rotation.angle(0.0f);
    ds.sideID = pside->GetObjectID();
    ds.stationID = pmission->GenerateNewStationID();

    ds.stationTypeID = pstationtype->GetObjectID();

    ds.bpHull = pasteroid->GetFraction();
    ds.bpShield = 0.0f;

	if (pcluster->GetProbes()) { //Imago 7/10 - if there are probes in this sector...
		//Xynth - Begin change to destroy probes too close to rock and would otherwise be lodged in base 7/10
		//ticket #10
		for (ProbeLinkIGC*  ppl = pcluster->GetProbes()->first(); (ppl != NULL); ppl = ppl->next())
		{
			IprobeIGC*  pprobe = ppl->data();
			Vector newPos = pprobe->GetPosition();
			Vector dxy = pprobe->GetPosition() - pasteroid->GetPosition();
			float l2 = dxy.LengthSquared();
			float rOffset = pstationtype->GetRadius() + pprobe->GetRadius();
			/*debugf("Probe xyz %f %f %f\n",newPos.X(), newPos.Y(), newPos.Z());
			debugf("Station vec %f %f %f\n",ds.position.X(), ds.position.Y(), ds.position.Z());
			debugf("radii %f %f\n", pstationtype->GetRadius(),  pprobe->GetRadius());
			debugf("l2 rOffset %f %f\n", l2, rOffset);*/
			if ((l2 < rOffset * rOffset) && (l2 != 0.0f))
			{				
				pprobe->SetExpiration(now + 1.0);
				//debugf("kill probe");							
			}
		}
		
	} //Xynth/Imago - End change

	//Imago #120 #121 
	const SideListIGC * plistSide0 = pmission->GetSides();
	for (SideLinkIGC * plinkSide = plistSide0->first(); plinkSide; plinkSide = plinkSide->next()) {
		IsideIGC * potherside = plinkSide->data();
		if (pside == potherside || pasteroid->GetAsteroidCurrentEye(potherside) || pasteroid->GetBuilderSeenSide(potherside->GetObjectID()) || pbseensides[potherside->GetObjectID()]) {
			KillAsteroidEvent(pasteroid, potherside, false);
		}
 	}
 	//

    IstationIGC * pstationNew = (IstationIGC *) (pmission->CreateObject(now, OT_station, &ds, sizeof(ds)));
    assert (pstationNew->SeenBySide(pside)); //Sides always see their own station (which will force the export)

	//#121 Imago 7/10
	pstationNew->SetRoidID(pasteroid->GetObjectID());

	const SideListIGC * plistSide1 = pmission->GetSides();
	for (SideLinkIGC * plinkSide = plistSide1->first(); plinkSide; plinkSide = plinkSide->next()) {
		IsideIGC * potherside = plinkSide->data();
		if (pside != potherside && !pasteroid->GetAsteroidCurrentEye(potherside) && !pasteroid->GetBuilderSeenSide(potherside->GetObjectID()) && !pbseensides[potherside->GetObjectID()] && pstationNew->SeenBySide(plinkSide->data())) {
			KillAsteroidEvent(pasteroid, potherside, false);
		} else if (pside != potherside && !pasteroid->GetAsteroidCurrentEye(potherside) && !pasteroid->GetBuilderSeenSide(potherside->GetObjectID()) && !pbseensides[potherside->GetObjectID()] && !pstationNew->SeenBySide(plinkSide->data())) {
			pstationNew->SetRoidSig(pasteroid->GetSignature());
			pstationNew->SetRoidPos(pasteroid->GetPosition());
			pstationNew->SetRoidRadius(pasteroid->GetRadius());
			pstationNew->SetRoidCaps(pasteroid->GetCapabilities());
			pstationNew->SetRoidSide(potherside->GetObjectID());
 		}
	}
	//
	pasteroid->Terminate();
	//
	// TE: Fire AGCEvent informing of station created
	 LPCSTR pszContext = m_pfsMission->GetIGCMission() ? m_pfsMission->GetIGCMission()->GetContextName() : NULL; // this is a timestamp.
	 _AGCModule.TriggerContextEvent(NULL, EventID_StationCreated, pszContext,
						pside->GetName(), pside->GetObjectID(), -1, -1, 4,
						"MissionID", VT_I4, m_pfsMission->GetMissionID(),
						"TeamID"    , VT_I4   , pside->GetObjectID(),
						"TeamName", VT_LPSTR, pside->GetName(),
						"zStationName", VT_LPSTR, pstationNew->GetName());
    // TE end

    // yp: Add event for players who escorted a constructor
    // mmf commented this out for now pending more testing
#if 0
	 ZString pszPlayerList = ""; // this creates a new ZString object and set its value to "", it's not a pointer to ""
	if(pmission->GetShips() != NULL && pside)	// GetShips() returns ShipListIGC*
	{
		ShipLinkIGC * pShiplink = pmission->GetShips()->first();
		while (pShiplink)
		{
			CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
			if (pfsShip != NULL & pfsShip->IsPlayer())
			{
				if(pfsShip->GetSide() && pfsShip->GetSide()->GetObjectID() == pside->GetObjectID() && // if they are on 'this' side. and..
					pfsShip->GetCluster()->GetObjectID() == pcluster->GetObjectID() && pfsShip->GetPlayer()) // they are in this sector
				{
					pszPlayerList = pszPlayerList + ";" + ZString(pfsShip->GetPlayer()->GetName()); // players name
					// The distance the player is from the asteroid that the constructor built into.
					if(pfsShip->GetPlayer()->GetIGCShip())
					{
						pszPlayerList = pszPlayerList +  ":" + ZString( (pasteroid->GetPosition() - pfsShip->GetPlayer()->GetIGCShip()->GetPosition()).Length()); // the distance?
					}
				}
			}
			pShiplink = pShiplink->next();
		}
	}
	 // Fire AGCEvent listing players in the sector
	_AGCModule.TriggerContextEvent(NULL, EventID_StationCreatedEscort, pszContext,
						pside->GetName(), pside->GetObjectID(), -1, -1, 4,
						"MissionID", VT_I4, m_pfsMission->GetMissionID(),
						"Team"    , VT_I4   , pside->GetObjectID(), // teamID the con was on.
						"TeamName", VT_LPSTR, pside->GetName(), // team the con was on.
						"zPlayerList", VT_LPSTR, pszPlayerList); // pszPlayerList should look like ";player@squad:1500;player2@squad:500"
	// YP: end
#endif

    pstationNew->Release();

    UpgradeDrones(pside);
    //Possibly the built themselves to a victory
    IsideIGC*   psideWin = m_pfsMission->CheckForVictoryByStationBuild(pside);
    if (psideWin)
    {
        static char szReason[100];     //Make this static so we only need to keep a pointer to it around
        sprintf(szReason, "%s won by out-building their opponents", psideWin->GetName());
        m_pfsMission->GameOver(psideWin, szReason);
    }
}

void FedSrvSiteBase::DroneTypeCompleted(IbucketIGC * pbucket, IstationIGC* pstation, IdroneTypeIGC * pdronetype, Time now)
{
  IsideIGC* pside = pstation->GetSide();
  if (pside->GetActiveF())
  {
      assert (pdronetype->GetPilotType() != c_ptBuilder);
      CFSDrone * pfsDrone = CreateStockDrone(pdronetype, pside, (pdronetype->GetPilotType() == c_ptLayer)
                                                                ? pdronetype->GetLaidExpendable()
                                                                : NULL);

      if (pfsDrone)
      {
        pfsDrone->GetIGCShip()->SetStation(pstation);  //start them at a station just like everyone else
        pfsDrone->GetIGCShip()->PickDefaultOrder(pstation->GetCluster(), pstation->GetPosition(), true);
        pfsDrone->GetIGCShip()->SetStation(NULL);      //but undock them immediately

        // Tell everyone on your side that you got one of these things
        // Can't send to team until ISideIGC keeps track of ships on a side
        if (pdronetype->GetPilotType() == c_ptLayer)
        {
            if (pfsDrone->GetIGCShip()->GetBaseData()->GetObjectType() == OT_mineType)
                SendChat(pfsDrone->GetIGCShip(), CHAT_TEAM, pfsDrone->GetIGCShip()->GetSide()->GetObjectID(),
                    droneWhereToLayMinefieldSound, "New minefield requesting location.");
            else
                SendChat(pfsDrone->GetIGCShip(), CHAT_TEAM, pfsDrone->GetIGCShip()->GetSide()->GetObjectID(),
                    droneWhereToLayTowerSound, "New tower requesting location.");
        }
        else if ((pdronetype->GetPilotType() == c_ptMiner) && (pfsDrone->GetIGCShip()->GetCommandID(c_cmdCurrent) != c_cidMine))
        {
            SendChat(pfsDrone->GetIGCShip(), CHAT_TEAM, pfsDrone->GetIGCShip()->GetSide()->GetObjectID(),
                droneWhereToSound, "New miner requesting He3 asteroid.");
        }
      }
  }
}

void FedSrvSiteBase::UpgradeShip(IshipIGC* pship, const IstationIGC*  pstation)
{
    assert (pship->GetParentShip() == NULL);
    assert (pship->GetStation() == pstation);

    //See if anything on the ship can be upgraded
    IhullTypeIGC* pht = pship->GetBaseHullType();
    assert (pht);
    IhullTypeIGC* phtSuccessor = (IhullTypeIGC*)(pstation->GetSuccessor(pht));

    if (pht != phtSuccessor)
    {
        //Nuke all the parts
        {
            const PartListIGC*  pparts = pship->GetParts();
            PartLinkIGC*        ppl;
            while (ppl = pparts->first())
                ppl->data()->Terminate();
        }
        pship->SetBaseHullType(phtSuccessor);

        //Buy a default loadout
        {
            for (PartTypeLinkIGC*   ptl = phtSuccessor->GetPreferredPartTypes()->first();
                 (ptl != NULL);
                 ptl = ptl->next())
            {
                IpartTypeIGC*   ppt = ptl->data();
                if (pstation->CanBuy(ppt))
                {
                    ppt = (IpartTypeIGC*)(pstation->GetSuccessor(ppt));

                    //Mount the part anyplace it can be mounted. Ignore price (included in the cost of the drone)
                    EquipmentType   et = ppt->GetEquipmentType();
                    Mount           mountMax = (et == ET_Weapon)
                                               ? phtSuccessor->GetMaxWeapons()
                                               : 1;

                    for (Mount i = 0; (i < mountMax); i++)
                    {
                        if ((pship->GetMountedPart(et, i) == NULL) && phtSuccessor->CanMount(ppt, i))
                            pship->CreateAndAddPart(ppt, i, 0x7fff);
                    }
                }
            }
        }
    }
    else
    {
        //Check each of the parts. Work backwards through the list
        //since new parts will be added to the end
        PartLinkIGC*    pplPart = pship->GetParts()->last();
        while (pplPart)
        {
            IpartIGC*   ppart = pplPart->data();
            pplPart = pplPart->txen();

            if (ppart->GetMountID() < 0)
                ppart->Terminate();             //Trash anything in cargo
            else
            {
                IpartTypeIGC*   ppt = ppart->GetPartType();
                IpartTypeIGC*   pptSuccessor = (IpartTypeIGC*)(pstation->GetSuccessor(ppt));

                if (pptSuccessor != ppt)
                {
                    Mount mount = ppart->GetMountID();

                    ppart->Terminate();
                    pship->CreateAndAddPart(pptSuccessor, mount, 0x7fff);
                }
            }
        }
    }
}

void FedSrvSiteBase::TerminateModelEvent(ImodelIGC*  pmodel)
{
}

void FedSrvSiteBase::KillShipEvent(Time timeCollision, IshipIGC* pship, ImodelIGC* pcredit, float amount, const Vector& p1, const Vector& p2)
{
  ImissionIGC*          pmission = m_pfsMission->GetIGCMission();
  const MissionParams*  pmp = pmission->GetMissionParams();
  IsideIGC*             pside = pship->GetSide();

  IclusterIGC*  pcluster = pship->GetCluster();
  IshipIGC*     pshipParent = pship->GetParentShip();

  DamageBucketLink* pdmglink = NULL;
  if (pshipParent == NULL)
  {
    //Transfer credit to the deserving individual, if we are tracking that sort of thing
    DamageTrack*  pdt = pship->GetDamageTrack();
    if (pdt)
    {
      pdmglink = pdt->GetDamageBuckets()->first();
      if (pdmglink)
      {
          if (pdmglink->data()->model()->GetMission() == pmission)
            pcredit = pdmglink->data()->model();

          DamageBucketLink* pblAssist = pdmglink->next();
          while (pblAssist)
          {
            ImodelIGC*  pAssist = pblAssist->data()->model();
            if ((pAssist->GetObjectType() == OT_ship) &&
                ( (pAssist->GetSide() != pside) && !pside->AlliedSides(pside,pAssist->GetSide()) ) && // #ALLY -imago 7/3/09 kgjv
                (pAssist->GetMission() == pmission))
            {
                CFSShip* pfsAssist = (CFSShip*)( ((IshipIGC*)pAssist)->GetPrivateData() );

                if (pfsAssist->IsPlayer())
                {
                    pfsAssist->GetPlayerScoreObject()->AddAssist();
                    break;
                }
            }

            pblAssist = pblAssist->next();
          }
      }
    }

    //If we die, then all of our children die.
    const ShipListIGC*  pshipsChildren = pship->GetChildShips();
    ShipLinkIGC*    psl;
    while (psl = pshipsChildren->first())   //Intentional assignment
    {
      KillShipEvent(timeCollision, psl->data(), pcredit, amount, p1, p2);
    }

      //Dead ship ... create treasures for all of its parts
    const PartListIGC*  plist = pship->GetParts();
    PartLinkIGC*    plink;
    while (plink = plist->first())  //Not ==
    {
      IpartIGC*   p = plink->data();

      if (randomInt(0, 4) == 0)
        CreateTreasure(timeCollision, pship, p, p->GetPartType(), p1, 100.0f, 60.0f);
      p->Terminate();
    }

    //No ammo or fuel either
    //Treasures for ammo
    {
        short ammo = pship->GetAmmo();
        if (ammo > 0)
        {
            IpartTypeIGC*  pptAmmo = pmission->GetAmmoPack();
            assert (pptAmmo);

            if (randomInt(0, 4) == 0)
            {
                CreateTreasure(timeCollision, pship, ammo, pptAmmo, p1, 100.0f, 30.0f);
            }
            pship->SetAmmo(0);
        }
    }
    //Ditto for fuel
    {
        short fuel = short(pship->GetFuel());
        if (fuel > 0)
        {
            IpartTypeIGC*  pptFuel = pmission->GetFuelPack();
            assert (pptFuel);

            if (randomInt(0, 4) == 0)
            {
                CreateTreasure(timeCollision, pship, fuel, pptFuel, p1, 100.0f, 30.0f);
            }
        }
        pship->SetFuel(0.0f);
    }

    //Jetison flags
    {
        SideID sidFlag = pship->GetFlag();
        if (sidFlag != NA)
        {
            pship->SetFlag(NA);

            const Vector&   p = pship->GetPosition();
            float           lm = pmission->GetFloatConstant(c_fcidLensMultiplier);
            float           r = 1.5f * pmission->GetFloatConstant(c_fcidRadiusUniverse);
            if (p.x*p.x + p.y*p.y + p.z*p.z/(lm*lm) > r*r)
            {
                RespawnFlag(sidFlag,NULL); // mmf 11/07 added second argument pside (which is set to null in this case)
            }
            else
            {
                DataTreasureIGC dt;
                dt.treasureCode = c_tcFlag;
                dt.treasureID = sidFlag;
                dt.amount = 0;
                dt.lifespan = 3600.0f * 24.0f * 10.0f;      //10 days
                dt.createNow = false;
                CreateTreasure(timeCollision, pship, &dt, p1, 100.0f);
            }

            BEGIN_PFM_CREATE(g.fm, pfmGain, S, GAIN_FLAG)
            END_PFM_CREATE
            pfmGain->sideidFlag = NA;
            pfmGain->shipidRecipient = pship->GetObjectID();
			pfmGain->bIsTreasureDocked = false; // KGJV #118
            g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);
        }
    }
  }

  CFSShip * pfsShip = (CFSShip *) pship->GetPrivateData();

  ShipID shipID = pship->GetObjectID();
  //Adjust side moneys for the forced ejection or eject pod kill

  bool  bKilled;
  bool  bLifepod;
  if (pfsShip->IsPlayer())
  {
    pfsShip->GetPlayer()->SetTreasureObjectID(NA);

    bLifepod = (pshipParent == NULL) &&
               pship->GetBaseHullType()->HasCapability(c_habmLifepod);

    bKilled = (!pmp->bEjectPods) || bLifepod || (amount < 0.0f);
  }
  else
  {
    bKilled = true;
    bLifepod = false;
  }

  if (bKilled)
        pship->SetExperience(0.0f);

  IshipIGC*   pshipCredit = NULL;
  if (amount != 0.0f)
  {
    BEGIN_PFM_CREATE(g.fm, pfmKillShip, S, KILL_SHIP)
    END_PFM_CREATE
    pfmKillShip->shipID = shipID;

    pfmKillShip->bKillCredit = false;
    pfmKillShip->bDeathCredit = bKilled;

    pfmKillShip->sideidKiller = NA;
    if (pcredit)
    {
        ObjectType  type = pcredit->GetObjectType();

        IsideIGC*   psideCredit = pcredit->GetSide();
        if (psideCredit)
        {
            //Killed by something that belongs to a side ... award appropriate money
            pfmKillShip->sideidKiller = psideCredit->GetObjectID();

            if ((!bLifepod) && (psideCredit != pside))
            {
                //Didn't slay a helpless eject pod or a team mate... award kill credit
                pfmKillShip->bKillCredit = true;

                if (type == OT_ship)
                {
                    CFSShip*    pfsshipCredit = (CFSShip*)(((IshipIGC*)pcredit)->GetPrivateData());
                    pfsshipCredit->AddKill();

                    PlayerScoreObject::AdjustCombatRating(pmission, pfsshipCredit->GetPlayerScoreObject(), pfsShip->GetPlayerScoreObject());

                    if ( (pside != psideCredit) && !pside->AlliedSides(pside,psideCredit) ) // #ALLY: kgjv dont up KB when FF is on - Imago 7/3/09
                        ((IshipIGC*)pcredit)->AddExperience();

                    if (pdmglink)
                    {
                        //Award score credit for partial kills to anyone who did damage in the damage track
                        float totalDamage = 0.0f;
                        {
                            for (DamageBucketLink* l = pdmglink; (l != NULL); l = l->next())
                            {
                                if (l->data()->model()->GetSide() != pside)
                                    totalDamage += l->data()->totalDamage();
                            }
                        }
                        if (totalDamage != 0.0f)
                        {
                            for (DamageBucketLink* l = pdmglink; (l != NULL); l = l->next())
                            {
                                ImodelIGC*    pmodel = l->data()->model();
                                if ((pmodel->GetObjectType() == OT_ship) &&
                                    ( (pmodel->GetSide() != pside) && !pside->AlliedSides(pside,pmodel->GetSide()) ) && // #ALLY -imago 7/3/09 kgjv
                                    (pmodel->GetMission() == pmission))
                                {
                                    IshipIGC*   pshipCredit = (IshipIGC*)pmodel;
                                    CFSShip*    pfsShip = (CFSShip*)(pshipCredit->GetPrivateData());
                                    pfsShip->GetPlayerScoreObject()->KillShip(pship, l->data()->totalDamage() / totalDamage);

                                    IshipIGC*   pshipParent = pshipCredit->GetParentShip();
                                    if (pshipParent)
                                    {
                                        IhullTypeIGC* phtParent = pshipParent->GetBaseHullType();
                                        assert (phtParent);
                                        float   divisor = 2.0f + float(phtParent->GetMaxWeapons() - phtParent->GetMaxFixedWeapons());

                                        CFSShip*  pfsParent = (CFSShip*)(pshipParent->GetPrivateData());
                                        pfsParent->GetPlayerScoreObject()->KillShip(pship,
                                                                                    l->data()->totalDamage() /
                                                                                    (totalDamage * divisor));
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    psideCredit->AddKill();
                }

                IsideIGC*   psideWin = m_pfsMission->CheckForVictoryByKills(psideCredit);
                if (psideWin)
                {
                    static char szReason[100];     //Make this static so we only need to keep a pointer to it around
                    if (pshipCredit)
                        sprintf(szReason, "%s pushes %s over the top with a kill", pshipCredit->GetName(), psideWin->GetName());
                    else
                        sprintf(szReason, "%s won by killing enough of their opponents", psideWin->GetName());
                    m_pfsMission->GameOver(psideWin, szReason);
                }
            }
				//Xynth #8 Check for achievements from ship destruction
				if (pcredit)
				{
					ObjectType  type = pcredit->GetObjectType();
					if (type == OT_ship)
					{
						CFSShip*    pfsshipCredit = (CFSShip*)(((IshipIGC*)pcredit)->GetPrivateData());
						if (pfsshipCredit->IsPlayer())
						{
							if (pcredit->GetSide()->GetNumPlayersOnSide() > 4 && pship->GetSide()->GetNumPlayersOnSide() > 4)
							{
								if (!bLifepod)
								{
									PilotType pt = pship->GetPilotType();

									CSteamAchievements *pSteamAchievements = pfsshipCredit->GetPlayer()->GetSteamAchievements();
									pSteamAchievements->AwardKillAchievement(pt);
									pSteamAchievements->SaveStats();
								}
							}
						}
					}
				}
        }

        pfmKillShip->typeCredit = type;
        pfmKillShip->idCredit = pcredit->GetObjectID();
    }
    else
        pfmKillShip->typeCredit = pfmKillShip->idCredit   = NA;

    g.fm.SendMessages(m_pfsMission->GetGroupMission(), FM_GUARANTEED, FM_FLUSH);
  }

  IhullTypeIGC*   pht = pside->GetCivilization()->GetLifepod();
  if (!bKilled)
  {
    CFSPlayer* pfsPlayer = pfsShip->GetPlayer();
    pfsPlayer->AddEjection();

    assert (pcluster);

    for (MissileLinkIGC*  pml = pcluster->GetMissiles()->first();
         (pml != NULL);
         pml = pml->next())
    {
        ImissileIGC*    pmissile = pml->data();
        if (pmissile->GetTarget() == pship)
            pmissile->SetTarget(NULL);
    }

    //Eject the player
    Vector  v = pship->GetSourceShip()->GetVelocity();

    //Let our teammates and any observers know that we are now in an eject pod
    pfsShip->ShipStatusHullChange(pht);
    //Put us in the eject pod.
    if (pshipParent)
        pship->SetParentShip(NULL);
    pship->SetBaseHullType(pht);

    Vector      f = Vector::RandomDirection();
    Vector      position;
    Orientation o;
    if (pshipParent)
    {
        o.Set(f);

        //Start the escape pod well away from the parent ship
        position = p1 - f * (40.0f + 0.5f * pht->GetLength() + pshipParent->GetRadius());
    }
    else
    {
        bool    bLook;
        Vector  vectorLook;
        if (pcredit)
        {
            ImodelIGC*  plauncher = (pcredit->GetObjectType() == OT_ship)
                                    ? ((IshipIGC*)pcredit)->GetSourceShip()
                                    : pcredit;

            if (plauncher->GetCluster() == pcluster)
            {
                bLook = true;
                vectorLook = plauncher->GetPosition();
            }
            else
            {
                bLook = false;
            }
        }
        else
        {
            bLook = true;
            vectorLook = p2;
        }

        if (bLook)
        {
            Vector  dp = (vectorLook - p1);
            float   length2 = dp.LengthSquared();

            if (length2 >= 1.0f)
            {
                o.Set(dp);

                Vector  fNew = (f + dp * (8.0f / float(sqrt(length2))));
                length2 = fNew.LengthSquared();
                if (length2 > 0.01f)
                    f = fNew / float(sqrt(length2));
            }
            else
                o.Set(f);
        }
        else
            o.Set(f);

        position = p1;
    }

    v -= f * 100.0f;
    pship->SetPosition(position);
    pship->SetVelocity(v);
    pship->SetOrientation(o);

    pship->SetCurrentTurnRate(c_axisRoll, pi * 2.0f);
    pship->SetBB(timeCollision, timeCollision, 0.0f);

    //Send the eject message to everyone in the sector
    {
        BEGIN_PFM_CREATE(g.fm, pfmEject, S, EJECT)
        END_PFM_CREATE
        pfmEject->shipID = shipID;
        pfmEject->position = position;
        pfmEject->velocity = v;
        pfmEject->forward = f;
        pfmEject->cookie = pfsPlayer->NewCookie();
        g.fm.SendMessages(GetGroupSectorFlying(pcluster), FM_GUARANTEED, FM_FLUSH);
    }
  }
  else
  {
      pfsShip->AnnounceExit(NULL, (amount != 0.0f) ? SDR_KILLED : SDR_TERMINATE);
      if (pfsShip->IsPlayer())
      {
        assert (amount != 0.0f);
        assert (pcluster);

        CFSPlayer * pfsPlayer = pfsShip->GetPlayer();
        pfsPlayer->AddDeath();

        IstationIGC* pstationDest = (IstationIGC*)(FindTarget(pship, c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster,
                                                              NULL, NULL, NULL, NULL,
                                                              c_sabmRestart));
        if (!pstationDest)
            pstationDest = m_pfsMission->GetBase(pside);

        //Oh ick ... if the game ended this tick, the side might not actually have a station
        //so there is no place to send them to. Fortunately, this really is not a problem because
        //if the game ends then the clients are all going to be rather firmly reset anyhow

        //there also might be more than two
        pship->SetCluster(NULL);

        if (pshipParent)
            pship->SetParentShip(NULL);
        assert (pfsShip->GetIGCShip()->GetChildShips()->n() == 0);

        //Put us in the eject pod.
        pship->SetBaseHullType(pht);

        // Look for a new space station for him
        if (pstationDest)
        {
          pfsPlayer->ShipStatusRestart(pstationDest);

          BEGIN_PFM_CREATE(g.fm, pfmEnterLifepod, S, ENTER_LIFEPOD)
          END_PFM_CREATE
          //pfmEnterLifepod->shipID = shipID;
          g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

          pfsPlayer->GetIGCShip()->SetStation(pstationDest);
          pfsPlayer->ShipStatusDocked(pstationDest);

          pfsPlayer->SetCluster(pcluster, true);      //Pretend the client sent a set view cluster


          if (pship->IsGhost() && !m_pfsMission->HasPlayers(pside, false))
          {
              m_pfsMission->DeactivateSide(pside);
          }
        }
        else
        {
            assert (!pside->GetActiveF() || m_pfsMission->GetSideWon());
        }
      }
      else // drone go bye bye
      {
        assert (pship->GetParentShip() == NULL);

        delete pfsShip->GetDrone();
      }
  }
}

void FedSrvSiteBase::DamageShipEvent(Time now, IshipIGC* ship, ImodelIGC* launcher, DamageTypeID type, float amount, float leakage, const Vector& p1, const Vector& p2)
{
    if ((amount >= 1.0f) && launcher && ((type & c_dmgidNoWarn) == 0))
    {
        IsideIGC*   psideLauncher = launcher->GetSide();
        if (psideLauncher)
        {
            CFSShip*    pcfs = (CFSShip *)(ship->GetPrivateData());
            if (!pcfs->IsPlayer())
			{
                CFSDrone*   pd = (CFSDrone*)pcfs;

                //Never report more than once every 10 seconds
                if ((now - pd->GetLastDamageReport()) > 10.0f)
                {
                    IsideIGC*   pside = ship->GetSide();
                    assert (pside);

                    if (pside != psideLauncher) //&& !pside->AlliedSides(pside,psideLauncher))  //ALLY -imago 7/3/09 reverted
					{
                        SoundID underAttackSound;

                        switch (ship->GetPilotType())
                        {
                            case c_ptCarrier:
                                underAttackSound = carrierUnderAttackSound;
                                break;

                            case c_ptMiner:
                                if (ship->GetFraction() < 0.5f)
                                  underAttackSound = minerCriticalSound;
                                else
                                  underAttackSound = minerUnderAttackSound;
                                break;

                            case c_ptBuilder:
                                underAttackSound =
                                    ((IstationTypeIGC*)(IbaseIGC*)(ship->GetBaseData()))
                                        ->GetConstructorUnderAttackSound();
                                break;

                            default:
                            case c_ptLayer:
                                underAttackSound = droneUnderAttackSound;
                                break;
                        }
						
						//ChatTarget ct = (pside->GetAllies() == NA) ? CHAT_TEAM : CHAT_ALLIES; //ALLY imago 7/12/09 reverted 7/22/09
						Color AllianceColors[3] = { Color::Green(), Color::Orange(), Color::Red() };						
						ZString strAllies = (pside->AlliedSides(pside,psideLauncher)) ? "\x81 " + ConvertColorToString(AllianceColors[pside->GetAllies()]*0.75) + "(Ally)" + END_COLOR_STRING + " " : "";
						SendChatf(ship, CHAT_TEAM, ship->GetSide()->GetObjectID(),
                                  underAttackSound, "Under attack by %s%s in %s",
                                  (PCC)strAllies, GetModelName(launcher), ship->GetCluster()->GetName());
						
                        // Ally FF
                        if (launcher->GetObjectType() == OT_ship)
                        {
                            CFSShip* pfsLauncher = (CFSShip*)(((IshipIGC*)(launcher))->GetPrivateData());
                            if (pfsLauncher->IsPlayer() && pside->AlliedSides(pside, psideLauncher))
                            {
                                SendChat(ship, CHAT_INDIVIDUAL, launcher->GetObjectID(),
                                    droneWatchFireSound, "Watch your fire");
                            }
                        }
                    }
                    else if (launcher != nullptr && launcher->GetObjectType() == OT_ship)
                    {
                        CFSShip*    pfsLauncher = (CFSShip*)(((IshipIGC*)(launcher))->GetPrivateData());
                        if (pfsLauncher->IsPlayer())
                        {
                            SendChat(ship, CHAT_INDIVIDUAL, launcher->GetObjectID(),
                                     droneWatchFireSound, "Watch your fire");
                        }
                    }
                    pd->SetLastDamageReport(now);
                }
			} else { //Imago ally FF PvP 7/22/09
				CFSPlayer*   pp = (CFSPlayer*)pcfs;
                //Never report more than once every 10 seconds
                if ((now - pp->GetLastDamageReport()) > 10.0f)
                {
                    IsideIGC*   pside = ship->GetSide();
					if (pside != psideLauncher && pside->AlliedSides(pside,psideLauncher)) {
						SendChat(ship, CHAT_INDIVIDUAL, launcher->GetObjectID(),
                                     droneWatchFireSound, "Watch your fire");
						pp->SetLastDamageReport(now);
					}

				}
			}
        }
    }
}

void FedSrvSiteBase::DamageStationEvent(IstationIGC* station, ImodelIGC* launcher, DamageTypeID type, float amount)
{
    if ((amount >= 1.0f) && launcher && ((type & c_dmgidNoWarn) == 0))
    {
        IsideIGC*   psideLauncher = launcher->GetSide();
        if (psideLauncher)
        {
            if ((g.timeNow - station->GetLastDamageReport()) > 10.0f)
            {
                IsideIGC*   pside = station->GetSide();

				if (pside != psideLauncher) //&& !IsideIGC::AlliedSides(pside,psideLauncher)) //#ALLY (Imago) reverted 7/12/09
                {
					Color AllianceColors[3] = { Color::Green(), Color::Orange(), Color::Red() };						
					ZString strAllies = (pside->AlliedSides(pside,psideLauncher)) ? "\x81 " + ConvertColorToString(AllianceColors[pside->GetAllies()]*0.75) + "(Ally)" + END_COLOR_STRING +" " : "";
					ChatTarget ct = (station->GetSide()->GetAllies() != NA) ? CHAT_ALLIES : CHAT_TEAM; //ALLY imago 7/12/09
                    if (station->GetFraction() < 0.30)
                    {
                        SendChatf(NULL, ct, station->GetSide()->GetObjectID(),
                                  station->GetStationType()->GetCriticalSound(),
                                  "%s in %s critically damaged and under attack by %s%s", station->GetName(),
                                  station->GetCluster()->GetName(), (PCC)strAllies, GetModelName(launcher));
                    }
                    else
                    {
                        SendChatf(NULL, ct, station->GetSide()->GetObjectID(),
                                  station->GetStationType()->GetUnderAttackSound(),
                                  "%s in %s under attack by %s%s", station->GetName(),
                                  station->GetCluster()->GetName(), (PCC)strAllies,GetModelName(launcher));
                    }
					//7/22/09 ALLY FF
					CFSShip*    pfsLauncher = (CFSShip*)(((IshipIGC*)(launcher))->GetPrivateData());
					if (pfsLauncher->IsPlayer() && pside->AlliedSides(pside,psideLauncher))
                    {
                        SendChat(NULL, CHAT_INDIVIDUAL, launcher->GetObjectID(),
                                 droneWatchFireSound, "Watch your fire");
                    }
                }
                else if (launcher != nullptr && launcher->GetObjectType() == OT_ship)
                {
                    CFSShip*    pfsLauncher = (CFSShip*)(((IshipIGC*)(launcher))->GetPrivateData());
                    if (pfsLauncher->IsPlayer())
                    {
                        SendChat(NULL, CHAT_INDIVIDUAL, launcher->GetObjectID(),
                                 droneWatchFireSound, "Watch your fire");
                    }
                }

                station->SetLastDamageReport(g.timeNow);
            }
        }
    }
}

void FedSrvSiteBase::KillStationEvent(IstationIGC* pstation, ImodelIGC* plauncher, float amount)
{
  StationID stationID = pstation->GetObjectID();

  m_pfsMission->VacateStation(pstation);

  ObjectType    otLauncher = NA;
  IsideIGC*     psideLauncher = NULL;
  if (plauncher)
  {
      otLauncher = plauncher->GetObjectType();
      if (otLauncher == OT_ship)
      {
          IshipIGC*     pship = (IshipIGC*)plauncher;
          CFSShip*      pfsship = (CFSShip*)(pship->GetPrivateData());
          pfsship->GetPlayerScoreObject()->KillBase(true);
		  if (pfsship->IsPlayer())
		  {
			  CSteamAchievements *pSteamAchievements = pfsship->GetPlayer()->GetSteamAchievements();
			  pSteamAchievements->AwardBaseKillOrCapture(true); //Award Base Kill Achievement
		  }
          
		  for (ShipLinkIGC* psl = pship->GetChildShips()->first();
               (psl != NULL);
               psl = psl->next())
          {
             CFSShip*      ps = (CFSShip*)(psl->data()->GetPrivateData());
             ps->GetPlayerScoreObject()->KillBase(false);
          }
      }

      psideLauncher = plauncher->GetSide();
      if (psideLauncher)
      {
          psideLauncher->AddBaseKill();
      }
  }

  BEGIN_PFM_CREATE(g.fm, pfmStationDestroyed, S, STATION_DESTROYED)
  END_PFM_CREATE
  pfmStationDestroyed->stationID = stationID;
  pfmStationDestroyed->launcher = (otLauncher == OT_ship) ? plauncher->GetObjectID() : NA;
  g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  // TE: Added default values for plauncher's members used in StationDeleted event
  const char* szLauncherName = "(unknown)";
  int iLauncherUniqueID = NA;
  int iLauncherSideUniqueID = NA;

  if (plauncher && otLauncher == OT_ship)
  {
	  // Safely read the values from the launcher
	  szLauncherName = plauncher->GetName();
	  iLauncherUniqueID = plauncher->GetUniqueID();
	  if (psideLauncher) iLauncherSideUniqueID = psideLauncher->GetUniqueID(); //fix reporting base destruction Fuzz/Imago/Rock/PKK/notjarvis 24-12-09
  }

  // TE: fire AGCEvent when base is destroyed
  LPCSTR pszContext = m_pfsMission->GetIGCMission() ? m_pfsMission->GetIGCMission()->GetContextName() : NULL;
  _AGCModule.TriggerContextEvent(NULL, EventID_StationDeleted, pszContext,
    szLauncherName, iLauncherUniqueID, iLauncherSideUniqueID, -1, 4,
	"MissionID", VT_I4, m_pfsMission->GetMissionID(),
    "TeamID", VT_I4, pstation->GetSide()->GetUniqueID(),
	"TeamName", VT_LPSTR, pstation->GetSide()->GetName(),
    "zStationName", VT_LPSTR, pstation->GetName());
  // parameter is named zStationName so it appears at end of alphabetical list, keeping compatibility with allsrv1.0
  // End-TE

	// yp: Add event for players who were involved in the destruction of an enemy base.
    // mmf commented this out for now pending more testing
#if 0
	 ZString pszPlayerList = ""; // this creates a new ZString object and set its value to "", it's not a pointer to ""
	 if(m_pfsMission->GetIGCMission() && m_pfsMission->GetIGCMission()->GetShips()) // we hate null pointers
	 {
		ShipLinkIGC * pShiplink = m_pfsMission->GetIGCMission()->GetShips()->first();
		while (pShiplink)
		{
			CFSShip * pfsShip = (CFSShip *) pShiplink->data()->GetPrivateData();
			if (pfsShip && pfsShip->IsPlayer())
			{
				if(pfsShip->GetSide()		&& pfsShip->GetSide()->GetObjectID()	== iLauncherSideUniqueID && // if they are on the side that did the killing. and..
				   pfsShip->GetCluster()	&& pstation->GetCluster()	&& pfsShip->GetCluster()->GetObjectID() == pstation->GetCluster()->GetObjectID()) // they are in this sector
				{
					pszPlayerList = pszPlayerList + ";" + ZString(pfsShip->GetPlayer()->GetName()); // players name
					// The distance the player is from the station that was destroyed.
					if(pfsShip->GetPlayer()->GetIGCShip() )
					{
						pszPlayerList = pszPlayerList +  ":" + ZString( (pstation->GetPosition() - pfsShip->GetPlayer()->GetIGCShip()->GetPosition()).Length()); // the distance
					}
				}
			}
			pShiplink = pShiplink->next();
		}
	 }

	 // Fire AGCEvent listing players in the sector
	// Might want to add into the event weither or not this was a VICTORY kill.. should we track that as well?
	_AGCModule.TriggerContextEvent(NULL, EventID_StationDeletedEscort, pszContext,
						szLauncherName, iLauncherUniqueID, iLauncherSideUniqueID, -1, 4,
						"MissionID", VT_I4, m_pfsMission->GetMissionID(),
						"TeamID"    , VT_I4   , pstation->GetSide()->GetUniqueID(), // teamID the base was on.
						"TeamName", VT_LPSTR, pstation->GetSide()->GetName(), // team the base was on.
						"zPlayerList", VT_LPSTR, pszPlayerList); // pszPlayerList should look like ";player@squad:1500;player2@squad:500"
	// YP: end
#endif

  IsideIGC*   pside = pstation->GetSide();
  IsideIGC*   psideWin = m_pfsMission->CheckForVictoryByStationKill(pstation, pside);
  if (psideWin)
  {
      static char szReason[256];     //Make this static so we only need to keep a pointer to it around
	  if ((otLauncher == OT_ship) && ((psideLauncher == psideWin) || IsideIGC::AlliedSides(psideLauncher, psideWin))) //#ALLY (Imago) 7/8/09
      {
		//joeld - added 4th %s and pstation->GetCluster()->GetName() in next line.
          sprintf(szReason, "%s has destroyed %s's %s in %s.", psideLauncher->GetName(), pside->GetName(), pstation->GetName(), pstation->GetCluster()->GetName());
		  m_pfsMission->GameOver(psideWin, szReason);
      }
	  else {
		  //make the allies team name list Imago ALLY 7/13/09
		  if (psideWin->GetAllies() != NA) {
			  ZString strWinningTeams;
				//lets get a list of allied sideIDs
			    for (SideLinkIGC* psidelink = psideWin->GetMission()->GetSides()->first();
					(psidelink != NULL);
					psidelink = psidelink->next())
				{
					IsideIGC*   otherside = psidelink->data();
					//this side is ally...and not ours
					if (psideWin->AlliedSides(psideWin,otherside) && otherside != psideWin)
						strWinningTeams += " & " + ZString(otherside->GetName());
				}
				strWinningTeams = psideWin->GetName() + strWinningTeams;
          		sprintf(szReason, "%s won by dominating their opponents", (PCC)strWinningTeams);
		  		m_pfsMission->GameOver(psideWin, szReason);
		  } else {
          	sprintf(szReason, "%s won by dominating their opponents", psideWin->GetName());
		 	m_pfsMission->GameOver(psideWin, szReason);
		  }
	  }
  } else if ((otLauncher == OT_ship) && psideLauncher)
      SendChatf(NULL, CHAT_EVERYONE, NA,
                NA, "%s has destroyed %s's %s.",
                psideLauncher->GetName(), pside->GetName(), pstation->GetName());


  //#121 kill the roid I'm on for teams that cant see me  -- Imago 8/10
	const SideListIGC * plistSide1 = pstation->GetMission()->GetSides();
	for (SideLinkIGC * plinkSide = plistSide1->first(); plinkSide; plinkSide = plinkSide->next()) {
		IsideIGC * psideother = plinkSide->data();
		if (psideother && !pstation->SeenBySide(psideother) && pstation->GetRoidID() != NA && pstation->GetSide()->GetObjectID() != psideother->GetObjectID() && pstation->GetRoidSide(psideother->GetObjectID()))
			KillAsteroidEvent(pstation->GetRoidID(),pstation->GetCluster()->GetObjectID(),psideother);
	}


  pstation->Terminate();

  for (SideLinkIGC* l = pside->GetMission()->GetSides()->first(); (l != NULL); l = l->next()) {
      l->data()->UpdateTerritory();
  }

  /*
  //NYI: hack to test simultaneous death
  {
      for (ShipLinkIGC* psl = pside->GetShips()->first(); (psl != NULL); )
      {
          IshipIGC* pship = psl->data();
          psl = psl->next();
          KillShipEvent(g.timeNow, pship, NULL, 1.0f,
                                               pship->GetSourceShip()->GetPosition(), Vector::GetZero());
      }
  }
  */
}

void FedSrvSiteBase::KillAsteroidEvent(IasteroidIGC* pasteroid, bool explodeF)
{
  BEGIN_PFM_CREATE(g.fm, pfmAsteroidDestroyed, S, ASTEROID_DESTROYED)
  END_PFM_CREATE
  pfmAsteroidDestroyed->clusterID = pasteroid->GetCluster()->GetObjectID();
  pfmAsteroidDestroyed->asteroidID = pasteroid->GetObjectID();
  pfmAsteroidDestroyed->explodeF = explodeF;
  g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  pasteroid->Terminate();
}

void FedSrvSiteBase::KillAsteroidEvent(IasteroidIGC* pasteroid, IsideIGC* side, bool explodeF)
{
  BEGIN_PFM_CREATE(g.fm, pfmAsteroidDestroyed, S, ASTEROID_DESTROYED)
  END_PFM_CREATE
  pfmAsteroidDestroyed->clusterID = pasteroid->GetCluster()->GetObjectID();
  pfmAsteroidDestroyed->asteroidID = pasteroid->GetObjectID();
  pfmAsteroidDestroyed->explodeF = explodeF;
  g.fm.SendMessages(CFSSide::FromIGC(side)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
}

void FedSrvSiteBase::KillAsteroidEvent(AsteroidID roid, SectorID soid, IsideIGC* side)
{
  BEGIN_PFM_CREATE(g.fm, pfmAsteroidDestroyed, S, ASTEROID_DESTROYED)
  END_PFM_CREATE
  pfmAsteroidDestroyed->clusterID = soid;
  pfmAsteroidDestroyed->asteroidID = roid;
  pfmAsteroidDestroyed->explodeF = false;
  g.fm.SendMessages(CFSSide::FromIGC(side)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
}

void FedSrvSiteBase::DrainAsteroidEvent(IasteroidIGC* pasteroid)
{
  BEGIN_PFM_CREATE(g.fm, pfmAsteroidDrained, S, ASTEROID_DRAINED)
  END_PFM_CREATE
  pfmAsteroidDrained->clusterID = pasteroid->GetCluster()->GetObjectID();
  pfmAsteroidDrained->asteroidID = pasteroid->GetObjectID();
  g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  pasteroid->SetOre(0.0f);
}
//Xynth #132 7/2010
void FedSrvSiteBase::MineAsteroidEvent(IasteroidIGC* pasteroid, float newOre)
{
	//Xynth #225 Loop through the sides and update visibile asteroids
	for (SideLinkIGC*   psidelink = pasteroid->GetMission()->GetSides()->first();
		(psidelink != NULL);
		psidelink = psidelink->next())
	{
		if (pasteroid->GetAsteroidCurrentEye(psidelink->data()))
		{
  BEGIN_PFM_CREATE(g.fm, pfmAsteroidMined, S, ASTEROID_MINED)
  END_PFM_CREATE
  pfmAsteroidMined->clusterID = pasteroid->GetCluster()->GetObjectID();
  pfmAsteroidMined->asteroidID = pasteroid->GetObjectID();
  pfmAsteroidMined->bpOreFraction = newOre;
			g.fm.SendMessages(CFSSide::FromIGC(psidelink->data())->GetGroup(), FM_GUARANTEED, FM_FLUSH);
		}
	}
	pasteroid->SetOreWithFraction(newOre, false);
}

void FedSrvSiteBase::KillProbeEvent(IprobeIGC* pprobe)
{
  BEGIN_PFM_CREATE(g.fm, pfmProbeDestroyed, S, PROBE_DESTROYED)
  END_PFM_CREATE
  pfmProbeDestroyed->clusterID = pprobe->GetCluster()->GetObjectID();
  pfmProbeDestroyed->probeID = pprobe->GetObjectID();
  g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  pprobe->Terminate();
}
void FedSrvSiteBase::KillBuildingEffectEvent(IbuildingEffectIGC* pbe)
{
  BEGIN_PFM_CREATE(g.fm, pfmBuildingEffectDestroyed, S, BUILDINGEFFECT_DESTROYED)
  END_PFM_CREATE
  pfmBuildingEffectDestroyed->asteroidID = pbe->GetAsteroid()->GetObjectID();
  g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  pbe->Terminate();
}
void FedSrvSiteBase::KillTreasureEvent(ItreasureIGC* ptreasure)
{
    if (m_pfsMission && m_pfsMission->GetIGCMission()->GetMissionStage() == STAGE_STARTED)
    {
        //No more treasure ... spread the word
        BEGIN_PFM_CREATE(g.fm, pfmDT, S, DESTROY_TREASURE)
        END_PFM_CREATE

        pfmDT->treasureID = ptreasure->GetObjectID();
        pfmDT->sectorID = ptreasure->GetCluster()->GetObjectID();
        g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_NOT_GUARANTEED, FM_FLUSH);

        if (ptreasure->GetTreasureCode() == c_tcFlag)
            RespawnFlag(ptreasure->GetTreasureID(),NULL); // mmf 11/07 added second argument pside (which is set to null in this case)
    }

    ptreasure->Terminate();
}

void FedSrvSiteBase::KillMissileEvent(ImissileIGC* pmissile, const Vector& position)
{
  BEGIN_PFM_CREATE(g.fm, pfmMissileDestroyed, S, MISSILE_DESTROYED)
  END_PFM_CREATE
  pfmMissileDestroyed->clusterID = pmissile->GetCluster()->GetObjectID();
  pfmMissileDestroyed->missileID = pmissile->GetObjectID();
  pfmMissileDestroyed->position = position;
  g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  // KGJV- explode the missile before terminating it
  // skip special case for resonators (position == 0). see CmissileIGC::HandleCollision
  // if we want resonators to actually explode on impact while still producing the resonate effect
  // then changes are required in CmissileIGC::HandleCollision to not set "position" to 0
  // as well as treatment of FM_S_MISSILE_DESTROYED in BaseClient::HandleMsg()
  if (position != Vector::GetZero()) pmissile->Explode(position);

  pmissile->Terminate();
}

void FedSrvSiteBase::KillMineEvent(ImineIGC* pmine)
{
  BEGIN_PFM_CREATE(g.fm, pfmMineDestroyed, S, MINE_DESTROYED)
  END_PFM_CREATE
  pfmMineDestroyed->clusterID = pmine->GetCluster()->GetObjectID();
  pfmMineDestroyed->mineID = pmine->GetObjectID();
  g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

  pmine->Terminate();
}


void FedSrvSiteBase::WarpBombEvent(IwarpIGC* pwarp, ImissileIGC* pmissile)
{
    assert (pwarp);
    assert (pmissile);
    ImissileTypeIGC*    pmt = pmissile->GetMissileType();
    assert (pmt);
    assert (pmt->GetObjectType() == OT_missileType);

    Time    timeExplosion = pwarp->GetLastUpdate() + m_pfsMission->GetIGCMission()->GetFloatConstant(c_fcidWarpBombDelay);
    {
        BEGIN_PFM_CREATE(g.fm, pfmWB, S, WARP_BOMB)
        END_PFM_CREATE
        pfmWB->timeExplosion = timeExplosion;
        pfmWB->warpidBombed = pwarp->GetObjectID();
        pfmWB->expendableidMissile = pmt->GetObjectID();
        g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);
    }

    pwarp->AddBomb(timeExplosion, pmt);

    /*
    DamageTypeID    dtid = pmt->GetDamageType();
    float           p    = pmt->GetPower();
    float           r    = pmt->GetBlastRadius();

    IclusterIGC*    pcluster = pwarp->GetCluster();
    pcluster->CreateExplosion(dtid,
                              p,
                              r,
                              c_etBigShip,
                              pcluster->GetLastUpdate(),
                              pwarp->GetPosition(),
                              NULL);

    pwarp = pwarp->GetDestination();
    pcluster = pwarp->GetCluster();
    pcluster->CreateExplosion(dtid,
                              p,
                              r,
                              c_etBigShip,
                              pcluster->GetLastUpdate(),
                              pwarp->GetPosition(),
                              NULL);
    */
}

static void RefillPart(IshipIGC* pship, IpartIGC* ppart, const TechTreeBitMask&   ttbm)
{
    IpartTypeIGC*   pptBase = ppart->GetPartType();
    if (pptBase->GetRequiredTechs() <= ttbm)
    {
        IpartTypeIGC*   ppt = pptBase;

        while (true)
        {
            IpartTypeIGC*   pptNext = ppt->GetSuccessorPartType();
            if (pptNext && (pptNext->GetRequiredTechs() <= ttbm))
                ppt = pptNext;
            else
                break;
        }

        short   newAmount = (ppart->GetPrice() == 0)
                            ? SHRT_MAX
                            : ppart->GetAmount();

        if (ppt == pptBase)
        {
            //Things that don't care about amounts wont care if I try and set it.
            ppart->SetAmount(newAmount);
        }
        else
        {
            Mount   oldMount = ppart->GetMountID();
            ppart->Terminate();
            pship->CreateAndAddPart(ppt, oldMount, newAmount);
        }
    }
}

static bool MatchPT(IpartTypeIGC* ppt1, IpartTypeIGC* ppt2)
{
    do
    {
        if (ppt1 == ppt2)
            return true;

        ppt1 = ppt1->GetSuccessorPartType();
    }
    while (ppt1);

    return false;
}

static void ErasePart(IshipIGC* pship, EquipmentType et, Mount mount, IpartTypeIGC*  ptDesired[c_maxCargo + 3])
{
    IpartIGC*   ppart = pship->GetMountedPart(et, mount);
    if (ppart)
    {
        IpartTypeIGC*   ppt = ppart->GetPartType();
        //Go through the list of parts and nuke one that corresponds to this part
        for (int i = 0; (i < c_maxCargo + 3); i++)
        {
            if (ptDesired[i] && MatchPT(ptDesired[i], ppt))
            {
                ptDesired[i] = NULL;
                break;
            }
        }
    }
}

void FedSrvSiteBase::AlliedRearm(IshipIGC* pship, bool bLaunch) { //Imago 8/10/09 ALLY
    //RIPPED FROM LAND ON CARRIER EVENT 7/9/09 

    CFSPlayer*  pfsPlayer = ((CFSShip*)(pship->GetPrivateData()))->GetPlayer();
    ImissionIGC*    pMission = pship->GetMission();
    TechTreeBitMask  ttbm = pship->GetSide()->GetDevelopmentTechs() | pship->GetSide()->GetBuildingTechs();

    //Now ... go through the players ship and restore the expendables that are
    //free and partially gone
    {
        //refill parts backwards so that newly added parts will not be revisited
        PartLinkIGC*    ppl = pship->GetParts()->last();
        while (ppl)
        {
            IpartIGC*   ppart = ppl->data();
            ppl = ppl->txen();

            if (ppart)
                RefillPart(pship, ppart, ttbm);
        }
    }

    //Copy the list of desired parts
    IpartTypeIGC*   ptDesired[c_maxCargo + 3];      //Must be kept sync'd with
    memcpy(ptDesired, pfsPlayer->GetDesiredLoadout(), sizeof(ptDesired));

    //Go through and fill any holes with the desired parts
    {
        ErasePart(pship, ET_Magazine, 0, ptDesired);
        ErasePart(pship, ET_Dispenser, 0, ptDesired);
        ErasePart(pship, ET_ChaffLauncher, 0, ptDesired);
        for (Mount i = 0; (i < c_maxCargo); i++)
            ErasePart(pship, NA, -1 - i, ptDesired);
    }

    {
        IpartTypeIGC*   ptFuel = (pship->GetMountedPart(ET_Afterburner, 0) != NULL)
                                 ? pMission->GetFuelPack()
                                 : NULL;

        IpartTypeIGC*   ptAmmo = NULL;
        {
            for (Mount i = pship->GetHullType()->GetMaxWeapons() - 1; (i >= 0); i--)
            {
                IweaponIGC* pw = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, i));
                if (pw && (pw->GetAmmoPerShot() != 0))
                {
                    ptAmmo = pMission->GetAmmoPack();
                    break;
                }
            }
        }

        bool    bBuyFuel = true;

        int     iDesired = 0;
        for (Mount i = 0; (i < c_maxCargo); i++)
        {
            IpartIGC*   ppart = pship->GetMountedPart(NA, -1 - i);
            if (ppart == NULL)
            {
                //Shove an instance of the first part left in the
                //desired part list into the slot
                while (iDesired < c_maxCargo + 3)
                {
                    IpartTypeIGC*   pt = ptDesired[iDesired++];

                    if (pt && (pt->GetRequiredTechs() <= ttbm))
                    {
                        while (true)
                        {
                            IpartTypeIGC*   ptNext = pt->GetSuccessorPartType();
                            if (ptNext && (ptNext->GetRequiredTechs() <= ttbm))
                                pt = ptNext;
                            else
                                break;
                        }

                        ppart = pship->CreateAndAddPart(pt, -1 - i, SHRT_MAX);
                        assert (ppart->GetPrice() == 0);
                        break;
                    }
                }

                if (ppart == NULL)
                {
                    if (bBuyFuel && ptFuel)
                    {
                        pship->CreateAndAddPart(ptFuel, -1 - i, SHRT_MAX);
                        bBuyFuel = (ptAmmo == NULL);
                    }
                    else if (ptAmmo)
                    {
                        pship->CreateAndAddPart(ptAmmo, -1 - i, SHRT_MAX);
                        bBuyFuel = true;
                    }
                }
            }
        }
    }

    pship->SetAmmo(SHRT_MAX);
    pship->SetFuel(FLT_MAX);
    pship->SetEnergy(pship->GetHullType()->GetMaxEnergy());
    pship->SetFraction(1.0f);
    {
        IshieldIGC* pshield = (IshieldIGC*)(pship->GetMountedPart(ET_Shield, 0));
        if (pshield)
            pshield->SetFraction(1.0f);
    }

    //IMAGO ALLY 7/10/09 REARM
    // tell the client what happened
    BEGIN_PFM_CREATE(g.fm, pfmBuyLoadoutAck, S, BUY_LOADOUT_ACK)
        FM_VAR_PARM(NULL, pfsPlayer->GetIGCShip()->ExportShipLoadout(NULL))
    END_PFM_CREATE

    pfsPlayer->GetIGCShip()->ExportShipLoadout((ShipLoadout*)(FM_VAR_REF(pfmBuyLoadoutAck, loadout)));

    pfsPlayer->SaveDesiredLoadout();

    pfmBuyLoadoutAck->fBoughtEverything = true;
    pfmBuyLoadoutAck->fLaunch = bLaunch;

    g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);

    //Tell the surviving kids about dad's new toys
    const ShipListIGC*    pshipsChildren = pfsPlayer->GetIGCShip()->GetChildShips();

    pfsPlayer->QueueLoadoutChange();

    {
      SideID    sid = pfsPlayer->GetSide()->GetObjectID();
      pfsPlayer->GetShipStatus(sid)->SetHullID(pfsPlayer->GetIGCShip()->GetBaseHullType()->GetObjectID());

      for (ShipLinkIGC* psl = pshipsChildren->first(); (psl != NULL); psl = psl->next())
      {
          CFSShip*  pfsChild = (CFSShip*)(psl->data()->GetPrivateData());

          //Since it might have changed ... reset the stored turret ID as well
          pfsChild->GetShipStatus(sid)->SetState(psl->data()->GetTurretID() == NA ? c_ssObserver : c_ssTurret);

          g.fm.SendMessages(pfsChild->GetPlayer()->GetConnection(), FM_GUARANTEED, FM_DONT_FLUSH);
      }
    }
    g.fm.PurgeOutBox();

    if (bLaunch) {
        //Move the children of this ship to space as well
        for (ShipLinkIGC*   psl = pshipsChildren->first();
             (psl != NULL);
             psl = psl->next())
        {
          psl->data()->SetStation(NULL);
        }
        pfsPlayer->GetIGCShip()->SetStation(NULL);
    }
}

bool FedSrvSiteBase::LandOnCarrierEvent(IshipIGC* pshipCarrier, IshipIGC* pship, float tCollision)
{
    CFSShip* pfsShip = (CFSShip*)pship->GetPrivateData();

    ImissionIGC*    pMission = pship->GetMission();
    TechTreeBitMask  ttbm = pship->GetSide()->GetDevelopmentTechs() | pship->GetSide()->GetBuildingTechs();

    //Now ... go through the ship and restore the expendables that are
    //free and partially gone
    {
        //refill parts backwards so that newly added parts will not be revisited
        PartLinkIGC*    ppl = pship->GetParts()->last();
        while (ppl)
        {
            IpartIGC*   ppart = ppl->data();
            ppl = ppl->txen();

            if (ppart)
                RefillPart(pship, ppart, ttbm);
        }
    }

    if (pfsShip->IsPlayer()) {
        //Copy the list of desired parts
        IpartTypeIGC*   ptDesired[c_maxCargo + 3];      //Must be kept sync'd with
        memcpy(ptDesired, pfsShip->GetPlayer()->GetDesiredLoadout(), sizeof(ptDesired));

        //Go through and fill any holes with the desired parts
        {
            ErasePart(pship, ET_Magazine, 0, ptDesired);
            ErasePart(pship, ET_Dispenser, 0, ptDesired);
            ErasePart(pship, ET_ChaffLauncher, 0, ptDesired);
            for (Mount i = 0; (i < c_maxCargo); i++)
                ErasePart(pship, NA, -1 - i, ptDesired);
        }

        {
            IpartTypeIGC*   ptFuel = (pship->GetMountedPart(ET_Afterburner, 0) != NULL)
                ? pMission->GetFuelPack()
                : NULL;

            IpartTypeIGC*   ptAmmo = NULL;
            {
                for (Mount i = pship->GetHullType()->GetMaxWeapons() - 1; (i >= 0); i--)
                {
                    IweaponIGC* pw = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, i));
                    if (pw && (pw->GetAmmoPerShot() != 0))
                    {
                        ptAmmo = pMission->GetAmmoPack();
                        break;
                    }
                }
            }

            bool    bBuyFuel = true;

            // Remount launcher parts if they are depleted AaronMoore 1/10 (Imago)
            for (int i = 0; i < 3; i++)
            {
                if ((i == 0 && pship->GetMountedPart(ET_Magazine, 0) == NULL) ||
                    (i == 1 && pship->GetMountedPart(ET_Dispenser, 0) == NULL) ||
                    (i == 2 && pship->GetMountedPart(ET_ChaffLauncher, 0) == NULL))
                {
                    IpartTypeIGC*   pt = ptDesired[i];

                    if (pt)
                    {
                        // Upgrade part if posible
                        while (true)
                        {
                            IpartTypeIGC*   ptNext = pt->GetSuccessorPartType();
                            if (ptNext && (ptNext->GetRequiredTechs() <= ttbm))
                                pt = ptNext;
                            else
                                break;
                        }
                        pship->CreateAndAddPart(pt, 0, SHRT_MAX);

                        // Remove this part from the desired list so it is not added again into cargo
                        ptDesired[i] = NULL;
                    }
                }
            }
            //end AaaronMoore 1/10


            int     iDesired = 0;
            for (Mount i = 0; (i < c_maxCargo); i++)
            {
                IpartIGC*   ppart = pship->GetMountedPart(NA, -1 - i);
                if (ppart == NULL)
                {
                    //Shove an instance of the first part left in the
                    //desired part list into the slot
                    while (iDesired < c_maxCargo + 3)
                    {
                        IpartTypeIGC*   pt = ptDesired[iDesired++];

                        if (pt && (pt->GetRequiredTechs() <= ttbm))
                        {
                            while (true)
                            {
                                IpartTypeIGC*   ptNext = pt->GetSuccessorPartType();
                                if (ptNext && (ptNext->GetRequiredTechs() <= ttbm))
                                    pt = ptNext;
                                else
                                    break;
                            }

                            ppart = pship->CreateAndAddPart(pt, -1 - i, SHRT_MAX);
                            assert(ppart->GetPrice() == 0);
                            break;
                        }
                    }

                    if (ppart == NULL)
                    {
                        if (bBuyFuel && ptFuel)
                        {
                            pship->CreateAndAddPart(ptFuel, -1 - i, SHRT_MAX);
                            bBuyFuel = (ptAmmo == NULL);
                        }
                        else if (ptAmmo)
                        {
                            pship->CreateAndAddPart(ptAmmo, -1 - i, SHRT_MAX);
                            bBuyFuel = true;
                        }
                    }
                }
            }
        }
    }

    pship->SetAmmo(SHRT_MAX);
    pship->SetFuel(FLT_MAX);
    pship->SetEnergy(pship->GetHullType()->GetMaxEnergy());
    pship->SetFraction(1.0f);
    {
        IshieldIGC* pshield = (IshieldIGC*)(pship->GetMountedPart(ET_Shield, 0));
        if (pshield)
            pshield->SetFraction(1.0f);
    }

    const Orientation&  orientation = pshipCarrier->GetOrientation();

    float   vLaunch = pMission->GetFloatConstant(c_fcidExitStationSpeed);

    Vector  position = pshipCarrier->GetPosition();
    Vector  velocity = pshipCarrier->GetVelocity();

    position.x += random(-0.5f, 0.5f);
    position.y += random(-0.5f, 0.5f);
    position.z += random(-0.5f, 0.5f);

    IhullTypeIGC*   phtCarrier = pshipCarrier->GetBaseHullType();

    Orientation         orientationBfr;
    const Orientation*  porientation;
    if (phtCarrier->GetLaunchSlots() == 0)
    {
        position -= orientation.GetBackward() * (pshipCarrier->GetRadius() + pship->GetRadius() + vLaunch * 0.5f);
        velocity -= orientation.GetBackward() * vLaunch;

        porientation = &orientation;
    }
    else
    {
        short   slot = pshipCarrier->GetLaunchSlot();

        position += phtCarrier->GetLaunchPosition(slot) * orientation;

        Vector  forward = phtCarrier->GetLaunchDirection(slot) * orientation;

        position += forward * (pship->GetRadius() + vLaunch * 0.5f);
        velocity += forward * vLaunch;

        orientationBfr = orientation;
        orientationBfr.TurnTo(forward);

        porientation = &orientationBfr;
    }

    pship->SetPosition(position);
    pship->SetVelocity(velocity);
    pship->SetOrientation(*porientation);

    IclusterIGC*    pcluster = pship->GetCluster();
    pcluster->RecalculateCollisions(tCollision, pship, NULL);

    //Send the launch message to everyone in the sector
    {
        BEGIN_PFM_CREATE(g.fm, pfmLaunch, S, RELAUNCH_SHIP)
            FM_VAR_PARM(NULL, pship->ExportShipLoadout(NULL))
        END_PFM_CREATE

        pship->ExportShipLoadout((ShipLoadout*)FM_VAR_REF(pfmLaunch, loadout));
        pfmLaunch->shipID = pship->GetObjectID();
        pfmLaunch->carrierID = pshipCarrier->GetObjectID();
        pfmLaunch->position = position;
        pfmLaunch->velocity = velocity;
        pfmLaunch->orientation.Set(*porientation);
        if (pfsShip->IsPlayer())
            pfmLaunch->cookie = pfsShip->GetPlayer()->NewCookie();
        g.fm.SendMessages(GetGroupSectorFlying(pship->GetCluster()), FM_GUARANTEED, FM_FLUSH);
    }

    if (pship->GetAutopilot()) {
        if (pship->GetCommandTarget(c_cmdAccepted) == (ImodelIGC*)pshipCarrier && pship->GetCommandID(c_cmdAccepted) == c_cidGoto) {
            pship->SetCommand(c_cmdAccepted, pshipCarrier, c_cidDefend);
            if (pship->GetPilotType() >= c_ptPlayer)
                pship->SetCommand(c_cmdPlan, pshipCarrier, c_cidDefend);
        }
        else
            pship->SetCommand(c_cmdPlan, NULL, c_cidNone); // drones will do c_cmdAccepted
    }

    return true;
}

bool FedSrvSiteBase::RescueShipEvent(IshipIGC* pshipRescued, IshipIGC* pshipRescuer)
{
  assert (pshipRescued);
  if (pshipRescuer)
  {
      CFSShip* pfsShip = (CFSShip*)(pshipRescuer->GetPrivateData());
      pfsShip->GetPlayerScoreObject()->AddRescue();
	  if (pfsShip->IsPlayer())
	  {
		  CSteamAchievements *pSteamAchievements = pfsShip->GetPlayer()->GetSteamAchievements();
		  pSteamAchievements->AwardPodPickup();
	  }

	  CFSShip* rescuedShip = (CFSShip*)(pshipRescued->GetPrivateData());
	  if (rescuedShip->IsPlayer())
	  {
		  CSteamAchievements *pSteamAchievements = rescuedShip->GetPlayer()->GetSteamAchievements();
		  pSteamAchievements->AwardGetRescued();
	  }
      BEGIN_PFM_CREATE(g.fm, pfmPlayerRescued, S, PLAYER_RESCUED)
      END_PFM_CREATE
      pfmPlayerRescued->shipIDRescuer = pshipRescuer->GetObjectID();
      pfmPlayerRescued->shipIDRescuee = pshipRescued->GetObjectID();

      if (pfsShip->IsPlayer())
        g.fm.SendMessages(pfsShip->GetPlayer()->GetConnection(), FM_GUARANTEED, FM_DONT_FLUSH);

      g.fm.SendMessages(((CFSShip*)(pshipRescued->GetPrivateData()))->GetPlayer()->GetConnection(), FM_GUARANTEED, FM_FLUSH);
  }

  ((CFSShip*)(pshipRescued->GetPrivateData()))->ResetWarpState();

  //Find the closest space station and put the rescued ship there  
  IstationIGC*    pstation = (IstationIGC*)FindTarget(pshipRescued,
                                                      (c_ttStation | c_ttFriendly | c_ttAnyCluster | c_ttNearest),
                                                      NULL, NULL, NULL, NULL, c_sabmRestart);
  //imago ALLY 7/13/09
  if (pstation == NULL)
	  pstation = m_pfsMission->GetBase(((CFSShip*)(pshipRescued->GetPrivateData()))->GetPlayer()->GetSide());
  else if (pstation->GetSide() != ((CFSShip*)(pshipRescued->GetPrivateData()))->GetPlayer()->GetSide())
	  pstation = m_pfsMission->GetBase(((CFSShip*)(pshipRescued->GetPrivateData()))->GetPlayer()->GetSide());


  return pstation ? DockWithStationEvent(pshipRescued, pstation) : false;
}


void FedSrvSiteBase::CaptureStationEvent(IshipIGC* pship, IstationIGC* pstation)
{
    CFSShip* pfsShip = (CFSShip *) pship->GetPrivateData();
    pfsShip->CaptureStation(pstation);
}


bool FedSrvSiteBase::DockWithStationEvent(IshipIGC* pship, IstationIGC* pstation)
{
  assert (pship->GetParentShip() == NULL);

  CFSShip* pfsShip = (CFSShip *) pship->GetPrivateData();

  if (!pfsShip->OkToWarp())
      return false;

  IsideIGC* pside = pship->GetSide();
  IsideIGC* psideOld = pstation->GetSide();

  
  if ((pside != psideOld) && (pship->GetHullType() && pship->GetHullType()->HasCapability(c_habmBoard)) && 
      (!pside->AlliedSides(pside,psideOld) || 
      pship->GetMission()->GetMissionParams()->bAllowFriendlyFire)) //#ALLY 7/3/09 8/10/09 imago
  {
      pfsShip->CaptureStation(pstation);
  }

  pfsShip->AnnounceExit(pship->GetCluster(), SDR_DOCKED);

  // let anyone who can see us know that we are docked
  pfsShip->ShipStatusDocked(pstation);

  {
      SideID sidFlag = pship->GetFlag();
      if ((sidFlag != NA) && (pside == psideOld)) //#ALLY - was: (sidFlag != NA)
	  {
          if (sidFlag == SIDE_TEAMLOBBY)
          {
              pfsShip->GetPlayerScoreObject()->AddArtifact();
              pside->SetArtifacts(pside->GetArtifacts() + 1);

              SendChatf(NULL, CHAT_TEAM, pside->GetObjectID(),
                        artifactSecuredSound, "%s has secured an artifact.",
                        pship->GetName());
          }
          else
          {
              pfsShip->GetPlayerScoreObject()->AddFlag();
              pside->SetFlags(pside->GetFlags() + 1);

              SendChatf(NULL, CHAT_TEAM, pside->GetObjectID(),
                        enemyFlagSecuredSound, "%s has secured %s's flag.",
                        pship->GetName(), pfsShip->GetIGCShip()->GetMission()->GetSide(sidFlag)->GetName());

              SendChatf(NULL, CHAT_TEAM, sidFlag,
                        flagSecuredSound, "%s of %s has secured your flag.",
                        pship->GetName(), pside->GetName ());
          }

          pship->SetFlag(NA);

          BEGIN_PFM_CREATE(g.fm, pfmGain, S, GAIN_FLAG)
          END_PFM_CREATE
          pfmGain->sideidFlag = NA;
          pfmGain->shipidRecipient = pship->GetObjectID();
          pfmGain->bIsTreasureDocked = false; // KGJV #118

          g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

          IsideIGC*   psideWin = m_pfsMission->CheckForVictoryByFlags(pside, sidFlag);
          if (psideWin)
          {
              static char szReason[100];     //Make this static so we only need to keep a pointer to it around
              sprintf(szReason,
                      (sidFlag != NA)
                      ? "%s wins with the flag recovered by %s"
                      : "%s wins with the artifact recovered by %s",
                      psideWin->GetName(), pship->GetName());
              m_pfsMission->GameOver(psideWin, szReason);
          }

          RespawnFlag(sidFlag,pside);  // mmf 11/07 added second argument pside
      }
  }

  pship->SetStation(pstation);
  {
      //Move the children of this ship to the station as well
      for (ShipLinkIGC*   psl = pship->GetChildShips()->first();
           (psl != NULL);
           psl = psl->next())
      {
          psl->data()->SetStation(pstation);
      }
  }

  return true;
}

void    FedSrvSiteBase::ChangeStation(IshipIGC* pship, IstationIGC* pstationOld, IstationIGC* pstationNew)
{
    assert (pship);
    CFSShip * pfsShip = (CFSShip *) pship->GetPrivateData();

    if (pstationNew == NULL)
    {
        //Let the parent handle the launch & only allow launched in the game.
        if ((m_pfsMission->GetStage() == STAGE_STARTED) && (pship->GetParentShip() == NULL))
        {
            pstationOld->RepairAndRefuel(pship);
            pfsShip->Launch(pstationOld);
        }
    }
    else
	{
        if ((pstationOld == NULL) && (pstationNew->GetSide() == pship->GetSide())) //#ALLY: dont dock tech at allied bases
		{
            //Go over the ship's parts and give the side the tech bits for any equipment onboard

            TechTreeBitMask ttbm;
            ttbm.ClearAll();
            for (PartLinkIGC*   ppl = pship->GetParts()->first(); (ppl != NULL); ppl = ppl->next())
            {
                IpartIGC*   ppart = ppl->data();

                ttbm |= ppart->GetPartType()->GetEffectTechs();
				// KGJV #118
				if (pship->GetSide()->IsNewDevelopmentTechs(ppart->GetPartType()->GetEffectTechs()))
				{
					// send message that player secured new tech
					BEGIN_PFM_CREATE(g.fm, pfmGain, S, GAIN_FLAG)
					END_PFM_CREATE
					pfmGain->sideidFlag = NA;
					pfmGain->shipidRecipient = pship->GetObjectID();
					pfmGain->bIsTreasureDocked = true;
					pfmGain->parttypeidDocked = ppart->GetPartType()->GetObjectID();
					g.fm.SendMessages(CFSSide::FromIGC(pship->GetSide())->GetGroup(), FM_GUARANTEED, FM_FLUSH);
				}
				// end of KGJV #118

            }

            IsideIGC*   pside = pship->GetSide();
            assert (pside);
			if (pside->ApplyDevelopmentTechs(ttbm))
			{
				pfsShip->GetPlayerScoreObject()->AddTechsRecovered();
				CSteamAchievements *pSteamAchievements = pfsShip->GetPlayer()->GetSteamAchievements();
				pSteamAchievements->AwardRecoverTechAchievement();
			}
		}
		
		if (pstationNew && pship && !pship->GetParentShip() && pship->GetBaseHullType() && //imago fix crash when boarding turret 7/22/09
			pship->GetBaseHullType()->HasCapability(c_habmLifepod) && pstationNew->GetSide() != pship->GetSide()) {
			//dont dock here in a lifepod....
        } else if (pstationNew && pship && pstationNew->GetSide() != pship->GetSide() && (pship->GetChildShips()->n() != 0 || pship->GetParentShip() != NULL)) {
            //dont dock here with turrets....
		} else {
        	pfsShip->Dock(pstationNew);
		}

        if (!pfsShip->IsPlayer())
            UpgradeShip(pship, pstationNew);
		// #ALLY  -imago 7/3/09 added allied check and repair/refuel REARM
		// if allied station, relaunch now, unless we're in a pod, then delete ourselves and goto our last base
		if (pfsShip->IsPlayer() && (pstationNew->GetSide() != pship->GetSide()) && pship->GetSide()->AlliedSides(pstationNew->GetSide(),pship->GetSide()))
		{
			CFSPlayer*  pfsPlayer = ((CFSShip*)(pship->GetPrivateData()))->GetPlayer();
			if ((pship->GetParentShip() == NULL) && pship->GetBaseHullType()->HasCapability(c_habmLifepod)) {

				//Fixed lifepods, we can't use DockWithStation() 
				//   'cause FindTarget will return allied base if closer 7/13/09 
				// imago NYI move all this #ALLY in changestation ugly to a pretty DockWithAlliedStation() handler
  				pfsShip->ResetWarpState();				
				
				//Find the LAST space station for this pilot and put the rescued ship there  //Imago 7/13/09
				 IstationIGC* pstation = m_pfsMission->GetIGCMission()->GetStation(pfsShip->GetOldShipStatus(pship->GetSide()->GetObjectID())->GetStationID());
				if (pstation == NULL || 
					pstation->GetSide() != pship->GetSide()) //Imago 7/22/09 fix restart in ally base after a rearm
					pstation = m_pfsMission->GetBase(pship->GetSide());

				  //Find the LAST ship type we used
				  IhullTypeIGC* pht = m_pfsMission->GetIGCMission()->GetHullType(pfsShip->GetOldShipStatus(pship->GetSide()->GetObjectID())->GetHullID());

		            //No saved hull (in non lifepod group)
				if ((pht && pht->GetGroupID() < 0) || (pht == NULL)) {
		            HullTypeLinkIGC*   phtl = m_pfsMission->GetIGCMission()->GetHullTypes()->last();
		            while (true)
		            {
		                assert (phtl != NULL);
		                pht = phtl->data();
		                if ((pht->GetGroupID() >= 0) && pstation->CanBuy(pht) && (pht->GetPrice() <= 0))
		                {
		                    pht = (IhullTypeIGC*)(pstation->GetSuccessor(pht));
		                    break;
		                }
						phtl = phtl->txen();
					}

				}
				pfsPlayer->GetIGCShip()->SetBaseHullType(pht);
				pfsShip->Dock(pstation);

				BEGIN_PFM_CREATE(g.fm, pfmShipDelete, S, SHIP_DELETE)
				  END_PFM_CREATE
				  pfmShipDelete->shipID = pship->GetObjectID();
				  pfmShipDelete->sdr = SDR_DOCKED;
				  g.fm.SendMessages(m_pfsMission->GetGroupMission(), FM_GUARANTEED, FM_FLUSH); //HACK: announce we docked to the whole mission, this should be cluster.

				pfsPlayer->GetIGCShip()->SetStation(pstation);
  				pfsShip->ShipStatusDocked(pstation);
				pfsPlayer->ForceLoadoutChange();

            } else { //not a pod --

                if (pship->GetChildShips()->n() != 0 && pship->GetParentShip() == NULL) { //imago 8/10/09 rearm @ ALLY base /w turret fix
                    for (ShipLinkIGC*   psl = pship->GetChildShips()->first();
                         (psl != NULL);
                         psl = psl->next())
                    {
                        CFSPlayer*  pfscPlayer = ((CFSShip*)(psl->data()->GetPrivateData()))->GetPlayer();
                        pfscPlayer->Dock(pstationNew);
                    }
                    pfsPlayer->Dock(pstationNew);
                
                } else if (pship->GetParentShip() == NULL) { //not a turret --
                    AlliedRearm(pship,true); 
                }
			}
		}
	}
}

void * FedSrvSiteBase::GetDroneFromShip(IshipIGC * pship) // return value is really a Drone*
{
  CFSShip * pfsShip = (CFSShip *) pship->GetPrivateData();
  return pfsShip->IsPlayer() ? NULL : (void *) pfsShip->GetDrone()->GetDrone();
}

void FedSrvSiteBase::RequestRipcord(IshipIGC* pship, IclusterIGC* pcluster)
{
    IclusterIGC*    pclusterShip = pship->GetCluster();

    CFSShip*    pfsShip = (CFSShip *)pship->GetPrivateData();
    CFSMission* pfsMission = pfsShip->GetMission();

    ImodelIGC*    pmodelRipcord = pship->GetRipcordModel();
    if (pcluster == NULL)
    {
        if (pmodelRipcord != NULL)
        {
            assert (pclusterShip);

            pship->SetRipcordModel(NULL);
			pship->SetStateBits(droneRipMaskIGC, 0);  //Xynth #188 7/2010
            BEGIN_PFM_CREATE(g.fm, pfmRipcordAborted, S, RIPCORD_ABORTED)
            END_PFM_CREATE
            pfmRipcordAborted->shipidRipcord = pship->GetObjectID();
            g.fm.SendMessages(GetGroupSectorFlying(pclusterShip), FM_GUARANTEED, FM_FLUSH);
        }
    }
    else if ((pmodelRipcord == NULL) ||
             (pmodelRipcord->GetCluster() != pcluster))
    {
        assert (pclusterShip);

        ImodelIGC*    pmodelNew = pship->FindRipcordModel(pcluster);
        if (pmodelNew)
        {
            ImodelIGC*  pmodelOld = pship->GetRipcordModel();
            if (pmodelNew != pmodelOld)
            {
                pship->SetRipcordModel(pmodelNew);
                pship->ResetRipcordTimeLeft();
            }

            if ((pmodelNew != pmodelOld) && pfsShip->IsPlayer())
            {
                BEGIN_PFM_CREATE(g.fm, pfmRA, S, RIPCORD_ACTIVATE)
                END_PFM_CREATE
                pfmRA->shipidRipcord = pship->GetObjectID();

                pfmRA->otRipcord = pmodelNew->GetObjectType();
                pfmRA->oidRipcord = pmodelNew->GetObjectID();
                pfmRA->sidRipcord = pcluster->GetObjectID();

                g.fm.SendMessages((pmodelOld == NULL)
                                  ? (CFMRecipient*)(GetGroupSectorFlying(pclusterShip))
                                  : (CFMRecipient*)(pfsShip->GetPlayer()->GetConnection()), FM_GUARANTEED, FM_FLUSH);
            }
        }
        else if (pfsShip->IsPlayer())
        {
            BEGIN_PFM_CREATE(g.fm, pfmRipcordDenied, S, RIPCORD_DENIED)
            END_PFM_CREATE
            g.fm.SendMessages(pfsShip->GetPlayer()->GetConnection(), FM_GUARANTEED, FM_FLUSH);
        }
    }
}
bool FedSrvSiteBase::ContinueRipcord(IshipIGC* pship, ImodelIGC* pmodel)
{
    bool    rc;

	//ally
    CFSShip*    pfsShip = (CFSShip *)pship->GetPrivateData();
    CFSMission* pfsMission = pfsShip->GetMission();

	if (pfsMission->GetMissionDef()->misparms.bAllowAlliedRip) {
	    if ((pship->GetFlag() != NA) ||
	        ((pship->GetSide() != pmodel->GetSide()) && (!pship->GetSide()->AlliedSides(pship->GetSide(),pmodel->GetSide()))) || //ALLY RIPCORD imago 7/8/09
	        (pmodel->GetCluster() == NULL))
	    {
	        rc = false;
	    }
	    else if (pmodel->GetObjectType() == OT_ship)
	    {
	        IshipIGC*   ps = (IshipIGC*)pmodel;

	        IhullTypeIGC*   pht = ps->GetBaseHullType();
	        rc = pht &&
	             pht->HasCapability(pship->GetBaseHullType()->HasCapability(c_habmCanLtRipcord)
	                                ? (c_habmIsRipcordTarget | c_habmIsLtRipcordTarget)
	                                : c_habmIsRipcordTarget);
	    }
	    else
	        rc = true;
	} else { //regular
	    if ((pship->GetFlag() != NA) ||
	        (pship->GetSide() != pmodel->GetSide()) ||
	        (pmodel->GetCluster() == NULL))
	    {
	        rc = false;
	    }
	    else if (pmodel->GetObjectType() == OT_ship)
	    {
	        IshipIGC*   ps = (IshipIGC*)pmodel;

	        IhullTypeIGC*   pht = ps->GetBaseHullType();
	        rc = pht &&
	             pht->HasCapability(pship->GetBaseHullType()->HasCapability(c_habmCanLtRipcord)
	                                ? (c_habmIsRipcordTarget | c_habmIsLtRipcordTarget)
	                                : c_habmIsRipcordTarget);
	    }
	    else
	        rc = true;
	}

    if (!rc)
    {
		pship->SetStateBits(droneRipMaskIGC, 0);  //Xynth #188 7/2010
        BEGIN_PFM_CREATE(g.fm, pfmRipcordAborted, S, RIPCORD_ABORTED)
        END_PFM_CREATE
        pfmRipcordAborted->shipidRipcord = pship->GetObjectID();
        g.fm.SendMessages(GetGroupSectorFlying(pship->GetCluster()), FM_GUARANTEED, FM_FLUSH);

    }

    return rc;
}

bool FedSrvSiteBase::UseRipcord(IshipIGC* pship, ImodelIGC* pmodel)
{
    assert (pmodel);

    ObjectType  type = pmodel->GetObjectType();

    if (type == OT_station)
        ((IstationIGC*)pmodel)->Launch(pship);
    else
    {
        if (type == OT_ship)
        {
            float   newEnergy = ((IshipIGC*)pmodel)->GetEnergy() - pship->GetBaseHullType()->GetRipcordCost();
            if (newEnergy < 0.0f)
            {
				pship->SetStateBits(droneRipMaskIGC, 0); //Xynth #188 7/2010
                //No energy for ripcord
                BEGIN_PFM_CREATE(g.fm, pfmRipcordAborted, S, RIPCORD_ABORTED)
                END_PFM_CREATE
                pfmRipcordAborted->shipidRipcord = pship->GetObjectID();
                g.fm.SendMessages(GetGroupSectorFlying(pship->GetCluster()), FM_GUARANTEED, FM_FLUSH);

                return true;
            }
            else
                ((IshipIGC*)pmodel)->SetEnergy(newEnergy);
        }

        float   r = pmodel->GetRadius() + pship->GetRadius() + 25.0f;
        Vector  v = Vector::RandomDirection();
        Orientation o(v);

        IclusterIGC*    pcluster = pmodel->GetCluster();
        Time    lastUpdate = pcluster->GetLastUpdate();

        pship->SetPosition(pmodel->GetPosition() + v * r);
        pship->SetVelocity(v * m_pfsMission->GetIGCMission()->GetFloatConstant(c_fcidExitStationSpeed));
        pship->SetOrientation(o);
        pship->SetCurrentTurnRate(c_axisYaw, 0.0f);
        pship->SetCurrentTurnRate(c_axisPitch, 0.0f);
        pship->SetCurrentTurnRate(c_axisRoll, 0.0f);

        pship->SetCluster(pcluster);

		//FAZ: this is where tp1/tp2 are randomly destroyed (1/6 chance)
		// future mod: use ((IprobeIGC)pmodel) to access the igcprobe datas
		// mmf comment out for now
		//     this is causing a server crash, not every time, but seems like if TP is destroyed while
		//     someone else is ripping to it, boom, server goes down
		//     we crashed it with a TP1 dropped in open space (not near an asteroid)
        /*
		if ((type == OT_probe) && (randomInt(0, 5) == 0))
        {
			// mmf debug
			debugf("mmf debug teleprobe random destroy\n");
            KillProbeEvent((IprobeIGC*)pmodel);
            //pmodel->Terminate();
        }
		*/
    }

    CFSShip* pfsShip = (CFSShip*)pship->GetPrivateData();
    pfsShip->ShipStatusSpotted(pship->GetSide());

    return true;
}
void FedSrvSiteBase::HitWarpEvent(IshipIGC* pship, IwarpIGC* pwarp)
{
  CFSShip * pfsShip = (CFSShip *) pship->GetPrivateData();

  pfsShip->HitWarp(pwarp);
}

void FedSrvSiteBase::ChangeCluster(IshipIGC* pship,
                                   IclusterIGC* pclusterOld,
                                   IclusterIGC* pclusterNew)
{
    CFSShip* pfsShip = (CFSShip *) pship->GetPrivateData();

    //Only announce my exit from the old cluster if going from one cluster to another
    //If going from a valid cluster, then
    //  I'm either docking (and an announce exit is sent by the dock event)
    //  I'm dying (and an announce exit is sent by the thing that killed me)
    //  I'm leaving the game (ditto)
    if ((pclusterOld && pclusterNew) && (pclusterOld != pclusterNew))
        pfsShip->AnnounceExit(pclusterOld, SDR_LEFTSECTOR);

    pfsShip->SetCluster(pclusterNew);
}

void FedSrvSiteBase::LoadoutChangeEvent(IshipIGC* pship, IpartIGC* ppart, LoadoutChange lc)
{
    // if we are mounting or dismounting an area effect weapon...
    if (lc == c_lcAdded && ppart->GetEquipmentType() == ET_Weapon
        && ((IweaponIGC*)ppart)->GetProjectileType()->GetBlastRadius() > 0.0f)
    {
        // REVIEW: we could add a check to see if there was already an area effect weapon mounted,
        // or even cache the last target that was broadcast.  It's probably not worth maintaining
        // the extra code, however.

        // ...resend the current target
        CommandChangedEvent(c_cmdCurrent, pship,
            pship->GetCommandTarget(c_cmdCurrent),
            pship->GetCommandID(c_cmdCurrent));
    }

    /*
    if (lc != c_lcTurretChange)
    {
        CFSShip* pfsShip = (CFSShip *) pship->GetPrivateData();
        if (pfsShip)
        {
            pfsShip->ShipLoadoutChange(lc);
        }
    }
    */
}

bool FedSrvSiteBase::HitTreasureEvent(Time now, IshipIGC* ship, ItreasureIGC* treasure)
{
    TreasureCode    tc = treasure->GetTreasureCode();
    ObjectID        oid = treasure->GetTreasureID();

    if (tc == c_tcFlag)
    {
        //Can only carry a single flag
        if ((ship->GetFlag() != NA) || (oid == ship->GetSide()->GetObjectID()))
        {
            return false;
        }

        BEGIN_PFM_CREATE(g.fm, pfmGain, S, GAIN_FLAG)
        END_PFM_CREATE
        pfmGain->sideidFlag = oid;
        pfmGain->shipidRecipient = ship->GetObjectID();
		pfmGain->bIsTreasureDocked = false; // KGJV #118
        g.fm.SendMessages(m_pfsMission->GetGroupRealSides(), FM_GUARANTEED, FM_FLUSH);

        //Clear the flag so it does not respawn
        treasure->SetTreasureID(NA);
    }

    short           amount = treasure->GetAmount();

    if (tc == c_tcPart)
    {
        // if this was a player, the message below will take care of it.
        // REVIEW: shouldn't drones pick up treasure if they can?
    }
    else if (tc == c_tcCash)
    {
        CFSShip*    pfsShip = (CFSShip*)(ship->GetPrivateData());
        if (pfsShip->IsPlayer())
        {
            IshipIGC*   pshipDonate = ship->GetAutoDonate();
            if (pshipDonate == NULL)
                pshipDonate = ship;

            CFSShip*    pfsDonate = (CFSShip*)(pshipDonate->GetPrivateData());
            assert (pfsDonate->IsPlayer());
            assert (amount >= 0);

            BEGIN_PFM_CREATE(g.fm, pfmMoney, S, MONEY_CHANGE)
            END_PFM_CREATE

            pfmMoney->sidTo = pshipDonate->GetObjectID();
            pfmMoney->sidFrom = NA;
            pfmMoney->dMoney = amount;

            pfsDonate->SetMoney(pfsDonate->GetMoney() + amount);
            g.fm.SendMessages(CFSSide::FromIGC(pfsShip->GetSide())->GetGroup(), FM_GUARANTEED, FM_FLUSH);
        }
        else
        {
          m_pfsMission->GiveSideMoney(ship->GetSide(), Money(amount));
        }
    }
    else
        ship->HitTreasure(tc, oid, amount);

    CFSShip*        pfsShip = (CFSShip*)(ship->GetPrivateData());

    if (pfsShip->IsPlayer())
    {
        CFSPlayer*      pfsPlayer = pfsShip->GetPlayer();

        //As long as either it isn't a part or there is space in the treasure buffer
        if ((tc != c_tcPart) || (pfsPlayer->GetTreasureObjectID() == NA))
        {
            //Tell the player they got it & let them deal with it.
            if (tc == c_tcPart)
                pfsPlayer->SetTreasureData(oid, amount);

            BEGIN_PFM_CREATE(g.fm, pfmAT, S, ACQUIRE_TREASURE)
            END_PFM_CREATE

            pfmAT->treasureCode = tc;
            pfmAT->treasureID = oid;
            pfmAT->amount = amount;

            g.fm.SendMessages(pfsPlayer->GetConnection(), FM_GUARANTEED, FM_FLUSH);
        }
    }

    return true;
}

void FedSrvSiteBase::PaydayEvent(IsideIGC* pside, float money)
{
  m_pfsMission->GiveSideMoney(pside, Money(money));
}

void FedSrvSiteBase::SendChat(IshipIGC*     pshipSender,
                              ChatTarget    chatTarget,
                              ObjectID      oidRecipient,
                              SoundID       soVoiceOver,
                              const char*   pszText,
                              CommandID     cid,
                              ObjectType    otTarget,
                              ObjectID      oidTarget,
                              ImodelIGC*    pmodelTarget,
                              bool          bObjectModel)
{
    BEGIN_PFM_CREATE_ALLOC(g.fm, pfmChatAlloc, CS, CHATMESSAGE)
      FM_VAR_PARM(pszText, CB_ZTS)
    END_PFM_CREATE

    pfmChatAlloc->cd.sidSender = pshipSender ? pshipSender->GetObjectID()
                                             : NA;

    pfmChatAlloc->cd.chatTarget = chatTarget;
    pfmChatAlloc->cd.oidRecipient = oidRecipient;

    pfmChatAlloc->cd.commandID = cid;
    pfmChatAlloc->cd.voiceOver = soVoiceOver;
    pfmChatAlloc->cd.bObjectModel = bObjectModel;

    pfmChatAlloc->otTarget = otTarget;
    pfmChatAlloc->oidTarget = oidTarget;

    ForwardChatMessage(m_pfsMission,
                       pshipSender
                       ? (CFSShip*)(pshipSender->GetPrivateData())
                       : NULL,
                       pfmChatAlloc,
                       &(pfmChatAlloc->cd),
                       pszText,
                       otTarget,
                       oidTarget,
                       NULL);

    PFM_DEALLOC(pfmChatAlloc);
}

void FedSrvSiteBase::ClusterUpdateEvent(IclusterIGC* c)
{

}

void FedSrvSiteBase::CommandChangedEvent(Command i, IshipIGC * pship, ImodelIGC* ptarget, CommandID cid)
{
    // forward the command change to everyone on the side.
    IclusterIGC*    pcluster;

    if ((i == c_cmdAccepted) ||
        ((i == c_cmdCurrent) &&
         (pship->GetPilotType() >= c_ptPlayer) &&
         (pcluster = pship->GetCluster()) && //intentional =
         (pship->IsUsingAreaOfEffectWeapon())))
    {
        BEGIN_PFM_CREATE(g.fm, pfmOC, CS, ORDER_CHANGE)
        END_PFM_CREATE
        pfmOC->command = i;
        pfmOC->shipID = pship->GetObjectID();
        pfmOC->commandID = cid;
        if (ptarget)
        {
            pfmOC->objectType = ptarget->GetObjectType();
            pfmOC->objectID = ptarget->GetObjectID();
        }
        else
        {
            pfmOC->objectType = OT_invalid;
            pfmOC->objectID = NA;
        }

        bool fCurrentTarget = (i == c_cmdCurrent);
        g.fm.SendMessages(fCurrentTarget
                              ? GetGroupSectorFlying(pcluster)
                              : CFSSide::FromIGC(pship->GetSide())->GetGroup(),
                          fCurrentTarget ? FM_NOT_GUARANTEED : FM_GUARANTEED, FM_FLUSH);
    }
}
void     FedSrvSiteBase::CreateBuildingEffect(Time           now,
                                              IasteroidIGC*  pasteroid,
                                              IshipIGC*      pshipBuilder)
{
    assert (pasteroid);
    assert (pshipBuilder);
    assert (pshipBuilder->GetPilotType() == c_ptBuilder);
    assert (pshipBuilder->GetBaseData());
    assert (pshipBuilder->GetCluster());

    IbuildingEffectIGC*    pbe = pshipBuilder->GetCluster()->CreateBuildingEffect(now,
                                                     pasteroid,
                                                     NULL,
                                                     pshipBuilder,
                                                     pasteroid->GetRadius(),
                                                     ((IstationTypeIGC*)(pshipBuilder->GetBaseData()))->GetRadius(),
                                                     pshipBuilder->GetPosition() - pshipBuilder->GetOrientation().GetBackward() * pshipBuilder->GetRadius(),
                                                     pasteroid->GetPosition());

    //Export the building effect to all sides that could see the asteroid - Imago changed to "see the builder", see below #120 #121
	bool bseenside[c_cSidesMax] = {false};
	ImodelIGC * pmodelBuilder = m_pfsMission->GetIGCMission()->GetModel(pshipBuilder->GetObjectType(),pshipBuilder->GetObjectID());
	ImodelIGC * pmodelRoid = m_pfsMission->GetIGCMission()->GetModel(pasteroid->GetObjectType(),pasteroid->GetObjectID());
	m_pfsMission->GetIGCMission()->GetSeenSides(pmodelBuilder,bseenside,pmodelRoid);
    for (SideLinkIGC*   psl = m_pfsMission->GetIGCMission()->GetSides()->first();
         (psl != NULL);
         psl = psl->next())
    {
		if ((bseenside[psl->data()->GetObjectID()]) || pasteroid->GetAsteroidCurrentEye(psl->data()) || pshipBuilder->GetSide()->GetObjectID() == psl->data()->GetObjectID()) {
			pasteroid->SetBuilderSeenSide(psl->data()->GetObjectID());
			ExportObj(pbe, OT_buildingEffect, NULL);
			g.fm.SendMessages(CFSSide::FromIGC(psl->data())->GetGroup(), FM_GUARANTEED, FM_FLUSH);
		}
    }
}

void        FedSrvSiteBase::LayExpendable(Time                   now,
                                          IexpendableTypeIGC*    pet,
                                          IshipIGC*              pshipLayer)
{
    assert (pet);
    assert (pshipLayer);

    ObjectType  type = pet->GetObjectType();

    ImissionIGC*    pMission = m_pfsMission->GetIGCMission();

    const Vector&   position = pshipLayer->GetPosition();
    IclusterIGC*    pcluster = pshipLayer->GetCluster();
    IsideIGC*       pside = pshipLayer->GetSide();

    if (type == OT_mineType)
    {
        DataMineIGC  dm;
        dm.pshipLauncher = NULL;
        dm.psideLauncher = pside;
        dm.mineID = pMission->GenerateNewMineID();

        dm.time0 = now;
        dm.p0 = position;
        dm.pminetype = (ImineTypeIGC*)pet;
        assert (dm.pminetype);

        dm.pcluster = pcluster;
        dm.exportF = false;

        ImineIGC * m = (ImineIGC*)(pMission->CreateObject(now, OT_mine, &dm, sizeof(dm)));

        assert (m);

        m->Release();
    }
    else
    {
        assert (type == OT_probeType);

        DataProbeIGC dp;
        dp.pside = pside;
        dp.pship = NULL;
        dp.pmodelTarget = NULL;
        dp.probeID = pMission->GenerateNewProbeID();

        dp.time0 = now;
        dp.p0 = position;
        dp.pprobetype = (IprobeTypeIGC*)pet;
        assert (dp.pprobetype);

        dp.pcluster = pcluster;
        dp.exportF = false;

        IprobeIGC * p = (IprobeIGC*)(pMission->CreateObject(now, OT_probe, &dp, sizeof(dp)));

        assert (p);
        p->Release();
    }

    //Quietly kill the ship (after nuking its parts to prevent treasure from being created)
    {
        const PartListIGC*  parts = pshipLayer->GetParts();
        PartLinkIGC*    plink;
        while (plink = parts->first())  //Not ==
            plink->data()->Terminate();
    }
    pshipLayer->SetAmmo(0);
    pshipLayer->SetFuel(0.0f);

    KillShipEvent(now, pshipLayer, NULL, 0.0f, position, Vector::GetZero());
}

void    FedSrvSiteBase::RespawnFlag(SideID  sidFlag, IsideIGC* pcapturersside) // mmf 11/07 added second argument pside of capturer
{
    //Don't respawn flags that have just been picked up by a ship
    if (sidFlag != NA)
    {
        if (sidFlag == SIDE_TEAMLOBBY)
        {
            //Legal clusters are any neutral cluster without a flag
            const int       c_maxClusters = 100;
            IclusterIGC*    pclusters[c_maxClusters];
            int             nClusters = 0;

            for (ClusterLinkIGC*    pcl = m_pfsMission->GetIGCMission()->GetClusters()->first();
                 (pcl != NULL);
                 pcl = pcl->next())
            {
                IclusterIGC*   pcluster = pcl->data();
                if (!pcluster->GetHomeSector())
                {
                    assert (nClusters < c_maxClusters);
                    pclusters[nClusters++] = pcluster;
                }
            }

            assert (nClusters != 0);

            m_pfsMission->GetIGCMission()->GenerateTreasure(g.timeNow, pclusters[randomInt(0, nClusters - 1)], -2);
        }
        else
        {
            IsideIGC*   pside = m_pfsMission->GetIGCMission()->GetSide(sidFlag);
            assert(pside);

            //Legal clusters are any neutral cluster without a flag
            const int       c_maxStations = 10;
            IstationIGC*    pstations[c_maxStations];
            int             nStations = 0;

            for (StationLinkIGC* psl = pside->GetStations()->first(); (psl != NULL); psl = psl->next())
            {
                IstationIGC*    pstation = psl->data();
                if (pstation->GetStationType()->HasCapability(c_sabmPedestal))
                {
                    //Is there a flag within spitting distance?
                    const float c_spittingDistance = 300.0f;
                    TreasureLinkIGC*   ptl;
                    for (ptl = pstation->GetCluster()->GetTreasures()->first(); (ptl != NULL); ptl = ptl->next())
                    {
                        ItreasureIGC*   pt = ptl->data();
                        if ((pt->GetTreasureCode() == c_tcFlag) && (pt->GetTreasureID() == sidFlag))
                        {
                            float   d = (pt->GetPosition() - pstation->GetPosition()).LengthSquared();
                            if (d < c_spittingDistance * c_spittingDistance)
                                break;
                        }
                    }

                    if (ptl == NULL)
                    {
                        assert (nStations < c_maxStations);
                        pstations[nStations++] = pstation;
                    }
                }
            }
            // assert (nStations > 0); mmf 11/07 commented this out as this can happen in 'only one side' games

			// mmf 11/07 add if nStations == 0
			//     If the side that the flag belongs to has no bases AND one of their flags was captured
			//     the side that captured it wins.
			//     Otherwise don't spawn a flag (there is no base to get a position for).
			//     It is unlikely this second case will occur but best to cover it just incase.
			if (nStations == 0) {
				if (pcapturersside != NULL) {
					static char szReason[100];	//Make this static so we only need to keep a pointer to it around
					sprintf(szReason,"%s wins. Team owning captured flag has no stations left.",
                      pcapturersside->GetName());
					m_pfsMission->GameOver(pcapturersside, szReason);
				}
			} else {
				IstationIGC*    pstation = pstations[randomInt(0, nStations - 1)];
				DataTreasureIGC dt;
	            dt.treasureCode = c_tcFlag;
	            dt.treasureID = sidFlag;
	            dt.amount = 0;
				dt.clusterID = pstation->GetCluster()->GetObjectID();
				dt.lifespan = 10.0f * 24.0f * 3600.0f;
				dt.createNow = false;

				dt.objectID = m_pfsMission->GetIGCMission()->GenerateNewTreasureID();

				dt.p0 = pstation->GetPosition();
	            dt.p0.z += pstation->GetRadius() + c_fFlagOffset;
				dt.v0 = Vector::GetZero();
				dt.time0 = g.timeNow;

				ItreasureIGC* t = (ItreasureIGC *)m_pfsMission->GetIGCMission()->CreateObject(g.timeNow, OT_treasure,
                                                                                          &dt, sizeof(dt));
				assert (t);
				t->Release();
			}
		}
    }
}

/*-------------------------------------------------------------------------
 * FedSrvSiteBase::CreateSideEvent()
 *-------------------------------------------------------------------------
 * Purpose:
 *   When a side is created in IGC, this is called.  This in turn
 *   creates the fsside
 */
void FedSrvSiteBase::CreateSideEvent(IsideIGC * pIsideIGC)
{
  m_pfsMission->CreateSide(pIsideIGC);
}

/*-------------------------------------------------------------------------
 * FedSrvSiteBase::DestroySideEvent()
 *-------------------------------------------------------------------------
 * Purpose:
 *    When a side is destroyed in IGC, this is called.  This
 *    in turn destroys the fsside.
 */
void FedSrvSiteBase::DestroySideEvent(IsideIGC * pIsideIGC)
{
  m_pfsMission->DeleteSide(pIsideIGC);
}



/*-------------------------------------------------------------------------
 * FedSrvSiteBase::Destroy()
 *-------------------------------------------------------------------------
 */
void FedSrvSiteBase::Destroy(CFSMission * pfsMission)
{
  delete this;
}

IshipIGC*   PickNewLeader(const ShipListIGC*    pships,
                          IshipIGC*             pshipIgnore,
                          int                   maxVotes)
{
    struct Contender
    {
        IshipIGC* pship;
        int       nVotes;
    };

    Contender*    pcontenders = new Contender[pships->n()];
    int           nContenders = 0;

    IshipIGC*     pshipMax = NULL;
    for (ShipLinkIGC* psl = pships->first(); (psl != NULL); psl = psl->next())
    {
        IshipIGC* pshipAD = psl->data()->GetAutoDonate();
        if (pshipAD && (pshipAD != pshipIgnore))
        {
            int   i;
            for (i = 0; (i < nContenders); i++)
            {
                if (pcontenders[i].pship == pshipAD)
                {
                    pcontenders[i].nVotes++;
                    break;
                }
            }

            if (i == nContenders)
            {
                pcontenders[nContenders].pship = pshipAD;
                pcontenders[nContenders++].nVotes = 1;
            }

            if (pcontenders[i].nVotes > maxVotes)
            {
                maxVotes = pcontenders[i].nVotes;
                pshipMax = pcontenders[i].pship;
            }
        }
    }
    delete pcontenders;

    return pshipMax;
}
/*-------------------------------------------------------------------------
 * FedSrvApp
 *-------------------------------------------------------------------------
 * Purpose:
 *    This class is called by the assert code when an assert happens
 */

// mmf log to file on SRVLOG define as well as _DEBUG
#ifdef _DEBUG
#define SRVLOG
#endif

#ifdef SRVLOG // mmf changed this from _DEBUG

    void    FlushDebugLog(void)
    {
        g_app.CloseLogFile();
    }

    VOID CALLBACK FileIOCompletionRoutine(
          DWORD dwErrorCode,                // completion code
          DWORD dwNumberOfBytesTransfered,  // number of bytes transferred
          LPOVERLAPPED lpOverlapped         // I/O information buffer
    )
    {
        delete [] (char*)lpOverlapped;
    }

    void FedSrvApp::AsyncFileOut(const char *psz)
    {
      SYSTEMTIME systime;
      DWORD l = strlen(psz);
      const DWORD c_cbPrefix = 19; // time format: 12/31 23:59:59.999
      DWORD cbLine = l + c_cbPrefix;
      OVERLAPPED* pov = (OVERLAPPED*)(new char [ sizeof(OVERLAPPED) + cbLine ]);
      *pov = m_overlapped;
      GetLocalTime(&systime);
      wsprintf((char*)(pov + 1), "%02d/%02d %02d:%02d:%02d.%03d ", systime.wMonth, systime.wDay,
          systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);
      //_strtime((char*)(pov + 1));
      //((char*)(pov + 1))[8] = ' ';

      memcpy(((char*)(pov + 1)) + c_cbPrefix, psz, l);
      pov->Offset = ::InterlockedExchangeAdd(&m_nOffset, cbLine);
      WriteFileEx(m_hFile, pov + 1, cbLine, pov, FileIOCompletionRoutine);
    }

    void FedSrvApp::DebugOutput(const char *psz)
    {
        static TCAutoCriticalSection critsec;
        Time    tStart = Time::Now();
        critsec.Lock();
          if (m_dwDebug & FED_DEBUG_FILE)
              AsyncFileOut(psz);

          if (m_dwDebug & FED_DEBUG_DEBUGOUT)
              ::OutputDebugStringA(psz);

          Time    tStop = Time::Now();

          DWORD   dt = (tStop.clock() - tStart.clock());
          if (dt > 20)
          {
              static char    bfr[200];
              sprintf(bfr, "DebugOutput delay of %f: %d %d\n", float(dt) / 1000.0f, tStart.clock(), tStop.clock());
              if (m_dwDebug & FED_DEBUG_FILE)
                  AsyncFileOut(bfr);

              if (m_dwDebug & FED_DEBUG_DEBUGOUT)
                  ::OutputDebugStringA(bfr);
          }
        critsec.Unlock();
    }

    bool FedSrvApp::OnAssert(const char* psz, const char* pszFile, int line, const char* pszModule)
    {
        _AGCModule.TriggerEvent(NULL, EventID_AGCAssert, "", -1, -1, -1, 3,
            "File", VT_LPSTR, pszFile,
            "Line", VT_I4, line,
            "Text", VT_LPSTR, psz);

        return g.fWantInt3;
    }

    void FedSrvApp::OnAssertBreak()
    {
        if (m_hFile)
        {
            CloseHandle(m_hFile);
            m_hFile = NULL;
        }

        *(DWORD*)0 = 0;
        //DebugBreak();
    }

    // Global Initialization
    FedSrvApp g_app;

#endif
