//
// redbooksound.cpp
//
// SoundEngine support for redbook audio.
//

#include "pch.h"

#include <mutex>

#include "soundbase.h"
#include "redbooksound.h"
#include "ds3dutil.h"

using std::list;

namespace SoundEngine {

//
// A class dedicated to controling the volume of the CD player(s) on a system
//
class CDVolume
{
public:
    enum { c_nMaxChannels = 8 };

private:
    struct MixerLineData
    {
        HMIXEROBJ hmixer;
        DWORD cChannels;
        MIXERCONTROL mixercontrol;
        MIXERCONTROLDETAILS_UNSIGNED vmixercontrolsOld[c_nMaxChannels];
    };

public:

    list<MixerLineData> m_listMixerLines;

    CDVolume()
    {
        UINT uMaxDevices = mixerGetNumDevs();  //Fix memory leak -Imago 8/2/09
        
        // find all of the CD line controls and store their ID and starting volume
        for (UINT uDeviceID = 0; uDeviceID < uMaxDevices; ++uDeviceID)
        {
            HMIXEROBJ hmixer;
            
            //
            // open the mixer in question
            //
            if (MMSYSERR_NOERROR != mixerOpen((LPHMIXER)&hmixer, uDeviceID, NULL, NULL, MIXER_OBJECTF_MIXER))
            {
                debugf("Failed to open mixer %d\n", uDeviceID);
                continue;
            }

            //
            // look for a mixer line attached to a CD
            //
            MIXERLINE mixerline;

            mixerline.cbStruct = sizeof(mixerline);
            mixerline.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;
            if (MMSYSERR_NOERROR != mixerGetLineInfo(hmixer, &mixerline, MIXER_GETLINEINFOF_COMPONENTTYPE)
                || mixerline.cControls == 0)
            {
                debugf("Failed to find CD line on mixer %d\n", uDeviceID);
                mixerClose((HMIXER)hmixer);
                continue;
            }

            //
            // look for a volume control for that mixer line
            //
            MIXERLINECONTROLS mixerlinecontrols; 
            MixerLineData mixerlinedata;

            mixerlinecontrols.cbStruct = sizeof(mixerlinecontrols);
            mixerlinecontrols.dwLineID = mixerline.dwLineID;
            mixerlinecontrols.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
            mixerlinecontrols.cControls = 1;
            mixerlinecontrols.cbmxctrl = sizeof(MIXERCONTROL);
            mixerlinecontrols.pamxctrl = &(mixerlinedata.mixercontrol);

            mixerlinedata.hmixer = hmixer;
            mixerlinedata.cChannels = mixerline.cChannels;

            if (MMSYSERR_NOERROR != 
                mixerGetLineControls(hmixer, &mixerlinecontrols, MIXER_GETLINECONTROLSF_ONEBYTYPE))
            {
                debugf("Failed to find CD volume fader on mixer %d\n", uDeviceID);
                mixerClose((HMIXER)hmixer);
                continue;
            }
            
            // don't try to use more than 8 channels (not likely to be a problem)
            if (mixerlinedata.cChannels > c_nMaxChannels)
                mixerlinedata.cChannels = 1;

            //
            // Get the initial volume settings (so we can restore them when we are done)
            //
            MIXERCONTROLDETAILS mixercontroldetails;

            mixercontroldetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
            mixercontroldetails.dwControlID = mixerlinedata.mixercontrol.dwControlID;
            mixercontroldetails.cChannels = mixerlinedata.cChannels;
            mixercontroldetails.cMultipleItems = 0;
            mixercontroldetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
            mixercontroldetails.paDetails = &(mixerlinedata.vmixercontrolsOld);
            
            if (MMSYSERR_NOERROR !=
                mixerGetControlDetails(hmixer, &mixercontroldetails, MIXER_GETCONTROLDETAILSF_VALUE))
            {
                debugf("Failed to get previous volume levels for mixer %d\n", uDeviceID);
                mixerClose((HMIXER)hmixer);
                continue;
            }

            // add this to the list of volume controls
            m_listMixerLines.push_back(mixerlinedata);
        }
    }

    ~CDVolume()
    {
        // restore the volume settings for all of the CD players
        while (!m_listMixerLines.empty())
        {
            MixerLineData& mixerlinedata = m_listMixerLines.back();

            MIXERCONTROLDETAILS mixercontroldetails;

            mixercontroldetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
            mixercontroldetails.dwControlID = mixerlinedata.mixercontrol.dwControlID;
            mixercontroldetails.cChannels = mixerlinedata.cChannels;
            mixercontroldetails.cMultipleItems = 0;
            mixercontroldetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
            mixercontroldetails.paDetails = &(mixerlinedata.vmixercontrolsOld);
            
            ZVerify(mixerSetControlDetails(mixerlinedata.hmixer, &mixercontroldetails, MIXER_SETCONTROLDETAILSF_VALUE) 
                == MMSYSERR_NOERROR);
            ZVerify(mixerClose((HMIXER)mixerlinedata.hmixer) == MMSYSERR_NOERROR);

            m_listMixerLines.pop_back();
        }
    }

    HRESULT SetGain(float fGain)
    {
        if (fGain > 0.0f)
        {
            return E_INVALIDARG;
        }

        const float fMinGain = -40;
        float fClippedGain = max(fMinGain, fGain);

        // set the volume on every CD player (since we can't map to the right one)
        // restore the volume settings for all of the CD players
        std::list<MixerLineData>::iterator mixerline;
        for (mixerline = m_listMixerLines.begin(); mixerline != m_listMixerLines.end(); ++mixerline)
        {
            MixerLineData& mixerlinedata = *mixerline;

            // translate the gain to a linear volume setting.
            MIXERCONTROLDETAILS_UNSIGNED volume;
            volume.dwValue = (DWORD)(mixerlinedata.mixercontrol.Bounds.dwMinimum
                + (mixerlinedata.mixercontrol.Bounds.dwMaximum - mixerlinedata.mixercontrol.Bounds.dwMinimum) 
                    * (1 - fClippedGain/fMinGain));

            // set the volume for this control
            MIXERCONTROLDETAILS mixercontroldetails;

            mixercontroldetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
            mixercontroldetails.dwControlID = mixerlinedata.mixercontrol.dwControlID;
            mixercontroldetails.cChannels = 1;
            mixercontroldetails.cMultipleItems = 0;
            mixercontroldetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
            mixercontroldetails.paDetails = &volume;
            
            ZVerify(mixerSetControlDetails(mixerlinedata.hmixer, &mixercontroldetails, MIXER_SETCONTROLDETAILSF_VALUE) 
                == MMSYSERR_NOERROR);
        }

        return S_OK;
    }
};

//
// An implementation of a generic disk player wrapper that wraps a CD player
//

class DiskPlayerImpl : public IDiskPlayer, private WorkerThread
{
    UINT m_idDevice;
    
    // the device name
    ZString m_strElementName;

    // a critical section controling access to the queued track and current track
    std::mutex m_csTrack;

    enum { trackEmpty = -1, trackStop = 0 };

    // the next track to play (trackEmpty if the queue is empty, or trackStop 
    // if a stop is queued).
    volatile int m_nQueuedTrack;

    // the current track of the CD player (or trackStop if the cd player is stopped).
    volatile int m_nCurrentTrack;

    CDVolume m_cdvolume;

public:

    DiskPlayerImpl() :
        m_nQueuedTrack(trackEmpty), 
        m_nCurrentTrack(trackStop)
    {
    };

    ~DiskPlayerImpl()
    {
        StopThread();
    }

    HRESULT Init(const ZString& strDevice)
    {
        DWORD dwError; 

        // try to open the device
        MCI_OPEN_PARMS mciOpenParms;
        DWORD dwFlags;

        mciOpenParms.lpstrDeviceType = (LPCSTR) MCI_DEVTYPE_CD_AUDIO;
        dwFlags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID;

        // translate the device name into an element name, if appropriate
        if (!strDevice.IsEmpty())
        {
            m_strElementName = TranslateElementName(strDevice);
            mciOpenParms.lpstrElementName = m_strElementName;
            dwFlags |= MCI_OPEN_ELEMENT;
        }

        // try opening it to make sure it exists
        dwError = mciSendCommand(NULL, MCI_OPEN, dwFlags, (UINT_PTR)&mciOpenParms);  //Fix memory leak -Imago 8/2/09
        if (dwError)
        {
            char cbError[256];
            mciGetErrorString(dwError, cbError, 256);
            debugf("Open failed for CD Audio device '%c': %s\n", (const char*)strDevice, cbError);
            return E_FAIL;
        }
        mciSendCommand(mciOpenParms.wDeviceID, MCI_CLOSE, 0, NULL);

        // start the background (io) thread
        StartThread(THREAD_PRIORITY_NORMAL, 200);

        return S_OK;
    }

    void ThreadInit()
    {
        DWORD dwError; 

        // try to open the device
        MCI_OPEN_PARMS mciOpenParms;
        DWORD dwFlags;
        ZString strElementName;

        mciOpenParms.lpstrDeviceType = (LPCSTR) MCI_DEVTYPE_CD_AUDIO;
        dwFlags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID;

        if (!m_strElementName.IsEmpty())
        {
            mciOpenParms.lpstrElementName = m_strElementName;
            dwFlags |= MCI_OPEN_ELEMENT;
        }

        dwError = mciSendCommand(NULL, MCI_OPEN, dwFlags, (UINT_PTR)&mciOpenParms);
        if (dwError)
        {
            char cbError[256];
            mciGetErrorString(dwError, cbError, 256);
            debugf("Open failed for CD Audio device '%': %s\n", (const char*)m_strElementName, cbError);
        }
        m_idDevice = mciOpenParms.wDeviceID;


        // Set the time format to track/minute/second/frame (TMSF).
        MCI_SET_PARMS mciSetParms;
        mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;

        dwError = mciSendCommand(m_idDevice, MCI_SET, MCI_SET_TIME_FORMAT, (UINT_PTR)&mciSetParms);
        if (dwError)
        {
            char cbError[256];
            mciGetErrorString(dwError, cbError, 256);
            debugf("Set format failed for CD Audio device: %s\n", cbError);
        } 
    }

    // called for background thread; changes state (since that can involve 
    // slow blocking calls) and updates the state (since the state is read 
    // frequently and querying the device is a bit slow)  
    bool ThreadIteration()
    {
        // fetch the current requested state and copy it to the current state
        int nRequestedTrack;
        int nOldTrack;
        {
            std::lock_guard<std::mutex> lock(m_csTrack);

            nOldTrack = m_nCurrentTrack;
            nRequestedTrack = m_nQueuedTrack;
            
            if (nRequestedTrack != trackEmpty)
                m_nCurrentTrack = nRequestedTrack;
            m_nQueuedTrack = trackEmpty;
        }

        // if there is a new state requested, make that change
        if (nRequestedTrack != trackEmpty)
        {
            if (nRequestedTrack == trackStop)
                StopImpl();
            else 
                PlayImpl(nRequestedTrack);
        }
        // otherwise, just update the current state
        else 
        {
            if ((nOldTrack != trackStop) && (IsPlayingImpl() != S_OK))
            {
				std::lock_guard<std::mutex> lock(m_csTrack);
                m_nCurrentTrack = trackStop;
            }
        }

        return true;
    }

    void ThreadCleanup()
    {
        StopImpl();
        mciSendCommand(m_idDevice, MCI_CLOSE, 0, NULL);
    }

    ZString TranslateElementName(const ZString& strDevice)
    {
        if (strDevice.Find(':') != -1)
        {
            return strDevice;
        }
        else
        {
            // get a list of all of the drives on the system
            char cTemp;
            int nDrivesStringLength = GetLogicalDriveStrings(1, &cTemp);
            if (nDrivesStringLength == 0)
            {
                ZError("Error getting drives list\n");
                return strDevice;
            }

            char* cbDrives = (char*)_alloca(nDrivesStringLength);
    
            nDrivesStringLength = GetLogicalDriveStrings(nDrivesStringLength, cbDrives);

            if (nDrivesStringLength == 0)
            {
                ZError("Error getting drives list\n");
                return strDevice;
            }

            // search through the list of drives looking for a CD-ROM who's volume 
            // label matches strDevice
            while (cbDrives[0] != '\0')
            {
                const int c_nVolumeNameLength = 1024;
                char cbVolumeName[c_nVolumeNameLength];
                
                if (GetDriveType(cbDrives) == DRIVE_CDROM
                    && GetVolumeInformation(cbDrives, cbVolumeName, 
                        c_nVolumeNameLength, NULL, NULL, NULL, NULL, 0))
                {
                    if (_stricmp(strDevice, cbVolumeName) == 0)
                    {
                        return cbDrives;
                    }
                }

                cbDrives += strlen(cbDrives) + 1;
            }

            return strDevice;
        }
    }

    // plays one track
    virtual HRESULT Play(int nTrack)
    {
		std::lock_guard<std::mutex> lock(m_csTrack);

        if (nTrack <= 0)
        {
            return E_INVALIDARG;
        }

        m_nQueuedTrack = nTrack;

        return S_OK;
    }

    // stops the CD player
    virtual HRESULT Stop()
    {
		std::lock_guard<std::mutex> lock(m_csTrack);

        m_nQueuedTrack = trackStop;

        return S_OK;
    }

    // returns S_OK if the CD player is playing, S_FALSE otherwise
    virtual HRESULT IsPlaying()
    {
		std::lock_guard<std::mutex> lock(m_csTrack);
        int nTrack;
        
        if (m_nQueuedTrack != trackEmpty)
        {
            nTrack = m_nQueuedTrack;
        }
        else
        {
            nTrack = m_nCurrentTrack;
        }

        return (nTrack == trackStop) ? S_FALSE : S_OK;
    }

    // returns the current track of the CD player
    virtual HRESULT GetCurrentTrack(int& nTrack)
    {
		std::lock_guard<std::mutex> lock(m_csTrack);
        
        if (m_nQueuedTrack != trackEmpty)
        {
            nTrack = m_nQueuedTrack;
        }
        else
        {
            nTrack = m_nCurrentTrack;
        }

        return (nTrack != trackStop) ? S_OK : E_FAIL;
    }

    // sets the gain on the CD player, from 0 to -100 dB
    virtual HRESULT SetGain(float fGain)
    {
        return m_cdvolume.SetGain(fGain);
    };

    // plays one track (blocking)
    HRESULT PlayImpl(int nTrack)
    {
        MCI_PLAY_PARMS mciPlayParms;
        mciPlayParms.dwFrom = MCI_MAKE_TMSF(nTrack, 0, 0, 0);
        mciPlayParms.dwTo = MCI_MAKE_TMSF(nTrack + 1, 0, 0, 0);
        DWORD dwError = mciSendCommand(m_idDevice, MCI_PLAY, MCI_FROM | MCI_TO, (UINT_PTR)&mciPlayParms);

        // if the track is out of range, retry without the stop point in case 
        // this is the last track on the CD.  (review: we could store it, but
        // this case handles switching the CD)
        if (dwError == MCIERR_OUTOFRANGE)
            dwError = mciSendCommand(m_idDevice, MCI_PLAY, MCI_FROM, (UINT_PTR)&mciPlayParms);

        // this is the highest track on the CD
        if (dwError)
        {
            char cbError[256];
            mciGetErrorString(dwError, cbError, 256);
            debugf("Play track %d failed for CD Audio device: %s\n", nTrack, cbError);
            return E_FAIL;
        }
        return S_OK;
    };

    // stops the CD player (blocking)
    HRESULT StopImpl()
    {
        DWORD dwError = mciSendCommand(m_idDevice, MCI_STOP, 0, NULL);
        if (dwError)
        {
            char cbError[256];
            mciGetErrorString(dwError, cbError, 256);
            debugf("Stop failed for CD Audio device: %s\n", cbError);
            return E_FAIL;
        }
        return S_OK;
    };

    // returns S_OK if the CD player is playing, S_FALSE otherwise (blocking)
    HRESULT IsPlayingImpl()
    {
        MCI_STATUS_PARMS mciStatusParams;
        mciStatusParams.dwItem = MCI_STATUS_MODE;

        DWORD dwError = mciSendCommand(m_idDevice, MCI_STATUS, MCI_STATUS_ITEM, (UINT_PTR)&mciStatusParams);
        if (dwError)
        {
            char cbError[256];
            mciGetErrorString(dwError, cbError, 256);
            debugf("Status:Mode failed for CD Audio device: %s\n", cbError);
            return E_FAIL;
        }
        return (mciStatusParams.dwReturn == MCI_MODE_PLAY) ? S_OK : S_FALSE;
    };

    // returns the current track of the CD player (blocking)
    HRESULT GetCurrentTrackImpl(int& nTrack)
    {
        MCI_STATUS_PARMS mciStatusParams;
        mciStatusParams.dwItem = MCI_STATUS_CURRENT_TRACK;

        DWORD dwError = mciSendCommand(m_idDevice, MCI_STATUS, MCI_STATUS_ITEM, (UINT_PTR)&mciStatusParams);
        if (dwError)
        {
            char cbError[256];
            mciGetErrorString(dwError, cbError, 256);
            debugf("Status:Track failed for CD Audio device: %s\n", cbError);
            return E_FAIL;
        }
        nTrack = mciStatusParams.dwReturn;
        return S_OK;
    };
};
    
// Creates a new disk player object (CD, minidisk, etc.).  strDevice can be 
// empty (choose any device), a path ("E:\"), or a volume label.  
HRESULT CreateDiskPlayer(TRef<IDiskPlayer>& pdiskplayer, const ZString& strDevice)
{
    TRef<DiskPlayerImpl> pdiskplayerimpl = new DiskPlayerImpl();

    HRESULT hr = pdiskplayerimpl->Init(strDevice);

    if (SUCCEEDED(hr))
        pdiskplayer = pdiskplayerimpl;

    return hr;
};


class DummyDiskPlayer : public IDiskPlayer
{
public:
    // plays one track
    virtual HRESULT Play(int nTrack)
    {
        return S_OK;
    };

    // stops the CD player
    virtual HRESULT Stop()
    {
        return S_OK;
    };

    // returns S_OK if the CD player is playing
    virtual HRESULT IsPlaying()
    {
        return S_FALSE;
    };

    // returns the current track of the CD player
    virtual HRESULT GetCurrentTrack(int& nTrack)
    {
        nTrack = 1;
        return S_OK;
    };


    // sets the gain on the CD player, from 0 to -100 dB
    virtual HRESULT SetGain(float fGain)
    {
        return S_OK;
    };
};
    
// Creates a new disk player object that only has stubs for each of the calls
HRESULT CreateDummyDiskPlayer(TRef<IDiskPlayer>& pdiskplayer)
{
    pdiskplayer = new DummyDiskPlayer();
    return S_OK;
}



// a template for redbook audio
class RedbookSoundTemplate : public ISoundTemplate
{
private:
    TRef<IDiskPlayer> m_pdiskplayer;
    int m_nTrack;

    //
    // Playback controls
    //
    class RedbookSoundInstance : public ISoundInstance
    {
        TRef<IDiskPlayer> m_pdiskplayer;
        int m_nTrack;

    public:

        RedbookSoundInstance(TRef<IDiskPlayer> pdiskplayer, int nTrack) :
            m_pdiskplayer(pdiskplayer),
            m_nTrack(nTrack)
        {
            pdiskplayer->Play(nTrack);
        }

        // Stops the sound.  If bForceNow is true the sound will stop ASAP, 
        // possibly popping.  If it is false some sounds may play a trail-off 
        // sound or fade away.  
        virtual HRESULT Stop(bool bForceNow = false)
        {
            HRESULT hr = IsPlaying();

            if (hr == S_OK)
            {
                return m_pdiskplayer->Stop();
            }
            else if (SUCCEEDED(hr))
                return S_OK;
            else
                return hr;
        }

        // returns S_OK if the sound is currently playing, S_FALSE otherwise.
        virtual HRESULT IsPlaying()
        {
            HRESULT hr = m_pdiskplayer->IsPlaying();

            if (hr == S_OK)
            {
                // it's playing, but is it playing our track or something else?
                int nTrack;
                hr = m_pdiskplayer->GetCurrentTrack(nTrack);

                if (FAILED(hr))
                    return hr;

                return (nTrack == m_nTrack) ? S_OK : S_FALSE;
            }
            else
                return hr;
        }

        // Gets an event which fires when the sound finishes playing (for any 
        // reason)
        virtual IEventSource* GetFinishEventSource()
        {
            ZError("NYI");
            return NULL;
        }

        // Gets an interface for tweaking the sound, if supported, NULL otherwise.
        virtual TRef<ISoundTweakable> GetISoundTweakable()
        {
            return NULL;
        }

        virtual TRef<ISoundTweakable3D> GetISoundTweakable3D()
        {
            return NULL;
        }
    };


public:
    // tries to initialize the object with the given file.
    HRESULT Init(TRef<IDiskPlayer> pdiskplayer, int nTrack)
    {
        if (!pdiskplayer)
        {
            ZAssert(false);
            return E_POINTER;
        }

        if (nTrack <= 0)
        {
            ZAssert(false);
            return E_INVALIDARG;
        }

        m_pdiskplayer = pdiskplayer;
        m_nTrack = nTrack;

        return S_OK;
    };

    // Creates a new instance of the given sound
    virtual HRESULT CreateSound(TRef<ISoundInstance>& psoundNew, 
        ISoundBufferSource* pbufferSource, ISoundPositionSource* psource = NULL)
    {
        if (!pbufferSource)
        {
            ZAssert(false);
            return E_POINTER;
        }

        if (psource)
        {
            ZAssert(false);
            return E_NOTIMPL;
        }

        psoundNew = new RedbookSoundInstance(m_pdiskplayer, m_nTrack);

        return S_OK;
    }
};

// creates a sound template representing a redbook audio track
HRESULT CreateRedbookSoundTemplate(TRef<ISoundTemplate>& pstDest, TRef<IDiskPlayer> pdiskplayer, int nTrack)
{
    TRef<RedbookSoundTemplate> ptemplate = new RedbookSoundTemplate();
    HRESULT hr = ptemplate->Init(pdiskplayer, nTrack);
    
    if (ZSucceeded(hr))
        pstDest = ptemplate; 

    return hr;
}

};