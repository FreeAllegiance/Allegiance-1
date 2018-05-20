#ifndef _TREKI_
#define _TREKI_

#include "cmdview.h"
#include "load.h"

#include "console.h"

//////////////////////////////////////////////////////////////////////////////
//
// Main Trek Window
//
//////////////////////////////////////////////////////////////////////////////

extern bool g_bEnableSound;

enum ScreenID {
    ScreenIDInvalid             =  -2,
    ScreenIDCombat              =  -1,
    ScreenIDSplashScreen        =   0,
    ScreenIDIntroScreen         =   1,
    ScreenIDCreateMission       =   2,
    ScreenIDTeamScreen          =   3,
    ScreenIDGameScreen          =   4,
    ScreenIDGameOverScreen      =   5,
    ScreenIDTrainScreen         =   6,
    ScreenIDZoneClubScreen      =   7,
    ScreenIDSquadsScreen        =   8,
    ScreenIDGameStarting        =   9,
    ScreenIDCharInfo            =   10,
    ScreenIDTrainSlideshow      =   11,
    ScreenIDPostTrainSlideshow  =   12,
    ScreenIDLeaderBoard         =   13,
    ScreenIDZoneEvents          =   14
};

//global
UpdatingConfiguration* GetConfiguration();

//////////////////////////////////////////////////////////////////////////////
//
// Main Trek Window
//
// A Trek Window object contains all of the information that is global to a
// a single connection to the server.  There could conceivably be more than
// one TrekWindow object in a process.
//
//////////////////////////////////////////////////////////////////////////////

class ChatListPane;

class TrekWindow : public EffectWindow {
protected:
    TrekWindow(
        EffectApp* papp,
        const ZString&     strCommandLine,
              bool         bStartFullscreen,
        const WinRect&     rect,
        const WinPoint&    sizeMin
    ) :
        EffectWindow(
            papp,
            GetConfiguration(),
            strCommandLine,
            GetWindowTitle(),
            bStartFullscreen,
            rect,
            sizeMin
        ) 
    {
    }

public:
    static TRef<TrekWindow> Create(
        EffectApp*     papp, 
        const ZString& strCommandLine, 
// BUILD_DX9
		const ZString& strArtPath,					// Added for DX9 build, due to reordered startup.
// BUILD_DX9
        bool           bMovies
    );

    static LPCTSTR          GetWindowTitle() { return TEXT("Allegiance"); };

    virtual TRef<AnimatedImage> LoadAnimatedImage(Number* ptime, const ZString& str) = 0;

    virtual void             UpdateGameStateContainer() = 0;

    virtual Camera*             GetCamera()             = 0;
    virtual Orientation         GetCameraOrientation()  = 0;
    virtual void                ResetCameraFOV()        = 0;
    virtual Camera*             GetCameraPosters()      = 0;
    virtual ConsoleImage*       GetConsoleImage()       = 0;
    virtual Viewport*           GetViewport()           = 0;
    virtual Viewport*           GetPosterViewport()     = 0;

    virtual void             SetTimeDamaged(Time now) = 0;

    virtual void             ToggleWeapon (Mount mount) = 0;
    virtual void             SwapPart(EquipmentType et, Mount mount) = 0;

    virtual float            GetDeltaTime (void) const = 0;

    virtual float            GetHUDStyle (void) const = 0;
    virtual void             SetHUDStyle (float newStyle) = 0;


    enum CameraMode
    { 
        cmCockpit,

        cmExternal,                 cmExternalMin=cmExternal, 
        cmExternalChase, 
        cmExternalTarget, 
        cmExternalBase, 
        cmExternalMissile,  
        cmExternalFlyBy,
        cmExternalFlyByTarget, 
        cmExternalOrbit,
        cmExternalStation,
        cmExternalOverride,
        cmExternalTurret,           cmExternalMax = cmExternalTurret,


        cmExternalCommandView34,    cmExternalCommandMin = cmExternalCommandView34,
        cmExternalCommandViewTD,    cmExternalCommandMax = cmExternalCommandViewTD,
    };

    enum ViewMode
    {
        vmHangar,
        vmLoadout,
        vmCommand,
        vmCombat,
        vmOverride,
        vmUI, // in the UI screens
        c_cViewModes
    };

    static bool NoCameraControl(CameraMode cm)
    {
        return InternalCamera(cm) || (cm == cmExternalMissile) || (cm == cmExternalChase);
    }

    static bool InternalCamera(CameraMode cm)
    {
        return (cm == cmCockpit);
    }

    static bool ExternalCamera(CameraMode cm)
    {
        return ((cm >= cmExternalMin) && (cm <= cmExternalMax));
    }

    static bool CommandCamera(CameraMode cm)
    {
        return ((cm >= cmExternalCommandMin) && (cm <= cmExternalCommandMax));
    }

    virtual int         GetRadarMode(void) const = 0;
    virtual void        SetRadarMode(int radarMode) = 0;
    virtual bool        GetRoundRadarMode(void) const = 0;

    virtual bool        GetShowJoystickIndicator(void) const = 0;

    virtual CameraMode  GetCameraMode(void) const = 0;
    virtual void        SetCameraMode(CameraMode cm) = 0;

    virtual ViewMode    GetViewMode(void) const = 0;
    virtual void        SetViewMode(ViewMode vm, bool bForce = false) = 0;

    virtual void        EnableDisplacementCommandView (bool bEnable) = 0;

    virtual OverlayMask GetOverlayFlags() const = 0;
    virtual void        SetOverlayFlags(OverlayMask om) = 0;
    virtual void        TurnOnOverlayFlags(OverlayMask om) = 0; // set & clear were ambiguous
    virtual void        TurnOffOverlayFlags(OverlayMask om) = 0;
    virtual void        ToggleOverlayFlags(OverlayMask om) = 0;

    virtual void        SetJiggle(float jiggle) = 0;

    virtual void            OverrideCamera(Time now, ImodelIGC* pmodelTarget, bool bOverridePosition) = 0;

    //virtual void        TurretChange(void) = 0;

    virtual void                StartTraining (int iMissionIndex) = 0;

    virtual void                screen(ScreenID s)        = 0;
    virtual ScreenID            screen(void) const        = 0;
    virtual void        CharInfoScreenForPlayer(int idZone) = 0;
    virtual void        SquadScreenForPlayer(const char * szName, int idZone, const char * szSquad) = 0;
    virtual void        LeaderBoardScreenForPlayer(const ZString & strCharacter) = 0;

    virtual void SetTarget(ImodelIGC*   pmodel, CommandID cid) = 0;
    virtual void SetAccepted(ImodelIGC* pmodel, CommandID cid) = 0;

    // call this to terminate this gaming session

    virtual void Terminate() = 0;

    virtual void ChangeChatMessage(void) = 0;
    virtual void AddMuzzleFlare(const Vector& vecDirection, float duration) = 0;

    virtual void SetCluster(IclusterIGC* pcluster) = 0;
    virtual void PositionCommandView(const Vector*  pposition = NULL, float dt = 2.0f) = 0;

    virtual HRESULT HandleMsg(FEDMESSAGE* pfm,
                              Time        lastUpdate,
                              Time        now) = 0;
    virtual VOID VTSetText(LPSTR szFormat, ...) = 0;

    virtual void SetCursor(const char* pszCursorImage) = 0;
    virtual void SetWaitCursor() = 0;
    virtual void RestoreCursor() = 0;
    virtual void SetLightDirection(const Vector& direction) = 0;
    virtual void SetTargetGeo(ImodelIGC* pmodel) = 0;

    virtual void OnHelp(bool bOn) = 0;
    virtual void OpenMainQuickChatMenu() = 0;
    virtual ZString GetSonicChatText(int id, int voice) = 0;
    virtual void PlaySonicChat(int id, int voice) = 0;

    virtual void                  SoundEngineUpdate (void) = 0;
    virtual TRef<ISoundInstance>  StartSound(ISoundTemplate* ptemplate, ISoundPositionSource* psource = NULL) = 0;
    virtual TRef<ISoundInstance>  StartSound(SoundID soundId, ISoundPositionSource* psource = NULL) = 0;

    virtual void PlayFFEffect(ForceEffectID effectID, LONG lDirection = 0) = 0;
    virtual void SetChatListPane(ChatListPane* pchatListPane) = 0;
    virtual void SetLobbyChatTarget(ChatTarget ct, ObjectID recip = NA) = 0;
    virtual ChatTarget GetLobbyChatTarget() = 0;
	virtual ObjectID GetLobbyChatRecip() = 0;

    virtual void StartLockDown(const ZString& strReason) = 0;
    virtual void EndLockDown() = 0;

    virtual void ShowWebPage(const char* szURL = "") = 0;

    virtual void DoInputConfigure() = 0;

	virtual void ShowOptionsMenu() = 0;

	virtual void ShowPlayerContextMenu(PlayerInfo * playerInfo) = 0;
	virtual void ShowPlayerPaneContextMenu(PlayerInfo * playerInfo) = 0;  //Xynth #48 8/2010

    virtual void RejectQueuedCommand(bool    bClear = true,
                                     bool    bNegative = false) = 0;
    virtual void AcceptQueuedCommand(bool    bExecute) = 0;
    virtual void SetQueuedCommand(IshipIGC* pshipSender, CommandID cid, ImodelIGC* pmodelTarget) = 0;
    virtual void SetQueuedCommand(IshipIGC*  pshipSender, Money      money, HullID     hid) = 0;
    virtual void StartQuitMission() = 0;
    virtual TrekInput*  GetInput (void) = 0;

    virtual AsteroidAbilityBitMask  GetCommandAsteroid(void) const = 0;
    virtual void                    SetCommandAsteroid(AsteroidAbilityBitMask aabm) = 0;
    virtual AsteroidAbilityBitMask  GetInvestAsteroid(void) const = 0;
    virtual void                    SetInvestAsteroid(AsteroidAbilityBitMask aabm) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
//  , These Hack Functions are here to support all of the old
//     static functions.  Eventually it would be nice for all the code
//     to get these pointers elsewhere.
//
//////////////////////////////////////////////////////////////////////////////

TrekWindow* GetWindow();
Engine*     GetEngine();
Modeler*    GetModeler();

#endif
