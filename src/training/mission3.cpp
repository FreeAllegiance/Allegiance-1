/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	Mission3.cpp
**
**  Author:
**
**  Description:
**      Implementation of the first training mission
**
**  History:
*/
#include    "pch.h"
#include    "Mission3.h"

#include    "Training.h"
#include    "GoalList.h"
#include    "Predicate.h"
#include    "TrueCondition.h"
#include    "FalseCondition.h"
#include    "AndCondition.h"
#include    "OrCondition.h"
#include    "NotCondition.h"
#include    "PeriodicCondition.h"
#include    "ElapsedTimeCondition.h"
#include    "ObjectMovingTowardsCondition.h"
#include    "ObjectWithinRadiusCondition.h"
#include    "ObjectPointingAtCondition.h"
#include    "ConditionalAction.h"
#include    "ConditionList.h"
#include    "ActionList.h"
#include    "MessageAction.h"
#include    "PlaySoundAction.h"
#include    "SetCommandAction.h"
#include    "NoAction.h"
#include    "NTimesCondition.h"
#include    "EndMissionAction.h"
#include    "ResetAction.h"
#include    "ResetShipAction.h"
#include    "SoundFinishedCondition.h"
#include    "SetDisplayModeAction.h"
#include    "TurnShipAboutCondition.h"
#include    "GetRadarLODCondition.h"
#include    "SetRadarLODAction.h"
#include    "GetControlActiveCondition.h"
#include    "GetStateBitsCondition.h"
#include    "GetShipIsStoppedCondition.h"
#include    "GetShipIsDestroyedCondition.h"
#include    "SetControlConstraintsAction.h"
#include    "SetControlsAction.h"
#include    "GetAutopilotCondition.h"
#include    "SetAutopilotAction.h"
#include    "SetSignatureAction.h"
#include    "GetTargetCondition.h"
#include    "CommandAcknowledgedCondition.h"
#include    "GetCommandCondition.h"
#include    "ReleaseConsumerAction.h"
#include    "CreateProbeAction.h"
#include    "CreateWaypointAction.h"
#include    "SetHUDOverlayAction.h"
#include    "GetKeyCondition.h"
#include    "ProxyCondition.h"
#include    "TurnToAction.h"
#include    "ToggleWeaponAction.h"
#include    "SwapWeaponAction.h"
#include    "DoDamageAction.h"
#include    "CurrentTarget.h"
#include    "PlayerShipTarget.h"
#include    "RepairRearmAction.h"
#include    "ShowPaneAction.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  Mission3::~Mission3(void)
    {
    }

    //------------------------------------------------------------------------------
    int         Mission3::GetMissionID(void)
    {
        return c_TM_3_Basic_Weaponry;
    }

    //------------------------------------------------------------------------------
    SectorID    Mission3::GetStartSectorID(void)
    {
        return 1010;
    }

    //------------------------------------------------------------------------------
    bool        Mission3::RecordKeyPress(TrekKey key)
    {
        switch (key)
        {
        case TK_TargetCenter:
            if (trekClient.flyingF())
            {
                IshipIGC*           pShip = trekClient.GetShip();
                const Orientation*  pOrientation = &(pShip->GetOrientation());

                ImodelIGC*          pTarget = FindTarget(pShip,
                    c_ttFront | c_ttShip | c_ttEnemy,
                    pShip->GetCommandTarget(c_cmdCurrent),
                    NULL, NULL, pOrientation);

                GetWindow()->SetTarget(pTarget, c_cidNone);
            }
            return false;
        case TK_SwapWeapon1:
        case TK_SwapWeapon2:
        case TK_SwapWeapon3:
        case TK_SwapWeapon4:
        case TK_SwapTurret1:
        case TK_SwapTurret2:
        case TK_SwapTurret3:
        case TK_SwapTurret4:
        case TK_SwapMissile:
        case TK_SwapChaff:
        case TK_SwapShield:
        case TK_SwapCloak:
        case TK_SwapAfterburner:
        case TK_SwapMine:
        case TK_NextCargo:
        case TK_ToggleWeapon1:
        case TK_ToggleWeapon2:
        case TK_ToggleWeapon3:
        case TK_ToggleWeapon4:
        case TK_DropCargo:
        case TK_Reload:
        case TK_ToggleGroupFire:
        case TK_PrevWeapon:
        case TK_NextWeapon:
        case TK_PrevPart:
        case TK_NextPart:
        case TK_MountPart:
        case TK_ReplacePart:
        case TK_UnmountPart:
        case TK_DropPart:
        {
            // only allow if the inventory pane is up
            TrekWindow* pWindow = GetWindow();
            if (pWindow->GetOverlayFlags() bit_and ofInventory)
                return true;
            return false;
        }
        }
        return TrainingMission::RecordKeyPress(key);
    }

    //------------------------------------------------------------------------------
    bool        Mission3::ShipLanded(void)
    {
        Terminate();
        return false;
    }

    //------------------------------------------------------------------------------
    void        Mission3::CreateUniverse(void)
    {
        LoadUniverse("training_1", 414, 1010);    // a fighter 3

                                                  // stuff for setup
        ImissionIGC*    pCore = trekClient.GetCore();
        Time            now = pCore->GetLastUpdate();
        IshipIGC*       pShip = trekClient.GetShip();
        ImodelIGC*      pStation = pCore->GetModel(OT_station, 1010);

        // take one of the guns off the ship
        IpartIGC*       pPart = pShip->GetMountedPart(ET_Weapon, 0);
        if (pPart)
            pPart->Terminate();

        // put an anti-shield weapon on the ship
        AddPartToShip(92, 0, 0);

        // take the missiles off the ship
        pPart = pShip->GetMountedPart(ET_Magazine, 0);
        if (pPart)
            pPart->Terminate();

        // activate only the starting weapon
        trekClient.SetSelectedWeapon(0);
        trekClient.fGroupFire = false;

        // set a global attribute to reduce overall energy of ship
        IsideIGC*       pSide = pShip->GetSide();
        GlobalAttributeSet  gas = pSide->GetGlobalAttributeSet();
        gas.SetAttribute(c_gaMaxEnergy, 0.25f);
        pSide->SetGlobalAttributeSet(gas);

        // put the commander ship in the station
        IshipIGC*       pCommander = pCore->GetShip(m_commanderID);
        pCommander->SetStation(static_cast<IstationIGC*> (pStation));
        pCommander->SetCommand(c_cmdAccepted, NULL, c_cidDoNothing);
        pCommander->SetCommand(c_cmdCurrent, NULL, c_cidDoNothing);
        pCommander->SetAutopilot(false);

        // create a bunch of derelict hulls in a sphere around the ship
        // at about 400m to 600m distance
        Vector  direction(1.0f, 0.0f, 0.0f);
        for (int i = 0; i < TM_NUMBER_OF_DERELICTS; i++)
        {
            float   fRadius = 400.0f + (static_cast<float> (i) * 20.0f);
            Vector  newPosition = direction * fRadius;
            HullID  hullID;
            switch (randomInt(0, 2))
            {
            case 0:
                hullID = 800;
                break;
            case 1:
            case 2:
                hullID = 801;
                break;
            }
            char        name[32];
            sprintf(name, "Derelict %02d", i + 1);
            m_derelict_hulls[i] = pCore->GenerateNewShipID();
            IshipIGC*   pShip = CreateDrone(name, m_derelict_hulls[i], hullID, 1, c_ptPlayer);
            pShip->SetAutopilot(false);
            pShip->SetPosition(newPosition);
            // XXX I hope this works out - if a random vector is generated that is the same as direction, we're in trouble
            pShip->SetOrientation(Orientation(pShip->GetOrientation().GetForward(), direction * -1.0f));
            pShip->SetCluster(pCore->GetCluster(1010));
            direction = Vector::RandomDirection();
        }
    }

    //------------------------------------------------------------------------------
    Condition*  Mission3::CreateMission(void)
    {
        // create the goal and the goal list
        GoalList*                       pGoalList = new GoalList;
        Goal*                           pGoal = new Goal(pGoalList);

        // put a constraint that will point the ship back in if it's more than 7500m from the base
        ImodelIGC*                      pShip = static_cast<ImodelIGC*> (trekClient.GetShip());
        TurnToAction*                   pTurnToAction = new TurnToAction(pShip, OT_station, 1010);
        ObjectWithinRadiusCondition*    pObjectWithinRadiusCondition = new ObjectWithinRadiusCondition(pShip, OT_station, 1010, 7500.0f);
        ObjectWithinRadiusCondition*    pObjectWithinRadiusCondition2 = new ObjectWithinRadiusCondition(pShip, OT_station, 1010, 7000.0f);
        ConditionalAction*              pConditionalAction = new ConditionalAction(new NotCondition(pObjectWithinRadiusCondition), pTurnToAction);
        ConditionalAction*              pConditionalAction2 = new ConditionalAction(new AndCondition(new NotCondition(pObjectWithinRadiusCondition2), new ProxyCondition(pObjectWithinRadiusCondition)), new SetControlsAction);
        pGoal->AddConstraintCondition(pConditionalAction);
        pGoal->AddConstraintCondition(pConditionalAction2);

        // add the subgoals
        pGoalList->AddGoal(CreateGoal01());
        pGoalList->AddGoal(CreateGoal02());
        pGoalList->AddGoal(CreateGoal03());
        pGoalList->AddGoal(CreateGoal04());
        pGoalList->AddGoal(CreateGoal05());
        pGoalList->AddGoal(CreateGoal06());
        pGoalList->AddGoal(CreateGoal07());
        pGoalList->AddGoal(CreateGoal08());
        pGoalList->AddGoal(CreateGoal09());
        pGoalList->AddGoal(CreateGoal10());
        pGoalList->AddGoal(CreateGoal11());
        pGoalList->AddGoal(CreateGoal12());
        pGoalList->AddGoal(CreateGoal13());

        // return the goal
        return pGoal;
    }

    //------------------------------------------------------------------------------
    Condition*  Mission3::CreateGotTargetCondition(void)
    {
        // generate the array of leaf conditions and fill it in
        Condition*      pTargetCondition[TM_NUMBER_OF_DERELICTS];
        for (int i = 0; i < TM_NUMBER_OF_DERELICTS; i++)
            pTargetCondition[i] = new GetTargetCondition(trekClient.GetShip(), OT_ship, m_derelict_hulls[i]);

        // walk the array of leaf conditions pairing them using Or conditions
        Condition*      pTmpCondition = pTargetCondition[0];
        for (int i = 1; i < TM_NUMBER_OF_DERELICTS; i++)
            pTmpCondition = new OrCondition(pTmpCondition, pTargetCondition[i]);

        // return the last Or condition generated
        return pTmpCondition;
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal01(void)
    {
        GoalList*   pGoalList = new GoalList;
        // play the introductory audio
        {
            Goal*                           pGoal = CreatePlaySoundGoal(salCommenceScanSound);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->DisableInputAction(0xffffffff ^ selectedWeaponMaskIGC);
            pSetControlConstraintsAction->ScaleInputControl(c_axisYaw, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisPitch, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisRoll, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisThrottle, 0.0f);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait two seconds
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(2.0f)));

        // tm_3_01
        // This ship is a fighter. It is well armed and has a moderate 
        // scan range.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_01Sound));

        // wait one second, zoom into ship
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.7f));
            pGoal->AddStartAction(new SetDisplayModeAction(TrekWindow::cmCockpit));
            pGoalList->AddGoal(pGoal);
        }

        // tm_3_02
        // In this mission, we're going to go over targeting with the 
        // peripheral radar and the weapon systems carried by your 
        // ships.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_02Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_03
        // As before, your controls will be disabled until I ask you 
        // to do something, so listen carefully.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_03Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal02(void)
    {
        GoalList*           pGoalList = new GoalList;

        // tm_3_04
        // First, notice the two gauges in the center of your heads-up 
        // display: the shield and armor gauges. These gauges indicate 
        // the relative health of your craft.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_04Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_05
        // The shield gauge shows how much damage the shields can 
        // absorb before you will start taking armor damage. Shields 
        // can regenerate, so it's not a disaster if this gauge shows 
        // that your shields are offline. When you take shield damage, 
        // it will look like this.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_05Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(ShieldTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait one second and show shield damage
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(1.0f));
            pGoal->AddConstraintCondition(new ConditionalAction(new TrueCondition, new DoDamageAction(false)));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_06
        // Now notice that the shield regenerates.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_06Sound));

        // wait four seconds
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(3.0f)));

        // tm_3_07
        // The armor gauge shows how much more damage your ship hull 
        // can withstand before breaking apart. If this gauge drops to 
        // 0, you will explodeÿ. This is what it will look like when 
        // you take armor damage.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_07Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(HullTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait one second and show hull damage
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(1.0f));
            pGoal->AddConstraintCondition(new ConditionalAction(new TrueCondition, new DoDamageAction(true)));
            pGoalList->AddGoal(pGoal);
        }

        // wait one and a half seconds
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(1.5f)));

        // tm_3_08
        // Notice that the armor does not regenerate.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_08Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.3f)));

        // tm_3_09
        // If you're in a battle and you get damaged, try to return to 
        // the nearest friendly station for repairs or search for a 
        // shield and armor power-up. 
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_09Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal03(void)
    {
        GoalList*   pGoalList = new GoalList;

        // tm_3_10
        // There are a number of derelict hulls in this sector of 
        // space. We'll use these for target practice.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_10Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_11
        // As your first introduction to targeting, you're going to 
        // learn to target objects in front of the craft. 
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_11Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_11r
        // Press the C key to target objects nearest to the center of 
        // your display.
        {
            Goal    *pGoal = new Goal(CreateGotTargetCondition());
            pGoal->AddStartAction(new MessageAction("Press the middle mouse button to target the objects in the center of your display."));
            //pGoal->AddStartAction (new PlaySoundAction (tm_3_11rSound)); - removed as it doesn't match the keymap
            pGoal->AddStartAction(new SetHUDOverlayAction(TargetCenterTrainingOverlay));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_11Sound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_12
        // Good! I'm enabling your flight controls now. Practice 
        // turning and targeting the derelict hulls with the C key.
        {
            Goal*                           pGoal = CreatePlaySoundGoal(tm_3_12Sound);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->ScaleInputControl(c_axisYaw, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisPitch, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisRoll, 1.0f);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_12r
        // When you are comfortable with targeting, press the SPACEBAR 
        // to proceed.
        {
            Goal*   pGoal = new Goal(new GetKeyCondition(TK_ThrustForward));
            pGoal->AddStartAction(new MessageAction("Press the SPACEBAR when you are ready to proceed."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_12rSound));
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_12rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal04(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_13
        // All ships have a scan range, within which you can detect 
        // the presence of other objects. Some ships, like the 
        // interceptor we flew in the first mission, have a very poor 
        // range. These ships are not very good in unexplored space 
        // because an enemy can sneak up on them quite easily. 
        // However, if a friendly ship can see something in its scan 
        // range, you can too. It's always handy to have a scout or 
        // two around when you're in uncharted regions, because they 
        // have a very large scan range.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_13Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // XXX target something if there isn't a target?

        // tm_3_14
        // Now examine the portion of your heads-up display that shows 
        // a detail of the target. Notice that this area displays the 
        // shield and armor strength of your target, as well as its 
        // velocity and range.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_14Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(TargetTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_15
        // Press the SPACEBAR when you are ready to proceed.
        {
            Goal*   pGoal = new Goal(new GetKeyCondition(TK_ThrustForward));
            pGoal->AddStartAction(new MessageAction("Press the SPACEBAR when you are ready to proceed."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_15Sound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_15Sound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal05(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_16
        // When you target an enemy craft, you will see an aiming 
        // reticule in the center of your display. This reticule will 
        // be green when you are in range and aiming at the target. If 
        // you are not aiming correctly, or you are not in range, the 
        // reticule will be gray. Your ship will now turn to show you 
        // this effect.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_16Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(ReticuleTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // turn the craft until it lines up
        {
            ObjectPointingAtCondition*  pObjectPointingAtCondition = new ObjectPointingAtCondition(static_cast<ImodelIGC*> (trekClient.GetShip()), new CurrentTarget(new PlayerShipTarget));
            Goal*                       pGoal = new Goal(pObjectPointingAtCondition);
            pObjectPointingAtCondition->SetMinimumAngle(1.0f);
            TurnToAction*               pTurnToAction = new TurnToAction(trekClient.GetShip(), new CurrentTarget(new PlayerShipTarget));
            ConditionalAction*          pConditionalAction = new ConditionalAction(new TrueCondition, pTurnToAction);
            pGoal->AddConstraintCondition(pConditionalAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second, and turn to the right
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputControl(c_axisYaw, -0.5f);
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // turn the craft until it lines up
        {
            ObjectPointingAtCondition*  pObjectPointingAtCondition = new ObjectPointingAtCondition(static_cast<ImodelIGC*> (trekClient.GetShip()), new CurrentTarget(new PlayerShipTarget));
            Goal*                       pGoal = new Goal(pObjectPointingAtCondition);
            pObjectPointingAtCondition->SetMinimumAngle(1.0f);
            TurnToAction*               pTurnToAction = new TurnToAction(trekClient.GetShip(), new CurrentTarget(new PlayerShipTarget));
            ConditionalAction*          pConditionalAction = new ConditionalAction(new TrueCondition, pTurnToAction);
            pGoal->AddConstraintCondition(pConditionalAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second, and reset the controls
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // tm_3_17
        // Okay. There are basically three weapon types: weapons that 
        // use energy, weapons that use ammunition, and missiles. Your 
        // craft is currently armed with all three. To start with, 
        // only the energy-based weapon is activated.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_17Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        {
            Goal*                           pGoal = new Goal(new ElapsedTimeCondition(0.2f));
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->EnableInputAction(weaponsMaskIGC);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait for weapons to fire
        {
            Goal*   pGoal = new Goal(new GetStateBitsCondition(trekClient.GetShip(), weaponsMaskIGC));
            if (GetEngineWindow()->GetInputEngine()->GetJoystickCount() > 0)
            {
                // tm_3_18
                // Press the trigger button on your joystick to fire your main 
                // weapon.
                pGoal->AddStartAction(new PlaySoundAction(tm_3_18Sound));
                pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_18Sound));
                pGoal->AddStartAction(new SetHUDOverlayAction(FireGunsTrainingOverlay));
                pGoal->AddStartAction(new MessageAction("Fire the main weapon by pressing the trigger button on your joystick, or left click if using a mouse"));
            }
            else
            {
                // tm_3_19
                // Press the SPACEBAR on the keyboard to fire your main weapon.
                //pGoal->AddStartAction (new PlaySoundAction (tm_3_19Sound)); - voiceover does not match keymap
                pGoal->AddStartAction(new SetHUDOverlayAction(FireGunsTrainingOverlay));
                pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_19Sound));
                pGoal->AddStartAction(new MessageAction("Left click to fire your main weapon."));
            }
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_20
        // Good!
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_20Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal06(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_21
        // Notice the energy gauge on your heads-up display. This 
        // gauge shows how much energy is available to power your gun. 
        // As you fire, energy is used up. This gauge shows you how 
        // much energy remains.
        {
            Goal*               pGoal = CreatePlaySoundGoal(tm_3_21Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(EnergyTrainingOverlay));
            /*
            SetControlsAction*  pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputAction (oneWeaponIGC);
            pGoal->AddStartAction (pSetControlsAction);
            */
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_3_22
        // One benefit of using energy-based weapons is that the 
        // energy available to your ship will recharge. In effect, you 
        // never run out.
        {
            Goal*               pGoal = CreatePlaySoundGoal(tm_3_22Sound);
            /*
            SetControlsAction*  pSetControlsAction = new SetControlsAction;
            pGoal->AddStartAction (pSetControlsAction);
            */
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_3_23
        // Notice how the shield and armor strength of your target are 
        // affected. 
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_23Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(TargetTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_23r
        // Keep firing until your target is destroyed.
        {
            Goal*   pGoal = new Goal(new GetShipIsDestroyedCondition(new CurrentTarget(new PlayerShipTarget, true)));
            pGoal->AddStartAction(new MessageAction("Destroy current target."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_23rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_23rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_24
        // Great. As you can see, stationary objects are easy 
        // pickings. 
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_24Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal07(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_25
        // Notice that your radar has targeted another derelict hull. 
        // Whenever an item that you have targeted goes away, the 
        // radar will automatically target the next item of the same 
        // type.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_25Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_26
        // You can selectively target the nearest friendly craft and 
        // the nearest enemy craft. To target the nearest enemy, press 
        // the E key. To target the nearest friendly object, press the 
        // F key.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_26Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_27
        // Select a new target now using the E key.
        {
            Goal*   pGoal = new Goal(new GetKeyCondition(TK_TargetEnemy));
            pGoal->AddStartAction(new MessageAction("Target an enemy using the T key."));
            pGoal->AddStartAction(new SetHUDOverlayAction(TargetEnemyTrainingOverlay));
            //pGoal->AddStartAction (new PlaySoundAction (tm_3_27Sound)); - does not match keymap
            //pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_3_27Sound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_28
        // Notice your radar. The targeted object has a set of square 
        // brackets around it, and there is a red arrow near the 
        // center of your display that points at the target. You can 
        // use this information to help you locate your target in 
        // space.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_28Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_29
        // Turn to face your new target.
        {
            ObjectPointingAtCondition*  pObjectPointingAtCondition = new ObjectPointingAtCondition(static_cast<ImodelIGC*> (trekClient.GetShip()), new CurrentTarget(new PlayerShipTarget));
            Goal*   pGoal = new Goal(pObjectPointingAtCondition);
            pGoal->AddStartAction(new PlaySoundAction(tm_3_29Sound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_29Sound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_30
        // Excellent! You're getting the hang of this!
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_30Sound));
        {
            Goal* pGoal = new Goal(new ElapsedTimeCondition(0.1f));
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal08(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_31
        // I am activating your machine gun now. 
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_31Sound);
            pGoal->AddStartAction(new ToggleWeaponAction(1));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_31r
        // Go ahead and fire.
        {
            Goal*   pGoal = new Goal(new GetStateBitsCondition(trekClient.GetShip(), weaponsMaskIGC));
            if (GetEngineWindow()->GetInputEngine()->GetJoystickCount() > 0)
                pGoal->AddStartAction(new MessageAction("Fire the main weapon by pressing the trigger button on your joystick."));
            else
                pGoal->AddStartAction(new MessageAction("Left click to fire your main weapon."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_31rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_31rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_32
        // Good!
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_32Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_33
        // The machine gun uses ammunition. This gauge shows how much 
        // ammunition remains. Note that the ammunition is not 
        // recharged until you land again, so watch your supply.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_33Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(AmmoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_34
        // Blow up your target with the machine guns.
        {
            Goal*   pGoal = new Goal(new GetShipIsDestroyedCondition(new CurrentTarget(new PlayerShipTarget, true)));
            pGoal->AddStartAction(new MessageAction("Destroy current target."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_34Sound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_34Sound));
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_35
        // Now turn to face the next targeted derelict hull.
        {
            ObjectPointingAtCondition*  pObjectPointingAtCondition = new ObjectPointingAtCondition(static_cast<ImodelIGC*> (trekClient.GetShip()), new CurrentTarget(new PlayerShipTarget));
            Goal*                       pGoal = new Goal(pObjectPointingAtCondition);
            pGoal->AddStartAction(new PlaySoundAction(tm_3_35Sound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_35Sound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal09(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_36
        // Perfect! I'm going to load a missile launcher from cargo. 
        // When the load is complete, you will see a new set of 
        // brackets around this target. These brackets indicate the 
        // quality of the lock you have on your target. When the lock 
        // is perfect, you will hear a special sound. Missiles will 
        // turn to follow targets when they have a lock, so you don't 
        // need to shoot quite as straight.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_36Sound);
            pGoal->AddStartAction(new SwapWeaponAction(ET_Magazine, 0));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // wait for missilies to fire
        {
            Goal*   pGoal = new Goal(new GetStateBitsCondition(trekClient.GetShip(), missileFireIGC));
            if (GetEngineWindow()->GetInputEngine()->GetJoystickCount() > 0)
            {
                // tm_3_37
                // Fire your missiles by pressing button 2 on your joystick.
                pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_37Sound));
                pGoal->AddStartAction(new MessageAction("Fire missiles by pressing the #2 button on your joystick."));
                pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_37Sound));
            }
            else
            {
                // tm_3_38
                // Fire your missiles by pressing CONTROL plus SPACEBAR on the 
                // keyboard.
                //pGoalList->AddGoal (CreatePlaySoundGoal (tm_3_38Sound)); - does not match keymap
                pGoal->AddStartAction(new MessageAction("Right click to fire your secondary weapon."));
                pGoal->AddStartAction(new SetHUDOverlayAction(MissileTrainingOverlay));
                pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_38Sound));
            }
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->EnableInputAction(missileFireIGC);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_39
        // Watch how these affect the target!
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_39Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_23r
        // Keep firing until your target is destroyed.
        {
            Goal*   pGoal = new Goal(new GetShipIsDestroyedCondition(new CurrentTarget(new PlayerShipTarget, true)));
            pGoal->AddStartAction(new MessageAction("Destroy current target."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_23rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_23rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_40
        // Good job.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_40Sound));
        {
            Goal* pGoal = new Goal(new ElapsedTimeCondition(0.1f));
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal10(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_41
        // This is the Inventory display. I?ve been using your 
        // inventory controls to enable and disable weapons and to 
        // load missiles from cargo. You can do this yourself by 
        // pressing the corresponding number keys to enable or disable 
        // the items and CONTROL plus the number keys to shift items 
        // to and from cargo.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_41Sound);
            pGoal->AddStartAction(new ShowPaneAction(ofInventory));
            pGoal->AddStartAction(new SetHUDOverlayAction(InventoryTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_42
        // Check that out and make sure you are comfortable with it.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_42Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_3_42r
        // Press the END key when you are ready to proceed.
        {
            Goal*   pGoal = new Goal(new GetKeyCondition(TK_CommandFinishTM));
            pGoal->AddStartAction(new MessageAction("Press the END key when you are comfortable with the Inventory Display."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_42rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_42rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_43
        // One more thing you?ll need to know about is your craft?s 
        // kill bonus.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_3_43Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(KillBonusTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_44
        // This number represents the percentage of extra damage your 
        // craft is doing against enemies. This number will increase 
        // every time you get a kill but will drop back to 0 every 
        // time you die.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_44Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal11(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_45
        // You've done well, Cadet! Here?s a chance to put your 
        // learning to the test ? I'm going to give you a full load of 
        // ammo and missiles now and enable all your controls. Fly 
        // around the derelicts and destroy them, use your targeting 
        // keys, throttle, and missiles.
        {
            Goal*                           pGoal = CreatePlaySoundGoal(tm_3_45Sound);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->EnableInputAction(0xffffffff);
            pSetControlConstraintsAction->ScaleInputControl(c_axisThrottle, 1.0f);
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoal->AddStartAction(new ToggleWeaponAction(1));
            pGoal->AddStartAction(new RepairRearmAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_46
        // When you are ready to go home, press the END key.
        {
            Goal*   pGoal = new Goal(new GetKeyCondition(TK_CommandFinishTM));
            pGoal->AddStartAction(new MessageAction("Press the END key when you are ready to go home."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_46Sound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(120.0f, tm_3_46Sound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal12(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_47
        // All right, you are going to land the ship yourself this 
        // time, Cadet. Remember what I told you about landing in the 
        // green bays.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_47Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_3_48
        // Press the B key to target your nearest base.
        {
            Goal*           pGoal = new Goal(new GetTargetCondition(trekClient.GetShip(), OT_station, 1010));
            pGoal->AddStartAction(new MessageAction("Press the B key to target the outpost."));
            pGoal->AddStartAction(new PlaySoundAction(tm_3_48Sound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_48Sound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission3::CreateGoal13(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_3_49
        // Good! Fly toward the base, and locate and land in a green 
        // hangar. It might help if you fly close to the base, then 
        // stop and line yourself up with the bay. That's how 
        // autopilot works.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_3_49Sound));

        // tm_3_49r
        // Land the ship in a green hangar bay to complete the mission.
        {
            Goal*   pGoal = new Goal(new FalseCondition);
            pGoal->AddStartAction(new MessageAction("Land in a green hangar to complete the mission."));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_3_49rSound));
            pGoalList->AddGoal(pGoal);
        }

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
}
