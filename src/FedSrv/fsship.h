/*-------------------------------------------------------------------------
 * FSShip.h
 * 
 * Declaration of FedSrv ships class (CFSShip) and derivatives for players and drones
 * 
 * Owner: 
 * 
 * Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/
#ifndef _FSSHIP_H_
#define _FSSHIP_H_


#define GETFSSHIP(pship) ((CFSShip *) (pship)->GetPrivateData())
#define ISPLAYER(pship) (GETFSSHIP(pship)->IsPlayer())

typedef TLargeBitMask<c_cPlayersMax + c_cDronesMax> ShipDirtyMask;

class CFSMission;
class CFSPlayer;
class CFSDrone;
class CAdminUser; 

const short c_chatBudgetMax = 100;
const short c_chatBudgetCost = 20;

//Never more than 20 parts (actually 19 plus room to grow)
//    4 guns
//    4 turrets
//    missile, shield, booster, chaff, dispenser, cloak
//    5 cargo
const int   c_cbLoadout = sizeof(ShipLoadout) + 20 * sizeof(ExpandedPartData);

/* CFSShip
This is the common object for players and drones
Inherited classes must call Init during construction
*/
class CFSShip // abstract base class
{
public:
  Money GetMoney()            {return m_money;} // move this to Iship
  void  SetMoney(Money money) {assert(money >= 0); m_money = money;}

  void  AddKill()
  {
    m_pShip->AddKill();
    m_plrscore.AddKill();
  }

  void  AddDeath()
  {
    m_pShip->AddDeath();
    m_plrscore.AddDeath();
  }
  void  AddEjection()
  {
    m_pShip->AddEjection();
    m_plrscore.AddEjection();
  }

  bool  GetHasUpdate(void) const { return m_bHasUpdate; }
  void  SetHasUpdate(void)       { m_bHasUpdate = true; }
  void  ClearHasUpdate(void)     { m_bHasUpdate = false; }

  float GetDeviation(void) const { return m_deviation; }
  void  SetDeviation(float f)    { m_deviation = f;    }

  bool IsPlayer() {return m_fIsPlayer;}

  CFSPlayer * GetPlayer()
  {
    assert(IsPlayer());
    return (CFSPlayer *) this;
  }

  CFSDrone * GetDrone()
  {
    assert(!IsPlayer());
    return (CFSDrone *) this;
  }

  IshipIGC *  GetIGCShip() const  {return m_pShip;}
  ShipID      GetShipID()   {return m_pShip->GetObjectID();}

  // These are also on the IGC ship, but I'm not sure whether we'll be 
  // able to guarantee that we always have an IGC ship.
  // This is the cluster the USER is in, not their IGC ship.
  IclusterIGC * GetCluster()
  {
    IstationIGC * pstation = m_pShip->GetStation();
    return pstation ? pstation->GetCluster() : m_pShip->GetCluster();
  }
  IsideIGC*     GetSide()               {return m_pShip->GetSide();}
  virtual void  SetSide(CFSMission * pfsMission, IsideIGC * pside);
  IstationIGC * GetStation()            {return m_pShip->GetStation();}
  const char *  GetName()               {return m_pShip->GetName();}
  void			SetName(char * szName)  { m_pShip->SetName(szName); }

  static CFSShip * GetShipFromID(ShipID shipID)  
  {
    assert (shipID < c_cShipsMax && shipID >= 0);
    return m_rgpfsShip[shipID];
  }
  void          AnnounceExit(IclusterIGC* pclusterOld, ShipDeleteReason sdr);

  void          ShipStatusSpotted(IsideIGC* pside);
  void          ShipStatusExit(void);
  void          ShipStatusHidden(IsideIGC* pside);
  void          ShipStatusHullChange(IhullTypeIGC*    pht);
  void          ShipStatusDocked(IstationIGC*   pstation);
  void          ShipStatusLaunched(void);
  void          ShipStatusStart(IstationIGC*   pstation);
  void          ShipStatusRestart(IstationIGC*   pstation);
  void          ShipStatusWarped(IwarpIGC*   pwarp);
  void          ShipStatusRecalculate(void);

  ShipStatus*   GetShipStatus(SideID    sid)
  {
    assert (sid >= 0);
    assert (sid < c_cSidesMax);

    return &(m_rgShipStatus[sid]);
  }

  ShipStatus*   GetOldShipStatus(SideID    sid)
  {
    assert (sid >= 0);
    assert (sid < c_cSidesMax);

    return &(m_rgOldShipStatus[sid]);
  }

  //void          ShipLoadoutChange(LoadoutChange lc);
  void          QueueLoadoutChange(bool bForce = false);
  virtual void  Launch(IstationIGC* pstation);

  /*
  virtual bool  UpdateIsValid() = 0;
  virtual void  GetHeavyShipUpdate(HeavyShipUpdate * phsu) = 0; 
  virtual void  GetLightShipUpdate(LightShipUpdate * plsu) = 0;
  */

  virtual void  Dock(IstationIGC * pstation) = 0;
  CFSMission *  GetMission()            {return m_pfsMission;}
  void          HitWarp(IwarpIGC * pwarp);
  void          CaptureStation(IstationIGC * pstation);

  virtual void  Reset(bool bFull);

  void          SetTreasureData(ObjectID oid, short amount)
  {
      assert (m_oidTreasure == NA);
      m_oidTreasure = oid;
      m_amountTreasure = amount;
  }

  void          SetTreasureObjectID(TreasureID oid)
  {
      m_oidTreasure = oid;
  }

  ObjectID      GetTreasureObjectID(void) const
  {
      return m_oidTreasure;
  }

  short         GetTreasureAmount(void) const
  {
      return m_amountTreasure;
  }
  virtual void    SetCluster(IclusterIGC * pcluster, bool bViewOnly = false);

  PlayerScoreObject*    GetPlayerScoreObject(void)
  {
      return &m_plrscore;
  }

  void                  SetPlayerScoreObject(PlayerScoreObject* pplrscore)
  {
      if (pplrscore)
          m_plrscore = *pplrscore;
      else
          m_plrscore.Reset(true);
  }

    bool                OkToWarp(void) const
    {
        return m_warpState == warpReady;
    }

    void                SetWarpState(void)
    {
        m_warpState = warpNoUpdate;
    }

    void                ResetWarpState(void)
    {
        m_warpState = warpReady;
    }

protected:
  // can't instantiate this class. Must use inherited classes
  CFSShip(TRef<IshipIGC> pShip, bool fIsPlayer);
  ~CFSShip();

  enum                  WarpState
  { warpNoUpdate,
    warpWaiting,
    warpReady
  }                     m_warpState;
  Time                  m_timeNextWarp;

private:
  PlayerScoreObject     m_plrscore; 
  ShipStatus            m_rgShipStatus[c_cSidesMax];
  ShipStatus            m_rgOldShipStatus[c_cSidesMax];
  CFSMission *          m_pfsMission;


  TRef<IshipIGC>        m_pShip;        // must NOT be null;
  Money                 m_money;
  bool                  m_fIsPlayer;    // T => CFSPlayer, F => CFSDrone
  static ShipID         m_shipidNext;   // next ship id to use
  static int            m_cShips;
  static CFSShip*       m_rgpfsShip[c_cShipsMax]; // indexed for O(1) lookup by ShipID 

  ObjectID              m_oidTreasure;
  short                 m_amountTreasure;

  bool                  m_bHasUpdate;
  float                 m_deviation;

  friend   void FedSrvSiteBase::ChangeCluster(IshipIGC* pship,
                                          IclusterIGC* pclusterOld,
                                          IclusterIGC* pclusterNew);

  friend HRESULT FedSrvSiteBase::OnAppMessage(FedMessaging * pthis, CFMConnection & cnxn, FEDMESSAGE * pfm);
};

const unsigned char c_ucNone                 = 0;
const unsigned char c_ucShipUpdate           = 1;
const unsigned char c_ucActiveTurretUpdate   = 2;
const unsigned char c_ucInactiveTurretUpdate = 3;

class CFSPlayer : 
  public CFSShip,
  public CAdminSponsor<CAdminUser>
{
public:
  CFSPlayer(CFMConnection * pcnxn, int characterId, const char * szCDKey,
            TRef<IshipIGC> pShip, bool fCanCheat, CSteamID &steamID); // BT - STEAM
  virtual ~CFSPlayer();
  CFMGroup *      GetGroup() {return m_pgrp;}
  virtual void    Launch(IstationIGC* pstation);

  virtual void    Dock(IstationIGC * pstation);

  // BT - STEAM
  inline CSteamAchievements * GetSteamAchievements() {
	  return &m_pSteamAchievements;
  };

  // these are for ship updates only
  static DWORD    GetMaxLatency()     {return m_latencyMax;}
  static DWORD    GetAverageLatency() {return m_cUpdates ? m_latencyTotal / m_cUpdates : 0;}
  static void     ResetLatency()
  {
    m_cUpdates      = 0; 
    m_latencyMax    = 0;
    m_latencyTotal  = 0;
  }

  unsigned char     GetLastUpdate(void) const
  {
      return m_ucLastUpdate;
  }

  //imago 7/22/09 ALLY ff pvp
  void  SetLastDamageReport(Time t)
  {
    m_timeLastDamageReport = t;
  }

  Time  GetLastDamageReport(void) const
  {
    return m_timeLastDamageReport;
  }
  //

  void              ResetLastUpdate(void)
  {
      m_ucLastUpdate = c_ucNone;
  }
  void                          SetShipUpdate(const ClientShipUpdate& su);
  const ClientShipUpdate&       GetShipUpdate(void) const { return m_su; }

  void                              SetActiveTurretUpdate(const ClientActiveTurretUpdate& atu);
  const ClientActiveTurretUpdate&   GetActiveTurretUpdate(void) const { return m_atu; }

  void              SetInactiveTurretUpdate(void);

  bool            GetReady()            
  {
    return m_fReady;
  }
  void            SetReady(bool fReady);
  void            UnreadyNoSide(void) 
  { 
    assert(GetSide() == NULL); 
    m_fReady = false; 
  }
  bool            IsMissionOwner();
  Cookie          GetCookie()           
  {
    return m_cookie;
  }
  Cookie          NewCookie()           
  {
    ResetLastUpdate();
    return ++m_cookie;
  }
  int             GetCharacterID()      
  {
    return m_characterId;
  }
  const char*     GetCDKey() 
  { 
    return m_strCDKey; 
  }
  virtual void    SetSide(CFSMission * pmission, IsideIGC * pside);
  bool            CanCheat()            
  {
    return m_fCanCheat;
  }

  CFSPlayer*      GetAutoDonate(void) const
  {
    IshipIGC*   pship = GetIGCShip()->GetAutoDonate();

    return pship ? ((CFSShip*)(pship->GetPrivateData()))->GetPlayer()
                 : NULL;
  }

  void            SetAutoDonate(CFSPlayer* pplayer, Money amount, bool bSend = true);

  CFMConnection * GetConnection() 
  {
    return m_pcnxn;
  }
  static CFSPlayer * GetPlayerFromConnection(CFMConnection & cnxn) 
  {
    return (CFSPlayer *)cnxn.GetPrivateData();
  }
  void            SetDPGroup(CFSCluster*  pfsCluster, bool bFlying);

  virtual void    SetCluster(IclusterIGC * pcluster, bool bViewOnly = false);

  PersistPlayerScoreObjectList * GetPersistPlayerScoreObjectList()
  {
      return &m_perplrscoreList;
  }

  PersistPlayerScoreObject * GetPersistPlayerScore(CivID civID)
  {
    PersistPlayerScoreObjectLink * pl;
    for (pl = m_perplrscoreList.first(); pl; pl = pl->next())
      if (pl->data()->GetCivID() == civID)
        return pl->data();

    // Somebody's asking for a civ we don't have, so let's add it.
    PersistPlayerScoreObject * pperplrsco = new PersistPlayerScoreObject;
    pperplrsco->SetCivID(civID);
    m_perplrscoreList.last(pperplrsco);
    return pperplrsco;
  }

  SquadMembershipList * GetSquadMembershipList()
  {
      return &m_pSquadMembershipList;
  }
  bool GetIsMemberOfSquad(SquadID squadID);
  bool GetCanLeadSquad(SquadID squadID);
  SquadID GetPreferredSquadToLead();

  void  SetLifepod(IclusterIGC* pcluster, const Vector& position)
  {
      m_pclusterLifepod = pcluster;
      m_positionLifepod = position;
  }

  IclusterIGC*  GetLifepodCluster(void) const
  {
      return m_pclusterLifepod;
  }

  const Vector& GetLifepodPosition(void) const
  {
      return m_positionLifepod;
  }

  unsigned char GetBannedSideMask(void) const
  {
      return m_bannedSideMask;
  }
  void          SetBannedSideMask(unsigned char bsm)
  {
      m_bannedSideMask = bsm;
  }
  void          OrBannedSideMask(unsigned char bsm)
  {
      m_bannedSideMask |= bsm;
  }

  // mdvalley: functions to set/retrieve last side flown for
  SideID GetLastSide(void) const
  {
	  return m_lastSide;
  }
  void SetLastSide(SideID side)
  {
	  m_lastSide = side;
  }
/*  void ClearLastSide(void)	// don't really need this one, come to think of it
  {
	  m_lastSide = SIDE_TEAMLOBBY;	// SetLastSide(SIDE_TEAMLOBBY) works just as well.
  }
*/
  virtual void  Reset(bool  bFull)
  {
    m_chatBudget = c_chatBudgetMax;
    SetDPGroup(NULL, false);
    if (bFull)
	{
        SetBannedSideMask(0);
		// mdvalley: Clear out the last side, too
		SetLastSide(SIDE_TEAMLOBBY);
	}
    GetPlayerScoreObject()->Disconnect(g.timeNow);
    GetPlayerScoreObject()->Reset(bFull);
    CFSShip::Reset(bFull);
  }

  void             IncrementChatBudget(void)
  {
      if (m_chatBudget < c_chatBudgetMax)
          m_chatBudget++;
  }

  bool              DecrementChatBudget(bool bPay)
  {
      bool  bSend;

      if (bPay)
      {
          bSend = m_chatBudget >= c_chatBudgetCost;

          m_chatBudget -= c_chatBudgetCost;
          if (m_chatBudget < -c_chatBudgetCost)
              m_chatBudget = -c_chatBudgetCost;
      }
      else
          bSend = m_chatBudget >= 0;

      return bSend;
  }

  void  ForceLoadoutChange(void);

  void  SaveDesiredLoadout(void)
  {
      {
          IpartIGC* ppart = GetIGCShip()->GetMountedPart(ET_Magazine, 0);
          m_ptDesiredLoadout[0] = (ppart && (ppart->GetPrice() == 0)) ? ppart->GetPartType() : NULL;
      }
      {
          IpartIGC* ppart = GetIGCShip()->GetMountedPart(ET_Dispenser, 0);
          m_ptDesiredLoadout[1] = (ppart && (ppart->GetPrice() == 0)) ? ppart->GetPartType() : NULL;
      }
      {
          IpartIGC* ppart = GetIGCShip()->GetMountedPart(ET_ChaffLauncher, 0);
          m_ptDesiredLoadout[2] = (ppart && (ppart->GetPrice() == 0)) ? ppart->GetPartType() : NULL;
      }
      for (Mount i = 0; (i < c_maxCargo); i++)
      {
          IpartIGC* ppart = GetIGCShip()->GetMountedPart(NA, -1 - i);
          m_ptDesiredLoadout[i + 3] = (ppart &&
                                       (ppart->GetPrice() == 0) &&
                                       (IlauncherIGC::IsLauncher(ppart->GetObjectType())))
                                      ? ppart->GetPartType()
                                      : NULL;
      }
  }

  IpartTypeIGC**  GetDesiredLoadout(void)
  {
      return m_ptDesiredLoadout;
  }

  /* pkk May 6th: Disabled bandwidth patch
  // w0dk4 Bandwidth Patch
  unsigned int	GetBandwidth() {return m_nBandwidth; };
  void			SetBandwidth(unsigned int iBandwidth)
  {
	  if (iBandwidth >= 2 && iBandwidth <= 32) {
		  m_nBandwidth = iBandwidth;
		  if(m_nBandwidth > g.cMaxBandwidth && g.cMaxBandwidth >= 2)
			m_nBandwidth = g.cMaxBandwidth;
	  } else
		m_nBandwidth = 2;	  
  }*/

// w0dk4  allow more time when joining
bool			GetJustJoined() {return b_JustJoined; };
void			SetJustJoined(bool b_joined) {b_JustJoined = b_joined; };

private:
	CSteamAchievements	m_pSteamAchievements; // BT - STEAM
  IpartTypeIGC*     m_ptDesiredLoadout[c_maxCargo + 3];

  bool              RemoveFromSide(bool fSendSideChange);

  // mdvalley: Track side last flown for
  SideID			m_lastSide;

  unsigned char     m_bannedSideMask;
  short             m_chatBudget;


  PersistPlayerScoreObjectList m_perplrscoreList;
  SquadMembershipList m_pSquadMembershipList;

  CFSCluster*               m_pfsClusterFlying;

  unsigned char             m_ucLastUpdate;
  ClientActiveTurretUpdate  m_atu;
  ClientShipUpdate          m_su;
  Time                      m_timeUpdate;
  Time						m_timeLastDamageReport; //imago 7/22/09 ally FF pvp
  Time                  m_dwStartTime;
  int                   m_characterId;  // character id in the database--constant over sessions
  CFMGroup *            m_pgrp;         // the *location* group I'm a member of--can only be one of:
                                        // everyone, flying(sector), or docked(sector)
  bool                  m_fInUse        : 1;
  bool                  m_fReady        : 1;
  bool                  m_fCanCheat     : 1;
  Cookie                m_cookie;
  static int            m_latencyTotal; // aggregate over all updates
  static int            m_cUpdates;
  static int            m_latencyMax;
  CFMConnection *       m_pcnxn;

  IclusterIGC*          m_pclusterLifepod;
  Vector                m_positionLifepod;

  ZString               m_strCDKey;

  /* pkk May 6th: Disabled bandwidth patch
  // w0dk4
  unsigned int			m_nBandwidth;*/

  // w0dk4 allow more time when joining
  bool					b_JustJoined;

  friend   void FedSrvSiteBase::ChangeCluster(IshipIGC* pship,
                                          IclusterIGC* pclusterOld,
                                          IclusterIGC* pclusterNew);
  friend HRESULT FedSrvSiteBase::OnAppMessage(FedMessaging * pthis, CFMConnection & cnxn, FEDMESSAGE * pfm);
};


class CFSDrone : public CFSShip
{
public:
  CFSDrone(IshipIGC* pship);
  ~CFSDrone();

  virtual void Launch(IstationIGC* pstation);
  virtual void Dock(IstationIGC * pstation);

  void  SetLastDamageReport(Time t)
  {
    m_timeLastDamageReport = t;
  }

  Time  GetLastDamageReport(void) const
  {
    return m_timeLastDamageReport;
  }

  DroneTypeID   GetDroneTypeID(void) const
  {
    return m_dtid;
  }

  void          SetDroneTypeID(DroneTypeID  dtid)
  {
    m_dtid = dtid;
  }

private:
  Time              m_timeLastDamageReport;
  DroneTypeID       m_dtid;
  bool              m_fInUse        : 1;
  bool              m_fUpdateToggle; // a drone update is valid every other cycle;
  static char       m_rgfsDrone[];
  static int        m_cDrones;
  static int        m_ifsdNext; // next slot to use
};

#endif // _FSSHIP_H_
