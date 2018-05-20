#pragma once

#define SOL_CHECK_ARGUMENTS 1
#define SOL_ENABLE_INTEROP 1

#include "sol.hpp"

class Exposer {
protected:
    std::shared_ptr<void> m_pobject;

public:
    Exposer(std::shared_ptr<void>& pobject) : m_pobject(pobject) {
    }

    template <typename T>
    Exposer(T object) {
        m_pobject = std::make_shared<T>(object);
    }

    virtual sol::object ExposeSolObject(lua_State* L) = 0;

    template <typename T>
    operator const T&() const {
        return *std::static_pointer_cast<T>(m_pobject);
    }
};