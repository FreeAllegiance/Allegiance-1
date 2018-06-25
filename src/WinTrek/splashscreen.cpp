#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Splash Screen
//
//////////////////////////////////////////////////////////////////////////////

class SplashScreen :
    public Screen,
    public EventTargetContainer<SplashScreen>,
    public IKeyboardInput
{
private:
    TRef<Pane>       m_ppane;
    TRef<ButtonPane> m_pbuttonQuit;
    TRef<IKeyboardInput>   m_keyboardDelegate;

public:
    SplashScreen(Modeler* pmodeler)
    {
        TRef<INameSpace> pns = pmodeler->GetNameSpace("splashscreen");
        CastTo(m_ppane, pns->FindMember("screen"));
        pmodeler->UnloadNameSpace("splashscreen");

        if (g_bQuickstart || true) // REVIEW
			// mdvalley: Needs a class name and pointer.
			AddEventTarget(&SplashScreen::OnTimeout, GetEngineWindow(), 0.01f);
        else
			AddEventTarget(&SplashScreen::OnTimeout, GetEngineWindow(), 6.85f);

        m_keyboardDelegate = IKeyboardInput::CreateDelegate(this);
        GetEngineWindow()->AddKeyboardInputFilter(m_keyboardDelegate);
    }

    ~SplashScreen()
    {
        GetEngineWindow()->RemoveKeyboardInputFilter(m_keyboardDelegate);
    }

    bool OnTimeout()
    {
        GetWindow()->screen(ScreenIDIntroScreen);
        return false;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IMouseInput
    //
    //////////////////////////////////////////////////////////////////////////////

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        GetWindow()->screen(ScreenIDIntroScreen);
        return MouseResult();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IKeyboardInput
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        if (ks.bDown)
            GetWindow()->screen(ScreenIDIntroScreen);
        return false;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateSplashScreen(Modeler* pmodeler)
{
    return new SplashScreen(pmodeler);
}
