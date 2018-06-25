#ifndef _TREKI_
#define _TREKI_

#include "cmdview.h"
#include "load.h"

#include "console.h"

#include <enginewindow.h>

class GameConfigurationWrapper : public EngineConfigurationWrapper {

public:
    GameConfigurationWrapper(TRef<UpdatingConfiguration> pconfiguration) :
        EngineConfigurationWrapper(pconfiguration)
    {}

    TRef<SimpleModifiableValue<ZString>> GetDataArtworkPath() {
        return m_pconfiguration->GetString("Data.ArtworkPath", m_pconfiguration->GetStringValue("ArtPath", ""));
    }

    TRef<SimpleModifiableValue<ZString>> GetOnlineCharacterName() {
        return m_pconfiguration->GetString("Online.CharacterName", m_pconfiguration->GetStringValue("CharacterName", ""));
    }

    TRef<SimpleModifiableValue<ZString>> GetOnlineSquadTag() {
        //we are going to store the string tag, we used to store the steam id of the squad
        return m_pconfiguration->GetString("Online.SquadTag", m_pconfiguration->GetStringValue("SteamClanId", ""));
    }

    TRef<SimpleModifiableValue<ZString>> GetOnlineOfficerToken() {
        return m_pconfiguration->GetString("Online.OfficerToken", m_pconfiguration->GetStringValue("SteamOfficerToken", ""));
    }

    TRef<SimpleModifiableValue<bool>> GetUiShowStartupCreditsMovie() {
        return m_pconfiguration->GetBool("Ui.ShowStartupCreditsMovie", true);
    }

    TRef<SimpleModifiableValue<bool>> GetUiShowStartupIntroMovie() {
        return m_pconfiguration->GetBool("Ui.ShowStartupIntroMovie", true);
    }

    TRef<SimpleModifiableValue<bool>> GetUiShowStartupTrainingSuggestion() {
        return m_pconfiguration->GetBool("Ui.ShowStartupTrainingSuggestion", true);
    }

    TRef<SimpleModifiableValue<bool>> GetUiUseOldUi() {
        return m_pconfiguration->GetBool("Ui.UseOldUi", true);
    }

    TRef<SimpleModifiableValue<bool>> GetChatFilterChatsToAll() {
        return m_pconfiguration->GetBool("Chat.FilterChatsToAll", false);
    }

    TRef<SimpleModifiableValue<bool>> GetChatFilterVoiceChats() {
        return m_pconfiguration->GetBool("Chat.FilterVoiceChats", false);
    }

    TRef<SimpleModifiableValue<bool>> GetChatFilterChatsFromLobby() {
        return m_pconfiguration->GetBool("Chat.FilterChatsFromLobby", false);
    }

    TRef<SimpleModifiableValue<bool>> GetChatFilterUnknownChats() {
        return m_pconfiguration->GetBool("Chat.FilterUnknownChats", false);
    }

    TRef<SimpleModifiableValue<bool>> GetChatCensorChat() {
        return m_pconfiguration->GetBool("Chat.CensorChat", m_pconfiguration->GetBoolValue("CensorChats", true));
    }

    TRef<SimpleModifiableValue<float>> GetChatNumberOfLines() {
        return m_pconfiguration->GetInt("Chat.NumberOfLines", m_pconfiguration->GetIntValue("ChatLines", 10));
    }

    TRef<SimpleModifiableValue<float>> GetSoundEffectVolume() {
        return m_pconfiguration->GetInt("Sound.EffectVolume", -8);
    }

    TRef<SimpleModifiableValue<float>> GetSoundVoiceVolume() {
        return m_pconfiguration->GetInt("Sound.VoiceVolume", -13);
    }

    TRef<SimpleModifiableValue<bool>> GetJoystickControlsLinear() {
        return m_pconfiguration->GetBool("Joystick.ControlsLinear", m_pconfiguration->GetBoolValue("LinearControlResponse", false));
    }

    TRef<SimpleModifiableValue<float>> GetJoystickDeadzoneSize() {
        return m_pconfiguration->GetInt("Joystick.DeadzoneSize", m_pconfiguration->GetIntValue("DeadZone", 10));
    }

    TRef<SimpleModifiableValue<bool>> GetJoystickFlipYAxis() {
        return m_pconfiguration->GetBool("Joystick.FlipYAxis", m_pconfiguration->GetBoolValue("FlipY", false));
    }

    TRef<SimpleModifiableValue<bool>> GetJoystickUseMouseAsJoystick() {
        return m_pconfiguration->GetBool("Joystick.UseMouseAsJoystick", m_pconfiguration->GetBoolValue("EnableVirtualJoystick", false));
    }

    TRef<SimpleModifiableValue<bool>> GetJoystickShowDirectionIndicator() {
        return m_pconfiguration->GetBool("Joystick.ShowDirectionIndicator", m_pconfiguration->GetBoolValue("ShowJoystickIndicator", true));
    }

    TRef<SimpleModifiableValue<bool>> GetJoystickEnableForceFeedback() {
        return m_pconfiguration->GetBool("Joystick.EnableForceFeedback", m_pconfiguration->GetBoolValue("EnableFeedback", false));
    }

    TRef<SimpleModifiableValue<float>> GetMouseSensitivity() {
        return m_pconfiguration->GetFloat("Mouse.Sensitivity", m_pconfiguration->GetFloatValue("MouseSensitivity", 0.6f));
    }

    TRef<SimpleModifiableValue<float>> GetMouseAcceleration() {
        return m_pconfiguration->GetInt("Mouse.Acceleration", m_pconfiguration->GetIntValue("MouseAcceleration", 0));
    }

    TRef<SimpleModifiableValue<bool>> GetGraphicsEnvironment() {
        return m_pconfiguration->GetBool("Graphics.Environment", m_pconfiguration->GetBoolValue("Environment", true));
    }

    TRef<SimpleModifiableValue<bool>> GetGraphicsPosters() {
        return m_pconfiguration->GetBool("Graphics.Posters", m_pconfiguration->GetBoolValue("Posters", true));
    }

    TRef<SimpleModifiableValue<float>> GetGraphicsDebris() {
        return m_pconfiguration->GetFloat("Graphics.Debris", m_pconfiguration->GetFloatValue("Debris", 1.0f));
    }

    TRef<SimpleModifiableValue<bool>> GetGraphicsStars() {
        return m_pconfiguration->GetBool("Graphics.Stars", m_pconfiguration->GetBoolValue("Stars", true));
    }

    TRef<SimpleModifiableValue<bool>> GetGraphicsBoundingBoxes() {
        return m_pconfiguration->GetBool("Graphics.BoundingBoxes", m_pconfiguration->GetBoolValue("Bounds", false));
    }

    TRef<SimpleModifiableValue<bool>> GetGraphicsTransparentObjects() {
        return m_pconfiguration->GetBool("Graphics.TransparentObjects", m_pconfiguration->GetBoolValue("TransparentObjects", false));
    }

    TRef<SimpleModifiableValue<bool>> GetGraphicsParticles() {
        return m_pconfiguration->GetBool("Graphics.Particles", m_pconfiguration->GetIntValue("SmokeEffects", 3) == 3);
    }

    TRef<SimpleModifiableValue<bool>> GetGraphicsLensFlare() {
        return m_pconfiguration->GetBool("Graphics.LensFlare", m_pconfiguration->GetBoolValue("LensFlare", true));
    }

    TRef<SimpleModifiableValue<float>> GetUiHudStyle() {
        return m_pconfiguration->GetInt("Ui.HudStyle", 0);
    }
};

#include "CallsignTagInfo.h"


//todo ModdingEngine: Move this to its own file
class Mod {
private:
    std::string m_strName;

    std::string m_strArtPath;

public:
    Mod(std::string strName, std::string strArtPath) :
        m_strName(strName),
        m_strArtPath(strArtPath)
    {

    }

    std::string GetName() {
        return m_strName;
    }

    std::string GetArtPath() {
        return m_strArtPath;
    }
};

class ModdingEngine {
private:
    std::vector<std::shared_ptr<Mod>> m_vMods;

public:

    static const std::string GetPath() {
        return GetExecutablePath() + "\\Mods";
    }

    ModdingEngine(const std::vector<std::shared_ptr<Mod>>& vMods) :
        m_vMods(vMods)
    {

    }

    static std::shared_ptr<ModdingEngine> Create() {
        std::vector<std::shared_ptr<Mod>> vMods = {};

        //Go through the mod directory and add each directory in there

        std::string search_path = ModdingEngine::GetPath() + "\\*";
        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && fd.cFileName[0] != '.') {
                    vMods.push_back(std::make_shared<Mod>(fd.cFileName, ModdingEngine::GetPath() + "\\" + fd.cFileName));
                }
            } while (::FindNextFile(hFind, &fd));
            ::FindClose(hFind);
        }

        return std::make_shared<ModdingEngine>(vMods);
    }

    const std::vector<std::shared_ptr<Mod>>& GetMods() {
        return m_vMods;
    }
};

class TrekApp : public EffectApp {
public:
    virtual std::shared_ptr<CallsignHandler> GetCallsignHandler() = 0;
    virtual TRef<GameConfigurationWrapper> GetGameConfiguration() = 0;
    virtual std::shared_ptr<ModdingEngine> GetModdingEngine() = 0;
};

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

class TrekWindow : public Value, public IKeyboardInput {
protected:
    TRef<EngineWindow> m_pEngineWindow;
    TRef<Modeler> m_pmodeler;
    TRef<Engine> m_pengine;

    TRef<IPopupContainer>      m_ppopupContainer;

    TrekWindow(
        EngineWindow* pengineWindow
    ) :
        m_pEngineWindow(pengineWindow)
    {

    }

    virtual void Terminate() = 0;

    virtual void EvaluateFrame(Time time) = 0;

public:

    static TRef<TrekWindow> Create(
        TrekApp*     papp,
        EngineWindow* pengineWindow,
        const ZString& strCommandLine, 
// BUILD_DX9
		const ZString& strArtPath,					// Added for DX9 build, due to reordered startup.
// BUILD_DX9
        bool           bMovies
    );

    virtual void Start() {
        m_pEngineWindow->GetOnCloseEventSource()->AddSink(new CallbackSink([this]() {
            this->Terminate();
            return true;
        }));

        m_pEngineWindow->GetEvaluateFrameEventSource()->AddSink(new CallbackValueSink<Time>([this](Time time) {
            this->EvaluateFrame(time);
            return true;
        }));
    }

    EngineWindow* GetEngineWindow() { 
        return m_pEngineWindow;
    }

    Modeler* GetModeler() {
        return m_pmodeler;
    }

    Number* GetTime() {
        return m_pEngineWindow->GetTime();
    }

    IPopupContainer* GetPopupContainer() { return m_ppopupContainer; }

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

    virtual void ChangeChatMessage(void) = 0;
    virtual void AddMuzzleFlare(const Vector& vecDirection, float duration) = 0;

    virtual void SetCluster(IclusterIGC* pcluster) = 0;
    virtual void PositionCommandView(const Vector*  pposition = NULL, float dt = 2.0f) = 0;

    virtual HRESULT HandleMsg(FEDMESSAGE* pfm,
                              Time        lastUpdate,
                              Time        now) = 0;

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
EngineWindow* GetEngineWindow();
Engine*     GetEngine();
Modeler*    GetModeler();

#endif
