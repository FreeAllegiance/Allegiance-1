#pragma once

#include <string>

#include "Windows.h"

class IConfigurationStore {
public:
    virtual void Commit() {}

    virtual bool ReadBool(std::string key, bool valueDefault) = 0;
    virtual void SetBool(std::string key, bool value) = 0;

    virtual float ReadFloat(std::string key, float valueDefault) = 0;
    virtual void SetFloat(std::string key, float value) = 0;

    virtual int ReadInt(std::string key, int valueDefault) = 0;
    virtual void SetInt(std::string key, int value) = 0;

    virtual std::string ReadString(std::string key, std::string valueDefault) = 0;
    virtual void SetString(std::string key, std::string value) = 0;
};

class RegistryConfigurationStore : public IConfigurationStore {
private:
    LPCSTR mRegistryPath;
    HKEY mHKey;

public:
    RegistryConfigurationStore(HKEY hKey, LPCSTR registryPath) :
        mHKey(hKey),
        mRegistryPath(registryPath)
    {
    }

    bool ReadBool(std::string key, bool valueDefault) override;
    void SetBool(std::string key, bool value) override;

    float ReadFloat(std::string key, float valueDefault) override;
    void SetFloat(std::string key, float value) override;

    int ReadInt(std::string key, int valueDefault) override;
    void SetInt(std::string key, int value) override;

    std::string ReadString(std::string key, std::string valueDefault) override;
    void SetString(std::string key, std::string value) override;
};

std::shared_ptr<IConfigurationStore> CreateJsonConfigurationStore(std::string path);

class FallbackConfigurationStore : public IConfigurationStore {
private:
    std::shared_ptr<IConfigurationStore> mJsonStore;
    std::shared_ptr<IConfigurationStore> mRegStore;

public:
    FallbackConfigurationStore(const std::shared_ptr<IConfigurationStore>& jsonStore, const std::shared_ptr<RegistryConfigurationStore>& regStore) :
        mJsonStore(jsonStore),
        mRegStore(regStore)
    {}

    void Commit() override;

    bool ReadBool(std::string key, bool valueDefault) override;
    void SetBool(std::string key, bool value) override;

    float ReadFloat(std::string key, float valueDefault) override;
    void SetFloat(std::string key, float value) override;

    int ReadInt(std::string key, int valueDefault) override;
    void SetInt(std::string key, int value) override;

    std::string ReadString(std::string key, std::string valueDefault) override;
    void SetString(std::string key, std::string value) override;
};