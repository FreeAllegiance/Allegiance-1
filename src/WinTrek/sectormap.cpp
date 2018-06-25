#include "pch.h"
#include "trekmdl.h"
#include "trekctrls.h"

const int   c_iClusterHasNoRipcord = 0;
const int   c_iClusterHasStationRipcord = 1;
const int   c_iClusterHasShipRipcord = 2;

const Color c_rgColorNeutral(0.5f, 0.5f, 0.5f);

static void GetClusterOwners(IclusterIGC*    pcluster, SideID& sideOwner, SideID& sideSecondaryOwner)
{
    assert (c_cSidesMax == 6);
    int nStations[c_cSidesMax] = {0, 0, 0, 0, 0, 0};

    // count the stations by side
    for (StationLinkIGC*    psl = pcluster->GetStations()->first();
         (psl != NULL);
         psl = psl->next())
    {
        if (!psl->data()->GetStationType()->HasCapability(c_sabmPedestal))
            nStations[psl->data()->GetSide()->GetObjectID()]++;
    }

    // the owner is the one with the most stations
    int     nStationsOwner = 0;
    int     nStationsSecondaryOwner = 0;
    sideOwner = NA;
    sideSecondaryOwner = NA;
    SideID  sideID;
    for (sideID = 0; sideID < c_cSidesMax; sideID++)
    {
        if (nStations[sideID] > nStationsOwner)
        {
            // demote the current top owner
            sideSecondaryOwner = sideOwner;
            nStationsSecondaryOwner = nStationsOwner;

            // track the new top owner
            sideOwner = sideID;
            nStationsOwner = nStations[sideID];
        }
        else if (nStations[sideID] > nStationsSecondaryOwner)
        {
            sideSecondaryOwner = sideID;
            nStationsSecondaryOwner = nStations[sideID];
        }
    }
}

static int GetClusterPopulation(IclusterIGC*   pcluster, IsideIGC*  pside)
{
    int n = 0;

    SectorID    sectorID = pcluster->GetObjectID();

    for (ShipLinkIGC*   psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())
    {
        IshipIGC*   pship = psl->data();

        if (pship->GetSide() == pside)
        {
            PlayerInfo* ppi = (PlayerInfo*)(pship->GetPrivateData());
            if (ppi && (ppi->LastSeenSector() == sectorID) && (ppi->StatusIsCurrent()))
            {
                if (ppi->LastSeenState() != c_ssObserver && ppi->LastSeenState() != c_ssTurret)
                {
                    IhullTypeIGC* pht = trekClient.GetCore()->GetHullType(ppi->LastSeenShipType());

                    if (pht != NULL && !(pht->GetCapabilities() & c_habmLifepod))
                        n++;
                }
            }
        }
    }

    return n;
}

//////////////////////////////////////////////////////////////////////////////
//
// SectorInfoPane
//
//////////////////////////////////////////////////////////////////////////////
struct  StoredIcon
{
    Surface*    psurfaceIcon;
    IsideIGC*   pside;
    int         count;
    int         sortOrder;


    StoredIcon(void)
    :
        count(1)
    {
    }
};

typedef Slist_utl<StoredIcon>   IconList;
typedef Slink_utl<StoredIcon>   IconLink;

static void AddIcon(IObject*    psurfaceIcon,
                    IsideIGC*   pside,
                    int         sortOrder,
                    int         sideID,
                    IconList*   picons)
{
    for (IconLink*  pil = picons->first(); (pil != NULL); pil = pil->next())
    {
        if ((pil->data().psurfaceIcon == (Surface*)psurfaceIcon) &&
            (pil->data().pside == pside))  //ALLYTD? VISIBILITY change this if we want allied assets shown in the sector info pane
        {
            pil->data().count++;
            return;
        }
    }

    IconLink*   p = new IconLink;
    p->data().psurfaceIcon = (Surface*)psurfaceIcon;
    p->data().pside = pside;

    //Hash in the side ID
    p->data().sortOrder = sortOrder += ((int)sideID) * 0x10000;
    {
        //Walk the list till we find the proper spot
        for (IconLink*  pil = picons->first(); (pil != NULL); pil = pil->next())
        {
            if (sortOrder < pil->data().sortOrder)
            {
                pil->txen(p);
                return;
            }
        }
    }
    picons->last(p);
}

class SectorInfoPane : public Image
{
private:

    TRef<Image>             m_pimageBkgnd;
    TRef<Image>             m_pimageSmallBkgnd;
    TRef<Image>             m_pimageLargeBkgnd;
    TRef<Image>             m_pimageExpand;

    TRef<IclusterIGC>       m_pClusterSel;
    TRef<IclusterIGC>       m_pClusterMouseOver;
	TRef<Surface>			m_pprobeIcon;

public:

    SectorInfoPane()
    {
        m_pimageSmallBkgnd = GetModeler()->LoadImage("consectorinfobmp", true);
        m_pimageLargeBkgnd = GetModeler()->LoadImage("conesectorinfobmp", true);
        m_pimageExpand = GetModeler()->LoadImage("consectorexpandbmp", true);

		// The minimap has an open and closed view. The selected background image controls which view is shown.
		m_pimageBkgnd = m_pimageLargeBkgnd;

		m_pprobeIcon = (Surface *)trekClient.LoadRadarIcon("probe");
    }

    void CalcBounds()
    {
        m_bounds = m_pimageBkgnd->GetBounds();
    }

    void ClearCluster(void)
    {
        m_pClusterSel = NULL;
        m_pClusterMouseOver = NULL;
    }

    void SelectCluster(IclusterIGC* pClusterSel)
    {
        m_pClusterSel = pClusterSel;
        Changed();
    }

    void SelectMouseOverCluster(IclusterIGC* pClusterMouseOver)
    {
        m_pClusterMouseOver = pClusterMouseOver;
        Changed();
    }

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    {
        Surface* psurfaceBackground = m_pimageBkgnd->GetSurface();
        Surface* psurfaceExpand = m_pimageExpand->GetSurface();
        WinPoint pntButton = psurfaceBackground->GetSize() - psurfaceExpand->GetSize();
        pntButton = pntButton - WinPoint(16, 4);
        if (point.X() > pntButton.X() && point.Y() > pntButton.Y()
            && point.X() < psurfaceBackground->GetSize().X()
            && point.Y() < psurfaceBackground->GetSize().Y())
        {
            return MouseResultHit();
        }

        return m_pimageBkgnd->HitTest(pprovider, point, bCaptured);
    }

    WinPoint GetButtonOffset()
    {
        Surface* psurfaceBackground = m_pimageBkgnd->GetSurface();
        Surface* psurfaceExpand = m_pimageExpand->GetSurface();
        WinPoint pntButton = psurfaceBackground->GetSize() - psurfaceExpand->GetSize();
        return pntButton - WinPoint(16, 4);
    }

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        if (button == 0 && !bDown)
        {
            WinPoint pntButton = GetButtonOffset();

            if (point.X() > pntButton.X() && point.Y() > pntButton.Y())
            {
                if (m_pimageBkgnd == m_pimageSmallBkgnd)
                {
                    m_pimageBkgnd = m_pimageLargeBkgnd;
                }
                else
                {
                    m_pimageBkgnd = m_pimageSmallBkgnd;
                }

                Changed();
            }
        }

        return MouseResult();
    }

    /*
    void DrawStationIconsOfClass(Context* pcontext, IclusterIGC* pcluster,
        Point& ptNext, StationClassID classID)
    {
        const StationListIGC* stationList = pcluster->GetStations();

        for (StationLinkIGC* stationLink = (stationList ? stationList->first() : NULL);
            stationLink; stationLink=stationLink->next())
        {
            IstationIGC* pstation = stationLink->data();

            // if this station is in the correct class of station
            if (pstation->GetBaseStationType()->GetClassID() == classID)
            {
                // draw this station icon
                Surface* psurfaceIcon = (Surface*)(pstation->GetIcon());

                if (ptNext.X() < GetButtonOffset().X())
                {
                    pcontext->DrawImage3D(psurfaceIcon, pstation->GetSide()->GetColor(), false, ptNext);
                    ptNext += Point(psurfaceIcon->GetSize().X(), 0);
                }
            }
        }
    }

    void DrawStationIcons(Context* pcontext, IclusterIGC* pcluster,
        const Point& ptStations)
    {
        Point ptNext = ptStations;

        // draw the station icons in order of importance
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scStarbase);
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scGarrison);
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scShipyard);
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scRipcord);
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scResearch);
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scOrdinance);
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scElectronics);
        DrawStationIconsOfClass(pcontext, pcluster, ptNext, c_scMining);
    }
    */

    void DrawAsteroidIcons(Context* pcontext, IEngineFont* pfont, IclusterIGC* pcluster,
        const Point& ptAsteroids)
    {
        Point ptNext = ptAsteroids;

        // Count the asteroids

        TMap<AsteroidAbilityBitMask, int> mapSpecialAsteroids;
        TMap<AsteroidAbilityBitMask, Surface*> mapSpecialAsteroidIcons;

        for (AsteroidLinkIGC* asteroidLink = pcluster->GetAsteroids()->first();
            asteroidLink != NULL; asteroidLink = asteroidLink->next())
        {
            AsteroidAbilityBitMask aabm = asteroidLink->data()->GetCapabilities();

            // if this asteroid has any special capabilities besides being buildable
            if ((aabm & ~c_aabmBuildable) != 0)
            {
                // let it stand up and be counted.

                int nCount = 0;

                mapSpecialAsteroids.Find(aabm, nCount);
                mapSpecialAsteroids.Set(aabm, nCount+1);
                mapSpecialAsteroidIcons.Set(aabm, (Surface*)asteroidLink->data()->GetIcon());
            }
        }

        // draw the asteroids

        TMap<AsteroidAbilityBitMask, int>::Iterator iterCount(mapSpecialAsteroids);
        TMap<AsteroidAbilityBitMask, Surface*>::Iterator iterIcon(mapSpecialAsteroidIcons);

        while (!iterCount.End())
        {
            pcontext->DrawImage3D(iterIcon.Value(), Color::White(), false, ptNext);
            pcontext->DrawString(
                pfont,
                Color::White(),
                ptNext + Point(iterIcon.Value()->GetSize().X() + 4, 0),
                ZString(iterCount.Value())
            );

            ptNext = ptNext - Point(0, iterIcon.Value()->GetSize().Y());
            iterCount.Next();
            iterIcon.Next();
        }

		//Xynth #129 7/2010 Number of probes in sector
		int probeCount = 0;
		for (ProbeLinkIGC*  ppl = pcluster->GetProbes()->first(); (ppl != NULL); ppl = ppl->next())
        {
            IprobeIGC*  pprobe = ppl->data();
            if (pprobe->GetSide() == trekClient.GetShip()->GetSide() &&
				(pprobe->GetProbeType()->GetCapabilities() == 0 || pprobe->GetProbeType()->GetCapabilities() >= c_eabmRescue)) //Imago 8/10
				probeCount++;
		}
		//Imago 8/10
		pcontext->DrawImage3D(m_pprobeIcon, trekClient.GetSide()->GetColor(), false, ptNext);
		pcontext->DrawString(
                pfont,
                Color::White(),
                ptNext + Point(m_pprobeIcon->GetSize().X() + 4, 0),
                ZString(probeCount)
				);


    }


    float GetHeliumInCluster(IclusterIGC* pcluster)
    {
        float fOre = 0;
		IsideIGC*   psideMine = trekClient.GetShip()->GetSide();
        for (AsteroidLinkIGC* asteriodLink = pcluster->GetAsteroids()->first();
            asteriodLink != NULL; asteriodLink = asteriodLink->next())
        {
            AsteroidAbilityBitMask aabm = asteriodLink->data()->GetCapabilities();

            // if we can mine helium at this asteroid
            if ((aabm & c_aabmMineHe3) != 0)
            {
                // count it.
                //Xynth #100 7/2010
				//fOre += asteriodLink->data()->GetOre();
				fOre += asteriodLink->data()->GetOreSeenBySide(psideMine);
            }
        }

        return fOre;
    }

	//Xynth #104 7/2010  Returns true if at least one He3 is eyed
	bool HeliumAsteroidVisibileInCluster(IclusterIGC* pcluster)
	{
		bool HeSpotted = false;
		IsideIGC*   psideMine = trekClient.GetShip()->GetSide();
        for (AsteroidLinkIGC* asteriodLink = pcluster->GetAsteroids()->first();
            asteriodLink != NULL; asteriodLink = asteriodLink->next())
        {
            AsteroidAbilityBitMask aabm = asteriodLink->data()->GetCapabilities();

            // if we can mine helium at this asteroid
            if ((aabm & c_aabmMineHe3) != 0)
            {
                if (asteriodLink->data()->GetAsteroidCurrentEye(psideMine))
					HeSpotted = true;
            }
        }
		return HeSpotted;
	}  //End #104 function

    void Render(Context* pcontext)
    {
		pcontext->SetBlendMode(BlendModeSourceAlpha); //Imago 7/15/09

        // draw the background
        Surface* psurfaceBackground = m_pimageBkgnd->GetSurface();
        pcontext->DrawImage(psurfaceBackground);

        // set font

        TRef<IEngineFont> pfont = TrekResources::SmallFont();

        // draw the resize button
        Surface* psurfaceExpand = m_pimageExpand->GetSurface();
        pcontext->DrawImage(psurfaceExpand, false,
            Point::Cast(psurfaceBackground->GetSize() - psurfaceExpand->GetSize() - WinPoint(16, 4)));

        if (m_pClusterSel)
        {
            const int xBorder = 4;
            float yTop = m_pimageBkgnd->GetBounds().GetRect().YSize();
            Point ptSectorName(xBorder + 20, yTop - 13);
            Point ptStations(xBorder, yTop - 26);

            // draw sectorname
            pcontext->DrawString(
                pfont,
                Color::White(),
                ptSectorName,
                ZString(m_pClusterSel->GetName()) + " Sector"
            );

            // draw station icons c_cSidesMax
            {
                IconList    icons;
                IsideIGC*   psideMe = trekClient.GetSide();
                {
                    for (StationLinkIGC*    psl = m_pClusterSel->GetStations()->first(); (psl != NULL); psl = psl->next())
                    {
                        if (!psl->data()->GetStationType()->HasCapability(c_sabmPedestal))
                        {
                            IsideIGC*   pside = psl->data()->GetSide();
                            AddIcon(psl->data()->GetIcon(), pside, NA,
                                    (pside == psideMe) ? (c_cSidesMax * 2) : (c_cSidesMax + pside->GetObjectID()), &icons);  //ALLYTD? VISIBILITY Y change this if we want allied assets shown in the sector info pane
                        }
                    }
                }
                {
                    SectorID    sid = m_pClusterSel->GetObjectID();
                    for (ShipLinkIGC*       psl = trekClient.m_pCoreIGC->GetShips()->first(); (psl != NULL); psl = psl->next())
                    {
                        PlayerInfo* ppi = (PlayerInfo*)(psl->data()->GetPrivateData());
                        if (ppi &&
                            ppi->StatusIsCurrent() &&
                            (ppi->LastSeenState() == c_ssFlying) &&
                            (ppi->LastSeenSector() == sid))
                        {
                            HullID          hid = ppi->LastSeenShipType();
                            IhullTypeIGC*   pht = trekClient.m_pCoreIGC->GetHullType(hid);
                            if (pht)
                            {
                                IsideIGC*   pside = psl->data()->GetSide();

                                AddIcon(pht->GetIcon(), pside, hid,
                                    (pside == psideMe) ? c_cSidesMax : pside->GetObjectID(), &icons); //ALLYTD? VISIBLITY
                            }
                        }
                    }
                }

                {
                    Point ptNext = ptStations;

                    for (IconLink*  pil = icons.first(); (pil != NULL); pil = pil->next())
                    {
                        if (ptNext.X() < GetButtonOffset().X())
                        {
                            const Color&  color = pil->data().pside->GetColor();
                            pcontext->DrawImage3D(pil->data().psurfaceIcon,
                                                  color,
                                                  false, ptNext);
                            ptNext += Point(pil->data().psurfaceIcon->GetSize().X(), 0);

                            if (pil->data().count > 1)
                            {
                                TRef<IEngineFont> pfont = TrekResources::SmallFont();
                                ZString strCount = pil->data().count;
                                WinPoint pt = pfont->GetTextExtent(strCount);
                                pcontext->DrawString(pfont, Color::White(),
                                    ptNext + Point(0, (pil->data().psurfaceIcon->GetSize().Y() - pt.Y())/2),
                                    strCount);
                                ptNext += Point(pt.X() + 2, 0);
                            }
                        }
                    }
                }
            }
            //DrawStationIcons(pcontext, m_pClusterSel, ptStations);

            if (m_pimageBkgnd == m_pimageLargeBkgnd)
            {
                const int xSecondColumn = m_pimageSmallBkgnd->GetBounds().GetRect().XSize() + xBorder - 15;
                Point ptHelium(xSecondColumn, yTop - 45);
                Point ptAsteroids(xSecondColumn, yTop - 60);

				//Xynth #104 7/2010
				//Function not reliable for this purpose
				Color color;
				//if (HeliumAsteroidVisibileInCluster(m_pClusterSel))
					color = Color::White();
				//else
				//	color = Color::Gray();

                // draw the helium count
                pcontext->DrawString(
                    pfont,
                    color, //Xynth #104 7/2010 Color::White(),
                    ptHelium,
                    "He3: " + ZString(int(GetHeliumInCluster(m_pClusterSel)))
                );

                // draw asteroids
                DrawAsteroidIcons(pcontext, pfont, m_pClusterSel, ptAsteroids);
            }

            if (m_pClusterMouseOver)
            {
                assert(m_pClusterMouseOver == m_pClusterSel);

                // draw the cluster warning
                AssetMask am = m_pClusterMouseOver->GetClusterSite()->GetClusterAssetMask();
                ClusterWarning cw = GetClusterWarning(am,
                    trekClient.MyMission()->GetMissionParams().bInvulnerableStations);

                if (cw > c_cwNoThreat)
                {
                    pcontext->DrawString(
                        pfont,
                        Color::White(),
                        Point(0,0),
                        GetClusterWarningText(cw)
                    );
                }
            }
        }
    }
};

TRef<IObject> SectorInfoPaneFactory::Apply(ObjectStack& stack)
{
    TRef<SectorInfoPane> psectorinfo = new SectorInfoPane();
    return (Value*)psectorinfo;
}

//////////////////////////////////////////////////////////////////////////////
//
// SectorMapPane
//
//////////////////////////////////////////////////////////////////////////////


class SectorMapPane : public Image, public IEventSink, public TrekClientEventSink
{
private:
    TRef<Image>             m_pimageSectorEmpty;
    TRef<Image>             m_pimageOwnerHighlight;
    TRef<Image>             m_pimageSecondaryOwnerHighlight;
    TRef<Image>             m_pimageBkgnd;
    TRef<Image>             m_pimageSectorHighlight;
    TRef<Image>             m_pimageSectorTargetHighlight;
    TRef<Image>             m_pimageSectorSel;
    TRef<Image>             m_pimageSectorPickable;
    TRef<Image>             m_pimageSectorPickTarget;
    TRef<Image>             m_pimageSectorPickableStation;
    TRef<Image>             m_pimageSectorPickStationTarget;
    TRef<Image>             m_pimageSectorQueued;
    TRef<Image>             m_pimageCapitalWarning;
    TRef<Image>             m_pimageSectorEnemy;
    TRef<Image>             m_pimageSectorMiner;
    TRef<Image>             m_pimageSectorWarning;
    TRef<Image>             m_pimageBomberWarning;
    TRef<Image>             m_pimageSectorCombat;
    TRef<IclusterIGC>       m_pClusterSel;
    TRef<IEventSink>        m_peventSinkTimer;
    TRef<SectorInfoPane>    m_pSectorInfoPane;
    float                   m_xMin;
    float                   m_xMax;
    float                   m_yMin;
    float                   m_yMax;

    float                   m_xClusterMin;
    float                   m_xClusterMax;
    float                   m_yClusterMin;
    float                   m_yClusterMax;

    bool                    m_bVisible;
    int                     m_nMaskModeActive;
    float                   m_xDrag;
    float                   m_yDrag;
    bool                    m_bDragging;
    bool                    m_bHovering;
    bool                    m_bCanDrag;
    Point                   m_pointLastDrag;
    WinRect                 m_rectMap;

    bool                    m_bFlashFrame;

public:

    enum { c_nXBorder = 4, c_nYBorder = 4 };

    SectorMapPane(SectorInfoPane* pSectorInfoPane, Number* pvalueMode, int nMaskModeActive) :
        Image(pvalueMode),
        m_nMaskModeActive(nMaskModeActive),
        m_pSectorInfoPane(pSectorInfoPane),
        m_bVisible(false),
        m_xDrag(0),
        m_yDrag(0),
        m_bDragging(false),
        m_bHovering(false),
        m_bCanDrag(false),
        m_bFlashFrame(true)
    {
        pvalueMode->Update();

        Modeler* pmodeler = GetModeler();
		// BUILD_DX9
        //m_pimageBkgnd = pmodeler->LoadImage("sectormapbkgndbmp", true);  //imago 7/15/09

        m_pimageBkgnd = pmodeler->LoadImage("sectormapbkgndbmp", false);
		// BUILD_DX9
        m_pimageSectorHighlight = pmodeler->LoadImage("sectorhighlightbmp", true);
        m_pimageSectorTargetHighlight = pmodeler->LoadImage("sectortargetbmp", true);
        m_pimageSectorSel = pmodeler->LoadImage("sectorselbmp", true);

        m_peventSinkTimer = IEventSink::CreateDelegate(this);
        //GetWindow()->GetTimer()->AddSink(m_peventSinkTimer, 1.0f);

        m_rectMap = WinRect::Cast(m_pimageBkgnd->GetBounds().GetRect());

        m_pimageSectorEmpty = pmodeler->LoadImage("sectoremptybmp", true);
        m_pimageOwnerHighlight = pmodeler->LoadImage("sectorownerbmp", true);
        m_pimageSecondaryOwnerHighlight = pmodeler->LoadImage("sectorinvaderbmp", true);

        m_pimageSectorPickable = pmodeler->LoadImage("sectorripcordbmp", true);
        m_pimageSectorPickTarget = pmodeler->LoadImage("sectorripcordtargetbmp", true);
        m_pimageSectorPickableStation = pmodeler->LoadImage("sectorripcordstationbmp", true);
        m_pimageSectorPickStationTarget = pmodeler->LoadImage("sectorripcordstationtargetbmp", true);
        m_pimageSectorQueued = pmodeler->LoadImage("sectorqueuedbmp", true);
        m_pimageCapitalWarning = pmodeler->LoadImage("sectorcapitalbmp", true);
        m_pimageSectorCombat = pmodeler->LoadImage("sectorcombatbmp", true);
        m_pimageSectorWarning = pmodeler->LoadImage("sectorwarningbmp", true);
        m_pimageBomberWarning = pmodeler->LoadImage("sectorbomberbmp", true);
        m_pimageSectorMiner = pmodeler->LoadImage("sectorminerbmp", true);
        m_pimageSectorEnemy = pmodeler->LoadImage("sectorenemybmp", true);
    }

    ~SectorMapPane()
    {
        SetVisible(false);
    }

    void CalcBounds()
    {
        m_bounds = m_pimageBkgnd->GetBounds();
    }

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    {
        return m_pimageBkgnd->HitTest(pprovider, point, bCaptured);
    }

    // <NKM>
    // Don't belive this is called? - comment out and just declare it - see if
    // we get unresolved symbol.
  //  void OnSessionLost(char * szReason, Time lastUpdate, Time now);
//    {
//        SetVisible(false);
//        //Clear the selected cluster without sending the advise
//        m_pClusterSel = NULL;
//        m_pSectorInfoPane->ClearCluster();
//    }

    void OnDelPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, QuitSideReason reason, const char* szMessageParam)
    {
        if (pPlayerInfo == trekClient.GetPlayerInfo())
        {
            SetVisible(false);

            //Clear the selected cluster without sending the advise
            m_pClusterSel = NULL;
            m_pSectorInfoPane->ClearCluster();
        }
    }

    void SelectCluster(IclusterIGC* pClusterSel)
    {
        if (pClusterSel == NULL)
        {
            IstationIGC*    pstation = trekClient.GetShip()->GetStation();
            if (pstation)
                pClusterSel = pstation->GetCluster();
        }

        if (pClusterSel != m_pClusterSel)
        {
            m_pClusterSel = pClusterSel;
            ZAssert(m_pSectorInfoPane);
            m_pSectorInfoPane->SelectCluster(pClusterSel);
            Changed();
        }
    }

    void SetVisible(bool bVisible)
    {
        if (m_bVisible != bVisible)
        {
            m_bVisible = bVisible;

            if (bVisible)
            {
                GetEngineWindow()->GetTimer()->AddSink(m_peventSinkTimer, 0.2f);

                SelectCluster(trekClient.GetChatCluster());
                m_pSectorInfoPane->SelectMouseOverCluster(NULL);
            }
            else
            {
                m_pClusterSel = NULL;
                ZAssert(m_pSectorInfoPane);
                m_pSectorInfoPane->SelectCluster(NULL);
                m_pSectorInfoPane->SelectMouseOverCluster(NULL);
                GetEngineWindow()->GetTimer()->RemoveSink(m_peventSinkTimer);
            }
        }
    }

    bool OnEvent(IEventSource* pevent)
    {
        Changed();
        const int cDivisor = 2;
        static int nTick = 0;
        ++nTick;
        if (nTick == cDivisor)
        {
            m_bFlashFrame = !m_bFlashFrame;
            nTick = 0;
        }
        return true;
    }

    void CalculateScreenMinAndMax()
    {
        // map the sector data
        bool    bFirstCluster = true;
        const ClusterListIGC* clusters = trekClient.m_pCoreIGC->GetClusters();
        for (ClusterLinkIGC* cLink = clusters->first(); cLink != NULL; cLink = cLink->next())
        {
            IclusterIGC* pCluster = cLink->data();
            if (pCluster->GetModels()->n() != 0)
            {
                float   x = pCluster->GetScreenX();
                float   y = pCluster->GetScreenY();
                if (bFirstCluster)
                {
                    bFirstCluster = false;

                    m_xClusterMin = m_xClusterMax = x;
                    m_yClusterMin = m_yClusterMax = y;
                }
                else
                {
                    if (x < m_xClusterMin)
                        m_xClusterMin = x;
                    else if (x > m_xClusterMax)
                        m_xClusterMax = x;

                    if (y < m_yClusterMin)
                        m_yClusterMin = y;
                    else if (y > m_yClusterMax)
                        m_yClusterMax = y;
                }
            }
        }

        const   float   c_bfr = 0.1f *
            std::max(0.0001f, std::max(m_xClusterMax - m_xClusterMin,
                m_yClusterMax - m_yClusterMin));
        m_xClusterMin -= c_bfr;
        m_xClusterMax += c_bfr;
        m_yClusterMin -= c_bfr;
        m_yClusterMax += c_bfr;

        // figure out the minimum and maximum screen coordinates
        m_xMin = m_xClusterMin;
        m_xMax = m_xClusterMax;
        m_yMin = m_yClusterMin;
        m_yMax = m_yClusterMax;

        float fDesiredAspectRatio = float(m_rectMap.XSize() - 2*c_nXBorder)/(m_rectMap.YSize() - 2*c_nYBorder);

        // scale it so that a map with height 2 will fit exactly
        const float fMapmakerHeight = 2.2f; // 10% fudge factor
        const float fMaxHeight = fMapmakerHeight;
        const float fMaxWidth = fMaxHeight * fDesiredAspectRatio;

        // if the map is bigger than we want to display, clip the view to the max size
        if (m_xMax - m_xMin > fMaxWidth || m_yMax - m_yMin > fMaxHeight)
        {
            m_bCanDrag = true;
            m_xMax = m_xMin + std::min(m_xMax - m_xMin, fMaxWidth);
            m_yMax = m_yMin + std::min(m_yMax - m_yMin, fMaxHeight);
        }
        else
        {
            m_bCanDrag = false;
        }

        //Preserve the aspect ratio
        float fAspectRatio = (m_xMax - m_xMin) / (m_yMax - m_yMin);

        if (fAspectRatio < fDesiredAspectRatio)
        {
            // grow the X size to correct the aspect ratio
            float fXGrowth = (fDesiredAspectRatio / fAspectRatio - 1) * (m_xMax - m_xMin);
            m_xMax += fXGrowth / 2;
            m_xMin -= fXGrowth / 2;
        }
        else if (fAspectRatio > fDesiredAspectRatio)
        {
            // grow the Y size to correct the aspect ratio
            float fYGrowth = (fAspectRatio / fDesiredAspectRatio - 1) * (m_yMax - m_yMin);
            m_yMax += fYGrowth / 2;
            m_yMin -= fYGrowth / 2;
        }

        // translate by the current drag offset
        m_xMin += m_xDrag;
        m_xMax += m_xDrag;
        m_yMin += m_yDrag;
        m_yMax += m_yDrag;
    }

    int ClusterHasFriendlyRipcord(IclusterIGC* pcluster, IsideIGC* pside, const ClusterListIGC&    clustersRipcord)
    {
        const StationListIGC* stationList = pcluster->GetStations();

        for (StationLinkIGC* stationLink = stationList->first();
            stationLink; stationLink=stationLink->next())
        {
            IstationIGC* pstation = stationLink->data();

            if (pstation->GetStationType()->HasCapability(c_sabmRipcord) &&
                ( (pstation->GetSide() == pside) ||
					(pstation->GetSide()->AlliedSides(pside,pstation->GetSide()) && trekClient.MyMission()->GetMissionParams().bAllowAlliedRip) ) ) //ALLY allow rip to allies 7/7/09 imago

            {
                return c_iClusterHasStationRipcord;
            }
        }

        float   ripcordSpeed;
        {
            IshipIGC*   pss = trekClient.GetShip()->GetSourceShip();
            if (pss->GetBaseHullType())
                ripcordSpeed = pss->GetHullType()->GetRipcordSpeed();
            else
                ripcordSpeed = -1.0f;
        }
        for (ProbeLinkIGC*  ppl = pcluster->GetProbes()->first(); (ppl != NULL); ppl = ppl->next())
        {
            IprobeIGC*  pprobe = ppl->data();
            if ( ( (pprobe->GetSide() == pside) || (pprobe->GetSide()->AlliedSides(pside,pprobe->GetSide()) && trekClient.MyMission()->GetMissionParams().bAllowAlliedRip) ) &&
				pprobe->GetCanRipcord(ripcordSpeed)) //ALLY allow rip to ally TP via. sector map imago 7/9/09
                return c_iClusterHasStationRipcord;
        }

        return clustersRipcord.find(pcluster) != NULL ? c_iClusterHasShipRipcord : c_iClusterHasNoRipcord;
    }

    IstationIGC* FindFriendlyStation(IclusterIGC* pcluster, IsideIGC* pside)
    {
        const StationListIGC* stationList = pcluster->GetStations();

        for (StationLinkIGC* stationLink = stationList->first();
            stationLink; stationLink=stationLink->next())
        {
            IstationIGC* pstation = stationLink->data();

            if (pstation->GetStationType()->HasCapability(c_sabmRestart) &&
                (pstation->GetSide() == pside)) //|| (pstation->GetSide()->AlliedSides(pside,pstation->GetSide()) && trekClient.MyMission()->GetMissionParams().bAllowAlliedRip)) ) //ALLY rip 7/9/09 imago
            {
                return pstation;
            }


        }

        return NULL;
    }

    bool    HasBuildableAsteroid(IclusterIGC*           pcluster,
                                 AsteroidAbilityBitMask aabm1,
                                 AsteroidAbilityBitMask aabm2)
    {
        AsteroidAbilityBitMask  aabm = (aabm1 != 0) ? aabm1 : aabm2;
        if ((aabm & ~c_aabmBuildable) != 0)
        {
            aabm &= ~c_aabmBuildable;
            for (AsteroidLinkIGC*   pal = pcluster->GetAsteroids()->first(); (pal != NULL); pal = pal->next())
            {
                if (pal->data()->HasCapability(aabm))
                    return true;
            }
        }
        else if (aabm == c_aabmBuildable)
        {
            for (AsteroidLinkIGC*   pal = pcluster->GetAsteroids()->first(); (pal != NULL); pal = pal->next())
            {
                if (pal->data()->HasCapability(aabm))
                    return true;
            }
        }

        return false;
    }

    bool HasCommandTarget(IclusterIGC* pCluster, Command cmd)
    {
        ImodelIGC* ptarget = trekClient.GetShip()->GetCommandTarget(cmd);

        if (ptarget)
        {
            if (ptarget->GetCluster() == pCluster)
                return true;
            else if (ptarget->GetObjectType() == OT_ship)
            {
                IshipIGC*   pship = (IshipIGC*)ptarget;
                PlayerInfo* ppi = (PlayerInfo*)(pship->GetPrivateData());
                if (ppi->LastSeenSector() == pCluster->GetObjectID())
                    return true;
            }
        }

        return false;
    }

    void Render(Context* pcontext)
    {
        pcontext->SetShadeMode(ShadeModeFlat);

        Rect rectClip = m_bounds.GetRect();
        rectClip.Expand(-1);
        pcontext->Clip(rectClip);
        CalculateScreenMinAndMax();

        // draw the background
        pcontext->DrawImage(m_pimageBkgnd->GetSurface());

        // draw all the connecting lines for warps
        {
            const WarpListIGC* warps =  trekClient.m_pCoreIGC->GetWarps();
            TVector<VertexL>    vertices(warps->n(), 0);
            TVector<WORD>       indices(warps->n(), 0);

            for (WarpLinkIGC* warpLink = warps->first(); warpLink != NULL; warpLink = warpLink->next())
            {
                IwarpIGC* pWarp = warpLink->data();
                IwarpIGC* pwarpDestination = pWarp->GetDestination();
				if (trekClient.MyMission()->GetMissionParams().bAllowAlliedViz && pwarpDestination == NULL) {  //ally VISIBILITY 7/11/09 imago
					continue;
				} else {
					assert (pwarpDestination != NULL);
				}

                if (pWarp->GetObjectID() > pwarpDestination->GetObjectID())
                {
                    Color colorWarp = 0.5f * Color::White();
                    WinPoint point1 = GetClusterPoint(pWarp->GetCluster());
                    WinPoint point2 = GetClusterPoint(pwarpDestination->GetCluster());

                    indices.PushEnd(vertices.GetCount());
                    vertices.PushEnd(VertexL(Vector(point1.x, point1.y, 0), colorWarp));
                    indices.PushEnd(vertices.GetCount());
                    vertices.PushEnd(VertexL(Vector(point2.x, point2.y, 0), colorWarp));
                }
            }

            if (vertices.GetCount() != 0)
            {
                pcontext->SetLineWidth(1.0f);
                pcontext->DrawLines(vertices, indices);
            }
        }

        ImodelIGC*      pmodelRipcord = trekClient.GetShip()->GetRipcordModel();
        IclusterIGC*    pclusterRipcord;
        IsideIGC*       pside = trekClient.GetSide();
        ClusterListIGC  clustersRipcord;

        if (pmodelRipcord)
        {
            {
                IshipIGC*       pshipSource = trekClient.GetShip()->GetSourceShip();
                IhullTypeIGC*   phtSource = pshipSource->GetBaseHullType();
                HullAbilityBitMask  habm = (phtSource && phtSource->HasCapability(c_habmCanLtRipcord))
                                           ? (c_habmIsRipcordTarget | c_habmIsLtRipcordTarget)
                                           : c_habmIsRipcordTarget;
				if (trekClient.MyMission()->GetMissionParams().bAllowAlliedRip) {
	                for (ShipLinkIGC*   psl = trekClient.m_pCoreIGC->GetShips()->first(); (psl != NULL); psl = psl->next())  //ALLY RIPCORD 7/10/09
					{
	                    IshipIGC*   pship = psl->data();
	                    if (pship != pshipSource && pship->GetSide()->AlliedSides(pside,pship->GetSide()))
	                    {
	                        IclusterIGC*    pc = trekClient.GetRipcordCluster(pship, habm);
	                        if (pc)
	                            clustersRipcord.last(pc);
	                    }
	                }
				} else {
	                for (ShipLinkIGC*   psl = pside->GetShips()->first(); (psl != NULL); psl = psl->next())  //not ally ripcord
					{
	                    IshipIGC*   pship = psl->data();
	                    if (pship != pshipSource)
	                    {
	                        IclusterIGC*    pc = trekClient.GetRipcordCluster(pship, habm);
	                        if (pc)
	                            clustersRipcord.last(pc);
	                    }
	                }
				}
            }

            pclusterRipcord = pmodelRipcord->GetCluster();
            if ((pclusterRipcord == NULL) && (pmodelRipcord->GetObjectType() == OT_ship))
            {
                PlayerInfo* ppi = (PlayerInfo*)(((IshipIGC*)pmodelRipcord)->GetPrivateData());
                if (ppi->StatusIsCurrent())
                    pclusterRipcord = trekClient.m_pCoreIGC->GetCluster(ppi->LastSeenSector());
            }
        }

        // draw the data for each sector
        const ClusterListIGC* clusters = trekClient.m_pCoreIGC->GetClusters();
        for (ClusterLinkIGC* cLink = clusters->first(); cLink != NULL; cLink = cLink->next())
        {
            IclusterIGC* pCluster = cLink->data();
            if (pCluster->GetModels()->n() != 0)
            {
                Point    xy = Point::Cast(GetClusterPoint(pCluster));

                // draw the sector outline Xynth #208 Draw in flashing Cyan if highlighted
				if (pCluster->GetHighlight() && !m_bFlashFrame)
					pcontext->DrawImage3D(m_pimageSectorEmpty->GetSurface(), Color::Cyan(), true, xy);
				else
					pcontext->DrawImage3D(m_pimageSectorEmpty->GetSurface(), Color::White(), true, xy);

                // color it by the owner(s), if any
                SideID sideOwner;
                SideID sideSecondaryOwner;
                GetClusterOwners(pCluster, sideOwner, sideSecondaryOwner);

                // draw the conflict state
                AssetMask am = pCluster->GetClusterSite()->GetClusterAssetMask();
                ClusterWarning warn = GetClusterWarning(am,
                    trekClient.MyMission()->GetMissionParams().bInvulnerableStations);

                if (warn >= c_cwStationThreatened)
                {
                    if (m_bFlashFrame)
                        pcontext->DrawImage3D(m_pimageSectorWarning->GetSurface(), Color::White(), true, xy);
                }
                else if (warn >= c_cwCapitalInCluster)
                {
                    if (m_bFlashFrame)
                        pcontext->DrawImage3D(m_pimageCapitalWarning->GetSurface(), Color::White(), true, xy);
                }
                else if (warn >= c_cwBomberInCluster)
                {
                    if (m_bFlashFrame)
                        pcontext->DrawImage3D(m_pimageBomberWarning->GetSurface(), Color::White(), true, xy);
                }
                else if (warn >= c_cwMinerThreatened)
                {
                    if (m_bFlashFrame)
                        pcontext->DrawImage3D(m_pimageSectorMiner->GetSurface(), Color::White(), true, xy);
                }
                else if (warn >= c_cwCombatInCluster)
                {
                    if (m_bFlashFrame)
                        pcontext->DrawImage3D(m_pimageSectorCombat->GetSurface(), Color::White(), true, xy);
                }
                /*
                else if (warn >= c_cwEnemyMinerInCluster)
                {
                    pcontext->DrawImage3D(m_pimageSectorMiner->GetSurface(), Color::White(), true, xy);
                }
                else if (warn > c_cwNoThreat)
                {
                    pcontext->DrawImage3D(m_pimageSectorEnemy->GetSurface(), Color::White(), true, xy);
                }
                */

                // highlight it if it is ours
                if (trekClient.GetChatCluster() == pCluster)
                    pcontext->DrawImage3D(m_pimageSectorHighlight->GetSurface(), Color::White(), true, xy);
                else if (m_pClusterSel == pCluster)
                    pcontext->DrawImage3D(m_pimageSectorSel->GetSurface(), Color::White(), true, xy);

                // highlight it if it contains our current or queued command target
                bool bHasCurrentCommand = HasCommandTarget(pCluster, c_cmdAccepted);
                bool bHasQueuedCommand = HasCommandTarget(pCluster, c_cmdQueued);

                if (bHasCurrentCommand)
                    pcontext->DrawImage3D(m_pimageSectorTargetHighlight->GetSurface(), Color::White(), true, xy);
                else if (bHasQueuedCommand)
                    pcontext->DrawImage3D(m_pimageSectorQueued->GetSurface(), Color::White(), true, xy);

                // overlay the ripcord info if we are ripcording
                if (pmodelRipcord)
                {
                    if (pmodelRipcord->GetObjectType() == OT_ship)
                    {
                        if (pCluster == pclusterRipcord)
                        {
                            if (m_bFlashFrame)
                                pcontext->DrawImage3D(m_pimageSectorPickTarget->GetSurface(), Color::White(), true, xy);
                            else
                                pcontext->DrawImage3D(m_pimageSectorPickable->GetSurface(), Color::White(), true, xy);
                        }
                        else
                        {
                            int cfr = ClusterHasFriendlyRipcord(pCluster, pside, clustersRipcord);
                            if (cfr != c_iClusterHasNoRipcord)
                            {
                                pcontext->DrawImage3D(cfr == c_iClusterHasStationRipcord
                                                      ? m_pimageSectorPickableStation->GetSurface()
                                                      : m_pimageSectorPickable->GetSurface(),
                                                      Color::White(), true, xy);
                            }
                        }
                    }
                    else
                    {
                        if (pCluster == pclusterRipcord)
                        {
                            if (m_bFlashFrame)
                                pcontext->DrawImage3D(m_pimageSectorPickStationTarget->GetSurface(), Color::White(), true, xy);
                            else
                                pcontext->DrawImage3D(m_pimageSectorPickableStation->GetSurface(), Color::White(), true, xy);
                        }
                        else
                        {
                            int cfr = ClusterHasFriendlyRipcord(pCluster, pside, clustersRipcord);
                            if (cfr != c_iClusterHasNoRipcord)
                            {
                                pcontext->DrawImage3D(cfr == c_iClusterHasStationRipcord
                                                      ? m_pimageSectorPickableStation->GetSurface()
                                                      : m_pimageSectorPickable->GetSurface(),
                                                      Color::White(), true, xy);
                            }
                        }
                    }
                }
                else if (GetWindow()->GetOverlayFlags() & ofTeleportPane)
                {
                    IstationIGC*    pstation = trekClient.GetShip()->GetStation();
                    if (pstation && (pstation->GetCluster() != pCluster) &&
                        (FindFriendlyStation(pCluster, pside) != NULL))
                    {
                        pcontext->DrawImage3D(m_pimageSectorPickable->GetSurface(), Color::White(), true, xy);
                    }
                }
                else if (HasBuildableAsteroid(pCluster,
                                              GetWindow()->GetCommandAsteroid(),
                                              GetWindow()->GetInvestAsteroid()))
                {
                    pcontext->DrawImage3D(m_pimageSectorPickable->GetSurface(), Color::White(), true, xy);
                }

                if (sideOwner != NA)
                {
                    pcontext->DrawImage3D(
                        m_pimageOwnerHighlight->GetSurface(),
                        trekClient.GetCore()->GetSide(sideOwner)->GetColor(),
                        true,
                        xy
                    );
                }

                if (sideSecondaryOwner != NA)
                {
                    pcontext->DrawImage3D(
                        m_pimageSecondaryOwnerHighlight->GetSurface(),
                        trekClient.GetCore()->GetSide(sideSecondaryOwner)->GetColor(),
                        true,
                        xy
                    );
                }

				if (pCluster->GetHighlight() && !m_bFlashFrame)  //Xynth #208 8/2010
				{  //Color center of circle cyan for highlight effect
					pcontext->DrawImage3D(
                        m_pimageSecondaryOwnerHighlight->GetSurface(),
                        Color::Cyan(),
                        true,
                        xy
                    );
				}

                // draw the dots for the ships
                {
                    int vnSidePopCount[c_cSidesMax];
                    int nSidesPresent = 0;

                    // count the population and sides in the sector
                    {
                        for (SideLinkIGC*   psl = trekClient.m_pCoreIGC->GetSides()->first();
                             (psl != NULL);
                             psl = psl->next())
                        {
                            SideID sideID = psl->data()->GetObjectID();

                            vnSidePopCount[sideID] = GetClusterPopulation(pCluster, psl->data());

                            if (vnSidePopCount[sideID] != 0)
                                ++nSidesPresent;
                        }
                    }


                    // draw the rows of dots
                    if (nSidesPresent > 0)
                    {
                        const int nDotHeight = 2;
                        const int nDotWidth = 2;
                        const int nShipWidth = 1;
                        const int nDotPitch = 1;
                        const int nRowPitch = 1;
                        const int nMaxPop = 7;

                        int nY = xy.Y() - (nSidesPresent * (nDotHeight + nDotPitch) - nRowPitch)/2;

                        for (SideLinkIGC*   psl = trekClient.m_pCoreIGC->GetSides()->first();
                             (psl != NULL);
                             psl = psl->next())
                        {
                            int nPopulation = std::min(nMaxPop, vnSidePopCount[psl->data()->GetObjectID()]);
                            int nX = xy.X() + 6;

                            if (nPopulation > 0)
                            {
                                while (nPopulation > 0)
                                {
                                    int nDotPopulation = std::min(nPopulation, nDotWidth/nShipWidth);

                                    pcontext->FillRect(
                                        WinRect(
                                            nX,
                                            nY,
                                            nX + nDotPopulation * nShipWidth,
                                            nY + nDotHeight
                                        ),
                                        psl->data()->GetColor()
                                    );

                                    nX += nDotWidth + nDotPitch;
                                    nPopulation -= nDotPopulation;
                                }
                                nY += nDotHeight + nRowPitch;
                            }
                        }
                    }
                }

                /*
                // draw the bars for the ships
                {
                    int nX = xy.X() + 6;
                    int nMaxPlayers = 32;
                    const int nMaxBarHeight = 8;
                    const int nBarWidth = (trekClient.m_pCoreIGC->GetSides()->n() > 2) ? 1 : 2;

                    for (SideLinkIGC*   psl = trekClient.m_pCoreIGC->GetSides()->first();
                         (psl != NULL);
                         psl = psl->next())
                    {
                        IsideIGC*   pside = psl->data();
                        SideID      sideID = pside->GetObjectID();
                        int nPopulation = GetClusterPopulation(pCluster, pside);
                        if (nPopulation > 0)
                        {
                            // draw bars for ships
                            int nBarHeight = std::min(std::min(nPopulation, nMaxBarHeight), std::max(1,
                                (int)((nMaxBarHeight - 1) * log((float)nPopulation)/log((float)nMaxPlayers))+ 1));

                            pcontext->FillRect(
                                WinRect(
                                    nX,
                                    xy.Y() - nMaxBarHeight/2,
                                    nX + nBarWidth,
                                    xy.Y() - nMaxBarHeight/2 + nBarHeight
                                ),
                                pside->GetColor()
                            );

                            nX += nBarWidth + 1;
                        }
                    }
                }
                */
            }
        }
    }


private:

    WinPoint GetClusterPoint(IclusterIGC* pcluster)
    {
        WinPoint pt = WinPoint(
            (int)(m_rectMap.XMin() + ((pcluster->GetScreenX() - m_xMin)/(m_xMax - m_xMin))*(m_rectMap.XSize() - 2*c_nXBorder) + c_nXBorder),
            (int)(m_rectMap.YMin() + ((pcluster->GetScreenY() - m_yMin)/(m_yMax - m_yMin))*(m_rectMap.YSize() - 2*c_nYBorder) + c_nYBorder)
            );

        return pt;
    }

    IclusterIGC* GetClusterAtPoint(const Point& point)
    {
        const ClusterListIGC* clusters = trekClient.m_pCoreIGC->GetClusters();
        IclusterIGC*  pClusterFound = NULL;
        int nMinDistance = 15;

        for (ClusterLinkIGC* cLink = clusters->first(); cLink != NULL; cLink = cLink->next())
        {
            IclusterIGC* pCluster = cLink->data();

            // if we know about this cluster...
            if (pCluster->GetModels()->n() != 0)
            {
                WinPoint    xy = GetClusterPoint(pCluster);

                Point pointSector((float)xy.X(), (float)xy.Y());
                Point offset = point - pointSector;

                int nDistance = sqrt(offset.LengthSquared());
                if (nDistance < nMinDistance)
                {
                    pClusterFound = pCluster;
                    nMinDistance = nDistance;
                }
            }
        }

        return pClusterFound;
    }

    void AttemptTeleportTo(IclusterIGC* pcluster)
    {
        // try teleporting to that station
        IstationIGC* pstation = FindFriendlyStation(pcluster, trekClient.GetSide());

        if (NULL == trekClient.GetShip()->GetStation())
        {
            // race condition?
        }
        else if (trekClient.GetShip()->GetStation()->GetCluster() == pcluster)
        {
            trekClient.PostText(false, "You are already in this cluster.");
            trekClient.PlaySoundEffect(errorSound);
        }
        else if (pstation == NULL)
        {
            trekClient.PostText(false, "You do not have an appropriate station in this cluster.");
            trekClient.PlaySoundEffect(errorSound);
        }
        else if (trekClient.GetShip()->GetParentShip() != NULL)
        {
            trekClient.DisembarkAndTeleport(pstation);

            GetWindow()->TurnOffOverlayFlags(ofTeleportPane);
        }
        else
        {
            trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
            BEGIN_PFM_CREATE(trekClient.m_fm, pfmDocked, C, DOCKED)
            END_PFM_CREATE

            pfmDocked->stationID = pstation->GetObjectID();

            trekClient.StartLockDown(
                "Teleporting to " + ZString(pstation->GetName()) + "....",
                BaseClient::lockdownTeleporting);

            GetWindow()->TurnOffOverlayFlags(ofTeleportPane);
        }
    }

    void ChildChanged(Value* pvalue, Value* pvalueNew)
    {
        ZAssert(NULL == pvalueNew);
        ZAssert(GetChild(0) == pvalue);
        pvalue->Update();
        if (trekClient.GetShip()->GetCluster() || trekClient.GetShip()->GetStation())
            SetVisible((((int)Number::Cast(pvalue)->GetValue()) & m_nMaskModeActive) != 0);
    }

    const char* GetClusterCursor()
    {
        if (trekClient.GetShip()->fRipcordActive()
                || (GetWindow()->GetOverlayFlags() & ofTeleportPane))
            return AWF_CURSOR_DEFAULT;
        else if (GetWindow()->GetConsoleImage()->GetConsoleData()->IsComposingCommand()
                || trekClient.GetShip()->GetCluster() != NULL)
            return "goto";
        else
            return AWF_CURSOR_DEFAULT;
    }

    void MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside)
    {
        if (bCaptured)
        {
            ZAssert(m_bDragging && m_bCanDrag);

            float fScale = (m_xMax - m_xMin)/m_rectMap.XSize();

            float fDeltaX = fScale * (m_pointLastDrag.X() - point.X());
            float fDeltaY = fScale * (m_pointLastDrag.Y() - point.Y());

            // make sure we don't drag the map off of the screen
            m_xDrag = std::max(std::min((m_xClusterMax - m_xClusterMin) - (m_xMax - m_xMin), m_xDrag + fDeltaX), 0.0f);
            m_yDrag = std::max(std::min((m_yClusterMax - m_yClusterMin) - (m_yMax - m_yMin), m_yDrag + fDeltaY), 0.0f);

            m_pointLastDrag = point;
            GetWindow()->SetCursor(AWF_CURSOR_DRAG);
            Changed();
        }
        else
        {
            IclusterIGC*  pClusterFound = GetClusterAtPoint(point);

            m_pSectorInfoPane->SelectMouseOverCluster(pClusterFound);

            if (pClusterFound)
            {
                m_bHovering = true;

                if (m_pClusterSel != pClusterFound)
                    trekClient.PlaySoundEffect(mouseoverSound);

                SelectCluster(pClusterFound);
                GetWindow()->SetCursor(GetClusterCursor());

                /*
                AssetMask am = pClusterFound->GetClusterSite()->GetClusterAssetMask();
                ClusterWarning cw = GetClusterWarning(am,
                    trekClient.MyMission()->GetMissionParams().bInvulnerableStations);
                trekClient.PostText(false, GetClusterWarningText(cw));
                */
            }
            else
            {
                m_bHovering = false;
                SelectCluster(trekClient.GetChatCluster());

                if (m_bCanDrag)
                {
                    GetWindow()->SetCursor(AWF_CURSOR_DRAG);
                }
                else
                {
                    GetWindow()->SetCursor(AWF_CURSOR_DEFAULT);
                }
            }
        }

        Changed();
    }

    virtual void MouseLeave(IInputProvider* pprovider)
    {
        m_bHovering = false;
        SelectCluster(trekClient.GetChatCluster());
        m_pSectorInfoPane->SelectMouseOverCluster(NULL);

        if (!m_bDragging)
            GetWindow()->SetCursor(AWF_CURSOR_DEFAULT);

        Changed();
    }

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        IclusterIGC*  pClusterFound = GetClusterAtPoint(point);

        if (bDown)
        {
			if (((GetKeyState(VK_CONTROL) & 0x8000) !=0) && (1 == button))  //Xynth #208 8/2010  Ctrl-Right click to highlight
			{
				if (pClusterFound)
				{
					if (trekClient.GetPlayerInfo()->IsTeamLeader())
					{  //commanders toggle and forward to team
						pClusterFound->SetHighlight(!pClusterFound->GetHighlight());

						bool newHighlight;
						newHighlight = pClusterFound->GetHighlight();

						trekClient.SetMessageType(BaseClient::c_mtGuaranteed);
						BEGIN_PFM_CREATE(trekClient.m_fm, pfmhighlight, CS, HIGHLIGHT_CLUSTER)
						END_PFM_CREATE
						pfmhighlight->clusterID = pClusterFound->GetObjectID();
						pfmhighlight->highlight = newHighlight;

					}
					else
						pClusterFound->SetHighlight(false);  //Only let players turn off highlight

				}
			}
            else if (0 == button)
            {
				// mmf 11/08 Don't allow pilots in a turret to do any of this (as in the below, namely changing viewed cluster).
				//           This addresses (until a better fix) the bug of eyeing enemy ships when in base, in a turret, and
				//           bomber pilot swtiches viewed sector
				//          added && (trekClient.GetShip()->GetParentShip() == NULL)
				//TheRock 4-1-2010 reverted, fixed on the server side.
                if (pClusterFound)
                {
                    trekClient.PlaySoundEffect(mouseclickSound);
                    SelectCluster(pClusterFound);

                    // if we are ripcording, set the target ripcord cluster
                    if (trekClient.GetShip()->fRipcordActive() && !trekClient.GetShip()->GetAutopilot())
                    {
                        trekClient.RequestRipcord(trekClient.GetShip(), pClusterFound);
                    }
                    // if we are teleporting, try to teleport to a station in
                    // the selected station.
                    else if (GetWindow()->GetOverlayFlags() & ofTeleportPane)
                    {
                        AttemptTeleportTo(pClusterFound);
                    }
                    else
                    {
                        // if we are in a station, change the view cluster
                        if (trekClient.GetShip()->GetCluster() == NULL)
                        {
                            ZAssert(trekClient.GetShip()->GetStation() != NULL);
                            trekClient.RequestViewCluster(pClusterFound);
                            GetWindow()->SetViewMode(TrekWindow::vmCommand);
                        }
                        else if (pClusterFound == trekClient.GetShip()->GetCluster())
                        {
                            // do nothing - users find anything else confusing.
                        }
                        else if (true)//!TrekWindow::CommandCamera(GetWindow()->GetCameraMode()))
                        {
                            // give the player a command to goto that sector
                            DataBuoyIGC db;

                            db.position = Vector(0, 0, 0);
                            db.clusterID = pClusterFound->GetObjectID();
                            db.type = c_buoyCluster;

                            IbuoyIGC*   b = (IbuoyIGC*)(trekClient.m_pCoreIGC->CreateObject(trekClient.m_now, OT_buoy, &db, sizeof(db)));
                            assert (b);
                            b->AddConsumer();

                            trekClient.SendChat(trekClient.GetShip(),
                                CHAT_INDIVIDUAL, trekClient.GetShipID(),
                                NA, NULL,
                                c_cidGoto, OT_buoy, b->GetObjectID(), b);

                            b->ReleaseConsumer();
                            b->Release();
                        }
                    }
                }
                else if (m_bCanDrag)
                {
                    // start a drag
                    m_bDragging = true;
                    m_pointLastDrag = point;
                    GetWindow()->SetCursor(AWF_CURSOR_DRAG);
                    Changed();
                    return MouseResultCapture();
                }
            }
            else if (1 == button && pClusterFound)
            {
                // let the console handle this
                trekClient.PlaySoundEffect(mouseclickSound);
                TrekWindow* pWindow = GetWindow ();
                pWindow->GetConsoleImage()->GetConsoleData()->PickCluster(pClusterFound, button);
                pWindow->GetInput ()->SetFocus (false);
            }
        }
        else
        {
            if (m_bDragging && 0 == button)
            {
                m_bDragging = false;

                if (pClusterFound)
                {
                    GetWindow()->SetCursor(GetClusterCursor());
                }
                else if (!bInside)
                {
                    GetWindow()->SetCursor(AWF_CURSOR_DEFAULT);
                }
                else
                {
                    GetWindow()->SetCursor(AWF_CURSOR_DRAG);
                }

                Changed();
                return MouseResultRelease();
            }
        }

        Changed();
        return MouseResult();
    }

    void OnClusterChanged(IclusterIGC* pcluster)
    {
        if (!m_bHovering)
        {
            SelectCluster(pcluster);
        }
    }
};

TRef<IObject> SectorMapPaneFactory::Apply(ObjectStack& stack)
{
    TRef<SectorInfoPane> pinfoPane; CastTo(pinfoPane, (Pane*)(IObject*)stack.Pop());
    TRef<Number> pnumberMode; CastTo(pnumberMode, (IObject*)stack.Pop());
    TRef<Number> pnumberMaskModeActive; CastTo(pnumberMaskModeActive, (IObject*)stack.Pop());
    TRef<Image> psectormap = new SectorMapPane(pinfoPane, pnumberMode, (int)pnumberMaskModeActive->GetValue());

    return (Value*)psectormap;
}
