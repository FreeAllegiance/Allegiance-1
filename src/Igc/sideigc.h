/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:    sideIGC.h
**
**  Author: 
**
**  Description:
**      Header for the CsideIGC class. This file was initially created by
**  the ATL wizard.
**
**  History:
*/
// sideIGC.h : Declaration of the CsideIGC

#ifndef __SIDEIGC_H_
#define __SIDEIGC_H_
#include "igc.h"

/////////////////////////////////////////////////////////////////////////////
// CsideIGC
class       CsideIGC : public IsideIGC
{
    public:
        CsideIGC(void)
        :
            m_activeF(false), // sides are inactive until mission creation is over
            m_dwPrivate(0)
        {
        }

        ~CsideIGC(void)
        {
        }

    public:

    // IbaseIGC
        virtual HRESULT                 Initialize(ImissionIGC* pMission, Time now, const void* data, int dataSize);
        virtual void                    Terminate(void);
        virtual void                    Update(Time now)
        {
            //Update the buckets attached to the station.
            for (BucketLinkIGC* l = m_buckets.first();
                 (l != NULL);
                 l = l->next())
            {
                l->data()->Update(now);
            }

            m_lastUpdate = now;
        }

        virtual int                     Export(void* data) const;

        virtual ObjectType              GetObjectType(void) const
        {
            return OT_side;
        }

        virtual ObjectID                GetObjectID(void) const
        {
            return m_data.sideID;
        }

        virtual ImissionIGC*    GetMission(void) const
        {
            return m_pMission;
        }

    // IsideIGC
        virtual IcivilizationIGC*       GetCivilization(void) const
        {
            return m_pCivilization;
        }
        virtual void                    SetCivilization(IcivilizationIGC*   pciv)
        {
            assert (pciv);
            assert (m_pCivilization);

            if (m_pCivilization != pciv)
            {
                m_pCivilization->Release();
                m_pCivilization = pciv;
                pciv->AddRef();

                // set the tech bits for the new civ
                m_data.ttbmDevelopmentTechs = GetCivilization()->GetBaseTechs();
                m_ttbmBuildingTechs.ClearAll();
                m_data.civilizationID = GetCivilization()->GetObjectID();

                //Turn all space stations into the base space station for the new civ
                {
                    for (StationLinkIGC*    psl = m_stations.first(); (psl != NULL); psl = psl->next())
                    {
                        IstationIGC*    pstation = psl->data();

                        pstation->SetBaseStationType(pciv->GetInitialStationType());
                    }
                }
            }
        }

        virtual void        DestroyBuckets(void);
        virtual void        CreateBuckets(void);

        void                SetName(const char* newVal)
        {
			//Rock / Imago 7/28/09
			ZString strName = newVal;
			int istart = strName.ReverseFind("\x81");
			int iend = strName.ReverseFind("\x82");
			if ( (istart != -1 && iend == -1) || iend < istart)
				strName += END_COLOR_STRING;
			UTL::putName(m_data.name, (PCC)strName);
        }

        virtual const char*             GetName(void) const
        {
            return m_data.name;
        }

        virtual SquadID     GetSquadID() const
        {
            return m_data.squadID;
        }

        virtual void        SetSquadID(SquadID squadID)
        {
            m_data.squadID = squadID;
        }

        virtual const TechTreeBitMask  GetTechs(void) const
        {
            return m_ttbmBuildingTechs | m_data.ttbmDevelopmentTechs;
        }
        
        virtual const TechTreeBitMask&  GetBuildingTechs(void) const
        {
            return m_ttbmBuildingTechs;
        }
        virtual void                    ResetBuildingTechs(void)
        {
            //See what the new set of building techs is, from scratch
            TechTreeBitMask ttbm;
            ttbm.ClearAll();

            {
                for (StationLinkIGC*    l = m_stations.first();
                     (l != NULL);
                     l = l->next())
                {
                    ttbm |= l->data()->GetStationType()->GetEffectTechs();
                }
            }

            SetBuildingTechs(ttbm);
        }

        virtual void                    SetBuildingTechs(const TechTreeBitMask& ttbm)
        {
            if (ttbm != m_ttbmBuildingTechs)
            {
                m_ttbmBuildingTechs = ttbm;

                m_pMission->GetIgcSite()->SideBuildingTechChange(this);

                AdjustBuckets();
            }
        }

        virtual const TechTreeBitMask&  GetDevelopmentTechs(void) const
        {
            return m_data.ttbmDevelopmentTechs;
        }
        virtual void                    SetDevelopmentTechs(const TechTreeBitMask& ttbm)
        {
            if (ttbm != m_data.ttbmDevelopmentTechs)
            {
                m_data.ttbmDevelopmentTechs = ttbm;
                m_pMission->GetIgcSite()->SideDevelopmentTechChange(this);

                AdjustBuckets();
            }
        }
		// KGJV #118 - same as ApplyDevelopmentTechs but without doing anything (just the rc)
		virtual bool					IsNewDevelopmentTechs(const TechTreeBitMask& ttbm)
		{
			if (!(ttbm <= m_data.ttbmDevelopmentTechs))
				return true;
			else
				return false;
		}
        virtual bool                    ApplyDevelopmentTechs(const TechTreeBitMask& ttbm)
        {
            bool    rc;
            if (!(ttbm <= m_data.ttbmDevelopmentTechs))
            {
                rc = true;

                m_data.ttbmDevelopmentTechs |= ttbm;
                m_pMission->GetIgcSite()->SideDevelopmentTechChange(this);

                AdjustBuckets();
            }
            else
                rc = false;

            return rc;
        }
        virtual const TechTreeBitMask&  GetInitialTechs(void) const
        {
            return m_data.ttbmInitialTechs;
        }
        virtual void                    SetInitialTechs(const TechTreeBitMask& ttbm)
        {
            m_data.ttbmInitialTechs = ttbm;
        }
        virtual void                    UpdateInitialTechs(void)
        {
            m_data.ttbmInitialTechs = m_data.ttbmDevelopmentTechs;
        }

        virtual bool                    CanBuy(const IbuyableIGC* b) const
        {

            ObjectType  type = b->GetObjectType();
            if (type == OT_bucket)
            {
                b = ((IbucketIGC*)b)->GetBuyable();
                type = b->GetObjectType();
            }

            TechTreeBitMask         ttbmSide = (m_ttbmBuildingTechs | m_data.ttbmDevelopmentTechs);
            const TechTreeBitMask&  ttbmRequired = b->GetRequiredTechs();

            bool    bAvailable;
            if (type == OT_development)
            {
                //A non-localized item ... buyable based only on global tech bits
                bAvailable = (ttbmRequired <= ttbmSide) ||
                             (b->GetObjectID() == c_didTeamMoney);        //If you have the team money dev ... you can buy it
            }
            else
            {
                //Station types can not be bought if they are obsolete
                if (type == OT_stationType)
                {
                    //Station types can not be bought if their successor is available
                    const IstationTypeIGC*    pstSuccessor = ((IstationTypeIGC*)b)->GetSuccessorStationType(this);
                    if (pstSuccessor != b)
                        return false;
                }

                //Station type or drone type: these are built at stations so see if any station
                //can build them.
                bAvailable = false;
                for (StationLinkIGC*    psl = m_stations.first(); (psl != NULL); psl = psl->next())
                {
                    TechTreeBitMask  ttbmStation = psl->data()->GetStationType()->GetLocalTechs() |
                                                   ttbmSide;

                    if (ttbmRequired <= ttbmStation)
                    {
                        bAvailable = true;
                        break;
                    }
                }
            }

            return bAvailable;
        }

        virtual void                    AddStation(IstationIGC* s)
        {
            AddIbaseIGC((BaseListIGC*)&m_stations, s);

            SetBuildingTechs(m_ttbmBuildingTechs |
                             s->GetStationType()->GetEffectTechs());
        }
        virtual void                    DeleteStation(IstationIGC* s)
        {
            DeleteIbaseIGC((BaseListIGC*)&m_stations, s);

            //See what the new set of building techs is, from scratch
            ResetBuildingTechs();
        }
        virtual IstationIGC*   GetStation(StationID id) const
        {
            return (IstationIGC*)GetIbaseIGC((BaseListIGC*)&m_stations, id);
        }
        virtual const StationListIGC*   GetStations(void) const
        {
            return &m_stations;
        }

        virtual void                    AddShip(IshipIGC* s)
        {
            AddIbaseIGC((BaseListIGC*)&m_ships, s);
        }
        virtual void                    DeleteShip(IshipIGC* s)
        {
            DeleteIbaseIGC((BaseListIGC*)&m_ships, s);
        }
        virtual IshipIGC*   GetShip(ShipID id) const
        {
            return (IshipIGC*)GetIbaseIGC((BaseListIGC*)&m_ships, id);
        }
        virtual const ShipListIGC*   GetShips(void) const
        {
            return &m_ships;
        }

        virtual void                    AddBucket(IbucketIGC* b)
        {
            AddIbaseIGC((BaseListIGC*)&m_buckets, b);
        }
        virtual void                    DeleteBucket(IbucketIGC* b)
        {
            DeleteIbaseIGC((BaseListIGC*)&m_buckets, b);
        }

        virtual IbucketIGC*                     GetBucket(BucketID      bucketID) const
        {
            return (IbucketIGC*)GetIbaseIGC((BaseListIGC*)&m_buckets, bucketID);
        }
        virtual const BucketListIGC*   GetBuckets(void) const
        {
            return &m_buckets;
        }

        virtual const GlobalAttributeSet&   GetGlobalAttributeSet(void) const
        {
            return m_data.gasAttributes;
        }

        virtual void                        SetGlobalAttributeSet(const GlobalAttributeSet& gas)
        {
            m_data.gasAttributes = gas;
            m_pMission->GetIgcSite()->SideGlobalAttributeChange(this);
        }

        virtual void                        ApplyGlobalAttributeSet(const GlobalAttributeSet& gas)
        {
            m_data.gasAttributes.Apply(gas);
            m_pMission->GetIgcSite()->SideGlobalAttributeChange(this);
        }

        virtual void                        ResetGlobalAttributeSet(void)
        {
            m_data.gasAttributes.Initialize();
            m_pMission->GetIgcSite()->SideGlobalAttributeChange(this);
        }

        virtual const TechTreeBitMask&      GetUltimateTechs(void) const
        {
            return m_ttbmUltimateTechs;
        }

        virtual bool                        GetActiveF(void) const
        {
            return m_activeF;
        }
        virtual void                        SetActiveF(bool activeF)
        {
            m_activeF = activeF;
        }

        virtual const Color&                GetColor(void) const
        {
            return m_data.color;
        }

        virtual void                        SetPrivateData(DWORD dwPrivate)
        {
            m_dwPrivate = dwPrivate;
        }

        virtual DWORD                       GetPrivateData(void) const
        {
            return m_dwPrivate;
        }

        virtual void                        AddToStockpile(IbuyableIGC* b, short count)
        {
            assert (count > 0);

            //See if there are already entries in the stockpile
            StockpileLink*  plink = m_stockpile.first();
            {
                while (plink)
                {
                    if (plink->data().buyable == b)
                        break;

                    plink = plink->next();
                }
            }

            if (!plink)
            {
                plink = new StockpileLink;
                plink->data().buyable = b;
                plink->data().count = 0;

                m_stockpile.first(plink);
            }

            assert (plink);
            plink->data().count += count;
        }
        virtual short                        RemoveFromStockpile(IbuyableIGC* b, short count)
        {
            assert (count > 0);

            StockpileLink*  plink = m_stockpile.first();
            while (plink)
            {
                assert (plink->data().count > 0);

                if (plink->data().buyable == b)
                {
                    if (count >= plink->data().count)
                    {
                        count = plink->data().count;
                        delete plink;
                    }
                    else
                        plink->data().count -= count;

                    return count;
                }

                plink = plink->next();
            }

            return 0;
        }
        virtual short                       GetStockpile(IbuyableIGC* b) const
        {
            StockpileLink*  plink = m_stockpile.first();
            while (plink)
            {
                if (plink->data().buyable == b)
                    return plink->data().count;

                plink = plink->next();
            }

            return 0;
        }
        virtual const StockpileList*        GetStockpile(void) const
        {
            return &m_stockpile;
        }

        virtual short                       GetKills(void) const
        {
            return m_data.nKills;
        }
        virtual void                        AddKill(void)
        {
            m_data.nKills++;
        }

        virtual short                       GetDeaths(void) const
        {
            return m_data.nDeaths;
        }
        virtual void                        AddDeath(void)
        {
            m_data.nDeaths++;
        }

        virtual short                       GetEjections(void) const
        {
            return m_data.nEjections;
        }
        virtual void                        AddEjection(void)
        {
            m_data.nEjections++;
        }

        virtual short                       GetBaseKills(void) const
        {
            return m_data.nBaseKills;
        }
        virtual void                        AddBaseKill(void)
        {
            m_data.nBaseKills++;
        }

        virtual short                       GetBaseCaptures(void) const
        {
            return m_data.nBaseCaptures;
        }
        virtual void                        AddBaseCapture(void)
        {
            m_data.nBaseCaptures++;
        }
		bool GetRandomCivilization(void) const
		{
			return bRandomCivilization;
		}
		void SetRandomCivilization(bool rand)
		{
			bRandomCivilization = rand;
		}
		
		//Xynth add funtion to set number of players on a side
        int GetNumPlayersOnSide(void) const
		{
			int toReturn = 0;
			for (ShipLinkIGC* psl = m_ships.first(); psl != NULL; psl = psl->next())
			{
				IshipIGC*    ps = psl->data();
				if (ps->GetPilotType() == c_ptPlayer)
					toReturn++;
			}

			return toReturn;

		}

        unsigned char GetConquestPercent(void) const
        {
            return m_data.conquest;
        }
        void          SetConquestPercent(unsigned char newVal)
        {
            m_data.conquest = newVal;
        }
        unsigned char GetTerritoryCount(void) const
        {
            return m_data.territory;
        }
        void          SetTerritoryCount(unsigned char newVal)
        {
            m_data.territory = newVal;
        }

        short GetFlags(void) const
        {
            return m_data.nFlags;
        }
        void          SetFlags(short newVal)
        {
            m_data.nFlags = newVal;
        }

        short GetArtifacts(void) const
        {
            return m_data.nArtifacts;
        }
        void          SetArtifacts(short newVal)
        {
            m_data.nArtifacts = newVal;
        }

        float GetTimeEndured() const
        {
            return m_data.fTimeEndured;
        }

        void SetTimeEndured(float fSeconds)
        {
            m_data.fTimeEndured = fSeconds;
        }

        virtual long GetProsperityPercentBought(void) const;
        virtual long GetProsperityPercentComplete(void) const;

        void Reset(void)
        {
            m_data.nFlags = 0;
            m_data.nArtifacts = 0;
            m_data.nDeaths = 0;
            m_data.nEjections = 0;
            m_data.nKills = 0;
            m_data.nBaseKills = 0;
            m_data.nBaseCaptures = 0;
            m_data.conquest = 0;
        }

		// #ALLY
		void SetAllies(char allies)
		{
			m_data.allies = allies;
		}
		char GetAllies()
		{
			return m_data.allies;
		}
        void HandleNewEnemyCluster(IclusterIGC* pcluster);

        void UpdateTerritory();

        ClusterListIGC GetTerritory() 
        {
            return m_territoryClusters;
        }

        bool IsTerritory(IclusterIGC* pcluster)
        {
            return (m_territoryClusters.find(pcluster) != NULL);
        }
		
    private:
        bool IsSurroundedByTerritory(IclusterIGC* pcluster, ClusterListIGC* clustersLinked, ClusterListIGC* clustersTerritory, int currentDepth = 0);

        void    AdjustBuckets(void)
        {
            //Empty the side buckets we can no longer build
            for (BucketLinkIGC* l = m_buckets.first();
                 (l != NULL);
                 l = l->next())
            {
                IbucketIGC* b = l->data();
                if (!b->GetCompleteF())
                {
                    IbuyableIGC*    pbuy = b->GetBuyable();
                    switch (pbuy->GetObjectType())
                    {
                        case OT_development:
                        {
                            IdevelopmentIGC*    d = (IdevelopmentIGC*)pbuy;
                            assert (d->GetObjectType() == OT_development);

                            if (d->IsObsolete(m_data.ttbmDevelopmentTechs))
                            {
                                //This will not give us anything ... so mark it as complete
                                b->SetTimeAndMoney(1000 * b->GetTimeToBuild(),
                                                   b->GetPrice());
                            }
                            else if ((b->GetMoney() != 0) && !CanBuy(pbuy))
                                b->SetEmpty();
                        }
                        break;

                        case OT_stationType:
                        {
                            Money   money = b->GetMoney();
                            if ((money != 0) && !CanBuy(pbuy))
                            {
                                IstationTypeIGC*    pstSuccessor = ((IstationTypeIGC*)pbuy)->GetSuccessorStationType(this);
                                if (pstSuccessor != pbuy)
                                {
                                    //Find the bucket that corresponds to the successor station
                                    for (BucketLinkIGC* l2 = m_buckets.first();
                                         (l2 != NULL);
                                         l2 = l2->next())
                                    {
                                        IbucketIGC* b2 = l2->data();
                                        if (b2->GetBuyable() == pstSuccessor)
                                        {
                                            b2->SetTimeAndMoney(b->GetTime(), money);
                                            break;
                                        }
                                    }
                                }

                                b->SetEmpty();
                            }
                        }
                        break;

                        default:
                        {
                            if ((b->GetMoney() != 0) && !CanBuy(pbuy))
                                b->SetEmpty();
                        }
                    }
                }
            }
        }

        ImissionIGC*        m_pMission;
        DWORD               m_dwPrivate; // private data for consumer
        DataSideIGC         m_data;

        IcivilizationIGC*   m_pCivilization;

        TechTreeBitMask     m_ttbmBuildingTechs;
        TechTreeBitMask     m_ttbmUltimateTechs;

        StationListIGC      m_stations;
        BucketListIGC       m_buckets;
        ShipListIGC         m_ships;
        ClusterListIGC      m_territoryClusters;

        StockpileList       m_stockpile;

        Time                m_lastUpdate;

        bool                m_activeF;		
		bool bRandomCivilization;  //Xynth #170 8/10
};

#endif //__SIDEIGC_H_
