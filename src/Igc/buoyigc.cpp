/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	buoyIGC.cpp
**
**  Author: 
**
**  Description:
**      Implementation of the CbuoyIGC class. This file was initially created by
**  the ATL wizard for the core object.
**
**      This particular implementation of buoys supports non-tracking missiles with
**  a fixed run (time at which they expire).
**
**  History:
*/
// buoyIGC.cpp : Implementation of CbuoyIGC
#include "buoyigc.h"

/////////////////////////////////////////////////////////////////////////////
// CbuoyIGC
CbuoyIGC::CbuoyIGC(void)
:
    m_nConsumers(0)
{
}

CbuoyIGC::~CbuoyIGC(void)
{
}

HRESULT CbuoyIGC::Initialize(ImissionIGC* pMission, Time now, const void* data, int dataSize)
{
    TmodelIGC<IbuoyIGC>::Initialize(pMission, now, data, dataSize);

    ZRetailAssert (data && (dataSize == sizeof(DataBuoyIGC)));
    DataBuoyIGC*  dataBuoy = (DataBuoyIGC*)data;

    LoadDecal("buoy", "buoyicon",
              Color(1.0f, 1.0f, 1.0f, 1.0f),
              false,
              1.0f,
              c_mtStatic | c_mtPredictable);

    if (dataBuoy->visible)
        SetRadius(20.0f);
    else
        SetRadius(2.0f);
    m_visible = dataBuoy->visible;

    SetPosition(dataBuoy->position);
    {
        Rotation    r(Vector(0.0f, 0.0f, 1.0f), 0.5f);
        SetRotation(r);
    }
    m_type = dataBuoy->type;

    m_buoyID = (dataBuoy->buoyID == NA) ? pMission->GenerateNewBuoyID() : dataBuoy->buoyID;

    IclusterIGC* pcluster = pMission->GetCluster(dataBuoy->clusterID);

    if (dataBuoy->type == c_buoyCluster)
        SetSecondaryName(pcluster->GetName());
    else
        SetSecondaryName("waypoint");

    SetMass(0.0f);

    pMission->AddBuoy(this);

    SetCluster(pcluster);

    return S_OK;
}

void    CbuoyIGC::Terminate(void)
{
    AddRef();

    GetMyMission()->DeleteBuoy(this);
	TmodelIGC<IbuoyIGC>::Terminate();

    Release();
}

int     CbuoyIGC::Export(void*    data) const
{
    if (data)
    {
        DataBuoyIGC*  dataBuoy = (DataBuoyIGC*)data;

        dataBuoy->position = GetPosition();
        dataBuoy->clusterID = GetCluster()->GetObjectID();
        dataBuoy->type = m_type;
    }

    return sizeof(DataBuoyIGC);
}
