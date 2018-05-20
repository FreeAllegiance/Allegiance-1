/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	buoyIGC.h
**
**  Author: 
**
**  Description:
**      Header for the CbuoyIGC class. This file was initially created by
**  the ATL wizard.
**
**  History:
*/
// buoyIGC.h : Declaration of the CbuoyIGC

#ifndef __BUOYIGC_H_
#define __BUOYIGC_H_

#include "igc.h"
#include "modelIGC.h"

class CbuoyIGC : public TmodelIGC<IbuoyIGC>
{
    public:
        CbuoyIGC(void);
        ~CbuoyIGC(void);

    public:
    // IbaseIGC
	    virtual HRESULT             Initialize(ImissionIGC* pMission, Time now, const void* data, int dataSize);
	    virtual void                Terminate(void);
        virtual int                 Export(void* data) const;

        virtual ObjectType          GetObjectType(void) const
        {
            return OT_buoy;
        }

        virtual ObjectID    GetObjectID(void) const
        {
            return m_buoyID;
        }

    // ImodelIGC
        virtual void                SetCluster(IclusterIGC* cluster)
        {
            AddRef();

            {
                IclusterIGC*    c = GetCluster();
                if (c)
                    c->DeleteModel(this);
            }

            TmodelIGC<IbuoyIGC>::SetCluster(cluster);

            if (cluster)
                cluster->AddModel(this);

            Release();
        }

        virtual void                 SetVisibleF(bool   vf) const
        {
        }

        virtual bool                 GetVisibleF(void) const
        {
            return (m_nConsumers > 0 && m_visible);
        }
        virtual void                 SetRender(unsigned char render)
        {
        }

    // IbuoyIGC
        virtual void                 AddConsumer(void)
        {
            m_nConsumers++;
        }
        virtual void                 ReleaseConsumer(void)
        {
            m_nConsumers--;
            assert (m_nConsumers >= 0);

            if (m_nConsumers == 0)
                Terminate();
        }

        virtual BuoyType             GetBuoyType(void)
        {
            return m_type;
        }

    private:
        short    m_nConsumers;
        BuoyID   m_buoyID;
        BuoyType m_type;
        bool     m_visible;
};

#endif //__BUOYIGC_H_
