
#pragma once

#include "ui.h"
#include "items.hpp"

using namespace std::literals;

class PointNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        m_pLua->new_usertype<SimpleModifiableValue<Point>>("SimpleModifiableValue<Point>",
            sol::base_classes, sol::bases<PointValue, TEvent<Point>::Sink>()
        );
        
        table["Create"] = [](sol::object x, sol::object y) {
            return PointTransform::Create(
                wrapValue<float>(x),
                wrapValue<float>(y)
            );
        };

        table["CreateEventSink"] = sol::overload(
            [](float x, float y) {
                return (TRef<SimpleModifiableValue<Point>>)new SimpleModifiableValue<Point>(Point(x, y));
            },
            [](TRef<Number> x, TRef<Number> y) {
                return (TRef<SimpleModifiableValue<Point>>)new SimpleModifiableValue<Point>(Point(x->GetValue(), y->GetValue()));
            }
        );
        
        table["X"] = [](const TRef<PointValue>& pPoint) {
            return PointTransform::X(pPoint);
        };
        table["Y"] = [](const TRef<PointValue>& pPoint) {
            return PointTransform::Y(pPoint);
        };
        m_pLua->set("Point", table);
    }
};
