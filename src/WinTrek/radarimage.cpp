#include "pch.h"
const char* c_szRadarLODs[] = {
                               "Radar mode: All",
                               "Radar mode: Default",
                               "Radar mode: Target",
                               "Radar mode: None"
                              };
                              /*
                               "Radar mode: Ships",
                               "Radar mode: Stations",
                               "Radar mode: Treasure",
                               "Radar mode: Asteroids",
                               };
                               */


static const char*  c_pszEmpty = "";
const float rangeClipLabels = 1000.0f;

const int   c_iNotUseful  = 0;
const int   c_iUsefulPart = 1;
const int   c_iUsefulTech = 2;

//////////////////////////////////////////////////////////////////////////////
//
// Peripheral Radar Image
//
//////////////////////////////////////////////////////////////////////////////

const int   c_maskOrder         = 0x01;
const int   c_maskTarget        = c_maskOrder << c_cmdCurrent;
const int   c_maskAccepted      = c_maskOrder << c_cmdAccepted;
const int   c_maskQueued        = c_maskOrder << c_cmdQueued;
const int   c_maskEnemy         = 0x08;
const int   c_maskThreat        = 0x10;
const int   c_maskFlash         = 0x20;
const int   c_maskRip           = 0x40; //Xynth #171 8/10
const int   c_maskMe            = 0x80;
const int   c_maskSubject       = 0x100;
const int   c_maskFlag          = 0x200;
const int   c_maskArtifact      = 0x400;
const int   c_maskHighlight     = 0x800;

class RadarImageImpl : public RadarImage {
    //////////////////////////////////////////////////////////////////////////////
    //
    // Types
    //
    //////////////////////////////////////////////////////////////////////////////

    class TextData
    {
    public:
        const char* m_pszName;
        int         m_range;
        float       m_shield;
        float       m_hull;
        float       m_fill;
        Command     m_cmd;
        CommandID   m_cid;
        Point       m_position;
        Point       m_direction;
        Color       m_color;

        TextData(const char*    pszName,
                 int            range,
                 float          shield,
                 float          hull,
                 float          fill,
                 Command        cmd,
                 CommandID      cid,
                 const Point&   position,
                 const Point&   direction,
                 const Color&   color)
        :
            m_pszName(pszName),
            m_range(range),
            m_shield(shield),
            m_hull(hull),
            m_fill (fill),
            m_cmd(cmd),
            m_cid(cid),
            m_position(position),
            m_direction(direction),
            m_color(color)
        {
        }

        TextData(const TextData& data)
        :
            m_pszName(data.m_pszName),
            m_range(data.m_range),
            m_shield(data.m_shield),
            m_hull(data.m_hull),
            m_fill(data.m_fill),
            m_cmd(data.m_cmd),
            m_cid(data.m_cid),
            m_position(data.m_position),
            m_direction(data.m_direction),
            m_color(data.m_color)
        {
        }
    };

    typedef TList<TextData> TextDataList;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Members
    //
    //////////////////////////////////////////////////////////////////////////////

    TextDataList        m_listTextData;

    TRef<Surface>       m_psurfaceLastFired;
    TRef<Surface>       m_psurfaceTargeted;
    TRef<Surface>       m_psurfaceLocked;
    TRef<Surface>       m_psurfacePartialLock;
    TRef<Surface>       m_psurfaceEnemy;
    TRef<Surface>       m_psurfaceMe;
    TRef<Surface>       m_psurfaceSubject;
    TRef<Surface>       m_psurfaceFlag;
    TRef<Surface>       m_psurfaceArtifact;
	TRef<Surface>       m_psurfaceRip;  //Xynth #171 8/10

    TRef<Surface>       m_psurfaceTechIcon;

    TRef<Surface>       m_psurfaceAccepted[c_cidMax];
    TRef<Surface>       m_psurfaceQueued[c_cidMax];

    RadarLOD            m_radarLOD;

    TVector<VertexL>    m_vertices;
    TVector<WORD>       m_indices;
    float               m_fRoundRadarRadius;

    static Color s_colorNeutral;

    //////////////////////////////////////////////////////////////////////////////
    //
    // Evaluate gets called when the view size changes
    //
    //////////////////////////////////////////////////////////////////////////////
    void        Evaluate (void)
    {
        Color   color (0.5f, 0.5f, 0.5f, 0.25f);

        Rect    rectImage = GetViewRect()->GetValue();
        {
            Point pointCenter = rectImage.Center();
            rectImage.Offset(-pointCenter);

            rectImage.Expand(-c_flBorderSide);
        }


        // establish the vertex and index array for the round radar circle, and set the indices
        const int   cCount = 60;
        const float fAngleStep = RadiansFromDegrees (360.0f / cCount);

        m_vertices.SetCount (cCount);
        m_indices.SetCount (cCount * 2);
        for (int j = 0; j < cCount; j++)
        {
            m_indices.Set (j * 2, j);
            m_indices.Set ((j * 2) + 1, (j + 1) % m_vertices.GetCount ());
        }

        {
            m_fRoundRadarRadius = ((rectImage.YMax () - rectImage.YMin ()) * 0.48f);

            // this recalculates the vertices of the round radar circle
            for (int i = 0; i < m_vertices.GetCount (); i++)
            {
                float   fTheta = fAngleStep * static_cast<float> (i),
                        fCosTheta = cosf (fTheta),
                        fSinTheta = sinf (fTheta);
                m_vertices.Set (i, VertexL (Vector (fCosTheta * m_fRoundRadarRadius, fSinTheta * m_fRoundRadarRadius, 0.0f), color));
            }
        }
        /*
        {
            m_vertices.Set (0, VertexL (Vector (rectImage.XMin(), rectImage.YMin(), 0.0f), color));
            m_vertices.Set (1, VertexL (Vector (rectImage.XMin(), rectImage.YMax(), 0.0f), color));
            m_vertices.Set (2, VertexL (Vector (rectImage.XMax(), rectImage.YMax(), 0.0f), color));
            m_vertices.Set (3, VertexL (Vector (rectImage.XMax(), rectImage.YMin(), 0.0f), color));
        }
        */
    }

public:
    //////////////////////////////////////////////////////////////////////////////
    //
    // Constructor
    //
    //////////////////////////////////////////////////////////////////////////////

    RadarImageImpl(Modeler* pmodeler, Viewport* pviewport) :
        RadarImage(pviewport),
        m_radarLOD(c_rlDefault)
    {
        m_psurfaceEnemy       = pmodeler->LoadSurface(AWF_FLIGHT_ENEMY_ICON, true);
        m_psurfaceMe          = pmodeler->LoadSurface(AWF_FLIGHT_ME_ICON, true);
        m_psurfaceTargeted    = pmodeler->LoadSurface(AWF_FLIGHT_TARGETED_ICON, true);
        m_psurfaceLocked      = pmodeler->LoadSurface(AWF_FLIGHT_MISSILE_LOCKED_ICON, true);
        m_psurfacePartialLock = pmodeler->LoadSurface(AWF_FLIGHT_PARTIAL_MISSILE_LOCK_ICON, true);
        m_psurfaceSubject     = pmodeler->LoadSurface(AWF_FLIGHT_SUBJECT_ICON, true);
        m_psurfaceFlag        = pmodeler->LoadSurface(AWF_FLIGHT_FLAG_ICON, true);
        m_psurfaceArtifact    = pmodeler->LoadSurface(AWF_FLIGHT_ARTIFACT_ICON, true);
		m_psurfaceRip         = pmodeler->LoadSurface(AWF_FLIGHT_RIP_ICON, true);

        m_psurfaceTechIcon    = pmodeler->LoadSurface("icontechbmp", true);

        for (CommandID i = c_cidAttack; (i < c_cidMax); i++)
        {
            if (c_cdAllCommands[i].szAccepted[0] != '\0')
                m_psurfaceAccepted[i]     = pmodeler->LoadSurface(c_cdAllCommands[i].szAccepted, true);

            if (c_cdAllCommands[i].szQueued[0] != '\0')
                m_psurfaceQueued[i]     = pmodeler->LoadSurface(c_cdAllCommands[i].szQueued, true);
        }
        m_psurfaceLastFired   = pmodeler->LoadSurface(AWF_FLIGHT_LASTFIRED_ICON, true);
    }


    void SetRadarLOD(RadarLOD radarLOD)
    {
        m_radarLOD = radarLOD;
    }
    RadarLOD    GetRadarLOD(void) const
    {
        return m_radarLOD;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw a blip expanded to fit around the object
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawExpandedBlip(Context* pcontext, float radius, Surface* psurface, const Color& color)
    {
        Point size  = Point::Cast(psurface->GetSize());
        float xsize = size.X();
        float ysize = size.Y();

        float xsizeMid = xsize / 2;
        float ysizeMid = ysize / 2;

        float delta = radius / sqrt2;
        {
            float   m = xsizeMid / 2;
            if (delta < m)
                delta = m;
        }

		pcontext->SetBlendMode(BlendModeSourceAlpha); //imago 7/15/09
        CD3DDevice9::Get()->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE );
        pcontext->DrawImage3D(psurface, Rect(       0,        0, xsizeMid, ysizeMid), color, true, Point(-delta, -delta));
        pcontext->DrawImage3D(psurface, Rect(xsizeMid,        0,    xsize, ysizeMid), color, true, Point( delta, -delta));
        pcontext->DrawImage3D(psurface, Rect(xsizeMid, ysizeMid,    xsize,    ysize), color, true, Point( delta,  delta));
        pcontext->DrawImage3D(psurface, Rect(       0, ysizeMid, xsizeMid,    ysize), color, true, Point(-delta,  delta));
        CD3DDevice9::Get()->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw a blip based on passed parameters
    //
    //////////////////////////////////////////////////////////////////////////////

    void DrawBlip(Context*      pcontext,
                  const Rect&   rectImage,
                  const Color&  colorIcon,
                  const Color&  colorOther,
                  unsigned char ucRadarState,
                  ThingSite*    pts,
                  Surface*      psurfaceIcon,
                  const char*   pszName,
                  int           range,
                  float         shield,
                  float         hull,
                  float         fill,
                  Command       cmd,
                  CommandID     cid,
                  int           maskBrackets,
                  float         lock,
                  bool          bIcon,
                  bool          bStats)
    {
        float               radiusObject = pts->GetScreenRadius();
        {
            static const float maxRadius = sqrt2 * 80.0f;
            if (radiusObject > maxRadius)
                radiusObject = maxRadius;
        }

        float radiusBracket = 1.05f * radiusObject;
        float distanceToEdge = pts->GetDistanceToEdge();

        //Clip the brackets so that they are always entirely on screen
        if (radiusBracket > distanceToEdge + c_flBorderSide)
            radiusBracket = distanceToEdge + c_flBorderSide;

        const Point&    positionObject = pts->GetScreenPosition();

        {

            //Draw the brackets around the "object"
            pcontext->Translate(positionObject);
            if (lock != 0.0f)
            {
                if (lock == 1.0f)
                    DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceLocked, Color::White());
                else
                    DrawExpandedBlip(pcontext, radiusBracket + 40.0f * (1.0f - lock), m_psurfacePartialLock, Color::White());
            }

            if (maskBrackets & c_maskEnemy)
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceEnemy, colorOther);

            if (maskBrackets & c_maskMe)
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceMe, colorOther);

            if (maskBrackets & c_maskSubject)
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceSubject, colorOther);

            if (maskBrackets & c_maskFlag)
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceFlag, colorOther);
            else if (maskBrackets & c_maskArtifact)
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceArtifact, colorOther);

			if (maskBrackets & c_maskRip)  //Xynth #171 8/10
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceRip, colorIcon);

			if (maskBrackets & c_maskTarget) {
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceTargeted, colorOther);
			}

            if (maskBrackets & c_maskThreat)
                DrawExpandedBlip(pcontext, radiusBracket, m_psurfaceLastFired, colorOther);
        }

        Point   directionCenter;
        {
            directionCenter.SetX(positionObject.X());
            directionCenter.SetY(GetWindow ()->GetRoundRadarMode()
                                   ? positionObject.Y()
                                   : (positionObject.Y() - rectImage.Size().Y() / 5.0f));

            float fLengthSquared = directionCenter.X() * directionCenter.X() + directionCenter.Y() * directionCenter.Y();

            if (fLengthSquared < 1.0f)
            {
                directionCenter.SetX(0.0f);
                directionCenter.SetY(-1.0f);
            }
            else
            {
                float   f = float(-1.0 / sqrt(fLengthSquared));
                directionCenter.SetX(directionCenter.X() * f);
                directionCenter.SetY(directionCenter.Y() * f);
            }
        }

        Point   positionIcon;
        if (ucRadarState != c_ucRadarOnScreenLarge)
        {
            positionIcon.SetX(0.0f);
            positionIcon.SetY(0.0f);
        }
        else
        {
            // the radius bracket is big, and the icon should appear near
            // the object instead of off it. Shrinking the radius bracket
            // accomplishes this.
            // XXX difficult to assess for now, Rob is inclined to leave the icon
            // XXX further away to avoid obscuring ships, so we agreed that I could
            // XXX check it in commented out.
            // radiusBracket *= 0.5f;

            //Get the vector from the object to the center of the screen
            //where this direction for something close to the center of the
            //screen is always straight down.
            {
                float   f = (distanceToEdge < radiusBracket) ? distanceToEdge : radiusBracket;

                positionIcon.SetX(directionCenter.X() * f);
                positionIcon.SetY(directionCenter.Y() * f);
            }
        }

        if (bIcon)
        {
            pcontext->Translate(positionIcon);
			pcontext->SetBlendMode(BlendModeAdd); //Imago 7/10 upgraded #181

            pcontext->DrawImage3D(psurfaceIcon, colorIcon, true);
        }

        if ((pszName[0] != '\0') || (range > 0) || bStats)
        {
            Point   positionLabel = positionObject + positionIcon;
            if (maskBrackets & (c_maskTarget | c_maskAccepted | c_maskThreat))
                m_listTextData.PushEnd(TextData(pszName, range, shield, hull, fill, cmd, cid, positionLabel, directionCenter, colorOther));
            else
                m_listTextData.PushFront(TextData(pszName, range, shield, hull, fill, cmd, cid, positionLabel, directionCenter, colorOther));
        }
    }


    //////////////////////////////////////////////////////////////////////////////
    //
    // Render one blip for each object in the cluster
    //
    //////////////////////////////////////////////////////////////////////////////
    static int UsefulTreasure(IshipIGC*    pship, ItreasureIGC* pt)
    {
        TreasureCode    tc = pt->GetTreasureCode();
        switch (tc)
        {
            case c_tcPowerup:
            {
                if (!pship->GetCluster())
                    return c_iNotUseful;
            }
            //no break

            case c_tcCash:
            case c_tcFlag:
            {
                return c_iUsefulPart;
            }
            break;

            case c_tcDevelopment:
            {
                IdevelopmentIGC*    pd = trekClient.m_pCoreIGC->GetDevelopment(pt->GetTreasureID());
                assert (pd);
                return pd->IsObsolete(trekClient.GetSide()->GetDevelopmentTechs())
                       ? c_iNotUseful
                       : c_iUsefulPart;     //It really is a tech, but use its icon instead of the special icon
            }
            break;

            default:
            {
                assert (tc == c_tcPart);

                IpartTypeIGC*       ppt = trekClient.m_pCoreIGC->GetPartType(pt->GetTreasureID());
                assert (ppt);
                EquipmentType       et  = ppt->GetEquipmentType();
                if (et == ET_Pack)
                {
                    if (pship->GetCluster() != NULL)
                    {
                        const DataPackTypeIGC*  pdata = (const DataPackTypeIGC*)(ppt->GetData());
                        if (pdata->packType == c_packFuel)
                        {
                            //Fuel is useful only if we have an afterburner
                            return (pship->GetMountedPart(ET_Afterburner, 0) != NULL)
                                   ? c_iUsefulPart
                                   : c_iNotUseful;
                        }
                        else
                        {
                            assert (pdata->packType == c_packAmmo);
                            Mount   m = pship->GetHullType()->GetMaxWeapons();

                            for (Mount i = 0; (i < m); i++)
                            {
                                IweaponIGC* pw = (IweaponIGC*)(pship->GetMountedPart(ET_Weapon, i));
                                if (pw && (pw->GetAmmoPerShot() != 0))
                                    return c_iUsefulPart;
                            }

                        }
                    }

                    return c_iNotUseful;
                }
                else
                {
                    if (!(ppt->GetEffectTechs() <= trekClient.GetSide()->GetDevelopmentTechs()))
                        return c_iUsefulTech;

                    if (IlauncherTypeIGC::IsLauncherType(et) && pship->GetCluster() && pship->GetHullType()->CanMount(ppt, 0))
                        return c_iUsefulPart;

                    return c_iNotUseful;
                }
            }
            break;
        }
    }

    void RenderBlips(Context* pcontext, const Rect& rectImage)
    {
        IclusterIGC*    pcluster = trekClient.GetCluster();
        if (pcluster == NULL)
            return;

        const ModelListIGC* models = pcluster->GetPickableModels();
        ZAssert(models != NULL);

        bool    bRangeLabels = !TrekWindow::CommandCamera(GetWindow()->GetCameraMode());
        assert (trekClient.GetShip()->GetCluster() || !bRangeLabels);

        //
        // Draw a blip for every model in the cluster
        //

        IshipIGC*       pshipSource = trekClient.GetShip()->GetSourceShip();
        const Vector&   vecPositionPlayer = pshipSource->GetPosition();
        float           radiusSource = pshipSource->GetRadius();

        ImodelIGC* pmodelOrders[c_cmdMax];

        {
            // For command targets not in the current sector ... show the user how to get there.

            for (Command i = 0; (i <= c_cmdCurrent); i++)       //Explicitly do not handle the case of planned
            {
                pmodelOrders[i] = trekClient.GetShip()->GetCommandTarget(i);
                if (pmodelOrders[i])
                {
                    IclusterIGC*    c = pmodelOrders[i]->GetCluster();

                    if (pmodelOrders[i]->GetObjectType() == OT_ship)
                    {
                        pmodelOrders[i] = ((IshipIGC*)pmodelOrders[i])->GetSourceShip();
                        if (c == NULL)
                            c = trekClient.GetCluster(trekClient.GetShip(), pmodelOrders[i]);
                    }

                    if (c == NULL)
                        pmodelOrders[i] = NULL;
                    else
                    {
                        IclusterIGC*    pcluster = trekClient.GetShip()->GetCluster();

                        if (pcluster && (c != pcluster))
                        {
                            // The target is not in the sector ... find out how to get to the target
                            pmodelOrders[i] = FindPath(trekClient.GetShip(), pmodelOrders[i], false);
                        }
                    }
                }
            }
        }

        const ShipListIGC*  psubjects;
        if (bRangeLabels)
            psubjects = NULL;
        else
            psubjects = GetWindow()->GetConsoleImage()->GetSubjects();

        IsideIGC*   psideMine = trekClient.GetShip()->GetSide();
        float       capacity = trekClient.m_pCoreIGC->GetFloatConstant(c_fcidCapacityHe3) *
                               psideMine->GetGlobalAttributeSet().GetAttribute(c_gaMiningCapacity);

        Time        now = Time::Now();

        ImodelIGC*  pmodelEnemy = bRangeLabels
                                     ? FindTarget(pshipSource, c_ttShip | c_ttEnemy | c_ttNearest)
                                     : NULL;
        for (ModelLinkIGC* l = models->first();
             (l != NULL); l = l->next())
        {
            ImodelIGC* pmodel = l->data();

            if (pmodel->GetVisibleF() && pshipSource->CanSee(pmodel))
            {
                ObjectType      type = pmodel->GetObjectType();
                Surface*        psurfaceIcon = (Surface*)(pmodel->GetIcon());
                if (psurfaceIcon)
                {
                    int   maskBrackets = 0x00;

                    const char*     pszName = pmodel->GetName();
                    Command         cmd;
                    CommandID       cidOrder = c_cidNone;
                    {
                        for (Command i = 0; (i < c_cmdMax); i++)
                        {
                            if (pmodel == pmodelOrders[i])
                            {
                                maskBrackets |= (c_maskOrder << i);
                                CommandID   cid = trekClient.GetShip()->GetCommandID(i);
                                if (cid != c_cidNone)
                                {
                                    cmd = i;
                                    cidOrder = cid;
                                }
                            }
                        }
                    }

                    float lock    = 0.0f;
                    if (maskBrackets & c_maskTarget)
                    {
                        ImagazineIGC* magazine = (ImagazineIGC*)(trekClient.GetShip()->GetMountedPart(ET_Magazine, 0));
                        if (magazine)
                            lock = magazine->GetLock();
                    }

                    float           separation = (pmodel->GetPosition() - vecPositionPlayer).Length();

                    int             range;
                    {
                        if ((pmodel == pshipSource) || !bRangeLabels)
                            range = 0;
                        else
                            range = int(separation + 0.5f);
                    }
                    separation -= radiusSource + pmodel->GetRadius();

                    if (type == OT_treasure)
                    {
                        if (((ItreasureIGC*)pmodel)->GetTreasureCode() == c_tcFlag)
                            maskBrackets |= c_maskHighlight;
                    }
                    else
                    {
                        SideID  sidFlag = pmodel->GetFlag();
                        if (sidFlag != NA)
                            maskBrackets |= (sidFlag == SIDE_TEAMLOBBY) ? c_maskArtifact : c_maskFlag;
                    }

                    ThingSite*  pts = pmodel->GetThingSite();
                    unsigned char ucRadarState = pts->GetRadarState();

                    if (psubjects && (type == OT_ship) && psubjects->find((IshipIGC*)pmodel))
                        maskBrackets |= c_maskSubject;

					IsideIGC* sthis = pmodel->GetSide();
					if (pmodelEnemy) {
						IsideIGC* sthat = pmodelEnemy->GetSide();
                    	if (pmodel == pmodelEnemy && !sthis->AlliedSides(sthis,sthat))  //ALLY - imago 7/3/09
                        	maskBrackets |= c_maskEnemy;
					}

                    {
                        const DamageBucketList* b = pmodel->GetDamageBuckets();
                        if (b->n() != 0)
                        {
                            assert (b->n() == 1);
                            DamageBucket*  db = b->first()->data();

                            maskBrackets |= (db->flash(now))
                                            ? (c_maskThreat | c_maskFlash)
                                            : c_maskThreat;
                        }
                    }

                    //
                    // Color for the side
                    //
                    IsideIGC*       pside = pmodel->GetSide();
                    if (pmodel == pshipSource)
                        maskBrackets |= c_maskMe;

                    Color color = pside
                                  ? ((maskBrackets & c_maskFlash) ? Color::Red() : pside->GetColor())
                                  : s_colorNeutral;



                    /*
                    static const int    closeRange = 100;
                    static const int    farRange = 1000;
                    static const float  closeAlpha = 0.75f;
                    static const float  farAlpha = 0.25f;


                    color.SetAlpha((range < closeRange)
                                   ? closeAlpha
                                   : ((range > farRange)
                                      ? farAlpha
                                      : (farAlpha +
                                         (float(range - farRange) * (closeAlpha - farAlpha) / float(closeRange - farRange)))));
                    */

                    bool    bIcon = true;
                    bool    bLabel = ((maskBrackets & (c_maskTarget |
                                                       c_maskAccepted |
                                                       c_maskQueued |
                                                       c_maskHighlight |
                                                       c_maskThreat |
                                                       c_maskFlag |
                                                       c_maskArtifact)) != 0);
                    bool    bStats = bLabel;

                    if (bLabel)
                    {
                        if ((type == OT_treasure) &&
                            (UsefulTreasure(pshipSource, (ItreasureIGC*)pmodel) == c_iUsefulTech))
                        {
                            psurfaceIcon = m_psurfaceTechIcon;
                        }
                    }
                    else
                    {
                        if (m_radarLOD == c_rlMinimal)
                        {
                            if (pmodel != pshipSource)
                            {
                                bIcon = false;
                                range = 0;
                            }
                        }
                        else
                        {
                            switch (type)
                            {
                                case    OT_ship:
                                {
                                    switch (m_radarLOD)
                                    {
                                        case c_rlAll:
                                        {
                                            bLabel = bStats = true;
                                            if ((ucRadarState == c_ucRadarOffScreen) && (pside == psideMine))
                                                range = 0;
                                        }
                                        break;
                                        case c_rlDefault:
                                        {
                                            if ((ucRadarState != c_ucRadarOffScreen) || (separation < rangeClipLabels))
                                                bLabel = true;

                                            if (pside == psideMine)
                                                range = 0;
                                            else
                                                bStats = true;
                                        }
                                        break;
                                        case c_rlTarget:
                                        {
                                            if ( (pside == psideMine) || pside->AlliedSides(pside,psideMine) ) //ALLY - Imago 7/3/09
                                            {
                                                if (pmodel != pshipSource)
                                                {
                                                    bIcon = false;
                                                    range = 0;
                                                }
                                            }
                                            else
                                            {
                                                if (ucRadarState != c_ucRadarOffScreen)
                                                    bStats = true;

                                                if (separation < rangeClipLabels)
                                                    bLabel = true;
                                            }
                                        }
                                        break;
                                    }
                                }
                                break;

                                case    OT_station:
                                {
                                    if (((IstationIGC*)pmodel)->GetBaseStationType()->HasCapability(c_sabmPedestal))
                                    {
                                        if (separation >= rangeClipLabels)
                                        {
                                            bIcon = bLabel = bStats = false;
                                            range = 0;
                                        }
                                    }
                                    else
                                    {
                                        switch (m_radarLOD)
                                        {
                                            case c_rlAll:
                                            {
                                                bLabel = bStats = true;
                                            }
                                            break;
                                            case c_rlDefault:
                                            {
                                                if ((separation >= rangeClipLabels) || (ucRadarState == c_ucRadarOffScreen))
                                                    range = 0;
                                            }
                                            break;
                                            case c_rlTarget:
                                            {
                                                range = 0;
                                                if ( ((pside == psideMine) || pside->AlliedSides(pside,psideMine)) && (ucRadarState == c_ucRadarOffScreen) ) //ALLY imag0 7/3/09
                                                    bIcon = false;
                                            }
                                            break;
                                        }
                                    }
                                }
                                break;

                                case    OT_missile:
                                {
                                    if (m_radarLOD != c_rlAll)
                                    {
                                        ImodelIGC*  pmodelTarget = ((ImissileIGC*)pmodel)->GetTarget();

                                        if ((pmodelTarget == NULL) || (pmodelTarget->GetSide() != psideMine)) //ALLYTD?
                                        {
                                            bIcon = false;
                                            range = 0;
                                        }
                                    }
                                }
                                break;

                                case    OT_probe:
                                case    OT_mine:
                                {
                                    switch (m_radarLOD)
                                    {
                                        case c_rlAll:
                                        {
                                            if ((pside == psideMine) || pside->AlliedSides(pside,psideMine) ) //ALLY imago 7/3/09
                                            {
                                                if ((ucRadarState == c_ucRadarOffScreen) && (separation >= rangeClipLabels))
                                                    range = 0;
                                            }
                                            else
                                            {
                                                if (ucRadarState != c_ucRadarOffScreen)
                                                    bLabel = true;
                                            }

                                            bStats = true;
                                        }
                                        break;
                                        case c_rlDefault:
                                        {
                                            if ((pside == psideMine) || pside->AlliedSides(pside,psideMine) ) //ALLY imago 7/3/09
                                            {
                                                if (separation >= rangeClipLabels)
                                                    range = 0;
                                            }
                                        }
                                        break;
                                        case c_rlTarget:
                                        {
                                            if ((pside == psideMine) || (separation >= rangeClipLabels) || (pside->AlliedSides(pside,psideMine))) //ALLY imago 7/3/09
                                            {
                                                bIcon = false;
                                                range = 0;
                                            }
                                        }
                                        break;
                                    }
                                }
                                break;

                                case    OT_treasure:
                                {
                                    int iUseful = UsefulTreasure(pshipSource, (ItreasureIGC*)pmodel);
                                    if (iUseful != c_iNotUseful)
                                    {
                                        bLabel = (m_radarLOD == c_rlAll);
                                        if ((m_radarLOD == c_rlTarget) && (separation >= rangeClipLabels))
                                        {
                                            bIcon = false;
                                            range = 0;
                                        }
                                        else if (iUseful == c_iUsefulTech)
                                        {
                                            psurfaceIcon = m_psurfaceTechIcon;
                                        }
                                    }
                                    else if (m_radarLOD != c_rlAll)
                                    {
                                        bIcon = false;
                                        range = 0;
                                    }
                                }
                                break;

                                case    OT_asteroid:
                                {
                                    switch (m_radarLOD)
                                    {
                                        case c_rlAll:
                                        {
                                            if ((pszName[0] == '\0') || (pszName[0] == 'A'))
                                            {
                                                if (separation >= rangeClipLabels)
                                                {
                                                    bIcon = false;
                                                    range = 0;
                                                }
                                                else if (range == 0)
                                                    bIcon = false;
                                            }
                                            else
                                                bLabel = bStats = true;
                                        }
                                        break;

                                        case c_rlDefault:
                                        {
                                            if ((pszName[0] == '\0') || (pszName[0] == 'A'))
                                            {
                                                if (separation >= rangeClipLabels)
                                                {
                                                    bIcon = false;
                                                    range = 0;
                                                }
                                                else if (range == 0)
                                                    bIcon = false;
                                            }
                                        }
                                        break;

                                        case c_rlTarget:
                                        {
                                            if (separation >= rangeClipLabels)
                                            {
                                                bIcon = false;
                                                range = 0;
                                            }
                                            else if (range == 0)
                                                bIcon = false;
                                        }
                                        break;
                                    }
                                }
                                break;

                                case    OT_warp:
                                {
                                    bLabel = (m_radarLOD <= c_rlDefault);
                                    if ((m_radarLOD == c_rlTarget) && (separation >= rangeClipLabels))
                                        range = 0;
                                }
                                break;
                            }
                        }
                    }

                    float   shield = 2.0f;
                    float   hull = 2.0f;
                    float   fill = 2.0f;
                    if (!bLabel)
                    {
                        pszName = c_pszEmpty;
                    }
                    else if (pszName[0] == '\0')
                    {
                        pszName = GetModelName(pmodel);
                    }


                    if (bStats)
                    {
                        switch(type)
                        {
                            case OT_ship:
                            {
                                IshipIGC*   pship = static_cast<IshipIGC*> (pmodel);
                                IshieldIGC* pshield = static_cast<IshieldIGC*> (pship->GetMountedPart(ET_Shield, 0));
                                shield = pshield ? pshield->GetFraction() : 2.0f;
                                hull = ((IdamageIGC*)pmodel)->GetFraction();
                                // this really only works if the server updates the client with
                                // the information. We would have to add a new message type
                                // for that to happen, so for now I'm just commenting it out and
                                // moving on.
                                //Xynth #156 7/2010 Uncomment and add condition for only own team to see
								if ((pship->GetPilotType () == c_ptMiner) && (pship->GetSide() == psideMine))
                                {
                                    fill = pship->GetOre () / capacity;
                                    if (fill > 1.0f)
                                        fill = 1.0f;
                                }

								//Xynth #47 7/2010
								if (((pship->GetStateM() & droneRipMaskIGC) != 0) &&
									 (pship->GetSide() == psideMine) &&
									 (pship->GetPilotType() < c_ptPlayer))  //Xynth #175 7/2010
								{
									maskBrackets |= c_maskRip; //Xynth #171 8/10
								}

                            }
                            break;

                            case OT_station:
                            {
                                shield = ((IstationIGC*)pmodel)->GetShieldFraction();
                                hull = ((IdamageIGC*)pmodel)->GetFraction();
                            }
                            break;

                            case OT_asteroid:
                            {
                                hull = ((IdamageIGC*)pmodel)->GetFraction();
                                //Xynth #100 2010
								//float   ore = ((IasteroidIGC*)pmodel)->GetOre();
								float   ore = ((IasteroidIGC*)pmodel)->GetOreSeenBySide(psideMine);
                                if (ore != 0.0f)
                                {
                                    // this is displaying how many shiploads of ore are on the asteroid.
                                    // more than two is shown as full.
                                    fill = ore / (2.0f * capacity);
                                    if (fill > 1.0f)
                                        fill = 1.0f;

									//Xynth #104 Color asteroids white/gray to match minimap
									//This function not reliable for client graphics
									//if (((IasteroidIGC*)pmodel)->GetAsteroidCurrentEye(psideMine))
										color = Color::White();
									//else
									//	color = Color::Gray();

                                }
                            }
                            break;

                            case OT_mine:
                            {
                                fill    = ((ImineIGC*)pmodel)->GetTimeFraction();
                            }
                            break;

                            case OT_probe:
                            {
                                fill    = ((IprobeIGC*)pmodel)->GetTimeFraction();
                                hull    = ((IprobeIGC*)pmodel)->GetFraction();
                            }
                            break;
                        }
                    }

                    //
                    // Draw the Blip
                    //


					Color   colorOther;
						colorOther = color;

                    if ((maskBrackets & (c_maskTarget | c_maskAccepted | c_maskThreat | c_maskHighlight | c_maskFlag | c_maskArtifact)) == 0)
                        colorOther = (maskBrackets & c_maskQueued)
                                     ? (colorOther * 0.75f)
                                     : (colorOther * 0.5f);

                    pcontext->PushState();

                    DrawBlip(pcontext,
                             rectImage,
                             color,
                             colorOther,
                             ucRadarState,
                             pts,
                             psurfaceIcon,
                             pszName,
                             range,
                             shield,
                             hull,
                             fill,
                             cmd,
                             cidOrder,
                             maskBrackets,
                             lock,
                             bIcon,
                             bStats);

                    pcontext->PopState();
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Draw a circle bordering the round radar
    //
    //////////////////////////////////////////////////////////////////////////////

	void RenderRoundBorder (Context* pcontext, const Rect& rectImage)
    {
        pcontext->PushState ();
        pcontext->SetClipping (false);
        pcontext->SetBlendMode (BlendModeSourceAlpha);
        pcontext->DrawLines(m_vertices, m_indices);
        pcontext->PopState ();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Render the image
    //
    //////////////////////////////////////////////////////////////////////////////

    void Render(Context* pcontext)
    {
		if (m_radarLOD != c_rlNone)
        {
            //
            // Use flat shading
            //
            pcontext->SetShadeMode(ShadeModeFlat);
            pcontext->SetLinearFilter(false, true);
            CD3DDevice9::Get()->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
            CD3DDevice9::Get()->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
            CD3DDevice9::Get()->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );

			//
			// Shrink the rect size by half the size of the gauge bitmap
			// and center the rect around the origin
			//
            Rect  rectImage   = GetViewRect()->GetValue();
            {
			    Point pointCenter = rectImage.Center();

			    pcontext->Translate(pointCenter);
            }

			//
			// Draw the round border if appropriate
            //
            if (GetWindow ()->GetRoundRadarMode ())
			    RenderRoundBorder (pcontext, rectImage);

			//
			// Draw some Blips
            //

			RenderBlips(pcontext, rectImage);

            //
            // Draw the text
            //

            TRef<IEngineFont> pfont = TrekResources::SmallFont();

            float heightFont = float(pfont->GetHeight());

            TextDataList::Iterator iter(m_listTextData);
            while (!iter.End())
            {
                const TextData& data = iter.Value();

                float   lines = 0.0f;
                float   width = 0.0f;
                if (data.m_pszName[0] != '\0')
                {
                    float   w = (float)(pfont->GetTextExtent(data.m_pszName).X());
                    if (w > width)
                        width = w;

                    lines += 1.0f;
                }

                char    szRange[20];
                if (data.m_range > 0)
                {
                    _itoa(data.m_range, szRange, 10);
                    float   w = (float)(pfont->GetTextExtent(szRange).X());
                    if (w > width)
                        width = w;

                    lines += 1.0f;
                }

                if (data.m_shield <= 1.0f)
                {
                    lines += 0.5f;
                }
                if (data.m_hull <= 1.0f)
                {
                    lines += 0.5f;
                }
                if (data.m_fill <= 1.0f)
                {
                    lines += 0.5f;
                }

                Surface*    psurfaceIcon = NULL;
                float       xshift;

                if (data.m_cid != c_cidNone)
                {
                    assert (data.m_cid >= 0);
                    assert (data.m_cid < c_cidMax);
                    psurfaceIcon = (data.m_cmd != c_cmdQueued) ? m_psurfaceAccepted[data.m_cid] :  m_psurfaceQueued[data.m_cid];
                    if (psurfaceIcon)
                    {
                        Point size  = Point::Cast(psurfaceIcon->GetSize());
                        xshift = size.X();
                        width += xshift;
                    }
                }

                //Find the offset to center the text at position + offset
                Point   offset(data.m_position.X() - 0.5f * width, data.m_position.Y()  + heightFont * 0.5f * (lines - 2.0f));

                //Adjust the offset by the amount required to move a point from the center to the edge
                //in the specified direction
                {
                    float   w = width + 15.0f;
                    float   h = heightFont * lines + 15.0f;

                    float   x = data.m_direction.X();
                    float   y = data.m_direction.Y();

                    if (x == 0.0f)
                        y = (y < 0.0f) ? -h : h;
                    else if (y == 0.0f)
                        x = (x <= 0.0f) ? -w : w;
                    else
                    {
                        float   tX = w / float(fabs(x));
                        float   tY = h / float(fabs(y));
                        float   tMin = (tX < tY) ? tX : tY;

                        x *= tMin;
                        y *= tMin;
                    }

                    offset.SetX(offset.X() + x * 0.5f);
                    offset.SetY(offset.Y() + y * 0.5f);

                    if (psurfaceIcon)
                    {
                        offset.SetX(offset.X() + xshift * 0.5f);
                        pcontext->DrawImage3D(psurfaceIcon, data.m_color, true, offset);
                        offset.SetX(offset.X() + xshift * 0.5f);
                    }
                }

                if (data.m_pszName[0] != '\0')
                {
                    pcontext->DrawString(pfont, data.m_color, offset, ZString(data.m_pszName));
                    offset.SetY(offset.Y() - heightFont);
                }

                if (data.m_range > 0)
                {
                    pcontext->DrawString(pfont, data.m_color, offset, ZString(szRange));
                    offset.SetY(offset.Y() - heightFont);
                }

                if ((data.m_hull <= 1.0f) || (data.m_fill <= 1.0f))
                {
                    CD3DDevice9::Get()->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, FALSE ); //8/8/09 Imago
                    const float c_width = 16.0f;

                    float   xOffset = float(floor(offset.X()));
                    float   yOffset = float(floor(offset.Y() + heightFont - 4.0f));

                    // draw the ore fill state if needed
                    if (data.m_fill <= 1.0f)
                    {
                        Rect    rectFill  (xOffset, yOffset, xOffset + (c_width * data.m_fill), yOffset + 2.0f);
                        pcontext->FillRect (rectFill, data.m_color);
                        // if the value isn't 1.0f, the bar has two parts, here we draw the "empty" part
                        if (data.m_fill < 1.0f)
                        {
                            rectFill.SetXMin(rectFill.XMax());
                            rectFill.SetXMax(xOffset + c_width);
                            pcontext->FillRect(rectFill, Color(0.333f, 0.0f, 0.0f));
                        }
                        yOffset -= 4.0f;
                    }

                    // if shield is valid, draw it and update the hull bar location
                    if (data.m_shield <= 1.0f)
                    {
                        Rect    rectShield (xOffset, yOffset, xOffset + c_width * data.m_shield, yOffset + 2.0f);
                        pcontext->FillRect (rectShield, data.m_color);
                        // if the value isn't 1.0f, the bar has two parts, here we draw the "empty" part
                        if (data.m_shield < 1.0f)
                        {
                            rectShield.SetXMin(rectShield.XMax());
                            rectShield.SetXMax(xOffset + c_width);
                            pcontext->FillRect(rectShield, Color(1.0f, 0.0f, 0.0f));
                        }
                        yOffset -= 4.0f;
                    }

                    // if hull is valid, so draw it and update the hull bar location
                    if (data.m_hull <= 1.0f)
                    {
                        Rect    rectHull  (xOffset, yOffset, xOffset + c_width * data.m_hull,   yOffset + 2.0f);
                        pcontext->FillRect(rectHull, data.m_color);
                        // if the value isn't 1.0f, the bar has two parts, here we draw the "empty" part
                        if (data.m_hull < 1.0f)
                        {
                            rectHull.SetXMin(rectHull.XMax());
                            rectHull.SetXMax(xOffset + c_width);
                            pcontext->FillRect(rectHull, Color(1.0f, 0.0f, 0.0f));
                        }
                    }
                    CD3DDevice9::Get()->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE ); //8/8/09 Imago
                }

                iter.Next();
            }

            m_listTextData.SetEmpty();
            CD3DDevice9::Get()->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
            CD3DDevice9::Get()->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
            CD3DDevice9::Get()->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP );
        }
    }
};

Color RadarImageImpl::s_colorNeutral(1, 1, 1);

//////////////////////////////////////////////////////////////////////////////
//
// RadarImage Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<RadarImage> RadarImage::Create(Modeler* pmodeler, Viewport* pviewport)
{
    return new RadarImageImpl(pmodeler, pviewport);
}
