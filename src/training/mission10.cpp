/*
**  File:	Mission10.cpp
**
**  Description:
**      Implementation of the tenth pseudo training mission "Free Flight", intended for dev use.
**      Cloned mission 6 without intro and with less constraints.
*/
#include    "pch.h"
#include    "Mission10.h"

#include    "Training.h"
#include    "GoalList.h"
#include    "ConditionalAction.h"
#include    "GetShipIsDestroyedCondition.h"
#include    "FalseCondition.h"
#include    "ElapsedTimeCondition.h"
#include    "SetDisplayModeAction.h"
#include    "CreateDroneAction.h"
#include    "MessageAction.h"
#include    "CreateWaypointAction.h"
#include    "SetCommandAction.h"
#include    "AddRespawnAction.h"
#include    "KillsReachedCondition.h"
#include    "ModifyCreateDroneActionAction.h"
#include    "GetSectorCondition.h"
#include    "orcondition.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    Mission10::Mission10(void) :
        bUndockHintShown(false)
    {
    }

    /* void */  Mission10::~Mission10(void)
    {
    }

    //------------------------------------------------------------------------------
    int         Mission10::GetMissionID(void)
    {
        return c_TM_10_Free_Flight;
    }

    //------------------------------------------------------------------------------
    SectorID    Mission10::GetStartSectorID(void)
    {
        //return 1030; //old map
        return 2080; //new map
    }

    //------------------------------------------------------------------------------
    bool        Mission10::RestoreShip(void)
    {
        // end the mission
        Terminate();

        // and return false so the engine stays in the override camera mode
        return false;
    }
    
    //------------------------------------------------------------------------------
    bool        Mission10::ShipLanded(void)
    {
        if (!bUndockHintShown) {
            trekClient.GetCore()->GetIgcSite()->SendChatf(trekClient.GetCore()->GetShip(m_commanderID), CHAT_INDIVIDUAL, trekClient.GetShipID(),
                NA,
                "Hint: Click the hangar door to launch your ship back into space.");
            bUndockHintShown = true;
        }
        return true;
    }

    //------------------------------------------------------------------------------
    bool       Mission10::HandlePickDefaultOrder(IshipIGC* pShip)
    {
        debugf("Mission10::HandlePickDefaultOrder for %s\n", pShip->GetName());
        ZAssert(pShip->GetCluster());
        bool orderSet = false;

        ImissionIGC*    pMission = trekClient.GetCore();

        IshipIGC* pSameClusterTarget = NULL;
        IshipIGC* pSupportShip = NULL;
        IshipIGC* pShipToSupport = NULL;
        float minSupportShipDistance = 1000.0f;
        float minShipToSupportDistance = 1000.0f;
        float minClusterTargetDRel = 15000.0f;
        for (ShipLinkIGC* l = pShip->GetCluster()->GetShips()->first(); l != NULL; l = l->next()) {
            IshipIGC* s = l->data();
            if (s != pShip && pShip->CanSee(s)) {
                float dist = (s->GetPosition() - pShip->GetPosition()).Length();
                if (s->GetSide() == pShip->GetSide()) {
                    if (dist < minSupportShipDistance && s->GetPilotType() == c_ptWingman) {
                        IpartIGC* p = s->GetMountedPart(ET_Weapon, 0);
                        ZAssert(p);
                        if (p && ((IweaponIGC*)p)->GetProjectileType()->GetPower() > 0.0f) {
                            pSupportShip = s;
                            minSupportShipDistance = dist;
                        }
                    }
                    if (dist < minShipToSupportDistance) {
                        // see if the s has a valid command for pShip
                        ImodelIGC* psupTarget = s->GetCommandTarget(c_cmdPlan);
                        if (psupTarget) {
                            bool valid = false;
                            if (s->GetCommandID(c_cmdPlan) == c_cidAttack &&
                                psupTarget->GetObjectType() == OT_ship &&
                                ((IshipIGC*)psupTarget)->GetFraction() > 0.0f)
                                valid = true;
                            else if (s->GetCommandID(c_cmdPlan) == c_cidGoto) {
                                ObjectType t = psupTarget->GetObjectType();
                                if (t != OT_station && t != OT_ship)
                                    if ((psupTarget->GetPosition() - pShip->GetPosition()).Length() > 700.0f)
                                        valid = true;
                            }
                            if (valid) {
                                pShipToSupport = s;
                                minShipToSupportDistance = dist;
                            }
                        }
                    }
                }
                else if (dist < minClusterTargetDRel && s->GetFraction() > 0.0f && 
                    (GetKillCount() > 2 || s->GetPilotType() < c_ptPlayer)) 
                {
                    pSameClusterTarget = s;
                    minClusterTargetDRel = (s->GetPilotType() == c_ptMiner ? dist : dist * 1.5f); //prefer targeting miners
                }
            }
        }


        if (pSameClusterTarget)
            debugf(" pSameClusterTarget: %s\n", pSameClusterTarget->GetName());
        if (pSupportShip)
            debugf(" psupportShip: %s\n", pSupportShip->GetName());
        if (pShipToSupport)
            debugf(" pShipToSupport: %s\n", pShipToSupport->GetName());

        //check for too close enemies and assisting
        if (minClusterTargetDRel < 1000.0f) { // miner within 1000 / ship within 667
            trekClient.GetCore()->GetIgcSite()->SendChatf(pShip, CHAT_TEAM, pShip->GetSide()->GetObjectID(),
                droneInTransitSound,
                "Attacking %s", GetModelName(pSameClusterTarget));
            pShip->SetCommand(c_cmdAccepted, pSameClusterTarget, c_cidAttack);
            orderSet = true;
            if (pSupportShip && pSupportShip->GetCommandTarget(c_cmdPlan) != pSameClusterTarget) {
                trekClient.GetCore()->GetIgcSite()->SendChatf(pSupportShip, CHAT_TEAM, pShip->GetSide()->GetObjectID(),
                    droneInTransitSound,
                    "Attacking %s*", GetModelName(pSameClusterTarget));
                pSupportShip->SetCommand(c_cmdAccepted, pSameClusterTarget, c_cidAttack);
            }
        }
        else if (pShipToSupport) {
            ImodelIGC* psupTarget = pShipToSupport->GetCommandTarget(c_cmdPlan);
            if (pShipToSupport->GetCommandID(c_cmdPlan) == c_cidAttack) {
                trekClient.GetCore()->GetIgcSite()->SendChatf(pShip, CHAT_TEAM, pShip->GetSide()->GetObjectID(),
                    droneInTransitSound,
                    "Attacking %s`", GetModelName(psupTarget));
                pShip->SetCommand(c_cmdAccepted, psupTarget, c_cidAttack);
                orderSet = true;
            }
            else if (pShipToSupport->GetCommandID(c_cmdPlan) == c_cidGoto) {
                pShip->SetCommand(c_cmdAccepted, psupTarget, c_cidGoto);
                orderSet = true;
                if (pShipToSupport->GetWingmanBehaviour() & c_wbbmTempSectorAggressive)
                    pShip->SetWingmanBehaviour(pShip->GetWingmanBehaviour() | c_wbbmTempSectorAggressive);
            }
        }

        if (!orderSet && pSameClusterTarget) {
            //attack targets in appropriate sectors
            if (pShip->GetCluster()->GetObjectID() == GetStartSectorID()) {
                trekClient.GetCore()->GetIgcSite()->SendChatf(pShip, CHAT_TEAM, pShip->GetSide()->GetObjectID(),
                    droneInTransitSound,
                    "Attacking %s", GetModelName(pSameClusterTarget));
                pShip->SetCommand(c_cmdAccepted, pSameClusterTarget, c_cidAttack);
                orderSet = true;
            }
            else if (pShip->GetCluster()->GetObjectID() == 2082) { // 2082 == Enemy
                // attack any trespassers
                trekClient.GetCore()->GetIgcSite()->SendChatf(pShip, CHAT_TEAM, pShip->GetSide()->GetObjectID(),
                    droneInTransitSound,
                    "Attacking %s", GetModelName(pSameClusterTarget));
                pShip->SetCommand(c_cmdAccepted, pSameClusterTarget, c_cidAttack);
                orderSet = true;
            }
            else if (trekClient.GetCore()->GetCluster(GetStartSectorID())->GetShips()->n() == 0) {
                // attack Mars targets, if none to be found in player home
                trekClient.GetCore()->GetIgcSite()->SendChatf(pShip, CHAT_TEAM, pShip->GetSide()->GetObjectID(),
                    droneInTransitSound,
                    "Attacking %s", GetModelName(pSameClusterTarget));
                pShip->SetCommand(c_cmdAccepted, pSameClusterTarget, c_cidAttack);
                orderSet = true;
            }
            
        }

        if (!orderSet) {
            //move to appropriate sectors
            DataBuoyIGC buoyData;

            if (trekClient.GetCore()->GetCluster(GetStartSectorID())->GetShips()->n() > 0) {
                // go into start sector
                buoyData.clusterID = GetStartSectorID();
            }
            else {
                // go to player / current sector
                IclusterIGC* playerSector = trekClient.GetShip()->GetCluster();
                buoyData.clusterID = playerSector ? playerSector->GetObjectID() : pShip->GetCluster()->GetObjectID();
            }

            if (pShip->GetCluster()->GetObjectID() != buoyData.clusterID) {
                buoyData.type = c_buoyCluster;
                buoyData.position = Vector(0.0f, 0.0f, 0.0f);

                IclusterIGC* pcSent = trekClient.GetCore()->GetCluster(buoyData.clusterID);
                //trekClient.PostText(true, "Pick sent %s towards cluster %s", pShip->GetName(), pcSent->GetName());
            }
            else {
                //find points to go to
                int stations = 0;
                int asteroids = 0;
                std::vector<Vector> pointsOfInterest;
                for (StationLinkIGC* l = pShip->GetCluster()->GetStations()->first(); l != NULL; l = l->next()) {
                    IstationIGC* s = l->data();
                    if (s->GetSide() != pShip->GetSide() && pShip->CanSee((ImodelIGC*)s)) {
                        pointsOfInterest.push_back(s->GetPosition() + Vector(300.0f, 300.0f, 250.0f));
                        stations++;
                    }
                }
                for (AsteroidLinkIGC* l = pShip->GetCluster()->GetAsteroids()->first(); l != NULL; l = l->next()) {
                    IasteroidIGC* a = l->data();
                    
                    if (a->HasCapability(c_aabmMineHe3)) {
                        pointsOfInterest.push_back(a->GetPosition() + Vector(0.0f, 0.0f, 350.0f));
                        asteroids++;
                    }
                }
                for (char i = 0; i < 2; i++) {
                    Vector wp = Vector(random(-3000.0f, 3000.0f), random(-3000.0f, 3000.0f), random(-1000.0f, 1000.0f));
                    for (ModelLinkIGC* l = pShip->GetCluster()->GetPickableModels()->first(); l != NULL; l = l->next()) {
                        ImodelIGC*  pmodel = l->data();
                        if ((pmodel != pShip) && (pmodel->GetHitTest()->GetNoHit() != pShip->GetHitTest())) {
                            if (pmodel->GetObjectType() != OT_mine && pmodel->GetObjectType() != OT_ship) {
                                float   minDistance = pmodel->GetRadius() + pShip->GetRadius() + 10.0f;
                                Vector modelToWP = pShip->GetPosition() - pmodel->GetPosition();
                                if (modelToWP.Length() < minDistance) {
                                    wp = pmodel->GetPosition() + modelToWP.Normalize()*minDistance;
                                }
                            }
                        }
                    }
                    pointsOfInterest.push_back(wp);
                }

                //choose one
                int i = randomInt(0, pointsOfInterest.size() - 1);
                /*const char* type;
                if (i < stations)
                    type = "station";
                else if (i < stations+asteroids)
                    type = "asteroid";
                else
                    type = "random";
                trekClient.PostText(true, "pick sends %s towards %d %s %f, %f, %f", pShip->GetName(), i, type, buoyData.position.X(), buoyData.position.Y(), buoyData.position.Z());*/
                buoyData.position = pointsOfInterest.at(i);
                buoyData.type = c_buoyWaypoint;
                pShip->SetWingmanBehaviour(pShip->GetWingmanBehaviour() | c_wbbmTempSectorAggressive);
            }
            //buoyData.buoyID = pMission->GenerateNewBuoyID(); // or just NA?
            buoyData.visible = false;

            ImodelIGC* buoy = (ImodelIGC*)pMission->CreateObject(pMission->GetLastUpdate(), OT_buoy, &buoyData, sizeof(buoyData));

            pShip->SetCommand(c_cmdAccepted, buoy, c_cidGoto);
            orderSet = true;
        }

        return orderSet; //always true
    }

    //------------------------------------------------------------------------------
    void        Mission10::CreateUniverse(void)
    {
        LoadUniverse("training_6", 488, 1030);    // 488 = Adv. IC fighter
        
        ImissionIGC*    pCore = trekClient.GetCore();

        //output map info
        const ClusterListIGC *clusters = pCore->GetClusters();
        for (ClusterLinkIGC* cLink = clusters->first(); cLink != NULL; cLink = cLink->next())
        {
            IclusterIGC* pCluster = cLink->data();
            debugf("Cluster %s - ObjectID: %d\n", pCluster->GetName(), pCluster->GetObjectID());
        }
        const StationListIGC *stations = pCore->GetStations();
        for (StationLinkIGC* cLink = stations->first(); cLink != NULL; cLink = cLink->next())
        {
            IstationIGC* pStation = cLink->data();
            debugf("Station %s - ObjectID: %d\n", pStation->GetName(), pStation->GetObjectID());
        }

        trekClient.fGroupFire = true;           // activate all the starting weapons

        //Add tech, so equipment doesn't show as new treassure and can be got at the station
        TechTreeBitMask ttbm = trekClient.GetSide()->GetDevelopmentTechs();
        debugf("Pre DevelopmentTechs: %s\n", (LPCSTR)trekClient.GetCore()->BitsToTechsList(ttbm));
        ttbm |= trekClient.GetCore()->GetPartType(154)->GetEffectTechs();   //Seeker 2 tech
        ttbm |= trekClient.GetCore()->GetPartType(104)->GetEffectTechs();   //Quickfire 2 tech
        ttbm |= trekClient.GetCore()->GetPartType(104)->GetRequiredTechs();
        ttbm |= trekClient.GetCore()->GetPartType(151)->GetEffectTechs();   //Dumbfire 2 tech
        ttbm |= trekClient.GetCore()->GetPartType(14)->GetEffectTechs();    //Mine Pack 2 tech
        ttbm |= trekClient.GetCore()->GetPartType(14)->GetRequiredTechs();
        ttbm |= trekClient.GetCore()->GetPartType(58)->GetEffectTechs();    //Prox 2
        ttbm |= trekClient.GetCore()->GetPartType(58)->GetRequiredTechs();
        ttbm |= trekClient.GetCore()->GetPartType(232)->GetEffectTechs();   //Pulse Probe 1
        ttbm |= trekClient.GetCore()->GetPartType(232)->GetRequiredTechs();
        ttbm |= trekClient.GetCore()->GetPartType(174)->GetEffectTechs();   //Hvy Booster
        ttbm |= trekClient.GetCore()->GetPartType(174)->GetRequiredTechs(); //also adds Afterburner 3
        ttbm |= trekClient.GetCore()->GetPartType(39)->GetEffectTechs();    //Sig Cloak 2
        ttbm |= trekClient.GetCore()->GetPartType(39)->GetRequiredTechs();

        ttbm |= trekClient.GetShip()->GetHullType()->GetEffectTechs();      //IC Adv Fighter
        ttbm |= trekClient.GetShip()->GetHullType()->GetRequiredTechs();
        ttbm |= trekClient.GetCore()->GetHullType(481)->GetEffectTechs();   //IC Hvy Interceptor (Interceptor 412)
        ttbm |= trekClient.GetCore()->GetHullType(481)->GetRequiredTechs();
        ttbm |= trekClient.GetCore()->GetHullType(486)->GetEffectTechs();   //IC Adv Stealth Fighter (Stealth Fighter 411)
        ttbm |= trekClient.GetCore()->GetHullType(486)->GetRequiredTechs();
        ttbm |= trekClient.GetCore()->GetHullType(87)->GetEffectTechs();    //IC Hvy Scout (Adv Scout 485)
        ttbm |= trekClient.GetCore()->GetHullType(87)->GetRequiredTechs(); 
        trekClient.GetSide()->SetDevelopmentTechs(ttbm);
        debugf("Post DevelopmentTechs: %s\n", (LPCSTR)trekClient.GetCore()->BitsToTechsList(ttbm));
        //add GAs?

        //Player loadout with that tech
        IshipIGC*       pShipU = trekClient.GetShip();
        // first delete all possibly outdated parts
        const PartListIGC*  pparts = pShipU->GetParts();
        PartLinkIGC*    ppl;
        while (ppl = pparts->first())
            ppl->data()->Terminate();
        // get default
        DefaultLoadout(pShipU);
        // Now switch missiles to Seeker 2
        // destroy mounted dumbfires
        IpartIGC*       pPart = pShipU->GetMountedPart(ET_Magazine, 0);
        if (pPart)
            pPart->Terminate();;
        // Mount Seeker 2s
        AddPartToShip(154, 0, 5);
        // Change missiles in cargo for Seeker 2s
        for (int i = -1; i >= -c_maxCargo; i--) {
            pPart = pShipU->GetMountedPart(NA, i);
            if (pPart->GetEquipmentType() == ET_Magazine)
            {
                pPart->Terminate();
                AddPartToShip(154, i, 5);
            }
        }
        trekClient.SaveLoadout(pShipU);
    }

    //------------------------------------------------------------------------------
    Condition*  Mission10::CreateMission(void)
    {
        debugf("Creating mission 10.\n");
        m_commandViewEnabled = true;

        GoalList*   pGoalList = new GoalList;

        ImissionIGC*        pMission = trekClient.GetCore();
        IstationIGC*        pHomeStation = (IstationIGC*)pMission->GetModel(OT_station, 2080);

        // put the commander ship in the station
        {
            IshipIGC*       pCommander = trekClient.GetCore()->GetShip(m_commanderID);
            pCommander->SetStation(pHomeStation);
            pCommander->SetCommand(c_cmdAccepted, NULL, c_cidDoNothing);
            pCommander->SetCommand(c_cmdCurrent, NULL, c_cidDoNothing);
            pCommander->SetAutopilot(false);
        }

        // show warps to the enemy
        for (WarpLinkIGC* l = pMission->GetWarps()->first(); l != NULL; l = l->next()) {
            l->data()->SetSideVisibility(pMission->GetSide(1), true);
        }

        // create friendly miner
        ShipID          miner01ID = pMission->GenerateNewShipID();
        CreateDroneAction*  pCreateMiner01Action = new CreateDroneAction("Miner 01", miner01ID, 438, 0, c_ptMiner);
        pCreateMiner01Action->SetCreatedLocation(GetStartSectorID(), Vector(0.0f, 2500.0f, 100.0f)); //create forward to draw aggro
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(0.1f));

            pGoal->AddStartAction(new AddRespawnAction(pCreateMiner01Action, 300.0f));
            pGoal->AddStartAction(new MessageAction("Hint: Press F1 to see an overview of your ship's controls."));

            //respawn at base instead of forward
            ModifyCreateDroneActionAction* modMiner = new ModifyCreateDroneActionAction(pCreateMiner01Action);
            Vector          pos = pHomeStation->GetPosition();
            pos.x += 250.0f; //simulate undock
            pos.y += 200.0f;
            modMiner->ModifyCreatedLocation(GetStartSectorID(), pos);
            pGoal->AddStartAction(modMiner);

            pGoalList->AddGoal(pGoal);
        }
        // create friendly carrier
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(0.1f));

            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction("Carrier", pMission->GenerateNewShipID(), 425, 0, c_ptCarrier);
            pCreateDroneAction->SetCreatedLocation(GetStartSectorID(), Vector(-3000.0f, 1500.0f, 250.0f)); 
            pCreateDroneAction->SetCreatedOrientation(Vector(0.3f, 1.0f, 0.0f), Vector(0.0f, 0.0f, 1.0f));
            pGoal->AddStartAction(pCreateDroneAction);

            pGoalList->AddGoal(pGoal);
        }

        // play the introductory audio and change camera
        {
            Goal*   pGoal = CreatePlaySoundGoal(salCommenceScanSound);
            pGoal->AddStartAction(new SetDisplayModeAction(TrekWindow::cmCockpit)); //this needs some delay prior, so stuff is initialized
            pGoalList->AddGoal(pGoal);
        }


        // create friendly miner 02
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(0.2f));

            ShipID          minerID = pMission->GenerateNewShipID();
            Vector          pos = pHomeStation->GetPosition();
            pos.x += 250.0f; //simulate undock
            pos.y += 200.0f;

            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction("Miner 02", minerID, 438, 0, c_ptMiner);
            pCreateDroneAction->SetCreatedLocation(GetStartSectorID(), pos);
            pGoal->AddStartAction(pCreateDroneAction);

            pGoalList->AddGoal(pGoal);
        }

        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(1.0f)));

        // tm_6_02
        // Iron Coalition Intelligence reports inbound enemy craft. 
        // They're using an old code for their comms, so we can hear 
        // them. Watch your message stream to see what they're up to.
        {
            Goal*   pGoal = CreatePlaySoundGoal(tm_6_02Sound);
            pGoal->AddStartAction(new MessageAction("Watch the chat to see what the enemy craft are after."));
            pGoalList->AddGoal(pGoal);
        }

        // create friendly miner 03
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(0.2f));

            ShipID          minerID = pMission->GenerateNewShipID();
            Vector          pos = pHomeStation->GetPosition();
            pos.x += -250.0f; //simulate undock
            pos.y += -200.0f;

            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction("Miner 03", minerID, 438, 0, c_ptMiner);
            pCreateDroneAction->SetCreatedLocation(GetStartSectorID(), pos);
            pGoal->AddStartAction(pCreateDroneAction);

            pGoalList->AddGoal(pGoal);
        }

        // create enemy carrier
        ShipID          nmyCarrierID = pMission->GenerateNewShipID();
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(1.0f));

            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction("Enemy Carrier", nmyCarrierID, 325, 1, c_ptCarrier);
            pCreateDroneAction->SetCreatedLocation(2081, Vector(3560.0f, 4880.0f, 850.0f)); //Mars sector 2081 //Near Neptune aleph
            pGoal->AddStartAction(pCreateDroneAction);

            pGoalList->AddGoal(pGoal);
        }

        //create enemy spawn actions
        BuoyID		        homeSectorBuoyID = pMission->GenerateNewBuoyID();
        BuoyID		        nearNeptuneAlephBuoyID = pMission->GenerateNewBuoyID();
        ShipID              enemy1ID = pMission->GenerateNewShipID();
        ShipID              enemy2ID = pMission->GenerateNewShipID();

        CreateDroneAction*  pCreateEnemy1Action = new CreateDroneAction("Enemy Scout", enemy1ID, 310, 1, c_ptWingman); //310 == Bios Scout
        pCreateEnemy1Action->SetCreatedLocation(2082, Vector(-140.0f, 280.0f, -20.0f));
        pCreateEnemy1Action->SetCreatedLocationAtShipID(nmyCarrierID);
        pCreateEnemy1Action->SetCreatedBehaviour(0);

        CreateDroneAction* pCreateEnemy2Action = new CreateDroneAction("Enemy Fighter", enemy2ID, 315, 1, c_ptWingman); //315 == Bios Fighter
        pCreateEnemy2Action->SetCreatedLocation(2082, Vector(-170.0f, 280.0f, -30.0f));
        pCreateEnemy2Action->SetCreatedLocationAtShipID(nmyCarrierID, Vector(0.0f, -70.0f, 30.0f));
        pCreateEnemy2Action->SetCreatedBehaviour(0);

        //start spawning enemies
        {
            Goal*           pGoal = new Goal(new ElapsedTimeCondition(0.5f));

            CreateDroneAction*  pCreateEnemyCarrierAction = new CreateDroneAction("Enemy Carrier", nmyCarrierID, 325, 1, c_ptCarrier); // Bios Carrier
            pCreateEnemyCarrierAction->SetCreatedLocation(2082, Vector(-1000.0f, 280.0f, -50.0f));
            pCreateEnemyCarrierAction->SetCreatedCommand(nearNeptuneAlephBuoyID, OT_buoy, c_cidGoto);

            pGoal->AddStartAction(new CreateWaypointAction(homeSectorBuoyID, Vector(0.0f, 0.0f, 0.0f), GetStartSectorID(), c_buoyCluster, false));
            pGoal->AddStartAction(new AddRespawnAction(pCreateEnemy1Action, 10.0f, 1));
            pGoal->AddStartAction(new AddRespawnAction(pCreateEnemy2Action, 10.0f, 1));
            pGoal->AddStartAction(new CreateWaypointAction(nearNeptuneAlephBuoyID, Vector(3560.0f, 4880.0f, 850.0f), 2081, c_buoyWaypoint, false));
            pGoal->AddStartAction(new AddRespawnAction(pCreateEnemyCarrierAction, 60.0f, NA, false));

            pGoalList->AddGoal(pGoal);
        }

        // tm_6_03
        // DEFEND THE MINERS! This simulation is over when you die.
        pGoalList->AddGoal(CreatePlaySoundGoal(tm_6_03Sound));

        // wait
        pGoalList->AddGoal(new Goal(new ElapsedTimeCondition(8.0f)));

        // create enemy miners
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(4.0f));

            ShipID          minerID = pMission->GenerateNewShipID();
            Vector          pos = Vector(3080.0f, -440.0f, -700.0f); //near mid he-3

            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction("Enemy Miner 01", minerID, 338, 1, c_ptMiner);
            pCreateDroneAction->SetCreatedLocation(2082, pos); //sector "Enemy"
            pGoal->AddStartAction(new AddRespawnAction(pCreateDroneAction, 300.0f));

            minerID = pMission->GenerateNewShipID();
            pos = Vector(-1760.0f, -3240.0f, -700.0f); //near lower left he-3
            pCreateDroneAction = new CreateDroneAction("Enemy Miner 02", minerID, 338, 1, c_ptMiner);
            pCreateDroneAction->SetCreatedLocation(2082, pos); //sector "Enemy"
            pGoal->AddStartAction(new AddRespawnAction(pCreateDroneAction, 300.0f));

            pGoalList->AddGoal(pGoal);
        }
        // create friendly miner 04
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(1.0f));

            ShipID          minerID = pMission->GenerateNewShipID();
            Vector          pos = pHomeStation->GetPosition();
            pos.x += 250.0f; //simulate undock
            pos.y += 200.0f;

            CreateDroneAction*  pCreateDroneAction = new CreateDroneAction("Miner 04", minerID, 438, 0, c_ptMiner);
            pCreateDroneAction->SetCreatedLocation(GetStartSectorID(), pos);
            pGoal->AddStartAction(pCreateDroneAction);

            pGoalList->AddGoal(pGoal);
        }

        //Wave 1: wait for already spawned ships to die
        pGoalList->AddGoal(new Goal(new KillsReachedCondition(2)));

        // 2: escalate behaviour -- changes apply before spawn
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(4));

            ModifyCreateDroneActionAction* pModScout = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pModFighter = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pModScout->ModifyCreatedBehaviour(c_wbbmUseMissiles);
            pModFighter->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmRunAt30Hull);
            pGoal->AddStartAction(pModScout);
            pGoal->AddStartAction(pModFighter);

            pGoalList->AddGoal(pGoal);
        }

        // 3: escalate behaviour
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(6));

            ModifyCreateDroneActionAction* pModScout = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pModFighter = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pModScout->ModifyCreatedTechLevel(2);
            pModScout->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmRunAt30Hull);
            pModFighter->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmRunAt60Hull);
            pGoal->AddStartAction(pModScout);
            pGoal->AddStartAction(pModFighter);

            pGoalList->AddGoal(pGoal);
        }

        //4: change fig to an int
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(8));

            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pMod2->ModifyCreatedHullType(324); //324 == Bios Lt Interceptor
            pMod2->ModifyCreatedName("Enemy Lt Interceptor");
            pMod2->ModifyCreatedBehaviour(c_wbbmInRangeAggressive);
            pGoal->AddStartAction(pMod2);

            pGoalList->AddGoal(pGoal);
        }

        // 5: strafe behaviour
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(10));

            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pMod1->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt60Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmRunAt30Hull);
            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);

            pGoalList->AddGoal(pGoal);
        }


        // 6: upgrade int tech
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(12));

            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pMod2->ModifyCreatedTechLevel(2);
            pMod1->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt60Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt30Hull);
            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);

            pGoalList->AddGoal(pGoal);
        }

        //7: 2 figs
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(14));

            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pMod1->ModifyCreatedHullType(315); //315 == Bios Fighter
            pMod1->ModifyCreatedName("Enemy Fighter");
            pMod2->ModifyCreatedHullType(315); //314 == Bios Enh Fighter
            pMod2->ModifyCreatedName("Enemy Fighter 2");
            pMod1->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt60Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmRunAt30Hull);

            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoalList->AddGoal(pGoal);
        }

        //8: fig tech 2
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(16));

            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pMod2->ModifyCreatedTechLevel(2);
            pMod2->ModifyCreatedHullType(314); //314 == Bios Enh Fighter
            pMod2->ModifyCreatedName("Enemy Enh Fighter");
            pMod1->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt60Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmRunAt30Hull);
            
            pGoal->AddStartAction(pMod2);
            pGoalList->AddGoal(pGoal);
        }

        // 9: fig hull 2
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(18));
            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);

            pMod1->ModifyCreatedTechLevel(2);
            pMod1->ModifyCreatedHullType(314); //314 == Bios Enh Fighter
            pMod1->ModifyCreatedName("Enemy Enh Fighter 2");
            pMod1->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe);

            pGoal->AddStartAction(pMod1);
            pGoalList->AddGoal(pGoal);
        }


        // 10: Bomb run
        //  3rd ship
        ShipID              enemy3ID = pMission->GenerateNewShipID();
        CreateDroneAction*  pCreateEnemy3Action = new CreateDroneAction("Enemy Bomber", enemy3ID, 304, 1, c_ptWingman); //304 == Bios Bomber
        pCreateEnemy3Action->SetCreatedLocation(2082, Vector(-170.0f, 280.0f, 0.0f));
        pCreateEnemy3Action->SetCreatedLocationAtShipID(nmyCarrierID, Vector(70.0f, 30.0f, 10.0f));
        pCreateEnemy3Action->SetCreatedCommand(2080, OT_station, c_cidAttack);
        pCreateEnemy3Action->SetCreatedBehaviour(c_wbbmUseMissiles);
        {
            //wait for spawn
            Goal*               pGoal = new Goal(new ElapsedTimeCondition(15.0f));
            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);

            pMod1->ModifyCreatedHullType(385); //385 == Bios Adv Scout
            pMod1->ModifyCreatedName("Enemy Adv Scout");
            pMod2->ModifyCreatedHullType(385); //385 == Bios Adv Scout
            pMod2->ModifyCreatedName("Enemy Adv Scout 2");
            pMod1->ModifyCreatedCommand(enemy3ID, OT_ship, c_cidRepair); //bomber has to spawn first for this to work! Last added spawns first.
            pMod2->ModifyCreatedCommand(enemy3ID, OT_ship, c_cidRepair);
            pMod1->ModifyCreatedTechLevel(-1);
            pMod2->ModifyCreatedTechLevel(-2);
            pMod1->ModifyCreatedBehaviour(0);
            pMod2->ModifyCreatedBehaviour(0);

            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoal->AddStartAction(new AddRespawnAction(pCreateEnemy3Action, 10.0f, 1, false));
            pGoalList->AddGoal(pGoal);
        }

        //Goal( bomber in home or dead )
        pGoalList->AddGoal(new Goal(new OrCondition(new GetSectorCondition(OT_ship, enemy3ID, GetStartSectorID()), new NotCondition(new GetShipIsDamagedCondition(OT_ship, enemy3ID, 0.0f)))));

        {
            //Goal( scout in home or dead )
            Goal*               pGoal = new Goal(new OrCondition(new GetSectorCondition(OT_ship, enemy2ID, GetStartSectorID()), new NotCondition(new GetShipIsDamagedCondition(OT_ship, enemy2ID, 0.0f))));
            pGoal->AddConstraintCondition(new ConditionalAction(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(enemy3ID), 0.0f), new MessageAction("They are trying to destroy our base! Stop them!"), false, true));
            pGoalList->AddGoal(pGoal);
        }
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(21));
            pGoal->AddConstraintCondition(new ConditionalAction(new GetShipIsDamagedCondition(OT_ship, static_cast<ObjectID>(enemy2ID), 0.0f), new MessageAction("The scouts can repair the bomber. Shoot them first."), false, true));
            pGoalList->AddGoal(pGoal);
        }

        // 11: Bomb run, smarter nans
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(24));
            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);

            pMod1->ModifyCreatedTechLevel(-2);
            pMod1->ModifyCreatedBehaviour(c_wbbmUseMines);
            pMod2->ModifyCreatedBehaviour(c_wbbmUseMines | c_wbbmStrafe);

            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoalList->AddGoal(pGoal);
        }

        // 12: 3 scouts
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(27)); 
            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            ModifyCreateDroneActionAction* pMod3 = new ModifyCreateDroneActionAction(pCreateEnemy3Action);

            pMod1->ModifyCreatedCommand(NA, OT_invalid, c_cidNone);
            pMod2->ModifyCreatedCommand(NA, OT_invalid, c_cidNone);
            pMod3->ModifyCreatedCommand(NA, OT_invalid, c_cidNone);
            pMod1->ModifyCreatedTechLevel(2);
            pMod2->ModifyCreatedTechLevel(2);
            pMod1->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles);
            pMod2->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe);
            pMod3->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmRunAt30Hull);
            pMod3->ModifyCreatedHullType(385);
            pMod3->ModifyCreatedName("Enemy Adv Scout 3");

            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoal->AddStartAction(pMod3);
            pGoalList->AddGoal(pGoal);
        }

        // 13: Tech 3, 1 hvy scout
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(30));

            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            pMod1->ModifyCreatedTechLevel(2);
            pMod2->ModifyCreatedTechLevel(3);
            ModifyCreateDroneActionAction* pMod3 = new ModifyCreateDroneActionAction(pCreateEnemy3Action);
            pMod3->ModifyCreatedTechLevel(3);
            pMod3->ModifyCreatedHullType(86); //86 == Bios Hvy Scout
            pMod3->ModifyCreatedName("Enemy Hvy Scout");
            pMod1->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt30Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe);
            pMod3->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles);
            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoal->AddStartAction(pMod3);

            pGoalList->AddGoal(pGoal);
        }

        // 14: 1 int, 2 scouts 
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(33));
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);

            pMod2->ModifyCreatedTechLevel(1);
            pMod2->ModifyCreatedHullType(312); //312 == Bios Interceptor
            pMod2->ModifyCreatedName("Enemy Interceptor");

            pGoal->AddStartAction(pMod2);
            pGoalList->AddGoal(pGoal);
        }

        // 15: 2 ints, 1 scout
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(36));
            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            ModifyCreateDroneActionAction* pMod3 = new ModifyCreateDroneActionAction(pCreateEnemy3Action);

            pMod1->ModifyCreatedName("Enemy Adv Scout");
            pMod1->ModifyCreatedTechLevel(2);
            pMod2->ModifyCreatedHullType(312); //312 == Bios Interceptor
            pMod2->ModifyCreatedName("Enemy Interceptor");
            pMod2->ModifyCreatedTechLevel(1);
            pMod3->ModifyCreatedHullType(312); //312 == Bios Interceptor
            pMod3->ModifyCreatedName("Enemy Interceptor 2");
            pMod3->ModifyCreatedTechLevel(2);
            pMod1->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt30Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmStrafe);
            pMod3->ModifyCreatedBehaviour(c_wbbmUseMissiles);

            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoal->AddStartAction(pMod3);
            pGoalList->AddGoal(pGoal);
        }

        // 16: 2 ints, 1 nan, move carrier to home
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(39));
            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            ModifyCreateDroneActionAction* pMod3 = new ModifyCreateDroneActionAction(pCreateEnemy3Action);

            pMod1->ModifyCreatedTechLevel(-2);
            pMod1->ModifyCreatedCommand(enemy2ID, OT_ship, c_cidRepair);
            pMod2->ModifyCreatedTechLevel(2);
            
            pMod1->ModifyCreatedBehaviour(c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt30Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles);
            pMod3->ModifyCreatedBehaviour(c_wbbmInRangeAggressive | c_wbbmUseMissiles | c_wbbmStrafe | c_wbbmRunAt30Hull);

            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoal->AddStartAction(pMod3);

            BuoyID		        somewhatInsideHomeBuoyID = pMission->GenerateNewBuoyID();
            pGoal->AddStartAction(new CreateWaypointAction(somewhatInsideHomeBuoyID, Vector(1500.0f, 2500.0f, 300.0f), GetStartSectorID(), c_buoyWaypoint, false));
            pGoal->AddStartAction(new SetCommandAction(nmyCarrierID, c_cmdAccepted, OT_buoy, somewhatInsideHomeBuoyID, c_cidGoto));

            pGoalList->AddGoal(pGoal);
        }

        // 17: int hull
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(42));

            ModifyCreateDroneActionAction* pModInt = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            ModifyCreateDroneActionAction* pModInt2 = new ModifyCreateDroneActionAction(pCreateEnemy3Action);
            pModInt->ModifyCreatedHullType(381); //381 == Bios Hvy Interceptor
            pModInt->ModifyCreatedName("Enemy Hvy Interceptor");
            pModInt2->ModifyCreatedName("Enemy Interceptor");

            pGoal->AddStartAction(pModInt);
            pGoal->AddStartAction(pModInt2);

            pGoalList->AddGoal(pGoal);
        }

        // 18: 3 adv figs
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(45));

            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            ModifyCreateDroneActionAction* pMod3 = new ModifyCreateDroneActionAction(pCreateEnemy3Action);
            pMod1->ModifyCreatedHullType(388); //388 == Bios Adv Fighter
            pMod1->ModifyCreatedName("Enemy Adv Fighter");
            pMod1->ModifyCreatedTechLevel(3);
            pMod1->ModifyCreatedCommand(NA, OT_invalid, c_cidNone);
            pMod2->ModifyCreatedHullType(388); //388 == Bios Adv Fighter
            pMod2->ModifyCreatedName("Enemy Adv Fighter 2");
            pMod2->ModifyCreatedTechLevel(3);
            pMod3->ModifyCreatedHullType(388); //388 == Bios Adv Fighter
            pMod3->ModifyCreatedName("Enemy Adv Fighter 3");
            pMod3->ModifyCreatedTechLevel(3);
            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoal->AddStartAction(pMod3);

            pGoalList->AddGoal(pGoal);
        }

        // 19: Bomb run
        //  4th ship
        ShipID              enemy4ID = pMission->GenerateNewShipID();
        CreateDroneAction*  pCreateEnemy4Action = new CreateDroneAction("Enemy Hvy Bomber", enemy4ID, 389, 1, c_ptWingman);
        pCreateEnemy4Action->SetCreatedLocation(2082, Vector(-170.0f, 280.0f, 0.0f));
        pCreateEnemy4Action->SetCreatedLocationAtShipID(nmyCarrierID, Vector(70.0f, 30.0f, 10.0f));
        pCreateEnemy4Action->SetCreatedCommand(2080, OT_station, c_cidAttack);
        pCreateEnemy4Action->SetCreatedBehaviour(c_wbbmUseMissiles | c_wbbmStrafe);
        pCreateEnemy4Action->SetCreatedTechLevel(3);
        {
            //wait for spawn
            Goal*               pGoal = new Goal(new ElapsedTimeCondition(13.0f));
            ModifyCreateDroneActionAction* pMod1 = new ModifyCreateDroneActionAction(pCreateEnemy1Action);
            ModifyCreateDroneActionAction* pMod2 = new ModifyCreateDroneActionAction(pCreateEnemy2Action);
            ModifyCreateDroneActionAction* pMod3 = new ModifyCreateDroneActionAction(pCreateEnemy3Action);

            pMod1->ModifyCreatedHullType(86); //86 == Bios Hvy Scout
            pMod1->ModifyCreatedName("Enemy Hvy Scout");
            pMod2->ModifyCreatedHullType(86); 
            pMod2->ModifyCreatedName("Enemy Hvy Scout 2");
            pMod1->ModifyCreatedCommand(enemy4ID, OT_ship, c_cidRepair); //bomber has to spawn first for this to work! Last added spawns first.
            pMod2->ModifyCreatedCommand(enemy4ID, OT_ship, c_cidRepair);
            pMod1->ModifyCreatedTechLevel(-3);
            pMod2->ModifyCreatedTechLevel(-3);
            pMod1->ModifyCreatedBehaviour(c_wbbmUseMines | c_wbbmStrafe | c_wbbmRunAt30Hull);
            pMod2->ModifyCreatedBehaviour(c_wbbmUseMines | c_wbbmStrafe);
            pMod3->ModifyCreatedHullType(381); //381 == Bios Hvy Interceptor
            pMod3->ModifyCreatedName("Enemy Hvy Interceptor");
            pMod3->ModifyCreatedBehaviour(0);
            pMod3->ModifyCreatedCommand(enemy4ID, OT_ship, c_cidDefend);
            

            pGoal->AddStartAction(pMod1);
            pGoal->AddStartAction(pMod2);
            pGoal->AddStartAction(pMod3);
            pGoal->AddStartAction(new AddRespawnAction(pCreateEnemy4Action, 10.0f, 1, false));
            pGoalList->AddGoal(pGoal);
        }
        {
            Goal*               pGoal = new Goal(new KillsReachedCondition(49));
            pGoal->AddStartAction(new MessageAction("Incoming bomber!"));

            /*BuoyID		        closerToBaseBuoyID = pMission->GenerateNewBuoyID();
            pGoal->AddConstraintCondition(new ConditionalAction(new GetSectorCondition(OT_ship, nmyCarrierID, GetStartSectorID()), new CreateWaypointAction(closerToBaseBuoyID, Vector(1500.0f, 2000.0f, 200.0f), GetStartSectorID(), c_buoyWaypoint, false), false, true));
            pGoal->AddConstraintCondition(new ConditionalAction(new NotCondition(new GetSectorCondition(OT_ship, nmyCarrierID, GetStartSectorID())), new CreateWaypointAction(closerToBaseBuoyID, Vector(3200, 3100, 600.0f), GetStartSectorID(), c_buoyWaypoint, false), false, true));
            pGoal->AddConstraintCondition(new ConditionalAction(new TrueCondition(), new SetCommandAction(enemy3ID, c_cmdAccepted, OT_ship, 0, c_cidAttack), false, true)); // Attack player -- ConditionalAction for execution order only
            pGoal->AddConstraintCondition(new ConditionalAction(new TrueCondition(), new SetCommandAction(enemy3ID, c_cmdPlan, OT_buoy, closerToBaseBuoyID, c_cidGoto), false, true));*/

            pGoalList->AddGoal(pGoal);
        }

        // Victory message
        {
            Goal*	pGoal = new Goal(new ElapsedTimeCondition(9.0f));

            pGoal->AddStartAction(new MessageAction("Congratulations, you managed to defeat all waves!!", voAwsomeSound));

            pGoalList->AddGoal(pGoal);
        }

        /*// wait forever until player died
        {
            Goal*               pGoal = new Goal(new FalseCondition); //will never evaluate as true - wait for player to be dead
            pGoalList->AddGoal(pGoal);
        }*/

        return new Goal(pGoalList);

    }

    //------------------------------------------------------------------------------
}