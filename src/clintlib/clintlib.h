#ifndef _CLINTLIB_
#define _CLINTLIB_

#include <Messages.h>
#include <messageslc.h>
#include <mission.h>
#include <steam_api.h>

class MissionInfo;
class SideInfo;
class PlayerInfo;
class ChatInfo;

class IClientEventSink;
class IClientEventSource;
class IAutoDownload;
class IAutoUpdateSink;

struct CachedPart
{
    TRef<IpartTypeIGC>  ppt;
    Mount               mount;
    bool                bDuplicated;
};

typedef Slist_utl<CachedPart>   CachedPartList;
typedef Slink_utl<CachedPart>   CachedPartLink;

struct  CachedLoadout
{
    TRef<IhullTypeIGC>  pht;
    CachedPartList      cpl;
};

typedef Slist_utl<CachedLoadout>   CachedLoadoutList;
typedef Slink_utl<CachedLoadout>   CachedLoadoutLink;

/////////////////////////////////////////////////////////////////////////////
// IClientEventSource

class IClientEventSource : public IObject 
{
public:
    virtual void AddSink(IClientEventSink* psink)    = 0;
    virtual void RemoveSink(IClientEventSink* psink) = 0;

    // mission events
    virtual void OnAddMission(MissionInfo* pMissionDef) = 0;
    virtual void OnDelMission(MissionInfo* pMissionDef) = 0;
    virtual void OnMissionCountdown(MissionInfo* pMissionDef) = 0;
    virtual void OnMissionStarted(MissionInfo* pMissionDef) = 0;
    virtual void OnMissionEnded(MissionInfo* pMissionDef) = 0;
    virtual void OnLockLobby(bool bLock) = 0;
    virtual void OnLockSides(bool bLock) = 0;
    virtual void OnFoundPlayer(MissionInfo* pMissionDef) = 0;
    virtual void OnEnterMission() = 0;
 
    // team events
    virtual void OnAddPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo) = 0;
    virtual void OnDelPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam = NULL) = 0;
    virtual void OnAddRequest(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo) = 0;
    virtual void OnDelRequest(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason) = 0;
    virtual void OnTeamInactive(MissionInfo* pMissionDef, SideID sideID) = 0;
    virtual void OnTeamReadyChange(MissionInfo* pMissionDef, SideID sideID, bool fTeamReady) = 0;
    virtual void OnTeamForceReadyChange(MissionInfo* pMissionDef, SideID sideID, bool fTeamForceReady) = 0;
    virtual void OnTeamAutoAcceptChange(MissionInfo* pMissionDef, SideID sideID, bool fAutoAccept) = 0;
    virtual void OnTeamCivChange(MissionInfo* pMissionDef, SideID sideID, CivID civID) = 0;
    virtual void OnTeamNameChange(MissionInfo* pMissionDef, SideID sideID) = 0;
	virtual void OnTeamAlliancesChange(MissionInfo* pMissionDef) = 0; // #ALLY

    // player events
    virtual void OnPlayerStatusChange(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo) = 0;
    virtual void OnMoneyChange(PlayerInfo* pPlayerInfo) = 0;

    // gameplay events
    virtual void OnBucketChange(BucketChange bc, IbucketIGC* b) = 0;
    virtual void OnTechTreeChanged(SideID sid) = 0;
    virtual void OnStationCaptured(StationID stationID, SideID sideID) = 0;
    virtual void OnModelTerminated(ImodelIGC* pmodel) = 0;
    virtual void OnLoadoutChanged(IpartIGC* ppart, LoadoutChange lc) = 0;
    virtual void OnPurchaseCompleted(bool bAllPartsBought) = 0;
    virtual void OnTurretStateChanging(bool bTurret) = 0;
    virtual void OnShipStatusChange(PlayerInfo* pplayer) = 0;
    virtual void OnBoardShip(IshipIGC* pshipChild, IshipIGC* pshipParent) = 0;
    virtual void OnBoardFailed(IshipIGC* pshipRequestedParent) = 0;
    virtual void OnDiscoveredStation(IstationIGC* pstation) = 0;
    virtual void OnDiscoveredAsteroid(IasteroidIGC* pasteroid) = 0;
    virtual void OnClusterChanged(IclusterIGC* pcluster) = 0;
    virtual void OnGameoverStats() = 0;
    virtual void OnGameoverPlayers() = 0;
 
    // chat events
    virtual void OnDeleteChatMessage(ChatInfo* pchatInfo) = 0;
    virtual void OnNewChatMessage() = 0;
    virtual void OnClearChat() = 0;
    virtual void OnChatMessageChange() = 0;

    // system events
    virtual void OnLostConnection(const char * szReason) = 0;
    virtual void OnEnterModalState() = 0;
    virtual void OnLeaveModalState() = 0;
    virtual void OnLogonClub() = 0;
    virtual void OnLogonClubFailed(bool bRetry, const char * szReason) = 0;
    virtual void OnLogonLobby() = 0;
    virtual void OnLogonLobbyFailed(bool bRetry, const char * szReason) = 0;
    virtual void OnLogonGameServer() = 0;
    virtual void OnLogonGameServerFailed(bool bRetry, const char * szReason) = 0;
	//KGJV #114 - callback for core and server lists
	virtual void OnServersList(int cCores, char *Cores, int cServers, char *Servers) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// IClientEventSink

class IClientEventSink : public IObject 
{
public:
    static TRef<IClientEventSink> CreateDelegate(IClientEventSink* psink);

    // mission events
    virtual void OnAddMission(MissionInfo* pMissionDef) {};
    virtual void OnDelMission(MissionInfo* pMissionDef) {};
    virtual void OnMissionCountdown(MissionInfo* pMissionDef) {};
    virtual void OnMissionStarted(MissionInfo* pMissionDef) {};
    virtual void OnMissionEnded(MissionInfo* pMissionDef) {};
    virtual void OnLockLobby(bool bLock) {};
    virtual void OnLockSides(bool bLock) {};
    virtual void OnFoundPlayer(MissionInfo* pMissionDef) {};
    virtual void OnEnterMission() {};

    // team events
    virtual void OnAddPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo) {};
    virtual void OnDelPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam = NULL) {};
    virtual void OnAddRequest(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo) {};
    virtual void OnDelRequest(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason) {};
    virtual void OnTeamInactive(MissionInfo* pMissionDef, SideID sideID) {};
    virtual void OnTeamReadyChange(MissionInfo* pMissionDef, SideID sideID, bool fTeamReady) {};
    virtual void OnTeamForceReadyChange(MissionInfo* pMissionDef, SideID sideID, bool fTeamForceReady) {};
    virtual void OnTeamAutoAcceptChange(MissionInfo* pMissionDef, SideID sideID, bool fAutoAccept) {};
    virtual void OnTeamCivChange(MissionInfo* pMissionDef, SideID sideID, CivID civID) {};
    virtual void OnTeamNameChange(MissionInfo* pMissionDef, SideID sideID) {};
	virtual void OnTeamAlliancesChange(MissionInfo* pMissionDef) {}; //#ALLY

    // player events
    virtual void OnPlayerStatusChange(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo) {};
    virtual void OnMoneyChange(PlayerInfo* pPlayerInfo) {};

    // gameplay events
    virtual void OnBucketChange(BucketChange bc, IbucketIGC* b){};
    virtual void OnTechTreeChanged(SideID sid){};
    virtual void OnStationCaptured(StationID stationID, SideID sideID) {};
    virtual void OnModelTerminated(ImodelIGC* pmodel) {};
    virtual void OnLoadoutChanged(IpartIGC* ppart, LoadoutChange lc) {};
    virtual void OnPurchaseCompleted(bool bAllPartsBought) {};
    virtual void OnTurretStateChanging(bool bTurret) {}
    virtual void OnShipStatusChange(PlayerInfo* pplayer) {};
    virtual void OnBoardShip(IshipIGC* pshipChild, IshipIGC* pshipParent) {};
    virtual void OnBoardFailed(IshipIGC* pshipRequestedParent) {};
    virtual void OnDiscoveredStation(IstationIGC* pstation) {};
    virtual void OnDiscoveredAsteroid(IasteroidIGC* pasteroid) {};
    virtual void OnClusterChanged(IclusterIGC* pcluster) {};
    virtual void OnGameoverStats() {};
    virtual void OnGameoverPlayers() {};

    // chat events
    virtual void OnDeleteChatMessage(ChatInfo* pchatInfo) {};
    virtual void OnNewChatMessage() {};
    virtual void OnClearChat() {};
    virtual void OnChatMessageChange() {};

    // system events
    virtual void OnLostConnection(const char * szReason) {};
    virtual void OnEnterModalState() {};
    virtual void OnLeaveModalState() {};
    virtual void OnLogonClub() {};
    virtual void OnLogonClubFailed(bool bRetry, const char * szReason) {};
    virtual void OnLogonLobby() {};
    virtual void OnLogonLobbyFailed(bool bRetry, const char * szReason) {};
    virtual void OnLogonGameServer() {};
    virtual void OnLogonGameServerFailed(bool bRetry, const char * szReason) {};
	//KGJV #114 - callback for core and server lists
	virtual void OnServersList(int cCores, char *Cores, int cServers, char *Servers) {} ;
};

/////////////////////////////////////////////////////////////////////////////
// ChatInfo

class  ChatInfo : public IObject
{
private:
    ZString             m_strMessage;
    Color               m_colorMessage;

    TRef<ImodelIGC>     m_pmodelTarget;
    CommandID           m_cidCommand;
    ChatTarget          m_ctRecipient;

    bool                m_bFromPlayer;
    bool                m_bFromObjectModel;
    bool                m_bFromLeader;

public:
    void            SetChat(ChatTarget       ctRecipient,
                            const ZString&   strText, 
                            CommandID        cid,
                            ImodelIGC*       pmodelTarget,
                            const Color&     color,
                            bool             bFromPlayer,
                            bool             bFromObjectModel,
                            bool             bFromLeader);

    ChatTarget      GetChatTarget(void) const       { return m_ctRecipient; }

    const ZString&  GetMessage(void) const          { return m_strMessage; }
    const Color&    GetColor(void) const            { return m_colorMessage; }

    CommandID       GetCommandID(void) const        { return m_cidCommand; }
    ImodelIGC*      GetTarget(void) const           { return m_pmodelTarget; }

    bool            IsFromPlayer(void) const        { return m_bFromPlayer; }
    bool            IsFromLeader(void) const        { return m_bFromLeader; }
    bool            IsFromObjectModel(void) const   { return m_bFromObjectModel; }

    void            ClearTarget(void);
};

typedef Slist_utl<ChatInfo>  ChatList;
typedef Slink_utl<ChatInfo>  ChatLink;


/////////////////////////////////////////////////////////////////////////////
// BallotInfo

class BallotInfo
{
    ZString m_strBallotText;
    BallotID m_ballotID;
    Time m_timeExpiration;

public:

    BallotInfo(const ZString& strBallotText, BallotID ballotID, Time timeExpiration)
        : m_strBallotText(strBallotText), m_ballotID(ballotID), m_timeExpiration(timeExpiration) {};


    const ZString&  GetBallotText() const           { return m_strBallotText; }
    Time            GetBallotExpirationTime() const { return m_timeExpiration; }
    BallotID        GetBallotID() const             { return m_ballotID; }
};

typedef TList<BallotInfo> BallotList;

/////////////////////////////////////////////////////////////////////////////
// PlayerInfo

class PlayerInfo : public IObject
{
    FMD_S_PLAYERINFO            m_fmPlayerInfo;

    TRef<IshipIGC>              m_pship;
    ImissionIGC*                m_pmission;

    ShipStatus                  m_shipStatus;
    PersistPlayerScoreObject*   m_vPersistPlayerScores;
    int                         m_cPersistPlayerScores;
    PersistPlayerScoreObject    m_persist; // default new score

    bool                        m_bMute;

	unsigned int				m_ping; // w0dk4 player-pings feature
	unsigned int				m_loss; // w0dk4 player-pings feature

public:
    PlayerInfo(void);
    ~PlayerInfo();

	// w0dk4 player-pings feature
	void GetConnectionData(unsigned int * ping,unsigned int * loss)
	{
		*ping = m_ping;
		*loss = m_loss;
	}
	void SetConnectionData(unsigned int ping, unsigned int loss)
	{
		m_ping = ping;
		m_loss = loss;
	}
	// end w0dk4


    IshipIGC*   GetShip(void) const
    {
        return m_pship;
    }
    void        SetShip(IshipIGC* pship)
    {
        m_pship = pship;

        if (pship)
            pship->SetPrivateData((DWORD)this);
    }

    void        SetMission(ImissionIGC* pmission)
    {
        m_pmission = pmission;
    }

    bool        GetMute(void) const
    {
        return m_bMute;
    }
    void        SetMute(bool    bMute)
    {
        m_bMute = bMute;
    }

    void        Set(FMD_S_PLAYERINFO* pfmPlayerInfo);

    void        Update(FMD_CS_PLAYER_READY* pfmPlayerReady) { m_fmPlayerInfo.fReady = pfmPlayerReady->fReady; }

    bool        IsMissionOwner(void) const          { return m_fmPlayerInfo.fMissionOwner; }
    void        SetMissionOwner(bool fMissionOwner) { m_fmPlayerInfo.fMissionOwner = fMissionOwner; }

    bool        IsTeamLeader(void) const            { return m_fmPlayerInfo.fTeamLeader; }
    void        SetTeamLeader(bool fTeamLeader)     { m_fmPlayerInfo.fTeamLeader = fTeamLeader; }

    bool        IsReady(void) const                 { return m_fmPlayerInfo.fReady; }
    void        SetReady(bool fReady)               { m_fmPlayerInfo.fReady = fReady; }
    bool        IsHuman(void) const                 { return m_fmPlayerInfo.dtidDrone == NA; }
    DroneTypeID GetDroneTypeID(void) const          { return m_fmPlayerInfo.dtidDrone; }

    short       MissionKills(void) const            { return m_pship->GetKills(); }
    short       MissionDeaths(void) const           { return m_pship->GetDeaths(); }
    short       MissionEjections(void) const        { return m_pship->GetEjections(); }
    
    void        AddDeath()                          {
                                                      m_pship->AddDeath();
                                                    }
    void        AddEjection()                       {
                                                      m_pship->AddEjection();
                                                    }
    void        AddKill()                           { 
                                                      m_pship->AddKill();
                                                    }
    RankID      Rank() const                        {
                                                      return GetPersistScore().GetRank();
                                                    }

    const PersistPlayerScoreObject& GetPersistScore() const { return GetPersistScore(GetCivID()); }
    const PersistPlayerScoreObject& GetPersistScore(CivID civId) const;
    void        UpdateScore(PersistPlayerScoreObject& ppso);

    CivID       GetCivID() const;
    SideID      SideID() const                      { return m_fmPlayerInfo.iSide; }
    void        SetSideID(::SideID sideID)          { m_fmPlayerInfo.iSide = sideID; };
    ShipID      ShipID() const                      { return m_fmPlayerInfo.shipID; }
    LPCSTR      CharacterName() const               { return m_fmPlayerInfo.CharacterName; }
    
    Money       GetMoney() const                    { return m_fmPlayerInfo.money; }
    void        SetMoney(Money money)               { m_fmPlayerInfo.money = money; }

    const ShipStatus& GetShipStatus(void) const             { return m_shipStatus; }
    void              SetShipStatus(const ShipStatus& ss)
    {
        ShipStatus  ssOld = m_shipStatus;
        m_shipStatus = ss;

        //If we were known and the sector is changing ... remove the threat/asset
        if ((ssOld.GetSectorID() != ss.GetSectorID()) ||
            ((ssOld.GetState() == c_ssFlying) != (ss.GetState() == c_ssFlying)) ||
            (ssOld.GetUnknown()  != ss.GetUnknown()))
        {
            if ((!ssOld.GetUnknown()) && (ssOld.GetSectorID() != NA))
            {
                IclusterIGC*    pcluster = m_pmission->GetCluster(ssOld.GetSectorID());
                assert (pcluster);
                if (pcluster)
                    pcluster->GetClusterSite()->MoveShip();
                else
                    assert(false);
            }

            if ((!ss.GetUnknown()) && (ss.GetSectorID() != NA))
            {
                IclusterIGC*    pcluster = m_pmission->GetCluster(ss.GetSectorID());
                assert (pcluster);
                if (pcluster)
                    pcluster->GetClusterSite()->MoveShip();
                else
                {
                    assert(false);
                    m_shipStatus.SetSectorID(NA);
                }
            }
        }
    }
    void              ResetShipStatus(void)
    {
        ShipStatus  ssOld = m_shipStatus;
        m_shipStatus.Reset();

        if ((!ssOld.GetUnknown()) && (ssOld.GetSectorID() != NA))
        {
            IclusterIGC*    pcluster = m_pmission->GetCluster(ssOld.GetSectorID());
            
            if (pcluster)
                pcluster->GetClusterSite()->MoveShip();
        }
    }
    void              Reset(bool bFull)
    {
        SetMoney(0);
        GetShip()->Reset(bFull);
        ResetShipStatus();
    }


    HullID      LastSeenShipType() const            { return m_shipStatus.GetHullID(); };
    SectorID    LastSeenSector() const              { return m_shipStatus.GetSectorID(); };
    StationID   LastSeenStation() const             { return m_shipStatus.GetStationID(); };
    ::ShipID    LastSeenParent() const              { return m_shipStatus.GetParentID(); };
    ShipState   LastSeenState() const               { return m_shipStatus.GetState(); };
    bool        StatusIsCurrent() const             { return !m_shipStatus.GetUnknown(); }
    bool        GetDetected() const                 { return !m_shipStatus.GetDetected(); }
	DWORD       LastStateChange() const             { return m_shipStatus.GetStateTime(); }; //#7 Imago 7/10
};

typedef Slist_utlListWrapper<PlayerInfo> PlayerList;
typedef Slist_utlListWrapper<PlayerInfo>::Link PlayerLink;

typedef TListListWrapper<IntItemIDWrapper<ShipID> > ShipList;

/////////////////////////////////////////////////////////////////////////////
// SideInfo

class SideInfo
{
protected:
    ShipList    m_listShipIDMembers;
    ShipList    m_listShipIDRequests;
    SideID      m_sideID;
    
public:
    SideInfo(SideID sideID)
        { m_sideID = sideID; }
    ~SideInfo(){}

    void        AddPlayer(ShipID shipID)    { m_listShipIDMembers.PushEnd(shipID); };
    void        RemovePlayer(ShipID shipID) { m_listShipIDMembers.Remove(shipID); };
    bool        FindPlayer(ShipID shipID)   { return m_listShipIDMembers.Find(shipID); };

    void        AddRequest(ShipID shipID)    { m_listShipIDRequests.PushEnd(shipID); };
    void        RemoveRequest(ShipID shipID) { m_listShipIDRequests.Remove(shipID); };
    bool        FindRequest(ShipID shipID)   { return m_listShipIDRequests.Find(shipID); };

    ShipList&   GetMembers() { return m_listShipIDMembers; }
    List*       GetMemberList() { return new ListDelegate(&m_listShipIDMembers); }
    ShipList&   GetRequests() { return m_listShipIDRequests; }
    List*       GetRequestList() { return new ListDelegate(&m_listShipIDRequests); }

    SideID      GetSideID() { return m_sideID; };
};

/////////////////////////////////////////////////////////////////////////////
// MissionInfo

class MissionInfo
{
protected: 
 
    FMD_S_MISSIONDEF* m_pfmMissionDef;
    TMapListWrapper<SideID, SideInfo*> m_mapSideInfo;
    SideInfo m_sideLobby;
    int m_nNumPlayers;
	int m_nNumNoatPlayers; //imago #169
    ZString m_strGameDetailsFiles;
    SquadID squadIDs[c_cSidesMax];
    bool m_fGuaranteedSlotsAvailable;
    bool m_fAnySlotsAvailable;
    bool m_fCountdownStarted;

public:
    MissionInfo(DWORD dwCookie);
    ~MissionInfo();

    // Operations
    void            Update(FMD_S_MISSIONDEF* pfmMissionDef);
    void            Update(FMD_LS_LOBBYMISSIONINFO* pfmLobbyMissionInfo);
    void            UpdateStartTime(Time timeStart);
    void            SetInProgress(bool fInProgress) { m_pfmMissionDef->fInProgress = fInProgress; }
    void            SetCountdownStarted(bool fCountdownStarted) { m_fCountdownStarted = fCountdownStarted; }
    STAGE           GetStage() { return m_pfmMissionDef->stage; }
    void            SetStage(STAGE stage) { m_pfmMissionDef->stage = stage; };
    void            AddPlayer(PlayerInfo* pPlayerInfo);
    void            RemovePlayer(PlayerInfo* pPlayerInfo);
    bool            FindPlayer(SideID sideID, ShipID shipID);
    void            AddRequest(SideID sideID, ShipID shipID);
    void            RemoveRequest(SideID sideID, ShipID shipID);
    bool            FindRequest(SideID sideID, ShipID shipID);
    bool            GetAnySlotsAreAvailable() { return m_fAnySlotsAvailable; };
    bool            GetGuaranteedSlotsAreAvailable() { return m_fGuaranteedSlotsAvailable; };
    void            SetLockLobby(bool bLock) { m_pfmMissionDef->misparms.bLockLobby = bLock; };
    bool            GetLockLobby() { return m_pfmMissionDef->misparms.bLockLobby; };
    void            SetLockSides(bool bLock) { m_pfmMissionDef->misparms.bLockSides = bLock; };
    bool            GetLockSides() { return m_pfmMissionDef->misparms.bLockSides; };
    
    // Mission Accessors
    SideID          NumSides()              { return m_pfmMissionDef->misparms.nTeams; }
    bool            InProgress()            { return !!m_pfmMissionDef->fInProgress; }
    bool            CountdownStarted()      { return m_fCountdownStarted; }
    LPCSTR          Name()                  { return m_pfmMissionDef->misparms.strGameName; }
    LPCSTR          Description()           { return m_pfmMissionDef->szDescription; }
    DWORD           GetCookie()             { return m_pfmMissionDef->dwCookie; }
    SideID          MissionOwnerSideID()    { return m_pfmMissionDef->iSideMissionOwner; }
    ShipID          MissionOwnerShipID()    { return SideLeaderShipID( MissionOwnerSideID() ); }
    bool            AutoAcceptLeaders()     { return m_pfmMissionDef->fAutoAcceptLeaders; }
    int             MaxPlayers()            { return m_pfmMissionDef->misparms.nTotalMaxPlayersPerGame; }
    int             NumPlayers()            { return m_nNumPlayers; }
	int             NumNoatPlayers()        { return m_nNumNoatPlayers; } //Imago #169
    int             AvailablePositions()    { return MaxPlayers() - NumPlayers(); }
    int             GuaranteedPositions();
    unsigned char   MinPlayersPerTeam()     { return m_pfmMissionDef->misparms.nMinPlayersPerTeam; }
    unsigned char   MaxPlayersPerTeam()     { return m_pfmMissionDef->misparms.nMaxPlayersPerTeam; }
    short           MaxImbalance()          { return m_pfmMissionDef->misparms.iMaxImbalance; }
    bool            ScoresCount()           { return m_pfmMissionDef->misparms.bScoresCount; }
    bool            GoalConquest()          { return m_pfmMissionDef->misparms.IsConquestGame(); }
    bool            GoalDeathMatch()        { return m_pfmMissionDef->misparms.IsDeathMatchGame(); }
    bool            GoalCountdown()         { return m_pfmMissionDef->misparms.IsCountdownGame(); }
    bool            GoalProsperity()        { return m_pfmMissionDef->misparms.IsProsperityGame(); }
    bool            GoalArtifacts()         { return m_pfmMissionDef->misparms.IsArtifactsGame(); }
    bool            GoalFlags()             { return m_pfmMissionDef->misparms.IsFlagsGame(); }
    bool            GoalTerritory()         { return m_pfmMissionDef->misparms.IsTerritoryGame(); }
    bool            WasObjectModelCreated() { return m_pfmMissionDef->misparms.bObjectModelCreated; }
    bool            AllowDevelopments()     { return m_pfmMissionDef->misparms.bAllowDevelopments; }
    bool            LimitedLives()          { return m_pfmMissionDef->misparms.iLives != 0x7fff; }
    Time            StartTime()             { assert(InProgress()); return m_pfmMissionDef->misparms.timeStart; }
    const MissionParams& GetMissionParams() { return m_pfmMissionDef->misparms; }
    const FMD_S_MISSIONDEF& GetMissionDef() { return *m_pfmMissionDef; }
    void            PurgePlayers();
    short           GetMinRank()            { return m_pfmMissionDef->misparms.iMinRank; };
    short           GetMaxRank()            { return m_pfmMissionDef->misparms.iMaxRank; };
    int             GetSlotsLeft()          { return m_pfmMissionDef->misparms.nMaxPlayersPerTeam
                                                    * m_pfmMissionDef->misparms.nTeams
                                                - m_nNumPlayers; };
    const ZString&  GetDetailsFiles()       { return m_strGameDetailsFiles; }

	// KGJV #62
	void			SetAllowEmptyTeams(bool bValue)	{ m_pfmMissionDef->misparms.bAllowEmptyTeams = bValue;}
	
	// IMAGO ALLY 7/5/09
	void			SetDefections(bool bValue)	{ m_pfmMissionDef->misparms.bAllowDefections = bValue;}
	void			SetAllowAlliedRip(bool bValue)	{ m_pfmMissionDef->misparms.bAllowAlliedRip = bValue;}
	void			SetAllowAlliedViz(bool bValue)	{ m_pfmMissionDef->misparms.bAllowAlliedViz = bValue;}
	void			SetMaxImbalance(short iMaxImbalance) { m_pfmMissionDef->misparms.iMaxImbalance = iMaxImbalance;} // 8/1/09
    
    // Team Accessors
    LPCSTR          SideName(SideID sideID)         { return (sideID == SIDE_TEAMLOBBY) ? "Not on a team" : m_pfmMissionDef->rgszName[sideID]; }
    //CivID           SideCivID(SideID sideID)        { return (sideID == SIDE_TEAMLOBBY) ? NA : m_pfmMissionDef->rgCivID[sideID]; }
    ShipID          SideLeaderShipID(SideID sideID) { return (sideID == SIDE_TEAMLOBBY) ? NA : m_pfmMissionDef->rgShipIDLeaders[sideID]; }
    bool            SideAutoAccept(SideID sideID)   { return (sideID == SIDE_TEAMLOBBY) ? true : !!m_pfmMissionDef->rgfAutoAccept[sideID]; }
    bool            SideReady(SideID sideID)        { return (sideID == SIDE_TEAMLOBBY) ? true : !!m_pfmMissionDef->rgfReady[sideID]; }
    bool            SideForceReady(SideID sideID)   { return (sideID == SIDE_TEAMLOBBY) ? true : !!m_pfmMissionDef->rgfForceReady[sideID]; }
    char            SideMaxPlayers(SideID sideID)   { return (sideID == SIDE_TEAMLOBBY) ? 1000 : m_pfmMissionDef->misparms.nMaxPlayersPerTeam; }
    char            SideNumPlayers(SideID sideID)   { return (sideID == SIDE_TEAMLOBBY) ? 0 : m_pfmMissionDef->rgcPlayers[sideID]; }
    int             SideAvailablePositions(SideID sideID) { return (sideID == SIDE_TEAMLOBBY) ? 1000 : (SideMaxPlayers(sideID) - SideNumPlayers(sideID)); }
    int             SideActive(SideID sideID)       { return (sideID == SIDE_TEAMLOBBY) ? true : (m_pfmMissionDef->rgfActive[sideID] != 0); }
    bool            HasSquad(SquadID squadID);

	char			SideAllies(SideID sideID)       { return (sideID == SIDE_TEAMLOBBY) ? NA : m_pfmMissionDef->rgfAllies[sideID]; } // #ALLY

    // Team Operations
	void			SetSideAllies(SideID sideID, char allies) { assert(sideID != SIDE_TEAMLOBBY); m_pfmMissionDef->rgfAllies[sideID] = allies; } // #ALLY
    void            SetSideActive(SideID sideID, bool fActive) 
                    { assert(sideID != SIDE_TEAMLOBBY); m_pfmMissionDef->rgfActive[sideID] = fActive; }
    void            SetSideAutoAccept(SideID sideID, bool fAutoAccept) 
                    { assert(sideID != SIDE_TEAMLOBBY); m_pfmMissionDef->rgfAutoAccept[sideID] = fAutoAccept; }
    void            SetSideReady(SideID sideID, bool fReady) 
                    { assert(sideID != SIDE_TEAMLOBBY); m_pfmMissionDef->rgfReady[sideID] = fReady; }
    void            SetSideForceReady(SideID sideID, bool fForceReady) 
                    { assert(sideID != SIDE_TEAMLOBBY); m_pfmMissionDef->rgfForceReady[sideID] = fForceReady; }
    void            SetSideLeader(PlayerInfo* pPlayerInfo);
    /*void            SetSideCivID(SideID sideID, CivID civID) 
                    { assert(sideID != SIDE_TEAMLOBBY); m_pfmMissionDef->rgCivID[sideID] = civID; 
                        GetSideInfo(sideID)->GetMembers().GetSink()(); m_mapSideInfo.GetSink()(); }*/
    void            SetSideName(SideID sideID, const char* szName)
                    { assert(sideID != SIDE_TEAMLOBBY); strcpy(m_pfmMissionDef->rgszName[sideID], szName); 
                        m_mapSideInfo.GetSink()(); }
    void            SetSideSquadID(SideID sideID, SquadID squadID)
                    { assert(sideID != SIDE_TEAMLOBBY); squadIDs[sideID] = squadID; m_mapSideInfo.GetSink()(); }

    SideInfo*       GetSideInfo(SideID sideID);
    List*           GetSideList();

    const char *    GetIGCStaticFile()
                    { return m_pfmMissionDef->misparms.szIGCStaticFile; }
    int             GetIGCStaticVer()
                    { return m_pfmMissionDef->misparms.verIGCcore;}
};




/////////////////////////////////////////////////////////////////////////////
// BucketStatusArray

typedef TVector<Money> MoneyVector;

class BucketStatusArray : public MoneyVector, public IObjectSingle 
{
};

class CfgInfo
{
private:
  ZString m_szConfigFile; //KGJV #114 - last config file name
public:
  void SetCfgFile(const char * szConfig); //KGJV #114
  // KGJV - pigs - added ctor to init some values in case Load is never called
  CfgInfo() :
	dwLobbyPort(2302),
	m_szConfigFile("") // KGJV: fix init value for LAN mode
  {
  }
  void Load(const char * szConfig);
  DWORD GetCfgProfileString(const char *c_szCfgApp,const char *c_szKeyName,const char *c_szDefaultValue,char *szStr,DWORD dwSize); // KGJV #114
  ZString strClubLobby;
  ZString strPublicLobby;
  ZString strClub;
  ZString strZAuth;
  ZString strClubMessageURL;
  ZString strPublicMessageURL;
  ZString strZoneEventsURL;
  ZString strZoneEventDetailsURL;
  ZString strTrainingURL;
  ZString strPassportUpdateURL;
  GUID    guidZoneAuth;
  int     crcFileList;
  int     nFilelistSize;
  ZString strFilelistSite;
  ZString strFilelistDirectory;
  int     crcClubMessageFile;
  int     crcPublicMessageFile;
  bool    bUsePassport;
  DWORD   dwLobbyPort;				// mdvalley: Port number to connect to lobby with
};

struct LANServerInfo : public IObject
{
    LANServerInfo(REFGUID guidSession_, const ZString& strGameName_, short nNumPlayers_, short nMaxPlayers_)
            : strGameName(strGameName_), nNumPlayers(nNumPlayers_), nMaxPlayers(nMaxPlayers_)
        { memcpy(&guidSession, &guidSession_, sizeof(GUID)); }
    GUID guidSession;
    ZString strGameName;
    short nNumPlayers;
    short nMaxPlayers;
};

/////////////////////////////////////////////////////////////////////////////
// BaseClient
class BaseClient : 
    public IIgcSite, 
    IFedMessagingSite
{
public:

    enum
    {
        lockdownDonating = 1,
        lockdownLoadout = 2,
        lockdownTeleporting = 4
    };
    typedef int LockdownCriteria;

private:
    PlayerInfo*             m_pPlayerInfo;
    IshipIGC*               m_ship;
    IclusterIGC*            m_viewCluster;
    bool                    m_bInGame;
    bool                    m_bWaitingForGameRestart;
    ZString                 m_strLockDownReason;
    bool                    m_bLaunchAfterDisembark;
    ShipID                  m_sidBoardAfterDisembark;
    StationID               m_sidTeleportAfterDisembark;
    TRef<ImodelIGC>         m_pmodelServerTarget;
    LockdownCriteria        m_lockdownCriteria;

    TRef<IshipIGC>          m_pshipLastSender;
    CfgInfo                 m_cfginfo;

    Money                   m_money;
    Money                   m_moneyLastRequest;

    RankInfo*               m_vRankInfo;
    short                   m_cRankInfo;

    StaticMapInfo*          m_vStaticMapInfo;
    short                   m_cStaticMapInfo;

    ZString                 m_strCDKey;

    BallotList              m_listBallots;

	// BT - STEAM
	HAuthTicket				m_hAuthTicketLobby = 0;
	HAuthTicket				m_hAuthTicketServer = 0;


public: //todo: make protected

    // messaging
    enum MessageType
    {
        c_mtNone,
        c_mtGuaranteed,
        c_mtNonGuaranteed
    };

    struct ConnectInfo // everything we need to connect and log on to the server
    {
    public:
      ConnectInfo() :
        pZoneTicket(NULL),
        cbZoneTicket(0)
      {
        ZeroMemory(&ftLastArtUpdate, sizeof(ftLastArtUpdate));
        ZeroMemory(szName, sizeof(szName));
      }
      FILETIME  ftLastArtUpdate;
      ZString   strServer;
      char      szName  [c_cbName];
	  DWORD     dwPort;				// mdvalley: The port number to connect to
      LPBYTE    pZoneTicket;
      CB        cbZoneTicket;
      GUID      guidSession;
	  int8		steamAuthTicket[1024]; // BT - STEAM
	  int32		steamAuthTicketLength; // BT - STEAM
    };
    
    bool                m_fLoggedOn   : 1;
    bool                m_fZoneClub   : 1;
    bool                m_fIsLobbied  : 1;
    bool                m_fLoggedOnToLobby   : 1;
    bool                m_fLoggedOnToClub    : 1; 
    bool                m_serverOffsetValidF : 1;
    bool                m_terminatedF        : 1;
    FedMessaging        m_fm;
    FedMessaging        m_fmLobby;
    FedMessaging        m_fmClub;
    ConnectInfo         m_ci; // we're just going to remember this from when they log into the lobby, and reuse it when they log into the game server
    DWORD               m_serverLag;
    DWORD               m_serverOffset;
    DWORD               m_lobbyServerOffset;
    Time                m_timeLastPing;
    Time                m_timeLastPingServer;
    Time                m_timeLastServerMessage;
    Time                m_lastSend;
    Time                m_lastLagCheck;
    int                 m_cUnansweredPings;
    float               m_sync;
    Cookie              m_cookie;
    MessageType         m_messageType;
    char                m_szCharName[c_cbName]; 
    char                m_szClubCharName[c_cbName]; // name specified when logging into Club server
    char                m_szLobbyCharName[c_cbName];// name specified when logging into Lobby server
    char                m_szIGCStaticFile[30];
    int                 m_nMemberID; // For Zone Club server

    // igc
    ImissionIGC*        m_pCoreIGC;

    // client state information
    MissionInfo*        m_pMissionInfo;
    DWORD               m_dwCookieToJoin;
    char                m_strPasswordToJoin[c_cbGamePassword];
    Mount               m_selectedWeapon;
    int                 m_oldStateM;
    ZString             m_strBriefingText;
    bool                m_bGenerateCivBriefing;
    //IclusterIGC*        m_cluster;
    //TRef<IsideIGC>      m_side;
    //ShipID              m_shipID;
    //StationID           m_stationID;
    bool                bInitTrekJoyStick; // moved from trekClient to here

    // cached time for all messages from a single FedMessaging::ReceiveMessages call
    Time                m_lastUpdate;
    Time                m_now;

    // tech advancement
    TMap<StationID, TRef<BucketStatusArray> >  m_mapBucketStatusArray;

    // missions
    TMapListWrapper<DWORD, MissionInfo*> m_mapMissions;
    TRef<ListDelegate> m_plistMissions;

    // players
    PlayerList                      m_listPlayers;

    // event notification
    TRef<IClientEventSource>        m_pClientEventSource;

    // chat 
    ChatList                        m_chatList;
    ChatLink*                       m_plinkSelectedChat;
    TRef<IchaffIGC>                 m_pchaffLastCreated;

    CachedLoadoutList               m_loadouts;

	//AaronMoore 1/10
    // An array (0-9) of Loadout Lists, each containing one or more Hull Types
    static const int                MAX_CUSTOM_LOADOUTS = 10;
    CachedLoadoutList *             m_customLoadouts;

    IAutoDownload *                 m_pAutoDownload;

#ifdef USEAUTH
    TRef<IZoneAuthClient>           m_pzac;
#endif

    TList<TRef<LANServerInfo> >*    m_plistFindServerResults;

public:

    BaseClient();
    virtual ~BaseClient();
    virtual void Initialize(Time timeNow);
    virtual void Reinitialize(Time timeNow);
    virtual void Terminate();
    virtual void FlushGameState();
    virtual void CreateMissionReq();
	virtual void ServerListReq(); // KGJV #114
	virtual void CreateMissionReq(const char *szServer, const char *szAddr, const char *szIGCStaticFile, const char *szGameName); // KGJV #114
    virtual void JoinMission(MissionInfo * pMission, const char* szMissionPassword);

    // AutoDownload functions
    virtual IAutoUpdateSink * OnBeginAutoUpdate(IAutoUpdateSink * pSink, bool bConnectToLobby) { return NULL; }  
    virtual bool ShouldCheckFiles() { return false; }
    void         HandleAutoDownload(DWORD dwTimeAlloted); 

    virtual const char* GetArtPath() { return NULL; } // This should be overridden in Wintrek/PIGS
    virtual bool ResetStaticData(const char * szIGCStaticFile, ImissionIGC** ppStaticIGC, Time tNow, bool bEncrypt);

    virtual Time ServerTimeFromClientTime(Time   timeClient)
    {
        ShouldBe(!GetNetwork() || m_serverOffsetValidF);
        return Time(timeClient.clock() + m_serverOffset);
    }
    virtual Time ClientTimeFromServerTime(Time   timeServer)
    {
        ShouldBe(!GetNetwork() || m_serverOffsetValidF);
        return Time(timeServer.clock() - m_serverOffset);
    }

    // messaging
    virtual HRESULT     ConnectToLobby(ConnectInfo * pci); // pci is NULL if relogging in
    virtual HRESULT     ConnectToServer(ConnectInfo & ci, DWORD dwCookie, Time now, const char* szPassword, bool bStandalonePrivate);
    virtual HRESULT     ConnectToClub(ConnectInfo * pci);
    virtual void        FindStandaloneServersByName(const char* szName, TList<TRef<LANServerInfo> >& listResults);
    bool                LoggedOn()                  
    {
      return m_fLoggedOn; 
    }
    bool                LoggedOnToLobby()                  
    {
      return m_fLoggedOnToLobby; 
    }
    bool                LoggedOnToClub()                  
    {
      return m_fLoggedOnToClub; 
    }
    virtual void      OverrideCamera(ImodelIGC*    pmodel) {}
    const ZString&      GetCDKey()
    {
        return m_strCDKey;
    }
    virtual void        SetCDKey(const ZString& strCDKey);

	// BT - STEAM
	void UpdateLobbyLoginRequestWithSteamAuthTokenInformation(FMD_C_LOGON_LOBBY *pfmLogon);
	void UpdateServerLoginRequestWithSteamAuthTokenInformation(FMD_C_LOGONREQ *pfmLogon);
	void CancelSteamAuthSessionToGameServer();
	void CancelSteamAuthSessionToLobby();

    virtual void        OnLogonAck(bool fValidated, bool bRetry, LPCSTR szFailureReason) = 0;
    virtual void        OnLogonLobbyAck(bool fValidated, bool bRetry, LPCSTR szFailureReason) = 0;
    virtual void        OnLogonClubAck(bool fValidated, bool bRetry, LPCSTR szFailureReason) {};
    virtual void        Disconnect();
    virtual void        DisconnectLobby();
    virtual void        DisconnectClub();
    void                SetCookie(Cookie cookie)    
    {
      m_cookie = cookie;
    }
    Cookie              GetCookie()                 
    {
      return m_cookie;
    }
    void                SetMessageType(MessageType mt);
    void                SendMessages();
    void                SendLobbyMessages()
    {
      m_fmLobby.SendMessages(m_fmLobby.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
    }
    void                SendClubMessages()
    {
      m_fmClub.SendMessages(m_fmClub.GetServerConnection(), FM_GUARANTEED, FM_FLUSH);
    }
    virtual HRESULT     ReceiveMessages(void);
    virtual HRESULT     OnSessionLost(const char *szReason, FedMessaging * pthis);
    virtual void        OnSessionFound(FedMessaging * pthis, FMSessionDesc * pSessionDesc);
    virtual HRESULT     HandleMsg(FEDMESSAGE* pfm, Time lastUpdate, Time now);
    bool                GetIsZoneClub()
    {
      return m_fZoneClub;
    }
    void                SetIsZoneClub(bool fzc)
    {
      m_fZoneClub = fzc;
    }
    bool                GetIsLobbied()
    {
      return m_fIsLobbied;
    }
    void                SetIsLobbied(bool fzc)
    {
      m_fIsLobbied = fzc;
    }
#ifdef USEAUTH
    TRef<IZoneAuthClient> GetZoneAuthClient()
    {
      return m_pzac;
    }
    TRef<IZoneAuthClient> CreateZoneAuthClient()
    {
      m_pzac = ::CreateZoneAuthClient();
      return m_pzac;
    }
    void                FreeZoneAuthClient()
    {
      m_pzac = NULL;
    }
#endif    
    //HRESULT     HandleShipUpdate(Time timeUpdate, const LightShipUpdate& shipupdate);
    //HRESULT     HandleShipUpdate(Time timeUpdate, const HeavyShipUpdate& shipupdate);

    // FedMessagingSite
    virtual HRESULT     OnSessionLost(FedMessaging * pthis);
    virtual void        OnMessageNAK(FedMessaging * pthis, DWORD dwTime, CFMRecipient * prcp);
    
    // game state
    inline ImissionIGC* GetCore()               { return m_pCoreIGC; }
    PlayerInfo*         MyPlayerInfo()          { return m_pPlayerInfo; }
    MissionInfo*        MyMission()             { return m_pMissionInfo; }
    bool                MyMissionInProgress();

    inline bool         IsInGame(void) const    { return m_bInGame; };
    inline bool         IsWaitingForGameRestart(void) const { return m_bWaitingForGameRestart; };
    const ZString&      GetBriefingText(void) const { return m_strBriefingText; }
    inline bool         GenerateCivBriefing(void) const { return m_bGenerateCivBriefing; }
    inline IsideIGC*    GetSide(void) const     { return m_ship->GetSide(); }
    inline SideID       GetSideID(void) const   { IsideIGC* s = GetSide(); return s ? s->GetObjectID() : NA; }
    TList<SquadMembership>& GetSquadMemberships() { return m_squadmemberships; } 
    virtual void        SaveSquadMemberships(const char* szCharacterName);
    virtual void        RestoreSquadMemberships(const char* szCharacterName);
    bool                HasPlayerSquad(MissionInfo* pMission);

    void                SetPlayerInfo(PlayerInfo*   ppinfo);
    inline PlayerInfo*  GetPlayerInfo(void) const { return m_pPlayerInfo; }
    PlayerList *        GetPlayerList() { return(&m_listPlayers); }
    List*               PlayerList() { return new ListDelegate(&m_listPlayers); }

    void                SetMoney(Money m);
    inline Money        GetMoney() const        { return m_pPlayerInfo ? m_pPlayerInfo->GetMoney() : 0; }

    inline ShipID       GetShipID() const       { return m_ship->GetObjectID(); }

    inline IshipIGC*    GetShip() const         { return m_ship; }

    inline IclusterIGC* GetCluster() const      {assert (m_ship); return m_viewCluster ? m_viewCluster : (m_ship ? m_ship->GetCluster() : NULL); }

    IclusterIGC*        GetChatCluster() const
    {
        IclusterIGC*  pcluster = GetCluster();
        if (!pcluster)
        {
            IstationIGC*    pstation = m_ship->GetStation();
            assert (pstation);
            if (pstation)
                pcluster = pstation->GetCluster();
        }

        assert (pcluster);
        return pcluster;
    }

    void                CreateBuildingEffect(Time now, IasteroidIGC* pasteroid, IshipIGC* pshipBuilder)
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
    }

    IclusterIGC*        GetCluster(IshipIGC* pship, ImodelIGC* pmodel)
    {
        IclusterIGC*    pcluster = pmodel->GetCluster();

        if (pcluster == NULL)
        {
            if (pmodel->GetObjectType() == OT_ship)
            {
                PlayerInfo* ppi = (PlayerInfo*)(((IshipIGC*)pmodel)->GetPrivateData());
                if (ppi->StatusIsCurrent())
                    pcluster = m_pCoreIGC->GetCluster(ppi->LastSeenSector());
            }
        }
        else if (!pship->CanSee(pmodel))
            pcluster = NULL;

        return pcluster;
    }

    IclusterIGC*        GetRipcordCluster(IshipIGC* pship, HullAbilityBitMask habm)
    {
        PlayerInfo* ppi = (PlayerInfo*)(pship->GetPrivateData());
        if (ppi->StatusIsCurrent() && (ppi->LastSeenState() == c_ssFlying))
        {
            SectorID    sid = ppi->LastSeenSector();
            if (sid != NA)
            {
                IhullTypeIGC*   pht = m_pCoreIGC->GetHullType(ppi->LastSeenShipType());
                if (pht && pht->HasCapability(habm))
                {
                    IclusterIGC*    pcluster = pship->GetCluster();
                    return pcluster
                           ? pcluster
                           : m_pCoreIGC->GetCluster(ppi->LastSeenSector());
                }
            }                    
        }

        return NULL;
    }

    void    SetWing(WingID  wid, IshipIGC*  pship = NULL)
    {
        if (pship == NULL)
            pship = m_ship;
        pship->SetWingID(wid);

        if (m_fm.IsConnected())
        {
            SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(m_fm, pfmSetWingID, CS, SET_WINGID)
            END_PFM_CREATE

            pfmSetWingID->shipID = pship->GetObjectID();
            pfmSetWingID->wingID = wid;
        }
    }

    void RequestRipcord(IshipIGC*   pship, IclusterIGC*  pcluster)
    {
        if (pship == m_ship)
        {
            if (m_fm.IsConnected())
            {
                SetMessageType(BaseClient::c_mtGuaranteed);
                BEGIN_PFM_CREATE(m_fm, pfmRequest, C, RIPCORD_REQUEST)
                    END_PFM_CREATE;

                pfmRequest->sidRipcord = pcluster ? pcluster->GetObjectID() : NA;
            }
            else
                RipcordLocal(pship, pcluster);
        }
    }

    void RipcordLocal(IshipIGC*   pship, IclusterIGC*  pcluster)
    {
        if (pcluster == nullptr)
            debugf("RipcordLocal destination cluster id: NULL\n");
        else
            debugf("RipcordLocal destination cluster id: %d\n", pcluster->GetObjectID());

        ImodelIGC*      pmodelRipcordOld = pship->GetRipcordModel();
        bool            player = true;

        if (pcluster == NULL)
        {
            if (pmodelRipcordOld != NULL)
            {
                assert(pship->GetCluster());

                pship->SetRipcordModel(NULL);
                pship->SetStateBits(droneRipMaskIGC, 0);

                if (player)
                    PlayNotificationSound(salRipcordAbortedSound, pship);

                // clear the ripcord effect
                pship->GetThingSite()->SetTimeUntilRipcord(-1.0f);
            }
        }
        else if (pmodelRipcordOld == NULL ||
                pmodelRipcordOld->GetCluster() != pcluster)
        {
            ImodelIGC*      pmodelRipcordDest = pship->FindRipcordModel(pcluster);
            if (pmodelRipcordDest != NULL)
            {

                IclusterIGC*    pclusterRipcord = pmodelRipcordDest->GetCluster();

                if (pclusterRipcord == NULL)
                {
                    assert(pmodelRipcordDest->GetObjectType() == OT_ship);

                    PlayerInfo* ppi = (PlayerInfo*)(((IshipIGC*)pmodelRipcordDest)->GetPrivateData());

                    assert(ppi->StatusIsCurrent());
                    pclusterRipcord = pship->GetMission()->GetCluster(ppi->LastSeenSector());
                    assert(pclusterRipcord);
                }

                if (player)
                {
                    const char*     name = pclusterRipcord->GetName();
                    char    bfr[100];
                    if (pclusterRipcord != pcluster)
                        sprintf(bfr, "Ripcording to %s, which is closest to %s",
                            name, pcluster->GetName());
                    else
                        sprintf(bfr, "Ripcording to %s", name);
                    PostPlainText(true, bfr);
                }

                if (pmodelRipcordDest != pship->GetRipcordModel())
                {
                    pship->SetRipcordModel(pmodelRipcordDest);
                    pship->ResetRipcordTimeLeft();
                }

                // set up the ripcord effect
                pship->GetThingSite()->SetTimeUntilRipcord(pship->GetRipcordTimeLeft());
            }
            else if (player)
            {
                PlayNotificationSound(salNoRipcordSound, m_ship); //RIPCORD_DENIED
                pship->SetRipcordModel(NULL);
            }
        }
    }

    IclusterIGC* GetViewCluster() const         { return m_viewCluster; }
    
    virtual void SetViewCluster(IclusterIGC* pcluster, const Vector*  pposition = NULL);
    virtual void RequestViewCluster(IclusterIGC* pcluster, ImodelIGC* pmodelTarget = NULL) { SetViewCluster(pcluster); }

    virtual void            ResetShip(void);
    //virtual void            ResetClusterScanners(IsideIGC*  pside);

    virtual void            BuyLoadout(IshipIGC* pshipLoadout, bool bLaunch);
    virtual IshipIGC*       CreateEmptyShip(ShipID sid = -2);
    virtual IshipIGC*       CopyCurrentShip(void);

    virtual void        PreviousWeapon();
    virtual bool        SendUpdate(Time now);
    IweaponIGC*         GetWeapon();
    virtual void        NextWeapon();
    virtual void        SetSelectedWeapon(Mount id);
    bool                flyingF(void) const 
    { 
        return 
               (m_ship != NULL)
            && (m_ship->GetCluster() != NULL); 
    }
        
    bool                autoPilot()             { return m_ship->GetAutopilot(); }
    void                SetAutoPilot(bool autoPilot);
    void                CheckServerLag(Time now);

    ChatList*           GetChatList(void) { return &m_chatList; }

    // Helpers
    MissionInfo* GetLobbyMission(DWORD dwCookie);
    PlayerLink*  FindPlayerLink(ShipID shipID);
    PlayerInfo*  FindAndCreatePlayerLink (ShipID shipID);
    PlayerInfo*  FindPlayer(ShipID shipID);
    PlayerInfo*  FindPlayer(const char* szName);
    PlayerInfo*  FindPlayerByPrefix(const char* szNamePrefix);
    ZString      LookupRankName(RankID rank, CivID civ = -1);
    const StaticMapInfo& GetStaticMapInfo(int index) { assert(index >= 0 && index < m_cStaticMapInfo); return m_vStaticMapInfo[index]; };
    int          GetNumStaticMaps() { return m_cStaticMapInfo; };

    List*        GetMissionList();
    void         QuitMission();

    Money GetBucketStatus(StationID stationID, short iBucket);

    Money AddMoneyToBucket(IbucketIGC* b, Money m);
    void DonateMoney(PlayerInfo* pPlayerInfo, Money money);
    FedMessaging* GetNetwork() 
    { 
      return &m_fm; 
    }
    FedMessaging* GetFMLobby() 
    { 
      return &m_fmLobby;
    }

    // Chat message handling
    bool         ParseShellCommand(const char* pszCommand);

    virtual void SendChat(IshipIGC*      pshipSender,
                          ChatTarget     ctRecipient,
                          ObjectID       oidRecipient,
                          SoundID        soundID,
                          const char*    pszText,
                          CommandID      cid = c_cidNone,
                          ObjectType     otTarget = NA,
                          ObjectID       oidTarget = NA,
                          ImodelIGC*     pmodelTarget = NULL,
                          bool           bObjectModel = false);

    virtual void  ReceiveChat(IshipIGC*   pshipSender,
                          ChatTarget  ctRecipient,
                          ObjectID    oidRecipient,
                          SoundID     voiceOver,
                          const char* szText,
                          CommandID   cid,
                          ObjectType  otTarget,
                          ObjectID    oidTarget,
                          ImodelIGC*  pmodelTarget = NULL,
                          bool        bObjectModel = false);

    void    ScrollChatUp(void);
    void    ScrollChatDown(void);
    ChatInfo* GetCurrentMessage(void);

    BallotInfo* GetCurrentBallot();
    void    Vote(bool bAgree);
    void    SkipCurrentBallot();

    // event notification
    IClientEventSource*     GetClientEventSource()  { return m_pClientEventSource; }

    void SendAllMissions(IClientEventSink* pSink);
    void SendAllPlayers(IClientEventSink* pSink, MissionInfo* pMissionInfo, SideID sideID);
    void SendAllRequests(IClientEventSink* pSink, MissionInfo* pMissionInfo, SideID sideID);
    void SendAllBucketStatus(IClientEventSink* pSink, StationID stationID);

    //
    // IGC Site Implementation
    //
    virtual void TerminateModelEvent(ImodelIGC* model);
    virtual void KillAsteroidEvent(IasteroidIGC* pasteroid, bool explodeF);
    virtual void KillProbeEvent(IprobeIGC* pprobe);
    virtual void KillMineEvent(ImineIGC*    pmine);
    virtual void KillMissileEvent(ImissileIGC*    pmissile, const Vector& position);
    virtual void KillTreasureEvent(ItreasureIGC* ptreasure);
    virtual void KillShipEvent(Time now, IshipIGC* ship, ImodelIGC* launcher, float amount, const Vector& p1, const Vector& p2);
    virtual void DamageStationEvent(IstationIGC* station, ImodelIGC* launcher, DamageTypeID type, float amount);
    virtual void FireMissile(IshipIGC* pship, ImagazineIGC* pmagazine,
                    Time timeFired, ImodelIGC* pTarget, float lock);
    virtual void FireExpendable(IshipIGC* pShip,
                          IdispenserIGC* pDispenser,
                          Time timeFired);
    virtual void BucketChangeEvent(BucketChange bc, IbucketIGC* b);
    virtual void SideBuildingTechChange(IsideIGC* s);
    virtual void SideDevelopmentTechChange(IsideIGC* s);
    virtual void StationTypeChange(IstationIGC* s);
    virtual void LoadoutChangeEvent(IshipIGC* pship, IpartIGC* ppart, LoadoutChange lc);
    ItreasureIGC*  CreateTreasureLocal(Time now, IshipIGC* pship, IpartIGC* p, IpartTypeIGC* ppt, const Vector& position, float dv, float lifespan = 600.0f);
    ItreasureIGC*  CreateTreasureLocal(Time now, IshipIGC* pship, short amount, IpartTypeIGC* ppt, const Vector& position, float dv, float lifespan = 600.0f);

    virtual bool Reload(IshipIGC* pship, IlauncherIGC* plauncher, EquipmentType type);

    static short    BuyPartOnBudget(IshipIGC*       pship,
                                    IpartTypeIGC*   ppt,
                                    Mount           mount,
                                    Money*          pbudget)
    {
        Money   price = ppt->GetPrice();

        short   amount;
        if (price != 0)
        {
            if (IlauncherTypeIGC::IsLauncherType(ppt->GetEquipmentType()))
            {
                amount = ppt->GetAmount(pship);
                if (price * amount > *pbudget)
                {
                    assert (price > 0);
                    amount = *pbudget / price;
                }

                price *= amount;
            }
            else
                amount = (*pbudget >= price) ? 0x7fff : 0;
        }
        else
            amount = 0x7fff;

        if (amount != 0)
        {
            *pbudget -= price;
            pship->CreateAndAddPart(ppt, mount, amount);
        }

        return amount;
    }

    static void     TryToBuyParts(IshipIGC*              pship,
                                  IstationIGC*           pstation, 
                                  Money*                 pbudget,
                                  IhullTypeIGC*          pht,
                                  const PartTypeListIGC* ppartsShip)
    {
        assert (pship);
        assert (pstation);
        assert (*pbudget >= 0);
        assert (pship->GetBaseHullType() == pht);

        for (PartTypeLinkIGC*   ptl = ppartsShip->first();
             (ptl != NULL);
             ptl = ptl->next())
        {
            IpartTypeIGC*   ppt = ptl->data();
            if (pstation->CanBuy(ppt))
            {
                ppt = (IpartTypeIGC*)(pstation->GetSuccessor(ppt));
                assert (pstation->CanBuy(ppt));

                //We can buy this part ... try mounting it somewhere
                EquipmentType   et = ppt->GetEquipmentType();
                Mount           iMountMax = (et == ET_Weapon)
                    ? pht->GetMaxWeapons()
                    : 1;

                for (Mount i = 0; (i < iMountMax); i++)
                {
                    //If the slot is empty and we can mount the part in the slot
                    if ((pship->GetMountedPart(et, i) == NULL) && pht->CanMount(ppt, i))
                    {
                        if (BuyPartOnBudget(pship, ppt, i, pbudget) == 0)
                            break;      //Ran out of money ... don't try to mount any more instances of this part
                    }
                }
            }
        }
    }

    void    ClearLoadout(void)
    {
        m_loadouts.purge();
        m_bLaunchAfterDisembark = false;
        m_sidBoardAfterDisembark = NA;
        m_sidTeleportAfterDisembark = NA;
    }

    void    DisembarkAndLaunch()
    {
        m_bLaunchAfterDisembark = true;
        BoardShip(NULL);
    }

    void    DisembarkAndBuy(IshipIGC* pship, bool fLaunch)
    {
        CachedLoadoutLink*  pcll = SaveLoadout(pship);

        //Hack ... make sure this is first in the list of loadouts (so it is picked when
        //we need a default configuration)
        m_loadouts.first(pcll);

        m_bLaunchAfterDisembark = fLaunch;
        BoardShip(NULL);
    }

    void    DisembarkAndBoard(IshipIGC* pship)
    {
        m_sidBoardAfterDisembark = pship->GetObjectID();
        BoardShip(NULL);
    }

    void    DisembarkAndTeleport(IstationIGC* pstation)
    {
        m_sidTeleportAfterDisembark = pstation->GetObjectID();
        BoardShip(NULL);
    }

    void    SaveLoadout(void)
    {
        SaveLoadout(m_ship);
    }

	//AaronMoore 1/10
    CachedLoadoutLink*  SaveCustomLoadout(IshipIGC* pship, int num)
    {
        assert(num >= 0 && num < MAX_CUSTOM_LOADOUTS);
        return SaveLoadout(pship, m_customLoadouts[num]);
    }

    CachedLoadoutLink*  SaveLoadout(IshipIGC* pship)
    {
        return SaveLoadout(pship, m_loadouts); //AaronMoore 1/10
    }

	//AaronMoore 1/10
    CachedLoadoutLink*  SaveLoadout(IshipIGC* pship, CachedLoadoutList & loadoutList)
    {
        IhullTypeIGC*   pht = pship->GetBaseHullType();

        CachedLoadoutLink*  pcll = GetLoadoutLink(pht, loadoutList);
        if (pcll)
        {
            pcll->data().cpl.purge();
            pcll->unlink();
        }
        else
        {
            pcll = new CachedLoadoutLink;

            pcll->data().pht = pht;
        }

        assert (pcll);
        loadoutList.first(pcll);

        {
            for (PartLinkIGC*   ppl = pship->GetParts()->first(); (ppl != NULL); ppl = ppl->next())
            {
                CachedPartLink* cpl = new CachedPartLink;
                cpl->data().ppt = ppl->data()->GetPartType();
                cpl->data().mount = ppl->data()->GetMountID();

                //Keep the parts sorted by mount
                CachedPartLink* cplAfter;
                for (cplAfter = pcll->data().cpl.first(); ((cplAfter != NULL) &&
                                                           (cplAfter->data().mount < cpl->data().mount)); cplAfter = cplAfter->next())
                {
                }
                if (cplAfter != NULL)
                    cplAfter->txen(cpl);
                else
                    pcll->data().cpl.last(cpl);
            }
        }

        return pcll;
    }

    Money    RestoreLoadout(IshipIGC*       pshipSource,
                            IshipIGC*       pshipSink,
                            IstationIGC*    pstation,
                            Money           budget)
    {
        assert (pshipSink->GetBaseHullType() == NULL);
        assert (pshipSink->GetParts()->n() == 0);
        assert (pshipSink->GetChildShips()->n() == 0);

        CachedLoadoutLink*  pcll = m_loadouts.first();
        IhullTypeIGC*       phtBase = pshipSource->GetBaseHullType();
        if (pcll && (pcll->data().pht == phtBase))
        {
            IhullTypeIGC*   pht = pcll->data().pht;

            //We have the same hull as the saved loaded ... try to preserve as much
            //of the old loadout as possible
            const PartListIGC*  pparts = pshipSource->GetParts();

            //Upgrade the hull if possible
            IhullTypeIGC*   phtSuccessor = (pstation->CanBuy(pht))
                                           ? (IhullTypeIGC*)(pstation->GetSuccessor(pht))
                                           : pht;

            assert (pht->GetPrice() == phtSuccessor->GetPrice());
            budget -= phtSuccessor->GetPrice();
            pshipSink->SetBaseHullType(phtSuccessor);

            {
                //Mark all the parts in the cached loadout as not duplicated
                for (CachedPartLink*  l = pcll->data().cpl.first(); (l != NULL); l = l->next())
                    l->data().bDuplicated = false;
            }

            //Copy over the parts that were mounted on the old hull
            {
                for (PartLinkIGC*  l = pparts->first(); (l != NULL); l = l->next())
                {
                    IpartIGC*       ppart = l->data();
                    Mount           mount = ppart->GetMountID();
                    if (mount >= 0)
                    {
                        IpartTypeIGC*   ppt = ppart->GetPartType();

                        if (phtSuccessor->CanMount(ppt, mount))
                        {
                            if (pstation->CanBuy(ppt))
                                BuyPartOnBudget(pshipSink, (IpartTypeIGC*)(pstation->GetSuccessor(ppt)), mount, &budget);
                            else
                            {
                                //Re-use the old part
                                Money   price = ppt->GetPrice();

                                short   amount = ppart->GetAmount();
                                if (price != 0)
                                {
                                    if (IlauncherTypeIGC::IsLauncherType(ppt->GetEquipmentType()))
                                    {
                                        if (price * amount > budget)
                                        {
                                            assert (price > 0);
                                            amount = budget / price;
                                        }

                                        price *= amount;
                                    }
                                    else if (budget >= price)
                                        amount = 0;
                                }

                                if (amount != 0)
                                {
                                    budget -= price;
                                    pshipSink->CreateAndAddPart(ppt, mount, amount);
                                }
                            }

                            //Mark the corresponding part in the cached loadout as having been duplicated
                            {
                                bool found = false;
                                CachedPartLink* lWrongMountMatch = NULL;
                                for (CachedPartLink* l = pcll->data().cpl.last(); (l != NULL); l = l->txen()) // mount 2,1,0,0,..,-1,-2,.. with previously sorted list
                                {
                                    if ((l->data().ppt == ppt) &&
                                        !l->data().bDuplicated)
                                    {
                                        if (l->data().mount == mount) //otherwise first slots get set as duplicated regardless of which are empty
                                        {  
                                            l->data().bDuplicated = true;
                                            found = true;
                                            break;
                                        }
                                        else
                                            lWrongMountMatch = l;
                                    }
                                }
                                if (!found && lWrongMountMatch)
                                    lWrongMountMatch->data().bDuplicated = true;
                            }
                        }
                        else
                            debugf("**** successor unable to mount %s/%s\n", phtSuccessor->GetName(), ppt->GetName());
                    }
                }
            }

            //Fill all of the cargo slots with what was there before. Also fill module slots which were empty.
            Mount   cargo = -c_maxCargo;
            for (CachedPartLink*  l = pcll->data().cpl.first(); (l != NULL); l = l->next())
            {
                CachedPart& cpl = l->data();

                if (!cpl.bDuplicated)
                {
                    IpartTypeIGC*   ppt;
                    if (pstation->CanBuy(cpl.ppt))
                    {
                        //We have an empty slot and a part we can buy ... now see if there is an instance
                        //of this part in the available instance of this part in the new loadout
                        ppt = (IpartTypeIGC*)(pstation->GetSuccessor(cpl.ppt));
                    }
                    else
                        ppt = pstation->GetSimilarPart(cpl.ppt);

                    if (ppt)
                    {
                        EquipmentType   et =  ppt->GetEquipmentType();
                        
                        if ((cpl.mount >= 0) && //anything not cargo
                            (pshipSink->GetMountedPart(et, cpl.mount) == NULL) &&
                            (phtSuccessor->CanMount(ppt, cpl.mount)))
                        {
                            debugf("RestoreLoadout: Restoring empty module slot %d, type %d with %d \n", cpl.mount, et, ppt->GetObjectID());
                            BuyPartOnBudget(pshipSink, ppt, cpl.mount, &budget);
                        }
                        else if (cargo < 0)
                        {
                            BuyPartOnBudget(pshipSink, ppt, cargo++, &budget);
                        }

                    }
                }
            }

            if (pht != phtSuccessor)
            {
                //Fill any remaining slots with the default items
                TryToBuyParts(pshipSink, pstation, &budget, phtSuccessor, phtSuccessor->GetPreferredPartTypes());
            }
        }
        else if (phtBase && !phtBase->HasCapability(c_habmLifepod))
            budget = ReplaceLoadout(pshipSink, pstation, phtBase, budget, m_customLoadouts[0]); //AaronMoore 1/10 #146 Imago
        else
            budget = ReplaceLoadout(pshipSink, pstation, (CachedLoadout*)NULL, budget, m_loadouts); //AaronMoore 1/10

        return budget;
    }

    void RestoreLoadoutOnCarrier(IshipIGC* pship) {
        assert(!m_fm.IsConnected());
        assert(pship);
        debugf("RestoreLoadoutOnCarrier for %s", pship->GetName());

        // check all loadouts for an applicable one -- problematic for different tech levels!
        CachedLoadoutLink*  pcll = m_loadouts.first();
        while (pcll && pcll->data().pht != pship->GetBaseHullType())
            pcll = pcll->next();

        if (pcll) {
            debugf(" Saved loadout (%d) for \"%s\"\n", m_loadouts.n(), pship->GetBaseHullType()->GetName());

            {
                //Mark all the parts in the cached loadout as not duplicated
                for (CachedPartLink* l = pcll->data().cpl.first(); (l != NULL); l = l->next())
                    l->data().bDuplicated = false;
            }

            //Mark parts that are mounted as duplicated and replenish them
            {
                const PartListIGC*  pparts = pship->GetParts();
                for (PartLinkIGC* l = pparts->first(); (l != NULL); l = l->next())
                {
                    IpartIGC*       ppart = l->data();
                    Mount           mount = ppart->GetMountID();

                    IpartTypeIGC*   ppt = ppart->GetPartType();
                    ppart->SetAmount(SHRT_MAX); //Things that don't care about amounts wont care if I try and set it.

                    //Mark the corresponding part in the cached loadout as having been duplicated
                    {
                        bool found = false;
                        CachedPartLink* lWrongMountMatch = NULL;
                        for (CachedPartLink* l = pcll->data().cpl.last(); (l != NULL); l = l->txen()) // mount 2,1,0,0,..,-1,-2,.. with previously sorted list
                        {
                            if ((l->data().ppt == ppt) &&
                                !l->data().bDuplicated)
                            {
                                if (l->data().mount == mount) //otherwise first slots get set as duplicated regardless of which are empty
                                {
                                    l->data().bDuplicated = true;
                                    found = true;
                                    break;
                                }
                                else
                                    lWrongMountMatch = l;
                            }
                        }
                        if (!found && lWrongMountMatch)
                            lWrongMountMatch->data().bDuplicated = true;
                    }
                }
            }

            //Fill all of the cargo slots with what was there before. Also fill module slots which were empty.
            Mount   cargo = -c_maxCargo;
            while (pship->GetMountedPart(NA, cargo) != NULL)
                cargo++;
            for (CachedPartLink* l = pcll->data().cpl.first(); (l != NULL); l = l->next())
            {
                CachedPart& cpl = l->data();

                if (!cpl.bDuplicated)
                {
                    IpartTypeIGC*   ppt = cpl.ppt;
                    if (ppt)
                    {
                        EquipmentType   et = ppt->GetEquipmentType();

                        if ((cpl.mount >= 0) && //anything not cargo
                            (pship->GetMountedPart(et, cpl.mount) == NULL) &&
                            (pship->GetHullType()->CanMount(ppt, cpl.mount)))
                        {
                            debugf(" RestoreLoadout: Restoring empty module slot %d, type %d with %d \n", cpl.mount, et, ppt->GetObjectID());
                            pship->CreateAndAddPart(ppt, cpl.mount, ppt->GetAmount(pship));
                        }
                        else if (cargo < 0)
                        {
                            pship->CreateAndAddPart(ppt, cargo++, SHRT_MAX);
                            while (pship->GetMountedPart(NA, cargo) != NULL)
                                cargo++;
                        }

                    }
                }
            }
        }
        else {
            debugf(" %s: no saved loadout (%d) matches hull type \"%s\", restore some other way\n", pship->GetName(), m_loadouts.n(), pship->GetBaseHullType()->GetName());
            IstationIGC* pstation = NULL;
            for (StationLinkIGC* l = pship->GetSide()->GetStations()->first(); l != NULL; l = l->next())
                if (l->data()->GetStationType()->GetCapabilities() & c_sabmLand) {
                    pstation = l->data();
                    break;
                }
            if (pstation) {
                //Money for the cargo
                Money   money = Money(0);
                Mount cargo = -c_maxCargo;
                while (cargo < 0) {
                    IpartIGC* pp = pship->GetMountedPart(NA, cargo++);
                    if (pp)
                        money += pp->GetPrice();
                }

                BuyDefaultLoadout(pship, pstation, NULL, &money);
            }
            else {
                debugf(" Just add fuel and ammo\n");

                IpartTypeIGC*   pptFuel = NULL;;
                if (pship->GetMountedPart(ET_Afterburner, 0) != NULL)
                    pptFuel = m_pCoreIGC->GetFuelPack();

                IpartTypeIGC*   pptAmmo = NULL;
                {
                    IweaponIGC* pweapon = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, 0));
                    if (pweapon && (pweapon->GetAmmoPerShot() > 0))
                        pptAmmo = m_pCoreIGC->GetAmmoPack();
                }

                int fuelCount = 0;
                Mount cargo = -c_maxCargo;
                while (cargo < 0) {
                    if (pship->GetMountedPart(NA, cargo) && pship->GetMountedPart(NA, cargo)->GetPartType() == pptFuel)
                        fuelCount++;
                    cargo++;
                }
                
                cargo = -c_maxCargo;
                while (cargo < 0) {
                    while (pship->GetMountedPart(NA, cargo) != NULL)
                        cargo++;
                    if (pptFuel && fuelCount < 3)
                        pship->CreateAndAddPart(pptFuel, cargo, 0x7fff);
                    else if (pptAmmo)
                        pship->CreateAndAddPart(pptAmmo, cargo, 0x7fff);
                    cargo++;
                }
            }

            const PartListIGC*  pparts = pship->GetParts();
            for (PartLinkIGC* l = pparts->first(); (l != NULL); l = l->next())
            {
                IpartIGC*       ppart = l->data();

                ppart->SetAmount(SHRT_MAX); //Things that don't care about amounts wont care if I try and set it.
            }
        }
    }

    Money    ReplaceLoadout(IshipIGC*       pship,
                            IstationIGC*    pstation,
                            IhullTypeIGC*   phulltype,
                            Money           budget,
                            CachedLoadoutList & loadoutList) //AaronMoore 1/10
    {
        CachedLoadoutLink*  pcll = GetLoadoutLink(phulltype, loadoutList); //AaronMoore 1/10
        if (pcll)
            budget = ReplaceLoadout(pship, pstation, &(pcll->data()), budget, loadoutList); //AaronMoore 1/10
        else    //No cached loadout ... 
            BuyDefaultLoadout(pship, pstation, phulltype, &budget);

        return budget;
    }
    Money   ReplaceLoadout(IshipIGC*        pship,
                           IstationIGC*     pstation,
                           CachedLoadout*   pcl,
                           Money            budget,
                           CachedLoadoutList & loadoutList) //AaronMoore 1/10
    {
        assert (pship->GetChildShips()->n() == 0);
        assert (pship->GetParts()->n() == 0);

        if (pcl == NULL)
            pcl = loadoutList.first() ? &(loadoutList.first()->data()) : NULL; //AaronMoore 1/10

        if (pcl && pcl->pht && pstation->CanBuy(pcl->pht) && (pcl->pht->GetPrice() <= budget))
        {
            IhullTypeIGC*   pht = (IhullTypeIGC*)(pstation->GetSuccessor(pcl->pht));

            budget -= pht->GetPrice();
            assert (budget >= 0);
            pship->SetBaseHullType(pht);

            //Clone the previous loadout
            for (CachedPartLink*  l = pcl->cpl.first(); (l != NULL); l = l->next())
            {
                CachedPart& cpl = l->data();

                IpartTypeIGC*   ppt;
                if (pstation->CanBuy(cpl.ppt))
                {
                    ppt = (IpartTypeIGC*)(pstation->GetSuccessor(cpl.ppt));
                }
                else
                    ppt = pstation->GetSimilarPart(cpl.ppt); //AaronMoore 1/10

                if (ppt)
                {
                    if (pht->CanMount(ppt, cpl.mount))
                        BuyPartOnBudget(pship, ppt, cpl.mount, &budget);
                    else
                        debugf("**** successor unable to mount %s/%s\n", pht->GetName(), ppt->GetName());
                }
            }

            //Fill any remaining slots with the default items if the hull type is upgraded
            if (pcl->pht != pht)
                TryToBuyParts(pship, pstation, &budget, pht, pht->GetPreferredPartTypes());
        }
        else
        {
            //No saved hull or one we can't buy ... get a reasonable default
            HullTypeLinkIGC*   phtl = m_pCoreIGC->GetHullTypes()->first();
            while (true)
            {
                assert (phtl != NULL);
                IhullTypeIGC*   pht = phtl->data();
                if ((pht->GetGroupID() >= 0) && pstation->CanBuy(pht) && (pht->GetPrice() <= budget))
                {
                    pht = (IhullTypeIGC*)(pstation->GetSuccessor(pht));

                    budget -= pht->GetPrice();
                    assert (budget >= 0);

                    BuyDefaultLoadout(pship, pstation, pht, &budget);
                    break;
                }

                phtl = phtl->next();
            }
        }

        return budget;
    }

    void    RestoreLoadout(IstationIGC* pstation)
    {
        IshipIGC*   pship = CreateEmptyShip(-3);
        assert (pship);
        RestoreLoadout(m_ship, pship, pstation, m_ship->GetValue() + GetMoney());

        //NYI disable loadout till we get an ack
        BuyLoadout(pship, false);

        pship->Terminate();
        pship->Release();
    }
    void    ReplaceLoadout(IstationIGC* pstation, bool bLaunch = false)
    {
        IshipIGC*   pship = CreateEmptyShip(-3);
        assert (pship);
        ReplaceLoadout(pship, pstation, (CachedLoadout*)NULL, GetMoney(), m_customLoadouts[0]); //AaronMoore 1/10 //Imago #146

        BuyLoadout(pship, bLaunch);

        pship->Terminate();
        pship->Release();
    }

	//AaronMoore 1/10
     bool    LoadCustomLoadoutFile(void)
    {
        bool result = false;

        // Retrieve the array of CachedLoadoutLists from an external FILE
        // Make sure we are using a different file for each core

        ZString strFilename = ZString("loadouts_");
        strFilename = strFilename + m_pMissionInfo->GetIGCStaticFile() + ".dat";

        FILE* inputFile;
		    inputFile = fopen((PCC)strFilename,"rb");
        if (inputFile)
        {
            fseek (inputFile, 0, SEEK_END);
            int size = ftell(inputFile);
            rewind(inputFile);

            byte * buffer = new byte[size];
            fread(buffer, size, 1, inputFile);

            if (buffer)
            {
                if (m_customLoadouts)
                    delete [] m_customLoadouts;

                m_customLoadouts = ImportCachedLoadout(buffer);

                delete [] buffer;
            }

            fclose(inputFile);

            result = true;
        }
        else
        {
            result = false;
        }

        return result;
    }

    bool    SaveCustomLoadoutFile(void)
    {
        bool result = false;

        // Save the array of CachedLoadoutLists to an external file
        // Make sure we are using a different file for each core

        int size = ExportCachedLoadout(m_customLoadouts, MAX_CUSTOM_LOADOUTS, NULL);
        byte * buffer = new byte[size];

        ExportCachedLoadout(m_customLoadouts, MAX_CUSTOM_LOADOUTS, buffer);

        ZString strFilename = ZString("loadouts_");
        strFilename = strFilename + m_pMissionInfo->GetIGCStaticFile() + ".dat";

        FILE* outputFile;
		    outputFile = fopen((PCC)strFilename,"wb");
        if (outputFile)
        {
            fwrite(buffer, size, 1, outputFile);
            fclose(outputFile);
            result = true;
        }
        else
        {
            result = false;
        }

        delete [] buffer;

        return result;
    }

    int     ExportCachedLoadout(CachedLoadoutList * loadouts, int num, byte * buffer)
    {
        int totalSize = 0;
        int numShips, numParts;
        ObjectID shipID, partID;
        Mount partMount;

        int size = sizeof(num);						                          // The Number of Loadout Lists
        if (buffer) memcpy ( buffer, &num, size);
        totalSize += size;

        for (int i = 0; i < num; i++)
        {
            numShips = loadouts[i].n();                               // The Number of Ships in this List
            size = sizeof(numShips);						
            if (buffer) memcpy(buffer+totalSize, &numShips, size);
            totalSize += size;
            
            if (numShips > 0)
            {
                for (CachedLoadoutLink* pcll = loadouts[i].first(); (pcll != NULL); pcll = pcll->next())
                {
                    CachedLoadout& pcl = pcll->data();                
                    {
                        shipID = pcl.pht->GetObjectID();               // The HullID for this ship
                        size = sizeof(shipID);				
                        if (buffer) memcpy(buffer+totalSize, &shipID, size);
                        totalSize += size;

                        numParts = pcl.cpl.n();                           // The Number of Parts in this Loadout
                        size = sizeof(numParts);					
                        if (buffer) memcpy(buffer+totalSize, &numParts, size);
                        totalSize += size;
    
                        for (CachedPartLink* cpl = pcl.cpl.first(); (cpl != NULL); cpl = cpl->next())
                        {
                            CachedPart& cp = cpl->data();
                            {
                                partID = cp.ppt->GetObjectID();        // The PartID of this part
                                size = sizeof(partID);				
                                if (buffer) memcpy(buffer+totalSize, &partID, size);
                                totalSize += size;

                                partMount = cp.mount;                   // The Mount of this part
                                size = sizeof(partMount);
                                if (buffer) memcpy(buffer+totalSize, &partMount, size);
                                totalSize += size;
                            }
                        }
                    }
                }
                
            }
        }

        return totalSize;
    }

    CachedLoadoutList * ImportCachedLoadout(byte * buffer)
    {
        assert (buffer);
        int readCount = 0;
        int size, numLists, numShips, numParts;
        ObjectID shipID, partID;
        Mount partMount;

        size = sizeof(numLists);
        memcpy(&numLists, buffer, size);                            // Read the Number of Lists
        readCount += size;

        CachedLoadoutList * loadouts = new CachedLoadoutList[numLists];

        for (int i = 0; i < numLists; i++)
        {
            size = sizeof(numShips);
            memcpy(&numShips, buffer+readCount, size);              // Read the Number of Ships
            readCount += size;

            for (int j = 0; j < numShips; j++)
            {
                size = sizeof(shipID);				
                memcpy(&shipID, buffer+readCount, size);           // Read this ship's HullID
                readCount += size;

                size = sizeof(numParts);				                    // The Number of Parts in this Loadout
                memcpy(&numParts, buffer+readCount, size);
                readCount+= size;

                // Find this ship and create a new CachedLoadout
                IhullTypeIGC *pht = m_pCoreIGC->GetHullType(shipID);

                if (pht)
                {
                    CachedLoadoutLink * pcll = new CachedLoadoutLink();
                    pcll->data().pht = pht;

                    // Add it to the end of this list
                    loadouts[i].last(pcll);

                    for (int k = 0; k < numParts; k++)
                    {
                        size = sizeof(partID);				
                        memcpy(&partID, buffer+readCount, size);       // The PartID of this part
                        readCount += size;
                    
                        size = sizeof(partMount);                      // The Mount of this part
                        memcpy(&partMount, buffer+readCount, size);
                        readCount += size;

                        // Find this Part and create a new CachedPart
                        IpartTypeIGC *ppt = m_pCoreIGC->GetPartType(partID);

                        if (ppt)
                        {
                            CachedPartLink* cpl = new CachedPartLink;
                            cpl->data().ppt = ppt;
                            cpl->data().mount = partMount;

                            // Add it to the end of this list
                            pcll->data().cpl.last(cpl);
                        }

                    }
                }
    
            }

        }

        return loadouts;
    } // end AaronMoore 1/10


    void    BuyDefaultLoadout(IshipIGC* pship, IstationIGC* pstation, IhullTypeIGC* pht, Money* pbudget)
    {
        assert (pship);
        assert (pship->GetChildShips()->n() == 0);

        if (pht)
        {
            const PartListIGC*  pparts = pship->GetParts();
            PartLinkIGC*    ppl;
            while (ppl = pparts->first())       //Intentional =
                ppl->data()->Terminate();

            assert(pship->GetParts()->n() == 0);

            assert(pht);
            pship->SetBaseHullType(pht);
        }
        else {
            // clear just the cargo -- for ship w/o loadout docking at a carrier
            pht = pship->GetBaseHullType();

            Mount   cargo = -c_maxCargo;
            while (cargo < 0) {
                IpartIGC* pp = pship->GetMountedPart(NA, cargo++);
                if (pp)
                    pp->Terminate();
            }
        }

        assert (pstation);

        TryToBuyParts(pship, pstation, pbudget, pht, pht->GetPreferredPartTypes());

        //Default loadout for cargo
        // First get part types to use.

        IpartTypeIGC*   pptFuel;
        if (pship->GetMountedPart(ET_Afterburner, 0) != NULL)
        {
            pptFuel = m_pCoreIGC->GetFuelPack();
            assert (pptFuel);
            assert (pptFuel->GetGroupID() >= 0);

            if (!pstation->CanBuy(pptFuel))
                pptFuel = NULL;
        }
        else
            pptFuel = NULL;

        IpartTypeIGC*   pptAmmo;
        {
            bool    bCarryAmmo = false;
            for (Mount i = 0; (i < pht->GetMaxWeapons()); i++)
            {
                IweaponIGC* pweapon = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, i));
                if (pweapon && (pweapon->GetAmmoPerShot() > 0))
                {
                    bCarryAmmo = true;
                    break;
                }
            }

            if (bCarryAmmo)
            {
                pptAmmo = m_pCoreIGC->GetAmmoPack();
                assert (pptAmmo);
                assert (pptAmmo->GetGroupID() >= 0);

                if (!pstation->CanBuy(pptAmmo))
                    pptAmmo = NULL;
            }
            else
                pptAmmo = NULL;
        }

        IpartTypeIGC*   pptMissile;
        if (pship->GetMountedPart(ET_Magazine, 0) != NULL)
        {
            pptMissile = pship->GetMountedPart(ET_Magazine, 0)->GetPartType();
            assert(pptMissile);
            assert(pptMissile->GetGroupID() >= 0);

            if (!pstation->CanBuy(pptMissile))
                pptMissile = NULL;
        }
        else
            pptMissile = NULL;

        IpartTypeIGC*   pptDispenser;
        if (pship->GetMountedPart(ET_Dispenser, 0) != NULL)
        {
            pptDispenser = pship->GetMountedPart(ET_Dispenser, 0)->GetPartType();
            assert(pptDispenser);
            assert(pptDispenser->GetGroupID() >= 0);

            if (!pstation->CanBuy(pptDispenser) ||
                pptDispenser->GetPartMask() == c_pbm2) //ignore int pulse probes
            {
                pptDispenser = NULL;
            }
        }
        else
            pptDispenser = NULL;

        IpartTypeIGC* nan = GetCore()->GetPartType(91);
        if (nan)
            nan = (IpartTypeIGC*)(pstation->GetSuccessor(nan));

        // Check if it's a combat ship or a scout-like one
        bool combatShip = !(pht->GetDefenseType() == 1 && // light armor
            pht->GetScannerRange() >= 2200.0f);

        //Start actually filling the cargo
        Mount   cargo = -c_maxCargo;
        while (pship->GetMountedPart(NA, cargo) != NULL)
            cargo++;

        // Buy nan
        if (pship->GetHullType()->CanMount(nan, 0) && pstation->CanBuy(nan)) // GT scouts can mount a nan only in the second slot
            if (nan && cargo < 0)
                BuyPartOnBudget(pship, nan, cargo++, pbudget);
        // Buy ammo
        if (pptAmmo && cargo < 0) {
            BuyPartOnBudget(pship, pptAmmo, cargo++, pbudget);
            if (pht->GetMaxWeapons() > 2 && cargo < 0)
                BuyPartOnBudget(pship, pptAmmo, cargo++, pbudget);
        }
        // Buy 2 fuel
        if (pptFuel && cargo < 0)
            BuyPartOnBudget(pship, pptFuel, cargo++, pbudget);
        if (pptFuel && cargo < 0 && combatShip)
            BuyPartOnBudget(pship, pptFuel, cargo++, pbudget);
        // Buy one dispenser item for combat ships
        if (combatShip && pptDispenser && cargo < 0)
            BuyPartOnBudget(pship, pptDispenser, cargo++, pbudget);
        // Buy items for non-combat ships
        if (!combatShip) {
            // One extra ammo for missile-less scouts
            if (!pptMissile && pptAmmo && cargo < 0)
                BuyPartOnBudget(pship, pptAmmo, cargo++, pbudget);
            // Buy prox
            IpartTypeIGC* prox = GetCore()->GetPartType(59);
            if (prox && pstation->CanBuy(prox) && pship->GetHullType()->CanMount(prox, 0)) {
                prox = (IpartTypeIGC*)(pstation->GetSuccessor(prox)); //won't return NULL
                if (cargo < 0)
                    BuyPartOnBudget(pship, prox, cargo++, pbudget);
                if (cargo < -2) //GT scout
                    BuyPartOnBudget(pship, prox, cargo++, pbudget);
            }
            else {
                //Buy Technoflux Plas Gen
                IpartTypeIGC* plasGen = GetCore()->GetPartType(65);
                if (plasGen && pstation->CanBuy(plasGen) && pship->GetHullType()->CanMount(plasGen, 0)) {
                    plasGen = (IpartTypeIGC*)(pstation->GetSuccessor(plasGen)); //won't return NULL
                    if (cargo < 0)
                        BuyPartOnBudget(pship, plasGen, cargo++, pbudget);
                    if (cargo < 0)
                        BuyPartOnBudget(pship, plasGen, cargo++, pbudget);
                }
            }
        }

        //Fill up any remaining slots
        bool    bBuyFuel = true;
        while (cargo < 0)
        {
            if (pptMissile)
                pship->CreateAndAddPart(pptMissile, cargo++, 0x7fff);
            else if (pptDispenser)
                pship->CreateAndAddPart(pptDispenser, cargo++, 0x7fff);
            else if (pptFuel && bBuyFuel) {
                pship->CreateAndAddPart(pptFuel, cargo++, 0x7fff);
                bBuyFuel = (pptAmmo == NULL);
            }
            else if (pptAmmo)
                pship->CreateAndAddPart(pptAmmo, cargo++, 0x7fff);
            else {
                // If nothing else is needed, take an extra counter (empty troop transport, EW bomber)
                if (pship->GetMountedPart(ET_ChaffLauncher, 0) != NULL)
                {
                    IpartTypeIGC*   pptChaff = pship->GetMountedPart(ET_ChaffLauncher, 0)->GetPartType();
                    if (pstation->CanBuy(pptChaff))
                        pship->CreateAndAddPart(pptChaff, cargo++, 0x7fff);
                }

                break;
            }
        }
    }

    CachedLoadoutLink*  GetLoadoutLink(IhullTypeIGC*    pht, CachedLoadoutList & loadoutList) //AaronMoore 1/10
    {
        for (CachedLoadoutLink* pcll = loadoutList.first(); (pcll != NULL); pcll = pcll->next()) //AaronMoore 1/10
        {
            if (pcll->data().pht == pht)
                return pcll;
        }

        return NULL;
    }
//AaronMoore 1/10 removed
/* 
    CachedLoadoutLink*  GetObsoleteLoadoutLink(IstationIGC* pst, IhullTypeIGC*    pht, CachedLoadoutList & loadout)
    {
        for (CachedLoadoutLink* pcll = loadout.first(); (pcll != NULL); pcll = pcll->next())
        {
            IhullTypeIGC*    matchHullType = pcll->data().pht;
        
            if (matchHullType == pht)
            {
                    return pcll;
            }
            
            else 
            {
                // I cant seem to check if this ship could be succeeded (user has saved a loadout on a newer version)
                // (As the current ship is not obsolete?)
                //for (IhullTypeIGC * psht = pht->GetSuccessorHullType(); (psht != NULL); psht = psht->GetSuccessorHullType())
                //{
                //    if (psht == matchHullType)
                //        return pcll;
                //}

                if (pst->IsObsolete(matchHullType))
                {
                    // Check each successor for this obsolete ship (user has saved a loadout on a previous version)
                    for (IhullTypeIGC * psht = matchHullType->GetSuccessorHullType(); (psht != NULL); psht = psht->GetSuccessorHullType())
                    {
                        if (psht == pht)
                            return pcll;
                    }
                }
            }
        }

        return NULL;
    }
*/
    virtual void    DropPart(IpartIGC*  ppart)
    {
        assert (ppart);
        assert (m_ship->GetStation() == NULL);
        assert (m_ship->GetCluster());

        assert (ppart->GetShip() == m_ship);

        if (m_fm.IsConnected())
        {
            SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(m_fm, pdata, CS, DROP_PART)
            END_PFM_CREATE

            pdata->et = ppart->GetEquipmentType();
            pdata->mount = ppart->GetMountID();
        }
        else
            CreateTreasureLocal(m_ship->GetLastUpdate(), m_ship, ppart, ppart->GetPartType(), m_ship->GetPosition(), 20.0f);

        ppart->Terminate();
    }

    virtual void    SwapPart(IpartIGC*  ppart, Mount mountNew)
    {
        assert (ppart);
        assert (mountNew >= -c_maxCargo);
        assert (ppart->GetShip() == m_ship);

        EquipmentType   et = ppart->GetEquipmentType();
        Mount           mountOld = ppart->GetMountID();

        IpartIGC*   ppartNew = m_ship->GetMountedPart(et, mountNew);
        if (ppartNew)
        {
            ppart->SetMountID(c_mountNA);
            ppartNew->SetMountID(mountOld);
        }

        ppart->SetMountID(mountNew);

        if (m_fm.IsConnected())
        {
            SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(m_fm, pdata, CS, SWAP_PART)
            END_PFM_CREATE

            pdata->etOld = et;
            pdata->mountOld = mountOld;
            pdata->mountNew = mountNew;
        }
		// mdvalley: If part is a missile, chaff, boost, or dispenser, reload it.
		if((et == ET_Magazine || et == ET_ChaffLauncher || et == ET_Afterburner || et == ET_Dispenser)
			&& mountNew >= 0 && mountOld != c_mountNA)
		{
			Reload(m_ship, (IlauncherIGC*)ppart, et);
		}
    }

    virtual void ForwardSquadMessage(FEDMESSAGE* pSquadMessage) {} // forward message to squads screen
    virtual void ForwardCharInfoMessage(FEDMESSAGE* pCharInfoMessage) {} // forward message to character info screen
    virtual void ForwardLeaderBoardMessage(FEDMESSAGE* pLeaderBoardMessage) {} // forward message to leader board screen

    virtual void OnReload(IpartIGC* ppart, bool bConsumed) {};
    virtual void OnQuitSide();
    virtual void OnJoinSide();
    virtual void OnEnterGame();
    virtual void OnQuitMission(QuitSideReason reason, const char* szMessageParam = NULL);
    virtual void AddPlayerToMission(PlayerInfo* pPlayer);
    virtual void AddPlayerToSide(PlayerInfo* pPlayerInfo, SideID sideID);
    virtual void RemovePlayerFromMission(PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam = NULL);
    virtual void RemovePlayerFromSide(PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam = NULL);

    void         SetZoneClubID(int nMemberID) { m_nMemberID = nMemberID; }
    int          GetZoneClubID() { return m_nMemberID; }

    IshipIGC*   GetLastSender(void) const
    {
        return m_pshipLastSender;
    }

    void        SetLastSender(IshipIGC* pship)
    {
        m_pshipLastSender = pship;
    }

    Money       GetLastMoneyRequest(void) const
    {
        return m_moneyLastRequest;
    }

    void        SetLastMoneyRequest(Money m)
    {
        m_moneyLastRequest = m;
    }


    //
    // Loadout lock downs
    //

    virtual void StartLockDown(const ZString& strReason, LockdownCriteria criteria);
    virtual void EndLockDown(LockdownCriteria criteria);

    bool IsLockedDown()
    {
        return m_lockdownCriteria != 0;
    };
    const ZString& GetLockDownReason()
    {
        assert(IsLockedDown());
        return m_strLockDownReason;
    };

    void BoardShip(IshipIGC*  pshipBoard);

    CfgInfo & GetCfgInfo()
    {
      return m_cfginfo;
    }

protected:

    TList<SquadMembership>  m_squadmemberships;

    virtual void CreateDummyShip();
    virtual void DestroyDummyShip();
    virtual void ModifyShipData(DataShipIGC* pds) {}
};

#endif
