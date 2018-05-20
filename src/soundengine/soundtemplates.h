//
// soundtemplates.h
//
// Several useful sound template implementations.
//
#include <windows.h>
#include <tref.h>
#include <zstring.h>

#include "soundbase.h"

namespace SoundEngine {

    extern bool bConvertToMono;

// creates a sound template for the given wave file
HRESULT CreateWaveFileSoundTemplate(TRef<ISoundTemplate>& pstDest, const ZString& strFilename);

// creates a dummy sound template 
// (NOTE: Not optimized - fix or use only for error handling!)
HRESULT CreateDummySoundTemplate(TRef<ISoundTemplate>& pstDest);

// creates a sound template which uses the given sound template to create 
// sounds and turns then into a 3D sounds with the given minimum distance.
HRESULT Create3DSoundTemplate(TRef<ISoundTemplate>& pstDest, 
                              ISoundTemplate* pstSource, float fMinimumDistance);

// creates a sound template which adds a sound cone to the given sound
HRESULT CreateSoundConeTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                float fInnerAngle, float fOuterAngle, float fOutsideGain);

// creates a sound template which shifts the pitch of the created sound
HRESULT CreatePitchSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                 float fPitch);

// creates a sound template which adjusts the gain of the created sound
HRESULT CreateGainSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                float fGain);

// creates a sound template which adjusts the Priority of the created sound
HRESULT CreatePrioritySoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                    float fPriority);

// creates a looping sound from the given template
HRESULT CreateLoopingSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource);

// creates an ASR sound from the given template with the given loop start and loop length (in seconds)
HRESULT CreateASRSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                               float fLoopStart, float fLoopLength);

// creates a sound template which plays two sounds at once
HRESULT CreatePairedSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource1, 
                                  ISoundTemplate* pstSource2);

// Creates an ASR sound from the given template for a weapon that fires every n
// seconds.  This assumes the sound for a single shot falls off dramaticly after
// 2n seconds, and plays some tricks based on this like only playing the two 
// most recent sounds.
HRESULT CreateRepeatingFireSoundTemplate(TRef<ISoundTemplate>& pstDest, 
                                         ISoundTemplate* pstSource, 
                                         float fFireDelay);

// provides an interface for adding new sound templates to a random sound 
// template.
class IRandomSoundTemplate : public ISoundTemplate
{
public:
    // adds the given template as a possible sound to play, with the given 
    // weight.  The weight can be any arbitrary number, but a sound with weight
    // f will play half as often as a sound with weight 2*f.  
    virtual HRESULT AddSoundTemplate(ISoundTemplate* pstSource, float fWeight) = 0;
};

// creates a sound template which randomly chooses from the sound templates
// it contains each time a sound is played.  
HRESULT CreateRandomSoundTemplate(TRef<IRandomSoundTemplate>& pstDest);

// creates a sound template for a sound which plays intermittently, and has a 50%
// chance of playing over a period of fPeriod seconds.  If bMultipleSounds is 
// true, this allows multiple sounds to play at the same time.  
HRESULT CreateIntermittentSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                        float fPeriod, bool bMultipleSounds);

// provides an interface for manipulating a sound mutex
class ISoundMutex : public ISoundObject
{
public:
    // erases any pending sounds without playing them
    virtual HRESULT Reset() = 0;
};

// Creates a mutex used for the serialized sound template below.  Each 
// mutex represents a single syncronization point - sounds with the same mutex
// won't overlap, but sounds with different mutexes won't be prevented from 
// overlapping.
HRESULT CreateSoundMutex(TRef<ISoundMutex>& pmutex);

// Creates a sound template for a sound which will not be played at the same 
// time as any other sound created by a serial sound template with the same 
// mutex.  fTimeout specifies the amount of time a sound can be delayed before 
// it is discarded.  fPriority specifies the relative priority of the sound, 
// which is added to the age of a sound when trying to find the oldest sound
// to play next.  bSingleInstance specifies whether multiple copies of the 
// given sound can queue up - if false, just the most recent one will play.
HRESULT CreateSerializedSoundTemplate(TRef<ISoundTemplate>& pstDest, ISoundTemplate* pstSource, 
                                      ISoundMutex* pmutex, float fTimeout, float fPriority,
                                      bool bSingleInstance);
};

