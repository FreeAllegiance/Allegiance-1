//
// soundtemplates.cpp
//
// Several useful sound template implementations.
//

#include "soundtemplates.h"

#include <algorithm>
#include <cast.h>
#include <list>
#include <zadapt.h>
#include <ztime.h>

#include "soundbase.h"
#include "soundutil.h"
#include "regkey.h"

namespace SoundEngine {

    bool bConvertToMono = false;

// a template for wave files
class WaveFileTemplate : public ISoundTemplate
{
private:
    TRef<ISoundPCMData> m_pdata;
    ZString m_strFilename;
public:
    // tries to initialize the object with the given file.
    HRESULT Init(const ZString& strFilename)
    {
        m_strFilename = strFilename;

        // check to make sure the file exists.
        HANDLE hFile;

        hFile = CreateFile(strFilename, 0, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, nullptr);

        if (INVALID_HANDLE_VALUE == hFile)
            return STG_E_FILENOTFOUND;
        else
        {
            CloseHandle(hFile);
            return S_OK;
        }
    };

    // tries to initialize the object with the given data object.
    HRESULT Init(ISoundPCMData* pdata)
    {
        if (!pdata)
            return E_INVALIDARG;

        m_pdata = pdata;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource = nullptr)
    {
        // if we have not loaded this sound yet...
        if (!m_pdata)
        {
            // try to load it.
            ZAssert(!m_strFilename.IsEmpty());

            if (FAILED(LoadWaveFile(m_pdata, m_strFilename, SoundEngine::bConvertToMono))) //AEM 7.4.07 sending new mono parameter
            {
                if (ZFailed(CreateDummyPCMData(m_pdata)))
                    return E_FAIL;
            }
        }

        if (!pbufferSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        return pbufferSource->CreateStaticBuffer(psoundNew, m_pdata, false, psource);
    }
};

// creates a sound template for the given wave file
HRESULT CreateWaveFileSoundTemplate(TRef<ISoundTemplate>& pstDest, const ZString& strFilename)
{
    TRef<WaveFileTemplate> ptemplate = new WaveFileTemplate();
    HRESULT hr = ptemplate->Init(strFilename);
    
	// Suppressed so debug version doesn't break on nonexistent wavs
    //if (ZSucceeded(hr))
	if (SUCCEEDED(hr))
        pstDest = ptemplate; 

    return hr;
};

// creates a dummy sound template 
// (NOTE: Not optimized - fix or use only for error handling!)
HRESULT CreateDummySoundTemplate(TRef<ISoundTemplate>& pstDest)
{
    HRESULT hr;
    TRef<ISoundPCMData> pdata;

    hr = CreateDummyPCMData(pdata);
    if (ZFailed(hr))
        return hr;

    TRef<WaveFileTemplate> ptemplate = new WaveFileTemplate();
    hr = ptemplate->Init(pdata);

    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};

// a template turning another template into a 3D sound
class ThreeDSoundTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;
    float m_fMinimumDistance;

public:
    // tries to initialize the object with the given file.
    HRESULT Init(ISoundTemplate* pstSource, float fMinimumDistance)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }
        if (fMinimumDistance <= 0)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_pstBase = pstSource;
        m_fMinimumDistance = fMinimumDistance;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        // REVIEW: Should we silently allow the 2D case?
        // we have to have a source to be 3D.
        /*if (!psource)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }*/

        HRESULT hr = m_pstBase->CreateSound(psoundNew, pbufferSource, psource);
        if (ZFailed(hr)) return hr;

        TRef<ISoundTweakable3D> ptweak3d = psoundNew->GetISoundTweakable3D();

        // we need to have a 3D sound for this to work.
        if (psource && !ptweak3d)
        {
            ZAssert(false);
            return E_FAIL;
        }

        if (ptweak3d)
        {
            hr = ptweak3d->Set3D(true);
            if (ZFailed(hr)) return hr;

            hr = ptweak3d->SetMinimumDistance(m_fMinimumDistance);
            if (ZFailed(hr)) return hr;
        }

        return S_OK;
    }
};

// creates a sound template which uses the given sound template to create 
// sounds and turns then into a 3D sounds with the given minimum distance.
HRESULT Create3DSoundTemplate(TRef<ISoundTemplate>& pstDest, 
                              ISoundTemplate* pstSource, float fMinimumDistance)
{
    TRef<ThreeDSoundTemplate> ptemplate = new ThreeDSoundTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fMinimumDistance);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};




// a template which adds a sound cone to another template
class SoundConeTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;
    float m_fInnerAngle, m_fOuterAngle, m_fOutsideGain;

public:
    // tries to initialize the object with the given file.
    HRESULT Init(ISoundTemplate* pstSource, float fInnerAngle, float fOuterAngle, float fOutsideGain)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }
        if ((fInnerAngle < 0 || fInnerAngle > 360)
            || (fOuterAngle < 0 || fOuterAngle > 360)
            || (fOutsideGain < -100 || fOutsideGain > 0)
            )
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_pstBase = pstSource;
        m_fInnerAngle = fInnerAngle;
        m_fOuterAngle = fOuterAngle;
        m_fOutsideGain = fOutsideGain;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        // we have to have a source to be 3D.
        if (!psource)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        HRESULT hr = m_pstBase->CreateSound(psoundNew, pbufferSource, psource);
        if (ZFailed(hr)) return hr;

        TRef<ISoundTweakable3D> ptweak3d = psoundNew->GetISoundTweakable3D();

        // we need to have a 3D sound for this to work.
        if (!ptweak3d)
        {
            ZAssert(false);
            return E_FAIL;
        }

        hr = ptweak3d->SetCone(m_fInnerAngle, m_fOuterAngle, m_fOutsideGain);

        return S_OK;
    }
};

// creates a sound template which adds a sound cone to the given sound
HRESULT CreateSoundConeTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                float fInnerAngle, float fOuterAngle, float fOutsideGain)
{
    TRef<SoundConeTemplate> ptemplate = new SoundConeTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fInnerAngle, fOuterAngle, fOutsideGain);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};



// a template adjusting the pitch of another template
class SoundPitchTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;
    float m_fPitch;
public:
    // a sound instance wrapper which adjusts pitch changes
    class PitchInstance : public SoundInstanceWrapper, public SoundTweakableWrapper
    {
        float m_fPitch;
    public:
        PitchInstance(float fPitch, ISoundInstance* pinstance, 
                ISoundTweakable* ptweak) :
            SoundInstanceWrapper(pinstance), 
            SoundTweakableWrapper(ptweak), 
            m_fPitch(fPitch) 
            {};

        // Sets the pitch shift, where 1.0 is normal, 0.5 is half of normal speed, 
        // and 2.0 is twice normal speed.  
        HRESULT SetPitch(float fPitch)
        {
            return SoundTweakableWrapper::SetPitch(m_fPitch * fPitch);
        };

        // Gets an interface for tweaking the sound, if supported, nullptr otherwise.
        TRef<ISoundTweakable> GetISoundTweakable()
        {
            return this;
        };
    };

public:
    HRESULT Init(ISoundTemplate* pstSource, float fPitch)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }
        if (fPitch < 0.25f || fPitch > 4.0f)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_pstBase = pstSource;
        m_fPitch = fPitch;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        HRESULT hr = m_pstBase->CreateSound(psoundNew, pbufferSource, psource);
        if (ZFailed(hr)) return hr;

        TRef<ISoundTweakable> ptweak = psoundNew->GetISoundTweakable();

        // we need to have a tweakable sound for this to work.
        if (!ptweak)
        {
            ZAssert(false);
            return E_FAIL;
        }

        hr = ptweak->SetPitch(m_fPitch);
        if (ZFailed(hr)) return hr;

        psoundNew = new PitchInstance(m_fPitch, psoundNew, ptweak);

        return S_OK;
    }
};

// creates a sound template which shifts the pitch of the created sound
HRESULT CreatePitchSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                 float fPitch)
{
    TRef<SoundPitchTemplate> ptemplate = new SoundPitchTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fPitch);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};



// a template adjusting the gain of another template
class SoundGainTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;
    float m_fGain;
public:
    // a sound instance wrapper which adjusts Gain changes
    class GainInstance : public SoundInstanceWrapper, public SoundTweakableWrapper
    {
        float m_fGain;
    public:
        GainInstance(float fGain, ISoundInstance* pinstance, 
                ISoundTweakable* ptweak) :
            SoundInstanceWrapper(pinstance), 
            SoundTweakableWrapper(ptweak), 
            m_fGain(fGain) 
            {};

        // Sets the Gain shift, where 1.0 is normal, 0.5 is half of normal speed, 
        // and 2.0 is twice normal speed.  
        HRESULT SetGain(float fGain)
        {
            return SoundTweakableWrapper::SetGain(m_fGain + fGain);
        };

        // Gets an interface for tweaking the sound, if supported, nullptr otherwise.
        TRef<ISoundTweakable> GetISoundTweakable()
        {
            return this;
        };
    };

public:
    HRESULT Init(ISoundTemplate* pstSource, float fGain)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }
        if (fGain > 0.0f || fGain < -100.0f)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_pstBase = pstSource;
        m_fGain = fGain;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        HRESULT hr = m_pstBase->CreateSound(psoundNew, pbufferSource, psource);
        if (ZFailed(hr)) return hr;

        TRef<ISoundTweakable> ptweak = psoundNew->GetISoundTweakable();

        // we need to have a tweakable sound for this to work.
        if (!ptweak)
        {
            ZAssert(false);
            return E_FAIL;
        }

        hr = ptweak->SetGain(m_fGain);
        if (ZFailed(hr)) return hr;

        psoundNew = new GainInstance(m_fGain, psoundNew, ptweak);

        return S_OK;
    }
};

// creates a sound template which adjusts the gain of the created sound
HRESULT CreateGainSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                 float fGain)
{
    TRef<SoundGainTemplate> ptemplate = new SoundGainTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fGain);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};



// a template adjusting the Priority of another template
class SoundPriorityTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;
    float m_fPriority;
public:
    // a sound instance wrapper which adjusts Priority changes
    class PriorityInstance : public SoundInstanceWrapper, public SoundTweakableWrapper
    {
        float m_fPriority;
    public:
        PriorityInstance(float fPriority, ISoundInstance* pinstance, 
                ISoundTweakable* ptweak) :
            SoundInstanceWrapper(pinstance), 
            SoundTweakableWrapper(ptweak), 
            m_fPriority(fPriority) 
            {};

        // Sets the Priority shift, where 1.0 is normal, 0.5 is half of normal speed, 
        // and 2.0 is twice normal speed.  
        HRESULT SetPriority(float fPriority)
        {
            return SoundTweakableWrapper::SetPriority(m_fPriority + fPriority);
        };

        // Gets an interface for tweaking the sound, if supported, nullptr otherwise.
        TRef<ISoundTweakable> GetISoundTweakable()
        {
            return this;
        };
    };

public:
    HRESULT Init(ISoundTemplate* pstSource, float fPriority)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        m_pstBase = pstSource;
        m_fPriority = fPriority;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        HRESULT hr = m_pstBase->CreateSound(psoundNew, pbufferSource, psource);
        if (ZFailed(hr)) return hr;

        TRef<ISoundTweakable> ptweak = psoundNew->GetISoundTweakable();

        // we need to have a tweakable sound for this to work.
        if (!ptweak)
        {
            ZAssert(false);
            return E_FAIL;
        }

        hr = ptweak->SetPriority(m_fPriority);
        if (ZFailed(hr)) return hr;

        psoundNew = new PriorityInstance(m_fPriority, psoundNew, ptweak);

        return S_OK;
    }
};

// creates a sound template which adjusts the Priority of the created sound
HRESULT CreatePrioritySoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                 float fPriority)
{
    TRef<SoundPriorityTemplate> ptemplate = new SoundPriorityTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fPriority);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};


class LoopingSoundTemplate : public ISoundTemplate
{
    TRef<ISoundTemplate> m_pstBase;

    // a wrapper for a buffer source which forces all sounds to be looping sounds
    class LoopingSoundBufferSource : public SoundBufferSourceWrapper
    {
    public:
        LoopingSoundBufferSource(ISoundBufferSource* pBase) :
            SoundBufferSourceWrapper(pBase) {};

        // Creates a static sound buffer of the given wave file.  
        // The sound will loop until stopped.
        HRESULT CreateStaticBuffer(TRef<ISoundInstance>& psoundNew, 
            ISoundPCMData* pcmdata, bool bLooping, ISoundPositionSource* psource)
        {
            return m_pBase->CreateStaticBuffer(psoundNew, pcmdata, true, psource);
        }
    };

public:
    // tries to initialize the object with the given file.
    HRESULT Init(ISoundTemplate* pstSource)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        m_pstBase = pstSource;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource = nullptr)
    {
        if (!pbufferSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        TRef<ISoundBufferSource> pbufferLoopingSource = new LoopingSoundBufferSource(pbufferSource);
        return m_pstBase->CreateSound(psoundNew, pbufferLoopingSource, psource);
    };
};

// creates a looping sound from the given template
HRESULT CreateLoopingSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource)
{
    TRef<LoopingSoundTemplate> ptemplate = new LoopingSoundTemplate();
    HRESULT hr = ptemplate->Init(pstSource);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};



class ASRSoundTemplate : public ISoundTemplate
{
    TRef<ISoundTemplate> m_pstBase;
    float m_fLoopStart, m_fLoopLength;

    // a wrapper for a buffer source which forces all sounds to be ASR sounds
    class ASRSoundBufferSource : public SoundBufferSourceWrapper
    {
        float m_fLoopStart, m_fLoopLength;

    public:
        ASRSoundBufferSource(ISoundBufferSource* pBase, float fLoopStart, 
                float fLoopLength) :
            SoundBufferSourceWrapper(pBase), 
            m_fLoopStart(fLoopStart), 
            m_fLoopLength(fLoopLength) 
            {};

        // Creates a static sound buffer of the given wave file.  
        // The sound will loop until stopped.
        HRESULT CreateStaticBuffer(TRef<ISoundInstance>& psoundNew, 
            ISoundPCMData* pcmdata, bool bLooping, ISoundPositionSource* psource)
        {
            unsigned uBytesPerSec = pcmdata->GetBytesPerSec();
            float fLength = pcmdata->GetSize()/float(uBytesPerSec);

            if (fLength < m_fLoopStart + m_fLoopLength)
            {
                ZAssert(false);
                return E_INVALIDARG;
            }

            unsigned uLoopOffset = unsigned(m_fLoopStart * uBytesPerSec);
            unsigned uLoopLength = unsigned(m_fLoopLength * uBytesPerSec);
            
            // fix the loop offset and length to be on sample boundaries
            unsigned uBytesPerSample = pcmdata->GetBytesPerSample();
            uLoopOffset -= uLoopOffset % uBytesPerSample;
            uLoopLength -= uLoopLength % uBytesPerSample;

            return m_pBase->CreateASRBuffer(psoundNew, pcmdata, 
                uLoopOffset, uLoopLength, psource);
        }
    };

public:
    // tries to initialize the object with the given file.
    HRESULT Init(ISoundTemplate* pstSource, float fLoopStart, float fLoopLength)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        if (fLoopStart < 0.0f || fLoopLength <= 0.0f)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_fLoopStart = fLoopStart;
        m_fLoopLength = fLoopLength; 
        m_pstBase = pstSource;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource = nullptr)
    {
        if (!pbufferSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        TRef<ISoundBufferSource> pbufferASRSource = 
            new ASRSoundBufferSource(pbufferSource, m_fLoopStart, m_fLoopLength);
        return m_pstBase->CreateSound(psoundNew, pbufferASRSource, psource);
    };
};

// creates a ASR sound from the given template
HRESULT CreateASRSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                               float fLoopStart, float fLoopLength)
{
    TRef<ASRSoundTemplate> ptemplate = new ASRSoundTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fLoopStart, fLoopLength);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};


// a generic wrapper for an ISoundInstance which just delegates everything to
// the object it wraps.
// (not very useful by itself, but useful as a base class)
class SoundPairWrapper : public ISoundInstance, public ISoundTweakable3D, public IEventSink
{
protected:
    TRef<ISoundInstance> m_pBase1;
    TRef<ISoundInstance> m_pBase2;

    TRef<ISoundTweakable> m_ptweak1;
    TRef<ISoundTweakable> m_ptweak2;

    TRef<ISoundTweakable3D> m_ptweak3D1;
    TRef<ISoundTweakable3D> m_ptweak3D2;

    TRef<EventSourceImpl> m_peventsourceFinished;
    TRef<IEventSink> m_peventDelegate;

    int m_nNumChildrenFinished;

public:
    SoundPairWrapper(ISoundInstance* pBase1, ISoundInstance* pBase2) :
        m_pBase1(pBase1), m_pBase2(pBase2), m_nNumChildrenFinished(0)
    {  
        ZAssert(pBase1 != nullptr);
        ZAssert(pBase2 != nullptr);
    };


    ~SoundPairWrapper()
    {
        // if we have an event source, we've hooked the event sources for the
        // two child sound instances.
        if (m_peventDelegate != nullptr)
        {
            // unhook ourselves.
            m_pBase1->GetFinishEventSource()->RemoveSink(m_peventDelegate);
            m_pBase2->GetFinishEventSource()->RemoveSink(m_peventDelegate);
        }
    }

    // Stops the sound.  If bForceNow is true the sound will stop ASAP, 
    // possibly popping.  If it is false some sounds may play a trail-off 
    // sound or fade away.  
    virtual HRESULT Stop(bool bForceNow = false)
    {
        HRESULT hr = m_pBase1->Stop(bForceNow);
        if (ZFailed(hr)) return hr;

        return m_pBase2->Stop(bForceNow);
    };

    // returns S_OK if the sound is currently playing, S_FALSE otherwise.
    virtual HRESULT IsPlaying()
    {
        // return S_OK if either sound is still playing
        HRESULT hr = m_pBase1->IsPlaying();
        if (ZFailed(hr) || hr == S_OK) return hr;

        return m_pBase2->IsPlaying();
    };

    // Gets an event which fires when the sound finishes playing (for any 
    // reason)
    virtual IEventSource* GetFinishEventSource()
    {
        if (m_peventsourceFinished == nullptr)
        {
            m_peventsourceFinished = new EventSourceImpl();
            m_peventDelegate = IEventSink::CreateDelegate(this);
            m_pBase1->GetFinishEventSource()->AddSink(m_peventDelegate);
            m_pBase2->GetFinishEventSource()->AddSink(m_peventDelegate);
        }

        return m_peventsourceFinished;
    };

    // Gets an interface for tweaking the sound, if supported, nullptr otherwise.
    virtual TRef<ISoundTweakable> GetISoundTweakable()
    {
        // try to get the child sounds' tweakable interfaces.
        if (!m_ptweak1 || !m_ptweak2)
        {
            m_ptweak1 = m_pBase1->GetISoundTweakable();
            m_ptweak2 = m_pBase2->GetISoundTweakable();
        }

        // this sound is only tweakable if both it's child sounds are tweakable
        if (!m_ptweak1 || !m_ptweak2)
        {
            return nullptr;
        }
        else
        {
            return this;
        }
    }
    virtual TRef<ISoundTweakable3D> GetISoundTweakable3D()
    {
        // try to get the child sounds' tweakable interfaces.
        if (!m_ptweak3D1 || !m_ptweak3D2)
        {
            m_ptweak3D1 = m_pBase1->GetISoundTweakable3D();
            m_ptweak3D2 = m_pBase2->GetISoundTweakable3D();
        }

        // this sound is only tweak3Dable if both it's child sounds are tweak3Dable
        if (!m_ptweak3D1 || !m_ptweak3D2)
        {
            return nullptr;
        }
        else
        {
            return this;
        }
    }

    // forward a child-finished event.
    virtual bool OnEvent(IEventSource* pevent)
    {
        m_nNumChildrenFinished++;

        ZAssert(m_nNumChildrenFinished <= 2);

        // fire when the last child finishes
        if (m_nNumChildrenFinished >= 2)
        {
            m_peventsourceFinished->Trigger();
        }

        return true;
    };


    //
    // ISoundTweakable
    //

    // Sets the gain, from 0 to -100 dB
    virtual HRESULT SetGain(float fGain)
    {
        HRESULT hr = m_ptweak1->SetGain(fGain);
        if (ZFailed(hr)) return hr;

        return m_ptweak2->SetGain(fGain);
    };

    // Sets the pitch shift, where 1.0 is normal, 0.5 is half of normal speed, 
    // and 2.0 is twice normal speed.  
    virtual HRESULT SetPitch(float fPitch)
    {
        HRESULT hr = m_ptweak1->SetPitch(fPitch);
        if (ZFailed(hr)) return hr;

        return m_ptweak2->SetPitch(fPitch);
    }

    // sets the priority - used as a addition to volume when choosing which 
    // sounds are most important to play.
    virtual HRESULT SetPriority(float fPriority)
    {
        HRESULT hr = m_ptweak1->SetPriority(fPriority);
        if (ZFailed(hr)) return hr;

        return m_ptweak2->SetPriority(fPriority);
    }


    //
    // ISoundTweakable3D
    //

    // toggles 3D Positioning on and off for the given sound.
    virtual HRESULT Set3D(bool b3D)
    {
        HRESULT hr = m_ptweak3D1->Set3D(b3D);
        if (ZFailed(hr)) return hr;

        return m_ptweak3D2->Set3D(b3D);
    }

    // Sets the distance at which the sound will be at max volume.  This
    // effects how quickly the sound drops off with distance.  
    virtual HRESULT SetMinimumDistance(float fMinimumDistance)
    {
        HRESULT hr = m_ptweak3D1->SetMinimumDistance(fMinimumDistance);
        if (ZFailed(hr)) return hr;

        return m_ptweak3D2->SetMinimumDistance(fMinimumDistance);
    }

    // Sets a sound cone of size fInnerAngle (in degrees) where the volume is at 
    // normal levels, outside of which it fades down by fOutsideGain 
    // (range of 0 to -100 db) at fOuterAngle (degrees) and beyond.  
    virtual HRESULT SetCone(float fInnerAngle, float fOuterAngle, float fOutsideGain)
    {
        HRESULT hr = m_ptweak3D1->SetCone(fInnerAngle, fOuterAngle, fOutsideGain);
        if (ZFailed(hr)) return hr;

        return m_ptweak3D2->SetCone(fInnerAngle, fOuterAngle, fOutsideGain);
    }
};


// a template for merging two templates into one
class PairedSoundTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase1;
    TRef<ISoundTemplate> m_pstBase2;

public:
    // tries to initialize the object with the given file.
    HRESULT Init(ISoundTemplate* pstSource1, ISoundTemplate* pstSource2)
    {
        if (!pstSource1 || !pstSource2)
        {
            ZAssert(false);
            return E_POINTER;
        }

        m_pstBase1 = pstSource1;
        m_pstBase2 = pstSource2;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        TRef<ISoundInstance> psound1, psound2;
        HRESULT hr;

        hr = m_pstBase1->CreateSound(psound1, pbufferSource, psource);
        if (ZFailed(hr)) return hr;

        hr = m_pstBase2->CreateSound(psound2, pbufferSource, psource);
        if (ZFailed(hr)) return hr;

        if (!psound1 || !psound2)
        {
            ZAssert(false);
            return E_FAIL;
        }

        psoundNew = new SoundPairWrapper(psound1, psound2);

        return S_OK;
    }
};



// creates a sound template which plays two sounds at once
HRESULT CreatePairedSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource1, 
                                  ISoundTemplate* pstSource2)
{
    TRef<PairedSoundTemplate> ptemplate = new PairedSoundTemplate();
    HRESULT hr = ptemplate->Init(pstSource1, pstSource2);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};



// a class to generate data for repeat-fire sounds
class RepeatFirePCMData : public ISoundPCMData
{
    // the base data we are working from
    TRef<ISoundPCMData> m_pdataBase;

    // a cached version of the sound playing and repeating once.
    void* m_pvData;

    // the length of the sound, in bytes
    DWORD m_dwSize;

    // the repeat delay, in bytes of sound
    DWORD m_dwRepeatLength;

    // constructor
    RepeatFirePCMData(ISoundPCMData* pdata, DWORD dwRepeatLength)
    {
        ZAssert(pdata);
        ZAssert(dwRepeatLength > 0);

        m_pvData = nullptr;
        m_pdataBase = pdata;
        m_dwRepeatLength = dwRepeatLength;
        m_dwSize = m_pdataBase->GetSize() + m_dwRepeatLength * 2;
    }


    //
    // caching support
    //

    // a cache of previously generated repeat-fire sounds
    typedef std::pair<ISoundPCMData*, DWORD> RepeatFireCacheKey;
    typedef std::map<RepeatFireCacheKey, TRef<RepeatFirePCMData> > 
        RepeatFireDataCache;
    static RepeatFireDataCache s_cache;
    
    // A ref count on the cache.
    // for proper destruction order we need to free the cache when the last
    // repeat fire template is released.  
    static int s_nCacheRefCount;

public:

    // adds a reference to the internal cache of repeat fire data
    static void AddCacheRef()
    {
        ++s_nCacheRefCount;
    }

    // releases a reference to the internal cache of repeat fire data
    static void ReleaseCacheRef()
    {
        --s_nCacheRefCount;
        ZAssert(s_nCacheRefCount >= 0);

        if (s_nCacheRefCount == 0 && !s_cache.empty())
        {
            s_cache.clear();
        }
    }

    // creates a repeat fire data source for the given sample and repeat delay
    // (in seconds).
    static RepeatFirePCMData* Create(ISoundPCMData* pdata, float fRepeatDelay)
    {
        ZAssert(s_nCacheRefCount > 0);

        if (!pdata)
            return nullptr;

        DWORD dwRepeatLength = (DWORD)(fRepeatDelay * (pdata->GetBytesPerSec())) 
            & ~(pdata->GetBytesPerSample() - 1);

        // if the repeat length is over 16 Meg, don't even try it.  
        if (dwRepeatLength > 0x1000000 || dwRepeatLength == 0)
            return nullptr;

        // otherwise, try to find it in the cache.
        RepeatFireDataCache::iterator iter;

        RepeatFireCacheKey key(pdata, dwRepeatLength);

        iter = s_cache.find(key);

        // if there was not a data source like this in the cache, create one
        // and add it.
        if (iter == s_cache.end())
        {
            iter = (s_cache.insert(std::pair<RepeatFireCacheKey, TRef<RepeatFirePCMData> >(
                key, new RepeatFirePCMData(pdata, dwRepeatLength)
                ))).first;
        }

        return (*iter).second;
    }


    // frees the sound
    ~RepeatFirePCMData()
    {
        // if we generated the data, delete it.
        if (m_pvData)
            delete [] (char*)m_pvData;
    }

    // gets the loop offset and loop length (which are equal) for this sound.
    DWORD GetLoopStart()
    {
        return m_dwRepeatLength*2;
    }


    // gets the loop offset and loop length (which are equal) for this sound.
    DWORD GetLoopLength()
    {
        return m_dwRepeatLength;
    }

    //
    // ISoundPCMData
    //

    // Gets the number of channels in the data
    virtual unsigned GetNumberOfChannels()
    {
        return m_pdataBase->GetNumberOfChannels();
    };

    // Gets the number of bits per sample
    virtual unsigned GetBitsPerSample()
    {
        return m_pdataBase->GetBitsPerSample();
    };

    // Gets the default frequency (in samples per second) of the data
    virtual unsigned GetSampleRate()
    {
        return m_pdataBase->GetSampleRate();
    }

    // Gets the size of the data
    virtual unsigned GetSize()
    {
        return m_dwSize;
    }

    // Copies the specified portion of the data
    virtual void GetData(void* dest, unsigned nOffset, unsigned nLength)
    {
        // if we have not created an ASR version of the data, do so
        if (!m_pvData)
        {
            m_pvData = new char[m_dwSize];
            
            // copy the old data to generate the second sound.  
            m_pdataBase->GetData((char*)(m_pvData) + m_dwRepeatLength * 2, 0, 
                m_dwSize - m_dwRepeatLength * 2);

            // copy the chunk of the first sound before the second sound starts
            memcpy(m_pvData, (char*)(m_pvData) + m_dwRepeatLength * 2, m_dwRepeatLength);

            // for the part where they overlap, add the two together by adding 
            // each sample to the sample m_dwRepeatLength bytes after it.  

            // if the samples are 8 bits
            if (m_pdataBase->GetBitsPerSample() == 8)
            {
                // add them as unsigned bytes with a 0x7F offset
                unsigned char* pcSample2Start = 
                    (unsigned char*)(m_pvData) + m_dwRepeatLength;
                unsigned char* pcSample3Start = 
                    (unsigned char*)(m_pvData) + m_dwRepeatLength * 2;
                unsigned char* pcSample1End = 
                    (unsigned char*)(m_pvData) + (m_dwSize - m_dwRepeatLength * 2);
                unsigned char* pcSample2End = 
                    (unsigned char*)(m_pvData) + (m_dwSize - m_dwRepeatLength);

                unsigned char* pcAdd;
                for (pcAdd = pcSample2Start; pcAdd < pcSample3Start; ++pcAdd)
                {
                    // add them with saturation
                    *pcAdd = (unsigned char)(std::max(0, std::min(0xFF,
                        (int)pcAdd[m_dwRepeatLength] + (int)pcAdd[m_dwRepeatLength*2] - 0x7F
                        )));
                }
                for (; pcAdd < pcSample1End; ++pcAdd)
                {
                    // add them with saturation
                    *pcAdd = (unsigned char)(std::max(0, std::min(0xFF,
                        (int)pcAdd[0] + (int)pcAdd[m_dwRepeatLength] 
                        + (int)pcAdd[m_dwRepeatLength*2] - 0x7F*2
                        )));
                }
                for (; pcAdd < pcSample2End; ++pcAdd)
                {
                    // add them with saturation
                    *pcAdd = (unsigned char)(std::max(0, std::min(0xFF,
                        (int)pcAdd[0] + (int)pcAdd[m_dwRepeatLength] - 0x7F
                        )));
                }
            }
            else
            {
                ZAssert(m_pdataBase->GetBitsPerSample() == 16);
                ZAssert(m_dwRepeatLength % 2 == 0);
                ZAssert(m_dwSize % 2 == 0);

                // add them as signed 16 bit shorts
                short* pwSample2Start = 
                    (short*)(m_pvData) + m_dwRepeatLength/2;
                short* pwSample3Start = 
                    (short*)(m_pvData) + m_dwRepeatLength;
                short* pwSample1End = 
                    (short*)(m_pvData) + (m_dwSize - m_dwRepeatLength*2)/2;
                short* pwSample2End = 
                    (short*)(m_pvData) + (m_dwSize - m_dwRepeatLength)/2;

                short* pwAdd;
                for (pwAdd = pwSample2Start; pwAdd < pwSample2Start; ++pwAdd)
                {
                    // add them with saturation
                    *pwAdd = (short)(std::max(-0x8000, std::min(0x7FFF,
                        (int)pwAdd[m_dwRepeatLength/2] + (int)pwAdd[m_dwRepeatLength]
                        )));
                }
                for (; pwAdd < pwSample1End; ++pwAdd)
                {
                    // add them with saturation
                    *pwAdd = (short)(std::max(-0x8000, std::min(0x7FFF,
                        (int)pwAdd[0] + (int)pwAdd[m_dwRepeatLength/2]
                         + (int)pwAdd[m_dwRepeatLength]
                        )));
                }
                for (; pwAdd < pwSample2End; ++pwAdd)
                {
                    // add them with saturation
                    *pwAdd = (short)(std::max(-0x8000, std::min(0x7FFF,
                        (int)pwAdd[0] + (int)pwAdd[m_dwRepeatLength/2]
                        )));
                }
            }
        }

        ZAssert(nOffset + nLength <= m_dwSize);
        memcpy(dest, ((BYTE*)m_pvData) + nOffset, nLength);
    };
};

RepeatFirePCMData::RepeatFireDataCache RepeatFirePCMData::s_cache;
int RepeatFirePCMData::s_nCacheRefCount = 0;

// a class for a repeat-fire weapon
class RepeatFireTemplate : public ISoundTemplate
{
    TRef<ISoundTemplate> m_pstBase;
    float m_fRepeatRate;

    // a wrapper for a buffer source which forces all sounds to be ASR sounds
    class RepeatFireBufferSource : public SoundBufferSourceWrapper
    {
        float m_fRepeatRate;

        TRef<RepeatFirePCMData> m_ppcmdataRepeatLast;
        TRef<ISoundPCMData> m_ppcmdataBaseLast;
    public:

        RepeatFireBufferSource(ISoundBufferSource* pBase, float fRepeatRate) :
            SoundBufferSourceWrapper(pBase), 
            m_fRepeatRate(fRepeatRate) 
            {};

        // Creates a static sound buffer of the given wave file.  
        // The sound will loop until stopped.
        HRESULT CreateStaticBuffer(TRef<ISoundInstance>& psoundNew, 
            ISoundPCMData* pcmdata, bool bLooping, ISoundPositionSource* psource)
        {
            unsigned uBytesPerSec = pcmdata->GetBytesPerSec();

            // create the repeating version of this sound

            // optimization: most of the time we will be playing the same sound
            // that we played last time, so there's no need to do the full lookup.
            TRef<RepeatFirePCMData> ppcmdataRepeat;
            if (pcmdata == m_ppcmdataBaseLast)
            {
                ppcmdataRepeat = m_ppcmdataRepeatLast;
            }
            else
            {
                m_ppcmdataBaseLast = pcmdata;
                ppcmdataRepeat = m_ppcmdataRepeatLast = 
                    RepeatFirePCMData::Create(pcmdata, m_fRepeatRate);
            }

            if (!ppcmdataRepeat)
            {
                ZAssert(false);
                return E_FAIL;
            }

            return m_pBase->CreateASRBuffer(psoundNew, ppcmdataRepeat, 
                ppcmdataRepeat->GetLoopStart(), 
                ppcmdataRepeat->GetLoopLength(), 
                psource
                );
        }
    };

public:

    // make sure the repeat fire cache is valid while we are alive
    RepeatFireTemplate()
    {
        RepeatFirePCMData::AddCacheRef();
    };
    ~RepeatFireTemplate()
    {
        RepeatFirePCMData::ReleaseCacheRef();
    };


    // tries to initialize the object with the given file.
    HRESULT Init(ISoundTemplate* pstSource, float fRepeatRate)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        if (fRepeatRate <= 0.0f)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_fRepeatRate = fRepeatRate;
        m_pstBase = pstSource;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource = nullptr)
    {
        if (!pbufferSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        TRef<ISoundBufferSource> pbufferRepeatFireSource = 
            new RepeatFireBufferSource(pbufferSource, m_fRepeatRate);
        return m_pstBase->CreateSound(psoundNew, pbufferRepeatFireSource, psource);
    };
};


// Creates an ASR sound from the given template for a weapon that fires every n
// seconds.  This assumes the sound for a single shot falls off dramaticly after
// 2n seconds, and plays some tricks based on this like only playing the two 
// most recent sounds.
HRESULT CreateRepeatingFireSoundTemplate(TRef<ISoundTemplate>& pstDest, 
                                         ISoundTemplate* pstSource, 
                                         float fFireDelay)
{
    TRef<RepeatFireTemplate> ptemplate = new RepeatFireTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fFireDelay);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
};

// provides a template for picking a random sound to play
class RandomSoundTemplate : public IRandomSoundTemplate
{
private:
    // a map from probability lower bound to template
    typedef std::map<float, ZAdapt<TRef<ISoundTemplate> > > TemplateMap;
    TemplateMap m_templates;

    // the current sum of all weights
    float m_fTotalWeight;

    // the last sound template we used (used to avoid repeats)
    ISoundTemplate* m_pstLast;

public:

    RandomSoundTemplate() :
            m_fTotalWeight(0),
            m_pstLast(nullptr)
    {
    }

    // adds the given template as a possible sound to play, with the given 
    // weight.  The weight can be any arbitrary number, but a sound with weight
    // f will play half as often as a sound with weight 2*f.  
    HRESULT AddSoundTemplate(ISoundTemplate* pstSource, float fWeight)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        if (fWeight <= 0.0f)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_templates.insert(std::pair<float, ZAdapt<TRef<ISoundTemplate> > >(
            m_fTotalWeight, TRef<ISoundTemplate>(pstSource)));
        m_fTotalWeight += fWeight;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource = nullptr)
    {
        ISoundTemplate* pstNew;

        ZAssert(m_templates.size() > 0);

        do 
        {
            float fChoice = (rand()+1) * m_fTotalWeight / (RAND_MAX + 1);
            pstNew = (TRef<ISoundTemplate>&)(*(--(m_templates.lower_bound(fChoice)))).second;
        } 
        while (m_templates.size() > 2 && pstNew == m_pstLast);

        pstNew->CreateSound(psoundNew, pbufferSource, psource);
        m_pstLast = pstNew;

        return S_OK;
    };
};

// creates a sound template which randomly chooses from the sound templates
// it contains each time a sound is played.  
HRESULT CreateRandomSoundTemplate(TRef<IRandomSoundTemplate>& pstDest)
{
    pstDest = new RandomSoundTemplate();
    return S_OK;
};



// a template for a sound which plays intermittently 
class IntermittentSoundTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;
    float m_fPeriod;
    bool m_bMultipleSounds;

public:
    // a sound source which contains intermittently playing sounds
    class IntermittentInstance : public StubbedTweakableSoundInstance
    {
        float m_fPeriodMS;
        TRef<ISoundTemplate> m_ptemplate;
        bool m_bMultipleSounds;
        TRef<ISoundBufferSource> m_pbufferSource;
        
        typedef std::list<ZAdapt<TRef<ISoundInstance> > > SoundInstanceList;
        SoundInstanceList m_listPlayingSounds;

    public:
        IntermittentInstance(float fPeriod, bool bMultipleSounds, 
                ISoundTemplate* ptemplate, ISoundBufferSource* pbufferSource,
                ISoundPositionSource* pposSource) :
            StubbedTweakableSoundInstance(pposSource),
            m_fPeriodMS(fPeriod*1000), 
            m_bMultipleSounds(bMultipleSounds),
            m_ptemplate(ptemplate),
            m_pbufferSource(pbufferSource)
        {
            // grab the update event for the sound buffer source
            m_pbufferSource->GetUpdateEventSource()->AddSink(this);
        };

        // fires the m_peventsourceStopped event if bHasStopped is true
        virtual void Update(DWORD dwElapsedTime, bool bHasStopped)
        {
            StubbedTweakableSoundInstance::Update(dwElapsedTime, bHasStopped);

            if (!bHasStopped)
            {
                // clean up the playing sounds list 
                // (polling is inefficent - oh well)
                SoundInstanceList::iterator iter;
                for (iter = m_listPlayingSounds.begin(); 
                    iter != m_listPlayingSounds.end();)
                {
                    if ((*iter)->IsPlaying() == S_OK)
                    {
                        ++iter;
                    }
                    else
                    {
                        iter = m_listPlayingSounds.erase(iter);
                    }
                }

                // if we can play a sound this frame
                if (m_bMultipleSounds || m_listPlayingSounds.empty())
                {
                    // calculate the probability of playing a sound in this 
                    // frame.
					// KGJV: typecasted pow args to keep compiler happy
                    float fProbability = 1 - (float)pow((float)2, -((float)dwElapsedTime/m_fPeriodMS));

                    if (rand() < fProbability * RAND_MAX)
                    {
                        // start a new sound
                        TRef<ISoundInstance> pNewSound;
                        if (ZSucceeded(m_ptemplate->CreateSound(
                            pNewSound, m_pbufferSource, m_pposSource)))
                        {
                            m_listPlayingSounds.push_back(pNewSound);
                            UpdateSound(pNewSound);                            
                        }
                    }
                }
            }
        }

        // Stops the sound.  If bForceNow is true the sound will stop ASAP, 
        // possibly popping.  If it is false some sounds may play a trail-off 
        // sound or fade away.  
        HRESULT Stop(bool bForceNow)
        {
            ZSucceeded(StubbedTweakableSoundInstance::Stop(bForceNow));

            // pass on the stop event to all playing sounds
            SoundInstanceList::iterator iter;
            for (iter = m_listPlayingSounds.begin(); 
                iter != m_listPlayingSounds.end(); ++iter)
            {
                ZSucceeded((*iter)->Stop(bForceNow));
            }
            
            return S_OK;
        };

        //
        // ISoundTweakable
        //

        // Sets the gain, from 0 to -100 dB
        virtual HRESULT SetGain(float fGain)
        {
            ZSucceeded(StubbedTweakableSoundInstance::SetGain(fGain));

            // pass on the stop event to all playing sounds
            SoundInstanceList::iterator iter;
            for (iter = m_listPlayingSounds.begin(); 
                iter != m_listPlayingSounds.end(); ++iter)
            {
                ZSucceeded((*iter)->GetISoundTweakable()->SetGain(fGain));
            }
            
            return S_OK;
        };

        // Sets the pitch shift, where 1.0 is normal, 0.5 is half of normal speed, 
        // and 2.0 is twice normal speed.  
        virtual HRESULT SetPitch(float fPitch)
        {
            ZSucceeded(StubbedTweakableSoundInstance::SetPitch(fPitch));

            // pass on the stop event to all playing sounds
            SoundInstanceList::iterator iter;
            for (iter = m_listPlayingSounds.begin(); 
                iter != m_listPlayingSounds.end(); ++iter)
            {
                ZSucceeded((*iter)->GetISoundTweakable()->SetPitch(fPitch));
            }
            
            return S_OK;
        };

        // sets the priority - used as a addition to volume when choosing which 
        // sounds are most important to play.
        virtual HRESULT SetPriority(float fPriority)
        {
            ZSucceeded(StubbedTweakableSoundInstance::SetPriority(fPriority));

            // pass on the stop event to all playing sounds
            SoundInstanceList::iterator iter;
            for (iter = m_listPlayingSounds.begin(); 
                iter != m_listPlayingSounds.end(); ++iter)
            {
                ZSucceeded((*iter)->GetISoundTweakable()->SetPriority(fPriority));
            }
            
            return S_OK;
        };


        //
        // ISoundTweakable3D
        // 

        // toggles 3D Positioning on and off for the given sound.
        virtual HRESULT Set3D(bool b3D)
        {
            ZSucceeded(StubbedTweakableSoundInstance::Set3D(b3D));

            // pass on the stop event to all playing sounds
            SoundInstanceList::iterator iter;
            for (iter = m_listPlayingSounds.begin(); 
                iter != m_listPlayingSounds.end(); ++iter)
            {
                TRef<ISoundTweakable3D> ptweak3d = (*iter)->GetISoundTweakable3D(); 
                
                if (ptweak3d) 
                    ZSucceeded(ptweak3d->Set3D(b3D));
            }
            
            return S_OK;
        };

        // Sets the distance at which the sound will be at max volume.  This
        // effects how quickly the sound drops off with distance.  
        virtual HRESULT SetMinimumDistance(float fMinimumDistance)
        {
            ZSucceeded(StubbedTweakableSoundInstance::SetMinimumDistance(fMinimumDistance));

            // pass on the stop event to all playing sounds
            SoundInstanceList::iterator iter;
            for (iter = m_listPlayingSounds.begin(); 
                iter != m_listPlayingSounds.end(); ++iter)
            {
                TRef<ISoundTweakable3D> ptweak3d = (*iter)->GetISoundTweakable3D(); 
                
                if (ptweak3d) 
                    ZSucceeded(ptweak3d->SetMinimumDistance(fMinimumDistance));
            }
            
            return S_OK;
        };

        // Sets a sound cone of size fInnerAngle (in degrees) where the volume is at 
        // normal levels, outside of which it fades down by fOutsideGain 
        // (range of 0 to -100 db) at fOuterAngle (degrees) and beyond.  
        virtual HRESULT SetCone(float fInnerAngle, float fOuterAngle, float fOutsideGain)
        {
            ZSucceeded(StubbedTweakableSoundInstance::SetCone(fInnerAngle, fOuterAngle, fOutsideGain));

            // pass on the stop event to all playing sounds
            SoundInstanceList::iterator iter;
            for (iter = m_listPlayingSounds.begin(); 
                iter != m_listPlayingSounds.end(); ++iter)
            {
                TRef<ISoundTweakable3D> ptweak3d = (*iter)->GetISoundTweakable3D(); 
                
                if (ptweak3d) 
                    ZSucceeded(ptweak3d->SetCone(fInnerAngle, fOuterAngle, fOutsideGain));
            }
            
            return S_OK;
        };
    };

public:
    HRESULT Init(ISoundTemplate* pstSource, float fPeriod, bool bMultipleSounds)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        m_pstBase = pstSource;
        m_fPeriod = fPeriod;
        m_bMultipleSounds = bMultipleSounds;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        // we need to have a tweakable sound for this to work.
        if (!pbufferSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        psoundNew = new IntermittentInstance(m_fPeriod, m_bMultipleSounds, 
            m_pstBase, pbufferSource, psource);

        return S_OK;
    }
};


// creates a sound template for a sound which plays intermittently, and has a 50%
// chance of playing over a period of fPeriod seconds.  If bMultipleSounds is 
// true, this allows multiple sounds to play at the same time.  
HRESULT CreateIntermittentSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                        float fPeriod, bool bMultipleSounds)
{
    TRef<IntermittentSoundTemplate> ptemplate = new IntermittentSoundTemplate();
    HRESULT hr = ptemplate->Init(pstSource, fPeriod, bMultipleSounds);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
}


// a sound instance with a delayed start
class DelayedSoundInstance : public ISoundInstance, public IEventSink
{
protected:
    TRef<ISoundInstance> m_pBase;
    TRef<ISoundTemplate> m_pstSource;
    TRef<ISoundBufferSource> m_pbufferSource;
    TRef<ISoundPositionSource> m_psource;
    TRef<EventSourceImpl> m_peventsource;
    TRef<IEventSink> m_psinkDelegate;

public:
    DelayedSoundInstance(ISoundTemplate* pstSource,
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource) :
            m_pstSource(pstSource), 
            m_pbufferSource(pbufferSource), 
            m_psource(psource) 
    { 
        ZAssert(m_pstSource != nullptr);
        ZAssert(m_pbufferSource != nullptr);
    }

    ~DelayedSoundInstance()
    {
        if (m_psinkDelegate)
        {
            m_pBase->GetFinishEventSource()->RemoveSink(m_psinkDelegate);
            m_psinkDelegate = nullptr;
        }
    }

    // Starts the sound.
    virtual HRESULT Start()
    {
        HRESULT hr = m_pstSource->CreateSound(m_pBase, m_pbufferSource, m_psource);

        if (SUCCEEDED(hr))
        {
            m_psinkDelegate = IEventSink::CreateDelegate(this);
            m_pBase->GetFinishEventSource()->AddSink(m_psinkDelegate);

            m_pstSource = nullptr;
            m_pbufferSource = nullptr;
            m_psource = nullptr;
        }

        return hr;
    }

    // Stops the sound.  If bForceNow is true the sound will stop ASAP, 
    // possibly popping.  If it is false some sounds may play a trail-off 
    // sound or fade away.  
    virtual HRESULT Stop(bool bForceNow = false)
    {
        // if the sound has not been started yet, force a start so that we
        // will get the finish event.
        if (m_pBase)
            return m_pBase->Stop();
        else if (m_peventsource)
        {
            m_peventsource->Trigger();
            m_pstSource = nullptr;
        }

        return S_OK;
    }

    // a sound finished
    virtual bool OnEvent(IEventSource* pevent)
    {
        m_pBase = nullptr;
        m_psinkDelegate = nullptr;
        if (m_peventsource)
            m_peventsource->Trigger();

        return false;
    }

    // returns S_OK if the sound is currently playing, S_FALSE otherwise.
    virtual HRESULT IsPlaying()
    {
        if (m_pBase)
            return m_pBase->IsPlaying();
        else
            return m_pstSource ? S_OK : S_FALSE;
    }

    // Gets an event which fires when the sound finishes playing (for any 
    // reason)
    virtual IEventSource* GetFinishEventSource()
    {
        if (m_peventsource)
            return m_peventsource;
        else if (m_pBase)
            return m_pBase->GetFinishEventSource();
        else
            return m_peventsource = new EventSourceImpl();
    }

    // Gets an interface for tweaking the sound, if supported, nullptr otherwise.
    virtual TRef<ISoundTweakable> GetISoundTweakable()
    {
        assert(false); // not fully implemented

        if (m_pBase)
            return m_pBase->GetISoundTweakable();
        else
            return nullptr;
    }

    virtual TRef<ISoundTweakable3D> GetISoundTweakable3D()
    {
        assert(false); // not fully implemented

        if (m_pBase)
            return m_pBase->GetISoundTweakable3D();
        else
            return nullptr;
    }
};

// a helper class for serialized sounds which handles the serialization part
class SoundMutex : public ISoundMutex, public IEventSink
{
private:
    // the set of sounds pending on this mutex and the coresponding timeout 
    // and (timeout - priority) values for that sound.
    struct TemplateData
    {
        TRef<DelayedSoundInstance> pinstance;
        Time timeExpiration;
        Time timeUrgency;

        TemplateData()
        {}

        TemplateData(TRef<DelayedSoundInstance> pinstance_, 
            Time timeExpiration_, Time timeUrgency_)
            :   pinstance(pinstance_), timeExpiration(timeExpiration_), 
                timeUrgency(timeUrgency_)
        {}
    };

    typedef std::multimap<ZAdapt<TRef<ISoundTemplate> >, TemplateData> TemplateMap;
    TemplateMap m_templatesPending;

    // the currently playing template (only used for avoiding duplicate sounds)
    TRef<ISoundTemplate> m_pstCurrent;
    
    // the currently playing sound instance
    TRef<ISoundInstance> m_psoundCurrent;
    TRef<IEventSource> m_peventSoundFinished;

    // start the given sound template and do the required book keeping.  
    HRESULT StartSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource,
        ISoundTemplate* pstSource) 
    {
        ZAssert(!m_pstCurrent);
        ZAssert(!m_peventSoundFinished);
        ZAssert(!m_psoundCurrent);

        HRESULT hr = pstSource->CreateSound(psoundNew, pbufferSource, psource);

        if (SUCCEEDED(hr))
        {
            if (!psoundNew)
            {
                ZAssert(false);
                return E_FAIL;
            }

            m_pstCurrent = pstSource;
            m_psoundCurrent = psoundNew;
            m_peventSoundFinished = psoundNew->GetFinishEventSource();
            m_peventSoundFinished->AddSink(this);
        }

        return hr;
    }

    void QueueNextSound()
    {
        ZAssert(!m_pstCurrent);
        ZAssert(!m_peventSoundFinished);
        ZAssert(!m_psoundCurrent);

        Time now = Time::Now();

        TemplateMap::iterator iterTemplate = m_templatesPending.begin();
        TemplateMap::iterator iterMostUrgentSound = m_templatesPending.end();
        float fMaxUrgency = -1e10f; // just a big negative value
        
        // find the sound closest to expiring which hasn't expired yet, 
        // adjusting for priority.
        while (iterTemplate != m_templatesPending.end())
        {
            if ((*iterTemplate).second.timeExpiration - now < 0.0f)
                iterTemplate = m_templatesPending.erase(iterTemplate);
            else
            {
                float fUrgency = now - (*iterTemplate).second.timeUrgency;
                
                if (fUrgency > fMaxUrgency)
                {
                    fMaxUrgency = fUrgency;
                    iterMostUrgentSound = iterTemplate;
                }

                ++iterTemplate;
            }
        }

        if (iterMostUrgentSound != m_templatesPending.end())
        {
            HRESULT hr = (*iterMostUrgentSound).second.pinstance->Start();

            if (SUCCEEDED(hr))
            {
                m_pstCurrent = (*iterMostUrgentSound).first;
                m_peventSoundFinished = 
                    (*iterMostUrgentSound).second.pinstance->GetFinishEventSource();
                m_peventSoundFinished->AddSink(this);
                m_psoundCurrent = (*iterMostUrgentSound).second.pinstance;
            }

            m_templatesPending.erase(iterMostUrgentSound);

            if (FAILED(hr))
            {
                // try again with the next sound
                QueueNextSound();
            }
        }
    }

public:

    HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource,
        ISoundTemplate* pstSource, float fTimeout, float fPriority, bool bSingleInstance)
    {
        // if we are not currently playing a sound with this mutex...
        if (!m_pstCurrent)
        {
            // just play the sound.
            return StartSound(psoundNew, pbufferSource, psource, pstSource);
        }
        else
        {
            if (bSingleInstance)
            {
                // if we are already playing the sound, there's nothing more 
                // that we need to do.  
                if (m_pstCurrent == pstSource)
                {
                    psoundNew = m_psoundCurrent;
                    return S_OK;
                }

                // look for any existing instances of the sound
                TemplateMap::iterator iterOld = m_templatesPending.find(TRef<ISoundTemplate>(pstSource));

                if (iterOld != m_templatesPending.end())
                {
                    // we found an existing instance of the sound.  Update it's timeout
                    // (but leave it's priority at the same elevated level)
                    (*iterOld).second.timeExpiration = Time::Now() + fTimeout;
                    psoundNew = (*iterOld).second.pinstance;

                    return S_OK;
                }
            }

            // add the sound to the sound template list
            Time timeExpiration = Time::Now() + fTimeout;
            TRef<DelayedSoundInstance> pinstance = new DelayedSoundInstance(pstSource, pbufferSource, psource);
            m_templatesPending.insert(TemplateMap::value_type(TRef<ISoundTemplate>(pstSource), 
                TemplateData(pinstance, timeExpiration, timeExpiration - fPriority)));
            psoundNew = pinstance;

            return S_OK;
        }
    }

    // a sound finished
    virtual bool OnEvent(IEventSource* pevent)
    {
        m_pstCurrent = nullptr;
        m_psoundCurrent = nullptr;
        m_peventSoundFinished = nullptr;
        QueueNextSound();
        return false;
    }

    // erases any pending sounds without playing them
    virtual HRESULT Reset()
    {
        m_pstCurrent = nullptr;
        m_psoundCurrent = nullptr;
        if (m_peventSoundFinished)
            m_peventSoundFinished->RemoveSink(this);
        m_peventSoundFinished = nullptr;
        m_templatesPending.clear();

        return S_OK;
    }
};

// Creates a mutex used for the serial sound template below.  Each 
// mutex represents a single syncronization point - sounds with the same mutex
// won't overlap, but sounds with different mutexes won't be prevented from 
// overlapping.
HRESULT CreateSoundMutex(TRef<ISoundMutex>& pmutex)
{
    pmutex = new SoundMutex();
    return S_OK;
}


// a template turning another template into a serialized sound
class SerializedSoundTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;

    TRef<SoundMutex> m_pmutex;
    float m_fTimeout;
    float m_fPriority;
    bool m_bSingleInstance;

public:
    // tries to initialize the object with the given file.
    HRESULT Init(ISoundTemplate* pstSource, ISoundMutex* pmutex, 
            float fTimeout, float fPriority, bool bSingleInstance)
    {
        if (!pstSource)
        {
            ZAssert(false);
            return E_POINTER;
        }
        if (fTimeout < 0)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }
        
        CastTo(m_pmutex, pmutex);

        if (!m_pmutex)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_pstBase = pstSource;
        m_fTimeout = fTimeout;
        m_fPriority = fPriority;
        m_bSingleInstance = bSingleInstance;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource)
    {
        return m_pmutex->CreateSound(psoundNew, pbufferSource, psource,
            m_pstBase, m_fTimeout, m_fPriority, m_bSingleInstance);
    }
};


// Creates a sound template for a sound which will not be played at the same 
// time as any other sound created by a serial sound template with the same 
// mutex.  If bSingleInstance is true, it will only be added if there is not
// an existing copy of a sound template with that source.  fTimeout specifies 
// the amount of time a sound can be delayed before it is discarded 
HRESULT CreateSerializedSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                      ISoundMutex* pmutex, float fTimeout, float fPriority,
                                      bool bSingleInstance)
{
    TRef<SerializedSoundTemplate> ptemplate = new SerializedSoundTemplate();
    HRESULT hr = ptemplate->Init(pstSource, pmutex, fTimeout, fPriority, bSingleInstance);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
}

};
