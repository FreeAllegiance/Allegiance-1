/*
**
**  File:	mappvmaker.cpp
**
**  Author: KGJV
**
**  Description: map preview maker
**
**  History:
*/

#include    "pch.h"
#include    "mapmakerigc.h"
#include    "mappvmaker.h"

CwarpPV::CwarpPV(ImissionPV *pmission,DataWarpIGC *ds,int dataSize)
  : m_destination(NULL),
    m_pMission(pmission)
{
	sectorID = ds->clusterID;
	memcpy(&m_warpDef,&(ds->warpDef),sizeof(WarpDef));
}	

void    CwarpPV::Terminate(void)
{
    AddRef();
    GetMyMission()->DeleteWarp(this);
    if (m_destination)
    {
        m_destination->Release();
        m_destination = NULL;
    }
    Release();
}
CMapPVCluster * CwarpPV::GetCluster()
{
	return GetMyMission()->GetCluster(sectorID);
}
CwarpPV* CwarpPV::GetDestination()
{
            if (!m_destination)
            {
                //Do a lazy evaluation of the destination so that all warps do not
                //need to be defined before defining any warps
                m_destination = GetMyMission()->GetWarp(m_warpDef.destinationID);
                if (m_destination)
                    m_destination->AddRef();
            }
            return m_destination;
}
void  CwarpPV::SetCluster(CMapPVCluster* cluster)
{
    AddRef();

    {
        CMapPVCluster*    c = GetCluster();
        if (c)
            c->DeleteWarp(this);
    }

    if (cluster)
        cluster->AddWarp(this);

    Release();
}

ImissionPV::ImissionPV(Modeler *pmodeler) :
	m_pmodeler(pmodeler)
{

}
ImissionPV::~ImissionPV()
{
	ImissionPV::Clear();
}

void ImissionPV::Clear()
{
	m_pClusters.purge(true);
	m_pWarps.purge(true);
}
CwarpPV* ImissionPV::GetWarp(WarpID id)
{
	for (WarpPVLink*   l = GetWarps()->first();
		 (l != NULL);
		 l = l->next())
	{
		CwarpPV*   b = l->data();

		if (b->GetObjectID() == id)
			return b;
	}
    return NULL;		
}
CMapPVCluster* ImissionPV::GetCluster(SectorID id)
{
	for (ClusterPVLink*   l = GetClusters()->first();
		 (l != NULL);
		 l = l->next())
	{
		CMapPVCluster*   b = l->data();

		if (b->GetObjectID() == id)
			return b;
	}
    return NULL;		
}
void ImissionPV::DeleteWarp(CwarpPV* pwarp)
{
    for (WarpPVLink*   l = GetWarps()->first();
         (l != NULL);
         l = l->next())
    {
        if (l->data() == pwarp)
        {
            delete l;                //remove it from the list
            pwarp->Release();        //reduce the ref count
            break;                   //all done
        }
    }
}

CMapPVCluster* ImissionPV::CreatePVCluster(DataClusterIGC *ds,int dataSize)
{
	//debugf("CreatePVCluster: %f %f %d\n",(double)ds->screenX,(double)ds->screenY,dataSize);
	CMapPVCluster * cluster = new CMapPVCluster(this,ds,dataSize); //Fix memory leak -Imago 8/2/09
	ZVerify(m_pClusters.last(cluster));
    cluster->AddRef();
    cluster->AddRef();
	return cluster;

}
CwarpPV* ImissionPV::CreatePVWrap(DataWarpIGC *ds,int dataSize,CMapPVCluster* cluster)
{
	CwarpPV *warp = new CwarpPV(this,ds,dataSize); //Fix memory leak -Imago 8/2/09
	m_pWarps.last(warp);
	warp->AddRef();
	warp->AddRef();
	warp->SetCluster(cluster);
	return warp;
}


CMapPVCluster::CMapPVCluster(ImissionPV* pmission,DataClusterIGC *ds,int dataSize)
{
	m_pMission = pmission;
	assert(dataSize==sizeof(m_data));
	memcpy(&m_data,ds,dataSize);
	for (int i=0;i < c_cSidesMax; i++) nStationsPerSide[i] = 0;
}
CMapPVCluster::~CMapPVCluster()
{
	m_Warps.purge(false);
}
void CMapPVCluster::AddWarp(CwarpPV *warp)
{
	GetWarps()->last(warp);
    warp->AddRef();
}
void CMapPVCluster::DeleteWarp(CwarpPV* pwarp)
{
    for (WarpPVLink*   l = GetWarps()->first();
         (l != NULL);
         l = l->next())
    {
        if (l->data() == pwarp)
        {
            delete l;                //remove it from the list
            pwarp->Release();        //reduce the ref count
            break;                   //all done
        }
    }
}

CMapPVData::CMapPVData()
{
	mpMission = NULL;
    mpMissionParams = NULL;

	ZeroMemory(mpCluster, sizeof(mpCluster));

	mcTeamClustersPerTeam = 0;
    mcNeutralClustersPerTeam = 0;
    mcOtherClusters = 0;
    mIDNextWarp = 0;
}


CMapPVData::~CMapPVData()
{
    SectorID sID;

    for(sID = 0; sID < c_MaxClustersPerMap; sID++)
    {
        if (NULL != mpCluster[sID])
            mpCluster[sID]->Release();  
    }
}


VOID CMapPVData::SetCluster(SectorID sID, CMapPVCluster * pCluster, SideID sideID)
{
    if (NULL != pCluster)
        pCluster->AddRef();
    if (NULL != mpCluster[sID])
        mpCluster[sID]->Release();
    mpCluster[sID] = pCluster;
    msideIDCluster[sID] = sideID;
}



const char* ImapPVMaker::IsValid(const MissionParams* pmp)
{
    switch (pmp->mmMapType)
    {
        case c_mmSingleRing:
            return CmapPVMakerSingleRing::IsValid(pmp);
        case c_mmDoubleRing:
            return CmapPVMakerDoubleRing::IsValid(pmp);
        case c_mmPinWheel:
            return CmapPVMakerPinWheel::IsValid(pmp);
        case c_mmDiamondRing:
            return CmapPVMakerDiamondRing::IsValid(pmp);
        case c_mmSnowFlake:
            return CmapPVMakerSnowFlake::IsValid(pmp);
        case c_mmSplitBase:
            return CmapPVMakerSplitBase::IsValid(pmp);
        case c_mmLargeSplit:
            return CmapPVMakerLargeSplit::IsValid(pmp);
        case c_mmBrawl:
            return CmapPVMakerBrawl::IsValid(pmp);
        case c_mmBigRing:
            return CmapPVMakerBigRing::IsValid(pmp);
        case c_mmHiLo:
            return CmapPVMakerHiLo::IsValid(pmp);
        case c_mmHiHigher:
            return CmapPVMakerHiHigher::IsValid(pmp);
        case c_mmStar:
            return CmapPVMakerStar::IsValid(pmp);
        case c_mmInsideOut:
            return CmapPVMakerInsideOut::IsValid(pmp);
        case c_mmGrid:
            return CmapPVMakerGrid::IsValid(pmp);
        case c_mmEastWest:
            return CmapPVMakerEastWest::IsValid(pmp);
    }

    return "Invalid map type";
}

const char *  ImapPVMaker::Create(const MissionParams*   pmp,
							 ImissionPV*           pmission)
{
    assert (pmp->nTeams >= 2);
    assert (pmp->nTeams <= c_cSidesMax);

	if (pmp->szCustomMapFile[0] != '\0')
	{
		CmapPVMakerFromIGCFile mm;
		return mm.GenerateMission(pmp->szCustomMapFile,pmission);
	}
#define MakeMap(x)  case c_mm##x:                                   \
                    {                                               \
                        CmapPVMaker##x  mm;                         \
                        mm.GenerateMission(pmp, pmission);     \
                    }                                               \
                    break;                                          \

    switch(pmp->mmMapType)
    {
        MakeMap(SingleRing)
        MakeMap(DoubleRing)
        MakeMap(PinWheel)
        MakeMap(DiamondRing)
        MakeMap(SnowFlake)
        MakeMap(SplitBase)
        MakeMap(Brawl)
        MakeMap(BigRing)
        MakeMap(HiLo)
        MakeMap(HiHigher)
        MakeMap(Star)
        MakeMap(InsideOut)
        MakeMap(Grid)
        MakeMap(EastWest)
        MakeMap(LargeSplit)
        default:
            assert (false);
    }
#undef MakeMap
	return NULL;
}

//
// Base class for the map maker.
//
CmapPVMaker::CmapPVMaker()
{
    mMMID = c_mmSingleRing;
    this->SetName("Single Ring");
}


CmapPVMaker::~CmapPVMaker()
{
}

static bool    pvFindPath(CMapPVCluster*    p2,
                        int*            pStart,
                        int*            pStop,
                        CMapPVCluster**   pclustersAdjacent,
                        CMapPVCluster**   pclusterBackTrack,
                        bool*           bVisited)
{
    if (*pStart == *pStop)
        return false;

    CMapPVCluster*    p1 = pclustersAdjacent[*pStart];
    assert (p1);

    if (p1 == p2)
        return true;

    (*pStart)++;

    for (WarpPVLink*   pwl = p1->GetWarps()->first(); (pwl != NULL); pwl = pwl->next())
    {
        CMapPVCluster*    p = pwl->data()->GetDestination()->GetCluster();
        SectorID        id = p->GetObjectID();

        if ((pclusterBackTrack[id] == NULL) && !bVisited[id])
        {
            pclusterBackTrack[id] = p1;
            pclustersAdjacent[*pStop] = p;

            (*pStop)++;
        }
    }

    return pvFindPath(p2, pStart, pStop,
                    pclustersAdjacent,
                    pclusterBackTrack,
                    bVisited);
}

static bool    pvEnoughPaths(CMapPVCluster**            pclusterBackTrack,
                           bool*                    bVisited,
                           const ClusterPVList*    pclusters,
                           CwarpPV*                pwarp,
                           int                      count)
{
    CMapPVCluster*    pcluster1 = pwarp->GetCluster();
    CMapPVCluster*    pcluster2 = pwarp->GetDestination()->GetCluster();

    for (int i = pclusters->n() - 1; (i >= 0); i--)
    {
        pclusterBackTrack[i] = NULL;
    }

    CMapPVCluster**   pclustersAdjacent = (CMapPVCluster**)alloca(pclusters->n() * pclusters->n() * sizeof(CMapPVCluster*));
    int         nStart = 0;
    int         nStop = 0;
    for (WarpPVLink*   pwl = pcluster1->GetWarps()->first(); (pwl != NULL); pwl = pwl->next())
    {
        CwarpPV*   pw = pwl->data();
        if (pw != pwarp)
        {
            CMapPVCluster*    pcluster = pw->GetDestination()->GetCluster();
            SectorID        id = pcluster->GetObjectID();

            if (!bVisited[id])
            {
                pclustersAdjacent[nStop++] = pcluster;
                pclusterBackTrack[id] = pcluster1;
            }
        }
    }

    if (!pvFindPath(pcluster2, &nStart, &nStop, pclustersAdjacent, pclusterBackTrack, bVisited))
    {
        return false;
    }
    else if (count == 1)
    {
        return true;
    }
    else
    {
        //Mark everything on the path from p1 to p2 as visited so they can be used
        //by subsequent paths
        //Note: p2 is never marked as visited
        //      p1 is marked as visited (not that we care)
        CMapPVCluster*    pcluster = pcluster2;
        SectorID    id = pcluster->GetObjectID();
        do
        {
            assert (pclusterBackTrack[id] != NULL);

            pcluster = pclusterBackTrack[id];
            id = pcluster->GetObjectID();

            bVisited[id] = true;
        }
        while (pcluster != pcluster1);

        return pvEnoughPaths(pclusterBackTrack,
                           bVisited,
                           pclusters,
                           pwarp,
                           --count);
    }
}

static bool pvOkToRemove(const ClusterPVList*    pclusters, CwarpPV* pwarp)
{
    CMapPVCluster**   pclusterBackTrack = (CMapPVCluster**)alloca(pclusters->n() * sizeof(CMapPVCluster*));
    bool*           bVisited = (bool*)alloca(pclusters->n() * sizeof(bool));

    {
        for (int i = pclusters->n() - 1; (i >= 0); i--)
            bVisited[i] = false;
    }

    bVisited[pwarp->GetCluster()->GetObjectID()] = true;

    return pvEnoughPaths(pclusterBackTrack, bVisited, pclusters, pwarp, 2);
}

VOID CmapPVMaker::GenerateMission(const MissionParams * pmp,
                                    ImissionPV * pMission)
{
    CMapPVData MapData;

    assert(NULL == IsValid(pmp));


	MapData.SetMission(pMission);
    MapData.SetMissionParams(pmp);

    //
    // Next generate the sector layout.
    //
    this->GenerateLayout(&MapData);

    {
        //Adjust the position of all alephs in all clusters
        float   majorRadius = 5000.0f;
        for (ClusterPVLink*    pcl = pMission->GetClusters()->first(); (pcl != NULL); pcl = pcl->next())
        {
            CMapPVCluster*    pcluster = pcl->data();

            const WarpPVList*  pwarps = pcluster->GetWarps();
            if (pwarps->n() != 0)
            {
                float   nWarps = (float)(pwarps->n());

                const int c_maxWarps = 10;
                assert (pwarps->n() <= c_maxWarps);

                float   offset[c_maxWarps] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
                float   displacement;
                if (pwarps->n() > 1)
                {
                    displacement =  sin(pi/nWarps) * 2.0f / 3.0f;
                    for (int index = 0; (index < pwarps->n()); index++)
                    {
                        int swap = randomInt(0, pwarps->n() - 1);
                        if (swap != index)
                        {
                            float   t = offset[index];
                            offset[index] = offset[swap];
                            offset[swap] = t;
                        }
                    }
                }
                else
                    displacement = 2.0f / 3.0f;

            }
        }
    }

    //Randomize the map if appropriate (connectivity) - removed for now
 //   if (pmp->iRandomEncounters > 1)
	//{
 //       const WarpPVList*      pwarpList = pMission->GetWarps();
 //       CwarpPV**  pwarps = (CwarpPV**)(alloca(pwarpList->n() * sizeof(CwarpPV*)));

 //       //Actually allocate twice as much space as we need since we are only storing a single instance for
 //       //both sides of the aleph
 //       int nWarps = 0;
 //       {
 //           for (WarpPVLink*   pwl = pwarpList->first(); (pwl != NULL); pwl = pwl->next())
 //           {
 //               CwarpPV*   pwarp = pwl->data();
 //               if ((pwarp->GetObjectID() < pwarp->GetDestination()->GetObjectID()) &&
 //                   (randomInt(0, pmp->iRandomEncounters) > 1))
 //               {
 //                   assert (nWarps < pwarpList->n());
 //                   pwarps[nWarps++] = pwarp;
 //               }
 //           }
 //       }

 //       const ClusterPVList*   pclusters = pMission->GetClusters();
 //       while (nWarps > 0)
 //       {
 //           int         index = randomInt(0, nWarps - 1);
 //           CwarpPV*   pwarp = pwarps[index];

 //           //Can we remove pwarp without destroying the map's connectivity?
 //           if (pvOkToRemove(pclusters, pwarp))
 //           {
 //               pwarp->GetDestination()->Terminate();
 //               pwarp->Terminate();
 //           }

 //           pwarps[index] = pwarps[--nWarps];
 //       }
 //   }

	// set Stations per side
	if (mMMID == c_mmBrawl)
	{
		for (SideID sideID = 0; (sideID < MapData.GetTeams()); sideID++)
		{
			pMission->GetClusters()->first()->data()->GetStationsPerSide()[sideID] = 1;
		}
	}
	else
    for (ClusterPVLink*    pcl = pMission->GetClusters()->first(); (pcl != NULL); pcl = pcl->next())
    {
        CMapPVCluster*    pcluster = pcl->data();
        if (pcluster->GetHomeSector())
		{
			SideID  sideID = MapData.GetClusterSide(pcluster->GetObjectID());
            pcluster->GetStationsPerSide()[sideID] = 1;
		}

    }	
}


VOID CmapPVMaker::GenerateTeamClusterScreenPosition(CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flTheta;

    flTheta = (float) (2.0 * pi * sID / pMapData->GetTotalClusters());

    pdc->screenX = cos(flTheta) * 0.8f;
    pdc->screenY = sin(flTheta) * 0.8f;
}


CMapPVCluster * CmapPVMaker::GenerateTeamCluster(CMapPVData * pMapData,
                                                SectorID sID)
{
    DataClusterIGC dc;
    INT n; 

    dc.bHomeSector = true;

    dc.clusterID = sID;
    dc.activeF = true;

    dc.lightColor = 0xffffffff;
    dc.lightDirection = Vector::RandomDirection();

    n = randomInt(0, c_NumberOfPlanetPosters - 1);
    dc.planetSinLatitude = random(0.0f, 1.0f);
    dc.planetLongitude = random(0.0f, 1.0f);
    dc.planetRadius = (unsigned char)randomInt(50, 100);

    dc.starSeed = rand();
    dc.nDebris = randomInt(250, 750);
    dc.nStars = randomInt(250, 750);

    //
    // Position cluster on the screen.
    //
    this->GenerateTeamClusterScreenPosition(pMapData, &dc, sID);

    CMapPVCluster * o = (CMapPVCluster *) pMapData->GetMission()->CreatePVCluster(
		                                            &dc,
                                                    sizeof(dc));
    assert(o);
    return(o);
}


VOID CmapPVMaker::GenerateNeutralClusterScreenPosition(CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flTheta;

    flTheta = (float) (2.0 * pi * sID / pMapData->GetTotalClusters());
    pdc->screenX = cos(flTheta) * 0.4f;
    pdc->screenY = sin(flTheta) * 0.4f;
}


CMapPVCluster * CmapPVMaker::GenerateNeutralCluster(CMapPVData * pMapData,
                                                    SectorID sID)
{
    DataClusterIGC dc;

    dc.bHomeSector = false;

    dc.clusterID = sID;
    dc.activeF = true;

    dc.lightColor = 0xffffffff;
    dc.lightDirection = Vector::RandomDirection();

    strcpy(dc.posterName, "globe");
    dc.posterName[5] = '0' + randomInt(1, 8);
    dc.posterName[6] = '\0';
    dc.planetName[0] = '\0';

    dc.starSeed = rand();
    dc.nDebris = randomInt(250, 750);
    dc.nStars = randomInt(250, 750);

    //
    // Position cluster on the screen.  Since we're doing all the
    // player clusters first, we need to space them appropriately.
    //
    this->GenerateNeutralClusterScreenPosition(pMapData, &dc, sID);

    CMapPVCluster * o = (CMapPVCluster *) pMapData->GetMission()->CreatePVCluster(
                                                    &dc,
                                                    sizeof(dc));
    assert(o);
    return(o);
}


VOID CmapPVMaker::GenerateRingClusters(CMapPVData * pMapData)
{
    SectorID sID, cTotalRingClusters;
    CMapPVCluster * pCluster;

    cTotalRingClusters = pMapData->GetTotalClusters() -
                            pMapData->GetOtherClusters();

    for(sID = 0; sID < cTotalRingClusters; sID++)
    {
        if (sID % pMapData->GetClustersPerTeam() <
            pMapData->GetTeamClustersPerTeam())
        {
            pCluster = this->GenerateTeamCluster(pMapData, sID);
            pMapData->SetCluster(sID,
                                pCluster,
                                sID / pMapData->GetClustersPerTeam());
        } else
        {
            pCluster = this->GenerateNeutralCluster(pMapData, sID);
            pMapData->SetCluster(sID, pCluster, NA);
        }
        pCluster->Release();
    }

    while(sID < pMapData->GetTotalClusters())
    {
        pCluster = this->GenerateNeutralCluster(pMapData, sID);
        pMapData->SetCluster(sID, pCluster, NA);
        sID++;
    }
}


VOID CmapPVMaker::GenerateWarp(CMapPVData * pMapData,
                                CMapPVCluster * pCluster,
                                DataWarpIGC * pdw)
{
    //
    // Add it to the cluster.
    //
    pdw->forward.x = pdw->forward.y = 0.0f;
    pdw->forward.z = 1.0f;

    pdw->rotation.axis(pdw->forward);
    pdw->rotation.angle(0.0f);

    pdw->warpDef.radius = 100;

    IObject * o = pMapData->GetMission()->CreatePVWrap(pdw,sizeof(*pdw),pCluster);
    assert(o);
    o->Release();
}


VOID CmapPVMaker::LinkClusters(CMapPVData* pMapData,
                                SectorID  sID1,
                                SectorID  sID2,
                                int       alephType)
{
    DataWarpIGC dw;

    static const char*  c_pszIconNeutral = "neutralaleph";
    static const char*  c_pszIconPlayer = "homealeph";

    static const char*  c_pszTextureNeutral = "plnt19";
    static const char*  c_pszTexturePlayer = "plnt42";

    float signature1 = 1.0f;
    float signature2 = 1.0f;

    float radius1 = 0.6f;
    float radius2 = 0.6f;

    if (alephType == NA)
    {
        SideID  side1 = pMapData->GetClusterSide(sID1);
        SideID  side2 = pMapData->GetClusterSide(sID2);
        if (NA == side1)
        {
            if (NA == side2)
            {
                alephType = c_NeutralAleph;
            }
            else
            {
                alephType = c_NeutralToPlayerAleph;
            }
        }
        else if (side1 == side2)
        {
            alephType = c_FriendlyAleph;
        }
        else if (NA != side2)
        {
            alephType = c_EnemyAleph;
        }
        else
        {
            alephType = c_PlayerToNeutralAleph;
        }
    }

    const char* pszTexture1;
    const char* pszTexture2;

    const char* pszIcon1;
    const char* pszIcon2;
    switch (alephType)
    {
        case c_FriendlyAleph:
            pszIcon1 = pszIcon2 = c_pszIconPlayer;
            pszTexture1 = pszTexture2 = c_pszTexturePlayer;
        break;

        case c_EnemyAleph:
            pszIcon1 = pszIcon2 = c_pszIconPlayer;
            pszTexture1 = pszTexture2 = c_pszTexturePlayer;
            signature1 = signature2 = 0.33f;
            radius1 = radius2 = 0.85f;
        break;

        case c_NeutralToPlayerAleph:
            pszIcon1 = c_pszIconPlayer;
            pszTexture1 = c_pszTexturePlayer;
            signature1 = 0.67f;

            pszIcon2 = c_pszIconNeutral;
            pszTexture2 = c_pszTextureNeutral;
        break;

        case c_PlayerToNeutralAleph:
            pszIcon2 = c_pszIconPlayer;
            pszTexture2 = c_pszTexturePlayer;
            signature2 = 0.67f;

            pszIcon1 = c_pszIconNeutral;
            pszTexture1 = c_pszTextureNeutral;
        break;

        case c_NeutralAleph:
            pszIcon1 = pszIcon2 = c_pszIconNeutral;
            pszTexture1 = pszTexture2 = c_pszTextureNeutral;
        break;

        case c_HiddenToPlayerAleph:
            pszIcon2 = c_pszIconPlayer;
            pszTexture2 = c_pszTexturePlayer;
            signature2 = 0.33f;
            radius2 = 0.85f;

            pszIcon1 = c_pszIconNeutral;
            pszTexture1 = c_pszTextureNeutral;
        break;

        default:
            assert (false);
    }

    //
    // Build the alephs.
    //
    dw.warpDef.warpID = pMapData->GetNextWarpID();
    dw.clusterID = sID1;

    dw.signature = signature1;
    dw.position.x = dw.position.y = 0.0f;
    dw.position.z = radius1;
    dw.warpDef.destinationID = dw.warpDef.warpID + 1;

    this->GenerateWarp(pMapData, pMapData->GetCluster(sID1), &dw);

    dw.warpDef.warpID = pMapData->GetNextWarpID();
    dw.clusterID = sID2;
    dw.signature = signature2;
    dw.position.x = dw.position.y = 0.0f;
    dw.position.z = radius2;
    dw.warpDef.destinationID = dw.warpDef.warpID - 1;

    this->GenerateWarp(pMapData, pMapData->GetCluster(sID2), &dw);
}


VOID CmapPVMaker::CrossLinkClusters(CMapPVData * pMapData, SectorID sFirstID)
{
    SectorID sMax, sWrap;

    sMax = pMapData->GetTotalClusters();
    if (pMapData->GetTeams() < 4)
        sMax = 0;
    else if (pMapData->GetTeams() == 4)
        sMax /= 2;

    while(sFirstID < sMax)
    {
        sWrap = (sFirstID + 2 * pMapData->GetClustersPerTeam()) %
                    pMapData->GetTotalClusters();
        LinkClusters(pMapData, sFirstID, sWrap);
        sFirstID += pMapData->GetClustersPerTeam();
    }
}


VOID CmapPVMaker::LinkClusters(CMapPVData * pMapData)
{
    SectorID sID, cTotalSectors;

    //
    // Default implementation is a single ring.
    //
    cTotalSectors = pMapData->GetTotalClusters();

    for(sID = 0; sID < cTotalSectors - 1; sID++)
        this->LinkClusters(pMapData, sID, sID + 1);

    this->LinkClusters(pMapData, sID, 0);

    //
    // If more than 3 teams, link the neutral sectors.
    //
    this->CrossLinkClusters(pMapData, pMapData->GetTeamClustersPerTeam());
}


VOID CmapPVMaker::GenerateLayout(CMapPVData * pMapData)
{
    //SectorID    s = pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1;
	//KGJV-: changed, use now MapSize parameter
    SectorID    s = pMapData->GetMapSize() != 0 ? 2 : 1;
    pMapData->SetTeamClustersPerTeam(s);
    pMapData->SetNeutralClustersPerTeam(1);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}



//
// Double ring implementation.
//
CmapPVMakerDoubleRing::CmapPVMakerDoubleRing()
{
    mMMID = c_mmDoubleRing;
    this->SetName("Double Ring");
}

VOID CmapPVMakerDoubleRing::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float   flTheta = (float) (2.0 * pi * sID / pMapData->GetTotalClusters());
    pdc->screenX = cos(flTheta) * 0.9f;
    pdc->screenY = sin(flTheta) * 0.9f;
}


VOID CmapPVMakerDoubleRing::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float   flTheta = (float) (2.0 * pi * sID / pMapData->GetTotalClusters());
    float   flRadius = (pMapData->GetTeams() == 2) && (pMapData->GetTeamClustersPerTeam() == 2)
                       ? 0.2f
                       : 0.3f;
    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}


VOID CmapPVMakerDoubleRing::LinkClusters(CMapPVData * pMapData)
{
    SectorID sID, cTotalSectors, sWrapID, sIndex;

    //
    // Link each team's group of sectors.
    //
    cTotalSectors = pMapData->GetTotalClusters();
    for(sID = 0; sID < cTotalSectors; sID += pMapData->GetClustersPerTeam())
    {
        sWrapID = (sID + cTotalSectors - 1) % cTotalSectors;

        for(sIndex = 0; sIndex < pMapData->GetTeamClustersPerTeam(); sIndex++)
        {
            CmapPVMaker::LinkClusters(pMapData,
                                    sID + sIndex,
                                    sID + pMapData->GetTeamClustersPerTeam());
            CmapPVMaker::LinkClusters(pMapData,
                                    sID + sIndex,
                                    sWrapID);
        }
    }

    //
    // Link the ring around the outside.
    //
    sWrapID = cTotalSectors - 1 - pMapData->GetNeutralClustersPerTeam();
    for(sID = 0; sID < cTotalSectors; sID++)
    {
        if (NA != pMapData->GetClusterSide(sID))
        {
            CmapPVMaker::LinkClusters(pMapData, sID, sWrapID);
            sWrapID = sID;
        }
    }

    //
    // Link the ring around the inside.
    //
    sWrapID = cTotalSectors - 1;
    for(sID = 0; sID < cTotalSectors; sID++)
    {
        if (NA == pMapData->GetClusterSide(sID))
        {
            CmapPVMaker::LinkClusters(pMapData, sID, sWrapID);
            sWrapID = sID;
        }
    }
}

//
// Big ring implementation.
//
CmapPVMakerBigRing::CmapPVMakerBigRing()
{
    mMMID = c_mmBigRing;
    this->SetName("Big Ring");
}

VOID CmapPVMakerBigRing::GenerateLayout(CMapPVData * pMapData)
{
    //SectorID    s = pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1;
	//KGJV-: changed, use now MapSize parameter
    SectorID    s = pMapData->GetMapSize() != 0 ? 2 : 1;
    pMapData->SetTeamClustersPerTeam(s);
    pMapData->SetNeutralClustersPerTeam(2);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}

VOID CmapPVMakerBigRing::GenerateTeamClusterScreenPosition(CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID    s = (pMapData->GetTeamClustersPerTeam() == 1) ? (sID / 3) : ((sID + 1) / 2);
    SectorID    n = pMapData->GetTeamClusters();
    float flTheta = (float) (2.0 * pi * s / n);

    pdc->screenX = cos(flTheta) * 0.9f;
    pdc->screenY = sin(flTheta) * 0.9f;
}
VOID CmapPVMakerBigRing::GenerateNeutralClusterScreenPosition(CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float       flTheta;

    SectorID    n = pMapData->GetNeutralClusters();
    if (pMapData->GetTeamClustersPerTeam() == 1)
    {
        SectorID    s = (sID - 1 - sID/3);
        flTheta = (float) (2.0 * pi * (s + 0.5f) / n);
    }
    else
    {
        SectorID    s =  ((sID + 1) / 2);
        flTheta = (float) (2.0 * pi * s / n);
    }

    pdc->screenX = cos(flTheta) * 0.6f;
    pdc->screenY = sin(flTheta) * 0.6f;
}

VOID CmapPVMakerBigRing::LinkClusters(CMapPVData * pMapData)
{
    //
    // Link the ring
    //
    SectorID    cTotalSectors = pMapData->GetTotalClusters();
    {
        SectorID    sWrapID = cTotalSectors - 1;
        for(SectorID    sID = 0; sID < cTotalSectors; sID++)
        {
            CmapPVMaker::LinkClusters(pMapData, sID, sWrapID);
            sWrapID = sID;
        }
    }


    if (pMapData->GetTeams() > 2)
    {
        SectorID    n = pMapData->GetTeamClustersPerTeam();
        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sID = i * pMapData->GetClustersPerTeam();
            CmapPVMaker::LinkClusters(pMapData, sID + n, (sID + 3 + 2*n) % cTotalSectors);
            if (pMapData->GetTeams() > 3)
                CmapPVMaker::LinkClusters(pMapData, sID + n + 1, (cTotalSectors + (sID - 2)) % cTotalSectors);
        }
    }
}
//
// HiLo implementation.
//
CmapPVMakerHiLo::CmapPVMakerHiLo()
{
    mMMID = c_mmHiLo;
    this->SetName("HiLo");
}

VOID CmapPVMakerHiLo::GenerateLayout(CMapPVData * pMapData)
{
    //pMapData->SetTeamClustersPerTeam(pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1);
	//KGJV-: changed, use now MapSize parameter
    pMapData->SetTeamClustersPerTeam(pMapData->GetMapSize() != 0 ? 2 : 1);
                                    
    pMapData->SetNeutralClustersPerTeam(3);

    pMapData->SetOtherClusters(1);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}

VOID CmapPVMakerHiLo::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flTheta, flRadius;
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    sID = sID % pMapData->GetClustersPerTeam();

    flTheta = (float) (2.0f * pi * float(sTeam) / float(pMapData->GetTeams()));
    flRadius = 0.5f + 0.4f * sID;

    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}

VOID CmapPVMakerHiLo::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    if (sTeam < pMapData->GetTeams())
    {
        sID = (sID % pMapData->GetClustersPerTeam()) - pMapData->GetTeamClustersPerTeam();

        float flTheta1 = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
        float flTheta2 = (float) (2.0 * pi * (sTeam + 1) / pMapData->GetTeams());

        float   flTheta;
        float   flRadius;
        switch (sID)
        {
            case 0:
                flTheta = (flTheta1 + flTheta2) / 2.0f;
                flRadius = 0.4f;
            break;
            case 1:
                flTheta = (2.0f * flTheta1 + flTheta2) / 3.0f;
                flRadius = 0.8f;
            break;
            case 2:
                flTheta = (flTheta1 + 2.0f * flTheta2) / 3.0f;
                flRadius = 0.8f;
            break;
        }

        pdc->screenX = cos(flTheta) * flRadius;
        pdc->screenY = sin(flTheta) * flRadius;
    }
    else
        pdc->screenX = pdc->screenY = 0.0f;
}

VOID CmapPVMakerHiLo::LinkClusters(CMapPVData * pMapData)
{
    if (pMapData->GetTeamClustersPerTeam() != 1)
    {
        //Link the split base clusters for each team
        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sID = i * pMapData->GetClustersPerTeam();
            CmapPVMaker::LinkClusters(pMapData, sID, sID + 1);
        }
    }

    {
        SectorID    sidCenter = pMapData->GetTotalClusters() - 1;

        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sID = i * pMapData->GetClustersPerTeam() +
                              pMapData->GetTeamClustersPerTeam();

            //Inner ring to center
            CmapPVMaker::LinkClusters(pMapData, sID, sidCenter);

            //Inner ring to inner previous team cluster
            SectorID    sidPrevious = i * pMapData->GetClustersPerTeam();
            CmapPVMaker::LinkClusters(pMapData, sID, sidPrevious);

            //Inner ring to inner next team cluster
            SectorID    sidNext = ((i+1)%pMapData->GetTeams()) * pMapData->GetClustersPerTeam();
            CmapPVMaker::LinkClusters(pMapData, sID, sidNext);

            //Outer ring 1 to outer ring 2
            CmapPVMaker::LinkClusters(pMapData, sID + 1, sID + 2);

            //Outer ring 1 to previous team
            CmapPVMaker::LinkClusters(pMapData, sID + 1, sidPrevious + pMapData->GetTeamClustersPerTeam() - 1);

            //Outer ring 2 to previous team
            CmapPVMaker::LinkClusters(pMapData, sID + 2, sidNext + pMapData->GetTeamClustersPerTeam() - 1);
        }
    }
}

//
// HiHigher implementation.
//
CmapPVMakerHiHigher::CmapPVMakerHiHigher()
{
    mMMID = c_mmHiHigher;
    this->SetName("HiHigher");
}

VOID CmapPVMakerHiHigher::GenerateLayout(CMapPVData * pMapData)
{
    pMapData->SetTeamClustersPerTeam((mMMID == c_mmHiHigher)
                                     //? (pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1)
									 //KGJV-: changed, use now MapSize parameter
									 ? (pMapData->GetMapSize() != 0 ? 2 : 1)
                                     : ((mMMID == c_mmLargeSplit)
                                        ? 2
                                        : 1));
                                    
    pMapData->SetNeutralClustersPerTeam(5);

    pMapData->SetOtherClusters(1);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}

VOID CmapPVMakerHiHigher::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam = sID / pMapData->GetClustersPerTeam();
    sID = (sID % pMapData->GetClustersPerTeam());

    float flTheta;
    float flRadius;
    if (mMMID == c_mmInsideOut)
    {
        flTheta = (float) (2.0 * pi * (sTeam + 0.5f) / pMapData->GetTeams());
        flRadius = 0.3f;
    }
    else
    {
        flTheta = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
        flRadius = (pMapData->GetTeamClustersPerTeam() == 1) ? 1.0f : (0.75f + 0.25f * sID);
    }

    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}

VOID CmapPVMakerHiHigher::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    if (sTeam < pMapData->GetTeams())
    {
        sID = (sID % pMapData->GetClustersPerTeam()) - pMapData->GetTeamClustersPerTeam();

        float flTheta1 = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
        float flTheta2 = (float) (2.0 * pi * (sTeam + 1) / pMapData->GetTeams());

        float   flTheta;
        float   flRadius;
        switch (sID)
        {
            case 0:
                flTheta = (3.0f * flTheta1 + flTheta2) / 4.0f;
                flRadius = 0.6f;
            break;
            case 1:
                if (mMMID == c_mmInsideOut)
                {
                    flTheta = flTheta1;
                    flRadius = 1.0f;
                }
                else
                {
                    flTheta = (flTheta1 + flTheta2) / 2.0f;
                    flRadius = 0.3f;
                }
            break;
            case 2:
                flTheta = (flTheta1 + 3.0f * flTheta2) / 4.0f;
                flRadius = 0.6f;
            break;
            case 3:
                flTheta = (2.0f * flTheta1 + flTheta2) / 3.0f;
                flRadius = 1.0f;
            break;
            case 4:
                flTheta = (flTheta1 + 2.0f * flTheta2) / 3.0f;
                flRadius = 1.0f;
            break;
        }

        pdc->screenX = cos(flTheta) * flRadius;
        pdc->screenY = sin(flTheta) * flRadius;
    }
    else
        pdc->screenX = pdc->screenY = 0.0f;
}

VOID CmapPVMakerHiHigher::LinkClusters(CMapPVData * pMapData)
{
    if ((pMapData->GetTeamClustersPerTeam() != 1) && (mMMID != c_mmLargeSplit))
    {
        //Link the split base clusters for each team
        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sID = i * pMapData->GetClustersPerTeam();
            CmapPVMaker::LinkClusters(pMapData, sID, sID + 1);
        }
    }

    {
        SectorID    sidCenter = pMapData->GetTotalClusters() - 1;

        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sIDp = i * pMapData->GetClustersPerTeam();
            SectorID    sID0 = sIDp +
                               pMapData->GetTeamClustersPerTeam();

            SectorID    sID1 = sID0 + 1;
            SectorID    sID2 = sID0 + 2;
            SectorID    sID3 = sID0 + 3;
            SectorID    sID4 = sID0 + 4;

            SectorID    sIDn = ((i+1)%pMapData->GetTeams()) * pMapData->GetClustersPerTeam();

            if (mMMID == c_mmInsideOut)
            {
                assert (pMapData->GetTeamClustersPerTeam() == 1);
                sID1 = sIDp;
                sIDp = sID0 + 1;
                sIDn = ((i+1)%pMapData->GetTeams()) * pMapData->GetClustersPerTeam() + 1 + 1;
            }

            //Inner ring to center
            CmapPVMaker::LinkClusters(pMapData, sID1, sidCenter,
                                       (mMMID == c_mmInsideOut) ? c_HiddenToPlayerAleph : NA);

            CmapPVMaker::LinkClusters(pMapData, sID0, sID1);
            CmapPVMaker::LinkClusters(pMapData, sID1, sID2);

            CmapPVMaker::LinkClusters(pMapData, sID0, sID3);
            CmapPVMaker::LinkClusters(pMapData, sID2, sID4);

            CmapPVMaker::LinkClusters(pMapData, sID3, sID4);

            CmapPVMaker::LinkClusters(pMapData, sID0, sIDp);
            CmapPVMaker::LinkClusters(pMapData, sID2, sIDn);

            CmapPVMaker::LinkClusters(pMapData, sID3, sIDp + pMapData->GetTeamClustersPerTeam() - 1);
            CmapPVMaker::LinkClusters(pMapData, sID4, sIDn + pMapData->GetTeamClustersPerTeam() - 1);

            if (pMapData->GetMissionParams()->iRandomEncounters != 0)
                CmapPVMaker::LinkClusters(pMapData, sID2, 
                                           (mMMID == c_mmInsideOut)
                                           ? (sIDn - 1)
                                           : (sIDn + pMapData->GetTeamClustersPerTeam()));
        }
    }
}

//
// Star implementation.
//
CmapPVMakerStar::CmapPVMakerStar()
{
    mMMID = c_mmStar;
    this->SetName("Star");
}

VOID CmapPVMakerStar::GenerateLayout(CMapPVData * pMapData)
{
    //pMapData->SetTeamClustersPerTeam(pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1);
	//KGJV-: changed, use now MapSize parameter
    pMapData->SetTeamClustersPerTeam(pMapData->GetMapSize() != 0 ? 2 : 1);
                                    
    pMapData->SetNeutralClustersPerTeam(5);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}

VOID CmapPVMakerStar::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam = sID / pMapData->GetClustersPerTeam();
    sID = sID % pMapData->GetClustersPerTeam();

    float   flDelta = (2.0f * pi / float(pMapData->GetTeams()));
    float   flTheta = flDelta * float(sTeam);

    if (pMapData->GetTeamClustersPerTeam() == 2)
        flTheta += flDelta * (float(sID) - 0.5f) * 0.5f;

    pdc->screenX = cos(flTheta);
    pdc->screenY = sin(flTheta);
}

VOID CmapPVMakerStar::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam = sID / pMapData->GetClustersPerTeam();
    sID = sID % pMapData->GetClustersPerTeam() - pMapData->GetTeamClustersPerTeam();

    float   flRadius;
    float   flDelta = (2.0f * pi / float(pMapData->GetTeams()));
    float   flTheta = flDelta * float(sTeam);
    switch (sID)
    {
        case 0:
        case 1:
        {
            flRadius = 0.7f;
            flTheta += flDelta * (float(sID) - 0.5f) * 0.5f;
        }
        break;
        case 3:
        {
            flTheta += flDelta * 0.5f;
        }
        //no break
        case 2:
        {
            flRadius = 0.5f;
        }
        break;

        case 4:
        {
            flRadius = 0.2f;
        }
        break;
    }

    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}

VOID CmapPVMakerStar::LinkClusters(CMapPVData * pMapData)
{
    {
        SectorID    sidCenter = pMapData->GetTotalClusters() - 1;

        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sID = i * pMapData->GetClustersPerTeam() +
                              pMapData->GetTeamClustersPerTeam();

            //Inner starting sectors to neutral sectors
            if (pMapData->GetTeamClustersPerTeam() == 1)
            {
                CmapPVMaker::LinkClusters(pMapData, sID,     sID - 1);
                CmapPVMaker::LinkClusters(pMapData, sID + 1, sID - 1);
            }
            else
            {
                CmapPVMaker::LinkClusters(pMapData, sID - 1, sID - 2);
                CmapPVMaker::LinkClusters(pMapData, sID,     sID - 2);
                CmapPVMaker::LinkClusters(pMapData, sID + 1, sID - 1);
            }

            CmapPVMaker::LinkClusters(pMapData, sID, sID + 2);
            CmapPVMaker::LinkClusters(pMapData, sID + 1, sID + 2);
            CmapPVMaker::LinkClusters(pMapData, sID + 1, sID + 3);

            CmapPVMaker::LinkClusters(pMapData, sID + 2, sID + 3);
            CmapPVMaker::LinkClusters(pMapData, sID + 2, sID + 4);

            {
                SectorID    s = (sID + pMapData->GetTotalClusters() - pMapData->GetClustersPerTeam()) % pMapData->GetTotalClusters();
                CmapPVMaker::LinkClusters(pMapData, s + 3, sID);
                CmapPVMaker::LinkClusters(pMapData, s + 3, sID + 2);
            }

            {
                SideID      j = (i + pMapData->GetTeams()/2) % pMapData->GetTeams();
                SectorID    s = j * pMapData->GetClustersPerTeam() +
                                pMapData->GetTeamClustersPerTeam();
                if (pMapData->GetTeams() % 2 == 0)
                {
                    CmapPVMaker::LinkClusters(pMapData, sID + 4, s + 4);
                }
                else
                {
                    CmapPVMaker::LinkClusters(pMapData, sID + 4, s + 4);
                    CmapPVMaker::LinkClusters(pMapData, sID + 4, (s + 4 + pMapData->GetClustersPerTeam()) % pMapData->GetTotalClusters());
                }
            }
        }
    }
}

//
// Brawl
//
CmapPVMakerBrawl::CmapPVMakerBrawl()
{
    mMMID = c_mmBrawl;
    this->SetName("Brawl");
}


const char* CmapPVMakerBrawl::IsValid(const MissionParams * pmp)
{
	// yp your_persona removed march 24 2006
    //if (pmp->nMaxPlayersPerTeam * pmp->nTeams > 36)
    //    return("Brawl maps must have less than 37 players; "
    //            "please change the map type, number of players per team, "
    //            "or number of teams.");

    return(NULL);
}


VOID CmapPVMakerBrawl::GenerateLayout(CMapPVData * pMapData)
{
    pMapData->SetTeamClustersPerTeam(0);

    pMapData->SetOtherClusters(1);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}


VOID CmapPVMakerBrawl::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    pdc->screenX = 0.0f;
    pdc->screenY = 0.0f;
}


VOID CmapPVMakerBrawl::LinkClusters(CMapPVData * pMapData)
{
}


//
// PinWheel
//
CmapPVMakerPinWheel::CmapPVMakerPinWheel()
{
    mMMID = c_mmPinWheel;
    this->SetName("PinWheel");
}


const char* CmapPVMakerPinWheel::IsValid(const MissionParams * pmp)
{
	// yp your_persona removed march 24 2006
    //if (pmp->nMaxPlayersPerTeam > 200)
    //  return("Pinwheel maps must have less than 201 player per team; "
    //"please change the map type, or number of players per team.");
	//
    return(NULL);
}


VOID CmapPVMakerPinWheel::GenerateLayout(CMapPVData * pMapData)
{
    pMapData->SetTeamClustersPerTeam(1);

	//if (pMapData->GetMaxPlayersInMission() <= 30)
    //KGJV-: changed, use now MapSize parameter
    if (pMapData->GetMapSize() == 0)
        pMapData->SetOtherClusters(1);
    else
        pMapData->SetNeutralClustersPerTeam(1);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}


VOID CmapPVMakerPinWheel::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flTheta;

    if (0 != pMapData->GetNeutralClustersPerTeam())
    {
        sID = sID / pMapData->GetClustersPerTeam();
        flTheta = (float) (2.0 * pi * sID / pMapData->GetNeutralClusters());
        pdc->screenX = cos(flTheta) * 0.4f;
        pdc->screenY = sin(flTheta) * 0.4f;
    } else
        pdc->screenX = pdc->screenY = 0.0f;
}


VOID CmapPVMakerPinWheel::LinkClusters(CMapPVData * pMapData)
{
    SectorID sID, cTotalClusters, sWrapID, sFirstNA;

    cTotalClusters = pMapData->GetTotalClusters();
    if (0 != pMapData->GetNeutralClustersPerTeam())
    {
        //
        // Multiple sectors in the center.
        //
        sWrapID = NA;
        for(sID = 0; sID < cTotalClusters; sID++)
        {
            if (NA != pMapData->GetClusterSide(sID))
                CmapPVMaker::LinkClusters(pMapData, sID, sID + 1);
            else
            {
                if (NA != sWrapID)
                    CmapPVMaker::LinkClusters(pMapData, sID, sWrapID);
                else
                    sFirstNA = sID;
                sWrapID = sID;
            }
        }

        if (pMapData->GetTeams() > 2)
            CmapPVMaker::LinkClusters(pMapData, sWrapID, sFirstNA);
        this->CrossLinkClusters(pMapData, pMapData->GetTeamClustersPerTeam());
    }
    else
    {
        //
        // Single sector in the middle.
        //
        for(sID = 0; sID < cTotalClusters; sID++)
        {
            if (NA != pMapData->GetClusterSide(sID))
                CmapPVMaker::LinkClusters(pMapData, sID, cTotalClusters - 1);
        }
    }
}

//
// Diamond ring.
//
CmapPVMakerDiamondRing::CmapPVMakerDiamondRing()
{
    mMMID = c_mmDiamondRing;
    this->SetName("Diamond Ring");
}


const char* CmapPVMakerDiamondRing::IsValid(const MissionParams * pmp)
{
    if (pmp->nTeams < 3)
        return("Diamond ring maps must have more than 2 teams; "
               "please change the map type, or number of teams.");

    return(NULL);
}


VOID CmapPVMakerDiamondRing::GenerateLayout(CMapPVData * pMapData)
{
    //pMapData->SetTeamClustersPerTeam(pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1);
	//KGJV-: changed, use now MapSize parameter
    pMapData->SetTeamClustersPerTeam(pMapData->GetMapSize() != 0 ? 2 : 1);
    pMapData->SetNeutralClustersPerTeam(2);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}


VOID CmapPVMakerDiamondRing::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flTheta, flAdjust, flRadius;
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    sID = sID % pMapData->GetTeamClustersPerTeam();

    flTheta = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
    switch(pMapData->GetTeamClustersPerTeam())
    {
        case 2:
            flAdjust = (float) (pi / 8.0f);
            if (1 == sID)
                flAdjust = -flAdjust;
            break;
        case 3:
            flAdjust = (sID - 1) * (float) (pi / 6.0f);
            break;
        default:
            flAdjust = 0.0f;
            break;
    }
    flTheta += flAdjust;
    flRadius = 0.55f;
    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}


VOID CmapPVMakerDiamondRing::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flTheta, flRadius;
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    sID = (sID % pMapData->GetClustersPerTeam()) -
            pMapData->GetTeamClustersPerTeam();
    if (0 == (sID & 0x01))
    {
        flRadius = 0.3f;
    } else
        flRadius = 0.8f;

    flTheta = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}


VOID CmapPVMakerDiamondRing::LinkClusters(CMapPVData * pMapData)
{
    SectorID sID, cTotalSectors, sNeutralID, sIndex;

    //
    // Link each team's group of sectors.
    //
    cTotalSectors = pMapData->GetTotalClusters();
    for(sID = 0; sID < cTotalSectors; sID += pMapData->GetClustersPerTeam())
    {
        sNeutralID = sID + pMapData->GetTeamClustersPerTeam();

        for(sIndex = 0; sIndex < pMapData->GetTeamClustersPerTeam(); sIndex++)
        {
            CmapPVMaker::LinkClusters(pMapData,
                                        sID + sIndex,
                                        sNeutralID);
            CmapPVMaker::LinkClusters(pMapData,
                                        sID + sIndex,
                                        sNeutralID + 1);
        }
    }

    //
    // Link the rings.
    //
    for(sID = 0; sID < cTotalSectors; sID++)
    {
        if (NA == pMapData->GetClusterSide(sID))
        {
            sNeutralID = (sID + pMapData->GetClustersPerTeam()) % cTotalSectors;
            CmapPVMaker::LinkClusters(pMapData, sID, sNeutralID);
        }
    }

    //
    // Link the center sectors.
    //
    this->CrossLinkClusters(pMapData, pMapData->GetTeamClustersPerTeam());
}


CmapPVMakerSnowFlake::CmapPVMakerSnowFlake()
{
    mMMID = c_mmSnowFlake;
    this->SetName("SnowFlake");
}


VOID CmapPVMakerSnowFlake::GenerateLayout(CMapPVData * pMapData)
{
    //SectorID    s = pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1;
	//KGJV-: changed, use now MapSize parameter
    SectorID    s = pMapData->GetMapSize() != 0 ? 2 : 1;
    pMapData->SetTeamClustersPerTeam(s);
    pMapData->SetNeutralClustersPerTeam(3);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}


VOID CmapPVMakerSnowFlake::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flRadius, flTheta, flAdjust;
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    sID = sID % pMapData->GetClustersPerTeam();

    flTheta = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
    flAdjust = pMapData->GetTeamClustersPerTeam() == 1 ? 0.0f : (float) (pi / (2.0f * pMapData->GetTeams()));

    flRadius = 0.9f;

    if (pMapData->GetTeamClustersPerTeam() == sID + 1)
        flAdjust = -flAdjust;
    flTheta += flAdjust;
    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}


VOID CmapPVMakerSnowFlake::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    float flRadius, flTheta, flAdjust;
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    sID = (sID % pMapData->GetClustersPerTeam()) -
            pMapData->GetTeamClustersPerTeam();

    flTheta = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
    if (sID % 3 < 2)
    {
        //
        // This cluster is in the second ring.
        //
        flRadius = 0.65f;

        flAdjust = (float) (pi / (2.0f * pMapData->GetTeams()));
        if (0 == sID)
            flAdjust = -flAdjust;
    }
    else
    {
        flRadius = 0.3f;
        flAdjust = 0.0f;
    }

    flTheta += flAdjust;
    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}


VOID CmapPVMakerSnowFlake::LinkClusters(CMapPVData * pMapData)
{
    SectorID sID, cTotalClusters, cTeamClustersPerTeam, sIndex;
    SectorID cClustersPerTeam;

    cTotalClusters = pMapData->GetTotalClusters();
    cTeamClustersPerTeam = pMapData->GetTeamClustersPerTeam();
    cClustersPerTeam = pMapData->GetClustersPerTeam();

    for(sID = 0; sID < cTotalClusters; sID += cClustersPerTeam)
    {
        //
        // Link friendly clusters.
        //
        if (cTeamClustersPerTeam > 1)
        {
            for(sIndex = 0; sIndex < cTeamClustersPerTeam - 1; sIndex++)
            {
                CmapPVMaker::LinkClusters(pMapData,
                                            sID + sIndex,
                                            sID + sIndex + 1);
            }
            CmapPVMaker::LinkClusters(pMapData, sID + sIndex, sID);
        }

        //
        // Link Neutral clusters.
        //
        sIndex = sID + cTeamClustersPerTeam;
        CmapPVMaker::LinkClusters(pMapData, sIndex, sIndex + 2);
        CmapPVMaker::LinkClusters(pMapData, sIndex + 1, sIndex + 2);
        
        //
        // Now we need to link the friendly to the neutral.
        //
        switch(cTeamClustersPerTeam)
        {
            case 3:
                CmapPVMaker::LinkClusters(pMapData, sID + 1, sIndex);
                CmapPVMaker::LinkClusters(pMapData, sID + 1, sIndex + 1);
                CmapPVMaker::LinkClusters(pMapData, sID + 2, sIndex);
                CmapPVMaker::LinkClusters(pMapData, sID + 2, sIndex + 1);
                break;
            case 2:
                CmapPVMaker::LinkClusters(pMapData, sID, sIndex);
                CmapPVMaker::LinkClusters(pMapData, sID, sIndex + 1);
                CmapPVMaker::LinkClusters(pMapData, sID + 1, sIndex);
                CmapPVMaker::LinkClusters(pMapData, sID + 1, sIndex + 1);
                break;
            default:
                CmapPVMaker::LinkClusters(pMapData, sID, sIndex);
                CmapPVMaker::LinkClusters(pMapData, sID, sIndex + 1);
                break;
        }
    }

    //
    // Now we need to link the second neutral ring.
    //
    sIndex = NA;
    for(sID = cTeamClustersPerTeam;
        sID < cTotalClusters;
        sID += cClustersPerTeam)
    {
        CmapPVMaker::LinkClusters(pMapData, sID, sID + 1);
        if (NA != sIndex)
            CmapPVMaker::LinkClusters(pMapData, sIndex, sID);
        sIndex = sID + 1;
    }
    CmapPVMaker::LinkClusters(pMapData, sIndex, cTeamClustersPerTeam);

    //
    // Cross link the center sectors.
    //
    CmapPVMaker::CrossLinkClusters(pMapData, cTeamClustersPerTeam + 2);

    //
    // If less than 4 teams, cross linking doesn't do anything.  So
    // make a ring out of them.
    //
    if (pMapData->GetTeams() < 4)
    {
        sIndex = NA;
        for(sID = cTeamClustersPerTeam + 2;
            sID < cTotalClusters;
            sID += cClustersPerTeam)
        {
            if (NA != sIndex)
                CmapPVMaker::LinkClusters(pMapData, sID, sIndex);
            sIndex = sID;
        }
        if (pMapData->GetTeams() > 2)
        {
            CmapPVMaker::LinkClusters(pMapData,
                                        sIndex,
                                        cTeamClustersPerTeam + 2);
        }
    }
}


//
// Split Base
//

const char* CmapPVMakerLargeSplit::IsValid(const MissionParams * pmp)
{
	// yp your_persona removed march 24 2006
    //if (pmp->nMinPlayersPerTeam < 10)
    //  return("Large split maps must have at least 10 players per team; "
    //          "please change the map type, or number of players per team.");
	//
    return(NULL);
}

CmapPVMakerLargeSplit::CmapPVMakerLargeSplit()
{
    mMMID = c_mmLargeSplit;
    this->SetName("LargeSplit");
}

CmapPVMakerInsideOut::CmapPVMakerInsideOut()
{
    mMMID = c_mmInsideOut;
    this->SetName("Inside out");
}

CmapPVMakerGrid::CmapPVMakerGrid()
{
    mMMID = c_mmGrid;
    this->SetName("Grid");
}


VOID CmapPVMakerGrid::GenerateLayout(CMapPVData * pMapData)
{
    //pMapData->SetTeamClustersPerTeam(pMapData->GetMaxPlayersOnTeam() >= 30 ? 2 : 1);
	//KGJV-: changed, use now MapSize parameter
    pMapData->SetTeamClustersPerTeam(pMapData->GetMapSize() != 0 ? 2 : 1);
                                    
    pMapData->SetNeutralClustersPerTeam(4);

    pMapData->SetOtherClusters(0);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}

VOID CmapPVMakerGrid::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam = sID / pMapData->GetClustersPerTeam();
    sID = (sID % pMapData->GetClustersPerTeam());

    float flTheta;
    float flRadius;
    if (pMapData->GetTeamClustersPerTeam() == 1)
    {
        flTheta = (float) (2.0 * pi * sTeam / pMapData->GetTeams());

        static const float  radii[c_cSidesMax + 1] = {0.0f, 0.0f, 0.55f, 0.6f, 0.65f, 0.70f, 0.75f};
        assert (pMapData->GetTeams() <= c_cSidesMax);
        flRadius = radii[pMapData->GetTeams()];
    }
    else
    {
        flTheta = (float) (2.0 * pi * sTeam / pMapData->GetTeams());

        static const float  radii0[c_cSidesMax + 1] = {0.0f, 0.0f, 0.475f, 0.525f, 0.575f, 0.625f, 0.675f};
        static const float  radii1[c_cSidesMax + 1] = {0.0f, 0.0f, 0.625f, 0.675f, 0.725f, 0.775f, 0.825f};

        assert (pMapData->GetTeams() <= c_cSidesMax);
        flRadius = (sID == 0)
                   ? radii0[pMapData->GetTeams()]
                   : radii1[pMapData->GetTeams()];
    }

    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}

VOID CmapPVMakerGrid::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    sID = (sID % pMapData->GetClustersPerTeam()) - pMapData->GetTeamClustersPerTeam();

    float flTheta1 = (float) (2.0 * pi * sTeam / pMapData->GetTeams());
    float flTheta2 = (float) (2.0 * pi * (sTeam + 1) / pMapData->GetTeams());

    float   flTheta;
    float   flRadius;
    switch (sID)
    {
        case 0:
            flTheta = (5.0f * flTheta1 + flTheta2) / 6.0f;
            flRadius = 1.0f;
        break;
        case 1:
            flTheta = (3.0f * flTheta1 + flTheta2) / 4.0f;
            flRadius = 0.5f;
        break;
        case 2:
            flTheta = (3.0f * flTheta2 + flTheta1) / 4.0f;
            flRadius = 0.5f;
        break;
        case 3:
            flTheta = (5.0f * flTheta2 + flTheta1) / 6.0f;
            flRadius = 1.0f;
        break;
    }

    pdc->screenX = cos(flTheta) * flRadius;
    pdc->screenY = sin(flTheta) * flRadius;
}

VOID CmapPVMakerGrid::LinkClusters(CMapPVData * pMapData)
{
    if (pMapData->GetTeamClustersPerTeam() != 1)
    {
        //Link the split base clusters for each team
        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sID = i * pMapData->GetClustersPerTeam();
            CmapPVMaker::LinkClusters(pMapData, sID, sID + 1);
        }
    }

    {
        SectorID    sidCenter = pMapData->GetTotalClusters() - 1;

        for (SideID i = 0; (i < pMapData->GetTeams()); i++)
        {
            SectorID    sIDh = i * pMapData->GetClustersPerTeam();
            SectorID    sID0 = sIDh +
                               pMapData->GetTeamClustersPerTeam();

            SectorID    sID1 = sID0 + 1;
            SectorID    sID2 = sID0 + 2;
            SectorID    sID3 = sID0 + 3;

            SectorID    sIDn  = ((i+1)%pMapData->GetTeams()) * pMapData->GetClustersPerTeam();
            SectorID    sIDn0 = sIDn +
                                pMapData->GetTeamClustersPerTeam();

            CmapPVMaker::LinkClusters(pMapData, sID0, sID1);
            CmapPVMaker::LinkClusters(pMapData, sID1, sID2);
            CmapPVMaker::LinkClusters(pMapData, sID2, sID3);
            CmapPVMaker::LinkClusters(pMapData, sID3, sID0);

            CmapPVMaker::LinkClusters(pMapData, sID2, sIDn0 + 1);
            CmapPVMaker::LinkClusters(pMapData, sID3, sIDn0 + 0);

            CmapPVMaker::LinkClusters(pMapData, sID0, sIDh + pMapData->GetTeamClustersPerTeam() - 1);
            CmapPVMaker::LinkClusters(pMapData, sID1, sIDh);

            CmapPVMaker::LinkClusters(pMapData, sID2, sIDn);
            CmapPVMaker::LinkClusters(pMapData, sID3, sIDn + pMapData->GetTeamClustersPerTeam() - 1);

            if (pMapData->GetMissionParams()->iRandomEncounters != 0)
            {
                if (pMapData->GetTeams() == 2)
                    CmapPVMaker::LinkClusters(pMapData, sID1 + i, sIDn0 + 1 + i);
            }
        }
    }
}

const char* CmapPVMakerEastWest::IsValid(const MissionParams * pmp)
{
    if (pmp->nTeams != 2) 
        return("East West maps must have exactly two teams; "
                "please change the map type, or the number of teams.");
	// yp your_persona removed march 24 2006
    // else if (pmp->nMinPlayersPerTeam < 10)
    //     return("East West maps must have at least 10 players per team; "
    //            "please change the map type, or the number or players per team.");

    return(NULL);
}

CmapPVMakerEastWest::CmapPVMakerEastWest()
{
    mMMID = c_mmEastWest;
    this->SetName("EastWest");
}


VOID CmapPVMakerEastWest::GenerateLayout(CMapPVData * pMapData)
{
    pMapData->SetTeamClustersPerTeam(2);
                                    
    pMapData->SetNeutralClustersPerTeam(11);

    pMapData->SetOtherClusters(0);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}

VOID CmapPVMakerEastWest::GenerateTeamClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam = sID / pMapData->GetClustersPerTeam();
    sID = (sID % pMapData->GetClustersPerTeam());

    pdc->screenX = sTeam == 0     ?  0.70f : -0.70f;
    pdc->screenY = (sID == sTeam) ? -0.55f :  0.55f;
}

VOID CmapPVMakerEastWest::GenerateNeutralClusterScreenPosition(
                                                    CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID sTeam;

    sTeam = sID / pMapData->GetClustersPerTeam();
    sID = (sID % pMapData->GetClustersPerTeam()) - pMapData->GetTeamClustersPerTeam();

    static const float  x[11] = { 1.0f,  1.0f,  0.4f,  0.4f, 1.0f, 1.0f, 0.4f, 0.4f,  0.15f, 0.15f, 0.15f };
    static const float  y[11] = {-0.9f, -0.2f, -0.9f, -0.2f, 0.2f, 0.9f, 0.2f, 0.9f, -0.55f, 0.00f, 0.55f };

    if (sTeam == 0)
    {
        pdc->screenX = x[sID];
        pdc->screenY = y[sID];
    }
    else
    {
        pdc->screenX = -x[sID];
        pdc->screenY = -y[sID];
    }
}

VOID CmapPVMakerEastWest::LinkClusters(CMapPVData * pMapData)
{
    assert (pMapData->GetTeams() == 2);
    assert (pMapData->GetClustersPerTeam() == 13);
    assert (pMapData->GetTeamClustersPerTeam() == 2);

    for (SideID i = 0; (i < 2); i++)
    {
        SectorID    sIDf0 = i * 13;
        SectorID    sIDf1 = sIDf0 + 1;
        SectorID    sID0  = sIDf1 + 1;

        CmapPVMaker::LinkClusters(pMapData, sIDf0, sID0 + 0);
        CmapPVMaker::LinkClusters(pMapData, sIDf0, sID0 + 1);
        CmapPVMaker::LinkClusters(pMapData, sIDf0, sID0 + 2);
        CmapPVMaker::LinkClusters(pMapData, sIDf0, sID0 + 3);



        CmapPVMaker::LinkClusters(pMapData, sIDf1, sID0 + 4);
        CmapPVMaker::LinkClusters(pMapData, sIDf1, sID0 + 5);
        CmapPVMaker::LinkClusters(pMapData, sIDf1, sID0 + 6);
        CmapPVMaker::LinkClusters(pMapData, sIDf1, sID0 + 7);

        CmapPVMaker::LinkClusters(pMapData, sID0 + 0, sID0 + 1);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 1, sID0 + 3);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 3, sID0 + 2);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 2, sID0 + 0);

        CmapPVMaker::LinkClusters(pMapData, sID0 + 4, sID0 + 5);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 5, sID0 + 7);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 7, sID0 + 6);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 6, sID0 + 4);


        CmapPVMaker::LinkClusters(pMapData, sID0 + 4, sID0 + 1);

        CmapPVMaker::LinkClusters(pMapData, sID0 + 8, sID0 + 2);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 8, sID0 + 3);

        CmapPVMaker::LinkClusters(pMapData, sID0 + 9, sID0 + 3);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 9, sID0 + 6);

        CmapPVMaker::LinkClusters(pMapData, sID0 + 10, sID0 + 6);
        CmapPVMaker::LinkClusters(pMapData, sID0 + 10, sID0 + 7);
    }

    CmapPVMaker::LinkClusters(pMapData, 10, 25);
    CmapPVMaker::LinkClusters(pMapData, 11, 24);
    CmapPVMaker::LinkClusters(pMapData, 12, 23);
}


const char* CmapPVMakerSplitBase::IsValid(const MissionParams * pmp)
{
	// yp your_persona removed  march 24 2006
    //if (pmp->nTeams * pmp->nMinPlayersPerTeam < 30)
    //    return("Split base maps must have at least 30 players; "
    //           "please change the map type, the number of teams, or number of players per team.");

    return(NULL);
}
CmapPVMakerSplitBase::CmapPVMakerSplitBase()
{
    mMMID = c_mmSplitBase;
    this->SetName("Split Base");
}
VOID CmapPVMakerSplitBase::GenerateLayout(CMapPVData * pMapData)
{
    pMapData->SetTeamClustersPerTeam(2);
    pMapData->SetNeutralClustersPerTeam(2);

    this->GenerateRingClusters(pMapData);

    //
    // Now that we have the clusters, link them together.
    //
    this->LinkClusters(pMapData);
}

VOID CmapPVMakerSplitBase::GenerateTeamClusterScreenPosition(CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID    n = pMapData->GetTeamClusters();
    SectorID    s = (sID + 1) / 2;
    float flTheta = (float) (2.0 * pi * s / n);
    //float flRadius = pMapData->GetTotalClusters() <= 18 ? 0.8f : 1.0f;
    pdc->screenX = cos(flTheta) * 0.9f;
    pdc->screenY = sin(flTheta) * 0.9f;
}

VOID CmapPVMakerSplitBase::GenerateNeutralClusterScreenPosition(CMapPVData * pMapData,
                                                    DataClusterIGC * pdc,
                                                    SectorID sID)
{
    SectorID    n = pMapData->GetTeamClusters();
    SectorID    s = (sID + 1) / 2;
    float flTheta = (float) (2.0 * pi * s / n);
    //float flRadius = pMapData->GetTotalClusters() <= 18 ? 0.5f : 0.7f;
    pdc->screenX = cos(flTheta) * 0.6f;
    pdc->screenY = sin(flTheta) * 0.6f;
}

VOID CmapPVMakerSplitBase::LinkClusters(CMapPVData * pMapData)
{
    SectorID    cTotalSectors = pMapData->GetTotalClusters();
    for (SideID i = 0; (i < pMapData->GetTeams()); i++)
    {
        SectorID    sID = i * pMapData->GetClustersPerTeam();

        CmapPVMaker::LinkClusters(pMapData, sID + 1, sID + 2);
        CmapPVMaker::LinkClusters(pMapData, sID + 2, sID + 3);
        CmapPVMaker::LinkClusters(pMapData, sID + 3, (sID + 4) % cTotalSectors);
        CmapPVMaker::LinkClusters(pMapData, sID + 2, (cTotalSectors + (sID - 1)) % cTotalSectors);

        if (pMapData->GetTeams() > 2)
        {
            CmapPVMaker::LinkClusters(pMapData, sID + 2, (sID + 7) % cTotalSectors);
            if (pMapData->GetTeams() > 3)
                CmapPVMaker::LinkClusters(pMapData, sID + 3, (cTotalSectors + (sID - 2)) % cTotalSectors);
        }
    }
}

// from IGC file stuff
const char * CmapPVMakerFromIGCFile::GenerateMission(const char * igcfile,	ImissionPV* pmission)
{
    Modeler* pmodeler = pmission->GetModeler();

	// BT - 10/17 - HighRes Textures
	TRef<ZFile> zf = pmodeler->GetFile(igcfile,"igc",false, pmodeler->GetUseHighResTextures());
    
	if (!zf) return "no preview available";

	int             iDatasize;
	int             iReadCount = zf->Read(&iDatasize, sizeof(iDatasize));
	if (iReadCount != sizeof(iDatasize)) return "bad custom map file";
	char*           pData = new char[iDatasize+4];      //leave a little extra space for the encryption (which takes dword chunks)
	iReadCount = zf->Read(pData, sizeof(char) * iDatasize);
	if (iReadCount != sizeof(char) * iDatasize) return "bad custom map file";

	//debugf("CmapPVMakerFromIGCFile: %s , size %d\n",igcfile,iDatasize);
	int datasize = iDatasize;
	char const *sError = NULL;
	char *pdata = pData;
// main read loop
	int nbwarps = 0;
	int nbclusters = 0;
	int nbstations = 0;
    while (datasize > 0)
    {
        ObjectType  type = *((ObjectType*)pdata);
        int size = *((int*)(pdata + sizeof(ObjectType)));

        if (__int64(-1) & (__int64(1) << __int64(type)))
        {
            if (type == OT_warp)
            {
				nbwarps++;
				assert(size == sizeof(DataWarpIGC));
				DataWarpIGC *p = (DataWarpIGC*)(pdata + sizeof(int) + sizeof(ObjectType));
				// assume all clusters are in file BEFORE warps
				// otherwise we reject the file
				// to avoid this, we could do this in 2 pass but for now we dont :)
				CMapPVCluster *pcluster = pmission->GetCluster(p->clusterID);
				if (pcluster)
					pmission->CreatePVWrap(p,size,pcluster);
				else
				{
					sError = "invalid aleph data";
					break;
				}
            }
			if (type == OT_cluster)
			{
				nbclusters++;
				assert(size == sizeof(DataClusterIGC));
				DataClusterIGC *p  = (DataClusterIGC*)(pdata + sizeof(int) + sizeof(ObjectType));
				pmission->CreatePVCluster(p,size);
			}
			if (type == OT_station)
			{
				nbstations++;
				assert(size == sizeof(DataStationIGC));
				DataStationIGC*  p = (DataStationIGC*)(pdata + sizeof(int) + sizeof(ObjectType));
				// assume all clusters are in file BEFORE stations
				CMapPVCluster *pcluster = pmission->GetCluster(p->clusterID);
				if (pcluster)
				{
					SideID side = p->sideID;
					assert(side>=0 && side<c_cSidesMax);
					pcluster->GetStationsPerSide()[side]++;
				}
				else
				{
					sError = "invalid station data";
					break;
				}
			}

            pdata += size + sizeof(int) + sizeof(ObjectType);
        }

        datasize -= (size + sizeof(int) + sizeof(ObjectType));
    }
// end of read loop
	debugf("CmapPVMakerFromIGCFile: %s , %d clusters, %d warps, %d stations\n",igcfile,nbclusters,nbwarps,nbstations);
	delete [] pData;
	return sError;

}
