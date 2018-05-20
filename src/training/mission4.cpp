/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	Mission4.cpp
**
**  Author:  
**
**  Description:
**      Implementation of the second training mission
**
**  History:
*/
#include    "pch.h"
#include    "Mission4.h"

#include    "Training.h"
#include    "Goal.h"
#include    "GoalList.h"
#include    "Predicate.h"
#include    "TrueCondition.h"
#include    "FalseCondition.h"
#include    "AndCondition.h"
#include    "OrCondition.h"
#include    "NotCondition.h"
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
#include    "GetAutopilotCondition.h"
#include    "SetAutopilotAction.h"
#include    "GetTargetCondition.h"
#include    "GetSectorCondition.h"
#include    "GetCommandCondition.h"
#include    "SetSignatureAction.h"
#include    "GetProbeCountCondition.h"
#include    "CreateProbeAction.h"
#include    "CreateWaypointAction.h"
#include    "ShowPaneAction.h"
#include    "ShowEyeballAction.h"
#include    "SetHUDOverlayAction.h"
#include    "GetKeyCondition.h"
#include    "TurnToAction.h"
#include    "ToggleWeaponAction.h"
#include    "SwapWeaponAction.h"
#include    "DoDamageAction.h"
#include    "CurrentTarget.h"
#include    "PlayerShipTarget.h"
#include    "RepairRearmAction.h"
#include    "SetControlsAction.h"
#include    "CreateDroneAction.h"
#include    "ProxyCondition.h"
#include    "ShipSeesObjectCondition.h"
#include    "GetChatCondition.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  Mission4::~Mission4 (void)
    {
    }

    //------------------------------------------------------------------------------
    int         Mission4::GetMissionID (void)
    {
        return c_TM_4_Enemy_Engagement;
    }

    //------------------------------------------------------------------------------
    SectorID    Mission4::GetStartSectorID (void)
    {
        return 1030;
    }

    //------------------------------------------------------------------------------
    bool        Mission4::ShipLanded (void)
    {
        // end the mission
        Terminate ();

        // signal the mission to end
        return false;
    }

    //------------------------------------------------------------------------------
    void        Mission4::CreateUniverse (void)
    {
        LoadUniverse ("training_3", 486, 1030);    // an advanced stealth fighter

        // stuff for setup
        IshipIGC*           pShip = trekClient.GetShip();

        // set a global attribute to increase the overall energy of the ship
        IsideIGC*           pSide = pShip->GetSide ();
        GlobalAttributeSet  gas = pSide->GetGlobalAttributeSet ();
        gas.SetAttribute (c_gaMaxEnergy, 10.0f);
        pSide->SetGlobalAttributeSet (gas);

        // put the commander ship in the station
        ImissionIGC*        pCore = trekClient.GetCore();
        ImodelIGC*          pStation = pCore->GetModel (OT_station, 1030);
        IshipIGC*           pCommander = pCore->GetShip (m_commanderID);
        pCommander->SetStation (static_cast<IstationIGC*> (pStation));
        pCommander->SetCommand (c_cmdAccepted, NULL, c_cidDoNothing);
        pCommander->SetCommand (c_cmdCurrent, NULL, c_cidDoNothing);
        pCommander->SetAutopilot (false);

        // activate all the starting weapons
        trekClient.fGroupFire = true;

        // take the missiles off the ship
        IpartIGC*           pPart = pShip->GetMountedPart (ET_Magazine, 0);
        if (pPart)
            pPart->Terminate ();

    }

    //------------------------------------------------------------------------------
    Condition*  Mission4::CreateMission (void)
    {
        GoalList*   pGoalList = new GoalList;
        Goal*       pGoal = new Goal (pGoalList);

        // generate ship ids that we'll be using throughout the mission
        ImissionIGC*    pMission = trekClient.GetCore ();
        m_enemyScoutID = pMission->GenerateNewShipID ();
        m_enemyMiner1ID = pMission->GenerateNewShipID ();
        m_enemyMiner2ID = pMission->GenerateNewShipID ();
        m_enemyMiner3ID = pMission->GenerateNewShipID ();
        m_enemyMiner4ID = pMission->GenerateNewShipID ();
        m_builderID = pMission->GenerateNewShipID ();

        // add the goals to the list
        pGoalList->AddGoal (CreateGoal01 ());
        pGoalList->AddGoal (CreateGoal02 ());
        pGoalList->AddGoal (CreateGoal03 ());
        pGoalList->AddGoal (CreateGoal04 ());
        pGoalList->AddGoal (CreateGoal05 ());
        pGoalList->AddGoal (CreateGoal06 ());
        pGoalList->AddGoal (CreateGoal07 ());
        pGoalList->AddGoal (CreateGoal08 ());
        pGoalList->AddGoal (CreateGoal09 ());
        pGoalList->AddGoal (CreateGoal10 ());

        return pGoal;
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal01 (void)
    {
        GoalList*   pGoalList = new GoalList;

        // play introductory audio
        {
            Goal*                           pGoal = CreatePlaySoundGoal (salCommenceScanSound);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->DisableInputAction (0xffffffff);
            pSetControlConstraintsAction->ScaleInputControl (c_axisYaw, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisPitch, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisRoll, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisThrottle, 0.0f);
            pGoal->AddStartAction (pSetControlConstraintsAction);
            pGoalList->AddGoal (pGoal);
        }

        // wait two more seconds
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (2.0f)));

		// tm_4_01
		// For this mission, we'll be using a tactical fighter. As 
		// usual, your controls are disabled until I tell you to use 
		// them.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_01Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_02
		// First, let's talk about how you communicate with other 
		// pilots. There are three mechanisms for communication.
        {
            Goal*           pGoal = CreatePlaySoundGoal (tm_4_02Sound);
            pGoal->AddStartAction (new SetDisplayModeAction (TrekWindow::cmCockpit));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal02 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_4_03
		// The first method is to compose a chat message. You can chat 
		// to individual recipients, a team, a sector, or everybody. 
		// Let's try sending a chat to All. 
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_03Sound);
            pGoal->AddStartAction (new SetHUDOverlayAction (ChatTrainingOverlay));
            pGoal->AddStartAction (new MessageAction ("This is a 'chat' message from the commander to you."));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_03r
		// Press the ENTER key to begin. Use the TAB key to select the 
		// chat target, in this case All. Then type your message, and 
		// press ENTER again to send it.
        {
            Goal*   pGoal = new Goal (new GetChatCondition (CHAT_EVERYONE));
            pGoal->AddStartAction (new MessageAction ("Press the ENTER key to begin a chat."));
            pGoal->AddStartAction (new MessageAction ("Press the TAB key to select 'All'."));
            pGoal->AddStartAction (new MessageAction ("Press the ENTER key again to send the chat."));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_03rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_03rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_04
		// Good! Go ahead and try picking a few different recipients 
		// and sending chats to them. 
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_04Sound);
            pGoal->AddStartAction (new SetHUDOverlayAction (NoTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_04r
		// Press the SPACEBAR when you are ready to proceed.
        {
            Goal*   pGoal = new Goal (new GetKeyCondition (TK_ThrustForward));
            pGoal->AddStartAction (new MessageAction ("Press the SPACEBAR when you are ready to proceed."));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_04rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_04rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_05
		// Good! The second method of communication is to use a quick 
		// comm message. You can send a quick comm by pressing the 
		// TILDE key and then choosing the comm you want to send from 
		// the list. This will play a sound and send a chat message to 
		// your last chat recipient.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_05Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_06
		// Try sending a few quick comm messages now. Press the 
		// SPACEBAR when you are comfortable with them.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_06Sound);
            pGoal->AddStartAction (new MessageAction ("Press the TILDE (~) key to activate the quickcomm menu."));
            pGoalList->AddGoal (pGoal);
        }

		// tm_4_06r
		// Press the SPACEBAR when you are comfortable with quick 
		// comms.
        {
            Goal*   pGoal = new Goal (new GetKeyCondition (TK_ThrustForward));
            pGoal->AddStartAction (new MessageAction ("Press the SPACEBAR when you are ready to proceed."));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_06rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal03 (void)
    {
        GoalList*       pGoalList = new GoalList;
        ImissionIGC*    pMission = trekClient.GetCore ();
        BuoyID          buoyID = pMission->GenerateNewBuoyID ();
        IshipIGC*       pShip = trekClient.GetShip ();

		// tm_4_07
		// The last means of communication is to receive an order from 
		// your commander. I'm giving you a command to go to a 
		// waypoint.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_07Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_08
		// This red text denotes an objective from the commander.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_08Sound);
            IwarpIGC*   pAleph = pMission->GetWarp (1030);
            Vector      delta = pAleph->GetPosition () - pShip->GetPosition ();
            Vector      position = pShip->GetPosition () + (delta * 0.25f) + (pAleph->GetOrientation ().GetUp () * (delta.Length () * 0.1f));
            pGoal->AddStartAction (new CreateWaypointAction (buoyID, position, 1030));
            pGoal->AddStartAction (new SetCommandAction (pShip, c_cmdQueued, OT_buoy, buoyID, c_cidGoto));
            //pGoal->AddStartAction (new SetHUDOverlayAction (CommandTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_08r
		// Press the INSERT key to accept this objective. 
        {
            Goal*   pGoal = new Goal (new GetCommandCondition (trekClient.GetShip (), c_cidGoto));
            pGoal->AddStartAction (new MessageAction ("Press the INSERT key to accept the command."));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_08rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_08rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait a few seconds
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (1.5f)));

		// tm_4_09
		// Good. Notice that autopilot is activated when you accept an 
		// objective to fly you to your goal.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_09Sound);
            //pGoal->AddStartAction (new SetHUDOverlayAction (NoTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // (Wait for ship to get near waypoint)
        {
            Goal*   pGoal = new Goal (new ObjectWithinRadiusCondition (static_cast<ImodelIGC*> (pShip), OT_buoy, buoyID, 100.0f));
            pGoal->AddConstraintCondition (new ConditionalAction (new TrueCondition, new SetAutopilotAction (trekClient.GetShip (), true)));
            pGoalList->AddGoal (pGoal);
        }

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal04 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_4_10
		// Now, we're going to talk about intersector travel. There 
		// are many different sectors of space. Sectors are centered 
		// on clusters of asteroids and are linked by alephs. An aleph 
		// is essentially an intergalactic subway system that allows 
		// instantaneous two-way transit between two distant sectors.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_10Sound);
            IshipIGC*   pShip = trekClient.GetShip ();
            pGoal->AddStartAction (new SetCommandAction (pShip, c_cmdAccepted, NA, NA, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (pShip, c_cmdCurrent, NA, NA, c_cidDoNothing));
            pGoal->AddStartAction (new SetAutopilotAction (pShip, false));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_11
		// This is the Map display. You can display and hide it by 
		// pressing the F7 key.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_11Sound);
            pGoal->AddStartAction (new ShowPaneAction (ofSectorPane));
            pGoal->AddStartAction (new SetHUDOverlayAction (SectorTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_12
		// We've already sent some scouts out, so there are three 
		// sectors mapped in this region of space. Whenever a new 
		// sector is discovered, it will appear on this map. 
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_12Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_13
		// We are in sector Neptune. The heavy ring around the sector 
		// on the left shows you where you are.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_13Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_14
		// There is an aleph that connects Neptune to another sector: 
		// Mars. Mars is connected to another sector known to contain 
		// an enemy mining outpost. We expect to see some enemy craft 
		// in Mars, which we will need to clean out so that we can 
		// build an outpost of our own.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_14Sound);
            pGoal->AddStartAction (new SetHUDOverlayAction (NoTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_15
		// Let's get going! Press the INSERT key to accept your orders.
        {
            Goal*   pGoal = new Goal (new GetCommandCondition (trekClient.GetShip (), c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdQueued, OT_warp, 1030, c_cidGoto));
            pGoal->AddStartAction (new MessageAction ("Press the INSERT key to accept the command."));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_15Sound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_15Sound));
            pGoalList->AddGoal (pGoal);
        }

        // wait a second or two
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (1.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal05 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_4_16
		// Excellent!
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_16Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_17
		// I'm enabling your controls. You can disengage autopilot by 
		// moving your joystick.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_17Sound);
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->EnableInputAction (0xffffffff);
            pSetControlConstraintsAction->ScaleInputControl (c_axisYaw, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisPitch, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisRoll, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisThrottle, 1.0f);
            pGoal->AddStartAction (pSetControlConstraintsAction);
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_17r
		// Go ahead and fly to the highlighted aleph. The aleph looks 
        // like a whirlpool in space. It's a big swirling brown vortex, 
        // with a bright spot in the center.
        {
            Goal*   pGoal = new Goal (new ObjectWithinRadiusCondition (static_cast<ImodelIGC*> (trekClient.GetShip ()), OT_warp, 1030, 750.0f));
            pGoal->AddStartAction (new MessageAction ("Fly to the aleph named 'Mars'."));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_17rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (60.0f, tm_4_17rSound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_4_18
		// All right, see the aleph? It's right in front of us. I'm 
		// going to fly us directly into it.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_18Sound);
            pGoal->AddStartAction (new MessageAction ("Flying through the aleph to sector 'Mars'..."));
			// pkk - Disable controls, so can AP take over flying into aleph (mission will not hang)
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->DisableInputAction (0xffffffff);
            pSetControlConstraintsAction->ScaleInputControl (c_axisYaw, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisPitch, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisRoll, 0.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisThrottle, 0.0f);
            pGoal->AddStartAction (pSetControlConstraintsAction);
            TurnToAction*           pTurnToAction = new TurnToAction (trekClient.GetShip (), OT_warp, 1030);
            ConditionalAction*      pConditionalAction = new ConditionalAction (new TrueCondition, pTurnToAction);
            pGoal->AddConstraintCondition (pConditionalAction);
            pGoalList->AddGoal (pGoal);
        }

        // (Wait for sector change)
        {
            Goal*   pGoal = new Goal (new GetSectorCondition (trekClient.GetShip (), 1031));

            // turn the throttle to 0.5 to insure that they fly through
            SetControlsAction*  pSetControlsAction = new SetControlsAction;
            pSetControlsAction->SetInputControl (c_axisThrottle, 0.5f);
            pGoal->AddConstraintCondition (new ConditionalAction (new TrueCondition, pSetControlsAction));

            // create the enemy miners
            IwarpIGC*   pAleph = trekClient.GetCore ()->GetWarp (1032);
            {
                Vector              pos = pAleph->GetPosition () + Vector (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
                CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Enemy Miner 01", m_enemyMiner1ID, 338, 1, c_ptMiner);
                pCreateDroneAction->SetCreatedLocation (1031, pos);
                pGoal->AddStartAction (pCreateDroneAction);
            }

            {
                Vector              pos = pAleph->GetPosition () + Vector (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
                CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Enemy Miner 02", m_enemyMiner2ID, 338, 1, c_ptMiner);
                pCreateDroneAction->SetCreatedLocation (1031, pos);
                pGoal->AddStartAction (pCreateDroneAction);
            }

            {
                Vector              pos = pAleph->GetPosition () + Vector (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
                CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Enemy Miner 03", m_enemyMiner3ID, 338, 1, c_ptMiner);
                pCreateDroneAction->SetCreatedLocation (1031, pos);
                pGoal->AddStartAction (pCreateDroneAction);
            }

            {
                Vector              pos = pAleph->GetPosition () + Vector (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
                CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Enemy Miner 04", m_enemyMiner4ID, 338, 1, c_ptMiner);
                pCreateDroneAction->SetCreatedLocation (1031, pos);
                pGoal->AddStartAction (pCreateDroneAction);
            }

            // set the signature on all those enemy craft to something ridiculous
            pGoal->AddStartAction (new SetSignatureAction (OT_ship, m_enemyMiner1ID, 1.0e6));
            pGoal->AddStartAction (new SetSignatureAction (OT_ship, m_enemyMiner2ID, 1.0e6));
            pGoal->AddStartAction (new SetSignatureAction (OT_ship, m_enemyMiner3ID, 1.0e6));
            pGoal->AddStartAction (new SetSignatureAction (OT_ship, m_enemyMiner4ID, 1.0e6));

            // tell them all to do nothing for a little while
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner1ID, c_cmdAccepted, NULL, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner1ID, c_cmdCurrent, NULL, c_cidDoNothing));

            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner2ID, c_cmdAccepted, NULL, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner2ID, c_cmdCurrent, NULL, c_cidDoNothing));

            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner3ID, c_cmdAccepted, NULL, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner3ID, c_cmdCurrent, NULL, c_cidDoNothing));

            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner4ID, c_cmdAccepted, NULL, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner4ID, c_cmdCurrent, NULL, c_cidDoNothing));

            pGoalList->AddGoal (pGoal);
        }

        // wait a few seconds
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (1.75f)));

		// tm_4_19
		// Amazing. Millions of kilometers in the blink of an eye. 
		// Here we are in sector Mars.
        {
            Goal*           pGoal = CreatePlaySoundGoal (tm_4_19Sound);
            pGoal->AddStartAction (new SetControlsAction);
            SetControlsAction*              pSetControlsAction = new SetControlsAction;
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->DisableInputAction (0xffffffff);
			// pkk - Allow ship to turn again
            pSetControlConstraintsAction->ScaleInputControl (c_axisYaw, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisPitch, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisRoll, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisThrottle, 0.0f);
            GetWindow()->SetTarget(trekClient.GetShip(), c_cidNone); // pkk - Target nothing
            pGoal->AddStartAction (pSetControlsAction);
            pGoal->AddStartAction (pSetControlConstraintsAction);
            pGoal->AddStartAction (new ShowPaneAction (ofSectorPane));
            pGoal->AddStartAction (new SetHUDOverlayAction (SectorTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal06 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_4_20
		// Also notice that there are some enemy miners in this 
		// sector. I've disabled your controls while I cover a few key 
		// points.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_20Sound);
            pGoal->AddStartAction (new SetHUDOverlayAction (NoTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.3f)));

		// tm_4_21
		// Right now, we are far enough away that none of those enemy 
		// craft can see you. 
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_21Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.3f)));

		// tm_4_22
		// You can tell if the enemy sees you because this eyeball 
		// will appear on your heads-up display.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_22Sound);
            pGoal->AddStartAction (new ShowEyeballAction (true));
            pGoal->AddStartAction (new SetHUDOverlayAction (EyeballTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_23
		// You can do several things to affect how visible you are to 
		// enemies.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_23Sound);
            pGoal->AddStartAction (new ShowEyeballAction (false));
            pGoal->AddStartAction (new SetHUDOverlayAction (NoTrainingOverlay));

            // tell all the miners to get a new command
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner1ID, c_cmdAccepted, NULL, c_cidNone));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner1ID, c_cmdCurrent, NULL, c_cidNone));

            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner2ID, c_cmdAccepted, NULL, c_cidNone));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner2ID, c_cmdCurrent, NULL, c_cidNone));

            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner3ID, c_cmdAccepted, NULL, c_cidNone));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner3ID, c_cmdCurrent, NULL, c_cidNone));

            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner4ID, c_cmdAccepted, NULL, c_cidNone));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner4ID, c_cmdCurrent, NULL, c_cidNone));


            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_24
		// First, you can stay out of their scan ranges. Enemies can't 
		// see you if they can't scan you. This includes hiding in the 
		// shadow of asteroids. Enemies can't scan you unless they 
		// have a line of sight on you. Remember that if any 
		// individual enemy can see you, then they all can.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_24Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_25
		// Second, you can adjust your scan signature. Firing weapons 
		// temporarily increases your signature, as does using your 
		// boosters, so avoid doing these things when stealth is 
		// important. Carrying missiles and shields raises signature, 
		// so keep these items in cargo when they aren't in use.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_25Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        {
            GoalList*   pGoalList2 = new GoalList;
            Condition*  pGetKeyCondition = new GetKeyCondition (TK_ToggleCloak);

		    // tm_4_26
		    // A cloak will lower your signature while activated. Only 
		    // certain ships carry cloaks, the tactical fighter being one 
		    // of them. 
            pGoalList2->AddGoal (CreatePlaySoundGoal (tm_4_26Sound));

            // wait half second
            pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		    // tm_4_26r
		    // Press the K key now to activate your cloak.
            {
                Goal*   pGoal = new Goal (new ProxyCondition (pGetKeyCondition));
                pGoal->AddStartAction (new MessageAction ("Press the G key to activate your cloak."));
                pGoal->AddStartAction(new SetHUDOverlayAction(CloakTrainingOverlay));
                pGoal->AddStartAction (new PlaySoundAction (tm_4_26rSound));
                pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_26rSound));
                pGoalList2->AddGoal (pGoal);
            }

            // set up the condition checking
            {
                Goal*   pGoal = new Goal (pGoalList2);
                pGoal->AddConstraintCondition (pGetKeyCondition);
                SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
                pSetControlConstraintsAction->EnableInputAction (cloakActiveIGC);
                pGoal->AddStartAction (pSetControlConstraintsAction);
                pGoalList->AddGoal (pGoal);
            }
        }

        // wait a few seconds
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (1.5f)));

		// tm_4_27
		// Good! The cloak consumes energy. If you deplete your energy 
		// reserve, the effectiveness of the cloak will be 
		// significantly reduced.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_27Sound);
            pGoal->AddStartAction (new SetHUDOverlayAction (EnergyTrainingOverlay));
            pGoal->AddStartAction (new RepairRearmAction);
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal07 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_4_28
		// Now let's talk about those enemy ships. They appear to be 
		// miners extracting Helium3 from a few choice asteroids. 
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_28Sound);
            pGoal->AddStartAction (new SetHUDOverlayAction (NoTrainingOverlay));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_30
		// Utility ships like miners and constructors are unarmed and 
		// will run when attacked. With your cloak on, you should be 
		// able to get close to them before they see you and start to 
		// run. This gives you a pretty good chance of destroying 
		// them. In addition, the weapons on scouts and tactical 
		// fighters are especially good against the hulls of utility 
		// ships.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_4_30Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_29
		// Target one of the enemy miners using the E key and try to 
		// sneak up on it. Remember not to fire or use your boosters 
		// until you are within range. Destroy the enemy craft if you 
		// can.
        {
            PlaySoundAction*                pPlaySoundAction = new PlaySoundAction (tm_4_29Sound);
            //Goal*                           pGoal = new Goal (new AndCondition (new GetKeyCondition (TK_TargetEnemy), new SoundFinishedCondition (pPlaySoundAction)));
            Goal*                           pGoal = new Goal (new GetKeyCondition (TK_TargetEnemy)); // pkk - Let's start  before sound is finished
            pGoal->AddStartAction (pPlaySoundAction);
            pGoal->AddStartAction (new MessageAction ("Target an enemy miner with the T key, and destroy it if you can."));
            pGoal->AddConstraintCondition (CreateTooLongCondition (20.0f, tm_4_29Sound));
            pGoalList->AddGoal (pGoal);
        }

		// pkk - Fix for #246
		// New Goal to activate controls
        {
            Goal*                           pGoal = new Goal (new ElapsedTimeCondition (0.5f));
            SetControlConstraintsAction*    pSetControlConstraintsAction = new SetControlConstraintsAction;
            pSetControlConstraintsAction->EnableInputAction (0xffffffff);
            pSetControlConstraintsAction->ScaleInputControl (c_axisThrottle, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisYaw, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisPitch, 1.0f);
            pSetControlConstraintsAction->ScaleInputControl (c_axisRoll, 1.0f);
            pGoal->AddStartAction (pSetControlConstraintsAction);
            pGoalList->AddGoal (pGoal);
        }

        // (Wait for enemy destruction) (or the enemy leaving the sector)
        {
            OrCondition*                    pOrConditionA = new OrCondition (new GetShipIsDestroyedCondition2 (OT_ship, m_enemyMiner1ID), new GetShipIsDestroyedCondition2 (OT_ship, m_enemyMiner2ID));
            OrCondition*                    pOrConditionB = new OrCondition (new GetShipIsDestroyedCondition2 (OT_ship, m_enemyMiner3ID), new GetShipIsDestroyedCondition2 (OT_ship, m_enemyMiner4ID));
            Condition*                      pInSectorCondition = new GetSectorCondition (new CurrentTarget (new PlayerShipTarget), 1031);
            Goal*                           pGoal = new Goal (new OrCondition (new OrCondition (pOrConditionA, pOrConditionB), new NotCondition (pInSectorCondition)));

            // show the eyeball if it is appropriate
            {
                ObjectID        shipID = trekClient.GetShip ()->GetObjectID ();
                Condition*      pOrConditionA = new OrCondition (new ShipSeesObjectCondition (m_enemyMiner1ID, OT_ship, shipID), new ShipSeesObjectCondition (m_enemyMiner2ID, OT_ship, shipID));
                Condition*      pOrConditionB = new OrCondition (new ShipSeesObjectCondition (m_enemyMiner3ID, OT_ship, shipID), new ShipSeesObjectCondition (m_enemyMiner4ID, OT_ship, shipID));
                Condition*      pSeesCondition = new OrCondition (new OrCondition (pOrConditionA, pOrConditionB), new ShipSeesObjectCondition (m_enemyScoutID, OT_ship, shipID));
                pGoal->AddConstraintCondition (new ConditionalAction (pSeesCondition, new ShowEyeballAction (true)));
                pGoal->AddConstraintCondition (new ConditionalAction (new NotCondition (new ProxyCondition (pSeesCondition)), new ShowEyeballAction (false)));
            }

		    // tm_4_29r
		    // Destroy the enemy miner if you can.
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_29rSound));

            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal08 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_4_31
		// Excellent work, Cadet! It looks like the other enemy miners 
		// are running now. I'm ordering a constructor into this 
		// sector to build a station here.
        {
            Goal*               pGoal = CreatePlaySoundGoal (tm_4_31Sound);

            // create the friendly builder
            IwarpIGC*           pAleph = trekClient.GetCore ()->GetWarp (1030);
            Vector              pos = pAleph->GetPosition () + (pAleph->GetOrientation ().GetForward () * 100.0f);
            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Constructor", m_builderID, 419, 0, c_ptBuilder);
            pCreateDroneAction->SetCreatedLocation (1030, pos);
            pCreateDroneAction->SetStationType (112);
            pGoal->AddStartAction (pCreateDroneAction);

            // command the builder to go to the asteroid on which it will build
            pGoal->AddStartAction(new SetCommandAction(m_builderID, c_cmdCurrent, OT_asteroid, static_cast<ObjectID>(10332), c_cidBuild));
            pGoal->AddStartAction(new SetCommandAction(m_builderID, c_cmdAccepted, OT_asteroid, static_cast<ObjectID>(10332), c_cidBuild));

            // create a waypoint for all of the miners to run to, and send them all there
            BuoyID              buoyID = trekClient.GetCore ()->GenerateNewBuoyID ();
            pGoal->AddStartAction (new CreateWaypointAction (buoyID, Vector (1000.0f, 1000.0f, 1000.0f), 1032));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner1ID, c_cmdCurrent, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner1ID, c_cmdAccepted, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner2ID, c_cmdCurrent, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner2ID, c_cmdAccepted, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner3ID, c_cmdCurrent, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner3ID, c_cmdAccepted, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner4ID, c_cmdCurrent, OT_buoy, buoyID, c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (m_enemyMiner4ID, c_cmdAccepted, OT_buoy, buoyID, c_cidGoto));

            // clear the player command set
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdCurrent, NA, NA, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdAccepted, NA, NA, c_cidDoNothing));

            pGoalList->AddGoal (pGoal);
        }


        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.3f)));

		// tm_4_32
		// I'm moving your ship around to protect the constructor. 
		// Stay near it in case an enemy fighter comes in. 
		// Constructors are very expensive, and we don't want to lose 
		// it.
        {
            Goal*               pGoal = new Goal (new GetCommandCondition (trekClient.GetShip (), c_cidGoto));
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdQueued, OT_ship, m_builderID, c_cidGoto));
            pGoal->AddStartAction (new MessageAction ("Press the INSERT key to accept the command."));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_32Sound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_4_08rSound));
            pGoalList->AddGoal (pGoal);
        }

        // (Wait for enemy scout to get within range)
        {
            Goal*               pGoal = new Goal (new ObjectWithinRadiusCondition (trekClient.GetShip(), OT_ship, m_enemyScoutID, 1500.0f));
            Vector              pos (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Enemy Scout", m_enemyScoutID, 310, 1, c_ptWingman);
            pCreateDroneAction->SetCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles); // - LANS, prevent scout from running away
            pCreateDroneAction->SetCreatedLocation (1031, pos);
            pGoal->AddStartAction (pCreateDroneAction);
            //LANS - make the scout attack the constructor, not the player. Not only is this better for teaching, but if the scout attacks the player
            //and the player is uneyed, this would throw an exception
            pGoal->AddStartAction (new SetCommandAction (m_enemyScoutID, c_cmdCurrent, OT_ship, m_builderID, c_cidAttack));
            pGoal->AddStartAction (new SetCommandAction (m_enemyScoutID, c_cmdAccepted, OT_ship, m_builderID, c_cidAttack));
            pGoalList->AddGoal (pGoal);
        }

		// tm_4_33
		// Enemy scout detected! Intercept it!
        {
            Goal* pGoal = new Goal (new GetCommandCondition (trekClient.GetShip (), c_cidAttack)); 
            pGoal->AddStartAction (new SetCommandAction(m_builderID, c_cmdCurrent, NA, NA, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction(m_builderID, c_cmdAccepted, NA, NA, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdQueued, OT_ship, m_enemyScoutID, c_cidAttack));// - Error here?
            pGoal->AddStartAction (new MessageAction ("Press the INSERT key to accept the command."));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_33Sound));
            pGoal->AddConstraintCondition(CreateTooLongCondition(30.0f, tm_4_33Sound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half a second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_4_33r
		// Stop the scout from destroying the builder.
        {
            Goal*   pGoal = new Goal (new GetCommandCondition (m_enemyScoutID, c_cidGoto));
            pGoal->AddStartAction (new PlaySoundAction (tm_4_33rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (45.0f, tm_4_33rSound));
            pGoalList->AddGoal (pGoal);
        }
        
        {
            Goal*   pGoal = new Goal (new ElapsedTimeCondition (0.5f));

            // clear the player command set
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdCurrent, NA, NA, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdAccepted, NA, NA, c_cidDoNothing));
            pGoalList->AddGoal (pGoal);
        }

        {
            Goal*   pGoal = new Goal (pGoalList);

            // show the eyeball if it is appropriate
            ObjectID            shipID = trekClient.GetShip ()->GetObjectID ();
            Condition*          pOrConditionA = new OrCondition (new ShipSeesObjectCondition (m_enemyMiner1ID, OT_ship, shipID), new ShipSeesObjectCondition (m_enemyMiner2ID, OT_ship, shipID));
            Condition*          pOrConditionB = new OrCondition (new ShipSeesObjectCondition (m_enemyMiner3ID, OT_ship, shipID), new ShipSeesObjectCondition (m_enemyMiner4ID, OT_ship, shipID));
            Condition*          pSeesCondition = new OrCondition (new OrCondition (pOrConditionA, pOrConditionB), new ShipSeesObjectCondition (m_enemyScoutID, OT_ship, shipID));
            pGoal->AddConstraintCondition (new ConditionalAction (pSeesCondition, new ShowEyeballAction (true)));
            pGoal->AddConstraintCondition (new ConditionalAction (new NotCondition (new ProxyCondition (pSeesCondition)), new ShowEyeballAction (false)));

            return pGoal;
        }
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal09 (void)
    {
        GoalList*   pGoalList = new GoalList;


		// tm_4_34
		// Good. Now I'll give our constructor an order to build a 
		// station on an asteroid. Different kinds of asteroids are 
		// good for different kinds of buildings. Different buildings 
		// perform different tasks. In this case, we are building an 
		// Expansion Complex, to enable us to improve our technologies 
		// in this locality. It will also be a suitable landing place 
		// for our miners.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_34Sound);
            pGoal->AddStartAction (new SetCommandAction (trekClient.GetShip (), c_cmdQueued, OT_ship, m_builderID, c_cidGoto));
            pGoal->AddStartAction (new MessageAction ("Press the INSERT key to accept the command."));
            pGoalList->AddGoal (pGoal);
        }

        // (Wait for building to complete)
        {
            Goal*   pGoal = new Goal(new GetShipIsDestroyedCondition2(OT_ship, m_builderID));
            pGoal->AddStartAction(new SetCommandAction(m_builderID, c_cmdCurrent, OT_asteroid, static_cast<ObjectID>(10332), c_cidBuild));
            pGoal->AddStartAction(new SetCommandAction(m_builderID, c_cmdAccepted, OT_asteroid, static_cast<ObjectID>(10332), c_cidBuild));
            pGoalList->AddGoal(pGoal);
        }

        // (Wait for command acceptance)
        pGoalList->AddGoal (new Goal (new GetCommandCondition (trekClient.GetShip (), c_cidGoto)));

        // wait a few seconds
        {
            Goal*   pGoal = new Goal (new ElapsedTimeCondition (4.0f));
            pGoal->AddStartAction (new SetAutopilotAction (trekClient.GetShip (), false));
            pGoalList->AddGoal (pGoal);
        }

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission4::CreateGoal10 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_4_35
		// We've occupied and secured this sector! I'll send some 
		// patrols out to make sure we keep control, but it's time for 
		// you to prepare for a tour of the command center. Land in 
		// the new Expansion Complex when you're ready. Remember to 
		// land in a hangar with a green door.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_4_35Sound);
            pGoal->AddStartAction (new MessageAction ("Land in the expansion complex."));
            pGoalList->AddGoal (pGoal);
        }

		// tm_3_49r
		// Land the ship in a green hangar bay to complete the mission.
        {
            Goal*   pGoal = new Goal (new FalseCondition);
            pGoal->AddConstraintCondition (CreateTooLongCondition (45.0f, tm_3_49rSound));
            pGoalList->AddGoal (pGoal);
        }

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
}
