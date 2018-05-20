
#pragma once

#include "ui.h"
#include "items.hpp"

template<typename TypeResult, typename TypeArgument>
class ValueToMappedValue : public TWrapValue<TypeResult> {
    typedef TStaticValue<TypeResult> TypeWrappedResult;
    typedef TStaticValue<TypeArgument> TypeWrappedArgument;

    TRef<TypeWrappedResult> m_default;
    std::map<TypeArgument, TRef<TypeWrappedResult>> m_mapOptions;

public:
    ValueToMappedValue(TypeWrappedArgument* tValue, std::map<TypeArgument, TRef<TypeWrappedResult>> mapOptions, TypeWrappedResult* tDefault) :
        m_default(tDefault),
        m_mapOptions(mapOptions),
        TWrapValue(tDefault)
    {
        AddChild(tValue);
    }

    void Evaluate()
    {
        TypeArgument value = ((TypeWrappedArgument*)GetChild(1))->GetValue();

        auto find = m_mapOptions.find(value);

        auto evaluated = find == m_mapOptions.end() ? m_default : find->second;
        evaluated->Update();
        SetChildSilently(0, evaluated);

        TWrapValue::Evaluate();
    }
};

class StringNamespace {
public:
    static void AddNamespace(LuaScriptContext& context) {
        sol::table table = context.GetLua().create_table();

        context.GetLua().new_usertype<SimpleModifiableValue<ZString>>("SimpleModifiableValue<ZString>",
            sol::base_classes, sol::bases<StringValue, TEvent<ZString>::Sink>()
        );

        table["CreateEventSink"] = [](std::string start) {
            return (TRef<SimpleModifiableValue<ZString>>)new SimpleModifiableValue<ZString>(ZString(start.c_str()));
        };

        table["Length"] = [](sol::object a) {
            return StringTransform::Length(wrapString(a));
        };

        table["Concat"] = [](sol::object a, sol::object b) {
            return StringTransform::Concat(wrapString(a), wrapString(b));
        };

        table["Slice"] = [](const TRef<StringValue>& string, const TRef<Number>& start, const TRef<Number>& length) {
            return StringTransform::Slice(string, start, length);
        };

        table["Equals"] = [](TRef<StringValue> const& a, TRef<StringValue> const& b) {
            return StringTransform::Equals(a, b);
        };

        table["Switch"] = [](sol::object value, sol::table table, sol::optional<sol::object> valueDefault) {
            int count = table.size();

            TRef<StringValue> valueDefaultNonOptional;
            if (valueDefault) {
                valueDefaultNonOptional = wrapString(valueDefault.value());
            }
            else {
                valueDefaultNonOptional = new StringValue(ZString(""));
            }

            if (value.is<TRef<Number>>() || value.is<float>()) {
                //float/int problems are likely
                std::map<float, TRef<StringValue>> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object entry_value) {
                    int fKey = (int)key.as<float>();
                    mapOptions[fKey] = wrapString(entry_value);
                });

                return (TRef<StringValue>)new ValueToMappedValue<ZString, float>(wrapValue<float>(value), mapOptions, valueDefaultNonOptional);
            }
            else if (value.is<TRef<TStaticValue<ZString>>>() || value.is<std::string>()) {
                //the wrapped value is a ZString, the unwrapped value a std::string
                std::map<ZString, TRef<StringValue>> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object entry_value) {
                    std::string strKey = key.as<std::string>();
                    mapOptions[strKey.c_str()] = wrapString(entry_value);
                });

                return (TRef<StringValue>)new ValueToMappedValue<ZString, ZString>(wrapString(value), mapOptions, valueDefaultNonOptional);
            }
            else if (value.is<TRef<Boolean>>() || value.is<bool>()) {
                std::map<bool, TRef<StringValue>> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object entry_value) {
                    bool bKey = key.as<bool>();
                    mapOptions[bKey] = wrapString(entry_value);
                });

                return (TRef<StringValue>)new ValueToMappedValue<ZString, bool>(wrapValue<bool>(value), mapOptions, valueDefaultNonOptional);
            }
            throw std::runtime_error("Expected value argument of String.Switch to be either a wrapped or unwrapped bool, int, or string");
        };

        context.GetLua().new_usertype<TRef<StringValue>>("StringValue", 
            sol::meta_function::concatenation, [](const TRef<StringValue>& a, const TRef<StringValue>& b) {
                return StringTransform::Concat(a, b);
            }
        );
        context.GetLua().new_usertype<TRef<SimpleModifiableValue<ZString>>>("StringValue",
            sol::meta_function::concatenation, [](const TRef<StringValue>& a, const TRef<StringValue>& b) {
                return StringTransform::Concat(a, b);
            }
        );

        context.GetLua().set("String", table);
    }
};
