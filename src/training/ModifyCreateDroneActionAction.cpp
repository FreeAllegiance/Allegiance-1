
#include    "pch.h"
#include    "ModifyCreateDroneActionAction.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  ModifyCreateDroneActionAction::ModifyCreateDroneActionAction(CreateDroneAction* cda) :
        m_cda(cda),
        m_locationSector(NA),
        m_behaviour(NA),
        m_techLevel(-100),
        m_hullID(NA),
        b_changeName(false),
        m_commandTargetID(-100)
    {
    }

    //------------------------------------------------------------------------------
    /* void */  ModifyCreateDroneActionAction::~ModifyCreateDroneActionAction(void)
    {
    }

    void ModifyCreateDroneActionAction::ModifyCreatedBehaviour(WingmanBehaviourBitMask behaviour) {
        m_behaviour = behaviour;
    }

    void ModifyCreateDroneActionAction::ModifyCreatedTechLevel(char techLevel) {
        m_techLevel = techLevel;
    }

    void ModifyCreateDroneActionAction::ModifyCreatedHullType(HullID hullID) {
        m_hullID = hullID;
    }

    void ModifyCreateDroneActionAction::ModifyCreatedName(ZString name) {
        m_name = name;
        b_changeName = true;
    }

    void ModifyCreateDroneActionAction::ModifyCreatedLocation(SectorID sectorID, const Vector& position) {
        m_locationSector = sectorID;
        m_locationPosition = position;
    }

    void ModifyCreateDroneActionAction::ModifyCreatedCommand(int targetID, ObjectType targetType, CommandID command) {
        m_commandTargetID = targetID;
        m_commandTargetType = targetType;
        m_commandID = command;
    }



    //------------------------------------------------------------------------------
    void        ModifyCreateDroneActionAction::Execute(void)
    {
        ZAssert(m_cda);
        //debugf("ModifyCreateDroneActionAction Execute shipID: %d\n", m_cda->GetShipID());
        
        if (m_behaviour > NA)
            m_cda->SetCreatedBehaviour(m_behaviour);
        if (m_techLevel > -100)
            m_cda->SetCreatedTechLevel(m_techLevel);
        if (m_hullID > NA)
            m_cda->SetCreatedHullType(m_hullID);
        if (b_changeName)
            m_cda->SetCreatedName(m_name);
        if (m_locationSector > NA)
            m_cda->SetCreatedLocation(m_locationSector, m_locationPosition);
        if (m_commandTargetID > -100)
            m_cda->SetCreatedCommand(m_commandTargetID, m_commandTargetType, m_commandID);
    }
}
