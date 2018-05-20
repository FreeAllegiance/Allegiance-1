/*-------------------------------------------------------------------------
 * \src\Inc\Messages.h
 * 
 * Interface between Allegiance and AllSrv
 * 
 * Owner: 
 * 
 * Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/

#ifndef _MESSAGES_ 
#define _MESSAGES_ 

#include <igc.h>

#include "messagecore.h"

// KGJV - MSGVER has its own include file now
#include "MessageVersion.h"
 

/*
  *************************************************
                 MESSAGES START HERE  

         Messages range for this file is 1 - 200

  *************************************************
*/
struct PassengerData
{
    ShipID  shipID;
    Mount   turretID;
};

DEFINE_FEDMSG(C, LOGONREQ, 1)   // First message the client sends to the server.
  FM_VAR_ITEM(CharacterName);   // Setting character name/pw to non-NULL implies 
  FM_VAR_ITEM(ZoneTicket);      // Encrypted
  FM_VAR_ITEM(CDKey);           // Scrambled using CharacterName
  FM_VAR_ITEM(MissionPassword); // The password (if any) required for the mission.
  USHORT    fedsrvVer;
  Time      time;
  DWORD     dwCookie;
  int       crcFileList; 
#ifndef NO_STEAM
  int8_t	steamAuthTicket[1024]; // BT - STEAM
  uint32_t	steamAuthTicketLength; // BT - STEAM
  uint64_t	steamID; // BT - STEAM
  char		drmHash[50]; // BT - STEAM
#endif
END_FEDMSG

DEFINE_FEDMSG(S, LOGONACK, 2) // sent when the server recives FM_C_LOGONREQ
  FM_VAR_ITEM(CharName_OR_FailureReason);
  Time timeServer;       // the server time when the request was processed
  ShipID    shipID; // uniquely identifies this ship
  bool      fValidated; // if false, server will disconnect client after sending this message
  bool      fRetry; // if true, the client may want to present the user with the logon dialog again
  Cookie    cookie;
END_FEDMSG

DEFINE_FEDMSG(CS, LOGOFF, 3) // sent when the client wants to logoff, mirrored back to client.
END_FEDMSG

DEFINE_FEDMSG(C, BUY_LOADOUT, 4)
  FM_VAR_ITEM(loadout);
  bool  fLaunch;
END_FEDMSG

DEFINE_FEDMSG(S, BUY_LOADOUT_ACK, 5)
  FM_VAR_ITEM(loadout);
  bool  fBoughtEverything   : 1;
  bool  fLaunch             : 1;  // the server will still send the normal launch messages
END_FEDMSG

DEFINE_FEDMSG(S, OBJECT_SPOTTED, 6)
  ObjectType    otObject;
  ObjectID      oidObject;
  ObjectType    otSpotter;
  ObjectID      oidSpotter;
END_FEDMSG
 
DEFINE_FEDMSG(S, SET_CLUSTER, 7)
  SectorID                  sectorID;
  ServerSingleShipUpdate    shipupdate;
  Cookie                    cookie;
END_FEDMSG
  
DEFINE_FEDMSG(S, DEV_COMPLETED, 8)
  DevelopmentID     devId;
END_FEDMSG

DEFINE_FEDMSG(S, SET_BRIEFING_TEXT, 9)
  FM_VAR_ITEM(text); // the full briefing text if fGenerateStandardText, else just the starting location
  bool      fGenerateCivBriefing;
END_FEDMSG

DEFINE_FEDMSG(S, PLAYERINFO, 10)
  FM_VAR_ITEM(rgPersistPlayerScores); 
  // PersistPlayerScoreObject rgPersistPlayerScores[pfm->cbrgPersistPlayerScores / sizeof(PersistPlayerScoreObject)] = FM_VAR_REF(pfm, rgPersistPlayerScores);
  char                      CharacterName[c_cbName];
  Money                     money;
  bool                      fReady        : 1;
  bool                      fTeamLeader   : 1;
  bool                      fMissionOwner : 1;
  short                     nKills;
  short                     nEjections;
  short                     nDeaths;
  float                     fExperience;
  SideID                    iSide;
  ShipID                    shipID;
  RankID                    rankID;
  //WingID                    wingID;
  DroneTypeID               dtidDrone;
  PilotType                 pilotType;
  AbilityBitMask            abmOrders;
  ObjectID                  baseObjectID;
END_FEDMSG

struct StaticMapInfo
{
    char    cbIGCFile[c_cbFileName];
    char    cbFriendlyName[c_cbName]; 
    int     nNumTeams;
};

DEFINE_FEDMSG(S, STATIC_MAP_INFO, 11)
  FM_VAR_ITEM(maps); // an array of StaticMapInfo structures in order of appearance
END_FEDMSG

DEFINE_FEDMSG(CS, PROJECTILE_INSTANCE, 12)
  Time              timeCreate;
  DataProjectileIGC data;
  Vector            position;
END_FEDMSG

// keep in sync copy in ClubMessages.h
#ifndef _MESSAGES_ALLCLUB_H_
struct RankInfo
{
    int     requiredRanking;
    CivID   civ;
    RankID  rank;
    char    RankName[c_cbName];
};
#endif

DEFINE_FEDMSG(S, RANK_INFO, 13)
  FM_VAR_ITEM(ranks); // an array of RankInfo sorted by civ, then rank
  short   cRanks;
END_FEDMSG

DEFINE_FEDMSG(S, SET_START_TIME, 14)
  Time  timeStart;
END_FEDMSG

DEFINE_FEDMSG(CS, SET_TEAM_INFO, 15)
  SquadID   squadID;
  SideID    sideID;
  char      SideName[c_cbName];
END_FEDMSG

DEFINE_FEDMSG(S, PLAYER_RESCUED, 16)
  ShipID    shipIDRescuer;
  ShipID    shipIDRescuee;
END_FEDMSG

typedef ObjectID BallotID;

DEFINE_FEDMSG(S, BALLOT, 17)
  FM_VAR_ITEM(BallotText);
  ObjectType  otInitiator;
  ObjectID    oidInitiator;
  BallotID    ballotID;
  Time        timeExpiration;
  bool		  bHideToLeader; // KGJV #110
END_FEDMSG

DEFINE_FEDMSG(S, CANCEL_BALLOT, 18)
  BallotID ballotID;
END_FEDMSG

DEFINE_FEDMSG(C, VOTE, 19)
  BallotID ballotID;
  bool bAgree;
END_FEDMSG

enum ShipDeleteReason 
{
  SDR_OUTOFRANGE, SDR_KILLED, SDR_LEFTSECTOR, SDR_DOCKED, // client will cache ships when deleted for these reasons
  SDR_LOGGEDOFF,  // cliend is gone, so waste their ship and everything about it
  SDR_TERMINATE,  // client is getting a new ship so blow the old one away.
  SDR_SUPERCEDED, // client will never get this. This happens when a client logs back in while their
                  //    previous connection still existed. The new connection supercedes the old one.
};
DEFINE_FEDMSG(S, SHIP_DELETE, 20) // when the ship is really not in the sector anymore
  ShipID           shipID;
  ShipDeleteReason sdr;
END_FEDMSG

DEFINE_FEDMSG(C, RANDOMIZE_TEAMS, 21) // randomly assigns everyone to a team
END_FEDMSG

struct  ChatData
{
  ShipID        sidSender;

  ChatTarget    chatTarget;
  ObjectID      oidRecipient;

  SoundID       voiceOver;
  CommandID     commandID;
  bool          bObjectModel;   // was chat object model generated?
};

DEFINE_FEDMSG(CS, CHATMESSAGE, 23)
  FM_VAR_ITEM(Message);

  ChatData      cd;

  ObjectID      oidTarget;
  ObjectType    otTarget;
END_FEDMSG

// ship updates, repeatedly sent by the client at the following interval
const float c_dsecUpdateClient = 0.15f;
DEFINE_FEDMSG(C, SHIP_UPDATE, 24)
  Time              timeUpdate;
  Cookie            cookie; // last cookie received from message that has a cookie
  ClientShipUpdate  shipupdate;
END_FEDMSG

// ***PARTS***
// KG- obsolete
//#include "parts.h"

// All part definition messages must start with Part, and have a single struct (in parts.h) after that
// In order to be able to just copy structs without allocating them, all parts are two structs only.
// First struct is always "Part part", and the next is specific to the part type.
DEFINE_FEDMSG(S, EXPORT, 26)
  FM_VAR_ITEM(exportData);
  ObjectType  objecttype;
END_FEDMSG

DEFINE_FEDMSG(S, POSTER, 27)
  char      textureName[c_cbFileName];
  float     latitude;
  float     longitude;
  float     radius;
  SectorID  sectorID;
END_FEDMSG

DEFINE_FEDMSG(S, TREASURE, 28)
  DataTreasureIGC   dataTreasure;
END_FEDMSG

DEFINE_FEDMSG(S, DESTROY_TREASURE, 30)
  TreasureID        treasureID;
  SectorID          sectorID;
END_FEDMSG

DEFINE_FEDMSG(CS, NEW_SHIP_INSTANCE, 31)
  FM_VAR_ITEM(exportData); // DataShip
END_FEDMSG

DEFINE_FEDMSG(CS, DROP_PART, 33)    // drop part to space
  ShipID        shipID;             // ignored for message sent to server
  EquipmentType et;
  Mount         mount;
END_FEDMSG

// Mount Locations
DEFINE_FEDMSG(CS, SWAP_PART, 34)    //Swap two parts
  ShipID        shipID;
  EquipmentType etOld;
  Mount         mountOld;
  Mount         mountNew;
END_FEDMSG

DEFINE_FEDMSG(CS, PING, 35) // just send it back to client as soon as you get it
  Time timeClient;       // client sets this, and server just leaves it
  Time timeServer;       // Server sets this
  FMGuaranteed fmg;      // how the client wants the server to send back the echo
END_FEDMSG

DEFINE_FEDMSG(S, URLROOT, 36) // where to download stuff from
  FM_VAR_ITEM(UrlRoot);
END_FEDMSG

DEFINE_FEDMSG(S, MISSION_STAGE, 37)
  STAGE stage;
END_FEDMSG
  
DEFINE_FEDMSG(S, MISSIONDEF, 38) // sent when a mission is created, and when it changes
  MissionParams misparms;
  char      szDescription[c_cbDescription];
  DWORD     dwCookie;
  SideID    iSideMissionOwner; // mission owner is the team leader on that side
  bool      fAutoAcceptLeaders : 1;
  bool      fInProgress        : 1;
  STAGE     stage;
  // KGJV #114 - added server name & addr
  char      szServerName[c_cbName];
  char      szServerAddr[64];
  // yes, for missions with fewer than c_cSidesMax sides, the following arrays will waste space,
  // but that allows us to just keep an array of missions
  // $CRC: should probably just make a struct for the per team stuff and have an array of the struct
  char      rgszName        [c_cSidesMax][c_cbSideName];
  //CivID     rgCivID         [c_cSidesMax];
  StationID rgStationID     [c_cSidesMax]; // the station the team starts at (1 per team for now)
  ShipID    rgShipIDLeaders [c_cSidesMax];
  char      rgcPlayers      [c_cSidesMax];
  char      rgfAutoAccept   [c_cSidesMax];
  char      rgfReady        [c_cSidesMax];
  char      rgfForceReady   [c_cSidesMax];
  char      rgfActive       [c_cSidesMax];
  char		rgfAllies		[c_cSidesMax]; // #ALLY - ally group - NA is no allies
END_FEDMSG

DEFINE_FEDMSG(C, POSITIONREQ, 39) // client requests position on a side.
  // Client can issue multiple of these
  SideID  iSide;
END_FEDMSG

DEFINE_FEDMSG(S, POSITIONREQ, 40) // server forwards request to appropriate person if permission required
// This goes to everyone so that they know the player in question is in the lobby
  ShipID    shipID;
  SideID    iSide; 
END_FEDMSG

DEFINE_FEDMSG(C, POSITIONACK, 41) // mission/team leader accepts or rejects person
  // everyone gets either a PLAYER_SIDE_CHANGE or a DELPOSITIONREQ
  ShipID        shipID;
  SideID        iSide;
  bool          fAccepted;
END_FEDMSG

enum DelPositionReqReason
{
    DPR_Rejected,
    DPR_Canceled,
    DPR_NoBase,
    DPR_TeamFull,
    DPR_TeamBalance,
    DPR_NoMission,
    DPR_BadPassword,
    DPR_LobbyLocked,
    DPR_InvalidRank,
    DPR_NoJoiners,
    DPR_NoDefections,
    DPR_WrongSquad,
    DPR_OutOfLives,
    DPR_Banned,
    DPR_SideGone,
    DPR_GameFull,
    DPR_PrivateGame,
    DPR_SidesLocked,
    DPR_SideDefeated,
    DPR_CantLeadSquad,
    DPR_ServerPaused,
    DPR_DuplicateLogin,
    DPR_Other,
	DPR_BotsOnTeam0Only
};

DEFINE_FEDMSG(CS, DELPOSITIONREQ, 42) // cancel/reject a pending request
  ShipID                shipID;
  SideID                iSide;
  DelPositionReqReason  reason;
END_FEDMSG

DEFINE_FEDMSG(CS, LOCK_LOBBY, 43) // lock/unlock the lobby
  bool                  fLock;
END_FEDMSG

DEFINE_FEDMSG(CS, LOCK_SIDES, 44) // allow/disallow team changes
  bool                  fLock;
END_FEDMSG

DEFINE_FEDMSG(CS, PLAYER_READY, 45) // client is ready or not
  // Sent to everyone on team
  bool fReady;
  ShipID shipID;
END_FEDMSG

DEFINE_FEDMSG(S, TEAM_READY, 46) // team is ready or not
  // Server-generated based on whether everyone on team is ready, or leader has forced ready
  SideID    iSide;
  bool      fReady;
END_FEDMSG

DEFINE_FEDMSG(CS, FORCE_TEAM_READY, 47) // force team to be ready
  // Sent only by team leaders. Forwarded to everyone
  // if fForceReady==true, causes TEAM_READY to also be sent
  SideID    iSide;
  bool      fForceReady;
END_FEDMSG

DEFINE_FEDMSG(C, DOCKED, 48) // temporary until server detects docking
  // If stationID != NA, clients that can see this guy to receive a SHIP_DELETE, with SDR_DOCKED
  // If stationID == NA (undocked), other clients receive SHIP_INSTANCE as appropriate
  // *** stationID != NA is obsolete and no longer supported. 
  // This is only used for undocking
  StationID stationID;
END_FEDMSG

DEFINE_FEDMSG(CS, CHANGE_TEAM_CIV, 51) // sent by team leader to change the civ of the side, and forwarded to everybody
  SideID    iSide;
  CivID     civID;
  bool		random;  //Xynth #170 8/2010 add random factions
END_FEDMSG

DEFINE_FEDMSG(CS, AUTO_ACCEPT, 52) // sent by team leader, forwarded to all
  SideID    iSide; //can be NA (for auto accept of team leaders) if sent by mission owner
  bool      fAutoAccept;
END_FEDMSG

struct StationState
{
  StationID      stationID;
  BytePercentage bpHullFraction;
  BytePercentage bpShieldFraction;
};

DEFINE_FEDMSG(S, STATIONS_UPDATE, 57)
  FM_VAR_ITEM(rgStationStates);
END_FEDMSG

DEFINE_FEDMSG(S, STATION_CAPTURE, 58)
  StationID stationID;
  SideID    sidOld;
  SideID    sidNew;
  ShipID    shipIDCredit;
END_FEDMSG

DEFINE_FEDMSG(S, STATION_DESTROYED, 59)
  StationID stationID;
  ShipID    launcher;
END_FEDMSG

DEFINE_FEDMSG(S, MONEY_CHANGE, 60)
  Money     dMoney;
  ShipID    sidTo;
  ShipID    sidFrom;    //== NA     means a server initiated change (cheat, probably)
END_FEDMSG              //== sidTo  means a player initiated change (purchase, etc.)
                        //otherwise means a transfer from -> to

DEFINE_FEDMSG(S, DOCKED, 61) 
  StationID stationID;
END_FEDMSG


struct SideEndgameInfo
{
  char  sideName[c_cbName];
  CivID civID;
  Color color;
  short cKills;
  short cDeaths;
  short cEjections;
  short cBaseKills;
  short cBaseCaptures;
  short cFlags;
  short cArtifacts;
};

DEFINE_FEDMSG(S, GAME_OVER, 62)
  FM_VAR_ITEM(szGameoverMessage);
  SideEndgameInfo   rgSides[c_cSidesMax];
  SideID            iSideWinner;
  char              nNumSides;
  bool              bEjectPods;
END_FEDMSG

struct PlayerEndgameInfo
{
  char                      characterName[c_cbName];
  GameOverScoreObject       scoring;
  PersistPlayerScoreObject  stats;
  SideID                    sideId;
};

DEFINE_FEDMSG(S, GAME_OVER_PLAYERS, 63)
  FM_VAR_ITEM(rgPlayerInfo);
END_FEDMSG

typedef short BucketID; // uniquely identifies some TEAM bucket (not personal) that you can donate money to
DEFINE_FEDMSG(C, BUCKET_DONATE, 64)
  Money         moneyGiven;
  BucketID      iBucket;
END_FEDMSG

DEFINE_FEDMSG(S, BUCKET_STATUS, 65)
  DWORD         timeTotal;
  SideID        sideID;
  Money         moneyTotal;
  BucketID      iBucket;
END_FEDMSG

enum BucketItemType
{
  BIT_ShipType, BIT_Factory, BIT_StoragePit, 
};

struct BucketData
{
  BucketItemType  bucketitemtype;
  Money           cost;
  char            szName[c_cbName];
  char            szDescription[c_cbDescription];
  union
  {
    // BIT_ShipType
    struct
    {
      HullID hullidDrone;
    } drone;
    // BIT_Factory
    struct
    {
      //FactoryID factoryID;
    } factory;
    // ...
  };
};

DEFINE_FEDMSG(C, PLAYER_DONATE, 67)
  Money     moneyGiven;
  ShipID    shipID;
END_FEDMSG

DEFINE_FEDMSG(CS, SIDE_INACTIVE, 68) // KGJV #62 both way now
  SideID    sideID;
  bool      bActive;    // KGJV #62 - extended for empty teams feature
  bool		bChangeAET; // KGJV #62 - extended for empty teams feature
  bool		bAET;       // KGJV #62 - extended for empty teams feature
END_FEDMSG

struct MissileLaunchData
{
    Vector      vecPosition;
    Vector      vecForward;
    Vector      vecVelocity;
    MissileID   missileID;
};

DEFINE_FEDMSG(CS, FIRE_MISSILE, 70)
    FM_VAR_ITEM(missileLaunchData);
    float               lock;
    Time                timeFired;
    ExpendableTypeID    missiletypeID;
    ShipID              launcherID;
    SectorID            clusterID;
    ObjectType          targetType;
    ObjectID            targetID;
    bool                bDud;
END_FEDMSG

//Sent just to the side that changed
DEFINE_FEDMSG(S, SIDE_TECH_CHANGE, 75)
    SideID          sideID;
    TechTreeBitMask ttbmDevelopments;
END_FEDMSG

//Sent to everyone on a change
DEFINE_FEDMSG(S, SIDE_ATTRIBUTE_CHANGE, 76)
    SideID              sideID;
    GlobalAttributeSet  gasAttributes;
END_FEDMSG

//Force a ship to eject a lifepod
DEFINE_FEDMSG(S, EJECT, 77)
    ShipID              shipID;
    Vector              position;
    Vector              velocity;
    Vector              forward;
    Cookie              cookie;     //Used only by the receiving client
END_FEDMSG

//Used to tell the client about a new ship that they
//are just getting information on (which gets sent
//immediately when a ship undocks, ejects, or enters the sector)
DEFINE_FEDMSG(S, SINGLE_SHIP_UPDATE, 78)
  ObjectType                otTarget;
  ObjectID                  oidTarget;
  ServerSingleShipUpdate    shipupdate;
  bool                      bIsRipcording;
END_FEDMSG

DEFINE_FEDMSG(C, SUICIDE, 80)
END_FEDMSG

DEFINE_FEDMSG(C, FIRE_EXPENDABLE, 82)
    EquipmentType    et;
END_FEDMSG

DEFINE_FEDMSG(S, MISSILE_DESTROYED, 83)
    Vector              position;
    SectorID            clusterID;
    MissileID           missileID;
END_FEDMSG

DEFINE_FEDMSG(S, MINE_DESTROYED, 84)
    SectorID            clusterID;
    MineID              mineID;
END_FEDMSG

DEFINE_FEDMSG(S, PROBE_DESTROYED, 85)
    SectorID            clusterID;
    ProbeID             probeID;
END_FEDMSG

DEFINE_FEDMSG(S, ASTEROID_DESTROYED, 86)
    SectorID            clusterID;
    AsteroidID          asteroidID;
    bool                explodeF;
END_FEDMSG

DEFINE_FEDMSG(S, FIRE_EXPENDABLE, 87)
    ShipID              launcherID;
    EquipmentType       equipmentType;
END_FEDMSG

DEFINE_FEDMSG(S, TREASURE_SETS, 89)
  FM_VAR_ITEM(treasureSet); // array of TreasureSet. But ignore pargChance. You don't get (or need) that blob.
  // useage: TreasureSet * pargTreasureSet = FM_VAR_REF(pfmTreasureSets, treasureSet);
  short cTreasureSets;
END_FEDMSG

DEFINE_FEDMSG(S, SHIP_STATUS, 90)
    ShipID          shipID;
    ShipStatus      status;
END_FEDMSG

//Force the probe update structure to be packed to 3 bytes
#pragma pack(push, 1)
struct ProbeState
{
  ProbeID        probeID;
  BytePercentage bpFraction;
};
#pragma pack(pop)

DEFINE_FEDMSG(S, PROBES_UPDATE, 92)
  FM_VAR_ITEM(rgProbeStates);
END_FEDMSG

DEFINE_FEDMSG(CS, ORDER_CHANGE, 93)
  ShipID        shipID;
  ObjectID      objectID;
  ObjectType    objectType;
  CommandID     commandID;
  Command       command;
END_FEDMSG

DEFINE_FEDMSG(S, LEAVE_SHIP, 94)
  ShipID        sidChild;
END_FEDMSG

DEFINE_FEDMSG(S, JOINED_MISSION, 95)
  ShipID        shipID;
  float         dtime; // time delta for client to *ADD* to its timeGetTime() timer
  Cookie        cookie; // this is NOT a lobby cookie. It's an in-game cookie.
  DWORD         dwCookie; // this IS the lobby cookie--this ugliness will go away when we have separate exes for each game
END_FEDMSG

DEFINE_FEDMSG(S, LOADOUT_CHANGE, 96)
  FM_VAR_ITEM(loadout);
  FM_VAR_ITEM(rgPassengers);
  ShipID            sidShip;
END_FEDMSG

DEFINE_FEDMSG(C, ACTIVE_TURRET_UPDATE, 99)
    Time                        timeUpdate;
    ClientActiveTurretUpdate    atu;
END_FEDMSG

DEFINE_FEDMSG(C, INACTIVE_TURRET_UPDATE, 100)
END_FEDMSG

DEFINE_FEDMSG(S, TELEPORT_ACK, 102)
    StationID           stationID;
    bool                bNewHull;
END_FEDMSG

DEFINE_FEDMSG(C, BOARD_SHIP, 103)
    ShipID              sidParent;
END_FEDMSG

DEFINE_FEDMSG(C, VIEW_CLUSTER, 104)
    SectorID            clusterID;
    ObjectType          otTarget;
    ObjectID            oidTarget;
END_FEDMSG

DEFINE_FEDMSG(S, KILL_SHIP, 105)
    ShipID              shipID;
    ObjectType          typeCredit;
    ObjectID            idCredit;
    SideID              sideidKiller;
    bool                bKillCredit;
    bool                bDeathCredit;
END_FEDMSG

DEFINE_FEDMSG(CS, SET_WINGID, 106)
    ShipID              shipID;
    WingID              wingID;
    bool                bCommanded;
END_FEDMSG

DEFINE_FEDMSG(S, ICQ_CHAT_ACK, 107) // mirrored back by server with icqid when chat starts with "icq,"
    FM_VAR_ITEM(Message);
    int                 icqid;
END_FEDMSG

DEFINE_FEDMSG(CS, CHATBUOY, 108)
  FM_VAR_ITEM(Message);

  ChatData      cd;

  DataBuoyIGC   db;
END_FEDMSG

DEFINE_FEDMSG(S, CREATE_CHAFF, 109)
  Vector            p0;
  Vector            v0;
  Time              time0;
  ExpendableTypeID  etid;
END_FEDMSG

DEFINE_FEDMSG(S, MISSILE_SPOOFED, 110)
  MissileID     missileID;
END_FEDMSG

DEFINE_FEDMSG(S, END_SPOOFING, 111)
END_FEDMSG

DEFINE_FEDMSG(C, AUTODONATE, 112)
    ShipID      sidDonateTo;
    Money       amount;
END_FEDMSG

DEFINE_FEDMSG(CS, MISSIONPARAMS, 113)
    MissionParams      missionparams;
END_FEDMSG

DEFINE_FEDMSG(S, PAYDAY, 114)
    Money   dMoney;
END_FEDMSG

DEFINE_FEDMSG(S, SET_MONEY, 115)
    Money   money;
    ShipID  shipID;
END_FEDMSG

DEFINE_FEDMSG(S, AUTODONATE, 116)
    ShipID      sidDonateBy;
    ShipID      sidDonateTo;
    Money       amount;
END_FEDMSG

DEFINE_FEDMSG(C, MUTE, 117)
    ShipID      shipID;
    bool        bMute;
END_FEDMSG

DEFINE_FEDMSG(CS, SET_TEAM_LEADER, 121)
  // sent by a team leader to promote someone else to team leader and by the
  // server to notify everyone of a new team leader
  SideID    sideID;
  ShipID    shipID;
END_FEDMSG

DEFINE_FEDMSG(CS, SET_TEAM_INVESTOR, 122)
  // sent by a team leader to promote someone else to investor and by the
  // server to notify everyone of a new team investor
  SideID    sideID;
  ShipID    shipID;
END_FEDMSG

DEFINE_FEDMSG(CS, SET_MISSION_OWNER, 123)
  // sent when someone is promoted to be mission owner
  SideID    sideID;
  ShipID    shipID;
END_FEDMSG

enum QuitSideReason
{
    QSR_LeaderBooted,
    QSR_OwnerBooted,
    QSR_AdminBooted,
    QSR_ServerShutdown,
    QSR_SquadChange,
    QSR_SideDestroyed,
    QSR_RankLimits,
    QSR_TeamSizeLimits,
    QSR_Quit,
    QSR_LinkDead,
    QSR_DuplicateRemoteLogon,
    QSR_DuplicateLocalLogon,
    QSR_DuplicateCDKey,

    QSR_SwitchingSides,
    QSR_RandomizeSides,
	QSR_FlushSides,		// TE: Add this for balance patch
	QSR_BalanceSides,	// TE: Add this for balance patch
	QSR_BannedBySteam // BT - STEAM
};

#define QSRIsBoot(reason) ((reason) >= QSR_LeaderBooted && (reason) <= QSR_AdminBooted)

DEFINE_FEDMSG(CS, QUIT_MISSION, 124)
  // removes a player from the mission.  Can be sent by the team leader to boot
  // a player on their team, or by any player to boot themselves
  FM_VAR_ITEM(szMessageParam);
  ShipID            shipID;
  QuitSideReason    reason;
END_FEDMSG

DEFINE_FEDMSG(S, JOIN_SIDE, 125)
  SideID    sideID;
  ShipID    shipID;
END_FEDMSG

DEFINE_FEDMSG(CS, QUIT_SIDE, 126)
  FM_VAR_ITEM(szMessageParam);
  ShipID            shipID;
  QuitSideReason    reason;
END_FEDMSG

DEFINE_FEDMSG(S, ENTER_GAME, 128)
  // sent to the client once the client has all of the info it needs to start
  // playing in the current game (note: the count down may not have elapsed 
  // yet).  
END_FEDMSG

struct ReloadData
{
  short         amountTransfered;
  Mount         mount;              //always < 0
};

DEFINE_FEDMSG(CS, RELOAD, 129)
    FM_VAR_ITEM(rgReloads);
    ShipID  shipID;
END_FEDMSG
/*
DEFINE_FEDMSG(S, THREAT, 131)
    SectorID        sectorID;
    ClusterWarning  warning;
END_FEDMSG
*/

DEFINE_FEDMSG(S, GAME_STATE, 155)
  unsigned char conquest[c_cSidesMax];
  unsigned char territory[c_cSidesMax];
  unsigned char nFlags[c_cSidesMax];
  unsigned char nArtifacts[c_cSidesMax];
END_FEDMSG

DEFINE_FEDMSG(S, GAIN_FLAG, 156)
  SideID        sideidFlag;
  ShipID        shipidRecipient;
  bool			bIsTreasureDocked; // KGJV #118 - extended to notify docking of new tech (if true)
  PartID		parttypeidDocked;  // KGJV #118 - which part was docked, only valid if bIsTreasureDocked is true
END_FEDMSG

DEFINE_FEDMSG(C, START_GAME, 157)
END_FEDMSG

DEFINE_FEDMSG(S, ACQUIRE_TREASURE, 162)
  TreasureCode      treasureCode;
  ObjectID          treasureID;
  short             amount;
END_FEDMSG

DEFINE_FEDMSG(C, TREASURE_ACK, 163)
  Mount             mountID;      //== c_mountNA
END_FEDMSG

DEFINE_FEDMSG(S, ADD_PART, 164)
  ShipID            shipID;
  PartData          newPartData;
END_FEDMSG

DEFINE_FEDMSG(S, ENTER_LIFEPOD, 166)
  ShipID        shipID;
END_FEDMSG

DEFINE_FEDMSG(S, LIGHT_SHIPS_UPDATE, 175)
    FM_VAR_ITEM(rgInactiveTurretUpdates);
    FM_VAR_ITEM(rgLightShipUpdates);
END_FEDMSG

DEFINE_FEDMSG(S, HEAVY_SHIPS_UPDATE, 176)
    FM_VAR_ITEM(rgActiveTurretUpdates);
    FM_VAR_ITEM(rgHeavyShipUpdates);

    Time                    timeReference;
    Vector                  positionReference;

    BytePercentage          bpTargetHull;
    BytePercentage          bpTargetShield;
    CompactShipFractions    fractions;
END_FEDMSG

DEFINE_FEDMSG(S, VIEW_CLUSTER, 177)
    Vector              position;
    SectorID            clusterID;
    bool                bUsePosition;
END_FEDMSG

DEFINE_FEDMSG(S, BOARD_NACK, 178)
  ShipID    sidRequestedParent;
END_FEDMSG

#pragma pack(push, 1)
struct AsteroidState
{
  AsteroidID            asteroidID;
  CompactOrientation    co;
  short                 ore;
  BytePercentage        bpFraction;
};
#pragma pack(pop)
DEFINE_FEDMSG(S, ASTEROIDS_UPDATE, 179)
  FM_VAR_ITEM(rgAsteroidStates);
END_FEDMSG

DEFINE_FEDMSG(S, ASTEROID_DRAINED, 180)
    SectorID            clusterID;
    AsteroidID          asteroidID;
END_FEDMSG


DEFINE_FEDMSG(S, BUILDINGEFFECT_DESTROYED, 181)
    AsteroidID          asteroidID;
END_FEDMSG

DEFINE_FEDMSG(CS, REQUEST_MONEY, 182)
    Money               amount;
    ShipID              shipidRequest;
    HullID              hidFor;
END_FEDMSG

DEFINE_FEDMSG(S, SHIP_RESET, 183)
    ServerSingleShipUpdate    shipupdate;
    Cookie                    cookie;
END_FEDMSG

DEFINE_FEDMSG(C, RIPCORD_REQUEST, 184)
    SectorID    sidRipcord;
END_FEDMSG

DEFINE_FEDMSG(S, RIPCORD_ACTIVATE, 185)
    ShipID        shipidRipcord;
    ObjectType    otRipcord;
    ObjectID      oidRipcord;
    SectorID      sidRipcord;
END_FEDMSG

DEFINE_FEDMSG(S, RIPCORD_DENIED, 186)
END_FEDMSG

DEFINE_FEDMSG(S, RIPCORD_ABORTED, 187)
    ShipID        shipidRipcord;
END_FEDMSG

DEFINE_FEDMSG(S, WARP_BOMB, 188)
    Time                timeExplosion;
    WarpID              warpidBombed;
    ExpendableTypeID    expendableidMissile;
END_FEDMSG

DEFINE_FEDMSG(S, PROMOTE, 189)
    ShipID              shipidPromoted;
END_FEDMSG

DEFINE_FEDMSG(C, PROMOTE, 190)
    Mount               mountidPromoted;
END_FEDMSG

DEFINE_FEDMSG(S, CREATE_BUCKETS, 191)
    TechTreeBitMask ttbmDevelopments;
    TechTreeBitMask ttbmInitial;
END_FEDMSG

DEFINE_FEDMSG(S, RELAUNCH_SHIP, 192)
    FM_VAR_ITEM(loadout);
    ShipID              shipID;
    ShipID              carrierID;
    Vector              position;
    Vector              velocity;
    CompactOrientation  orientation;
    Cookie              cookie;
END_FEDMSG

/* pkk May 6th: Disabled bandwidth patch
// w0dk4 June 2007: Bandwith Patch
DEFINE_FEDMSG(C, BANDWIDTH, 193)
  unsigned int          value;     
END_FEDMSG*/


// w0dk4 player-pings feature
DEFINE_FEDMSG(S, PINGDATA, 194)
  ShipID              shipID;
  unsigned short	  ping;
  unsigned short      loss;
END_FEDMSG

DEFINE_FEDMSG(C, REQPINGDATA, 195)
END_FEDMSG
// end w0dk4

// #ALLY
DEFINE_FEDMSG(C, CHANGE_ALLIANCE, 196) // sent by game owner when changing alliances
  SideID    sideID;
  SideID    sideAlly; // side to ally to or NA to clear all alliances
END_FEDMSG

DEFINE_FEDMSG(S, CHANGE_ALLIANCES, 197) // sent by server when alliances change
  char      Allies[c_cSidesMax];
END_FEDMSG
// end #ALLY

DEFINE_FEDMSG(S, ASTEROID_MINED, 198)  //Xynth #132 7/2010 sent by server when an He3 rock is mined
    SectorID            clusterID;
    AsteroidID          asteroidID;
	BytePercentage      bpOreFraction;	
END_FEDMSG

DEFINE_FEDMSG(CS, HIGHLIGHT_CLUSTER, 199)  //Xynth #208 Notify clients of sector highlight
    SectorID            clusterID;	
	bool				highlight;	
END_FEDMSG

#endif // _MESSAGES_ 





