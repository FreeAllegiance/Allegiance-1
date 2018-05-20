
#include "ui.h"
#include "items.hpp"

#include "ns_math.hpp"
#include "ns_string.hpp"
#include "ns_color.hpp"
#include "ns_event.hpp"
#include "ns_image.hpp"
#include "ns_file.hpp"
#include "ns_font.hpp"
#include "ns_point.hpp"
#include "ns_screen.hpp"
#include "ns_list.hpp"

#include <stdexcept>
#include <fstream>

std::string UiEngine::m_stringLogPath = "";

void WriteLog(const std::string &text)
{
    if (UiEngine::m_stringLogPath == "") {
        return;
    }
    std::ofstream log_file(UiEngine::m_stringLogPath, std::ios_base::out | std::ios_base::app);
    log_file << text << std::endl;
}

std::shared_ptr<IFileLoader> UiEngine::m_fileLoader;
void UiEngine::SetGlobalFileLoader(const std::shared_ptr<IFileLoader>& pLoader)
{
    m_fileLoader = pLoader;
}

class UiScreenConfigurationImpl : public UiScreenConfiguration {

    std::string m_strPath;

public:
    UiScreenConfigurationImpl(std::string path, std::map<std::string, std::shared_ptr<Exposer>> map) :
        UiScreenConfiguration(map)
    {
        m_strPath = path;
    }

    std::string GetPath() {
        return m_strPath;
    }

    IEventSink& GetEventSink(std::string name) {
        return *UiObjectContainer::Get<TRef<IEventSink>>(name);
    }

};

std::shared_ptr<UiScreenConfiguration> UiScreenConfiguration::Create(std::string path, std::map<std::string, std::function<bool()>> event_listeners, std::map<std::string, std::shared_ptr<Exposer>> map) {
    
    std::for_each(event_listeners.begin(), event_listeners.end(),
        [&map](auto& p) {
        std::shared_ptr<Exposer> tmp = std::shared_ptr<Exposer>(new TypeExposer<TRef<IEventSink>>(new CallbackSink(p.second)));
        map[p.first] = tmp;
    });

    return std::make_shared<UiScreenConfigurationImpl>(path, map);
}

Loader::Loader(sol::state& lua, Engine* pEngine, const std::shared_ptr<IFileLoader>& pFileLoader)
    : m_pFileLoader(pFileLoader)
{
    m_pLua = &lua;        
}

Loader::~Loader() {
}

void Loader::InitNamespaces(LuaScriptContext& context) {
    auto pLua = &context.GetLua();

    pLua->open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math);
    pLua->set("loadfile", [](std::string path) {
        throw std::runtime_error("The standard function loadfile is disabled, use File.LoadLua(string)");
    });
    pLua->set("load", []() {
        throw std::runtime_error("The standard function load is disabled, what would you use this for?");
    });

    ImageNamespace::AddNamespace(context);
    EventNamespace::AddNamespace(pLua);

    NumberNamespace::AddNamespace(pLua);
    RectNamespace::AddNamespace(pLua);
    PointNamespace::AddNamespace(pLua);
    BooleanNamespace::AddNamespace(pLua);

    StringNamespace::AddNamespace(context);

    ColorNamespace::AddNamespace(pLua);

    FileNamespace::AddNamespace(pLua, this);

    FontNamespace::AddNamespace(pLua);

    ScreenNamespace::AddNamespace(context);

    ListNamespace::AddNamespace(context);

    pLua->new_usertype<MouseEventImage>("MouseEventImage",
        sol::base_classes, sol::bases<Image>()
        );
}

sol::function Loader::LoadScript(std::string subpath) {
    if (m_pFileLoader->HasFile(subpath.c_str()) == false) {
        throw std::runtime_error("File not found: " + subpath);
    }

    ZString path = m_pFileLoader->GetFilePath(subpath.c_str());

    sol::load_result script = m_pLua->load_file(std::string(path));
    if (script.valid() == false) {
        sol::error error = script;
        throw error;
    }
    sol::function function = script;

    sol::environment envScript(*m_pLua, sol::create, (*m_pLua).globals());
    sol::set_environment(envScript, function);

    return function;
}

template <class T>
void Initialize(T value) {
}

template <>
void Initialize(TRef<Image> value) {
    value->Update();
}

template <>
void Initialize(TRef<TStaticValue<float>> value) {
    value->Update();
}

template <>
void Initialize(TRef<TStaticValue<bool>> value) {
    value->Update();
}

template <>
void Initialize(TRef<TStaticValue<ZString>> value) {
    value->Update();
}

template <class T, typename... TArgs>
T Executor::Execute(sol::function script, TArgs ... args) {
    try {
        m_countInScriptLevel += 1;
        sol::function_result function_result = script.call(args...);
        if (function_result.valid() == false) {
            sol::error err = function_result;
            throw err;
        }

        sol::optional<sol::object> object = function_result;
        if (!object || object.value().is<T>() == false) {
            throw std::runtime_error("Expected return value to be of a specific type");
        }
        T result = function_result;
        Initialize<T>(result);

        m_countInScriptLevel -= 1;
        return result;
    }
    catch (const sol::error& e) {
        m_countInScriptLevel -= 1;
        throw std::runtime_error(e.what());
    }
    catch (const std::runtime_error& e) {
        m_countInScriptLevel -= 1;
        throw e;
    }
    catch (const std::exception& e) {
        m_countInScriptLevel -= 1;
        throw std::runtime_error(e.what());
    }
    catch (...) {
        m_countInScriptLevel -= 1;
        ZAssert(false);
        throw std::runtime_error("Unknown error");
    }
};

LuaScriptContext::LuaScriptContext(Window* pWindow, Engine* pEngine, ISoundEngine* pSoundEngine, const std::shared_ptr<IFileLoader>& pFileLoader, const std::shared_ptr<UiScreenConfiguration>& pConfiguration, std::function<void(std::string)> funcOpenWebsite) :
    m_pWindow(pWindow),
    m_pEngine(pEngine),
    m_pSoundEngine(pSoundEngine),
    m_pConfiguration(pConfiguration),
    m_loader(Loader(m_lua, pEngine, pFileLoader)),
    m_pFileLoader(pFileLoader),
    m_funcOpenWebsite(funcOpenWebsite),
    m_executor(Executor()),
    m_pHasKeyboardFocus(new SimpleModifiableValue<bool>(false)),
    m_pKeyboardCharSource(new TEvent<const KeyState&>::SourceImpl()),
    m_pKeyboardKeySource(new TEvent<const KeyState&>::SourceImpl())
{
    m_loader.InitNamespaces(*this);
}

sol::state& LuaScriptContext::GetLua() {
    return m_lua;
}

sol::function LuaScriptContext::LoadScript(std::string path) {
    return m_loader.LoadScript(path);
}

std::string LuaScriptContext::FindPath(std::string path) {
    if (m_pFileLoader->HasFile(path.c_str()) == false) {
        throw std::runtime_error("File path not found: " + path);
    }
    ZString full_path = m_pFileLoader->GetFilePath(path.c_str());
    return std::string(full_path);
}

Engine* LuaScriptContext::GetEngine() {
    return m_pEngine;
}

ISoundEngine* LuaScriptContext::GetSoundEngine() {
    return m_pSoundEngine;
}

std::function<void(std::string)> LuaScriptContext::GetOpenWebsiteFunction() {
    return m_funcOpenWebsite;
}

template <typename T, typename... TArgs>
std::function<T(TArgs...)> LuaScriptContext::WrapCallback(sol::function callback, T valueDefault) {
    Executor* executor = GetExecutor();
    return [executor, callback, valueDefault](TArgs ... args) {
        try {
            //WriteLog("(Callback function): Started");

            auto start = std::chrono::high_resolution_clock::now();

            T result = executor->Execute<T, TArgs...>(callback, args...);

            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

            if (!executor->IsInScript()) {
                //only output that we have done anything if this is the main script
                WriteLog("(Callback function): Finished " + std::to_string(milliseconds) + " ms");
            }
            return result;
        }
        catch (const std::runtime_error& e) {
            if (executor->IsInScript()) {
                //if we are in a script we are wrapped in a try-catch which can catch this
                throw e;
            }
            WriteLog(std::string("(Callback function): ERROR ") + e.what());
            return valueDefault;
        }
    };
}

IEventSink& LuaScriptContext::GetExternalEventSink(std::string name) {
    return *m_pConfiguration->Get<TRef<IEventSink>>(name);
}

UiObjectContainer& LuaScriptContext::GetScreenGlobals() {
    return *m_pConfiguration;
}

class UiEngineImpl : public UiEngine {

private:
    TRef<Window> m_pWindow;
    TRef<Engine> m_pEngine;
    TRef<ISoundEngine> m_pSoundEngine;
    std::function<void(std::string)> m_funcOpenWebsite;

    TRef<EventSourceImpl> m_pReloadEventSource;


public:
    UiEngineImpl(Window* pWindow, Engine* pEngine, ISoundEngine* pSoundEngine, std::function<void(std::string)> funcOpenWebsite) :
        m_pWindow(pWindow),
        m_pEngine(pEngine),
        m_pSoundEngine(pSoundEngine),
        m_pReloadEventSource(new EventSourceImpl()),
        m_funcOpenWebsite(funcOpenWebsite)
    {
    }

    void TriggerReload() {
        m_pReloadEventSource->Trigger();
    }

    ~UiEngineImpl() {}

    //Image* LoadImage(std::string path) {
    //
    //}


    class ValueChangeSource : public Value, public EventSourceImpl {
    public:
        ValueChangeSource(const TRef<Value>& value) :
            Value(value)
        {
        }

        void ChildChanged(Value* pvalue, Value* pvalueNew) override {
            Trigger();
        }
    };

    class ContextImage : public WrapImage, public IKeyboardInput {
    private:
        std::unique_ptr<LuaScriptContext> m_pContext;
        TRef<IEventSource> m_pFocusChangedSource;
        bool m_bFocus;
        TRef<IKeyboardInput> m_pPreviousFocus;

    public:
        ContextImage(std::unique_ptr<LuaScriptContext> pContext, Image* pImage) :
            WrapImage(pImage),
            m_pContext(std::move(pContext)),
            m_bFocus(false),
            m_pPreviousFocus(nullptr)
        {
            m_pFocusChangedSource = new ValueChangeSource(m_pContext->HasKeyboardFocus());
            m_pFocusChangedSource->AddSink(new CallbackSink([this]() {
                auto window = m_pContext->GetWindow();
                if (m_pContext->HasKeyboardFocus()->GetValue()) {
                    if (window->GetFocus() != this) {
                        m_pPreviousFocus = window->GetFocus();
                        window->SetFocus(this);
                    }
                }
                else {
                    if (window->GetFocus() == this) {
                        if (!m_pPreviousFocus) {
                            window->RemoveFocus(this);
                        }
                        else {
                            window->SetFocus(m_pPreviousFocus);
                            m_pPreviousFocus = nullptr;
                        }
                    }
                }
                return true;
            }));
        }

        ~ContextImage() {
            //control order of deallocations, first remove all references to sol before clearing the context
            SetImage(Image::GetEmpty());
            m_pContext = nullptr;
        }

        void Render(Context* pcontext) override {
            pcontext->SetYAxisInversion(false);
            WrapImage::Render(pcontext);
            pcontext->SetYAxisInversion(true); //not part of the state, so revert manually
        }

        void SetFocusState(bool bFocus) override {
            m_bFocus = bFocus;
            if (m_pContext->HasKeyboardFocus()->GetValue() != bFocus) {
                m_pContext->HasKeyboardFocus()->SetValue(bFocus);
            }
        }

        bool OnChar(IInputProvider* pprovider, const KeyState& ks) override
        {
            switch (ks.vk) {
            case VK_RETURN:
            case VK_BACK:
                //I think only a very limited subset arrives here
                return false;
            default:
                m_pContext->GetKeyboardCharSource()->Trigger(ks);
                return true;
            }
        }

        bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate) override
        {
            m_pContext->GetKeyboardKeySource()->Trigger(ks);

            return false;
        }

        void MouseMove(IInputProvider* pprovider, const Point& point, bool bCaptured, bool bInside) override
        {
            float sizeY = (float)m_pContext->GetEngine()->GetFullscreenSize().Y();

            WrapImage::MouseMove(pprovider, Point(point.X(), sizeY - point.Y()), bCaptured, bInside);
        }

        MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured) override
        {
            float sizeY = (float)m_pContext->GetEngine()->GetFullscreenSize().Y();

            return WrapImage::HitTest(pprovider, Point(point.X(), sizeY - point.Y()), bCaptured);
        }

        MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown) override
        {
            float sizeY = (float)m_pContext->GetEngine()->GetFullscreenSize().Y();

            return WrapImage::Button(pprovider, Point(point.X(), sizeY - point.Y()), button, bCaptured, bInside, bDown);
        }
    };

    TRef<Image> InnerLoadImageFromLua(const std::shared_ptr<UiScreenConfiguration>& screenConfiguration) {
        std::unique_ptr<LuaScriptContext> pContext = std::make_unique<LuaScriptContext>(m_pWindow, m_pEngine, m_pSoundEngine, m_fileLoader, screenConfiguration, m_funcOpenWebsite);

        Executor* executor = pContext->GetExecutor();

        auto path = screenConfiguration->GetPath();

        WriteLog(path + ": " + "Loading");
        try {
            sol::function script = pContext->LoadScript(path);

            WriteLog(path + ": " + "Parsed");

            auto start = std::chrono::high_resolution_clock::now();

            TRef<Image> image = executor->Execute<TRef<Image>>(script);

            auto elapsed = std::chrono::high_resolution_clock::now() - start;
            long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

            WriteLog(path + ": " + "Executed " + std::to_string(milliseconds) + " ms");

            return new ContextImage(std::move(pContext), image);
        }
        catch (const std::runtime_error& e) {
            WriteLog(path + ": ERROR " + e.what());
            return Image::GetEmpty();
        }
    }

    class ImageReloadSink : public IEventSink, public WrapImage {

        UiEngineImpl* m_pUiEngine;
        std::shared_ptr<UiScreenConfiguration> m_pConfiguration;

        TRef<IEventSink> m_pSinkDelegate;

    public:
        ImageReloadSink(UiEngineImpl* pUiEngine, const std::shared_ptr<UiScreenConfiguration>& screenConfiguration) :
            m_pUiEngine(pUiEngine),
            m_pConfiguration(screenConfiguration),
            WrapImage(pUiEngine->InnerLoadImageFromLua(screenConfiguration))
        {
            m_pSinkDelegate = IEventSink::CreateDelegate(this);
        }

        ~ImageReloadSink() {
            m_pUiEngine->m_pReloadEventSource->RemoveSink(m_pSinkDelegate);
        }

        IEventSink* GetSinkDelegate() {
            return m_pSinkDelegate;
        }

        bool OnEvent(IEventSource* pevent) {
            SetImage(m_pUiEngine->InnerLoadImageFromLua(m_pConfiguration));
            return true;
        }

        void Render(Context* pContext) override {
            WrapImage::Render(pContext);
        }

    };

    TRef<Image> LoadImageFromLua(const std::shared_ptr<UiScreenConfiguration>& screenConfiguration) {
        auto path = screenConfiguration->GetPath();

        ImageReloadSink* result = new ImageReloadSink(this, screenConfiguration);
        m_pReloadEventSource->AddSink(result->GetSinkDelegate());
        return result;
    }
};

UiEngine* UiEngine::Create(Window* pWindow, Engine* pEngine, ISoundEngine* pSoundEngine, std::function<void(std::string)> funcOpenWebsite)
{
    return new UiEngineImpl(pWindow, pEngine, pSoundEngine, funcOpenWebsite);
}