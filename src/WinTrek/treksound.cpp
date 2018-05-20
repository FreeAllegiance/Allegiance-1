#include "pch.h"

class WaveTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    WaveTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    bool FileExists(const ZString& strFilename)
    {
        HANDLE hFile;

        hFile = CreateFile(strFilename, 0, FILE_SHARE_READ, NULL, 
            OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);

        if (INVALID_HANDLE_VALUE == hFile)
            return false;
        else
        {
            CloseHandle(hFile);
            return true;
        }
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<StringValue> pstring; CastTo(pstring, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

		// mdvalley: Look for files in this order: Soundpack WAV, Soundpack OGG, Artwork WAV, Artwork OGG
        // pkk: Removed SoundPack support, Allegiance no longer looking for files in a not existing folder
		// Load wave from artwork directory

        auto loader = m_pmodeler->GetFileLoader();
        ZString strFilename = "";

        if (loader->HasFile(pstring->GetValue() + ".wav")) {
            strFilename = loader->GetFilePath(pstring->GetValue() + ".wav");
        }
        else if (loader->HasFile(pstring->GetValue() + ".ogg")) {
            // Load ogg file, if wave file doesn't exists
            strFilename = loader->GetFilePath(pstring->GetValue() + ".ogg");
        }
		
        if (FAILED(CreateWaveFileSoundTemplate(pTemplate, strFilename)))
        {
            // Fail over to a dummy sound template
            ZSucceeded(CreateDummySoundTemplate(pTemplate));
        }

        return pTemplate;
    }
};


class ThreeDSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    ThreeDSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumMinDist; CastTo(pnumMinDist, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(Create3DSoundTemplate(pTemplate, ptemplateBase, pnumMinDist->GetValue()));

        return pTemplate;
    }
};


class ConeSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    ConeSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumInnerAngle; CastTo(pnumInnerAngle, (IObject*)stack.Pop());
        TRef<Number>         pnumOuterAngle; CastTo(pnumOuterAngle, (IObject*)stack.Pop());
        TRef<Number>         pnumOuterGain; CastTo(pnumOuterGain, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreateSoundConeTemplate(pTemplate, ptemplateBase, 
            pnumInnerAngle->GetValue(), pnumOuterAngle->GetValue(), pnumOuterGain->GetValue()));

        return pTemplate;
    }
};


class PitchSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    PitchSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumPitch;    CastTo(pnumPitch, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreatePitchSoundTemplate(pTemplate, ptemplateBase, pnumPitch->GetValue()));

        return pTemplate;
    }
};


// a template adjusting the gain of another template
class SoundGainValueTemplate : public ISoundTemplate
{
private:
    TRef<ISoundTemplate> m_pstBase;
    TRef<Number> m_pnumGain;
public:
    // a sound instance wrapper which adjusts Gain changes
    class GainInstance : public SoundInstanceWrapper, public SoundTweakableWrapper, public Value
    {
        TRef<Number> m_pnumGain;
        float m_fCurrentGain;
    public:
        GainInstance(Number* pnumGain, ISoundInstance* pinstance, 
                ISoundTweakable* ptweak) :
            SoundInstanceWrapper(pinstance), 
            SoundTweakableWrapper(ptweak), 
            m_fCurrentGain(0.0f),
            m_pnumGain(pnumGain),
            Value(pnumGain)
            {};

        // Sets the Gain shift, where 1.0 is normal, 0.5 is half of normal speed, 
        // and 2.0 is twice normal speed.  
        HRESULT SetGain(float fGain)
        {
            m_fCurrentGain = fGain;
            return SoundTweakableWrapper::SetGain(m_pnumGain->GetValue() + m_fCurrentGain);
        };

        // Gets an interface for tweaking the sound, if supported, NULL otherwise.
        TRef<ISoundTweakable> GetISoundTweakable()
        {
            return this;
        };

        void ChildChanged(Value* pvalue, Value* pvalueNew)
        {
            // the only child is the gain value, so let's adjust that.  
            ZSucceeded(SetGain(m_fCurrentGain));
        };        
    };

public:
    SoundGainValueTemplate(ISoundTemplate* pstSource, Number* pnumGain)
    {
        m_pstBase = pstSource;
        m_pnumGain = pnumGain;
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

        hr = ptweak->SetGain(m_pnumGain->GetValue());
        if (ZFailed(hr)) return hr;

        psoundNew = new GainInstance(m_pnumGain, psoundNew, ptweak);

        return S_OK;
    }
};

class GainSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    GainSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumGain;    CastTo(pnumGain, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());

        ZAssert(pnumGain && ptemplateBase);

        return new SoundGainValueTemplate(ptemplateBase, pnumGain);
    }
};


class PrioritySoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    PrioritySoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumPriority;    CastTo(pnumPriority, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreatePrioritySoundTemplate(pTemplate, ptemplateBase, pnumPriority->GetValue()));

        return pTemplate;
    }
};


class LoopingSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    LoopingSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreateLoopingSoundTemplate(pTemplate, ptemplateBase));

        return pTemplate;
    }
};


class ASRSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    ASRSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumLoopStart; CastTo(pnumLoopStart, (IObject*)stack.Pop());
        TRef<Number>         pnumLoopLength; CastTo(pnumLoopLength, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreateASRSoundTemplate(pTemplate, ptemplateBase, 
            pnumLoopStart->GetValue(), pnumLoopLength->GetValue()));

        return pTemplate;
    }
};


class PairedSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    PairedSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<ISoundTemplate> ptemplateBase1; CastTo(ptemplateBase1, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase2; CastTo(ptemplateBase2, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreatePairedSoundTemplate(pTemplate, ptemplateBase1, ptemplateBase2));

        return pTemplate;
    }
};


class RepeatingFireSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    RepeatingFireSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumRepeatRate; CastTo(pnumRepeatRate, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase; CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        if (ZFailed(CreateRepeatingFireSoundTemplate(pTemplate, ptemplateBase, pnumRepeatRate->GetValue())))
        {
            // Fail over to a dummy sound template
            ZSucceeded(CreateDummySoundTemplate(pTemplate));
        }

        return pTemplate;
    }
};


class RandomSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    RandomSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<IObjectList> plist; CastTo(plist, (IObject*)stack.Pop());
        TRef<IRandomSoundTemplate> pTemplate;

        ZSucceeded(CreateRandomSoundTemplate(pTemplate));

        plist->GetFirst();

        while (plist->GetCurrent() != NULL) {
            IObjectPair*      ppair;       CastTo(ppair,       plist->GetCurrent());
            TRef<ISoundTemplate>    ptemplateBase;  CastTo(ptemplateBase,  ppair->GetFirst()  );
            TRef<Number>            pnumWeight; CastTo(pnumWeight, ppair->GetSecond() );

            ZSucceeded(pTemplate->AddSoundTemplate(ptemplateBase, pnumWeight->GetValue()));

            plist->GetNext();
        }

        return pTemplate;
    }
};


class IntermittentSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    IntermittentSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>         pnumPeriod;        CastTo(pnumPeriod, (IObject*)stack.Pop());
        TRef<Boolean>        pboolPlayMultiple; CastTo(pboolPlayMultiple, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase;     CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreateIntermittentSoundTemplate(pTemplate, ptemplateBase, 
            pnumPeriod->GetValue(), pboolPlayMultiple->GetValue()));

        return pTemplate;
    }
};

class SerializedSoundTemplateFactory : public IFunction
{
    TRef<Modeler> m_pmodeler;

public:
    SerializedSoundTemplateFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<ISoundMutex>    pmutex;              CastTo(pmutex, (IObject*)stack.Pop());
        TRef<Number>         pnumTimeout;         CastTo(pnumTimeout, (IObject*)stack.Pop());
        TRef<Number>         pnumPriority;        CastTo(pnumPriority, (IObject*)stack.Pop());
        TRef<Boolean>        pboolSingleInstance; CastTo(pboolSingleInstance, (IObject*)stack.Pop());
        TRef<ISoundTemplate> ptemplateBase;       CastTo(ptemplateBase, (IObject*)stack.Pop());
        TRef<ISoundTemplate> pTemplate;

        ZSucceeded(CreateSerializedSoundTemplate(pTemplate, ptemplateBase, pmutex,
            pnumTimeout->GetValue(), pnumPriority->GetValue(), pboolSingleInstance->GetValue()));

        return pTemplate;
    }
};


TRef<INameSpace> CreateSoundNameSpace(
    Modeler*    pmodeler,
    INameSpace* pnsParent
) {
    TRef<INameSpace> pns = 
        pmodeler->CreateNameSpace(
            "soundEngine",
            pnsParent
        );

    pns->AddMember("ImportWave",         new WaveTemplateFactory(pmodeler));
    pns->AddMember("ThreeDSound",        new ThreeDSoundTemplateFactory(pmodeler));
    pns->AddMember("ConeSound",          new ConeSoundTemplateFactory(pmodeler));
    pns->AddMember("PitchSound",         new PitchSoundTemplateFactory(pmodeler));
    pns->AddMember("GainSound",          new GainSoundTemplateFactory(pmodeler));
    pns->AddMember("PrioritySound",      new PrioritySoundTemplateFactory(pmodeler));
    pns->AddMember("LoopingSound",       new LoopingSoundTemplateFactory(pmodeler));
    pns->AddMember("ASRSound",           new ASRSoundTemplateFactory(pmodeler));
    pns->AddMember("PairedSound",        new PairedSoundTemplateFactory(pmodeler));
    pns->AddMember("RepeatingFireSound", new RepeatingFireSoundTemplateFactory(pmodeler));
    pns->AddMember("RandomSound",        new RandomSoundTemplateFactory(pmodeler));
    pns->AddMember("IntermittentSound",  new IntermittentSoundTemplateFactory(pmodeler));
    pns->AddMember("SerializedSound",    new SerializedSoundTemplateFactory(pmodeler));

    return pns;
}
