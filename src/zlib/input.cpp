#include "input.h"

//////////////////////////////////////////////////////////////////////////////
//
// Keyboard Input Delegate
//
//////////////////////////////////////////////////////////////////////////////

class KeyboardInputDelegate : public IKeyboardInput {
private:
    IKeyboardInput* m_pkeyboardInput;

public:
    KeyboardInputDelegate(IKeyboardInput* pkeyboardInput) :
        m_pkeyboardInput(pkeyboardInput)
    {
    }

    bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    {
        return m_pkeyboardInput->OnChar(pprovider, ks);
    }

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        return m_pkeyboardInput->OnKey(pprovider, ks, fForceTranslate);
    }

};

TRef<IKeyboardInput> IKeyboardInput::CreateDelegate(IKeyboardInput* pkeyboardInput)
{
    return new KeyboardInputDelegate(pkeyboardInput);
}

//////////////////////////////////////////////////////////////////////////////
//
// Mouse Input Delegate
//
//////////////////////////////////////////////////////////////////////////////

class MouseInputDelegate : public IMouseInput {
private:
    IMouseInput* m_pmouseInput;

public:
    MouseInputDelegate(IMouseInput* pmouseInput) :
        m_pmouseInput(pmouseInput)
    {
    }

    void RemoveCapture()
    {
        m_pmouseInput->RemoveCapture();
    }

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    {
        return m_pmouseInput->HitTest(pprovider, point, bCaptured);
    }

    void MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside)
    {
        m_pmouseInput->MouseMove(pprovider, point, bCaptured, bInside);
    }

    void MouseEnter(IInputProvider* pprovider, const Point& point)
    {
        m_pmouseInput->MouseEnter(pprovider, point);
    }

    void MouseLeave(IInputProvider* pprovider)
    {
        m_pmouseInput->MouseLeave(pprovider);
    }

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        return m_pmouseInput->Button(pprovider, point, button, bCaptured, bInside, bDown);
    }
};

TRef<IMouseInput> IMouseInput::CreateDelegate(IMouseInput* pmouseInput)
{
    return new MouseInputDelegate(pmouseInput);
}
