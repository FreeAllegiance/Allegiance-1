/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:    igc.h
**
**  Author:
**
**  Description:
**      Interface decriptions for the igc library. This file was created by the ATL wizard.
**
**  History:
*/
#ifndef IGC_H
#define IGC_H

#include <Utility.h>
#include <color.h>
#include <mask.h>
#include <olddxdefns.h>
#include <orientation.h>
#include <point.h>
#include <quaternion.h>
#include <tref.h>
#include <vector.h>

const float c_fPedestalOffset = 2000.0f;
const float c_fFlagOffset = 50.0f;

const int   c_maxLaunchSlots = 6;
const int   c_maxLandSlots = 4;
const int   c_maxLandPlanes = 6;

const int   c_maxShipLandSlots = 2;
const int   c_maxShipLaunchSlots = 2;

typedef int ExplosionType;
const ExplosionType c_etMine            = 1;
const ExplosionType c_etMissile         = 2;
const ExplosionType c_etProjectile      = 2;    //Intentional duplication
const ExplosionType c_etSmallShip       = 3;
const ExplosionType c_etProbe           = 3;    //Intentional duplication
const ExplosionType c_etBigShip         = 4;
const ExplosionType c_etSmallStation    = 5;
const ExplosionType c_etAsteroid        = 6;
const ExplosionType c_etLargeStation    = 7;

typedef char    DamageResult;
const DamageResult  c_drKilled          = 1;
const DamageResult  c_drHullDamage      = 2;
const DamageResult  c_drShieldDamage    = 3;
const DamageResult  c_drNoDamage        = 4;

const int   c_tsmaskNothing     = 0;
const int   c_tsmaskSelected    = 0x01;
const int   c_tsmaskSubject     = 0x02;
const int   c_tsmaskLegalObject = 0x04;

typedef short   FloatConstantID;
const FloatConstantID   c_fcidLensMultiplier           = 0;
const FloatConstantID   c_fcidRadiusUniverse           = 1;
const FloatConstantID   c_fcidOutOfBounds              = 2;
const FloatConstantID   c_fcidExitWarpSpeed            = 3;
const FloatConstantID   c_fcidExitStationSpeed         = 4;
const FloatConstantID   c_fcidDownedShield             = 5;
const FloatConstantID   c_fcidSecondsBetweenPaydays    = 6;
const FloatConstantID   c_fcidCapacityHe3              = 7;
const FloatConstantID   c_fcidValueHe3                 = 8;
const FloatConstantID   c_fcidAmountHe3                = 9;
const FloatConstantID   c_fcidMountRate                = 10;

const FloatConstantID   c_fcidStartingMoney            = 11;
const FloatConstantID   c_fcidWinTheGameMoney          = 12;
const FloatConstantID   c_fcidRipcordTime              = 13;

const FloatConstantID   c_fcidHe3Regeneration          = 14;

const FloatConstantID   c_fcidPointsWarp            = 15;
const FloatConstantID   c_fcidPointsAsteroid        = 16;
const FloatConstantID   c_fcidPointsTech            = 17;
const FloatConstantID   c_fcidPointsMiner           = 18;
const FloatConstantID   c_fcidPointsBuilder         = 19;
const FloatConstantID   c_fcidPointsLayer           = 20;
const FloatConstantID   c_fcidPointsCarrier         = 21;
const FloatConstantID   c_fcidPointsPlayer          = 22;
const FloatConstantID   c_fcidPointsBaseKill        = 23;
const FloatConstantID   c_fcidPointsBaseCapture     = 24;

const FloatConstantID   c_fcidPointsFlags           = 25;
const FloatConstantID   c_fcidPointsArtifacts       = 26;
const FloatConstantID   c_fcidPointsRescues         = 27;

const FloatConstantID   c_fcidRatingAdd             = 28;
const FloatConstantID   c_fcidRatingDivide          = 29;

const FloatConstantID    c_fcidIncome               = 30;
const FloatConstantID    c_fcidLifepodEndurance     = 31;

const FloatConstantID    c_fcidWarpBombDelay        = 32;

const FloatConstantID    c_fcidLifepodCost          = 33;
const FloatConstantID    c_fcidTurretCost           = 34;
const FloatConstantID    c_fcidDroneCost            = 35;
const FloatConstantID    c_fcidPlayerCost           = 36;
const FloatConstantID    c_fcidBaseClusterCost      = 37;
const FloatConstantID    c_fcidClusterDivisor       = 38;

const FloatConstantID    c_fcidMax                  = 40;


//
// Note: if you add or change any new ObjectTypes, then please notify
// Mark C or John T so that AGCIDL.idl will be updated.
//
//
typedef  short   ObjectType;
const ObjectType    OT_invalid          = -1;

const ObjectType  OT_modelBegin         =  0;
const ObjectType    OT_ship             =  0;                //All classes derived from model
const ObjectType    OT_station          =  1;
const ObjectType    OT_missile          =  2;
const ObjectType    OT_mine             =  3;
const ObjectType    OT_probe            =  4;
const ObjectType    OT_asteroid         =  5;

const ObjectType    OT_projectile       =  6;
const ObjectType    OT_warp             =  7;
const ObjectType    OT_treasure         =  8;           //All of the above are models
const ObjectType    OT_buoy             =  9;
const ObjectType    OT_chaff            = 10;
const ObjectType    OT_buildingEffect   = 11;
const ObjectType  OT_modelEnd           = 11;

const ObjectType    OT_side             = 12;
const ObjectType    OT_cluster          = 13;
const ObjectType    OT_bucket           = 14;

const ObjectType  OT_partBegin          = 15;
const ObjectType    OT_weapon           = 15;
const ObjectType    OT_shield           = 16;
const ObjectType    OT_cloak            = 17;
const ObjectType    OT_pack             = 18;
const ObjectType    OT_afterburner      = 19;
const ObjectType  OT_launcherBegin      = 20;
const ObjectType    OT_magazine         = 20;
const ObjectType    OT_dispenser        = 21;
const ObjectType  OT_launcherEnd        = 21;
const ObjectType  OT_partEnd            = 21;


const ObjectType  OT_staticBegin        = 22;
const ObjectType    OT_projectileType   = 22;      //All static objects
const ObjectType    OT_missileType      = 23;
const ObjectType    OT_mineType         = 24;
const ObjectType    OT_probeType        = 25;
const ObjectType    OT_chaffType        = 26;
const ObjectType    OT_civilization     = 27;
const ObjectType    OT_treasureSet      = 28;

const ObjectType  OT_bucketStart        = 29;
const ObjectType    OT_hullType         = 29;
const ObjectType    OT_partType         = 30;
const ObjectType    OT_stationType      = 31;
const ObjectType    OT_development      = 32;
const ObjectType    OT_droneType        = 33;
const ObjectType  OT_bucketEnd          = 33;
const ObjectType  OT_staticEnd          = 33;

const ObjectType    OT_constants        = 34;
const ObjectType    OT_allsrvUser       = 35; // used by the admin object model
const ObjectType    OT_Max              = 36;// don't put anything after this
                                             // OT_Max should be less then 256 for
                                             // AGC event firing.

const   int64_t         c_maskStaticTypes = (int64_t(1) << int64_t(OT_projectileType)) |
                                            (int64_t(1) << int64_t(OT_treasureSet)) |
                                            (int64_t(1) << int64_t(OT_hullType)) |
                                            (int64_t(1) << int64_t(OT_partType)) |
                                            (int64_t(1) << int64_t(OT_missileType)) |
                                            (int64_t(1) << int64_t(OT_mineType)) |
                                            (int64_t(1) << int64_t(OT_probeType)) |
                                            (int64_t(1) << int64_t(OT_civilization)) |
                                            (int64_t(1) << int64_t(OT_stationType)) |
                                            (int64_t(1) << int64_t(OT_development)) |
                                            (int64_t(1) << int64_t(OT_droneType)) |
                                            (int64_t(1) << int64_t(OT_chaffType)) |
                                            (int64_t(1) << int64_t(OT_constants));

const   int64_t         c_maskMapTypes =    (int64_t(1) << int64_t(OT_asteroid)) |
                                            (int64_t(1) << int64_t(OT_station))  |
                                            (int64_t(1) << int64_t(OT_cluster))  |
                                            (int64_t(1) << int64_t(OT_mine))     |
                                            (int64_t(1) << int64_t(OT_probe))    |
                                            (int64_t(1) << int64_t(OT_treasure)) |
                                            (int64_t(1) << int64_t(OT_warp));



class Geo;
class ThingGeo;
class ThingGeoSite;
class GroupGeo;
class BuildEffectGeo;
class PosterImage;
class DamageTrackSet;
class DamageTrack;
class DamageBucket;

typedef Slist_utl<DamageBucket*> DamageBucketList;
typedef Slink_utl<DamageBucket*> DamageBucketLink;

typedef short  ForceEffectID;

const ForceEffectID c_maxEffect  = 10;

const ForceEffectID effectFire = 0;
const ForceEffectID effectBounce = 1;
const ForceEffectID effectExplode = 2;

typedef short VisualEffectID;

const ForceEffectID effectJiggle = 0;

typedef short  TrekKey;

const TrekKey TK_NoKeyMapping               = -1;  // Key not mapped to anything
const TrekKey TK_NextCommMsg                = 0;   // Scroll focus to next chat message in buffer
const TrekKey TK_PrevCommMsg                = 1;   // Scroll focus to previous chat message in buffer
const TrekKey TK_PrevWeapon                 = 2;   // Select the previous weapon from inventory
const TrekKey TK_NextWeapon                 = 3;   // Select the next weapon from inventory
const TrekKey TK_ToggleGroupFire            = 4;   // Toggle all weapons firing together
const TrekKey TK_PrevPart                   = 5;   // Select the previous part from inventory
const TrekKey TK_NextPart                   = 6;   // Select the next part from inventory
const TrekKey TK_MountPart                  = 7;   // Mount the selected part
const TrekKey TK_ReplacePart                = 8;   // Replace the selected part
const TrekKey TK_UnmountPart                = 9;   // Unmount the selected part
const TrekKey TK_DropPart                   = 10;  // Drop the selected part
const TrekKey TK_ZoomOut                    = 11;  // Zoom out
const TrekKey TK_ZoomIn                     = 12;  // Zoom in
const TrekKey TK_PitchUp                    = 17;  // Turn the nose of the ship up
const TrekKey TK_PitchDown                  = 18;  // Turn the nose of the ship down
const TrekKey TK_YawLeft                    = 19;  // Turn the nose of the ship left
const TrekKey TK_YawRight                   = 20;  // Turn the nose of the ship right
const TrekKey TK_RollLeft                   = 21;  // Roll the ship to the left
const TrekKey TK_RollRight                  = 22;  // Roll the ship to the right
const TrekKey TK_FOVOut                     = 23;  // Adjust frame-of-view wider
const TrekKey TK_FOVIn                      = 24;  // Adjust frame-of-view narrower
const TrekKey TK_MainMenu                   = 25;  // View the main menu
const TrekKey TK_ViewFrontLeft              = 26;  // Look forward to the left
const TrekKey TK_ViewFrontRight             = 27;  // Look forward to the right
const TrekKey TK_ViewLeft                   = 28;  // Look to the left
const TrekKey TK_ViewRight                  = 29;  // Look to the right
const TrekKey TK_ViewExternal               = 30;  // View the ship from outside
const TrekKey TK_ViewFlyby                  = 31;  // View an animated flyby of the ship
const TrekKey TK_ViewMissile                = 32;  // View from the missile camera
const TrekKey TK_ViewTarget                 = 33;  // View the targeted item
const TrekKey TK_ViewChase                  = 34;  // View the ship from a chase position
const TrekKey TK_ViewTurret                 = 35;  // View from the ship's turret
const TrekKey TK_ViewBase                   = 36;  // View from the base
const TrekKey TK_ViewSector                 = 37;  // View the sector map
const TrekKey TK_ViewCommand                = 38;  // Display a tactical view of the sector
const TrekKey TK_CycleChatMsgs              = 39;  // Currently does nothing
const TrekKey TK_AcceptCommand              = 40;  // Accept pending command
const TrekKey TK_ClearCommand               = 41;  // Clear current command
const TrekKey TK_RejectCommand              = 42;  // current target == current command
const TrekKey TK_ToggleAutoPilot            = 43;  // Activate autopilot
const TrekKey TK_ThrustLeft                 = 44;  // Apply thrust to cause ship to move left
const TrekKey TK_ThrustRight                = 45;  // Apply thrust to cause ship to move right
const TrekKey TK_ThrustUp                   = 46;  // Apply thrust to cause ship to move up
const TrekKey TK_ThrustDown                 = 47;  // Apply thrust to cause ship to move down
const TrekKey TK_ThrustForward              = 48;  // Apply thrust to cause ship to move forward
const TrekKey TK_ThrustBackward             = 49;  // Apply thrust to cause ship to move backward
const TrekKey TK_FireBooster                = 50;  // Fire booster rockets
const TrekKey TK_ConModeGameState           = 51;  // Game State Console Mode
const TrekKey TK_QuickChatMenu              = 52;  // Bring up quick chat menu
const TrekKey TK_ToggleLODSlider            = 53;  // Toggle the LOD Slider
const TrekKey TK_StartChat                  = 54;  // Start a chat message
const TrekKey TK_FireWeapon                 = 55;  // Fire the selected weapon
const TrekKey TK_FireMissile                = 56;  // Fire the selected missile
const TrekKey TK_LockVector                 = 57;  // Lock the current heading and speed
const TrekKey TK_ThrottleUp                 = 58;  // Increase the ship speed
const TrekKey TK_ThrottleDown               = 59;  // Decrease the ship speed
const TrekKey TK_ThrottleZero               = 60;  // Shut down engine
const TrekKey TK_Throttle33                 = 61;  // Throttle at one-third
const TrekKey TK_Throttle66                 = 62;  // Throttle at two-thirds
const TrekKey TK_ThrottleFull               = 63;  // Throttle at full
const TrekKey TK_ToggleBooster              = 64;  // Toggle booster rocket on/off
const TrekKey TK_LODUp                      = 65;  // Move the LOD scale higher
const TrekKey TK_LODDown                    = 66;  // Move the LOD scale lower
const TrekKey TK_ConModeCombat              = 67;  // Sets console mode to combat
const TrekKey TK_ConModeCommand             = 68;  // Sets console mode to command view
const TrekKey TK_ConModeNav                 = 69;  // Sets console mode to navigation
const TrekKey TK_ConModeInventory           = 70;  // Sets console mode to inventory
const TrekKey TK_ConModeInvest              = 71;  // Sets console mode to investments
const TrekKey TK_ConModeTeam                = 72;  // Sets console mode to the team pane
const TrekKey TK_ViewRear                   = 73;  // Sets the camera to point directly back
const TrekKey TK_Target                     = 74;  // Target next object (any type)
const TrekKey TK_TargetNearest              = 75;  // Target nearest object (any type)
const TrekKey TK_TargetPrev                 = 76;  // Target previous object (any type)
const TrekKey TK_TargetFriendly             = 77;  // Target next friendly ship
const TrekKey TK_TargetFriendlyNearest      = 78;  // Target nearest friendly ship
const TrekKey TK_TargetFriendlyPrev         = 79;  // Target previous friendly ship
const TrekKey TK_TargetEnemy                = 80;  // Target next enemy ship
const TrekKey TK_TargetEnemyNearest         = 81;  // Target nearest enemy ship
const TrekKey TK_TargetEnemyPrev            = 82;  // Target previous enemy ship
const TrekKey TK_TargetEnemyBase            = 83;  // Targets the nearest enemy base
const TrekKey TK_TargetEnemyBaseNearest     = 84;  // Targets the nearest (any) object
const TrekKey TK_TargetEnemyBasePrev        = 85;  // Targets the nearest enemy ship
const TrekKey TK_TargetFriendlyBase         = 86;  // Targets the nearest friendly ship
const TrekKey TK_TargetFriendlyBaseNearest  = 87;  // Targets the nearest object within a cone ahead
const TrekKey TK_TargetFriendlyBasePrev     = 88;  // Target previous friendly base
const TrekKey TK_TargetCommand              = 89;  // Target the object of current command
const TrekKey TK_TargetCenter               = 90;  // Target ship straight ahead
const TrekKey TK_TargetHostile              = 91;  // Target the ship causing most recent damage
const TrekKey TK_Suicide                    = 92;  // Kill yourself
const TrekKey TK_ToggleGrid                 = 93;  // Toggle visible grid in combat view
const TrekKey TK_ToggleCloak                = 94;  // Toggle cloaking
const TrekKey TK_DropMine                   = 95;  // Drop a minefield
const TrekKey TK_Ripcord                    = 96;  // Ripcord
const TrekKey TK_ViewRearLeft               = 97;  // Look back and to the left
const TrekKey TK_ViewRearRight              = 99;  // Look back and to the right
const TrekKey TK_Obsolete1                  = 99;  // Not used - feel free to reuse
const TrekKey TK_TargetSelf                 =100;  // Target myself
const TrekKey TK_ToggleCollisions	        =101;  // Toggle collision detection
const TrekKey TK_OccupyNextTurret           =102;  // Occupy the next available turret position
const TrekKey TK_TargetNothing              =103;  // Reset target so that nothing is targeted
const TrekKey TK_MatchSpeed                 =104;  // Match speed with target
const TrekKey TK_ChatPageUp                 =105;  // Scroll the chat pane one page up
const TrekKey TK_ChatPageDown               =106;  // Scroll the chat pane one page down
const TrekKey TK_ViewExternalOrbit          =107;  // Orbit the camera around the ship
const TrekKey TK_ViewExternalStation        =108;  // Have the camera face the station through the ship
const TrekKey TK_QuickChat1                 =109;  // Send quick chat 1
const TrekKey TK_QuickChat2                 =110;  // Send quick chat 2
const TrekKey TK_QuickChat3                 =111;  // Send quick chat 3
const TrekKey TK_QuickChat4                 =112;  // Send quick chat 4
const TrekKey TK_QuickChat5                 =113;  // Send quick chat 5
const TrekKey TK_QuickChat6                 =114;  // Send quick chat 6
const TrekKey TK_QuickChat7                 =115;  // Send quick chat 7
const TrekKey TK_QuickChat8                 =116;  // Send quick chat 8
const TrekKey TK_QuickChat9                 =117;  // Send quick chat 9
const TrekKey TK_QuickChat0                 =118;  // Send quick chat 10
const TrekKey TK_DebugTest1                 = 119; // DebugTest1
const TrekKey TK_DebugTest2                 = 120; // DebugTest2
const TrekKey TK_DebugTest3                 = 121; // DebugTest3
const TrekKey TK_DebugTest4                 = 122; // DebugTest4
const TrekKey TK_DebugTest5                 = 123; // DebugTest5
const TrekKey TK_DebugTest6                 = 124; // DebugTest6
const TrekKey TK_DebugTest7                 = 125; // DebugTest7
const TrekKey TK_DebugTest8                 = 126; // DebugTest8
const TrekKey TK_DebugTest9                 = 127; // DebugTest9
const TrekKey TK_DropChaff                  = 128; // Drop Chaff
const TrekKey TK_ToggleCommand              = 129; // Toggle command mode 3/4 vs. top down
const TrekKey TK_SwapWeapon1                = 130; // Swap the current weapon with cargo
const TrekKey TK_SwapWeapon2                = 131; // Swap the current weapon with cargo
const TrekKey TK_SwapWeapon3                = 132; // Swap the current weapon with cargo
const TrekKey TK_SwapWeapon4                = 133; // Swap the current weapon with cargo
const TrekKey TK_SwapTurret1                = 134; // Swap the current weapon with cargo
const TrekKey TK_SwapTurret2                = 135; // Swap the current weapon with cargo
const TrekKey TK_SwapTurret3                = 136; // Swap the current weapon with cargo
const TrekKey TK_SwapTurret4                = 137; // Swap the current weapon with cargo
const TrekKey TK_SwapMissile                = 138; // Swap the current item with cargo
const TrekKey TK_SwapChaff                  = 139; // Swap the current item with cargo
const TrekKey TK_SwapShield                 = 140; // Swap the current item with cargo
const TrekKey TK_SwapCloak                  = 141; // Swap the current item with cargo
const TrekKey TK_SwapAfterburner            = 142; // Swap the current item with cargo
const TrekKey TK_SwapMine                   = 143; // Swap the current item with cargo
const TrekKey TK_NextCargo                  = 144; // Rotate the selected cargo item
const TrekKey TK_ToggleWeapon1              = 145; // select the weapon for singe fire or group fire
const TrekKey TK_ToggleWeapon2              = 146; // select the weapon for singe fire or group fire
const TrekKey TK_ToggleWeapon3              = 147; // select the weapon for singe fire or group fire
const TrekKey TK_ToggleWeapon4              = 148; // select the weapon for singe fire or group fire
const TrekKey TK_DropCargo                  = 154; // drop the current cargo item
const TrekKey TK_Reload                     = 155;
const TrekKey TK_ExecuteCommand             = 156; // accept and activate autopilot
const TrekKey TK_PauseTM                    = 157; // pause training missions

const TrekKey TK_TargetEnemyBomber          = 158;
const TrekKey TK_TargetEnemyFighter         = 159;
const TrekKey TK_TargetEnemyConstructor     = 160;
const TrekKey TK_TargetEnemyMiner           = 161;
const TrekKey TK_TargetEnemyMajorBase       = 162;
const TrekKey TK_TargetFriendlyBomber       = 163;
const TrekKey TK_TargetFriendlyFighter      = 164;
const TrekKey TK_TargetFriendlyConstructor  = 165;
const TrekKey TK_TargetFriendlyMiner        = 166;
const TrekKey TK_TargetFriendlyLifepod      = 167;
const TrekKey TK_TargetFriendlyMajorBase    = 168;
const TrekKey TK_TargetFlag                 = 169;
const TrekKey TK_TargetArtifact             = 170;

const TrekKey TK_TargetEnemyBomberNearest          = 171;
const TrekKey TK_TargetEnemyFighterNearest         = 172;
const TrekKey TK_TargetEnemyConstructorNearest     = 173;
const TrekKey TK_TargetEnemyMinerNearest           = 174;
const TrekKey TK_TargetEnemyMajorBaseNearest       = 175;
const TrekKey TK_TargetFriendlyBomberNearest       = 176;
const TrekKey TK_TargetFriendlyFighterNearest      = 177;
const TrekKey TK_TargetFriendlyConstructorNearest  = 178;
const TrekKey TK_TargetFriendlyMinerNearest        = 179;
const TrekKey TK_TargetFriendlyLifepodNearest      = 180;
const TrekKey TK_TargetFriendlyMajorBaseNearest    = 181;
const TrekKey TK_TargetFlagNearest                 = 182;
const TrekKey TK_TargetArtifactNearest             = 183;

const TrekKey TK_TargetEnemyBomberPrev          = 184;
const TrekKey TK_TargetEnemyFighterPrev         = 185;
const TrekKey TK_TargetEnemyConstructorPrev     = 186;
const TrekKey TK_TargetEnemyMinerPrev           = 187;
const TrekKey TK_TargetEnemyMajorBasePrev       = 188;
const TrekKey TK_TargetFriendlyBomberPrev       = 189;
const TrekKey TK_TargetFriendlyFighterPrev      = 190;
const TrekKey TK_TargetFriendlyConstructorPrev  = 191;
const TrekKey TK_TargetFriendlyMinerPrev        = 192;
const TrekKey TK_TargetFriendlyLifepodPrev      = 193;
const TrekKey TK_TargetFriendlyMajorBasePrev    = 194;
const TrekKey TK_TargetFlagPrev                 = 195;
const TrekKey TK_TargetArtifactPrev             = 196;

const TrekKey TK_TargetWarp                     = 197;
const TrekKey TK_TargetWarpNearest              = 198;
const TrekKey TK_TargetWarpPrev                 = 199;

// debugging keys
const TrekKey TK_SkipTMGoal                     = 200; // skip a training mission goal
const TrekKey TK_IncreaseTMClockSpeed           = 201; // increase clock speed in training missions
const TrekKey TK_DecreaseTMClockSpeed           = 202; // decrease clock speed in training missions
const TrekKey TK_ResetTMClockSpeed              = 203; // reset clock speed in training missions
const TrekKey TK_Help                           = 204; // help

const TrekKey TK_CycleRadar                     = 205; // help
const TrekKey TK_ConModeMiniTeam                = 206; // Sets console mode to the mini-team pane

const TrekKey TK_CommandFinishTM                = 207;
const TrekKey TK_RollModifier                   = 208;
const TrekKey TK_TrackCommandView               = 209;
const TrekKey TK_ConModeTeleport                = 210;
const TrekKey TK_ToggleMouse                    = 211;
const TrekKey TK_PromoteTurret1                 = 212; // Swap the current weapon with cargo
const TrekKey TK_PromoteTurret2                 = 213; // Swap the current weapon with cargo
const TrekKey TK_PromoteTurret3                 = 214; // Swap the current weapon with cargo
const TrekKey TK_PromoteTurret4                 = 215; // Swap the current weapon with cargo
const TrekKey TK_VoteYes                        = 216; // Vote Yes on the current issue
const TrekKey TK_VoteNo                         = 217; // Vote No on the current issue

const TrekKey TK_ScrnShot                       = 218; // Take a screen shot

const TrekKey TK_TargetAlliedBase				= 219; //Imago 8/1/09
const TrekKey TK_TargetAlliedBaseNearest		= 220;
const TrekKey TK_TargetAlliedBasePrev			= 221;

const TrekKey TK_Max                            = 222; // Must be last trekkey

typedef short   SoundID;
typedef short   VoiceID;

#define DEFSOUND(id) id##Sound,
enum {
    lastDynamicSound = 1000,
#include "sounds.h"
};
#undef DEFSOUND

//Redefined data types (so we can change them later)
const int NA = -1; // meaning unspecified, none, or all.
const float fNA = -1; // To support supression of compiler warnings.  Floats should probably all be doubles anyways, but hey.  --Dhauzimmer, 8/14/04

typedef long            MissionID;

//*ID are unique IDs for something
typedef short           ObjectID;
typedef ObjectID        BucketID;
typedef ObjectID        PartID;
typedef ObjectID        ShipID;
typedef ObjectID        BuoyID;
typedef ObjectID        AsteroidID;
typedef ObjectID        HullID;
typedef ObjectID        StationTypeID;
typedef ObjectID        DroneTypeID;
typedef ObjectID        DevelopmentID;
typedef ObjectID        SectorID;
typedef ObjectID        WarpID;
typedef ObjectID        ProjectileTypeID;
typedef ObjectID        TreasureID;
typedef ObjectID        SideID;
typedef ObjectID        StationID;
typedef ObjectID        MissileID;
typedef ObjectID        ChaffID;
typedef ObjectID        MineID;
typedef ObjectID        ProbeID;
typedef ObjectID        ExpendableTypeID;
typedef ObjectID        CivID;
typedef ObjectID        MunitionID;
typedef ObjectID        TreasureSetID;
const ObjectID         RANDOM_ID = 9999;  //Xynth #170 8/2010
typedef int             SquadID;

typedef ObjectID        WingID;
const WingID            c_widMax = 10;

const SideID c_cSidesMax = 6;
const int c_cAlliancesMax = c_cSidesMax/2; // #ALLY max alliances possible (distinct groups of allied teams)

extern const char*      c_pszWingName[c_widMax];

const DevelopmentID     c_didTeamMoney = 1;

const SideID    SIDE_TEAMLOBBY = -2;

typedef BYTE            Cookie;

//"Enumerated" types which probably should be something other than integers
//This has to be signed so that NA means NA
typedef unsigned char    ModelAttributes;
const ModelAttributes  c_mtNotPickable         = 0x01;          //User can never pick the object
const ModelAttributes  c_mtScanner             = 0x02;          //Can see other objects
const ModelAttributes  c_mtPredictable         = 0x04;          //Send this object to a team when it is seen for the 1st time
const ModelAttributes  c_mtSeenBySide          = 0x08;          //Need to track its visibility wrt side
const ModelAttributes  c_mtStatic              = 0x10;          //Can move
const ModelAttributes  c_mtDamagable           = 0x20;          //Can take damage
const ModelAttributes  c_mtHitable             = 0x40;          //Goes into the collision KD tree
const ModelAttributes  c_mtCastRay             = 0x80;          //casts a ray for purposes of collisions

typedef short     EquipmentType; // this is PartTypeID in the database
const EquipmentType   ET_ChaffLauncher  = 0;
const EquipmentType   ET_Weapon         = 1;
const EquipmentType   ET_Magazine       = 2;
const EquipmentType   ET_Dispenser      = 3;
const EquipmentType   ET_Shield         = 4;
const EquipmentType   ET_Cloak          = 5;
const EquipmentType   ET_Pack           = 6;
const EquipmentType   ET_Afterburner    = 7;
const EquipmentType   ET_MAX            = 8;

typedef short     AbilityType; // Imago added
const AbilityType	  AT_Hull			= 0;
const AbilityType	  AT_Station		= 1;
const AbilityType	  AT_Expendable		= 2;
const AbilityType	  AT_Asteroid		= 3;


typedef char     PackType;
const PackType      c_packAmmo    = 0;
const PackType      c_packFuel    = 1;

typedef char     BuoyType;
const BuoyType      c_buoyWaypoint = 0;
const BuoyType      c_buoyCluster  = 1;

typedef unsigned char     DamageTypeID;
typedef unsigned char     DefenseTypeID;

const DamageTypeID      c_dmgidNoWarn   = 0x20;
const DamageTypeID      c_dmgidNoDebris = 0x40;
const DamageTypeID      c_dmgidNoFlare =  0x80;
const DamageTypeID      c_dmgidMask = 0x1f;

const DamageTypeID      c_dmgidCollision = c_dmgidNoDebris | c_dmgidNoFlare | c_dmgidNoWarn;
const DamageTypeID      c_dmgidMax = 20;

const DefenseTypeID     c_defidAsteroid       = 0;
const DefenseTypeID     c_defidMax = 20;

typedef int     TechTreeBit;

const TechTreeBit   c_ttbShipyardAllowed  = 1;
const TechTreeBit   c_ttbExpansionAllowed = 2;
const TechTreeBit   c_ttbTacticalAllowed  = 3;
const TechTreeBit   c_ttbSupremacyAllowed = 4;

// these #defs need to be modified when the number of bits changes
const TechTreeBit   c_ttbMax = 400;
#define CbTechBits 400 // keep in sync w/ TechBits sql data type
#define CbTechBitName 40
#define cTechs 400

typedef short   GlobalAttributeID;
typedef short   GlobalAttribute;
const GlobalAttribute   c_gaMaxSpeed                    =  0;   //Implemented
const GlobalAttribute   c_gaThrust                      =  1;   //I

const GlobalAttribute   c_gaTurnRate                    =  2;   //I
const GlobalAttribute   c_gaTurnTorque                  =  3;   //I

const GlobalAttribute   c_gaMaxArmorStation             =  4;   //I
const GlobalAttribute   c_gaArmorRegenerationStation    =  5;   //I

const GlobalAttribute   c_gaMaxShieldStation            =  6;   //I
const GlobalAttribute   c_gaShieldRegenerationStation   =  7;   //I

const GlobalAttribute   c_gaMaxArmorShip                =  8;   //I
const GlobalAttribute   c_gaMaxShieldShip               =  9;   //I
const GlobalAttribute   c_gaShieldRegenerationShip      = 10;   //I

const GlobalAttribute   c_gaScanRange                   = 11;   //I
const GlobalAttribute   c_gaSignature                   = 12;   //I

const GlobalAttribute   c_gaMaxEnergy                   = 13;   //I

const GlobalAttribute   c_gaSpeedAmmo                   = 14;   //I
const GlobalAttribute   c_gaLifespanEnergy              = 15;   //I

const GlobalAttribute   c_gaTurnRateMissile             = 16;   //I

const GlobalAttribute   c_gaMiningRate                  = 17;   //I
const GlobalAttribute   c_gaMiningYield                 = 18;   //I
const GlobalAttribute   c_gaMiningCapacity              = 19;   //I

const GlobalAttribute   c_gaRipcordTime                 = 20;   //I

const GlobalAttribute   c_gaDamageGuns                  = 21;   //I
const GlobalAttribute   c_gaDamageMissiles              = 22;   //I

const GlobalAttribute   c_gaDevelopmentCost             = 23;   //I
const GlobalAttribute   c_gaDevelopmentTime             = 24;   //I

const GlobalAttribute   c_gaMax = 25;

typedef unsigned char StationClassID;
const StationClassID    c_scStarbase                    = 0;
const StationClassID    c_scGarrison                    = 1;
const StationClassID    c_scShipyard                    = 2;
const StationClassID    c_scRipcord                     = 3;
const StationClassID    c_scMining                      = 4;
const StationClassID    c_scResearch                    = 5;
const StationClassID    c_scOrdinance                   = 6;
const StationClassID    c_scElectronics                 = 7;

typedef char    WarningMask;
const WarningMask   c_wmOutOfBounds   = 0x01;
const WarningMask   c_wmCrowdedSector = 0x02;

typedef char BuyableGroupID;

/*
� Max Speed: Up the sides maximum speed
� Rate of Yaw: Increase angle of turn per sec
� Rate of Pitch: Increase angle of turn per sec
� Rate of acceleration: Increase your acceleration
� Energy level: Boosts available energy for firing weapons and other stuff.
� Energy Level recharge rate: Faster recharge rate
� Mounts: Give ships bigger carrying capabilities.
� Shield strength: Better shields more hp
� Shield Recharge: Better recharge.
� Hull strength: Better Armor more hp
� Afterburner fuel efficiency: More miles per gallon
� Afterburner speed multiplier: Top speed enhanced
� Signature: Dampens your global signature.
� Scan Range: Ups the area by an amount
� Space station shield strength: More hit points.
� Space station hull strength: More hull hit points
� Miner productivity: Increase the dollar amount given to players at the completion of a round trip.
� Weapon Cost: Make all weapons a little cheaper
� Equipment Cost: make all equipment cheaper
� Ship Cost: Make ships cheaper
� Missile Speed/Accuracy?/Recalc time: A global attribute to improve Missiles chance of hitting.
� Projectile Damage: Ups damage
� Missile Damage: Ups damage
� Weapon Range: Improves distance
� Drone targeting : Improves accuracy
� Auto turret accuracy rating.: Improves accuracy (possibly same as above.)



*/
enum STAGE // Keep in sync with AGCGameStage in AGCIDL.h
{
  STAGE_NOTSTARTED,
  STAGE_STARTING,
  STAGE_STARTED,
  STAGE_OVER,
  STAGE_TERMINATE
};


/*
** PilotType ... these are the various automatic pilots that can be plugged into a ship
*/
typedef unsigned char PilotType;
const PilotType c_ptMiner       =  0;
const PilotType c_ptWingman     =  2;
const PilotType c_ptLayer       =  5;
const PilotType c_ptBuilder     =  6;
const PilotType c_ptCarrier     =  9;
const PilotType c_ptPlayer      = 10;
const PilotType c_ptCheatPlayer = 11;

const unsigned char             c_ucRenderNone  = 0x00;
const unsigned char             c_ucRenderTrail = 0x01;
const unsigned char             c_ucRenderAll   = 0x02;

const unsigned char             c_ucRadarOnScreenLarge = 0;
const unsigned char             c_ucRadarOnScreenSmall = 1;
const unsigned char             c_ucRadarOffScreen = 2;

typedef short                   PartMask;

//Imago has "generic" part mask flags 7/29/08
typedef PartMask				PartBitMask;
const PartBitMask				c_pbm1	= 0x01;
const PartBitMask				c_pbm2  = 0x02;
const PartBitMask				c_pbm3  = 0x04;
const PartBitMask				c_pbm4  = 0x08;
const PartBitMask				c_pbm5  = 0x10;
const PartBitMask				c_pbm6  = 0x20;
const PartBitMask				c_pbm7  = 0x40;
const PartBitMask				c_pbm8  = 0x80;
const PartBitMask				c_pbm9  = 0x100;
const PartBitMask				c_pbm10  = 0x200;
const PartBitMask				c_pbm11  = 0x400;
const PartBitMask				c_pbm12  = 0x800;
const PartBitMask				c_pbm13  = 0x1000;
const PartBitMask				c_pbm14  = 0x2000;
const PartBitMask				c_pbm15  = 0x4000;
const PartBitMask				c_pbm16  = (short)0x8000;

typedef short                   AbilityBitMask;

typedef AbilityBitMask          HullAbilityBitMask;
const HullAbilityBitMask        c_habmBoard                 = 0x01;
const HullAbilityBitMask        c_habmRescue                = 0x02;
const HullAbilityBitMask        c_habmLifepod               = 0x04;
const HullAbilityBitMask        c_habmCaptureThreat         = 0x08;
const HullAbilityBitMask        c_habmLandOnCarrier         = 0x10;
const HullAbilityBitMask        c_habmNoRipcord             = 0x20;
const HullAbilityBitMask        c_habmIsRipcordTarget       = 0x40;
const HullAbilityBitMask        c_habmFighter               = 0x80;
const HullAbilityBitMask        c_habmRemoteLeadIndicator   = 0x100;
const HullAbilityBitMask        c_habmThreatToStation       = 0x200;
const HullAbilityBitMask        c_habmCarrier               = 0x400;
const HullAbilityBitMask        c_habmLeadIndicator         = 0x800;
const HullAbilityBitMask        c_habmIsLtRipcordTarget     = 0x1000;
const HullAbilityBitMask        c_habmCanLtRipcord          = 0x2000;
const HullAbilityBitMask        c_habmMiner                 = 0x4000;
const HullAbilityBitMask        c_habmBuilder               = (short)0x8000;

typedef AbilityBitMask          StationAbilityBitMask;
const StationAbilityBitMask     c_sabmUnload                = 0x01;      //Ability to offload mined minerals
const StationAbilityBitMask     c_sabmStart                 = 0x02;      //           start the game at this station
const StationAbilityBitMask     c_sabmRestart               = 0x04;      //           restart after dying
const StationAbilityBitMask     c_sabmRipcord               = 0x08;      //           teleport to the station
const StationAbilityBitMask     c_sabmCapture               = 0x10;      //           be captured
const StationAbilityBitMask     c_sabmLand                  = 0x20;      //           land at
const StationAbilityBitMask     c_sabmRepair                = 0x40;      //           get repaired
const StationAbilityBitMask     c_sabmRemoteLeadIndicator   = 0x80;      //           relay lead indicator
const StationAbilityBitMask     c_sabmReload                = 0x100;     //           free fuel and ammo on launch
const StationAbilityBitMask     c_sabmFlag                  = 0x200;     //           counts for victory
const StationAbilityBitMask     c_sabmPedestal              = 0x400;     //           be a pedestal for a flag
const StationAbilityBitMask     c_sabmTeleportUnload        = 0x800;     //           offload mined materials without docking
const StationAbilityBitMask     c_sabmCapLand               = 0x1000;    //           land capital ships
const StationAbilityBitMask     c_sabmRescue                = 0x2000;    //           rescue pods
const StationAbilityBitMask     c_sabmRescueAny             = 0x4000;    //           not used (but reserved for pods)

typedef AbilityBitMask          AsteroidAbilityBitMask;
const AsteroidAbilityBitMask    c_aabmMineHe3      = 0x01;      //Has minable ore of some type (all mutually exclusive)
const AsteroidAbilityBitMask    c_aabmMineLotsHe3  = 0x02;
const AsteroidAbilityBitMask    c_aabmMineGold     = 0x04;

const AsteroidAbilityBitMask    c_aabmBuildable    = 0x08;      //Buildings can be built on it
const AsteroidAbilityBitMask    c_aabmSpecial      = 0x10;      //Special buildings can be built on it

typedef TLargeBitMask<c_ttbMax> TechTreeBitMask;

typedef AbilityBitMask          ExpendableAbilityBitMask;
const ExpendableAbilityBitMask  c_eabmCapture         = 0x01;
const ExpendableAbilityBitMask  c_eabmWarpBombDual    = 0x02; // KGJV: both sides aleph rez
const ExpendableAbilityBitMask  c_eabmWarpBombSingle  = 0x04; // KGJV: one side aleph rez
const ExpendableAbilityBitMask  c_eabmWarpBomb        = c_eabmWarpBombDual | c_eabmWarpBombSingle; // KGJV: both types into one for backward compatibility
const ExpendableAbilityBitMask  c_eabmQuickReady      = 0x08;
const ExpendableAbilityBitMask  c_eabmRipcord         = 0x10;
const ExpendableAbilityBitMask  c_eabmShootStations   = 0x20;
const ExpendableAbilityBitMask  c_eabmShootShips      = 0x40;
const ExpendableAbilityBitMask  c_eabmShootMissiles   = 0x80;
const ExpendableAbilityBitMask  c_eabmShootOnlyTarget = 0x1000;
const ExpendableAbilityBitMask  c_eabmRescue          = c_sabmRescue;     //0x2000 Rescue lifepods that collide with it
const ExpendableAbilityBitMask  c_eabmRescueAny       = c_sabmRescueAny;  //0x4000 Rescue any lifepod that collide with it

typedef short AchievementMask;
const AchievementMask c_achmProbeKill = 0x01;
const AchievementMask c_achmProbeSpot = 0x02;
const AchievementMask c_achmNewRepair = 0x04;
const AchievementMask c_achmGarrSpot = 0x08;

enum    ShipControlStateIGC
{
    selectedWeaponOneIGC        =  1,
    selectedWeaponTwoIGC        =  2 * selectedWeaponOneIGC,        //2 bits of weapon selection
    selectedWeaponMaskIGC       = (2 * selectedWeaponTwoIGC) - 1,
    selectedWeaponShiftIGC      =  0,

    coastButtonIGC              =    2 * selectedWeaponTwoIGC,      //Vector lock
    backwardButtonIGC           =    2 * coastButtonIGC,            //Thrust back
    forwardButtonIGC            =    4 * coastButtonIGC,            //       forward
    leftButtonIGC               =    8 * coastButtonIGC,            //       left
    rightButtonIGC              =   16 * coastButtonIGC,            //       right
    upButtonIGC                 =   32 * coastButtonIGC,            //       up
    downButtonIGC               =   64 * coastButtonIGC,            //       down
    afterburnerButtonIGC        =  128 * coastButtonIGC,            //       with afterburners
	keyMaskIGC					=  (256 * coastButtonIGC - 4),		//Xynth #210 coastButton-afterburnerbutton
    drillingMaskIGC             =  256 * coastButtonIGC,            //on rails to avoid collisions
    cloakActiveIGC              =  512 * coastButtonIGC,            //Activating the cloak
    droneRipMaskIGC             = 1024 * coastButtonIGC,            //Xynth #47 7/2010
    miningMaskIGC               = 2048 * coastButtonIGC,            //Play mine effect
    buttonsMaskIGC              = 4095 * coastButtonIGC,            //12 possible state buttons

    oneWeaponIGC                =    2 * miningMaskIGC,
    allWeaponsIGC               =    2 * oneWeaponIGC,
    weaponsMaskIGC              =    3 * oneWeaponIGC,              //2 possible weapon buttons

    //High word: these are not transmitted as part of the state mask
    missileFireIGC              =  0x10000,                         //Trying to shoot missiles
    mineFireIGC                 =  2 * missileFireIGC,              //                mines
    chaffFireIGC                =  2 * mineFireIGC,                 //                chaff
    wantsToMineMaskIGC          =  2 * chaffFireIGC,                //wants to mine an asteroid
    buildingMaskIGC             =  2 * wantsToMineMaskIGC           //in the process of building at an asteroid
};

// No "*Types" please if the VALUE of variable is a not a type of something! :-)
// ships have a capacity, and mounted parts use up that capacity (drain)
typedef float HitPoints;
typedef int   Money;
typedef char  Mount;
typedef char  Command;
typedef short RankID;

const Mount c_maxCargo = 5;
const Mount c_mountNA = -c_maxCargo - 1;

const Command c_cmdQueued   = 0;
const Command c_cmdAccepted = 1;
const Command c_cmdCurrent  = 2;
const Command c_cmdPlan     = 3;
const Command c_cmdMax      = 4;

typedef unsigned char Axis;
const Axis c_axisYaw       = 0;
const Axis c_axisPitch     = 1;
const Axis c_axisRoll      = 2;
const Axis c_axisThrottle  = 3;
const Axis c_axisMax       = 4;

//Constants used to avoid having to allocate arrays.
const Mount   c_maxUnmannedWeapons = 4;
const Mount   c_maxMountedWeapons = 8;

struct  CommandData
{
    char            szVerb[c_cbFileName];
    char            szAccepted[c_cbFileName];
    char            szQueued[c_cbFileName];

    enum    Match
    {
        c_mNone, c_mPartial, c_mComplete
    };

    Match   MatchCommand(const char*    szString) const
    {
        assert (szString);
        const char* p1 = szString;
        const char* p2 = szVerb;

        while ((*p1 != '\0') && (*p2 != '\0'))
        {
            if (tolower(*p1) != tolower(*p2))
                return c_mNone;

            p1++;
            p2++;
        }

        return ((*p1 == '\0') && (*p2 == '\0')) ? c_mComplete : c_mPartial;
    }
};

//Must agree to CommandID constants

typedef char CommandID;
const CommandID   c_cidDoNothing    = -3;
const CommandID   c_cidMyAccepted   = -2;
const CommandID   c_cidNone         = -1;
const CommandID   c_cidDefault      =  0;
const CommandID   c_cidAttack       =  1;
const CommandID   c_cidCapture      =  2;
const CommandID   c_cidDefend       =  3;
const CommandID   c_cidPickup       =  4;
const CommandID   c_cidGoto         =  5;
const CommandID   c_cidRepair       =  6;
const CommandID   c_cidJoin         =  7;
const CommandID   c_cidMine         =  8;
const CommandID   c_cidBuild        =  9;

const CommandID   c_cidMax = 10;
extern const CommandData   c_cdAllCommands[c_cidMax];

struct GlobalAttributeSet
{
    public:
        GlobalAttributeSet(void)
        {
            //Initialize();
        }

        void    Initialize(void)
        {
            for (int i = 0; (i < c_gaMax); i++)
                m_attributes[i] = 1.0f;
        }

        void    Set(const float*    a)
        {
            for (int i = 0; (i < c_gaMax); i++)
                m_attributes[i] = a[i];
        }

        void    Apply(const GlobalAttributeSet& gas)
        {
            for (int i = 0; (i < c_gaMax); i++)
            {
                assert (m_attributes[i] > 0.0f);
                assert (m_attributes[i] < 100.0f);
                assert (gas.m_attributes[i] > 0.0f);
                assert (gas.m_attributes[i] < 100.0f);

                m_attributes[i] *= gas.m_attributes[i];
            }
        }

        float    GetAttribute(GlobalAttribute ga) const
        {
            assert (m_attributes[ga] > 0.0f);
            assert (m_attributes[ga] < 100.0f);

            return m_attributes[ga];
        }
        void    SetAttribute(GlobalAttribute ga, float v)
        {
            m_attributes[ga] = v;
        }

        bool    operator == (const GlobalAttributeSet& gas) const
        {
            for (int i = 0; (i < c_gaMax); i++)
            {
                if (m_attributes[i] != gas.m_attributes[i])
                    return false;
            }

            return true;
        }

    private:
        float   m_attributes[c_gaMax];
};

typedef short MapMakerID;

const MapMakerID c_mmSingleRing     = 0;
const MapMakerID c_mmDoubleRing     = 1;
const MapMakerID c_mmPinWheel       = 2;
const MapMakerID c_mmDiamondRing    = 3;
const MapMakerID c_mmSnowFlake      = 4;
const MapMakerID c_mmSplitBase      = 5;
const MapMakerID c_mmBrawl          = 6;
const MapMakerID c_mmBigRing        = 7;
const MapMakerID c_mmHiLo           = 8;
const MapMakerID c_mmHiHigher       = 9;
const MapMakerID c_mmStar           = 10;
const MapMakerID c_mmInsideOut      = 11;
const MapMakerID c_mmGrid           = 12;
const MapMakerID c_mmEastWest       = 13;
const MapMakerID c_mmLargeSplit     = 14;
const MapMakerID c_mmMax            = 15;

const int c_cbGameName = 65;
const int c_cbGamePassword = 17;

//------------------------------------------------------------------------------
// for now, there is only one static data core, and this is its name
//------------------------------------------------------------------------------
#define IGC_STATIC_CORE_FILENAME    "static_core"
#define IGC_ENCRYPT_CORE_FILENAME   "zone_core"


const float c_fMissionBriefingCountdown = 15.0f; // seconds

const int c_cUnlimitedLives = 0x7fff;

//Forward definitions of interfaces
struct MissionParams;
class ThingSite;
class ClusterSite;

class CstaticIGC;

class   IIgcSite;

class   ImissionIGC;

class   IbaseIGC;

class   ImodelIGC;
class   IbuoyIGC;

class   IdamageIGC;
class   IscannerIGC;
class   IshipIGC;
class   IprojectileIGC;
class   IstationIGC;

class   IbuyableIGC;

class   IprojectileTypeIGC;
class   ItypeIGC;
class   IexpendableTypeIGC;
class   ImissileTypeIGC;
class   ImineTypeIGC;
class   IprobeTypeIGC;
class   IchaffTypeIGC;

class   IhullTypeIGC;
class   IpartTypeIGC;
class   IlauncherTypeIGC;

class   IpartIGC;
class   IweaponIGC;
class   IshieldIGC;
class   IcloakIGC;
class   IafterburnerIGC;
class   IpackIGC;
class   IlauncherIGC;
class   ImagazineIGC;
class   IdispenserIGC;
class   ImissileIGC;
class   ImineIGC;
class   IprobeIGC;
class   ImunitionIGC;
class   IbuildingEffectIGC;

class   IclusterIGC;

class   IasteroidIGC;
class   IwarpIGC;
class   ItreasureIGC;

class   IsideIGC;
class   IcivilizationIGC;

class   IstationTypeIGC;
class   IdroneTypeIGC;
class   IdevelopmentIGC;
class   IbucketIGC;

class   ItreasureSetIGC;

struct  WarpBomb
{
    Time                timeExplosion;
    ImissileTypeIGC*    pmt;
};
typedef Slist_utl<WarpBomb> WarpBombList;
typedef Slink_utl<WarpBomb> WarpBombLink;

class ImapMakerIGC
{
    public:
        static const char*  IsValid(const MissionParams * pmp);
        static void         Create(Time                   now,
                                   const MissionParams*   pmp,
                                   ImissionIGC*           pmission);

        virtual MapMakerID  GetMapMakerID() const = 0;
        virtual VOID        SetMapMakerID(MapMakerID mbID) = 0;
        virtual char const* GetName(void) const = 0;
        virtual void        SetName(const char* newVal) = 0;

        virtual VOID        GenerateMission(Time                 now,
                                            const MissionParams* pmp,
                                            ImissionIGC*         pMission) = 0;
};


struct MissionParams
{
    //------------------------------------------------------------------------------
    // If you add or remove properties, please update
    // AGCGameParameters.cpp, AGCGameParameters.h, and AGCIDL.h.
    //
    //------------------------------------------------------------------------------
    char        strGameName[c_cbGameName];              //Name of game
    char        szIGCStaticFile[c_cbFileName];          //Name of static IGC file
    char        szCustomMapFile[c_cbFileName];          //Name of custom Map file; used only if not blank
    char        strGamePassword[c_cbGamePassword];      //Password
    bool        bEjectPods          : 1;                //Are eject pods used
    bool        bInvulnerableStations : 1;              //Do station NOT take damage
    bool        bShowMap            : 1;                //Show all warps at the start of the game
    bool        bAllowPrivateTeams  : 1;
    bool        bAllowEmptyTeams    : 1;                //Allow teams without players
	bool		bAllowAlliedRip		: 1;				//Imago 7/8/09 ALLY
	bool		bAllowAlliedViz		: 1;				//Imago 7/11/09 ALLY
    bool        bAllowDevelopments  : 1;                //Allow investment in tech
    bool        bAllowShipyardPath  : 1;                //Allow building Shipyards
    bool        bAllowTacticalPath  : 1;                //Allow building Tactical Labs
    bool        bAllowSupremacyPath : 1;                //Allow building Supremacy Centers
    bool        bAllowExpansionPath : 1;                //Allow building Expansion Complexes
    bool        bPowerUps           : 1;                //Create treasure when a ship is destroyed
    bool        bAllowDefections    : 1;                //Allow players to switch from one team to another
    bool        bAllowJoiners       : 1;                //Allow players to join a game in progress
    bool        bLockLobby          : 1;                //Prevent new players from joining a game
    bool        bLockSides          : 1;                //Prevent players from switching sides
    bool        bLockTeamSettings   : 1;                //Prevent players from changing team attributes like name
    bool        bLockGameOpen       : 1;                //Prevent players from limiting the size of the game
    bool        bStations           : 1;        //??
    bool        bScoresCount        : 1;        //??
    bool        bSquadGame          : 1;        //??
    bool        bDrones             : 1;        //??
    bool        iResources          : 1;        //??
    bool        bResourceAmountsVisible : 1;    //??
    bool        bRandomWormholes    : 1;        //??
    bool        bNoTeams            : 1;        //??
    bool        bShowHomeSector     : 1;                //Show everything in a player's home sector at start of game
    bool        bAllowFriendlyFire  : 1;                //Allow friends to damage friends
    bool        bObjectModelCreated : 1;                //Was this game created by admin tools or the server app?
    bool        bLobbiedGame        : 1;                //Is this game listed in an internet lobby?
    bool        bClubGame           : 1;                //Is this game on the zone club?
    bool        bAutoStart          : 1;                //Does the game start automatically when all sides are ready?
    bool        bAutoRestart        : 1;                //Does the game restart automatically
    bool        bAllowRestart       : 1;                //Can the game be restarted at all?
	bool        bExperimental       : 1;                // mmf 10/07 Experimental game type
	float       fGoalTeamMoney;                         //Cost of win the game tech = fGoalTeamMoney * WinTheGameMoney, 0 == no win the game tech
    int         verIGCcore;                             //this is set only by the server, so the client can know whether it needs to get a new igc static core
    float       nPlayerSectorTreasureRate;              //# of treasures that generate/second in player sectors
    float       nNeutralSectorTreasureRate;             //                                       neutral
    float       dtGameLength;                           //Seconds till end of game, 0 == no limit
    float       fHe3Density;                            //Mulitplier on He3 found at asteroids
    Money       m_killPenalty;                  //Not used
    Money       m_killReward;                   //Not used
    Money       m_ejectPenalty;                 //Not used
    Money       m_ejectReward;                  //Not used

    Time        timeStart;                              //Time at which the game started
    float       fStartCountdown;                        //Countdown (seconds) between automatically restarting missions
    float       fRestartCountdown;                      //Countdown (seconds) between automatically restarting missions

    CivID       rgCivID[c_cSidesMax];                   //IDs within the szIGCCore data set
    short       iGoalConquestPercentage;                //% of flagged stations that need to be held to win the game
    short       iGoalTerritoryPercentage;               //sole control of % of territories
    short       iGoalArtifactsPercentage;       //Not used
    short       nGoalFlagsCount;                        //# of enemy flags returned to station to win
    short       nGoalArtifactsCount;                    //# of neutral artifacts returned to station to win
    short       nGoalTeamKills;                         //Number of kills required to end the game
    short       tsiPlayerStart;                         //Treasure index for random treasures that start in player sectors. NA == none
    short       tsiNeutralStart;                        //                                                  neutral
    short       tsiPlayerRegenerate;                    //Treasure index for random treasures that spawn in player sectors, NA = none
    short       tsiNeutralRegenerate;                   //                                                  neutral
    float       fStartingMoney;                         //Multiplier on team starting money
    short       iLives;                                 //Player must die more than this number of times to be exit, c_cUnlimitedLives = unlimited
    MapMakerID  mmMapType;                              //Map type
    short       iMapSize;								//KGJV: non zero value = 2 starting garrisons
    short       iRandomEncounters;                      //Use to indicate how many alephs are randomly removed
    short       bNeutralSectors;                //Not used
    short       iAlephPositioning;              //Not used
    short       iAlephsPerSector;               //Not used
    short       nTeams;                                 //Number of sides in the game
    short       iMinRank;                               //Minimum player rank
    short       iMaxRank;                               //Maximum player rank
    int         nInvitationListID;                      // zero means no invitation required

    short       iMaxImbalance;                          //Maximum allowed difference between smallest and largest team

    short       nPlayerSectorAsteroids;                 //# generic asteroids in player sector
    short       nNeutralSectorAsteroids;                //                       neutral

    short       nPlayerSectorTreasures;                 //# of treasures placed in player sector to start with
    short       nNeutralSectorTreasures;                //                         neutral

    short       nPlayerSectorMineableAsteroids;         //# of He3 asteroids in player sector
    short       nNeutralSectorMineableAsteroids;        //                      neutral

    short       nPlayerSectorSpecialAsteroids;          //# of special asteroids (C, U or Si) in player sector
    short       nNeutralSectorSpecialAsteroids;         //                                       neutral

    unsigned char nMinPlayersPerTeam;                   //Min players on team
    unsigned char nMaxPlayersPerTeam;                   //Max players on team

    char        nInitialMinersPerTeam;                  //Number of miners to start the game with
    char        nMaxDronesPerTeam;                      //Maximum # of drones a team is allowed to control

    short       nTotalMaxPlayersPerGame;                //Maximum # of players per game (mostly used for StandAlone server)

    MissionParams()
    {
        Reset();
    }

    void    Reset(void)
    {
        ZeroMemory(this, sizeof(*this));
        //
        // default to reasonable values
        //
        strcpy(strGameName, "Uninitialized Game Name");
        szIGCStaticFile[0] = '\0';
        m_killPenalty                   = 0;
        m_killReward                    = 0;

        m_ejectPenalty                  = 0;
        m_ejectReward                   = 0;

        bEjectPods                      = true ;
        bInvulnerableStations           = false;
        bAllowPrivateTeams              = true ;
        bAllowEmptyTeams                = false;
		bAllowAlliedRip	                = false; //imago 7/8/09 ALLY
		bAllowAlliedViz	                = false; //imago 7/8/09 ALLY 7/17/09 done testing, defaults off unless allies
        bShowMap                        = false;
        bAllowDevelopments              = true ;
        bAllowShipyardPath              = true ;
        bAllowTacticalPath              = true ;
        bAllowSupremacyPath             = true ;
        bAllowExpansionPath             = true ;
        bPowerUps                       = true ;
        bAllowJoiners                   = true ;
        bLockLobby                      = false;
        bLockSides                      = false;
        bLockTeamSettings               = false;
        bAllowDefections                = false;
        bStations                       = true ;
        bScoresCount                    = true;
        bSquadGame                      = false;
        bDrones                         = true ;
        iResources                      = 0;
        bResourceAmountsVisible         = true ;
        bRandomWormholes                = true ;
        bNoTeams                        = false;
        bObjectModelCreated             = false;
        bLobbiedGame                    = false;
        bClubGame                       = false;
        bAutoStart                      = false;
        bAutoRestart                    = false;
        bAllowRestart                   = true;
		bExperimental                   = false; // mmf 10/07 Experimental game type
        nInvitationListID               = 0;

        fStartCountdown                 = 15.0f;
        fRestartCountdown               = 60.0f;

        //
        // Assign to NA for now, this cues the mission maker to reset it later
        //
        for (int iSide = 0; iSide < c_cSidesMax; iSide++)
        {
            rgCivID[iSide] = NA;
        }

        fHe3Density                     = 1.0f;
        fStartingMoney                  = 1.0f;
        fGoalTeamMoney                  = 0;
        tsiPlayerStart                  = 1;
        tsiNeutralStart                 = 1;
        tsiPlayerRegenerate             = 1;
        tsiNeutralRegenerate            = 1;
        iLives                          = c_cUnlimitedLives;
        nGoalTeamKills                  = 0;
        mmMapType                       = c_mmHiLo;
        iMapSize                        = 0;
        iRandomEncounters               = 0;
        bNeutralSectors                 = true;
        iAlephPositioning               = 0;
        iAlephsPerSector                = 0;
        nTeams                          = 2;
        iMinRank                        = -1;
        iMaxRank                        = 1000;

        iMaxImbalance                   = 1;

        dtGameLength                    = 0.0f;

        iGoalConquestPercentage         = 100;
        iGoalTerritoryPercentage        = 100;
        iGoalArtifactsPercentage        = 100;
        nGoalFlagsCount                 = 0;
        nGoalArtifactsCount             = 0;

        bShowHomeSector                 = false;

        nPlayerSectorAsteroids          = 20;
        nNeutralSectorAsteroids         = 20;

        nPlayerSectorTreasures          = 0;
        nNeutralSectorTreasures         = 5;

        nPlayerSectorTreasureRate       = 0.2f / 60.0f;  //0.2 per minute == 0.2/60 per second
        nNeutralSectorTreasureRate      = 0.5f / 60.0f;

        nPlayerSectorMineableAsteroids  = 2;
        nNeutralSectorMineableAsteroids = 4;

        nPlayerSectorSpecialAsteroids   = 1;
        nNeutralSectorSpecialAsteroids  = 1;

        nInitialMinersPerTeam           = 1;
        nMaxDronesPerTeam               = 4;

        nMinPlayersPerTeam              = 1;   // KGJV #114 changed
        nMaxPlayersPerTeam              = 100; // KGJV #114 changed

        nTotalMaxPlayersPerGame         = 0x7FFF;
    }

    const char* Invalid(bool bFromClient = false) const
    {
        // do some data validation... should do this is a cleaner way

        if (bFromClient)
        {
            if ((nMaxPlayersPerTeam > 100) ||
                 (nMaxPlayersPerTeam < 1))
            {
                return "Maximum number of players must be between 1 and 100.";
            }
            else if (IsNoWinningConditionsGame())
            {
                return "You must choose one or more winning conditions.";
            }
			else if (bAutoRestart || (bObjectModelCreated && bClubGame)) // KGJV #62 || bAllowEmptyTeams)
            {
                return "HACK: one or more admin-only flags are set.";
            }
        }

        if (nInitialMinersPerTeam > nMaxDronesPerTeam)
        {
            return "Initial miners per team must be less than max miners per team.";
        }
        else if (nMaxDronesPerTeam > 10)
        {
            return "Max drones per team must be less than or equal to 10.";
        }
        else if (nNeutralSectorSpecialAsteroids > 9)
        {
            return "NeutralSectorSpecialAsteroids must be less than 10.";
        }
        else if (nPlayerSectorSpecialAsteroids > 9)
        {
            return "PlayerSectorSpecialAsteroids must be less than 10.";
        }
        else if (nNeutralSectorMineableAsteroids > 9)
        {
            return "NeutralSectorMineableAsteroids must be less than 10.";
        }
        else if (nPlayerSectorMineableAsteroids > 9)
        {
            return "PlayerSectorMineableAsteroids must be less than 10.";
        }
        else if (nNeutralSectorTreasureRate > 1.0f/60.0f)
        {
            return "NeutralSectorTreasureRate must be less than 1/60.";
        }
        else if (nPlayerSectorTreasureRate > 1.0f/60.0f)
        {
            return "PlayerSectorTreasureRate must be less than 1/60.";
        }
        else if (nNeutralSectorTreasures > 20)
        {
            return "NeutralSectorTreasures must be less than 21.";
        }
        else if (nPlayerSectorTreasures > 20)
        {
            return "PlayerSectorTreasures must be less than 21.";
        }
        else if (nNeutralSectorAsteroids > 40)
        {
            return "NeutralSectorAsteroids must be less than 40.";
        }
        else if (nPlayerSectorAsteroids > 40)
        {
            return "PlayerSectorAsteroids must be less than 40.";
        }
        else if ((nGoalArtifactsCount < 0) || (nGoalArtifactsCount > 100))
        {
            return "GoalArtifactsCount must be between 0% and 100%.";
        }
        else if ((nGoalFlagsCount < 0) || (nGoalFlagsCount > 100))
        {
            return "GoalFlagsCount must be between 0% and 100%.";
        }
        else if ((fHe3Density < 0.0f) || (fHe3Density > 9.0f))
        {
            return "He3Density must be between 0 and 9.";
        }
        else if ((fGoalTeamMoney < 0.0f) || (fGoalTeamMoney > 9.0f))
        {
            return "GoalTeamMoney must be between 0 and 9.";
        }
        else if ((fStartingMoney < 0.0f) || (fStartingMoney > 9.0f))
        {
            return "StartingMoney must be between 0 and 9.";
        }
        else if (NULL == memchr(strGameName, 0, c_cbGameName))
        {
            return "invalid game name";
        }
        else if (NULL == memchr(szIGCStaticFile, 0, c_cbFileName))
        {
            return "invalid IGC static file name";
        }
        else if (NULL == memchr(szCustomMapFile, 0, c_cbFileName))
        {
            return "invalid IGC static file name";
        }
        else if (NULL == memchr(strGamePassword, 0, c_cbGamePassword))
        {
            return "invalid IGC static file name";
        }
        else if ((nTeams > 6) || (nTeams < 2))
        {
            return "Number of teams must be between 2 and 6.";
        }
        else if (nTotalMaxPlayersPerGame < 1)
        {
            return "Number of players per game must be greater than 0.";
        }
        else if ((nMaxPlayersPerTeam > 200) ||
                 (nMaxPlayersPerTeam < 1))
        {
            return "Maximum number of players must be between 1 and 200.";
        }
        else if (nMinPlayersPerTeam < 1)
        {
            return "Minimum number of players must be 1 or greater.";
        }
        else if (nMinPlayersPerTeam > nMaxPlayersPerTeam)
        {
            return "Minimum number of players must not be greater than the maximum number of players.";
        }
		// BT - STEAM - Removing the limitation on defections and imbal for Steam stats. Steam 
		// doesn't care how many teams you show up on. It's your play time that counts!
  //      else if (bScoresCount && bAllowDefections)
  //      {
  //          return "Scores can't be counted for a game where defections are allowed; "
  //              "please turn off defections or stats count.";
  //      }
		//// TE: Confirms that the MaxImbalance = AUTO when scores count
  //      else if (bScoresCount && iMaxImbalance != 0x7ffe)
  //      {
  //          return "Scores can't be counted for a game where the MaxImbalance setting is not Auto; "
  //              "please set the MaxImbalance setting to Auto, or turn off stats count.";
  //      }
        else if (IsConquestGame() && bInvulnerableStations)
        {
            return "You can't play a conquest game with invulnerable stations; "
                "You must disable the conquest winning condition or turn off "
                "the invulnerable stations option.";
        }
        else if (IsProsperityGame() && !bAllowDevelopments)
        {
            return "You can't play a prosperity game with developments turned off; "
                "You must disable the prosperity winning condition or turn on "
                "developments.";
        }
        else if (nPlayerSectorTreasureRate > 0.01 ||  nNeutralSectorTreasureRate > 0.01)
        {
            return "Treasure rate(s) too high; max is 0.01";
        }
        else if (IsTerritoryGame())
        {
            if (mmMapType == c_mmBrawl)
                return "Brawl maps can not have a territory win condition";
            else if ((nTeams == 2) &&
                     ((mmMapType == c_mmSingleRing) ||
                      (mmMapType == c_mmDoubleRing)))
                return "Small maps can not have a territory win condition";
        }

        return ImapMakerIGC::IsValid(this);
    }

    //
    // Winning Condition Info
    //

    bool IsConquestGame() const
    {
        return iGoalConquestPercentage != 0;
    }

    bool IsTerritoryGame() const
    {
        return iGoalTerritoryPercentage != 100;
    }

    bool IsDeathMatchGame() const
    {
        return nGoalTeamKills != 0;
    }

    bool IsCountdownGame() const
    {
        return dtGameLength != 0.0f;
    }

    bool IsProsperityGame() const
    {
        return fGoalTeamMoney != 0;
    }

    bool IsArtifactsGame() const
    {
        return (nGoalArtifactsCount != 0);
    }

    bool IsFlagsGame() const
    {
        return (nGoalFlagsCount != 0);
    }

    bool IsNoWinningConditionsGame() const
    {
        return !(IsConquestGame()   ||
                 IsDeathMatchGame() ||
                 IsCountdownGame()  ||
                 IsProsperityGame() ||
                 IsArtifactsGame()  ||
                 IsTerritoryGame()  ||
                 IsFlagsGame());
    }

    float GetCountDownTime() const
    {
        return dtGameLength;
    }

    int GetConquestPercentage() const
    {
        return iGoalConquestPercentage;
    }

    int GetTerritoryPercentage() const
    {
        return iGoalTerritoryPercentage;
    }

    short GetDeathMatchKillLimit() const
    {
        return nGoalTeamKills;
    }

    int GetArtifactsPercentage() const
    {
        return iGoalArtifactsPercentage;
    }
};

//Utility data structures
typedef  Slist_utl<IpartIGC*>           PartListIGC;
typedef  Slink_utl<IpartIGC*>           PartLinkIGC;

typedef  Slist_utl<IclusterIGC*>        ClusterListIGC;
typedef  Slink_utl<IclusterIGC*>        ClusterLinkIGC;

typedef  Slist_utl<IshipIGC*>           ShipListIGC;
typedef  Slink_utl<IshipIGC*>           ShipLinkIGC;

typedef  Slist_utl<IscannerIGC*>        ScannerListIGC;
typedef  Slink_utl<IscannerIGC*>        ScannerLinkIGC;

typedef  Slist_utl<IpartTypeIGC*>       PartTypeListIGC;
typedef  Slink_utl<IpartTypeIGC*>       PartTypeLinkIGC;

typedef  Slist_utl<ItreasureSetIGC*>    TreasureSetListIGC;
typedef  Slink_utl<ItreasureSetIGC*>    TreasureSetLinkIGC;

typedef  Slist_utl<IprojectileTypeIGC*> ProjectileTypeListIGC;
typedef  Slink_utl<IprojectileTypeIGC*> ProjectileTypeLinkIGC;

typedef  Slist_utl<ImissileIGC*>        MissileListIGC;
typedef  Slink_utl<ImissileIGC*>        MissileLinkIGC;

typedef  Slist_utl<ImineIGC*>           MineListIGC;
typedef  Slink_utl<ImineIGC*>           MineLinkIGC;

typedef  Slist_utl<IprobeIGC*>          ProbeListIGC;
typedef  Slink_utl<IprobeIGC*>          ProbeLinkIGC;

typedef  Slist_utl<IhullTypeIGC*>       HullTypeListIGC;
typedef  Slink_utl<IhullTypeIGC*>       HullTypeLinkIGC;

typedef  Slist_utl<IcivilizationIGC*>   CivilizationListIGC;
typedef  Slink_utl<IcivilizationIGC*>   CivilizationLinkIGC;

typedef  Slist_utl<IstationTypeIGC*>    StationTypeListIGC;
typedef  Slink_utl<IstationTypeIGC*>    StationTypeLinkIGC;

typedef  Slist_utl<IdroneTypeIGC*>      DroneTypeListIGC;
typedef  Slink_utl<IdroneTypeIGC*>      DroneTypeLinkIGC;

typedef  Slist_utl<IdevelopmentIGC*>    DevelopmentListIGC;
typedef  Slink_utl<IdevelopmentIGC*>    DevelopmentLinkIGC;

typedef  Slist_utl<IsideIGC*>           SideListIGC;
typedef  Slink_utl<IsideIGC*>           SideLinkIGC;

typedef  Slist_utl<ImissileTypeIGC*>    MissileTypeListIGC;
typedef  Slink_utl<ImissileTypeIGC*>    MissileTypeLinkIGC;

typedef  Slist_utl<IexpendableTypeIGC*> ExpendableTypeListIGC;
typedef  Slink_utl<IexpendableTypeIGC*> ExpendableTypeLinkIGC;

typedef  Slist_utl<ImodelIGC*>          ModelListIGC;
typedef  Slink_utl<ImodelIGC*>          ModelLinkIGC;

typedef  Slist_utl<IwarpIGC*>           WarpListIGC;
typedef  Slink_utl<IwarpIGC*>           WarpLinkIGC;

typedef  Slist_utl<IstationIGC*>        StationListIGC;
typedef  Slink_utl<IstationIGC*>        StationLinkIGC;

typedef  Slist_utl<IbucketIGC*>         BucketListIGC;
typedef  Slink_utl<IbucketIGC*>         BucketLinkIGC;

typedef  Slist_utl<IasteroidIGC*>       AsteroidListIGC;
typedef  Slink_utl<IasteroidIGC*>       AsteroidLinkIGC;

typedef  Slist_utl<ItreasureIGC*>       TreasureListIGC;
typedef  Slink_utl<ItreasureIGC*>       TreasureLinkIGC;

typedef  Slist_utl<IbuoyIGC*>           BuoyListIGC;
typedef  Slink_utl<IbuoyIGC*>           BuoyLinkIGC;

typedef Slist_utl<IbaseIGC*>            BaseListIGC;
typedef Slink_utl<IbaseIGC*>            BaseLinkIGC;

struct  ControlData
{
    float   jsValues[4];    //yaw, pitch, roll, throttle

    inline void    Reset(void)
    {
        jsValues[c_axisYaw] = jsValues[c_axisPitch] = jsValues[c_axisRoll] = 0.0f;
        jsValues[c_axisThrottle] = -1.0f;
    }
};

#pragma pack(push, 1)
struct  PartData
{
    PartID          partID;
    short           amount;
    Mount           mountID;
};

struct  Stockpile
{
    TRef<IbuyableIGC>   buyable;
    short               count;
};

struct  ExpandedPartData : public PartData
{
    BytePercentage  fractionMounted;
};
#pragma pack(pop)

typedef Slist_utl<Stockpile>    StockpileList;
typedef Slink_utl<Stockpile>    StockpileLink;

struct  HardpointData
{
    //float       minDot;                     //Manned turrets only
    SoundID     interiorSound;
    SoundID     turnSound;

    char        frameName[c_cbFileName];
    //char        hudName[c_cbFileName];
    char        locationAbreviation[c_cbLocAbrev];
    PartMask    partMask;
    bool        bFixed;
};

struct AsteroidDef
{
    float                   ore;
    float                   oreMax;	
    AsteroidAbilityBitMask  aabmCapabilities;
    AsteroidID              asteroidID;
    HitPoints               hitpoints;
    short                   radius;
    char                    modelName[c_cbFileName];
    char                    textureName[c_cbFileName];
    char                    iconName[c_cbFileName];
};

struct  WarpDef
{
    WarpID              warpID;
    WarpID              destinationID;
    short               radius;
    char                textureName[c_cbFileName];
    char                iconName[c_cbFileName];
};

//Exported data structures ... these data structures define the data used by Initialize()
//for the various classes.
struct  DataClusterIGC
{
    unsigned int        starSeed;
    Vector              lightDirection;
    D3DCOLOR            lightColor;
    float               screenX;
    float               screenY;
    SectorID            clusterID;
    short               nDebris;
    short               nStars;
    char                name[c_cbName];
    char                posterName[c_cbFileName];
    char                planetName[c_cbFileName];
    BytePercentage      planetSinLatitude;
    BytePercentage      planetLongitude;
    unsigned char       planetRadius;
    bool                activeF;
    bool                bHomeSector;
};

struct  DataBuyableIGC
{
    Money               price;
    DWORD               timeToBuild;
    char                modelName[c_cbFileName + 1]; // because we allow a 1 character prefix
    char                iconName[c_cbFileName];
    char                name[c_cbName];
    char                description[c_cbDescription];
    BuyableGroupID      groupID;
    TechTreeBitMask     ttbmRequired;
    TechTreeBitMask     ttbmEffects;
};

const int   c_cMaxPreferredPartTypes = 14;
struct  DataHullTypeIGC : public DataBuyableIGC
{
    float               mass;
    float               signature;
    float               speed;
    float               maxTurnRates[3];    //yaw, pitch, roll
    float               turnTorques[3];      //yaw, pitch, roll
    float               thrust;
    float               sideMultiplier;
    float               backMultiplier;
    float               scannerRange;
    float               maxFuel;
    float               ecm;
    float               length;
    float               maxEnergy;
    float               rechargeRate;
    float               ripcordSpeed;
    float               ripcordCost;

    short               maxAmmo;

    HullID              hullID;
    HullID              successorHullID;
    Mount               maxWeapons;
    Mount               maxFixedWeapons;
    HitPoints           hitPoints;
    short               hardpointOffset;

    DefenseTypeID       defenseType;

    short               capacityMagazine;
    short               capacityDispenser;
    short               capacityChaffLauncher;

    PartID              preferredPartsTypes[c_cMaxPreferredPartTypes];

    HullAbilityBitMask  habmCapabilities;
    char                textureName[c_cbFileName];

    //char                pilotHUDName[c_cbFileName];
    //char                observerHUDName[c_cbFileName];

    PartMask            pmEquipment[ET_MAX];

    SoundID             interiorSound;
    SoundID             exteriorSound;
    SoundID             mainThrusterInteriorSound;
    SoundID             mainThrusterExteriorSound;
    SoundID             manuveringThrusterInteriorSound;
    SoundID             manuveringThrusterExteriorSound;


    // HardpointData[] comes here
};

struct  DataPartTypeIGC : public DataBuyableIGC
{
    float               mass;
    float               signature;
    PartID              partID;
    PartID              successorPartID;
    EquipmentType       equipmentType;
    PartMask            partMask;
    char                inventoryLineMDL[c_cbFileName];
};

struct  DataWeaponTypeIGC : public DataPartTypeIGC
{
    float               dtimeReady;
    float               dtimeBurst;
    float               energyPerShot;
    float               dispersion;
    short               cAmmoPerShot;
    ProjectileTypeID    projectileTypeID;
    SoundID             activateSound;
    SoundID             singleShotSound;
    SoundID             burstSound;
};

struct  DataShieldTypeIGC : public DataPartTypeIGC
{
    float           rateRegen;
    float           maxStrength;
    DefenseTypeID   defenseType;
    SoundID         activateSound;
    SoundID         deactivateSound;
};

struct  DataCloakTypeIGC : public DataPartTypeIGC
{
    float           energyConsumption;
    float           maxCloaking;
    float           onRate;
    float           offRate;
    SoundID         engageSound;
    SoundID         disengageSound;
};

struct  DataAfterburnerTypeIGC : public DataPartTypeIGC
{
    float           fuelConsumption;
    float           maxThrust;
    float           onRate;
    float           offRate;
    SoundID         interiorSound;
    SoundID         exteriorSound;
};

struct  DataPackTypeIGC : public DataPartTypeIGC
{
    PackType        packType;
    short           amount;
};

struct  DataLauncherTypeIGC
{
    short           amount;
    PartID          partID;
    PartID          successorPartID;
    short           launchCount;
    ObjectID        expendabletypeID;
    char            inventoryLineMDL[c_cbFileName];
};

struct  DataAsteroidIGC
{
    float               signature;
    Vector              position;
    Vector              up;
    Vector              forward;
    Rotation            rotation;
    AsteroidDef         asteroidDef;
    SectorID            clusterID;
    char                name[c_cbName];
    BytePercentage      fraction;
};

struct  DataObjectIGC
{
    COLORVALUE          color; // was D3DCOLORVALUE
    float               radius;
    float               rotation;
    char                modelName[c_cbFileName];
    char                textureName[c_cbFileName];
};

struct  DataBuoyIGC
{
    Vector              position;
    SectorID            clusterID;
    BuoyType            type;
    BuoyID              buoyID;

    DataBuoyIGC (void) {buoyID = NA;}
};

struct  DataProjectileTypeIGC : public DataObjectIGC
{
    float               power;
    float               blastPower;
    float               blastRadius;
    float               speed;
    float               lifespan;
    ProjectileTypeID    projectileTypeID;
    DamageTypeID        damageType;
    bool                absoluteF;
    bool                bDirectional;
    float               width;
    SoundID             ambientSound;
};

struct  LauncherDef : public DataBuyableIGC
{
    float               signature;
    float               mass;
    PartMask            partMask;
    short               expendableSize;
};

struct  DataExpendableTypeIGC : public DataObjectIGC
{
    float               loadTime;
    float               lifespan;
    float               signature;
    LauncherDef         launcherDef;
    HitPoints           hitPoints;
    DefenseTypeID       defenseType;
    ExpendableTypeID    expendabletypeID;
    AbilityBitMask      eabmCapabilities;
    char                iconName[c_cbFileName];
};

struct  DataMissileTypeIGC : public DataExpendableTypeIGC
{
    float               acceleration;
    float               turnRate;
    float               initialSpeed;
    //float               armTime;
    float               lockTime;
    float               readyTime;

    float               maxLock;
    float               chaffResistance;

    float               dispersion;
    float               lockAngle;

    float               power;

    float               blastPower;
    float               blastRadius;

    float               width;

    DamageTypeID        damageType;

    bool                bDirectional;

    SoundID             launchSound;
    SoundID             ambientSound;
};

struct  DataMineTypeIGC : public DataExpendableTypeIGC
{
    float               radius;
    float               power;
    float               endurance;
    DamageTypeID        damageType;
};

struct  DataChaffTypeIGC : public DataExpendableTypeIGC
{
    float               chaffStrength;
};

struct  DataProbeTypeIGC : public DataExpendableTypeIGC
{
    float               scannerRange;

    float               dtimeBurst;
    float               dispersion;
    float               accuracy;

    short               ammo;

    ProjectileTypeID    projectileTypeID;

    SoundID             ambientSound;
    float               dtRipcord;
};

struct  DataPartIGC
{
    IpartTypeIGC*       partType;
};

struct  DataProjectileIGC
{
    Vector              velocity;
    Vector              forward;
    float               lifespan;
    ProjectileTypeID    projectileTypeID;
};

struct  DataMissileIGC
{
    Vector              position;
    Vector              velocity;
    Vector              forward;
    ImissileTypeIGC*    pmissiletype;
    IshipIGC*           pLauncher;
    ImodelIGC*          pTarget;
    IclusterIGC*        pCluster;
    float               lock;
    MissileID           missileID;
    bool                bDud;
};

struct  DataSideIGC
{
    char                name[c_cbName];
    CivID               civilizationID;
    SideID              sideID;
    TechTreeBitMask     ttbmInitialTechs;
    TechTreeBitMask     ttbmDevelopmentTechs;
    GlobalAttributeSet  gasAttributes;
    Color               color;
    float               fTimeEndured;
    short               nFlags;
    short               nArtifacts;
    short               nKills;
    short               nEjections;
    short               nDeaths;
    short               nBaseKills;
    short               nBaseCaptures;
    SquadID             squadID;

    unsigned char       conquest;
    unsigned char       territory;

	char				allies; // #ALLY
};

struct  DataCivilizationIGC
{
    float               incomeMoney;
    float               bonusMoney;
    char                name[c_cbName];
    char                iconName[c_cbFileName];
    char                hudName[c_cbFileName];
    TechTreeBitMask     ttbmBaseTechs;
    TechTreeBitMask     ttbmNoDevTechs;
    GlobalAttributeSet  gasBaseAttributes;
    HullID              lifepod;
    CivID               civilizationID;
    StationTypeID       initialStationTypeID;
};

typedef char    TreasureCode;
const TreasureCode  c_tcPart        = 1;
const TreasureCode  c_tcPowerup     = 2;
const TreasureCode  c_tcDevelopment = 3;
const TreasureCode  c_tcCash        = 4;
const TreasureCode  c_tcFlag        = 5;

typedef ObjectID    PowerupCode;
const PowerupCode   c_pcHull   = 0x01;
const PowerupCode   c_pcShield = 0x02;
const PowerupCode   c_pcEnergy = 0x04;
const PowerupCode   c_pcFuel   = 0x08;
const PowerupCode   c_pcAmmo   = 0x10;

struct  TreasureData
{
    ObjectID        treasureID;
    TreasureCode    treasureCode;
    unsigned char   chance;
};

struct DataTreasureSetIGC
{
    char            name[c_cbName];
    TreasureSetID   treasureSetID;
    short           nTreasureData;
    bool            bZoneOnly;

    TreasureData*   treasureData0(void)
    {
        return (TreasureData*)(this + 1);
    }
};

#pragma pack(push, 1)
class   ShipLoadout
{
    public:
        HullID      hullID;

        ExpandedPartData*   PartData0(void) const
        {
            return (ExpandedPartData*)(this + 1);
        }

        ExpandedPartData*   PartDataN(short cb) const
        {
            return (ExpandedPartData*)(((char*)this) + cb);
        }
};

class CompactOrientation
{
    public:
        void    Set(const Orientation&   orientation)
        {
            Quaternion q(orientation);

            assert (q.GetA() >= 0.0f);

            m_qa = (unsigned short)(q.GetA() * 65535.0f + 0.5f);

            m_qx = short(q.GetX() * 32767.0f);
            m_qy = short(q.GetY() * 32767.0f);
            m_qz = short(q.GetZ() * 32767.0f);
        }

        void    Export(Orientation*      pOrientation) const
        {
            if ((m_qa != 0) || (m_qx != 0) || (m_qy != 0) || (m_qz != 0))
            {
                Quaternion  q(float(m_qa) / 65535.0f,
                              float(m_qx) / 32767.0f,
                              float(m_qy) / 32767.0f,
                              float(m_qz) / 32767.0f);

                *pOrientation = q;
            }
            else
                pOrientation->Reset();
        }

        void    Validate(void) const
        {
            ZRetailAssert ((m_qa != 0) || (m_qx != 0) || (m_qy != 0) || (m_qz != 0));
        }

    private:
        unsigned short  m_qa;
        short           m_qx, m_qy, m_qz;
};

class   CompactTime          //2 bytes
{
    private:
        short   m_timeOffset;

    public:
        void        Set(Time  timeBase, Time  timeNow)
        {
            long    delta = (long)(timeBase.clock() - timeNow.clock());
            m_timeOffset = (short)delta;

            assert (m_timeOffset == delta);
        }

        void        Export(Time   timeBase, Time* timeNow)  const
        {
            *timeNow = (DWORD)((long)(timeBase.clock()) - (long)m_timeOffset);
        }
};

class   CompactPosition      //6 bytes
{
    private:
        short           m_deltaXYZ[3];

        //Come up with a more (space) efficient means of storing a position near another position
        //Displacement(delta) = delta * |delta| / K*K
        //
        //Delta(displacement) = +- sqrt(|displacement|) * K
        //
        //If delta max is 32767 (short) and K is 120 then
        //      Displacement(32766) = 74556.3
        //      Displacement(32767) = 74560.9
        //
        //      Displacement(0) = 0
        //      Displacement(1) = 1/(120^2)
        //
        //      Delta(1000) = 3795
        //      Displacement(3795) = 1000.14        error of 0.14 units
        //      Displacement(3796) = 1000.67        maximum error of 0.40 units
        // so ... out to distances of out to 1000 from the reference point, accuracy will be within 0.5 units
        static  float   Displacement(short delta)
        {
            float   fDelta = float(delta);

            return fDelta * fDelta / ((fDelta > 0.0f) ? (120.0f * 120.0f) : -(120.0f * 120.0f));
        }

        static short    Delta(float displacement)
        {
            double  sad = sqrt(fabs(displacement));

            if (sad > double(0x7fff) / 120.0f)
            {
                return (displacement > 0.0f)
                       ? 0x7fff
                       : -(0x7fff);
            }
            else
                return (displacement > 0.0f)
                       ? short(sad *  120.0 + 0.5)
                       : short(sad * -120.0 - 0.5);
        }

    public:
        void        Set(const Vector&   pReference, const Vector& p)
        {
            m_deltaXYZ[0] = Delta(p.x - pReference.x);
            m_deltaXYZ[1] = Delta(p.y - pReference.y);
            m_deltaXYZ[2] = Delta(p.z - pReference.z);
        }

        void        Export(const Vector&    pReference, Vector* p)  const
        {
            p->x = pReference.x + Displacement(m_deltaXYZ[0]);
            p->y = pReference.y + Displacement(m_deltaXYZ[1]);
            p->z = pReference.z + Displacement(m_deltaXYZ[2]);
        }
};

class   CompactState          //2 bytes
{
    private:
        unsigned short  m_state;

    public:
            void        Set(int   stateM)
        {
            m_state = (unsigned short)stateM;
        }

        void        Export(int*  pStateM) const
        {
            *pStateM = (int)((unsigned int)m_state);
        }
};

class   CompactVelocity     //5 bytes
{
    private:
        unsigned short      m_speed;          //low order 13 bits. High order bits are sign for x, y, z
        BytePercentage      m_xVelocity, m_yVelocity, m_zVelocity;

    public:
        void            Set(const Vector&   v)
        {
            float   speed = v.Length();
            if (speed < 1.0f)
            {
                m_speed = 0;
            }
            else
            {
                assert (speed < 1000.0f);

                m_speed = ((unsigned short)(speed * 8.0f)) |
                                     ((unsigned short)(v.x >= 0.0f ? 0 : 0x2000)) |
                                     ((unsigned short)(v.y >= 0.0f ? 0 : 0x4000)) |
                                     ((unsigned short)(v.z >= 0.0f ? 0 : 0x8000));

                m_xVelocity = (float)fabs(v.x / speed);
                m_yVelocity = (float)fabs(v.y / speed);
                m_zVelocity = (float)fabs(v.z / speed);
            }
        }

        void            Export(Vector* pVelocity)  const
        {
            float   speed =  0.125f * (float)(m_speed & 0x1fff);

            pVelocity->x = m_xVelocity * ((m_speed & 0x2000) ? -speed : speed);
            pVelocity->y = m_yVelocity * ((m_speed & 0x4000) ? -speed : speed);
            pVelocity->z = m_zVelocity * ((m_speed & 0x8000) ? -speed : speed);
        }
};

class   CompactTurnRates           //3 bytes
{
    private:
        char                m_turnRates[3];

    public:
        void        Set(const float  turnRates[3])
        {
            for (Axis i = c_axisYaw; (i <= c_axisRoll); i++)
                m_turnRates[i] = (char)floor(turnRates[i] * 127.0f / (2.0f * pi) + 0.5f);
        }

        void            Export(float   turnRates[3]) const
        {
            for (Axis i = c_axisYaw; (i <= c_axisRoll); i++)
                turnRates[i] = ((float)(m_turnRates[i])) * (2.0f * pi) / 127.0f;
        }
};

class   CompactControlData      //4 bytes
{
    private:
        char        m_jsValues[c_axisMax]; //joystick settings

    public:
        void        Set(const ControlData&  cd)
        {
            for (Axis i = c_axisYaw; (i <= c_axisThrottle); i++)
                m_jsValues[i]  = (char)floor(cd.jsValues[i] * 127.0f + 0.5f);
        }

        void            Export(ControlData*    cd) const
        {
            for (Axis i = c_axisYaw; (i <= c_axisThrottle); i++)
                cd->jsValues[i] = float(m_jsValues[i]) / 127.0f;
        }
};

class   CompactShipFractions         //6 bytes  
{
    private:
        BytePercentage      m_bpHullFraction;       //1
        BytePercentage      m_bpShieldFraction;     //1
        BytePercentage      m_bpAmmo;               //1
        BytePercentage      m_bpFuel;               //1
        BytePercentage      m_bpEnergy;             //1
		BytePercentage      m_bpOre;                //1  //Xynth #156 7/2010

    public:
        void        SetHullFraction(float hf)
        {
            m_bpHullFraction = hf;
        }
        float        GetHullFraction(void) const
        {
            return m_bpHullFraction;
        }

        void        SetShieldFraction(float sf)
        {
            m_bpShieldFraction = sf;
        }
        float        GetShieldFraction(void) const
        {
            return (float)m_bpShieldFraction;
        }

        void        SetFuel(float maxFuel, float   fuel)
        {
            m_bpFuel = maxFuel == 0.0f ? 0.0f : (fuel / maxFuel);
        }

        float       GetFuel(float maxFuel) const
        {
            return m_bpFuel * maxFuel;
        }

        void        SetAmmo(short maxAmmo, short ammo)
        {
            m_bpAmmo = maxAmmo == 0 ? BytePercentage(0.0f) : BytePercentage(float(ammo) / float(maxAmmo));
        }

        short       GetAmmo(short maxAmmo) const
        {
            return short(m_bpAmmo * float(maxAmmo));
        }

        void        SetEnergy(float maxEnergy, float   energy)
        {
			// mmf note rixian inverter trips second assert in BytePercentage, only effects debug build of server
			m_bpEnergy = maxEnergy == 0.0f ? BytePercentage(0.0f) : BytePercentage(energy / maxEnergy);
        }

        float        GetEnergy(float maxEnergy) const
        {
            return m_bpEnergy * maxEnergy;
        }
		//Xynth #156 7/2010 new functions for new m_bpOre data
		float        GetOre(float maxOre) const
        {
            return m_bpOre * maxOre;
        }

		void        SetOre(float maxOre, float   ore)
        {
            m_bpOre = maxOre == 0.0f ? 0.0f : (ore / maxOre);
        }        
};

class   ServerLightShipUpdate                           //8 bytes
{
    public:
        ShipID              shipID;                     //2
        CompactState        stateM;                     //2
        CompactControlData  controls;                   //4
};

class  ServerHeavyShipUpdate : public ServerLightShipUpdate  //38 bytes
{
    public:
        CompactTime             time;                   //2
        CompactPosition         position;               //6

        CompactOrientation      orientation;            //8
        CompactVelocity         velocity;               //5
        CompactTurnRates        turnRates;              //3
        CompactShipFractions    fractions;              //5
        BytePercentage          power;                  //1
};

class  ServerSingleShipUpdate : public ServerLightShipUpdate  //46 bytes
{
    public:
        Time                    time;                   //4
        Vector                  position;               //12

        CompactOrientation      orientation;            //8
        CompactVelocity         velocity;               //5
        CompactTurnRates        turnRates;              //3
        CompactShipFractions    fractions;              //5
        BytePercentage          power;                  //1
};

class   ClientShipUpdate                                //33 bytes
{
    public:
        Time                time;                       //4
        Vector              position;                   //12
        CompactOrientation  orientation;                //8
        CompactState        stateM;                     //2
        CompactControlData  controls;                   //4
        CompactVelocity     velocity;                   //5
        CompactTurnRates    turnRates;                  //3
        BytePercentage      power;                      //1
};

class ClientActiveTurretUpdate                                      //12 bytes
{
    public:
        Time                time;                       //4
        CompactControlData  controls;                   //4
        CompactOrientation  orientation;                //8
};

class ServerActiveTurretUpdate                                      //16 bytes
{
    public:
        ShipID              shipID;                     //2
        CompactTime         time;                       //2
        CompactControlData  controls;                   //4
        CompactOrientation  orientation;                //8
};
#pragma pack(pop)

struct  DataShipIGC
{
    DWORD           dwPrivate;
    short           partsOffset;
    HullID          hullID;
    ShipID          shipID;
    char            name[c_cbName];
    Mount           nParts;
    SideID          sideID;
    //WingID          wingID;
    PilotType       pilotType;
    AbilityBitMask  abmOrders;
    ObjectID        baseObjectID;
    short           nDeaths;
    short           nEjections;
    short           nKills;
};

struct  DataWarpIGC
{
    WarpDef             warpDef;
    char                name[c_cbName];
    Vector              position;
    Vector              forward;
    Rotation            rotation;
    float               signature;
    SectorID            clusterID;
};

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

struct DataMunitionExport
{
    unsigned char       lifespan;
    char                dx;
    char                dy;
    char                dz;
};

struct  DataMunitionIGC
{
    ImineIGC*           pmine;
    IclusterIGC*        pcluster;
    IsideIGC*           pside;
    Vector              position;
    Time                timeExpire;
    MunitionID          munitionID;
};

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

struct DataBuildingEffectBase
{
    Time                timeStart;
    float               radiusAsteroid;
    float               radiusStation;
    Vector              positionStart;
    Vector              positionStop;
};

struct DataBuildingEffectIGC : DataBuildingEffectBase
{
    IclusterIGC*        pcluster;
    IshipIGC*           pshipBuilder;
    IasteroidIGC*       pasteroid;
    IstationIGC*        pstation;
};

struct DataBuildingEffectExport : DataBuildingEffectBase
{
    SectorID            clusterID;
    AsteroidID          asteroidID;
    Color               color;
};

struct  DataProbeBase
{
    Vector              p0;
    Time                time0;
    ProbeID             probeID;
    bool                exportF;
};

struct  DataProbeIGC : public DataProbeBase
{
    IprobeTypeIGC*      pprobetype;
    IsideIGC*           pside;
    IclusterIGC*        pcluster;
    IshipIGC*           pship;
    ImodelIGC*          pmodelTarget;
};
struct DataProbeExport : public DataProbeBase
{
    ExpendableTypeID    probetypeID;
    SideID              sideID;
    SectorID            clusterID;
    ShipID              shipID;
    ObjectType          otTarget;
    ObjectID            oidTarget;
    bool                createNow;
};

struct  DataChaffIGC
{
    Vector              p0;
    Vector              v0;
    Time                time0;
    IclusterIGC*        pcluster;
    IchaffTypeIGC*      pchafftype;
};

struct  DataStationIGC
{
    Vector              position;
    Vector              up;
    Vector              forward;
    Rotation            rotation;
    SectorID            clusterID;
    SideID              sideID;
    StationID           stationID;
    StationTypeID       stationTypeID;
    BytePercentage      bpHull;
    BytePercentage      bpShield;
    char                name[c_cbName];
};

struct  DataStationTypeIGC : public DataBuyableIGC
{
    float                   signature;
    float                   maxArmorHitPoints;
    float                   maxShieldHitPoints;
    float                   armorRegeneration;
    float                   shieldRegeneration;
    float                   scannerRange;
    Money                   income;
    float                   radius;
    TechTreeBitMask         ttbmLocal;
    StationTypeID           stationTypeID;
    StationTypeID           successorStationTypeID;
    DefenseTypeID           defenseTypeArmor;
    DefenseTypeID           defenseTypeShield;
    StationAbilityBitMask   sabmCapabilities;
    AsteroidAbilityBitMask  aabmBuild;
    StationClassID          classID;

    DroneTypeID             constructionDroneTypeID;

    SoundID                 constructorNeedRockSound;
    SoundID                 constructorUnderAttackSound;
    SoundID                 constructorDestroyedSound;
    SoundID                 completionSound;

    SoundID                 interiorSound;
    SoundID                 exteriorSound;
    SoundID                 interiorAlertSound;

    SoundID                 underAttackSound;
    SoundID                 criticalSound;
    SoundID                 destroyedSound;
    SoundID                 capturedSound;

    SoundID                 enemyCapturedSound;
    SoundID                 enemyDestroyedSound;

    char                    textureName[c_cbFileName];
    char                    builderName[c_cbName];
};

struct DataDevelopmentIGC : public DataBuyableIGC
{
    GlobalAttributeSet  gas;
    DevelopmentID       developmentID;
    SoundID             completionSound;
};

struct DataDroneTypeIGC : public DataBuyableIGC
{
    float               shootSkill;
    float               moveSkill;
    float               bravery;
    PilotType           pilotType;
    HullID              hullTypeID;
    DroneTypeID         droneTypeID;
    ExpendableTypeID    etidLaid;
};

struct DataBucketIGC
{
    IbuyableIGC*        buyable;
    IsideIGC*           side;
};

struct  ExplosionData
{
    float           amount;
    float           radius;
    int             explosionType;
    Time            time;
    Vector          position;
    DamageTypeID    damageType;
    ImodelIGC*      launcher;
};

//
// PLEASE! Update HandleCOMChatEvents in FedSrv.cpp if you change this enum
//
enum ChatTarget // if you change this please update AGCChatTarget in AGCIDL.idl
{
    CHAT_EVERYONE = 0, CHAT_LEADERS, CHAT_ADMIN, CHAT_SHIP,
    CHAT_ALLIES, //imago added allies 7/3/09 ALLY
    CHAT_TEAM, CHAT_INDIVIDUAL, CHAT_INDIVIDUAL_NOFILTER, CHAT_WING, CHAT_INDIVIDUAL_ECHO,     //require objectID to be set
    CHAT_ALL_SECTOR, CHAT_FRIENDLY_SECTOR,                      //ditto
    CHAT_GROUP, CHAT_GROUP_NOECHO,                               //client only ... get translated into multiple sends
    CHAT_NOSELECTION,
    CHAT_MAX,
};

//Interfaces ....
class IstaticIGC
{
    public:
        virtual ~IstaticIGC(void) {};
        virtual void                            Initialize(void) = 0;
        virtual void                            Terminate(void) = 0;

        virtual float                           GetFloatConstant(FloatConstantID fcid) const = 0;
        virtual void                            SetFloatConstant(FloatConstantID fcid, float f) = 0;

        virtual float                           GetDamageConstant(DamageTypeID dmgid, DefenseTypeID defid) const = 0;
        virtual void                            SetDamageConstant(DamageTypeID dmgid, DefenseTypeID defid, float f) = 0;

        virtual const void*                     GetConstants(void) const = 0;
        virtual void                            SetConstants(const void* pf) = 0;
        virtual int                             GetSizeOfConstants(void) const = 0;

        virtual void                            AddDroneType(IdroneTypeIGC*                 dt) = 0;
        virtual void                            DeleteDroneType(IdroneTypeIGC*              dt) = 0;
        virtual IdroneTypeIGC*                  GetDroneType(DroneTypeID                    id) const = 0;
        virtual const DroneTypeListIGC*         GetDroneTypes(void) const = 0;

        virtual void                            AddDevelopment(IdevelopmentIGC*             d) = 0;
        virtual void                            DeleteDevelopment(IdevelopmentIGC*          d) = 0;
        virtual IdevelopmentIGC*                GetDevelopment(DevelopmentID                id) const = 0;
        virtual const DevelopmentListIGC*       GetDevelopments(void) const = 0;

        virtual void                            AddStationType(IstationTypeIGC*             st) = 0;
        virtual void                            DeleteStationType(IstationTypeIGC*          st) = 0;
        virtual IstationTypeIGC*                GetStationType(StationTypeID                id) const = 0;
        virtual const StationTypeListIGC*       GetStationTypes(void) const = 0;

        virtual void                            AddCivilization(IcivilizationIGC*           c) = 0;
        virtual void                            DeleteCivilization(IcivilizationIGC*        c) = 0;
        virtual IcivilizationIGC*               GetCivilization(CivID                       id) const = 0;
        virtual const CivilizationListIGC*      GetCivilizations(void) const = 0;

        virtual void                            AddHullType(IhullTypeIGC*                   ht) = 0;
        virtual void                            DeleteHullType(IhullTypeIGC*                ht) = 0;
        virtual IhullTypeIGC*                   GetHullType(HullID                          id) const = 0;
        virtual const HullTypeListIGC*          GetHullTypes(void) const = 0;

        virtual void                            AddExpendableType(IexpendableTypeIGC*       mt) = 0;
        virtual void                            DeleteExpendableType(IexpendableTypeIGC*    mt) = 0;
        virtual IexpendableTypeIGC*             GetExpendableType(ExpendableTypeID          id) const = 0;
        virtual const ExpendableTypeListIGC*    GetExpendableTypes(void) const = 0;

        virtual void                            AddPartType(IpartTypeIGC*                   pt) = 0;
        virtual void                            DeletePartType(IpartTypeIGC*                pt) = 0;
        virtual IpartTypeIGC*                   GetPartType(PartID                          id) const = 0;
        virtual const PartTypeListIGC*          GetPartTypes(void) const = 0;

        virtual void                            AddProjectileType(IprojectileTypeIGC*       pt) = 0;
        virtual void                            DeleteProjectileType(IprojectileTypeIGC*    pt) = 0;
        virtual IprojectileTypeIGC*             GetProjectileType(ProjectileTypeID          id) const = 0;
        virtual const ProjectileTypeListIGC*    GetProjectileTypes(void) const = 0;

        virtual void                            AddTreasureSet(ItreasureSetIGC*                   pt) = 0;
        virtual void                            DeleteTreasureSet(ItreasureSetIGC*                pt) = 0;
        virtual ItreasureSetIGC*                GetTreasureSet(TreasureSetID                      id) const = 0;
        virtual const TreasureSetListIGC*       GetTreasureSets(void) const = 0;

        virtual IpartTypeIGC*                   GetAmmoPack(void) const = 0;
        virtual IpartTypeIGC*                   GetFuelPack(void) const = 0;
};

const int c_cbSideName = 40;

class ImissionIGC : public IstaticIGC
{
    public:
        virtual int                     Export(int64_t   maskTypes,
                                               char*     pdata) const = 0;
        virtual void                    Import(Time      now,
                                               int64_t   maskTypes,
                                               char*     pdata,
                                               int       datasize) = 0;
		//Imago added
		virtual ZString					BitsToTechsList(TechTreeBitMask & ttbm) = 0;
		virtual void					TechsListToBits(const char * szTechs, TechTreeBitMask & ttbm) = 0;

		virtual ZString					BitsToPartsList(PartMask & pm, EquipmentType et) = 0;
		virtual PartMask				PartMaskFromToken(const char * szToken, EquipmentType et) = 0;
		virtual PartMask				PartsListToMask(const char * szParts, EquipmentType et) = 0;

		virtual bool					LoadTechBitsList(void) = 0;
		virtual bool					LoadPartsBitsList(void) = 0;

		virtual void					ExportStaticIGCObjs(void) = 0;
		virtual void					ImportStaticIGCObjs(void) = 0;
		// Imago ^

        virtual MissionID               GetMissionID(void) const = 0;
        virtual void                    SetMissionID(MissionID  mid) = 0;

        virtual void                    Initialize(Time now, IIgcSite* pIgcSite) = 0;
        virtual void                    Terminate(void) = 0;
        virtual void                    Update(Time now) = 0;

        virtual IIgcSite*               GetIgcSite(void) const = 0;

        virtual IbaseIGC*               CreateObject(Time now, ObjectType objectID,
                                                     const void* data, int dataSize) = 0;

        virtual ImodelIGC*              GetModel(ObjectType type, ObjectID id) const = 0;
        virtual IbaseIGC*               GetBase(ObjectType type, ObjectID id) const = 0;

        virtual DamageTrack*            CreateDamageTrack(void) = 0;

        virtual void                    AddSide(IsideIGC* s) = 0;
        virtual void                    DeleteSide(IsideIGC* s) = 0;
        virtual IsideIGC*               GetSide(SideID sideID) const = 0;
        virtual const SideListIGC*      GetSides(void) const = 0;
		virtual void                    GetSeenSides(ImodelIGC * pmodel, bool (&bseensides)[c_cSidesMax], ImodelIGC * poptionalmodel = NULL) = 0; //Imago #120 #121 8/10

        virtual void                    AddCluster(IclusterIGC* c) = 0;
        virtual void                    DeleteCluster(IclusterIGC* c) = 0;
        virtual IclusterIGC*            GetCluster(SectorID clusterID) const = 0;
        virtual const ClusterListIGC*   GetClusters(void) const = 0;

        virtual void                    AddShip(IshipIGC* s) = 0;
        virtual void                    DeleteShip(IshipIGC* s) = 0;
        virtual IshipIGC*               GetShip(ShipID shipID) const = 0;
        virtual const ShipListIGC*      GetShips(void) const = 0;

        virtual void                    AddStation(IstationIGC* s) = 0;
        virtual void                    DeleteStation(IstationIGC* s) = 0;
        virtual IstationIGC*            GetStation(StationID stationID) const = 0;
        virtual const StationListIGC*   GetStations(void) const = 0;

        virtual void                    AddAsteroid(IasteroidIGC* p) = 0;
        virtual void                    DeleteAsteroid(IasteroidIGC* p) = 0;
        virtual IasteroidIGC*           GetAsteroid(AsteroidID asteroidID) const = 0;
        virtual const AsteroidListIGC*  GetAsteroids(void) const = 0;

        virtual void                    AddWarp(IwarpIGC* w) = 0;
        virtual void                    DeleteWarp(IwarpIGC* w) = 0;
        virtual IwarpIGC*               GetWarp(WarpID warpID) const = 0;
        virtual const WarpListIGC*      GetWarps(void) const = 0;

        virtual void                    AddBuoy(IbuoyIGC* t) = 0;
        virtual void                    DeleteBuoy(IbuoyIGC* t) = 0;
        virtual IbuoyIGC*               GetBuoy(BuoyID buoyID) const = 0;
        virtual const BuoyListIGC*      GetBuoys(void) const = 0;

        virtual void                    AddTreasure(ItreasureIGC* t) = 0;
        virtual void                    DeleteTreasure(ItreasureIGC* t) = 0;
        virtual ItreasureIGC*           GetTreasure(TreasureID treasureID) const = 0;
        virtual const TreasureListIGC*  GetTreasures(void) const = 0;

        virtual void                    AddMine(ImineIGC* ms) = 0;
        virtual void                    DeleteMine(ImineIGC* m) = 0;
        virtual ImineIGC*               GetMine(MineID mineID) const = 0;
        virtual const MineListIGC*      GetMines(void) const = 0;

        virtual void                    AddProbe(IprobeIGC* ms) = 0;
        virtual void                    DeleteProbe(IprobeIGC* m) = 0;
        virtual IprobeIGC*              GetProbe(ProbeID probeID) const = 0;
        virtual const ProbeListIGC*     GetProbes(void) const = 0;

        virtual IpartIGC*               CreatePart(Time             now,
                                                   IpartTypeIGC*    ppt) = 0;

        virtual BuoyID                  GenerateNewBuoyID(void) = 0;
        virtual ShipID                  GenerateNewShipID(void) = 0;
        virtual AsteroidID              GenerateNewAsteroidID(void) = 0;
        virtual StationID               GenerateNewStationID(void) = 0;
        virtual TreasureID              GenerateNewTreasureID(void) = 0;
        virtual MineID                  GenerateNewMineID(void) = 0;
        virtual ProbeID                 GenerateNewProbeID(void) = 0;
        virtual MissileID               GenerateNewMissileID(void) = 0;

        virtual void                    SetMissionStage(STAGE  st) = 0;
        virtual STAGE                   GetMissionStage() = 0;
        virtual void                    EnterGame(void) = 0;

        virtual void                    UpdateSides(Time now,
                                                    const MissionParams * pmp,
                                                    const char  sideNames[c_cSidesMax][c_cbSideName]) = 0;
        virtual void                    ResetMission() = 0;
        virtual void                    GenerateMission(Time                     now,
                                                        const MissionParams *    pmp,
                                                        TechTreeBitMask*         pttbmShouldOverride,
                                                        TechTreeBitMask *        pttbmOverrideValue) = 0;
        virtual void                    GenerateTreasure(Time           now,
                                                         IclusterIGC*   pcluster,
                                                         short          tsi) = 0;
        virtual void                    SetPrivateData(DWORD dwPrivate) = 0;
        virtual DWORD                   GetPrivateData(void) const = 0;

        virtual Time                    GetLastUpdate(void) const = 0;

        virtual const MissionParams*    GetMissionParams(void) const = 0;
        virtual void                    SetMissionParams(const MissionParams* pmp) = 0;
        virtual void                    SetStartTime(Time timeStart) = 0;

        //Warning these should never be called except from LoadIGCStaticCore
        virtual CstaticIGC*             GetStaticCore(void) const = 0;
        virtual void                    SetStaticCore(CstaticIGC*   pStatic) = 0;

        virtual short                   GetReplayCount(void) const = 0;
        virtual const char*             GetContextName(void) = 0;
		//#ALLY
		virtual void                    UpdateAllies(const char  Allies[c_cSidesMax]) = 0;
};

class IbaseIGC : public IObject
{
    public:
        virtual HRESULT         Initialize(ImissionIGC* pMission, Time now, const void* data, int length) = 0;
        virtual void            Terminate(void) = 0;
        virtual void            Update(Time   now)          {}

        //Exporting an object which doesn't support export is also bad.
        virtual int             Export(void*   data) const  { assert (false); return -1;}

        // GetUniqueID() is provided for convenience because AGC uses GetObjectType and GetObjectID often.
        virtual int             GetUniqueID(void) const     { return GetObjectType() | (GetObjectID() << 16); }
        virtual ObjectType      GetObjectType(void) const = 0;

        //Calling either of these methods on something that doesn't have either an object ID or a mission is bad.
        virtual ObjectID        GetObjectID(void) const     { assert (false); return NA;}
        virtual ImissionIGC*    GetMission(void) const      { assert (false); return NULL; }
};

class ItypeIGC : public IbaseIGC
{
    public:
        virtual const void*     GetData(void) const = 0;
};

class AttachSite : public IObject
{
    public:
        virtual ~AttachSite(void)     {}

        virtual void Terminate(void)  {}

        virtual void AddExplosion(ImodelIGC* pmodel, int type) {}
        virtual void AddExplosion(const Vector& vecPosition, float scale, int type) {}
        virtual void AddPulse (float fExplodeTime, const Vector& vecPosition, float fRadius, const Color& color) {}
        virtual void AddThingSite(ThingSite* thing) {}
        virtual void DeleteThingSite(ThingSite* thing) {}
};

class ThingSite : public AttachSite
{
    public:
        virtual void        Purge(void) {}
        virtual Vector      GetChildModelOffset(const ZString& strFrame) { return Vector(0, 0, 0); }
        virtual Vector      GetChildOffset(const ZString& strFrame) { return Vector(0, 0, 0); }
        virtual void        AddHullHit(const Vector& vecPosition, const Vector& vecNormal) {}
        virtual void        AddFlare(Time ptime, const Vector& vecPosition, int id, const Vector* ellipseEquation) {}
        virtual void        AddMuzzleFlare(const Vector& vecEmissionPoint, float duration) {}

        virtual void        SetVisible(unsigned char render)  {}

        virtual void        SetAfterburnerThrust (const Vector& direction, float power) {}
        virtual void        SetAfterburnerSmokeSize (float size) {}
        virtual void        SetAfterburnerFireDuration (float duration) {}
        virtual void        SetAfterburnerSmokeDuration (float duration) {}

        virtual void        AddDamage (const Vector& vecDamagePosition, float fDamageFraction) {}
        virtual void        RemoveDamage (float fDamageFraction) {}

        virtual void        SetTimeUntilRipcord (float fTimeUntilTeleport) {}
        virtual void        SetTimeUntilAleph (float fTimeUntilTeleport) {}

        virtual const Point& GetScreenPosition(void) const { const static Point pointZero(0.0f,0.0f);
                                                             return pointZero; }
        virtual float           GetDistanceToEdge(void) { return 0.0f; }
        virtual float           GetScreenRadius(void) {return 1.0f; }
        virtual unsigned char   GetRadarState(void) { return c_ucRadarOnScreenLarge; }

        virtual ThingGeo*   GetThingGeo(void) { return NULL; }
        virtual Geo*        GetGeo(void) { return NULL; }

        virtual void        SetPosition(const Vector& position)  {}
        virtual float       GetRadius(void) { return 1.0f; }
        virtual void        SetRadius(float r) { }

        virtual void        SetOrientation(const Orientation& orientation)  {}
        virtual void        Spin(float r)  {}

        virtual void        SetTexture(const char* pszTextureName) {}

        virtual void        SetTrailColor(const Color& c) {}

        virtual HRESULT     LoadDecal(const char* textureName, bool bDirectional, float width) { return S_OK; }
        virtual HRESULT     LoadModel(int options, const char* modelName, const char* textureName) { return S_OK; }
        virtual HRESULT     LoadAleph(const char* textureName) { return S_OK; }
        virtual HRESULT     LoadEffect(const Color& color) { return S_OK; }
        virtual void        SetColors(float aInner, float fInner, float fOuter) {}

        virtual HRESULT     LoadMine(const char* textureName, float strength, float radius) { return S_OK; }
        virtual void        SetMineStrength(float strength) {}


        virtual void        SetCluster(ImodelIGC*   pmodel,
                                       IclusterIGC* pcluster) {};
        virtual void        UpdateSideVisibility(ImodelIGC*         pmodel,
                                                 IclusterIGC*       pcluster) {}
        virtual bool        GetSideVisibility(IsideIGC*             side) { return false; }
        //Xynth #100 7/2010
		virtual bool        GetCurrentEye(IsideIGC*             side) { return false; }
		virtual void        SetSideVisibility(IsideIGC*             side,
                                              bool                  fVisible) {}

        virtual void             ActivateBolt(void) {}
        virtual void             DeactivateBolt(void) {}

        virtual int         GetMask(void) const {return 0;}
        virtual void        SetMask(int mask) {}
        virtual void        OrMask(int mask) {}
        virtual void        AndMask(int mask) {}
        virtual void        XorMask(int mask) {}
};

class ImodelIGC : public IbaseIGC
{
    public:
        virtual Time                 GetLastUpdate(void) const = 0;
        virtual void                 SetLastUpdate(Time now) = 0;

        virtual void                 SetMission(ImissionIGC*    pmission) = 0;
        virtual void                 LoadCVH(const char*        pszFileName,
                                             const char*        iconName,
                                             ModelAttributes    attributes,
                                             HitTestShape       htsDefault = c_htsSphere) = 0;
        virtual HRESULT              LoadEffect(const Color&    color,
                                                ModelAttributes attributes) = 0;
        virtual HRESULT              LoadWarp(const char*       textureName,
                                              const char*       iconName,
                                              ModelAttributes   attributes) = 0;
        virtual HRESULT              LoadMine(const char*       textureName,
                                              float             strength,
                                              float             radius,
                                              const char*       iconName,
                                              ModelAttributes   attributes) = 0;
        virtual HRESULT              LoadDecal(const char*      textureName,
                                               const char*      iconName,
                                               const Color&     color,
                                               bool             bDirectional,
                                               float            width,
                                               ModelAttributes  attributes,
                                               HitTestShape     htsDefault = c_htsPoint) = 0;

        virtual HRESULT              Load(int                  options,
                                          const char*          model,
                                          const char*          texture,
                                          const char*          iconName,
                                          ModelAttributes      attributes) = 0;

        virtual bool                 GetVisibleF(void) const = 0;
        virtual void                 SetVisibleF(bool visibleF) = 0;
        virtual void                 SetRender(unsigned char render) = 0;

        virtual bool                 SeenBySide(IsideIGC* side) const = 0;
		virtual bool                 GetCurrentEye(IsideIGC* side) const = 0; //Xynth #225
        virtual void                 UpdateSeenBySide(void) = 0;
        virtual void                 SetSideVisibility(IsideIGC* side, bool fVisible) = 0;

        virtual ThingSite*           GetThingSite(void) const = 0;
        virtual void                 FreeThingSite(void) = 0;

        virtual const Vector&        GetPosition(void) const = 0;
        virtual void                 SetPosition(const Vector& newVal) = 0;

        virtual const Vector&        GetVelocity(void) const = 0;
        virtual void                 SetVelocity(const Vector& newVal) = 0;

        virtual const Orientation&   GetOrientation(void) const = 0;
        virtual void                 SetOrientation(const Orientation& newVal) = 0;

        virtual const Rotation&      GetRotation(void) const = 0;
        virtual void                 SetRotation(const Rotation& newVal) = 0;

        virtual float                GetRadius(void) const = 0;
        virtual void                 SetRadius(float newVal) = 0;

        virtual ModelAttributes      GetAttributes(void) const = 0;

        virtual IsideIGC*            GetSide(void) const = 0;
        virtual void                 SetSide(IsideIGC* newVal) = 0;

        virtual float                GetMass(void) const = 0;
        virtual void                 SetMass(float newVal) = 0;

        virtual IclusterIGC*         GetCluster(void) const = 0;
        virtual void                 SetCluster(IclusterIGC* pVal) = 0;

        virtual char const*          GetName(void) const = 0;
        virtual void                 SetName(const char* newVal) = 0;

        virtual void                 SetSecondaryName(const char* newVal) = 0;

        virtual float                GetSignature(void) const = 0;
        virtual void                 SetSignature(float newVal) = 0;
        virtual void                 ChangeSignature(float delta) = 0;

        virtual void                 SetTexture(const char* pszTextureName) = 0;

        virtual void                 Move(float t) = 0;
        virtual void                 Move(void) = 0;
        virtual void                 SetBB(Time tStart, Time tStop, float dt) = 0;
        virtual HitTest*             GetHitTest(void) const = 0;

        virtual void                 HandleCollision(Time                   timeCollision,
                                                     float                  tCollision,
                                                     const CollisionEntry&  entry,
                                                     ImodelIGC*             pModel) = 0;

        virtual IObject*             GetIcon(void) const = 0;
        virtual void                 SetIcon(IObject* picon) = 0;

        virtual SideID               GetFlag(void) const        { return NA; };
        virtual void                 SetFlag(SideID sideID)     { assert (false); }

        virtual const DamageBucketList*  GetDamageBuckets(void) const = 0;
        virtual void                     AddDamageBucket(DamageBucket*   db) = 0;
        virtual void                     DeleteDamageBucket(DamageBucket*   db) = 0;
};

class IdamageIGC : public ImodelIGC
{
    public:
        virtual DamageResult    ReceiveDamage(DamageTypeID            type,
                                              float                   amount,
                                              Time                    timeCollision,
                                              const Vector&           position1,
                                              const Vector&           position2,
                                              ImodelIGC*              launcher) = 0;
        virtual float           GetFraction(void) const = 0;
        virtual void            SetFraction(float newVal) = 0;

        virtual float           GetHitPoints(void) const = 0;
};

class IscannerIGC : public IdamageIGC
{
    public:
        virtual bool                 InScannerRange(ImodelIGC*  pModel) const = 0;
        virtual bool                 CanSee(ImodelIGC*  pModel) const = 0;
};

typedef int ShipUpdateStatus;
const ShipUpdateStatus   c_susOutOfDate =  0;
const ShipUpdateStatus   c_susInvalid   = -2;
const ShipUpdateStatus   c_susRejected  = -1;
const ShipUpdateStatus   c_susAccepted  =  1;

class IshipIGC : public IscannerIGC
{
    public:
        virtual void                 SetObjectID(ObjectID    id) {}

        virtual Money                GetValue(void) const = 0;

        virtual IstationIGC*         GetStation(void) const = 0;
        virtual void                 SetStation(IstationIGC* s) = 0;

        virtual void                 Reset(bool bFull) = 0;

        virtual float                GetTorqueMultiplier(void) const = 0;

        virtual float                GetCurrentTurnRate(Axis axis) const = 0;
        virtual void                 SetCurrentTurnRate(Axis axis, float newVal) = 0;

        virtual const IhullTypeIGC*  GetHullType(void) const = 0;

        virtual IhullTypeIGC*        GetBaseHullType(void) const = 0;
        virtual void                 SetBaseHullType(IhullTypeIGC* newVal) = 0;

        virtual void                 SetMission(ImissionIGC* pMission) = 0;

        //Note ... the next two methods should never be called from outside IGC
        virtual void                 AddPart(IpartIGC*        part) = 0;
        virtual void                 DeletePart(IpartIGC*    part) = 0;
        virtual const PartListIGC*   GetParts(void) const = 0;

        virtual IpartIGC*            GetMountedPart(EquipmentType type, Mount mountID) const = 0;

        //Note ... the next method should never be called from outside IGC
        virtual void                 MountPart(IpartIGC* p, Mount mountNew, Mount* pmountOld) = 0;

        virtual short                GetAmmo(void) const = 0;
        virtual void                 SetAmmo(short amount) = 0;

        virtual float                GetFuel(void) const = 0;
        virtual void                 SetFuel(float newVal) = 0;

        virtual float                GetEnergy(void) const = 0;
        virtual void                 SetEnergy(float newVal) = 0;

        virtual float                GetCloaking(void) const = 0;
        virtual void                 SetCloaking(float newVal) = 0;

        virtual bool                 GetVectorLock(void) const = 0;
        virtual void                 SetVectorLock(bool bVectorLock) = 0;

        virtual const ControlData&   GetControls(void) const = 0;
        virtual void                 SetControls(const ControlData&  newVal) = 0;

        virtual bool                 fRipcordActive(void) const = 0;
        virtual float                GetRipcordTimeLeft(void) const = 0;
        virtual void                 ResetRipcordTimeLeft(void) = 0;

        virtual const Vector&        GetEngineVector(void) const = 0;

        virtual int                  GetStateM(void) const = 0;
        virtual void                 SetStateM(int newVal) = 0;
        virtual void                 SetStateBits(int mask, int newBits) = 0;

        virtual ImodelIGC*           GetCommandTarget(Command i) const = 0;
        virtual CommandID            GetCommandID(Command i) const = 0;
        virtual void                 SetCommand(Command i, ImodelIGC* target, CommandID cid) = 0;

        virtual void                 PreplotShipMove(Time          timeStop) = 0;
        virtual void                 PlotShipMove(Time          timeStop) = 0;
        virtual void                 ExecuteShipMove(Time          timeStop) = 0;
        virtual void                 ExecuteShipMove(Time          timeStart,
                                                     Time          timeStop,
                                                     Vector*       pVelocity,
                                                     Orientation*  pOrientation) = 0;
        virtual void                 ExecuteTurretMove(Time          timeStart,
                                                       Time          timeStop,
                                                       Orientation*  pOrientation) = 0;

        virtual bool                EquivalentShip(IshipIGC* pship) const = 0;

        virtual short               ExportShipLoadout(ShipLoadout*         ploadout) = 0;
        virtual void                ProcessShipLoadout(short               cbLoadout,
                                                       const ShipLoadout*  ploadout,
                                                       bool                bReady) = 0;
        virtual bool                PurchaseShipLoadout(short              cbLoadout,
                                                        const ShipLoadout* ploadout) = 0;   //Returns true is entire purchase is OK

        virtual void                ExportFractions(CompactShipFractions* pfractions) const = 0;

        virtual void                ExportShipUpdate(ServerLightShipUpdate*     pshipupdate) const = 0;
        virtual void                ExportShipUpdate(Time                       timeReference,
                                                     const Vector&              positionReference,
                                                     ServerHeavyShipUpdate*     pshipupdate) const = 0;


        virtual void                ExportShipUpdate(ServerSingleShipUpdate*    pshipupdate) const = 0;
        virtual void                ExportShipUpdate(ClientShipUpdate*          pshipupdate) const = 0;

        virtual void                ExportShipUpdate(ClientActiveTurretUpdate*  patu) const = 0;

        virtual void                ProcessFractions(const CompactShipFractions& fractions) = 0;

        virtual ShipUpdateStatus    ProcessShipUpdate(const ServerLightShipUpdate&      shipupdate) = 0;
        virtual ShipUpdateStatus    ProcessShipUpdate(Time                              timeReference,
                                                      Vector                            positionReference,
                                                      const ServerHeavyShipUpdate&      shipupdate) = 0;

        virtual ShipUpdateStatus    ProcessShipUpdate(const ClientShipUpdate&           shipupdate) = 0;
        virtual ShipUpdateStatus    ProcessShipUpdate(const ServerSingleShipUpdate&     shipupdate,
                                                      bool                              bOrient = true) = 0;

        virtual ShipUpdateStatus    ProcessShipUpdate(Time                              timeReference,
                                                      const ServerActiveTurretUpdate&   shipupdate) = 0;
        virtual ShipUpdateStatus    ProcessShipUpdate(const ClientActiveTurretUpdate&   shipupdate) = 0;

        virtual void                SetPrivateData(DWORD dwPrivate) = 0;
        virtual DWORD               GetPrivateData(void) const = 0;

        virtual void                ReInitialize(DataShipIGC * dataShip, Time now) = 0;

        virtual Mount               HitTreasure(TreasureCode treasureCode, ObjectID objectID, short amount) = 0;

        virtual ImissileIGC*        GetLastMissileFired(void) const = 0;
        virtual void                SetLastMissileFired(ImissileIGC* pmissile) = 0;

		//Imago #7 7/10
		virtual Time                GetLastTimeLaunched(void) const = 0;
		virtual void                SetLastTimeLaunched(Time timeLastLaunch) = 0;
        virtual void                SetLastTimeDocked(Time timeLastDock) = 0;
        virtual Time                GetLastTimeDocked(void) const = 0;

        virtual void                Promote(void) = 0;

        virtual void                SetParentShip(IshipIGC* pship) = 0;
        virtual IshipIGC*           GetParentShip(void) const = 0;

        virtual Mount               GetTurretID(void) const = 0;
        virtual void                SetTurretID(Mount   turretID) = 0;

        virtual IshipIGC*           GetGunner(Mount turretID) const = 0;

        virtual const ShipListIGC*  GetChildShips(void) const = 0;

        virtual void    Complain(SoundID    sid, const char* pszMsg) = 0;

        virtual short               GetKills(void) const = 0;
        virtual void                SetKills(short n) = 0;
        virtual void                AddKill(void) = 0;

        virtual short               GetDeaths(void) const = 0;
        virtual void                SetDeaths(short n) = 0;
        virtual void                AddDeath(void) = 0;

        virtual short               GetEjections(void) const = 0;
        virtual void                SetEjections(short n) = 0;
        virtual void                AddEjection(void) = 0;

        virtual float               GetExperienceMultiplier(void) const = 0;
        virtual float               GetExperience(void) const = 0;
        virtual void                SetExperience(float) = 0;
        virtual void                AddExperience(void) = 0;

        //These two methods are never called from outside of CshipIGC
        virtual void                AddChildShip(IshipIGC* pship) = 0;
        virtual void                DeleteChildShip(IshipIGC* pship) = 0;

        virtual IshipIGC*           GetSourceShip(void) = 0;

        virtual IpartIGC*           CreateAndAddPart(const PartData* ppd) = 0;
        virtual IpartIGC*           CreateAndAddPart(IpartTypeIGC*  ppt, Mount mount, short amount) = 0;

        virtual WingID              GetWingID(void) const = 0;
        virtual void                SetWingID(WingID    wid) = 0;

        virtual PilotType           GetPilotType(void) const = 0;
        virtual AbilityBitMask      GetOrdersABM(void) const = 0;

        virtual bool                GetAutopilot(void) const = 0;
        virtual void                SetAutopilot(bool bAutopilot) = 0;

        virtual bool                LegalCommand(CommandID   cid) const = 0;
        virtual bool                LegalCommand(CommandID   cid,
                                                 ImodelIGC*  pmodel) const = 0;

        virtual IshipIGC*           GetAutoDonate(void) const = 0;
        virtual void                SetAutoDonate(IshipIGC* pship) = 0;

        virtual ImodelIGC*          GetRipcordModel(void) const = 0;
        virtual void                SetRipcordModel(ImodelIGC*  pmodel) = 0;
        virtual ImodelIGC*          FindRipcordModel(IclusterIGC*   pcluster) = 0;

        virtual float               GetRipcordDebt(void) const = 0;
        virtual void                AdjustRipcordDebt(float delta) = 0;
		virtual void				SetStayDocked(bool stayDock) = 0; //Xynth #48 8/2010
		virtual bool				GetStayDocked(void) const =0; //Xynth #48
		virtual void				AddRepair(float repair) = 0;
		virtual float				GetRepair(void) const = 0;
		virtual void				SetAchievementMask(AchievementMask am) = 0;
		virtual void				ClearAchievementMask(void) = 0;
		virtual AchievementMask		GetAchievementMask(void) const = 0;
        virtual DamageTrack*        GetDamageTrack(void) = 0;
        virtual void                CreateDamageTrack(void) = 0;
        virtual void                DeleteDamageTrack(void) = 0;
        virtual void                ResetDamageTrack(void) = 0;

        virtual void                AddMineDamage(DamageTypeID            type,
                                                  float                   amount,
                                                  Time                    timeCollision,
                                                  ImodelIGC*              pmodelLauncher,
                                                  const Vector&           position1,
                                                  const Vector&           position2) = 0;
        virtual void                ApplyMineDamage(void) = 0;

        virtual CommandID           GetDefaultOrder(ImodelIGC*  pmodel) = 0;

        virtual bool                OkToLaunch(Time now) = 0;
        virtual bool                PickDefaultOrder(IclusterIGC*   pcluster, const Vector& position, bool bDocked = true) = 0;

        virtual bool                IsGhost(void) const = 0;

        virtual float               GetEndurance(void) const = 0;

        virtual bool                InGarage(IshipIGC*  pship, float tCollision) const = 0;

        virtual bool                IsUsingAreaOfEffectWeapon(void) const = 0;

        virtual short               GetLaunchSlot(void) = 0;

        virtual WarningMask         GetWarningMask(void) const = 0;
        virtual void                SetWarningMaskBit(WarningMask wm) = 0;
        virtual void                ClearWarningMaskBit(WarningMask wm) = 0;

        //Special methods that work only if the ship is piloted by a particular pilot type
        //Builders & Layers
        virtual void                SetBaseData(IbaseIGC*  pbase) = 0;
        virtual IbaseIGC*           GetBaseData(void) const = 0;

        //Miners
        virtual float               GetOre(void) const = 0;
};

class IbuoyIGC : public ImodelIGC
{
    public:
        virtual void                 AddConsumer(void) = 0;
        virtual void                 ReleaseConsumer(void) = 0;
        virtual BuoyType             GetBuoyType(void) = 0;
};


class IchaffIGC : public ImodelIGC
{
    public:
};

class IprojectileIGC : public ImodelIGC
{
    public:
        virtual IprojectileTypeIGC*  GetProjectileType(void) const = 0;

        virtual ImodelIGC*           GetLauncher(void) const = 0;
        virtual void                 SetLauncher(ImodelIGC*  newVal) = 0;
        virtual void                 SetGunner(IshipIGC* pship) = 0;
};

class ImissileIGC : public IdamageIGC
{
    public:
        virtual ImissileTypeIGC*    GetMissileType(void) const = 0;
        virtual IshipIGC*           GetLauncher(void) const = 0;

        virtual ImodelIGC*          GetTarget(void) const = 0;
        virtual void                SetTarget(ImodelIGC* newVal) = 0;

        virtual float               GetLock(void) const = 0;

        virtual void                Explode(const Vector&   position) = 0;

        virtual void                Disarm(void) = 0;
};

class ImineIGC : public ImodelIGC
{
    public:
        virtual ImineTypeIGC*        GetMineType(void) const = 0;
        virtual IshipIGC*            GetLauncher(void) const = 0;
        virtual void                 SetCreateNow (void) = 0;
        virtual float                GetStrength(void) const = 0;
        virtual void                 ReduceStrength(float amount) = 0;
        virtual float                GetTimeFraction(void) const = 0;
};

class IbuildingEffectIGC : public IdamageIGC
{
    public:
        virtual void                BuilderTerminated(void) = 0;
        virtual IasteroidIGC*       GetAsteroid(void) const = 0;
        virtual void                MakeUnhitable(void) = 0;
        virtual void                AsteroidUpdate(Time now) = 0;
};

class IprobeIGC : public IscannerIGC
{
    public:
        virtual IprobeTypeIGC*       GetProbeType(void) const = 0;
        virtual IprojectileTypeIGC*  GetProjectileType(void) const = 0;
        virtual const Vector&        GetEmissionPt(void) const = 0;
        virtual float                GetProjectileLifespan(void) const = 0;
        virtual float                GetLifespan(void) const = 0;
        virtual float                GetDtBurst(void) const = 0;
        virtual float                GetAccuracy(void) const = 0;
        virtual bool                 GetCanRipcord(float ripcordspeed = -1.0f) const = 0;
        virtual float                GetRipcordDelay(void) const = 0;
        virtual SoundID              GetAmbientSound(void) const = 0;
        virtual void                 SetCreateNow (void) = 0;
        virtual float                GetTimeFraction(void) const = 0;
		//Xynth 7/2010 function to set probe expiration	ticket #10	
		virtual void				SetExpiration(Time time) = 0;
		virtual IshipIGC*			GetProbeLauncherShip() const = 0;
};

class IstationIGC : public IscannerIGC
{
    public:
        virtual void                    AddShip(IshipIGC* s) = 0;
        virtual void                    DeleteShip(IshipIGC* s) = 0;
        virtual IshipIGC*               GetShip(ShipID shipID) const = 0;
        virtual const ShipListIGC*      GetShips(void) const = 0;
        virtual void                    RepairAndRefuel(IshipIGC*   pship) const = 0;
        virtual void                    Launch(IshipIGC* pship) = 0;
        virtual bool                    InGarage(IshipIGC*  pship, const Vector& position) = 0;
		
		//Imago #121
		virtual ObjectID				GetRoidID() const = 0;
		virtual void SetRoidID(ObjectID id) = 0;

		virtual Vector					GetRoidPos() const = 0;
		virtual void SetRoidPos(Vector pos) = 0;

		virtual float					GetRoidSig() const = 0;
		virtual void SetRoidSig(float Sig) = 0;

		virtual float					GetRoidRadius() const = 0;
		virtual void SetRoidRadius(float Radius) = 0;
		
		virtual AsteroidAbilityBitMask	GetRoidCaps() const = 0;
		virtual void SetRoidCaps(AsteroidAbilityBitMask aabm) = 0;

		virtual void SetRoidSide(SideID sid, bool bset = true) = 0;
		virtual bool GetRoidSide(SideID sid) = 0;
		//

        virtual float                   GetShieldFraction(void) const = 0;
        virtual void                    SetShieldFraction(float sf) = 0;

        virtual const IstationTypeIGC*  GetStationType(void) const = 0;
        virtual IstationTypeIGC*        GetBaseStationType(void) const = 0;
        virtual void                    SetBaseStationType(IstationTypeIGC* pst) = 0;

        virtual bool                    CanBuy(const IbuyableIGC* b) const = 0;
        virtual bool                    IsObsolete(IbuyableIGC* b) const = 0;
        virtual IbuyableIGC*            GetSuccessor(IbuyableIGC* b) const = 0;
        virtual IpartTypeIGC*           GetSimilarPart(IpartTypeIGC* ppt) const = 0;

        virtual void                    SetLastDamageReport(Time timeLastDamage) = 0;
        virtual Time                    GetLastDamageReport(void) const = 0;

        virtual SoundID                 GetInteriorSound() const = 0;
        virtual SoundID                 GetExteriorSound() const = 0;
};

class IbuyableIGC : public ItypeIGC
{
    public:
        virtual const char*             GetName(void) const = 0;
        virtual const char*             GetIconName(void) const = 0;
        virtual const char*             GetDescription(void) const = 0;
        virtual const char*             GetModelName(void) const = 0;
        virtual const TechTreeBitMask&  GetRequiredTechs(void) const = 0;
        virtual const TechTreeBitMask&  GetEffectTechs(void) const = 0;

        virtual Money                   GetPrice(void) const = 0;
        virtual DWORD                   GetTimeToBuild(void) const = 0;
        virtual BuyableGroupID          GetGroupID(void) const = 0;
};

class IdevelopmentIGC : public IbuyableIGC
{
    public:
        virtual const GlobalAttributeSet&   GetGlobalAttributeSet(void) const = 0;
        virtual bool                        GetTechOnly(void) const = 0;
        virtual bool                        IsObsolete(const TechTreeBitMask&  ttbm) const = 0;
        virtual SoundID                     GetCompletionSound() const = 0;
};

class IdroneTypeIGC : public IbuyableIGC
{
    public:
        virtual HullID                  GetHullTypeID(void) const = 0;
        virtual PilotType               GetPilotType(void) const = 0;
        virtual float                   GetShootSkill(void) const = 0;
        virtual float                   GetMoveSkill(void) const = 0;
        virtual float                   GetBravery(void) const = 0;
        virtual IexpendableTypeIGC*     GetLaidExpendable(void) const = 0;
};

class IstationTypeIGC : public IbuyableIGC
{
    public:
        virtual HitPoints               GetMaxArmorHitPoints(void) const = 0;
        virtual DefenseTypeID           GetArmorDefenseType(void) const = 0;
        virtual HitPoints               GetMaxShieldHitPoints(void) const = 0;
        virtual DefenseTypeID           GetShieldDefenseType(void) const = 0;
        virtual float                   GetArmorRegeneration(void) const = 0;
        virtual float                   GetShieldRegeneration(void) const = 0;
        virtual float                   GetSignature(void) const = 0;
        virtual float                   GetRadius(void) const = 0;
        virtual float                   GetScannerRange(void) const = 0;
        virtual const TechTreeBitMask&  GetLocalTechs(void) const = 0;

        virtual Money                   GetIncome(void) const = 0;

        virtual StationAbilityBitMask   GetCapabilities(void) const = 0;
        virtual bool                    HasCapability(StationAbilityBitMask habm) const = 0;

        virtual const char*             GetTextureName(void) const = 0;
        virtual const char*             GetBuilderName(void) const = 0;

        virtual  IstationTypeIGC*       GetSuccessorStationType(const IsideIGC*   pside) = 0;
        virtual  IstationTypeIGC*       GetDirectSuccessorStationType() = 0; // EF5P - see wintrek\loadout.cpp
        virtual AsteroidAbilityBitMask  GetBuildAABM(void) const = 0;

        virtual int                     GetLaunchSlots(void) const = 0;
        virtual const Vector&           GetLaunchPosition(int   slotID) const = 0;
        virtual const Vector&           GetLaunchDirection(int   slotID) const = 0;

        virtual int                     GetLandSlots(void) const = 0;
        virtual int                     GetCapLandSlots(void) const = 0;
        virtual int                     GetLandPlanes(int   slotID) const = 0;
        virtual const Vector&           GetLandPosition(int slotID, int planeID) const = 0;
        virtual const Vector&           GetLandDirection(int slotID, int planeID) const = 0;

        virtual SoundID                 GetInteriorSound() const = 0;
        virtual SoundID                 GetInteriorAlertSound() const = 0;
        virtual SoundID                 GetExteriorSound() const = 0;

        virtual SoundID                 GetConstructorNeedRockSound() const = 0;
        virtual SoundID                 GetConstructorUnderAttackSound() const = 0;
        virtual SoundID                 GetConstructorDestroyedSound() const = 0;
        virtual SoundID                 GetCompletionSound() const = 0;

        virtual SoundID                 GetUnderAttackSound() const = 0;
        virtual SoundID                 GetCriticalSound() const = 0;
        virtual SoundID                 GetDestroyedSound() const = 0;
        virtual SoundID                 GetCapturedSound() const = 0;
        virtual SoundID                 GetEnemyCapturedSound() const = 0;
        virtual SoundID                 GetEnemyDestroyedSound() const = 0;

        virtual StationClassID          GetClassID() const = 0;
        virtual IdroneTypeIGC*          GetConstructionDroneType(void) const = 0;
};

class IbucketIGC : public IbuyableIGC
{
    public:
        virtual IbuyableIGC*            GetBuyable(void) const = 0;
        virtual IsideIGC*               GetSide(void) const = 0;

        virtual void                    SetPrice(Money m) = 0;

        virtual int                     GetPercentBought(void) const = 0;
        virtual int                     GetPercentComplete(void) const = 0;
        virtual bool                    GetCompleteF(void) const = 0;
        virtual void                    ForceComplete(Time now) = 0;

        virtual DWORD                   GetTime(void) const = 0;
        virtual Money                   GetMoney(void) const = 0;
        virtual void                    SetTimeAndMoney(DWORD dwTime, Money money) = 0;

        virtual Money                   AddMoney(Money m) = 0;
        virtual void                    SetEmpty(void) = 0;

        virtual ObjectType              GetBucketType(void) const = 0;

        virtual IbucketIGC*             GetPredecessor(void) const = 0;
};


class IprojectileTypeIGC : public ItypeIGC
{
    public:
        virtual float            GetPower(void) const = 0;
        virtual float            GetBlastPower(void) const = 0;
        virtual float            GetBlastRadius(void) const = 0;
        virtual float            GetSpeed(void) const = 0;
        virtual bool             GetAbsoluteF(void) const = 0;
        virtual char const*      GetModelName(void) const = 0;
        virtual char const*      GetModelTexture(void) const = 0;
        virtual float            GetLifespan(void) const = 0;
        virtual float            GetRadius(void) const = 0;
        virtual float            GetRotation(void) const = 0;
        virtual COLORVALUE       GetColor(void) const = 0; // was D3DCOLORVALUE
        virtual DamageTypeID     GetDamageType(void) const = 0;
        virtual SoundID          GetAmbientSound(void) const = 0;
};

class   IexpendableTypeIGC : public ItypeIGC
{
    public:
        virtual const LauncherDef*          GetLauncherDef(void) const = 0;
        virtual EquipmentType               GetEquipmentType(void) const = 0;
        virtual float                       GetLoadTime(void) const = 0;
        virtual float                       GetMass(void) const = 0;
        virtual float                       GetSignature(void) const = 0;
        virtual HitPoints                   GetHitPoints(void) const = 0;
        virtual DefenseTypeID               GetDefenseType(void) const = 0;
        virtual float                       GetLifespan(void) const = 0;
        virtual char const*                 GetName(void) const = 0;
        virtual char const*                 GetModelName(void) const = 0;
        virtual char const*                 GetModelTexture(void) const = 0;
        virtual float                       GetRadius(void) const = 0;

        virtual ExpendableAbilityBitMask    GetCapabilities(void) const = 0;
        virtual bool                        HasCapability(ExpendableAbilityBitMask habm) const = 0;
};

class   ImissileTypeIGC : public IexpendableTypeIGC
{
    public:
        virtual float           GetAcceleration(void) const = 0;
        virtual float           GetTurnRate(void) const = 0;
        virtual float           GetInitialSpeed(void) const = 0;
        //virtual float           GetArmTime(void) const = 0;

        virtual float           GetMaxLock(void) const = 0;
        virtual float           GetChaffResistance(void) const = 0;

        virtual float           GetLockTime(void) const = 0;
        virtual float           GetReadyTime(void) const = 0;

        virtual float           GetDispersion(void) const = 0;
        virtual float           GetLockAngle(void) const = 0;

        virtual float           GetPower(void) const = 0;
        virtual float           GetBlastPower(void) const = 0;
        virtual float           GetBlastRadius(void) const = 0;
        virtual DamageTypeID    GetDamageType(void) const = 0;

        virtual SoundID         GetLaunchSound(void) const = 0;
        virtual SoundID         GetAmbientSound(void) const = 0;
};

class   ImineTypeIGC : public IexpendableTypeIGC
{
    public:
        virtual float           GetPower(void) const = 0;
        virtual float           GetEndurance(void) const = 0;
        virtual DamageTypeID    GetDamageType(void) const = 0;
};

class   IchaffTypeIGC : public IexpendableTypeIGC
{
    public:
        virtual float               GetChaffStrength(void) const = 0;
};

class   IprobeTypeIGC : public IexpendableTypeIGC
{
    public:
        virtual float               GetScannerRange(void) const = 0;

        virtual float               GetDtBurst(void) const = 0;
        virtual float               GetDispersion(void) const = 0;
        virtual float               GetAccuracy(void) const = 0;
        virtual short               GetAmmo(void) const = 0;
        virtual IprojectileTypeIGC* GetProjectileType(void) const = 0;
        virtual const Vector&       GetEmissionPt(void) const = 0;
        virtual float               GetRipcordDelay(void) const = 0;
        virtual SoundID             GetAmbientSound(void) const = 0;
};

class IhullTypeIGC : public IbuyableIGC
{
    public:
        virtual float                GetLength(void) const = 0;
        virtual float                GetMaxSpeed(void) const = 0;
        virtual float                GetMaxTurnRate(Axis    axis) const = 0;
        virtual float                GetTurnTorque(Axis     axis) const = 0;
        virtual float                GetThrust(void) const = 0;
        virtual float                GetSideMultiplier(void) const = 0;
        virtual float                GetBackMultiplier(void) const = 0;
        virtual float                GetScannerRange(void) const = 0;

        virtual float                GetMaxEnergy(void) const = 0;
        virtual float                GetRechargeRate(void) const = 0;

        virtual HitPoints            GetHitPoints(void) const = 0;
        virtual DefenseTypeID        GetDefenseType(void) const = 0;
        virtual PartMask             GetPartMask(EquipmentType et, Mount  mountID) const = 0;
        virtual short                GetCapacity(EquipmentType et) const = 0;
        virtual Mount                GetMaxWeapons(void) const = 0;
        virtual Mount                GetMaxFixedWeapons(void) const = 0;
        virtual const HardpointData& GetHardpointData(Mount hardpointID) const = 0;
        virtual bool                 CanMount(IpartTypeIGC* ppt, Mount  mountID) const = 0;

        virtual const char*          GetTextureName(void) const = 0;

        virtual const Vector&        GetCockpit(void) const = 0;
		virtual const Vector&		 GetChaffPosition(void) const = 0; // TurkeyXIII 11/09 #94

        virtual float                GetMass(void) const = 0;
        virtual float                GetSignature(void) const = 0;

        virtual HullAbilityBitMask   GetCapabilities(void) const = 0;
        virtual bool                 HasCapability(HullAbilityBitMask habm) const = 0;

        virtual const Vector&        GetWeaponPosition(Mount mount) const = 0;
        virtual const Orientation&   GetWeaponOrientation(Mount mount) const = 0;
        virtual float                GetScale() const = 0;

        virtual short                GetMaxAmmo(void) const = 0;
        virtual float                GetMaxFuel(void) const = 0;

        virtual float                GetECM(void) const = 0;
        virtual float                GetRipcordSpeed(void) const = 0;
        virtual float                GetRipcordCost(void) const = 0;

        virtual IhullTypeIGC*        GetSuccessorHullType(void) const = 0;

        //virtual const char*          GetPilotHUDName(void) const = 0;
        //virtual const char*          GetObserverHUDName(void) const = 0;

        virtual SoundID              GetInteriorSound(void) const = 0;
        virtual SoundID              GetExteriorSound(void) const = 0;
        virtual SoundID              GetMainThrusterInteriorSound(void) const = 0;
        virtual SoundID              GetMainThrusterExteriorSound(void) const = 0;
        virtual SoundID              GetManuveringThrusterInteriorSound(void) const = 0;
        virtual SoundID              GetManuveringThrusterExteriorSound(void) const = 0;

        virtual const PartTypeListIGC*      GetPreferredPartTypes(void) const = 0;

        virtual IObject*             GetIcon(void) const = 0;

        virtual int                     GetLaunchSlots(void) const = 0;
        virtual const Vector&           GetLaunchPosition(int   slotID) const = 0;
        virtual const Vector&           GetLaunchDirection(int   slotID) const = 0;

        virtual int                     GetLandSlots(void) const = 0;
        virtual int                     GetLandPlanes(int   slotID) const = 0;
        virtual const Vector&           GetLandPosition(int slotID, int planeID) const = 0;
        virtual const Vector&           GetLandDirection(int slotID, int planeID) const = 0;
};

class IpartTypeIGC : public IbuyableIGC
{
    public:
        virtual EquipmentType   GetEquipmentType(void) const = 0;
        virtual PartMask        GetPartMask(void) const = 0;

        virtual float           GetMass(void) const = 0;
        virtual float           GetSignature(void) const = 0;

        virtual IpartTypeIGC*   GetSuccessorPartType(void) const = 0;

        virtual const char*     GetInventoryLineMDLName(void) const = 0;

        static const char*      GetEquipmentTypeName(EquipmentType et);

        virtual short           GetAmount(IshipIGC* pship) const { return 1; }
};

class IlauncherTypeIGC : public IpartTypeIGC
{
    public:
        static  bool                IsLauncherType(EquipmentType et)
        {
            return (et == ET_ChaffLauncher) ||
                   (et == ET_Magazine) ||
                   (et == ET_Dispenser);
        }

        virtual IexpendableTypeIGC* GetExpendableType(void) const = 0;
        virtual short               GetLaunchCount (void) const = 0;
};

class IpartIGC : public IbaseIGC
{
    public:
        virtual EquipmentType    GetEquipmentType(void) const = 0;
        virtual IpartTypeIGC*    GetPartType(void) const = 0;

        virtual IshipIGC*        GetShip(void) const = 0;
        virtual void             SetShip(IshipIGC* newVal, Mount mount) = 0;

        virtual Mount            GetMountID(void) const = 0;
        virtual void             SetMountID(Mount newVal) = 0;

        virtual bool             fActive(void) const = 0;
        virtual void             Activate(void) = 0;
        virtual void             Deactivate(void) = 0;

        virtual float            GetMass(void) const = 0;
        virtual Money            GetPrice(void) const = 0;

        virtual float            GetMountedFraction(void) const = 0;
        virtual void             SetMountedFraction(float f) = 0;

        virtual void             Arm(void) = 0;

        virtual short            GetAmount(void) const { return 1; }
        virtual void             SetAmount(short a) {}
};

class IweaponIGC : public IpartIGC
{
    public:
        virtual IprojectileTypeIGC*  GetProjectileType(void) const = 0;
        virtual short                GetAmmoPerShot(void) const = 0;
        virtual float                GetLifespan(void) const = 0;
        virtual float                GetDtBurst(void) const = 0;
        virtual const Vector&        GetPosition(void) const = 0;
        virtual IshipIGC*            GetGunner(void) const = 0;
        virtual void                 SetGunner(IshipIGC* newVal) = 0;

        virtual bool                 fFiringShot(void) const = 0;
        virtual bool                 fFiringBurst(void) const = 0;

        virtual SoundID              GetActivateSound(void) const = 0;
        virtual SoundID              GetSingleShotSound(void) const = 0;
        virtual SoundID              GetBurstSound(void) const = 0;
};

class IshieldIGC : public IpartIGC
{
    public:
        virtual float           GetRegeneration(void) const = 0;
        virtual float           GetMaxStrength(void) const = 0;

        virtual DefenseTypeID   GetDefenseType(void) const = 0;

        virtual float           GetFraction(void) const = 0;
        virtual void            SetFraction(float newVal) = 0;

        virtual float           ApplyDamage(Time           timeCollision,
                                            DamageTypeID   type,
                                            float          amount,
                                            const Vector&  deltaP) = 0;
};

class IcloakIGC : public IpartIGC
{
    public:
        virtual float    GetEnergyConsumption(void) const = 0;
        virtual float    GetMaxCloaking(void) const = 0;
        virtual float    GetOnRate(void) const = 0;
        virtual float    GetOffRate(void) const = 0;

        virtual SoundID  GetEngageSound() const = 0;
        virtual SoundID  GetDisengageSound() const = 0;
};

class IafterburnerIGC : public IpartIGC
{
    public:
        virtual float    GetFuelConsumption(void) const = 0;
		virtual float    GetMaxThrustWithGA(void) const = 0; //TheRock 15-8-2009
        virtual float    GetMaxThrust(void) const = 0;
        virtual float    GetOnRate(void) const = 0;
        virtual float    GetOffRate(void) const = 0;

        virtual void     IncrementalUpdate(Time lastUpdate, Time now, bool bUseFuel) = 0;

        virtual float    GetPower(void) const = 0;
        virtual void     SetPower(float p) = 0;

        virtual SoundID  GetInteriorSound(void) const = 0;
        virtual SoundID  GetExteriorSound(void) const = 0;
};

class IpackIGC : public IpartIGC
{
    public:
        virtual PackType GetPackType(void) const = 0;
};

class IlauncherIGC : public IpartIGC
{
    public:
        static bool                 IsLauncher(ObjectType type)
        {
            return ((type == OT_magazine) || (type == OT_dispenser));
        }

        virtual const Vector&       GetEmissionPt(void) const = 0;

        virtual void                SetTimeFired(Time   now) = 0;

        virtual Time                GetTimeLoaded(void) const = 0;
        virtual void                SetTimeLoaded(Time  tl) = 0;
        virtual void                ResetTimeLoaded(void) = 0;

        virtual float               GetArmedFraction(void) = 0;
};

class ImagazineIGC : public IlauncherIGC
{
    public:
        virtual ImissileTypeIGC*    GetMissileType(void) const = 0;

        virtual short               GetLaunchCount (void) const = 0;

        virtual float               GetLock(void) const = 0;
        virtual void                SetLock(float l) = 0;
};

class IdispenserIGC : public IlauncherIGC
{
    public:
        virtual IexpendableTypeIGC* GetExpendableType(void) const = 0;
};

class IclusterIGC : public IbaseIGC
{
    public:
        virtual const char*             GetName(void) const = 0;

        virtual Time                    GetLastUpdate(void) const = 0;
        virtual void                    SetLastUpdate(Time now) = 0;

        //Note: the Add/Delete methods should never be called from outside IGC -- that is handled automatically

        virtual void                    AddModel(ImodelIGC* modelNew) = 0;
        virtual void                    DeleteModel(ImodelIGC* modelOld) = 0;

        virtual ImodelIGC*              GetModel(const char* name) const = 0;
        virtual const ModelListIGC*     GetModels(void) const = 0;

        virtual const ModelListIGC*     GetPickableModels(void) const = 0;

        virtual void                    AddWarp(IwarpIGC* warpNew) = 0;
        virtual void                    DeleteWarp(IwarpIGC* warpOld) = 0;
        virtual const WarpListIGC*      GetWarps(void) const = 0;

        virtual void                    AddMine(ImineIGC* ms) = 0;
        virtual void                    DeleteMine(ImineIGC* m) = 0;
        virtual ImineIGC*               GetMine(MineID mineID) const = 0;
        virtual const MineListIGC*      GetMines(void) const = 0;

        virtual void                    AddMissile(ImissileIGC* ms) = 0;
        virtual void                    DeleteMissile(ImissileIGC* m) = 0;
        virtual ImissileIGC*            GetMissile(MissileID missileID) const = 0;
        virtual const MissileListIGC*   GetMissiles(void) const = 0;

        virtual void                    AddProbe(IprobeIGC* ms) = 0;
        virtual void                    DeleteProbe(IprobeIGC* m) = 0;
        virtual IprobeIGC*              GetProbe(ProbeID probeID) const = 0;
        virtual const ProbeListIGC*     GetProbes(void) const = 0;

        virtual float                   GetScreenX(void) const = 0;
        virtual float                   GetScreenY(void) const = 0;

        virtual void                    SetActive(bool bActive) = 0;

        virtual void                    AddStation(IstationIGC* stationNew) = 0;
        virtual void                    DeleteStation(IstationIGC* stationOld) = 0;
        virtual IstationIGC*            GetStation(StationID stationID) const = 0;
        virtual const StationListIGC*   GetStations(void) const = 0;

        virtual void                    AddShip(IshipIGC* shipNew) = 0;
        virtual void                    DeleteShip(IshipIGC* shipOld) = 0;
        virtual IshipIGC*               GetShip(ShipID shipID) const = 0;
        virtual const ShipListIGC*      GetShips(void) const = 0;

        virtual void                    AddAsteroid(IasteroidIGC* asteroidNew) = 0;
        virtual void                    DeleteAsteroid(IasteroidIGC* asteroidOld) = 0;
        virtual IasteroidIGC*           GetAsteroid(AsteroidID asteroidID) const = 0;
        virtual const AsteroidListIGC*  GetAsteroids(void) const = 0;

        virtual void                    AddTreasure(ItreasureIGC*    treasureNew) = 0;
        virtual void                    DeleteTreasure(ItreasureIGC* treasureOld) = 0;
        virtual ItreasureIGC*           GetTreasure(TreasureID treasureID) const = 0;
        virtual const TreasureListIGC*  GetTreasures(void) const = 0;

        virtual ClusterSite*            GetClusterSite(void) const = 0;

        virtual IbuildingEffectIGC*     CreateBuildingEffect(Time           now,
                                                             IasteroidIGC*  pasteroid,
                                                             IstationIGC*   pstation,
                                                             IshipIGC*      pshipBuilder,
                                                             float          radiusAsteroid,
                                                             float          radiusStation,
                                                             const Vector&  positionStart,
                                                             const Vector&  positionStop) = 0;

        virtual short                   GetNstars(void) const = 0;
        virtual short                   GetNdebris(void) const = 0;
        virtual unsigned int            GetStarSeed(void) const = 0;
        virtual const Vector&           GetLightDirection(void) const = 0;
        virtual Color                   GetLightColor(void) const = 0;
        virtual Color                   GetLightColorAlt(void) const = 0;
        virtual float                   GetBidirectionalAmbientLevel(void) const = 0;
        virtual float                   GetAmbientLevel(void) const = 0;

        virtual void             RecalculateCollisions(float        tOffset,
                                                       ImodelIGC*   pModel1,
                                                       ImodelIGC*   pModel2) = 0;

        virtual ExplosionData*   CreateExplosion(DamageTypeID   damageType,
                                                 float          amount,
                                                 float          radius,
                                                 int            explosionType,
                                                 Time           time,
                                                 const Vector&  position,
                                                 ImodelIGC*     pLauncher) = 0;

        virtual const char*      GetPosterName(void) const = 0;

        virtual void             SetPrivateData(DWORD dwPrivate) = 0;
        virtual DWORD            GetPrivateData(void) const = 0;

        virtual void             FlushCollisionQueue(void) = 0;
        virtual void             MakeModelStatic(ImodelIGC* pmodel) = 0;
        virtual void             MakeModelUnhitable(ImodelIGC* pmodel) = 0;
        virtual void             FreeThingSite(ModelAttributes  mt, ImodelIGC*   pmodel, HitTest* pht) = 0;

        virtual bool             GetHomeSector(void) const = 0;
        virtual float            GetPendingTreasures(void) const = 0;
        virtual void             SetPendingTreasures(float  fpt) = 0;
        virtual float            GetCost(void) const = 0;
		virtual void			 SetHighlight(bool hl) = 0; //Xynth #208
		virtual bool			 GetHighlight(void) const = 0;
};

class IasteroidIGC : public IdamageIGC
{
    public:
        virtual float                   GetOre(void) const = 0;
        virtual void                    SetOre(float newVal) = 0;
        virtual float                   MineOre(float dt) = 0;

        virtual AsteroidAbilityBitMask  GetCapabilities(void) const = 0;
        virtual bool                    HasCapability(AsteroidAbilityBitMask aabm) const = 0;

        virtual IbuildingEffectIGC*     GetBuildingEffect(void) const = 0;
        virtual void                    SetBuildingEffect(IbuildingEffectIGC* pbe) = 0;

        static const char*              GetTypeName(AsteroidAbilityBitMask aabm);
        static const char*              GetTypePrefix(int   index);
        static const AsteroidDef&       GetTypeDefaults(int index);

        static int                      NumberSpecialAsteroids(const MissionParams*  pmp);
        static int                      GetSpecialAsterioid(const MissionParams*  pmp, int index);
        static int                      GetRandomType(AsteroidAbilityBitMask aabm);
		//Xynth #100 7/2010
		virtual float GetOreSeenBySide(IsideIGC *side1) const = 0;
		virtual bool GetAsteroidCurrentEye(IsideIGC *side1) const = 0;
		virtual void SetOreWithFraction(float oreFraction, bool clientUpdate) = 0;  //Xynth #163 7/2010
		virtual float GetOreFraction() const = 0; //Xynth #163
		//Imago 8/10 #120 #121
		virtual void SetBuilderSeenSide(ObjectID oid) = 0;
		virtual bool GetBuilderSeenSide(ObjectID oid) = 0;
		virtual void SetInhibitUpdate(bool inhib) = 0; //Xynth #225 9/10
};

class IwarpIGC : public ImodelIGC
{
    public:
        virtual IwarpIGC*           GetDestination(void) = 0;  //Intentionally not const since we cache the destination
        virtual void                AddBomb(Time               timeExplode,
                                            ImissileTypeIGC*   pmt) = 0;
        virtual const WarpBombList* GetBombs(void) const = 0;
		virtual bool                IsFixedPosition()    = 0; // KG- added
		virtual double				MassLimit()			 = 0; // Andon - Added for Mass Limted Alephs
};

class ItreasureIGC : public ImodelIGC
{
    public:
        virtual TreasureCode        GetTreasureCode(void) const = 0;
        virtual IbuyableIGC*        GetBuyable(void) const = 0;
        virtual ObjectID            GetTreasureID(void) const = 0;
        virtual void                SetTreasureID(ObjectID newVal)  = 0;
        virtual short               GetAmount(void) const = 0;
        virtual void                SetAmount(short a) = 0;
        virtual void                ResetExpiration(Time    now) = 0;
        virtual void                SetCreateNow (void) = 0;
};


class IsideIGC : public IbaseIGC
{
    public:
        virtual IcivilizationIGC*           GetCivilization(void) const = 0;
        virtual void                        SetCivilization(IcivilizationIGC*   pciv) = 0;
        virtual void                        DestroyBuckets(void) = 0;
        virtual void                        CreateBuckets(void) = 0;

        virtual void                        SetName(const char* newVal) = 0;
        virtual const char*                 GetName(void) const = 0;

        virtual SquadID                     GetSquadID() const = 0;
        virtual void                        SetSquadID(SquadID squadID) = 0;

        virtual const TechTreeBitMask       GetTechs(void) const = 0;

        virtual const TechTreeBitMask&      GetBuildingTechs(void) const = 0;
        virtual void                        ResetBuildingTechs(void) = 0;
        virtual void                        SetBuildingTechs(const TechTreeBitMask& ttbm) = 0;

        virtual const TechTreeBitMask&      GetDevelopmentTechs(void) const = 0;
        virtual void                        SetDevelopmentTechs(const TechTreeBitMask& ttbm) = 0;
		virtual bool						IsNewDevelopmentTechs(const TechTreeBitMask& ttbm) = 0; // KGJV #118
        virtual bool                        ApplyDevelopmentTechs(const TechTreeBitMask& ttbm) = 0;

        virtual const TechTreeBitMask&      GetInitialTechs(void) const = 0;
        virtual void                        SetInitialTechs(const TechTreeBitMask& ttbm) = 0;
        virtual void                        UpdateInitialTechs(void) = 0;

        virtual const TechTreeBitMask&      GetUltimateTechs(void) const = 0;

        virtual void                        AddStation(IstationIGC* s) = 0;
        virtual void                        DeleteStation(IstationIGC* s) = 0;
        virtual IstationIGC*                GetStation(StationID id) const = 0;
        virtual const StationListIGC*       GetStations(void) const = 0;

        virtual void                        AddShip(IshipIGC* s) = 0;
        virtual void                        DeleteShip(IshipIGC* s) = 0;
        virtual IshipIGC*                   GetShip(ShipID id) const = 0;
        virtual const ShipListIGC*          GetShips(void) const = 0;

        virtual void                        AddBucket(IbucketIGC* s) = 0;
        virtual void                        DeleteBucket(IbucketIGC* s) = 0;
        virtual IbucketIGC*                 GetBucket(BucketID      bucketID) const = 0;
        virtual const BucketListIGC*        GetBuckets(void) const = 0;

        virtual bool                        CanBuy(const IbuyableIGC* b) const = 0;

        virtual const GlobalAttributeSet&   GetGlobalAttributeSet(void) const = 0;
        virtual void                        SetGlobalAttributeSet(const GlobalAttributeSet& gas) = 0;
        virtual void                        ApplyGlobalAttributeSet(const GlobalAttributeSet& gas) = 0;
        virtual void                        ResetGlobalAttributeSet(void) = 0;

        virtual bool                        GetActiveF(void) const = 0;
        virtual void                        SetActiveF(bool activeF) = 0;

        virtual const Color&                GetColor(void) const = 0;

        virtual void                        SetPrivateData(DWORD dwPrivate) = 0;
        virtual DWORD                       GetPrivateData(void) const = 0;

        virtual short                       GetKills(void) const = 0;
        virtual void                        AddKill(void) = 0;

        virtual short                       GetDeaths(void) const = 0;
        virtual void                        AddDeath(void) = 0;

        virtual short                       GetEjections(void) const = 0;
        virtual void                        AddEjection(void) = 0;

        virtual short                       GetBaseKills(void) const = 0;
        virtual void                        AddBaseKill(void) = 0;

        virtual short                       GetBaseCaptures(void) const = 0;
        virtual void                        AddBaseCapture(void) = 0;

        virtual void                        Reset(void) = 0;

        virtual void                        AddToStockpile(IbuyableIGC* b, short count = 1) = 0;
        virtual short                       RemoveFromStockpile(IbuyableIGC* b, short count = 1) = 0;
        virtual short                       GetStockpile(IbuyableIGC* b) const = 0;
        virtual const StockpileList*        GetStockpile(void) const = 0;

        //
        // Game state
        //

        virtual unsigned char GetConquestPercent(void) const  = 0;
        virtual void          SetConquestPercent(unsigned char newVal) = 0;
        virtual unsigned char GetTerritoryCount(void) const = 0;
        virtual void          SetTerritoryCount(unsigned char newVal) = 0;
        virtual short         GetFlags(void) const  = 0;
        virtual void          SetFlags(short newVal) = 0;
        virtual short         GetArtifacts(void) const  = 0;
        virtual void          SetArtifacts(short newVal) = 0;
        virtual float         GetTimeEndured() const = 0;
        virtual void          SetTimeEndured(float fSeconds) = 0;
        virtual long          GetProsperityPercentBought(void) const = 0;
        virtual long          GetProsperityPercentComplete(void) const = 0;

		// ALLIES #ALLY
		virtual void		  SetAllies(char allies) = 0;
		virtual char          GetAllies() = 0;
		static bool           AlliedSides(IsideIGC *side1, IsideIGC *side2)
		{
			if( side1==side2) return true;
			if (side1==NULL) return false;
			if (side2==NULL) return false;
			if (side1->GetAllies() == NA) return false;
			return (side1->GetAllies() == side2->GetAllies());
		}
		//
		//Xynth #170 8/10
		virtual bool GetRandomCivilization(void) const = 0;
		virtual void SetRandomCivilization(bool rand) = 0;

		//Xynth Adding function to return number of players on a side
		virtual int GetNumPlayersOnSide(void) const = 0;
};

class IcivilizationIGC : public IbaseIGC
{
    public:
        virtual const char*                 GetName(void) const = 0;
        virtual const char*                 GetIconName(void) const = 0;
        virtual const char*                 GetHUDName(void) const = 0;
        virtual IhullTypeIGC*               GetLifepod(void) const = 0;

        virtual IstationTypeIGC*            GetInitialStationType(void) const = 0;

        virtual const TechTreeBitMask&      GetBaseTechs(void) const = 0;
        virtual const TechTreeBitMask&      GetNoDevTechs(void) const = 0;

        virtual const GlobalAttributeSet&   GetBaseAttributes(void) const = 0;

        virtual float                       GetBonusMoney(void) const = 0;
        virtual float                       GetIncomeMoney(void) const = 0;
};

class ItreasureSetIGC : public IbaseIGC
{
    public:
        virtual const char*                 GetName(void) const = 0;
        virtual bool                        GetZoneOnly(void) const = 0;
		virtual short						GetSize(void) const = 0; //imago added 7/30/08
        virtual void                        AddTreasureData(TreasureCode tc, ObjectID oid, unsigned char chance) = 0;

        virtual const TreasureData&         GetRandomTreasureData(void) const = 0;
};

typedef short   AssetMask;
const AssetMask     c_amFighter           = 0x01;
const AssetMask     c_amMiner             = 0x02;
const AssetMask     c_amBuilder           = 0x04;
const AssetMask     c_amStation           = 0x08;
const AssetMask     c_amEnemyFighter      = 0x10;
const AssetMask     c_amEnemyMiner        = 0x20;
const AssetMask     c_amEnemyBuilder      = 0x40;
const AssetMask     c_amEnemyAPC          = 0x80;
const AssetMask     c_amEnemyStation      = 0x100;
const AssetMask     c_amEnemyCapital      = 0x200;
const AssetMask     c_amEnemyCarrier      = 0x400;
const AssetMask     c_amCarrier           = 0x800;
const AssetMask     c_amEnemyTeleport     = 0x1000;
const AssetMask     c_amEnemyTeleportShip = 0x2000;
const AssetMask     c_amEnemyBomber       = 0x4000;

typedef short   ClusterWarning;
const ClusterWarning    c_cwNoThreat                = 0;
const ClusterWarning    c_cwEnemyFighterInCluster   = 1;
const ClusterWarning    c_cwEnemyMinerInCluster     = 2;
const ClusterWarning    c_cwEnemyBuilderInCluster   = 3;
const ClusterWarning    c_cwCombatInCluster         = 4;
const ClusterWarning    c_cwMinerThreatened         = 5;
const ClusterWarning    c_cwBuilderThreatened       = 6;
const ClusterWarning    c_cwCarrierThreatened       = 7;
const ClusterWarning    c_cwBomberInCluster         = 8;
const ClusterWarning    c_cwEnemyCarrierInCluster   = 9;
const ClusterWarning    c_cwCapitalInCluster        = 10;
const ClusterWarning    c_cwTeleportInCluster       = 11;
const ClusterWarning    c_cwTransportInCluster      = 12;
const ClusterWarning    c_cwStationThreatened       = 13;
const ClusterWarning    c_cwStationTeleportThreat   = 14;
const ClusterWarning    c_cwStationCaptureThreat    = 15;
const ClusterWarning    c_cwMax                     = 16;

ClusterWarning  GetClusterWarning(AssetMask am, bool bInvulnerableStations);
const char*     GetClusterWarningText(ClusterWarning cw);

typedef char    ShipState;

const ShipState    c_ssDead = 1;                //A nice enumeration of all the things it could have been doing
const ShipState    c_ssDocked = 2;              //when it was last seen (if only briefly)

const ShipState    c_ssFlying = 3;

const ShipState    c_ssObserver = 4;
const ShipState    c_ssTurret = 5;

class   ShipStatus
{
    public:
        ShipStatus(void)
        {
            Reset();
        }

        void    Reset(void)
        {
            m_hullID = NA;
            m_sectorID = NA;
            m_stationID = NA;
            m_state = c_ssDead;
            m_unknown = true;
            m_detected = false;
			m_dTime = Time::Now().clock();
        }

        bool        operator != (const ShipStatus& ss)
        {
            return (m_hullID != ss.m_hullID) ||
                   (m_sectorID != ss.m_sectorID) ||
                   (m_stationID != ss.m_stationID) ||
                   (m_unknown != ss.m_unknown) ||
                   (m_detected != ss.m_detected) ||
                   (m_state != ss.m_state);
        }

        HullID      GetHullID(void) const
        {
            return m_hullID;
        }
        void        SetHullID(HullID    newVal)
        {
            m_hullID = newVal;
        }

        SectorID    GetSectorID(void) const
        {
            return m_sectorID;
        }
        void        SetSectorID(SectorID    newVal)
        {
            m_sectorID = newVal;
        }

        StationID   GetStationID(void) const
        {
            return m_stationID;
        }
        void        SetStationID(StationID stationID)
        {
            m_stationID = stationID;
        }

        StationID   GetParentID(void) const
        {
            return m_shipIDParent;
        }
        void        SetParentID(ShipID shipID)
        {
            m_shipIDParent = shipID;
        }

        ShipState   GetState(void) const
        {
            return m_state;
        }
        void        SetState(ShipState    newVal)
        {
            m_state = newVal;
            m_unknown = false;
        }

        bool        GetUnknown(void) const
        {
            return m_unknown;
        }
        void        SetUnknown(void)
        {
            m_unknown = true;
        }

        bool        GetDetected(void) const
        {
            return m_detected;
        }
        void        SetDetected(bool    bDetected)
        {
            m_detected = bDetected;
        }
        DWORD        GetStateTime(void) const
        {
            return m_dTime;
        }
        void        SetStateTime(DWORD    dTime)
        {
            m_dTime = dTime;
        }

    private:
        HullID      m_hullID;
        SectorID    m_sectorID;
        StationID   m_stationID;
        ShipID      m_shipIDParent;
        ShipState   m_state : 6;
        bool        m_unknown : 1;
        bool        m_detected : 1;
		DWORD		m_dTime;
};

class ClusterSite : public AttachSite
{
    public:
        virtual HRESULT         AddPoster(const char*      textureName,
                                          const Vector&    position,
                                          float            radius) { return S_OK;}

        virtual HRESULT         GetEnvironmentGeo(const char* pszName) { return S_OK; }

        virtual Geo*            GetEnvironmentGeo() { return NULL; }
        virtual GroupGeo*       GetGroupScene()     { return NULL; }
        virtual PosterImage*    GetPosterImage()    { return NULL; }
        virtual void            SetEnvironmentGeo(const char* pszName) {}

        virtual float           GetRotation(void) const { return 0.0f; }
        virtual void            AddRotation(float da) {}

        virtual void                    AddScanner(SideID   sid, IscannerIGC* scannerNew) {}
        virtual void                    DeleteScanner(SideID   sid, IscannerIGC* scannerOld) {}
        virtual const ScannerListIGC*   GetScanners(SideID   sid) const { return NULL; }

        virtual AssetMask               GetClusterAssetMask(void) { return 0; }
        virtual void                    SetClusterAssetMask(AssetMask am) {}

        virtual void                    MoveShip(void) {};
        virtual void                    MoveStation(void) {}
};

//Utility functions
float    solveForImpact(const Vector&      deltaP,
                        const Vector&      deltaV,
                        float              speed,
                        float              radius,
                        Vector*            direction);
float    solveForLead(ImodelIGC*         shooter,
                      ImodelIGC*         target,
                      IweaponIGC*        weapon,
                      Vector*            direction,
                      float              skill = 1.0f);
float    turnToFace(const Vector&       deltaTarget,
                    float               dt,
                    IshipIGC*           pship,
                    ControlData*        controls,
                    float               skill = 1.0f);

IwarpIGC*   FindPath(IshipIGC* pShip, ImodelIGC* pTarget, bool bCowardly);

const char* GetModelType(ImodelIGC* pmodel);
const char* GetModelName(ImodelIGC* pmodel);

bool        LineOfSightExist(const IclusterIGC* pcluster,
                             const ImodelIGC*   pmodel1,
                             const ImodelIGC*   pmodel2);

//Note ... the following enum defines bits in a mask
enum TargetType
{
    c_ttSpecial     = 0x80000,        //Special hack for commands
    c_ttMyQueued    = 0x80000,        //Low byte must agree with c_cmd
    c_ttMyAccepted  = 0x80001,
    c_ttMyTarget    = 0x80002,
    c_ttWorstEnemy  = 0x80003,
    c_ttMe          = 0x80004,

    c_ttFriendly    = 0x0001,
    c_ttEnemy       = 0x0002,
    c_ttNeutral     = 0x0004,

    c_ttShip        = 0x0008,
    c_ttStation     = 0x0010,
    c_ttAsteroid    = 0x0020,
    c_ttTreasure    = 0x0040,
    c_ttMissile     = 0x0080,
    c_ttWarp        = 0x0100,
    c_ttBuoy        = 0x0200,

    c_ttFront       = 0x0400,    //these two are exclusive
    c_ttNearest     = 0x0800,

    c_ttAnyCluster  = 0x1000,
    c_ttPrevious    = 0x2000,

    c_ttMine        = 0x4000,
    c_ttProbe       = 0x8000,

    c_ttLeastTargeted = 0x10000,
    c_ttNoRipcord     = 0x20000,
    c_ttCowardly      = 0x40000,

    c_ttShipTypes   = c_ttShip | c_ttStation,

    c_ttAllTypes    = c_ttShip | c_ttStation |
                      c_ttAsteroid | c_ttBuoy |
                      c_ttWarp | c_ttTreasure | c_ttMissile | c_ttMine | c_ttProbe,

    c_ttAllSides    = c_ttFriendly | c_ttEnemy | c_ttNeutral,

    c_ttAll         = c_ttAllSides | c_ttAllTypes
};

extern const int c_ttTypebits[OT_modelEnd+1];

inline int GetTypebits(ObjectType  ot)
{
    assert (c_ttTypebits[OT_warp] == c_ttWarp);
    assert (c_ttTypebits[OT_ship] == c_ttShip);
    assert (c_ttTypebits[OT_asteroid] == c_ttAsteroid);
    assert (c_ttTypebits[OT_station] == c_ttStation);
    assert (c_ttTypebits[OT_treasure] == c_ttTreasure);
    assert (c_ttTypebits[OT_missile] == c_ttMissile);
    assert (c_ttTypebits[OT_mine] == c_ttMine);
    assert (c_ttTypebits[OT_probe] == c_ttProbe);
    assert (c_ttTypebits[OT_buoy] == c_ttBuoy);

    return c_ttTypebits[ot];
}

bool  FindableModel(ImodelIGC*    		m,
                    IsideIGC*     		pside,
                    int                 ttMask,
                    AbilityBitMask      abmAbilities,
					int 				iAllies = 1);  //Imago 7/31/09 e.g. 1 = normal (your side + allies) 0 = your side only 2 = allies only

ImodelIGC*  FindTarget(IshipIGC*            pShip,
                       int                  ttMask,
                       ImodelIGC*           pmodelCurrent = NULL,
                       IclusterIGC*         pcluster = NULL,
                       const Vector*        pposition = NULL,
                       const Orientation*   porientation = NULL,
                       AbilityBitMask       abmAbilities = 0,                   //e.g. anything
                       int                  maxDistance = 0x7fffffff,			//e.g. anywhere
					   int 					bAllies = 1);  //Imago 7/31/09        e.g. pass-thru to FindableModel --^

int         GetDistance(IshipIGC*       pship,
                        IclusterIGC*    pclusterOne,
                        IclusterIGC*    pclusterTwo,
                        int             maxDistance = 0x7fffffff);

void        PopulateCluster(ImissionIGC*            pmission,
                            const MissionParams*    pmp,
                            IclusterIGC*            pcluster,
                            float                   amountHe3,
                            bool                    bAsteroids = true,
                            bool                    bTreasures = true,
                            short                   nMineableAsteroidsMultiplier = 1,
                            short                   nSpecialAsteroidsMultiplier = 1,
                            short                   nAsteroidsMultiplier = 1);

void        CreateAsteroid(ImissionIGC*         pmission,
                           IclusterIGC*         pcluster,
                           int                  type,
                           float                amountHe3);

class   SideVisibility
{
    public:
        SideVisibility(void)
        :
            m_fVisible(false),
			m_currentEyed(false),
            m_pLastSpotter(NULL)
        {
        }

        ~SideVisibility(void)
        {
            if (m_pLastSpotter)
                m_pLastSpotter->Release();
        }

        bool    fVisible(void) const
        {
            return m_fVisible;
        }

        IscannerIGC*    pLastSpotter(void) const
        {
            return m_pLastSpotter;
        }

        void        fVisible(bool   v)
        {
            m_fVisible = v;
        }

        void        pLastSpotter(IscannerIGC*  s)
        {
            if (m_pLastSpotter)
                m_pLastSpotter->Release();

            m_pLastSpotter = s;

            if (s)
                s->AddRef();
        }

		//Xynth #100 7/2010
		void	CurrentEyed(bool v)
		{
			m_currentEyed = v;
		}

		bool	CurrentEyed(void)
		{			
			return m_currentEyed;
		}

    private:
        bool            m_fVisible;
		//Xynth #100 7/2010 if static is it actively eyed by a scanner
		//for non-static this will always equal m_fVisible
		bool            m_currentEyed;
        IscannerIGC*    m_pLastSpotter;
};

class GameSite // this is going away soon
{
    public:
        static int MessageBox(const ZString& strText, const ZString& strCaption, UINT nType);
};

enum LoadoutChange
{
    c_lcMounted,
    c_lcDismounted,
    c_lcAdded,
    c_lcRemoved,
    c_lcHullChange,
    c_lcQuantityChange,
    c_lcAddPassenger,
    c_lcRemovePassenger,
    c_lcPassengerMove,
    c_lcTurretChange,
    c_lcDisembark,
    c_lcCargoSelectionChanged
};

enum BucketChange
{
    c_bcEmptied,        //Bucket money set to zero
    c_bcTerminated,     //Bucket eliminated (station capture or made redundant)
    c_bcMoneyChange,    //More money (less?) in the bucket
    c_bcPercentCompleteChange
};


class IIgcSite : public IObject
{
    public:
        virtual Time ServerTimeFromClientTime(Time   timeClient)
        {
            return timeClient;
        }
        virtual Time ClientTimeFromServerTime(Time   timeServer)
        {
            return timeServer;
        }

        virtual void    DevelopmentCompleted(IbucketIGC*  b, IdevelopmentIGC*   d, Time now) {}
        virtual void    StationTypeCompleted(IbucketIGC*  b, IstationIGC* pstation, IstationTypeIGC*   st, Time now) {}
        virtual void    HullTypeCompleted(IsideIGC* pside, IhullTypeIGC* pht) {}
        virtual void    PartTypeCompleted(IsideIGC* pside, IpartTypeIGC* ppt) {}
        virtual void    DroneTypeCompleted(IbucketIGC*  b, IstationIGC* pstation, IdroneTypeIGC*   dt, Time now) {}

        virtual void    BucketChangeEvent(BucketChange bc, IbucketIGC* b) {}
        virtual void    SideBuildingTechChange(IsideIGC* s) {}
        virtual void    SideDevelopmentTechChange(IsideIGC* s) {}
        virtual void    SideGlobalAttributeChange(IsideIGC* s) {}
        virtual void    StationTypeChange(IstationIGC* s) {}

        virtual void    ClusterUpdateEvent(IclusterIGC* c) {}

        virtual void    BuildStation(IasteroidIGC*      pasteroid,
                                         IsideIGC*          pside,
                                         IstationTypeIGC*   pstationtype,
										 Time               now,
										 bool pbseensides[]) { } //Imago #120 #121 8/10

        virtual TRef<ThingSite>      CreateThingSite(ImodelIGC* pModel){return new ThingSite;}
        virtual TRef<ClusterSite>    CreateClusterSite(IclusterIGC* pCluster){return new ClusterSite;}

        virtual void ReceiveChat(IshipIGC*      pshipSender,
                                 ChatTarget     ctRecipient,
                                 ObjectID       oidRecipient,
                                 SoundID        voiceOver,
                                 const char*    szText,
                                 CommandID      cid,
                                 ObjectType     otTarget,
                                 ObjectID       oidTarget,
                                 ImodelIGC*     pmodelTarget = NULL) {}
        virtual void SendChat(IshipIGC*         pshipSender,
                              ChatTarget        chatTarget,
                              ObjectID          oidRecipient,
                              SoundID           soVoiceOver,
                              const char*       szText,
                              CommandID         cid           = c_cidNone,
                              ObjectType        otTarget      = NA,
                              ObjectID          oidTarget     = NA,
                              ImodelIGC*        pmodelTarget  = NULL,
                              bool              bObjectModel  = false) {}

        void SendChatf(IshipIGC*   pshipSender,
                       ChatTarget  ctRecipient, ObjectID  oidRecipient,
                       SoundID     soVoiceOver, const char* pszText, ...)
        {
            const int c_cbBfr = 256;
            char szChatBfr[c_cbBfr];

            va_list vl;
            va_start(vl, pszText);
            _vsnprintf(szChatBfr, c_cbBfr, pszText, vl);
            va_end(vl);

            SendChat(pshipSender, ctRecipient, oidRecipient,
                     soVoiceOver, szChatBfr);
        }

        virtual void GetMoneyRequest(IshipIGC*  pshipSender, Money amount, HullID hidFor) {}

        virtual void PlaySoundEffect(SoundID soundID, ImodelIGC* model = NULL){}
        virtual void PlaySoundEffect(SoundID soundID, ImodelIGC* model, const Vector& vectOffset) {}
        virtual void PlayNotificationSound(SoundID soundID, ImodelIGC* model){}
        virtual void PlayFFEffect(ForceEffectID effectID, ImodelIGC* model = NULL, LONG lDirection = 0){}
        virtual void PlayVisualEffect(VisualEffectID effectID, ImodelIGC* model = NULL, float fIntensity = 1.0f){};
        virtual int  MessageBox(const ZString& strText, const ZString& strCaption, UINT nType){return 0;}
        virtual void TerminateModelEvent(ImodelIGC* model){}
        virtual void TerminateMissionEvent(ImissionIGC* pMission){}
        virtual void KillAsteroidEvent(IasteroidIGC* pasteroid, bool explodeF) {}
		virtual void KillAsteroidEvent(AsteroidID roid, SectorID soid, IsideIGC* side) {} //Imago #120 #121 8/10
        virtual void DrainAsteroidEvent(IasteroidIGC* pasteroid) {}
		virtual void MineAsteroidEvent(IasteroidIGC* pasteroid, float newOre) {}  //Xynth #132 7/2010
		virtual void KillProbeEvent(IprobeIGC* pprobe) {}
        virtual void KillMissileEvent(ImissileIGC* pmissile, const Vector& position) {}
        virtual void KillBuildingEffectEvent(IbuildingEffectIGC* pbe) {}
        virtual void KillMineEvent(ImineIGC* pmine) {}
        virtual void KillTreasureEvent(ItreasureIGC* ptreasure) { ptreasure->Terminate(); }
        virtual void PostText(bool bCritical, const char* pszText, ...) {}
        virtual void PostNotificationText(ImodelIGC* pmodel, bool bCritical, const char* pszText, ...) {}
        virtual void EjectPlayer(ImodelIGC* pmodelCredit) {}
        virtual void TargetKilled(ImodelIGC* pmodel) {}
        virtual void ShipWarped(IshipIGC* pship, SectorID sidOld, SectorID sidNew) {};
        virtual void DamageShipEvent(Time now, IshipIGC* ship, ImodelIGC* launcher, DamageTypeID type, float amount, float leakage, const Vector& p1, const Vector& p2){}
        virtual void KillShipEvent(Time now, IshipIGC* ship, ImodelIGC* launcher, float amount, const Vector& p1, const Vector& p2){}
        virtual void DamageStationEvent(IstationIGC* station, ImodelIGC* launcher, DamageTypeID type, float amount){}
        virtual void KillStationEvent(IstationIGC* station, ImodelIGC* launcher, float amount){}
        virtual bool DockWithStationEvent(IshipIGC* ship, IstationIGC* station){return true;}
        virtual void CaptureStationEvent(IshipIGC* ship, IstationIGC* station){}
        virtual bool LandOnCarrierEvent(IshipIGC* shipCarrier, IshipIGC* ship, float tCollision){return true;}
        virtual bool RescueShipEvent(IshipIGC* shipRescued, IshipIGC* shipRescuer){return true;}
        virtual void WarpBombEvent(IwarpIGC* pwap, ImissileIGC* pmissile) {}
        virtual void HitWarpEvent(IshipIGC* ship, IwarpIGC* warp){}
        virtual bool HitTreasureEvent(Time now, IshipIGC* ship, ItreasureIGC* treasure){ return true; }
        virtual void PaydayEvent(IsideIGC* pside, float money){}
        virtual void RequestRipcord(IshipIGC* ship, IclusterIGC* pcluster){}
        virtual bool ContinueRipcord(IshipIGC* ship, ImodelIGC* pmodel) { return true; }
        virtual bool UseRipcord(IshipIGC* ship, ImodelIGC* pmodel){ return false; }
        virtual void FireMissile(IshipIGC* pship, ImagazineIGC* pmagazine,
                                 Time timeFired, ImodelIGC* pTarget, float lock){}
        virtual void FireExpendable(IshipIGC* pship, IdispenserIGC* pdispenser,
                                    Time timeFired){}
        virtual void *GetDroneFromShip(IshipIGC * pship){return NULL;} // return value is really a Drone*
        virtual void CreateSideEvent(IsideIGC * pIsideIGC) {};
        virtual void DestroySideEvent(IsideIGC * pIsideIGC) {};

        virtual void ChangeStation(IshipIGC* pship,
                                   IstationIGC* pstationOld,
                                   IstationIGC* pstationNew)  {}    //changing clusters
        virtual void ChangeCluster(IshipIGC* pship,
                                   IclusterIGC* pclusterOld,
                                   IclusterIGC* pclusterNew)  {}    //changing clusters
        virtual void CommandChangedEvent(Command i, IshipIGC * pship, ImodelIGC* ptarget, CommandID cid) {};

        virtual void Preload(const char*    pszModelName,
                             const char*    pszFileName) {};

        virtual void ActivateTeleportProbe(IprobeIGC* pprobe) {};

        virtual void DestroyTeleportProbe(IprobeIGC* pprobe) {};

        virtual void LoadoutChangeEvent(IshipIGC* pship, IpartIGC* ppart, LoadoutChange lc) {}

        virtual IObject*    LoadRadarIcon(const char* szName)
        {
            return NULL;
        }

        virtual void        ChangeGameState(void) {}

        virtual void        CreateBuildingEffect(Time           now,
                                                 IasteroidIGC*  pasteroid,
                                                 IshipIGC*      pshipBuilder) {}

        virtual void        LayExpendable(Time                   now,
                                          IexpendableTypeIGC*    pet,
                                          IshipIGC*              pshipLayer) {}

        virtual IclusterIGC*    GetCluster(IshipIGC* pship,
                                           ImodelIGC*   pmodel)
        {
            return pship->CanSee(pmodel) ? pmodel->GetCluster() : NULL;
        }

        virtual IclusterIGC*    GetRipcordCluster(IshipIGC* pship, HullAbilityBitMask habm = c_habmIsRipcordTarget)
        {
            IhullTypeIGC*   pht = pship->GetBaseHullType();
            return (pht && pht->HasCapability(habm))
                   ? pship->GetCluster()
                   : NULL;
        }

        virtual bool        Reload(IshipIGC* pship, IlauncherIGC* plauncher, EquipmentType type) { return false; }
};

const DWORD c_dwFlashInterval = 250;            //flash for 1/4 second on receiving damage
const DWORD c_dwDamageSlotInterval = 5000;    //switch buckets every 5 seconds
const int   c_iDamageSlotCount = 8;             //and keep track of 8 damage buckets

class   DamageTrack
{
    public:
        ~DamageTrack(void);

        void    SwitchSlots(void);

        void    ApplyDamage(Time        timeNow,
                            ImodelIGC*  pmodel,
                            float       damage);

        void    Reset(void);

        void    AddDamageBucket(DamageBucket*   pdb)
        {
            assert (pdb);
            m_buckets.last(pdb);
        }

        void    DeleteDamageBucket(DamageBucket*   pdb)
        {
            assert (pdb);
            DamageBucketLink*   pdbl = m_buckets.find(pdb);
            assert (pdbl);
            delete pdbl;
        }

        DamageBucketList*    GetDamageBuckets(void)
        {
            return &m_buckets;
        }

    private:
        DamageTrack(DamageTrackSet* pdts);

        static void sort(DamageBucketList*  pListBuckets);

        DamageBucketList    m_buckets;
        DamageTrackSet*     m_pset;

        friend class DamageTrackSet;
};
typedef Slist_utl<DamageTrack*> DamageTrackList;
typedef Slink_utl<DamageTrack*> DamageTrackLink;

class   DamageBucket
{
    public:
        ~DamageBucket(void)
        {
            assert (m_model);
            m_model->DeleteDamageBucket(this);
            m_model->Release();

            assert (m_track);
            m_track->DeleteDamageBucket(this);
        }

        void    Initialize(Time timeNow, ImodelIGC*  pmodel)
        {
            assert (m_model == NULL);
            assert (pmodel);

            m_model = pmodel;
            pmodel->AddRef();

            m_timeLastDamage = timeNow;
        }

        ImodelIGC*   model(void) const
        {
            return m_model;
        }

        float       totalDamage(void) const
        {
            return m_totalDamage;
        }

        bool        flash(Time timeNow)
        {
            return (timeNow < Time(m_timeLastDamage.clock() + c_dwFlashInterval)) &&
                   (timeNow >= m_timeLastDamage);
        }

        void        SwitchSlots(int     idSlot)
        {
            assert (idSlot >= 0);
            assert (idSlot < c_iDamageSlotCount);

            m_totalDamage -= m_damage[idSlot];
            m_damage[idSlot] = 0.0f;
        }

        void        ApplyDamage(Time    timeNow,
                                int     idSlot,
                                float   d)
        {
            assert (idSlot >= 0);
            assert (idSlot < c_iDamageSlotCount);

            m_totalDamage += d;
            m_damage[idSlot] += d;

            //If we are not already in the bright phase of a flash
            if (timeNow >= Time(m_timeLastDamage.clock() + c_dwFlashInterval))
            {
                //Two cases ... we are taking damage for the first time in a while
                //or we're taking damage but in the dim phase of the previous cycle
                Time    timeReset = m_timeLastDamage.clock() + 2 * c_dwFlashInterval;
                m_timeLastDamage = (timeNow > timeReset) ? timeNow : timeReset;
            }
        }

    private:
        DamageBucket(DamageTrack*   ptrack,
                     ImodelIGC*     pmodel)
        :
            m_model(pmodel),
            m_track(ptrack),
            m_totalDamage(0.0f)
        {
            assert (pmodel);
            pmodel->AddRef();
            pmodel->AddDamageBucket(this);

            assert (ptrack);
            ptrack->AddDamageBucket(this);

            for (int i = 0; (i < c_iDamageSlotCount); i++)
                m_damage[i] = 0.0f;
        }

        ImodelIGC*      m_model;
        DamageTrack*    m_track;
        float           m_damage[c_iDamageSlotCount];
        float           m_totalDamage;
        Time            m_timeLastDamage;

    friend class DamageTrack;
};

class   DamageTrackSet
{
    public:
        void Initialize(Time now)
        {
            m_idSlot = 0;
            m_timeSlotChange = Time(now.clock() + c_dwDamageSlotInterval);
        }

        void Update(Time now)
        {
            if (now >= m_timeSlotChange)
            {
                m_idSlot = (m_idSlot + 1) % c_iDamageSlotCount;
                m_timeSlotChange = Time(m_timeSlotChange.clock() + c_dwDamageSlotInterval);

                for (DamageTrackLink*   pdtl = m_tracks.first(); (pdtl != NULL); pdtl = pdtl->next())
                    pdtl->data()->SwitchSlots();
            }
        }

        DamageTrack*    Create(void)
        {
            return new DamageTrack(this);
        }

    private:
        void            AddTrack(DamageTrack*   pdt)
        {
            m_tracks.last(pdt);
        }

        void            DeleteTrack(DamageTrack*    pdt)
        {
            DamageTrackLink*    pdtl = m_tracks.find(pdt);
            assert (pdtl);
            delete pdtl;
        }

        Time            m_timeSlotChange;
        int             m_idSlot;
        DamageTrackList m_tracks;

    friend class DamageTrack;
};

ImissionIGC*        CreateMission(void);

const float    c_fTimeIncrement = 0.25f;

enum AmmoState
{
    c_asEmpty,
    c_asLow,
    c_asFull
};
AmmoState   GetAmmoState(IshipIGC*  pship);

inline void        AddIbaseIGC(BaseListIGC*        list, IbaseIGC* base)
{
    assert (list);
    assert (base);

    ZVerify(list->last(base));
    base->AddRef();
}


inline void        DeleteIbaseIGC(BaseListIGC*     list, IbaseIGC* base)
{
	assert(list);
	assert(base);

	// BT - 9/17 - Debugging AllSrv crashes.
	if (list == nullptr)
	{
		debugf("ERROR: IGC::DeleteIbaseIGC() - list was null.\n");
		return;
	}

	if (base == nullptr)
	{
		debugf("ERROR: IGC::DeleteIbaseIGC() - base was null.\n");
		return;
	}
#ifndef __GNUC__
	__try
	{
#endif
		for (BaseLinkIGC*   l = list->first();
			(l != NULL);
			l = l->next())
		{
			if (l->data() == base)
			{
				delete l;               //remove it from the list
				base->Release();        //reduce the ref count
				break;                  //all done
			}
		}
#ifndef __GNUC__
    }
	__except (StackTracer::ExceptionFilter(GetExceptionInformation()))
	{
		StackTracer::OutputStackTraceToDebugF();
	}
#endif
}


inline IbaseIGC*   GetIbaseIGC(const BaseListIGC*  list, ObjectID    id)
{
    assert (list);

    for (BaseLinkIGC*   l = list->first();
         (l != NULL);
         l = l->next())
    {
        IbaseIGC*   b = l->data();

        if (b->GetObjectID() == id)
            return b;
    }

    return NULL;
}

bool    Dodge(IshipIGC* pship, ImodelIGC* pmodelIgnore, int* pstate, bool bShipsOnly = false, float dtDodge = -1.0f);
typedef int    GotoPositionMask;
class   Waypoint
{
    public:
        enum    Objective
        {
            c_oEnter,       //dock for starbases, warp for alephs
            c_oGoto,        //get to within m_fOffset and stop
            c_oNothing
        };

        Waypoint(void)
        :
            m_pmodelTarget(NULL),
            m_objective(c_oNothing)
        {
        }

        ~Waypoint(void)
        {
            if (m_pmodelTarget)
                m_pmodelTarget->Release();
        }

        void    Reset(void)
        {
            if (m_pmodelTarget)
            {
                m_pmodelTarget->Release();
                m_pmodelTarget = NULL;
            }

            m_objective = c_oNothing;
        }

        void    Set(Objective       o,
                    ImodelIGC*      pmodelTarget)
        {
            if(pmodelTarget)
            {

                assert ((o != c_oEnter) ||
                        (pmodelTarget->GetObjectType() == OT_ship) ||
                        (pmodelTarget->GetObjectType() == OT_station) ||
                        (pmodelTarget->GetObjectType() == OT_warp) ||
                        (pmodelTarget->GetObjectType() == OT_asteroid) ||
                        (pmodelTarget->GetObjectType() == OT_probe) ||
                        (pmodelTarget->GetObjectType() == OT_treasure));

                if (m_pmodelTarget)
                    m_pmodelTarget->Release();

                pmodelTarget->AddRef();
            }
            m_pmodelTarget = pmodelTarget;

            m_objective = o;
        }

    private:
        GotoPositionMask    DoApproach(IshipIGC*        pship,
                                       const Vector&    myPosition,
                                       const Vector&    itsPosition,
                                       int              nLand,
                                       const Vector*    pCenters,
                                       const Vector*    pDirections,
                                       float            distanceRest,
                                       const Vector&    positionRest,
                                       Vector*          pvectorGoto,
                                       ImodelIGC**      ppmodelSkip,
                                       Vector*          pvectorFacing);
        GotoPositionMask    GetGotoPosition(IshipIGC*           pship,
                                            float               speed,
                                            const Vector&       positionRest,
                                            Vector*             pvectorGoto,
                                            ImodelIGC**         ppmodelSkip,
                                            Vector*             pvectorFacing);

        ImodelIGC*  m_pmodelTarget;

        Objective   m_objective;

        friend class GotoPlan;
};

class   GotoPlan
{
    public:
        //Is this really the only way to do this?
        enum
        {
            c_wpTarget = 0x01,
            c_wpWarp   = 0x02
        };

        GotoPlan(IshipIGC*  pship,
                 float      fSkill = 0.5f)
        :
            m_pship(pship),
            m_fSkill(fSkill),
            m_maskWaypoints(0)
        {
            //Gotoplan is always a child of its ship, so no need to addref the ship pointer
            assert (pship);
        }

        ~GotoPlan(void)
        {
            assert (m_pship);
        }

        void    SetSkill(float fSkill)
        {
            m_fSkill = fSkill;
        }

        void    Reset(void)
        {
            m_wpTarget.Reset();
            m_wpWarp.Reset();

            m_maskWaypoints = 0;
        }

        void    Set(Waypoint::Objective o,
                    ImodelIGC*          pmodelTarget)
        {
            assert (pmodelTarget);

            m_wpTarget.Set(o, pmodelTarget);
            m_wpWarp.Reset();
            m_maskWaypoints = c_wpTarget;

            m_pvOldCluster = m_pship->GetCluster();
            m_pvOldClusterTarget = pmodelTarget->GetCluster();
        }

        int     GetMaskWaypoints(void) const
        {
            return m_maskWaypoints;
        }

        bool    Execute(Time    now, float   dt, bool    bDodge);

    private:
        bool    SetControls(float           dt,
                            bool            bDodge,
                            ControlData*    pcontrols,
                            int*            pstate);

        IshipIGC*   m_pship;
        float       m_fSkill;
        int         m_maskWaypoints;

        Waypoint    m_wpTarget;         //Primary target
        Waypoint    m_wpWarp;           //Warp we need to travel through to get there

        void*       m_pvOldCluster;                 //so we can tell when we change clusters
        void*       m_pvOldClusterTarget;           //ditto for the target
};

static  AssetMask   GetAssetMask(IshipIGC* pship, const IhullTypeIGC* pht, bool bFriendly)
{
    AssetMask   am;

    switch (pship->GetPilotType())
    {
        case c_ptLayer: //NYI do we need to differentiate from builders? for the momment, assume no
        case c_ptBuilder:
            am = bFriendly ? c_amBuilder : c_amEnemyBuilder;
        break;
        case c_ptMiner:
            am = bFriendly ? c_amMiner : c_amEnemyMiner;
        break;
        case c_ptCarrier:
            am = bFriendly ? c_amCarrier : c_amEnemyCarrier;
        break;
        default:
            if (bFriendly)
                am = pht->HasCapability(c_habmLifepod) ? 0 : c_amFighter;
            else
            {
                HullAbilityBitMask  habm = pht->GetCapabilities();

                if (habm & (c_habmBoard | c_habmCaptureThreat))
                {
                    am = (habm & c_habmFighter)
                         ? (c_amEnemyFighter | c_amEnemyAPC)
                         : (c_amEnemyCapital | c_amEnemyAPC);
                }
                else if (habm & c_habmThreatToStation)
                {
                    am = (habm & c_habmFighter)
                         ? (c_amEnemyFighter | c_amEnemyBomber)
                         : (c_amEnemyCapital | c_amEnemyBomber);
                }
                else
                    am = (habm & c_habmLifepod)
                         ? 0
                         : c_amEnemyFighter;

                if (habm & c_habmIsRipcordTarget)
                    am |= c_amEnemyTeleportShip;
            }
    }

    return am;
}

IshipIGC*   CreateDrone(ImissionIGC*     pmission,
                        ShipID           shipID,
                        PilotType        pt,
                        const char*      pszName,
                        HullID           hullID,
                        IsideIGC*        pside,
                        AbilityBitMask   abmOrders = 0,
                        float            shootSkill = 1.0f,
                        float            moveSkill  = 1.0f,
                        float            bravery    = 1.0f);


class PersistPlayerScoreObject
{
    public:
        PersistPlayerScoreObject(void)
        {
            Reset();
        }
        PersistPlayerScoreObject(
            float totalScore,
            float rating,
            RankID rank,
            CivID civID
            ) :
            m_totalScore(totalScore),
            m_fCombatRating(rating),
            m_rank(rank),
            m_civID(civID)
        {
        }
        void Set(float totalScore,
                 float rating,
                 RankID rank)
        {
            m_totalScore            = totalScore;
            m_fCombatRating         = rating;
            m_rank                  = rank;
        }

        void    Reset(void)
        {
            m_totalScore = 0.0f;

            m_fCombatRating = 0.0f;
            m_rank = 0;
            m_civID = NA;
        }

        short   GetRank(void) const
        {
            return m_rank;
        }

        float   GetScore(void) const
        {
            return m_totalScore;
        }


        void  SetCivID(CivID civID)
        {
          m_civID = civID;
        }

        CivID GetCivID() const
        {
          return m_civID;
        }

        float   GetCombatRating(void) const
        {
            return m_fCombatRating;
        }
        void    SetCombatRating(float cr)
        {
            m_fCombatRating = cr;
        }

    private:
        float               m_totalScore;
        float               m_fCombatRating;

        RankID              m_rank;

        CivID               m_civID;

        friend class PlayerScoreObject;
};

typedef Slist_utl<PersistPlayerScoreObject*> PersistPlayerScoreObjectList;
typedef Slink_utl<PersistPlayerScoreObject*> PersistPlayerScoreObjectLink;

class PlayerScoreObject
{
    public:
        PlayerScoreObject(bool  bPlayer)
        :
            m_bPlayer(bPlayer),
            m_bConnected(false),
            m_bCommanding(false),
            m_fCombatRating(0.0f)
        {
                Reset(true);
        }

        void    CalculateScore(ImissionIGC* pmission);

        void    SetScore(float fNewScore);

        void    SetCommanderScore(float fCommanderScore)
        {
            if (fCommanderScore > m_fScore)
                m_fScore = fCommanderScore;

            m_bCommandWin = m_bWin;
            m_bCommandLose = m_bLose;

            m_bCommandCredit = true;
        }
        float   GetScore(void) const;

        void    Reset(bool bFull)
        {
            m_cWarpsSpotted = 0;
            m_cAsteroidsSpotted = 0;

            m_cMinerKills = 0.0f;
            m_cBuilderKills = 0.0f;
            m_cLayerKills = 0.0f;
            m_cCarrierKills = 0.0f;
            m_cPlayerKills = 0.0f;
            m_cBaseKills = 0.0f;
            m_cBaseCaptures = 0.0f;
			m_cProbeSpot = 0;
			m_cRepair = 0;

            m_cRescues = 0;

            m_cTechsRecovered = 0;
            m_cFlags = 0;
            m_cArtifacts = 0;

            m_cKills = 0;
            m_cAssists = 0;
            if (bFull)
            {
                m_cDeaths = 0;
                m_cEjections = 0;
            }

            m_cPilotBaseKills = 0;
            m_cPilotBaseCaptures = 0;

            m_dtPlayed = 0.0f;
            m_dtCommanded = 0.0f;

            m_bWin = m_bLose = false;
            m_bCommandWin = m_bCommandLose = false;
            m_bCommandCredit = false;

            m_fScore = 0.0f;
			m_rankRatio = 1.0f;

            assert (!m_bConnected);
        }

		void SetRankRatio(float rankRatio)
		{
			m_rankRatio = rankRatio;
		}

        bool    Connected(void) const
        {
            return m_bConnected;
        }

        bool    Commanding(void) const
        {
            return m_bCommanding;
        }

        void    Connect(Time t)
        {
            assert (!m_bConnected);

            m_bConnected = true;

            m_timeConnect = t;
            if (m_bCommanding)
                m_timeStartCommand = t;
        }

        void    Disconnect(Time t)
        {
            if (m_bConnected)
            {
                m_bConnected = false;

                m_dtPlayed += (t - m_timeConnect);

                if (m_bCommanding)
                    m_dtCommanded += (t - m_timeStartCommand);
            }
        }

        void    StartCommand(Time t)
        {
            m_bCommanding = true;
            if (m_bConnected)
                m_timeStartCommand = t;
        }

        void    StopCommand(Time t)
        {
            if (m_bCommanding)
            {
                m_bCommanding = false;

                if (m_bConnected)
                    m_dtCommanded += (t - m_timeStartCommand);
            }
        }

        void    EndGame(ImissionIGC*    pmission,
                        bool            bWin,
                        bool            bLose)
        {
            assert (!m_bConnected);
            assert (!(bWin && bLose)); 

            m_bWin = bWin;
            m_bLose = bLose;

            CalculateScore(pmission);
        }

        void    SpotWarp(void)
        {
            m_cWarpsSpotted++;
        }

        void    SpotSpecialAsteroid(void)
        {
            m_cAsteroidsSpotted++;
        }

		void	AddProbeSpot(void)
		{
			m_cProbeSpot++;
		}
		void	SetRepair(float repair)
		{
			m_cRepair = repair;
		}
		
		float	GetRepair(void) const
		{			
			return m_cRepair;
		}


        void    KillShip(IshipIGC*      pship,
                         float          fraction)
        {
            switch (pship->GetPilotType())
            {
                case c_ptMiner:
                {
                    m_cMinerKills += fraction;
                }
                break;
                case c_ptBuilder:
                {
                    m_cBuilderKills += fraction;
                }
                break;
                case c_ptLayer:
                {
                    m_cLayerKills += fraction;
                }
                break;
                case c_ptCarrier:
                {
                    m_cCarrierKills += fraction;
                }
                break;
                default:
                {
                    m_cPlayerKills += fraction;
                }
                break;
            }
        }

        void    KillBase(bool bPilot)
        {
            if (bPilot)
            {
                m_cBaseKills += 1.0f;
                m_cPilotBaseKills++;
            }
            else
                m_cBaseKills += 0.5f;
        }
        void    CaptureBase(bool bPilot)
        {
            if (bPilot)
            {
                m_cBaseCaptures += 1.0f;
                m_cPilotBaseCaptures++;
            }
            else
                m_cBaseCaptures += 0.5f;
        }

        short   GetPilotBaseKills(void) const
        {
            return m_cPilotBaseKills;
        }
        short   GetPilotBaseCaptures(void) const
        {
            return m_cPilotBaseCaptures;
        }
        float   GetWarpsSpotted(void) const
        {
            return m_cWarpsSpotted;
        }
        float   GetAsteroidsSpotted(void) const
        {
            return m_cAsteroidsSpotted;
        }

        float   GetMinerKills(void) const
        {
            return m_cMinerKills;
        }
        float   GetBuilderKills(void) const
        {
            return m_cBuilderKills;
        }
        float   GetLayerKills(void) const
        {
            return m_cLayerKills;
        }
        float   GetCarrierKills(void) const
        {
            return m_cCarrierKills;
        }
        float   GetPlayerKills(void) const
        {
            return m_cPlayerKills;
        }
        float   GetBaseKills(void) const
        {
            return m_cBaseKills;
        }
        float   GetBaseCaptures(void) const
        {
            return m_cBaseCaptures;
        }
        short   GetTechsRecovered(void) const
        {
            return m_cTechsRecovered;
        }
        void    AddTechsRecovered(void)
        {
            m_cTechsRecovered++;
        }

        short   GetFlags(void) const
        {
            return m_cFlags;
        }

        void    AddFlag(void)
        {
            m_cFlags++;
        }

        short   GetArtifacts(void) const
        {
            return m_cArtifacts;
        }

        void    AddArtifact(void)
        {
            m_cArtifacts++;
        }

        void    AddRescue(void)
        {
            m_cRescues++;
        }

        short   GetRescues(void) const
        {
            return m_cRescues;
        }

        short   GetKills(void) const
        {
            return m_cKills;
        }
        void    AddKill(void)
        {
            m_cKills++;
        }

        short   GetAssists(void) const
        {
            return m_cAssists;
        }
        void    AddAssist(void)
        {
            m_cAssists++;
        }

        short   GetDeaths(void) const
        {
            return m_cDeaths;
        }
        void    AddDeath(void)
        {
            m_cDeaths++;
        }
        void    SetDeaths(short d)
        {
            m_cDeaths = d;
        }
        short   GetEjections(void) const
        {
            return m_cEjections;
        }
        void    AddEjection(void)
        {
            m_cEjections++;
        }
        void    SetEjections(short e)
        {
            m_cEjections = e;
        }

        float   GetCombatRating(void) const
        {
            return m_fCombatRating;
        }
        void    SetCombatRating(float cr)
        {
            m_fCombatRating = cr;
        }
        static void    AdjustCombatRating(ImissionIGC*          pmission,
                                          PlayerScoreObject*    ppsoKiller,
                                          PlayerScoreObject*    ppsoKillee);

        const PersistPlayerScoreObject& GetPersist(void) const
        {
            return m_persist;
        }
        PersistPlayerScoreObject& GetPersist(void)
        {
            return m_persist;
        }

        void                       SetPersist(const PersistPlayerScoreObject * p)
        {
            assert (p);

            m_persist = *p;
            m_fCombatRating = m_persist.GetCombatRating();
        }

        bool                        GetWinner(void) const
        {
            return m_bWin;
        }
        bool                        GetLoser(void) const
        {
            return m_bLose;
        }

        bool                        GetCommandWinner(void) const
        {
            return m_bCommandWin;
        }
        bool                        GetCommandLoser(void) const
        {
            return m_bCommandLose;
        }

        float GetTimePlayed() const
        {
            return m_dtPlayed;
        }

        float GetTimeCommanded() const
        {
            return m_dtCommanded;
        }

        bool    GetCommandCredit(void) const
        {
            return m_bCommandCredit;
        }

    private:
        Time                        m_timeConnect;
        Time                        m_timeStartCommand;

        short                       m_cWarpsSpotted;
        short                       m_cAsteroidsSpotted;
        float                       m_cMinerKills;
        float                       m_cBuilderKills;
        float                       m_cLayerKills;
        float                       m_cCarrierKills;
        float                       m_cPlayerKills;
        float                       m_cBaseKills;
        float                       m_cBaseCaptures;
		short						m_cProbeSpot;
		float						m_cRepair;

        short                       m_cTechsRecovered;
        short                       m_cFlags;
        short                       m_cArtifacts;

        short                       m_cRescues;

        short                       m_cKills;
        short                       m_cAssists;
        short                       m_cDeaths;
        short                       m_cEjections;
        short                       m_cPilotBaseKills;
        short                       m_cPilotBaseCaptures;

        float                       m_dtPlayed;
        float                       m_dtCommanded;

        float                       m_fCombatRating;

        float                       m_fScore;
		float						m_rankRatio;

        bool                        m_bPlayer;
        bool                        m_bConnected;
        bool                        m_bCommanding;

        bool                        m_bWin;
        bool                        m_bLose;
        bool                        m_bCommandWin;
        bool                        m_bCommandLose;
        bool                        m_bCommandCredit;

        PersistPlayerScoreObject    m_persist;
};

typedef Slist_utl<PlayerScoreObject*> PlayerScoreObjectList;
typedef Slink_utl<PlayerScoreObject*> PlayerScoreObjectLink;
class GameOverScoreObject
{
    public:
        GameOverScoreObject(void)
        {
        }

        void    Set(const PlayerScoreObject* ppso)
        {
            //m_totalScore            = ppso->GetScore();
            m_dtRecentPlayed        = ppso->GetTimePlayed();
            //m_fCombatRating         = ppso->GetPersist().GetCombatRating();
            m_cRecentBaseKills      = ppso->GetPilotBaseKills();
            m_cRecentBaseCaptures   = ppso->GetPilotBaseCaptures();
            m_cRecentKills          = ppso->GetKills();
            m_cRecentDeaths         = ppso->GetDeaths();
            m_cRecentEjections      = ppso->GetEjections();
            //m_cTotalBaseKills       = ppso->GetPersist().GetBaseKills();
            //m_cTotalBaseCaptures    = ppso->GetPersist().GetBaseCaptures();
            //m_cTotalKills           = ppso->GetPersist().GetKills();
            //m_cTotalDeaths          = ppso->GetPersist().GetDeaths();
            //m_cTotalEjections       = ppso->GetPersist().GetEjections();
            m_rank                  = ppso->GetPersist().GetRank();
            //m_cTotalWins            = ppso->GetPersist().GetWins();
            //m_cTotalLosses          = ppso->GetPersist().GetLosses();
            m_cRecentScore          = ppso->GetScore();
            m_cRecentAssists        = ppso->GetAssists();
            //m_bWinner               = ppso->GetWinner();
            //m_bLoser                = ppso->GetLoser();
            //m_cFlags                = ppso->GetFlags();
            //m_cArtifacts            = ppso->GetArtifacts();
            //m_cRescues              = ppso->GetRescues();
            m_bCommander            = ppso->GetCommandCredit();
            m_bGameCounted          = ppso->GetLoser() || ppso->GetWinner();
        }

        short   GetRank(void) const
        {
            return m_rank;
        }
        /*
        float   GetScore(void) const
        {
            return m_totalScore;
        }
        */
        float   GetRecentTimePlayed(void) const
        {
            return m_dtRecentPlayed;
        }

        short   GetRecentBaseKills(void) const
        {
            return m_cRecentBaseKills;
        }

        short   GetRecentBaseCaptures(void) const
        {
            return m_cRecentBaseCaptures;
        }

        short   GetRecentKills(void) const
        {
            return m_cRecentKills;
        }

        short   GetRecentDeaths(void) const
        {
            return m_cRecentDeaths;
        }

        short   GetRecentEjections(void) const
        {
            return m_cRecentEjections;
        }
        /*
        short   GetTotalBaseKills(void) const
        {
            return m_cTotalBaseKills;
        }

        short   GetTotalBaseCaptures(void) const
        {
            return m_cTotalBaseCaptures;
        }

        short   GetFlags(void) const
        {
            return m_cFlags;
        }

        short   GetArtifacts(void) const
        {
            return m_cArtifacts;
        }

        short   GetRescues(void) const
        {
            return m_cRescues;
        }

        short   GetTotalKills(void) const
        {
            return m_cTotalKills;
        }

        short   GetTotalDeaths(void) const
        {
            return m_cTotalDeaths;
        }

        short   GetTotalEjections(void) const
        {
            return m_cTotalEjections;
        }
        */
        /*
        short   GetWins(void) const
        {
            return m_cTotalWins;
        }

        short   GetLosses(void) const
        {
            return m_cTotalLosses;
        }
        */
        float   GetRecentScore(void) const
        {
            return m_cRecentScore;
        }

        unsigned short   GetRecentAssists(void) const
        {
            return m_cRecentAssists;
        }
        /*
        float   GetCombatRating(void) const
        {
            return m_fCombatRating;
        }

        bool    GetWinner(void) const
        {
            return m_bWinner;
        }

        bool    GetLoser(void) const
        {
            return m_bLoser;
        }
        */

        bool        GetCommander(void) const
        {
            return m_bCommander;
        }

        bool        GetGameCounted(void) const
        {
            return m_bGameCounted;
        }

    private:
        //float               m_totalScore;
        float               m_dtRecentPlayed;
        //float               m_fCombatRating;
        float               m_cRecentScore;

        //short               m_cFlags;
        //short               m_cArtifacts;
        //short               m_cRescues;

        short               m_cRecentBaseKills;
        short               m_cRecentBaseCaptures;
        short               m_cRecentKills;
        short               m_cRecentDeaths;
        short               m_cRecentEjections;
        unsigned short      m_cRecentAssists;

        //short               m_cTotalBaseKills;
        //short               m_cTotalBaseCaptures;
        //short               m_cTotalKills;

        //short               m_cTotalDeaths;
        //short               m_cTotalEjections;

        short               m_rank;

        bool                m_bCommander;
        bool                m_bGameCounted;

        //unsigned short      m_cTotalWins;
        //unsigned short      m_cTotalLosses;

        //bool                m_bWinner;
        //bool                m_bLoser;
};


//------------------------------------------------------------------------------
// normal igc files, i.e. missions can be dumped and loaded using these two
// functions. They return true if successful.
//------------------------------------------------------------------------------
bool    DumpIGCFile (const char* name, ImissionIGC* pMission, int64_t iMaskExportTypes,
                     void (*munge)(int size, char* data) = NULL);
bool    LoadIGCFile (const char* name, ImissionIGC* pMission, void (*munge)(int size, char* data) = NULL);

//------------------------------------------------------------------------------
// static data core files are dealt with by these functions. They are
// almost identical to the normal igc file loaders, but there is a version
// number in the file, and it is returned by the LoadIGCStaticCore function.
// if the load function fails, it returns NA.
//------------------------------------------------------------------------------
bool    DumpIGCStaticCore (const char* name, ImissionIGC* pMission, int64_t iMaskExportTypes, void (*munge)(int size, char* data) = NULL);
int     LoadIGCStaticCore (const char* name, ImissionIGC* pMission, bool fGetVersionOnly, void (*munge)(int size, char* data) = NULL);
int     CacheIGCStaticCore (const char* name, ImissionIGC* pMission, bool fGetVersionOnly, void (*munge)(int size, char* data) = NULL);

#endif
