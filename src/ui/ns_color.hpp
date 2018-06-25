
#pragma once

#include "ui.h"
#include "items.hpp"

class ColorNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();
        table["Create"] = [](sol::object r, sol::object g, sol::object b, sol::optional<TRef<Number>> alpha) {
            TRef<Number> pWrappedAlpha;
            if (alpha) {
                pWrappedAlpha = alpha.value();
            }
            else {
                pWrappedAlpha = new Number(1.0f);
            }
            return ColorTransform::Create(
                wrapValue<float>(r),
                wrapValue<float>(g),
                wrapValue<float>(b),
                pWrappedAlpha
            );
        };

        m_pLua->set("Color", table);
    }
};
