
#pragma once

#include "ui.h"
#include "items.hpp"

template<typename TypeResult, typename A, typename B>
auto createAutoWrappingFunction(std::function<TypeResult(A, B)> callback) {

    return [callback](sol::object a, sol::object b) {
        return (TStaticValue<TypeResult>*)new TransformedValue<TypeResult, A, B>(callback, wrapValue<A>(a), wrapValue<B>(b));
    };
};

class NumberNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["Create"] = [](float a) {
            return new Number(a);
        };

        table["CreateEventSink"] = [](float a) {
            return (TRef<SimpleModifiableValue<float>>)new SimpleModifiableValue<float>(a);
        };

        table["ToString"] = [](sol::object a, sol::optional<int> decimals) {
            
            return NumberTransform::ToString(wrapValue<float>(a), decimals.value_or(0));
        };

        table["Equals"] = [](TRef<Number> const& a, TRef<Number> const& b) {
            return NumberTransform::Equals(a, b);
        };

        table["Add"] = [](sol::object a, sol::object b) {
            return NumberTransform::Add(wrapValue<float>(a), wrapValue<float>(b));
        };
        table["Subtract"] = [](sol::object a, sol::object b) {
            return NumberTransform::Subtract(wrapValue<float>(a), wrapValue<float>(b));
        };
        table["Multiply"] = [](sol::object a, sol::object b) {
            return NumberTransform::Multiply(wrapValue<float>(a), wrapValue<float>(b));
        };
        table["Divide"] = [](sol::object a, sol::object b) {
            return NumberTransform::Divide(wrapValue<float>(a), wrapValue<float>(b));
        };
        table["Mod"] = [](sol::object a, sol::object b) {
            return NumberTransform::Mod(wrapValue<float>(a), wrapValue<float>(b));
        };
        table["Min"] = [](sol::object a, sol::object b) {
            return NumberTransform::Min(wrapValue<float>(a), wrapValue<float>(b));
        };
        table["Max"] = [](sol::object a, sol::object b) {
            return NumberTransform::Max(wrapValue<float>(a), wrapValue<float>(b));
        };
        table["Clamp"] = [](TRef<Number> value, TRef<Number> min, TRef<Number> max) {
            return NumberTransform::Clamp(value, min, max);
        };
        table["Round"] = [](sol::object a, sol::optional<int> decimals) {
            return NumberTransform::Round(wrapValue<float>(a), decimals.value_or(0));
        };
        table["Sin"] = [](sol::object a) {
            return NumberTransform::Sin(wrapValue<float>(a));
        };
        table["Cos"] = [](sol::object a) {
            return NumberTransform::Cos(wrapValue<float>(a));
        };
        table["Sqrt"] = [](const TRef<Number>& value) {
            return NumberTransform::Sqrt(value);
        };
        table["Power"] = [](const TRef<Number>& value, const TRef<Number>& power) {
            return NumberTransform::Power(value, power);
        };

        m_pLua->new_usertype<TRef<Number>>("Number",
            sol::meta_function::addition, [](sol::object a, sol::object b) {
                return NumberTransform::Add(wrapValue<float>(a), wrapValue<float>(b));
            },
            sol::meta_function::subtraction, [](sol::object a, sol::object b) {
                return NumberTransform::Subtract(wrapValue<float>(a), wrapValue<float>(b));
            },
            sol::meta_function::multiplication, [](sol::object a, sol::object b) {
                return NumberTransform::Multiply(wrapValue<float>(a), wrapValue<float>(b));
            },
            sol::meta_function::division, [](sol::object a, sol::object b) {
                return NumberTransform::Divide(wrapValue<float>(a), wrapValue<float>(b));
            }
        );

        m_pLua->new_usertype<TRef<SimpleModifiableValue<float>>>("SimpleModifiableValue<float>",
            //sol::base_classes, sol::bases<Number, TEvent<float>::Sink>(),
            sol::meta_function::addition, [](sol::object a, sol::object b) {
                return NumberTransform::Add(wrapValue<float>(a), wrapValue<float>(b));
            },
                sol::meta_function::subtraction, [](sol::object a, sol::object b) {
                return NumberTransform::Subtract(wrapValue<float>(a), wrapValue<float>(b));
            },
                sol::meta_function::multiplication, [](sol::object a, sol::object b) {
                return NumberTransform::Multiply(wrapValue<float>(a), wrapValue<float>(b));
            },
                sol::meta_function::division, [](sol::object a, sol::object b) {
                return NumberTransform::Divide(wrapValue<float>(a), wrapValue<float>(b));
            }
        );

        m_pLua->set("Number", table);
    }
};

class RectNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["Create"] = sol::overload(
            [](int xmin, int ymin, int xmax, int ymax) {
                return (TRef<RectValue>)new RectValue(Rect(xmin, ymin, xmax, ymax));
            },
            [](sol::object xmin, sol::object ymin, sol::object xmax, sol::object ymax) {
                return RectTransform::Create(
                    wrapValue<float>(xmin),
                    wrapValue<float>(ymin),
                    wrapValue<float>(xmax),
                    wrapValue<float>(ymax)
                );
            }
        );

        m_pLua->set("Rect", table);
    }
};

class BooleanNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        m_pLua->new_usertype<SimpleModifiableValue<bool>>("SimpleModifiableValue<bool>",
            sol::base_classes, sol::bases<Boolean, TEvent<bool>::Sink>()
        );

        table["CreateEventSink"] = [](bool a) {
            return (TRef<SimpleModifiableValue<bool>>)new SimpleModifiableValue<bool>(a);
        };

        table["Equals"] = [](TRef<Boolean> const& a, TRef<Boolean> const& b) {
            return BooleanTransform::Equals(a, b);
        };

        table["And"] = [](sol::object a, sol::object b) {
            return (TRef<Boolean>)BooleanTransform::And(wrapValue<bool>(a), wrapValue<bool>(b));
        };
        table["Or"] = [](sol::object a, sol::object b) {
            return (TRef<Boolean>)BooleanTransform::Or(wrapValue<bool>(a), wrapValue<bool>(b));
        };
        table["Not"] = [](sol::object a) {
            return (TRef<Boolean>)BooleanTransform::Not(wrapValue<bool>(a));
        };

        table["ToNumber"] = [](sol::object a) {
            return (TRef<Number>)BooleanTransform::ToNumber(wrapValue<bool>(a));
        };
        table["Count"] = [](sol::table table) {
            std::vector<TRef<Boolean>> vector;
            table.for_each([&vector](sol::object key, sol::object value) {
                vector.push_back(wrapValue<bool>(value));
            });
            return (TRef<Number>)BooleanTransform::Count(vector);
        };

        m_pLua->set("Boolean", table);
    }
};