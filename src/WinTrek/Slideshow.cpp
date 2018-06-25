#include "pch.h"
#include "slideshow.h"


//////////////////////////////////////////////////////////////////////////////
//
// Training Screen
//
//////////////////////////////////////////////////////////////////////////////

void    Slideshow::CleanUpTimers (void)
{
    if (m_bInTimer)
    {
        // if a timer event had already been fired, then we need to remove it now
        ITimerEventSource*  pTimer = GetEngineWindow()->GetTimer ();
        pTimer->RemoveSink (m_pEventSink);
        m_bInTimer = false;
    }
}

void    Slideshow::StopSound (void)
{
    // check to see if there is a sound already playing
    if (static_cast<ISoundInstance*> (m_pSoundInstance))
        if (m_pSoundInstance->IsPlaying () == S_OK)
        {
            m_pSoundInstance->GetFinishEventSource ()->RemoveSink(m_pEventSink);
            m_pSoundInstance->Stop (true);
        }
}

Slideshow::Slideshow (Modeler* pmodeler, const ZString& strNamespace) :
m_bNextSlide (false),
m_bInTimer (false)
{
    // create the wrap image
    m_pWrapImage = new WrapImage(Image::GetEmpty());
    m_pImage = new GroupImage(new PickImage (this), m_pWrapImage);

    // create a namespace for exporting information
    TRef<INameSpace>    pTrainingDataNameSpace = pmodeler->CreateNameSpace ("SlideshowData", pmodeler->GetNameSpace ("gamepanes"));

    // Load the members from MDL
    TRef<INameSpace> pNameSpace = pmodeler->GetNameSpace(strNamespace);
    CastTo(m_pSlideList,    pNameSpace->FindMember("slides"));
    pmodeler->UnloadNameSpace(strNamespace);

    // create a delegate for myself
    m_pEventSink = IEventSink::CreateDelegate(this);

    // make the keyboard inputs come to us
    m_pkeyboardInputOldFocus = GetEngineWindow()->GetFocus();
    GetEngineWindow()->SetFocus(IKeyboardInput::CreateDelegate(this));

    // set the flag to indicate we are in a slideshow
    m_bIsInSlideShow = true;

    // start with the first slide
    m_pSlideList->GetFirst();
    NextSlide ();
}

Slideshow::~Slideshow (void)
{
    // set the flag to indicate we are no longer in a slideshow
    m_bIsInSlideShow = false;

    // clean up timers so nothing fires an event at us when we're gone
    CleanUpTimers ();

    // stop sounds so nothing fires an event at us when we're gone
    StopSound ();

    // reset the focus for inputs
    GetEngineWindow()->SetFocus(m_pkeyboardInputOldFocus);
}

// do the work
void    Slideshow::NextSlide (void)
{
    // stop any playing sounds
    StopSound ();

    // clean up any existing timers
    CleanUpTimers ();

    // check to see if there are any more slides to show
    if (m_pSlideList->GetCurrent() != NULL)
    {
        // get the image and sound id for the slide
        TRef<IObjectPair> pPair;
        CastTo (pPair, m_pSlideList->GetCurrent());
        //TRef<Image>     pImage = Image::Cast((Value*)pPair->GetFirst ());
        TRef<Image>     pImage = Image::Cast (static_cast<Value*> (pPair->GetFirst ()));
        SoundID         soundID = static_cast<SoundID> (GetNumber (pPair->GetSecond ()));

        // advance to the next slide
        m_pSlideList->GetNext();

        // set the image to the next image in the list
        m_pWrapImage->SetImage (pImage);

        // start the sound
        m_pSoundInstance = GetWindow ()->StartSound (soundID);

        // wait for the sound to finish
        m_pSoundInstance->GetFinishEventSource ()->AddSink (m_pEventSink);
    }
    else
    {
        // get out of the slideshow
        Dismiss ();
    }

}

void    Slideshow::Dismiss (void)
{
}

// UI Events
bool    Slideshow::OnEvent (IEventSource* pEventSource)
{
    // get the timer event source
    ITimerEventSource*  pTimer = GetEngineWindow()->GetTimer ();

    // XXX something strange, in that timers are not passing the event source in,
    // so I am assuming that if there is an event source, then it is from the
    // current sound ending.
    if (pEventSource)
    {
        // delay one second before processing the event
        pTimer->AddSink (m_pEventSink, 1.0f);
        m_bInTimer = true;
    }
    else
    {
        // skip to the next slide on the next update
        m_bNextSlide = true;
        pTimer->RemoveSink (m_pEventSink);
        m_bInTimer = false;
    }
    return false;
}

void    Slideshow::Picked (void)
{
    NextSlide ();
}

// Screen Methods
Image*  Slideshow::GetImage (void)
{
    return m_pImage;
}

void    Slideshow::OnFrame (void) 
{
    if (m_bNextSlide)
    {
        m_bNextSlide = false;
        NextSlide ();
    }
}

bool        Slideshow::m_bIsInSlideShow = false;
bool        Slideshow::IsInSlideShow (void)
{
    return m_bIsInSlideShow;
}

//////////////////////////////////////////////////////////////////////////////
//
// IKeyboardInput
//
//////////////////////////////////////////////////////////////////////////////

bool    Slideshow::OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
{
    if (ks.bDown)
        NextSlide();
    return false;
}

