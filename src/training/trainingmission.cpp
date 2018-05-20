 /*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	trainingmission.cpp
**
**  Author:  
**
**  Description:
**      Implementation of the training library "trainingmission" interface.
**
**  History:
*/
#include    "pch.h"
#include    "TrainingMission.h"

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
#include    "SetControlConstraintsAction.h"
#include    "GetAutopilotCondition.h"
#include    "SetAutopilotAction.h"
#include    "GetTargetCondition.h"
#include    "SuspendedPlaySoundAction.h"
#include    "SuspendedSoundFinishedCondition.h"
#include    "CommandAcknowledgedCondition.h"
#include    "GetKeyCondition.h"
#include    "GetChatCondition.h"

namespace Training
{
    //------------------------------------------------------------------------------  
    // class methods
    //------------------------------------------------------------------------------
    /* void */  TrainingMission::TrainingMission (void) :
    m_pCondition (0),
    m_pDeadCondition (0),
    m_commanderID (NA),
    m_pChatCondition (0),
    m_bSkipPostSlideShow (false),
    m_commandViewEnabled(false),
    m_killCount(0)
    {
        // get the window pointer
        TrekWindow* pWindow = GetWindow ();

        // check to see what the HUD style is, and set it to hardware
        if ((m_fHUDStyle = pWindow->GetHUDStyle ()) != 0.0f)
            pWindow->SetHUDStyle (0.0f);

        // load the existing key map Imago 6/20/09 (TheRock@RT)
		// BT - 9/17 - Added a training input map so that the keys match up to the instructor's requests.
		//if(!GetWindow()->GetInput()->LoadMap(TRAINING_INPUTMAP_FILE))  // BT - 10/17 - Taking this back out, default input map is now back to the original values.
			if (!GetWindow()->GetInput()->LoadMap(INPUTMAP_FILE)) 
				GetWindow()->GetInput()->LoadMap(DEFAULTINPUTMAP_FILE);
        
    }

    //------------------------------------------------------------------------------
    /* void */  TrainingMission::~TrainingMission (void)
    {
        // clean up the old condition
        assert (m_pCondition == 0);
        delete m_pDeadCondition;

        // clean up the wait conditions
        std::list<Condition*>::iterator pWaitConditionIterator = m_WaitConditionList.begin ();
        std::list<Condition*>::iterator end = m_WaitConditionList.end ();
        while (pWaitConditionIterator != end)
            delete (*pWaitConditionIterator++);

        std::list<RespawnData*>::iterator pRespawnDataIterator = m_RespawnList.begin();
        std::list<RespawnData*>::iterator endR = m_RespawnList.end();
        while (pRespawnDataIterator != endR) {
            delete((*pRespawnDataIterator)->pspawnCDA);
            delete(*pRespawnDataIterator++);
        }

        // get the window pointer
        TrekWindow* pWindow = GetWindow ();

        // check to see if we have quit or ar just running through the end of the mission
        if (m_bSkipPostSlideShow)
            // go back to the training screen
            pWindow->screen (ScreenIDTrainScreen);
        else
            // go to the post training mission slideshow
            pWindow->screen (ScreenIDPostTrainSlideshow);

        // quit the mission
        PlayerInfo* pPlayerInfo = trekClient.GetPlayerInfo ();
        trekClient.RemovePlayerFromSide (pPlayerInfo, QSR_Quit);
        trekClient.RemovePlayerFromMission (pPlayerInfo, QSR_Quit);

        // set the HUD style back to what it was
        pWindow->SetHUDStyle (m_fHUDStyle);

        // load the customized key map if one exists
        pWindow->GetInput ()->LoadMap (INPUTMAP_FILE);

        // restart the game clock (if it was paused)
        if (Time::IsPaused ())
            Time::Continue ();
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::SetupShipAndCamera (void)
    {
        // set up the ship
        IshipIGC*   pShip = trekClient.GetShip ();
        //trekClient.ResetClusterScanners(trekClient.GetShip()->GetSide()); //very much not needed - station initialize already adds scanners
        pShip->SetOrientation(Orientation (Vector (1.0f, 0.0f, 0.0f), Vector (0.0f, 0.0f, 1.0f)));
        pShip->SetPosition(Vector(0.0f, 0.0f, 0.0f));
        pShip->SetCluster(trekClient.GetCore()->GetCluster(GetStartSectorID ()));

        // set up the camera
        TrekWindow* pWindow = GetWindow ();
        pWindow->SetViewMode (TrekWindow::vmCombat);
        pWindow->SetCameraMode (TrekWindow::cmExternalOrbit);
        pWindow->SetRadarMode (RadarImage::c_rlAll);
        //pWindow->EnableDisplacementCommandView (false);

        // set the unnecessary panes to off
        pWindow->TurnOffOverlayFlags (ofSectorPane | ofInventory | ofGameState);
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::Start (void)
    {
        trekClient.ClearLoadout ();
        trekClient.GetClientEventSource ()->OnClearChat ();
        CreateUniverse ();
        trekClient.OnEnterGame();
        m_pCondition = CreateMission ();
        ResetAction::Initialize ();
        CommandAcknowledgedCondition::SetCommandAcknowledged (c_cidDoNothing);
        m_pCondition->Start ();
        trekClient.m_serverOffset = 0;
    }

    //------------------------------------------------------------------------------
    bool        TrainingMission::Frame (void)
    {
        // Check that the play condition is valid.
        if (m_pCondition)
        {
            // Check to see if the mission is paused, and evaluate the mission
            // goals only if it is running.
            if (not Time::IsPaused ())
            {
                // Respawn mechanism
                char skipGroup = NA;
                for each (RespawnData* rd in m_RespawnList) {
                    if (!trekClient.GetCore()->GetShip(rd->pspawnCDA->GetShipID()) && Time::Now() > rd->tDestroyed + rd->downtime) { //ship is destroyed && ready to respawn
                        if (rd->tDestroyed == 0)
                            rd->tDestroyed = Time::Now();
                        else {
                            if (rd->spawnGroup == NA) {
                                rd->pspawnCDA->Execute();
                                rd->tDestroyed = 0;
                            }
                            else if (rd->spawnGroup != skipGroup) {
                                std::list<RespawnData*> spawnGroup;
                                spawnGroup.push_front(rd);
                                bool waveReady = true;
                                for each (RespawnData* rdOther in m_RespawnList) {
                                    if (rdOther->spawnGroup == rd->spawnGroup && rdOther->pspawnCDA->GetShipID() != rd->pspawnCDA->GetShipID()) {
                                        if (trekClient.GetCore()->GetShip(rdOther->pspawnCDA->GetShipID())) { // other ship is alive
                                            waveReady = false;
                                            break;
                                        }
                                        else { // other ship is destroyed
                                            if (rdOther->tDestroyed == 0)
                                                rdOther->tDestroyed = Time::Now();
                                            if (Time::Now() > rdOther->tDestroyed + rdOther->downtime)
                                                spawnGroup.push_back(rdOther);
                                            else {
                                                waveReady = false;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (waveReady) {
                                    for each (RespawnData* rdgm in spawnGroup) {
                                        ZAssert(!trekClient.GetCore()->GetShip(rdgm->pspawnCDA->GetShipID()));
                                        rdgm->pspawnCDA->Execute();
                                        rdgm->tDestroyed = 0;
                                    }
                                    break; // Enough time to spawn other ships next time ..
                                }
                                skipGroup = rd->spawnGroup; //groups get added in sequence, so only need to remember last
                            }
                        }
                    }
                }
                    

                // Check to see if there is a wait condition.
                std::list<Condition*>::iterator pWaitConditionIterator = m_WaitConditionList.begin ();
                std::list<Condition*>::iterator end = m_WaitConditionList.end ();
                if (pWaitConditionIterator != end)
                {
                    while (pWaitConditionIterator != end)
                    {
                        // See if the wait condition is satisfied.
                        if ((*pWaitConditionIterator)->Evaluate () == true)
                        {
                            // Delete the condition and remove it from the list so we don't
                            // check it next time around. Note that we have to advance the
                            // iterator at the same time in order to preserve traversal
                            // order.
                            delete (*pWaitConditionIterator);
                            m_WaitConditionList.erase (pWaitConditionIterator++);
                        }
                        else
                        {
                            // The wait condition is false, so leave it alone and advance
                            // the iterator.
                            pWaitConditionIterator++;
                        }
                    }
                }
                else
                {
                    // There is no wait condition, so evaluate the play condition.
                    if (m_pCondition->Evaluate ())
                        Terminate ();
                }
            }
            // Always return 'false' when running.
            return false;
        }
        else
        {
            // There is no play condition, so return 'true' to indicate completion.
            // This will result in termination of the mission. This path will occur
            // after Terminate has been called on the mission.
            return true;
        }
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::Terminate (void)
    {
        // pause the game clock, if it isn't already stopped
        if (not Time::IsPaused ())
            Time::Pause ();

        // swap the condition into dead space, and set it to null
        m_pDeadCondition = m_pCondition;
        m_pCondition = 0;
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::AddWaitCondition (Condition* pWaitCondition)
    {
        // XXX I don't remember if order is important, but look at ResetAction to see if it is
        m_WaitConditionList.push_front (pWaitCondition);
    }

    void TrainingMission::AddRespawn(CreateDroneAction* pCreateDroneAction, float downtime, char spawnGroup, bool spawnOnAdd) {
        RespawnData* rd = new RespawnData();
        rd->pspawnCDA = pCreateDroneAction;
        rd->downtime = downtime;
        rd->spawnGroup = spawnGroup;

        bool spawned = false;
        if (spawnOnAdd) {
            ImodelIGC* existingShip = trekClient.GetCore()->GetModel(OT_ship, rd->pspawnCDA->GetShipID());
            if (!existingShip) {
                rd->pspawnCDA->Execute();
                spawned = true;
            }
            rd->tDestroyed = 0;
        }
        else
            rd->tDestroyed = Time::Now();
        if (!spawned) {
            if (rd->pspawnCDA->GetCommandTargetType() == OT_buoy) {
                ImodelIGC* pmodel = trekClient.GetCore()->GetModel(OT_buoy, rd->pspawnCDA->GetCommandTargetID());
                ZAssert(pmodel);
                if (pmodel) {
                    ((IbuoyIGC*)pmodel)->AddConsumer();
                    rd->pspawnCDA->SetAddedConsumer();
                }
            }
        }

        m_RespawnList.push_front(rd);
    }

    void TrainingMission::RemoveRespawn(ShipID shipID) {
        for each (RespawnData* rd in m_RespawnList)
            if (rd->pspawnCDA->GetShipID() == shipID) {
                delete(rd->pspawnCDA);
                m_RespawnList.remove(rd);
            }
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::RecordChat (ChatTarget recipient)
    {
        if (m_pChatCondition)
            static_cast<GetChatCondition*> (m_pChatCondition)->RecordChat (recipient);
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::SetChatCondition (Condition* pChatCondition)
    {
        if (m_pChatCondition)
            delete m_pChatCondition;
        m_pChatCondition = pChatCondition;
    }

    //------------------------------------------------------------------------------
    bool        TrainingMission::RecordKeyPress (TrekKey key)
    {
        // check the key for tm conditions
        switch (key)
        {
            case TK_PauseTM:
            {
                // if the game is paused
                if (Time::IsPaused ())
                    // resume play
                    Time::Continue ();
                else
                    // otherwise pause it
                    Time::Pause ();
            }
            break;

          #ifdef _DEBUG_TRAINING
            case TK_SkipTMGoal:
            {
                // XXX NYI
            }
            break;

            case TK_IncreaseTMClockSpeed:
            {
                // increase the clock speed by increasing the clock shift value
                Time::SetShift (Time::GetShift () + 1);
            }
            break;

            case TK_DecreaseTMClockSpeed:
            {
                // decrease the clock speed by decreasing the clock shift value
                Time::SetShift (Time::GetShift () - 1);
            }
            break;

            case TK_ResetTMClockSpeed:
            {
                // reset the clock speed by clearing the clock shift value
                Time::SetShift (0);
            }
            break;
          #endif

            // XXX hack to disable some keys in training
            case TK_ConModeInvest:
            case TK_TargetSelf:
            case TK_Suicide:
            case TK_ConModeGameState:
            case TK_ConModeTeleport:
            case TK_RejectCommand: // pkk - Some training missions can't be finished
                if (GetMissionID() != 10) //Training::c_TM_10_Free_Flight
                    return false;
            case TK_ViewCommand:
            case TK_ConModeCommand:
            case TK_ToggleAutoPilot:
                if (!m_commandViewEnabled)
                    return false;
                //fallthrough otherwise
            default:
            {
                // iterate over the key conditions with the unknown key
                std::list<Condition*>::iterator iterator = m_KeyConditionList.begin ();
                while (iterator != m_KeyConditionList.end ())
                    (static_cast<GetKeyCondition*> (*iterator++))->KeyPressed (key);
            }
            break;
        }
        return true;
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::AddKeyCondition (Condition* pKeyCondition)
    {
        m_KeyConditionList.push_front (pKeyCondition);
    }

    //------------------------------------------------------------------------------
    bool        TrainingMission::ShipLanded (void)
    {
        return true;
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::ShipDied (ImodelIGC* pLauncher)
    {
        // fetch a few needed variables
        IshipIGC*               pShip = trekClient.GetShip ();
        TrekWindow*             pWindow = GetWindow ();

        // save the current status of the ship and camera
        m_deadViewMode = pWindow->GetViewMode ();
        m_deadCameraMode = pWindow->GetCameraMode ();
        m_deadSectorID = static_cast<SectorID> (pShip->GetCluster ()->GetObjectID ());


        /*******
        * Note from LANS on change here
        * Rather than only moving the player ship if it died hitting something, we want to move it no matter what
        * This way if it died in combat, the respawn isn't in quite the same place and gives the player a moment
        * before they get shot again
        *
        *
        * To change it back to the way it was, uncomment if (pLauncher && pLauncher->GetMission ())
        *******/

        // check if there was a second party involved in the player's death, and that party is valid
        //if (pLauncher && pLauncher->GetMission ())
        //{
            // if the death occurred because the ship whacked something, we want to be sure that 
            // it won't die immediately upon returning. To that end, we move the ship away from
            // whatever it collided with, and adjust its velocity accordingly
            Vector      shipPosition = pShip->GetPosition ();
            Vector      launcherPosition = pLauncher->GetPosition ();
            Vector      delta = shipPosition - launcherPosition;
            float       fDeltaLength = delta.Length ();
            Vector      deltaUnit = delta / fDeltaLength;
            float       fNewVelocity = (pShip->GetVelocity ().Length () * 0.5f);
            Vector      newVelocity = deltaUnit * fNewVelocity;
            //Increase distance moved. Used to be 1.5f here, not 15f.
            float       fNewSeparation = 1.5f * (pShip->GetRadius () + pLauncher->GetRadius ());
            if (fNewSeparation > fDeltaLength)
                pShip->SetPosition (launcherPosition + (deltaUnit * fNewSeparation));
            pShip->SetVelocity (newVelocity);
        //}

        // toss the player from the ship
        trekClient.EjectPlayer (pLauncher);

        // move the ship to empty space
        pShip->SetCluster (NULL);
    }

    //------------------------------------------------------------------------------
    bool        TrainingMission::RestoreShip (void)
    {
        // fetch a few needed variables
        IshipIGC*               pShip = trekClient.GetShip ();
        TrekWindow*             pWindow = GetWindow ();
        ImissionIGC*            pMission = trekClient.GetCore ();

        // restore the ship's health
        pShip->SetFraction (1.0f);

        // set the view cluster to end the camera override
        trekClient.SetViewCluster (NULL);

        // Set the ship cluster back to what it was before dying
        pShip->SetCluster (pMission->GetCluster (m_deadSectorID));

        // restore the view and camera modes to what they were before dying
        pWindow->SetViewMode (m_deadViewMode);
        pWindow->SetCameraMode (m_deadCameraMode);

        // set all of the clusters to be active so that movement works
        const ClusterListIGC*   pClusterList = pMission->GetClusters ();
        ClusterLinkIGC*         pClusterLink;
        for (pClusterLink = pClusterList->first (); pClusterLink; pClusterLink = pClusterLink->next ())
        {
            IclusterIGC * pCluster = pClusterLink->data ();
            pCluster->SetActive (true);
        }

        // wait for the override camera to end (wintrek.cpp, in OverrideCamera - time is 5 seconds or 3 seconds)
        AddWaitCondition (new ElapsedTimeCondition (5.0f));

        // always return true
        return true;
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::ShipKilled(IshipIGC* pShip, ImodelIGC* pLauncher)
    {
        ZAssert(pShip);
        ZAssert(pShip != trekClient.GetShip());
        if (pLauncher != NULL) {
            if (pLauncher->GetSide() == trekClient.GetSide() && pShip->GetPilotType() == c_ptWingman) {
                m_killCount++;
            }
        }
    }

    //------------------------------------------------------------------------------
    bool        TrainingMission::HandlePickDefaultOrder(IshipIGC* pShip)
    {
        return false;
    }

    //------------------------------------------------------------------------------
    void            TrainingMission::KillStationEvent(IstationIGC* pStation, ImodelIGC* pLauncher)
    {
        assert(pStation);
        IsideIGC* pside = pStation->GetSide();
        debugf("TrainingMission::KillStationEvent for %s, side: %s\n",pStation->GetName(), pside ? pside->GetName() : "NULL");

        if (pStation->GetSide() == trekClient.GetSide() && pStation->GetBaseStationType()->HasCapability(c_sabmRestart)) {
            bool stationRemaining = false;
            for (StationLinkIGC* l = pStation->GetSide()->GetStations()->first(); l != NULL; l = l->next()) {
                IstationIGC* s = l->data();
                if ((s != pStation) && s->GetBaseStationType()->HasCapability(c_sabmRestart)) {
                    stationRemaining = true;
                    break;
                }
            }
            if (!stationRemaining) {
                trekClient.PostText(true, "Your side's stations have all been destroyed!");
                debugf("Your side's stations have all been destroyed!\n");

                //End training in 4 seconds
                // clean up the wait conditions
                std::list<Condition*>::iterator pWaitConditionIterator = m_WaitConditionList.begin();
                std::list<Condition*>::iterator end = m_WaitConditionList.end();
                while (pWaitConditionIterator != end)
                    delete (*pWaitConditionIterator++);
                // add new wait condition
                m_pCondition = new Goal(new ElapsedTimeCondition(4.0f));
                m_pCondition->Start();
            }
        }
    }

    //------------------------------------------------------------------------------
    IshipIGC*   TrainingMission::GetCommanderShip (void) const
    {
        return trekClient.GetCore ()->GetShip (m_commanderID);
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::AddPartToShip (IshipIGC* pShip, PartID part, Mount mount, short ammo)
    {
        PartData        pd;
        pd.partID = part;
        pd.mountID = mount;
        pd.amount = ammo;
        pShip->CreateAndAddPart (&pd);
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::AddPartToShip (PartID part, Mount mount, short ammo)
    {
        AddPartToShip (trekClient.GetShip (), part, mount, ammo);
    }

    //------------------------------------------------------------------------------
    IshipIGC*   TrainingMission::CreateDrone (const ZString& name, ShipID shipID, HullID hullID, SideID sideID, PilotType pilotType)
    {
        DataShipIGC     dataShip;
        ImissionIGC*    pMission = trekClient.GetCore ();

        // generate a ship ID if we need to
        if (shipID == NA)
            shipID = pMission->GenerateNewShipID ();

        // set the known parameters
        dataShip.shipID = (shipID != NA) ? shipID : pMission->GenerateNewShipID ();
        dataShip.hullID = hullID;
        dataShip.sideID = sideID;
        dataShip.pilotType = pilotType;
        strcpy (dataShip.name, name);

        // clear out the dynamic values
        //dataShip.wingID = 0;
        dataShip.nKills = 0;
        dataShip.nDeaths = 0;
        dataShip.nParts = 0;
        dataShip.baseObjectID = NA;

        // assign order abilities appropriate to the pilot type
        switch (pilotType)
        {
            case c_ptMiner:     dataShip.abmOrders = c_aabmMineHe3; break;
            case c_ptBuilder:   dataShip.abmOrders = c_aabmBuildable; break;
            default:            dataShip.abmOrders = 0; break;
        }

        // create the ship
        IshipIGC* pShip = static_cast<IshipIGC*> (pMission->CreateObject (pMission->GetLastUpdate(), OT_ship, &dataShip, sizeof(DataShipIGC)));

        // create the player info
        CreatePlayerInfo (pShip, sideID, shipID);

        // set up the default loadouts for the desired ship
        DefaultLoadout (pShip);

        // make sure the drone works with autopilot
        pShip->SetAutopilot(true);

        // free the pointer
        pShip->Release();

        // return the ship pointer
        return pShip;
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::SetSkipPostSlideshow (void)
    {
        m_bSkipPostSlideShow = true;
    }

    //------------------------------------------------------------------------------
    bool        TrainingMission::GetCommandViewEnabled(void)
    {
        return m_commandViewEnabled;
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::LoadUniverse (const ZString& name, HullID hullID, StationID homeStationID)
    {
        ImissionIGC*        pCore = trekClient.ResetStaticData ();
        Time                now = pCore->GetLastUpdate();
        char*               szTrainingCoreFilename = IGC_TRAINING_CORE_FILENAME; //use IGC_STATIC_CORE_FILENAME for missions 1-5
        int                 iStaticCoreVersion = LoadIGCStaticCore (szTrainingCoreFilename, pCore, false);


        // stuff for creating the sides
        /*static const float  fSideColors[c_cSidesMax][3] =
                            {
                                {200.0f/255.0f,  15.0f/255.0f, 200.0f/255.0f},  //purple
                                {  8.0f/255.0f, 184.0f/255.0f, 184.0f/255.0f},  //teal
                                {  0.0f/255.0f,   0.0f/255.0f, 233.0f/255.0f},  //blue
                                {184.0f/255.0f, 184.0f/255.0f,  50.0f/255.0f},  //icky yellow
                                {184.0f/255.0f,  92.0f/255.0f,   0.0f/255.0f},  //icky orange
                                {123.0f/255.0f,  61.0f/255.0f,  61.0f/255.0f}   //icky magenta
                            }; // Changed because it doesn't match F6 colors*/
        static const float fSideColors[c_cSidesMax][3] =
                { { 188.0f / 255.0f, 160.0f / 255.0f,   0.0f / 255.0f }, //Gold
                { 0.0f / 255.0f, 138.0f / 255.0f, 217.0f / 255.0f }, //Blue
                { 156.0f / 255.0f,  16.0f / 255.0f, 102.0f / 255.0f }, //Purple
                { 50.0f / 255.0f, 140.0f / 255.0f,  20.0f / 255.0f }, //icky yellow
                { 255.0f / 255.0f, 145.0f / 255.0f, 145.0f / 255.0f }, //icky orange
                { 50.0f / 255.0f, 200.0f / 255.0f, 125.0f / 255.0f } };//icky magenta1*/
        static const char*  szSideNames[c_cSidesMax] =
                            {
                                "Iron League",
                                "Bios",
                                "Gigacorp",
                                "Protectorate",
                                "Colossal Mining Corp",
                                "Midnight Runners"
                            };
        static CivID        civs[c_cSidesMax] = {25, 35, 18, 25, 35, 18};

        if (GetMissionID() == 8) //c_TM_8_Nanite
        {
            szSideNames[0] = "Rixian Unity";
            civs[0] = 201;
        }

        assert (sizeofArray(szSideNames) == c_cSidesMax);
        assert (sizeofArray(civs) == c_cSidesMax);

        // set up the civs associated with the sides. There are more sides than civs, so we want to loop
        // over them once
        /*
        const CivilizationListIGC*  pCivsList = pCore->GetCivilizations ();
        CivilizationLinkIGC*    first = pCivsList->first ();
        int                     iCivCount = pCivsList->n ();
        for (int iSideIndex = 0; iSideIndex < iCivCount; iSideIndex++)
        {
            IcivilizationIGC*   civ = first->data ();
            civs[iSideIndex] =static_cast<CivID> (civ->GetObjectID ());
            first = first->next ();
        }
        for (; iSideIndex < c_cSidesMax; iSideIndex++)
            civs[iSideIndex] = civs[iSideIndex % iCivCount];
        */

        // set up the sides  
        for (SideID sid = 0; sid < c_cSidesMax; sid++)
        {
            IcivilizationIGC*   pCiv = pCore->GetCivilization (civs[sid]);
            DataSideIGC         ds;
 
            ds.nFlags = 0;
            ds.sideID = sid;
            ds.gasAttributes.Initialize();
            assert (civs[sid] != NA);
            ds.civilizationID = civs[sid];
            ds.color.SetRGBA(fSideColors[sid][0], fSideColors[sid][1], fSideColors[sid][2]);
            strcpy(ds.name, szSideNames[sid]);
            ds.ttbmDevelopmentTechs = pCiv->GetBaseTechs();
            ds.ttbmInitialTechs = pCiv->GetBaseTechs();
            ds.conquest = 100 / c_cSidesMax;
            ds.nKills = ds.nDeaths = ds.nBaseKills = ds.nBaseCaptures = ds.nFlags = ds.nArtifacts = 0;
            ds.squadID = NA;
            IsideIGC*           pSide = static_cast<IsideIGC*> (pCore->CreateObject(now, OT_side, &ds, sizeof(ds)));

            // make sure the player's side has all the tech bits
            if (sid == 0)
            {
                TechTreeBitMask allTechs;
                allTechs.SetAll ();
                pSide->SetDevelopmentTechs (allTechs);
                pSide->SetBuildingTechs (allTechs);
            }

            pSide->Release ();
        }

        // create the mission def
        FMD_S_MISSIONDEF    fmMissionDef;
        fmMissionDef.szDescription[0] = 0;
        strcpy (fmMissionDef.misparms.szIGCStaticFile, szTrainingCoreFilename);
        fmMissionDef.misparms.verIGCcore = iStaticCoreVersion;
        fmMissionDef.dwCookie = static_cast<DWORD> (pCore->GetMissionID ());
        fmMissionDef.iSideMissionOwner = 0;
        fmMissionDef.fAutoAcceptLeaders = false;
        fmMissionDef.fInProgress = true;
        fmMissionDef.stage = STAGE_STARTED;
        for (int q = 0; q < c_cSidesMax; q++)
        {
            strcpy (fmMissionDef.rgszName[q], szSideNames[q]);
            //fmMissionDef.rgCivID[q] = civs[q];
            fmMissionDef.rgStationID[q] = NA;
            fmMissionDef.rgShipIDLeaders[q] = NA;
            fmMissionDef.rgcPlayers[q] = 0;
            fmMissionDef.rgfAutoAccept[q] = true;
            fmMissionDef.rgfReady[q] = true;
            fmMissionDef.rgfForceReady[q] = true;
            fmMissionDef.rgfActive[q] = true;
			fmMissionDef.rgfAllies[q] = NA; //Imago 6/20/09 fix training AI for allies
        }
        MissionInfo* pMissionInfo = trekClient.MyMission();
        ZAssert (pMissionInfo);
        pMissionInfo->Update (&fmMissionDef);
        trekClient.GetClientEventSource ()->OnAddMission (pMissionInfo);

        // get the player ship
        IshipIGC*       pShip = trekClient.GetShip ();

        // set the player name
        pShip->SetName ("Cadet");

        // set the player info for the player
        CreatePlayerInfo (trekClient.GetShip (), 0, pCore->GenerateNewShipID());

        // now load the actual map world
        LoadIGCFile (name, pCore);

        // set up the default loadouts for the desired ship
        IhullTypeIGC*   pHullType = pCore->GetHullType (hullID);
        pShip->SetBaseHullType (pHullType);
        DefaultLoadout (pShip);
        trekClient.SaveLoadout (pShip);

        // set the trail color
        ThingSite*      pThingSite = trekClient.GetShip ()->GetThingSite();
        assert (pThingSite);
        pThingSite->SetTrailColor (pCore->GetSide (0)->GetColor());

        // create the commander ship
        m_commanderID = pCore->GenerateNewShipID();
        IshipIGC*   commanderShip = CreateDrone ("Commander", m_commanderID, 412, 0, c_ptWingman);

        // set all of the clusters to be active so that movement works
        const ClusterListIGC*   pClusterList = pCore->GetClusters ();
        ClusterLinkIGC*         pClusterLink;
        for (pClusterLink = pClusterList->first (); pClusterLink; pClusterLink = pClusterLink->next ())
        {
            IclusterIGC * pCluster = pClusterLink->data ();
            pCluster->SetActive (true);
        }
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::DefaultLoadout (IshipIGC* pShip)
    {
        IstationIGC* pstation = NULL;
        for (StationLinkIGC* l = pShip->GetSide()->GetStations()->first(); l!=NULL; l=l->next())
            if (l->data()->GetStationType()->GetCapabilities() & c_sabmLand) {
                pstation = l->data();
                break;
            }
        if (pstation) {
            debugf("DefaultLoadout station %s found for %s\n",pstation->GetName(), pShip->GetName());
            Money   refund = pShip->GetValue();
            trekClient.BuyDefaultLoadout(pShip, pstation, NULL, &refund);
        }
        else {
            debugf("DefaultLoadout station NOT found for %s\n", pShip->GetName());
            // install all the preferred part types
            const IhullTypeIGC* pHullType = pShip->GetHullType();
            for (PartTypeLinkIGC* pPartTypeLink = pHullType->GetPreferredPartTypes()->first(); pPartTypeLink != NULL; pPartTypeLink = pPartTypeLink->next())
            {
                IpartTypeIGC*   pPartType = pPartTypeLink->data();
                EquipmentType   equipmentType = pPartType->GetEquipmentType();
                Mount           mountMax = (equipmentType == ET_Weapon) ? pHullType->GetMaxWeapons() : 1;
                for (Mount i = 0; i < mountMax; i++)
                    if ((pShip->GetMountedPart(equipmentType, i) == NULL) && pHullType->CanMount(pPartType, i))
                        pShip->CreateAndAddPart(pPartType, i, 0x7fff);
            }

            // fill the empty cargo slots
            AddPartToShip(pShip, 24, -1, 0x7fff);  // ammo
            AddPartToShip(pShip, 24, -2, 0x7fff);  // ammo
            AddPartToShip(pShip, 25, -3, 0x7fff);  // fuel
            AddPartToShip(pShip, 150, -4, 0x7fff); // missiles
            AddPartToShip(pShip, 150, -5, 0x7fff); // missiles
        }

        // if there's a shield, make sure its charged
        IshieldIGC* pShield = static_cast<IshieldIGC*> (pShip->GetMountedPart (ET_Shield, 0));
        if (pShield)
            pShield->SetFraction (1.0f);

        // weapons, energy, and ammo
        pShip->SetAmmo (pShip->GetHullType ()->GetMaxAmmo ());
        pShip->SetEnergy (pShip->GetHullType ()->GetMaxEnergy ());
        pShip->SetFuel (pShip->GetHullType ()->GetMaxFuel ());
    }

    //------------------------------------------------------------------------------
    void        TrainingMission::CreatePlayerInfo (IshipIGC* pShip, SideID side, ShipID shipID)
    {
        ImissionIGC*        pMission = trekClient.GetCore ();

        // check that the side is set correctly
        IsideIGC*           pSide = pShip->GetSide ();
        if (pSide)
        {
            assert (static_cast<SideID> (pSide->GetObjectID ()) == side);
            pShip->SetSide (NULL);
        }

        // create the player info
        FMD_S_PLAYERINFO    fmPlayerInfo;
        PlayerInfo*         pPlayerInfo = trekClient.FindAndCreatePlayerLink (shipID);
        memset(&fmPlayerInfo, 0, sizeof(fmPlayerInfo));
        strcpy(fmPlayerInfo.CharacterName, pShip->GetName ());
        fmPlayerInfo.iSide = NA;
        fmPlayerInfo.dtidDrone = NA;
        fmPlayerInfo.shipID = shipID;
        fmPlayerInfo.cbrgPersistPlayerScores = 0;
        if (m_commanderID != NA && pShip->GetObjectID() == m_commanderID)
            fmPlayerInfo.fTeamLeader = true;
        pPlayerInfo->Set (&fmPlayerInfo);
        pPlayerInfo->SetMission (pMission);

        if (pShip == trekClient.GetShip ())
            trekClient.SetPlayerInfo (pPlayerInfo);

        // update the playerinfo data
        pPlayerInfo->SetShip (pShip);
        pShip->SetPrivateData (reinterpret_cast<DWORD> (pPlayerInfo));

        // join the mission and side
        trekClient.AddPlayerToMission (pPlayerInfo);
        trekClient.AddPlayerToSide (pPlayerInfo, side);
    }

    //------------------------------------------------------------------------------
    Goal*       TrainingMission::CreatePlaySoundGoal (SoundID sound)
    {
        PlaySoundAction*    pPlaySoundAction = new PlaySoundAction (sound);
        Goal*               pGoal = new Goal (new SoundFinishedCondition (pPlaySoundAction));
        pGoal->AddStartAction (pPlaySoundAction);
        return pGoal;
    }

    //------------------------------------------------------------------------------
    Condition*  TrainingMission::CreateTooCloseCondition (TRef<ImodelIGC> place)
    {
        TRef<ImodelIGC>                 ship = static_cast<ImodelIGC*> (trekClient.GetShip());
        ObjectWithinRadiusCondition*    pWithinRadiusCondition = new ObjectWithinRadiusCondition (ship, place, ship->GetRadius () * 10.0f);
        Condition*                      pMovingTowardsCondition = new ObjectMovingTowardsCondition (ship, place);
        Condition*                      pAndCondition = new AndCondition (pWithinRadiusCondition, pMovingTowardsCondition);
        MessageAction*                  pTooCloseMessage = new MessageAction (ZString ("Don't get too close to ") + GetModelName (place) + "!");
        float                           fDisplayRadius = pWithinRadiusCondition->GetRadarRadius ();
        char                            cFloatStringBuffer[256];
        fDisplayRadius = floorf (fDisplayRadius * 0.02f) * 50.0f;
        sprintf (cFloatStringBuffer, "Stay at least %g meters from ", floorf (fDisplayRadius));
        pTooCloseMessage->AddMessage (ZString (cFloatStringBuffer) + GetModelName (place) + "!");
        pTooCloseMessage->AddMessage ("A little close, don't ya think?");
        pTooCloseMessage->AddMessage ("Watch your range!");
        pTooCloseMessage->AddMessage ("Whoa! where are the parachutes?");
        pTooCloseMessage->AddMessage ("You scratch my ship, I scratch your hide!");
        Condition*                      pPeriodicCondition = new PeriodicCondition (pAndCondition, 2.0f);
        ConditionalAction*              pTooCloseCondition = new ConditionalAction (pPeriodicCondition, pTooCloseMessage, ConditionalAction::DELAYED_EXECUTION);
        return pTooCloseCondition;
    }

    //------------------------------------------------------------------------------
    Condition*  TrainingMission::CreateTooLongCondition (float fDelay, SoundID sound)
    {
        Condition*          pPeriodicCondition = new PeriodicCondition (new NTimesCondition (new TrueCondition, 2, true), fDelay);
        ConditionalAction*  pTooLongCondition = new ConditionalAction (pPeriodicCondition, new PlaySoundAction (sound));
        return pTooLongCondition;
    }

    //------------------------------------------------------------------------------
}
