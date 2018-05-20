
#pragma once

#include "ui.h"
#include "items.hpp"

using namespace std::literals;

class FontNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["Create"] = [](std::string name, TRef<Number> size, sol::optional<sol::table> object) {
            std::map<std::string, TRef<Boolean>> props = {
                { "Bold", new Boolean(false) },
                { "Italic", new Boolean(false) },
                { "Underline", new Boolean(false) }
            };

            if (object) {
                object.value().for_each([&props](sol::object key, sol::object value) {
                    std::string strKey = key.as<std::string>();
                    if (props.find(strKey) == props.end()) {
                        throw std::runtime_error("Unknown key. Use 'Bold', 'Italic', 'Underline'");
                    }
                    props[strKey] = value.as<TRef<Boolean>>();
                });
            }

            return (TRef<FontValue>)(FontValue*)new TransformedValue4<TRef<IEngineFont>, float, bool, bool, bool>([name](float size, bool bold, bool italic, bool underline) {
                return CreateEngineFont(name, std::min(100, (int)size), 0, bold, italic, underline);
            }, size, props["Bold"], props["Italic"], props["Underline"]);
        };

        table["Height"] = [](const TRef<FontValue>& font) {
            return (TRef<Number>)new TransformedValue<float, TRef<IEngineFont>>([](IEngineFont* font) {
                return (float)font->GetHeight();
            }, font);
        };

        m_pLua->set("Font", table);
    }
};
