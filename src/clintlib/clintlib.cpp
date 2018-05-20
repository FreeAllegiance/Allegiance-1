#include "pch.h"

#include <AllegianceSecurity.h>
#include <ClubMessages.h>
#include <guids.h>
#include <regkey.h>

#include "AutoDownload.h"

ALLOC_MSG_LIST;

bool g_bCheckFiles = false;

/////////////////////////////////////////////////////////////////////////////
// ClientEventSource

class ClientEventSource : public IClientEventSource 
{
private:
    TList<TRef<IClientEventSink> >   m_listSinks;

    void RemoveDeadSinks()
    {
        // remove all of the NULL elements from the list
        while (m_listSinks.Remove(NULL)) 
            {
            }
    }
    
public:
    void AddSink(IClientEventSink* psink)
    {
        m_listSinks.PushFront(psink);
    }
    
    void RemoveSink(IClientEventSink* psink)
    {
        // in order to let us remove the current sync while walking the list of
        // sinks, just set the sink to NULL for the moment.  
        m_listSinks.Replace(psink, NULL);
    }

#define ForEachSink(fnc) \
        TList<TRef<IClientEventSink> >::Iterator iter(m_listSinks); \
        while (!iter.End()) \
            { \
            if (iter.Value() != NULL) \
                iter.Value()->fnc; \
            iter.Next(); \
            } \
        RemoveDeadSinks(); \

    void OnAddMission(MissionInfo* pMissionInfo)
    {
        ForEachSink( OnAddMission(pMissionInfo) )
    }

    void OnMissionCountdown(MissionInfo* pMissionInfo)
    {
        ForEachSink( OnMissionCountdown(pMissionInfo) )
    }
    
    void OnMissionStarted(MissionInfo* pMissionInfo)
    {
        ForEachSink( OnMissionStarted(pMissionInfo) )
    }
    
    void OnMissionEnded(MissionInfo* pMissionInfo)
    {
        ForEachSink( OnMissionEnded(pMissionInfo) )
    }

    void OnLockLobby(bool bLock)
    {
        ForEachSink( OnLockLobby(bLock) )
    }

    void OnLockSides(bool bLock)
    {
        ForEachSink( OnLockSides(bLock) )
    }

    void OnFoundPlayer(MissionInfo* pMissionInfo)
    {
        ForEachSink( OnFoundPlayer(pMissionInfo) )
    }

    void OnEnterMission()
    {
        ForEachSink( OnEnterMission() )
    }

    void OnDelMission(MissionInfo* pMissionInfo)
    {
        ForEachSink( OnDelMission(pMissionInfo) )
    }
    
    void OnAddPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        ForEachSink( OnAddPlayer(pMissionInfo, sideID, pPlayerInfo) )
    }

    void OnDelPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
    {
        ForEachSink( OnDelPlayer(pMissionInfo, sideID, pPlayerInfo, reason, szMessageParam) )
    }

    void OnAddRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        ForEachSink( OnAddRequest(pMissionInfo, sideID, pPlayerInfo) )
    }

    void OnDelRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason)
    {
        ForEachSink( OnDelRequest(pMissionInfo, sideID, pPlayerInfo, reason) )
    }

    void OnTeamInactive(MissionInfo* pMissionInfo, SideID sideID)
    {
        ForEachSink( OnTeamInactive(pMissionInfo, sideID) )
    }

    void OnTeamReadyChange(MissionInfo* pMissionInfo, SideID sideID, bool fTeamReady)
    {
        ForEachSink( OnTeamReadyChange(pMissionInfo, sideID, fTeamReady) )
    }

    void OnTeamForceReadyChange(MissionInfo* pMissionInfo, SideID sideID, bool fTeamForceReady)
    {
        ForEachSink( OnTeamForceReadyChange(pMissionInfo, sideID, fTeamForceReady) )
    }

    void OnTeamAutoAcceptChange(MissionInfo* pMissionInfo, SideID sideID, bool fAutoAccept)
    {
        ForEachSink( OnTeamAutoAcceptChange(pMissionInfo, sideID, fAutoAccept) )
    }

    void OnTeamCivChange(MissionInfo* pMissionInfo, SideID sideID, CivID civID)
    {
        ForEachSink( OnTeamCivChange(pMissionInfo, sideID, civID) )
    }

    void OnTeamNameChange(MissionInfo* pMissionInfo, SideID sideID)
    {
        ForEachSink( OnTeamNameChange(pMissionInfo, sideID) )
    }

	// #ALLY
	void OnTeamAlliancesChange(MissionInfo* pMissionInfo)
	{
		ForEachSink( OnTeamAlliancesChange(pMissionInfo) )
	}

    void OnPlayerStatusChange(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        ForEachSink( OnPlayerStatusChange(pMissionInfo, sideID, pPlayerInfo) )
    }

    void OnMoneyChange(PlayerInfo* pPlayerInfo)
    {
        ForEachSink( OnMoneyChange(pPlayerInfo) )
    }

    void OnBucketChange(BucketChange bc, IbucketIGC* b)
    {
        ForEachSink( OnBucketChange(bc, b) )
    }

    void OnTechTreeChanged(SideID sid)
    {
        ForEachSink( OnTechTreeChanged(sid) )
    }

    void OnStationCaptured(StationID stationID, SideID sideID)
    {
        ForEachSink( OnStationCaptured(stationID, sideID) )
    }

    void OnModelTerminated(ImodelIGC* pmodel)
    {
        ForEachSink( OnModelTerminated(pmodel) )
    }

    void OnLoadoutChanged(IpartIGC* ppart, LoadoutChange lc)
    {
        ForEachSink( OnLoadoutChanged(ppart, lc) )
    }
    
    void OnTurretStateChanging(bool bTurret)
    {
        ForEachSink( OnTurretStateChanging(bTurret) )
    }

    void OnPurchaseCompleted(bool bAllPartsBought)
    {
        ForEachSink( OnPurchaseCompleted(bAllPartsBought) )
    }

    void OnShipStatusChange(PlayerInfo* pplayer) 
    {
        ForEachSink( OnShipStatusChange(pplayer) )
    }

    void OnBoardShip(IshipIGC* pshipChild, IshipIGC* pshipParent)
    {
        ForEachSink( OnBoardShip(pshipChild, pshipParent) )
    }

    void OnBoardFailed(IshipIGC* pshipParent)
    {
        ForEachSink( OnBoardFailed(pshipParent) )
    }

    void OnDiscoveredStation(IstationIGC* pstation)
    {
        ForEachSink( OnDiscoveredStation(pstation) )
    }

    void OnDiscoveredAsteroid(IasteroidIGC* pasteroid)
    {
        ForEachSink( OnDiscoveredAsteroid(pasteroid) )
    }

    void OnClusterChanged(IclusterIGC* pcluster)
    {
        ForEachSink( OnClusterChanged(pcluster) )
    }

    void OnGameoverStats()
    {
        ForEachSink( OnGameoverStats() )
    }

    void OnGameoverPlayers()
    {
        ForEachSink( OnGameoverPlayers() )
    }

    void OnDeleteChatMessage(ChatInfo* pchatInfo)
    {
        ForEachSink( OnDeleteChatMessage(pchatInfo) )
    }

    void OnNewChatMessage()
    {
        ForEachSink( OnNewChatMessage() )
    }

    void OnClearChat()
    {
        ForEachSink( OnClearChat() )
    }

    void OnChatMessageChange()
    {
        ForEachSink( OnChatMessageChange() )
    }

    void OnLostConnection(const char * szReason)
    {
        ForEachSink( OnLostConnection(szReason) )
    }

    void OnEnterModalState()
    {
        ForEachSink( OnEnterModalState() )
    }

    void OnLeaveModalState()
    {
        ForEachSink( OnLeaveModalState() )
    }

    void OnLogonClub() 
    {
        ForEachSink( OnLogonClub() )
    }

    void OnLogonClubFailed(bool bRetry, const char * szReason) 
    {
        ForEachSink( OnLogonClubFailed(bRetry, szReason) )
    }

    void OnLogonLobby() 
    {
        ForEachSink( OnLogonLobby() )
    }

    void OnLogonLobbyFailed(bool bRetry, const char * szReason) 
    {
        ForEachSink( OnLogonLobbyFailed(bRetry, szReason) )
    }

    void OnLogonGameServer() 
    {
        ForEachSink( OnLogonGameServer() )
    }

    void OnLogonGameServerFailed(bool bRetry, const char * szReason) 
    {
        ForEachSink( OnLogonGameServerFailed(bRetry, szReason) )
    }
	// KGJV #114
	void OnServersList(int cCores, char *Cores, int cServers, char *Servers)
	{
		ForEachSink( OnServersList(cCores, Cores, cServers, Servers));
	}
};

/////////////////////////////////////////////////////////////////////////////
// IClientEventSink

class ClientEventSinkDelegate : public IClientEventSink {
private:
    IClientEventSink* m_psink;

public:
    ClientEventSinkDelegate(IClientEventSink* psink) :
        m_psink(psink)
    {
    }

    void OnAddMission(MissionInfo* pMissionInfo)
    {
        m_psink->OnAddMission(pMissionInfo);
    }

    void OnMissionCountdown(MissionInfo* pMissionInfo)
    {
        m_psink->OnMissionCountdown(pMissionInfo);
    }
    
    void OnMissionStarted(MissionInfo* pMissionInfo)
    {
        m_psink->OnMissionStarted(pMissionInfo);
    }
    
    void OnMissionEnded(MissionInfo* pMissionInfo)
    {
        m_psink->OnMissionEnded(pMissionInfo);
    }
    
    void OnLockLobby(bool bLock)
    {
        m_psink->OnLockLobby(bLock);
    }
    
    void OnLockSides(bool bLock)
    {
        m_psink->OnLockSides(bLock);
    }
    
    void OnFoundPlayer(MissionInfo* pMissionDef)
    {
        m_psink->OnFoundPlayer(pMissionDef);
    }

    void OnEnterMission()
    {
        m_psink->OnEnterMission();
    }

    void OnDelMission(MissionInfo* pMissionInfo)
    {
        m_psink->OnDelMission(pMissionInfo);
    }
    
    void OnAddPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        m_psink->OnAddPlayer(pMissionInfo, sideID, pPlayerInfo);
    }

    void OnDelPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
    {
        m_psink->OnDelPlayer(pMissionInfo, sideID, pPlayerInfo, reason, szMessageParam);
    }

    void OnAddRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        m_psink->OnAddRequest(pMissionInfo, sideID, pPlayerInfo);
    }

    void OnDelRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason)
    {
        m_psink->OnDelRequest(pMissionInfo, sideID, pPlayerInfo, reason);
    }

    void OnTeamInactive(MissionInfo* pMissionInfo, SideID sideID)
    {
        m_psink->OnTeamInactive(pMissionInfo, sideID);
    }

    void OnTeamReadyChange(MissionInfo* pMissionInfo, SideID sideID, bool fTeamReady)
    {
        m_psink->OnTeamReadyChange(pMissionInfo, sideID, fTeamReady);
    }

    void OnTeamForceReadyChange(MissionInfo* pMissionInfo, SideID sideID, bool fTeamForceReady)
    {
        m_psink->OnTeamForceReadyChange(pMissionInfo, sideID, fTeamForceReady);
    }

    void OnTeamAutoAcceptChange(MissionInfo* pMissionInfo, SideID sideID, bool fAutoAccept)
    {
        m_psink->OnTeamAutoAcceptChange(pMissionInfo, sideID, fAutoAccept);
    }

    void OnTeamCivChange(MissionInfo* pMissionInfo, SideID sideID, CivID civID)
    {
        m_psink->OnTeamCivChange(pMissionInfo, sideID, civID);
    }

    void OnTeamNameChange(MissionInfo* pMissionInfo, SideID sideID)
    {
        m_psink->OnTeamNameChange(pMissionInfo, sideID);
    }

	// #ALLY
	void OnTeamAlliancesChange(MissionInfo* pMissionInfo)
	{
		m_psink->OnTeamAlliancesChange(pMissionInfo);
	} 

    void OnPlayerStatusChange(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        m_psink->OnPlayerStatusChange(pMissionInfo, sideID, pPlayerInfo);
    }

    void OnMoneyChange(PlayerInfo* pPlayerInfo)
    {
        m_psink->OnMoneyChange(pPlayerInfo);
    }

    void OnBucketChange(BucketChange bc, IbucketIGC* b)
    {
        m_psink->OnBucketChange(bc, b);
    }

    void OnTechTreeChanged(SideID sid)
    {
        m_psink->OnTechTreeChanged(sid);
    }

    void OnStationCaptured(StationID stationID, SideID sideID)
    {
        m_psink->OnStationCaptured(stationID, sideID);
    }

    void OnModelTerminated(ImodelIGC* pmodel)
    {
        m_psink->OnModelTerminated(pmodel);
    }

    void OnLoadoutChanged(IpartIGC* ppart, LoadoutChange lc)
    {
        m_psink->OnLoadoutChanged(ppart, lc);
    }

    void OnTurretStateChanging(bool bTurret)
    {
        m_psink->OnTurretStateChanging(bTurret);
    }

    void OnPurchaseCompleted(bool bAllPartsBought)
    {
        m_psink->OnPurchaseCompleted(bAllPartsBought);
    }

    void OnShipStatusChange(PlayerInfo* pplayer) 
    {
        m_psink->OnShipStatusChange(pplayer);
    }

    void OnBoardShip(IshipIGC* pshipChild, IshipIGC* pshipParent)
    {
        m_psink->OnBoardShip(pshipChild, pshipParent);
    }

    void OnBoardFailed(IshipIGC* pshipParent)
    {
        m_psink->OnBoardFailed(pshipParent);
    }

    void OnDiscoveredStation(IstationIGC* pstation)
    {
        m_psink->OnDiscoveredStation(pstation);
    }

    void OnDiscoveredAsteroid(IasteroidIGC* pasteroid)
    {
        m_psink->OnDiscoveredAsteroid(pasteroid);
    }

    void OnClusterChanged(IclusterIGC* pcluster)
    {
        m_psink->OnClusterChanged(pcluster);
    }

    void OnGameoverStats()
    {
        m_psink->OnGameoverStats();
    }

    void OnGameoverPlayers()
    {
        m_psink->OnGameoverPlayers();
    }

    void OnDeleteChatMessage(ChatInfo* pchatInfo)
    {
        m_psink->OnDeleteChatMessage(pchatInfo);
    }

    void OnNewChatMessage()
    {
        m_psink->OnNewChatMessage();
    }

    void OnClearChat()
    {
        m_psink->OnClearChat();
    }

    void OnChatMessageChange()
    {
        m_psink->OnChatMessageChange();
    }

    void OnLostConnection(const char* szReason)
    {
        m_psink->OnLostConnection(szReason);
    }

    void OnEnterModalState()
    {
        m_psink->OnEnterModalState();
    }

    void OnLeaveModalState()
    {
        m_psink->OnLeaveModalState();
    }

    void OnLogonClub() 
    {
        m_psink->OnLogonClub();
    }

    void OnLogonClubFailed(bool bRetry, const char * szReason) 
    {
        m_psink->OnLogonClubFailed(bRetry, szReason);
    }

    void OnLogonLobby() 
    {
        m_psink->OnLogonLobby();
    }

    void OnLogonLobbyFailed(bool bRetry, const char * szReason) 
    {
        m_psink->OnLogonLobbyFailed(bRetry, szReason);
    }

    void OnLogonGameServer() 
    {
        m_psink->OnLogonGameServer();
    }

    void OnLogonGameServerFailed(bool bRetry, const char * szReason) 
    {
        m_psink->OnLogonGameServerFailed(bRetry, szReason);
    }
	// KGJV #114
	void OnServersList(int cCores, char *Cores, int cServers, char *Servers)
	{
		m_psink->OnServersList(cCores, Cores, cServers, Servers);
	}
};

TRef<IClientEventSink> IClientEventSink::CreateDelegate(IClientEventSink* psink)
{
    return new ClientEventSinkDelegate(psink);
}

/////////////////////////////////////////////////////////////////////////////
// ChatInfo
void ChatInfo::SetChat(ChatTarget       ctRecipient,
                       const ZString&   strText, 
                       CommandID        cid,
                       ImodelIGC*       pmodelTarget,
                       const Color&     color,
                       bool             bFromPlayer,
                       bool             bFromObjectModel,
                       bool             bFromLeader)
{
    m_ctRecipient = ctRecipient;

    m_cidCommand = cid;
    m_pmodelTarget = pmodelTarget;

    m_strMessage = strText;
    m_colorMessage = color;

    m_bFromPlayer = bFromPlayer;
    m_bFromObjectModel = bFromObjectModel;
    m_bFromLeader = bFromLeader;
}

void    ChatInfo::ClearTarget(void)
{
    m_cidCommand = c_cidNone;
    if (m_pmodelTarget->GetObjectType() != OT_buoy)
    {
        m_strMessage += " <dead>";
    }
    m_pmodelTarget = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// MissionInfo


MissionInfo::MissionInfo(DWORD dwCookie) :
    m_sideLobby(SIDE_TEAMLOBBY)
{
    m_pfmMissionDef = new FMD_S_MISSIONDEF;
    memset(m_pfmMissionDef, 0, sizeof(FMD_S_MISSIONDEF));
    m_pfmMissionDef->dwCookie = dwCookie;
    m_fGuaranteedSlotsAvailable = false;
    m_fAnySlotsAvailable = false;
    m_fCountdownStarted = false;
    m_nNumPlayers = 0;
	m_nNumNoatPlayers = 0; //Imago #169
}

MissionInfo::~MissionInfo()
{
    if (m_pfmMissionDef)
        delete m_pfmMissionDef;

    TMapListWrapper<SideID, SideInfo*>::Iterator iterSideInfo(m_mapSideInfo);

    while (!iterSideInfo.End())
        {
        delete iterSideInfo.Value();
        iterSideInfo.Next();
        }
}

void MissionInfo::Update(FMD_S_MISSIONDEF* pfmMissionDef)
{
    int numSidesOld = NumSides();
    memcpy(m_pfmMissionDef, pfmMissionDef, sizeof(FMD_S_MISSIONDEF));

    // create or destroy sides, as needed.
    if (NumSides() > numSidesOld)
    {
        // create the new sides the easy way
        for(SideID sideID = numSidesOld; sideID < NumSides(); sideID++)
        {
            GetSideInfo(sideID);
        }
    }
    else
    {
        // destroy the old sides
        for (SideID sideID = numSidesOld - 1; sideID >= NumSides(); sideID--)
        {
            m_mapSideInfo.Remove(sideID);
        }
    }

    // signal the sides list in case something has changed.
    m_mapSideInfo.GetSink()();
}

void MissionInfo::Update(FMD_LS_LOBBYMISSIONINFO* pfmLobbyMissionInfo)
{
    int numSidesOld = NumSides();
    Strncpy(m_pfmMissionDef->misparms.strGameName, FM_VAR_REF(pfmLobbyMissionInfo, szGameName), c_cbGameName);
    m_pfmMissionDef->misparms.strGameName[c_cbGameName - 1] = '\0';
    m_strGameDetailsFiles = (FM_VAR_REF(pfmLobbyMissionInfo, szGameDetailsFiles) != NULL)
      ? FM_VAR_REF(pfmLobbyMissionInfo, szGameDetailsFiles) : "";
    m_pfmMissionDef->misparms.nTeams = pfmLobbyMissionInfo->nTeams;
    m_pfmMissionDef->misparms.nMinPlayersPerTeam = pfmLobbyMissionInfo->nMinPlayersPerTeam;
    m_pfmMissionDef->misparms.nMaxPlayersPerTeam = pfmLobbyMissionInfo->nMaxPlayersPerTeam;
    m_pfmMissionDef->misparms.iMinRank = pfmLobbyMissionInfo->nMinRank;
    m_pfmMissionDef->misparms.iMaxRank = pfmLobbyMissionInfo->nMaxRank;
    m_pfmMissionDef->misparms.timeStart = Time(pfmLobbyMissionInfo->dwStartTime);
    m_fGuaranteedSlotsAvailable = pfmLobbyMissionInfo->fGuaranteedSlotsAvailable;
    m_fAnySlotsAvailable = pfmLobbyMissionInfo->fAnySlotsAvailable;
    m_nNumPlayers = pfmLobbyMissionInfo->nNumPlayers;
	m_nNumNoatPlayers = pfmLobbyMissionInfo->nNumNoatPlayers; //Imago #169
    SetInProgress(pfmLobbyMissionInfo->fInProgress);
    SetCountdownStarted(pfmLobbyMissionInfo->fCountdownStarted);
    m_pfmMissionDef->misparms.bScoresCount = pfmLobbyMissionInfo->fScoresCount;
    m_pfmMissionDef->misparms.bAllowDevelopments = pfmLobbyMissionInfo->fAllowDevelopments;
    m_pfmMissionDef->misparms.bInvulnerableStations = pfmLobbyMissionInfo->fInvulnerableStations;
    m_pfmMissionDef->misparms.bObjectModelCreated = pfmLobbyMissionInfo->fMSArena;
	// KGJV - receive game core file
	Strncpy(m_pfmMissionDef->misparms.szIGCStaticFile, FM_VAR_REF(pfmLobbyMissionInfo, szIGCStaticFile), c_cbFileName);
	// KGJV #114 - fill in server name & ip
	Strncpy(m_pfmMissionDef->szServerName, FM_VAR_REF(pfmLobbyMissionInfo,szServerName), c_cbName);
	Strncpy(m_pfmMissionDef->szServerAddr, FM_VAR_REF(pfmLobbyMissionInfo,szServerAddr), 16);
	UTL::SetPrivilegedUsers( ((FM_VAR_REF(pfmLobbyMissionInfo, szPrivilegedUsers) != NULL) ?  FM_VAR_REF(pfmLobbyMissionInfo, szPrivilegedUsers) : ""),m_pfmMissionDef->dwCookie); //Imago 6/10 #2
	UTL::SetServerVersion(FM_VAR_REF(pfmLobbyMissionInfo, szServerVersion),m_pfmMissionDef->dwCookie); //Imago 7/10 #62
    m_pfmMissionDef->misparms.nTotalMaxPlayersPerGame = pfmLobbyMissionInfo->nMaxPlayersPerGame;
    m_pfmMissionDef->misparms.bSquadGame = pfmLobbyMissionInfo->fSquadGame;
    m_pfmMissionDef->misparms.bEjectPods = pfmLobbyMissionInfo->fEjectPods;

    if (pfmLobbyMissionInfo->fLimitedLives && m_pfmMissionDef->misparms.iLives == 0x7fff)
        m_pfmMissionDef->misparms.iLives = 1;
    else if (!pfmLobbyMissionInfo->fLimitedLives && m_pfmMissionDef->misparms.iLives != 0x7fff)
        m_pfmMissionDef->misparms.iLives = 0x7fff;

    if (pfmLobbyMissionInfo->fConquest && !m_pfmMissionDef->misparms.IsConquestGame())
        m_pfmMissionDef->misparms.iGoalConquestPercentage = 100;
    else if (!pfmLobbyMissionInfo->fConquest && m_pfmMissionDef->misparms.IsConquestGame())
        m_pfmMissionDef->misparms.iGoalConquestPercentage = 0;

    if (pfmLobbyMissionInfo->fDeathMatch && !m_pfmMissionDef->misparms.IsDeathMatchGame())
        m_pfmMissionDef->misparms.nGoalTeamKills = 1;
    else if (!pfmLobbyMissionInfo->fDeathMatch && m_pfmMissionDef->misparms.IsDeathMatchGame())
        m_pfmMissionDef->misparms.nGoalTeamKills = 0;

    if (pfmLobbyMissionInfo->fCountdown && !m_pfmMissionDef->misparms.IsCountdownGame())
        m_pfmMissionDef->misparms.dtGameLength = 600;
    else if (!pfmLobbyMissionInfo->fCountdown && m_pfmMissionDef->misparms.IsCountdownGame())
        m_pfmMissionDef->misparms.dtGameLength = 0;

    if (pfmLobbyMissionInfo->fProsperity && !m_pfmMissionDef->misparms.IsProsperityGame())
        m_pfmMissionDef->misparms.fGoalTeamMoney = 100;
    else if (!pfmLobbyMissionInfo->fProsperity && m_pfmMissionDef->misparms.IsProsperityGame())
        m_pfmMissionDef->misparms.fGoalTeamMoney = 0;

    if (pfmLobbyMissionInfo->fArtifacts && !m_pfmMissionDef->misparms.IsArtifactsGame())
        m_pfmMissionDef->misparms.nGoalArtifactsCount = 10;
    else if (!pfmLobbyMissionInfo->fArtifacts && m_pfmMissionDef->misparms.IsArtifactsGame())
        m_pfmMissionDef->misparms.nGoalArtifactsCount = 0;

    if (pfmLobbyMissionInfo->fFlags && !m_pfmMissionDef->misparms.IsFlagsGame())
        m_pfmMissionDef->misparms.nGoalFlagsCount = 10;
    else if (!pfmLobbyMissionInfo->fFlags && m_pfmMissionDef->misparms.IsFlagsGame())
        m_pfmMissionDef->misparms.nGoalFlagsCount = 0;

    if (pfmLobbyMissionInfo->fTerritorial && !m_pfmMissionDef->misparms.IsTerritoryGame())
        m_pfmMissionDef->misparms.iGoalTerritoryPercentage = 70;
    else if (!pfmLobbyMissionInfo->fTerritorial && m_pfmMissionDef->misparms.IsTerritoryGame())
        m_pfmMissionDef->misparms.iGoalTerritoryPercentage = 100;

    // create or destroy sides, as needed.
    if (NumSides() > numSidesOld)
    {
        // create the new sides the easy way
        for(SideID sideID = numSidesOld; sideID < NumSides(); sideID++)
        {
            GetSideInfo(sideID);
        }
    }
    else
    {
        // destroy the old sides
        for (SideID sideID = numSidesOld - 1; sideID >= NumSides(); sideID--)
        {
            m_mapSideInfo.Remove(sideID);
        }
    }

    // copy the squad info
    {        
        SideID sideID;
        SquadID* vSquadIDs = (SquadID*)FM_VAR_REF(pfmLobbyMissionInfo, rgSquadIDs);
        int numSquads = pfmLobbyMissionInfo->cbrgSquadIDs / sizeof(SquadID);

        assert(numSquads < c_cSidesMax);
        for (sideID = 0; sideID < numSquads; sideID++)
            squadIDs[sideID] = vSquadIDs[sideID];
        for (; sideID < c_cSidesMax; sideID++)
            squadIDs[sideID] = NA;
    }

    // signal the sides list in case something has changed.
    m_mapSideInfo.GetSink()();
}

void MissionInfo::UpdateStartTime(Time timeStart)
{
    m_pfmMissionDef->misparms.timeStart = timeStart;
}

int MissionInfo::GuaranteedPositions()
{
    SideID sideID;
    int nTotal = 0;

    for(sideID = 0; sideID < NumSides(); sideID++)
    {
        if (SideActive(sideID) && (SideAutoAccept(sideID) 
            || (SideNumPlayers(sideID) == 0 && AutoAcceptLeaders())))
        {
            nTotal += SideMaxPlayers(sideID) - SideNumPlayers(sideID);
        }
    }
    return nTotal;
}

void MissionInfo::PurgePlayers()
{
    TMapListWrapper<SideID, SideInfo*>::Iterator iterSideInfo(m_mapSideInfo);

    while (!iterSideInfo.End())
        {
        delete iterSideInfo.Value();
        iterSideInfo.Next();
        }

    m_mapSideInfo.SetEmpty();

    for (SideID id = 0; id < NumSides(); id++)
    {
        m_pfmMissionDef->rgcPlayers[id] = 0;
    }
}

bool MissionInfo::HasSquad(SquadID squadID)
{
    for (SideID id = 0; id < NumSides(); id++)
    {
        if (squadIDs[id] == squadID)
            return true;
    }

    return false;
}

void MissionInfo::SetSideLeader(PlayerInfo* pPlayerInfo)
{ 
    m_pfmMissionDef->rgShipIDLeaders[pPlayerInfo->SideID()] = pPlayerInfo->ShipID();
    if (pPlayerInfo->IsMissionOwner()) 
        m_pfmMissionDef->iSideMissionOwner = pPlayerInfo->SideID();
    GetSideInfo(pPlayerInfo->SideID())->GetMembers().GetSink()();
}

bool MissionInfo::FindPlayer(SideID sideID, ShipID shipID)
{
    ZAssert(sideID != NA);
    ZAssert(shipID >= 0);
    return GetSideInfo(sideID)->FindPlayer(shipID);
}

void MissionInfo::AddPlayer(PlayerInfo* pPlayerInfo)
{
    SideID sideID = pPlayerInfo->SideID();
    ShipID shipID = pPlayerInfo->ShipID();
    ZAssert(sideID != NA);
    ZAssert(shipID >= 0);
    
    SideInfo* psideInfo = GetSideInfo(sideID);
    if (!psideInfo->FindPlayer(shipID))
    {
        if ((sideID >= 0) && pPlayerInfo->IsHuman())
            m_pfmMissionDef->rgcPlayers[sideID]++;
        psideInfo->RemoveRequest(shipID);
        psideInfo->AddPlayer(shipID);
        m_mapSideInfo.GetSink()();
    }
}

void MissionInfo::RemovePlayer(PlayerInfo* pPlayerInfo)
{
    SideID sideID = pPlayerInfo->SideID();
    ShipID shipID = pPlayerInfo->ShipID();
    ZAssert(sideID != NA);
    ZAssert(shipID >= 0);

    if (sideID != SIDE_TEAMLOBBY)
    {
        // update the team owner info if the team owner is quiting
        if (m_pfmMissionDef->rgShipIDLeaders[sideID] == pPlayerInfo->ShipID())
        {
            pPlayerInfo->SetTeamLeader(false);
            m_pfmMissionDef->rgShipIDLeaders[sideID] = NA;

            if (m_pfmMissionDef->iSideMissionOwner == sideID)
            {
                m_pfmMissionDef->iSideMissionOwner = NA;
                pPlayerInfo->SetMissionOwner(false);
            }
        }

    }

    SideInfo* psideInfo = GetSideInfo(sideID);
    if (psideInfo->FindPlayer(shipID))
    {
        if ((sideID >= 0) && pPlayerInfo->IsHuman())
            m_pfmMissionDef->rgcPlayers[sideID]--;
        psideInfo->RemovePlayer(shipID);
        m_mapSideInfo.GetSink()();
    }
}

bool MissionInfo::FindRequest(SideID sideID, ShipID shipID)
{
    ZAssert(sideID != NA);
    ZAssert(shipID >= 0);
    if (sideID == SIDE_TEAMLOBBY)
        return false;
    return GetSideInfo(sideID)->FindRequest(shipID);
}

void MissionInfo::AddRequest(SideID sideID, ShipID shipID)
{
    ZAssert(sideID != NA);
    ZAssert(shipID >= 0);
    if (sideID == SIDE_TEAMLOBBY)
        return;
    SideInfo* psideInfo = GetSideInfo(sideID);
    ZAssert(psideInfo);
    if (!psideInfo->FindPlayer(shipID) && !psideInfo->FindRequest(shipID))
        psideInfo->AddRequest(shipID);
    m_mapSideInfo.GetSink()();
}

void MissionInfo::RemoveRequest(SideID sideID, ShipID shipID)
{
    ZAssert(sideID != NA);
    ZAssert(shipID >= 0);
    if (sideID == SIDE_TEAMLOBBY)
        return;
    GetSideInfo(sideID)->RemoveRequest(shipID);
    m_mapSideInfo.GetSink()();
}


SideInfo* MissionInfo::GetSideInfo(SideID sideID)
{
    ZAssert(sideID != NA);
    SideInfo* pSideInfo = NULL;
    
    if (sideID >= m_pfmMissionDef->misparms.nTeams)
    {
        assert(false);
        return NULL;
    }
    else if (!m_mapSideInfo.Find(sideID, pSideInfo) && sideID != SIDE_TEAMLOBBY)
    {
        pSideInfo = new SideInfo(sideID);
        m_mapSideInfo.Set(sideID, pSideInfo);
    }
    else if (sideID == SIDE_TEAMLOBBY)
    {
        return &m_sideLobby;
    }
    return pSideInfo;
}

List* MissionInfo::GetSideList()
{
    // make sure we have a complete sides list
    for (SideID id = 0; id < NumSides(); id++)
        GetSideInfo(id);
    GetSideInfo(SIDE_TEAMLOBBY);

    return new ListDelegate(&m_mapSideInfo); //Fix memory leak -Imago 8/2/09
}

/////////////////////////////////////////////////////////////////////////////
// PlayerInfo

PlayerInfo::PlayerInfo(void) :
    m_pship(NULL),
    m_vPersistPlayerScores(NULL),
    m_cPersistPlayerScores(0),
    m_bMute(false)
{
}

PlayerInfo::~PlayerInfo()
{
    if (m_vPersistPlayerScores)
        delete m_vPersistPlayerScores;

    ResetShipStatus();
}

void PlayerInfo::Set(FMD_S_PLAYERINFO* pfmPlayerInfo)
{
    if (m_vPersistPlayerScores)
        delete m_vPersistPlayerScores;

    memcpy(&m_fmPlayerInfo, pfmPlayerInfo, sizeof(m_fmPlayerInfo));

    m_cPersistPlayerScores = pfmPlayerInfo->cbrgPersistPlayerScores / sizeof(PersistPlayerScoreObject);

    if (m_cPersistPlayerScores > 0)
    {
        PersistPlayerScoreObject* vSrcScores 
            = (PersistPlayerScoreObject*)FM_VAR_REF(pfmPlayerInfo, rgPersistPlayerScores);
        m_vPersistPlayerScores = new PersistPlayerScoreObject[m_cPersistPlayerScores];

        for (int i = 0; i < m_cPersistPlayerScores; i++)
            m_vPersistPlayerScores[i] = vSrcScores[i];
    }
    else
    {
        m_vPersistPlayerScores = NULL;
    }
}

const PersistPlayerScoreObject& PlayerInfo::GetPersistScore(CivID civId) const
{
    for (int i = 0; i < m_cPersistPlayerScores; i++)
    {
        if (m_vPersistPlayerScores[i].GetCivID() == civId)
            return m_vPersistPlayerScores[i];
    }

    return m_persist;
}

void        PlayerInfo::UpdateScore(PersistPlayerScoreObject& ppso)
{
    CivID civId = ppso.GetCivID();

    if (civId != NA)
    {
        int iCivScore;

        // look for the ppso for this civ
        for (iCivScore = 0; iCivScore < m_cPersistPlayerScores; ++iCivScore)
        {
            if (m_vPersistPlayerScores[iCivScore].GetCivID() == civId)
                break;
        }

        // if the given civ was not found, create a new entry for it.
        if (iCivScore >= m_cPersistPlayerScores) 
        {
            PersistPlayerScoreObject* vppsoNew 
                = new PersistPlayerScoreObject[m_cPersistPlayerScores + 1];
            
            for (int i = 0; i < m_cPersistPlayerScores; ++i)
                vppsoNew[i] = m_vPersistPlayerScores[i];

            if (m_vPersistPlayerScores)
                delete m_vPersistPlayerScores;

            m_vPersistPlayerScores = vppsoNew;

            ++m_cPersistPlayerScores;
        }

        // copy the stats
        m_vPersistPlayerScores[iCivScore] = ppso;
    }
}

CivID PlayerInfo::GetCivID() const
{
    if (SideID() < 0)
        return NA;
    else
        return GetShip()->GetSide()->GetCivilization()->GetObjectID();
}

/////////////////////////////////////////////////////////////////////////////
// BaseClient

BaseClient::BaseClient(void)
:
    m_pCoreIGC(CreateMission()),
    m_lastSend(Time::Now()),
    m_lastLagCheck(Time::Now()),
    m_serverOffset(0),
    m_serverOffsetValidF(false),
    m_selectedWeapon(0),
    m_messageType(c_mtNone),
    m_cookie(NA),
    m_plinkSelectedChat(NULL),
    m_pPlayerInfo(NULL),
    m_pMissionInfo(NULL),
    m_fLoggedOn(false),
    m_fLoggedOnToLobby(false),
    m_fLoggedOnToClub(false),
    m_fm(this),
    m_fmLobby(this),
    m_fmClub(this),
    m_ship(NULL),
    m_pshipLastSender(NULL),
    m_moneyLastRequest(0),
    m_sync(1.0f),
    m_viewCluster(NULL),
    m_cUnansweredPings(0),
    m_pAutoDownload(NULL),
    m_fZoneClub(false),
    m_fIsLobbied(false),
    m_dwCookieToJoin(NA),    
    m_bInGame(false),
    m_bWaitingForGameRestart(false),
    m_cRankInfo(0),
    m_vRankInfo(NULL),
    m_cStaticMapInfo(0),
    m_vStaticMapInfo(NULL),
    m_lockdownCriteria(0),
    m_lobbyServerOffset(0),
    m_plistFindServerResults(NULL),
    m_strCDKey(""),
	// KGJV pigs - extra default init
	m_sidBoardAfterDisembark(NA),
	m_sidTeleportAfterDisembark(NA)
{
    CoInitialize(NULL);
    m_timeLastPing = m_lastSend;
    m_plistMissions = new ListDelegate(&m_mapMissions);
    m_pClientEventSource = new ClientEventSource();
    m_szCharName[0] = '\0';
    m_szClubCharName[0] = '\0';
    m_szLobbyCharName[0] = '\0';
    m_szIGCStaticFile[0] = '\0';
    m_pMissionInfo = NULL;

    m_customLoadouts = new CachedLoadoutList[MAX_CUSTOM_LOADOUTS]; //AaronMoore 1/10
}

BaseClient::~BaseClient(void)
{
#ifdef USEAUTH
    FreeZoneAuthClient();
#endif
    CoUninitialize();
    TMapListWrapper<DWORD, MissionInfo*>::Iterator iterMissions(m_mapMissions);

    while (!iterMissions.End())
    {
        delete iterMissions.Value();
        iterMissions.Next();
    }

    if (m_pMissionInfo)
        delete m_pMissionInfo;

    if (m_ci.pZoneTicket)
        HeapFree(GetProcessHeap(), 0, m_ci.pZoneTicket);


    m_fm.Shutdown();

    if (m_pAutoDownload)
        delete m_pAutoDownload;

    if (m_vRankInfo)
        delete [] m_vRankInfo;
    m_vRankInfo = NULL;

    if (m_vStaticMapInfo)
        delete [] m_vStaticMapInfo;
    m_vStaticMapInfo = NULL;
	
	//AaronMoore 1/10
    if (m_customLoadouts)
        delete [] m_customLoadouts;
    m_customLoadouts = NULL;
}

void    BaseClient::Initialize(Time timeNow)
{
    assert (m_pCoreIGC);
    
    //Now that we have a mission, create a (no-hull) ship for the player
    CreateDummyShip();

    m_pCoreIGC->Initialize(timeNow, this);
}

void    BaseClient::Reinitialize(Time timeNow)
{
    BaseClient::Terminate();

    // dont change m_pClientEventSource... too many things in WinTrek are holding on to it
    // and don't get reinitialized

    ZAssert(m_pCoreIGC == NULL);
    m_pCoreIGC = CreateMission();
    m_lastSend = timeNow;
    m_lastLagCheck = timeNow;
    m_timeLastPing = timeNow;
    m_serverOffset = 0;
    m_serverOffsetValidF = false;
    m_selectedWeapon = 0;
    m_messageType = c_mtNone;
    m_cookie = NA;
    m_plinkSelectedChat = NULL;
    m_pPlayerInfo = NULL;
    if (m_pMissionInfo)
        delete m_pMissionInfo;
    m_pMissionInfo = NULL;
    m_fLoggedOn = false;

    m_pchaffLastCreated = NULL;

    m_pshipLastSender = NULL;
    m_moneyLastRequest = 0;

    m_mapBucketStatusArray.SetEmpty();
    m_listPlayers.purge(true);
    m_chatList.purge(true);

    Initialize(timeNow);
    
}

void    BaseClient::Terminate(void)
{
    assert (m_pCoreIGC);
    Disconnect();

    FlushGameState();
}

void BaseClient::FlushGameState()
{
    m_pchaffLastCreated = NULL;

    if (m_ship)
        m_ship->AddRef();

    //Offline, we need to explicitly terminate all of the ships
	if(m_pCoreIGC != nullptr) // BT - 10/17 - Added crash guards
    {
        const ShipListIGC*  ships = m_pCoreIGC->GetShips();
        ShipLinkIGC*  l;
        while ((l = ships->first()) != NULL)
        {
            l->data()->Terminate();
        }
    }

    m_listPlayers.purge();

	if (m_pCoreIGC != nullptr) // BT - 10/17 - Added crash guards
		m_pCoreIGC->Terminate();

    // Destroy the dummy ship
    if (m_ship)
    {
        DestroyDummyShip();
        m_ship->Release();
        m_ship = NULL;
    }

	if (m_pCoreIGC != nullptr) // BT - 10/17 - Added crash guards
	{
		delete m_pCoreIGC;
		m_pCoreIGC = NULL;
	}

    m_bInGame = false;
}

HRESULT BaseClient::ConnectToServer(ConnectInfo & ci, DWORD dwCookie, Time now, const char* szPassword, bool bStandalonePrivate)
{ 
    // connect to a particular *game* on this server
    HRESULT hr = S_OK;
    
    // review:  in the event of retry logon after logon failure 
    // we need to treat this as a reconnect and dump first connection...
    // we should see about reusing the connection

    assert(IFF(ci.cbZoneTicket > 0, ci.pZoneTicket));

    Reinitialize(now);
    
    assert (!m_fm.IsConnected() && !m_fLoggedOn);

    // this does everything up to, and including, creating a new player
    if (ci.guidSession != GUID_NULL)
    {
        assert(ci.strServer.IsEmpty());
        hr = m_fm.JoinSessionInstance(ci.guidSession, ci.szName);
    }
    else
	{
		if (bStandalonePrivate) {
			hr = m_fm.JoinSession(FEDSRV_STANDALONE_PRIVATE_GUID, ci.strServer, ci.szName);		  
		}
		else {
            hr = m_fm.JoinSession(FEDSRV_GUID, ci.strServer, ci.szName, ci.dwPort);
        }
    }

    // TODO: Remove this when we are ready to enforce CD Keys
    if (m_strCDKey.IsEmpty())
        m_strCDKey = ZString(ci.szName).ToUpper();

	char szCdKey[2064];
	strcpy(szCdKey, (PCC)m_strCDKey);

	// BT - STEAM
	char szDrmHash[256];

    if (m_fm.IsConnected())
    {
        ZSucceeded(hr);
        // Let's formally announce ourselves to the server
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmLogon, C, LOGONREQ)
            FM_VAR_PARM(ci.szName, CB_ZTS)
            FM_VAR_PARM(ci.pZoneTicket, ci.cbZoneTicket)
            FM_VAR_PARM(szCdKey, CB_ZTS) // BT - 9/11/2010 - Sending the token to the server so that the server will also enforce authentication. 
            FM_VAR_PARM(szPassword, CB_ZTS)
			FM_VAR_PARM(szDrmHash, CB_ZTS) // BT - STEAM
        END_PFM_CREATE
        pfmLogon->fedsrvVer = MSGVER;
        pfmLogon->dwCookie = dwCookie;
        //pfmLogon->zgs = m_fm.GetEncryptedZoneTicket();
        pfmLogon->time = Time::Now ();

		// BT - STEAM
		UpdateServerLoginRequestWithSteamAuthTokenInformation(pfmLogon);

        debugf("Logging on to game server \"%s\"...\n",
          ci.strServer.IsEmpty() ? "" : (LPCSTR)ci.strServer);
        SendMessages();
    }
    retailf("$$MSRGuard:Set:UserName=%s\n", ci.szName);
    m_cUnansweredPings = 0;
    m_serverOffsetValidF = false;
    return hr;
}


HRESULT BaseClient::ConnectToLobby(ConnectInfo * pci) // pci is NULL if relogging in
{
    HRESULT hr = S_OK;

    debugf("ConnectToLobby: Start");
    
    // review:  in the event of retry logon after logon failure 
    // we need to treat this as a reconnect and dump first connection...
    // we should see about reusing the connection

    if (pci)
    {
        if (m_ci.pZoneTicket)
          HeapFree(GetProcessHeap(), 0, m_ci.pZoneTicket);

        m_ci = *pci;
    }

    // but we always use config-specified lobby server. Is this too restrictive to derived clients?
    m_ci.strServer = GetIsZoneClub() ? GetCfgInfo().strClubLobby : GetCfgInfo().strPublicLobby;
    debugf("ConnectToLobby: Server address is " + m_ci.strServer);

    assert(IFF(m_ci.cbZoneTicket > 0, m_ci.pZoneTicket));
    
    // TODO: Remove this when we are ready to enforce CD Keys
    if (m_strCDKey.IsEmpty())
        m_strCDKey = ZString(m_ci.szName).ToUpper();

    debugf("ConnectToLobby: CDKey " + m_strCDKey);

    if (m_fmLobby.IsConnected()) {
        debugf("ConnectToLobby: Already connected");
        return S_OK;
    }

    assert(m_fLoggedOnToLobby == false);

    debugf("ConnectToLobby: Clearing list of already loaded missions");
    TMapListWrapper<DWORD, MissionInfo*>::Iterator iterMissions(m_mapMissions);
    while (!iterMissions.End())
    {
        delete iterMissions.Value();
        iterMissions.Next();
    }
    m_mapMissions.SetEmpty();

	hr = m_fmLobby.JoinSession(GetIsZoneClub() ? FEDLOBBYCLIENTS_GUID : FEDFREELOBBYCLIENTS_GUID, m_ci.strServer, m_ci.szName, GetCfgInfo().dwLobbyPort);
    assert(IFF(m_fmLobby.IsConnected(), SUCCEEDED(hr)));
    if (m_fmLobby.IsConnected())
    {
        debugf("ConnectToLobby: Session started with the lobby");

        DWORD dwTime = Time::Now().clock();
        int crcFileList = (g_bCheckFiles ? 0 : FileCRC("FileList.txt", NULL));
        char * szEncryptionKey = (char *)_alloca(strlen(CL_LOGON_KEY) + 30);
        wsprintf(szEncryptionKey, CL_LOGON_KEY, dwTime, m_ci.szName);

        // Let's formally announce ourselves to the server
        BEGIN_PFM_CREATE(m_fmLobby, pfmLogon, C, LOGON_LOBBY)
            FM_VAR_PARM(m_ci.pZoneTicket, m_ci.cbZoneTicket)
            FM_VAR_PARM(PCC(m_strCDKey), CB_ZTS)                   // Wlp 2006 - This is the ASGS Ticket now
        END_PFM_CREATE
        pfmLogon->verLobby = LOBBYVER;
        pfmLogon->crcFileList = crcFileList;
        pfmLogon->dwTime = dwTime;
        lstrcpy(pfmLogon->szName, m_ci.szName);

		// BT - STEAM
		UpdateLobbyLoginRequestWithSteamAuthTokenInformation(pfmLogon);

        // do art update--see ConnectToServer
        debugf("Logging on to lobby \"%s\"...\n",
          m_ci.strServer.IsEmpty() ? "" : (LPCSTR)m_ci.strServer);
        lstrcpy(m_szLobbyCharName, m_ci.szName);
        SendLobbyMessages();
    }
    retailf("$$MSRGuard:Set:UserName=%s\n", m_szLobbyCharName);
    m_cUnansweredPings = 0;
    m_serverOffsetValidF = false;

    if (SUCCEEDED(hr)) {
        debugf("ConnectToLobby: Completed succesfully");
    }
    else {
        debugf("ConnectToLobby: Completed with error");
    }

    return hr;
}

// BT - STEAM
void BaseClient::UpdateLobbyLoginRequestWithSteamAuthTokenInformation(FMD_C_LOGON_LOBBY *pfmLogon)
{
	if (SteamUser() != nullptr)
	{
		CancelSteamAuthSessionToLobby();

		m_hAuthTicketLobby = SteamUser()->GetAuthSessionTicket(pfmLogon->steamAuthTicket, sizeof(pfmLogon->steamAuthTicket), &pfmLogon->steamAuthTicketLength);

		pfmLogon->steamID = SteamUser()->GetSteamID().ConvertToUint64();
	}
}

// BT - STEAM
void BaseClient::UpdateServerLoginRequestWithSteamAuthTokenInformation(FMD_C_LOGONREQ *pfmLogon)
{
	if (SteamUser() != nullptr)
	{
		CancelSteamAuthSessionToGameServer();

		m_hAuthTicketServer = SteamUser()->GetAuthSessionTicket(pfmLogon->steamAuthTicket, sizeof(pfmLogon->steamAuthTicket), &pfmLogon->steamAuthTicketLength);

		pfmLogon->steamID = SteamUser()->GetSteamID().ConvertToUint64();

		int authTicketCRC = MemoryCRC(pfmLogon->steamAuthTicket, pfmLogon->steamAuthTicketLength);

		DrmChecker drmChecker;
		drmChecker.GetDrmWrapChecksum(authTicketCRC, pfmLogon->drmHash, sizeof(pfmLogon->drmHash)); // Note, this is always empty, unless it's an official retail steam build!


	}
}

// BT - STEAM
void BaseClient::CancelSteamAuthSessionToGameServer()
{
	if (m_hAuthTicketServer != 0)
	{
		SteamUser()->CancelAuthTicket(m_hAuthTicketServer);
		m_hAuthTicketServer = 0;
	}
}

// BT - STEAM
void BaseClient::CancelSteamAuthSessionToLobby()
{
	if (m_hAuthTicketLobby != 0)
	{
		SteamUser()->CancelAuthTicket(m_hAuthTicketLobby);
		m_hAuthTicketLobby = 0;
	}
}

HRESULT BaseClient::ConnectToClub(ConnectInfo * pci) // pci is NULL if relogging in
{
    HRESULT hr = S_OK;
    
    if (m_fmClub.IsConnected())
        return S_OK;

    // review:  in the event of retry logon after logon failure 
    // we need to treat this as a reconnect and dump first connection...
    // we should see about reusing the connection

    if (pci)
    {
        if (m_ci.pZoneTicket)
          HeapFree(GetProcessHeap(), 0, m_ci.pZoneTicket);

        m_ci = *pci;
    }
    assert(IFF(m_ci.cbZoneTicket > 0, m_ci.pZoneTicket));
    
    if (m_fmClub.IsConnected())
        return S_OK;

    assert(m_fLoggedOnToClub == false);
    
    hr = m_fmClub.JoinSession(FEDCLUB_GUID, m_ci.strServer, m_ci.szName);
    assert(IFF(m_fmClub.IsConnected(), SUCCEEDED(hr)));
    if (m_fmClub.IsConnected())
    {
        // Let's formally announce ourselves to the server
        BEGIN_PFM_CREATE(m_fmClub, pfmLogon, C, LOGON_CLUB)
            FM_VAR_PARM(pci->szName, CB_ZTS)
            FM_VAR_PARM(m_ci.pZoneTicket, m_ci.cbZoneTicket)
        END_PFM_CREATE
        pfmLogon->verClub = ALLCLUBVER;

        debugf("Logging on to Club Server...\n");
        lstrcpy(m_szClubCharName, pci->szName);
        SendClubMessages();
    }
    m_cUnansweredPings = 0;
    m_serverOffsetValidF = false;
    return hr;
}


void BaseClient::FindStandaloneServersByName(const char* szName, TList<TRef<LANServerInfo> >& listResults)
{
    listResults.SetEmpty();
    m_plistFindServerResults = &listResults;

    m_fm.EnumSessions(FEDSRV_STANDALONE_PRIVATE_GUID, szName);

    m_plistFindServerResults = NULL;
};

void BaseClient::SetCDKey(const ZString& strCDKey)
{
    m_strCDKey = strCDKey;
}

void BaseClient::HandleAutoDownload(DWORD dwTimeAlloted)
{
    if (m_pAutoDownload)
    {
       m_pAutoDownload->HandleAutoDownload(dwTimeAlloted);
    }
}


IweaponIGC* BaseClient::GetWeapon(void)
{
    return (IweaponIGC*)m_ship->GetMountedPart(ET_Weapon, m_selectedWeapon);
}

void BaseClient::SetAutoPilot(bool autoPilot)
{
    m_ship->SetAutopilot(autoPilot);
}

void BaseClient::DisconnectLobby()
{
    m_fLoggedOnToLobby = false;

    if (m_fmLobby.IsConnected())
        m_fmLobby.Shutdown();
}

void BaseClient::DisconnectClub()
{
    m_fLoggedOnToClub = false;

    if (m_fmClub.IsConnected())
        m_fmClub.Shutdown();

    m_szClubCharName[0] = '\0';
}

void BaseClient::Disconnect(void) 
{
    if (m_fLoggedOn)
    {
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmLogoff, CS, LOGOFF)
        END_PFM_CREATE
        SendMessages();
        // let's just spin for a few seconds handling messages directly from here, and wait until we either see our logoff
        // come back (m_fLoggedOn = false) or until we get sick of waiting. This will ensure that the server cleanly kills 
        // this player. Obviously the ui will be hung during this time.

        // Ok, that was a nice idea, but entering the receive loop re-entrantly was not safe, so we're just going to wait, 
        // and hope that the message gets out. The only risk here is that if the server doesn't get the logoff, it will think
        // the player was dropped instead of cleanly logged off.
        Sleep(500);
    }

    if (m_fm.IsConnected())
    {
        m_fm.Shutdown();
        m_fLoggedOn = false;
    }

	// BT - STEAM 
	CancelSteamAuthSessionToGameServer();

    m_szCharName[0] = '\0';
    m_szIGCStaticFile[0] = '\0';
}

void    BaseClient::SetPlayerInfo(PlayerInfo* p)
{
    assert (p);
    m_pPlayerInfo = p;

    //Set the fields that are derived from the player's info
    m_ship->SetObjectID(p->ShipID());
    m_ship->SetName(p->CharacterName());

    assert (p->SideID() == NA);
    assert (m_ship->GetSide() == NULL);
}

void BaseClient::SetViewCluster(IclusterIGC* pcluster, const Vector*  pposition)
{
    m_viewCluster = pcluster;
}

void    BaseClient::ResetShip(void)
{
    assert (GetSide());

    //Lose all of the parts
    {
        const PartListIGC*  plist = m_ship->GetParts();
        PartLinkIGC*        plink;
        while (plink = plist->first())  //Not ==
        {
            plink->data()->Terminate();
        }
    }

    m_ship->SetBaseHullType(GetSide()->GetCivilization()->GetLifepod());

    //Do not set these before setting the hull (since we might not have has a hull and that would
    //trigger the 0 mass assert).
    m_ship->SetAmmo(0);
    m_ship->SetFuel(0.0f);

    m_pmodelServerTarget = NULL;
}

/*void    BaseClient::ResetClusterScanners(IsideIGC*  pside)
{
    SideID  sid = pside->GetObjectID();

    //Because the player joined a side the scanner information for each "our" side is not correct
    //fix that problem
    {
        //Stations ...
        for (StationLinkIGC*   l = pside->GetStations()->first();
             (l != NULL);
             l = l->next())
        {
            IstationIGC*    pstation = l->data();
            pstation->GetCluster()->GetClusterSite()->AddScanner(sid, pstation);
        }
    }
    {
        //Probes ...
    }
}*/

void    BaseClient::BuyLoadout(IshipIGC*    pshipLoadout, bool bLaunch)
{
    assert(pshipLoadout);

    if (m_fm.IsConnected ())
    {
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmBuyLoadout, C, BUY_LOADOUT)
            FM_VAR_PARM(NULL, pshipLoadout->ExportShipLoadout(NULL))
        END_PFM_CREATE

        assert (pshipLoadout->GetBaseHullType());

        pshipLoadout->ExportShipLoadout((ShipLoadout*)(FM_VAR_REF(pfmBuyLoadout, loadout)));

        pfmBuyLoadout->fLaunch = bLaunch;

        if (bLaunch)
            StartLockDown("Preparing ship for launch....", lockdownLoadout);
        else
            StartLockDown("Loading out ship....", lockdownLoadout);
    }
    else
    {
        // I'm training here
        short   size = pshipLoadout->ExportShipLoadout(NULL);
//TODO: Check me !
        void*   ptr = malloc(size);
        pshipLoadout->ExportShipLoadout (static_cast <ShipLoadout*> (ptr));
        m_ship->PurchaseShipLoadout (size, static_cast <ShipLoadout*> (ptr));
        m_pClientEventSource->OnPurchaseCompleted (true);
        free(ptr);

        if (bLaunch) {
            assert(m_ship->GetStation());
            IstationIGC* s = m_ship->GetStation();
            m_ship->SetStation(NULL);
            s->Launch(m_ship);
        }
    }
}

IshipIGC*   BaseClient::CopyCurrentShip(void)
{
    assert (m_ship);

    TRef<IshipIGC>  pshipLoadout = CreateEmptyShip();

    //Copy the hull
    pshipLoadout->SetBaseHullType(m_ship->GetBaseHullType());

    //Copy the parts
    for (PartLinkIGC*   ppl = m_ship->GetParts()->first(); (ppl != NULL); ppl = ppl->next())
    {
        IpartIGC*   ppart = ppl->data();
        pshipLoadout->CreateAndAddPart(ppart->GetPartType(), ppart->GetMountID(), ppart->GetAmount());
    }

    return pshipLoadout;
}

IshipIGC*   BaseClient::CreateEmptyShip(ShipID sid)
{
    DataShipIGC    ds;

    ds.hullID = NA;
    ds.shipID = sid;
    ds.nParts = 0;
    ds.sideID = SIDE_TEAMLOBBY;
    ds.name[0] = '\0';
    //ds.wingID = 0;
    ds.pilotType = c_ptCheatPlayer;
    ds.abmOrders = 0;
    ds.nDeaths = 0;
    ds.nEjections = 0;
    ds.nKills = 0;
    ds.baseObjectID = NA;

    IshipIGC* pship = (IshipIGC*)(m_pCoreIGC->CreateObject(m_lastSend, OT_ship, &ds, sizeof(ds)));
    //pship->SetSide(GetCore()->GetSide(SIDE_TEAMLOBBY));

    return pship;
}

void    BaseClient::SetMoney(Money m)
{ 
    assert(m >= 0);

    // XXX still a hack until I work out exactly how to get the player info set up
    if (m_pPlayerInfo)
    {
        m_pPlayerInfo->SetMoney(m);
        m_pClientEventSource->OnMoneyChange(m_pPlayerInfo);
    }

    if (m_pMissionInfo)
    {
        SideInfo* psideinfo = m_pMissionInfo->GetSideInfo(GetSideID());
        if (psideinfo)
        {
            psideinfo->GetMembers().GetSink()();
        }
    }
}

bool    BaseClient::SendUpdate(Time now)
{
    bool fSent = false;

    if (flyingF() && m_fm.IsConnected())
    {
        if (m_ship->GetParentShip() == NULL)
        {
            static const int    smSendUpdateNow = weaponsMaskIGC | selectedWeaponMaskIGC |
                                                  coastButtonIGC |
                                                  backwardButtonIGC | forwardButtonIGC |
                                                  leftButtonIGC | rightButtonIGC |
                                                  upButtonIGC | downButtonIGC |
                                                  afterburnerButtonIGC;


            //int     stateM = m_ship->GetStateM();
            if ((now >= m_lastSend + c_dsecUpdateClient) /* || ((m_oldStateM ^ stateM) & smSendUpdateNow) */)
            {
                ZAssert(m_ship->GetPosition().LengthSquared());

                m_lastSend = now;

                /*
                ** We only "own" one object which needs to be updated -- the player's ship --
                ** so just send an update for it.
                */
                SetMessageType(c_mtNonGuaranteed);
                BEGIN_PFM_CREATE(m_fm, pfmShip, C, SHIP_UPDATE)
                END_PFM_CREATE

                m_ship->ExportShipUpdate(&(pfmShip->shipupdate));
                pfmShip->cookie = m_cookie;
                pfmShip->timeUpdate = ServerTimeFromClientTime(m_ship->GetLastUpdate());

                //m_oldStateM = stateM;
                fSent = true;
            }
        }
        else
        {
            static const int    smSendUpdateNow = weaponsMaskIGC;

            //int     stateM = m_ship->GetStateM();
            if ((now >= m_lastSend + c_dsecUpdateClient) /* || ((m_oldStateM ^ stateM) & smSendUpdateNow)*/)
            {
                m_lastSend = now;

                /*
                ** We only "own" one object which needs to be updated -- the player's ship --
                ** so just send an update for it.
                */
                SetMessageType(c_mtNonGuaranteed);

                int     stateM = m_ship->GetStateM();
                if (stateM & weaponsMaskIGC)
                {
                    //Turret is shooting ... facing matters
                    BEGIN_PFM_CREATE(m_fm, pfmTurret, C, ACTIVE_TURRET_UPDATE)
                    END_PFM_CREATE

                    m_ship->ExportShipUpdate(&(pfmTurret->atu));
                    pfmTurret->timeUpdate = ServerTimeFromClientTime(m_ship->GetLastUpdate());
                }
                else
                {
                    //Turret is not shooting ... facing doesn't matter so don't bother to send it
                    BEGIN_PFM_CREATE(m_fm, pfmTurret, C, INACTIVE_TURRET_UPDATE)
                    END_PFM_CREATE
                }

                //m_oldStateM = stateM;
                fSent = true;
            }
        }
    }

    return(fSent);
}

void    BaseClient::SetMessageType(MessageType  messageType)
{
    // If the message type was None, there shldn't be anything in the message outbox
    // CURTC: I didn't write this, and I don't like it :-)
    assert(IMPLIES(m_messageType == c_mtNone, m_fm.CbUsedSpaceInOutbox() == 0));
    if (messageType != m_messageType)
    {
        SendMessages();
        m_messageType = messageType;
    }
}

void    BaseClient::SendMessages(void)
{
    // Message type should be none iff there is nothing in the outbox
    assert (IMPLIES((m_messageType == c_mtNone), (m_fm.CbUsedSpaceInOutbox() == 0))); 
    if (m_messageType != c_mtNone)
    {
        ZSucceeded(m_fm.SendMessages(m_fm.GetServerConnection(), 
                                     m_messageType == c_mtGuaranteed ? FM_GUARANTEED : FM_NOT_GUARANTEED, 
                                     FM_FLUSH));
        m_messageType = c_mtNone;
    }
}

void BaseClient::NextWeapon(void)
{
    assert (m_ship);
    if (m_ship->GetParentShip() == NULL)
    {
        Mount nHardpoints = m_ship->GetHullType()->GetMaxFixedWeapons();
        if (nHardpoints > 0)
        {
            Mount       startWeapon = m_selectedWeapon;
            IpartIGC*   p;
            do
            {
                m_selectedWeapon = (m_selectedWeapon + 1) % nHardpoints;
                p = m_ship->GetMountedPart(ET_Weapon, m_selectedWeapon);
            }
            while ((p == NULL) && (m_selectedWeapon != startWeapon));
        }
    }
}

void BaseClient::PreviousWeapon(void)
{
    assert (m_ship);
    if (m_ship->GetParentShip() == NULL)
    {
        Mount nHardpoints = m_ship->GetHullType()->GetMaxFixedWeapons();

        if (nHardpoints > 0)
        {
            Mount       startWeapon = m_selectedWeapon;
            IpartIGC*   p;
            do
            {
                m_selectedWeapon =
                    m_selectedWeapon ? (m_selectedWeapon - 1) : nHardpoints - 1;
                p = m_ship->GetMountedPart(ET_Weapon, m_selectedWeapon);
            }
            while ((p == NULL) && (m_selectedWeapon != startWeapon));
        }
    }
}


void BaseClient::SetSelectedWeapon(Mount id)
{
    assert (m_ship);

    if (m_ship->GetParentShip() == NULL)
    {
        Mount nHardpoints = m_ship->GetHullType()->GetMaxFixedWeapons();

        if (id < nHardpoints && m_ship->GetMountedPart(ET_Weapon, id))
        {
            m_selectedWeapon = id;
        }
    }
}


void BaseClient::JoinMission(MissionInfo * pMission, const char* szMissionPassword)
{
    assert (pMission);
    BEGIN_PFM_CREATE(m_fmLobby, pfmJoinGameReq, C, JOIN_GAME_REQ)
    END_PFM_CREATE
    pfmJoinGameReq->dwCookie = pMission->GetCookie();

	if (m_fmLobby.IsConnected() == true)
	{
		SendLobbyMessages();
		m_dwCookieToJoin = pMission->GetCookie();
		assert(strlen(szMissionPassword) < c_cbGamePassword);
		strncpy(m_strPasswordToJoin, szMissionPassword, c_cbGamePassword);
		m_strPasswordToJoin[c_cbGamePassword - 1] = '\0';
		// waiting for FM_L_JOIN_GAME_ACK. When we get that we can join it
	}	
}


void BaseClient::CreateMissionReq()
{
    //No need to set message type for non m_fm messages
    BEGIN_PFM_CREATE(m_fmLobby, pfmCreateMission, C, CREATE_MISSION_REQ)
    END_PFM_CREATE
    SendLobbyMessages();
    // now we wait for FM_L_CREATE_MISSION_ACK
}
// KGJV #114
void BaseClient::ServerListReq()
{
    //No need to set message type for non m_fm messages
    BEGIN_PFM_CREATE(m_fmLobby, pfmGetServerReq, C, GET_SERVERS_REQ)
    END_PFM_CREATE
    SendLobbyMessages();
    // now we wait for FM_L_SERVERS_LIST
}
void BaseClient::CreateMissionReq(const char *szServer, const char *szAddr, const char *szIGCStaticFile, const char *szGameName)
{
    //No need to set message type for non m_fm messages
    BEGIN_PFM_CREATE(m_fmLobby, pfmCreateMission, C, CREATE_MISSION_REQ)
		FM_VAR_PARM(szServer,        CB_ZTS)
		FM_VAR_PARM(szAddr,          CB_ZTS)
		FM_VAR_PARM(szIGCStaticFile, CB_ZTS)
		FM_VAR_PARM(szGameName,      CB_ZTS)
    END_PFM_CREATE
    SendLobbyMessages();
    // now we wait for FM_L_CREATE_MISSION_ACK
}
#ifdef markcu
    const int g_cPingsTimeout = int(unsigned(1<<31)-1); // close to infinity as possible for int
#else
    const int g_cPingsTimeout = 5;
#endif
const DWORD g_sDeadTime = 30; // 30 seconds


void BaseClient::CheckServerLag(Time now)
{
    const float maxLagCheck = 6.0f;
    const float minLagCheck = 1.0f;

    if (m_fm.IsConnected())
    {
        //if (0 == m_timeLastPing.clock()) // start the ping clock once we get a ship
          //m_timeLastPing = now;
        if (m_lastLagCheck + maxLagCheck <= now)
        {
            // debugf("DPlay latency=%dms\n", m_fm.GetLatency(m_fm.GetServerConnection())); -- this doesn't report accurate values

            // if we haven't received x pings in a row and it's been at least g_sDeadTime since the last message, we're a goner
            if (m_cUnansweredPings > g_cPingsTimeout && (now - m_timeLastServerMessage) >= g_sDeadTime)
            {
                // what should that number be? 
                OnSessionLost("Pings aren't coming back.", &m_fm);
            }
            else
            {
                m_lastLagCheck = now;

                // make sure we get an update on the server offset even if the 
                // connection is poor.
                bool bGuaranteed = m_cUnansweredPings > g_cPingsTimeout;

                SetMessageType(bGuaranteed ? c_mtGuaranteed : c_mtNonGuaranteed);
                BEGIN_PFM_CREATE(m_fm, pfmPing, CS, PING)
                END_PFM_CREATE

                pfmPing->fmg = bGuaranteed ? FM_GUARANTEED : FM_NOT_GUARANTEED;
                pfmPing->timeClient  = Time::Now();

                SendMessages();
                m_cUnansweredPings++;
            }
        }
    }
}


HRESULT BaseClient::ReceiveMessages(void)
{
    HRESULT hr;

    if (m_fm.IsConnected())
        hr = m_fm.ReceiveMessages();
    else 
        hr = S_OK;

    if (m_fmLobby.IsConnected())
    {
        if (FAILED(hr)) // preserve fail sign
            m_fmLobby.ReceiveMessages();
        else
            hr = m_fmLobby.ReceiveMessages();
    }

    if (m_fmClub.IsConnected())
    {
        if (FAILED(hr)) // preserve fail sign
            m_fmClub.ReceiveMessages();
        else
            hr = m_fmClub.ReceiveMessages();
    }

    return(hr);
}

HRESULT BaseClient::OnSessionLost(const char * szReason, FedMessaging * pthis)
{
    debugf("OnSessionLost. %s. lastUpdate=%d now=%d Time::Now()=%d\n", 
           szReason, m_lastUpdate.clock(), m_now.clock(), Time::Now().clock());
    m_fLoggedOn = false; // we don't want to try and log off, cause the link is dead

    if (pthis == &m_fm)
        Disconnect();
    else if (pthis == &m_fmClub)
        DisconnectClub();
    else if (pthis == &m_fmLobby)
        DisconnectLobby();
    else
        assert(false);

    return(S_OK);
}


void BaseClient::OnMessageNAK(FedMessaging * pthis, DWORD dwTime, CFMRecipient * prcp)
{
    // we can't recover from this, so...
    OnSessionLost("guaranteed message couldn't be delivered", pthis);
}


HRESULT BaseClient::OnSessionLost(FedMessaging * pthis)
{
    return OnSessionLost("DPlay session terminated", pthis);
}


void BaseClient::OnSessionFound(FedMessaging * pthis, FMSessionDesc * pSessionDesc)
{
    m_plistFindServerResults->PushEnd(
        new LANServerInfo(pSessionDesc->GetInstance(), pSessionDesc->GetGameName(), 
            pSessionDesc->GetNumPlayers(), pSessionDesc->GetMaxPlayers())
        );
}

void BaseClient::ReceiveChat(IshipIGC*   pshipSender,
                             ChatTarget  ctRecipient,
                             ObjectID    oidRecipient,
                             SoundID     voiceOver,
                             const char* pszText,
                             CommandID   cid,
                             ObjectType  otTarget,
                             ObjectID    oidTarget,
                             ImodelIGC*  pmodelTarget,
                             bool        bObjectModel)
{
    m_pClientEventSource->OnNewChatMessage();
}

/*
void BaseClient::OfflineCommandToDrone (const ChatData* pcd, const DataBuoyIGC* pdb, CFSShip* pfsSender, IshipIGC* pshipTo, ImodelIGC** ppmodelTarget)
{
    assert (pfsSender);

    IsideIGC*   pside = pshipTo->GetSide();
    if ((pcd->commandID != c_cidNone) &&
        (pfsSender->GetIGCShip()->GetSide() == pside))
    {
        if (*ppmodelTarget == NULL)
        {
            if (pdb)
            {
                //Create a buoy for this chat message
                *ppmodelTarget = (ImodelIGC*)(pfsMission->GetIGCMission()->CreateObject(g.timeNow, OT_buoy, pdb, sizeof(*pdb)));
                assert (*ppmodelTarget);

                ((IbuoyIGC*)*ppmodelTarget)->AddConsumer();
            }
            else
            {
                *ppmodelTarget = pfsMission->GetIGCMission()->GetModel(otTarget, oidTarget);
            }
        }

        CommandID   cid = pcd->commandID;
        if ((cid == c_cidDefault) && (*ppmodelTarget != NULL))
        {
            cid = c_cidGoto;
            PilotType   pt = pshipTo->GetPilotType();
            switch (pt)
            {
                case c_ptMiner:
                case c_ptBuilder:
                {
                    if (((*ppmodelTarget)->GetObjectType() == OT_asteroid) &&
                        ((IasteroidIGC*)(*ppmodelTarget))->HasCapability(pshipTo->GetOrdersABM()))
                    {
                        cid = (pt == c_ptMiner) ? c_cidMine : c_cidBuild;
                    }
                }
                break;

                case c_ptLayer:
                {
                    if ((*ppmodelTarget)->GetObjectType() == OT_buoy)
                    {
                        cid = c_cidBuild;
                    }
                }
                break;

                case c_ptWingman:
                {
                    IsideIGC*   psideTarget = (*ppmodelTarget)->GetSide();
                    if (psideTarget == pside)
                        cid = c_cidDefend;
                    else
                        cid = c_cidAttack;
                }
            }
        }

        if (pshipTo->LegalCommand(cid, *ppmodelTarget))
        {
            pfsMission->GetSite()->SendChat(pshipTo, CHAT_INDIVIDUAL, pfsSender->GetIGCShip()->GetObjectID(),
                                            voAffirmativeSound, "Affirmative.");

            if (cid == c_cidJoin)
            {
                //Join the targets wing
                assert ((*ppmodelTarget)->GetObjectType() == OT_ship);

                WingID  wid = ((IshipIGC*)*ppmodelTarget)->GetWingID();

                pshipTo->SetWingID(wid);

                BEGIN_PFM_CREATE(g.fm, pfmWingChange, CS, SET_WINGID)
                END_PFM_CREATE
                pfmWingChange->shipID = pshipTo->GetObjectID();
                pfmWingChange->wingID = wid;
                g.fm.SendMessages(CFSSide::FromIGC(pside)->GetGroup(), FM_GUARANTEED, FM_FLUSH);
            }
            else
            {
                //Set both current and accepted commands
                pshipTo->SetCommand(c_cmdCurrent,
                                    *ppmodelTarget,
                                    cid);
                pshipTo->SetCommand(c_cmdAccepted,
                                    *ppmodelTarget,
                                    cid);
            }
        }
        else
        {
            pfsMission->GetSite()->SendChat(pshipTo, CHAT_INDIVIDUAL, pfsSender->GetIGCShip()->GetObjectID(),
                                            voNegativeSound, "Negative.");
        }
    }
}
*/

bool    BaseClient::ParseShellCommand(const char* pszCommand)
{
    bool    bCommand = false;

    if (_strnicmp(pszCommand, "!mute ", 6) == 0)
    {
        PlayerInfo* ppi = FindPlayer(pszCommand + 6);
        if (ppi)
            ppi->SetMute(true);
        bCommand = true;
    }
    else if (_strnicmp(pszCommand, "!unmute ", 8) == 0)
    {
        PlayerInfo* ppi = FindPlayer(pszCommand + 8);
        if (ppi)
            ppi->SetMute(false);
        bCommand = true;
    }
#ifdef _DEBUG
    else if (_strnicmp(pszCommand, "!chatcountdown ", 8) == 0)
    {
        int i = atoi(pszCommand + strlen("!chatcountdown "));
        char cbTemp[20];

        for (; i > 0; i--)
        {
            SendChat(m_ship, CHAT_EVERYONE, NA, NA, _itoa(i, cbTemp, 10));

            // send the messages every so often so that we don't overflow the 
            // send buffer
            if (i % 16 == 0)
                SendMessages();
        }
        bCommand = true;
    }
#endif

    return bCommand;
}
void    BaseClient::SendChat(IshipIGC*      pshipSender,
                             ChatTarget     ctRecipient,
                             ObjectID       oidRecipient,
                             SoundID        soundID,
                             const char*    pszText,
                             CommandID      cid,
                             ObjectType     otTarget,
                             ObjectID       oidTarget,
                             ImodelIGC*     pmodelTarget,
                             bool           bObjectModel)
{
    if (oidRecipient == NA)
    {
        //Convert from NA to an actual ID based on the player's ship
        switch (ctRecipient)
        {
            case CHAT_INDIVIDUAL:
                oidRecipient = m_ship->GetObjectID();
            break;

            case CHAT_WING:
                oidRecipient = m_ship->GetWingID();
            break;

            case CHAT_TEAM:
                oidRecipient = m_ship->GetSide()->GetObjectID();
            break;

            case CHAT_ALL_SECTOR:
            case CHAT_FRIENDLY_SECTOR:
            {
                IclusterIGC*    pcluster = GetChatCluster();
                if (pcluster)
                    oidRecipient = pcluster->GetObjectID();
                else
                {
                    oidRecipient = m_ship->GetSide()->GetObjectID();
                    ctRecipient = CHAT_TEAM;
                }
            }
            break;
        }
    }

    if ((ctRecipient == CHAT_INDIVIDUAL) || (ctRecipient == CHAT_GROUP) || (ctRecipient == CHAT_GROUP_NOECHO))
    {
        IshipIGC*   pship = m_pCoreIGC->GetShip(oidRecipient);
        if ((pship == NULL) || (pship->GetMission() == NULL))
            return;     //Don't bother to send to the dead

        // do the right thing for training missions
        PilotType   pilotType = pship->GetPilotType();
        if ((!m_fm.IsConnected ()) && (pilotType < c_ptPlayer) && (cid != c_cidNone) && (pship->GetSide()->GetObjectID() == m_pPlayerInfo->SideID()))
        {
            if ((cid == c_cidDefault) && (pmodelTarget != NULL))
            {
                cid = c_cidGoto;
                if (ctRecipient == CHAT_INDIVIDUAL)
                {
                    switch (pilotType)
                    {
                        case c_ptMiner:
                        case c_ptBuilder:
                        {
                            if ((pmodelTarget->GetObjectType() == OT_asteroid) && ((IasteroidIGC*) pmodelTarget)->HasCapability(pship->GetOrdersABM()))
                            {
                                cid = (pilotType == c_ptMiner) ? c_cidMine : c_cidBuild;
                            }
                        }
                        break;

                        case c_ptLayer:
                        {
                            if (pmodelTarget->GetObjectType() == OT_buoy)
                            {
                                cid = c_cidBuild;
                            }
                        }
                        break;

                        case c_ptWingman:
                        {
                            if (pmodelTarget->GetObjectType() == OT_buoy)
                                cid = c_cidGoto;
                            else if (pmodelTarget->GetSide() == pship->GetSide())
                                cid = c_cidDefend;
                            else
                                cid = c_cidAttack;
                        }
                    }
                }
            }
            pship->SetCommand (c_cmdCurrent, pmodelTarget, cid);
            pship->SetCommand (c_cmdAccepted, pmodelTarget, cid);
            //ReceiveChat (m_ship, ctRecipient, oidRecipient, soundID, pszText, cid, otTarget, oidTarget, pmodelTarget);
        }
    }

    //Don't bother sending messages to the server if there is no server or the message it to ourself
    if (m_fm.IsConnected() &&
        (ctRecipient != CHAT_NOSELECTION) &&
        ((ctRecipient != CHAT_INDIVIDUAL) ||
         (oidRecipient != m_ship->GetObjectID())))
    {
        SetMessageType(BaseClient::c_mtGuaranteed);

        ChatData*   pcd;

        if (otTarget == OT_buoy)
        {
            BEGIN_PFM_CREATE(m_fm, pfmChatBuoy, CS, CHATBUOY)
            FM_VAR_PARM(pszText, CB_ZTS)
            END_PFM_CREATE
            pcd = &(pfmChatBuoy->cd);

            assert (pmodelTarget);
            pmodelTarget->Export(&(pfmChatBuoy->db));
        }
        else
        {
            BEGIN_PFM_CREATE(m_fm, pfmChatMessage, CS, CHATMESSAGE)
            FM_VAR_PARM(pszText, CB_ZTS)
            END_PFM_CREATE

            pcd = &(pfmChatMessage->cd);

            pfmChatMessage->otTarget = otTarget;
            pfmChatMessage->oidTarget = oidTarget;
        }

        //Don't bother to set the sender
        pcd->chatTarget = ctRecipient;

        pcd->oidRecipient = oidRecipient;
        pcd->commandID = cid;
        pcd->voiceOver = soundID;
        pcd->bObjectModel = bObjectModel;
    }

    //We always get chats we send
    if (ctRecipient != CHAT_GROUP_NOECHO)
    {
        ReceiveChat(pshipSender,
                    ctRecipient, oidRecipient, 
                    soundID, pszText,
                    cid, otTarget, oidTarget, pmodelTarget, bObjectModel);
    }
}

void BaseClient::ScrollChatUp(void)
{
    if (m_plinkSelectedChat == NULL)
        m_plinkSelectedChat = m_chatList.last();
    else if (m_plinkSelectedChat != m_chatList.first())
        m_plinkSelectedChat = m_plinkSelectedChat->txen();
}

void BaseClient::ScrollChatDown(void)
{
    if (m_plinkSelectedChat != NULL)
        m_plinkSelectedChat = m_plinkSelectedChat->next();
}

ChatInfo* BaseClient::GetCurrentMessage(void)
{
    ChatLink*   l = (m_plinkSelectedChat != NULL)
                    ? m_plinkSelectedChat
                    : m_chatList.last();

    return l ? &(l->data()) : NULL;
}

BallotInfo* BaseClient::GetCurrentBallot()
{
    Time now = Time::Now();

    // remove any expired ballots before the first valid ballot
    while (!m_listBallots.IsEmpty() && m_listBallots.GetFront().GetBallotExpirationTime() < now)
        m_listBallots.PopFront();

    if (m_listBallots.IsEmpty())
        return NULL;
    else
        return &(m_listBallots.GetFront());
}

void BaseClient::Vote(bool bAgree)
{
    if (!m_listBallots.IsEmpty())
    {
        // compose a vote message
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmVote, C, VOTE)
        END_PFM_CREATE

        pfmVote->ballotID = m_listBallots.GetFront().GetBallotID();
        pfmVote->bAgree = bAgree;

        m_listBallots.PopFront();
    }
}

void BaseClient::SkipCurrentBallot()
{
    if (!m_listBallots.IsEmpty())
    {
        m_listBallots.PopFront();
    }
}

Money BaseClient::GetBucketStatus(StationID stationID, short iBucket)
{
    TRef<BucketStatusArray> prgStatus;
    if (m_mapBucketStatusArray.Find(stationID, prgStatus))
        return (*prgStatus)[iBucket];
    else
        return 0;
}


Money        BaseClient::AddMoneyToBucket(IbucketIGC*    b,
                                         Money          m)
{
    assert (b);
    Money   spent = b->AddMoney(m);
    SetMoney(GetMoney() - spent);

    if (m_fm.IsConnected())
    {
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmBD, C, BUCKET_DONATE)
        END_PFM_CREATE

        pfmBD->moneyGiven = spent;
        pfmBD->iBucket = b->GetObjectID();
    }

    return spent;
}

void BaseClient::DonateMoney(PlayerInfo* pPlayerInfo, Money money)
{
    if (m_fm.IsConnected())
        {
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmDonate, C, PLAYER_DONATE)
        END_PFM_CREATE

        pfmDonate->moneyGiven = money;
        pfmDonate->shipID = pPlayerInfo->ShipID();
        }

    // the recipients money changes
    pPlayerInfo->SetMoney(pPlayerInfo->GetMoney() + money);
    m_pClientEventSource->OnMoneyChange(pPlayerInfo);

    // and my money changes
    SetMoney(GetMoney() - money);
}

MissionInfo* BaseClient::GetLobbyMission(DWORD dwCookie)
{
    MissionInfo* pMissionInfo = NULL;
    if (!m_mapMissions.Find(dwCookie, pMissionInfo))
    {
        pMissionInfo = new MissionInfo(dwCookie);
        m_mapMissions.Set(dwCookie, pMissionInfo);
    }
    return pMissionInfo;
}

PlayerLink* BaseClient::FindPlayerLink(ShipID shipID)
{
    PlayerLink* l = m_listPlayers.first();
    while ((l != NULL) && (l->data().ShipID() != shipID))
         l = l->next();
    return l;
}

PlayerInfo* BaseClient::FindAndCreatePlayerLink(ShipID shipID)
{
    PlayerLink* l = FindPlayerLink(shipID);

    if (l == NULL)
    {
        l = new PlayerLink;
        m_listPlayers.last(l);
    }

    return &(l->data ());
}

PlayerInfo* BaseClient::FindPlayer(ShipID shipID)
{
    PlayerLink* l = FindPlayerLink(shipID);
    return l ? &(l->data()) : NULL;
}

PlayerInfo* BaseClient::FindPlayer(const char* szName)
{
    PlayerInfo* p = NULL;

    for (PlayerLink* l = m_listPlayers.first(); (l != NULL); l = l->next())
        {
        if (_stricmp(l->data().CharacterName(), szName) == 0)
            {
            p = &(l->data());
            break;
            }
        }

    return p;
}

PlayerInfo* BaseClient::FindPlayerByPrefix(const char* szNamePrefix)
{
    PlayerInfo* p = NULL;
    int lenNamePrefix = strlen(szNamePrefix);

    for (PlayerLink* l = m_listPlayers.first(); (l != NULL); l = l->next())
    {
        PlayerInfo& player = l->data();

        if (_strnicmp(player.CharacterName(), szNamePrefix, lenNamePrefix) == 0)
        {
            p = &player;
            break;
        }
    }

    return p;
}

namespace {
  // remove trailing whitespaces from a string
  void RemoveTrailingSpaces(char* sz)
  {
    for (int nIndex = strlen(sz); nIndex > 0 && sz[nIndex - 1] == ' '; --nIndex)
    {
      sz[nIndex - 1] = '\0';    
    }
  };
}


// -Imago: Same ranks for all civs w/o club
ZString BaseClient::LookupRankName(RankID rank, CivID civ)
{
    const char* szRankNameTemplate = "Unknown (%d)";
    int nClosestRank = -1;

    if (m_cRankInfo <= 0 && !GetIsZoneClub())
    {
        assert(!m_fm.IsConnected() || !GetIsZoneClub());
        szRankNameTemplate = "";
    }
    else
    {
		// BT - 7/15 CSS Integration - TODO STEAM: Modify to work with steam ranks.
		if (rank >= 0)
		{
			if (rank <= m_cRankInfo)
			{
				nClosestRank = rank;
				szRankNameTemplate = m_vRankInfo[rank].RankName;
			}
			else
			{
				nClosestRank = m_cRankInfo;
				szRankNameTemplate = m_vRankInfo[m_cRankInfo].RankName;
			}
		}

		// BT - 7/15 - Removing; this was too slow. 

		//  //if (!GetIsZoneClub()) //Imago REVISIT 9/14
		//civ = -1;
		//    // 'slow', but probably still fast enough
		//    for (int iEntry = 0; iEntry < m_cRankInfo; iEntry++)
		//    {
		//        if (m_vRankInfo[iEntry].civ == civ 
		//            && m_vRankInfo[iEntry].rank <= rank 
		//            && m_vRankInfo[iEntry].rank >= nClosestRank)
		//        {
		//            szRankNameTemplate = m_vRankInfo[iEntry].RankName;
		//            nClosestRank = m_vRankInfo[iEntry].rank;
		//        }
		//    }
		//    assert(nClosestRank >= 0);
    }

    char cbTemp[c_cbName + 8];
    wsprintf(cbTemp, szRankNameTemplate, rank - nClosestRank + 1);
	RemoveTrailingSpaces(cbTemp);

    return cbTemp;
}

List* BaseClient::GetMissionList()
{
    return m_plistMissions;
}

void BaseClient::QuitMission()
{
    // Create and queue the message to the server
    BaseClient::SetMessageType(c_mtGuaranteed);
    BEGIN_PFM_CREATE(*GetNetwork(), pfmQuitMission, CS, QUIT_MISSION)
    END_PFM_CREATE
    pfmQuitMission->shipID = GetShipID();
    pfmQuitMission->reason = QSR_Quit;
}

bool BaseClient::MyMissionInProgress()
{
    if (m_pMissionInfo)
        return m_pMissionInfo->InProgress();
    else
        return false;
}

void BaseClient::SaveSquadMemberships(const char* szCharacterName)
{
}

void BaseClient::RestoreSquadMemberships(const char* szCharacterName)
{
}

bool BaseClient::HasPlayerSquad(MissionInfo* pMission)
{
    for (TList<SquadMembership>::Iterator iterSquad(m_squadmemberships);
        !iterSquad.End(); iterSquad.Next())
    {
        if (pMission->HasSquad(iterSquad.Value().GetID()))
            return true;
    }

    return false;
}

void BaseClient::SendAllMissions(IClientEventSink* pSink)
{
    TMapListWrapper<DWORD, MissionInfo*>::Iterator iterMissions(m_mapMissions);
    MissionInfo* pMissionInfo;
    while (!iterMissions.End())
        {
        pMissionInfo = iterMissions.Value();
        pSink->OnAddMission(pMissionInfo);
        iterMissions.Next();
        }
}

void BaseClient::SendAllPlayers(IClientEventSink* pSink, MissionInfo* pMissionInfo, SideID sideID)
{
    if (pMissionInfo)
        {
        ShipList::Iterator iterPlayers(pMissionInfo->GetSideInfo(sideID)->GetMembers());
        PlayerInfo* pPlayerInfo;
        ShipID shipID;
        while (!iterPlayers.End()) 
            {
            shipID = iterPlayers.Value();
            if (pPlayerInfo = FindPlayer(shipID))
                pSink->OnAddPlayer(pMissionInfo, sideID, pPlayerInfo);
            iterPlayers.Next();
            }
        }
}

void BaseClient::SendAllRequests(IClientEventSink* pSink, MissionInfo* pMissionInfo, SideID sideID)
{
    ShipList::Iterator iterRequests(pMissionInfo->GetSideInfo(sideID)->GetRequests());
    PlayerInfo* pPlayerInfo;
    ShipID shipID;
    while (!iterRequests.End()) 
        {
        shipID = iterRequests.Value();
        if (pPlayerInfo = FindPlayer(shipID))
            pSink->OnAddRequest(pMissionInfo, sideID, pPlayerInfo);
        iterRequests.Next();
        }
}


//
// IgcSite implementation.
//
void BaseClient::TerminateModelEvent(ImodelIGC* pModel)
{
    ObjectType    type = pModel->GetObjectType();

    if (type != OT_projectile)  //can never target projectiles
    {
        //Clean up any references to the model
        {
            bool    fChanges = false;
            for (ChatLink*  l = m_chatList.first();
                 (l != NULL);
                 l = l->next())
            {
                ChatInfo&    cw = l->data();

                if (cw.GetTarget() == pModel)
                {
                    cw.ClearTarget();

                    fChanges = true;
                }
            }

            if (fChanges)
                m_pClientEventSource->OnChatMessageChange();
        }

        for (Command i = 0; (i < c_cmdMax); i++)
        {
            if (m_ship->GetCommandTarget(i) == pModel)
                m_ship->SetCommand(i, NULL, c_cidNone);
        }

        if (pModel == m_pmodelServerTarget)
            m_pmodelServerTarget = NULL;
    }

    m_pClientEventSource->OnModelTerminated(pModel);
}

void BaseClient::LoadoutChangeEvent(IshipIGC* pship, IpartIGC* ppart, LoadoutChange lc)
{
    if (m_ship->GetSourceShip() == pship)
        m_pClientEventSource->OnLoadoutChanged(ppart, lc);
}


void BaseClient::BucketChangeEvent(BucketChange bc, IbucketIGC* b)
{
    m_pClientEventSource->OnBucketChange(bc, b);
}

void BaseClient::SideBuildingTechChange(IsideIGC* s)
{
    SideID  sid = s->GetObjectID();
    m_pClientEventSource->OnTechTreeChanged(sid);
}

void BaseClient::SideDevelopmentTechChange(IsideIGC* s)
{
    SideID  sid = s->GetObjectID();
    m_pClientEventSource->OnTechTreeChanged(sid);
}

void BaseClient::StationTypeChange(IstationIGC* s)
{
    SideID  sid = s->GetSide()->GetObjectID();
    m_pClientEventSource->OnTechTreeChanged(sid);
}

ItreasureIGC*  BaseClient::CreateTreasureLocal(Time now, IshipIGC* pship, IpartIGC* p, IpartTypeIGC* ppt, const Vector& position, float dv, float lifespan)
{
    assert(p);

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

    return CreateTreasureLocal(now, pship, amount, ppt, position, dv, lifespan);
}

ItreasureIGC*  BaseClient::CreateTreasureLocal(Time now, IshipIGC* pship, short amount, IpartTypeIGC* ppt, const Vector& position, float dv, float lifespan)
{
    assert(pship);
    assert(ppt);
    assert(dv > 1.0f);

    DataTreasureIGC dt;
    dt.treasureCode = c_tcPart;
    dt.treasureID = ppt->GetObjectID();
    dt.lifespan = lifespan;
    dt.amount = amount;
    dt.createNow = false;

    dt.objectID = pship->GetMission()->GenerateNewTreasureID();
    dt.p0 = position;
    Vector direction = Vector::RandomDirection();
    float   radius = pship->GetRadius() + 10.0f;
    dt.p0 += radius * direction;
    dt.v0 = direction * dv + pship->GetVelocity();
    IclusterIGC*    pcluster = pship->GetCluster();
    dt.clusterID = pcluster->GetObjectID();
    dt.time0 = now;

    ItreasureIGC* t = (ItreasureIGC *)
        pship->GetMission()->CreateObject(now, OT_treasure,
            &dt, sizeof(dt));

    //Note: bad form releasing a pointer before we return it but we know it will
    //stick around since it is in a cluster.
    assert(t);
    t->Release();
    return t;
}

void BaseClient::KillAsteroidEvent(IasteroidIGC*            pasteroid, bool explodeF)
{
    if (!m_fm.IsConnected())
    {
        if (explodeF)
            pasteroid->GetCluster()->GetClusterSite()->AddExplosion(pasteroid, c_etAsteroid);

        pasteroid->Terminate();
    }
}
void BaseClient::KillProbeEvent(IprobeIGC*            pprobe)
{
    if (!m_fm.IsConnected())
        pprobe->Terminate();
}
void BaseClient::KillMineEvent(ImineIGC*            pmine)
{
    if (!m_fm.IsConnected())
        pmine->Terminate();
}
void BaseClient::KillMissileEvent(ImissileIGC*            pmissile, const Vector& position)
{
    if (!m_fm.IsConnected())
    {
        pmissile->Explode(position);
        pmissile->Terminate();
    }
    else
        pmissile->Disarm();
}
void BaseClient::KillTreasureEvent(ItreasureIGC* ptreasure)
{
    if (!m_fm.IsConnected()) {
        ptreasure->Terminate();
    }
}
void BaseClient::KillShipEvent(Time now, IshipIGC* pShip, ImodelIGC* pLauncher, float flAmount, const Vector& p1, const Vector& p2)
{
    if (!m_fm.IsConnected())
    {
        // Previous version commented out because these were causing performance problems after a while BSW 10/27/1999
        // Blow the parts into space
        {
            const PartListIGC*  plist = pShip->GetParts();
            PartLinkIGC*        plink;
            while (plink = plist->first())  //Not ==
            {
                IpartIGC*   p = plink->data();

                if (randomInt(0, 4) == 0)
                    CreateTreasureLocal(now, pShip, p, p->GetPartType(), p1, 100.0f, 60.0f);
                p->Terminate();
            }

            //Treasures for ammo
            {
                short ammo = pShip->GetAmmo();
                if (ammo > 0)
                {
                    IpartTypeIGC*  pptAmmo = m_pCoreIGC->GetAmmoPack();
                    assert(pptAmmo);

                    if (randomInt(0, 4) == 0)
                        CreateTreasureLocal(now, pShip, ammo, pptAmmo, p1, 100.0f, 30.0f);
                    //pShip->SetAmmo(0); //What's the point .. let's not trigger reload here
                }
            }
            //Ditto for fuel
            {
                short fuel = short(pShip->GetFuel());
                if (fuel > 0)
                {
                    IpartTypeIGC*  pptFuel = m_pCoreIGC->GetFuelPack();
                    assert(pptFuel);

                    if (randomInt(0, 4) == 0)
                        CreateTreasureLocal(now, pShip, fuel, pptFuel, p1, 100.0f, 30.0f);
                    //pShip->SetFuel(0.0f);
                }
            }
        }

        if (pShip == m_ship->GetSourceShip())
        {
            //Eject the player
            Vector  v = pShip->GetVelocity();

            pShip->SetBaseHullType(pShip->GetSide()->GetCivilization()->GetLifepod());

            Vector      f = Vector::RandomDirection();
            Orientation o;
            if (pLauncher)
            {
                if (pLauncher->GetCluster() == pShip->GetCluster())
                {
                    Vector  dp = (pLauncher->GetPosition() - p1).Normalize();

                    o.Set(dp);
                    f = (f + dp * 8.0f).Normalize();
                }
                else
                {
                    o.Set(f);
                }
            }
            else
            {
                Vector  dp = (p2 - p1).Normalize();

                o.Set(dp);
                f = (f + dp * 8.0f).Normalize();
            }

            v -= f * 100.0f;

            //Put a spin on the ship as it leaves the bad guys.
            pShip->SetCurrentTurnRate(c_axisRoll, pi * 2.0f);

            pShip->SetPosition(p1);
            pShip->SetVelocity(v);
            pShip->SetOrientation(o);

            pShip->SetBB(now, now, 0.0f);

            EjectPlayer(pLauncher);
        }
        else
        {
            PlayerInfo* pPlayerInfo = reinterpret_cast<PlayerInfo*> (pShip->GetPrivateData ());
            RemovePlayerFromSide (pPlayerInfo, QSR_Quit);
            RemovePlayerFromMission (pPlayerInfo, QSR_Quit);
        }
    }
}


void BaseClient::DamageStationEvent(IstationIGC* pStation,
                                    ImodelIGC* pLauncher,
                                    DamageTypeID type, 
                                    float amount)
{
    if ((NULL != pStation->GetCluster()) &&
        (pStation->GetCluster() == GetCluster()))
    {
        if (pStation->GetShieldFraction() < 0.0f)
            this->PlaySoundEffect(otherHullHitSound, pStation);
        else
            this->PlaySoundEffect(otherShieldHitSound, pStation);
    }
}

void BaseClient::FireMissile(IshipIGC* pShip,
                            ImagazineIGC* pMagazine,
                            Time timeFired,
                            ImodelIGC* pTarget,
                            float flLock)
{
    assert (pShip == m_ship || !m_fm.IsConnected());

    if (pTarget &&
        ((pTarget->GetCluster() != m_ship->GetCluster()) ||
         !m_ship->CanSee(pTarget)))
        pTarget = NULL;

    this->PlaySoundEffect(pMagazine->GetMissileType()->GetLaunchSound(), pShip);
    this->PlayFFEffect(effectFire);

    IclusterIGC*    pCluster = pShip->GetCluster();
    assert (pCluster);

    const Vector&       myVelocity      = pShip->GetVelocity();
    const Orientation&  myOrientation   = pShip->GetOrientation();
    Vector              myPosition      = pShip->GetPosition() +
                                          pMagazine->GetEmissionPt() * myOrientation;

    int                 iNumMissiles = pMagazine->GetLaunchCount ();
    int                 iNumRemainingMissiles = pMagazine->GetAmount ();
    Time                lastUpdate = pShip->GetLastUpdate();

    if (iNumRemainingMissiles < iNumMissiles)
        iNumMissiles = iNumRemainingMissiles;
    iNumRemainingMissiles -= iNumMissiles;

    DataMissileIGC      dataMissile;
    dataMissile.pmissiletype    = pMagazine->GetMissileType();
    dataMissile.pLauncher       = pShip;
    dataMissile.pTarget         = pTarget;
    dataMissile.pCluster        = pCluster;
    dataMissile.lock            = flLock;


    Vector              vecLaunchPosition = myPosition + (myVelocity * (timeFired - lastUpdate));
    Vector              vecLaunchForward = myOrientation.GetForward();
    Vector              vecLaunchVelocity = myVelocity;
    MissileLaunchData*  missileLaunchData = new MissileLaunchData[iNumMissiles];

    // disperse the missiles over the launch region
    float               fDispersion = dataMissile.pmissiletype->GetDispersion();
    float               fDispersionBase = -fDispersion;
    float               fUniformDispersion = (2.0f * fDispersion) / (float) iNumMissiles;
    float               fHalfUniformDispersion = fUniformDispersion * 0.5f;
    for (int i = 0; i < iNumMissiles; i++)
    {
        missileLaunchData[i].vecForward = vecLaunchForward;
        if (fDispersion != 0.0f)
        {
            float   fRandomDispersion = (random (-0.75f, 0.75f) * fHalfUniformDispersion) + (fDispersionBase + fHalfUniformDispersion);
            fDispersionBase += fUniformDispersion;
            missileLaunchData[i].vecForward += fRandomDispersion * myOrientation.GetRight();
            missileLaunchData[i].vecForward += (random (-0.1f, 0.1f) * fDispersion) * myOrientation.GetUp();
            missileLaunchData[i].vecForward.SetNormalize();
        }
        ZRetailAssert(missileLaunchData[i].vecForward * missileLaunchData[i].vecForward >= 0.95f);
        ZRetailAssert(missileLaunchData[i].vecForward * missileLaunchData[i].vecForward <= 1.05f);

        missileLaunchData[i].vecPosition = vecLaunchPosition + (missileLaunchData[i].vecForward * (dataMissile.pmissiletype->GetInitialSpeed() * random (0.1f, 0.2f)));
        ZRetailAssert(missileLaunchData[i].vecPosition * missileLaunchData[i].vecPosition >= 0.0f);
        ZRetailAssert(missileLaunchData[i].vecPosition * missileLaunchData[i].vecPosition <= 4.0e10f);

        missileLaunchData[i].vecVelocity = vecLaunchVelocity + (dataMissile.pmissiletype->GetInitialSpeed() * missileLaunchData[i].vecForward);
        ZRetailAssert(missileLaunchData[i].vecVelocity * missileLaunchData[i].vecVelocity >= 0.0f);
        //ZRetailAssert(missileLaunchData[i].vecVelocity * missileLaunchData[i].vecVelocity <= 1.0e6); //Imago 8/12/09
    }

    if (!m_fm.IsConnected())
    {
        for (int i = 0; i < iNumMissiles; i++)
        {
            dataMissile.position    = missileLaunchData[i].vecPosition;
            dataMissile.forward     = missileLaunchData[i].vecForward;
            dataMissile.velocity    = missileLaunchData[i].vecVelocity;
            dataMissile.missileID   = m_pCoreIGC->GenerateNewMissileID ();
            dataMissile.bDud        = false;
            ImissileIGC*  m = (ImissileIGC*)(m_pCoreIGC->CreateObject(timeFired, OT_missile, &dataMissile, sizeof(dataMissile)));
            assert (m != NULL);
            m->Release();
        }
    }
    else
    {

        //Tell the server (& everyone else) to fire a missile
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmFM, CS, FIRE_MISSILE)
        FM_VAR_PARM(missileLaunchData, sizeof(MissileLaunchData) * iNumMissiles)
        END_PFM_CREATE

        if (NULL != pTarget)
        {
            pfmFM->targetType = pTarget->GetObjectType();
            pfmFM->targetID = pTarget->GetObjectID();
        }
        else
        {
            pfmFM->targetType = OT_invalid;
            pfmFM->targetID = NA;
        }
        pfmFM->clusterID = pCluster->GetObjectID();
        pfmFM->lock = flLock;

        pfmFM->timeFired = ServerTimeFromClientTime(timeFired);

        //Don't bother to set the ship, missile or missile type ID ... the server will set them
    }

    delete missileLaunchData;

    pMagazine->SetAmount (iNumRemainingMissiles);
    if (0 == iNumRemainingMissiles)
    {
        //
        // Nothing left ... nuke it (which may also cause it to be
        // released & deleted).
        //
        if (Reload(pShip, pMagazine, ET_Magazine))
        {
            if (pShip == m_ship) {
                PlayNotificationSound(salReloadingMissilesSound, GetShip());
                PlaySoundEffect(startReloadSound, GetShip());
                PostText(false, "Reloading missiles...");
            }
        }
        else
        {
            if (pShip == m_ship) {
                PlayNotificationSound(salMissilesDepletedSound, GetShip());
                PostText(false, "Missiles depleted.");
            }
        }
    }
}

void BaseClient::FireExpendable(IshipIGC* pShip,
                                IdispenserIGC* pDispenser,
                                Time timeFired)
{
    assert(pShip == m_ship || !m_fm.IsConnected());

    this->PlayFFEffect(effectFire, pShip);

    IclusterIGC*    pCluster = pShip->GetCluster();
    assert (pCluster);

    IexpendableTypeIGC* pet = pDispenser->GetExpendableType();
    ObjectType type = pet->GetObjectType();

    if (pShip == m_ship) {
        switch (type)
        {
        case OT_chaffType:
            this->PlaySoundEffect(deployChaffSound, pShip);
            break;

        case OT_mineType:
            this->PlaySoundEffect(deployMineSound, pShip);
            break;

        case OT_probeType:
            this->PlaySoundEffect(deployProbeSound, pShip);
            break;
        }
    }

    if (!m_fm.IsConnected())
    {
        const Vector&      myPosition = pShip->GetPosition();
        const Vector&      myVelocity = pShip->GetVelocity();

        if (type == OT_chaffType)
        {
            assert (type == OT_chaffType);

            //Drop the chaff "behind" the ship

            DataChaffIGC   dataChaff;

            dataChaff.time0 = timeFired;
            dataChaff.p0 = myPosition;
            dataChaff.v0 = myVelocity + pShip->GetOrientation().GetUp() * 25.0f;

            dataChaff.pcluster = pCluster;
            dataChaff.pchafftype = (IchaffTypeIGC*)pet;

            IchaffIGC*  c = (IchaffIGC*)(m_pCoreIGC->CreateObject(timeFired,
                                                                  OT_chaff,
                                                                  &dataChaff,
                                                                  sizeof(dataChaff)));
            assert(c != NULL);

            //Confuse any missiles lauched at the ship
            for (MissileLinkIGC*  pml = pCluster->GetMissiles()->first(); (pml != NULL); pml = pml->next()) {
                ImissileIGC*    pmissile = pml->data();
                if (pmissile->GetTarget() == pShip)
                {
                    //A missile aimed at me .. does the chaff work?
                    float   chaff = ((IchaffTypeIGC*)pet)->GetChaffStrength();
                    float   missile = pmissile->GetMissileType()->GetChaffResistance();

                    //The following is equivalent to random(0, chaff) > random(0, missile)
                    float   cm = chaff * missile;
                    float   f = (chaff > missile)
                        ? (cm - 0.5f * missile * missile)
                        : (0.5f * chaff * chaff);

                    if (random(0.0f, cm) <= f) {
                        //Missile lost lock
                        pmissile->SetTarget(c);
                    }
                }
            }

            c->Release();
        }
        else
        {

            float              speed2 = myVelocity.LengthSquared();
            float              offset = (pet->GetRadius() + pShip->GetRadius()) + 5.0f;
            Vector             displace = (speed2 < 1.0f)
                                          ? (pShip->GetOrientation().GetBackward() * offset)
                                          : (myVelocity * (-offset / float(sqrt(speed2))));
            Vector             launchPosition = myPosition + displace;
            if (type == OT_mineType)
            {
                DataMineIGC   dataMine;
                dataMine.pminetype = (ImineTypeIGC*)pet;

                dataMine.time0 = timeFired + 3.0f;  //And the count shall be 3.
                dataMine.p0 = launchPosition;
                dataMine.exportF = false;

				dataMine.psideLauncher = pShip->GetSide(); //bahdohday (commit by AEM 7.12.07) Fix to allow mines to be dropped in training.
                dataMine.pshipLauncher = pShip;
                dataMine.pcluster = pCluster;

                dataMine.mineID = m_pCoreIGC->GenerateNewMineID();

                ImineIGC*  m = (ImineIGC*)(m_pCoreIGC->CreateObject(timeFired,
                                                                    OT_mine,
                                                                    &dataMine,
                                                                    sizeof(dataMine)));
                assert (m != NULL);
                m->Release();
            }
            else if (type == OT_probeType)
            {
                //Drop the probe "behind" the player's ship

                DataProbeIGC   dataProbe;
                dataProbe.pprobetype = (IprobeTypeIGC*)pet;

                dataProbe.time0 = timeFired;
                dataProbe.p0 = launchPosition;
                dataProbe.exportF = false;

                dataProbe.pcluster = pCluster;
                dataProbe.pside = pShip->GetSide();
                dataProbe.pship = pShip;
                dataProbe.pmodelTarget = pShip->GetCommandTarget(c_cmdCurrent);

                dataProbe.probeID = m_pCoreIGC->GenerateNewProbeID();

                IprobeIGC*  p = (IprobeIGC*)(m_pCoreIGC->CreateObject(timeFired,
                                                                      OT_probe,
                                                                      &dataProbe,
                                                                      sizeof(dataProbe)));
                assert (p != NULL);
                p->Release();
            }
        }
    }
    else
    {
        //Tell the server to fire an expendable
        SetMessageType(c_mtGuaranteed);
        BEGIN_PFM_CREATE(m_fm, pfmFE, C, FIRE_EXPENDABLE)
        END_PFM_CREATE

        pfmFE->et = pet->GetEquipmentType();
    }

    short   amount = pDispenser->GetAmount() - 1;
    pDispenser->SetAmount(amount);
    if (0 == amount)
    {
        //
        // Nothing left ... nuke it (which may also cause it to be
        // released & deleted).
        //
        if (Reload(pShip, pDispenser, pet->GetEquipmentType()))
        {
            if (pShip == m_ship) {
                switch (pet->GetObjectType())
                {
                case OT_chaffType:
                    PlayNotificationSound(salReloadingChaffSound, GetShip());
                    PostText(false, "Reloading chaff...");
                    break;

                default:
                    PlayNotificationSound(salReloadingDispenserSound, GetShip());
                    PostText(false, "Reloading dispenser...");
                    break;
                }

                PlaySoundEffect(startReloadSound, GetShip());
            }
        }
        else if (pShip == m_ship)
        {
            switch (pet->GetObjectType())
            {
            case OT_chaffType:
                PlayNotificationSound(salChaffDepletedSound, GetShip());
                PostText(false, "Chaff depleted.");
                break;

            default:
                PlayNotificationSound(salDispenserEmptySound, GetShip());
                PostText(false, "Dispenser empty.");
                break;
            }
        }
    }
}

bool BaseClient::Reload(IshipIGC* pship, IlauncherIGC* plauncher, EquipmentType type)
{
    int         nReloads = 0;

    if (pship == m_ship || !m_fm.IsConnected())
    {
        const int   c_MaxReloads = 8;
        ReloadData  reloads[c_MaxReloads];

        const IhullTypeIGC* pht = pship->GetHullType();

        switch (type)
        {
            case ET_Weapon:
            case ET_Afterburner:
            {
                PackType    ptDesired;
                short       ammo;
                short       maxAmmo;
                if (type == ET_Weapon)
                {
                    ptDesired = c_packAmmo;
                    ammo = pship->GetAmmo();
                    maxAmmo = pht->GetMaxAmmo();
                }
                else
                {
                    ptDesired = c_packFuel;
                    ammo = short(pship->GetFuel());
                    maxAmmo = short(pht->GetMaxFuel());
                }

                while (ammo < maxAmmo)
                {
                    /*
                    //Find the ammo/fuel pack with the least amount of ammo/fuel in it (first if all the same)
                    IpackIGC*   ppack = NULL;
                    short       amount = 0x7fff;
                    {
                        for (Mount i = -1; (i >= -c_maxCargo); i--)
                        {
                            IpartIGC*   ppart = pship->GetMountedPart(NA, i);

                            if (ppart && (ppart->GetObjectType() == OT_pack))
                            {
                                IpackIGC*   p = (IpackIGC*)ppart;

                                if (p->GetPackType() == ptDesired)
                                {
                                    short   a = p->GetAmount();
                                    if (a < amount)
                                    {
                                        ppack = p;
                                        amount = a;
                                    }
                                }
                            }
                        }
                    }
                    */
                    // Find the last ammo/fuel pack (avoids mysterious behavior in inventory pane)
                    IpackIGC*   ppack = NULL;
                    short       amount = 0x7fff;
                    {
                        for (Mount i = -1; (i >= -c_maxCargo); i--)
                        {
                            IpartIGC*   ppart = pship->GetMountedPart(NA, i);

                            if (ppart && (ppart->GetObjectType() == OT_pack))
                            {
                                IpackIGC*   p = (IpackIGC*)ppart;

                                if (p->GetPackType() == ptDesired)
                                {
                                    ppack = p;
                                    amount = p->GetAmount();
                                }
                            }
                        }
                    }

                    if (ppack)
                    {
                        reloads[nReloads].mount = ppack->GetMountID();
                        short   transfer;
                        if (ammo + amount > maxAmmo)
                        {
                            //There will be stuff left in the pack
                            transfer = maxAmmo - ammo;
                            ammo = maxAmmo;

                            ppack->SetAmount(amount - transfer);
                        }
                        else
                        {
                            //Entire pack gets trasnfered
                            transfer = NA;
                            ammo += amount;

                            ppack->Terminate();
                        }
                        reloads[nReloads++].amountTransfered = transfer;
                    }
                    else
                        break;
                }

                if (nReloads != 0)
                {
                    if (type == ET_Weapon)
                    {
                        pship->SetAmmo(ammo);

                        //disable all mounted weapons that use ammo
                        Mount   maxWeapons = pht->GetMaxWeapons();
                        for (Mount i = 0; (i < maxWeapons); i++)
                        {
                            IweaponIGC* pw = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, i));
                            if (pw && (pw->GetAmmoPerShot() != 0))
                                pw->SetMountedFraction(0.0f);
                        }
                    }
                    else
                    {
                        pship->SetFuel(float(ammo));

                        //disable the afterburner
                        IpartIGC*   p = pship->GetMountedPart(ET_Afterburner, 0);
                        if (p)
                            p->SetMountedFraction(0.0f);
                    }
                }
            }
            break;

            case ET_Magazine:
            case ET_Dispenser:
            case ET_ChaffLauncher:
            {
                IpartTypeIGC*   pparttype;
                short           ammo;
                short           maxAmmo;
                if (plauncher)
                {
                    pparttype = plauncher->GetPartType();
                    ammo = plauncher->GetAmount();
                    maxAmmo = ((IlauncherTypeIGC*)pparttype)->GetAmount(pship);
                }
                else
                {
                    pparttype = NULL;
                    ammo = 0;
                    maxAmmo = 0x7fff;
                }

                while (ammo < maxAmmo)
                {
                    IlauncherIGC*       plauncherReload = NULL;
                    IpartTypeIGC*       pptReload;
                    short               amount = 0x7fff;

                    {
                        for (Mount mount = -1; (mount >= -c_maxCargo); mount--)
                        {
                            IpartIGC*       p = pship->GetMountedPart(NA, mount);
                            if (p && (p->GetEquipmentType() == type))
                            {
                                IpartTypeIGC*   ppt = p->GetPartType();

                                if (pht->CanMount(ppt, 0))
                                {
                                    short           a = ((IlauncherIGC*)p)->GetAmount();
                                    assert (a > 0);

                                    if (pparttype == NULL)
                                    {
                                        //when one has nothing ... one will pick anything
                                        reloads[nReloads].amountTransfered = NA;
                                        reloads[nReloads++].mount = mount;

                                        if (plauncher)
                                            plauncher->Terminate();

                                        p->SetMountID(0);

                                        plauncher = (IlauncherIGC*)p;
                                        pparttype = plauncher->GetPartType();
                                        ammo = plauncher->GetAmount();
                                        assert (ammo > 0);
                                        maxAmmo = ((IlauncherTypeIGC*)pparttype)->GetAmount(pship);

                                        assert (plauncherReload == NULL);

                                        if (ammo == maxAmmo)
                                            break;              //No point in continuing if we are full up
                                    }
                                    else if ((ppt == pparttype) /*&& (a < amount)*/)
                                    {
                                        plauncherReload = (IlauncherIGC*)p;
                                        pptReload = ppt;
                                        amount = a;
                                    }
                                }
                            }
                        }
                    }

                    if (plauncherReload)
                    {
                        reloads[nReloads].mount = plauncherReload->GetMountID();

                        short   transfer;
                        if (ammo + amount <= maxAmmo)
                        {
                            transfer = amount;
                            plauncherReload->Terminate();
                        }
                        else
                        {
                            transfer = maxAmmo - ammo;
                            plauncherReload->SetAmount(amount - transfer);
                        }

                        ammo += transfer;

                        plauncher->SetAmount(ammo);
                        plauncher->SetMountedFraction(0.0f);
                        plauncher->ResetTimeLoaded();

                        reloads[nReloads++].amountTransfered = transfer;
                    }
                    else if (pparttype && (ammo == 0))
                    {
                        //Couldn't find a way to reload the existing launcher
                        //try again being less picky
                        assert (plauncher);

                        pparttype = NULL;
                        maxAmmo = 0x7fff;
                    }
                    else
                        break;
                }
            }
            break;
        }

        if (m_fm.IsConnected())
        {
            if (nReloads > 0)
            {
                int size = sizeof(ReloadData) * nReloads;

                SetMessageType(c_mtGuaranteed);
                BEGIN_PFM_CREATE(m_fm, pfmReload, CS, RELOAD)
                    FM_VAR_PARM(NULL, size)
                END_PFM_CREATE

                memcpy(FM_VAR_REF(pfmReload, rgReloads), reloads, size);
            }
            else if (plauncher && (plauncher->GetAmount() == 0))
            {
                SetMessageType(c_mtGuaranteed);
                BEGIN_PFM_CREATE(m_fm, pfmDrop, CS, DROP_PART)
                END_PFM_CREATE

                pfmDrop->et = plauncher->GetEquipmentType();
                pfmDrop->mount = plauncher->GetMountID();

                plauncher->Terminate();
            }
        }
        else {
            if (plauncher && (plauncher->GetAmount() == 0))
                plauncher->Terminate();
            if (nReloads <= 0 && type == ET_Weapon && pship->GetPilotType() == c_ptWingman) {
                debugf("Wingman %s out of ammo.\n", pship->GetName());

                int ttMask = c_ttFriendly | c_ttStation | c_ttNearest | c_ttAnyCluster;
                if (pship->GetHullType()->GetCapabilities() & c_habmLandOnCarrier)
                    ttMask |= c_ttShip;
                ImodelIGC*  pmodel = FindTarget(pship, ttMask,
                    NULL, NULL, NULL, NULL, c_sabmReload);
                debugf("found %s\n", pmodel ? pmodel->GetName() : "NULL");

                if (pmodel) {
                    pship->SetCommand(c_cmdPlan, pmodel, c_cidGoto);
                    m_pCoreIGC->GetIgcSite()->SendChat(pship, CHAT_TEAM, GetSide()->GetObjectID(),
                        NA, "Out of ammo, I'm getting resupplies."); //salNoAmmoSound
                    pship->SetGettingAmmo(true);
                }
            }
        }
    }

    return nReloads > 0;
}

void BaseClient::OnQuitSide()
{
    m_bInGame = false;
    m_bWaitingForGameRestart = false;

    if (IsLockedDown())
        EndLockDown(lockdownDonating | lockdownLoadout | lockdownTeleporting);

    // clear any team chats, in case they join another team
	/* Removed by Avalanche to ensure chat messages are kept. 
    ChatLink*  lChat = m_chatList.first();
    while (lChat != NULL)
    {
        ChatLink* lChatPrev = lChat;
        lChat = lChat->next();
        if (lChatPrev->data().GetChatTarget() != CHAT_EVERYONE)
            delete lChatPrev;
    }
	*/

    // nuke any saved player status information 
    // (in case they rejoin this game on a different side)
    PlayerLink* l = m_listPlayers.first();

    while (l != NULL)
    {
        PlayerInfo& playerCurrent = l->data();

        playerCurrent.ResetShipStatus();

        l = l->next();
    }


    assert (m_ship->GetStation() == NULL);
    assert (m_ship->GetCluster() == NULL);

    {
        // set all ships to the null sector
        ShipLinkIGC*         plinkShip = m_pCoreIGC->GetShips()->first();
        while (plinkShip)
        {
            plinkShip->data()->SetCluster(NULL);
            plinkShip = plinkShip->next();
        }
    }

    // nuke all clusters
    SetViewCluster(NULL);
    m_pCoreIGC->ResetMission();

    // nuke any pending votes
    m_listBallots.SetEmpty();
};


void BaseClient::OnJoinSide()
{
	// BT - STEAM
	// WHen joining a server, the player leaves the lobby, so cancel their lobby auth ticket, and transition to the server auth ticket.
	CancelSteamAuthSessionToLobby();

    ResetShip();
    m_strBriefingText.SetEmpty();
    m_bGenerateCivBriefing = false;

    if (m_pPlayerInfo->SideID() != SIDE_TEAMLOBBY)
        SetMoney(m_pPlayerInfo->GetMoney());

    if (m_pMissionInfo->InProgress())
    {
        m_pCoreIGC->SetMissionStage(STAGE_STARTED);
    }

    m_pClientEventSource->OnChatMessageChange();
    m_plinkSelectedChat = NULL;

    // if this is the lobby side, we need to manually send the OnAddPlayer message
    if (m_pPlayerInfo->SideID() == SIDE_TEAMLOBBY)
        m_pClientEventSource->OnAddPlayer(m_pMissionInfo, m_pPlayerInfo->SideID(), m_pPlayerInfo);
};

void BaseClient::OnQuitMission(QuitSideReason reason, const char* szMessageParam)
{
    Disconnect();

	// BT - STEAM
	CancelSteamAuthSessionToGameServer();

    // clear chat messages
    m_chatList.purge(true);
    m_pClientEventSource->OnClearChat();

    m_ship->AddRef();
    {
        // destroy all ships
        ShipLinkIGC*         plinkShip = m_pCoreIGC->GetShips()->first();
        while (plinkShip)
        {
            ShipLinkIGC*    lNext = plinkShip->next();

            IshipIGC*   s = plinkShip->data();
            plinkShip->data()->Terminate();

            plinkShip = lNext;
        }
    }
    m_ship->Release();

    CreateDummyShip();

    {
        // and sides
        const SideListIGC * pSides = m_pCoreIGC->GetSides();
        SideLinkIGC * pSidelink = NULL;
        while (pSidelink = pSides->first())
            pSidelink->data()->Terminate();
    }
    {
        // and players...
        m_listPlayers.purge(true);
        m_pPlayerInfo = NULL;
    }

    delete m_pMissionInfo;
    m_pMissionInfo = NULL;
};

void BaseClient::OnEnterGame()
{
    m_pCoreIGC->EnterGame();
    m_bInGame = true;

    m_pClientEventSource->OnEnterMission();
};

void BaseClient::CreateDummyShip()
{
    // create a (no-hull) ship for the player
    DataShipIGC ds;
    ds.hullID = NA;
    ds.shipID = NA;
    ds.nParts = 0;
    ds.sideID = NA;
    ds.name[0] = '\0';
    //ds.wingID = 0;
    ds.pilotType = c_ptPlayer;
    ds.abmOrders = 0;
    ds.nDeaths = 0;
    ds.nEjections = 0;
    ds.nKills = 0;
    ds.baseObjectID = NA;

    // Allow the ship data to be modified by derived classes
    ModifyShipData(&ds);

    m_ship = (IshipIGC*)(m_pCoreIGC->CreateObject(m_lastSend, OT_ship, &ds, sizeof(ds)));
    assert (m_ship);
    m_ship->Release();
};

void BaseClient::DestroyDummyShip()
{
  // Base class does nothing
}

void BaseClient::RemovePlayerFromSide(PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
{
    if (!pPlayerInfo)
    {
        assert(false);
        return;
    }

    ZAssert(pPlayerInfo->SideID() != SIDE_TEAMLOBBY && pPlayerInfo->SideID() != NA);  

    SideID sideOld = pPlayerInfo->SideID();

    ZAssert(pPlayerInfo->SideID() != NA);

    // pPlayerInfo->SetReady(true); // Imago commented out so afk does not reset
    pPlayerInfo->SetTeamLeader(false);
    pPlayerInfo->SetMissionOwner(false);
    pPlayerInfo->Reset(false);
    if (STAGE_STARTED != m_pMissionInfo->GetStage())
        pPlayerInfo->GetShip()->SetExperience(1.0f);

    m_pClientEventSource->OnMoneyChange(pPlayerInfo);
    m_pMissionInfo->RemovePlayer(pPlayerInfo);
    m_pClientEventSource->OnDelPlayer(m_pMissionInfo, pPlayerInfo->SideID(), pPlayerInfo, reason);

    IshipIGC* pship = m_pCoreIGC->GetShip(pPlayerInfo->ShipID());
    pship->SetSide(NULL);

    if (pPlayerInfo == m_pPlayerInfo)
    {
        OnQuitSide();
    }
    else
    {
        if (pPlayerInfo->IsHuman() && (m_pMissionInfo->GetStage() == STAGE_STARTED) && (m_fm.IsConnected ()))
        {
            ZString msg;

            // tell the player that someone has just quit their team        
            if (m_pPlayerInfo->SideID() == sideOld)
            {
                switch (reason)
                {
                case QSR_LeaderBooted:
                    msg = pPlayerInfo->CharacterName() + ZString(" was booted from the team by the team leader.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_OwnerBooted:
                    msg = pPlayerInfo->CharacterName() + ZString(" was booted from the game by the mission owner.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_AdminBooted:
                    msg = pPlayerInfo->CharacterName() + ZString(" was booted from the team by an admin.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_ServerShutdown:
                    break;

                case QSR_SquadChange:
                    msg = pPlayerInfo->CharacterName() + ZString(" was booted because of a squad change.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_SideDestroyed:
                    break;

                case QSR_RankLimits:
                    msg = pPlayerInfo->CharacterName() + ZString(" was booted because he/she did not meet the new rank limits.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_TeamSizeLimits:
                    msg = pPlayerInfo->CharacterName() + ZString(" was booted to reduce the team size within it's new limits.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_Quit:
                    msg = pPlayerInfo->CharacterName() + ZString(" has quit.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_SwitchingSides: 
                    msg = pPlayerInfo->CharacterName() + ZString(" is switching sides.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salChangeSidesSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_DuplicateLocalLogon:
                case QSR_DuplicateCDKey:
				case QSR_BannedBySteam:  // BT - STEAM
                    // don't send a chat in case they were dropped.
                    //msg = pPlayerInfo->CharacterName() + ZString(" has been booted due to a duplicate logon.");
                    //ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                case QSR_RandomizeSides:
                    break;

                case QSR_DuplicateRemoteLogon:
                    msg = pPlayerInfo->CharacterName() + ZString(" has been booted due to a duplicate logon.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salQuitSound, msg, c_cidNone, NA, NA);
                    break;

                default:
                    ZAssert(false);
                    // intentional fallthrough
                case QSR_LinkDead:
                    msg = pPlayerInfo->CharacterName() + ZString(" is MIA.");
                    ReceiveChat(NULL, CHAT_TEAM, NA, salMIASound, msg, c_cidNone, NA, NA);
                    break;
                };
            }
            else
            { //#ALLY: imago made this sound corrected for allies 7/3/09
				IsideIGC* otherside = m_pCoreIGC->GetSide(pPlayerInfo->SideID());
				int sfx = (otherside->AlliedSides(otherside,GetSide())) ? salQuitSound : salEnemyLeavesSound;
                msg = pPlayerInfo->CharacterName() + ZString(" has left ") 
                    + GetCore()->GetSide(sideOld)->GetName() + ZString(".");
                ReceiveChat(NULL, CHAT_TEAM, NA, sfx, msg, c_cidNone, NA, NA);
            }
        }
    }

    pPlayerInfo->SetTeamLeader(false);
    pPlayerInfo->SetMissionOwner(false);
    pPlayerInfo->SetSideID(SIDE_TEAMLOBBY);
    pPlayerInfo->SetMoney(0);

    IsideIGC* pside = m_pCoreIGC->GetSide(SIDE_TEAMLOBBY);
    pship->SetSide(pside);
 
    if (pPlayerInfo == m_pPlayerInfo)
    {
        ResetShip();

        m_pClientEventSource->OnChatMessageChange();
        m_plinkSelectedChat = NULL;
    }
    m_pMissionInfo->AddPlayer(pPlayerInfo);

    // if this is the lobby side, we need to manually send the OnAddPlayer message
    if (m_pPlayerInfo->SideID() == SIDE_TEAMLOBBY)
        m_pClientEventSource->OnAddPlayer(m_pMissionInfo, pPlayerInfo->SideID(), pPlayerInfo);
}


void BaseClient::RemovePlayerFromMission(PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
{
    if (!pPlayerInfo)
    {
        assert(false);
        return;
    }

    // must call RemovePlayerFromSide first if the player is not on the team lobby
    ZAssert(pPlayerInfo->SideID() == SIDE_TEAMLOBBY);  
    assert(pPlayerInfo);
    
    m_pMissionInfo->RemovePlayer(pPlayerInfo);
    // pPlayerInfo->SetReady(true); Imago commented out so afk does not reset

    if (pPlayerInfo == m_pPlayerInfo)
    {
        // we need to manually send the DelPlayerMessage
        m_pClientEventSource->OnDelPlayer(m_pMissionInfo, pPlayerInfo->SideID(), pPlayerInfo, reason);
    }    

    pPlayerInfo->SetSideID(NA);

    IshipIGC*   pship = pPlayerInfo->GetShip();
    
    pship->SetSide(NULL);

    if (pPlayerInfo != m_pPlayerInfo)
    {
        // destroy their ship and player info
        pship->Terminate();

        PlayerLink* l = FindPlayerLink(pPlayerInfo->ShipID());
        delete l;
    }
    else
    {
        // we are quitting the mission...
        OnQuitMission(reason, szMessageParam);
    }
}


void BaseClient::AddPlayerToMission(PlayerInfo* pPlayerInfo)
{
    assert(pPlayerInfo);

    pPlayerInfo->SetSideID(SIDE_TEAMLOBBY);

    IshipIGC*   pship = pPlayerInfo->GetShip();
    IsideIGC*   pside = m_pCoreIGC->GetSide(SIDE_TEAMLOBBY);
    pship->SetSide(pside);

    if (pPlayerInfo == m_pPlayerInfo)
    {
        m_pCoreIGC->SetMissionParams(&(m_pMissionInfo->GetMissionParams()));
        m_pCoreIGC->UpdateSides(Time::Now(), &(m_pMissionInfo->GetMissionParams()), m_pMissionInfo->GetMissionDef().rgszName);
		m_pCoreIGC->UpdateAllies(m_pMissionInfo->GetMissionDef().rgfAllies);//#ALLY
        OnJoinSide();
    }
    m_pMissionInfo->AddPlayer(pPlayerInfo);
}


void BaseClient::AddPlayerToSide(PlayerInfo* pPlayerInfo, SideID sideID)
{
    assert(pPlayerInfo);
    assert(pPlayerInfo->SideID() == SIDE_TEAMLOBBY);

    SideID sideOld = pPlayerInfo->SideID();

    m_pMissionInfo->RemovePlayer(pPlayerInfo);
    assert(pPlayerInfo->GetMoney() == 0);
    // pPlayerInfo->SetReady(true); Imago commented out so afk does not reset

    if (pPlayerInfo == m_pPlayerInfo)
    {
        OnQuitSide();
    }

    pPlayerInfo->SetSideID(sideID);

    IshipIGC*   pship = pPlayerInfo->GetShip();
    IsideIGC*   pside = m_pCoreIGC->GetSide(sideID);
    pship->SetSide(pside);

    if (pPlayerInfo == m_pPlayerInfo)
    {
        OnJoinSide();
    }

    m_pMissionInfo->AddPlayer(pPlayerInfo);

    if (pPlayerInfo->IsTeamLeader())
        m_pMissionInfo->SetSideLeader(pPlayerInfo);

    // now officially on team so remove any requests to this mission
    if (m_pMissionInfo->FindRequest(pPlayerInfo->SideID(), pPlayerInfo->ShipID()))
        m_pMissionInfo->RemoveRequest(pPlayerInfo->SideID(), pPlayerInfo->ShipID()); 
            // don't notify... notification assumed in OnAddPlayer

    // update the last seen info
    pPlayerInfo->ResetShipStatus();

    m_pClientEventSource->OnAddPlayer(m_pMissionInfo, pPlayerInfo->SideID(), pPlayerInfo);

    if (pPlayerInfo->IsHuman() && m_pMissionInfo->GetStage() == STAGE_STARTED && m_fm.IsConnected())
    {
        // tell the players that someone has just joined their team
		IsideIGC* otherside = GetCore()->GetSide(pPlayerInfo->SideID());
        if ( (m_pPlayerInfo != pPlayerInfo) && (((m_pPlayerInfo->SideID() == pPlayerInfo->SideID()) || otherside->AlliedSides(otherside,GetSide())) )) //#ALLY -imago 7/3/09
        {
			if (otherside != GetSide() && otherside->AlliedSides(otherside,GetSide())) {
                Color AllianceColors[3] = { Color::Green(), Color::Orange(), Color::Red() };
            	ZString msg = pPlayerInfo->CharacterName() + ZString(" has joined ") + pside->GetName() + ZString(" \x81 ") + ConvertColorToString(AllianceColors[pside->GetAllies()]*0.75) + "(allied)" + END_COLOR_STRING + ".";
                ReceiveChat(NULL, CHAT_TEAM, NA, 0, msg, c_cidNone, NA, NA);
			} else {
            	ZString msg = pPlayerInfo->CharacterName() + ZString(" has joined your team.");
	            ReceiveChat(NULL, CHAT_TEAM, NA, salRecruitsArrivedSound, msg, c_cidNone, NA, NA);
			}
        }
        else if (m_pPlayerInfo && GetSide() && GetSideID() != SIDE_TEAMLOBBY)
        {
            ZString msg = pPlayerInfo->CharacterName() + ZString(" has joined ")
                + pside->GetName() + ZString(".");
            ReceiveChat(NULL, CHAT_TEAM, NA, salEnemyJoinersSound, msg, c_cidNone, NA, NA);
        }
    }
}


/*-------------------------------------------------------------------------
 * ResetStaticData
 *-------------------------------------------------------------------------
  Purpose:
      Out with the old IGC data, in with the new

  Parameters:
      IGC staic file to load

  Returns:
      whether data set was successfully loaded

  Side Effects:
      Old data set is nuked (better not have any references to IGC stuff when calling this)
 */
static void DoDecrypt(int size, char* pdata)
{
    DWORD encrypt = 0;
    for (int i = 0; (i < size); i += 4)
    {
        DWORD*  p = (DWORD*)(pdata + i);

        encrypt = *p = *p ^ encrypt;
    }
}
bool BaseClient::ResetStaticData(const char * szIGCStaticFile, ImissionIGC** ppStaticIGC, Time tNow, bool bEncrypt)
{
    assert(ppStaticIGC);
    if (&m_pCoreIGC == ppStaticIGC)
    {
        FlushGameState();
        *ppStaticIGC = CreateMission();
        Initialize(tNow);
    }
    else
    {
        // In this special case, the static mission must have already been created and initialized
        assert(*ppStaticIGC);
    }

    // copy the core name
    lstrcpy(m_szIGCStaticFile, szIGCStaticFile);

    // here we load the static core data
    int iStaticCoreVersion = LoadIGCStaticCore (szIGCStaticFile, *ppStaticIGC, true); // first just check version

    // only reload the static igc core if this game uses a different one
    if (iStaticCoreVersion != m_pMissionInfo->GetIGCStaticVer())
    {
        return false;
    }

    // actually load the data
    LoadIGCStaticCore (szIGCStaticFile, *ppStaticIGC, false,  bEncrypt ? DoDecrypt : NULL); // then actually load the data

    return true;
}

void BaseClient::StartLockDown(const ZString& strReason, LockdownCriteria criteria)
{
    ZAssert((m_lockdownCriteria & criteria) == 0);
    
    m_lockdownCriteria |= criteria;
    m_strLockDownReason = strReason;
}

void BaseClient::EndLockDown(LockdownCriteria criteria)
{
    ZAssert(m_lockdownCriteria != 0);
    m_lockdownCriteria &= ~criteria;
}

void BaseClient::BoardShip(IshipIGC*  pship)
{
    SetMessageType(BaseClient::c_mtGuaranteed);
    BEGIN_PFM_CREATE(m_fm, pfmBoardShip, C, BOARD_SHIP)
    END_PFM_CREATE

    pfmBoardShip->sidParent = pship ? pship->GetObjectID() : NA;

    if (pship)
    {
        assert(GetShip()->GetParentShip() == NULL);
        StartLockDown("Boarding " + ZString(pship->GetName()) + "'s ship....", lockdownTeleporting);
    }
    else
    {
        IshipIGC* pshipParent = GetShip()->GetParentShip();
        assert(pshipParent);
        StartLockDown("Disembarking from " + ZString(pshipParent->GetName()) + "'s ship....", lockdownTeleporting);
    }
}
//KGJV #114
// call GetPrivateProfileString with preset cfgfile. Valid after a Load(...) only.
DWORD CfgInfo::GetCfgProfileString(
	const char *c_szCfgApp,
	const char *c_szKeyName,
	const char *c_szDefaultValue,
	char *szStr,
	DWORD dwSize)
{

	return GetPrivateProfileString(
		c_szCfgApp,
		c_szKeyName,
		c_szDefaultValue,
		szStr,
		dwSize,
		m_szConfigFile);
}
//KGJV #114 - set config file name for GetCfgProfileString without loading
void CfgInfo::SetCfgFile(const char * szConfig)
{
	m_szConfigFile = ZString(szConfig);
}
void CfgInfo::Load(const char * szConfig)
{
	m_szConfigFile = ZString(szConfig); // KGJV #114 - save config file name for GetCfgProfileString

    const char * c_szCfgApp = "Allegiance";
    char szStr[128]; // random number;
    
    GetPrivateProfileString(c_szCfgApp, "ZAuth", "auth.zone.com", 
                                  szStr, sizeof(szStr), szConfig);
    strZAuth = szStr;

    GetPrivateProfileString(c_szCfgApp, "ClubLobby", "", 
                                   szStr, sizeof(szStr), szConfig);
    strClubLobby = szStr;

    GetPrivateProfileString(c_szCfgApp, "PublicLobby", "", 
                                   szStr, sizeof(szStr), szConfig);
    strPublicLobby = szStr;

    GetPrivateProfileString(c_szCfgApp, "Club", "", 
                                   szStr, sizeof(szStr), szConfig);
    strClub = szStr;

    GetPrivateProfileString(c_szCfgApp, "ClubMessageURL", "http://fdl.msn.com/zone/allegiance/messageoftheday.mdl", // http://a-markcu1/test/messageoftheday.mdl
                                   szStr, sizeof(szStr), szConfig);
    strClubMessageURL = szStr;

    GetPrivateProfileString(c_szCfgApp, "PublicMessageURL", "http://fdl.msn.com/zone/allegiance/messageoftheday.mdl", // http://a-markcu1/test/messageoftheday.mdl
                                   szStr, sizeof(szStr), szConfig);
    strPublicMessageURL = szStr;

    GetPrivateProfileString(c_szCfgApp, "ZoneEventsURL", "", szStr, sizeof(szStr), szConfig);
    strZoneEventsURL = szStr;

    GetPrivateProfileString(c_szCfgApp, "ZoneEventDetailsURL", "http://fdl.msn.com/zone/allegiance/zoneevents.mdl", 
                                   szStr, sizeof(szStr), szConfig);
    strZoneEventDetailsURL = szStr;

    GetPrivateProfileString(c_szCfgApp, "TrainingURL", "http://www.zone.com/allegiance/downloads.asp", 
                                   szStr, sizeof(szStr), szConfig);
    strTrainingURL = szStr;

    GetPrivateProfileString(c_szCfgApp, "PassportUpdateURL", "http://www.zone.com/allegiance/passportupdate.html", 
                                   szStr, sizeof(szStr), szConfig);
    strPassportUpdateURL = szStr;

    GetPrivateProfileString(c_szCfgApp, "ZoneAuthGUID", "{00000000-0000-0000-C000-000000000046}", 
                                   szStr, sizeof(szStr), szConfig);
    int cbStr = strlen(szStr) + 1;
    LPOLESTR wszStr = (LPOLESTR)_alloca(sizeof(OLECHAR) * cbStr);
    MultiByteToWideChar(CP_ACP, 0, PCC(szStr), cbStr, wszStr, cbStr);
    if (cbStr == 1 || !ZSucceeded(IIDFromString(wszStr, &guidZoneAuth)))
    {
        guidZoneAuth = GUID_NULL;
    }

    GetPrivateProfileString(c_szCfgApp, "FilelistCRC", "0", 
                                   szStr, sizeof(szStr), szConfig);
    _strupr(szStr);
    crcFileList = UTL::hextoi(szStr);

    if (crcFileList == 0)
        debugf("FilelistCRC is missing or invalid from %s.\n", szConfig);

    GetPrivateProfileString(c_szCfgApp, "FilelistSize", "0", 
                                   szStr, sizeof(szStr), szConfig);

    nFilelistSize = atoi(szStr);

    if (nFilelistSize == 0)
        debugf("FilelistSize is missing or invalid from %s.\n", szConfig);

    GetPrivateProfileString(c_szCfgApp, "FilelistSite", "", 
                                   szStr, sizeof(szStr), szConfig);
    strFilelistSite = szStr;
    
    GetPrivateProfileString(c_szCfgApp, "FilelistDirectory", "", 
                                   szStr, sizeof(szStr), szConfig);
    strFilelistDirectory = szStr;

    GetPrivateProfileString(c_szCfgApp, "ClubMessageCRC", "0", 
                                    szStr, sizeof(szStr), szConfig);
    _strupr(szStr);
    crcClubMessageFile = UTL::hextoi(szStr);

    GetPrivateProfileString(c_szCfgApp, "PublicMessageCRC", "0", 
                                    szStr, sizeof(szStr), szConfig);
    _strupr(szStr);
    crcPublicMessageFile = UTL::hextoi(szStr);

    GetPrivateProfileString(c_szCfgApp, "UsePassport", "0", 
                                   szStr, sizeof(szStr), szConfig);
    bUsePassport = atoi(szStr) != 0;

    // mdvalley: get lobby port
    GetPrivateProfileString(c_szCfgApp, "LobbyClientPort", "2302",
                                   szStr, sizeof(szStr), szConfig);
	dwLobbyPort = atoi(szStr);
}

void _debugf(const char* format, ...)
{
    const size_t size = 512;
    char         bfr[size];

    va_list vl;
    va_start(vl, format);
    _vsnprintf(bfr, size, format, vl);
    va_end(vl);

    debugf(bfr);
}


