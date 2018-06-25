#pragma once

#include "FileLoader.h"

class UiScreenConfiguration : public UiObjectContainer {
public:
    UiScreenConfiguration(std::map<std::string, std::shared_ptr<Exposer>> map) :
        UiObjectContainer(map)
    {}

    virtual std::string GetPath() = 0;
    virtual IEventSink& GetEventSink(std::string) = 0;

    static std::shared_ptr<UiScreenConfiguration> Create(std::string path, std::map<std::string, std::shared_ptr<Exposer>> map);

    //deprecated:
    static std::shared_ptr<UiScreenConfiguration> Create(std::string path, std::map<std::string, std::function<bool()>> event_listeners, std::map<std::string, std::shared_ptr<Exposer>> map);
}; 

class UiEngine : public IObject
{
protected:

    static std::shared_ptr<IFileLoader> m_fileLoader;

public:

    static std::string m_stringLogPath;

    static void SetGlobalFileLoader(const std::shared_ptr<IFileLoader>& pLoader);
    static UiEngine* UiEngine::Create(Window* pWindow, Engine* pEngine, ISoundEngine* pSoundEngine, std::function<void(std::string)> funcOpenWebsite);

    //virtual Image* LoadImage(std::string path) = 0;
    virtual TRef<Image> LoadImageFromLua(const std::shared_ptr<UiScreenConfiguration>& screenConfiguration) = 0;

    virtual void TriggerReload() = 0;
};

class PathFinder {

private:
    // from: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
    inline bool file_exists(const std::string& name) {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }
    std::vector<std::string> m_paths;

public:

    PathFinder(std::vector<std::string> paths) :
        m_paths(paths)
    {

    }

    std::string FindPath(std::string& subpath) {
        for (auto& path : m_paths) {
            if (file_exists(path + '/' + subpath) == true) {
                return path + '/' + subpath;
            }
        }
        return "";
    }
};

class LuaScriptContext;
class Loader {

private:
    sol::state* m_pLua;
    std::shared_ptr<IFileLoader> m_pFileLoader;

public:
    Loader(sol::state& lua, Engine* pEngine, const std::shared_ptr<IFileLoader>& pFileLoader);
    ~Loader();

    void InitNamespaces(LuaScriptContext& context);

    sol::function LoadScript(std::string subpath);
};

class Executor {
private:
    int m_countInScriptLevel;

public:
    Executor() :
        m_countInScriptLevel(0)
    {}

    bool IsInScript() {
        return m_countInScriptLevel > 0;
    }

    template <class T, typename... TArgs>
    T Execute(sol::function script, TArgs ... args);
};

class LuaScriptContext {
private:
    TRef<Window> m_pWindow;
    TRef<Engine> m_pEngine;
    TRef<ISoundEngine> m_pSoundEngine;
    Loader m_loader;
    std::shared_ptr<IFileLoader> m_pFileLoader;
    std::shared_ptr<UiScreenConfiguration> m_pConfiguration;
    std::function<void(std::string)> m_funcOpenWebsite;
    TRef<SimpleModifiableValue<bool>> m_pHasKeyboardFocus;

    TRef<TEvent<const KeyState&>::SourceImpl> m_pKeyboardCharSource;
    TRef<TEvent<const KeyState&>::SourceImpl> m_pKeyboardKeySource;

    Executor m_executor;

    sol::state m_lua;

public:
    LuaScriptContext(
        Window* pWindow,
        Engine* pEngine,
        ISoundEngine* pSoundEngine, 
        const std::shared_ptr<IFileLoader>& pFileLoader,
        const std::shared_ptr<UiScreenConfiguration>& pConfiguration, 
        std::function<void(std::string)> funcOpenWebsite
    );

    sol::state& GetLua();

    sol::function LoadScript(std::string path);

    std::string FindPath(std::string path);

    Engine* GetEngine();

    ISoundEngine* GetSoundEngine();

    Executor* GetExecutor() {
        return &m_executor;
    }

    TRef<SimpleModifiableValue<bool>> HasKeyboardFocus() {
        return m_pHasKeyboardFocus;
    }

    TRef<TEvent<const KeyState&>::SourceImpl> GetKeyboardCharSource() {
        return m_pKeyboardCharSource;
    }

    TRef<TEvent<const KeyState&>::SourceImpl> GetKeyboardKeySource() {
        return m_pKeyboardKeySource;
    }

    TRef<Window> GetWindow() {
        return m_pWindow;
    }

    IEventSink& GetExternalEventSink(std::string name);

    std::function<void(std::string)> GetOpenWebsiteFunction();

    template <typename T, typename... TArgs>
    std::function<T(TArgs...)> WrapCallback(sol::function callback, T valueDefault);

    UiObjectContainer& GetScreenGlobals();
};