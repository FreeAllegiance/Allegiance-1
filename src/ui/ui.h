#pragma once

#ifndef __MODULE__
#define __MODULE__ "Ui"
#endif

#include "soundengine.h"
#include "model.h"
#include "image.h"
#include "engine.h"

#include "Exposer.h"

#include "UiState.h"

#include "ui_types.h"

template <typename T>
class TypeExposer : public Exposer {
    //T m_object;

public:
    TypeExposer(const T& obj) : Exposer(obj) {
    }

    static std::shared_ptr<TypeExposer<T>> Create(const T& obj) {
        return std::shared_ptr<TypeExposer<T>>(new TypeExposer(obj));
    }

    sol::object ExposeSolObject(lua_State* L) {
        return sol::make_object<T>(L, *std::static_pointer_cast<T>(m_pobject));
    }

    operator T() const {
        return m_object;
    }
};

template <typename T>
class TStaticValueExposer : public TypeExposer<TRef<TStaticValue<T>>> {
    typedef TRef<TStaticValue<T>> ExposedType;

public:
    TStaticValueExposer(const ExposedType& obj) : TypeExposer(obj) {
    }

    static std::shared_ptr<TStaticValueExposer<T>> CreateStatic(const T& value) {
        return std::shared_ptr<TStaticValueExposer<T>>(new TStaticValueExposer<T>(new TStaticValue<T>(value)));
    }

    //static std::shared_ptr<TStaticValueExposer<T>> CreateModifiable(const T& value) {
    //    return std::shared_ptr<T>(new TypeExposer(TRef(new TModifiableValue<T>(value))));
    //}
};

typedef TStaticValueExposer<bool> BooleanExposer;
typedef TStaticValueExposer<float> NumberExposer;
typedef TStaticValueExposer<ZString> StringExposer;

class Exposed {
    sol::object m_object;
    sol::state m_lua;

public:
    template <typename T>
    Exposed(T arg) {
        m_object = sol::make_object<T>(m_lua, arg);
    }

    sol::object GetSolObject(lua_State* L) {
        return sol::make_object<sol::object>(L, m_object);
    }

};


class CallbackSink : public IEventSink {
private:
    std::function<void()> m_funcCallback;

public:
    CallbackSink(std::function<void()> funcCallback) :
        m_funcCallback(funcCallback)
    {}

    bool OnEvent(IEventSource* pevent) {
        m_funcCallback();
        return true;
    }
};

template <class ...Types>
class CallbackValueSink : public TEvent<Types...>::Sink {
private:
    std::function<bool(Types...)> m_funcCallback;

public:
    CallbackValueSink(std::function<bool(Types...)> funcCallback) :
        m_funcCallback(funcCallback)
    {}

    bool OnEvent(typename TEvent<Types...>::Source* pevent, Types ... values) override {
        return m_funcCallback(values...);
    }
};

#include "UiEngine.h"
