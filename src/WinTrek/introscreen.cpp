#include "pch.h"
#include "regkey.h"
#include "training.h"
#include "valuetransform.h"

#include "CreateGameDialog.h"

#include "ui.h"
#include "downloader.h"

extern bool CheckNetworkDevices(ZString& strDriverURL);

//KGJV test
#define ENABLE3DINTROSCREEN

class ErrorState : public UiState {
    using UiState::UiState;
};
class NoErrorState : public ErrorState {
public:
    NoErrorState() :
        ErrorState("No")
    {}
};
class YesErrorState : public ErrorState {
public:
    YesErrorState(std::string message) :
        ErrorState("Yes", {
            { "Message", TypeExposer<TRef<StringValue>>::Create(new ModifiableString(message.c_str())) }
        })
    {}
};

class LoggedOutState : public UiState {
private:
    TRef<UiStateValue> m_stateError;

public:
    LoggedOutState(UiStateValue* stateError, IEventSink* sinkLogin) :
        m_stateError(stateError),
        UiState("Logged out", {
            { "Has error", TypeExposer<TRef<UiStateValue>>::Create(stateError) },
            { "Login", TypeExposer<TRef<IEventSink>>::Create(sinkLogin) }
        })
    {
    }

    TRef<UiStateValue> GetHasError() {
        return Get<TRef<UiStateValue>>("Has error");
    }
};

class LoggingInState : public UiState {
public:
    LoggingInState() :
        UiState("Logging in", {
            { "Step number", TypeExposer<TRef<Number>>::Create(new ModifiableNumber(0.0f)) },
            { "Step message", TypeExposer<TRef<StringValue>>::Create(new ModifiableString("")) },
            //{ "Step number", (TRef<Number>)new ModifiableNumber(0.0f) },
            //{ "Step message", (TRef<StringValue>)new ModifiableString("") }
        })
    {
    }

    void SetStep(int step, std::string message) {
        debugf(("Introscreen login step: " + message).c_str());

        ((ModifiableNumber*)(Number*)Get<TRef<Number>>("Step number"))->SetValue((float)step);
        ((ModifiableString*)(StringValue*)Get<TRef<StringValue>>("Step message"))->SetValue(message.c_str());
    }
};

class LoginHelper;
class LoggedInState : public UiState {
private:

public:
    LoggedInState(IEventSink* sinkLogout, IEventSink* sinkCreateGame) :
        UiState("Logged in", {
            { "Callsign", TypeExposer<std::string>::Create(std::string(trekClient.GetNameLogonZoneServer())) },
            { "Mission list", TypeExposer<TRef<ContainerList>>::Create(new ContainerList({})) },
            { "Server list", TypeExposer<TRef<ContainerList>>::Create(new ContainerList({})) },
            { "Core list", TypeExposer<TRef<ContainerList>>::Create(new ContainerList({})) },
            { "Logout", TypeExposer<TRef<IEventSink>>::Create(sinkLogout) },
            { "Create mission dialog", TypeExposer<TRef<IEventSink>>::Create(sinkCreateGame) },
            { "Create mission", TypeExposer<TRef<TEvent<ZString, ZString, ZString>::Sink>>::Create(new CallbackValueSink<ZString, ZString, ZString>([this](ZString serverName, ZString coreName, ZString missionName) {
                auto server = this->GetServer(serverName);
                auto core = this->GetCore(coreName);

                ServerCoreInfo serverinfo = server->Get<ServerCoreInfo>("ServerCoreInfo");
                StaticCoreInfo coreinfo = core->Get<StaticCoreInfo>("StaticCoreInfo");

                trekClient.CreateMissionReq(
                    serverinfo.szName,
                    serverinfo.szRemoteAddress,
                    coreinfo.cbIGCFile,
                    missionName
                );
                return true;
            })) },
            { "Server has core", TypeExposer<std::function<TRef<Boolean>(const TRef<StringValue>&, const TRef<StringValue>&)>>::Create([this](const TRef<StringValue>& server_name, const TRef<StringValue>& core_name) {

                return new TransformedValue2<bool, ZString, ZString>([this](ZString server_name, ZString core_name) {
                    auto server = this->FindServer(server_name);
                    auto core = this->FindCore(core_name);

                    if (!server || !core) {
                        return false;
                    }

                    uint32 server_bitmask = (uint32)server->Get<ServerCoreInfo>("ServerCoreInfo").dwCoreMask;
                    uint32 core_bitmask = core->Get<uint32>("Bitmask");

                    return (bool)(server_bitmask & core_bitmask);
                }, server_name, core_name);
            }) }
        })
    {}

    TRef<UiObjectContainer> FindCore(ZString coreName) {
        auto listCores = Get<TRef<UiList<TRef<UiObjectContainer>>>>("Core list")->GetList();

        auto current_iterator = listCores.begin();

        while (current_iterator != listCores.end()) {
            ZString current_core_name = (*current_iterator)->Get<TRef<StringValue>>("Name")->GetValue();

            if (coreName == current_core_name) {
                return *current_iterator;
            }

            ++current_iterator;
        }

        return nullptr;
    }

    TRef<UiObjectContainer> GetCore(ZString coreName) {
        auto core = FindCore(coreName);
        if (!core) {
            throw std::runtime_error("Core not found");
        }
        return core;
    }

    TRef<UiObjectContainer> FindServer(ZString name) {
        auto listCores = Get<TRef<UiList<TRef<UiObjectContainer>>>>("Server list")->GetList();

        auto current_iterator = listCores.begin();

        while (current_iterator != listCores.end()) {
            ZString current_name = (*current_iterator)->Get<TRef<StringValue>>("Name")->GetValue();

            if (name == current_name) {
                return *current_iterator;
            }

            ++current_iterator;
        }

        return nullptr;
    }

    TRef<UiObjectContainer> GetServer(ZString name) {
        auto server = FindServer(name);
        if (!server) {
            throw std::runtime_error("Server not found");
        }
        return server;
    }

    void SetMissionList(List* plist) {
        std::list<TRef<UiObjectContainer>> result;
        auto current_modifiablelist = Get<TRef<UiList<TRef<UiObjectContainer>>>>("Mission list");
        auto current_list = current_modifiablelist->GetList();

        ItemID pitem = plist->GetItem(0);
        auto current_iterator = current_list.begin();
        int i = 0;
        /* For now, disable compare and just delete/add everything
        while (pitem && current_iterator != current_list.end()) {
            MissionInfo* game = (MissionInfo*)pitem;
            TRef<UiObjectContainer> current_entry = *current_iterator;
            {
                //compare
            }

            pitem = plist->GetNext(pitem);
            ++current_iterator;

        }
        */

        //remove entries
        while (current_iterator != current_list.end()) {
            current_modifiablelist->Remove(i);
            ++current_iterator;
        }
         
        //add entries
        while (pitem) {
            MissionInfo* game = (MissionInfo*)pitem;

            Time timeApplicationStart = GetWindow()->GetTimeStart();
            TRef<Number> pTimeSinceApplicationStart = GetWindow()->GetTime();

            TRef<Number> pTimeMisisonInProgress;
            if (game->InProgress()) {
                float secondsOffset = game->StartTime() - timeApplicationStart;

                pTimeMisisonInProgress = NumberTransform::Subtract(
                    pTimeSinceApplicationStart,
                    new Number(secondsOffset)
                );
            }
            else {
                pTimeMisisonInProgress = new Number(0.0f);
            }

            current_modifiablelist->Insert(i, new UiObjectContainer({
                { "Join", TypeExposer<TRef<IEventSink>>::Create(new CallbackSink([game]() {
                    trekClient.JoinMission(game, "");
                    return false;
                })) },
                { "Name", StringExposer::CreateStatic(game->Name()) },
                { "Player count", NumberExposer::CreateStatic((float)game->NumPlayers()) },
                { "Player noat count", NumberExposer::CreateStatic((float)game->NumNoatPlayers()) },
                { "Max player count", NumberExposer::CreateStatic((float)game->MaxPlayers()) },
                { "Is in progress", BooleanExposer::CreateStatic(game->InProgress()) },
                { "Time in progress", NumberExposer::Create(pTimeMisisonInProgress) },
                { "Server name", StringExposer::CreateStatic(game->GetMissionDef().szServerName) },
                { "Core name", StringExposer::CreateStatic(trekClient.CfgGetCoreName(game->GetIGCStaticFile())) },

                { "Has goal conquest", BooleanExposer::CreateStatic(game->GoalConquest()) },
                { "Has goal territory", BooleanExposer::CreateStatic(game->GoalTerritory()) },
                { "Has goal prosperity", BooleanExposer::CreateStatic(game->GoalProsperity()) },
                { "Has goal artifacts", BooleanExposer::CreateStatic(game->GoalArtifacts()) },
                { "Has goal flags", BooleanExposer::CreateStatic(game->GoalFlags()) },
                { "Has goal deathmatch", BooleanExposer::CreateStatic(game->GoalDeathMatch()) },
                { "Has goal countdown", BooleanExposer::CreateStatic(game->GoalCountdown()) },
            }));
            pitem = plist->GetNext(pitem);
            ++i;
        }
    }

    void SetCoreList(int count, StaticCoreInfo* plist) {
        std::list<TRef<UiObjectContainer>> result;
        auto current_modifiablelist = Get<TRef<UiList<TRef<UiObjectContainer>>>>("Core list");
        auto current_list = current_modifiablelist->GetList();

        auto current_iterator = current_list.begin();
        int i = 0;

        //remove entries
        while (current_iterator != current_list.end()) {
            current_modifiablelist->Remove(i);
            ++current_iterator;
        }

        //add entries
        for (i = 0; i < count; ++i) {
            StaticCoreInfo pitem = plist[i];

            current_modifiablelist->Insert(i, new UiObjectContainer({
                { "Name", StringExposer::CreateStatic(trekClient.CfgGetCoreName(pitem.cbIGCFile)) },
                { "Bitmask", TypeExposer<uint32>::Create(1 << i) },
                { "StaticCoreInfo", TypeExposer<StaticCoreInfo>::Create(pitem) }
            }));
        }
    }

    void SetServerList(int count, ServerCoreInfo* plist) {
        std::list<TRef<UiObjectContainer>> result;
        auto current_modifiablelist = Get<TRef<UiList<TRef<UiObjectContainer>>>>("Server list");
        auto current_list = current_modifiablelist->GetList();

        auto current_iterator = current_list.begin();
        int i = 0;

        //remove entries
        while (current_iterator != current_list.end()) {
            current_modifiablelist->Remove(i);
            ++current_iterator;
        }

        //add entries
        for (i = 0; i < count; ++i) {
            ServerCoreInfo pitem = plist[i];

            uint32 bitmaskServer = (uint32)pitem.dwCoreMask;

            current_modifiablelist->Insert(i, new UiObjectContainer({
                { "Name", StringExposer::CreateStatic(pitem.szName) },
                { "ServerCoreInfo", TypeExposer<ServerCoreInfo>::Create(pitem) }
            }));
        }
    }
};

class LoginHelper : public IClientEventSink {
private:
    TRef<UiStateModifiableValue> m_state;
    TRef<IClientEventSink>      m_pClientEventSink;
    TRef<IEventSink>            m_pServerlistChangedSink;

    TRef<CreateGameDialogPopup> m_pcreateDialog;

    std::shared_ptr<DownloadHandle> m_pConfigDownload;

    static const int STEP_CONFIG_DOWNLOAD = 1;
    static const int STEP_CONFIG_READ = 2;
    static const int STEP_LOBBY_LOGIN = 3;
    static const int STEP_LOBBY_MISSIONLIST = 4;

public:
    LoginHelper() :
        m_state(new UiStateModifiableValue(std::make_shared<LoggedOutState>(new UiStateValue(std::make_shared<NoErrorState>()), GetLoginSink())))
    {

    }

    ~LoginHelper() {
        if (m_pClientEventSink && trekClient.GetClientEventSource()) {
            trekClient.GetClientEventSource()->RemoveSink(m_pClientEventSink);
        }
        if (m_pServerlistChangedSink) {
            trekClient.GetMissionList()->GetChangedEvent()->RemoveSink(m_pServerlistChangedSink);
        }
        trekClient.DisconnectLobby();
    }

    TRef<IEventSink> GetLoginSink() {
        return new CallbackSink([this]() {
            this->Login();
            return true;
        });
    }

    TRef<IEventSink> GetLogoutSink() {
        return new CallbackSink([this]() {
            this->Logout();
            return true;
        });
    }

    TRef<UiStateValue> GetState() {
        return m_state;
    }

    void SetLoginError(std::string message) {
        debugf(("Introscreen login error: " + message).c_str());
        trekClient.GetClientEventSource()->RemoveSink(m_pClientEventSink);
        m_pClientEventSink = nullptr;
        m_state->SetValue(std::make_shared<LoggedOutState>(new UiStateValue(std::make_shared<YesErrorState>(message)), this->GetLoginSink()));
    }

    void Async(std::function<void()> callback) {
        //the callback needs to stay alive, copy into pointer
        std::function<void()>* pCallback = new std::function<void()>(callback);

        CreateThread(NULL, 0, [](void* pData) {
            std::function<void()> callback = *(std::function<void()>*)pData;

            callback();

            //delete pointer
            delete pData;
            return (DWORD)0;
        }, pCallback, 0, NULL);
    }

    void Logout() {
        if (m_state->GetValue()->GetName() != "Logged in") {
            ZAssert(false);
            return;
        }

        trekClient.DisconnectLobby();

        if (m_pClientEventSink && trekClient.GetClientEventSource()) {
            trekClient.GetClientEventSource()->RemoveSink(m_pClientEventSink);
            m_pClientEventSink = nullptr;
        }

        if (m_pServerlistChangedSink) {
            trekClient.GetMissionList()->GetChangedEvent()->RemoveSink(m_pServerlistChangedSink);
            m_pServerlistChangedSink = nullptr;
        }

        m_state->SetValue(std::make_shared<LoggedOutState>(new UiStateValue(std::make_shared<NoErrorState>()), this->GetLoginSink()));
    }

    void Login() {
        if (m_state->GetValue()->GetName() != "Logged out") {
            ZAssert(false);
            return;
        }

        m_pClientEventSink = IClientEventSink::CreateDelegate(this);
        trekClient.GetClientEventSource()->AddSink(m_pClientEventSink);

        m_state->SetValue(std::make_shared<LoggingInState>());
        m_state->GetValue()->as<LoggingInState>()->SetStep(STEP_CONFIG_DOWNLOAD, "Downloading configuration file");

        ZString name = trekClient.GetSavedCharacterName();

        // BT - Steam - User is logged into steam, and has a steam profile name
        // The steam reviewer was somehow launching the game with steam authorization but no persona name. If 
        // there is an player name, then the server rejects the user as a hacker with a DPlay error. 
        bool isUserLoggedIntoSteamWithValidPlayerName = SteamUser() != nullptr && strlen(name) > 0;

        if (isUserLoggedIntoSteamWithValidPlayerName == false)
        {
            SetLoginError("Invalid user supplied by Steam");
            return;
        }

        // BT - STEAM - Add players callsign and token.
        CallsignTagInfo callSignTagInfo;
        ZString characterName = callSignTagInfo.Render(name);

        std::string configPath = GetConfiguration()->GetStringValue(
            "Online.ConfigFile", 
            GetConfiguration()->GetStringValue("CfgFile", "http://allegiance.zaphop.com/allegiance.txt")
        );

        if (ZString(configPath.c_str()).Find("http://") == -1)
        {
            StartConfigRead(characterName, configPath.c_str(), "Using local lobby configuration. Reading lobby configuration");
        }
        else {
            m_pConfigDownload = Downloader().Download(configPath);
            m_pConfigDownload->AddCallback([this, characterName]() {
                try {
                    PathString pathLocalConfig(m_pConfigDownload->GetResultFilePath().c_str());
                    StartConfigRead(characterName, pathLocalConfig, "Downloaded lobby configuration. Reading lobby configuration");
                }
                catch (const DownloadException& e) {
                    SetLoginError(std::string("Configuration: ") + e.what());
                }
            });
        }
    }

    void StartConfigRead(ZString characterName, PathString pathLocalConfig, std::string strStepMessage) {
        m_state->GetValue()->as<LoggingInState>()->SetStep(STEP_CONFIG_READ, strStepMessage);
        Async([this, characterName, pathLocalConfig]() {
            trekClient.GetCfgInfo().Load(pathLocalConfig);

            BaseClient::ConnectInfo ci;
            DWORD cbZoneTicket = 0;
            ci.pZoneTicket = NULL;
            DWORD cbName = sizeof(ci.szName);

            lstrcpy(ci.szName, characterName);

            ZeroMemory(&ci.ftLastArtUpdate, sizeof(ci.ftLastArtUpdate));

            m_state->GetValue()->as<LoggingInState>()->SetStep(STEP_LOBBY_LOGIN, "Connecting to the lobby");
            HRESULT success = ((BaseClient*)&trekClient)->ConnectToLobby(&ci);
            if (!trekClient.m_fmLobby.IsConnected()) {
                SetLoginError("Failed to connect to the lobby.");
                return;
            }
            m_state->GetValue()->as<LoggingInState>()->SetStep(STEP_LOBBY_MISSIONLIST, "Connected to the lobby. Waiting for the server list.");
        });
    }

    void OnLostConnection(const char * szReason) override {};
    void OnLogonLobby() override {
        if (m_state->GetValue()->GetName() != "Logging in") {
            ZAssert(false);
            return;
        }
        m_state->GetValue()->as<LoggingInState>()->SetStep(3, "Logged in to the lobby. Waiting for server/core list");

        Async([this]() {
            trekClient.ServerListReq();
            if (!GetWindow()->GetPopupContainer()->IsEmpty())
                GetWindow()->GetPopupContainer()->ClosePopup(NULL);
            GetWindow()->RestoreCursor();
        });
    };
    void OnLogonLobbyFailed(bool bRetry, const char * szReason) override {
        SetLoginError(std::string("Lobby login failed: ") + std::string(szReason));
    };
    void OnLogonGameServer() override {};
    void OnLogonGameServerFailed(bool bRetry, const char * szReason) override {};

    /*int m_cCores;
    StaticCoreInfo* m_pcores;
    int m_cServers;
    ServerCoreInfo* m_pservers;*/

    void OnServersList(int cCores, char *Cores, int cServers, char *Servers) override {
        //prepare the new game dialog

        if (!m_pcreateDialog) {
            Modeler* pmodeler = GetModeler();
            //export to mdl
            TRef<INameSpace> pnsGameScreen = pmodeler->CreateNameSpace("gamescreendata", pmodeler->GetNameSpace("effect"));
            pnsGameScreen->AddMember("playerCount", new ModifiableNumber(0));

            TRef<INameSpace> pns = pmodeler->GetNameSpace("gamescreen");

            m_pcreateDialog = new CreateGameDialogPopup(pns);
        }

        StaticCoreInfo *pcores = (StaticCoreInfo*)Cores;
        ServerCoreInfo *pservers = (ServerCoreInfo*)Servers;

        m_pcreateDialog->OnServersList(cCores, pcores, cServers, pservers);

        //finish login process
        if (m_pServerlistChangedSink) {
            //we are already connected?
            ZAssert(false);
        }
        m_pServerlistChangedSink = new CallbackSink([this]() {
            if (m_state->GetValue()->GetName() != "Logged in") {
                ZAssert(false);
                return false;
            }
            m_state->GetValue()->as<LoggedInState>()->SetMissionList(trekClient.GetMissionList());
            return true;
        });

        List* plist = trekClient.GetMissionList();
        plist->GetChangedEvent()->AddSink(m_pServerlistChangedSink);

        m_state->SetValue(std::make_shared<LoggedInState>(this->GetLogoutSink(), this->GetShowNewGamePopupSink()));
        m_state->GetValue()->as<LoggedInState>()->SetMissionList(plist);
        m_state->GetValue()->as<LoggedInState>()->SetCoreList(cCores, pcores);
        m_state->GetValue()->as<LoggedInState>()->SetServerList(cServers, pservers);
    };

    TRef<IEventSink> GetShowNewGamePopupSink() {
        return new CallbackSink([this]() {
            if (!GetWindow()->GetPopupContainer()->IsEmpty())
                GetWindow()->GetPopupContainer()->ClosePopup(NULL);
            GetWindow()->RestoreCursor();

            GetWindow()->GetPopupContainer()->OpenPopup(m_pcreateDialog, false);
            return true;
        });
    }

};

//////////////////////////////////////////////////////////////////////////////
//
// Intro Screen
//
//////////////////////////////////////////////////////////////////////////////

class IntroScreen :
    public Screen,
    public TrekClientEventSink,
    public EventTargetContainer<IntroScreen>,
    public LogonSite,
    public PasswordDialogSink
{
private:
    bool                m_bUseOldUi;

    TRef<Modeler>       m_pmodeler;
    UiEngine&           m_uiEngine;
    TRef<Pane>          m_ppane;
    TRef<Image>         m_pimage;

	TRef<ButtonPane>	m_pbuttonDiscord;
    TRef<ButtonPane>    m_pbuttonPlayLan;
    TRef<ButtonPane>    m_pbuttonPlayInt;
#ifdef USEAZ
    TRef<ButtonPane>    m_pbuttonZoneClub;
#endif
    //TRef<ButtonPane> m_pbuttonTraining;
    TRef<ButtonPane>    m_pbuttonTrainingBig;
    TRef<ButtonPane>    m_pbuttonZoneWeb;
    TRef<ButtonPane>    m_pbuttonOptions;
    TRef<ButtonPane>    m_pbuttonIntro;
    TRef<ButtonPane>    m_pbuttonCredits;
    //TRef<ButtonPane> m_pbuttonQuickstart;
    TRef<ButtonPane>    m_pbuttonExit;
    TRef<ButtonPane>    m_pbuttonHelp;
    TRef<IMessageBox>   m_pMsgBox;
    GUID                m_guidSession;
    ZString             m_strCharacterName;
    ZString             m_strPassword;
    MissionInfo*        m_pmissionJoining;

    TRef<LoginHelper>   m_pLoginHelper;

#ifdef ENABLE3DINTROSCREEN
	//KGJV
	TRef<Number>            m_ptime;
	TRef<Pane>              m_ppaneGeo;
    TRef<Camera>            m_pcamera;
    TRef<Viewport>          m_pviewport;
    TRef<WrapGeo>           m_pwrapGeo;
    TRef<WrapImage>         m_pwrapImageGeo;
	TRef<WrapImage>		    m_pwrapImage;
	TRef<ThingGeo>          m_pthing;
#endif

    enum
    {
        hoverNone,
        hoverPlayLan,
        hoverPlayInt,
#ifdef USEAZ
        hoverZoneClub,
#endif        
        hoverTrain,
        hoverZoneWeb,
        hoverOptions,
        hoverIntro,
        hoverCredits,
        hoverQuickstart,
        hoverExit,
        hoverHelp,
        hoverDiscord
    };

    TRef<ModifiableNumber>  m_pnumberCurrentHover;

    friend class CreditsPopup;
    class CreditsPopup : public IPopup, public EventTargetContainer<CreditsPopup> 
    {
    private:
        TRef<Pane> m_ppane;
        TRef<ButtonPane> m_pbuttonClose;
        
        IntroScreen* m_pparent;

        TRef<IKeyboardInput> m_pkeyboardInputOldFocus;

    public:
        
        CreditsPopup(TRef<INameSpace> pns, IntroScreen* pparent, Number* ptime)
        {
            m_pparent = pparent;

            TRef<WrapNumber> pcreditsTime;

            CastTo(pcreditsTime,   pns->FindMember("creditsTime"));
            CastTo(m_ppane,        pns->FindMember("screen"));
            CastTo(m_pbuttonClose, pns->FindMember("closeButtonPane"));

            pcreditsTime->SetWrappedValue(NumberTransform::Subtract(ptime, new Number(ptime->GetValue())));

			AddEventTarget(&IntroScreen::CreditsPopup::OnButtonClose, m_pbuttonClose->GetEventSource());
        }

        //
        // IPopup methods
        //

        virtual void OnClose()
        {
            if (m_pkeyboardInputOldFocus)
                GetWindow()->SetFocus(m_pkeyboardInputOldFocus);

            m_pkeyboardInputOldFocus = NULL;

            IPopup::OnClose();
        }

        virtual void SetContainer(IPopupContainer* pcontainer)
        {
            // initialize the check boxes
            
            m_pkeyboardInputOldFocus = GetWindow()->GetFocus();
            
            IPopup::SetContainer(pcontainer);
        }

        Pane* GetPane()
        {
            return m_ppane;
        }

        bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
        {
            // we need to make sure we get OnChar calls to pass on to the edit box
            fForceTranslate = true;
            return false;
        }

        bool OnChar(IInputProvider* pprovider, const KeyState& ks)
        {
            return true;
        }

        bool OnButtonClose()
        {
            
            m_pcontainer->ClosePopup(this);
            
            return true;
        }
    };


    friend class FindServerPopup;
    class FindServerPopup : public IPopup, public EventTargetContainer<FindServerPopup>, public IItemEvent::Sink
    {
    private:
        TRef<Pane> m_ppane;
        //TRef<ButtonPane>    m_pbuttonFind;
        TRef<ButtonPane>    m_pbuttonJoin;
        TRef<ButtonPane>    m_pbuttonCancel;
        //TRef<EditPane>      m_peditPane;
        TRef<ListPane>      m_plistPane;

        TRef<IItemEvent::Source>   m_peventServerList;
        TRef<TEvent<ItemID>::Sink> m_psinkServerList;

        IntroScreen* m_pparent;
        //TRef<IKeyboardInput> m_pkeyboardInputOldFocus;
        bool m_bDoBackgroundPolling;
        TRef<LANServerInfo> m_pserverSearching;

        TRef<IMessageBox> m_pmsgBox;

        typedef TListListWrapper<TRef<LANServerInfo> > ServerList;
        TRef<ServerList> m_plistServers;

        class ServerItemPainter : public ItemPainter
        {
            int m_nWidth;
            int m_nGameNameX;

        public:

            ServerItemPainter(int nGameNameX, int nWidth)
                : m_nWidth(nWidth), m_nGameNameX(nGameNameX) {};

            int GetXSize()
            {
                return m_nWidth;
            }

            int GetYSize()
            {
                return 14;
            }

            void Paint(ItemID pitemArg, Surface* psurface, bool bSelected, bool bFocus)
            {
                LANServerInfo* serverInfo = (LANServerInfo*)pitemArg;
                const int nPlayerWidth = 40;
                
                if (bSelected) {
                    psurface->FillRect(
                        WinRect(0, 0, GetXSize(), GetYSize()),
                        Color(1, 0, 0)
                    );
                }

                TRef<IEngineFont> pfont = TrekResources::SmallFont();
                Color             color = Color::White();

                WinRect rectClipOld = psurface->GetClipRect();
                psurface->SetClipRect(WinRect(WinPoint(0, 0), WinPoint(GetXSize() - nPlayerWidth, GetYSize()))); // clip name to fit in row
                psurface->DrawString(pfont, color, WinPoint(2, 0), serverInfo->strGameName);
                psurface->RestoreClipRect(rectClipOld);

                if (serverInfo->nMaxPlayers != 0)
                {
                    char cbPlayers[20];
                    wsprintf(cbPlayers, "(%d/%d)", serverInfo->nNumPlayers, serverInfo->nMaxPlayers);
                    psurface->DrawString(pfont, color, WinPoint(GetXSize() - nPlayerWidth + 2, 0), cbPlayers);
                }
            }
        };

        static bool GameNameCompare(ItemID pitem1, ItemID pitem2)
        {
            LANServerInfo* serverInfo1 = (LANServerInfo*)pitem1;
            LANServerInfo* serverInfo2 = (LANServerInfo*)pitem2;

            return _stricmp(serverInfo1->strGameName, serverInfo2->strGameName) > 0;
        }

    public:
        
        FindServerPopup(TRef<INameSpace> pns, IntroScreen* pparent)
        {
            m_bDoBackgroundPolling = false;
            m_pparent = pparent;
            Value* pGameNameX;
            Value* pListXSize;

            CastTo(m_ppane,                 pns->FindMember("ServerDialog"));

            //CastTo(m_pbuttonFind,           pns->FindMember("serverFindButtonPane"));
            CastTo(m_pbuttonJoin,           pns->FindMember("serverJoinButtonPane"));
            CastTo(m_pbuttonCancel,         pns->FindMember("serverCancelButtonPane"));
            //CastTo(m_peditPane,      (Pane*)pns->FindMember("serverEditPane"));
            CastTo(m_plistPane,      (Pane*)pns->FindMember("serverListPane"));
            CastTo(pGameNameX,              pns->FindMember("serverListGameNameX"));
            CastTo(pListXSize,              pns->FindMember("serverListWidth"));

            //AddEventTarget(OnButtonFind, m_pbuttonFind->GetEventSource());
			AddEventTarget(&IntroScreen::FindServerPopup::OnButtonJoin, m_pbuttonJoin->GetEventSource());
            AddEventTarget(&IntroScreen::FindServerPopup::OnButtonCancel, m_pbuttonCancel->GetEventSource());
            AddEventTarget(&IntroScreen::FindServerPopup::OnButtonJoin, m_plistPane->GetDoubleClickEventSource());

            m_peventServerList = m_plistPane->GetSelectionEventSource();
            m_peventServerList->AddSink(m_psinkServerList = new IItemEvent::Delegate(this));
            m_plistPane->NeedLayout();
            m_plistPane->SetItemPainter(new ServerItemPainter(GetNumber(pGameNameX), GetNumber(pListXSize)));
            m_plistServers = new ServerList();
            m_plistPane->SetList(new SortedList<ItemIDCompareFunction>(m_plistServers, GameNameCompare));

            m_pbuttonJoin->SetEnabled(false);

            m_pserverSearching = new LANServerInfo(GUID_NULL, "Searching...", 0, 0);

            AddEventTarget(&IntroScreen::FindServerPopup::PollForServers, GetWindow(), 1.0f);
        }

        ~FindServerPopup()
        {
            m_peventServerList->RemoveSink(m_psinkServerList);
        }

        //
        // IPopup methods
        //

        Pane* GetPane()
        {
            return m_ppane;
        }

        virtual void OnClose()
        {
            //if (m_pkeyboardInputOldFocus)
            //    GetWindow()->SetFocus(m_pkeyboardInputOldFocus);

            //m_pkeyboardInputOldFocus = NULL;
            m_bDoBackgroundPolling = false;

            IPopup::OnClose();
        }

        virtual void SetContainer(IPopupContainer* pcontainer)
        {
            //m_pkeyboardInputOldFocus = GetWindow()->GetFocus();
            //GetWindow()->SetFocus(m_peditPane);
            m_bDoBackgroundPolling = true;

            FindGames("");
            IPopup::SetContainer(pcontainer);
        }

        bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
        {
            // we need to make sure we get OnChar calls to pass on to the edit box
            fForceTranslate = true;
            return false;
        }
        /*
        bool OnChar(IInputProvider* pprovider, const KeyState& ks)
        {
            if (ks.vk == 13)
            {
                OnButtonFind();
                return true;
            }
            else
                return ((IKeyboardInput*)m_peditPane)->OnChar(pprovider, ks);
        }
        
        bool OnButtonFind()
        {
            FindGames(m_peditPane->GetString());

            return true;
        }
        */
        bool PollForServers()
        {
            if (m_bDoBackgroundPolling)
                FindGames(NULL);

            return true;
        }

        void FindGames(const char* szServerName)
        {
			// WLP 2005 - turned off the every second search to keep the mouse responding
			// added next line to turn off polling
			// TE: Commented now that enumeration is done asynchronously
			// mmf: LAN mode is still sync so leave this in, otherwise GUI hangs on LAN game select screen
			m_bDoBackgroundPolling = false ;

            TList<TRef<LANServerInfo> > listResults;

            trekClient.FindStandaloneServersByName(szServerName, listResults);


            // WLP 2005 - DPLAY8 upgrade addition code
            // DPLAY8 EnumHosts returns duplicates in the list
            // so we have to strip them out
            // How this works =
            // destroy all dupes to the first entry
            // destroy all dupes to the second entry - etc.
            // just keep at it until the end
            
		    TList<TRef<LANServerInfo> >::Iterator iterNew(listResults);   // Walking list 
		    for( int i = 1 ; i < listResults.GetCount() ; i++ ) // Walk the entire list 
		    {
		    iterNew.First();     // set on the first one
            for ( int j = 1 ; j < i ; j++, iterNew.Next() ) ;               // find the starting position
			GUID matchGUID = iterNew.Value()->guidSession ;               // this is the one to check for dupes
		
			iterNew.Next();             // bump to the next one to start comparing
            while (!iterNew.End())                                   // walk from here to the end checking dupes
                {
				    if (memcmp(&(iterNew.Value()->guidSession),      // compare GUID for dupes
				            &matchGUID, sizeof(GUID)) == 0)
	    	        iterNew.Remove();
			        else iterNew.Next();
		        }
		    }

            // WLP - 2005 end of add for DPLAY8 upgrade

            // add to or update the the server list with the new results
            // Note: we have to maintain some degree of consistancy to 
            // make sure scrolling an selection work, so unfortunately we
            // can't just copy.

            ServerList::Iterator iterOld(*m_plistServers);
            while (!iterOld.End())
            {
                bool bFound = false;
                ZString strOldName = iterOld.Value()->strGameName;

                for (TList<TRef<LANServerInfo> >::Iterator iterNew(listResults); 
                    !iterNew.End(); iterNew.Next())
                {
                    ZString strNewName = iterNew.Value()->strGameName;
                    if (memcmp(&(iterNew.Value()->guidSession), 
                            &(iterOld.Value()->guidSession), sizeof(GUID)) == 0
                        && iterNew.Value()->nMaxPlayers > 1
                        && iterNew.Value()->nMaxPlayers < 1000)
                    {
                        // found it - update it with the new info
                        bFound = true;
                        iterOld.Value()->strGameName = iterNew.Value()->strGameName;
                        iterNew.Remove();
                        break;
                    }
                }

                // didn't find it in the new list, so nuke it.
                if (!bFound)
                    iterOld.Remove();
                else
                    iterOld.Next();
            }

            // for anything else that's new, just add it to the end.
            for (TList<TRef<LANServerInfo> >::Iterator iterNew(listResults); 
                !iterNew.End(); iterNew.Next())
            {
                // don't show single player games or zone server games
                if (iterNew.Value()->nMaxPlayers > 1
                    && iterNew.Value()->nMaxPlayers < 1000)
                {
                    ZString strNewName = iterNew.Value()->strGameName;
                    m_plistServers->PushEnd(iterNew.Value());
                }
            }

            if (m_plistServers->IsEmpty())
            {
                // put "Searching..." in the list.
                m_plistServers->PushEnd(m_pserverSearching);
            }
        }

        bool OnButtonJoin()
        {
            //GetWindow()->SetFocus(m_pkeyboardInputOldFocus);
            //m_pkeyboardInputOldFocus = NULL;
            m_bDoBackgroundPolling = false;

            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }

            LANServerInfo* serverInfo = (LANServerInfo*)(m_plistPane->GetSelection());
            
            
            if (serverInfo != NULL && serverInfo != m_pserverSearching)
                m_pparent->OnPickServer(serverInfo->guidSession);

            return true;
        }

        bool OnButtonCancel()
        {
            //GetWindow()->SetFocus(m_pkeyboardInputOldFocus);
            //m_pkeyboardInputOldFocus = NULL;
            m_bDoBackgroundPolling = false;

            if (m_ppopupOwner) {
                m_ppopupOwner->ClosePopup(this);
            } else {
                m_pcontainer->ClosePopup(this);
            }

            return true;
        }

        bool OnEvent(IItemEvent::Source *pevent, ItemID pitem)
        {
            LANServerInfo* serverInfo = (LANServerInfo*)pitem;

            m_pbuttonJoin->SetEnabled(serverInfo != NULL && serverInfo != m_pserverSearching);

            return true;
        }
    };    

    TRef<CreditsPopup> m_pcreditsPopup;
    TRef<FindServerPopup> m_pfindServerPopup;

    class CloseTrainWarnSink : public IIntegerEventSink {
    public:
        IntroScreen* m_pScreen;
        TRef<ButtonPane> m_pDontAskMeAgain;

        CloseTrainWarnSink(IntroScreen* pScreen, ButtonPane* pDontAskMeAgain) 
            : m_pScreen (pScreen), m_pDontAskMeAgain (pDontAskMeAgain) {}

        bool OnEvent(IIntegerEventSource* pevent, int value)
        {
            if (value == IDOK) {
                m_pScreen->OnButtonTraining ();
            }
            
            if (m_pDontAskMeAgain->GetChecked()) {
                // here we mark things as if a training mission has been launched
                GetConfiguration()->SetBoolValue("Ui.ShowStartupTrainingSuggestion", false);
            }
            return false;
        }
    };

    class URLMessageSink : public IIntegerEventSink {
    public:
        ZString m_strURL;

        URLMessageSink(ZString strURL) :
            m_strURL(strURL)
        {
        }

        bool OnEvent(IIntegerEventSource* pevent, int value)
        {
            if (value == IDOK) {
                GetWindow()->ShowWebPage(m_strURL);
            }
            return false;
        }
    };

    bool CheckNetworkDrivers()
    {
        ZString strDriverURL;

        if (!CheckNetworkDevices(strDriverURL))
        {
            TRef<IMessageBox> pMsgBox = CreateMessageBox("The drivers of the "
                "network card you are using are known not to work with "
                "Microsoft Allegiance.  Please visit " + strDriverURL 
                + " to download updated drivers.  Press <OK> to go there now.",
                NULL, true, true);

            pMsgBox->GetEventSource()->AddSink(new URLMessageSink(strDriverURL));
            GetWindow()->GetPopupContainer()->OpenPopup(pMsgBox, false);

            return false;
        }        
        else
            return true;
    }

public:

    Image* GetImage()
    {
        return m_pimage;
    }

    IntroScreen(Modeler* pmodeler, UiEngine& uiEngine, bool bUseOldUi) :
        m_pmodeler(pmodeler),
        m_uiEngine(uiEngine),
        m_bUseOldUi(bUseOldUi)
    {
        if (m_bUseOldUi == false) {
            std::map<std::string, std::function<bool()>> listeners;
            listeners["open.training"] = [this]() {
                IntroScreen::OnButtonTraining();
                return true;
            };
            listeners["open.motd"] = [this]() {
                g_bAutomaticallySkipMotdScreen = false;
                GetWindow()->screen(ScreenIDZoneClubScreen);
                return true;
            };
            listeners["open.lobby"] = [pmodeler]() {
                g_bAutomaticallySkipMotdScreen = true;
                GetWindow()->screen(ScreenIDZoneClubScreen);
                return true;
            };
            listeners["open.website"] = [this]() {
                GetWindow()->ShowWebPage();
                return true;
            };
            listeners["open.discord"] = [this]() {
                IntroScreen::OnButtonDiscord();
                return true;
            };
            listeners["open.options"] = [this]() {
                GetWindow()->ShowOptionsMenu();
                return true;
            };
            listeners["open.lan"] = [this]() {
                IntroScreen::OnButtonLAN();
                return true;
            };
            listeners["open.intro"] = [this]() {
                IntroScreen::OnButtonIntro();
                return true;
            };
            listeners["open.credits"] = [this]() {
                IntroScreen::OnButtonCredits();
                return true;
            };
            listeners["open.help"] = [this]() {
                IntroScreen::OnButtonHelp();
                return true;
            };
            listeners["open.exit"] = [this]() {
                IntroScreen::OnButtonExit();
                return true;
            };

            std::map<std::string, std::shared_ptr<Exposer>> map;

            map["time"] = NumberExposer::Create(GetWindow()->GetTime());
            map["callsign"] = StringExposer::CreateStatic(trekClient.GetSavedCharacterName());

            /*TRef<UiStateModifiableValue> login_state = new UiStateModifiableValue(UiState("Logged out", UiObjectContainer({
                {"event.login", (TRef<IEventSink>)new CallbackSink([]() {
                    //auto screen = CreateZoneClubScreen();
                })}
            })));*/
            m_pLoginHelper = new LoginHelper();
            map["Login state"] = TypeExposer<TRef<UiStateValue>>::Create(m_pLoginHelper->GetState());

            m_pimage = m_uiEngine.LoadImageFromLua(UiScreenConfiguration::Create("menuintroscreen/introscreen.lua", listeners, map));
        }

        trekClient.DisconnectClub();


        TRef<INameSpace> pnsIntroScreen = m_pmodeler->CreateNameSpace("introscreendata", m_pmodeler->GetNameSpace("gamepanes"));
        pnsIntroScreen->AddMember("hoverNone", new Number(hoverNone));
        pnsIntroScreen->AddMember("hoverPlayLan", new Number(hoverPlayLan));
        pnsIntroScreen->AddMember("hoverPlayInt", new Number(hoverPlayInt));
#ifdef USEAZ
        pnsIntroScreen->AddMember("hoverZoneClub", new Number(hoverZoneClub));
#endif
        pnsIntroScreen->AddMember("hoverZoneWeb", new Number(hoverZoneWeb));
        pnsIntroScreen->AddMember("hoverTrain", new Number(hoverTrain));
        pnsIntroScreen->AddMember("hoverOptions", new Number(hoverOptions));
        pnsIntroScreen->AddMember("hoverIntro", new Number(hoverIntro));
        pnsIntroScreen->AddMember("hoverCredits", new Number(hoverCredits));
        pnsIntroScreen->AddMember("hoverQuickstart", new Number(hoverQuickstart));
        pnsIntroScreen->AddMember("hoverExit", new Number(hoverExit));
        pnsIntroScreen->AddMember("hoverHelp", new Number(hoverHelp));
        pnsIntroScreen->AddMember("hoverDiscord", new Number(hoverDiscord));

        pnsIntroScreen->AddMember("CurrentHover", m_pnumberCurrentHover = new ModifiableNumber(hoverNone));

#ifdef ENABLE3DINTROSCREEN
        //KGJV
        pnsIntroScreen->AddMember("bgImage", (Value*)(m_pwrapImage = new WrapImage(Image::GetEmpty())));
        m_pthing = NULL;
#endif
        TRef<INameSpace> pns = pmodeler->GetNameSpace("introscreen");
        if (m_bUseOldUi) {
            CastTo(m_ppane, pns->FindMember("screen"));
            CastTo(m_pbuttonPlayLan, pns->FindMember("playLanButtonPane"));
            CastTo(m_pbuttonPlayInt, pns->FindMember("playIntButtonPane"));
            CastTo(m_pbuttonDiscord, pns->FindMember("discordButtonPane"));

    #ifdef USEAZ
            CastTo(m_pbuttonZoneClub, pns->FindMember("zoneClubButtonPane"));
    #endif
            //CastTo(m_pbuttonTraining,   pns->FindMember("trainButtonPane"));
            CastTo(m_pbuttonTrainingBig, pns->FindMember("trainBigButtonPane"));
            CastTo(m_pbuttonZoneWeb, pns->FindMember("zoneWebButtonPane"));
            CastTo(m_pbuttonOptions, pns->FindMember("optionsButtonPane"));
            CastTo(m_pbuttonIntro, pns->FindMember("introButtonPane"));
            CastTo(m_pbuttonCredits, pns->FindMember("creditsButtonPane"));
            //CastTo(m_pbuttonQuickstart, pns->FindMember("quickstartButtonPane"));
            CastTo(m_pbuttonExit, pns->FindMember("exitButtonPane"));
            CastTo(m_pbuttonHelp, pns->FindMember("helpButtonPane"));


            //AddEventTarget(OnButtonGames,       m_pbuttonPlayLan->GetEventSource());
            //AddEventTarget(OnButtonTraining,    m_pbuttonTraining->GetEventSource());


            AddEventTarget(&IntroScreen::OnButtonDiscord, m_pbuttonDiscord->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonTraining, m_pbuttonTrainingBig->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonExit, m_pbuttonExit->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonHelp, m_pbuttonHelp->GetEventSource());
    #ifdef USEAZ
            AddEventTarget(OnButtonZoneClub, m_pbuttonZoneClub->GetEventSource());
    #endif
            AddEventTarget(&IntroScreen::OnButtonInternet, m_pbuttonPlayInt->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonLAN, m_pbuttonPlayLan->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonZoneWeb, m_pbuttonZoneWeb->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonOptions, m_pbuttonOptions->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonCredits, m_pbuttonCredits->GetEventSource());
            AddEventTarget(&IntroScreen::OnButtonIntro, m_pbuttonIntro->GetEventSource());


            AddEventTarget(&IntroScreen::OnHoverPlayLan, m_pbuttonPlayLan->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverPlayInt, m_pbuttonPlayInt->GetMouseEnterEventSource());
    #ifdef USEAZ
            AddEventTarget(OnHoverZoneClub, m_pbuttonZoneClub->GetMouseEnterEventSource());
    #endif
            //AddEventTarget(OnHoverTrain,        m_pbuttonTraining->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverTrain, m_pbuttonTrainingBig->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverZoneWeb, m_pbuttonZoneWeb->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverOptions, m_pbuttonOptions->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverIntro, m_pbuttonIntro->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverCredits, m_pbuttonCredits->GetMouseEnterEventSource());
            //AddEventTarget(OnHoverQuickstart,   m_pbuttonQuickstart->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverExit, m_pbuttonExit->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverHelp, m_pbuttonHelp->GetMouseEnterEventSource());
            AddEventTarget(&IntroScreen::OnHoverDiscord, m_pbuttonDiscord->GetMouseEnterEventSource());

            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonPlayLan->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonPlayInt->GetMouseLeaveEventSource());
    #ifdef USEAZ
            AddEventTarget(OnHoverNone, m_pbuttonZoneClub->GetMouseLeaveEventSource());
    #endif
            //AddEventTarget(OnHoverNone,     m_pbuttonTraining->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonTrainingBig->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonZoneWeb->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonOptions->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonIntro->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonCredits->GetMouseLeaveEventSource());
            //AddEventTarget(OnHoverNone,     m_pbuttonQuickstart->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonHelp->GetMouseLeaveEventSource());
            AddEventTarget(&IntroScreen::OnHoverNone, m_pbuttonExit->GetMouseLeaveEventSource());

            //m_pbuttonPlayLan->SetEnabled(false);
            //m_pbuttonPlayInt->SetEnabled(false);

            // BT - Steam - Hiding these irrelevant buttons for now.
            //m_pbuttonPlayLan->SetHidden(false);
        }

        m_pfindServerPopup = new FindServerPopup(pns, this);
          
        pmodeler->UnloadNameSpace(pns);
        
        trekClient.Disconnect();
        trekClient.DisconnectLobby();
        if (g_bQuickstart || g_bReloaded)
            AddEventTarget(&IntroScreen::OnQuickstart, GetWindow(), 0.01f);

        // we only do this once per execution, and only if training is installed
        static  bool    bHaveVisited = false;
        if (Training::IsInstalled () && !bHaveVisited)
        {
            // first time through, show a dialog explaining that users
            // really should do the training first
            if (GetConfiguration()->GetBoolValue("Ui.ShowStartupTrainingSuggestion", false == GetConfiguration()->GetBoolValue("HasTrained", false)))
            {
                TRef<IMessageBox>   pMsgBox = 
                    CreateMessageBox(
                        "ALLEGIANCE is a team-based massively-multiplayer space combat game, in which your goal is to conquer the universe.\n\nTraining is strongly recommended before going online, and will take approximately 30 minutes. Press the OK button to begin training now.",
                        NULL,
                        true,
                        true
                    );
                TRef<Surface> psurface = GetModeler()->LoadSurface("btndontaskmeagainbmp", true);
                TRef<ButtonPane> pDontAskMeAgain
                    = CreateTrekButton(CreateButtonFacePane(psurface, ButtonNormalCheckBox), true, mouseclickSound);
                pDontAskMeAgain->SetOffset(WinPoint(183, 140));
                pMsgBox->GetPane()->InsertAtBottom(pDontAskMeAgain);
                pMsgBox->GetEventSource()->AddSink(new CloseTrainWarnSink(this, pDontAskMeAgain));
                GetWindow()->GetPopupContainer()->OpenPopup (pMsgBox, false);
            }
        }

        if (g_bAskForCDKey && !bHaveVisited && trekClient.GetCDKey().IsEmpty())
        {
            TRef<IPopup> ppopupCDKey = CreateCDKeyPopup();
            GetWindow()->GetPopupContainer()->OpenPopup(ppopupCDKey, false);
        }

        // stop us from asking again during this execution
        bHaveVisited = true;

        trekClient.FlushSessionLostMessage();


#ifdef ENABLE3DINTROSCREEN
        // KGJV test
        if (m_bUseOldUi) {
            m_pwrapImage->SetImage((Image*)(Value *)pns->FindMember("bkImage"));
            m_ppaneGeo = CreateGeoPane(
                pns->FindPoint("geoSize"),
                pns->FindPoint("geoPosition")
            );
            m_ppaneGeo->SetOffset(pns->FindWinPoint("geoPosition"));
            m_ppane->InsertAtTop(m_ppaneGeo);

            TRef<StringValue> pstring; CastTo(pstring, pns->FindMember("geoModel"));
            TRef<INameSpace> pnsgeo = m_pmodeler->GetNameSpace(pstring->GetValue());

            if (pnsgeo) {
                m_pthing = ThingGeo::Create(m_pmodeler, m_ptime);
                m_pthing->LoadMDL(0, pnsgeo, NULL);
                m_pthing->SetShadeAlways(true);
                //m_pthing->SetTransparentObjects(true);
                //m_pthing->SetShowBounds(true);

                SetGeo(m_pthing->GetGeo());
            }

        }
#endif

    }

    virtual ~IntroScreen() 
    {
        m_pmodeler->UnloadNameSpace("introscreendata");
#ifdef ENABLE3DINTROSCREEN
		m_pthing->SetTransparentObjects(false);
		m_pthing->SetShowBounds(false);
#endif
    }


    bool OnQuickstart()
    {
#ifdef USEAZ //Imago 7/4/09 #78
        OnButtonZoneClub();
#else
		OnButtonInternet();
#endif
        return false;
    }

#ifdef ENABLE3DINTROSCREEN
	// KGJV
    void SetGeo(Geo* pgeo)
    {
        m_pwrapGeo->SetGeo(pgeo);

        float radius = pgeo->GetRadius(Matrix::GetIdentity());

        //m_scaleCone = 0.75f * radius;

        m_pcamera->SetPosition(
            Vector(
                0,
                0,
                2.4f * radius
            )
        );
    }

	// KGJV    
    TRef<Pane> CreateGeoPane(const Point& size, const Point& offset)
    {
        m_pcamera = new Camera();
        m_pcamera->SetZClip(1, 10000);
        m_pcamera->SetFOV(RadiansFromDegrees(50));

        Rect rect(Point(0, 0), size);
        TRef<RectValue> prect = new RectValue(rect);

        m_pviewport = new Viewport(m_pcamera, prect);

        m_pwrapGeo  = new WrapGeo(Geo::GetEmpty());
		m_pwrapImageGeo = new WrapImage(Image::GetEmpty());
		m_ptime = GetWindow()->GetTime();
        GeoImage* pgeo =
			new GeoImage(                
                    new TransformGeo(
                        new TransformGeo(
                            m_pwrapGeo,
                            new AnimateRotateTransform(
                                new VectorValue(Vector(0, 1, 0)),
                                NumberTransform::Multiply(m_ptime, new Number(1.0))
                            )
                        ),
                        new RotateTransform(Vector(1, 0, 0), pi/8)
                    ),
                    m_pviewport,
                    true
				);
        TRef<Image> pimage = 
			new GroupImage(
				m_pwrapImageGeo,
				pgeo,
				new TranslateImage(
                    m_pwrapImage,
                    Point(-offset.X(), -(600 - size.Y() - offset.Y()))    
                )
				);

        //
        // Give this guy a zbuffer
        //

        return
            new AnimatedImagePane(
                CreatePaneImage( 
                    GetEngine(),
                    false,
                    new AnimatedImagePane(
                        pimage,
                        rect
                    )
                )
            );
    }
#endif
    //////////////////////////////////////////////////////////////////////////////
    //
    // Event handlers
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnPickServer(GUID guidSession)
    {
        assert (!trekClient.LoggedOn());
        m_guidSession = guidSession;

        //
        // pop up the call sign/login dialog
        //

        TRef<IPopup> plogonPopup = CreateLogonPopup(m_pmodeler, this, LogonLAN, 
            "Enter a call sign to use for this game.", trekClient.GetSavedCharacterName(), "", false);
        GetWindow()->GetPopupContainer()->OpenPopup(plogonPopup, false);

        return true;
    }

	bool OnButtonDiscord()
	{
		// Gonna send them to NOAT for now, maybe put a better channel here later?
		GetWindow()->ShowWebPage("https://discord.gg/WcEJ9VH");
		return true;
	}

	class OpenWikiSink : public IIntegerEventSink {
	public:
		TrekWindow* m_pwindow;

		OpenWikiSink(TrekWindow* pwindow) :
			m_pwindow(pwindow)
		{
		}

		bool OnEvent(IIntegerEventSource* pevent, int value)
		{
            if (value == IDOK)
                m_pwindow->screen(ScreenIDTrainScreen);
				//m_pwindow->ShowWebPage("http://www.freeallegiance.org/FAW/index.php/Quick_Crash_Course"); - not anymore - LANS
			
			m_pwindow->screen(ScreenIDTrainScreen);

			return false;
		}
	};

	TRef<IMessageBox> m_pmessageBox;

	//void StartClose()
	//{
	//	if (m_pmessageBox == NULL) {
	//		m_pmessageBox = CreateMessageBox("Quit Allegiance?", NULL, true, true);
	//		m_pmessageBox->GetEventSource()->AddSink(new CloseSink(this));
	//		GetPopupContainer()->OpenPopup(m_pmessageBox, false);
	//		m_ptrekInput->SetFocus(false);
	//	}

	//	//
	//	// Make sure the window isn't minimized
	//	//

	//	if (!GetFullscreen()) {
	//		OnCaptionRestore();
	//	}
	//}

    bool OnButtonTraining()
    {
        if (Training::IsInstalled ())
        {
            // go train, because we have all the training files installed
            trekClient.SetIsLobbied(false);
            trekClient.SetIsZoneClub(false);
            
			m_pmessageBox = CreateMessageBox("Welcome to Allegiance training! In addition to these training missions, you can push F1 at any time to see a quick - reference keymap. We also have a wiki and discord, which are linked from the F1 screen", NULL, true, false);
			m_pmessageBox->GetEventSource()->AddSink(new OpenWikiSink(GetWindow()));
			GetWindow()->GetPopupContainer()->OpenPopup(m_pmessageBox, false);
		
			/*if (trekClient.bTrainingFirstClick)
				GetWindow()->screen(ScreenIDTrainScreen);
			else
			{
				GetWindow()->ShowWebPage("http://www.freeallegiance.org/FAW/index.php/Quick_Crash_Course");
				trekClient.bTrainingFirstClick = true;
			}*/
        }
        else // wlp 2006 - go straight to lobby
        {
            // otherwise give the player the option to download the training files
            // if they do, then go to the download, and restart allegiance
            // otherwise don't do anything
            m_pMsgBox = CreateMessageBox ("TRAINING IS NOT INSTALLED.\n\nClick 'OK' to open the Allegiance website, then follow the directions there to download and install training.", NULL, false);
            TRef<Surface>       psurfaceOKButton = GetModeler ()->LoadImage ("btnokbmp", true)->GetSurface();
            TRef<Surface>       psurfaceAbortButton = GetModeler ()->LoadImage ("btnabortbmp", true)->GetSurface();
            TRef<ButtonPane>    pOKButton = CreateButton ( CreateButtonFacePane(psurfaceOKButton, ButtonNormal, 0, psurfaceOKButton->GetSize().X()), false, 0, 1.0);
            TRef<ButtonPane>    pAbortButton = CreateButton ( CreateButtonFacePane(psurfaceAbortButton, ButtonNormal, 0, psurfaceAbortButton->GetSize().X()), false, 0, 1.0);
            pOKButton->SetOffset (WinPoint(100, 170));
            pAbortButton->SetOffset (WinPoint(300, 170));
            m_pMsgBox->GetPane ()->InsertAtBottom (pOKButton);
            m_pMsgBox->GetPane ()->InsertAtBottom (pAbortButton);
            AddEventTarget(&IntroScreen::OnButtonBailTraining, pAbortButton->GetEventSource());
            GetWindow()->GetPopupContainer()->OpenPopup (m_pMsgBox, false);
        }
        return true;
    }

    bool    OnButtonBailTraining ()
    {
        GetWindow ()->GetPopupContainer ()->ClosePopup (m_pMsgBox);
        return true;
    }

    void OnLogon(const ZString& strName, const ZString& strPassword, BOOL fRememberPW)
    {
        // remember the character name for next time
        trekClient.SaveCharacterName(strName);
        m_strCharacterName = strName;
        m_strPassword = "";

        // pop up the Connecting... dialog.   
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Connecting...", NULL, false);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

        // pause to let the "connecting..." box draw itself
        AddEventTarget(&IntroScreen::OnTryLogon, GetWindow(), 0.1f);
    }

    bool OnTryLogon()
    {
        trekClient.SetIsLobbied(false);
        trekClient.SetIsZoneClub(false);

        BaseClient::ConnectInfo ci;
        ci.guidSession = m_guidSession;
        strcpy(ci.szName, m_strCharacterName);        
        assert(strlen(m_strPassword) < c_cbGamePassword);

        trekClient.ConnectToServer(ci, NA, Time::Now(), m_strPassword, true);

        return false;
    }

    void OnLogonGameServer()
    {
        // wait for a join message.
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox = 
            CreateMessageBox("Joining mission....", NULL, false, false);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
    }

    void OnLogonGameServerFailed(bool bRetry, const char* szReason)
    {
        if (bRetry)
        {
            // pop up the call sign/login dialog
            TRef<IPopup> plogonPopup = CreateLogonPopup(m_pmodeler, this, LogonLAN, 
                szReason, trekClient.GetSavedCharacterName(), "", false);
            GetWindow()->GetPopupContainer()->OpenPopup(plogonPopup, false);
        }
        else
        {
            // tell the user why the logon failed
            TRef<IMessageBox> pmsgBox = CreateMessageBox(szReason);
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
    }

    void OnAddPlayer(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo)
    {
        if (pMissionInfo && pPlayerInfo == trekClient.MyPlayerInfo())
        {
            // go to the team lobby...
            GetWindow()->screen(ScreenIDTeamScreen);
        }
    }

    void OnDelRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason)
    {
        if (pPlayerInfo == trekClient.MyPlayerInfo())
        {
            m_pmissionJoining = pMissionInfo;

            // rejected !!  Bad password?
            if (!GetWindow()->GetPopupContainer()->IsEmpty())
                GetWindow()->GetPopupContainer()->ClosePopup(NULL);
            GetWindow()->RestoreCursor();

            if (reason == DPR_BadPassword)
            {
                TRef<IPopup> ppopupPassword = CreatePasswordPopup(this, m_strPassword);
                GetWindow()->GetPopupContainer()->OpenPopup(ppopupPassword, false);
            }
            else if (reason == DPR_NoMission)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The mission you were trying to join has ended.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else if (reason == DPR_LobbyLocked)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The lobby for this mission is locked.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else if (reason == DPR_OutOfLives)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("You have run out of lives in this mission.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else if (reason == DPR_Banned)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("You have been removed (i.e. booted) from this game by the commander(s)."); // mmf 09/07 was "You have been banned from this mission."
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else if (reason == DPR_GameFull)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("This server has reached its maximum number of players.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else if (reason == DPR_PrivateGame)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The game you tried to join is private.  Your login id does not have access to it.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else if (reason == DPR_DuplicateLogin)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("Someone else is already using that name in this game.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else if (reason == DPR_ServerPaused)
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("The server for this game is shutting down and is not accepting new users.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
            else
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox("You have not been accepted into the game.");
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                trekClient.Disconnect();
            }
        }
    }

    virtual void OnCancelPassword()
    {
        trekClient.Disconnect();
    }

    virtual void OnPassword(ZString strPassword)
    {
        // try the new password
        m_strPassword = strPassword;

        // pop up the Connecting... dialog.   
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Connecting...", NULL, false);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

        // pause to let the "connecting..." box draw itself
        AddEventTarget(&IntroScreen::OnTryLogon, GetWindow(), 0.1f);
    }

    void OnAbort()
    {
        // user aborted logging on; we don't need to do anything
    };

    bool OnButtonExit()
    {
        GetWindow()->StartClose();

        return true;
    }

    bool OnButtonHelp()
    {
        GetWindow()->OnHelp (true);
        return true;
    }

    bool OnButtonZoneClub()
    {
        if (CheckNetworkDrivers())
        {
            trekClient.SetIsZoneClub(true);
            GetWindow()->screen(ScreenIDZoneClubScreen);
        }
        return true;
    }

    bool OnButtonInternet()
    {
        if (CheckNetworkDrivers())
        {
            trekClient.SetIsZoneClub(false);
            GetWindow()->screen(ScreenIDZoneClubScreen);
        }
        return true;
    }

    bool OnButtonLAN()
    {
        if (CheckNetworkDrivers())
        {
            GetWindow()->GetPopupContainer()->OpenPopup(m_pfindServerPopup, false);
        }
        return true;
    }

    bool OnButtonZoneWeb()
    {
        GetWindow()->ShowWebPage();
        return true;
    }

    bool OnButtonOptions()
    {
        GetWindow()->ShowOptionsMenu();
        return true;
    }

    bool OnButtonCredits()
    {
		// BUILD_DX9
		GetModeler()->SetColorKeyHint( true );
		// BUILD_DX9

        TRef<INameSpace> pnsCredits = GetModeler()->GetNameSpace("creditspane");
        m_pcreditsPopup = new CreditsPopup(pnsCredits, this, GetWindow()->GetTime());
        GetWindow()->GetPopupContainer()->OpenPopup(m_pcreditsPopup, false);
        return true;
    }

    bool OnButtonIntro()
    {
        GetWindow()->screen(ScreenIDSplashScreen);
        return true;
    }

    bool OnHoverPlayLan()
    {
		debugf("************ %s\n",m_pnumberCurrentHover->GetString(0));
        m_pnumberCurrentHover->SetValue(hoverPlayLan);
        return true;
    }

    bool OnHoverPlayInt()
    {
        m_pnumberCurrentHover->SetValue(hoverPlayInt);
        return true;
    }

#ifdef USEAZ
    bool OnHoverZoneClub()
    {
        m_pnumberCurrentHover->SetValue(hoverZoneClub);
        return true;
    }
#endif

    bool OnHoverTrain()
    {
        m_pnumberCurrentHover->SetValue(hoverTrain);
        return true;
    }

    bool OnHoverZoneWeb()
    {
        m_pnumberCurrentHover->SetValue(hoverZoneWeb);
        return true;
    }

    bool OnHoverOptions()
    {
        m_pnumberCurrentHover->SetValue(hoverOptions);
        return true;
    }

    bool OnHoverIntro()
    {
        m_pnumberCurrentHover->SetValue(hoverIntro);
        return true;
    }

    bool OnHoverCredits()
    {
        m_pnumberCurrentHover->SetValue(hoverCredits);
        return true;
    }

    bool OnHoverQuickstart()
    {
        m_pnumberCurrentHover->SetValue(hoverQuickstart);
        return true;
    }

    bool OnHoverHelp()
    {
        m_pnumberCurrentHover->SetValue(hoverHelp);
        return true;
    }

    bool OnHoverDiscord()
    {
        m_pnumberCurrentHover->SetValue(hoverDiscord);
        return true;
    }

    bool OnHoverExit()
    {
        m_pnumberCurrentHover->SetValue(hoverExit);
        return true;
    }
    
    bool OnHoverNone()
    {
        m_pnumberCurrentHover->SetValue(hoverNone);
        return true;
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
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateIntroScreen(Modeler* pmodeler, UiEngine& uiEngine, bool bUseOldUi)
{
    return new IntroScreen(pmodeler, uiEngine, bUseOldUi);
}


