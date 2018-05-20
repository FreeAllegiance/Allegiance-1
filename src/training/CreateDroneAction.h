/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	CreateDroneAction.h
**
**  Author:  
**
**  Description:
**      Header file for the training library "CreateDroneAction" interface.
**
**  History:
*/
#ifndef _CREATE_DRONE_ACTION_H_
#define _CREATE_DRONE_ACTION_H_

#ifndef _ACTION_H_
#include "Action.h"
#endif// _ACTION_H_

namespace Training
{
    //------------------------------------------------------------------------------
    // class definitions
    //------------------------------------------------------------------------------
    class CreateDroneAction : public Action
    {
        public:
                    /* void */          CreateDroneAction (const ZString& name, ShipID shipID, HullID hullID, SideID sideID, PilotType pilotType, int techLeve = 1);
            virtual /* void */          ~CreateDroneAction (void);
            virtual void                Execute (void);
                    void                SetCreatedLocation (SectorID sectorID, const Vector& position);
                    void                SetCreatedOrientation (const Vector& forward, const Vector& up);
                    void                SetStationType (StationTypeID stationTypeID);
                    void                SetExpendableType (ExpendableTypeID expendableTypeID);
                    void                SetCreatedLocationAtShipID(ShipID shipID, const Vector& offset = Vector(100.0f, 0.0f, 0.0f));
                    void                SetCreatedCommand(int targetID, ObjectType targetType, CommandID command);
                    void                SetCreatedBehaviour(WingmanBehaviourBitMask m_wingmanBehaviour);
                    void                SetCreatedTechLevel(int techLevel);
                    void                SetCreatedHullType(HullID hullID);
                    void                SetCreatedName(ZString name);

                    HullID              GetShipID() 
                    {
                        return m_shipID;
                    }
                    ObjectType          GetCommandTargetType() 
                    {
                        return m_commandTargetType;
                    }
                    int                 GetCommandTargetID() 
                    {
                        return m_commandTargetID;
                    }
                    void                SetAddedConsumer() 
                    {
                        b_addedConsumer = true;
                    }

        protected:
                    ZString             m_name;
                    ShipID              m_shipID;
                    HullID              m_hullID;
                    SideID              m_sideID;
                    PilotType           m_pilotType;
                    SectorID            m_sectorID;
                    Vector              m_position;
                    Orientation         m_orientation;
                    StationTypeID       m_stationTypeID;
                    ExpendableTypeID    m_expendableTypeID;
                    ShipID              m_SpawnAtShipID;
                    Vector              m_SpawnAtShipOffset;
                    int                 m_techLevel;
                    int                 m_commandTargetID;
                    ObjectType          m_commandTargetType;
                    CommandID           m_commandID;
                    bool                b_addedConsumer;
                    WingmanBehaviourBitMask m_wingmanBehaviour;
    };

    //------------------------------------------------------------------------------
}

#endif  //_CREATE_DRONE_ACTION_H_
