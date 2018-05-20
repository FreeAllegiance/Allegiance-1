/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	CreateDroneAction.cpp
**
**  Author:  
**
**  Description:
**      Implementation of the training library "CreateDroneAction" interface.
**
**  History:
*/
#include    "pch.h"
#include    "CreateDroneAction.h"
#include    "TrainingMission.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // global variabes
    //------------------------------------------------------------------------------
    extern TrainingMission*    g_pMission;

    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  CreateDroneAction::CreateDroneAction(const ZString& name, ShipID shipID, HullID hullID, SideID sideID, PilotType pilotType, int techLevel) :
        m_name(name),
        m_shipID(shipID),
        m_hullID(hullID),
        m_sideID(sideID),
        m_pilotType(pilotType),
        m_techLevel(techLevel),
        m_sectorID(NA),
        m_position(0.0f, 0.0f, 0.0f),
        m_orientation(Vector(1.0f, 0.0f, 0.0f), Vector(0.0f, 0.0f, 1.0f)),
        m_stationTypeID(NA),
        m_expendableTypeID(NA),
        m_SpawnAtShipID(NA),
        m_commandTargetType(OT_invalid),
        m_commandTargetID(NA),
        b_addedConsumer(false),
        m_wingmanBehaviour(c_wbbmUseMissiles | c_wbbmRunAt60Hull)
    {
    }

    //------------------------------------------------------------------------------
    /* void */  CreateDroneAction::~CreateDroneAction(void)
    {
        if (b_addedConsumer) {
            ZAssert(m_commandTargetID != NA);
            ZAssert(m_commandTargetType == OT_buoy);
            ImodelIGC* pmodel = trekClient.GetCore()->GetModel(m_commandTargetType, m_commandTargetID);
            if (pmodel)
                ((IbuoyIGC*)pmodel)->ReleaseConsumer();
        }
    }

    //------------------------------------------------------------------------------
    void        CreateDroneAction::Execute(void)
    {
        debugf("CreateDroneActionExecute for %s\n", (LPCSTR)m_name);
        IshipIGC*   pShip = g_pMission->CreateDrone(m_name, m_shipID, m_hullID, m_sideID, m_pilotType);

        bool spawningAtShip = false;
        if (m_SpawnAtShipID > NA) {
            IshipIGC* pspawnAt = trekClient.GetCore()->GetShip(m_SpawnAtShipID);
            if (pspawnAt && pspawnAt->GetCluster()) {
                spawningAtShip = true;
                pShip->SetPosition(pspawnAt->GetPosition() + m_SpawnAtShipOffset);
                pShip->SetOrientation(pspawnAt->GetOrientation());
                pShip->SetCluster(pspawnAt->GetCluster());
            }
        }
        if (!spawningAtShip) {
            pShip->SetPosition(m_position);
            pShip->SetOrientation(m_orientation);
            if (m_sectorID == NA)
                m_sectorID = Training::g_pMission->GetStartSectorID();
            pShip->SetCluster(trekClient.GetCore()->GetCluster(m_sectorID));
        }

        //if (m_pilotType == c_ptWingman)
        //    pShip->SetAggressiveness(10);

        if (m_stationTypeID != NA)
            pShip->SetBaseData(trekClient.GetCore()->GetStationType(m_stationTypeID));
        else if (m_expendableTypeID != NA)
            pShip->SetBaseData(trekClient.GetCore()->GetExpendableType(m_expendableTypeID));

        if (m_techLevel != 1) {
            //  Upgrade weapon
            for (int i = 0; i < pShip->GetHullType()->GetMaxWeapons(); i++) {
                IpartIGC* pPart = pShip->GetMountedPart(ET_Weapon, i);
                if (pPart) {
                    int partTypeID = pPart->GetPartType()->GetObjectID();
                    debugf(" Weapon slot %d: %s (%d)", i, pPart->GetPartType()->GetName(), partTypeID);
                    if (m_techLevel < 0 && pShip->GetHullType()->CanMount(trekClient.GetCore()->GetPartType(91), 0)) {
                        pPart->Terminate();
                        ZAssert(i == 0);
                        if (m_techLevel == -1)
                            pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(91), i, 1);  //Nan1
                        else if (m_techLevel == -2)
                            pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(190), i, 1); //Nan2
                        else if (m_techLevel <= -3)
                            pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(223), i, 1); //Nan3
                        break;
                    }
                    if (partTypeID == 33) { //33 PW Gat Gun
                        if (abs(m_techLevel) > 1)
                            pPart->Terminate();
                        if (abs(m_techLevel) == 2)
                            pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(71), i, 1); //PW Gat 2
                        else if (abs(m_techLevel) >= 3)
                            pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(72), i, 1); //PW Gat 3
                    }
                    else if (partTypeID == 120) { //120 PW Mini-Gun
                        if (abs(m_techLevel) > 1)
                            pPart->Terminate();
                        if (abs(m_techLevel) == 2)
                            pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(121), i, 1); //PW Mini 2
                        else if (abs(m_techLevel) >= 3)
                            pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(122), i, 1); //PW Mini 3
                    }
                    else if (partTypeID == 200) // EW A-B Stinger 1 -- Nerf for rix bomber. One gun vs Bios outpost is enough
                        if (i > 0 && m_techLevel < 1)
                            pPart->Terminate();
                    pPart = pShip->GetMountedPart(ET_Weapon, i);
                    if (pPart)
                        debugf(" changed to %s (%d)\n", pPart->GetPartType()->GetName(), pPart->GetPartType()->GetObjectID());
                    else
                        debugf(" removed\n");
                }
            }
            // Upgrade booster
            IpartIGC* ppBooster = pShip->GetMountedPart(ET_Afterburner, 0);
            if (ppBooster && ppBooster->GetPartType()->GetObjectID() == 23) { // Booster 1 equipped
                if (abs(m_techLevel) > 1)
                    ppBooster->Terminate();
                if (abs(m_techLevel) == 2)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(27), 0, 1);
                else if (abs(m_techLevel) >= 3)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(29), 0, 1);
            }
            // Upgrade missiles
            IpartIGC* ppMissiles = pShip->GetMountedPart(ET_Magazine, 0);
            if (ppMissiles && ppMissiles->GetPartType()->GetObjectID() == 153) { // Seeker 1 equipped
                if (abs(m_techLevel) > 1)
                    ppMissiles->Terminate();
                if (abs(m_techLevel) == 2)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(154), 0, MAXCHAR);
                else if (abs(m_techLevel) >= 3)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(156), 0, MAXCHAR);
            }
            else if (ppMissiles && ppMissiles->GetPartType()->GetObjectID() == 150) { // Dumbfire 1 equipped
                if (abs(m_techLevel) > 1)
                    ppMissiles->Terminate();
                if (abs(m_techLevel) == 2)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(103), 0, MAXCHAR); // Quickfire 1
                else if (abs(m_techLevel) >= 3)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(105), 0, MAXCHAR); // Quickfire 3
            }
            else if (ppMissiles && ppMissiles->GetPartType()->GetObjectID() == 157) { // SRM Anti-Base 1 equipped
                if (abs(m_techLevel) > 1)
                    ppMissiles->Terminate();
                if (abs(m_techLevel) == 2)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(158), 0, MAXCHAR);
                else if (abs(m_techLevel) >= 3)
                    pShip->CreateAndAddPart(trekClient.GetCore()->GetPartType(601), 0, MAXCHAR);
            }
            // Fill with prox
            if (m_techLevel < 0 && pShip->GetHullType()->CanMount(trekClient.GetCore()->GetPartType(59),0)) {
                IpartIGC* pPart = pShip->GetMountedPart(ET_Dispenser, 0);
                ZAssert(pPart);
                if (pPart)
                    pPart->Terminate();
                IpartTypeIGC* pprox;
                if (m_techLevel == -1)
                    pprox = trekClient.GetCore()->GetPartType(59); //Prox 1
                else if (m_techLevel == -2)
                    pprox = trekClient.GetCore()->GetPartType(58); //Prox 2
                else if (m_techLevel == -3)
                    pprox = trekClient.GetCore()->GetPartType(57); //Prox 3
                pShip->CreateAndAddPart(pprox, 0, MAXCHAR); //Prox 3
                Mount   cargo = -c_maxCargo;
                while (cargo < 0) {
                    pShip->GetMountedPart(NA, cargo)->Terminate();
                    pShip->CreateAndAddPart(pprox, cargo, MAXCHAR);
                    cargo++;
                }
            }
            trekClient.SaveLoadout(pShip);
        }

        ZAssert(pShip->GetPilotType() != c_ptWingman || pShip->GetMountedPart(ET_Weapon, 0)); // wingmen need a weapon. Check the team's tech bits.

        // Set behaviour
        pShip->SetWingmanBehaviour(m_wingmanBehaviour);

        // Set command on spawn
        if (m_commandTargetID > NA) {
            ImodelIGC* pmodel = trekClient.GetCore()->GetModel(m_commandTargetType, m_commandTargetID);
            if (pmodel) {
                pShip->SetCommand(c_cmdAccepted, pmodel, m_commandID);
                debugf("CreateDroneAction for %s found command target type: %d, id: %d\n", (LPCSTR)m_name, m_commandTargetType, m_commandTargetID);
                if (m_commandTargetType == OT_buoy && !b_addedConsumer)
                    ((IbuoyIGC*)pmodel)->AddConsumer();
            }
            else
                debugf("CreateDroneAction for %s can't find command target type: %d, id: %d\n", (LPCSTR)m_name, m_commandTargetType, m_commandTargetID);
        }
    }

    //------------------------------------------------------------------------------
    void        CreateDroneAction::SetCreatedLocation (SectorID sectorID, const Vector& position)
    {
        m_sectorID = sectorID;
        m_position = position;
    }

    //------------------------------------------------------------------------------
    void        CreateDroneAction::SetCreatedOrientation (const Vector& forward, const Vector& up)
    {
        m_orientation.Set (forward, up);
    }

    //------------------------------------------------------------------------------
    void        CreateDroneAction::SetStationType (StationTypeID stationTypeID)
    {
        m_stationTypeID = stationTypeID;
    }

    //------------------------------------------------------------------------------
    void        CreateDroneAction::SetExpendableType (ExpendableTypeID expendableTypeID)
    {
        m_expendableTypeID = expendableTypeID;
    }

    //------------------------------------------------------------------------------
    void        CreateDroneAction::SetCreatedLocationAtShipID(ShipID shipID, const Vector& offset)
    {
        m_SpawnAtShipID = shipID;
        m_SpawnAtShipOffset = offset;
    }

    //------------------------------------------------------------------------------
    void        CreateDroneAction::SetCreatedCommand(int targetID, ObjectType targetType, CommandID command)
    {
        m_commandTargetID = targetID;
        m_commandTargetType = targetType;
        m_commandID = command;
    }

    void        CreateDroneAction::SetCreatedBehaviour(WingmanBehaviourBitMask wingmanBehaviour)
    {
        m_wingmanBehaviour = wingmanBehaviour;
    }

    void        CreateDroneAction::SetCreatedTechLevel(int techLevel)
    {
        m_techLevel = techLevel;
    }

    void        CreateDroneAction::SetCreatedHullType(HullID hullID)
    {
        m_hullID = hullID;
    }

    void        CreateDroneAction::SetCreatedName(ZString name)
    {
        m_name = name;
    }
}
