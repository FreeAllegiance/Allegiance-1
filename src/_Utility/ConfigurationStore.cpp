
#include "pch.h"

#include "ConfigurationStore.h"

#include "json.hpp"
#include <fstream>

class JsonConfigurationStore : public IConfigurationStore {
private:
    nlohmann::json m_json;

    std::string m_path;

public:
    JsonConfigurationStore(std::string path);

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

std::shared_ptr<IConfigurationStore> CreateJsonConfigurationStore(std::string path) {
    return std::make_shared<JsonConfigurationStore>(path);
}

DWORD GetDWordRegValue(HKEY hkey, LPCSTR appKey, std::string key, DWORD valueDefault) {
    HKEY hKey;
    if (ERROR_SUCCESS == ::RegOpenKeyEx(hkey, appKey,
        0, KEY_READ, &hKey))
    {
        DWORD dwResult;
        DWORD dwSize = sizeof(dwResult);
        DWORD dwType = REG_DWORD;

        ::RegQueryValueEx(hKey, key.c_str(), NULL, &dwType, (BYTE*)&dwResult, &dwSize);
        ::RegCloseKey(hKey);

        if (dwType == REG_DWORD) {
            return dwResult;
        }
    }
    return valueDefault;
}

void SetDWordRegValue(HKEY hkey, LPCSTR appKey, std::string key, DWORD value) {
    HKEY hKey;

    if (ERROR_SUCCESS == ::RegCreateKeyEx(hkey, appKey,
        0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
    {
        ::RegSetValueEx(hKey, key.c_str(), NULL, REG_DWORD, (const BYTE*)&value, sizeof(value));
        ::RegCloseKey(hKey);
    }
}

std::string GetStringRegValue(HKEY hkey, LPCSTR appKey, std::string key, std::string valueDefault) {
    HKEY hKey;
    if (ERROR_SUCCESS != ::RegOpenKeyEx(hkey, appKey,
        0, KEY_READ, &hKey))
    {
        return valueDefault;
    }

    DWORD cbValue = 512;
    char result[512];
    result[0] = '\0';
    if (ERROR_SUCCESS != ::RegQueryValueEx(hKey, key.c_str(), NULL, NULL, (LPBYTE)&result, &cbValue)) {
        return valueDefault;
    }

    return std::string(result);
}

void SetStringRegValue(HKEY hkey, LPCSTR appKey, std::string key, std::string value) {
    HKEY hKey;

    if (ERROR_SUCCESS == ::RegCreateKeyEx(hkey, appKey,
        0, "", REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
    {
        char target[512];
        lstrcpy(target, value.c_str());

        ::RegSetValueEx(hKey, key.c_str(), NULL, REG_DWORD, (const BYTE*)&target, sizeof(target));
        ::RegCloseKey(hKey);
    }
}

bool RegistryConfigurationStore::ReadBool(std::string key, bool valueDefault) {
    return (0 != GetDWordRegValue(mHKey, mRegistryPath, key, valueDefault ? 1 : 0));
}

void RegistryConfigurationStore::SetBool(std::string key, bool value) {
    SetDWordRegValue(mHKey, mRegistryPath, key, value ? 1 : 0);
}

int RegistryConfigurationStore::ReadInt(std::string key, int valueDefault) {
    return (int)GetDWordRegValue(mHKey, mRegistryPath, key, (DWORD)valueDefault);
}

void RegistryConfigurationStore::SetInt(std::string key, int value) {
    SetDWordRegValue(mHKey, mRegistryPath, key, (DWORD)value);
}

float RegistryConfigurationStore::ReadFloat(std::string key, float valueDefault) {
    auto regvalue = GetStringRegValue(mHKey, mRegistryPath, key, "");

    if (regvalue == "") {
        return valueDefault;
    }

    float fResult = std::stof(regvalue);
    if (std::isnan(fResult) == true) {
        return valueDefault;
    }

    return fResult;
}

void RegistryConfigurationStore::SetFloat(std::string key, float value) {
    std::ostringstream ss;
    ss << value;
    std::string s(ss.str());

    SetStringRegValue(mHKey, mRegistryPath, key, std::to_string(value));
}

std::string RegistryConfigurationStore::ReadString(std::string key, std::string valueDefault) {
    return GetStringRegValue(mHKey, mRegistryPath, key, valueDefault);
}

void RegistryConfigurationStore::SetString(std::string key, std::string value) {
    SetStringRegValue(mHKey, mRegistryPath, key, value);
}

// ##### JSON #####

template <typename T>
T GetJsonValue(const nlohmann::json& json, std::string key, T defaultValue) {
    auto found = json.find(key);
    
    if (found == json.end()) {
        return defaultValue;
    }

    try {
        return found->get<T>();
    }
    catch (std::exception e) {
        return defaultValue;
    }
}

template <typename T>
void SetJsonValue(nlohmann::json& json, std::string key, T value) {
    json[key] = value;
}

JsonConfigurationStore::JsonConfigurationStore(std::string path) :
    m_path(path)
{
    try {
        std::ifstream filestream(path);
        m_json = nlohmann::json::parse(filestream);
    }
    catch (std::exception e) {
        //initial value for m_json is empty, which is fine
    }
}

void JsonConfigurationStore::Commit() {
    std::ofstream filestream(m_path);
    filestream << m_json.dump(4);
}

bool JsonConfigurationStore::ReadBool(std::string key, bool valueDefault) {
    return GetJsonValue<bool>(m_json, key, valueDefault);
}

void JsonConfigurationStore::SetBool(std::string key, bool value) {
    SetJsonValue<bool>(m_json, key, value);
}

int JsonConfigurationStore::ReadInt(std::string key, int valueDefault) {
    return GetJsonValue<int>(m_json, key, valueDefault);
}

void JsonConfigurationStore::SetInt(std::string key, int value) {
    SetJsonValue<int>(m_json, key, value);
}

float JsonConfigurationStore::ReadFloat(std::string key, float valueDefault) {
    return GetJsonValue<float>(m_json, key, valueDefault);
}

void JsonConfigurationStore::SetFloat(std::string key, float value) {
    SetJsonValue<float>(m_json, key, value);
}

std::string JsonConfigurationStore::ReadString(std::string key, std::string valueDefault) {
    return GetJsonValue<std::string>(m_json, key, valueDefault);
}

void JsonConfigurationStore::SetString(std::string key, std::string value) {
    SetJsonValue<std::string>(m_json, key, value);
}


// #### FALLBACK ####

void FallbackConfigurationStore::Commit() {
    mJsonStore->Commit();
}

bool FallbackConfigurationStore::ReadBool(std::string key, bool valueDefault) {
    return mJsonStore->ReadBool(key, mRegStore->ReadBool(key, valueDefault));
}

void FallbackConfigurationStore::SetBool(std::string key, bool value) {
    mJsonStore->SetBool(key, value);
}

int FallbackConfigurationStore::ReadInt(std::string key, int valueDefault) {
    return mJsonStore->ReadInt(key, mRegStore->ReadInt(key, valueDefault));
}

void FallbackConfigurationStore::SetInt(std::string key, int value) {
    mJsonStore->SetInt(key, value);
}

float FallbackConfigurationStore::ReadFloat(std::string key, float valueDefault) {
    return mJsonStore->ReadFloat(key, mRegStore->ReadFloat(key, valueDefault));
}

void FallbackConfigurationStore::SetFloat(std::string key, float value) {
    mJsonStore->SetFloat(key, value);
}

std::string FallbackConfigurationStore::ReadString(std::string key, std::string valueDefault) {
    return mJsonStore->ReadString(key, mRegStore->ReadString(key, valueDefault));
}

void FallbackConfigurationStore::SetString(std::string key, std::string value) {
    mJsonStore->SetString(key, value);
}