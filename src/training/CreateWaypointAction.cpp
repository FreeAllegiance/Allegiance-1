/*
**  Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
**
**  File:	CreateWaypointAction.cpp
**
**  Author:  
**
**  Description:
**      Implementation of the training library "CreateWaypointAction" interface.
**
**  History:
*/
#include    "pch.h"
#include    "CreateWaypointAction.h"

namespace Training
{
    //------------------------------------------------------------------------------
    // class methods
    //------------------------------------------------------------------------------
    /* void */  CreateWaypointAction::CreateWaypointAction (BuoyID buoyID, const Vector& position, SectorID sectorID, BuoyType type, bool visible) : 
    CreateObjectAction (OT_buoy, &m_buoyData, sizeof (m_buoyData)),
    m_buoyID (buoyID),
    m_position (position),
    m_sectorID (sectorID),
    m_type(type),
    m_visible(visible)
    {
    }

    //------------------------------------------------------------------------------
    /* void */  CreateWaypointAction::~CreateWaypointAction (void)
    {
    }

    //------------------------------------------------------------------------------
    void        CreateWaypointAction::Execute (void)
    {
        ImissionIGC*    pMission = trekClient.GetCore();

        // create the waypoint object
        m_buoyData.position = m_position;
        m_buoyData.type = m_type; // c_buoyWaypoint;
        m_buoyData.clusterID = m_sectorID;
        m_buoyData.buoyID = m_buoyID;
        m_buoyData.visible = m_visible;
        CreateObjectAction::Execute ();
    }

    //------------------------------------------------------------------------------
}
