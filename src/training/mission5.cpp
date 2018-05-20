/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	Mission5.cpp
**
**  Author:  
**
**  Description:
**      Implementation of the fourth training mission
**
**  History:
*/
#include    "pch.h"
#include    "Mission5.h"

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
#include    "SetControlConstraintsAction.h"
#include    "GetAutopilotCondition.h"
#include    "SetAutopilotAction.h"
#include    "GetTargetCondition.h"
#include    "CreateDroneAction.h"
#include    "GetPickedCondition.h"
#include    "GetCommandCondition.h"
#include    "GetViewSectorCondition.h"
#include    "SetHUDOverlayAction.h"
#include    "GetShipIsDestroyedCondition.h"
#include    "SetSignatureAction.h"
#include    "ShowPaneAction.h"
#include    "GetKeyCondition.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  Mission5::~Mission5 (void)
    {
    }

    //------------------------------------------------------------------------------
    int         Mission5::GetMissionID (void)
    {
        return c_TM_5_Command_View;
    }

    //------------------------------------------------------------------------------
    SectorID    Mission5::GetStartSectorID (void)
    {
        return 1050;
    }

    //------------------------------------------------------------------------------
    void        Mission5::SetupShipAndCamera (void)
    {
        // needed parameters
        IshipIGC*       pShip = trekClient.GetShip ();
        ImissionIGC*    pCore = trekClient.GetCore ();
        IstationIGC*    pStation = pCore->GetStation (1050);
        IclusterIGC*    pCluster = pCore->GetCluster(GetStartSectorID ());

        // set up the ship
        //trekClient.ResetClusterScanners(pShip->GetSide());

        // put the ship in the station
        pShip->SetStation (pStation);

        // set up the outside view from the station
        trekClient.SetViewCluster (pCluster);

        // set up the camera
        TrekWindow*     pWindow = GetWindow ();
        pWindow->SetViewMode (TrekWindow::vmCommand);
        pWindow->PositionCommandView (NULL, 0.0);
        pWindow->SetRadarMode (RadarImage::c_rlAll);
        pWindow->EnableDisplacementCommandView (true);
    }

    //------------------------------------------------------------------------------
    bool        Mission5::RecordKeyPress (TrekKey key)
    {
        // check the key for tm conditions
        switch (key)
        {
            // XXX hack to disable some keys in training
            case TK_ConModeCombat:
            case TK_ConModeInventory:
            case TK_ViewExternalOrbit:
            case TK_ViewExternalStation:
            case TK_ViewExternal:
            case TK_ViewFlyby:
            case TK_ViewMissile:
            case TK_ViewTarget:
            case TK_ViewChase:
            case TK_ViewTurret:
            case TK_ViewBase:
            case TK_ViewSector:
            return false;
        }
        return TrainingMission::RecordKeyPress (key);
    }

    //------------------------------------------------------------------------------
    void        Mission5::CreateUniverse (void)
    {
        LoadUniverse ("training_5", 412, 1050);    // an intercepter

        // put the commander ship in the station
        ImissionIGC*    pCore = trekClient.GetCore();
        ImodelIGC*      pStation = pCore->GetModel (OT_station, 1050);
        IshipIGC*       pCommander = pCore->GetShip (m_commanderID);
        pCommander->SetStation (static_cast<IstationIGC*> (pStation));
        pCommander->SetCommand (c_cmdAccepted, NULL, c_cidDoNothing);
        pCommander->SetCommand (c_cmdCurrent, NULL, c_cidDoNothing);
        pCommander->SetAutopilot (false);
    }

    //------------------------------------------------------------------------------
    Condition*  Mission5::CreateMission (void)
    {
        GoalList*           pGoalList = new GoalList;
        Goal*               pGoal = new Goal (pGoalList);
        ImissionIGC*        pMission = trekClient.GetCore ();
        IstationIGC*        pStationNeptune = pMission->GetStation (1050);
        IstationIGC*        pStationMars = pMission->GetStation (1052);
        IwarpIGC*           pAlephNeptune = pMission->GetWarp (1050);
        IwarpIGC*           pAlephMars = pMission->GetWarp (1052);

        //Enable "Take me to" button in F6 window
        m_commandViewEnabled = true;

        // generate ship ids that we'll be using throughout the mission
        m_enemyScoutID = pMission->GenerateNewShipID ();
        m_miner1ID = pMission->GenerateNewShipID ();
        m_miner2ID = pMission->GenerateNewShipID ();
        m_miner3ID = pMission->GenerateNewShipID ();
        m_miner4ID = pMission->GenerateNewShipID ();
        m_scoutID = pMission->GenerateNewShipID ();
        m_builder1ID = pMission->GenerateNewShipID ();
        m_builder2ID = pMission->GenerateNewShipID ();
        m_builder3ID = pMission->GenerateNewShipID ();

        // add the goals to the list
        pGoalList->AddGoal (CreateGoal01 ());
        pGoalList->AddGoal (CreateGoal02 ());
        pGoalList->AddGoal (CreateGoal03 ());
        pGoalList->AddGoal (CreateGoal04 ());
        pGoalList->AddGoal (CreateGoal05 ());

        // create the tower builder drones
        Vector              pos = pStationNeptune->GetPosition () + Vector (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
        CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Tower Layer 01", m_builder1ID, 418, 0, c_ptLayer);
        pCreateDroneAction->SetExpendableType (87);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID (), pos);
        pGoal->AddStartAction (pCreateDroneAction);

		pos = pStationNeptune->GetPosition () + Vector (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
		pCreateDroneAction = new CreateDroneAction ("Tower Layer 02", m_builder2ID, 418, 0, c_ptLayer);
        pCreateDroneAction->SetExpendableType (87);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID (), pos);
        pGoal->AddStartAction (pCreateDroneAction);

        pos = pStationNeptune->GetPosition () + Vector (random(-300.0f, 300.0f), random(-300.0f, 300.0f), random(-300.0f, 300.0f));
        pCreateDroneAction = new CreateDroneAction ("Tower Layer 03", m_builder3ID, 418, 0, c_ptLayer);
        pCreateDroneAction->SetExpendableType (87);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID (), pos);
        pGoal->AddStartAction (pCreateDroneAction);

        // create the miners
        pos = pStationNeptune->GetPosition () + Vector (random(-600.0f, 600.0f), random(-600.0f, 600.0f), random(-300.0f, 300.0f));
        pCreateDroneAction = new CreateDroneAction ("Miner 01", m_miner1ID, 438, 0, c_ptMiner);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID (), pos);
        pGoal->AddStartAction (pCreateDroneAction);

        pos = pStationMars->GetPosition () + Vector (random(-600.0f, 600.0f), random(-600.0f, 600.0f), random(-300.0f, 300.0f));
        pCreateDroneAction = new CreateDroneAction ("Miner 02", m_miner2ID, 438, 0, c_ptMiner);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID () + 1, pos);
        pGoal->AddStartAction (pCreateDroneAction);

        pos = pStationMars->GetPosition () + Vector (random(-600.0f, 600.0f), random(-600.0f, 600.0f), random(-300.0f, 300.0f));
        pCreateDroneAction = new CreateDroneAction ("Miner 03", m_miner3ID, 438, 0, c_ptMiner);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID () + 1, pos);
        pGoal->AddStartAction (pCreateDroneAction);

        pos = pStationMars->GetPosition () + Vector (random(-600.0f, 600.0f), random(-600.0f, 600.0f), random(-300.0f, 300.0f));
        pCreateDroneAction = new CreateDroneAction ("Miner 04", m_miner4ID, 438, 0, c_ptMiner);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID () + 1, pos);
        pGoal->AddStartAction (pCreateDroneAction);

        // create the scout
        pos = pStationMars->GetPosition () + ((pAlephMars->GetPosition () - pStationMars->GetPosition ()) * random (0.5f, 0.75f));
        pCreateDroneAction = new CreateDroneAction ("Stealth Fighter", m_scoutID, 411, 0, c_ptWingman);
        pCreateDroneAction->SetCreatedLocation (GetStartSectorID () + 1, pos);
        pGoal->AddStartAction (pCreateDroneAction);

        // tell all of these guys to do nothing
        pGoal->AddStartAction (new SetCommandAction (m_scoutID, c_cmdAccepted, NULL, c_cidDoNothing));
        pGoal->AddStartAction (new SetCommandAction (m_scoutID, c_cmdCurrent, NULL, c_cidDoNothing));

        pGoal->AddStartAction (new SetCommandAction (m_miner2ID, c_cmdAccepted, NULL, c_cidDoNothing));
        pGoal->AddStartAction (new SetCommandAction (m_miner2ID, c_cmdCurrent, NULL, c_cidDoNothing));

        pGoal->AddStartAction (new SetCommandAction (m_miner3ID, c_cmdAccepted, NULL, c_cidDoNothing));
        pGoal->AddStartAction (new SetCommandAction (m_miner3ID, c_cmdCurrent, NULL, c_cidDoNothing));

        pGoal->AddStartAction (new SetCommandAction (m_miner4ID, c_cmdAccepted, NULL, c_cidDoNothing));
        pGoal->AddStartAction (new SetCommandAction (m_miner4ID, c_cmdCurrent, NULL, c_cidDoNothing));

        pGoal->AddStartAction (new SetCommandAction (m_builder1ID, c_cmdAccepted, NULL, c_cidDoNothing));
        pGoal->AddStartAction (new SetCommandAction (m_builder1ID, c_cmdCurrent, NULL, c_cidDoNothing));

        pGoal->AddStartAction (new SetCommandAction (m_builder2ID, c_cmdAccepted, NULL, c_cidDoNothing));
        pGoal->AddStartAction (new SetCommandAction (m_builder2ID, c_cmdCurrent, NULL, c_cidDoNothing));

        pGoal->AddStartAction (new SetCommandAction (m_builder3ID, c_cmdAccepted, NULL, c_cidDoNothing));
        pGoal->AddStartAction (new SetCommandAction (m_builder3ID, c_cmdCurrent, NULL, c_cidDoNothing));

        return pGoal;
    }

    //------------------------------------------------------------------------------
    Goal*       Mission5::CreateGoal01 (void)
    {
        GoalList*   pGoalList = new GoalList;

        // wait two a second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (1.0f)));

		// tm_5_01
		// You can move your view around by moving the mouse to the 
		// edge of the screen. You can also adjust the field of view 
		// using the PLUS and MINUS keys. Using the targeting keys you 
		// learned in training mission 3 is also good practice, 
		// because that will move your view to the targeted object.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_01Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_02
		// Practice manipulating your view, and then press the 
		// SPACEBAR when you are ready to proceed.
        {
            Goal*   pGoal = new Goal (new GetKeyCondition (TK_ThrustForward));
            pGoal->AddStartAction (new MessageAction ("Press the SPACEBAR when you are ready to proceed."));
            pGoal->AddStartAction (new PlaySoundAction (tm_5_02Sound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_02Sound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_03
		// You can buy items like towers and mines to defend your 
		// bases and block off alephs. There are three tower layers 
		// waiting for orders here. We want to deploy the towers to 
		// defend the aleph from incoming enemies. 
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_03Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_03r
		// First, left-click a tower layer.
        {
            Condition*  pOrConditionA = new OrCondition (new GetPickedCondition (OT_ship, m_builder1ID), new GetPickedCondition (OT_ship, m_builder2ID));
            Condition*  pConditionB = new GetPickedCondition (OT_ship, m_builder3ID);
            Goal*       pGoal = new Goal (new OrCondition (pOrConditionA, pConditionB));
            pGoal->AddStartAction (new MessageAction ("Left click on a Tower Layer to select it."));
            pGoal->AddStartAction (new PlaySoundAction (tm_5_03rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_03rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_04
		// Now, left-click the Build command. If you click and drag 
		// when ordering towers around, you can set the height of the 
		// deployment position off the horizontal plane. 
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_5_04Sound);
            pGoal->AddStartAction (new MessageAction ("Left click on 'Build' command."));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_04r
		// Right-click in space and drag to indicate where you want 
		// the tower to go. 
        {
            Condition*  pOrConditionA = new OrCondition (new GetCommandCondition (m_builder1ID, c_cidBuild), new GetCommandCondition (m_builder2ID, c_cidBuild));
            Condition*  pConditionB = new GetCommandCondition (m_builder3ID, c_cidBuild);
            Goal*       pGoal = new Goal (new OrCondition (pOrConditionA, pConditionB));
            pGoal->AddStartAction (new MessageAction ("Right click in space to deploy tower."));
            pGoal->AddStartAction (new PlaySoundAction (tm_5_04rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_04rSound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_05
		// Now order all the towers to deploy in the same area. We 
		// don't want any enemies coming in.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_5_05Sound);
            pGoal->AddStartAction (new MessageAction ("Order all towers to defend aleph 'Mars'."));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_05r
		// Order all the towers to deploy near the aleph.
        {
            Condition*  pAndCondition = new AndCondition (new GetCommandCondition (m_builder1ID, c_cidBuild), new GetCommandCondition (m_builder2ID, c_cidBuild));
            Condition*  pConditionB = new GetCommandCondition (m_builder3ID, c_cidBuild);
            Goal*       pGoal = new Goal (new AndCondition (pAndCondition, pConditionB));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_05rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission5::CreateGoal02 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_5_06
		// Good job! You can watch those guys build if you want. 
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_06Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_06r
		// When you are ready, left-click sector Mars in the Map 
		// display to change the sector you are viewing.
        {
            Goal*   pGoal = new Goal (new GetViewSectorCondition (GetStartSectorID () + 1));
            pGoal->AddStartAction (new MessageAction ("Left click on 'Mars' in the Sector Map to change view."));
            pGoal->AddStartAction (new SetHUDOverlayAction (SectorTrainingOverlay));
            pGoal->AddStartAction (new PlaySoundAction (tm_5_06rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_06rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_07
		// Now you are looking at sector Mars. There is a friendly 
		// base here with several friendly craft: three miners and a 
		// stealth fighter on patrol. They are waiting for further 
		// orders. First, identify a miner and tell it to go mine 
		// Helium3. Start by using the mouse to left-click it.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_5_07Sound);
            pGoal->AddStartAction (new SetHUDOverlayAction (NoTrainingOverlay));
            pGoal->AddStartAction (new MessageAction ("Left click on a Miner to select it."));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_07r
		// Left-click on a miner.
        {
            Condition*  pOrConditionA = new OrCondition (new GetPickedCondition (OT_ship, m_miner2ID), new GetPickedCondition (OT_ship, m_miner3ID));
            Condition*  pConditionB = new GetPickedCondition (OT_ship, m_miner4ID);
            Goal*       pGoal = new Goal (new OrCondition (pOrConditionA, pConditionB));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_07rSound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_08
		// Now order the miner to go mine a Helium3 asteroid. You can 
		// find the asteroid by looking for one whose name starts with 
		// He and  looks like a bright white rock icon, or you can use 
		// the mouse to point at asteroids until the cursor changes to 
		// the Mine icon. You can left-click the Mine command and then 
		// right-click the Helium3 asteroid.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_5_08Sound);
            pGoal->AddStartAction (new MessageAction ("Left click on 'Mine' command, and right click on the He3 asteroid to begin mining."));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_08r
		// Right-click on the Helium asteroid you want to mine.
        {
            Condition*  pOrConditionA = new OrCondition (new GetCommandCondition (m_miner2ID, c_cidMine), new GetCommandCondition (m_miner3ID, c_cidMine));
            Condition*  pConditionB = new GetCommandCondition (m_miner4ID, c_cidMine);
            Goal*       pGoal = new Goal (new OrCondition (pOrConditionA, pConditionB));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_08rSound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_09
		// Nicely done. Miners will leave an asteroid they are working 
		// at when another miner approaches. If this happens, the 
		// miner may not be at full capacity when it leaves, and your 
		// paydays will be smaller. It is more efficient to have 
		// different miners at different asteroids, rather than 
		// ganging them up on a single asteroid.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_09Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_10
		// Get all the miners working.
        {
            Condition*  pAndConditionA = new AndCondition (new GetCommandCondition (m_miner2ID, c_cidMine), new GetCommandCondition (m_miner3ID, c_cidMine));
            Condition*  pConditionB = new GetCommandCondition (m_miner4ID, c_cidMine);
            Goal*       pGoal = new Goal (new AndCondition (pAndConditionA, pConditionB));
            pGoal->AddStartAction (new MessageAction ("Order all the miners to work."));
            pGoal->AddStartAction (new PlaySoundAction (tm_5_10Sound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_10Sound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_11
		// Perfect. To speed up mining, you can build Outposts and 
		// other buildings to shorten the round-trip for miners. If 
		// the miners have to go to a different sector to mine, that 
		// can make a big difference.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_11Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission5::CreateGoal03 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_5_12
		// Attention, Cadet, new contact: an enemy scout. In some 
		// cases, utility craft like miners will run home for 
		// protection, but you can tell them to hide by giving them 
		// orders to go to a base. You left-click the miners, then 
		// left-click the Go To command, and then right-click ÿ the 
		// base.
        {
            Goal*               pGoal = CreatePlaySoundGoal (tm_5_12Sound);
            IwarpIGC*           pAlephMars = trekClient.GetCore ()->GetWarp (1052);
            Orientation         alephMarsOrientation = pAlephMars->GetOrientation ();

            // create the enemy scout
            Vector              pos = pAlephMars->GetPosition () + (alephMarsOrientation.GetForward () * random (800.0f, 1200.0f)) + (alephMarsOrientation.GetRight () * random(-100.0f, 200.0f));
            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Enemy Scout", m_enemyScoutID, 310, 1, c_ptWingman);
            pCreateDroneAction->SetCreatedLocation (GetStartSectorID () + 1, pos);
            pGoal->AddStartAction (pCreateDroneAction);

            // tell the enemy scout of these guys to do nothing
            pGoal->AddStartAction (new SetCommandAction (m_enemyScoutID, c_cmdAccepted, NULL, c_cidDoNothing));
            pGoal->AddStartAction (new SetCommandAction (m_enemyScoutID, c_cmdCurrent, NULL, c_cidDoNothing));

            // set its signature to infinity
            pGoal->AddStartAction (new SetSignatureAction (OT_ship, m_enemyScoutID, 1.0e6f));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_13
		// Most enemy ships can be considered a threat to your miners 
		// unless you have plenty of defending pilots ? which you 
		// don?t. So let's get those miners to shelter ? send all 
		// three of them to the base.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_13Sound));

		// tm_5_13r
		// Send all three of your miners to base for safety.
        {
            Condition*  pAndConditionA = new AndCondition (new GetCommandCondition (m_miner2ID, c_cidGoto), new GetCommandCondition (m_miner3ID, c_cidGoto));
            Condition*  pConditionB = new GetCommandCondition (m_miner4ID, c_cidGoto);
            Goal*       pGoal = new Goal (new AndCondition (pAndConditionA, pConditionB));
            pGoal->AddStartAction (new MessageAction ("Order all three miners back to base for safety."));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_13rSound));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_14
		// Good! Utility ships like miners and constructors are 
		// drones. They will always accept your orders.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_14Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission5::CreateGoal04 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_5_15
		// Now let's talk about what orders you can give combat 
		// pilots. You give commands to pilots the same way you give 
		// commands to utility craft. Pilots are real people, though, 
		// so they may not always accept the orders you give. 
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_5_15Sound);
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_15r
		// Give that stealth fighter an order to attack the enemy 
		// scout.
        {
            Goal*   pGoal = new Goal (new GetCommandCondition (m_scoutID, c_cidAttack));
            pGoal->AddStartAction (new MessageAction ("Order stealth fighter to attack enemy craft."));
            pGoal->AddStartAction (new PlaySoundAction (tm_5_15rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_15rSound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_16
		// Watch what happens.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_16Sound));

        // (Wait for one of the ships to be destroyed)
        pGoalList->AddGoal (new Goal (new GetShipIsDestroyedCondition2(OT_ship, m_enemyScoutID)));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission5::CreateGoal05 (void)
    {
        GoalList*   pGoalList = new GoalList;

		// tm_5_17
		// Sometimes you may want to give an order to a pilot but you 
		// don't know where he is.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_17Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_18
		// This is the Team display. Here, you can select a pilot in 
		// the list, and then click the "Take Me To" button to jump 
		// to where he is. Let's find where Miner 01 is right now.
        {
            Goal*   pGoal = CreatePlaySoundGoal (tm_5_18Sound);
            pGoal->AddStartAction (new ShowPaneAction (ofExpandedTeam));
            pGoalList->AddGoal (pGoal);
        }

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_18r
		// Select Miner 01 in the Team display, and click the "Take 
		// Me To" button.
        {
            Goal*   pGoal = new Goal (new GetViewSectorCondition (GetStartSectorID ()));
            pGoal->AddStartAction (new MessageAction ("Select Miner 01 in the Team Display, and click on the 'Take Me To' button."));
            pGoal->AddStartAction (new PlaySoundAction (tm_5_18rSound));
            pGoal->AddConstraintCondition (CreateTooLongCondition (30.0f, tm_5_18rSound));
            pGoalList->AddGoal (pGoal);
        }

		// tm_5_19
		// Notice that you are now viewing the sector that Miner 01 is 
		// in, and you are prepared to give it a command.
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_19Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

		// tm_5_20
		// The Team display is very helpful for keeping track of 
		// utility ships and getting the status of your team members. 
		// If a utility ship is docked, you can use that screen to 
		// send them back to work. You can press the F6 key to bring 
		// up the Team display ?
        pGoalList->AddGoal (CreatePlaySoundGoal (tm_5_20Sound));

        // wait half second
        pGoalList->AddGoal (new Goal (new ElapsedTimeCondition (0.5f)));

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
}
