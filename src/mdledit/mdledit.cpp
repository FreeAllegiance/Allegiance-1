//////////////////////////////////////////////////////////////////////////////
//
// MDL Editor
//
//////////////////////////////////////////////////////////////////////////////

// jul 08 - KG global changes to work with DX9



#include "pch.h"

#include "enginewindow.h"

//////////////////////////////////////////////////////////////////////////////
//
// The main entry point
//
//////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "../Inc/regkey.h"

#include "VideoSettingsDX9.h"

//////////////////////////////////////////////////////////////////////////////
//
// Network
//
//////////////////////////////////////////////////////////////////////////////

/*
#include "network.h"
#include "network.cpp"
*/

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

/*
class WaveDeformation : public Deformation {
private:
    float m_frequency;
    float m_amplitude;
    float m_phase;

public:
    WaveDeformation(
        float wavelength,
        float amplitude
    ) :
        m_frequency(2.0f * pi / wavelength),
        m_amplitude(amplitude),
        m_phase(0)
    {
    }

    void SetWavelength(float wavelength)
    {
        m_frequency = 2.0f * pi / wavelength;
    }

    void SetAmplitude(float amplitude)
    {
        m_amplitude = amplitude;
    }

    void SetPhase(float phase)
    {
        m_phase = phase;
    }

    Vector Deform(const Vector& vec)
    {
        return 
            Vector(
                vec.x,
                vec.y + m_amplitude * sin(m_frequency * vec.x + m_phase),
                vec.z
                //std::min(0, vec.z * std::max(1, vec.x * 0.25f))
            );
    }
};
*/

//////////////////////////////////////////////////////////////////////////////
//
// ModelerSite
//
//////////////////////////////////////////////////////////////////////////////

class ModelerSiteImpl : public ModelerSite {
public:
    void Error(const ZString& str)
    {
        ZError(str);
        MessageBox(NULL, str, "Error", MB_OK);
        _exit(0);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class MDLEditWindow :
    public EngineWindow,
    public IIntegerEventSink,
    //public IEventSink,
    public IMenuCommandSink,
    public ISubmenuEventSink
    /*,
    public NetworkSite,
    public SocketSite*/
{
public:
    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    float m_angle;

    class TestGeo : public WrapGeo {
    private:
        //TRef<WaveDeformation> m_pdeform;

        TRef<Modeler>  m_pmodeler;
        TRef<Surface>  m_psurface;
        TRef<ThingGeo> m_pgeo;
        TRef<GroupGeo> m_pgroupGeo;
        MDLEditWindow* m_pwindow;
        int            m_test;

        VertexL        vertices[4 * 100];
        WORD           indices[6 * 100];

        float GetTime() { return Number::Cast(GetChild(1))->GetValue(); }

    public:
        TestGeo(Modeler* pmodeler, MDLEditWindow* pwindow, Number* ptime, int initialTest) :
            WrapGeo(Geo::GetEmpty(), ptime),
            m_pmodeler(pmodeler),
            m_pwindow(pwindow),
            m_test(initialTest)
        {
            //m_pdeform = new WaveDeformation(1.0f, 0.25f);

            m_pwindow->ToggleShowFPS();
            InitializePolyTest();
            InitializeGeoTest();
            InitializeRingTest();
            SetTest(initialTest);
        }

        void InitPoly(int qindex)
        {
            int   index  = qindex * 6;
            int   vindex = qindex * 4;
            //float z      = -0.1f * float(2 * qindex - qindex); // front to back
            float z      = -0.1f * float(99 - qindex); // back to front
            //float z = 0; 

            //vertices[vindex + 0] = Vertex(-1,  1, z, 0, 0, 1, 0, 0);
            //vertices[vindex + 1] = Vertex(-1, -1, z, 0, 0, 1, 0, 1);
            //vertices[vindex + 2] = Vertex( 1, -1, z, 0, 0, 1, 1, 1);
            //vertices[vindex + 3] = Vertex( 1,  1, z, 0, 0, 1, 1, 0);

            float a = 1.0f;
            float r = 1.0f * a;
            float g = 0.5f * a;
            float b = 0.25f * a;

            vertices[vindex + 0] = VertexL(-1,  1, z, r, g, b, a, 0, 0);
            vertices[vindex + 1] = VertexL(-1, -1, z, r, g, b, a, 0, 1);
            vertices[vindex + 2] = VertexL( 1, -1, z, r, g, b, a, 1, 1);
            vertices[vindex + 3] = VertexL( 1,  1, z, r, g, b, a, 1, 0);

            indices[index + 0] = vindex + 0; // cw
            indices[index + 1] = vindex + 2;
            indices[index + 2] = vindex + 1;
            indices[index + 3] = vindex + 0; // ccw
            indices[index + 4] = vindex + 3;
            indices[index + 5] = vindex + 2;
        }

        void InitializePolyTest()
        {
            m_pwindow->m_angle = 0;

            m_psurface = m_pmodeler->LoadSurface("testtexturebmp", false);
            //m_psurface = m_pmodeler->LoadSurface("fig04bmp", false);
            //m_psurface = m_pmodeler->LoadSurface("f101bmp", false);

            for (int qindex = 0; qindex < 100; qindex++) {
                InitPoly(qindex);
            }
        }

        void InitializeGeoTest()
        {
            m_pgeo = m_pwindow->CreateThingGeo("bom01a", Vector(0, 0, 0));
        }

        /*
        Matrix mat;

        float  y      = (value - 0.5f) * 10;
        float  radius = 10;
        float  angle  = value * 4 * pi;
        float  z      = radius * cos(angle);
        float  x      = radius * sin(angle);
        Vector center(0, 0, -20);

        mat.SetRotate(Vector(0, 1, 0), - pi / 2);
        mat.Translate(center + Vector(x, y, z));
        m_pgroupGeo->AddGeo(
            new TransformGeo(
                new CullGeo(pgeo),
                new MatrixTransform(mat)
            )
        );
        */

        void InitializeRingTest()
        {
            m_pgroupGeo = GroupGeo::Create();

            float max    = 32.0f;
            float radius = 50.0f;

            for (float value = 0.0f; value < max; value++) {
                TRef<ThingGeo> pthing =
                    m_pwindow->CreateThingGeo(
                        "bom01a", 
                        Vector::RandomPosition(radius) + Vector(0, 0, -3.0f * radius)
                    );

                m_pgroupGeo->AddGeo(pthing->GetGeo());
            }
        }

        void SetTest(int test)
        {
            m_test = test;

            switch (m_test) {
                case 2:  
                    ThingGeo::SetShowLights(true);
                    SetGeo(m_pgeo->GetGeo());          
                    break;

                case 3:  
                    ThingGeo::SetShowLights(false);
                    SetGeo(m_pgroupGeo);     
                    break;

                default: SetGeo(Geo::GetEmpty()); break;
            }
        }

        void NextTest()
        {
            m_test++;

            if (m_test > 3) {
                m_test = 0;
            }

            SetTest(m_test);
        }

        void SetMode(Context* pcontext)
        {
            pcontext->SetColorKey(false);
            pcontext->SetCullMode(CullModeNone);
            pcontext->SetZWrite(true);
            pcontext->SetZTest(true);

            //pcontext->SetShadeMode(ShadeModeCopy);
            pcontext->SetShadeMode(ShadeModeFlat);
            //pcontext->SetShadeMode(ShadeModeGouraud);

            pcontext->SetBlendMode(BlendModeSource);
            //pcontext->SetBlendMode(BlendModeAdd);
            //pcontext->SetBlendMode(BlendModeSourceAlpha);

            //pcontext->SetTexture(NULL);
            pcontext->SetTexture(m_psurface);

            //m_pdeform->SetPhase(4.0f * GetTime());
            //pcontext->SetDeformation(m_pdeform);
        }

        void DrawTriangles(Context* pcontext, int count, float scale)
        {
            pcontext->PushState();
            pcontext->Scale(Vector(scale, scale, 1));
            pcontext->DrawTriangles(vertices, 4 * count, indices, 6 * count);
            //pcontext->DrawTriangles(vertices, 4, indices, 3);
            pcontext->PopState();
        }

        void Test0(Context* pcontext)
        {
            SetMode(pcontext);

            pcontext->Rotate(Vector(0, 0, 1), m_pwindow->m_angle);
            //pcontext->Rotate(Vector(0, 0, 1), 0.25f * pi);

            DrawTriangles(pcontext, 1, 1.0f);
        }

        void Test1(Context* pcontext)
        {
            SetMode(pcontext);
            pcontext->SetShadeMode(ShadeModeNone);

            pcontext->Rotate(Vector(0, 1, 0), m_pwindow->m_angle);
            pcontext->Rotate(Vector(0, 0, 1), 1.0f);

            for(int index = 0; index < 400; index++) {
                DrawTriangles(pcontext, 100, 0.01f);
            }
        }

        void DefaultRender(Context* pcontext)
        {
            SetMode(pcontext);
            WrapGeo::Render(pcontext);
        }

        void Render(Context* pcontext)
        {
            switch (m_test) {
                case  0: Test0(pcontext);         break;
                case  1: Test1(pcontext);         break;
                default: DefaultRender(pcontext); break;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class TestImage : public Image {
    public:
        TRef<Modeler>  m_pmodeler;
        MDLEditWindow* m_pwindow;
        TRef<Surface>  m_psurfaceIcon;

        float GetTime() { return Number::Cast(GetChild(0))->GetValue(); }

        //////////////////////////////////////////////////////////////////////////////
        //
        // constructor
        //
        //////////////////////////////////////////////////////////////////////////////

        TestImage(Modeler* pmodeler, MDLEditWindow* pwindow, Number* ptime) :
            Image(ptime),
            m_pmodeler(pmodeler),
            m_pwindow(pwindow)
        {
            m_pwindow->m_angle = 0;
            m_psurfaceIcon = m_pmodeler->LoadSurface("fighterbmp", true);
        }

        //////////////////////////////////////////////////////////////////////////////
        //
        // Image methods
        //
        //////////////////////////////////////////////////////////////////////////////

        void Render(Context* pcontext)
        {
            /*
            float angle = m_pwindow->m_angle;//GetTime();

            VertexScreen pvertex[3];
            MeshIndex    pindex[4]  = { 0, 1, 1, 2 };

            pvertex[0].x        = 100.5f;
            pvertex[0].y        = 100.5f;
            pvertex[0].z        = 0;
            pvertex[0].qw       = 1;
            pvertex[0].color    = MakeD3DCOLOR(Color(1, 1, 1));
            pvertex[0].specular = 0;
            pvertex[0].u        = 0;
            pvertex[0].v        = 0;

            pvertex[1].x        = 100.5f + 20 * cos(angle);
            pvertex[1].y        = 100.5f + 20 * sin(angle);
            pvertex[1].z        = 0;
            pvertex[1].qw       = 1;
            pvertex[1].color    = MakeD3DCOLOR(Color(1, 1, 1));
            pvertex[1].specular = 0;
            pvertex[1].u        = 0;
            pvertex[1].v        = 0;

            pvertex[2].x        = 30;
            pvertex[2].y        = 20;
            pvertex[2].z        = 0;
            pvertex[2].qw       = 1;
            pvertex[2].color    = MakeD3DCOLOR(Color(1, 1, 1));
            pvertex[2].specular = 0;
            pvertex[2].u        = 0;
            pvertex[2].v        = 0;

            pvertex[1].color    = MakeD3DCOLOR(Color(1, 0, 0));
            pcontext->DrawPoints(pvertex + 1, 1);

            pvertex[1].color    = MakeD3DCOLOR(Color(1, 1, 1));
            pcontext->DrawLines(pvertex, 2, pindex, 2);
            */

            pcontext->Translate(Point(50, 50));
            pcontext->SetBlendMode(BlendModeSourceAlpha);
            pcontext->DrawImage3D(m_psurfaceIcon, Color(1, 0.5f, 0.5f), true);
        }

        //////////////////////////////////////////////////////////////////////////////
        //
        // IMouseInput methods
        //
        //////////////////////////////////////////////////////////////////////////////

        MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
        {
            return MouseResult();
        }
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class CursorSurfaceSite : public SurfaceSite {
    public:
        void UpdateSurface(Surface* psurface) 
        {
            const Color& color = Color::White();

            const WinPoint& size = psurface->GetSize();
            int             x    = size.X();
            int             y    = size.Y();

            psurface->FillSurface(Color::Black());

            psurface->FillRect(WinRect(        0, y / 2 - 1, x / 2 - 2, y / 2 + 1), color);
            psurface->FillRect(WinRect(x / 2 + 2, y / 2 - 1,         x, y / 2 + 1), color);

            psurface->FillRect(WinRect(x / 2 - 1,         0, x / 2 + 1, y / 2 - 2), color);
            psurface->FillRect(WinRect(x / 2 - 1, y / 2 + 2, x / 2 + 1,         y), color);
        }
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    //TRef<SoundEngine>      m_psoundEngine;
    TRef<Camera>           m_pcamera;
    TRef<Viewport>         m_pviewport;
    TRef<GroupGeo>         m_pgroupGeo;
    TRef<GroupImage>       m_pgroupImage;
    TRef<GeoImage>         m_pgeoImage;
    TRef<WrapImage>        m_pwrapImageInput;
    TRef<Image>            m_pimageCursor;
    TRef<ModifiableColorValue> m_pcolorBackground;

    //
    // LOD scroll bar
    //

    TRef<ScrollPane>          m_pscrollPane;
    TRef<IIntegerEventSource> m_peventLOD;
    TRef<ModifiableBoolean>   m_pbooleanVisibleLOD;

    //
    // ThingGeo
    //

    TRef<ThingGeo>         m_pthing;
    TRef<ModifiableNumber> m_pnumberFrame;
    TRef<ParticleGeo>      m_pParticleGeo;
    TRef<BitsGeo>          m_pbitsGeo;
    bool                   m_bAnimate;
    bool                   m_bGlow;
    bool                   m_bClearColor;

    //
    // Tests
    //

    TRef<TestGeo>          m_ptestGeo;


    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    MDLEditWindow(
        EffectApp* papp, 
        EngineConfigurationWrapper* pConfiguration,
        const ZString& strCommandLine, 
        bool bImageTest,
        bool bTest, 
        int initialTest,
		const ZString& strArtPath
    ) :
        EngineWindow(
            pConfiguration,
            strCommandLine,
            "MDLEdit",
            false,
            WinRect(0 + CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetX, 
					0 + CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetY,
					CD3DDevice9::Get()->GetCurrentMode()->mode.Width + 
									CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetX,
					CD3DDevice9::Get()->GetCurrentMode()->mode.Height + 
									CD3DDevice9::Get()->GetDeviceSetupParams()->iWindowOffsetY)
        ),
        m_bAnimate(false),
        m_bGlow(false),
        m_bClearColor(false)
    {

		// Move this call here, so that engine initialisation is performed *AFTER* we have a valid HWND.
		papp->Initialize( strCommandLine, GetHWND() );
		m_pengine = papp->GetEngine();
		m_pmodeler = papp->GetModeler();

		// Now set the art path, performed after initialise, else Modeler isn't valid.
		GetModeler()->SetArtPath(strArtPath);

		// load the fonts
		TrekResources::Initialize(GetModeler());

		// Perform post window creation initialisation. Initialise the time value.
        SetEngine(m_pengine);
        SetModeler(m_pmodeler);
		InitialiseTime();

        GetModeler()->SetSite(new ModelerSiteImpl());

        //
        // This app runs in Game Mode
        //

        //SetFullscreen(true);
        //GetEngine()->SetDebugFullscreen(true);
        SetShowFPS(true);

        //
        // Create the sound engine
        //

        //m_psoundEngine = CreateSoundEngine(GetHWND());

        //
        // Create the image hierarchy
        //

        m_pgroupImage = new GroupImage();
        SetImage(m_pgroupImage);
        m_pgroupImage->AddImage(papp->GetPopupContainer()->GetImage());

        AddLODScrollBar();

        m_pgroupImage->AddImage(
            m_pwrapImageInput = new WrapImage(Image::GetEmpty())
        );

        //
        // The Camera
        //

        TRef<Image> pimageCamera;
        TRef<IKeyboardInput> pkeyboardInput;

        Vector cameraPosition(0, 0, 10);

        m_pcamera =
            CreateMotionCamera(
                cameraPosition,
                GetTime(),
                pimageCamera,
                pkeyboardInput
            );

        AddKeyboardInputFilter(pkeyboardInput);
        m_pgroupImage->AddImage(pimageCamera);

        m_pcamera->SetZClip(1, 10000);
        //m_pcamera->SetFOV(RadiansFromDegrees(65));
        m_pcamera->SetPerspective(4.0f);

        //
        // The viewport
        //

        m_pviewport = new Viewport(m_pcamera, GetRenderRectValue());

        //
        // Geometry
        //

        m_pgroupGeo   = GroupGeo::Create();
        m_pgeoImage   = new GeoImage(m_pgroupGeo, m_pviewport, true);
        m_pgroupImage->AddImage(m_pgeoImage);

        Color color(0, 0.5f, 1.0f);

        m_pgeoImage->SetLight(Color(1, 0, 0), color);
        m_pgeoImage->SetAmbientLevel(0.5f);

        //
        // Image Layers
        //

        //Add3DTests(bTest, initialTest);
        //Add2DTests(bImageTest);

        //
        // Stars
        //

        m_pgroupImage->AddImage(StarImage::Create(m_pviewport, 5000));

        //
        // Background
        //

        m_pcolorBackground = new ModifiableColorValue(Color::Black());
        m_pgroupImage->AddImage(CreateColorImage(m_pcolorBackground));

        //
        // Popups
        //

        Window::AddKeyboardInputFilter(papp->GetPopupContainer());
        InitializeMenu();

        //
        // Cursor
        //

        volatile float size = 16;

        TRef<Surface> psurfaceCursor =
            GetEngine()->CreateSurface(
                WinPoint(int(size), int(size)), 
                SurfaceType2D(), 
                new CursorSurfaceSite()
            );

        m_pimageCursor =
           new TransformImage(
                new ConstantImage(
                    psurfaceCursor,
                    ZString("cursor")
                ),
                new TranslateTransform2(
                    new PointValue(
                        Point(-(size / 2), -(size / 2))
                    )
                )
           );

        RestoreCursor();

        //
        // Test CreateMMLPopup
        //

        /*
        GetPopupContainer()->OpenPopup(
            CreateMMLPopup(
                GetModeler(),
                "hlp7a4.mml",false
            )
        );
		*/
		
        
		// user tests here
		AddMineFieldTest();
		//AddIcosahedron(); //for testing "Intel SWVP clipping issue" -Imago 
        
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void Add2DTests(bool bTest)
    {
        if (bTest) {
            m_pgroupImage->AddImage(new TestImage(GetModeler(), this, GetTime()));
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void Add3DTests(bool bTest, int initialTest)
    {
        if (bTest) {
            m_ptestGeo = new TestGeo(GetModeler(), this, GetTime(), initialTest);
            m_pgroupGeo->AddGeo(m_ptestGeo); 
        }

		//AddBoltTest();
        //AddDebris();
        //AddConeGeoTest();
        //AddIcosahedron();
        //AddLightening();
        //AddThingGeo();
        //AddSphere();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    TRef<MineFieldGeo> m_pmineFieldGeo;
	TRef<Geo> m_alephGeo;
	TRef<ExplosionGeo> m_expGeo;
	TRef<TEvent<float>::SourceImpl> m_peventSourceAleph;

    void AddMineFieldTest()
    {
        //TRef<Surface> psurface = GetModeler()->LoadSurface("fxminebmp", true);

		TRef<ZFile> zf = m_pmodeler->GetFile("fxmine.png","",true);

	ZFile * pFile = (ZFile*) zf;
		
		D3DXIMAGE_INFO fileInfo;
		D3DXGetImageInfoFromFileInMemory(	pFile->GetPointer(),
												pFile->GetLength(),
												&fileInfo );
		
		WinPoint targetSize( fileInfo.Width, fileInfo.Height );
		TRef<Surface> psurface =
			m_pengine->CreateSurfaceD3DX(
				&fileInfo,
				&targetSize,
				zf,
				true,
				Color( 0, 0, 0 ),
				"fxmine", true );



		// KG - old version here
        //for (int index = 0; index < 100; index++) {
        //    m_pmineFieldGeo = CreateMineFieldGeo(psurface, 1, 100);
        //    m_pgroupGeo->AddGeo(
        //        new TransformGeo(
        //            CreateCullGeo(m_pmineFieldGeo), 
        //            new TranslateTransform(
        //                Vector::RandomPosition(1000)
        //            )
        //        )
        //    );
        //}

		//aleph
		TRef<ThingGeo> pthing = ThingGeo::Create(GetModeler(), GetTime());
		TRef<Image> pimageAleph = GetModeler()->LoadImage("plnt19bmp", false);
		
		m_peventSourceAleph = new TEvent<float>::SourceImpl;
		m_alephGeo = CreateAlephGeo(GetModeler(), m_peventSourceAleph, GetTime());
        pthing->Load(0, m_alephGeo, pimageAleph);

		m_pgroupGeo->AddGeo(pthing->GetGeo());

		//explosion
		
		TRef<Image> m_pimageShockWave = GetModeler()->LoadImage("fx18bmp", true);
		TVector<TRef<AnimatedImage> > m_vpimageExplosion[8];

		TRef<AnimatedImage> img1 = new AnimatedImage(new Number(0.0f), GetModeler()->LoadSurface("exp20bmp", true, true, true));
		TRef<AnimatedImage> img2 = new AnimatedImage(new Number(0.0f), GetModeler()->LoadSurface("exp22bmp", true, true, true));
		TRef<AnimatedImage> img3 = new AnimatedImage(new Number(0.0f), GetModeler()->LoadSurface("exp23bmp", true, true, true));
		TRef<AnimatedImage> img4 = new AnimatedImage(new Number(0.0f), GetModeler()->LoadSurface("exp24bmp", true, true, true));
		TRef<AnimatedImage> img5 = new AnimatedImage(new Number(0.0f), GetModeler()->LoadSurface("exp25bmp", true, true, true));
	    m_vpimageExplosion[0].SetCount(5);
	    m_vpimageExplosion[0].Set(0, img1);
	    m_vpimageExplosion[0].Set(1, img2);
	    m_vpimageExplosion[0].Set(2, img3);
	    m_vpimageExplosion[0].Set(3, img4);
	    m_vpimageExplosion[0].Set(4, img5);

 		for (int index = 0; index < 200; index++) {
			pthing = ThingGeo::Create(GetModeler(), (GetTime()));
			m_expGeo = CreateExplosionGeo(GetTime());
			m_expGeo->AddExplosion(
	                Vector::RandomPosition(4000),
	                Vector(0, 1, 0),
	                Vector(1, 0, 0),
	                Vector(0, 0, 0),
					350,
	                1000,
	                Color(200.0f / 255.0f, 130.0f / 255.0f, 50.0f / 255.0f),
					24,
					m_vpimageExplosion[0],
					m_pimageShockWave);

			m_pgroupGeo->AddGeo(m_expGeo);
		}

		//minefield



		pthing = ThingGeo::Create(GetModeler(), GetTime());

        m_pmineFieldGeo = CreateMineFieldGeo(psurface, 2.0f, 100.0f);
        pthing->Load(0, m_pmineFieldGeo, NULL);

		m_pgroupGeo->AddGeo(pthing->GetGeo());





 		for (int index = 0; index < 200; index++) {
			pthing = ThingGeo::Create(GetModeler(), (GetTime()));
			m_expGeo = CreateExplosionGeo(GetTime());
			m_expGeo->AddExplosion(
	                Vector::RandomPosition(4000),
	                Vector(0, 1, 0),
	                Vector(1, 0, 0),
	                Vector(0, 0, 0),
					350,
	                1000,
	                Color(200.0f / 255.0f, 130.0f / 255.0f, 50.0f / 255.0f),
					24,
					m_vpimageExplosion[0],
					m_pimageShockWave);

			m_pgroupGeo->AddGeo(m_expGeo);
		}
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    void AddBoltTest()
    {
        m_pgroupGeo->AddGeo(
            CreateBoltGeo(
                new VectorValue(Vector(-20, 0, 0)), 
                new VectorValue(Vector( 20, 0, 0)), 
                0.125f,
                GetModeler()->LoadSurface("lightningbmp", true)
            )
        );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class SweepNumber : public Number {
    private:
        float m_min;
        float m_max;
        float m_duration;

    float GetTime() { return Number::Cast(GetChild(0))->GetValue(); }

    public:
        SweepNumber(Number* ptime, float min, float max, float duration) :
            Number(ptime),
            m_min(min),
            m_max(max),
            m_duration(duration)
        {
        }

        void Evaluate()
        {
            GetValueInternal() =
                m_min + (sin(2 * pi * GetTime() / m_duration) + 1) * 0.5f * (m_max - m_min);
        }
    };

    TRef<Number> m_pnumberAngle;

    void AddConeGeoTest()
    {
        m_pnumberAngle =
            GetInputEngine()->GetJoystick(0)->GetValue(0);
            //new Number(pi * 0.25f);
            //new SweepNumber(GetTime(), 0.1f, 0.5f * pi, 5);

        m_pgroupGeo->AddGeo(
            new TransformGeo(
                new MaterialGeo(
                    CreateConeGeo(m_pnumberAngle),
                    CreateMaterial(Color(1.0, 1.0, 1.0))
                ),
                new AnimateRotateTransform(
                    new VectorValue(Vector(0, 1, 0)),
                    GetInputEngine()->GetJoystick(0)->GetValue(1)
                )
            )
        );
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    ~MDLEditWindow()
    {
        m_pcamera      = NULL;
        m_pgroupGeo    = NULL;
        m_pgroupImage  = NULL;
        m_pnumberFrame = NULL;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    bool IsValid()
    {
        return EngineWindow::IsValid();
    }

    void RestoreCursor()
    {
        SetCursorImage(m_pimageCursor);
    }

    TRef<Pane> CreateGeoPane(const Point& size)
    {
        TRef<Camera> m_pcamera = new Camera();
        m_pcamera->SetZClip(1, 10000);
        m_pcamera->SetFOV(RadiansFromDegrees(50));
        m_pcamera->SetPosition(Vector(0, 0, 10));

        TRef<Viewport> m_pviewport =
            new Viewport(
                m_pcamera,
                new RectValue(Rect(Point(0, 0), size))
            );

        TRef<GeoImage> pimage =
                new GeoImage(
                    new TransformGeo(
                        new TransformGeo(
                            GetModeler()->LoadGeo("bom01a"),
                            new AnimateRotateTransform(
                                new VectorValue(Vector(0, 1, 0)),
                                NumberTransform::Multiply(GetTime(), new Number(1.0))
                            )
                        ),
                        new RotateTransform(Vector(1, 0, 0), pi/8)
                    ),
                    m_pviewport,
                    true
                );

        return new AnimatedImagePane(pimage);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////
/*
    TRef<Network>        m_pnetwork;
    bool                 m_bServer;
    int                  m_nmessage;
    TList<TRef<Socket> > m_listSockets;

    void Receive(Socket* psocket, const void* pbufArg, DWORD length)
    {
        BYTE* pbuf = (BYTE*)pbufArg;

        while (length > 0) {
            ZString str(pbuf);

            ZDebugOutput("Received " + str + "\n");

            pbuf   += str.GetLength() + 1;
            length -= str.GetLength() + 1;
        }
    }

    void Disconnect(Socket* psocket)
    {
        m_listSockets.Remove(psocket);
    }

    TRef<SocketSite> Connect(Socket* psocket)
    {
        m_listSockets.PushFront(psocket);
        return this;
    }

    void StartServer()
    {
        m_nmessage = 0;
        m_bServer  = true;
        m_bClient  = false;
        m_pnetwork = CreateNetwork(this, 5000);
    }

    void ServerDoIdle()
    {
        m_pnetwork->Receive();

        //
        // Send some messages
        //

        {
            TList<TRef<Socket> >::Iterator iter(m_listSockets);

            while (!iter.End()) {
                Socket* psocket = iter.Value();

                psocket->Send("Hi client, message " + ZString(m_nmessage), true);
                m_nmessage++;

                iter.Next();
            }
        }

        Sleep(250);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    bool         m_bClient;
    TRef<Socket> m_psocket;

    void StartClient()
    {
        m_nmessage = 0;
        m_bServer  = false;
        m_bClient  = true;
        m_pnetwork = CreateNetwork(NULL, 0);
        m_psocket  = m_pnetwork->ConnectToServer("mikekehome", 5000, this);
    }

    void ClientDoIdle()
    {
        m_pnetwork->Receive();
        m_psocket->Send("Hi Server!" + ZString(m_nmessage), false);
        m_nmessage++;
        Sleep(100);
    }
*/

/*
    class CullGeo : public WrapGeo {
    private:
        float m_radius;

    public:
        CullGeo(Geo* pgeo) :
            WrapGeo(pgeo)
        {
            m_radius = GetGeo()->GetRadius(Matrix::GetIdentity());
        }

        void Evaluate()
        {
        }

        void Render(Context* pcontext)
        {
            bool bNoClipping;

            if (!pcontext->IsCulled(Vector::GetZero(), m_radius, bNoClipping)) {
                if (bNoClipping) {
                    pcontext->SetClipping(false);
                }

                //float screenRadius = pcontext->GetScreenRadius(Vector::GetZero(), m_radius);
                //pcontext->SetLOD(screenRadius * std::max(m_lodBiasMin, s_lodBias));

                GetGeo()->Render(pcontext);

                if (bNoClipping) {
                    pcontext->SetClipping(true);
                }
            }
        }
    };

*/

    void AddDebris()
    {
		TRef<ModifiableNumber> m_mdlDebrisDensity = new ModifiableNumber(1.0f);
        m_pgroupGeo->AddGeo(
            CreateDebrisGeo(
                GetModeler(),
                GetTime(),
                m_pviewport,
				m_mdlDebrisDensity
            )
        );
    }

    void AddThingGeo()
    {
        //
        // Add a bits geo and a smoke geo
        //

        m_pParticleGeo = CreateParticleGeo(GetModeler(), GetTime());
        m_pgroupGeo->AddGeo(m_pParticleGeo);

        m_pbitsGeo = CreateBitsGeo(GetModeler(), GetTime());
        m_pgroupGeo->AddGeo(m_pbitsGeo);

    }

    void AddLODScrollBar()
    {
        //
        // Add the LOD scroll bar
        //

        TRef<Pane> ppane =
            CreateScrollPane(
                WinPoint(128, 10),
                100, 10, 1, 90,
                m_pscrollPane,
                m_peventLOD
            );
        m_peventLOD->AddSink(IIntegerEventSink::CreateDelegate(this));

        m_pbooleanVisibleLOD = new ModifiableBoolean(false);

        m_pgroupImage->AddImage(
            CreateVisibleImage(
                CreatePaneImage(
                    GetEngine(),
                    false,
                    ppane
                ),
                m_pbooleanVisibleLOD
            )
        );
    }

    void AddSphere()
    {
        /*
        m_pgroupGeo->AddGeo(
            new CullModeGeo(
                MakeSphere(1),
                CullModeNone
            )
        );
        */
    }

    bool OnEvent(IIntegerEventSource* pevent, int value)
    {
        if (pevent == m_peventLOD) {
            ThingGeo::SetLODBias(((float)value) / 100);
            //GetEngine()->SetClearColor(Color(0, 0, (float)value / 100));
        }
        return true;
    }

    void AddIcosahedron()
    {
        m_pgroupGeo->AddGeo(Geo::GetIcosahedron());
    }

    void OpenXFile(const ZString& strFilename)
    {
        m_pnumberFrame = new ModifiableNumber(0);

        bool bAnimation;
        TRef<Geo> pgeo = GetModeler()->LoadXFile(strFilename, m_pnumberFrame, bAnimation);

        if (pgeo == NULL) {
            if (bAnimation) {
                m_pnumberFrame = NULL;
            }
        } else {
            m_pgroupGeo->AddGeo(pgeo);
        }
    }

    TRef<INameSpace> OpenMDL(const ZString& strFilename)
    {
        m_pnumberFrame = NULL;

        TRef<INameSpace> pns = GetModeler()->GetNameSpace(strFilename);

        if (pns) {
            TRef<Geo> pgeo;

            CastTo(  pgeo,         pns->FindMember("object"));
            CastTo(m_pnumberFrame, pns->FindMember("frame"));

            m_pgroupGeo->AddGeo(pgeo);
        }

        return pns;
    }

    void OpenImage(const PathString& strPath)
    {
        TRef<INameSpace> pns = GetModeler()->GetNameSpace(strPath);

        if (pns) {
            TRef<Image> pimage; CastTo(pimage, (Value*)(pns->FindMember(strPath.GetName())));

            if (pimage) {
                m_pwrapImageInput->SetImage(pimage);
            }
        }
    }

    TRef<ThingGeo> CreateThingGeo(const ZString& strFilename, const Vector& vec)
    {
        TRef<INameSpace> pns = GetModeler()->GetNameSpace(strFilename);

        if (pns) {
            TRef<ThingGeo> pthing = ThingGeo::Create(GetModeler(), GetTime());
            pthing->LoadMDL(0, pns, NULL);
            pthing->SetPosition(vec);
            pthing->SetParticleGeo(m_pParticleGeo);
            pthing->SetBitsGeo(m_pbitsGeo);
            
            return pthing;
        }

        return NULL;
    }

    void OpenThingMDL(const ZString& strFilename, const Vector& vec)
    {
        m_pthing = CreateThingGeo(strFilename, vec);

        if (m_pthing) {
            {
                bool bAnimation;
                ZString str = strFilename + "_m.x";

                TRef<Geo> pgeo = GetModeler()->LoadXFile(str, new Number(0.0f), bAnimation, false);

                if (pgeo) {
                    m_pthing->SetBoundsGeo(pgeo);
                }
            }

            m_pgroupGeo->AddGeo(m_pthing->GetGeo());
        }
    }

    void OnClose()
    {
        RemoveAllChildren();
        EngineWindow::OnClose();
    }

    void EvaluateFrame(Time ttime)
    {
        //m_psoundEngine->Update();

        //UpdateExplosions();

        /* network
        if (m_bClient) {
            ClientDoIdle();
        }

        if (m_bServer) {
            ServerDoIdle();
        }
        */

        float time = GetTime()->GetValue();

        if (m_pnumberFrame) {
            m_pnumberFrame->SetValue(mod((time) * 10, 100));
        }

        if (m_pthing) {
            Orientation orient;

            if (m_bAnimate) {
                m_pthing->SetPosition(
                    Vector(5 * cos(time), 0, 5 * sin(time))
                );


                orient.Roll(time / pi);
                orient.Yaw(time);
            } else {
                m_pthing->SetPosition(Vector(0, 0, 0));
            }

            m_pthing->SetOrientation(orient);
        }
    }

    TRef<ISmoothPosition> m_psmoothPosition;

    void MoveCamera()
    {
        if (m_psmoothPosition == NULL) {
            m_psmoothPosition =
                CreateSmoothPosition(
                    GetTime(),
                    m_pcamera->GetPosition()
                );

            m_pcamera->SetPositionValue(m_psmoothPosition);
            m_pcamera->SetOrientationValue(
                CreateLookAtOrientation(
                    new VectorValue(Vector(0, 0, 0)),
                    m_psmoothPosition,
                    new VectorValue(Vector(0, 1, 0))
                )
            );
        }

        m_psmoothPosition->Goto(
            m_pcamera->GetPosition() +
                Vector(
                    random(-100, 100),
                    random(-100, 100),
                    random(-100, 100)
            ),
            2
        );
    }

    class PopupImpl : public IPopup {
    private:
        IPopupContainer* m_pcontainer;
        IPopup*          m_ppopupOwner;
        TRef<Pane>       m_ppane;

    public:
        PopupImpl(Modeler* pmodeler) :
            m_pcontainer(NULL),
            m_ppopupOwner(NULL)
        {
            m_ppane =
                new ImagePane(
                    pmodeler->LoadImage("btnjoinbmp", false) //"btncrimsonbmp"
                );
        }

        //
        // IKeyboardInput
        //

        bool OnChar(IInputProvider* pprovider, const KeyState& ks)
        {
            return true;
        }

        bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
        {
            if (ks.vk == VK_RETURN) {
                if (m_ppopupOwner) {
                    m_ppopupOwner->ClosePopup(this);
                } else {
                    m_pcontainer->ClosePopup(this);
                }
            }

            return true;
        }

        //
        // IPopup methods
        //

        void SetContainer(IPopupContainer* pcontainer)
        {
            m_pcontainer = pcontainer;
        }

        void SetOwner(IPopup* ppopupOwner)
        {
            m_ppopupOwner = ppopupOwner;
        }

        void ClosePopup(IPopup* ppopup)
        {
        }

        Pane* GetPane()
        {
            return m_ppane;
        }
    };

    TRef<IMenu>             m_pmenu;
    TRef<IMenuCommandSink>  m_pmenuCommandSink;
    TRef<ISubmenuEventSink> m_psubmenuEventSink;

    #define idmOptions           1
    #define idmExit              2

    void InitializeMenu()
    {
        m_pmenuCommandSink  = IMenuCommandSink::CreateDelegate(this);
        m_psubmenuEventSink = ISubmenuEventSink::CreateDelegate(this);
    }

    IEngineFont* GetFont()
    {
        return GetModeler()->GetNameSpace("model")->FindFont("defaultFont");
    }

    TRef<IPopupContainer> GetPopupContainer() {
        return CreatePopupContainer(); //todo: Rock is breaking things badly
    }

    void ShowMenu()
    {
         m_pmenu =
             CreateMenu(
                 GetModeler(),
                 GetFont(),
                 m_pmenuCommandSink
             );

         m_pmenu->AddMenuItem(idmOptions  , "Engine Options", 0, m_psubmenuEventSink);
         m_pmenu->AddMenuItem(idmExit     , "Exit"                                  );

         Point point(10, 100);
         GetPopupContainer()->OpenPopup(m_pmenu, Rect(point, point));
    }

    void CloseMenu()
    {
        GetPopupContainer()->ClosePopup(m_pmenu);
        m_pmenu = NULL;
    }

    TRef<IPopup> GetSubMenu(IMenuItem* pitem)
    {
        TRef<IMenu> pmenu =
            CreateMenu(
                GetModeler(),
                GetFont(),
                m_pmenuCommandSink
            );

        switch (pitem->GetID()) {
            case idmOptions:
                return GetEngineMenu(GetFont());
                break;
        }

        return pmenu;
    }

    void OnMenuCommand(IMenuItem* pitem)
    {
        switch (pitem->GetID()) {
            case idmExit:
                CloseMenu();
                PostMessage(WM_CLOSE);
                break;
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnKey(const KeyState& ks)
    {
        if (Window::OnKey(ks)) {
            return true;
        }

        if (ks.bDown) {
            switch(ks.vk) {
                case 'B':
                    ThingGeo::SetShowBounds(!ThingGeo::GetShowBounds());
                    break;

                case 'T':
                    ThingGeo::SetTransparentObjects(!ThingGeo::GetTransparentObjects());
                    break;

                case 'O':
                    m_angle -= 0.1f;
                    break;

                case 'P':
                    m_angle += 0.1f;
                    break;

                case VK_ESCAPE:
                    ShowMenu();
                    return true;

                case 'M':
                    m_bAnimate = !m_bAnimate;
                    break;

                case 'N':
                    if (m_ptestGeo) {
                        m_ptestGeo->NextTest();
                    }
                    break;

                case 'G':
                    m_bGlow    = !m_bGlow;
                    if (m_pthing) {
                        if (m_bGlow) {
                            m_pthing->SetThrust(1);
                        } else {
                            m_pthing->SetThrust(0);
                        }
                    }
                    break;

                case 'C':
                    m_bClearColor = !m_bClearColor;
                    if (m_bClearColor) {
                        m_pcolorBackground->SetValue(Color(0.25, 0, 0.25));
                    } else {
                        m_pcolorBackground->SetValue(Color(0.0, 0, 0.0));
                    }
                    break;

                case 'X':
                    {
                        TRef<IMessageBox> pmsgBox = CreateMessageBox(this, GetModeler(), "Message Box", NULL, true, NULL, 0.0f);
                        GetPopupContainer()->OpenPopup(pmsgBox, false);
                    }
                    break;

                case 'L':
                    m_pbooleanVisibleLOD->SetValue(
                        !m_pbooleanVisibleLOD->GetValue()
                    );
                    break;
            }
        }

        if (m_pscrollPane) {
            if (ks.bDown) {
                switch(ks.vk) {
                    
                    case VK_SPACE:
                        {
                            GetPopupContainer()->OpenPopup(
                                new PopupImpl(GetModeler()),
                                Rect(20, 20, 20, 20),
                                false
                            );
                            m_pscrollPane->SetPos(0);
                        }
                        return true;
                    

                    case 0xdb: // '['
                        m_pscrollPane->PageUp();
                        return true;

                    case 0xdd: // ']'
                        m_pscrollPane->PageDown();
                        return true;
                }
            }
        }
        return false;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// MDLEdit Application
//
//////////////////////////////////////////////////////////////////////////////

class MDLEdit : public EffectApp {
protected:
   TRef<MDLEditWindow> m_pwindow;

public:
    HRESULT Initialize(const ZString& strCommandLine)
    {
		PathString pathStr;
        HKEY hKey;
        DWORD dwType;
        char  szValue[MAX_PATH];
        DWORD cbValue = MAX_PATH;

        if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT, 0, KEY_READ, &hKey))
        {
            // Get the art path from the registry
            if (ERROR_SUCCESS != ::RegQueryValueEx(hKey, "ArtPath", NULL, &dwType, (unsigned char*)&szValue, &cbValue))
            {
                // Set ArtPath to be relative to the application path
                GetModuleFileNameA(NULL, szValue, MAX_PATH);
                char*   p = strrchr(szValue, '\\');
                if (!p)
                    p = szValue;
                else
                    p++;

                strcpy(p, "artwork");

                //Create a subdirectory for the artwork (nothing will happen if it already there)
                CreateDirectoryA(szValue, NULL);
            }
            pathStr = szValue;
		}

        TRef<UpdatingConfiguration> pConfiguration = new UpdatingConfiguration(
            std::make_shared<FallbackConfigurationStore>(
                CreateJsonConfigurationStore(GetExecutablePath() + "\\config_mdledit.json"),
                std::make_shared<RegistryConfigurationStore>(HKEY_CURRENT_USER, ALLEGIANCE_REGISTRY_KEY_ROOT "\\MDLEdit3DSettings")
            )
        );

		// Ask the user for video settings.
		if( PromptUserForVideoSettings(false, 0, GetModuleHandle(NULL), pathStr, pConfiguration) == false )
		{
			return E_FAIL;
		}
		CD3DDevice9::Get()->UpdateCurrentMode( );


        //
        // parse for -test
        //

        bool bTest       = false;
        bool bImageTest  = false;
        int  initialTest = -1;

        {
            PCC pcc = strCommandLine;
            CommandLineToken token(pcc, strCommandLine.GetLength());

            while (token.MoreTokens()) {
                ZString str;

                if (token.IsMinus(str)) {
                    if (str == "test") {
                        bTest = true;

                        float value;
                        if (token.IsNumber(value)) {
                            initialTest = int(value);
                        }

                        break;
                    }
                } else {
                    token.Skip();
                }
            }
        }

        //
        // Create the window
        //

        m_pwindow = new MDLEditWindow(this, new EngineConfigurationWrapper(pConfiguration), strCommandLine, bImageTest, bTest, initialTest, pathStr);

        //
        // Parse the command line
        //

        {
            PCC pcc = strCommandLine;
            CommandLineToken token(pcc, strCommandLine.GetLength());

            while (token.MoreTokens()) {
                ZString strInput;
                ZString str;

                if (token.IsMinus(str)) {
                    if (str == "image") {
                        ZString strInput;
                        if (token.IsString(strInput)) {
                            m_pwindow->OpenImage(strInput);
                            return S_OK;
                        }
                    } else if (str == "test") {
                        float value;
                        if (token.IsNumber(value)) {
                        }
                    } else if (str == "imageTest") {
                        bImageTest = true;
                    }
                    /*
                    } else if (str == "client") {
                        m_pwindow->StartClient();
                    } else if (str == "server") {
                        m_pwindow->StartServer();
                    }
                    */
                } else if (token.IsString(strInput)) {
                    PathString strPath(strInput);

                    if (strPath.GetExtension() == "x") {
                        m_pwindow->OpenXFile(strInput);
                    } else {
                        m_pwindow->OpenThingMDL(strInput, Vector(0, 0, 0));
                    }

                    return S_OK;
                }
            }
        }

        return S_OK;
    }

    void Terminate()
    {
        m_pwindow = NULL;
        EffectApp::Terminate();
    }
} g_app;
