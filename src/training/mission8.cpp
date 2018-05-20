/*
**
**  File:	MissionNan.cpp
**
**  Author:  Adam Davison
**
**  Description:
**      Implementation of the nanite training mission
**
**  History:
*/
#include "pch.h"
#include "Mission8.h"
#include "Training.h"
#include "GoalList.h"
#include "ConditionalAction.h"
#include "GetShipIsDestroyedCondition.h"
#include "GetShipHasMountedCondition.h"
#include "GetShipHasCargoCondition.h"
#include "FalseCondition.h"
#include "ElapsedTimeCondition.h"
#include "SetDisplayModeAction.h"
#include "CreateDroneAction.h"
#include "MessageAction.h"
#include "DoDamageAction.h"
#include "DoDamageAdjustableAction.h"
#include "SetCommandAction.h"
#include "notcondition.h"
#include "orcondition.h"
#include "andcondition.h"
#include "CreateWaypointAction.h"
#include "SetControlConstraintsAction.h"
#include "SetControlsAction.h"
#include "GetSectorCondition.h"
#include "ShowPaneAction.h"
#include "SetDroneNanAction.h"
#include "TurnToAction.h"
#include "objectpointingatcondition.h"
#include "GetShipIsStoppedCondition.h"
#include "PlaySoundAction.h"
#include "SetCargoNanAction.h"
#include "ResetAction.h"
#include "ResetShipAction.h"
#include "objectwithinradiuscondition.h"

namespace Training
{
    Mission8::Mission8(void) :
        bUndockHintShown(false)
    {
    }

	Mission8::~Mission8(void)
	{
	}

	int Mission8::GetMissionID(void)
	{
		return c_TM_8_Nanite; // For now
	}

	SectorID Mission8::GetStartSectorID(void)
	{
		return 1051;
	}

	/*
	bool Mission8::RestoreShip(void)
	{
		Terminate();
		return false;
	}
	*/

    //------------------------------------------------------------------------------
    bool        Mission8::ShipLanded(void)
    {
        if (!bUndockHintShown) {
            trekClient.GetCore()->GetIgcSite()->SendChatf(trekClient.GetCore()->GetShip(m_commanderID), CHAT_INDIVIDUAL, trekClient.GetShipID(),
                NA,
                "Hint: Click the hangar door to launch your ship back into space. Only the scout can mount the Nanite Repair System.");
            bUndockHintShown = true;
        }
        return true;
    }

	void Mission8::CreateUniverse(void)
	{
		LoadUniverse("training_nan", 685, 1051); // Adv Rix Scout in sector 1051; 3 (old) vs 685 difference: scan range reduced from 50000 to 3000 
		trekClient.fGroupFire = true;

        pShip = trekClient.GetShip();
        pStation = trekClient.GetSide()->GetStations()->first()->data();
        pMission = trekClient.GetCore();
        homeRedDoor = pStation->GetPosition();
        homeRedDoor.x += 50.0f;
        homeRedDoor.y += -200.0f;
        homeRedDoor.z += -20.0f;

		IshipIGC*		pCommanderU = pMission->GetShip(m_commanderID);
        pCommanderU->SetStation (static_cast<IstationIGC*> (pStation));
        pCommanderU->SetCommand (c_cmdAccepted, NULL, c_cidDoNothing);
        pCommanderU->SetCommand (c_cmdCurrent, NULL, c_cidDoNothing);
        pCommanderU->SetAutopilot (false);

        pShip->SetPosition(Vector(-1500.0f, 1500.0f, 0.0f));

        //Add tech
        TechTreeBitMask ttbm;
        ttbm.ClearAll();
        ttbm |= trekClient.GetCore()->GetPartType(190)->GetEffectTechs(); // so nan2 is allowed on undock
        ttbm |= trekClient.GetShip()->GetHullType()->GetEffectTechs();
        ttbm |= trekClient.GetShip()->GetHullType()->GetRequiredTechs();
        pShip->GetSide()->ApplyDevelopmentTechs(ttbm);

		// Take out the default probe
        IpartIGC* pPart = pShip->GetMountedPart(ET_Dispenser, 0);
		if (pPart) {
			pPart->Terminate();
		}
		// Put in a prox 1
		AddPartToShip(59, 0, 0x7fff);

		// Load a cargo a little more appropriate for a nan
		for (int i = -1; i >= -c_maxCargo; i--) { 
			pPart = pShip->GetMountedPart(NA, i);
            if (pPart)
            {
				pPart->Terminate();
            }
			if (i == -1) {
				// Put one nan in cargo
				AddPartToShip (190, i, 0x7fff);
			} else {
				// Fill the rest with spare prox
				AddPartToShip (59, i, 0x7fff);
			}
		}
        trekClient.SaveLoadout(pShip);

        //Add enemy tech
        ttbm.ClearAll();
        ttbm |= trekClient.GetCore()->GetPartType(60)->GetEffectTechs(); //Sniper 1 to nerf stealth fighters vs miners
        ttbm |= trekClient.GetCore()->GetPartType(60)->GetRequiredTechs();
        trekClient.GetCore()->GetSide(1)->ApplyDevelopmentTechs(ttbm);
	}

	Condition* Mission8::CreateMission(void)
	{
        m_commandViewEnabled = true;
        GoalList*                       pGoalList = new GoalList;

        pGoalList->AddGoal (CreateGoal01 ());
        pGoalList->AddGoal (CreateGoal02 ());
		pGoalList->AddGoal (CreateGoal03 ());

        // return the goal
        return new Goal(pGoalList);
	}

	    //------------------------------------------------------------------------------
    Goal*       Mission8::CreateGoal01 (void)
    {
        GoalList*   pGoalList = new GoalList;

        // play the introductory audio
        {
            Goal*                           pGoal = CreatePlaySoundGoal (salCommenceScanSound);
            pGoalList->AddGoal (pGoal);
        }

        {
            Goal*   pGoal = new Goal (new ElapsedTimeCondition (1.0f));
            pGoal->AddStartAction (new SetDisplayModeAction (TrekWindow::cmCockpit));
            pGoalList->AddGoal (pGoal);
        }

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_01Sound));
        {
            Goal*	pGoal = CreatePlaySoundGoal(tm_8_02Sound);
            pGoal->SetSkipGoalCondition(new GetShipHasMountedCondition(trekClient.GetShip(), 190));
            pGoal->AddStartAction(new MessageAction("F4 toggles the inventory. You will want it to be open."));
            pGoalList->AddGoal(pGoal);
        }

        {
            Goal*	pGoal = CreatePlaySoundGoal(tm_8_03Sound);
            pGoal->SetSkipGoalCondition(new GetShipHasMountedCondition(trekClient.GetShip(), 190));
            pGoalList->AddGoal(pGoal);
        }

		{
			Goal*	pGoal = new Goal (new GetShipHasMountedCondition(trekClient.GetShip(), 190 ) ); 
			pGoal->AddStartAction (new MessageAction("Swap the Nanite Repair System into slot 1 by pressing Ctrl+1.")); 
            pGoal->AddConstraintCondition(CreateTooLongCondition(40.0f, tm_8_03Sound));
			pGoalList->AddGoal(pGoal);
		}

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_04Sound));
		
		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition (3.0f));
			pStation->ReceiveDamage(c_dmgidNoWarn, 12500.0f, Time::Now(), Vector (-1.0f, 0.0f, 0.0f), Vector (1.0f, 0.0f, 0.0f), 0 );
			pGoalList->AddGoal(pGoal);
		}

		{	
			Goal*	pGoal = new Goal(new ElapsedTimeCondition (8.0f));
			pGoal->AddStartAction (new PlaySoundAction (tm_8_05Sound));
            pGoal->AddStartAction(new SetCommandAction(pShip, c_cmdCurrent, pStation, c_cidRepair)); //target the base
			pGoalList->AddGoal(pGoal);
		}

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_06Sound));
		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_07Sound));
		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_08Sound));
		//pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_09Sound)); //Enabling your controls now

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition (0.5f));;
			pGoal->AddStartAction (new MessageAction("Repair the station.", voRepairStationSound)); 
			pGoalList->AddGoal(pGoal);
		}

		{
			Goal*	pGoal = new Goal(new NotCondition(new GetShipIsDamagedCondition(OT_station, static_cast<ImodelIGC*>(pStation)->GetObjectID())));
			pGoalList->AddGoal(pGoal);
		}

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(6.0f));
			pGoal->AddStartAction(new MessageAction("Nice work.", voNiceWorkSound));
			pGoalList->AddGoal (pGoal);
		}
		
        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission8::CreateGoal02 (void)
    {
        GoalList*   pGoalList = new GoalList;
		ShipID      minerID = pMission->GenerateNewShipID ();
		ShipID		atID = pMission->GenerateNewShipID();
		ShipID		atID2 = pMission->GenerateNewShipID();
		ShipID		defID = pMission->GenerateNewShipID();
		ShipID		defID2 = pMission->GenerateNewShipID();
		ShipID		defID3 = pMission->GenerateNewShipID();
		/*  Code used when only testing Goal02
		{
            Goal*                           pGoal = CreatePlaySoundGoal (salCommenceScanSound);
            pGoalList->AddGoal (pGoal);
        }

		pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(3.0f)));

        {
            Goal*   pGoal = new Goal (new ElapsedTimeCondition (1.0f));
            pGoal->AddStartAction (new SetDisplayModeAction (TrekWindow::cmCockpit));
            pGoalList->AddGoal (pGoal);
        }
		*/

		{
			Goal*	pGoal = new Goal (new ElapsedTimeCondition(0.5f));

            // create friendly miner
            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction ("Miner 01", minerID, 437, 0, c_ptMiner);
            pCreateDroneAction->SetCreatedLocation (GetStartSectorID (), homeRedDoor);
			pGoal->AddStartAction(pCreateDroneAction);

			pGoalList->AddGoal (pGoal);			
		}

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_10Sound));
        {
            Goal*	pGoal = CreatePlaySoundGoal(tm_8_11Sound);
            pGoal->AddStartAction(new SetCommandAction(pShip, c_cmdCurrent, OT_ship, minerID, c_cidGoto)); //target miner
            pGoalList->AddGoal(pGoal);
        }

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_12Sound));

		pGoalList->AddGoal (new Goal(new ElapsedTimeCondition(16.0f)));

        //Create enemy stealth fighters
		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(1.5f));

            Vector pos = Vector(-2600.0f, 2600.0f, 0.0f);
			
			CreateDroneAction*  pCreateDroneAction = new CreateDroneAction("solap", atID, 386, 1, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(1051, pos);
            pCreateDroneAction->SetCreatedBehaviour(c_wbbmUseMissiles);
			pGoal->AddStartAction(pCreateDroneAction);
			pGoal->AddStartAction(new SetCommandAction(atID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(minerID), c_cidAttack));

			pos.y = 4000.0f;
			pCreateDroneAction = new CreateDroneAction("toast", atID2, 386, 1, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(1051, pos);
            pCreateDroneAction->SetCreatedBehaviour(c_wbbmUseMissiles);
			pGoal->AddStartAction(pCreateDroneAction);
			pGoal->AddStartAction(new SetCommandAction(atID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(minerID), c_cidAttack));

			pGoalList->AddGoal (pGoal);
		}

        //Create defenders
		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(1.0f));

			CreateDroneAction* pCreateDroneAction = new CreateDroneAction("TheBored", defID, 615, 0, c_ptWingman);
            pCreateDroneAction->SetCreatedLocation(1051, homeRedDoor + Vector(100.0f, 0.0f, 10.0f));
            pCreateDroneAction->SetCreatedOrientation(Vector(0.0f, -1.0f, 0.0f), Vector(0.0f, 0.0f, 1.0f));
            pCreateDroneAction->SetCreatedBehaviour(c_wbbmRunAt60Hull);

			pGoal->AddStartAction(pCreateDroneAction);

			pGoal->AddStartAction(new SetCommandAction(defID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(atID), c_cidAttack));

			pGoalList->AddGoal (pGoal);
		}

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(0.5f));

			CreateDroneAction* pCreateDroneAction = new CreateDroneAction("AEM", defID2, 615, 0, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(1051, homeRedDoor+ Vector(-20.0f, 0.0f, 10.0f));
            pCreateDroneAction->SetCreatedOrientation(Vector(0.0f, -1.0f, 0.0f), Vector(0.0f, 0.0f, 1.0f));
			pGoal->AddStartAction(pCreateDroneAction);
            pCreateDroneAction->SetCreatedBehaviour(c_wbbmRunAt60Hull);

			pGoal->AddStartAction(new SetCommandAction(defID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(atID), c_cidAttack));

			pGoalList->AddGoal (pGoal);
		}

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(1.0f));

			CreateDroneAction* pCreateDroneAction = new CreateDroneAction("lawson", defID3, 615, 0, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(1051, homeRedDoor+Vector(0.0f, 0.0f, -10.0f));
            pCreateDroneAction->SetCreatedOrientation(Vector(0.0f, -1.0f, 0.0f), Vector(0.0f, 0.0f, 1.0f));
			pGoal->AddStartAction(pCreateDroneAction);
            pCreateDroneAction->SetCreatedBehaviour(0);

			pGoal->AddStartAction(new SetCommandAction(defID3, c_cmdAccepted, OT_ship, static_cast<ObjectID>(atID), c_cidAttack));

			pGoalList->AddGoal (pGoal);
		}

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_13Sound));
		pGoalList->AddGoal (new Goal(new ElapsedTimeCondition(2.0f)));

		{
            //new Goal( Both stealth fighters destroyed )
			Goal*	pGoal = new Goal(new AndCondition(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(atID), 0.0f)), new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(atID2), 0.0f))));
			pGoal->AddStartAction(new MessageAction("Defend miners.", voDefendMinerSound));

            //stealth fighters idle once the miner is destroyed
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(minerID), 0.0f)), new SetCommandAction(atID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(minerID), c_cidDoNothing), true, true ));
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(minerID), 0.0f)), new SetCommandAction(atID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(minerID), c_cidDoNothing), true, true ));
			
            //Miners are dead?
            Condition*                      pMinerDeadCondition = new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(minerID), 0.0f));
            pGoal->AddConstraintCondition(new ConditionalAction(pMinerDeadCondition, new MessageAction("Our miners are dead!", voMinersAreDeadSound), false, true));

            //Switch defender targets
			Condition*                      pAttackerDeadCondition = new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(atID), 0.0f));
			Condition*                      pAttackerDeadCondition2 = new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(atID), 0.0f));
			Condition*                      pAttackerDeadCondition3 = new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(atID), 0.0f));
			SetCommandAction*				pKillOtherAttackerAction = new SetCommandAction(defID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(atID2), c_cidAttack);
			SetCommandAction*				pKillOtherAttackerAction2 = new SetCommandAction(defID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(atID2), c_cidAttack);
			SetCommandAction*				pKillOtherAttackerAction3 = new SetCommandAction(defID3, c_cmdAccepted, OT_ship, static_cast<ObjectID>(atID2), c_cidAttack);
			ConditionalAction*              pConditionalAction = new ConditionalAction (pAttackerDeadCondition, pKillOtherAttackerAction, false, true);
			ConditionalAction*              pConditionalAction3 = new ConditionalAction (pAttackerDeadCondition2, pKillOtherAttackerAction2, false, true);
			ConditionalAction*              pConditionalAction5 = new ConditionalAction (pAttackerDeadCondition3, pKillOtherAttackerAction3, false, true);
			pGoal->AddConstraintCondition (pConditionalAction);
			pGoal->AddConstraintCondition (pConditionalAction3);
			pGoal->AddConstraintCondition (pConditionalAction5);

			pGoalList->AddGoal (pGoal);
		}
		
		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(3.0f));
			Condition*                      pMinerAliveCondition = new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(minerID), 0.0f);
			//Condition*                      pMinerDeadCondition = new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(minerID), 0.0f));
			pGoal->AddConstraintCondition ( new ConditionalAction( pMinerAliveCondition, new MessageAction("Yee-haaaaaw!", voYeeHaSound), false, true));
			//pGoal->AddConstraintCondition ( new ConditionalAction( pMinerDeadCondition, new MessageAction("Our miners are dead!", voMinersAreDeadSound), false, true));
			pGoalList->AddGoal (pGoal);
		}

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(8.0f));
			Condition*                      pMinerAliveCondition = new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(minerID), 0.0f);
			Condition*                      pMinerDeadCondition = new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(minerID), 0.0f));
			pGoal->AddConstraintCondition ( new ConditionalAction( pMinerAliveCondition, new PlaySoundAction (tm_8_14Sound), false, true));
			pGoal->AddConstraintCondition ( new ConditionalAction( pMinerDeadCondition, new PlaySoundAction (tm_8_15Sound), false, true));
			pGoalList->AddGoal (pGoal);
		}

        return new Goal (pGoalList);
    }

    //------------------------------------------------------------------------------
    Goal*       Mission8::CreateGoal03 (void)
    {

		GoalList*   pGoalList = new GoalList;
		ShipID		bomberShipID = pMission->GenerateNewShipID();
		ShipID		nanShipID = pMission->GenerateNewShipID();
		BuoyID		alephBuoyID1 = pMission->GenerateNewBuoyID();
        ImodelIGC*	pEnemyStation = pMission->GetModel(OT_station, 1051);
        pEnemyStation->SetSideVisibility(pShip->GetSide(), true);


		/*  //Code used when only testing Goal03
		{
            Goal*                           pGoal = CreatePlaySoundGoal (salCommenceScanSound);
            pGoalList->AddGoal (pGoal);
        }

		pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(3.0f)));

        {
            Goal*   pGoal = new Goal (new ElapsedTimeCondition (1.0f));
            pGoal->AddStartAction (new SetDisplayModeAction (TrekWindow::cmCockpit));
            pGoalList->AddGoal (pGoal);
        }
		//*/

        //spawn bomber
		{
            Goal*       pGoal = new Goal(new ElapsedTimeCondition(2.0f));
			CreateDroneAction* pCreateDroneAction = new CreateDroneAction("Commander", bomberShipID, 604, 0, c_ptWingman, 0);
            pCreateDroneAction->SetCreatedLocation(GetStartSectorID(), homeRedDoor); //Vector(-2500.0f, 2500.0f, 0.0f));
			pGoal->AddStartAction(pCreateDroneAction);
            pGoalList->AddGoal(pGoal);
		}

		{
			Goal* pGoal = new Goal(new ElapsedTimeCondition (2.0f));
			pGoal->AddStartAction(new CreateWaypointAction(alephBuoyID1, Vector(-2500.0f, 2500.0f, 0.0f), 1051));
			pGoal->AddStartAction(new SetCommandAction(bomberShipID, c_cmdCurrent, OT_buoy, alephBuoyID1, c_cidGoto));
			pGoal->AddStartAction(new SetCommandAction(bomberShipID, c_cmdAccepted, OT_buoy, alephBuoyID1, c_cidGoto));
			pGoalList->AddGoal (pGoal);
		}

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(10.0f));
			pGoal->AddStartAction(new PlaySoundAction (tm_8_16Sound));
			pGoal->AddStartAction(new MessageAction("Meet me in front of the aleph."));
			pGoal->AddStartAction(new SetCommandAction(trekClient.GetShipID(), c_cmdQueued, OT_buoy, alephBuoyID1, c_cidGoto));
			pGoalList->AddGoal (pGoal);
		}

		{
			Goal* pGoal = new Goal(new ElapsedTimeCondition (3.0f));
			
			CreateDroneAction* pCreateDroneAction = new CreateDroneAction("bahdohday", nanShipID, 685, 0, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(GetStartSectorID(), homeRedDoor);

			pGoal->AddStartAction(pCreateDroneAction);
			pGoal->AddStartAction(new SetDroneNanAction(nanShipID));

			pGoal->AddStartAction(new SetCommandAction(nanShipID, c_cmdCurrent, OT_buoy, alephBuoyID1, c_cidGoto));
			pGoal->AddStartAction(new SetCommandAction(nanShipID, c_cmdAccepted, OT_buoy, alephBuoyID1, c_cidGoto));

			pGoalList->AddGoal(pGoal);
		}
	
		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_17Sound));

		{
			Goal*	pGoal = new Goal(new GetShipIsStoppedCondition(bomberShipID));
			pGoal->AddStartAction(new PlaySoundAction (tm_8_18Sound));
			pGoalList->AddGoal (pGoal);
		}

        //Wait for player to be in range
        {
            Goal*   pGoal = new Goal(new ObjectWithinRadiusCondition(static_cast<ImodelIGC*> (pShip), OT_ship, bomberShipID, 500.0f));
            pGoal->AddConstraintCondition(CreateTooLongCondition(40.0f, tm_8_16Sound));
            pGoalList->AddGoal(pGoal);
        }

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(4.0f));
			pGoal->AddStartAction(new MessageAction("Wait for signal.", voWaitForSignalSound));
			pGoalList->AddGoal (pGoal);
		}

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_19Sound));

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(5.0f));
			pGoal->AddStartAction(new MessageAction("Everybody ready?", voEveryoneReadySound));
			pGoalList->AddGoal (pGoal);
		}

		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_20Sound));
        
		{
            BuoyID alephBuoyID = pMission->GenerateNewBuoyID();

            Goal*	pGoal = new Goal(new GetSectorCondition(OT_ship, static_cast<ObjectID>(bomberShipID), 1052));
			pGoal->AddStartAction(new CreateWaypointAction(alephBuoyID, Vector(0.0f, 0.0f, 0.0f), 1052));

            Condition*          pPeriodicCondition = new PeriodicCondition(new TrueCondition, 30.0f);
            Condition*          pPeriodicCondition2 = new PeriodicCondition(new TrueCondition, 30.0f); 
            Condition*          pPeriodicCondition3 = new PeriodicCondition(new TrueCondition, 30.0f);
            Condition*          pPeriodicCondition4 = new PeriodicCondition(new TrueCondition, 30.0f);
            pGoal->AddConstraintCondition(new ConditionalAction(pPeriodicCondition, new MessageAction("Go Go Go!", voGoGoGoSound)));
            pGoal->AddConstraintCondition(new ConditionalAction(pPeriodicCondition2, new CreateWaypointAction(alephBuoyID, Vector(0.0f, 0.0f, 0.0f), 1052)));
            pGoal->AddConstraintCondition(new ConditionalAction(pPeriodicCondition3, new SetCommandAction(bomberShipID, c_cmdCurrent, OT_buoy, alephBuoyID, c_cidGoto))); //resend bbr in case player gave it commands
            pGoal->AddConstraintCondition(new ConditionalAction(pPeriodicCondition4, new SetCommandAction(bomberShipID, c_cmdAccepted, OT_buoy, alephBuoyID, c_cidGoto))); //resend bbr in case player gave it commands

            pGoal->AddStartAction(new SetCommandAction(nanShipID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidGoto));
            pGoal->AddStartAction(new SetCommandAction(nanShipID, c_cmdCurrent, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidGoto));

			pGoalList->AddGoal (pGoal);
		}

		{
			Goal*	pGoal = new Goal(new ElapsedTimeCondition(2.0f));
			pGoal->AddStartAction(new SetCommandAction(bomberShipID, c_cmdCurrent, pEnemyStation, c_cidAttack));
			pGoal->AddStartAction(new SetCommandAction(bomberShipID, c_cmdAccepted, pEnemyStation, c_cidAttack));
			pGoalList->AddGoal(pGoal);
		}
		
        ShipID defID = pMission->GenerateNewShipID();
        ShipID defID2 = pMission->GenerateNewShipID();
        ShipID defID3 = pMission->GenerateNewShipID();

		{
            //Spawn enemy defenders
            // defID scout  Tkela       -> nan (bahoday)
            // defID2 scout ribski      -> nan
            // defID3 fig   dontmess    -> bomber

			Goal*	pGoal = new Goal(new ElapsedTimeCondition (3.0f));
			Vector pos(0.0f, -300.0f, 400.0f);
			Vector pos2(0.0f, 100.0f, 300.0f);
			
			CreateDroneAction* pCreateDroneAction = new CreateDroneAction("Tkela", defID, 385, 1, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(1052, pos);
            pCreateDroneAction->SetCreatedBehaviour(c_wbbmRunAt30Hull);
            pCreateDroneAction->SetCreatedCommand(nanShipID, OT_ship, c_cidAttack);
			pGoal->AddStartAction(pCreateDroneAction);
			
			pCreateDroneAction = new CreateDroneAction("ribski", defID2, 385, 1, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(1052, pos2);
            pCreateDroneAction->SetCreatedBehaviour(c_wbbmRunAt30Hull);
            pCreateDroneAction->SetCreatedCommand(nanShipID, OT_ship, c_cidAttack);
			pGoal->AddStartAction(pCreateDroneAction);

			pGoal->AddStartAction(new SetCommandAction(nanShipID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidGoto));

  		    pGoalList->AddGoal(pGoal);
        }

        {
			Goal* pGoal = new Goal (new ElapsedTimeCondition(2.0f));
            Vector pos3(0.0f, 0.0f, -400.0f);
			
            CreateDroneAction* pCreateDroneAction = new CreateDroneAction("dontmess", defID3, 314, 1, c_ptWingman);
			pCreateDroneAction->SetCreatedLocation(1052, pos3);
			pGoal->AddStartAction(pCreateDroneAction);

			pGoal->AddStartAction(new SetCommandAction(defID3, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack));
			pGoal->AddStartAction(new SetCommandAction(defID3, c_cmdCurrent, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack));		

			pGoalList->AddGoal(pGoal);
        }
			
		// "Here come a couple defenders. Keep me alive long enough to blow the base."
		pGoalList->AddGoal (CreatePlaySoundGoal (tm_8_21Sound));
		pGoalList->AddGoal (new Goal(new ElapsedTimeCondition(4.0f)));

        {
            // defID scout  Tkela       -> nan (bahoday) or bomber, if nan is destroyed
            // defID2 fig   ribski      -> nan (bahoday) or bomber, if nan is destroyed
            // defID3 fig   dontmess    -> bomber (unchanged)

            Goal* pGoal = new Goal(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(bomberShipID))); //until bomber hull is damaged

            pGoal->AddConstraintCondition(new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(nanShipID), 0.0f)), new SetCommandAction(defID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack), false, true));
            pGoal->AddConstraintCondition(new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(nanShipID), 0.0f)), new SetCommandAction(defID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack), false, true));

            pGoal->AddStartAction(new MessageAction("Defend bombers.", voDefendBomberSound));
            pGoalList->AddGoal(pGoal);
        }

        {
            // defID scout  -> player or bomber if player hull touched (player killed seems to mess conditions up)
            // defID2 fig   -> nan (bahoday) or bomber if nan is destroyed
            // defID3 fig   -> bomber (unchanged)

            Goal* pGoal = new Goal(new OrCondition(new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)),
                new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(bomberShipID), 0.0f)))); //until bomber or station is destroyed

            pGoal->AddStartAction(new SetCommandAction(defID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(trekClient.GetShip()->GetObjectID()), c_cidAttack));
            pGoal->AddStartAction(new SetCommandAction(defID, c_cmdCurrent, OT_ship, static_cast<ObjectID>(trekClient.GetShip()->GetObjectID()), c_cidAttack));

            pGoal->AddStartAction(new SetCommandAction(nanShipID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidRepair));
            pGoal->AddStartAction(new SetCommandAction(nanShipID, c_cmdCurrent, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidRepair));

            pGoal->AddConstraintCondition(new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(nanShipID), 0.0f)), new SetCommandAction(defID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack), false, true));
            pGoal->AddConstraintCondition(new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(nanShipID), 0.0f)), new SetCommandAction(defID2, c_cmdCurrent, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack), false, true));

            pGoal->AddConstraintCondition(new ConditionalAction(new GetShipIsDamagedCondition(OT_ship, pShip->GetObjectID()), new SetCommandAction(defID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack), false, true));
            pGoal->AddConstraintCondition(new ConditionalAction(new GetShipIsDamagedCondition(OT_ship, pShip->GetObjectID()), new SetCommandAction(defID, c_cmdCurrent, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidAttack), false, true));

            pGoal->AddStartAction(new  MessageAction("Stay on target.", voStayOnTargetSound));
            pGoalList->AddGoal(pGoal);
        }
		
        {
            Goal* pGoal = new Goal(new ElapsedTimeCondition(1.0f));
            //actually blow up the station if stuck on 0hp
            //pGoal->AddConstraintCondition(new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)), new SetStationDestroyedAction(pEnemyStation), false, true));
            pGoalList->AddGoal(pGoal);
        }

        {
            Goal* pGoal = new Goal(new ElapsedTimeCondition(1.4f));
            //station is destroyed conditionnal actions
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)), new SetCommandAction(defID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidDoNothing), true, true ));
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)), new SetCommandAction(defID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidDoNothing), true, true ));
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)), new SetCommandAction(defID3, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidDoNothing), true, true ));
            //end message
            pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)), new MessageAction("Enemy outpost destroyed.", 901), false, true)); //"Well done, we destroyed the enemy station!", voYeeHaSound

            //bomber is destroyed conditional actions
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(bomberShipID), 0.0f)), new SetCommandAction(defID, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidDoNothing), true, true ));
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(bomberShipID), 0.0f)), new SetCommandAction(defID2, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidDoNothing), true, true ));
			pGoal->AddConstraintCondition( new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(bomberShipID), 0.0f)), new SetCommandAction(defID3, c_cmdAccepted, OT_ship, static_cast<ObjectID>(bomberShipID), c_cidDoNothing), true, true ));
            			
            //bomber dead and station alive
            pGoal->AddConstraintCondition(new ConditionalAction(new AndCondition(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(bomberShipID), 0.0f)), new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)), new MessageAction("The bomber got destroyed, better luck next time.", voArghSound), false, true));
			pGoalList->AddGoal (pGoal);
        }

        {
            Goal* pGoal = new Goal(new ElapsedTimeCondition(5.0f));
            //station is destroyed
            pGoal->AddConstraintCondition(new ConditionalAction(new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f)), new MessageAction("Yee-haaaaaw!", voYeeHaSound), false, true));
            //bomber dead and station dead
            pGoal->AddConstraintCondition(new ConditionalAction(new AndCondition(new NotCondition(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(bomberShipID), 0.0f)), new NotCondition(new GetShipIsDamagedCondition(OT_station, 1051, 0.0f))), new MessageAction("Our bomber got destroyed too, but that is an acceptable price to pay."), false, true));
            pGoalList->AddGoal(pGoal);
        }

        return new Goal(pGoalList);
    }
}