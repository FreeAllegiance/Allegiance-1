
// ***
// *** 
// *** This is a generated file. Please do not modify this file by hand or your changes will be lost.
// ***
// ***
//
// These values bridge COM Enums defined in IDL into JS compatible enums that can be used for intellisense.
// These values are also available at the global level, but I'm putting them into an Enums wrapper so that 
// intellisense from *IDL.d.ts can be used at design time, but at run time, this file is used to bridge to 
// the real values.


// Enums is the top level namespace. All Enums will go under this namespace. This will avoid collisions
// with global COM variables like "PigState" which has the same name as the enum. 

var Enums = function () { };



////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCObjectType Enum

Enums.AGCObjectType = function () { };

Enums.AGCObjectType.AGCObjectType_Invalid = -1;
Enums.AGCObjectType.AGC_ModelBegin = 0;
Enums.AGCObjectType.AGC_Ship = 0;
Enums.AGCObjectType.AGC_Station = 1;
Enums.AGCObjectType.AGC_Missile = 2;
Enums.AGCObjectType.AGC_Mine = 3;
Enums.AGCObjectType.AGC_Probe = 4;
Enums.AGCObjectType.AGC_Asteroid = 5;
Enums.AGCObjectType.AGC_Projectile = 6;
Enums.AGCObjectType.AGC_Warp = 7;
Enums.AGCObjectType.AGC_Treasure = 8;
Enums.AGCObjectType.AGC_Buoy = 9;
Enums.AGCObjectType.AGC_Chaff = 10;
Enums.AGCObjectType.AGC_BuildingEffect = 11;
Enums.AGCObjectType.AGC_ModelEnd = 11;
Enums.AGCObjectType.AGC_Side = 12;
Enums.AGCObjectType.AGC_Cluster = 13;
Enums.AGCObjectType.AGC_Bucket = 14;
Enums.AGCObjectType.AGC_PartBegin = 15;
Enums.AGCObjectType.AGC_Weapon = 15;
Enums.AGCObjectType.AGC_Shield = 16;
Enums.AGCObjectType.AGC_Cloak = 17;
Enums.AGCObjectType.AGC_Pack = 18;
Enums.AGCObjectType.AGC_Afterburner = 19;
Enums.AGCObjectType.AGC_LauncherBegin = 20;
Enums.AGCObjectType.AGC_Magazine = 20;
Enums.AGCObjectType.AGC_Dispenser = 21;
Enums.AGCObjectType.AGC_LauncherEnd = 21;
Enums.AGCObjectType.AGC_PartEnd = 21;
Enums.AGCObjectType.AGC_StaticBegin = 22;
Enums.AGCObjectType.AGC_ProjectileType = 22;
Enums.AGCObjectType.AGC_MissileType = 23;
Enums.AGCObjectType.AGC_MineType = 24;
Enums.AGCObjectType.AGC_ProbeType = 25;
Enums.AGCObjectType.AGC_ChaffType = 36;
Enums.AGCObjectType.AGC_Civilization = 27;
Enums.AGCObjectType.AGC_TreasureSet = 28;
Enums.AGCObjectType.AGC_BucketStart = 29;
Enums.AGCObjectType.AGC_HullType = 29;
Enums.AGCObjectType.AGC_PartType = 30;
Enums.AGCObjectType.AGC_StationType = 31;
Enums.AGCObjectType.AGC_Development = 32;
Enums.AGCObjectType.AGC_DroneType = 33;
Enums.AGCObjectType.AGC_BucketEnd = 33;
Enums.AGCObjectType.AGC_StaticEnd = 33;
Enums.AGCObjectType.AGC_Constants = 34;
Enums.AGCObjectType.AGC_AdminUser = 35;
Enums.AGCObjectType.AGCObjectType_Max = 36;
Enums.AGCObjectType.AGC_Any_Objects = 36;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCHullAbility Enum

Enums.AGCHullAbility = function () { };

Enums.AGCHullAbility.AGCHullAbility_Board = 0x01;
Enums.AGCHullAbility.AGCHullAbility_Rescue = 0x02;
Enums.AGCHullAbility.AGCHullAbility_Lifepod = 0x04;
Enums.AGCHullAbility.AGCHullAbility_NoPickup = 0x08;
Enums.AGCHullAbility.AGCHullAbility_NoEjection = 0x10;
Enums.AGCHullAbility.AGCHullAbility_NoRipcord = 0x20;
Enums.AGCHullAbility.AGCHullAbility_CheatToDock = 0x40;
Enums.AGCHullAbility.AGCHullAbility_Fighter = 0x80;
Enums.AGCHullAbility.AGCHullAbility_Captital = 0x100;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCAsteroidAbility Enum

Enums.AGCAsteroidAbility = function () { };

Enums.AGCAsteroidAbility.AGCAsteroidAbility_MineHe3 = 0x01;
Enums.AGCAsteroidAbility.AGCAsteroidAbility_MineIce = 0x02;
Enums.AGCAsteroidAbility.AGCAsteroidAbility_MineGold = 0x04;
Enums.AGCAsteroidAbility.AGCAsteroidAbility_Buildable = 0x08;
Enums.AGCAsteroidAbility.AGCAsteroidAbility_Special = 0x10;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCAxis Enum

Enums.AGCAxis = function () { };

Enums.AGCAxis.AGCAxis_Yaw = 0;
Enums.AGCAxis.AGCAxis_Pitch = 1;
Enums.AGCAxis.AGCAxis_Roll = 2;
Enums.AGCAxis.AGCAxis_Throttle = 3;
Enums.AGCAxis.AGCAxis_Max = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCEquipmentType Enum

Enums.AGCEquipmentType = function () { };

Enums.AGCEquipmentType.AGCEquipmentType_ChaffLauncher = 0;
Enums.AGCEquipmentType.AGCEquipmentType_Weapon = 1;
Enums.AGCEquipmentType.AGCEquipmentType_Magazine = 2;
Enums.AGCEquipmentType.AGCEquipmentType_Dispenser = 3;
Enums.AGCEquipmentType.AGCEquipmentType_Shield = 4;
Enums.AGCEquipmentType.AGCEquipmentType_Cloak = 5;
Enums.AGCEquipmentType.AGCEquipmentType_Pack = 6;
Enums.AGCEquipmentType.AGCEquipmentType_Afterburner = 7;
Enums.AGCEquipmentType.AGCEquipmentType_MAX = 8;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCTreasureType Enum

Enums.AGCTreasureType = function () { };

Enums.AGCTreasureType.AGCTreasureType_Part = 0;
Enums.AGCTreasureType.AGCTreasureType_Development = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCChatTarget Enum

Enums.AGCChatTarget = function () { };

Enums.AGCChatTarget.AGCChat_Everyone = 0;
Enums.AGCChatTarget.AGCChat_Leaders = 1;
Enums.AGCChatTarget.AGCChat_Admin = 2;
Enums.AGCChatTarget.AGCChat_Ship = 3;
Enums.AGCChatTarget.AGCChat_Allies = 4;
Enums.AGCChatTarget.AGCChat_Team = 5;
Enums.AGCChatTarget.AGCChat_Individual = 6;
Enums.AGCChatTarget.AGCChat_Individual_NoFilter = 7;
Enums.AGCChatTarget.AGCChat_Wing = 8;
Enums.AGCChatTarget.AGCChat_Echo = 9;
Enums.AGCChatTarget.AGCChat_AllSector = 10;
Enums.AGCChatTarget.AGCChat_FriendlySector = 11;
Enums.AGCChatTarget.AGCChat_Group = 12;
Enums.AGCChatTarget.AGCChat_GroupNoEcho = 13;
Enums.AGCChatTarget.AGCChat_NoSelection = 14;
Enums.AGCChatTarget.AGCChat_Max = 15;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCGameStage Enum

Enums.AGCGameStage = function () { };

Enums.AGCGameStage.AGCGameStage_NotStarted = 0;
Enums.AGCGameStage.AGCGameStage_Starting = 1;
Enums.AGCGameStage.AGCGameStage_Started = 2;
Enums.AGCGameStage.AGCGameStage_Over = 3;
Enums.AGCGameStage.AGCGameStage_Terminate = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////
// FileAttributes Enum

Enums.FileAttributes = function () { };

Enums.FileAttributes.FileAttribute_None = 0x00000000;
Enums.FileAttributes.FileAttribute_ReadOnly = 0x00000001;
Enums.FileAttributes.FileAttribute_Hidden = 0x00000002;
Enums.FileAttributes.FileAttribute_System = 0x00000004;
Enums.FileAttributes.FileAttribute_Directory = 0x00000010;
Enums.FileAttributes.FileAttribute_Archive = 0x00000020;
Enums.FileAttributes.FileAttribute_Encrypted = 0x00000040;
Enums.FileAttributes.FileAttribute_Normal = 0x00000080;
Enums.FileAttributes.FileAttribute_Temporary = 0x00000100;
Enums.FileAttributes.FileAttribute_SparseFile = 0x00000200;
Enums.FileAttributes.FileAttribute_ReparsePoint = 0x00000400;
Enums.FileAttributes.FileAttribute_Compressed = 0x00000800;
Enums.FileAttributes.FileAttribute_Offline = 0x00001000;
Enums.FileAttributes.FileAttribute_NotContentIndexed = 0x00002000;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigState Enum

Enums.PigState = function () { };

Enums.PigState.PigState_NonExistant = 0;
Enums.PigState.PigState_LoggingOn = 1;
Enums.PigState.PigState_LoggingOff = 2;
Enums.PigState.PigState_MissionList = 3;
Enums.PigState.PigState_CreatingMission = 4;
Enums.PigState.PigState_JoiningMission = 5;
Enums.PigState.PigState_QuittingMission = 6;
Enums.PigState.PigState_TeamList = 7;
Enums.PigState.PigState_JoiningTeam = 8;
Enums.PigState.PigState_WaitingForMission = 9;
Enums.PigState.PigState_Docked = 10;
Enums.PigState.PigState_Launching = 11;
Enums.PigState.PigState_Flying = 12;
Enums.PigState.PigState_Terminated = 13;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigMapType Enum

Enums.PigMapType = function () { };

Enums.PigMapType.PigMapType_SingleRing = 0;
Enums.PigMapType.PigMapType_DoubleRing = 1;
Enums.PigMapType.PigMapType_PinWheel = 2;
Enums.PigMapType.PigMapType_DiamondRing = 3;
Enums.PigMapType.PigMapType_Snowflake = 4;
Enums.PigMapType.PigMapType_SplitBases = 5;
Enums.PigMapType.PigMapType_Brawl = 6;
Enums.PigMapType.PigMapType_Count = 7;
Enums.PigMapType.PigMapType_Default = PigMapType_Brawl;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigLobbyMode Enum

Enums.PigLobbyMode = function () { };

Enums.PigLobbyMode.PigLobbyMode_Club = 0;
Enums.PigLobbyMode.PigLobbyMode_Free = 1;
Enums.PigLobbyMode.PigLobbyMode_LAN = 2;

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigThrust Enum

Enums.PigThrust = function () { };

Enums.PigThrust.ThrustNone = 0;
Enums.PigThrust.ThrustLeft = 1;
Enums.PigThrust.ThrustRight = 2;
Enums.PigThrust.ThrustUp = 3;
Enums.PigThrust.ThrustDown = 4;
Enums.PigThrust.ThrustForward = 5;
Enums.PigThrust.ThrustBackward = 6;
Enums.PigThrust.ThrustBooster = 7;

////////////////////////////////////////////////////////////////////////////////////////////////////
// These functions are simple pass-throughs so that your code will work properly at run-time.  
// This is how you can get intellisense support without having to use typescript to create a pig.

var CType = function() { };
CType.IAGCEventSink = function(o) { return o; };
CType.IAGCEventSinkSynchronous = function(o) { return o; };
CType.IAGCDebugHook = function(o) { return o; };
CType.IAGCGlobal = function(o) { return o; };
CType.IAGCVector = function(o) { return o; };
CType.IAGCVectorPrivate = function(o) { return o; };
CType.IAGCOrientation = function(o) { return o; };
CType.IAGCOrientationPrivate = function(o) { return o; };
CType.IAGCEventType = function(o) { return o; };
CType.IAGCEvent = function(o) { return o; };
CType.IAGCEventCreate = function(o) { return o; };
CType.IAGCGameParameters = function(o) { return o; };
CType.IAGCCommand = function(o) { return o; };
CType.IAGCCommandPrivate = function(o) { return o; };
CType.IAGCVersionInfo = function(o) { return o; };
CType.IAGCDBParams = function(o) { return o; };
CType.IAGCEventLogger = function(o) { return o; };
CType.IAGCEventLoggerHook = function(o) { return o; };
CType.IAGCEventLoggerPrivate = function(o) { return o; };
CType.IAGCRangePrivate = function(o) { return o; };
CType.IAGCRangesPrivate = function(o) { return o; };
CType.IAGCEventIDRange = function(o) { return o; };
CType.IAGCEventIDRanges = function(o) { return o; };
CType.ITCCollection = function(o) { return o; };
CType.ITCStrings = function(o) { return o; };
CType.ITCPropBagOnRegKey = function(o) { return o; };
CType.ITCSessionInfo = function(o) { return o; };
CType.ITCSessionInfos = function(o) { return o; };
CType.ITCCollectionPersistHelper = function(o) { return o; };
CType.ITCUtility = function(o) { return o; };
CType.IAdminSessionHelper = function(o) { return o; };
CType.IAGCPrivate = function(o) { return o; };
CType.IAGCCommon = function(o) { return o; };
CType.IAGCCollection = function(o) { return o; };
CType.IAGCBase = function(o) { return o; };
CType.IAGCModel = function(o) { return o; };
CType.IAGCModels = function(o) { return o; };
CType.IAGCBuyable = function(o) { return o; };
CType.IAGCHullType = function(o) { return o; };
CType.IAGCTreasure = function(o) { return o; };
CType.IAGCTreasures = function(o) { return o; };
CType.IAGCDamage = function(o) { return o; };
CType.IAGCScanner = function(o) { return o; };
CType.IAGCProbe = function(o) { return o; };
CType.IAGCProbes = function(o) { return o; };
CType.IAGCGame = function(o) { return o; };
CType.IAGCShip = function(o) { return o; };
CType.IAGCShips = function(o) { return o; };
CType.IAGCStation = function(o) { return o; };
CType.IAGCStations = function(o) { return o; };
CType.IAGCAleph = function(o) { return o; };
CType.IAGCAlephs = function(o) { return o; };
CType.IAGCSector = function(o) { return o; };
CType.IAGCSectors = function(o) { return o; };
CType.IAGCTeam = function(o) { return o; };
CType.IAGCTeams = function(o) { return o; };
CType.IAGCAsteroid = function(o) { return o; };
CType.IAGCAsteroids = function(o) { return o; };
CType.IPigBehaviorType = function(o) { return o; };
CType.IPigBehaviorTypes = function(o) { return o; };
CType.IPigBehaviorScriptType = function(o) { return o; };
CType.IPigBehaviorHost = function(o) { return o; };
CType.IPigBehavior = function(o) { return o; };
CType.IPigBehaviors = function(o) { return o; };
CType.IPigBehaviorStack = function(o) { return o; };
CType.IPig = function(o) { return o; };
CType.IPigs = function(o) { return o; };
CType.IPigTeam = function(o) { return o; };
CType.IPigTeams = function(o) { return o; };
CType.IPigMissionParams = function(o) { return o; };
CType.IPigMissionParamsPrivate = function(o) { return o; };
CType.IPigMission = function(o) { return o; };
CType.IPigMissions = function(o) { return o; };
CType.IPigLobby = function(o) { return o; };
CType.IPigLobbies = function(o) { return o; };
CType.IPigSession = function(o) { return o; };
CType.IPigSessionEvents = function(o) { return o; };
CType.IPigEvent = function(o) { return o; };
CType.IPigTimer = function(o) { return o; };
CType.IPigShip = function(o) { return o; };
CType.IPigShipEvent = function(o) { return o; };
CType.IPigVector = function(o) { return o; };
CType.IPigAccount = function(o) { return o; };
CType.IPigAccountDispenser = function(o) { return o; };
CType.IPigHullTypes = function(o) { return o; };
