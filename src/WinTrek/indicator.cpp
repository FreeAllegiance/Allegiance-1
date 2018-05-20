#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// The pointer geometry
//
//////////////////////////////////////////////////////////////////////////////

float height = 30;
float base = 0.25f * height;
float c = sqrt(3.0f) / 2.0f;
float s = 0.5f;
float x = c * base;
float y = s * base;
float zn = 0.5f * height;
float zf = -0.5f * height;

Vector vector1(0, base, zn);
Vector vector2(-x, -y, zn);
Vector vector3(x, -y, zn);
Vector vector4(0, 0, zf);

float normalLength = sqrt(0.125f * 0.125f + 1.0f);
float normalPlane = 1.0f / normalLength;
float normalZ = 0.125f / normalLength;

Vector normal1(0, 0, 1);
Vector normal2(-c * normalPlane, s * normalPlane, -normalZ);
Vector normal3(0, -normalPlane, -normalZ);
Vector normal4(c * normalPlane, s * normalPlane, -normalZ);

Vertex g_pvertex[12] = {
    Vertex(vector1, normal1),
    Vertex(vector3, normal1),
    Vertex(vector2, normal1),

    Vertex(vector4, normal2),
    Vertex(vector1, normal2),
    Vertex(vector2, normal2),

    Vertex(vector4, normal3),
    Vertex(vector2, normal3),
    Vertex(vector3, normal3),

    Vertex(vector4, normal4),
    Vertex(vector3, normal4),
    Vertex(vector1, normal4),
};

WORD g_pindex[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

/*
Color color1(1, 1, 1);
Color color2(1, 0, 0);
Color color3(0, 1, 0);
Color color4(0, 0, 1);

VertexL g_pvertexL[12] = {
VertexL(vector1, color1),
VertexL(vector3, color1),
VertexL(vector2, color1),

VertexL(vector4, color2),
VertexL(vector1, color2),
VertexL(vector2, color2),

VertexL(vector4, color3),
VertexL(vector2, color3),
VertexL(vector3, color3),

VertexL(vector4, color4),
VertexL(vector3, color4),
VertexL(vector1, color4),
};
*/

//////////////////////////////////////////////////////////////////////////////
//
// Indicator Image
//
//////////////////////////////////////////////////////////////////////////////

class IndicatorImage : public Image {
    TRef<Surface>    m_psurfaceDirection;
    TRef<Surface>    m_psurfaceDirectionIn;
    TRef<Surface>    m_psurfaceCenterInRange;
    TRef<Surface>    m_psurfaceCenterOutRange;
    TRef<Surface>    m_psurfaceLeadInRange;
    TRef<Surface>    m_psurfaceLeadOutRange;
    TRef<Camera>     m_pcamera;
    TRef<Geo>        m_pgeoPointer;
    TRef<Geo>        m_pgeoJoystickPointer;
    TRef<Material>   m_pmaterial;

    Viewport*  GetViewport() { return Viewport::Cast(GetChild(0)); }
    Camera*    GetCamera() { return GetViewport()->GetCamera(); }
    RectValue* GetViewRect() { return GetViewport()->GetViewRect(); }

    Number*    GetTime() { return Number::Cast(GetChild(1)); }

    TRef<Surface>    m_psurfaceTrainingOverlay;

public:
    static TrainingOverlay  s_trainingOverlay;

public:
    IndicatorImage(Modeler* pmodeler, Viewport* pviewport, Number* ptime) :
        Image(pviewport, ptime)
    {
        m_pcamera = new Camera();
        m_pcamera->SetZClip(-10000, 10000);

        m_psurfaceDirection = pmodeler->LoadSurface(AWF_FLIGHT_CURRENT_DIRECTION_ICON, true);
        m_psurfaceDirectionIn = pmodeler->LoadSurface("directioninbmp", true);
        m_psurfaceCenterInRange = pmodeler->LoadSurface(AWF_FLIGHT_CROSSHAIR_IN_ICON, true);
        m_psurfaceCenterOutRange = pmodeler->LoadSurface(AWF_FLIGHT_CROSSHAIR_OUT_ICON, true);
        m_psurfaceLeadInRange = pmodeler->LoadSurface(AWF_FLIGHT_LEAD_IN_ICON, true);
        m_psurfaceLeadOutRange = pmodeler->LoadSurface(AWF_FLIGHT_LEAD_OUT_ICON, true);

        m_psurfaceTrainingOverlay = pmodeler->LoadSurface(AWF_HUD_TRAINING_OVERLAY, true);

        m_pgeoPointer = pmodeler->LoadGeo("pointer", true);
        m_pgeoJoystickPointer = pmodeler->LoadGeo("joystickindicator", true);

        m_pmaterial = CreateMaterial(Color::Black());
    }

    void RenderDirectionIndicator(Context* pcontext)
    {
        const Vector& myVelocity = trekClient.GetShip()->GetSourceShip()->GetVelocity();
        int speed = (int)(myVelocity.Length() + 0.5f);

        if (GetWindow()->GetCameraMode() == TrekWindow::cmCockpit)
        {
            const Rect& rect = GetViewRect()->GetValue();

            if (speed != 0)
            {
                Point pointImage;
                if (GetCamera()->TransformDirectionToImage(myVelocity, pointImage))
                {
                    pointImage = rect.TransformNDCToImage(pointImage);

                    if (rect.Inside(pointImage))
                        pcontext->DrawImage(m_psurfaceDirection, true, pointImage);
                }
            }

            pcontext->SetBlendMode(BlendModeAdd);
            pcontext->DrawImage3D(m_psurfaceDirectionIn, Color(0.4f, 0.4f, 0.4f, 0.4f), true, rect.Center());
        }
    }

    void RenderJoystickIndicator(Context* pcontext) {
        const ControlData& shipControls = trekClient.GetShip()->GetControls();
        //Point pointJoystick = GetViewRect()->GetValue().Center() - Point(100.0f*shipControls.jsValues[c_axisYaw], 100.0f*shipControls.jsValues[c_axisPitch]);
        //pitch + yaw <= 1.0

        float angle = atan(shipControls.jsValues[c_axisPitch] / shipControls.jsValues[c_axisYaw]) + pi / 2;
        if (shipControls.jsValues[c_axisYaw] < 0) {
            angle += pi;
        }
        float turningSpeedRelativeToMax = std::min(1.0f, Point(shipControls.jsValues[c_axisYaw], shipControls.jsValues[c_axisPitch]).Length());

        pcontext->PushState();
        pcontext->Translate(GetViewRect()->GetValue().Center() - Point(100.0f*shipControls.jsValues[c_axisYaw], 100.0f*shipControls.jsValues[c_axisPitch]));
        pcontext->Begin3DLayer(m_pcamera, false);
        pcontext->DirectionalLight(Vector(0, 0, 1), Color(1.0f, 1.0f, 1.0f));
        pcontext->SetAmbientLevel(0.25f);
        pcontext->Rotate(Vector(0, 0, 1), angle);
        pcontext->Scale3(turningSpeedRelativeToMax / 2 + 0.5f);
        m_pmaterial->SetDiffuse(Color(1.0f, 1.0f, 1.0f, turningSpeedRelativeToMax));
        pcontext->SetMaterial(m_pmaterial);
        m_pgeoJoystickPointer->Render(pcontext);
        pcontext->End3DLayer();
        pcontext->PopState();
    }

    void RenderLeadIndicator(Context* pcontext)
    {
        IshipIGC*       pshipSource = trekClient.GetShip()->GetSourceShip();
        IclusterIGC*    pcluster = pshipSource->GetCluster();

        if (pcluster)
        {
            bool            bIisInRange = false;

            ImodelIGC* targetModel = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);
            if (targetModel &&
                (targetModel->GetCluster() == pcluster) &&
                //(targetModel->GetSide() != trekClient.GetSide()) &&
                pshipSource->CanSee(targetModel))
            {
                IshipIGC*   pshipParent = trekClient.GetShip()->GetParentShip();
                IshipIGC*   pship;

                IweaponIGC* w;

                if (pshipParent)
                {
                    pship = pshipParent;

                    Mount   turretID = trekClient.GetShip()->GetTurretID();
                    w = (turretID != NA) ? ((IweaponIGC*)(pship->GetMountedPart(ET_Weapon, turretID))) : NULL;
                }
                else
                {
                    pship = trekClient.GetShip();
                    w = trekClient.GetWeapon();
                }

                // Only show the indicator if we have a weapon, and it is a healing weapon on a friendly or a non-healing weapon on an enemy
                // or it is the external chase view or missile view
                IsideIGC*   pside = trekClient.GetSide();
                //Imago fixed 7/8/09 ALLY
                if (w && ((GetWindow()->GetCameraMode() == TrekWindow::cmExternalChase) ||
                    (GetWindow()->GetCameraMode() == TrekWindow::cmExternalMissile) ||
                    (((targetModel->GetSide() == pside) || IsideIGC::AlliedSides(targetModel->GetSide(), pside)) == (w->GetProjectileType()->GetPower() < 0.0f))))
                {
                    Vector      vecLeadPosition;
                    float       t = solveForLead(pship, targetModel, w, &vecLeadPosition);
                    float       l = w->GetLifespan();
                    Rect        rect = GetViewRect()->GetValue();
                    Point       pointCenter = rect.Center();
                    Point       pointLead;

                    if (t <= l)
                        bIisInRange = true;

                    if ((GetWindow()->GetCameraMode() == TrekWindow::cmExternalChase) || (GetWindow()->GetCameraMode() == TrekWindow::cmExternalMissile))
                    {
                        // get needed qualities of the shooter
                        Vector              shipPosition = trekClient.GetShip()->GetPosition();
                        Vector              shipAimDirection = trekClient.GetShip()->GetOrientation().GetForward();

                        // get needed qualities of the shooter weapon
                        IprojectileTypeIGC* pt = w->GetProjectileType();
                        float               fProjectileSpeed = pt->GetSpeed();

                        // get needed qualities of the shootee
                        Vector              targetPosition = targetModel->GetPosition();
                        float               fTargetRadius = targetModel->GetRadius();

                        // compute the distance between the ships
                        float               fDistanceBetweenShips = (targetPosition - shipPosition).Length();

                        // compute the distance at which a hit could occur
                        float               fLengthAlongTargetVector = t * fProjectileSpeed;

                        // get the camera position
                        Vector              cameraPosition = GetWindow()->GetCamera()->GetPosition();

                        // compute values needed for determining which indicators to draw
                        float               fCosAngle = vecLeadPosition * shipAimDirection;
                        float               fA = fLengthAlongTargetVector;
                        float               fH = fA / fCosAngle;
                        float               fHASquared = (fH * fH) - (fA * fA);
                        float               fRadiusSquared = fTargetRadius * fTargetRadius;
                        bool                bWillHit = (fHASquared <= fRadiusSquared) ? true : false;

                        // project a tracking point at the target range
                        Vector              trackingRelativeToShip = shipAimDirection * fDistanceBetweenShips;
                        Vector              trackingPosition = shipPosition + trackingRelativeToShip;
                        Vector              realAimDirection = (trackingPosition - cameraPosition).Normalize();
                        if (GetCamera()->TransformDirectionToImage(realAimDirection, pointLead))
                        {
                            pointLead = rect.TransformNDCToImage(pointLead);
                            pcontext->SetBlendMode(BlendModeAdd);
                            if (bIisInRange)
                            {
                                float   green = bWillHit ? 1.0f - (fHASquared / fRadiusSquared) : 0.0f;
                                pcontext->DrawImage3D(m_psurfaceCenterOutRange, Color(1.0f - green, green, 0.0f), true, pointLead);
                            }
                            else
                            {
                                pcontext->DrawImage3D(m_psurfaceCenterOutRange, Color(0.33f, 0.33f, 0.33f, 0.5f), true, pointLead);
                                //pcontext->DrawImage (m_psurfaceCenterOutRange, true, pointLead);
                            }
                        }
                    }
                    else
                    {
                        if (GetCamera()->TransformDirectionToImage(vecLeadPosition, pointLead))
                        {
                            pointLead = rect.TransformNDCToImage(pointLead);

                            bool    bLeadIndicator;
                            if (pshipParent || pship->GetBaseHullType()->HasCapability(c_habmLeadIndicator) /*|| g_bCheatLeadIndicator*/)
                            {
                                bLeadIndicator = true;
                            }
                            else
                            {
                                bLeadIndicator = false;
                                //See if a friendly relay station can spot the target
                                for (StationLinkIGC* psl = pcluster->GetStations()->first(); (psl != NULL); psl = psl->next())
                                {
                                    IstationIGC*   ps = psl->data();
                                    if ((ps->GetSide() == pside) &&
                                        ps->GetBaseStationType()->HasCapability(c_sabmRemoteLeadIndicator) &&
                                        ps->InScannerRange(targetModel))
                                    {
                                        bLeadIndicator = true;
                                        break;
                                    }
                                }

                                //See if a friendly relay ship can spot the target
                                if (!bLeadIndicator)
                                {
                                    for (ShipLinkIGC* psl = pcluster->GetShips()->first(); (psl != NULL); psl = psl->next())
                                    {
                                        IshipIGC*   ps = psl->data();
                                        if ((ps->GetSide() == pside) && //#ALLYTD: change this if allies should relay lead ind SCAN
                                            (ps->GetParentShip() == NULL) &&
                                            ps->GetBaseHullType()->HasCapability(c_habmRemoteLeadIndicator) &&
                                            ps->InScannerRange(targetModel))
                                        {
                                            bLeadIndicator = true;
                                            break;
                                        }
                                    }
                                }
                            }

                            if (bLeadIndicator)
                            {
                                pcontext->DrawImage((t <= l) ? m_psurfaceLeadInRange : m_psurfaceLeadOutRange,
                                    true,
                                    pointLead);
                            }


                            if (bIisInRange)
                            {
                                float  dx = (pointCenter.X() - pointLead.X());
                                float  dy = (pointCenter.Y() - pointLead.Y());

                                float   l2 = dx * dx + dy * dy;
                                float   r = targetModel->GetThingSite()->GetScreenRadius();

                                float   g;
                                float   f = l2 / (r * r);
                                if (f < 2.0f)
                                {
                                    if (f < 1.0f)
                                        g = 1.0f - f * 0.75f;
                                    else
                                        g = 0.5f - f / 4.0f;
                                }
                                else
                                    g = 0.0f;

                                /*
                                Surface*    psurfaceIndicator   = m_psurfaceCenterOutRange;
                                if (l2 <= r*r)
                                psurfaceIndicator = m_psurfaceCenterInRange;
                                pcontext->DrawImage(psurfaceIndicator, true, pointCenter);
                                */

                                pcontext->SetBlendMode(BlendModeAdd);
                                pcontext->DrawImage3D(m_psurfaceCenterOutRange, Color(1.0f - g, g, 0.0f), true, pointCenter);
                            }
                        }
                    }
                }
            }
            else if ((GetWindow()->GetCameraMode() == TrekWindow::cmExternalChase) || (GetWindow()->GetCameraMode() == TrekWindow::cmExternalMissile))
            {
                IshipIGC*   pship = trekClient.GetShip();
                if (!pship->GetParentShip())
                {
                    IweaponIGC* pWeapon = trekClient.GetWeapon();
                    if (pWeapon)
                    {
                        Rect        rect = GetViewRect()->GetValue();
                        Point       pointCenter = rect.Center();
                        Point       pointRender;

                        // get needed qualities of the shooter
                        Vector              shipPosition = pship->GetPosition();
                        Vector              shipAimDirection = pship->GetOrientation().GetForward();

                        // get needed qualities of the shooter weapon
                        IprojectileTypeIGC* pt = pWeapon->GetProjectileType();
                        float               fProjectileSpeed = pt->GetSpeed();
                        float               fProjectileDistance = fProjectileSpeed * pWeapon->GetLifespan();

                        // get the camera position
                        Vector              cameraPosition = GetWindow()->GetCamera()->GetPosition();

                        // project a tracking point at the target range
                        Vector              trackingRelativeToShip = shipAimDirection * fProjectileDistance;
                        Vector              trackingPosition = shipPosition + trackingRelativeToShip;
                        Vector              realAimDirection = (trackingPosition - cameraPosition).Normalize();
                        if (GetCamera()->TransformDirectionToImage(realAimDirection, pointRender))
                        {
                            pointRender = rect.TransformNDCToImage(pointRender);
                            pcontext->SetBlendMode(BlendModeAdd);
                            pcontext->DrawImage3D(m_psurfaceCenterOutRange, Color(0.33f, 0.33f, 0.33f, 0.5f), true, pointRender);
                            //pcontext->DrawImage(m_psurfaceCenterOutRange, true, pointRender);
                        }
                    }
                }
            }

            GetWindow()->GetConsoleImage()->SetInRange(bIisInRange);
        }
    }

    void RenderTurnIndicator(Context* pcontext, ImodelIGC* pmodelTarget, const Color& color)
    {
        IshipIGC*   pshipSource = trekClient.GetShip()->GetSourceShip();

        if (pmodelTarget &&
            ((pmodelTarget->GetObjectType() != OT_ship) ||
            (((IshipIGC*)pmodelTarget)->GetSourceShip() != pshipSource)))
        {
            IclusterIGC*    pcluster = pmodelTarget->GetCluster();
            if (pcluster && (pcluster == pshipSource->GetCluster()) && pshipSource->CanSee(pmodelTarget))
            {
                const Point&    dxy = pmodelTarget->GetThingSite()->GetScreenPosition();
                float   l = dxy.X() * dxy.X() + dxy.Y() * dxy.Y();
                const float radiusRing = 108.0f;
                if (l > radiusRing * radiusRing)
                {
                    Point ndx(dxy / float(sqrt(l)));

                    Orientation orientShip(/*trekClient.GetShip()->GetOrientation()*/ GetWindow()->GetCameraOrientation());
                    Vector vecForward(pmodelTarget->GetPosition() - pshipSource->GetPosition());
                    vecForward = orientShip.TimesInverse(vecForward).Normalize();

                    Vector vecLeft = vecForward.GetOrthogonalVector(); //CrossProduct(Vector(0, 0, -1), vecForward);
                    Vector vecUp = CrossProduct(vecForward, vecLeft);

                    Orientation orient(vecForward, vecUp);

                    pcontext->PushState();
                    pcontext->Translate(GetViewRect()->GetValue().Center() + radiusRing * ndx);

                    pcontext->Begin3DLayer(m_pcamera, false);
                    pcontext->DirectionalLight(Vector(0, 0, 1), color);
                    pcontext->SetAmbientLevel(0.25f);

                    pcontext->Multiply(Matrix(orient, Vector(0, 0, 0), 1.0f));
                    pcontext->Rotate(Vector(0, 0, 1), GetTime()->GetValue());

                    m_pmaterial->SetDiffuse(color);
                    pcontext->SetMaterial(m_pmaterial);

                    m_pgeoPointer->Render(pcontext);
                    //pcontext->DrawTriangles(g_pvertex, 12, g_pindex, 12);

                    pcontext->End3DLayer();
                    pcontext->PopState();
                }
            }
        }
    }

    void RenderTrainingOverlay(Context* pContext)
    {
        // the training overlay consists of two lines in an angular formation,
        // a hilite dot, and some text describing the element referred to.
        Rect                rect = GetViewRect()->GetValue();
        Point               pointCenter = rect.Center();
        Point               pointA;
        Point               pointB;
        Point               pointC;
        Point               pointDelta;
        Point               pointText;
        Point				pointText2;
        Point               pointTextSize;
        TVector<VertexL>    edgeVertices;
        TVector<MeshIndex>  edgeIndices;
        ZString             description;
        ZString				description2;
        Color               colorLine(1.0f, 0.0f, 0.0f, 0.9f);

        // depending on the overlay we want to draw, this sets the point for the hilite dot,
        // the direction that the arrow will draw from, the second point in the hilite line,
        // and the textual description
        switch (s_trainingOverlay)
        {
        case NoTrainingOverlay:
            return;

        case SpeedTrainingOverlay:
        {
            pointDelta = Point(-1.0f, -1.0f);
            pointA = pointCenter + Point(-25.0f, -105.5f);
            pointB = pointA + (pointDelta * 100.0f);
            description = "SPEED (meters per second)";
            break;
        }

        case MotionIndicatorTrainingOverlay:
        {
            pointDelta = Point(-1.0f, 1.0f);
            pointA = pointCenter + Point(-10.5f, 10.5f);
            pointB = pointA + (pointDelta * 150.0f);
            description = "MOTION INDICATOR";
            break;
        }

        case ReticuleTrainingOverlay:
        {
            pointDelta = Point(-1.0f, 1.0f);
            pointA = pointCenter + Point(-10.5f, 10.5f);
            pointB = pointA + (pointDelta * 150.0f);
            description = "RETICULE";
            break;
        }

        case ThrottleTrainingOverlay:
        {
            pointDelta = Point(-1.0f, 1.0f);
            pointA = pointCenter + Point(-126.5f, 93.5f);
            pointB = pointA + (pointDelta * 50.0f);
            description = "THROTTLE GAUGE";
            description2 = "SCROLL WHEEL ALSO CONTROLS THROTTLE";
            break;
        }

        case FuelTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(126.5f, 93.5f);
            pointB = pointA + (pointDelta * 100.0f);
            description = "FUEL GAUGE";
            description2 = "PUSH R TO RELOAD";
            break;
        }

        case EnergyTrainingOverlay:
        {
            pointDelta = Point(1.0f, -1.0f);
            pointA = pointCenter + Point(126.5f, -112.5f);
            pointB = pointA + (pointDelta * 50.0f);
            description = "ENERGY GAUGE";
            break;
        }

        case AmmoTrainingOverlay:
        {
            pointDelta = Point(-1.0f, -1.0f);
            pointA = pointCenter + Point(-126.5f, -112.5f);
            pointB = pointA + (pointDelta * 50.0f);
            description = "AMMUNITION GAUGE";
            break;
        }

        case HullTrainingOverlay:
        {
            pointDelta = Point(-1.0f, 1.0f);
            pointA = pointCenter + Point(-65.5f, 35.5f);
            pointB = pointA + (pointDelta * 100.0f);
            description = "ARMOR STRENGTH GAUGE";
            break;
        }

        case ShieldTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(65.5f, 35.5f);
            pointB = pointA + (pointDelta * 100.0f);
            description = "SHIELD STRENGTH GAUGE";
            break;
        }

        case ChatTrainingOverlay:
        {
            pointDelta = Point(-1.0f, -1.0f);
            pointA = pointCenter + Point(-140.0f, -50.0f + (rect.YMax() - pointCenter.Y()));
            pointB = pointA + (pointDelta * 100.0f);
            description = "CHAT PANE";
            break;
        }

        case SectorTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = Point(99.0f, 190.0f);
            pointB = pointA + (pointDelta * 20.0f);
            description = "SECTOR MAP";
            break;
        }

        case InventoryTrainingOverlay:
        {
            pointDelta = Point(-1.0f, 1.0f);
            pointA = Point(rect.XMax() - 150.0f, 265.0f);
            pointB = pointA + (pointDelta * 50.0f);
            description = "INVENTORY DISPLAY";
            break;
        }

        case TargetTrainingOverlay:
        {
            pointDelta = Point(-1.0f, 1.0f);
            pointA = pointCenter + Point(-70.0f, 120.0f - pointCenter.Y());
            pointB = pointA + (pointDelta * 100.0f);
            description = "TARGET HUD";
            break;
        }

        case CommandTrainingOverlay:
        {
            pointDelta = Point(1.0f, -1.0f);
            pointA = pointCenter + Point(-(pointCenter.X() + -100.0f), -85.0f + (rect.YMax() - pointCenter.Y()));
            pointB = pointA + (pointDelta * 50.0f);
            description = "CURRENT COMMAND";
            break;
        }

        case EyeballTrainingOverlay:
        {
            pointDelta = Point(1.0f, -1.0f);
            pointA = pointCenter + Point(140.5f, -140.5f);
            pointB = pointA + (pointDelta * 50.0f);
            description = "DETECTION INDICATOR";
            break;
        }

        case KillBonusTrainingOverlay:
        {
            pointDelta = Point(-1.0f, -0.375f);
            pointA = pointCenter + Point(-15.0f, -160.5f);
            pointB = pointA + (pointDelta * 70.0f);
            description = "KILL BONUS";
            break;
        }

        case RollTrainingOverlay:
        {
            pointDelta = Point(1.0f, 0.0f);
            pointA = pointCenter + Point(-110.0f, -170.0f);
            pointB = pointA + Point(5.0f, 0.0f);
            description = "PUSH Q AND E TO ROLL LEFT AND RIGHT";
            description2 = "PUSH TAB TO TOGGLE MOUSE CONTROL";
            break;
        }

        case ShiftTabTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(-70.0f, -170.0f);
            pointB = pointA + Point(5.0f, 0.0f);
            description = "SHIFT+SPACE TO BOOST";
            description2 = "";
            break;
        }

        case TargetCenterTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(-144.0f, -170.0f);
            pointB = pointA + Point(5.0f, 0.0f);
            description = "CLICK MIDDLE MOUSE BUTTON TO TARGET CENTER";
            description2 = "";
            break;
        }

        case FireGunsTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(-75.0, -170.0f);
            pointB = pointA + Point(5.0f, 0.0f);
            description = "LEFT CLICK TO FIRE GUNS";
            description2 = "";
            break;
        }

        case TargetEnemyTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(-72.0, -170.0f);
            pointB = pointA + Point(5.0f, 0.0f);
            description = "PUSH T TO TARGET ENEMY";
            description2 = "";
            break;
        }

        case MissileTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(-80.0, -170.0f);
            pointB = pointA + Point(5.0f, 0.0f);
            description = "RIGHT CLICK TO FIRE MISSILE";
            description2 = "";
            break;
        }

        case CloakTrainingOverlay:
        {
            pointDelta = Point(1.0f, 1.0f);
            pointA = pointCenter + Point(-75.0, -170.0f);
            pointB = pointA + Point(5.0f, 0.0f);
            description = "PUSH G TO TOGGLE CLOAK";
            description2 = "";
            break;
        }

        }

        // begin the ddraw procedure
        pContext->PushState();

        // we want to use a red line with red text, and the thing has to be visible
        pContext->SetLineWidth(2.0f);

        // compute the third vertex from the first two and the width of the description.
        // this is then used to compute where to draw the text

        TRef<IEngineFont> pfont = TrekResources::SmallFont();

        pointTextSize = Point::Cast(pfont->GetTextExtent(description));
        pointC = pointB + Point(pointDelta.X() * pointTextSize.X(), 0.0f);
        float   fLeft;
        float   fBottom;
        if (pointDelta.X() < 0.0f)
            fLeft = pointC.X();
        else
            fLeft = pointB.X();

        if (pointDelta.Y() < 0.0f)
            fBottom = pointB.Y() + 2.0f;
        else
            fBottom = pointB.Y() - (float(pfont->GetHeight()) + 2.0f);
        pointText = Point(fLeft, fBottom);
        pointText2 = Point(fLeft, (fBottom - (float(pfont->GetHeight()) + 4.0f)));

        // build the vertex list from the three points
        edgeVertices.SetCount(3);
        edgeVertices.Set(0, VertexL(Vector(pointA.X(), pointA.Y(), 0.0f), colorLine));
        edgeVertices.Set(1, VertexL(Vector(pointB.X(), pointB.Y(), 0.0f), colorLine));
        edgeVertices.Set(2, VertexL(Vector(pointC.X(), pointC.Y(), 0.0f), colorLine));

        // set the indices up for the edge list. There are two edges to draw
        edgeIndices.SetCount(4);
        edgeIndices.Set(0, 0);
        edgeIndices.Set(1, 1);
        edgeIndices.Set(2, 1);
        edgeIndices.Set(3, 2);

        // draw the lines, the description, and the hilite image
        pContext->DrawLines(edgeVertices, edgeIndices);
        pContext->DrawString(pfont, colorLine, pointText, description);
        if (s_trainingOverlay == RollTrainingOverlay || s_trainingOverlay == FuelTrainingOverlay) {
            pContext->DrawString(pfont, colorLine, pointText2, description2);
        }
        if (s_trainingOverlay == ThrottleTrainingOverlay) {
            pointText2 = Point((fLeft - 140.0f), (fBottom - (float(pfont->GetHeight()) + 4.0f)));
            pContext->DrawString(pfont, colorLine, pointText2, description2);
        }
        pContext->DrawImage(m_psurfaceTrainingOverlay, true, pointA);

        // finished drawing
        pContext->PopState();
    }

    void Render(Context* pcontext)
    {
        if (TrekWindow::InternalCamera(GetWindow()->GetCameraMode()) || (GetWindow()->GetCameraMode() == TrekWindow::cmExternalChase) || (GetWindow()->GetCameraMode() == TrekWindow::cmExternalMissile))
        {
            //
            // Rendering attributes
            //

            pcontext->SetShadeMode(ShadeModeFlat);
            pcontext->SetLinearFilter(false, true);
            pcontext->Clip(GetViewRect()->GetValue());

            //
            // Draw the indicators
            //

            if (GetWindow()->GetShowJoystickIndicator())
                RenderJoystickIndicator(pcontext);

            RenderLeadIndicator(pcontext);

            ImodelIGC* pmodelAccepted = trekClient.GetShip()->GetCommandTarget(c_cmdAccepted);
            ImodelIGC* pmodelCurrent = trekClient.GetShip()->GetCommandTarget(c_cmdCurrent);

            if (pmodelAccepted == pmodelCurrent)
                RenderTurnIndicator(pcontext, pmodelAccepted, Color(1, 1, 0));
            else
            {
                RenderTurnIndicator(pcontext, pmodelAccepted, Color(0, 1, 0));
                RenderTurnIndicator(pcontext, pmodelCurrent, Color(1, 0, 0));
            }

            RenderTrainingOverlay(pcontext);

            RenderDirectionIndicator(pcontext);
        }
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Indicator Image Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Image> CreateIndicatorImage(Modeler* pmodeler, Viewport* pviewport, Number* ptime)
{
    return new IndicatorImage(pmodeler, pviewport, ptime);
}

//////////////////////////////////////////////////////////////////////////////
//
// static member management
//
//////////////////////////////////////////////////////////////////////////////
TrainingOverlay  IndicatorImage::s_trainingOverlay = NoTrainingOverlay;

void    SetTrainingOverlay(TrainingOverlay overlay)
{
    IndicatorImage::s_trainingOverlay = overlay;
}
