#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Forward declaration
//
//////////////////////////////////////////////////////////////////////////////

TRef<IPopup> CreateInputMapPopup(
    Modeler*       pmodeler,
    InputEngine*   pinputEngine,
    TrekInput*     ptrekInput,
    IEngineFont*   pfont,
    Number*        ptime
);

//////////////////////////////////////////////////////////////////////////////
//
// Trek Input
//
//////////////////////////////////////////////////////////////////////////////

const int countAxis       = 6;
const int keyMax          = 256;
const int ModifierNone    = 0;
const int ModifierShift   = 1;
const int ModifierControl = 2;
const int ModifierAlt     = 4;
const int ModifierAny     = 8;

class TrekInputImpl : public TrekInput {
public:
    //
    // Input Engine
    //

    TRef<InputEngine>                       m_pinputEngine;
    TRef<JoystickImage>                     m_pjoystickImage;
    TArray<TRef<Number>, countAxis>         m_ppnumberAxis;
    TRef<TrekInputSite>                     m_psite;

    //
    // Joystick mapping
    //

    TArray<TRef<Boolean>, TK_Max>           m_ppboolTrekKeyButtonDown;
    TArray<bool         , TK_Max>           m_boolTrekKeyButtonDown;

    TArray<TRef<ModifiableBoolean>, 4>      m_ppboolHatButton;

    // Mouse mapping Imago 8/14/09 (special buttons / mouse wheel)
    TrekKey                                 m_wheeldownTK;
    TrekKey                                 m_wheelupTK;
    TrekKey                                 m_wheelclickTK;
    TrekKey                                 m_xbutton1TK;
    TrekKey                                 m_xbutton2TK;
    TrekKey                                 m_xbutton3TK;
    TrekKey                                 m_xbutton4TK;
    TrekKey                                 m_xbutton5TK;

    //
    // Keyboard mapping
    //

    bool                                    m_bFocus;

    TArray<TrekKey, keyMax>                 m_KeyMap;
    TArray<TrekKey, keyMax>                 m_ShiftKeyMap;
    TArray<TrekKey, keyMax>                 m_ControlKeyMap;
    TArray<TrekKey, keyMax>                 m_AltKeyMap;
    TArray<TrekKey, keyMax>                 m_ControlShiftKeyMap;
    TArray<TrekKey, keyMax>                 m_ShiftAltKeyMap;
    TArray<TrekKey, keyMax>                 m_ControlAltKeyMap;
    TArray<TrekKey, keyMax>                 m_ControlShiftAltKeyMap;

    TArray<TRef<ModifiableBoolean>, keyMax> m_pboolKeyDown;
    TArray<TRef<Boolean>,           TK_Max> m_pboolTrekKeyDown;
    TArray<bool         ,           TK_Max> m_boolMouseTrekKeyDown;	//Turkey added 8/10 #56

    TRef<Boolean>                           m_pboolNone;
    TRef<Boolean>                           m_pboolJustShift;
    TRef<Boolean>                           m_pboolJustControl;
    TRef<Boolean>                           m_pboolJustAlt;
    TRef<Boolean>                           m_pboolJustControlShift;
    TRef<Boolean>                           m_pboolJustShiftAlt;
    TRef<Boolean>                           m_pboolJustControlAlt;
    TRef<Boolean>                           m_pboolJustControlShiftAlt;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    TrekInputImpl(
        HINSTANCE    hInst,
        HWND         hWnd,
        InputEngine* pinputEngine,
        JoystickImage* pjoystickImage
    ) :
        m_pinputEngine(pinputEngine),
        m_pjoystickImage(pjoystickImage)

	{
        InitializeCommandsMDL();

        //
        // Create the virtual hat buttons
        //

        for (int index = 0; index < 4; index++) {
            m_ppboolHatButton[index] = new ModifiableBoolean(false);
        }

        //
        // All the button states start up
        //

        ClearButtonStates();

        //
        // Initialize input mapping
        //

        if (!LoadMap(INPUTMAP_FILE)) {
            LoadMap(DEFAULTINPUTMAP_FILE);
        }

        Flush();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void Flush(void)
    {
        //
        // Do things this way because SetKeyboardState() doesn't appear
        // to work on Win'95...
        //

        for (int x = 0; x < 256; x++) {
            GetKeyState(x);
        }
    }

    int normalize (int val, int minval, int maxval);

    void InitializeCommandsMDL()
    {
        //
        // Trek Key definitions
        //

        TRef<INameSpace> pnsCommands =
            GetModeler()->CreateNameSpace(
                "commands",
                GetModeler()->GetNameSpace("model")
            );

        pnsCommands->AddMember("CommandHelp"                     , new Number((float)TK_Help                     ));
		pnsCommands->AddMember("CommandScreenShot"               , new Number((float)TK_ScrnShot                 ));
        pnsCommands->AddMember("CommandNextCommMsg"              , new Number((float)TK_NextCommMsg              ));
        pnsCommands->AddMember("CommandPrevCommMsg"              , new Number((float)TK_PrevCommMsg              ));
        pnsCommands->AddMember("CommandPrevWeapon"               , new Number((float)TK_PrevWeapon               ));
        pnsCommands->AddMember("CommandNextWeapon"               , new Number((float)TK_NextWeapon               ));
        pnsCommands->AddMember("CommandToggleGroupFire"          , new Number((float)TK_ToggleGroupFire          ));
        pnsCommands->AddMember("CommandPrevPart"                 , new Number((float)TK_PrevPart                 ));
        pnsCommands->AddMember("CommandNextPart"                 , new Number((float)TK_NextPart                 ));
        pnsCommands->AddMember("CommandMountPart"                , new Number((float)TK_MountPart                ));
        pnsCommands->AddMember("CommandReplacePart"              , new Number((float)TK_ReplacePart              ));
        pnsCommands->AddMember("CommandUnmountPart"              , new Number((float)TK_UnmountPart              ));
        pnsCommands->AddMember("CommandDropPart"                 , new Number((float)TK_DropPart                 ));
        pnsCommands->AddMember("CommandZoomOut"                  , new Number((float)TK_ZoomOut                  ));
        pnsCommands->AddMember("CommandZoomIn"                   , new Number((float)TK_ZoomIn                   ));
        pnsCommands->AddMember("CommandPitchUp"                  , new Number((float)TK_PitchUp                  ));
        pnsCommands->AddMember("CommandPitchDown"                , new Number((float)TK_PitchDown                ));
        pnsCommands->AddMember("CommandYawLeft"                  , new Number((float)TK_YawLeft                  ));
        pnsCommands->AddMember("CommandYawRight"                 , new Number((float)TK_YawRight                 ));
        pnsCommands->AddMember("CommandRollModifier"             , new Number((float)TK_RollModifier             ));
        pnsCommands->AddMember("CommandRollLeft"                 , new Number((float)TK_RollLeft                 ));
        pnsCommands->AddMember("CommandRollRight"                , new Number((float)TK_RollRight                ));
        pnsCommands->AddMember("CommandMainMenu"                 , new Number((float)TK_MainMenu                 ));
        pnsCommands->AddMember("CommandViewFrontLeft"            , new Number((float)TK_ViewFrontLeft            ));
        pnsCommands->AddMember("CommandViewFrontRight"           , new Number((float)TK_ViewFrontRight           ));
        pnsCommands->AddMember("CommandViewLeft"                 , new Number((float)TK_ViewLeft                 ));
        pnsCommands->AddMember("CommandViewRight"                , new Number((float)TK_ViewRight                ));
        pnsCommands->AddMember("CommandViewExternal"             , new Number((float)TK_ViewExternal             ));
        pnsCommands->AddMember("CommandViewFlyby"                , new Number((float)TK_ViewFlyby                ));
        pnsCommands->AddMember("CommandViewMissile"              , new Number((float)TK_ViewMissile              ));
        pnsCommands->AddMember("CommandViewTarget"               , new Number((float)TK_ViewTarget               ));
        pnsCommands->AddMember("CommandViewChase"                , new Number((float)TK_ViewChase                ));
//        pnsCommands->AddMember("CommandViewTurret"               , new Number((float)TK_ViewTurret               ));
        pnsCommands->AddMember("CommandViewBase"                 , new Number((float)TK_ViewBase                 ));
        pnsCommands->AddMember("CommandViewSector"               , new Number((float)TK_ViewSector               ));
        pnsCommands->AddMember("CommandViewCommand"              , new Number((float)TK_ViewCommand              ));
        pnsCommands->AddMember("CommandCycleChatMsgs"            , new Number((float)TK_CycleChatMsgs            ));
        pnsCommands->AddMember("CommandExecuteCommand"           , new Number((float)TK_ExecuteCommand           ));
        pnsCommands->AddMember("CommandAcceptCommand"            , new Number((float)TK_AcceptCommand            ));
        pnsCommands->AddMember("CommandRejectCommand"            , new Number((float)TK_RejectCommand            ));
        pnsCommands->AddMember("CommandClearCommand"             , new Number((float)TK_ClearCommand             ));
        pnsCommands->AddMember("CommandToggleAutoPilot"          , new Number((float)TK_ToggleAutoPilot          ));
        pnsCommands->AddMember("CommandThrustLeft"               , new Number((float)TK_ThrustLeft               ));
        pnsCommands->AddMember("CommandThrustRight"              , new Number((float)TK_ThrustRight              ));
        pnsCommands->AddMember("CommandThrustUp"                 , new Number((float)TK_ThrustUp                 ));
        pnsCommands->AddMember("CommandThrustDown"               , new Number((float)TK_ThrustDown               ));
        pnsCommands->AddMember("CommandThrustForward"            , new Number((float)TK_ThrustForward            ));
        pnsCommands->AddMember("CommandThrustBackward"           , new Number((float)TK_ThrustBackward           ));
        pnsCommands->AddMember("CommandFireBooster"              , new Number((float)TK_FireBooster              ));
        pnsCommands->AddMember("CommandFireWeapon"               , new Number((float)TK_FireWeapon               ));
        pnsCommands->AddMember("CommandFireMissile"              , new Number((float)TK_FireMissile              ));
        pnsCommands->AddMember("CommandLockVector"               , new Number((float)TK_LockVector               ));
        pnsCommands->AddMember("CommandThrottleUp"               , new Number((float)TK_ThrottleUp               ));
        pnsCommands->AddMember("CommandThrottleDown"             , new Number((float)TK_ThrottleDown             ));
        pnsCommands->AddMember("CommandThrottleZero"             , new Number((float)TK_ThrottleZero             ));
        pnsCommands->AddMember("CommandThrottle33"               , new Number((float)TK_Throttle33               ));
        pnsCommands->AddMember("CommandThrottle66"               , new Number((float)TK_Throttle66               ));
        pnsCommands->AddMember("CommandThrottleFull"             , new Number((float)TK_ThrottleFull             ));
        pnsCommands->AddMember("CommandToggleBooster"            , new Number((float)TK_ToggleBooster            ));
        pnsCommands->AddMember("CommandLODUp"                    , new Number((float)TK_LODUp                    ));
        pnsCommands->AddMember("CommandLODDown"                  , new Number((float)TK_LODDown                  ));
        pnsCommands->AddMember("CommandConModeCombat"            , new Number((float)TK_ConModeCombat            ));
        pnsCommands->AddMember("CommandConModeCommand"           , new Number((float)TK_ConModeCommand           ));
        pnsCommands->AddMember("CommandConModeNav"               , new Number((float)TK_ConModeNav               ));
        pnsCommands->AddMember("CommandConModeInventory"         , new Number((float)TK_ConModeInventory         ));
        pnsCommands->AddMember("CommandConModeInvest"            , new Number((float)TK_ConModeInvest            ));
        pnsCommands->AddMember("CommandConModeTeam"              , new Number((float)TK_ConModeTeam              ));
        pnsCommands->AddMember("CommandConModeGameState"         , new Number((float)TK_ConModeGameState         ));
        pnsCommands->AddMember("CommandViewRear"                 , new Number((float)TK_ViewRear                 ));
        pnsCommands->AddMember("CommandTarget"                   , new Number((float)TK_Target                   ));
        pnsCommands->AddMember("CommandTargetNearest"            , new Number((float)TK_TargetNearest            ));
        pnsCommands->AddMember("CommandTargetPrev"               , new Number((float)TK_TargetPrev               ));
        pnsCommands->AddMember("CommandTargetFriendly"           , new Number((float)TK_TargetFriendly           ));
        pnsCommands->AddMember("CommandTargetFriendlyNearest"    , new Number((float)TK_TargetFriendlyNearest    ));
        pnsCommands->AddMember("CommandTargetFriendlyPrev"       , new Number((float)TK_TargetFriendlyPrev       ));
        pnsCommands->AddMember("CommandTargetEnemy"              , new Number((float)TK_TargetEnemy              ));
        pnsCommands->AddMember("CommandTargetEnemyNearest"       , new Number((float)TK_TargetEnemyNearest       ));
        pnsCommands->AddMember("CommandTargetEnemyPrev"          , new Number((float)TK_TargetEnemyPrev          ));
        pnsCommands->AddMember("CommandTargetEnemyBase"          , new Number((float)TK_TargetEnemyBase          ));
        pnsCommands->AddMember("CommandTargetEnemyBaseNearest"   , new Number((float)TK_TargetEnemyBaseNearest   ));
        pnsCommands->AddMember("CommandTargetEnemyBasePrev"      , new Number((float)TK_TargetEnemyBasePrev      ));
        pnsCommands->AddMember("CommandTargetFriendlyBase"       , new Number((float)TK_TargetFriendlyBase       ));
        pnsCommands->AddMember("CommandTargetFriendlyBaseNearest", new Number((float)TK_TargetFriendlyBaseNearest)); 
        pnsCommands->AddMember("CommandTargetFriendlyBasePrev"   , new Number((float)TK_TargetFriendlyBasePrev   ));
        pnsCommands->AddMember("CommandTargetAlliedBase"         , new Number((float)TK_TargetAlliedBase         )); //imago 8/1/09
        pnsCommands->AddMember("CommandTargetAlliedBaseNearest"  , new Number((float)TK_TargetAlliedBaseNearest  )); 
        pnsCommands->AddMember("CommandTargetAlliedBasePrev"     , new Number((float)TK_TargetAlliedBasePrev     ));
        pnsCommands->AddMember("CommandTargetCommand"            , new Number((float)TK_TargetCommand            ));
        pnsCommands->AddMember("CommandTargetCenter"             , new Number((float)TK_TargetCenter             ));
        pnsCommands->AddMember("CommandTargetHostile"            , new Number((float)TK_TargetHostile            ));
        pnsCommands->AddMember("CommandSuicide"                  , new Number((float)TK_Suicide                  ));
        pnsCommands->AddMember("CommandToggleGrid"               , new Number((float)TK_ToggleGrid               ));
        pnsCommands->AddMember("CommandToggleCloak"              , new Number((float)TK_ToggleCloak              ));
        pnsCommands->AddMember("CommandDropMine"                 , new Number((float)TK_DropMine                 ));
        pnsCommands->AddMember("CommandRipcord"                  , new Number((float)TK_Ripcord                  ));
        pnsCommands->AddMember("CommandViewRearLeft"             , new Number((float)TK_ViewRearLeft             ));
        pnsCommands->AddMember("CommandViewRearRight"            , new Number((float)TK_ViewRearRight            ));
        pnsCommands->AddMember("CommandTargetSelf"               , new Number((float)TK_TargetSelf               ));
        pnsCommands->AddMember("CommandToggleCollisions"         , new Number((float)TK_ToggleCollisions         ));
        pnsCommands->AddMember("CommandOccupyNextTurret"         , new Number((float)TK_OccupyNextTurret         ));
        pnsCommands->AddMember("CommandTargetNothing"            , new Number((float)TK_TargetNothing            ));
        pnsCommands->AddMember("CommandMatchSpeed"               , new Number((float)TK_MatchSpeed               ));
        pnsCommands->AddMember("CommandChatPageUp"               , new Number((float)TK_ChatPageUp               ));
        pnsCommands->AddMember("CommandChatPageDown"             , new Number((float)TK_ChatPageDown             ));
        pnsCommands->AddMember("CommandViewExternalOrbit"        , new Number((float)TK_ViewExternalOrbit        ));
        pnsCommands->AddMember("CommandViewExternalStation"      , new Number((float)TK_ViewExternalStation      ));
        pnsCommands->AddMember("CommandDropChaff"                , new Number((float)TK_DropChaff                ));
        pnsCommands->AddMember("CommandToggleCommand"            , new Number((float)TK_ToggleCommand            ));
        pnsCommands->AddMember("CommandSwapWeapon1"              , new Number((float)TK_SwapWeapon1              ));
        pnsCommands->AddMember("CommandSwapWeapon2"              , new Number((float)TK_SwapWeapon2              ));
        pnsCommands->AddMember("CommandSwapWeapon3"              , new Number((float)TK_SwapWeapon3              ));
        pnsCommands->AddMember("CommandSwapWeapon4"              , new Number((float)TK_SwapWeapon4              ));
        pnsCommands->AddMember("CommandSwapTurret1"              , new Number((float)TK_SwapTurret1              ));
        pnsCommands->AddMember("CommandSwapTurret2"              , new Number((float)TK_SwapTurret2              ));
        pnsCommands->AddMember("CommandSwapTurret3"              , new Number((float)TK_SwapTurret3              ));
        pnsCommands->AddMember("CommandSwapTurret4"              , new Number((float)TK_SwapTurret4              ));
        pnsCommands->AddMember("CommandPromoteTurret1"           , new Number((float)TK_PromoteTurret1           ));
        pnsCommands->AddMember("CommandPromoteTurret2"           , new Number((float)TK_PromoteTurret2           ));
        pnsCommands->AddMember("CommandPromoteTurret3"           , new Number((float)TK_PromoteTurret3           ));
        pnsCommands->AddMember("CommandPromoteTurret4"           , new Number((float)TK_PromoteTurret4           ));
        pnsCommands->AddMember("CommandSwapMissile"              , new Number((float)TK_SwapMissile              ));
        pnsCommands->AddMember("CommandSwapChaff"                , new Number((float)TK_SwapChaff                ));
        pnsCommands->AddMember("CommandSwapShield"               , new Number((float)TK_SwapShield               ));
        pnsCommands->AddMember("CommandSwapCloak"                , new Number((float)TK_SwapCloak                ));
        pnsCommands->AddMember("CommandSwapAfterburner"          , new Number((float)TK_SwapAfterburner          ));
        pnsCommands->AddMember("CommandSwapMine"                 , new Number((float)TK_SwapMine                 ));
        pnsCommands->AddMember("CommandNextCargo"                , new Number((float)TK_NextCargo                ));
        pnsCommands->AddMember("CommandToggleWeapon1"            , new Number((float)TK_ToggleWeapon1            ));
        pnsCommands->AddMember("CommandToggleWeapon2"            , new Number((float)TK_ToggleWeapon2            ));
        pnsCommands->AddMember("CommandToggleWeapon3"            , new Number((float)TK_ToggleWeapon3            ));
        pnsCommands->AddMember("CommandToggleWeapon4"            , new Number((float)TK_ToggleWeapon4            ));
        pnsCommands->AddMember("CommandDropCargo"                , new Number((float)TK_DropCargo                ));
        pnsCommands->AddMember("CommandQuickChatMenu"            , new Number((float)TK_QuickChatMenu            ));
        pnsCommands->AddMember("CommandToggleLODSlider"          , new Number((float)TK_ToggleLODSlider          ));
        pnsCommands->AddMember("CommandStartChat"                , new Number((float)TK_StartChat                ));
        pnsCommands->AddMember("CommandReload"                   , new Number((float)TK_Reload                   ));
        pnsCommands->AddMember("CommandCycleRadar"               , new Number((float)TK_CycleRadar               ));
        pnsCommands->AddMember("CommandConModeMiniTeam"          , new Number((float)TK_ConModeMiniTeam          ));
        pnsCommands->AddMember("CommandConModeTeleport"          , new Number((float)TK_ConModeTeleport          ));
        pnsCommands->AddMember("CommandTargetEnemyBomber"        , new Number((float)TK_TargetEnemyBomber        ));
        pnsCommands->AddMember("CommandTargetEnemyFighter"       , new Number((float)TK_TargetEnemyFighter       ));
        pnsCommands->AddMember("CommandTargetEnemyConstructor"   , new Number((float)TK_TargetEnemyConstructor   ));
        pnsCommands->AddMember("CommandTargetEnemyMiner"         , new Number((float)TK_TargetEnemyMiner         ));
        pnsCommands->AddMember("CommandTargetEnemyMajorBase"     , new Number((float)TK_TargetEnemyMajorBase     ));
        pnsCommands->AddMember("CommandTargetFriendlyBomber"     , new Number((float)TK_TargetFriendlyBomber     ));
        pnsCommands->AddMember("CommandTargetFriendlyFighter"    , new Number((float)TK_TargetFriendlyFighter    ));
        pnsCommands->AddMember("CommandTargetFriendlyConstructor", new Number((float)TK_TargetFriendlyConstructor));
        pnsCommands->AddMember("CommandTargetFriendlyMiner"      , new Number((float)TK_TargetFriendlyMiner      ));
        pnsCommands->AddMember("CommandTargetFriendlyLifepod"    , new Number((float)TK_TargetFriendlyLifepod    ));
        pnsCommands->AddMember("CommandTargetFriendlyMajorBase"  , new Number((float)TK_TargetFriendlyMajorBase  ));
        pnsCommands->AddMember("CommandTargetFlag"               , new Number((float)TK_TargetFlag               ));
        pnsCommands->AddMember("CommandTargetArtifact"           , new Number((float)TK_TargetArtifact           ));
        pnsCommands->AddMember("CommandTargetEnemyBomberNearest"        , new Number((float)TK_TargetEnemyBomberNearest        ));
        pnsCommands->AddMember("CommandTargetEnemyFighterNearest"       , new Number((float)TK_TargetEnemyFighterNearest       ));
        pnsCommands->AddMember("CommandTargetEnemyConstructorNearest"   , new Number((float)TK_TargetEnemyConstructorNearest   ));
        pnsCommands->AddMember("CommandTargetEnemyMinerNearest"         , new Number((float)TK_TargetEnemyMinerNearest         ));
        pnsCommands->AddMember("CommandTargetEnemyMajorBaseNearest"     , new Number((float)TK_TargetEnemyMajorBaseNearest     ));
        pnsCommands->AddMember("CommandTargetFriendlyBomberNearest"     , new Number((float)TK_TargetFriendlyBomberNearest     ));
        pnsCommands->AddMember("CommandTargetFriendlyFighterNearest"    , new Number((float)TK_TargetFriendlyFighterNearest    ));
        pnsCommands->AddMember("CommandTargetFriendlyConstructorNearest", new Number((float)TK_TargetFriendlyConstructorNearest));
        pnsCommands->AddMember("CommandTargetFriendlyMinerNearest"      , new Number((float)TK_TargetFriendlyMinerNearest      ));
        pnsCommands->AddMember("CommandTargetFriendlyLifepodNearest"    , new Number((float)TK_TargetFriendlyLifepodNearest    ));
        pnsCommands->AddMember("CommandTargetFriendlyMajorBaseNearest"  , new Number((float)TK_TargetFriendlyMajorBaseNearest  ));
        pnsCommands->AddMember("CommandTargetFlagNearest"               , new Number((float)TK_TargetFlagNearest               ));
        pnsCommands->AddMember("CommandTargetArtifactNearest"           , new Number((float)TK_TargetArtifactNearest           ));
        pnsCommands->AddMember("CommandTargetEnemyBomberPrev"        , new Number((float)TK_TargetEnemyBomberPrev        ));
        pnsCommands->AddMember("CommandTargetEnemyFighterPrev"       , new Number((float)TK_TargetEnemyFighterPrev       ));
        pnsCommands->AddMember("CommandTargetEnemyConstructorPrev"   , new Number((float)TK_TargetEnemyConstructorPrev   ));
        pnsCommands->AddMember("CommandTargetEnemyMinerPrev"         , new Number((float)TK_TargetEnemyMinerPrev         ));
        pnsCommands->AddMember("CommandTargetEnemyMajorBasePrev"     , new Number((float)TK_TargetEnemyMajorBasePrev     ));
        pnsCommands->AddMember("CommandTargetFriendlyBomberPrev"     , new Number((float)TK_TargetFriendlyBomberPrev     ));
        pnsCommands->AddMember("CommandTargetFriendlyFighterPrev"    , new Number((float)TK_TargetFriendlyFighterPrev    ));
        pnsCommands->AddMember("CommandTargetFriendlyConstructorPrev", new Number((float)TK_TargetFriendlyConstructorPrev));
        pnsCommands->AddMember("CommandTargetFriendlyMinerPrev"      , new Number((float)TK_TargetFriendlyMinerPrev      ));
        pnsCommands->AddMember("CommandTargetFriendlyLifepodPrev"    , new Number((float)TK_TargetFriendlyLifepodPrev    ));
        pnsCommands->AddMember("CommandTargetFriendlyMajorBasePrev"  , new Number((float)TK_TargetFriendlyMajorBasePrev  ));
        pnsCommands->AddMember("CommandTargetFlagPrev"               , new Number((float)TK_TargetFlagPrev               ));
        pnsCommands->AddMember("CommandTargetArtifactPrev"           , new Number((float)TK_TargetArtifactPrev           ));
        pnsCommands->AddMember("CommandTargetWarp"                   , new Number((float)TK_TargetWarp                   ));
        pnsCommands->AddMember("CommandTargetWarpNearest"            , new Number((float)TK_TargetWarpNearest            ));
        pnsCommands->AddMember("CommandTargetWarpPrev"               , new Number((float)TK_TargetWarpPrev               ));
        pnsCommands->AddMember("CommandTrackCommandView"             , new Number((float)TK_TrackCommandView             ));
        pnsCommands->AddMember("CommandPauseTM"                      , new Number((float)TK_PauseTM                      ));
        pnsCommands->AddMember("CommandFinishTM"                     , new Number((float)TK_CommandFinishTM              ));
        pnsCommands->AddMember("CommandToggleMouse"                  , new Number((float)TK_ToggleMouse                  ));
        pnsCommands->AddMember("CommandVoteYes"                      , new Number((float)TK_VoteYes                      ));
        pnsCommands->AddMember("CommandVoteNo"                       , new Number((float)TK_VoteNo                       ));

        // debug only
        //#ifdef _DEBUG_TRAINING
        pnsCommands->AddMember("CommandSkipTMGoal"               , new Number((float)TK_SkipTMGoal               ));
        pnsCommands->AddMember("CommandIncreaseTMClockSpeed"     , new Number((float)TK_IncreaseTMClockSpeed     ));
        pnsCommands->AddMember("CommandDecreaseTMClockSpeed"     , new Number((float)TK_DecreaseTMClockSpeed     ));
        pnsCommands->AddMember("CommandResetTMClockSpeed"        , new Number((float)TK_ResetTMClockSpeed        ));
        //#endif

        pnsCommands->AddMember("None"   , new Number((float)ModifierNone   ));
        pnsCommands->AddMember("Shift"  , new Number((float)ModifierShift  ));
        pnsCommands->AddMember("Control", new Number((float)ModifierControl));
        pnsCommands->AddMember("Alt"    , new Number((float)ModifierAlt    ));
        pnsCommands->AddMember("Any"    , new Number((float)ModifierAny    ));

        TRef<Boolean> pboolShift      = GetKeyDown(VK_SHIFT  );
        TRef<Boolean> pboolControl    = GetKeyDown(VK_CONTROL);
        TRef<Boolean> pboolAlt        = GetKeyDown(VK_MENU   );

        TRef<Boolean> pboolNotShift   = Not(pboolShift  );
        TRef<Boolean> pboolNotControl = Not(pboolControl);
        TRef<Boolean> pboolNotAlt     = Not(pboolAlt    );

        TRef<Boolean> pboolNotControlShift = And(pboolNotControl, pboolNotShift);
        TRef<Boolean> pboolNotShiftAlt     = And(pboolNotShift,   pboolNotAlt  );
        TRef<Boolean> pboolNotControlAlt   = And(pboolNotControl, pboolNotAlt  );

        TRef<Boolean> pboolControlShift = And(pboolControl, pboolShift);
        TRef<Boolean> pboolShiftAlt     = And(pboolShift,   pboolAlt  );
        TRef<Boolean> pboolControlAlt   = And(pboolControl, pboolAlt  );

        m_pboolNone                = And(pboolNotControlShift, pboolNotAlt         );
        m_pboolJustShift           = And(pboolShift,           pboolNotControlAlt  );
        m_pboolJustControl         = And(pboolControl,         pboolNotShiftAlt    );
        m_pboolJustAlt             = And(pboolAlt,             pboolNotControlShift);
        m_pboolJustControlShift    = And(pboolControlShift,    pboolNotAlt         );
        m_pboolJustShiftAlt        = And(pboolShiftAlt,        pboolNotControl     );
        m_pboolJustControlAlt      = And(pboolControlAlt,      pboolNotShift       );
        m_pboolJustControlShiftAlt = And(pboolControlShift,    pboolAlt            );
    }

    Boolean* GetKeyDown(int vk)
    {
        if (m_pboolKeyDown[vk] == NULL) {
            m_pboolKeyDown[vk] = new ModifiableBoolean(false);
        }

        return m_pboolKeyDown[vk];
    }

    ZString GetFPSString()
    {
        return
              ZString(m_pboolNone->GetValue()                ? "N "   : "  "  )
            + ZString(m_pboolJustControl->GetValue()         ? "C "   : "  "  )
            + ZString(m_pboolJustShift->GetValue()           ? "S "   : "  "  )
            + ZString(m_pboolJustAlt->GetValue()             ? "A "   : "  "  )
            + ZString(m_pboolJustControlShift->GetValue()    ? "CS "  : "   " )
            + ZString(m_pboolJustShiftAlt->GetValue()        ? "SA "  : "   " )
            + ZString(m_pboolJustControlAlt->GetValue()      ? "CA "  : "   " )
            + ZString(m_pboolJustControlShiftAlt->GetValue() ? "CSA " : "    ")
            ;
    }

    bool LoadMap(const ZString& str)
    {
        //
        // load the input map
        //

        TRef<INameSpace> pns = GetModeler()->GetNameSpace(str, false);

        if (pns == NULL) {
            return false;
        }

        //
        // Redetect the joysticks
        //

        m_pinputEngine->EnumerateJoysticks();

        //
        // Initialize Joystick
        //

        if (
               m_pinputEngine->GetJoystick(0) 
            && m_pinputEngine->GetJoystick(0)->HasForceFeedback()
        ) {
            m_pinputEngine->GetJoystick(0)->CreateEffects();
        }

        //
        // Clear out the old input map
        //

        int index;

        for (index = 0; index < keyMax; index++) {
            m_KeyMap[index]                = -1;
            m_ShiftKeyMap[index]           = -1;
            m_ControlKeyMap[index]         = -1;
            m_AltKeyMap[index]             = -1;
            m_ControlShiftKeyMap[index]    = -1;
            m_ShiftAltKeyMap[index]        = -1;
            m_ControlAltKeyMap[index]      = -1;
            m_ControlShiftAltKeyMap[index] = -1;
        }

		m_wheelupTK						   = -1; //Imago 8/15/09 //Turkey changed to -1 and moved down to LoadMap 8/10 #56
        m_wheeldownTK		  			   = -1;
        m_wheelclickTK					   = -1;
        m_xbutton1TK				       = -1;
        m_xbutton2TK				       = -1;
        m_xbutton3TK				       = -1;
        m_xbutton4TK				       = -1;
        m_xbutton5TK				       = -1; // --^

        for (index = 0; index < TK_Max; index++) {
            m_pboolTrekKeyDown[index]       = NULL;
            m_ppboolTrekKeyButtonDown[index] = NULL;
        }

        for (index = 0; index < m_ppnumberAxis.GetCount(); index++) {
            m_ppnumberAxis[index] = NULL;
        }

        //
        // keyboard mapping
        //

        {
            TRef<IObjectList> plist = pns->FindList("keyCommandMap");

			// BT - 10/17 - Fix for crash when the keyCommandMap corrupted.
			if (plist == nullptr)
				return false;

            plist->GetFirst();
            while (plist->GetCurrent() != NULL) {
                TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                int vk        = (int)GetNumber(ppair->GetNth(0)    );
                int modifiers = (int)GetNumber(ppair->GetNth(1)    );
                int tk        = (int)GetNumber(ppair->GetLastNth(2));

                TArray<TrekKey, keyMax>* pptk;

                if (modifiers & ModifierShift) {
                    if (modifiers & ModifierControl) {
                        if (modifiers & ModifierAlt) {
                            pptk = &m_ControlShiftAltKeyMap;
                        } else {
                            pptk = &m_ControlShiftKeyMap;
                        }
                    } else {
                        if (modifiers & ModifierAlt) {
                            pptk = &m_ShiftAltKeyMap;
                        } else {
                            pptk = &m_ShiftKeyMap;
                        }
                    }
                } else {
                    if (modifiers & ModifierControl) {
                        if (modifiers & ModifierAlt) {
                            pptk = &m_ControlAltKeyMap;
                        } else {
                            pptk = &m_ControlKeyMap;
                        }
                    } else {
                        if (modifiers & ModifierAlt) {
                            pptk = &m_AltKeyMap;
                        } else {
                            pptk = &m_KeyMap;
                        }
                    }
                }

                ZAssert((*pptk)[vk] == -1);
                (*pptk)[vk] = tk;

                //
                // Boolean inputs, the key
                //

                TRef<Boolean> pbool = GetKeyDown(vk);

                //
                // And with modifiers
                //

                if (modifiers != ModifierAny) {
                    TRef<Boolean> pboolModifier;

                    if (modifiers & ModifierShift) {
                        if (modifiers & ModifierControl) {
                            if (modifiers & ModifierAlt) {
                                pboolModifier = m_pboolJustControlShiftAlt;
                            } else {
                                pboolModifier = m_pboolJustControlShift;
                            }
                        } else {
                            if (modifiers & ModifierAlt) {
                                pboolModifier = m_pboolJustShiftAlt;
                            } else {
                                pboolModifier = m_pboolJustShift;
                            }
                        }
                    } else {
                        if (modifiers & ModifierControl) {
                            if (modifiers & ModifierAlt) {
                                pboolModifier = m_pboolJustControlAlt;
                            } else {
                                pboolModifier = m_pboolJustControl;
                            }
                        } else {
                            if (modifiers & ModifierAlt) {
                                pboolModifier = m_pboolJustAlt;
                            } else {
                                pboolModifier = m_pboolNone;
                            }
                        }
                    }

                    pbool = And(pbool, pboolModifier);
                }

                //
                // Or all of the key combinations together
                //

                if (m_pboolTrekKeyDown[tk] == NULL) {
                    m_pboolTrekKeyDown[tk] = pbool;
                } else {
                    m_pboolTrekKeyDown[tk] =
                        Or(
                            pbool,
                            m_pboolTrekKeyDown[tk]
                        );
                }

                //
                // go to next mapping
                //

                plist->GetNext();
            }
        }

        //
        // mouse button mapping
        //

        {
            TRef<IObjectList> plist = pns->FindList("mouseButtonCommandMap");

            if (plist) {
                plist->GetFirst();
                while (plist->GetCurrent() != NULL) {
                    TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                    int index = (int)GetNumber(ppair->GetFirst() );
                    int tk    = (int)GetNumber(ppair->GetSecond());
                    
                    //Imago save special buttons for use outside virtual joystick 8/14/09
                    switch(index) {
                        case 2:
                            m_wheelclickTK = tk;
                            break;
                        case 3:
                            m_xbutton1TK = tk;
                            break;
                        case 4:
                            m_xbutton2TK = tk;
                            break;
                        case 5:
                            m_xbutton3TK = tk;
                            break;
                        case 6:
                            m_xbutton4TK = tk;
                            break;
                        case 7:
                            m_xbutton5TK = tk;
                            break;
                        case 8:
                            m_wheeldownTK = tk;
                            break;
                        case 9:
                            m_wheelupTK = tk;
                            break;

                        default:
                            break;
                    }

                    //
                    // Get the button
                    //

                    TRef<Boolean> pboolButton = m_pjoystickImage->IsDown(index);

                    if (pboolButton) {
                        //
                        // Or all of the key combinations together
                        //

                        if (m_ppboolTrekKeyButtonDown[tk] == NULL) {
                            m_ppboolTrekKeyButtonDown[tk] = pboolButton;
                        } else {
                            m_ppboolTrekKeyButtonDown[tk] =
                                Or(
                                    pboolButton,
                                    m_ppboolTrekKeyButtonDown[tk]
                                );
                        }
                    }

                    plist->GetNext();
                }
            }
        }

        //
        // joystick button mapping
        //

        if (m_pinputEngine->GetJoystick(0) && m_pinputEngine->GetJoystick(0)->GetButtonCount()) { //Imago 8/18/09
            TRef<IObjectList> plist = pns->FindList("buttonCommandMap");

            plist->GetFirst();
            while (plist->GetCurrent() != NULL) {
                TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                int indexJoystick = (int)GetNumber(ppair->GetNth(0)    );
                int indexButton   = (int)GetNumber(ppair->GetNth(1)    );
                int tk            = (int)GetNumber(ppair->GetLastNth(2));

                //
                // Get the button
                //

                TRef<JoystickInputStream> pjoystick = m_pinputEngine->GetJoystick(indexJoystick);
                TRef<Boolean>             pboolButton;

                if (indexButton < 0) {
                    //
                    // It's a virtual hat button
                    //

                    pboolButton = m_ppboolHatButton[-indexButton - 1];
                } else {
                    //
                    // A joystick button
                    //

                    if (pjoystick) {
                        pboolButton = pjoystick->IsDown(indexButton);
                    }
                }

                if (pboolButton) {
                    //
                    // Or all of the key combinations together
                    //

                    if (m_ppboolTrekKeyButtonDown[tk] == NULL) {
                        m_ppboolTrekKeyButtonDown[tk] = pboolButton;
                    } else {
                        m_ppboolTrekKeyButtonDown[tk] =
                            Or(
                                pboolButton,
                                m_ppboolTrekKeyButtonDown[tk]
                            );
                    }
                }

                plist->GetNext();
            }
        }

        //
        // joystick value mapping
        //

        if (m_pinputEngine->GetJoystick(0) && m_pinputEngine->GetJoystick(0)->GetValueCount()) { //Imago 8/18/09
            TRef<IObjectList> plist = pns->FindList("numericValuesNew");

            bool bNew = (plist != NULL);

            if (!bNew) {
                plist = pns->FindList("numericValues");
            }
        
            plist->GetFirst();
            for (int indexValueOutput = 0; indexValueOutput < m_ppnumberAxis.GetCount(); indexValueOutput++) {
                TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                if (ppair) {
                    int indexJoystick   = (int)GetNumber(ppair->GetFirst() );
                    int indexValueInput = (int)GetNumber(ppair->GetSecond());

                    if (!bNew) {
                        if (indexValueInput == 2) {
                            indexValueInput = 3;
                        } else if (indexValueInput == 3) {
                            indexValueInput = 2;
                        }
                    }

                    TRef<JoystickInputStream> pjoystick = m_pinputEngine->GetJoystick(indexJoystick);
                    TRef<Number>              pvalueInput;

                    if (pjoystick) {
                        pvalueInput = pjoystick->GetValue(indexValueInput);
                    }

                    if (pvalueInput) {
                        m_ppnumberAxis[indexValueOutput] = pvalueInput;    
                    } else {
                        m_ppnumberAxis[indexValueOutput] = NULL;
                    }
                }

                plist->GetNext();
            }
        }

        //
        // Unload the map
        //

        GetModeler()->UnloadNameSpace(pns);

        return true;
    }

    void ClearButtonStates()
    {
        for (int index = 0; index < TK_Max; index++) {
			m_boolTrekKeyButtonDown[index] = false;
			m_boolMouseTrekKeyDown[index] = false;	//8/10 #56
        // mmf pull yp's changes for now
			//m_pboolTrekKeyDown[index]       = false; // keyboard // yp - Your_Persona buttons get stuck patch. aug-03-2006
            //m_ppboolTrekKeyButtonDown[index] = false;
        }
		// yp - After that for loop we lose responce from most of our keys.. so..		 
		// hack.. we reload the map, and something in there fixes it.
		//if (!LoadMap(INPUTMAP_FILE)) {
        //    LoadMap(DEFAULTINPUTMAP_FILE);
        //}
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // TrekInput Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    bool IsTrekKeyDown(TrekKey tk, bool bCheckKeyboard)
    {
        bool bDown = false;

        //
        // check joystick buttons
        //

        Boolean* pbool = m_ppboolTrekKeyButtonDown[tk];

        if (pbool) {
            bDown = pbool->GetValue();
        }

		//
		// check mouse buttons 8/10 #56
		// only applies if virtual joystick is off
		//

		bDown |= m_boolMouseTrekKeyDown[tk];

        //
        // check keys
        //

        if (bCheckKeyboard) {
            Boolean* pbool = m_pboolTrekKeyDown[tk];

            if (pbool) {
                bDown = bDown || pbool->GetValue();
            }
        }

        return bDown;
    }

    //Imago 8/16/09
	//Turkey renamed and modified to use bool instead of Tref<boolean> 8/10 #56
    void SetMouseTrekKey(TrekKey tk, Boolean* pboolDown)
	{
		if (tk != TK_NoKeyMapping) {
			m_boolMouseTrekKeyDown[tk] = pboolDown->GetValue();
			if (pboolDown->GetValue()) m_psite->OnTrekKey(tk);
		}
    }

    void SetFocus(bool bFocus)
    {
        if (m_bFocus != bFocus) {
            m_bFocus = bFocus;

            if (!m_bFocus) {
                for (int index = 0; index < keyMax; index++) {
                    if (m_pboolKeyDown[index] != NULL) {
                        m_pboolKeyDown[index]->SetValue(false);
                    }
                }

                ClearButtonStates();
            }
        }
    }

    TrekKey TranslateKeyMessage(const KeyState& ks)
    {
        //
        // Translate to a trek key
        //

        if (ks.bShift) {
            if (ks.bControl) {
                if (ks.bAlt) {
                    return m_ControlShiftAltKeyMap[ks.vk];
                } else {
                    return m_ControlShiftKeyMap[ks.vk];
                }
            } else {
                if (ks.bAlt) {
                    return m_ShiftAltKeyMap[ks.vk];
                } else {
                    return m_ShiftKeyMap[ks.vk];
                }
            }
        } else {
            if (ks.bControl) {
                if (ks.bAlt) {
                    return m_ControlAltKeyMap[ks.vk];
                } else {
                    return m_ControlKeyMap[ks.vk];
                }
            } else {
                if (ks.bAlt) {
                    return m_AltKeyMap[ks.vk];
                } else {
                    return m_KeyMap[ks.vk];
                }
            }
        }

        return TK_NoKeyMapping;
    }

    TrekKey HandleKeyMessage(const KeyState& ks)
    {
        /* 
        ZDebugOutput(
            "Key: " 
            + ZString(ks.vk) 
            + " Down: " 
            + (ks.bDown    ? "True " : "False ")
            + (ks.bShift   ? "S"     : ""      )
            + (ks.bControl ? "C"     : ""      )
            + (ks.bAlt     ? "A"     : ""      )
            + "\n"
        );
        */

        if (m_bFocus) {
            //
            // Update boolean value
            //
        
            if (m_pboolKeyDown[ks.vk] != NULL) {
                m_pboolKeyDown[ks.vk]->SetValue(ks.bDown);
            }

            //
            // Translate to a trek key
            //

            return TranslateKeyMessage(ks);
        }

        return TK_NoKeyMapping;
    }

    void PlayFFEffect(ForceEffectID effectID, LONG lDirection)
    {
        if (
               m_pinputEngine->GetJoystick(0) 
            && m_pinputEngine->GetJoystick(0)->HasForceFeedback()
        ) {
            m_pinputEngine->GetJoystick(0)->PlayFFEffect(effectID, lDirection);
        }
    }

    float GetAxis(int index)
    {
        if (m_ppnumberAxis[index] != NULL) {
            return m_ppnumberAxis[index]->GetValue();
        } else {
            return 0;
        }
    }

    bool IsAxisValid(int index)
    {
        return m_ppnumberAxis[index] != NULL;
    }

    void GetButtonTrekKeys(TrekInputSite* psite)
    {
        //
        // Update hat buttons
        //

        if (m_ppnumberAxis[5] != NULL) {
            float value = m_ppnumberAxis[5]->GetValue();

            //ZDebugOutput("hat: " + ZString(value) + "\n");

            // 0 up     =  0
            // 1 right  =  0.5
            // 2 down   =  1
            // 3 left   = -0.5
            //   center = -2.0

            for (int index = 0; index < 4; index++) {
                m_ppboolHatButton[index]->SetValue(0);
            } 

            if (value > 0.75f) {
                m_ppboolHatButton[2]->SetValue(1);
            } else if (value > 0.25f) {
                m_ppboolHatButton[1]->SetValue(1);
            } else if (value > -0.25f) {
                m_ppboolHatButton[0]->SetValue(1);
            } else if (value > -0.75f) {
                m_ppboolHatButton[3]->SetValue(1);
            } else if (value > -1.25f) {
                m_ppboolHatButton[2]->SetValue(1);
            }
        }

        //
        // If any of the button states changes send a trek key
        //

        for (int index = 0; index < TK_Max; index++) {

            if (
                   m_ppboolTrekKeyButtonDown[index] != NULL
                && m_ppboolTrekKeyButtonDown[index]->GetValue() != m_boolTrekKeyButtonDown[index]
            ) {
                m_boolTrekKeyButtonDown[index] = m_ppboolTrekKeyButtonDown[index]->GetValue();

                if (m_boolTrekKeyButtonDown[index]) {
                    psite->OnTrekKey(index);
                }
            }
        }        
    }

    //Imago 8/14/09 allow OnTrekKey usage outside of trekinput
    TRef<TrekInputSite> GetInputSite() {
        return m_psite;
    }

    void SetInputSite(TrekInputSite* psite) {
        m_psite = psite;
    }

    //Imago 8/14/09 expose mappings for use outside virtual joystick
    TrekKey OnWheelDown() {
        return m_wheeldownTK;
    }

    TrekKey OnWheelUp() {
        return m_wheelupTK;
    }

   TrekKey OnWheelClick() {
        return m_wheelclickTK;
    }

   TrekKey OnXButton1() {
        return m_xbutton1TK;
    }

   TrekKey OnXButton2() {
        return m_xbutton2TK;
    }

   TrekKey OnXButton3() {
        return m_xbutton3TK;
    }

   TrekKey OnXButton4() {
        return m_xbutton4TK;
    }

   TrekKey OnXButton5() {
        return m_xbutton5TK;
    }

    TRef<IPopup> CreateInputMapPopup(Modeler* pmodeler, IEngineFont* pfont, Number* ptime)
    {
        return 
            ::CreateInputMapPopup(
                pmodeler,
                m_pinputEngine,
                this,
                pfont,
                ptime
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////

TRef<TrekInput> CreateTrekInput(HINSTANCE hInst, HWND hwnd, InputEngine* pinputEngine, JoystickImage* pjoystickImage)
{
    return new TrekInputImpl(hInst, hwnd, pinputEngine, pjoystickImage);
}

//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////

PCC g_ppccAxisDescription[] =
{
    "Yaw",
    "Pitch",
    "Roll",
    "Throttle",
    "Hat",
    "Hat as Buttons"
};

class KeyInfo {
public:
    PCC m_pcc;
};

KeyInfo g_pkeyInfo[] =
{
    { NULL               }, // 00
    { NULL               }, // 01
    { NULL               }, // 02
    { NULL               }, // 03
    { NULL               }, // 04
    { NULL               }, // 05
    { NULL               }, // 06
    { NULL               }, // 07
    { "Backspace"        }, // 08
    { "Tab"              }, // 09
    { NULL               }, // 0a
    { NULL               }, // 0b
    { NULL               }, // 0c
    { "Enter"            }, // 0d
    { NULL               }, // 0e
    { NULL               }, // 0f

    { NULL               }, // 10
    { NULL               }, // 11
    { NULL               }, // 12
    { "Pause"            }, // 13
    { "Caps Lock"        }, // 14
    { NULL               }, // 15
    { NULL               }, // 16
    { NULL               }, // 17
    { NULL               }, // 18
    { NULL               }, // 19
    { NULL               }, // 1a
    { "Escape"           }, // 1b
    { NULL               }, // 1c
    { NULL               }, // 1d
    { NULL               }, // 1e
    { NULL               }, // 1f

    { "Space"            }, // 20
    { "Page Up"          }, // 21
    { "Page Down"        }, // 22
    { "End"              }, // 23
    { "Home"             }, // 24
    { "Left"             }, // 25
    { "Up"               }, // 26
    { "Right"            }, // 27
    { "Down"             }, // 28
    { NULL               }, // 29
    { NULL               }, // 2a
    { NULL               }, // 2b
    { "PrintScreen"      }, // 2c
    { "Insert"           }, // 2d
    { "Delete"           }, // 2e
    { NULL               }, // 2f

    { "0"                }, // 30
    { "1"                }, // 31
    { "2"                }, // 32
    { "3"                }, // 33
    { "4"                }, // 34
    { "5"                }, // 35
    { "6"                }, // 36
    { "7"                }, // 37
    { "8"                }, // 38
    { "9"                }, // 39
    { NULL               }, // 3a
    { NULL               }, // 3b
    { NULL               }, // 3c
    { NULL               }, // 3d
    { NULL               }, // 3e
    { NULL               }, // 3f

    { NULL               }, // 40
    { "A"                }, // 41
    { "B"                }, // 42
    { "C"                }, // 43
    { "D"                }, // 44
    { "E"                }, // 45
    { "F"                }, // 46
    { "G"                }, // 47
    { "H"                }, // 48
    { "I"                }, // 49
    { "J"                }, // 4a
    { "K"                }, // 4b
    { "L"                }, // 4c
    { "M"                }, // 4d
    { "N"                }, // 4e
    { "O"                }, // 4f

    { "P"                }, // 50
    { "Q"                }, // 51
    { "R"                }, // 52
    { "S"                }, // 53
    { "T"                }, // 54
    { "U"                }, // 55
    { "V"                }, // 56
    { "W"                }, // 57
    { "X"                }, // 58
    { "Y"                }, // 59
    { "Z"                }, // 5a
    { NULL               }, // 5b
    { NULL               }, // 5c
    { NULL               }, // 5d
    { NULL               }, // 5e
    { NULL               }, // 5f

    { "Num Pad 0"        }, // 60
    { "Num Pad 1"        }, // 61
    { "Num Pad 2"        }, // 62
    { "Num Pad 3"        }, // 63
    { "Num Pad 4"        }, // 64
    { "Num Pad 5"        }, // 65
    { "Num Pad 6"        }, // 66
    { "Num Pad 7"        }, // 67
    { "Num Pad 8"        }, // 68
    { "Num Pad 9"        }, // 69
    { "Num Pad Multiply" }, // 6a
    { "Num Pad Plus"     }, // 6b
    { "Num Pad Enter"    }, // 6c
    { "Num Pad Minus"    }, // 6d
    { "Num Pad Decimal"  }, // 6e
    { "Num Pad Divide"   }, // 6f

    { "F1"               }, // 70
    { "F2"               }, // 71
    { "F3"               }, // 72
    { "F4"               }, // 73
    { "F5"               }, // 74
    { "F6"               }, // 75
    { "F7"               }, // 76
    { "F8"               }, // 77
    { "F9"               }, // 78
    { "F10"              }, // 79
    { "F11"              }, // 7a
    { "F12"              }, // 7b
    { NULL               }, // 7c
    { NULL               }, // 7d
    { NULL               }, // 7e
    { NULL               }, // 7f

    { NULL               }, // 80
    { NULL               }, // 81
    { NULL               }, // 82
    { NULL               }, // 83
    { NULL               }, // 84
    { NULL               }, // 85
    { NULL               }, // 86
    { NULL               }, // 87
    { NULL               }, // 88
    { NULL               }, // 89
    { NULL               }, // 8a
    { NULL               }, // 8b
    { NULL               }, // 8c
    { NULL               }, // 8d
    { NULL               }, // 8e
    { NULL               }, // 8f

    { "Num Lock"         }, // 90
    { "Scroll Lock"      }, // 91
    { NULL               }, // 92
    { NULL               }, // 93
    { NULL               }, // 94
    { NULL               }, // 95
    { NULL               }, // 96
    { NULL               }, // 97
    { NULL               }, // 98
    { NULL               }, // 99
    { NULL               }, // 9a
    { NULL               }, // 9b
    { NULL               }, // 9c
    { NULL               }, // 9d
    { NULL               }, // 9e
    { NULL               }, // 9f

    { NULL               }, // a0
    { NULL               }, // a1
    { NULL               }, // a2
    { NULL               }, // a3
    { NULL               }, // a4
    { NULL               }, // a5
    { NULL               }, // a6
    { NULL               }, // a7
    { NULL               }, // a8
    { NULL               }, // a9
    { NULL               }, // aa
    { NULL               }, // ab
    { NULL               }, // ac
    { NULL               }, // ad
    { NULL               }, // ae
    { NULL               }, // af

    { NULL               }, // b0
    { NULL               }, // b1
    { NULL               }, // b2
    { NULL               }, // b3
    { NULL               }, // b4
    { NULL               }, // b5
    { NULL               }, // b6
    { NULL               }, // b7
    { NULL               }, // b8
    { NULL               }, // b9
    { "SemiColon"        }, // ba
    { "Equals"           }, // bb
    { "Comma"            }, // bc
    { "Minus"            }, // bd
    { "Period"           }, // be
    { "Forward Slash"    }, // bf

    { "Tilde"            }, // c0
    { NULL               }, // c1
    { NULL               }, // c2
    { NULL               }, // c3
    { NULL               }, // c4
    { NULL               }, // c5
    { NULL               }, // c6
    { NULL               }, // c7
    { NULL               }, // c8
    { NULL               }, // c9
    { NULL               }, // ca
    { NULL               }, // cb
    { NULL               }, // cc
    { NULL               }, // cd
    { NULL               }, // ce
    { NULL               }, // cf

    { NULL               }, // d0
    { NULL               }, // d1
    { NULL               }, // d2
    { NULL               }, // d3
    { NULL               }, // d4
    { NULL               }, // d5
    { NULL               }, // d6
    { NULL               }, // d7
    { NULL               }, // d8
    { NULL               }, // d9
    { NULL               }, // da
    { "Left Square"      }, // db
    { "Back Slash"       }, // dc
    { "Right Square"     }, // dd
    { "Quote"            }, // de
    { NULL               }, // df

    { NULL               }, // e0
    { NULL               }, // e1
    { NULL               }, // e2
    { NULL               }, // e3
    { NULL               }, // e4
    { NULL               }, // e5
    { NULL               }, // e6
    { NULL               }, // e7
    { NULL               }, // e8
    { NULL               }, // e9
    { NULL               }, // ea
    { NULL               }, // eb
    { NULL               }, // ec
    { NULL               }, // ed
    { NULL               }, // ee
    { NULL               }, // ef

    { NULL               }, // f0
    { NULL               }, // f1
    { NULL               }, // f2
    { NULL               }, // f3
    { NULL               }, // f4
    { NULL               }, // f5
    { NULL               }, // f6
    { NULL               }, // f7
    { NULL               }, // f8
    { NULL               }, // f9
    { NULL               }, // fa
    { NULL               }, // fb
    { NULL               }, // fc
    { NULL               }, // fd
    { NULL               }, // fe
    { NULL               }  // ff
};

class CommandInfo {
public:
    int m_tk;
    PCC m_pcc;
};

CommandInfo g_pCommandInfo[] =
{
    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Chat"                                  },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_QuickChatMenu                   , "Quick Chat Menu"                       },
    { TK_StartChat                       , "Start Chat"                            },
    { TK_ChatPageUp                      , "Chat Page Up"                          },
    { TK_ChatPageDown                    , "Chat Page Down"                        },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Commands"                              },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_AcceptCommand                   , "Accept Command"                        },
    { TK_ClearCommand                    , "Clear Command"                         },
    { TK_RejectCommand                   , "Reject Command"                        },
    { TK_ExecuteCommand                  , "Execute Command"                       },

    { TK_VoteYes                         , "Vote Yes"                              },
    { TK_VoteNo                          , "Vote No"                               },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Weapons"                               },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_FireWeapon                      , "Fire Weapon"                           },
    { TK_FireMissile                     , "Fire Missile"                          },
    { TK_PrevWeapon                      , "Previous Weapon"                       },
    { TK_NextWeapon                      , "Next Weapon"                           },
    { TK_ToggleGroupFire                 , "Toggle Group Fire"                     },
    { TK_Reload                          , "Reload"                                },
    { TK_DropMine                        , "Drop Mine or Probe"                    },// yp - Your_Persona fix bug65 (rewording) Aug-04-2006
    { TK_DropChaff                       , "Drop Chaff"                            },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Turning"                               },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_PitchUp                         , "Pitch Up"                              },
    { TK_PitchDown                       , "Pitch Down"                            },
    { TK_YawLeft                         , "Yaw Left"                              },
    { TK_YawRight                        , "Yaw Right"                             },
    { TK_RollLeft                        , "Roll Left"                             },
    { TK_RollRight                       , "Roll Right"                            },
    { TK_RollModifier                    , "Roll Modifier"                         },
    { TK_ToggleMouse                     , "Toggle Mouse Control"                  },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Thrust"                                },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_ThrustLeft                      , "Thrust Left"                           },
    { TK_ThrustRight                     , "Thrust Right"                          },
    { TK_ThrustUp                        , "Thrust Up"                             },
    { TK_ThrustDown                      , "Thrust Down"                           },
    { TK_ThrustForward                   , "Thrust Forward"                        },
    { TK_ThrustBackward                  , "Thrust Backward"                       },
    { TK_FireBooster                     , "Fire Booster"                          },
    { TK_ThrottleUp                      , "Throttle Up"                           },
    { TK_ThrottleDown                    , "Throttle Down"                         },
    { TK_ThrottleZero                    , "Throttle Zero"                         },
    { TK_Throttle33                      , "Throttle 33%"                          },
    { TK_Throttle66                      , "Throttle 66%"                          },
    { TK_ThrottleFull                    , "Throttle Full"                         },
    { TK_LockVector                      , "Lock Vector"                           },
    { TK_ToggleAutoPilot                 , "Toggle AutoPilot"                      },
    { TK_MatchSpeed                      , "Match Speed"                           },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Camera"                                },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_ZoomOut                         , "Zoom Out"                              },
    { TK_ZoomIn                          , "Zoom In"                               },
    { TK_ViewFrontLeft                   , "View Front Left"                       },
    { TK_ViewFrontRight                  , "View Front Right"                      },
    { TK_ViewLeft                        , "View Left"                             },
    { TK_ViewRight                       , "View Right"                            },
    { TK_ViewExternal                    , "View External"                         },
    { TK_ViewFlyby                       , "View Flyby"                            },
    { TK_ViewMissile                     , "View Missile"                          },
    { TK_ViewTarget                      , "View Target"                           },
    { TK_ViewChase                       , "View Chase"                            },
//    { TK_ViewTurret                      , "View Turret"                           },
    { TK_ViewBase                        , "View Base"                             },
    { TK_ViewSector                      , "View Sector"                           },
    { TK_ViewCommand                     , "View Command"                          },
    { TK_ViewRear                        , "View Rear"                             },
    { TK_ViewRearLeft                    , "View Rear Left"                        },
    { TK_ViewRearRight                   , "View Rear Right"                       },
    { TK_ViewExternalOrbit               , "View External Orbit"                   },
    { TK_ViewExternalStation             , "View External Target"                  },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "UI Toggles"                            },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_ConModeGameState                , "Switch To Game State"                  },
    { TK_ConModeCombat                   , "Switch To Combat"                      },
    { TK_ConModeCommand                  , "Switch To Command"                     },
    { TK_ConModeInventory                , "Switch To Inventory"                   },
    { TK_ConModeInvest                   , "Switch To Invest"                      },
    { TK_ConModeTeam                     , "Switch To Team"                        },
    { TK_ConModeNav                      , "Switch To Nav"                         },
    { TK_ConModeMiniTeam                 , "Switch To Mini Team"                   },
    { TK_ConModeTeleport                 , "Switch To Teleport"                    },
    { TK_ToggleGrid                      , "Toggle Grid"                           },
    { TK_CycleRadar                      , "Cycle Radar Modes"                     },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Inventory"                             },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_SwapWeapon1                     , "Swap Weapon1"                          },
    { TK_SwapWeapon2                     , "Swap Weapon2"                          },
    { TK_SwapWeapon3                     , "Swap Weapon3"                          },
    { TK_SwapWeapon4                     , "Swap Weapon4"                          },
    { TK_SwapTurret1                     , "Swap Turret1"                          },
    { TK_SwapTurret2                     , "Swap Turret2"                          },
    { TK_SwapTurret3                     , "Swap Turret3"                          },
    { TK_SwapTurret4                     , "Swap Turret4"                          },
    { TK_PromoteTurret1                  , "Promote Turret1"                       },
    { TK_PromoteTurret2                  , "Promote Turret2"                       },
    { TK_PromoteTurret3                  , "Promote Turret3"                       },
    { TK_PromoteTurret4                  , "Promote Turret4"                       },
    { TK_SwapMissile                     , "Swap Missile"                          },
    { TK_SwapChaff                       , "Swap Chaff"                            },
    { TK_SwapShield                      , "Swap Shield"                           },
    { TK_SwapCloak                       , "Swap Cloak"                            },
    { TK_SwapAfterburner                 , "Swap Afterburner"                      },
    { TK_SwapMine                        , "Swap Mine"                             },
    { TK_NextCargo                       , "Next Cargo"                            },
    { TK_ToggleWeapon1                   , "Toggle Weapon1"                        },
    { TK_ToggleWeapon2                   , "Toggle Weapon2"                        },
    { TK_ToggleWeapon3                   , "Toggle Weapon3"                        },
    { TK_ToggleWeapon4                   , "Toggle Weapon4"                        },
    { TK_DropCargo                       , "Drop Cargo"                            },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Targeting"                             },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_Target                          , "Target"                                },
    { TK_TargetNothing                   , "Target Nothing"                        },
    { TK_TargetSelf                      , "Target Self"                           },
    { TK_TargetNearest                   , "Target Nearest"                        },
    { TK_TargetPrev                      , "Target Previous"                       },
    { TK_TargetCommand                   , "Target Command"                        },
    { TK_TargetCenter                    , "Target Center"                         },
    { TK_NoKeyMapping                    , ""                                      },
    { TK_TargetFlag                      , "Target Flag"                           },
    { TK_TargetFlagNearest               , "Target Nearest flag"                   },
    { TK_TargetFlagPrev                  , "Target Previous flag"                  },
    { TK_NoKeyMapping                    , ""                                      },
    { TK_TargetArtifact                  , "Target Artifact"                       },
    { TK_TargetArtifactNearest           , "Target Nearest artifact"               },
    { TK_TargetArtifactPrev              , "Target Previous artifact"              },
    { TK_NoKeyMapping                    , ""                                      },
    { TK_TargetWarp                      , "Target Aleph"                          },
    { TK_TargetWarpNearest               , "Target Nearest Aleph"                  },
    { TK_TargetWarpPrev                  , "Target Previous Alpeh"                 },
    { TK_NoKeyMapping                    , ""                                      },
    { TK_TargetHostile                   , "Target Hostile"                        },
    { TK_TargetEnemy                     , "Target Enemy"                          },
    { TK_TargetEnemyNearest              , "Target Enemy Nearest"                  },
    { TK_TargetEnemyPrev                 , "Target Enemy Previous"                 },
    { TK_TargetEnemyBase                 , "Target Enemy Base"                     },
    { TK_TargetEnemyBaseNearest          , "Target Enemy Base Nearest"             },
    { TK_TargetEnemyBasePrev             , "Target Enemy Base Previous"            },
    { TK_TargetEnemyBomber               , "Target Enemy Bomber"                   },
    { TK_TargetEnemyFighter              , "Target Enemy Fighter"                  },
    { TK_TargetEnemyConstructor          , "Target Enemy Constructor"              },
    { TK_TargetEnemyMiner                , "Target Enemy Miner"                    },
    { TK_TargetEnemyMajorBase            , "Target Enemy Major Base"               },
    { TK_TargetEnemyBomberNearest        , "Target Nearest Enemy Bomber"           },
    { TK_TargetEnemyFighterNearest       , "Target Nearest Enemy Fighter"          },
    { TK_TargetEnemyConstructorNearest   , "Target Nearest Enemy Constructor"      },
    { TK_TargetEnemyMinerNearest         , "Target Nearest Enemy Miner"            },
    { TK_TargetEnemyMajorBaseNearest     , "Target Nearest Enemy Major Base"       },
    { TK_TargetEnemyBomberPrev           , "Target Previous Enemy Bomber"          },
    { TK_TargetEnemyFighterPrev          , "Target Previous Enemy Fighter"         },
    { TK_TargetEnemyConstructorPrev      , "Target Previous Enemy Constructor"     },
    { TK_TargetEnemyMinerPrev            , "Target Previous Enemy Miner"           },
    { TK_TargetEnemyMajorBasePrev        , "Target Previous Enemy Major Base"      },
    { TK_NoKeyMapping                    , ""                                      },
 	{ TK_TargetAlliedBase                , "Target Allied Base"                    }, //Imago 8/1/09
    { TK_TargetAlliedBaseNearest         , "Target Allied Base Nearest"            },
    { TK_TargetAlliedBasePrev            , "Target Allied Base Previous"           },
    { TK_TargetFriendly                  , "Target Friendly"                       },
    { TK_TargetFriendlyNearest           , "Target Friendly Nearest"               },
    { TK_TargetFriendlyPrev              , "Target Friendly Previous"              },
    { TK_TargetFriendlyBase              , "Target Friendly Base"                  },
    { TK_TargetFriendlyBaseNearest       , "Target Friendly Base Nearest"          },
    { TK_TargetFriendlyBasePrev          , "Target Friendly Base Previous"         },
    { TK_TargetFriendlyBomber            , "Target Friendly Bomber"                },
    { TK_TargetFriendlyFighter           , "Target Friendly Fighter"               },
    { TK_TargetFriendlyConstructor       , "Target Friendly Constructor"           },
    { TK_TargetFriendlyMiner             , "Target Friendly Miner"                 },
    { TK_TargetFriendlyLifepod           , "Target Friendly Lifepod"               },
    { TK_TargetFriendlyMajorBase         , "Target Friendly Major Base"            },
    { TK_TargetFriendlyBomberNearest     , "Target Nearest Friendly Bomber"        },
    { TK_TargetFriendlyFighterNearest    , "Target Nearest Friendly Fighter"       },
    { TK_TargetFriendlyConstructorNearest, "Target Nearest Friendly Constructor"   },
    { TK_TargetFriendlyMinerNearest      , "Target Nearest Friendly Miner"         },
    { TK_TargetFriendlyLifepodNearest    , "Target Nearest Friendly Lifepod"       },
    { TK_TargetFriendlyMajorBaseNearest  , "Target Nearest Friendly Major Base"    },
    { TK_TargetFriendlyBomberPrev        , "Target Previous Friendly Bomber"       },
    { TK_TargetFriendlyFighterPrev       , "Target Previous Friendly Fighter"      },
    { TK_TargetFriendlyConstructorPrev   , "Target Previous Friendly Constructor"  },
    { TK_TargetFriendlyMinerPrev         , "Target Previous Friendly Miner"        },
    { TK_TargetFriendlyLifepodPrev       , "Target Previous Friendly Lifepod"      },
    { TK_TargetFriendlyMajorBasePrev     , "Target Previous Friendly Major Base"   },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Other"                                 },
    { TK_NoKeyMapping                    , ""                                      },

    { TK_Suicide                         , "Eject From Turret (Press 3 times)"     },
    { TK_ToggleCloak                     , "Toggle Cloak"                          },
    { TK_Ripcord                         , "Ripcord (teleport)"                    },
    { TK_PauseTM                         , "Pause (training mission only)"         },
    { TK_CommandFinishTM                 , "Finish goal (training mission only)"   },
    { TK_TrackCommandView                , "Toggle command view target tracking"   },
	{ TK_ScrnShot                        , "Take a screen shot"                    },

    { TK_NoKeyMapping                    , ""                                      },
    { TK_NoKeyMapping                    , "Joystick"                              },
    { TK_NoKeyMapping                    , ""                                      },
};

const int g_nCommandInfo = ArrayCount(g_pCommandInfo);

//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////

class InputMapPopup :
    public IPopup,
    public EventTargetContainer<InputMapPopup>,
    public List,
    public ItemPainter
{
private:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Types
    //
    //////////////////////////////////////////////////////////////////////////////

    class KeyMapping {
    public:
        int m_vk       ;
        int m_modifiers;
    };

    class ButtonMapping {
    public:
        int m_indexJoystick;
        int m_indexButton  ;
    };

    class CommandMap {
    public:
		int indexJoystick;
        TList<KeyMapping>    m_listKeyMapping;
        TList<ButtonMapping> m_listButtonMapping;

        void Clear()
        {
            m_listKeyMapping.SetEmpty();
            m_listButtonMapping.SetEmpty();
        }

        void AddKeyMapping(int vk, int modifiers)
        {
            m_listKeyMapping.PushFront();
            m_listKeyMapping.GetFront().m_vk = vk;

            if (modifiers == ModifierAny) {
                m_listKeyMapping.GetFront().m_modifiers = ModifierNone;
            } else {
                m_listKeyMapping.GetFront().m_modifiers = modifiers;
            }
        }

        void AddButtonMapping(int indexJoystick, int indexButton)
        {
            m_listButtonMapping.PushFront();
            m_listButtonMapping.GetFront().m_indexJoystick = indexJoystick;
            m_listButtonMapping.GetFront().m_indexButton   = indexButton;
        }

        bool HasKeyMapping(int vk, int modifiers)
        {
            TList<KeyMapping>::Iterator iter(m_listKeyMapping);

            while (!iter.End()) {
                KeyMapping& map = iter.Value();

                if (
                       map.m_vk        == vk 
                    && map.m_modifiers == modifiers
                ) {
                    return true;
                }

                iter.Next();
            }

            return false;
        }

        void RemoveKeyMapping(int vk, int modifiers)
        {
            TList<KeyMapping>::Iterator iter(m_listKeyMapping);

            while (!iter.End()) {
                KeyMapping& map = iter.Value();

                if (
                       map.m_vk        == vk 
                    && map.m_modifiers == modifiers
                ) {
                    iter.Remove();
                } else {
                    iter.Next();
                }
            }
        }

        bool HasButtonMapping(int indexJoystick, int indexButton)
        {
            TList<ButtonMapping>::Iterator iter(m_listButtonMapping);

            while (!iter.End()) {
                ButtonMapping& map = iter.Value();

                if (
                       map.m_indexJoystick == indexJoystick
                    && map.m_indexButton   == indexButton
                ) {
                    return true;
                }

                iter.Next();
            }

            return false;
        }

        void RemoveButtonMapping(int indexJoystick, int indexButton)
        {
            TList<ButtonMapping>::Iterator iter(m_listButtonMapping);

            while (!iter.End()) {
                ButtonMapping& map = iter.Value();

                if (
                       map.m_indexJoystick == indexJoystick
                    && map.m_indexButton   == indexButton
                ) {
                    iter.Remove();
                } else {
                    iter.Next();
                }
            }
        }

        ZString GetString(InputEngine* pinputEngine, bool bInternational)
        {
            ZString str;
            bool bFirst = true;

            //
            // Key mappings
            //

            {
                TList<KeyMapping>::Iterator iter(m_listKeyMapping);

                while (!iter.End()) {
                    KeyMapping& map = iter.Value();

                    if (bFirst) {
                        bFirst = false;
                    } else {
                        str += ", ";
                    }

                    if (map.m_modifiers & ModifierShift) {
                        str += "Shift+";
                    }

                    if (map.m_modifiers & ModifierControl) {
                        str += "Control+";
                    }

                    if (map.m_modifiers & ModifierAlt) {
                        str += "Alt+";
                    }

                    char buf[128];

                    UINT scan   = ::MapVirtualKey(map.m_vk, 0) << 16;
#pragma warning(disable:4293)
                    if (scan & (1 << 32)) {
                        scan |= (1 << 24);
                        scan &= ~(1 << 23);
                    }
#pragma warning(default:4293)

                    int length = 0;

                    if (bInternational) {
                        length = ::GetKeyNameText(scan, buf, 128);
                    }

                    if (length == 0) {
                        str += g_pkeyInfo[map.m_vk].m_pcc;
                    } else {
                        str += ZString(buf, length);
                    }

                    iter.Next();
                }
            }

            //
            // Button mappings
            //

            {
                TList<ButtonMapping>::Iterator iter(m_listButtonMapping);

                while (!iter.End()) {
                    ButtonMapping& map = iter.Value();

                    if (bFirst) {
                        bFirst = false;
                    } else {
                        str += ", ";
                    }

                    if (map.m_indexJoystick == -1) {
                        if (map.m_indexButton == 8) //Imago 8/14/09 mouse wheel
                            str += "Wheel Down";
                        else if (map.m_indexButton == 9)
                            str += "Wheel Up";
                        else
                            str += "Mouse Btn " + ZString(map.m_indexButton + 1);
                    } else {
                        str += "Joy " + ZString(map.m_indexJoystick);

                        switch(map.m_indexButton) {
                            case -1: str += " Hat Forward"                          ; break;
                            case -2: str += " Hat Right"                            ; break;
                            case -3: str += " Hat Backward"                         ; break;
                            case -4: str += " Hat Left    "                         ; break;
                            default: str += " Btn " + ZString(map.m_indexButton + 1); break;
                        }
                    }

                    iter.Next();
                }
            }

            //
            // return the string
            //

            return str;
        }

        void FillModifierMap(TArray<bool, 256>& pbNoModifiers)
        {
            TList<KeyMapping>::Iterator iter(m_listKeyMapping);

            while (!iter.End()) {
                KeyMapping& map = iter.Value();

                if (map.m_modifiers != ModifierNone) {
                    pbNoModifiers[map.m_vk] = false;
                }

                iter.Next();
            }
        }

        void WriteKeyboardMapping(ZFile* pfile, int cmd, bool& bFirst, TArray<bool, 256>& pbNoModifiers)
        {
            {
                TList<KeyMapping>::Iterator iter(m_listKeyMapping);

                while (!iter.End()) {
                    KeyMapping& map = iter.Value();

                    if (bFirst) {
                        bFirst = false;
                    } else {
                        pfile->Write(ZString(",\n"));
                    }

                    int modifiers = map.m_modifiers;

                    if (
                           modifiers == ModifierNone
                        && pbNoModifiers[map.m_vk]
                    ) {
                        modifiers = ModifierAny;
                    }

                    pfile->Write(
                          "        (" 
                        + ZString(map.m_vk) 
                        + ", " 
                        + ZString(modifiers)
                        + ", "
                        + ZString(cmd)
                        + ")"
                    );

                    iter.Next();
                }
            }
        }

        void WriteMouseButtonMapping(ZFile* pfile, int cmd, bool& bFirst)
        {
            TList<ButtonMapping>::Iterator iter(m_listButtonMapping);

            while (!iter.End()) {
                ButtonMapping& map = iter.Value();

                if (map.m_indexJoystick == -1) {
                    if (bFirst) {
                        bFirst = false;
                    } else {
                        pfile->Write(ZString(",\n"));
                    }

                    pfile->Write(
                          "        (" 
                        + ZString(map.m_indexButton)
                        + ", "
                        + ZString(cmd)
                        + ")"
                    );
                }

                iter.Next();
            }
        }

        void WriteButtonMapping(ZFile* pfile, int cmd, bool& bFirst)
        {
            TList<ButtonMapping>::Iterator iter(m_listButtonMapping);

            while (!iter.End()) {
                ButtonMapping& map = iter.Value();

                if (map.m_indexJoystick != -1) {
                    if (bFirst) {
                        bFirst = false;
                    } else {
                        pfile->Write(ZString(",\n"));
                    }

                    pfile->Write(
                          "        (" 
                        + ZString(map.m_indexJoystick) 
                        + ", " 
                        + ZString(map.m_indexButton)
                        + ", "
                        + ZString(cmd)
                        + ")"
                    );
                }

                iter.Next();
            }
        }
    };

    class Axis {
    public:
        int   m_indexJoystick;
        int   m_indexValue;
    };

    class EvaluateImage : public WrapImage {
    private:
        InputMapPopup* m_ppopup;

    public:
        EvaluateImage(InputMapPopup* ppopup, Image* pimage, Number* ptime) :
            WrapImage(pimage, ptime),
            m_ppopup(ppopup)
        {
        }

        void Evaluate()
        {
            m_ppopup->Evaluate();
        }

        //////////////////////////////////////////////////////////////////////////////
        //
        // Intercept all of the mouse input while editing
        //
        //////////////////////////////////////////////////////////////////////////////

        MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
        { 
            MouseResult result = WrapImage::HitTest(pprovider, point, bCaptured);

            if (m_ppopup->IsEditing()) {
                result.Set(MouseResultHit());
            }

            return result;
        }

        MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
        { 
            if (m_ppopup->IsEditing()) {
                if (bDown) {
                    m_ppopup->Button(button);
                }
                return MouseResult();
            }
            return WrapImage::Button(pprovider, point, button, bCaptured, bInside, bDown);
        }
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    // members
    //
    //////////////////////////////////////////////////////////////////////////////

    CommandMap                  m_pcommandMap[TK_Max];
    TArray<Axis, countAxis>     m_paxis;
    TVector<float>              m_vvalue;
    TArray<bool, 256>           m_pbNoModifiers;

    TRef<Modeler>               m_pmodeler;
    TRef<InputEngine>           m_pinputEngine;
    TRef<TrekInput>             m_ptrekInput;
    TRef<IEngineFont>           m_pfont;
    TRef<Number>                m_ptime;
    TRef<ModifiableString>      m_pstringText;

    TRef<Pane>                  m_ppane;
    TRef<ButtonPane>            m_pbuttonOK;
    TRef<ButtonPane>            m_pbuttonCancel;
    TRef<ButtonPane>            m_pbuttonRestore;
    TRef<ButtonPane>            m_pbuttonClose;
    TRef<ListPane>              m_plistPane;

    TRef<EventSourceImpl>       m_peventSource;

    int                         m_column;
    Color                       m_colorSelection;
    Color                       m_colorText;
    Color                       m_colorNoMapping;

    bool                        m_bInternational;
    bool                        m_bEditing;
    bool                        m_bQuestion;
    bool                        m_bButton;
    bool                        m_bAxis;
    int                         m_vkQuestion;
    int                         m_modifiersQuestion;
    int                         m_indexJoystickQuestion;
    int                         m_indexButtonQuestion;
    int                         m_indexAxisQuestion;

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    bool IsInternationalKeyboard()
    {
        HKL hkl = ::GetKeyboardLayout(0);

        return (hkl != (HKL)0x04090409);
    }

    InputMapPopup(
        Modeler*     pmodeler,
        InputEngine* pinputEngine,
        TrekInput*   ptrekInput,
        IEngineFont* pfont,
        Number*      ptime
    ) :
        m_pmodeler(pmodeler),
        m_pinputEngine(pinputEngine),
        m_ptrekInput(ptrekInput),
        m_pfont(pfont),
        m_ptime(ptime),
        m_peventSource(new EventSourceImpl()),
        m_bEditing(false),
        m_bQuestion(false),
        m_bButton(false),
        m_bAxis(false)
    {
        m_bInternational = IsInternationalKeyboard();

        //
        // Exports
        //

        TRef<INameSpace> pnsTeamScreenData = GetModeler()->CreateNameSpace("inputmappanedata");
        pnsTeamScreenData->AddMember("text", m_pstringText = new ModifiableString(ZString()));
        SetMessage();

        //
        // Imports
        //

        TRef<INameSpace> pns = pmodeler->GetNameSpace("inputmappane");

        CastTo(m_ppane, pns->FindMember("inputMapPane"));

        //
        // buttons
        //

        CastTo(m_pbuttonOK,       pns->FindMember("okButton"));
        CastTo(m_pbuttonCancel,   pns->FindMember("cancelButton"));
        CastTo(m_pbuttonRestore,  pns->FindMember("restoreButton"));
        CastTo(m_pbuttonClose,    pns->FindMember("closeButton"));

		// mdvalley: Pointers and class names
        AddEventTarget(&InputMapPopup::OnButtonOK,      m_pbuttonOK->GetEventSource());
        AddEventTarget(&InputMapPopup::OnButtonCancel,  m_pbuttonCancel->GetEventSource());
        AddEventTarget(&InputMapPopup::OnButtonRestore, m_pbuttonRestore->GetEventSource());
        //AddEventTarget(OnButtonCancel,  m_pbuttonClose->GetEventSource());

        //
        // details
        //

        m_column         = (int)pns->FindNumber("mappingOffset");
        m_colorText      = pns->FindColor("textColor");
        m_colorNoMapping = pns->FindColor("noMappingColor");
        m_colorSelection = pns->FindColor("selectionColor");

        //
        // the list
        //

        CastTo(m_plistPane, (Pane*)pns->FindMember("listPane"));

		// mdvalley: needs pointer and class
        AddEventTarget(&InputMapPopup::OnDoubleClick, m_plistPane->GetDoubleClickEventSource());

        m_plistPane->SetItemPainter(this);
        m_plistPane->SetList(this);

        //
        // Unload the name space
        //

        pmodeler->UnloadNameSpace(pns);
        
        //
        // Load the current mapping
        //

        if (!LoadMap(INPUTMAP_FILE)) {
            LoadMap(DEFAULTINPUTMAP_FILE);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void SetMessage(const ZString& str = ZString())
    {
        if (str.IsEmpty()) {
            m_pstringText->SetValue("Press Enter to add a mapping or Delete to remove mapping.");
        } else {
            m_pstringText->SetValue(str);
        }
    }

    void Changed()
    {
        m_peventSource->Trigger();
    }

    bool LoadMap(const ZString& str)
    {
        //
        // load the input map
        //

        TRef<INameSpace> pns = GetModeler()->GetNameSpace(str, false);

        if (pns == NULL) {
            return false;
        }

        //
        // Redetect the joysticks
        //

        m_pinputEngine->EnumerateJoysticks();

        //
        // clear the old input map
        //

        int index;
        for (index = 0; index < TK_Max; index++) {
            m_pcommandMap[index].Clear();
        }

        //
        // keyboard mapping
        //

        {
            TRef<IObjectList> plist = pns->FindList("keyCommandMap");

			// BT - 10/17 - Fix for crash when the keyCommandMap corrupted.
			if (plist == nullptr)
				return false;

            plist->GetFirst();
            while (plist->GetCurrent() != NULL) {
                TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                int vk        = (int)GetNumber(ppair->GetNth(0)    );
                int modifiers = (int)GetNumber(ppair->GetNth(1)    );
                int tk        = (int)GetNumber(ppair->GetLastNth(2));

                m_pcommandMap[tk].AddKeyMapping(vk, modifiers);

                plist->GetNext();
            }
        }

        //
        // mouse button mapping
        //

        {
            TRef<IObjectList> plist = pns->FindList("mouseButtonCommandMap");

            if (plist) {
                plist->GetFirst();
                while (plist->GetCurrent() != NULL) {
                    TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                    int index = (int)GetNumber(ppair->GetFirst() );
                    int tk    = (int)GetNumber(ppair->GetSecond());

                    m_pcommandMap[tk].AddButtonMapping(-1, index);

                    plist->GetNext();
                }
            }
        }

        //
        // joystick button mapping
        //


        if (m_pinputEngine->GetJoystick(0) && m_pinputEngine->GetJoystick(0)->GetButtonCount()) { //Imago 8/18/09
            TRef<IObjectList> plist = pns->FindList("buttonCommandMap");

            plist->GetFirst();
            while (plist->GetCurrent() != NULL) {
                TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                int indexJoystick = (int)GetNumber(ppair->GetNth(0)    );
                int indexButton   = (int)GetNumber(ppair->GetNth(1)    );
                int tk            = (int)GetNumber(ppair->GetLastNth(2));

                m_pcommandMap[tk].AddButtonMapping(indexJoystick, indexButton);

                plist->GetNext();
            }
        }

        //
        // joystick value mapping
        //

        if (m_pinputEngine->GetJoystick(0) && m_pinputEngine->GetJoystick(0)->GetValueCount()) { //Imago 8/18/09
            TRef<IObjectList> plist = pns->FindList("numericValuesNew");

            bool bNew = (plist != NULL);

            if (!bNew) {
                plist = pns->FindList("numericValues");
            }
        
            plist->GetFirst();
            for (int indexValueOutput = 0; indexValueOutput < m_paxis.GetCount(); indexValueOutput++) {
                TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

                if (ppair) {
                    m_paxis[indexValueOutput].m_indexJoystick = (int)GetNumber(ppair->GetFirst() );
                    m_paxis[indexValueOutput].m_indexValue    = (int)GetNumber(ppair->GetSecond());

                    if (!bNew) {
                        if (m_paxis[indexValueOutput].m_indexValue == 2) {
                            m_paxis[indexValueOutput].m_indexValue = 3;
                        } else if (m_paxis[indexValueOutput].m_indexValue == 3) {
                            m_paxis[indexValueOutput].m_indexValue = 2;
                        }
                    } 
                } else {
                    m_paxis[indexValueOutput].m_indexJoystick = -1;
                    m_paxis[indexValueOutput].m_indexValue    =  0;
                }

                plist->GetNext();
            }
        }

        //
        // Unload the map
        //

        GetModeler()->UnloadNameSpace(pns);

        return true;
    }

    void SaveMap(const ZString& str)
    {
        TRef<ZFile> pfile = new ZWriteFile(m_pmodeler->GetArtPath() + "/" + str + ".mdl");

        pfile->Write(ZString("use \"commands\";\n"));
        pfile->Write(ZString("use \"keycode\";\n" ));

        //
        // Calculate the modifier map
        //
        
        int  index;
        
        for (index = 0; index < 256; index++) {
            m_pbNoModifiers[index] = true;    
        }

        for (index = 0; index < TK_Max; index++) {
            m_pcommandMap[index].FillModifierMap(m_pbNoModifiers);
        }

        //
        // keyCommandMap
        //

        pfile->Write(ZString("keyCommandMap =\n"  ));
        pfile->Write(ZString("    [\n"            ));

        bool bFirst = true;

        for (index = 0; index < TK_Max; index++) {
            m_pcommandMap[index].WriteKeyboardMapping(pfile, index, bFirst, m_pbNoModifiers);
        }

        if (!bFirst) {
            pfile->Write(ZString("\n"));
        }

        pfile->Write(ZString("    ];\n"            ));

        //
        // mouseButtonCommandMap
        //

        pfile->Write(ZString("mouseButtonCommandMap =\n"));
        pfile->Write(ZString("    [\n"             ));

        bFirst = true;
        for (index = 0; index < TK_Max; index++) {
            m_pcommandMap[index].WriteMouseButtonMapping(pfile, index, bFirst);
        }

        if (!bFirst) {
            pfile->Write(ZString("\n"));
        }

        pfile->Write(ZString("    ];\n"         ));

        //
        // buttonCommandMap
        //

        pfile->Write(ZString("buttonCommandMap =\n"));
        pfile->Write(ZString("    [\n"             ));

        bFirst = true;
        for (index = 0; index < TK_Max; index++) {
            m_pcommandMap[index].WriteButtonMapping(pfile, index, bFirst);
        }

        if (!bFirst) {
            pfile->Write(ZString("\n"));
        }

        pfile->Write(ZString("    ];\n"         ));

        //
        // numericValues
        //

        pfile->Write(ZString("numericValuesNew =\n"));
        pfile->Write(ZString("    [\n"          ));

        for (index = 0; index < m_paxis.GetCount(); index++) {
            pfile->Write(
                   "        ("
                + ZString(m_paxis[index].m_indexJoystick)
                + ", "
                + ZString(m_paxis[index].m_indexValue)
                + ")"
            );

            if (index != m_paxis.GetCount() - 1) {
                pfile->Write(ZString(","));
            }

            pfile->Write(ZString("\n"));
        }

        pfile->Write(ZString("    ];\n"         ));
    }

    void Close()
    {
        if (!m_ptrekInput->LoadMap(INPUTMAP_FILE)) {
            m_ptrekInput->LoadMap(DEFAULTINPUTMAP_FILE);
        }
        m_plistPane->SetList(NULL);
        m_plistPane->SetItemPainter(NULL);
        ClosePopup(this);
    }

    bool OnButtonOK()
    {
        SaveMap(INPUTMAP_FILE);
        Close();
        return true;
    }

    bool OnButtonCancel()
    {
        Close();
        return true;
    }

    bool OnButtonRestore()
    {
        LoadMap(DEFAULTINPUTMAP_FILE);
        Changed();
        return true;
    }

    bool OnDoubleClick()
    {
        EditSelection();
        return true;
    }

    int GetSelectionIndex()
    {
        return GetIndex(m_plistPane->GetSelection());
    }

    bool IsAxisSelection()
    {
        return GetSelectionIndex() >= g_nCommandInfo;
    }

    int GetCommandMapIndex()
    {
        return g_pCommandInfo[GetSelectionIndex()].m_tk;
    }

    int GetAxisIndex()
    {
        return GetSelectionIndex() - g_nCommandInfo;
    }

    void DeleteMapping()
    {
        if (IsAxisSelection()) {
            m_paxis[GetAxisIndex()].m_indexJoystick = -1;
            m_paxis[GetAxisIndex()].m_indexValue    =  0;
        } else {
            TrekKey tk = GetCommandMapIndex();

            if (tk != TK_NoKeyMapping) {
                m_pcommandMap[GetCommandMapIndex()].Clear();
            }
        }

        Changed();
    }

    bool IsEditing()
    {
        return m_bEditing;
    }

    void EditSelection()
    {
        if (IsAxisSelection()) {
            //
            // Record the initial values of all the joystick axis
            //

            m_vvalue.SetCount(0);

            int countJoystick = m_pinputEngine->GetJoystickCount();
            for (int indexJoystick = 0; indexJoystick < countJoystick; indexJoystick++) {
                JoystickInputStream* pjoystick = m_pinputEngine->GetJoystick(indexJoystick);

                int count = pjoystick->GetValueCount();

                for (int index = 0; index < count; index++) {
                    m_vvalue.PushEnd(pjoystick->GetValue(index)->GetValue());
                }
            }
            m_bEditing = true;

            SetMessage("Move desired joystick axis.");
        } else {
            TrekKey tk = GetCommandMapIndex();
            m_bEditing = (tk != TK_NoKeyMapping);

            if (m_bEditing) {
                SetMessage("Type desired key sequence or button.  Escape cancels.");
            }
        }

        Changed();
    }

    void StopEditing()
    {
        m_bEditing  = false;
        m_bQuestion = false;

        SetMessage();
        Changed();
    }

    void RemoveKeyMappings(int vk, int modifiers)
    {
        for (int index = 0; index < TK_Max; index++) {
            m_pcommandMap[index].RemoveKeyMapping(vk, modifiers);
        }
    }

    bool AddKeyMapping(int vk, int modifiers)
    {
        //
        // See if the key is already being used
        //

        for (int index = 0; index < TK_Max; index++) {
            if (m_pcommandMap[index].HasKeyMapping(vk, modifiers)) {
                if (index == GetCommandMapIndex()) {
                    return true;
                }

                //
                // Key is being used
                //

                m_vkQuestion        = vk;
                m_modifiersQuestion = modifiers;
                m_bQuestion         = true;
                m_bButton           = false;
                m_bAxis             = false;

                SetMessage("Key in use. Press Enter to lose old mapping.  Escape cancels");

                return false;
            }
        }

        //
        // Not being used add the mapping
        //

        m_pcommandMap[GetCommandMapIndex()].AddKeyMapping(vk, modifiers);

        return true;
    }

    void RemoveButtonMappings(int indexJoystick, int indexButton)
    {
        for (int index = 0; index < TK_Max; index++) {
            m_pcommandMap[index].RemoveButtonMapping(indexJoystick, indexButton);
        }
    }

    bool AddButtonMapping(int indexJoystick, int indexButton)
    {
        //
        // See if the button is already being used
        //

        for (int index = 0; index < TK_Max; index++) {
            if (m_pcommandMap[index].HasButtonMapping(indexJoystick, indexButton)) {
                if (index == GetCommandMapIndex()) {
                    return true;
                }

                //
                // Button is being used
                //

                m_indexJoystickQuestion = indexJoystick;
                m_indexButtonQuestion   = indexButton;
                m_bQuestion             = true;
                m_bButton               = true;
                m_bAxis                 = false;

                SetMessage("Button in use.  Press Enter to lose old mapping.  Escape cancels");

                return false;
            }
        }

        //
        // Not being used add the mapping
        //

        m_pcommandMap[GetCommandMapIndex()].AddButtonMapping(indexJoystick, indexButton);

        return true;
    }

    void RemoveAxisMapping(int indexJoystick, int indexValue)
    {
        for (int index = 0; index < m_paxis.GetCount(); index++) {
            if (
                   m_paxis[index].m_indexJoystick == indexJoystick
                && m_paxis[index].m_indexValue    == indexValue
            ) {
                m_paxis[index].m_indexJoystick = -1;
                m_paxis[index].m_indexValue    =  0;
            }
        }
    }

    bool AddAxisMapping(int indexJoystick, int indexValue)
    {
        //
        // See if the axis is already being used
        //

        for (int index = 0; index < m_paxis.GetCount(); index++) {
            if (
                   (m_paxis[index].m_indexJoystick == indexJoystick)
                && (m_paxis[index].m_indexValue    == indexValue)
            ) {
                if (index == GetAxisIndex()) {
                    return true;
                }

                //
                // Axis is being used
                //

                m_indexJoystickQuestion = indexJoystick;
                m_indexAxisQuestion     = indexValue;
                m_bQuestion             = true;
                m_bButton               = false;
                m_bAxis                 = true;

                SetMessage("Axis in use.  Press Enter to lose old mapping.  Escape cancels");

                return false;
            }
        }

        //
        // Not being used add the mapping
        //

        m_paxis[GetAxisIndex()].m_indexJoystick = indexJoystick;
        m_paxis[GetAxisIndex()].m_indexValue    = indexValue;

        return true;
    }

    void Button(int index) 
    {
        if (!IsAxisSelection()) {
            if (AddButtonMapping(-1, index)) {
                StopEditing();
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Value methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void Evaluate()
    {
        if (m_bEditing) {
            int countJoystick = m_pinputEngine->GetJoystickCount();
            int vindex        = 0;
			int indexJoystick;
            for (indexJoystick = 0; indexJoystick < countJoystick; indexJoystick++) {
                JoystickInputStream* pjoystick = m_pinputEngine->GetJoystick(indexJoystick);

                if (IsAxisSelection()) {
                    //
                    // axis: check the axis
                    //

                    int count = pjoystick->GetValueCount();

                    for (int index = 0; index < count; index++) {
                        float dvalue = m_vvalue[vindex] - pjoystick->GetValue(index)->GetValue();
                        vindex++;

                        if (dvalue < -0.9 || dvalue > 0.9) {
                            if (AddAxisMapping(indexJoystick, index)) {
                                StopEditing();
                            }
                            return;
                        }
                    }
                } else {
                    //
                    // command: check the buttons
                    //

                    int count = pjoystick->GetButtonCount();

                    for (int index = 0; index < count; index++) {
                        if (pjoystick->IsDown(index)->GetValue()) {
                            if (AddButtonMapping(indexJoystick, index)) {
                                StopEditing();
                            }
                            return;
                        }
                    }
                }
            }

            //
            // Check the virtual buttons
            //

            if (!IsAxisSelection()) {
                JoystickInputStream* pjoystick = m_pinputEngine->GetJoystick(m_paxis[5].m_indexJoystick);

                if (pjoystick) {
                    Number* pvalue = pjoystick->GetValue(m_paxis[5].m_indexValue);

                    if (pvalue) {
                        float value = pvalue->GetValue();

                        if (value > 0.75f) {
                            if (AddButtonMapping(indexJoystick, -3)) {
                                StopEditing();
                            }
                            return;
                        } else if (value > 0.25f) {
                            if (AddButtonMapping(indexJoystick, -2)) {
                                StopEditing();
                            }
                            return;
                        } else if (value > -0.25f) {
                            if (AddButtonMapping(indexJoystick, -1)) {
                                StopEditing();
                            }
                            return;
                        } else if (value > -0.75f) {
                            if (AddButtonMapping(indexJoystick, -4)) {
                                StopEditing();
                            }
                            return;
                        } else if (value > -1.25f) {
                            if (AddButtonMapping(indexJoystick, -3)) {
                                StopEditing();
                            }
                            return;
                        }
                    }
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // List methods
    //
    //////////////////////////////////////////////////////////////////////////////

    int GetCount()
    {
        return g_nCommandInfo + m_paxis.GetCount();
    }

    ItemID GetItem(int index)
    {
        return (ItemID)(index + 1);
    }

    int GetIndex(ItemID pitem)
    {
        return int(pitem) - 1;
    }

    ItemID GetNext(ItemID pitem)
    {
        int index = GetIndex(pitem);

        if (index == GetCount() - 1) {
            return 0;
        } else {
            return GetItem(index + 1);
        }
    }

    IEventSource* GetChangedEvent()
    {
        return m_peventSource;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // ItemPainter methods
    //
    //////////////////////////////////////////////////////////////////////////////

    int GetXSize()
    {
        return m_plistPane->XSize();
    }

    int GetYSize()
    {
        return m_pfont->GetHeight();
    }

    void Paint(ItemID itemID, Surface* psurface, bool bSelected, bool bFocus)
    {
        ZString strCommand;
        ZString strMapping;
        int index = GetIndex(itemID);
        bool bNoMapping = false;

        if (index >= g_nCommandInfo) {
            int indexAxis = index - g_nCommandInfo;

            strCommand = g_ppccAxisDescription[indexAxis];

            if (m_paxis[indexAxis].m_indexJoystick != -1) {
                JoystickInputStream* pjoystick = m_pinputEngine->GetJoystick(m_paxis[indexAxis].m_indexJoystick);
                TRef<Number>         pvalueInput;

                if (pjoystick) {
                    pvalueInput = pjoystick->GetValue(m_paxis[indexAxis].m_indexValue);
                }

                if (pvalueInput) {
                    strMapping = 
                          pjoystick->GetShortDescription(m_paxis[indexAxis].m_indexJoystick)
                        + " "
                        + pjoystick->GetValueDescription(m_paxis[indexAxis].m_indexValue);
                } else {
                    strMapping = 
                          "Joy " + ZString(m_paxis[indexAxis].m_indexJoystick)
                        + " Value "
                        + ZString(m_paxis[indexAxis].m_indexValue);
                }
            }
        } else {
            strCommand = g_pCommandInfo[index].m_pcc;
            TrekKey tk = g_pCommandInfo[index].m_tk;

            if (tk != TK_NoKeyMapping) {
                strMapping = m_pcommandMap[tk].GetString(m_pinputEngine, m_bInternational);
            } else {
                bNoMapping = true;
            }
        }

        if (m_bEditing && index == GetSelectionIndex()) {
            if (m_bQuestion) {
                strMapping = "!!!";
            } else {
                strMapping = "???";
            }
        }

        if (bSelected) {
            psurface->FillRect(
                WinRect(0, 0, GetXSize(), GetYSize()),
                m_colorSelection
            );
        }

        if (bNoMapping) {
            psurface->DrawString(m_pfont, m_colorNoMapping, WinPoint(0, 0), strCommand);
        } else {
            psurface->DrawString(m_pfont, m_colorText,      WinPoint(0, 0), strCommand);
        }

        psurface->DrawString(m_pfont, m_colorText, WinPoint(m_column, 0), strMapping);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IPopup methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

    TRef<Image> GetImage(Engine* pengine)
    {
        return 
            new EvaluateImage(
                this,
                CreatePaneImage(pengine, true, GetPane()),
                m_ptime
            );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IKeyboardInput methods
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    { 
        if (ks.bDown) {
            if (m_bQuestion) {
                if (ks.vk == VK_ESCAPE) {
                    StopEditing();
                } else if (ks.vk == VK_RETURN) {
                    if (m_bButton) {
                        RemoveButtonMappings(m_indexJoystickQuestion, m_indexButtonQuestion);
                        AddButtonMapping(m_indexJoystickQuestion, m_indexButtonQuestion);
                    } else if (m_bAxis) {
                        RemoveAxisMapping(m_indexJoystickQuestion, m_indexAxisQuestion);
                        AddAxisMapping(m_indexJoystickQuestion, m_indexAxisQuestion);
                    } else {
                        RemoveKeyMappings(m_vkQuestion, m_modifiersQuestion);
                        AddKeyMapping(m_vkQuestion, m_modifiersQuestion);
                    }
                    
                    StopEditing();
                }
            } else if (m_bEditing) {
                if (ks.vk == VK_ESCAPE) {
                    StopEditing();
                } else if (
                       ks.vk != VK_MENU
                    && ks.vk != VK_CONTROL
                    && ks.vk != VK_SHIFT
                ) {
                    if (!IsAxisSelection()) {
                        //
                        // Calculate the modifiers
                        //

                        int modifiers = 0;

                        if (ks.bAlt) {
                            modifiers |= ModifierAlt;
                        }

                        if (ks.bShift) {
                            modifiers |= ModifierShift;
                        }

                        if (ks.bControl) {
                            modifiers |= ModifierControl;
                        }

                        if (AddKeyMapping(ks.vk, modifiers)) {
                            StopEditing();
                        }
                    }
                }
            } else {
                switch (ks.vk) {
                    case VK_ESCAPE:
                        OnButtonCancel();
                        break;

                    case VK_DELETE:
                        DeleteMapping();
                        break;

                    case VK_RETURN:
                        EditSelection();
                        break;

                    case VK_SPACE:
                        OnButtonOK();
                        break;

                    default:
                        m_plistPane->OnKey(pprovider, ks, fForceTranslate);
                        break;
                }
            }
        }

        return true;
    }
};

TRef<IPopup> CreateInputMapPopup(
    Modeler*     pmodeler,
    InputEngine* pinputEngine,
    TrekInput*   ptrekInput,
    IEngineFont* pfont,
    Number*      ptime
) {
    return 
        new InputMapPopup(
            pmodeler,
            pinputEngine,
            ptrekInput,
            pfont,
            ptime
        );
}
