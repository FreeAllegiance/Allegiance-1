/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	Mission2.cpp
**
**  Author:
**
**  Description:
**      Implementation of the very first training mission
**
**  History:
*/
#include    "pch.h"
#include    "Mission2.h"

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
#include    "CreateDroneAction.h"
#include    "CreateWaypointAction.h"
#include    "SetHUDOverlayAction.h"
#include    "GetKeyCondition.h"
#include    "TurnToAction.h"
#include    "ProxyCondition.h"
#include    "missionIGC.h"


namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  Mission2::~Mission2(void)
    {
    }

    //------------------------------------------------------------------------------
    int         Mission2::GetMissionID(void)
    {
        return c_TM_2_Basic_Flight;
    }

    //------------------------------------------------------------------------------
    SectorID    Mission2::GetStartSectorID(void)
    {
        return 1010;
    }

    //------------------------------------------------------------------------------
    bool        Mission2::ShipLanded(void)
    {
        Terminate();
        return false;
    }

    //------------------------------------------------------------------------------
    void        Mission2::CreateUniverse(void)
    {
        LoadUniverse("training_1", 412, 1010);    // an interceptor

                                                  // make the universe bigger so people don't die going out of bounds
        trekClient.GetCore()->SetFloatConstant(c_fcidRadiusUniverse, 20000.0f);
        trekClient.GetCore()->SetFloatConstant(c_fcidOutOfBounds, 100.0f);

        // stuff for setup
        ImissionIGC*    pCore = trekClient.GetCore();

        // put the commander ship in the scene
        IshipIGC*   pCommander = pCore->GetShip(m_commanderID);
        assert(pCommander);
        Vector      position(500.0f, 0.0f, 0.0f);
        pCommander->SetPosition(position);
        pCommander->SetOrientation(Orientation(pCommander->GetOrientation().GetForward() * -1.0f, position * -1.0f));
        pCommander->SetCluster(pCore->GetCluster(1010));
    }

    //------------------------------------------------------------------------------
    Condition*  Mission2::CreateMission(void)
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

        // return the goal
        return pGoal;
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal01(void)
    {
        GoalList*   pGoalList = new GoalList;

        // play the introductory audio
        {
            Goal*                           pGoal = CreatePlaySoundGoal(salCommenceScanSound);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->DisableInputAction(0xffffffff);
            pSetControlConstraintsAction->ScaleInputControl(c_axisYaw, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisPitch, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisRoll, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisThrottle, 0.0f);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait two seconds
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(2.0f)));

        // tm_2_01
        // In this mission, you will receive a basic introduction to 
        // your controls and heads-up display and do a little bit of 
        // flying.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_01Sound));

        // wait one second, zoom into ship
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.7f));
            pGoal->AddStartAction(new SetDisplayModeAction(TrekWindow::cmCockpit));
            pGoalList->AddGoal(pGoal);
        }

        // tm_2_02
        // For now, your flight controls are disabled; so don't do 
        // anything until I tell you to.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_02Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_2_03
        // First, I?ll explain the peripheral radar system. This 
        // system highlights objects of interest in space by placing 
        // icons to represent them on your screen. When an object is 
        // in your field of view, its icon is located appropriately on 
        // your screen. When the object is out of your field of view, 
        // its icon rides in a gutter at the edge of your screen.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_03Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal02(void)
    {
        ImissionIGC*    pMission = trekClient.GetCore();
        IshipIGC*       pCommander = pMission->GetShip(m_commanderID);
        GoalList*       pGoalList = new GoalList;
        BuoyID          buoyID = pMission->GenerateNewBuoyID();

        // tm_2_04
        // Look out in front of your ship right now. You should see a 
        // small craft with a purple icon near it. That's me. This 
        // icon indicates my ship?s class. Beside it is the name of 
        // the pilot, me. The number just under the icon is my 
        // distance from you, measured in meters.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_04Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_2_05
        // Watch how the icon moves as I approach you. I am going to 
        // fly over your head. Pay attention to how the icon travels 
        // as my ship approaches the edge of your screen and then 
        // moves out of your field of view.
        {
            Goal*           pGoal = CreatePlaySoundGoal(tm_2_05Sound);
            pGoal->AddStartAction(new CreateWaypointAction(buoyID, Vector(-200.0f, -200.0f, 400.0f), 1010));
            pGoal->AddStartAction(new SetCommandAction(pCommander, c_cmdCurrent, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction(new SetCommandAction(pCommander, c_cmdAccepted, OT_buoy, buoyID, c_cidGoto));
            pGoalList->AddGoal(pGoal);
        }

        // wait 'til ship is close to waypoint
        pGoalList->AddGoal(new Goal(new ObjectWithinRadiusCondition(pCommander, OT_buoy, buoyID, 100.0f)));

        // tm_2_06
        // Notice that the icon remains on the screen, in a gutter. 
        // You can find me again by turning your ship toward my icon 
        // on the edge of the screen. I am above you and to your 
        // right. Now your craft will turn to show you how it works.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_06Sound);
            pGoal->AddStartAction(new SetCommandAction(pCommander, c_cmdCurrent, NA, NA, c_cidDoNothing));
            pGoal->AddStartAction(new SetCommandAction(pCommander, c_cmdAccepted, NA, NA, c_cidDoNothing));
            pGoalList->AddGoal(pGoal);
        }

        // turn the craft until it lines up
        {
            ObjectPointingAtCondition*  pObjectPointingAtCondition = new ObjectPointingAtCondition(static_cast<ImodelIGC*> (trekClient.GetShip()), static_cast<ImodelIGC*> (pCommander));
            Goal*                       pGoal = new Goal(pObjectPointingAtCondition);
            pObjectPointingAtCondition->SetMinimumAngle(1.0f);
            TurnToAction*               pTurnToAction = new TurnToAction(trekClient.GetShip(), pCommander);
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

        // tm_2_07
        // You'll get the chance to use the radar later.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_07Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal03(void)
    {
        GoalList*   pGoalList = new GoalList;

        // tm_2_08
        // Now I'm going to tell you about your ship controls. You can 
        // change the orientation of your craft using your joystick or 
        // keyboard. I'm going to release the controls now, so you can 
        // try them out while I explain them.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_08Sound));

        // wait half second, and release the controls
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.1f));
            pGoal->AddStartAction(new SetHUDOverlayAction(RollTrainingOverlay));
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->ScaleInputControl(c_axisYaw, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisPitch, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisRoll, 1.0f);
            // XXX the keys for rolling don't seem to be active
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoalList->AddGoal(pGoal);
        }

        //Commented as this is entirely unecessary - LANS
        //  if (GetWindow ()->GetInputEngine ()->GetJoystickCount () > 0)
        //  {
        //// tm_2_09
        //// With your joystick, pushing the stick forward will tip your 
        //// ship?s nose down, and pulling the stick back will tip its 
        //// nose up. Pushing the stick to the right or left will turn 
        //// the nose to the right or left, respectively. If your 
        //// joystick has rudder controls, you can use these to roll 
        //// your craft as well. Otherwise, you can hold down the SHIFT 
        //// key while pressing the RIGHT and LEFT ARROW keys to roll 
        //// your craft.
        //      pGoalList->AddGoal (CreatePlaySoundGoal (tm_2_09Sound));
        //  }
        //  else
        //  {
        //// tm_2_10
        //// On the keyboard, the UP ARROW key will tip your ship?s nose 
        //// down, and the DOWN ARROW key will tip its nose up. The 
        //// RIGHT and LEFT ARROW keys work as you might expect, facing 
        //// the ship nose to the right and left, respectively. You can 
        //// hold down the SHIFT key while pressing the RIGHT and LEFT 
        //// ARROW keys to roll your craft.
        //      pGoalList->AddGoal (CreatePlaySoundGoal (tm_2_10Sound));
        //  }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_11
        // Now I want you to practice turning the ship about. 
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_11Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        //Display on screen text with movement controls



        // tm_2_11r
        // When you are comfortable with the operation of your craft, 
        // press the SPACEBAR to proceed.
        {
            Goal*   pGoal = new Goal(new GetKeyCondition(TK_ThrustForward));
            pGoal->AddStartAction(new MessageAction("Press the SPACEBAR when you are ready to proceed."));
            pGoal->AddStartAction(new PlaySoundAction(tm_2_11rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_2_11rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_2_12
        // Good. Now you know how to turn the craft.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_12Sound));
        {
            Goal* pGoal = new Goal(new ElapsedTimeCondition(0.01f));
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal04(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_2_13
        // Next, you're going to move this interceptor around a bit 
        // and get used to the throttle. I'm going to disable turning 
        // for a moment, but the throttle will be active so you can 
        // experiment with it while I talk.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_13Sound));

        // wait half second
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->ScaleInputControl(c_axisYaw, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisPitch, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisRoll, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisThrottle, 1.0f);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoal->AddStartAction(new SetHUDOverlayAction(ThrottleTrainingOverlay));
            pGoalList->AddGoal(pGoal);;
        }

        // XXX hope ship faces empty space

        if (GetWindow()->GetInputEngine()->GetJoystickCount() > 0)
        {
            // tm_2_14
            // If your joystick has a throttle control, you can use that 
            // to finely adjust your throttle.
            pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_14Sound));
        }
        else
        {
            // tm_2_15
            // You can use the keyboard to set the throttle. The BACKSLASH 
            // key is full throttle, and BACKSPACE is stop. You can use 
            // the RIGHT BRACKET key for 66% throttle and the LEFT BRACKET 
            // key for 33% throttle. 
            pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_15Sound));
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_16
        // Now I want you to fly forward at full throttle.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_16Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_16r
        // Adjust the throttle control full forward on your joystick, 
        // or press the BACKSLASH key.
        {
            TRef<ImodelIGC>     pShip = static_cast<ImodelIGC*> (trekClient.GetShip());
            Goal*               pGoal = new Goal(new GetControlActiveCondition(trekClient.GetShip(), c_axisThrottle, 1.0f));
            pGoal->AddStartAction(new MessageAction("Press BACKSLASH (\\) or set your joystick throttle to maximum for full thrust."));
            pGoal->AddStartAction(new PlaySoundAction(tm_2_16rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_2_16rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_17
        // Good.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_17Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal05(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_2_18
        // You can see the precise setting of your throttle by looking 
        // at this gauge. 
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_18Sound);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_2_18r
        // Try adjusting the throttle to see how this gauge changes.
        {
            TRef<ImodelIGC>     pShip = static_cast<ImodelIGC*> (trekClient.GetShip());
            Goal*               pGoal = new Goal(new NotCondition(new GetControlActiveCondition(trekClient.GetShip(), c_axisThrottle, 1.0f)));
            pGoal->AddStartAction(new MessageAction("Use your scroll wheel or joystick throttle to adjust thrust."));
            pGoal->AddStartAction(new PlaySoundAction(tm_2_18rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_2_18rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_2_19
        // Now you know how to adjust the throttle and read a gauge on 
        // your heads-up display. Now I'm going to go over some of the 
        // other items on your display. Your controls will be disabled 
        // while I talk about these things, so just pay attention.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_19Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal06(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_2_20
        // When you are moving, the velocity indicator will tell you 
        // how fast you are moving. The indicator reads in meters per 
        // second, or M.P.S.
        {
            Goal*                           pGoal = CreatePlaySoundGoal(tm_2_20Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(SpeedTrainingOverlay));
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->ScaleInputControl(c_axisThrottle, 0.0f);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            SetControlsAction*              pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputControl(c_axisThrottle, 0.33f);
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_21
        // Notice the motion indicator in the center of your screen. 
        // This indicator tells you which way you are moving. In 
        // space, there is nothing to stop your ship from sliding 
        // sideways. When you turn, it takes a moment for the ship's 
        // engines to compensate for your velocity. As a result, you 
        // won't always be facing the same direction you are moving. 
        // This indicator will show you the direction that your craft 
        // is traveling. Now your ship will turn a bit to demonstrate.
        {
            Goal*                           pGoal = CreatePlaySoundGoal(tm_2_21Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(MotionIndicatorTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second, while turning right
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pSetControlsAction->SetInputControl(c_axisYaw, -0.5f);
            pSetControlsAction->SetInputControl(c_axisThrottle, 0.33f);
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second, and reset the controls
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputControl(c_axisThrottle, 0.33f);
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second, while pitching up
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputControl(c_axisPitch, -0.5f);
            pSetControlsAction->SetInputControl(c_axisThrottle, 0.33f);
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second, and reset the controls
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputControl(c_axisThrottle, 0.33f);
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // wait half second, and reset the controls
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // tm_2_22
        // Got it? ? Good.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_22Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.2f)));

        // tm_2_23
        // I'm going to activate your controls again, and I want you 
        // to practice turning and adjusting your throttle.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_23Sound));

        // wait half second
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.5f));
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->ScaleInputControl(c_axisYaw, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisPitch, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisRoll, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl(c_axisThrottle, 1.0f);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoalList->AddGoal(pGoal);
        }

        // tm_2_24
        // Notice how the motion indicator floats around the screen as 
        // you turn and that you don't stop immediately when you 
        // throttle down. 
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_24Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_24r
        // When you are comfortable with the operation of your ship, 
        // press the SPACEBAR to proceed.
        {
            Goal*   pGoal = new Goal(new GetKeyCondition(TK_ThrustForward));
            pGoal->AddStartAction(new MessageAction("Press the SPACEBAR when you are ready to proceed."));
            pGoal->AddStartAction(new PlaySoundAction(tm_2_24rSound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_2_24rSound));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_25
        // Okay. That wasn?t so bad was it? You?re doing good ? You?ve 
        // learned the basics of space flight.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_25Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal07(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_2_26
        // I'm going to introduce you to the booster now. 
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_26Sound));

        // wait quarter second, and set the controls
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(0.25f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputAction(afterburnerButtonIGC);
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // tm_2_27
        // This shaking and rumbling is your booster firing.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_27Sound));

        // wait quarter second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.25f)));

        // tm_2_28
        // Notice your speed.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_28Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(SpeedTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_29
        // Notice how the fuel gauge drops when using the boosters.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_29Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(FuelTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait a couple of seconds, turn off the boosters
        {
            Goal*   pGoal = new Goal(new ElapsedTimeCondition(1.75f));
            SetControlsAction*          pSetControlsAction = new SetControlsAction;
            pGoal->AddStartAction(pSetControlsAction);
            pGoalList->AddGoal(pGoal);
        }

        // tm_2_30
        // You carry a limited supply of fuel on the ship, and it 
        // doesn't regenerate, so make sure you watch your fuel gauge 
        // when firing the boosters.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_30Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_31
        // Now I want you to give it a try.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_31Sound);
            pGoal->AddStartAction(new SetHUDOverlayAction(ShiftTabTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // explain and wait for burners to take effect
        {
            TRef<ImodelIGC> ship = static_cast<ImodelIGC*> (trekClient.GetShip());
            Condition*      pGetStateBitsCondition = new GetStateBitsCondition(trekClient.GetShip(), afterburnerButtonIGC);
            Goal*           pGoal = new Goal(pGetStateBitsCondition);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->EnableInputAction(afterburnerButtonIGC);
            pGoal->AddStartAction(pSetControlConstraintsAction);
        
            if (GetWindow()->GetInputEngine()->GetJoystickCount() > 0)
            {
                // tm_2_32
                // Activate the booster with your joystick by pressing and 
                // holding button 3.
                pGoal->AddStartAction(new PlaySoundAction(tm_2_32Sound));
                pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_2_32Sound));
                pGoal->AddStartAction(new MessageAction("Press and hold button #3 on your joystick to fire the booster."));
            }
            else
            {
                // tm_2_33
                // On the keyboard, press and hold the TAB key to fire your 
                // boosters.
                //pGoal->AddStartAction (new PlaySoundAction (tm_2_33Sound));
                pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_2_33Sound));
                pGoal->AddStartAction(new MessageAction("Press and hold the SHIFT+SPACE key to fire the booster."));
            }

            pGoalList->AddGoal(pGoal);

        }

        // wait five seconds
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(5.0f)));

        // tm_2_34
        // Good!
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_34Sound);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->DisableInputAction(afterburnerButtonIGC);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoal->AddStartAction(new SetHUDOverlayAction(NoTrainingOverlay));
            pGoalList->AddGoal(pGoal);
        }

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal08(void)
    {
        GoalList*       pGoalList = new GoalList;

        // tm_2_35
        // I cut the boosters there because I didn't want to get too 
        // far away from home. The boosters can get you up to speed 
        // very fast and can even take you over the top handling speed 
        // for your craft. You have to be careful where you point the 
        // ship when you use the boosters, because you might not be 
        // able turn or stop fast enough to avoid obstacles. 
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_35Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_36
        // All right, Cadet, you've made it through your introductory 
        // flight! Excellent. 
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_36Sound));

        // wait half second
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(0.5f)));

        // tm_2_37
        // Let's go home now. I'm switching you over to autopilot, so 
        // you can look over the gauges and such in the heads-up 
        // display. Enjoy the ride.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_2_37Sound));

        // wait to get close to the station
        {
            ImodelIGC*  pStation = trekClient.GetCore()->GetModel(OT_station, 1010);
            Goal*       pGoal = new Goal(new AndCondition(new ObjectWithinRadiusCondition(trekClient.GetShip(), pStation, 500.0f), new ElapsedTimeCondition(0.5f)));
            pGoal->AddStartAction(new SetCommandAction(trekClient.GetShip(), c_cmdCurrent, pStation, c_cidGoto));
            pGoal->AddStartAction(new SetCommandAction(trekClient.GetShip(), c_cmdAccepted, pStation, c_cidGoto));
            pGoal->AddStartAction(new SetAutopilotAction(trekClient.GetShip()));
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->DisableInputAction(afterburnerButtonIGC);
            pGoal->AddStartAction(pSetControlConstraintsAction);
            pGoal->AddConstraintCondition(new PeriodicCondition(new ConditionalAction(new NotCondition(new GetAutopilotCondition(trekClient.GetShip())), new SetAutopilotAction(trekClient.GetShip())), 2.0f));
            pGoalList->AddGoal(pGoal);
        }

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission2::CreateGoal09(void)
    {
        GoalList*       pGoalList = new GoalList;

        // XXX I could stop the ship here if there is a problem

        // tm_2_38
        // I'm going to go back to the outpost and land in a green 
        // hangar bay. The red bays are the launching points. Make 
        // sure you never try to land in a red hangar, or you'll be a 
        // trophy on somebody's canopy.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_2_38Sound);
            pGoal->AddConstraintCondition(new PeriodicCondition(new ConditionalAction(new NotCondition(new GetAutopilotCondition(trekClient.GetShip())), new SetAutopilotAction(trekClient.GetShip())), 2.0f));
            pGoalList->AddGoal(pGoal);
        }

        // wait for ship to land
        {
            Goal*   pGoal = new Goal(new FalseCondition);
            pGoal->AddStartAction(new SetDisplayModeAction(TrekWindow::cmExternalStation));
            pGoal->AddConstraintCondition(new PeriodicCondition(new ConditionalAction(new NotCondition(new GetAutopilotCondition(trekClient.GetShip())), new SetAutopilotAction(trekClient.GetShip())), 2.0f));
            pGoalList->AddGoal(pGoal);
        }

        return new Goal(pGoalList);
    }

    //------------------------------------------------------------------------------
}








