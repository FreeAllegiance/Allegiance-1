#include "efapp.h"

#include <EngineSettings.h>
#include <base.h>
#include <button.h>
#include <controls.h>
#include <geometry.h>
#include <token.h>

#include "efart.h"
#include "efimage.h"
#include "efpane.h"
#include "framedata.h"
#include "gamestate.h"
#include "help.h"

//////////////////////////////////////////////////////////////////////////////
//
// Trek Resources
//
//////////////////////////////////////////////////////////////////////////////

TRef<IEngineFont> g_pfontSmall;
TRef<IEngineFont> g_pfontSmallBold;
TRef<IEngineFont> g_pfontLarge;
TRef<IEngineFont> g_pfontLargeBold;
TRef<IEngineFont> g_pfontHuge;
TRef<IEngineFont> g_pfontHugeBold;

IEngineFont* TrekResources::SmallFont()
{
    return g_pfontSmall;
}

IEngineFont* TrekResources::SmallBoldFont()
{
    return g_pfontSmallBold;
}

IEngineFont* TrekResources::LargeFont()
{
    return g_pfontLarge;
}

IEngineFont* TrekResources::LargeBoldFont()
{
    return g_pfontLargeBold;
}

IEngineFont* TrekResources::HugeFont()
{
    return g_pfontHuge;
}

IEngineFont* TrekResources::HugeBoldFont()
{
    return g_pfontHugeBold;
}

void TrekResources::Initialize(Modeler* pmodeler)
{
	TRef<INameSpace> pns = pmodeler->GetNameSpace("font");

	g_pfontSmall     = pns->FindFont("smallFont");
	g_pfontSmallBold = pns->FindFont("smallBoldFont");
	g_pfontLarge     = pns->FindFont("largeFont");
	g_pfontLargeBold = pns->FindFont("largeBoldFont");
	g_pfontHuge      = pns->FindFont("hugeFont");
	g_pfontHugeBold  = pns->FindFont("hugeBoldFont");
}

//////////////////////////////////////////////////////////////////////////////
//
// Lights Geo
//
//////////////////////////////////////////////////////////////////////////////

class LightData {
public:
    LightData()
    {
    }

    Color  m_color;
    Vector m_vec;
    float  m_period;
    float  m_phase;
    float  m_rampUp;
    float  m_hold;
    float  m_rampDown;
};

class LightsGeo : public Geo {
private:
    typedef TList<LightData, DefaultNoEquals> LightDataList;

    TRef<Image>   m_pimage;
    LightDataList m_list;
    float         m_time;

    Number* GetTime() { return Number::Cast(GetChild(0)); }

public:
    LightsGeo(Modeler* pmodeler, Number* ptime) :
        Geo(ptime)
    {
        m_pimage = pmodeler->LoadImage(AWF_EFFECT_STROBE, true);

    }

    void AddLight(const LightData& data)
    {
        m_list.PushFront();
        m_list.GetFront() = data;
    }

    void AddLight(
        const Color& color,
        const Vector& vec,
        float period,
        float phase,
        float rampUp,
        float hold,
        float rampDown
    ) {
        m_list.PushFront();
        m_list.GetFront().m_color    = color;
        m_list.GetFront().m_vec      = vec;
        m_list.GetFront().m_period   = period;
        m_list.GetFront().m_phase    = phase;
        m_list.GetFront().m_rampUp   = rampUp;
        m_list.GetFront().m_hold     = hold;
        m_list.GetFront().m_rampDown = rampDown;
    }

    int GetCount()
    {
        return m_list.GetCount();
    }

    //
    // Geo members
    //

    float GetRadius(const Matrix& mat)
    {
        return 1;
    }

    void Render(Context* pcontext)
    {
        TRef<Surface> psurface =  m_pimage->GetSurface();

        LightDataList::Iterator iter(m_list);

        while (!iter.End()) {
            LightData& data = iter.Value();

            float value;

            while (true) {
                value = m_time - data.m_phase;
                if (value < data.m_period) {
                    break;
                }
                data.m_phase += data.m_period;
            }

            if (value < data.m_rampUp) {
                value /= data.m_rampUp;
            } else {
                value -= data.m_rampUp;

                if (value < data.m_hold) {
                    value = 1;
                } else {
                    value -= data.m_hold;

                    if (value < data.m_rampDown) {
                        value = 1 - (value / data.m_rampDown);
                    } else {
                        value = 0;
                    }
                }
            }

            if (value > 0) {
                pcontext->DrawDecal(
                    psurface, 
                    data.m_color, 
                    data.m_vec, 
                    Vector::GetZero(), 
                    Vector::GetZero(), 
                    0.5f * value, 
                    0
                );
            }

            iter.Next();
        }
    }

    //
    // Value members
    //

    void Evaluate()
    {
        m_time = GetTime()->GetValue();
    }

    ZString GetFunctionName()
    {
        return "LightsGeo";
    }

    ZString GetString(int indent)
    {
        ZString str = "Lights([\n";

        LightDataList::Iterator iter(m_list);

        while (!iter.End()) {
            LightData& data = iter.Value();

            str +=
                  Indent(indent + 1)
                + "( " + ::GetString(indent, data.m_color)
                + ", " + ::GetString(indent, data.m_vec)
                + ", " + ZString(data.m_period)
                + ", " + ZString(data.m_phase)
                + ", " + ZString(data.m_rampUp)
                + ", " + ZString(data.m_hold)
                + ", " + ZString(data.m_rampDown)
                + ")";

            iter.Next();

            if (iter.End()) {
                str += "\n";
            } else {
                str += ",\n";
            }
        }

        return str + Indent(indent) + "])";
    }

    void Write(IMDLBinaryFile* pmdlFile)
    {
        pmdlFile->WriteReference("LightsGeo");
        TRef<ZFile> pfile = pmdlFile->WriteBinary();

        pfile->Write(m_list.GetCount());

        LightDataList::Iterator iter(m_list);

        while (!iter.End()) {
            LightData& data = iter.Value();

            pfile->Write((void*)&data, sizeof(LightData));
            iter.Next();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Lights
//
//////////////////////////////////////////////////////////////////////////////

class LightsFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;
    TRef<Number> m_ptime;

public:
    LightsFactory(Modeler* pmodeler, Number* ptime) :
        m_pmodeler(pmodeler),
        m_ptime(ptime)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<LightsGeo> plights = new LightsGeo(m_pmodeler, m_ptime);

        TRef<IObjectList> plist;  CastTo(plist, (IObject*)stack.Pop());

        while (plist->GetCurrent() != NULL) {
            IObjectPair* ppair; CastTo(ppair, plist->GetCurrent());

            Color  color    =  GetColor(ppair->GetNth(0));
            Vector vector   = GetVector(ppair->GetNth(1));
            float  period   = GetNumber(ppair->GetNth(2));
            float  phase    = GetNumber(ppair->GetNth(3));
            float  rampUp   = GetNumber(ppair->GetNth(4));
            float  hold     = GetNumber(ppair->GetNth(5));
            float  rampDown = GetNumber(ppair->GetLastNth(6));

            plights->AddLight(color, vector, period, phase, rampUp, hold, rampDown);

            plist->GetNext();
        }

        return plights;
    }

    TRef<IObject> Read(IBinaryReaderSite* psite, ObjectStack& stack)
    {
        TRef<LightsGeo> plights = new LightsGeo(m_pmodeler, m_ptime);
        DWORD count = psite->GetDWORD();

        for(DWORD index = 0; index < count; index++) {
                LightData* pdata = (LightData*)psite->GetPointer();
                psite->MovePointer(sizeof(LightData));
                plights->AddLight(*pdata);
        }

        return plights;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// FrameData
//
//////////////////////////////////////////////////////////////////////////////

class FrameDataFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<FrameDataListValue> plistValue = new FrameDataListValue();

        TRef<IObjectList> plist;  CastTo(plist, (IObject*)stack.Pop());

        while (plist->GetCurrent()) {
            IObjectPair* ppair; CastTo(ppair, plist->GetCurrent());

            ZString strName     = GetString(ppair->GetNth(0));
            Vector  vecPosition = GetVector(ppair->GetNth(1));
            Vector  vecForward  = GetVector(ppair->GetNth(2));
            Vector  vecUp       = GetVector(ppair->GetLastNth(3));

            plistValue->GetList().PushEnd(
                FrameData(
                    strName,
                    vecPosition,
                    vecForward,
                    vecUp
                )
            );

            plist->GetNext();
        }

        return plistValue;
    }

    TRef<IObject> Read(IBinaryReaderSite* psite, ObjectStack& stack)
    {
        TRef<FrameDataListValue> plistValue = new FrameDataListValue();

        DWORD count = psite->GetDWORD();

        for(DWORD index = 0; index < count; index++) {
            ZString str = psite->GetString();

            Vector* pvecPosition; psite->GetStructure(pvecPosition);
            Vector* pvecForward;  psite->GetStructure(pvecForward);
            Vector* pvecUp;       psite->GetStructure(pvecUp);

            plistValue->GetList().PushEnd(
                FrameData(
                    str, 
                    *pvecPosition,
                    *pvecForward,
                    *pvecUp
                )
            );
        }

        return plistValue;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// GameStateContainer
//
//////////////////////////////////////////////////////////////////////////////

class GameStateContainerFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;

public:
    GameStateContainerFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<FontValue>  pfontTitles; CastTo(pfontTitles, (IObject*)stack.Pop());
        TRef<FontValue>  pfontTime;   CastTo(pfontTime,   (IObject*)stack.Pop());
        TRef<ButtonPane> pbutton;     CastTo(pbutton,     (IObject*)stack.Pop());

        return 
            CreateGameStateContainer(
                m_pmodeler,
                pfontTitles->GetValue(),
                pfontTime->GetValue(),
                pbutton
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// GameStateImage
//
//////////////////////////////////////////////////////////////////////////////

class GameStateImageFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<GameStateContainer> pgsc; CastTo(pgsc, (IObject*)stack.Pop());

        return (Value*)pgsc->GetImage();
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Model Optimizer
//
//////////////////////////////////////////////////////////////////////////////

class GroupGeoCallbackImpl : public GroupGeoCallback {
private:
    class Token : public TextTokenImpl {
    public:
        Token(PCC pcc, int length) :
            TextTokenImpl(pcc, length)
        {
            Next();
        }

        void ReadStrobe(float& phase, float& period)
        {
            if (IsNumber(phase, false)) {
                phase = -phase;
                if (IsNumber(period, false)) {
                    period = -period;
                }
            }
        }
    };

    TRef<LightsGeo>          m_plights;
    TRef<FrameDataListValue> m_pframes;

public:
    GroupGeoCallbackImpl(
        FrameDataListValue* pframes,
        LightsGeo*          plights
    ) :
        m_pframes(pframes),
        m_plights(plights)
    {
    }

    void AddLight(
        const ZString& str,
        const Color&   color,
        const Vector&  vecPosition
    ) {
        //
        // default values
        //

        float period   = 1.25;
        float phase    = 0;
        float rampUp   = 0.1f;
        float hold     = 0.0f;
        float rampDown = 0.05f;

        //
        // parse the string
        //

        Token(str, str.GetLength()).ReadStrobe(phase, period);


        //
        // Add it
        //

        m_plights->AddLight(
            color,
            vecPosition,
            period, phase, rampUp, hold, rampDown
        );
    }

    TRef<Geo> Execute(const Matrix& mat, GroupGeo* pgroup)
    {
        ZString strName = pgroup->GetName();

        if (!strName.IsEmpty()) {
            if (   strName.Find("frm-") == 0
                && (!pgroup->AnyChildGroups())
            ) {
                Vector vecPosition = mat.Transform(Vector(0, 0, 0));
                Vector vecForward  = mat.TransformDirection(Vector(0, 0, -1));
                Vector vecUp       = mat.TransformDirection(Vector(0, 1,  0));

                strName = strName.RightOf(4);

                if (strName.Find("SS") != -1) {
                    //
                    // a strobe light
                    //

                    ValueList* plist = pgroup->GetList();

                    if (plist->GetCount() == 1) {
                        MaterialGeo* pmatGeo; CastTo(pmatGeo, plist->GetFirst());
                        Material* pmaterial = pmatGeo->GetMaterial();

                        AddLight(strName, pmaterial->GetDiffuse(), vecPosition);
                    } else {
                        AddLight(strName, Color(1, 1, 1), vecPosition);
                    }

                    return Geo::GetEmpty();
                } else if (
                       strName.Find("thrust") != -1
                    || strName.Find("smoke") != -1
                    || strName.Find("rocket") != -1
                ) {
                    //
                    // this is an engine
                    //

                    m_pframes->GetList().PushFront(
                        FrameData(strName, vecPosition, vecForward, vecUp)
                    );

                    return Geo::GetEmpty();
                } else if (
                       (strName.Find("weapon") != -1)
                    || (strName.Find("wepatt") != -1)
                    || (strName.Find("wepemt") != -1)
                    || (strName.Find("wepmnt") != -1)
                    || (strName.Find("trail")  != -1)
                ) {
                    //
                    // This is an attachment point
                    //

                    m_pframes->GetList().PushFront(
                        FrameData(strName, vecPosition, vecForward, vecUp)
                    );
                    return Geo::GetEmpty();
                } else if (
                       (strName.Find("garage") != -1)
                ) {
                    //
                    // This is a garage we need to leave the frame in the graph
                    //

                    m_pframes->GetList().PushFront(
                        FrameData(strName, vecPosition, vecForward, vecUp)
                    );
                }
            }
        }

        return NULL;
    }

    void ReportTriangles(const Matrix& mat, const TVector<Vertex>&  vertices, const TVector<WORD>& indices)
    {
        // we don't care about triangles here but might be nice to count for LOD analysis
    }

    void ReportTriangles(const Matrix& mat, const TVector<VertexL>& vertices, const TVector<WORD>& indices)
    {
    }

    void ReportTriangles(const Matrix& mat, const TVector<D3DLVertex>& vertices, const TVector<WORD>& indices)
    {
    }

    void ReportTriangles(const Matrix& mat, const TVector<D3DVertex>& vertices, const TVector<WORD>& indices)
    {
    }

};

//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////

class ScrollPositionNumber : public Number {
private:
    TRef<ScrollPane> m_pscroll;

public:
    ScrollPositionNumber(ScrollPane* pscroll) :
        m_pscroll(pscroll)
    {
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////

class BlendColorImageFactory : public IFunction {
public:
    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<ColorValue> pcolor = ColorValue::Cast((IObject*)stack.Pop());
        TRef<Image>      pimage =      Image::Cast((Value*)(IObject*)stack.Pop());

        return (Value*)CreateBlendColorImage(pimage, pcolor);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// EffectApp
//
//////////////////////////////////////////////////////////////////////////////

HRESULT EffectApp::Initialize(const ZString& strCommandLine, HWND hWindow )
{
    if( SUCCEEDED( EngineApp::Initialize( strCommandLine, hWindow ) ) ) 
	{
        TRef<INameSpace> pnsModel = GetModeler()->GetNameSpace("model");
        TRef<Number>     pnumberTime; CastTo(pnumberTime,  pnsModel->FindMember("time"));

        m_pns =
            GetModeler()->CreateNameSpace(
                "effect",
                pnsModel
            );

        //
        // Types
        //

        m_pns->AddType("QuickChatNode", CreateIObjectMDLType("QuickChatNode"  , "qcn"  ));
        m_pns->AddType("Sound"        , CreateIObjectMDLType("IObject", "sound"));

        //
        // Constructors
        //

        m_pns->AddMember("LightsGeo",          new LightsFactory(GetModeler(), pnumberTime));
        m_pns->AddMember("FrameData",          new FrameDataFactory()                      );
        m_pns->AddMember("GameStateContainer", new GameStateContainerFactory(GetModeler()) );
        m_pns->AddMember("GameStateImage",     new GameStateImageFactory()                 );
        m_pns->AddMember("BlendColorImage",    new BlendColorImageFactory()                );

        AddPaneFactories(m_pns, GetModeler(), GetPopupContainer(), pnumberTime);
        AddPagePaneFactory(m_pns, GetModeler());
        AddNavPaneFactory(m_pns);

        return S_OK;
    }

    return E_FAIL;
}

void EffectApp::Terminate()
{
    m_pns = NULL;
    EngineApp::Terminate( );
}

TRef<INameSpace> EffectApp::OptimizeThingGeo(const ZString& str, Geo* pgeo, Number* pnumber)
{
    //
    // Get the time value
    //

    TRef<INameSpace> pnsModel = GetModeler()->GetNameSpace("model");
    TRef<Number>     pnumberTime; CastTo(pnumberTime, pnsModel->FindMember("time"));

    //
    // we are going to create a namespace with all the stuff needed by a ThingGeo
    //

    TRef<INameSpace> pns = GetModeler()->CreateNameSpace(str, m_pns);

    if (pnumber != NULL) {
        pns->AddMember("frame", pnumber);
    }

    //
    // Wrap the Geo up so we don't lose the pointer as we do optimizations
    //

    TRef<WrapGeo> pwrap = new WrapGeo(pgeo);
    pwrap->Update();

    //
    // Add a LightsGeo to the Group
    //

    TRef<LightsGeo> plights = new LightsGeo(GetModeler(), pnumberTime);

    //
    // Add a list of frames
    //

    TRef<FrameDataListValue> pframes = new FrameDataListValue();

    //
    // Extract the lights and frames
    //

    ZDebugOutput("Before Callback: " + ZString(pwrap->GetGeo()->GetNodeCount()) + "\n");

    TRef<GroupGeoCallbackImpl> pcallback = new GroupGeoCallbackImpl(pframes, plights);
    pwrap->GetGeo()->CallGroupGeoCallback(Matrix::GetIdentity(), pcallback);

    //
    // If there are any lights or frames add them to the namespace
    //

    if (plights->GetCount() > 0) {
        pns->AddMember("lights", plights);
    }

    if (pframes->GetList().GetCount() > 0) {
        pns->AddMember("frames", pframes);
    }

    //
    // Optimize the geo
    //

    bool bAnyFold;
    TRef<Geo> pgeoFold;
    do {
        bAnyFold = false;
        ZDebugOutput("Before RemoveMaterial: " + ZString(pwrap->GetGeo()->GetNodeCount()) + "\n");

        pgeoFold = pwrap->GetGeo()->RemoveMaterial();

        if (pgeoFold) {
            bAnyFold = true;
            pwrap->SetGeo(pgeoFold);
            ZDebugOutput("Before           Fold: " + ZString(pwrap->GetGeo()->GetNodeCount()) + "\n");
            pwrap->GetGeo()->DoFold();
        }

        ZDebugOutput("Before    FoldTexture: " + ZString(pwrap->GetGeo()->GetNodeCount()) + "\n");
        pgeoFold = pwrap->GetGeo()->FoldTexture();

        if (pgeoFold) {
            bAnyFold = true;
            pwrap->SetGeo(pgeoFold);
            ZDebugOutput("Before           Fold: " + ZString(pwrap->GetGeo()->GetNodeCount()) + "\n");
            pwrap->GetGeo()->DoFold();
        }
        if (!bAnyFold) {
            ZDebugOutput("Before           Fold: " + ZString(pwrap->GetGeo()->GetNodeCount()) + "\n");
            bAnyFold = pwrap->GetGeo()->DoFold();
        }
    } while (bAnyFold);

    ZDebugOutput("After Optimization: " + ZString(pwrap->GetGeo()->GetNodeCount()) + "\n");

    //
    // We're done
    //

    pns->AddMember("object", pwrap->GetGeo());

    return pns;
}
