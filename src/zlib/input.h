#ifndef _input_h_
#define _input_h_

#include "mask.h"
#include "point.h"
#include "tref.h"

//////////////////////////////////////////////////////////////////////////////
//
// Forward declarations
//
//////////////////////////////////////////////////////////////////////////////

class IKeyboardInput;
class IMouseInput;
class IInputProvider;

//////////////////////////////////////////////////////////////////////////////
//
// Mouse Results
//
//////////////////////////////////////////////////////////////////////////////

class MouseResultBaseType {};
typedef TBitMask<MouseResultBaseType, uint32_t> MouseResult;

class MouseResultHit     : public MouseResult { public: MouseResultHit()     : MouseResult(0x02) {} };
class MouseResultCapture : public MouseResult { public: MouseResultCapture() : MouseResult(0x04) {} };
class MouseResultRelease : public MouseResult { public: MouseResultRelease() : MouseResult(0x08) {} };

//////////////////////////////////////////////////////////////////////////////
//
// Keyboard Input Interface
//
//////////////////////////////////////////////////////////////////////////////

class KeyState {
public:
    int  vk;
    bool bShift;
    bool bControl;
    bool bAlt;
    bool bDown;
    uint16_t countRepeat;
};

class IKeyboardInput : public IObject {
public:
    static TRef<IKeyboardInput> CreateDelegate(IKeyboardInput* pkeyboardInput);

    virtual bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    { 
        return false;
    }

    virtual bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    { 
        return false; 
    }

    virtual void SetFocusState(bool bFocus) {}
};

//////////////////////////////////////////////////////////////////////////////
//
// Mouse Input Interface
//
//////////////////////////////////////////////////////////////////////////////

class IMouseInput : public IObject {
public:
    static TRef<IMouseInput> CreateDelegate(IMouseInput* pmouseInput);

    virtual void RemoveCapture()
    {
    }

    virtual MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    { 
	    return MouseResult(); 
    }

    virtual void MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside) {}
    virtual void MouseEnter(IInputProvider* pprovider, const Point& point)                              {}
    virtual void MouseLeave(IInputProvider* pprovider)                                                  {}

    virtual MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    { 
	    return MouseResult(); 
    }
};

/////////////////////////////////////////////////////////////////////////////
//
// Input Provider
//
/////////////////////////////////////////////////////////////////////////////

class ITimerEventSource;

class IInputProvider : public IObject {
public:
    virtual ITimerEventSource* GetTimer()                          = 0;
    virtual IKeyboardInput*    GetFocus()                          = 0;
    virtual bool               IsDoubleClick()                     = 0;
    virtual bool               IsFocus(IKeyboardInput* pinput)     = 0;
    virtual void               SetFocus(IKeyboardInput* pinput)    = 0;
    virtual void               RemoveFocus(IKeyboardInput* pinput) = 0;
    virtual void               SetCursorPos(const Point& point)    = 0;
    virtual void               ShowCursor(bool bShow)              = 0;
};

#endif
