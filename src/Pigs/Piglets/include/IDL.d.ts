
// ***
// *** 
// *** This is a generated file. Please do not modify this file by hand or your changes will be lost.
// ***
// ***

interface IDispatch { }
interface IDictionary { }
interface IUnknown { }
interface IStream { }


type Guid = string;

declare namespace Enums
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCObjectType Enum

	/**
	 * AGCUniqueID type.
	 */
	enum AGCObjectType
		{
		AGCObjectType_Invalid,
		AGC_ModelBegin,
		AGC_Ship,
		AGC_Station,
		AGC_Missile,
		AGC_Mine,
		AGC_Probe,
		AGC_Asteroid,
		AGC_Projectile,
		AGC_Warp,
		AGC_Treasure,
		AGC_Buoy,
		AGC_Chaff,
		AGC_BuildingEffect,
		AGC_ModelEnd,
		AGC_Side,
		AGC_Cluster,
		AGC_Bucket,
		AGC_PartBegin,
		AGC_Weapon,
		AGC_Shield,
		AGC_Cloak,
		AGC_Pack,
		AGC_Afterburner,
		AGC_LauncherBegin,
		AGC_Magazine,
		AGC_Dispenser,
		AGC_LauncherEnd,
		AGC_PartEnd,
		AGC_StaticBegin,
		AGC_ProjectileType,
		AGC_MissileType,
		AGC_MineType,
		AGC_ProbeType,
		AGC_ChaffType,
		AGC_Civilization,
		AGC_TreasureSet,
		AGC_BucketStart,
		AGC_HullType,
		AGC_PartType,
		AGC_StationType,
		AGC_Development,
		AGC_DroneType,
		AGC_BucketEnd,
		AGC_StaticEnd,
		AGC_Constants,
		AGC_AdminUser,
		AGCObjectType_Max,
		AGC_Any_Objects,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCHullAbility Enum

	/**
	 * Constants for the AGCHullAbilityBitMask type.
	 */
	enum AGCHullAbility
		{
		AGCHullAbility_Board,
		AGCHullAbility_Rescue,
		AGCHullAbility_Lifepod,
		AGCHullAbility_NoPickup,
		AGCHullAbility_NoEjection,
		AGCHullAbility_NoRipcord,
		AGCHullAbility_CheatToDock,
		AGCHullAbility_Fighter,
		AGCHullAbility_Captital,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCAsteroidAbility Enum

	/**
	 * Constants for the AGCAsteroidAbilityBitMask type.
	 */
	enum AGCAsteroidAbility
		{
		AGCAsteroidAbility_MineHe3,
		AGCAsteroidAbility_MineIce,
		AGCAsteroidAbility_MineGold,
		AGCAsteroidAbility_Buildable,
		AGCAsteroidAbility_Special,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCAxis Enum

	/**
	 * Constants for AGCAxis.
	 */
	enum AGCAxis
		{
		AGCAxis_Yaw,
		AGCAxis_Pitch,
		AGCAxis_Roll,
		AGCAxis_Throttle,
		AGCAxis_Max,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCEquipmentType Enum

	/**
	 * Constants for AGCEquipmentType.
	 */
	enum AGCEquipmentType
		{
		AGCEquipmentType_ChaffLauncher,
		AGCEquipmentType_Weapon,
		AGCEquipmentType_Magazine,
		AGCEquipmentType_Dispenser,
		AGCEquipmentType_Shield,
		AGCEquipmentType_Cloak,
		AGCEquipmentType_Pack,
		AGCEquipmentType_Afterburner,
		AGCEquipmentType_MAX,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCTreasureType Enum

	/**
	 * Constants for AGCTreasureType.
	 */
	enum AGCTreasureType
		{
		AGCTreasureType_Part,
		AGCTreasureType_Development,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCChatTarget Enum

	/**
	 * Constants for AGCChatTarget.
	 */
	enum AGCChatTarget
		{
		AGCChat_Everyone,
		AGCChat_Leaders,
		AGCChat_Admin,
		AGCChat_Ship,
		AGCChat_Allies,
		AGCChat_Team,
		AGCChat_Individual,
		AGCChat_Individual_NoFilter,
		AGCChat_Wing,
		AGCChat_Echo,
		AGCChat_AllSector,
		AGCChat_FriendlySector,
		AGCChat_Group,
		AGCChat_GroupNoEcho,
		AGCChat_NoSelection,
		AGCChat_Max,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCGameStage Enum

	/**
	 * Constants for AGCGameStage.
	 */
	enum AGCGameStage
		{
		AGCGameStage_NotStarted,
		AGCGameStage_Starting,
		AGCGameStage_Started,
		AGCGameStage_Over,
		AGCGameStage_Terminate,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FileAttributes Enum

	/**
	 * Constants for file attributes.
	 */
	enum FileAttributes
		{
		FileAttribute_None,
		FileAttribute_ReadOnly,
		FileAttribute_Hidden,
		FileAttribute_System,
		FileAttribute_Directory,
		FileAttribute_Archive,
		FileAttribute_Encrypted,
		FileAttribute_Normal,
		FileAttribute_Temporary,
		FileAttribute_SparseFile,
		FileAttribute_ReparsePoint,
		FileAttribute_Compressed,
		FileAttribute_Offline,
		FileAttribute_NotContentIndexed,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigState Enum

	/**
	 * Constants for pig player states.
	 */
	enum PigState
		{
		PigState_NonExistant,
		PigState_LoggingOn,
		PigState_LoggingOff,
		PigState_MissionList,
		PigState_CreatingMission,
		PigState_JoiningMission,
		PigState_QuittingMission,
		PigState_TeamList,
		PigState_JoiningTeam,
		PigState_WaitingForMission,
		PigState_Docked,
		PigState_Launching,
		PigState_Flying,
		PigState_Terminated,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigMapType Enum

	/**
	 * Constants for mission map types.
	 */
	enum PigMapType
		{
		PigMapType_SingleRing,
		PigMapType_DoubleRing,
		PigMapType_PinWheel,
		PigMapType_DiamondRing,
		PigMapType_Snowflake,
		PigMapType_SplitBases,
		PigMapType_Brawl,
		PigMapType_Count,
		PigMapType_Default,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigLobbyMode Enum

	/**
	 * Constants for pig lobby server mode.
	 */
	enum PigLobbyMode
		{
		PigLobbyMode_Club,
		PigLobbyMode_Free,
		PigLobbyMode_LAN,
	}

////////////////////////////////////////////////////////////////////////////////////////////////////
// PigThrust Enum

	/**
	 * Constants for pig thrust actions.
	 */
	enum PigThrust
		{
		ThrustNone,
		ThrustLeft,
		ThrustRight,
		ThrustUp,
		ThrustDown,
		ThrustForward,
		ThrustBackward,
		ThrustBooster,
	}

}


////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCUniqueID Alias

	/**
	 * AGCUniqueID type.
	 */
	type AGCUniqueID = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCObjectID Alias

	/**
	 * AGCObjectID type.
	 */
	type AGCObjectID = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCGameID Alias

	/**
	 * AGCGameID type.
	 */
	type AGCGameID = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCMoney Alias

	/**
	 * AGCMoney type.
	 */
	type AGCMoney = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCHitPoints Alias

	/**
	 * AGCHitPoints type.
	 */
	type AGCHitPoints = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCPartMask Alias

	/**
	 * AGCPartMask type.
	 */
	type AGCPartMask = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCMount Alias

	/**
	 * AGCMount type.
	 */
	type AGCMount = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCHullAbilityBitMask Alias

	/**
	 * AGCHullAbilityBitMask type.
	 */
	type AGCHullAbilityBitMask = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCAsteroidAbilityBitMask Alias

	/**
	 * AGCAsteroidAbilityBitMask type.
	 */
	type AGCAsteroidAbilityBitMask = number;

////////////////////////////////////////////////////////////////////////////////////////////////////
// AGCSoundID Alias

	/**
	 * AGCSoundID type.
	 */
	type AGCSoundID = number;



////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventSink Interface

/**
 * Interface implemented by the hosting application to receive triggered events. Intended for use ONLY by host applications.
 */
interface IAGCEventSink extends IUnknown
{
	/**
	 * Called when an AGC event has been triggered.
	 */
	 OnEventTriggered(pEvent: IAGCEvent): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventSinkSynchronous Interface

/**
 * Interface implemented by the hosting application to receive triggered events. Intended for use ONLY by host applications.
 */
interface IAGCEventSinkSynchronous extends IUnknown
{
	/**
	 * Called when an AGC event has been triggered.
	 */
	 OnEventTriggeredSynchronous(pEvent: IAGCEvent): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCDebugHook Interface

/**
 * Implemented by an AGC host to hook debug events.
 */
interface IAGCDebugHook extends IUnknown
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCGlobal Interface

/**
 * Interface to the AGC Global object. Intended for use ONLY by host applications that deal directly with Igc.
 */
interface IAGCGlobal extends IUnknown
{
	/**
	 * Registers the function that is called to create the object that suports the specified interface
	 */
	 GetAGCObject(pvIgc: object, riid: Guid, ppUnk: object): void;
	/**
	 * Adds a mapping of an Igc object pointer to an AGC object.
	 */
	 MakeAGCEvent(idEvent: number, pszContext: string, pszSubject: string, idSubject: AGCUniqueID, cArgTriplets: number, pvArgs: object): IAGCEvent;
	/**
	 * Sets the ranges of event ID's available from the host application.
	 */
	 MakeAGCVector(pVectorRaw: object): IAGCVector;
	/**
	 * Creates an AGCOrientation object, initialized with the specified raw ZLib orientation.
	 */
	 MakeAGCOrientation(pOrientationRaw: object): IAGCOrientation;
	/**
	 * Creates an AGCEventIDRange object, initialized with the specified values.
	 */
	 MakeAGCEventIDRange(lower: number, upper: number): IAGCEventIDRange;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCVector Interface

/**
 * Interface to a AGC Vector object.
 */
interface IAGCVector extends IDispatch
{
	/**
	 * Gets/sets the x coordinate of the vector.
	 */
	 X(xArg: number): void;
	/**
	 * Gets/sets the y coordinate of the vector.
	 */
	 Y(yArg: number): void;
	/**
	 * Gets/sets the z coordinate of the vector.
	 */
	 Z(zArg: number): void;
	/**
	 * Gets/sets the displayable string representation of the vector.
	 */
	 DisplayString(bstrDisplayString: string): void;
	/**
	 * Initializes the object with the specified x, y, and z values.
	 */
	 InitXYZ(xArg: number, yArg: number, zArg: number): void;
	/**
	 * Initializes the object by copying the specified vector's x, y, and z values.
	 */
	 InitCopy(pVector: IAGCVector): void;
	/**
	 * Initializes the object with a random direction.
	 */
	 InitRandomDirection(): void;
	/**
	 * Initializes the object with a random position.
	 */
	 InitRandomPosition(fRadius: number): void;
	/**
	 * READONLY: Compares the object to a zero vector.
	 */
	readonly IsZero: boolean;
	/**
	 * Compares the object to the specified vector.
	 */
	 IsEqual(pVector: IAGCVector): boolean;
	/**
	 * READONLY: Computes the length of the object from a zero vector.
	 */
	readonly Length: number;
	/**
	 * READONLY: Computes the squared length of the object from a zero vector.
	 */
	readonly LengthSquared: number;
	/**
	 * READONLY: Computes the orthogonal vector of the object.
	 */
	readonly OrthogonalVector: IAGCVector;
	/**
	 * Computes the sum of the object and the specified vector. Does not modify the object.
	 */
	 Add(pVector: IAGCVector): IAGCVector;
	/**
	 * Computes the difference of the object and the specified vector. Does not modify the object.
	 */
	 Subtract(pVector: IAGCVector): IAGCVector;
	/**
	 * Computes the product of the object and the specified multiplier value. Does not modify the object.
	 */
	 Multiply(f: number): IAGCVector;
	/**
	 * Computes the quotient of the object and the specified divisor value. Does not modify the object.
	 */
	 Divide(f: number): IAGCVector;
	/**
	 * Computes the normalized vector of the object. Does not modify the object.
	 */
	 Normalize(): IAGCVector;
	/**
	 * Computes the cross product of the object and the specified vector. Does not modify the object.
	 */
	 CrossProduct(pVector: IAGCVector): IAGCVector;
	/**
	 * Computes an interpolation of the object with the specified vector. Does not modify the object.
	 */
	 Interpolate(pVector: IAGCVector, fValue: number): IAGCVector;
	/**
	 * Computes the sum of the object and the specified vector, with the result being stored in the object.
	 */
	 AddInPlace(pVector: IAGCVector): void;
	/**
	 * Computes the difference of the object and the specified vector, with the result being stored in the object.
	 */
	 SubtractInPlace(pVector: IAGCVector): void;
	/**
	 * Computes the product of the object and the specified multiplier value, with the result being stored in the object.
	 */
	 MultiplyInPlace(f: number): void;
	/**
	 * Computes the quotient of the object and the specified divisor value, with the result being stored in the object.
	 */
	 DivideInPlace(f: number): void;
	/**
	 * Computes the normalized vector of the object, with the result being stored in the object.
	 */
	 NormalizeInPlace(): void;
	/**
	 * Computes the cross product of the object and the specified vector, with the result being stored in the object.
	 */
	 CrossProductInPlace(pVector: IAGCVector): void;
	/**
	 * Computes an interpolation of the object with the specified vector, with the result being stored in the object.
	 */
	 InterpolateInPlace(pVector: IAGCVector, fValue: number): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCVectorPrivate Interface

/**
 * Private interface to an AGC Vector object.
 */
interface IAGCVectorPrivate extends IUnknown
{
	/**
	 * Initializes the object from a raw (ZLib/IGC) Vector pointer.
	 */
	 InitFromVector(pvVector: object): void;
	/**
	 * Copies the object's raw (ZLib/IGC) Vector to the specified Vector pointer.
	 */
	 CopyVectorTo(pvVector: object): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCOrientation Interface

/**
 * Interface to an AGC Orientation object.
 */
interface IAGCOrientation extends IDispatch
{
	/**
	 * Initializes the object by copying the specified object.
	 */
	 InitCopy(pOrientation: IAGCOrientation): void;
	/**
	 * READONLY: Gets the Forward vector of the orientation matrix.
	 */
	readonly Forward: IAGCVector;
	/**
	 * READONLY: Gets the Backward vector of the orientation matrix.
	 */
	readonly Backward: IAGCVector;
	/**
	 * READONLY: Gets the Up vector of the orientation matrix.
	 */
	readonly Up: IAGCVector;
	/**
	 * READONLY: Gets the Right vector of the orientation matrix.
	 */
	readonly Right: IAGCVector;
	/**
	 * Compares the object to the specified object for absolute equality.
	 */
	 IsEqual(pOrientation: IAGCOrientation): boolean;
	/**
	 * Compares the object to the specified object for 'fuzzy' equality.
	 */
	 IsRoughlyEqual(pOrientation: IAGCOrientation): boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCOrientationPrivate Interface

/**
 * Private interface to an AGC Orientation object.
 */
interface IAGCOrientationPrivate extends IUnknown
{
	/**
	 * Initializes the object from a raw (ZLib/IGC) Orientation pointer.
	 */
	 InitFromOrientation(pvOrientation: object): void;
	/**
	 * Copies the object's raw (ZLib/IGC) Orientation to the specified Orientation pointer.
	 */
	 CopyOrientationTo(pvOrientation: object): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventType Interface

/**
 * IAGCEventType Interface
 */
interface IAGCEventType extends IDispatch
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEvent Interface

/**
 * IAGCEvent Interface
 */
interface IAGCEvent extends IDispatch
{
	/**
	 * READONLY: Gets the event identifier.
	 */
	readonly ID: number;
	/**
	 * READONLY: Gets the date and time that the event occurred.
	 */
	readonly Time: Date;
	/**
	 * READONLY: Gets the textual description of the event that occurred.
	 */
	readonly Description: string;
	/**
	 * READONLY: Gets the number of event parameters in the property map.
	 */
	readonly PropertyCount: number;
	/**
	 * Gets whether or not the specified event parameter is in the property map.
	 */
	 PropertyExists(bstrKey: string): boolean;
	/**
	 * Gets the specified event parameter from the property map.
	 */
	 Property(pvKey: object): object;
	/**
	 * READONLY: Gets the name of the computer on which the event occurred.
	 */
	readonly ComputerName: string;
	/**
	 * READONLY: Gets the ID of the subject of the event, if any. Otherwise, -1.
	 */
	readonly SubjectID: number;
	/**
	 * READONLY: Gets the name of the subject of the event, if any.
	 */
	readonly SubjectName: string;
	/**
	 * Persists the object to a string representation, to be resolved with LoadFromString.
	 */
	 SaveToString(): string;
	/**
	 * Initializes the object from a persistence string returned by SaveToString.
	 */
	 LoadFromString(bstr: string): void;
	/**
	 * READONLY: Gets a string representing the context in which the event occurred.
	 */
	readonly Context: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventCreate Interface

/**
 * IAGCEventCreate Interface
 */
interface IAGCEventCreate extends IDispatch
{
	/**
	 * Initializes object.
	 */
	 Init(): void;
	/**
	 * Sets the event identifier.
	 */
	 ID(Val: number): void;
	/**
	 * Sets the date and time that the event occurred.
	 */
	 Time(Val: Date): void;
	/**
	 * Sets the time property to the current date/time.
	 */
	 SetTimeNow(): void;
	/**
	 * Adds the specified event parameter to the property map.
	 */
	 AddProperty(pbstrKey: string, pvValue: object): void;
	/**
	 * Removes the specified event parameter from the property map.
	 */
	 RemoveProperty(pbstrKey: string, pvValue: object): void;
	/**
	 * Sets the ID of the subject of the event, if any. Otherwise, -1.
	 */
	 SubjectID(idSubject: number): void;
	/**
	 * Sets the name of the subject of the event, if any.
	 */
	 SubjectName(bstrSubject: string): void;
	/**
	 * Sets the string representing the context in which the event occurred.
	 */
	 Context(bstrContext: string): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCGameParameters Interface

/**
 * IAGCGameParameters Interface
 */
interface IAGCGameParameters extends IDispatch
{
	/**
	 * Validates the properties of the object.
	 */
	 Validate(): void;
	/**
	 * Gets/sets the minimum players per team.
	 */
	 MinPlayers(Val: number): void;
	/**
	 * Gets/sets the maximum players per team.
	 */
	 MaxPlayers(Val: number): void;
	/**
	 * property GameName
	 */
	GameName: string;
	/**
	 * property EjectPods
	 */
	EjectPods: boolean;
	/**
	 * property AllowPrivateTeams
	 */
	AllowPrivateTeams: boolean;
	/**
	 * property PowerUps
	 */
	PowerUps: boolean;
	/**
	 * property AllowJoiners
	 */
	AllowJoiners: boolean;
	/**
	 * property Stations
	 */
	Stations: boolean;
	/**
	 * property ScoresCount
	 */
	ScoresCount: boolean;
	/**
	 * property Drones
	 */
	Drones: boolean;
	/**
	 * property IsGoalConquest
	 */
	IsGoalConquest: boolean;
	/**
	 * property IsGoalArtifacts
	 */
	IsGoalArtifacts: boolean;
	/**
	 * property IsGoalTeamMoney
	 */
	IsGoalTeamMoney: boolean;
	/**
	 * property IsGoalTeamKills
	 */
	IsGoalTeamKills: boolean;
	/**
	 * property Resources
	 */
	Resources: boolean;
	/**
	 * property ResourceAmountsVisible
	 */
	ResourceAmountsVisible: boolean;
	/**
	 * property RandomWormholes
	 */
	RandomWormholes: boolean;
	/**
	 * property NoTeams
	 */
	NoTeams: boolean;
	/**
	 * property ShowHomeSector
	 */
	ShowHomeSector: boolean;
	/**
	 * property CivIDs
	 */
	 CivIDs(element: number, newVal: number): number;
	/**
	 * property GoalTeamMoney
	 */
	GoalTeamMoney: AGCMoney;
	/**
	 * property TsiPlayerStart
	 */
	TsiPlayerStart: number;
	/**
	 * property TsiNeutralStart
	 */
	TsiNeutralStart: number;
	/**
	 * property TsiPlayerRegenerate
	 */
	TsiPlayerRegenerate: number;
	/**
	 * property TsiNeutralRegenerate
	 */
	TsiNeutralRegenerate: number;
	/**
	 * property StartingMoney
	 */
	StartingMoney: number;
	/**
	 * property Lives
	 */
	Lives: number;
	/**
	 * property GoalTeamKills
	 */
	GoalTeamKills: number;
	/**
	 * property MapType
	 */
	MapType: number;
	/**
	 * property MapSize
	 */
	MapSize: number;
	/**
	 * property RandomEncounters
	 */
	RandomEncounters: number;
	/**
	 * property NeutralSectors
	 */
	NeutralSectors: boolean;
	/**
	 * property AlephPositioning
	 */
	AlephPositioning: number;
	/**
	 * property AlephsPerSector
	 */
	AlephsPerSector: number;
	/**
	 * property Teams
	 */
	Teams: number;
	/**
	 * property MinRank
	 */
	MinRank: number;
	/**
	 * property MaxRank
	 */
	MaxRank: number;
	/**
	 * property PlayerSectorAsteroids
	 */
	PlayerSectorAsteroids: number;
	/**
	 * property NeutralSectorAsteroids
	 */
	NeutralSectorAsteroids: number;
	/**
	 * property PlayerSectorTreasures
	 */
	PlayerSectorTreasures: number;
	/**
	 * property NeutralSectorTreasures
	 */
	NeutralSectorTreasures: number;
	/**
	 * property PlayerSectorTreasureRate
	 */
	PlayerSectorTreasureRate: number;
	/**
	 * property NeutralSectorTreasureRate
	 */
	NeutralSectorTreasureRate: number;
	/**
	 * property PlayerSectorMineableAsteroids
	 */
	PlayerSectorMineableAsteroids: number;
	/**
	 * property NeutralSectorMineableAsteroids
	 */
	NeutralSectorMineableAsteroids: number;
	/**
	 * property PlayerSectorSpecialAsteroids
	 */
	PlayerSectorSpecialAsteroids: number;
	/**
	 * property NeutralSectorSpecialAsteroids
	 */
	NeutralSectorSpecialAsteroids: number;
	/**
	 * property IGCStaticFile
	 */
	IGCStaticFile: string;
	/**
	 * property GamePassword
	 */
	GamePassword: string;
	/**
	 * property InvulnerableStations
	 */
	InvulnerableStations: boolean;
	/**
	 * property ShowMap
	 */
	ShowMap: boolean;
	/**
	 * property AllowDevelopments
	 */
	AllowDevelopments: boolean;
	/**
	 * property AllowDefections
	 */
	AllowDefections: boolean;
	/**
	 * property SquadGame
	 */
	SquadGame: boolean;
	/**
	 * property AllowFriendlyFire
	 */
	AllowFriendlyFire: boolean;
	/**
	 * READONLY: property IGCcoreVersion
	 */
	readonly IGCcoreVersion: number;
	/**
	 * property GameLength
	 */
	GameLength: number;
	/**
	 * property He3Density
	 */
	He3Density: number;
	/**
	 * property KillPenalty
	 */
	KillPenalty: AGCMoney;
	/**
	 * property KillReward
	 */
	KillReward: AGCMoney;
	/**
	 * property EjectPenalty
	 */
	EjectPenalty: AGCMoney;
	/**
	 * property EjectReward
	 */
	EjectReward: AGCMoney;
	/**
	 * READONLY: property TimeStart
	 */
	readonly TimeStart: number;
	/**
	 * READONLY: property TimeStartClock
	 */
	readonly TimeStartClock: number;
	/**
	 * property GoalArtifactsCount
	 */
	GoalArtifactsCount: number;
	/**
	 * property AutoRestart
	 */
	AutoRestart: boolean;
	/**
	 * property DefaultCountdown (in seconds)
	 */
	DefaultCountdown: number;
	/**
	 * property InitialMinersPerTeam
	 */
	InitialMinersPerTeam: number;
	/**
	 * property MaxDronesPerTeam
	 */
	MaxDronesPerTeam: number;
	/**
	 * property CustomMap
	 */
	CustomMap: string;
	/**
	 * property RestartCountdown (in seconds)
	 */
	RestartCountdown: number;
	/**
	 * property TotalMaxPlayers (for the entire game)
	 */
	TotalMaxPlayers: number;
	/**
	 * property LockTeamSettings (locks settings for players not admins)
	 */
	LockTeamSettings: boolean;
	/**
	 * property InvitationListID
	 */
	InvitationListID: number;
	/**
	 * property IsSquadGame
	 */
	IsSquadGame: boolean;
	/**
	 * property LockGameOpen
	 */
	LockGameOpen: boolean;
	/**
	 * property TeamName
	 */
	 TeamName(iTeam: number, newVal: string): string;
	/**
	 * property IsTechBitOverridden; returns true iff OverriddenTechBit was set for this Team's BitID
	 */
	 IsTechBitOverridden(iTeam: number, iBitID: number): boolean;
	/**
	 * property OverriddenTechBit
	 */
	 OverriddenTechBit(iTeam: number, iBitID: number, newVal: boolean): boolean;
	/**
	 * property SetOverriddenTechBitRange
	 */
	 SetOverriddenTechBitRange(iTeam: number, iBitID_First: number, iBitID_Last: number, newVal: boolean): void;
	/**
	 * property IsGoalFlags
	 */
	IsGoalFlags: boolean;
	/**
	 * property GoalFlagsCount
	 */
	GoalFlagsCount: number;
	/**
	 * The text that describes the story of the game.
	 */
	StoryText: string;
	/**
	 * property AllowEmptyTeams
	 */
	AllowEmptyTeams: boolean;
	/**
	 * property AutoStart
	 */
	AutoStart: boolean;
	/**
	 * property AllowRestart
	 */
	AllowRestart: boolean;
	/**
	 * property Experimental
	 */
	Experimental: boolean;
	/**
	 * property AllowShipyardPath
	 */
	AllowShipyardPath: boolean;
	/**
	 * property AllowSupremacyPath
	 */
	AllowSupremacyPath: boolean;
	/**
	 * property AllowTacticalPath
	 */
	AllowTacticalPath: boolean;
	/**
	 * property AllowExpansionPath
	 */
	AllowExpansionPath: boolean;
	/**
	 * property MaxImbalance: Maximum allowed difference between smallest and largest team.
	 */
	MaxImbalance: number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCCommand Interface

/**
 * Interface to an AGC Command object.
 */
interface IAGCCommand extends IDispatch
{
	/**
	 * READONLY: Gets the command target.
	 */
	readonly Target: string;
	/**
	 * READONLY: Gets the command verb.
	 */
	readonly Verb: string;
	/**
	 * READONLY: Gets the textual represenation of the command.
	 */
	readonly Text: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCCommandPrivate Interface

/**
 * Private interface to an AGC Command object.
 */
interface IAGCCommandPrivate extends IUnknown
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCVersionInfo Interface

/**
 * IAGCVersionInfo Interface
 */
interface IAGCVersionInfo extends IDispatch
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCDBParams Interface

/**
 * IAGCDBParams Interface
 */
interface IAGCDBParams extends IDispatch
{
	/**
	 * Gets/sets the database connection string.
	 */
	 ConnectionString(bstr: string): void;
	/**
	 * Gets/sets the table name.
	 */
	 TableName(bstr: string): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventLogger Interface

/**
 * IAGCEventLogger Interface
 */
interface IAGCEventLogger extends IDispatch
{
	/**
	 * READONLY: Gets the entire list of AGC Events as an XML string.
	 */
	readonly EventList: string;
	/**
	 * Gets/sets the name of the (WinNT or Win2000) computer to which AGC Events will be logged as NT Events.
	 */
	 NTEventLog(bstrComputer: string): void;
	/**
	 * Gets/sets the parameters of the database to which AGC Events will be logged.
	 */
	 DBEventLog(pDBParams: IAGCDBParams): void;
	/**
	 * Gets/sets the range(s) of AGC Events to be logged to the NT Event log.
	 */
	 EnabledNTEvents(pEvents: IAGCEventIDRanges): void;
	/**
	 * Gets/sets the range(s) of AGC Events to be logged to the database event log.
	 */
	 EnabledDBEvents(pEvents: IAGCEventIDRanges): void;
	/**
	 * READONLY: Gets the default range(s) of AGC Events to be logged to the NT Event log.
	 */
	readonly DefaultEnabledNTEvents: IAGCEventIDRanges;
	/**
	 * READONLY: Gets the default range(s) of AGC Events to be logged to the DB Event log.
	 */
	readonly DefaultEnabledDBEvents: IAGCEventIDRanges;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventLoggerHook Interface

/**
 * Implemented by an AGC host to hook event logging.
 */
interface IAGCEventLoggerHook extends IUnknown
{
	/**
	 * Called to allow the host application to log an event in its own way. Host should return S_FALSE to indicate to the event logger that it should perform 'normal' event logging.
	 */
	 LogEvent(pEvent: IAGCEvent, bSynchronous: boolean): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventLoggerPrivate Interface

/**
 * IAGCEventLoggerPrivate Interface
 */
interface IAGCEventLoggerPrivate extends IUnknown
{
	/**
	 * Should be called immediately upon creating the object (and from the same thread or apartment).
	 */
	 Initialize(bstrSourceApp: string, bstrRegKey: string): void;
	/**
	 * Should be called immediately prior to releasing the object.
	 */
	 Terminate(): void;
	/**
	 * Gets/sets whether or not logging to the NT Event log is enabled or not.
	 */
	 LoggingToNTEnabled(bEnabled: boolean): void;
	/**
	 * Gets/sets whether or not logging to the DB Event log is enabled or not.
	 */
	 LoggingToDBEnabled(bEnabled: boolean): void;
	/**
	 * Gets/sets a callback interface to hook events logged to the NT Event log.
	 */
	 HookForNTLogging(pHook: IAGCEventLoggerHook): void;
	/**
	 * Gets/sets a callback interface to hook events logged to the DB Event log.
	 */
	 HookForDBLogging(pHook: IAGCEventLoggerHook): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCRangePrivate Interface

/**
 * Private interface to an AGC Range object.
 */
interface IAGCRangePrivate extends IUnknown
{
	/**
	 * Initializes the object from a raw (TCLib) range<T> pointer.
	 */
	 InitFromRange(pvRange: object): void;
	/**
	 * Copies the object's raw (TCLib) range to the specified range<T> pointer.
	 */
	 CopyRangeTo(pvRange: object): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCRangesPrivate Interface

/**
 * Private interface to an AGC Ranges object.
 */
interface IAGCRangesPrivate extends IUnknown
{
	/**
	 * Initializes the object from a raw (TCLib) rangeset< range<T> > pointer.
	 */
	 InitFromRanges(pvRanges: object): void;
	/**
	 * Copies the object's raw (TCLib) range to the specified rangeset< range<T> > pointer.
	 */
	 CopyRangesTo(pvRanges: object): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventIDRange Interface

/**
 * IAGCEventIDRange Interface
 */
interface IAGCEventIDRange extends IDispatch
{
	/**
	 * Gets/sets the range as a string, formatted for display purposes.
	 */
	 DisplayString(bstr: string): void;
	/**
	 * Sets the lower and upper (open) ends of the range.
	 */
	 Init(lower: number, upper: number): void;
	/**
	 * READONLY: Gets the lower end of the range.
	 */
	readonly Lower: number;
	/**
	 * READONLY: Gets the upper (open) end of the range.
	 */
	readonly Upper: number;
	/**
	 * READONLY: Determines whether this range is empty (Lower equals Upper).
	 */
	readonly IsEmpty: boolean;
	/**
	 * Determines whether the specified value intersects with this range.
	 */
	 IntersectsWithValue(value: number): boolean;
	/**
	 * Determines whether the specified range intersects with this range.
	 */
	 IntersectsWithRangeValues(value1: number, value2: number): boolean;
	/**
	 * Determines whether the specified range intersects with this range.
	 */
	 IntersectsWithRange(pRange: IAGCEventIDRange): boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCEventIDRanges Interface

/**
 * IAGCEventIDRanges Interface
 */
interface IAGCEventIDRanges extends IDispatch
{
	/**
	 * READONLY: Returns the number of items in the collection.
	 */
	readonly Count: number;
	/**
	 * READONLY: Returns an enumerator object that implements IEnumVARIANT.
	 */
	readonly _NewEnum: IUnknown;
	/**
	 * Returns a AGCShip from the collection, or NULL if the item does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCEventIDRange;
	/**
	 * Gets/sets the range set as a string, formatted for display purposes.
	 */
	 DisplayString(bstr: string): void;
	/**
	 * Adds a range into the set, specified by the range's lower and upper bounds.
	 */
	 AddByValues(value1: number, value2: number): void;
	/**
	 * Adds a range into the set, specified by a range object.
	 */
	 Add(pRange: IAGCEventIDRange): void;
	/**
	 * Removes a range from the set, specified by the range's lower and upper bounds.
	 */
	 RemoveByValues(value1: number, value2: number): void;
	/**
	 * Removes a range from the set, specified by a range object.
	 */
	 Remove(pRange: IAGCEventIDRange): void;
	/**
	 * Removes all ranges from the set, specified by a range object.
	 */
	 RemoveAll(): void;
	/**
	 * Determines whether the specified value intersects with any range in this set.
	 */
	 IntersectsWithValue(value: number): boolean;
	/**
	 * Determines whether the specified range intersects with any range in this set.
	 */
	 IntersectsWithRangeValues(value1: number, value2: number): boolean;
	/**
	 * Determines whether the specified range intersects with any range in this set.
	 */
	 IntersectsWithRange(pRange: IAGCEventIDRange): boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ITCCollection Interface

/**
 * Base interface for most collection interfaces.
 */
interface ITCCollection extends IDispatch
{
	/**
	 * READONLY: Returns the number of items in the collection.
	 */
	readonly Count: number;
	/**
	 * READONLY: Returns an enumerator object that implements IEnumVARIANT.
	 */
	readonly _NewEnum: IUnknown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ITCStrings Interface

/**
 * Interface to a collection of strings.
 */
interface ITCStrings extends ITCCollection
{
	/**
	 * Returns a BSTR from the collection or NULL if the index is out of range. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): string;
	/**
	 * Adds the specified string to the collection.
	 */
	 Add(bstr: string): void;
	/**
	 * Removes the specified string from the collection.
	 */
	 Remove(pvIndex: object): void;
	/**
	 * Removes all strings from the collection.
	 */
	 RemoveAll(): void;
	/**
	 * Adds the specified delimited strings to the collection.
	 */
	 AddDelimited(bstrDelimiter: string, bstrStrings: string): void;
	/**
	 * Returns the entire array of strings as a single string, delimited by the specified delimiter string.
	 */
	 DelimitedItems(bstrDelimiter: string): string;
	/**
	 * Adds the strings of the specified collection.
	 */
	 AddStrings(pStrings: ITCStrings): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ITCPropBagOnRegKey Interface

/**
 * Interface to create a property bag on a registry key.
 */
interface ITCPropBagOnRegKey extends IDispatch
{
	/**
	 * Creates or opens the specified registry key.
	 */
	 CreateKey(bstrRegKey: string, bReadOnly: boolean): void;
	/**
	 * Opens the specified registry key.  Fails if the key does not exist.
	 */
	 OpenKey(bstrRegKey: string, bReadOnly: boolean): void;
	/**
	 * Creates the object that is stored on the current registry key.
	 */
	 CreateObject(): IUnknown;
	/**
	 * Creates the object, always on the local machine, that is stored on the current registry key.
	 */
	 CreateLocalObject(): IUnknown;
	/**
	 * Creates the object, on the specified server, that is stored on the current registry key.
	 */
	 CreateRemoteObject(bstrServer: string): IUnknown;
	/**
	 * Load the specified object from the values stored in the current registry key.
	 */
	 LoadObject(punkObj: IUnknown): void;
	/**
	 * Saves the specified object to the current registry key.
	 */
	 SaveObject(punkObj: IUnknown, bClearDirty: boolean, bSaveAllProperties: boolean, bSaveCreationInfo: boolean): void;
	/**
	 * Gets/sets the server on which the object stored on the current registry key will be created.
	 */
	 Server(bstrServer: string): void;
	/**
	 * READONLY: Gets the string representation of the CLSID of the object stored on the current registry key.
	 */
	readonly ObjectCLSID: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ITCSessionInfo Interface

/**
 * ITCSessionInfo Interface
 */
interface ITCSessionInfo extends IDispatch
{
	/**
	 * READONLY: Gets the user name of this session.
	 */
	readonly UserName: string;
	/**
	 * READONLY: Gets the computer name of this session.
	 */
	readonly ComputerName: string;
	/**
	 * Gets/sets the application name of this session.
	 */
	 ApplicationName(bstrAppName: string): void;
	/**
	 * READONLY: Returns the time the session was created.
	 */
	readonly TimeCreated: Date;
	/**
	 * READONLY: Returns the duration of the session.
	 */
	readonly Duration: Date;
	/**
	 * READONLY: Uniquely identifies this session within a process.
	 */
	readonly Cookie: number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ITCSessionInfos Interface

/**
 * ITCSessionInfos interface
 */
interface ITCSessionInfos extends ITCCollection
{
	/**
	 * Returns a Session from the collection, or NULL if the session does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(index: object): ITCSessionInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ITCCollectionPersistHelper Interface

/**
 * ITCCollectionPersistHelper interface
 */
interface ITCCollectionPersistHelper extends IDispatch
{
	/**
	 * Gets/sets the current thread priority.
	 */
	 Collection1(pvarSafeArray: object): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// ITCUtility Interface

/**
 * ITCUtility interface
 */
interface ITCUtility extends IDispatch
{
	/**
	 * Creates the specified object on the specified computer.
	 */
	 CreateObject(bstrProgID: string, bstrComputer: string): IUnknown;
	/**
	 * Create an object reference string.
	 */
	 ObjectReference(pUnk: IUnknown): string;
	/**
	 * Sleeps for the specified number of milliseconds.
	 */
	 Sleep(nDurationMS: number): void;
	/**
	 * Creates an object by binding the specified moniker display name.
	 */
	 GetObject(bstrMoniker: string, bAllowUI: boolean, nTimeoutMS: number): IUnknown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAdminSessionHelper Interface

/**
 * IAdminSessionHelper interface
 */
interface IAdminSessionHelper extends IDispatch
{
	/**
	 * READONLY: Indicates whether or not the Allegiance Game Server is currently running.
	 */
	readonly IsAllSrvRunning: boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCPrivate Interface

/**
 * Internally-used interface on an AGC object.
 */
interface IAGCPrivate extends IUnknown
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCCommon Interface

/**
 * IAGCCommon Interface
 */
interface IAGCCommon extends IDispatch
{
	/**
	 * READONLY: Every object in AGC has a unique id number and this is it.
	 */
	readonly Type: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCCollection Interface

/**
 * Base interface for AGC collection interfaces.
 */
interface IAGCCollection extends IAGCCommon
{
	/**
	 * READONLY: Returns the number of items in the collection.
	 */
	readonly Count: number;
	/**
	 * READONLY: Returns an enumerator object that implements IEnumVARIANT.
	 */
	readonly _NewEnum: IUnknown;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCBase Interface

/**
 * IAGCBase Interface
 */
interface IAGCBase extends IAGCCommon
{
	/**
	 * READONLY: 
	 */
	readonly ObjectType: Enums.AGCObjectType;
	/**
	 * READONLY: This returns the id of the object in terms of its type; use UniqueID() for a completely unique id.
	 */
	readonly ObjectID: AGCObjectID;
	/**
	 * READONLY: Returns the AGC game in which this object lives.
	 */
	readonly Game: IAGCGame;
	/**
	 * READONLY: Every object in AGC that has a base also has a unique id number and this is it.
	 */
	readonly UniqueID: AGCUniqueID;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCModel Interface

/**
 * IAGCModel Interface
 */
interface IAGCModel extends IAGCBase
{
	/**
	 * READONLY: 
	 */
	readonly IsVisible: boolean;
	/**
	 * 
	 */
	 IsSeenBySide(pTeam: IAGCTeam): boolean;
	/**
	 * READONLY: 
	 */
	readonly Position: IAGCVector;
	/**
	 * READONLY: 
	 */
	readonly Velocity: IAGCVector;
	/**
	 * READONLY: 
	 */
	readonly Orientation: IAGCOrientation;
	/**
	 * READONLY: 
	 */
	readonly Radius: number;
	/**
	 * READONLY: 
	 */
	readonly Team: IAGCTeam;
	/**
	 * READONLY: 
	 */
	readonly Mass: number;
	/**
	 * READONLY: 
	 */
	readonly Sector: IAGCSector;
	/**
	 * READONLY: 
	 */
	readonly Signature: number;
	/**
	 * READONLY: Gets the model's name
	 */
	readonly Name: string;
	/**
	 * READONLY: User can never pick the object
	 */
	readonly IsSelectable: boolean;
	/**
	 * READONLY: Can see other objects
	 */
	readonly IsScanner: boolean;
	/**
	 * READONLY: Send this object to a team when it is seen for the 1st time
	 */
	readonly IsPredictable: boolean;
	/**
	 * READONLY: Need to track its visibility with regard to side
	 */
	readonly IsScanRequired: boolean;
	/**
	 * READONLY: Can not move
	 */
	readonly IsStatic: boolean;
	/**
	 * READONLY: Can take damage
	 */
	readonly IsDamagable: boolean;
	/**
	 * READONLY: Goes into the collision KD tree
	 */
	readonly IsHitable: boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCModels Interface

/**
 * IAGCModels Interface
 */
interface IAGCModels extends IAGCCollection
{
	/**
	 * Returns an AGCModel from the collection, or NULL if the item does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCModel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCBuyable Interface

/**
 * Properties of an object that is buyable.
 */
interface IAGCBuyable extends IAGCBase
{
	/**
	 * READONLY: Gets the name of the object.
	 */
	readonly Name: string;
	/**
	 * READONLY: Gets the description of the object.
	 */
	readonly Description: string;
	/**
	 * READONLY: Gets the name of the model that buying this creates.
	 */
	readonly ModelName: string;
	/**
	 * READONLY: Gets price of the object.
	 */
	readonly Price: AGCMoney;
	/**
	 * READONLY: Gets time to build the object.
	 */
	readonly TimeToBuild: number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCHullType Interface

/**
 * Interface to an AGC Hull Type object.
 */
interface IAGCHullType extends IAGCBuyable
{
	/**
	 * READONLY: Gets the Length.
	 */
	readonly Length: number;
	/**
	 * READONLY: Gets the MaxSpeed.
	 */
	readonly MaxSpeed: number;
	/**
	 * Gets the MaxTurnRate.
	 */
	 MaxTurnRate(eAxis: Enums.AGCAxis): number;
	/**
	 * Gets the TurnTorque.
	 */
	 TurnTorque(eAxis: Enums.AGCAxis): number;
	/**
	 * READONLY: Gets the Thrust.
	 */
	readonly Thrust: number;
	/**
	 * READONLY: Gets the SideMultiplier.
	 */
	readonly SideMultiplier: number;
	/**
	 * READONLY: Gets the BackMultiplier.
	 */
	readonly BackMultiplier: number;
	/**
	 * READONLY: Gets the ScannerRange.
	 */
	readonly ScannerRange: number;
	/**
	 * READONLY: Gets the MaxEnergy.
	 */
	readonly MaxEnergy: number;
	/**
	 * READONLY: Gets the RechargeRate.
	 */
	readonly RechargeRate: number;
	/**
	 * READONLY: Gets the HitPoints.
	 */
	readonly HitPoints: AGCHitPoints;
	/**
	 * Gets the PartMask.
	 */
	 PartMask(et: Enums.AGCEquipmentType, mountID: AGCMount): AGCPartMask;
	/**
	 * READONLY: Gets the MaxWeapons.
	 */
	readonly MaxWeapons: AGCMount;
	/**
	 * READONLY: Gets the MaxFixedWeapons.
	 */
	readonly MaxFixedWeapons: AGCMount;
	/**
	 * READONLY: Gets the CanMount.
	 */
	readonly Mass: number;
	/**
	 * READONLY: Gets the Signature.
	 */
	readonly Signature: number;
	/**
	 * READONLY: Gets the Capabilities.
	 */
	readonly Capabilities: AGCHullAbilityBitMask;
	/**
	 * Determines if the object has the specified capabilities..
	 */
	 HasCapability(habm: AGCHullAbilityBitMask): boolean;
	/**
	 * READONLY: Gets the MaxAmmo.
	 */
	readonly MaxAmmo: number;
	/**
	 * READONLY: Gets the MaxFuel.
	 */
	readonly MaxFuel: number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCTreasure Interface

/**
 * 
 */
interface IAGCTreasure extends IAGCModel
{
	/**
	 * READONLY: 
	 */
	readonly Type: Enums.AGCTreasureType;
	/**
	 * READONLY: 
	 */
	readonly Amount: number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCTreasures Interface

/**
 * IAGCTreasures Interface
 */
interface IAGCTreasures extends IAGCCollection
{
	/**
	 * Returns an AGCTreasure from the collection, or NULL if the game does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCTreasure;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCDamage Interface

/**
 * IAGCDamage Interface
 */
interface IAGCDamage extends IAGCModel
{
	/**
	 * READONLY: 
	 */
	readonly Fraction: number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCScanner Interface

/**
 * IAGCScanner Interface
 */
interface IAGCScanner extends IAGCDamage
{
	/**
	 * 
	 */
	 InScannerRange(pModel: IAGCModel): boolean;
	/**
	 * 
	 */
	 CanSee(pModel: IAGCModel): boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCProbe Interface

/**
 * IAGCProbe Interface
 */
interface IAGCProbe extends IAGCScanner
{
	/**
	 * READONLY: Gets the type of projectiles fired, if any.
	 */
	readonly EmissionPoint: IAGCVector;
	/**
	 * READONLY: Gets the lifespan.
	 */
	readonly Lifespan: number;
	/**
	 * READONLY: Gets the weapon burst rate(?).
	 */
	readonly DtBurst: number;
	/**
	 * READONLY: Gets the weapon firing accuracy.
	 */
	readonly Accuracy: number;
	/**
	 * READONLY: Gets the indicator of the probe being a ripcord destination.
	 */
	readonly IsRipcord: boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCProbes Interface

/**
 * IAGCProbes Interface
 */
interface IAGCProbes extends IAGCCollection
{
	/**
	 * Returns an AGCProbe from the collection, or NULL if the item does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCProbe;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCGame Interface

/**
 * IAGCGame Interface
 */
interface IAGCGame extends IAGCCommon
{
	/**
	 * READONLY: Gets the game's name
	 */
	readonly Name: string;
	/**
	 * READONLY: Gets the collection of sectors in the game.
	 */
	readonly Sectors: IAGCSectors;
	/**
	 * READONLY: Gets the collection of teams in the game.
	 */
	readonly Teams: IAGCTeams;
	/**
	 * Gets the Ship associated with specified AGC ID.
	 */
	 LookupShip(idAGC: AGCUniqueID): IAGCShip;
	/**
	 * READONLY: Gets the collection of the ships in the game.
	 */
	readonly Ships: IAGCShips;
	/**
	 * READONLY: Gets the collection of alephs in the game.
	 */
	readonly Alephs: IAGCAlephs;
	/**
	 * READONLY: Gets the collection of asteroids in the game.
	 */
	readonly Asteroids: IAGCAsteroids;
	/**
	 * READONLY: Gets the parameters used to create the game.
	 */
	readonly GameParameters: IAGCGameParameters;
	/**
	 * READONLY: Gets unique mission id.
	 */
	readonly GameID: AGCGameID;
	/**
	 * Gets the team associated with specified AGC ID.
	 */
	 LookupTeam(idAGC: AGCObjectID): IAGCTeam;
	/**
	 * Sends the specified chat text to everyone in the Game.
	 */
	 SendChat(bstrText: string, idSound: AGCSoundID): void;
	/**
	 * Sends the specified command to everyone in the Game.
	 */
	 SendCommand(bstrCommand: string, pModelTarget: IAGCModel, idSound: AGCSoundID): void;
	/**
	 * READONLY: Gets the stage of the game.
	 */
	readonly GameStage: Enums.AGCGameStage;
	/**
	 * READONLY: Gets the collection of probes in the game.
	 */
	readonly Probes: IAGCProbes;
	/**
	 * READONLY: Gets the collection of buoys in the game.
	 */
	readonly Buoys: IAGCModels;
	/**
	 * READONLY: Gets the collection of treasures in the game.
	 */
	readonly Treasures: IAGCModels;
	/**
	 * READONLY: Gets the collection of mines in the game.
	 */
	readonly Mines: IAGCModels;
	/**
	 * READONLY: Gets the number of times this game has been 'reused'.
	 */
	readonly ReplayCount: number;
	/**
	 * READONLY: Gets a string that uniquely identifies the game context, within the game server.
	 */
	readonly ContextName: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCShip Interface

/**
 * IAGCShip Interface
 */
interface IAGCShip extends IAGCScanner
{
	/**
	 * Amount of ammo the ship has.
	 */
	 Ammo(Val: number): void;
	/**
	 * Amount of fuel the ship has.
	 */
	 Fuel(Val: number): void;
	/**
	 * Amount of energy the ship has.
	 */
	 Energy(Val: number): void;
	/**
	 * Ship's Wing ID.
	 */
	 WingID(Val: number): void;
	/**
	 * Sends the specified chat text to the ship.
	 */
	 SendChat(bstrText: string, idSound: AGCSoundID): void;
	/**
	 * Sends the specified command to the ship.
	 */
	 SendCommand(bstrCommand: string, pModelTarget: IAGCModel, idSound: AGCSoundID): void;
	/**
	 * Gets/sets the AutoDonate ship.
	 */
	 AutoDonate(pShip: IAGCShip): void;
	/**
	 * Gets/sets the ship's ShieldFraction
	 */
	 ShieldFraction(pVal: number): void;
	/**
	 * READONLY: Gets the ship's Hull type.
	 */
	readonly HullType: IAGCHullType;
	/**
	 * READONLY: Gets the ship's base hull type.
	 */
	readonly BaseHullType: IAGCHullType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCShips Interface

/**
 * IAGCShips Interface
 */
interface IAGCShips extends IAGCCollection
{
	/**
	 * Returns a AGCShip from the collection, or NULL if the item does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCShip;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCStation Interface

/**
 * IAGCStation Interface
 */
interface IAGCStation extends IAGCScanner
{
	/**
	 * Gets/sets the station's ShieldFraction
	 */
	 ShieldFraction(pVal: number): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCStations Interface

/**
 * IAGCStations Interface
 */
interface IAGCStations extends IAGCCollection
{
	/**
	 * Returns a AGCStation from the collection, or NULL if the item does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCStation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCAleph Interface

/**
 * IAGCAleph Interface
 */
interface IAGCAleph extends IAGCModel
{
	/**
	 * READONLY: Gets the destination aleph.
	 */
	readonly Destination: IAGCAleph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCAlephs Interface

/**
 * IAGCAlephs Interface
 */
interface IAGCAlephs extends IAGCCollection
{
	/**
	 * Returns a AGCAleph from the collection, or NULL if the AGCAleph does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCAleph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCSector Interface

/**
 * IAGCSector Interface
 */
interface IAGCSector extends IAGCBase
{
	/**
	 * READONLY: Gets the AGCSector's name
	 */
	readonly Name: string;
	/**
	 * Gets the collection of stations in the sector.
	 */
	Stations: IAGCStations;
	/**
	 * READONLY: Gets the collection of ships in the sector.
	 */
	readonly Ships: IAGCShips;
	/**
	 * READONLY: Gets the collection of alephs in the sector.
	 */
	readonly Alephs: IAGCAlephs;
	/**
	 * READONLY: Gets the collection of asteroids in the sector.
	 */
	readonly Asteroids: IAGCAsteroids;
	/**
	 * Sends the specified chat text to everyone in the Sector.
	 */
	 SendChat(bstrText: string, bIncludeEnemies: boolean, idSound: AGCSoundID): void;
	/**
	 * Sends the specified command to everyone in the Sector.
	 */
	 SendCommand(bstrCommand: string, pModelTarget: IAGCModel, bIncludeEnemies: boolean, idSound: AGCSoundID): void;
	/**
	 * READONLY: Gets sector's X position relative to other sectors.  This is used in the sector overview display.  This value never changes.
	 */
	readonly ScreenX: number;
	/**
	 * READONLY: Gets sector's Y position relative to other sectors.  This is used in the sector overview display.  This value never changes.
	 */
	readonly ScreenY: number;
	/**
	 * READONLY: Gets the collection of mines in the sector.
	 */
	readonly Mines: IAGCModels;
	/**
	 * READONLY: Gets the collection of missiles in the sector.
	 */
	readonly Missiles: IAGCModels;
	/**
	 * READONLY: Gets the collection of probes in the sector.
	 */
	readonly Probes: IAGCProbes;
	/**
	 * READONLY: Gets the collection of models in the sector.
	 */
	readonly Models: IAGCModels;
	/**
	 * READONLY: Gets the collection of selectable models in the sector.
	 */
	readonly SelectableModels: IAGCModels;
	/**
	 * READONLY: Gets the collection of treasures in the sector.
	 */
	readonly Treasures: IAGCModels;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCSectors Interface

/**
 * IAGCSectors Interface
 */
interface IAGCSectors extends IAGCCollection
{
	/**
	 * Returns a AGCSector from the collection, or NULL if the sector does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCSector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCTeam Interface

/**
 * IAGCTeam Interface
 */
interface IAGCTeam extends IAGCBase
{
	/**
	 * READONLY: Gets the team's name
	 */
	readonly Name: string;
	/**
	 * Returns a collection of stations that belong to the team.
	 */
	Stations: IAGCStations;
	/**
	 * READONLY: Returns a Ships collection of the ships in the team.
	 */
	readonly Ships: IAGCShips;
	/**
	 * READONLY: Returns name of civ for team.
	 */
	readonly Civ: string;
	/**
	 * Sends the specified chat text to everyone on the Team.
	 */
	 SendChat(bstrText: string, idWing: number, idSound: AGCSoundID): void;
	/**
	 * Sends the specified command to everyone on the Team.
	 */
	 SendCommand(bstrCommand: string, pModelTarget: IAGCModel, idWing: number, idSound: AGCSoundID): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCTeams Interface

/**
 * IAGCTeams Interface
 */
interface IAGCTeams extends IAGCCollection
{
	/**
	 * Returns a AGCTeam from the collection, or NULL if the game does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCTeam;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCAsteroid Interface

/**
 * IAGCAsteroid Interface
 */
interface IAGCAsteroid extends IAGCDamage
{
	/**
	 * READONLY: Gets the amount of ore remaining on the asteroid.
	 */
	readonly Ore: number;
	/**
	 * READONLY: Gets the Capabilities bit mask of the object.
	 */
	readonly Capabilities: AGCAsteroidAbilityBitMask;
	/**
	 * Determines if the object has the specified capabilities.
	 */
	 HasCapability(aabm: AGCAsteroidAbilityBitMask): boolean;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IAGCAsteroids Interface

/**
 * IAGCAsteroids Interface
 */
interface IAGCAsteroids extends IAGCCollection
{
	/**
	 * Returns an AGCAsteroid from the collection, or NULL if the game does not exist. Takes an argument, index, which must be the index into the collection.
	 */
	 Item(pvIndex: object): IAGCAsteroid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigBehaviorType Interface

/**
 * Interface to a pig behavior type object.
 */
interface IPigBehaviorType extends IDispatch
{
	/**
	 * READONLY: Gets the collection of command strings that will invoke this behavior.
	 */
	readonly InvokeCommands: ITCStrings;
	/**
	 * READONLY: Gets the name of this behavior type.
	 */
	readonly Name: string;
	/**
	 * READONLY: Gets the description of this behavior type.
	 */
	readonly Description: string;
	/**
	 * READONLY: Gets the collection of pig behavior objects currently referencing this behavior type.
	 */
	readonly Behaviors: IPigBehaviors;
	/**
	 * READONLY: Gets the indicator of whether or not this object is based on a script or not.
	 */
	readonly IsScript: boolean;
	/**
	 * READONLY: Gets the behavior type that serves as this one's base class.
	 */
	readonly BaseBehaviorType: IPigBehaviorType;
	/**
	 * Gets/sets the flag indicating whether or not the script has encountered an error yet.
	 */
	 AppearsValid(bAppearsValid: boolean): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigBehaviorTypes Interface

/**
 * Interface to a collection of pig behavior script objects.
 */
interface IPigBehaviorTypes extends ITCCollection
{
	/**
	 * Gets an item from the collection. Takes an argument, index, that is either the 0-relative index into the collection, the name of the behavior, or one of the invoke commands of the behavior.
	 */
	 Item(pvIndex: object): IPigBehaviorType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigBehaviorScriptType Interface

/**
 * Interface to a pig behavior script object.
 */
interface IPigBehaviorScriptType extends IPigBehaviorType
{
	/**
	 * READONLY: Gets the path and name of the script file that defines this behavior.
	 */
	readonly FilePathName: string;
	/**
	 * READONLY: Gets the name of the script file that defines this behavior.
	 */
	readonly FileName: string;
	/**
	 * READONLY: Gets the time (UTC) that the script file was last modified.
	 */
	readonly FileModifiedTime: Date;
	/**
	 * READONLY: Gets the attributes of the script file.
	 */
	readonly FileAttributes: Enums.FileAttributes;
	/**
	 * READONLY: Gets the text of the entire script file.
	 */
	readonly FileText: string;
	/**
	 * READONLY: Gets the text of the script sections within the file.
	 */
	readonly ScriptText: ITCStrings;
	/**
	 * READONLY: Gets the Prog ID of the Active Scripting engine used to parse/execute this script.
	 */
	readonly ScriptEngineProgID: string;
	/**
	 * READONLY: Gets the CLSID of the Active Scripting engine used to parse/execute this script.
	 */
	readonly ScriptEngineCLSID: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigBehaviorHost Interface

/**
 * Interface to a pig behavior host object.
 */
interface IPigBehaviorHost extends IDispatch
{
	/**
	 * Useful test method that beeps as specified.
	 */
	 Beep(nFrequency: number, nDuration: number): void;
	/**
	 * Creates a new pig object with the specified behavior type.
	 */
	 CreatePig(bstrType: string, bstrCommandLine: string): IPig;
	/**
	 * Useful test method that echoes the specified text to the debugger output.
	 */
	 Trace(bstrText: string): void;
	/**
	 * Gets the textual name of the specified pig state.
	 */
	 StateName(ePigState: Enums.PigState): string;
	/**
	 * Generates a random integer between 0 and 32,767.
	 */
	 Random(): number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigBehavior Interface

/**
 * Interface to a pig behavior object.
 */
interface IPigBehavior extends IDispatch
{
	/**
	 * READONLY: Gets the object that describes this class of behavior.
	 */
	readonly BehaviorType: IPigBehaviorType;
	/**
	 * READONLY: Get the behavior, if any, that serve's as the this one's base class.
	 */
	readonly BaseBehavior: IDispatch;
	/**
	 * READONLY: Gets the pig player object using this behavior instance.
	 */
	readonly Pig: IPig;
	/**
	 * READONLY: Gets the object used to access the rest of the Pigs object model.
	 */
	readonly Host: IPigBehaviorHost;
	/**
	 * READONLY: Gets the indicator of whether or not this behaior is the one currently active for the pig player object.
	 */
	readonly IsActive: boolean;
	/**
	 * READONLY: Gets the dictionary containing the behavior-defined properties.
	 */
	readonly Properties: IDictionary;
	/**
	 * READONLY: Gets the timer for which the event expression is currently executing, if any
	 */
	readonly Timer: IPigTimer;
	/**
	 * READONLY: Gets the command line text used to activate the behvaior, if any.
	 */
	readonly CommandLine: string;
	/**
	 * Creates a new timer object.
	 */
	 CreateTimer(fInterval: number, bstrEventExpression: string, nRepetitions: number, bstrName: string): IPigTimer;
	/**
	 * Called when this behavior becomes a pig's active behavior.
	 */
	 OnActivate(pPigDeactived: IPigBehavior): void;
	/**
	 * Called when this behavior is deactivated as a pig's active behavior.
	 */
	 OnDeactivate(pPigActivated: IPigBehavior): void;
	/**
	 * Called when the state of the pig has changed to PigState_NonExistant.
	 */
	 OnStateNonExistant(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_LoggingOn.
	 */
	 OnStateLoggingOn(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_LoggingOff.
	 */
	 OnStateLoggingOff(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_MissionList.
	 */
	 OnStateMissionList(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_CreatingMission.
	 */
	 OnStateCreatingMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_JoiningMission.
	 */
	 OnStateJoiningMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_QuittingMission.
	 */
	 OnStateQuittingMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_TeamList.
	 */
	 OnStateTeamList(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_JoiningTeam.
	 */
	 OnStateJoiningTeam(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_WaitingForMission.
	 */
	 OnStateWaitingForMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Docked.
	 */
	 OnStateDocked(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Launching.
	 */
	 OnStateLaunching(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Flying.
	 */
	 OnStateFlying(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Terminated.
	 */
	 OnStateTerminated(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the Pig's current mission begins.
	 */
	 OnMissionStarted(): void;
	/**
	 * Called when the Pig receives chat text.
	 */
	 OnReceiveChat(bstrText: string, pShipSender: IAGCShip): boolean;
	/**
	 * Called when any ship takes damage.
	 */
	 OnShipDamaged(pShip: IAGCShip, pModelLauncher: IAGCModel, fAmount: number, fLeakage: number, pVector1: IAGCVector, pVector2: IAGCVector): void;
	/**
	 * Called when any ship is killed.
	 */
	 OnShipKilled(pShip: IAGCShip, pModelLauncher: IAGCModel, fAmount: number, pVector1: IAGCVector, pVector2: IAGCVector): void;
	/**
	 * Called when the Pig ship is transported to a new Sector.
	 */
	 OnSectorChange(pSectorOld: IAGCSector, pSectorNew: IAGCSector): void;
	/**
	 * Called when the Pig ship hits an Aleph.
	 */
	 OnAlephHit(pAleph: IAGCAleph): void;
}


/**                
 * These are global objects that are injected by COM into the javascript space. Both IPig and IPigBehavior are available, 
 * but because you can get a Pig object from IPigBehavior, I am not going to make constants for those here. 
 */

	/**
	 * READONLY: Gets the object that describes this class of behavior.
	 */
	declare const BehaviorType: IPigBehaviorType;
	/**
	 * READONLY: Get the behavior, if any, that serve's as the this one's base class.
	 */
	declare const BaseBehavior: IDispatch;
	/**
	 * READONLY: Gets the pig player object using this behavior instance.
	 */
	declare const Pig: IPig;
	/**
	 * READONLY: Gets the object used to access the rest of the Pigs object model.
	 */
	declare const Host: IPigBehaviorHost;
	/**
	 * READONLY: Gets the indicator of whether or not this behaior is the one currently active for the pig player object.
	 */
	declare const IsActive: boolean;
	/**
	 * READONLY: Gets the dictionary containing the behavior-defined properties.
	 */
	declare const Properties: IDictionary;
	/**
	 * READONLY: Gets the timer for which the event expression is currently executing, if any
	 */
	declare const Timer: IPigTimer;
	/**
	 * READONLY: Gets the command line text used to activate the behvaior, if any.
	 */
	declare const CommandLine: string;
	/**
	 * Creates a new timer object.
	 */
	declare function CreateTimer(fInterval: number, bstrEventExpression: string, nRepetitions: number, bstrName: string): IPigTimer;
	/**
	 * Called when this behavior becomes a pig's active behavior.
	 */
	declare function OnActivate(pPigDeactived: IPigBehavior): void;
	/**
	 * Called when this behavior is deactivated as a pig's active behavior.
	 */
	declare function OnDeactivate(pPigActivated: IPigBehavior): void;
	/**
	 * Called when the state of the pig has changed to PigState_NonExistant.
	 */
	declare function OnStateNonExistant(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_LoggingOn.
	 */
	declare function OnStateLoggingOn(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_LoggingOff.
	 */
	declare function OnStateLoggingOff(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_MissionList.
	 */
	declare function OnStateMissionList(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_CreatingMission.
	 */
	declare function OnStateCreatingMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_JoiningMission.
	 */
	declare function OnStateJoiningMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_QuittingMission.
	 */
	declare function OnStateQuittingMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_TeamList.
	 */
	declare function OnStateTeamList(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_JoiningTeam.
	 */
	declare function OnStateJoiningTeam(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_WaitingForMission.
	 */
	declare function OnStateWaitingForMission(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Docked.
	 */
	declare function OnStateDocked(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Launching.
	 */
	declare function OnStateLaunching(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Flying.
	 */
	declare function OnStateFlying(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the state of the pig has changed to PigState_Terminated.
	 */
	declare function OnStateTerminated(eStatePrevious: Enums.PigState): void;
	/**
	 * Called when the Pig's current mission begins.
	 */
	declare function OnMissionStarted(): void;
	/**
	 * Called when the Pig receives chat text.
	 */
	declare function OnReceiveChat(bstrText: string, pShipSender: IAGCShip): boolean;
	/**
	 * Called when any ship takes damage.
	 */
	declare function OnShipDamaged(pShip: IAGCShip, pModelLauncher: IAGCModel, fAmount: number, fLeakage: number, pVector1: IAGCVector, pVector2: IAGCVector): void;
	/**
	 * Called when any ship is killed.
	 */
	declare function OnShipKilled(pShip: IAGCShip, pModelLauncher: IAGCModel, fAmount: number, pVector1: IAGCVector, pVector2: IAGCVector): void;
	/**
	 * Called when the Pig ship is transported to a new Sector.
	 */
	declare function OnSectorChange(pSectorOld: IAGCSector, pSectorNew: IAGCSector): void;
	/**
	 * Called when the Pig ship hits an Aleph.
	 */
	declare function OnAlephHit(pAleph: IAGCAleph): void;
////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigBehaviors Interface

/**
 * Interface to a collection of pig behavior objects.
 */
interface IPigBehaviors extends ITCCollection
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigBehaviorStack Interface

/**
 * Interface to a stack of pig behavior objects.
 */
interface IPigBehaviorStack extends ITCCollection
{
	/**
	 * Creates a new behavior of the specified type and pushes it onto the top of the behavior stack.
	 */
	 Push(bstrType: string, bstrCommandLine: string): IPigBehavior;
	/**
	 * Pops the top-most behavior off the stack, unless there is only one behavior remaining on the stack.
	 */
	 Pop(): void;
	/**
	 * Gets a reference to a behavior on the stack without removing it from the stack.
	 */
	 Top(nFromTop: number): IPigBehavior;
	/**
	 * READONLY: Gets the pig object which owns this behavior stack.
	 */
	readonly Pig: IPig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPig Interface

/**
 * Interface to a pig player object.
 */
interface IPig extends IDispatch
{
	/**
	 * READONLY: Gets the current state of the pig.
	 */
	readonly PigState: Enums.PigState;
	/**
	 * READONLY: Gets the textual name of the state of the pig.
	 */
	readonly PigStateName: string;
	/**
	 * READONLY: Gets the stack of behaviors associated with this pig.
	 */
	readonly Stack: IPigBehaviorStack;
	/**
	 * READONLY: Gets the character name of the pig.
	 */
	readonly Name: string;
	/**
	 * READONLY: Gets the ship object of the pig.
	 */
	readonly Ship: IPigShip;
	/**
	 * Gets/sets the number of times per second that the IGC layer is updated.
	 */
	 UpdatesPerSecond(nPerSecond: number): void;
	/**
	 * READONLY: Gets the collection of hull types available to this pig.
	 */
	readonly HullTypes: IPigHullTypes;
	/**
	 * READONLY: Gets this pig's liquid worth.
	 */
	readonly Money: AGCMoney;
	/**
	 * Logs on to the mission server.
	 */
	 Logon(): void;
	/**
	 * Logs off from the lobby server.
	 */
	 Logoff(): void;
	/**
	 * Creates (and joins) a new mission with the specified parameters.
	 */
	 CreateMission(bstrServerName: string, bstrServerAddr: string, pMissionParams: IPigMissionParams): void;
	/**
	 * Joins a mission, which may be specified by name, by a buddy player, or not at all.
	 */
	 JoinMission(bstrMissionOrPlayer: string): void;
	/**
	 * Requests a position on a team, which may be specified by name, by a buddy player, or not at all.
	 */
	 JoinTeam(bstrCivName: string, bstrTeamOrPlayer: string): void;
	/**
	 * Quits the current game.
	 */
	 QuitGame(): void;
	/**
	 * Undocks from the station and enters space.
	 */
	 Launch(): void;
	/**
	 * Forces the pig to exit the game.
	 */
	 Shutdown(): void;
	/**
	 * Returns true if player has game control
	 */
	 IsMissionOwner(): boolean;
	/**
	 * Returns true if player is the team leader
	 */
	 IsTeamLeader(): boolean;
	/**
	 * Sets the shoot, turn and goto skills
	 */
	 SetSkills(fShoot: number, fTurn: number, fGoto: number): void;
	/**
	 * READONLY: Gets the AGCGame object that the Pig is a member of, if any.
	 */
	readonly Game: IAGCGame;
	/**
	 * READONLY: Gets the AGCShip object that sent the current chat. NULL if chat was from HQ or Pig is not currently processing chat text.
	 */
	readonly Me: IAGCShip;
	/**
	 * READONLY: Gets the target of the current chat. AGCChat_NoSelection if Pig is not currently processing chat text.
	 */
	readonly ChatTarget: Enums.AGCChatTarget;
	/**
	 * READONLY: Gets the object used to access the rest of the Pigs object model.
	 */
	readonly Host: IPigBehaviorHost;
	/**
	 * Sets mission params then starts the game.
	 */
	 StartCustomGame(pMissionParams: IPigMissionParams): void;
	/**
	 * Starts the game.
	 */
	 StartGame(): void;
	/**
	 * Contains a threshold value, representing an angle in radians, to which each ship's movement is compared.
	 */
	 ShipAngleThreshold1(rRadians: number): void;
	/**
	 * Contains a threshold value, representing an angle in radians, to which each ship's movement is compared.
	 */
	 ShipAngleThreshold2(rRadians: number): void;
	/**
	 * Contains a threshold value, representing an angle in radians, to which each ship's movement is compared.
	 */
	 ShipAngleThreshold3(rRadians: number): void;
	/**
	 * Contains a threshold value, representing a distance, to which each ship's movement is compared.
	 */
	 ShipDistanceThreshold1(rDistance: number): void;
	/**
	 * Contains a threshold value, representing a distance, to which each ship's movement is compared.
	 */
	 ShipDistanceThreshold2(rDistance: number): void;
	/**
	 * Contains a threshold value, representing a distance, to which each ship's movement is compared.
	 */
	 ShipDistanceThreshold3(rDistance: number): void;
	/**
	 * Contains a threshold value, representing ZTime, to which the (heavy_update_time - client_time) is compared.
	 */
	 ShipsUpdateLatencyThreshold1(nMilliseconds: number): void;
	/**
	 * Contains a threshold value, representing ZTime, to which the (heavy_update_time - client_time) is compared.
	 */
	 ShipsUpdateLatencyThreshold2(nMilliseconds: number): void;
	/**
	 * Contains a threshold value, representing ZTime, to which the (heavy_update_time - client_time) is compared.
	 */
	 ShipsUpdateLatencyThreshold3(nMilliseconds: number): void;
	/**
	 * Sends the bytes specified to the connected lobby server, if any. pvBytes is either the name of a file, or a SAFEARRAY of bytes.
	 */
	 SendBytesToLobby(pvBytes: object, bGuaranteed: boolean): void;
	/**
	 * Sends the bytes specified to the connected game server, if any. pvBytes is either the name of a file, or a SAFEARRAY of bytes.
	 */
	 SendBytesToGame(pvBytes: object, bGuaranteed: boolean): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigs Interface

/**
 * Interface to a collection of pig player objects.
 */
interface IPigs extends ITCCollection
{
	/**
	 * Returns an IPig* from the collection or NULL if the specified named pig does not exist in the collection. Takes an argument, index, which must be the name of a pig in the collection.
	 */
	 Item(pvIndex: object): IPig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigTeam Interface

/**
 * Interface to a Team object.
 */
interface IPigTeam extends IDispatch
{
	/**
	 * READONLY: Gets the name of the team.
	 */
	readonly Name: string;
	/**
	 * READONLY: Gets/sets the civilization of the team.
	 */
	readonly PlayersMaximum: number;
	/**
	 * READONLY: Gets the current number of players on this team.
	 */
	readonly PlayerCount: number;
	/**
	 * Gets the collection of players on this team.
	 */
	 IsAutoAccept(bAutoAccept: boolean): void;
	/**
	 * Gets/sets the 'ready' flag of this team.
	 */
	 IsReady(bReady: boolean): void;
	/**
	 * Gets/sets the 'force-ready' flag of this team.
	 */
	 IsForceReady(bForceReady: boolean): void;
	/**
	 * Gets/sets the 'active' flag of this team.
	 */
	 IsActive(bActive: boolean): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigTeams Interface

/**
 * Interface to a collection of Team objects.
 */
interface IPigTeams extends ITCCollection
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigMissionParams Interface

/**
 * Interface to a MissionParams object.
 */
interface IPigMissionParams extends IDispatch
{
	/**
	 * Validates the properties of the object.
	 */
	 Validate(): void;
	/**
	 * Gets/sets the number of teams for the mission.
	 */
	 TeamCount(nTeamCount: number): void;
	/**
	 * Gets/sets the maximum number of players allowed on a team.
	 */
	 MaxTeamPlayers(nMaxTeamPlayers: number): void;
	/**
	 * Gets/sets the minimum number of players allowed on a team.
	 */
	 MinTeamPlayers(nMinTeamPlayers: number): void;
	/**
	 * Gets/sets the map type for the mission.
	 */
	 MapType(eMapType: Enums.PigMapType): void;
	/**
	 * Gets/sets the death match goal.
	 */
	 TeamKills(nGoalTeamKills: number): void;
	/**
	 * Gets/sets the game name for the mission.
	 */
	 GameName(bstrGameName: string): void;
	/**
	 * Gets/sets the core for the mission.
	 */
	 CoreName(bstrCoreName: string): void;
	/**
	 * Gets/sets the KB Level.
	 */
	 KillBonus(KBLevel: number): void;
	/**
	 * Gets/sets Defections allowed.
	 */
	 Defections(Defections: boolean): void;
	/**
	 * Gets/sets the number of starting miners.
	 */
	 Miners(Miners: number): void;
	/**
	 * Gets/sets Developments allowed.
	 */
	 Developments(Developments: boolean): void;
	/**
	 * Gets/sets the Conquest goal.
	 */
	 Conquest(Conquest: number): void;
	/**
	 * Gets/sets the Flags goal.
	 */
	 Flags(Flags: number): void;
	/**
	 * Gets/sets the Artifacts goal.
	 */
	 Artifacts(Artifacts: number): void;
	/**
	 * Gets/sets Pods allowed.
	 */
	 Pods(Pods: boolean): void;
	/**
	 * Gets/sets Experimental allowed.
	 */
	 Experimental(Experimental: boolean): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigMissionParamsPrivate Interface

/**
 * Private interface to a MissionParams object.
 */
interface IPigMissionParamsPrivate extends IUnknown
{
	/**
	 * Gets a stream containing the length-prefixed data structure.
	 */
	 GetData(): IStream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigMission Interface

/**
 * Interface to a Mission object.
 */
interface IPigMission extends IDispatch
{
	/**
	 * READONLY: Gets the collection of (pig) players.
	 */
	readonly Pigs: IPigs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigMissions Interface

/**
 * Interface to a collection of Mission objects.
 */
interface IPigMissions extends ITCCollection
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigLobby Interface

/**
 * Interface to a Lobby object.
 */
interface IPigLobby extends IDispatch
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigLobbies Interface

/**
 * Interface to a collection of Lobby objects.
 */
interface IPigLobbies extends ITCCollection
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigSession Interface

/**
 * IPigSession Interface
 */
interface IPigSession extends IDispatch
{
	/**
	 * Gets/sets the session information object.
	 */
	 SessionInfo(pSessionInfo: ITCSessionInfo): void;
	/**
	 * READONLY: Gets the collection of connected sessions.
	 */
	readonly SessionInfos: ITCSessionInfos;
	/**
	 * Gets/sets the directory for script files.
	 */
	 ScriptDir(bstrScriptDir: string): void;
	/**
	 * Gets/sets the directory for artwork files.
	 */
	 ArtPath(bstrArtPath: string): void;
	/**
	 * READONLY: Gets the collection of behavior scripts.
	 */
	readonly BehaviorTypes: IPigBehaviorTypes;
	/**
	 * READONLY: Gets the collection of game lobby servers.
	 */
	readonly Lobbies: IPigLobbies;
	/**
	 * READONLY: Gets the collection of all (pig) players.
	 */
	readonly Pigs: IPigs;
	/**
	 * Creates a new pig object with the specified behavior type.
	 */
	 CreatePig(bstrType: string, bstrCommandLine: string): IPig;
	/**
	 * Gets/sets the server on which all pigs will be created.
	 */
	 MissionServer(bstrServer: string): void;
	/**
	 * Gets/sets maximum number of pigs that can be created on this server.
	 */
	 MaxPigs(nMaxPigs: number): void;
	/**
	 * READONLY: Gets the object that dispenses pig accounts.
	 */
	readonly AccountDispenser: IPigAccountDispenser;
	/**
	 * READONLY: Gets the version information of the Pig server.
	 */
	readonly Version: IAGCVersionInfo;
	/**
	 * READONLY: Gets the process ID of the server, meaningful only on the local machine.
	 */
	readonly ProcessID: number;
	/**
	 * Enables the firing of all available events from this session.
	 */
	 ActivateAllEvents(): void;
	/**
	 * Disables the firing of all available events from this session.
	 */
	 DeactivateAllEvents(): void;
	/**
	 * READONLY: Gets the event log object.
	 */
	readonly EventLog: IAGCEventLogger;
	/**
	 * Contains the server from which pig accounts will be dispensed. When empty, the MissionServer is used. When this property is a period character, the local machine is used.
	 */
	 AccountServer(bstrServer: string): void;
	/**
	 * Contains the server on which the pig accounts will be authenticated. When empty, no authentication is performed.
	 */
	 ZoneAuthServer(bstrServer: string): void;
	/**
	 * Contains the maximum amount of time allowed for authentication of pig accounts. Ignored when ZoneAuthServer is an empty string.
	 */
	 ZoneAuthTimeout(nMilliseconds: number): void;
	/**
	 * Specifies the mode of lobby connection that is used for server connections. Affects the usage of the MissionServer property.
	 */
	 LobbyMode(eMode: Enums.PigLobbyMode): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigSessionEvents Interface

/**
 * IPigSessionEvents Interface
 */
interface IPigSessionEvents extends IUnknown
{
	/**
	 * Called when an event is fired for this session.
	 */
	 OnEvent(pEvent: IAGCEvent): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigEvent Interface

/**
 * Interface to a Pig Event object.
 */
interface IPigEvent extends IDispatch
{
	/**
	 * Gets/sets the name of the object.
	 */
	 Name(bstrName: string): void;
	/**
	 * Gets/sets the expression to be evaluated on the normal expiration of this action.
	 */
	 ExpirationExpression(bstrExpr: string): void;
	/**
	 * READONLY: Gets whether or not the event has expired.
	 */
	readonly IsExpired: boolean;
	/**
	 * Kills the event.
	 */
	 Kill(): void;
	/**
	 * Gets/sets the expression to be evaluated on the interrupted expiration of this action.
	 */
	 InterruptionExpression(bstrExpr: string): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigTimer Interface

/**
 * Interface to a Timer object.
 */
interface IPigTimer extends IPigEvent
{
	/**
	 * Gets/sets the number of interval occurences remaining for this timer.
	 */
	 RepetitionCount(nRepetitionCount: number): void;
	/**
	 * Gets/sets the time, in seconds, of each timer interval.
	 */
	 Interval(fInterval: number): void;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigShip Interface

/**
 * Interface to a Pig Ship object.
 */
interface IPigShip extends IAGCShip
{
	/**
	 * Sells all parts loaded onto the ship.
	 */
	 SellAllParts(): void;
	/**
	 * Buys the specified hull type.
	 */
	 BuyHull(pHullType: IAGCHullType): void;
	/**
	 * Buys the default loadout for the current hull type.
	 */
	 BuyDefaultLoadout(): void;
	/**
	 * Kills the ship.
	 */
	 CommitSuicide(): string;
	/**
	 * Stops all thrusting, throttling, axis turning, and action events.
	 */
	 AllStop(): string;
	/**
	 * Thrusts the ship in the specified direction(s).
	 */
	 Thrust(e1: Enums.PigThrust, e2: Enums.PigThrust, e3: Enums.PigThrust, e4: Enums.PigThrust, e5: Enums.PigThrust, e6: Enums.PigThrust, e7: Enums.PigThrust): string;
	/**
	 * Turns afterburner on/off.
	 */
	 Boost(bOn: boolean): string;
	/**
	 * Begins/ends the firing of the active weapon.
	 */
	 FireWeapon(bBegin: boolean): string;
	/**
	 * Begins/ends the firing of the missile launcher.
	 */
	 FireMissile(bBegin: boolean): string;
	/**
	 * Begins/ends the dropping of mines (or probes).
	 */
	 DropMine(bBegin: boolean): string;
	/**
	 * Activates/deactivates the vector lock.
	 */
	 LockVector(bLockVector: boolean): string;
	/**
	 * Activates/deactivates the rip cording action.
	 */
	 RipCord(pvParam: object): string;
	/**
	 * Cloaks/decloaks the ship.
	 */
	 Cloak(bCloak: boolean): string;
	/**
	 * Accepts the queued command.
	 */
	 AcceptCommand(): string;
	/**
	 * Clears the accepted command.
	 */
	 ClearCommand(): string;
	/**
	 * Turns the ship the specified number of positive degrees upward.
	 */
	 PitchUp(fDegrees: number): string;
	/**
	 * Turns the ship the specified number of positive degrees downward.
	 */
	 PitchDown(fDegrees: number): string;
	/**
	 * Turns the ship the specified number of positive degrees to the left.
	 */
	 YawLeft(fDegrees: number): string;
	/**
	 * Turns the ship the specified number of positive degrees to the right.
	 */
	 YawRight(fDegrees: number): string;
	/**
	 * Rolls the ship the specified number of positive degrees to the left
	 */
	 RollLeft(fDegrees: number): string;
	/**
	 * Rolls the ship the specified number of positive degrees to the right.
	 */
	 RollRight(fDegrees: number): string;
	/**
	 * Turns the ship to face the vector, specified by either an object name (VT_BSTR) or an IAGCVector* or IAGCModel* (VT_DISPATCH).
	 */
	 Face(pvarObject: object, bstrCompletionExpr: string, bstrInterruptionExpr: string, bMatchUpVector: boolean): string;
	/**
	 * Defends the specified target.
	 */
	 Defend(bstrObject: string): string;
	/**
	 * Attacks the specified target ship.
	 */
	 AttackShip(pTarget: IAGCShip): string;
	/**
	 * Attacks the specified target station.
	 */
	 AttackStation(pTarget: IAGCStation): string;
	/**
	 * Goto the specified target.
	 */
	 Goto(bstrObject: string, bFriendly: boolean): string;
	/**
	 * Goto the specified station ID.
	 */
	 GotoStationID(oid: number): string;
	/**
	 * Gets/sets the AutoPilot state.
	 */
	 AutoPilot(bEngage: boolean): void;
	/**
	 * READONLY: Gets the Pig Ship action event object for this ship.
	 */
	readonly AutoAction: IPigShipEvent;
	/**
	 * Indicates whether the ship is thrusting in the specified direction.
	 */
	 IsThrusting(eThrust: Enums.PigThrust): boolean;
	/**
	 * Gets/sets the weapon that is active. This must be 0 thru 3, or -1 for the default of 'all weapons'.
	 */
	 ActiveWeapon(iWeapon: number): void;
	/**
	 * READONLY: Indicates whether the ship is firing the active weapon.
	 */
	readonly IsFiringWeapon: boolean;
	/**
	 * READONLY: Indicates whether the ship is firing the missile launcher.
	 */
	readonly IsFiringMissile: boolean;
	/**
	 * READONLY: Indicates whether the ship is dropping a mine (or probe).
	 */
	readonly IsDroppingMine: boolean;
	/**
	 * READONLY: Indicates whether the ship's vector lock is active.
	 */
	readonly IsVectorLocked: boolean;
	/**
	 * READONLY: Indicates whether the ship is rip-cording.
	 */
	readonly IsRipCording: boolean;
	/**
	 * READONLY: Indicates whether the ship is cloaked.
	 */
	readonly IsCloaked: boolean;
	/**
	 * READONLY: Gets the max range of the current missile
	 */
	readonly MissileRange: number;
	/**
	 * READONLY: Gets the currnt missile lock 0.0 to 1.0
	 */
	readonly MissileLock: number;
	/**
	 * Gets/sets the ship's throttle. The valid range is 0 to 100.
	 */
	 Throttle(fThrottle: number): void;
	/**
	 * Gets/sets the ship's pitch speed. The valid range is -100 to 100.
	 */
	 Pitch(fPitch: number): void;
	/**
	 * Gets/sets the ship's yaw speed. The valid range is -100 to 100.
	 */
	 Yaw(fYaw: number): void;
	/**
	 * Gets/sets the ship's roll speed. The valid range is -100 to 100.
	 */
	 Roll(fRoll: number): void;
	/**
	 * READONLY: Gets the queued command.
	 */
	readonly QueuedCommand: IAGCCommand;
	/**
	 * READONLY: Gets the accepted command.
	 */
	readonly AcceptedCommand: IAGCCommand;
	/**
	 * Gets/sets whether the ship automatically accepts queued commands.
	 */
	 AutoAcceptCommands(bAutoAccept: boolean): void;
	/**
	 * READONLY: Gets the amount of time left until the ripcordind action completes.
	 */
	readonly RipCordTimeLeft: number;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigShipEvent Interface

/**
 * Interface to a Pig Ship Event object.
 */
interface IPigShipEvent extends IPigEvent
{
	/**
	 * READONLY: Gets name of the action in progress.
	 */
	readonly Action: string;
	/**
	 * READONLY: Gets the name of the target object for the action in progress.
	 */
	readonly Target: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigVector Interface

/**
 * Interface to a Pig Vector object.
 */
interface IPigVector extends IDispatch
{
	/**
	 * Gets/sets the x coordinate of the vector.
	 */
	 X(xArg: number): void;
	/**
	 * Gets/sets the y coordinate of the vector.
	 */
	 Y(yArg: number): void;
	/**
	 * Gets/sets the z coordinate of the vector.
	 */
	 Z(zArg: number): void;
	/**
	 * Sets the x, y, and z coordinates of the vector.
	 */
	 SetXYZ(xArg: number, yArg: number, zArg: number): void;
	/**
	 * READONLY: Gets the displayable string representation of the vector.
	 */
	readonly DisplayString: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigAccount Interface

/**
 * Interface to a Pig Account object.
 */
interface IPigAccount extends IDispatch
{
	/**
	 * READONLY: Gets the account name.
	 */
	readonly Name: string;
	/**
	 * READONLY: Gets the account password.
	 */
	readonly Password: string;
	/**
	 * READONLY: Gets the account CDKEY.
	 */
	readonly CdKey: string;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigAccountDispenser Interface

/**
 * Interface to a Pig Account Dispenser object.
 */
interface IPigAccountDispenser extends IDispatch
{
	/**
	 * READONLY: Gets an available account and marks it 'in-use'.
	 */
	readonly NextAvailable: IPigAccount;
	/**
	 * READONLY: Gets a collection of all the managed user names.
	 */
	readonly Names: ITCStrings;
	/**
	 * READONLY: Gets a collection of the available user names.
	 */
	readonly NamesAvailable: ITCStrings;
	/**
	 * READONLY: Gets a collection of the user names in use.
	 */
	readonly NamesInUse: ITCStrings;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// IPigHullTypes Interface

/**
 * Interface to a collection of hull type objects.
 */
interface IPigHullTypes extends ITCCollection
{
	/**
	 * Returns an IAGCHullType* from the collection or NULL if the specified named hull type does not exist in the collection. Takes an argument, index, which must be either the name of a hull type in the collection or a 0-based index into the collection.
	 */
	 Item(pvIndex: object): IAGCHullType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// These declarations enable you to convert your JS objects into COM Interfaces so that you can  
// get intellisense for event handler parameters.

declare class CType {
/**
 * Casts the passed object to IAGCEventSink - Interface implemented by the hosting application to receive triggered events. Intended for use ONLY by host applications.
 */
	IAGCEventSink(o: any): IAGCEventSink;
/**
 * Casts the passed object to IAGCEventSinkSynchronous - Interface implemented by the hosting application to receive triggered events. Intended for use ONLY by host applications.
 */
	IAGCEventSinkSynchronous(o: any): IAGCEventSinkSynchronous;
/**
 * Casts the passed object to IAGCDebugHook - Implemented by an AGC host to hook debug events.
 */
	IAGCDebugHook(o: any): IAGCDebugHook;
/**
 * Casts the passed object to IAGCGlobal - Interface to the AGC Global object. Intended for use ONLY by host applications that deal directly with Igc.
 */
	IAGCGlobal(o: any): IAGCGlobal;
/**
 * Casts the passed object to IAGCVector - Interface to a AGC Vector object.
 */
	IAGCVector(o: any): IAGCVector;
/**
 * Casts the passed object to IAGCVectorPrivate - Private interface to an AGC Vector object.
 */
	IAGCVectorPrivate(o: any): IAGCVectorPrivate;
/**
 * Casts the passed object to IAGCOrientation - Interface to an AGC Orientation object.
 */
	IAGCOrientation(o: any): IAGCOrientation;
/**
 * Casts the passed object to IAGCOrientationPrivate - Private interface to an AGC Orientation object.
 */
	IAGCOrientationPrivate(o: any): IAGCOrientationPrivate;
/**
 * Casts the passed object to IAGCEventType - IAGCEventType Interface
 */
	IAGCEventType(o: any): IAGCEventType;
/**
 * Casts the passed object to IAGCEvent - IAGCEvent Interface
 */
	IAGCEvent(o: any): IAGCEvent;
/**
 * Casts the passed object to IAGCEventCreate - IAGCEventCreate Interface
 */
	IAGCEventCreate(o: any): IAGCEventCreate;
/**
 * Casts the passed object to IAGCGameParameters - IAGCGameParameters Interface
 */
	IAGCGameParameters(o: any): IAGCGameParameters;
/**
 * Casts the passed object to IAGCCommand - Interface to an AGC Command object.
 */
	IAGCCommand(o: any): IAGCCommand;
/**
 * Casts the passed object to IAGCCommandPrivate - Private interface to an AGC Command object.
 */
	IAGCCommandPrivate(o: any): IAGCCommandPrivate;
/**
 * Casts the passed object to IAGCVersionInfo - IAGCVersionInfo Interface
 */
	IAGCVersionInfo(o: any): IAGCVersionInfo;
/**
 * Casts the passed object to IAGCDBParams - IAGCDBParams Interface
 */
	IAGCDBParams(o: any): IAGCDBParams;
/**
 * Casts the passed object to IAGCEventLogger - IAGCEventLogger Interface
 */
	IAGCEventLogger(o: any): IAGCEventLogger;
/**
 * Casts the passed object to IAGCEventLoggerHook - Implemented by an AGC host to hook event logging.
 */
	IAGCEventLoggerHook(o: any): IAGCEventLoggerHook;
/**
 * Casts the passed object to IAGCEventLoggerPrivate - IAGCEventLoggerPrivate Interface
 */
	IAGCEventLoggerPrivate(o: any): IAGCEventLoggerPrivate;
/**
 * Casts the passed object to IAGCRangePrivate - Private interface to an AGC Range object.
 */
	IAGCRangePrivate(o: any): IAGCRangePrivate;
/**
 * Casts the passed object to IAGCRangesPrivate - Private interface to an AGC Ranges object.
 */
	IAGCRangesPrivate(o: any): IAGCRangesPrivate;
/**
 * Casts the passed object to IAGCEventIDRange - IAGCEventIDRange Interface
 */
	IAGCEventIDRange(o: any): IAGCEventIDRange;
/**
 * Casts the passed object to IAGCEventIDRanges - IAGCEventIDRanges Interface
 */
	IAGCEventIDRanges(o: any): IAGCEventIDRanges;
/**
 * Casts the passed object to ITCCollection - Base interface for most collection interfaces.
 */
	ITCCollection(o: any): ITCCollection;
/**
 * Casts the passed object to ITCStrings - Interface to a collection of strings.
 */
	ITCStrings(o: any): ITCStrings;
/**
 * Casts the passed object to ITCPropBagOnRegKey - Interface to create a property bag on a registry key.
 */
	ITCPropBagOnRegKey(o: any): ITCPropBagOnRegKey;
/**
 * Casts the passed object to ITCSessionInfo - ITCSessionInfo Interface
 */
	ITCSessionInfo(o: any): ITCSessionInfo;
/**
 * Casts the passed object to ITCSessionInfos - ITCSessionInfos interface
 */
	ITCSessionInfos(o: any): ITCSessionInfos;
/**
 * Casts the passed object to ITCCollectionPersistHelper - ITCCollectionPersistHelper interface
 */
	ITCCollectionPersistHelper(o: any): ITCCollectionPersistHelper;
/**
 * Casts the passed object to ITCUtility - ITCUtility interface
 */
	ITCUtility(o: any): ITCUtility;
/**
 * Casts the passed object to IAdminSessionHelper - IAdminSessionHelper interface
 */
	IAdminSessionHelper(o: any): IAdminSessionHelper;
/**
 * Casts the passed object to IAGCPrivate - Internally-used interface on an AGC object.
 */
	IAGCPrivate(o: any): IAGCPrivate;
/**
 * Casts the passed object to IAGCCommon - IAGCCommon Interface
 */
	IAGCCommon(o: any): IAGCCommon;
/**
 * Casts the passed object to IAGCCollection - Base interface for AGC collection interfaces.
 */
	IAGCCollection(o: any): IAGCCollection;
/**
 * Casts the passed object to IAGCBase - IAGCBase Interface
 */
	IAGCBase(o: any): IAGCBase;
/**
 * Casts the passed object to IAGCModel - IAGCModel Interface
 */
	IAGCModel(o: any): IAGCModel;
/**
 * Casts the passed object to IAGCModels - IAGCModels Interface
 */
	IAGCModels(o: any): IAGCModels;
/**
 * Casts the passed object to IAGCBuyable - Properties of an object that is buyable.
 */
	IAGCBuyable(o: any): IAGCBuyable;
/**
 * Casts the passed object to IAGCHullType - Interface to an AGC Hull Type object.
 */
	IAGCHullType(o: any): IAGCHullType;
/**
 * Casts the passed object to IAGCTreasure - 
 */
	IAGCTreasure(o: any): IAGCTreasure;
/**
 * Casts the passed object to IAGCTreasures - IAGCTreasures Interface
 */
	IAGCTreasures(o: any): IAGCTreasures;
/**
 * Casts the passed object to IAGCDamage - IAGCDamage Interface
 */
	IAGCDamage(o: any): IAGCDamage;
/**
 * Casts the passed object to IAGCScanner - IAGCScanner Interface
 */
	IAGCScanner(o: any): IAGCScanner;
/**
 * Casts the passed object to IAGCProbe - IAGCProbe Interface
 */
	IAGCProbe(o: any): IAGCProbe;
/**
 * Casts the passed object to IAGCProbes - IAGCProbes Interface
 */
	IAGCProbes(o: any): IAGCProbes;
/**
 * Casts the passed object to IAGCGame - IAGCGame Interface
 */
	IAGCGame(o: any): IAGCGame;
/**
 * Casts the passed object to IAGCShip - IAGCShip Interface
 */
	IAGCShip(o: any): IAGCShip;
/**
 * Casts the passed object to IAGCShips - IAGCShips Interface
 */
	IAGCShips(o: any): IAGCShips;
/**
 * Casts the passed object to IAGCStation - IAGCStation Interface
 */
	IAGCStation(o: any): IAGCStation;
/**
 * Casts the passed object to IAGCStations - IAGCStations Interface
 */
	IAGCStations(o: any): IAGCStations;
/**
 * Casts the passed object to IAGCAleph - IAGCAleph Interface
 */
	IAGCAleph(o: any): IAGCAleph;
/**
 * Casts the passed object to IAGCAlephs - IAGCAlephs Interface
 */
	IAGCAlephs(o: any): IAGCAlephs;
/**
 * Casts the passed object to IAGCSector - IAGCSector Interface
 */
	IAGCSector(o: any): IAGCSector;
/**
 * Casts the passed object to IAGCSectors - IAGCSectors Interface
 */
	IAGCSectors(o: any): IAGCSectors;
/**
 * Casts the passed object to IAGCTeam - IAGCTeam Interface
 */
	IAGCTeam(o: any): IAGCTeam;
/**
 * Casts the passed object to IAGCTeams - IAGCTeams Interface
 */
	IAGCTeams(o: any): IAGCTeams;
/**
 * Casts the passed object to IAGCAsteroid - IAGCAsteroid Interface
 */
	IAGCAsteroid(o: any): IAGCAsteroid;
/**
 * Casts the passed object to IAGCAsteroids - IAGCAsteroids Interface
 */
	IAGCAsteroids(o: any): IAGCAsteroids;
/**
 * Casts the passed object to IPigBehaviorType - Interface to a pig behavior type object.
 */
	IPigBehaviorType(o: any): IPigBehaviorType;
/**
 * Casts the passed object to IPigBehaviorTypes - Interface to a collection of pig behavior script objects.
 */
	IPigBehaviorTypes(o: any): IPigBehaviorTypes;
/**
 * Casts the passed object to IPigBehaviorScriptType - Interface to a pig behavior script object.
 */
	IPigBehaviorScriptType(o: any): IPigBehaviorScriptType;
/**
 * Casts the passed object to IPigBehaviorHost - Interface to a pig behavior host object.
 */
	IPigBehaviorHost(o: any): IPigBehaviorHost;
/**
 * Casts the passed object to IPigBehavior - Interface to a pig behavior object.
 */
	IPigBehavior(o: any): IPigBehavior;
/**
 * Casts the passed object to IPigBehaviors - Interface to a collection of pig behavior objects.
 */
	IPigBehaviors(o: any): IPigBehaviors;
/**
 * Casts the passed object to IPigBehaviorStack - Interface to a stack of pig behavior objects.
 */
	IPigBehaviorStack(o: any): IPigBehaviorStack;
/**
 * Casts the passed object to IPig - Interface to a pig player object.
 */
	IPig(o: any): IPig;
/**
 * Casts the passed object to IPigs - Interface to a collection of pig player objects.
 */
	IPigs(o: any): IPigs;
/**
 * Casts the passed object to IPigTeam - Interface to a Team object.
 */
	IPigTeam(o: any): IPigTeam;
/**
 * Casts the passed object to IPigTeams - Interface to a collection of Team objects.
 */
	IPigTeams(o: any): IPigTeams;
/**
 * Casts the passed object to IPigMissionParams - Interface to a MissionParams object.
 */
	IPigMissionParams(o: any): IPigMissionParams;
/**
 * Casts the passed object to IPigMissionParamsPrivate - Private interface to a MissionParams object.
 */
	IPigMissionParamsPrivate(o: any): IPigMissionParamsPrivate;
/**
 * Casts the passed object to IPigMission - Interface to a Mission object.
 */
	IPigMission(o: any): IPigMission;
/**
 * Casts the passed object to IPigMissions - Interface to a collection of Mission objects.
 */
	IPigMissions(o: any): IPigMissions;
/**
 * Casts the passed object to IPigLobby - Interface to a Lobby object.
 */
	IPigLobby(o: any): IPigLobby;
/**
 * Casts the passed object to IPigLobbies - Interface to a collection of Lobby objects.
 */
	IPigLobbies(o: any): IPigLobbies;
/**
 * Casts the passed object to IPigSession - IPigSession Interface
 */
	IPigSession(o: any): IPigSession;
/**
 * Casts the passed object to IPigSessionEvents - IPigSessionEvents Interface
 */
	IPigSessionEvents(o: any): IPigSessionEvents;
/**
 * Casts the passed object to IPigEvent - Interface to a Pig Event object.
 */
	IPigEvent(o: any): IPigEvent;
/**
 * Casts the passed object to IPigTimer - Interface to a Timer object.
 */
	IPigTimer(o: any): IPigTimer;
/**
 * Casts the passed object to IPigShip - Interface to a Pig Ship object.
 */
	IPigShip(o: any): IPigShip;
/**
 * Casts the passed object to IPigShipEvent - Interface to a Pig Ship Event object.
 */
	IPigShipEvent(o: any): IPigShipEvent;
/**
 * Casts the passed object to IPigVector - Interface to a Pig Vector object.
 */
	IPigVector(o: any): IPigVector;
/**
 * Casts the passed object to IPigAccount - Interface to a Pig Account object.
 */
	IPigAccount(o: any): IPigAccount;
/**
 * Casts the passed object to IPigAccountDispenser - Interface to a Pig Account Dispenser object.
 */
	IPigAccountDispenser(o: any): IPigAccountDispenser;
/**
 * Casts the passed object to IPigHullTypes - Interface to a collection of hull type objects.
 */
	IPigHullTypes(o: any): IPigHullTypes;
}


