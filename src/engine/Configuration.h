#pragma once

#include <string>
#include <memory>
#include <map>

#include "ConfigurationStore.h"
#include "valuetransform.h"

template <class T>
class CacheValueMap : public Value {
    typedef TRef<SimpleModifiableValue<T>> EntryType;
private:
    std::map<std::string, EntryType> mMap;

public:
    CacheValueMap() :
        mMap({})
    {}

    const std::map<std::string, EntryType>& GetMap() {
        return mMap;
    }

    bool Has(std::string key) {
        auto find = mMap.find(key);
        if (find == mMap.end()) {
            return false;
        }
        return true;
    }

    const EntryType& Get(std::string key) {
        auto find = mMap.find(key);
        if (find == mMap.end()) {
            throw std::runtime_error("Entry not found: " + key);
        }
        return find->second;
    }

    const EntryType& Set(std::string key, T value) {
        auto find = mMap.find(key);
        if (find != mMap.end()) {
            throw std::runtime_error("Entry already exists: " + key);
        }

        TRef<SimpleModifiableValue<T>> result = new SimpleModifiableValue<T>(value);
        auto pair = std::pair<std::string, EntryType>(key, result);

        std::pair<std::map<std::string, EntryType>::iterator, bool> insertResult;
        insertResult = mMap.insert(pair);
        //mMap.insert(key, (EntryType)result);

        result->Update();
        AddChild(result);

        return insertResult.first->second;
    }
};

class UpdatingConfiguration : public Value {
private:
    std::shared_ptr<IConfigurationStore> m_inner;

    TRef<CacheValueMap<bool>> mBoolMap;
    TRef<CacheValueMap<float>> mFloatMap;
    TRef<CacheValueMap<float>> mIntMap;
    TRef<CacheValueMap<ZString>> mStringMap;

public:
    UpdatingConfiguration(const std::shared_ptr<IConfigurationStore>& inner) :
        m_inner(inner)
    {
        mBoolMap = new CacheValueMap<bool>();
        mFloatMap = new CacheValueMap<float>();
        mIntMap = new CacheValueMap<float>();
        mStringMap = new CacheValueMap<ZString>();

        AddChild(mBoolMap);
        AddChild(mFloatMap);
        AddChild(mIntMap);
        AddChild(mStringMap);
    }

    void Evaluate() override {
        this->Write();
    }

    void Write();

    const TRef<SimpleModifiableValue<bool>>& GetBool(std::string key, bool valueDefault);
    bool GetBoolValue(std::string key, bool valueDefault);
    void SetBoolValue(std::string key, bool value);

    const TRef<SimpleModifiableValue<float>>& GetFloat(std::string key, float valueDefault);
    float GetFloatValue(std::string key, float valueDefault);
    void SetFloatValue(std::string key, float value);

    const TRef<SimpleModifiableValue<float>>& GetInt(std::string key, int valueDefault);
    int GetIntValue(std::string key, int valueDefault);
    void SetIntValue(std::string key, int value);

    const TRef<SimpleModifiableValue<ZString>>& GetString(std::string key, const std::string& valueDefault);
    std::string GetStringValue(std::string key, const std::string& valueDefault);
    void SetStringValue(std::string key, const std::string& value);

};
