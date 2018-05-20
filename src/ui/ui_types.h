#include <type_traits>
#include "event.h"

template <typename EntryType>
class UiList;

namespace sol {

    namespace stack {
        //TStaticValue

        template <typename T>
        struct checker<TRef<TStaticValue<T>>, type::userdata> {
            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                if (getter<TRef<TStaticValue<T>>>::get(L, index, tracking)) {
                    return true;
                }

                handler(L, index, type::userdata, type_of(L, index), "value at this index does not properly reflect the desired type");
                return false;
            }
        };

        template <typename T>
        struct getter<TRef<TStaticValue<T>>> {
            static TRef<TStaticValue<T>> get(lua_State* L, int index, record& tracking) {
                if (sol::stack::check_usertype<TRef<TStaticValue<T>>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<TStaticValue<T>>>>{}.get(L, index, tracking);
                }
                if (sol::stack::check_usertype<TRef<SimpleModifiableValue<T>>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<SimpleModifiableValue<T>>>>{}.get(L, index, tracking);
                }
                if (sol::stack::check<T>(L, index, sol::no_panic, record())) {
                    return new TStaticValue<T>(sol::stack::get<T>(L, index, tracking));
                }
                return nullptr;
            }
        };

        template <>
        struct getter<TRef<TStaticValue<ZString>>> {
            typedef TRef<TStaticValue<ZString>> ReturnType;
            static ReturnType get(lua_State* L, int index, record& tracking) {
                if (sol::stack::check_usertype<ReturnType>(L, index)) {
                    return getter<detail::as_value_tag<ReturnType>>{}.get(L, index, tracking);
                }
                if (sol::stack::check_usertype<TRef<SimpleModifiableValue<ZString>>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<SimpleModifiableValue<ZString>>>>{}.get(L, index, tracking);
                }
                if (sol::stack::check<std::string>(L, index, sol::no_panic, record())) {
                    return new TStaticValue<ZString>(sol::stack::get<std::string>(L, index, tracking).c_str());
                }

                //automatically convert from number to string
                if (sol::stack::check<TRef<Number>>(L, index)) {
                    return NumberTransform::ToString(sol::stack::get<TRef<Number>>(L, index), 0);
                }
                return nullptr;
            }
        };

        // Sinks

        template <>
        struct checker<TRef<TEvent<bool>::Sink>, type::userdata> {
            typedef TRef<TEvent<bool>::Sink> ReturnType;
            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                if (getter<ReturnType>::get(L, index, tracking)) {
                    return true;
                }

                handler(L, index, type::userdata, type_of(L, index), "value at this index does not properly reflect the desired type");
                return false;
            }
        };

        template <>
        struct getter<TRef<TEvent<bool>::Sink>> {
            typedef TRef<TEvent<bool>::Sink> ReturnType;
            static ReturnType get(lua_State* L, int index, record& tracking) {
                if (sol::stack::check_usertype<ReturnType>(L, index)) {
                    return getter<detail::as_value_tag<ReturnType>>{}.get(L, index, tracking);
                }
                if (sol::stack::check_usertype<TRef<SimpleModifiableValue<bool>>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<SimpleModifiableValue<bool>>>>{}.get(L, index, tracking);
                }
                return nullptr;
            }
        };

        template <>
        struct checker<TRef<TEvent<float>::Sink>, type::userdata> {
            typedef TRef<TEvent<float>::Sink> ReturnType;
            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                if (getter<ReturnType>::get(L, index, tracking)) {
                    return true;
                }

                handler(L, index, type::userdata, type_of(L, index), "value at this index does not properly reflect the desired type");
                return false;
            }
        };

        template <>
        struct getter<TRef<TEvent<float>::Sink>> {
            typedef TRef<TEvent<float>::Sink> ReturnType;
            static ReturnType get(lua_State* L, int index, record& tracking) {
                if (sol::stack::check_usertype<ReturnType>(L, index)) {
                    return getter<detail::as_value_tag<ReturnType>>{}.get(L, index, tracking);
                }
                if (sol::stack::check_usertype<TRef<SimpleModifiableValue<float>>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<SimpleModifiableValue<float>>>>{}.get(L, index, tracking);
                }
                return nullptr;
            }
        };

        template <>
        struct checker<TRef<TEvent<ZString>::Sink>, type::userdata> {
            typedef TRef<TEvent<ZString>::Sink> ReturnType;
            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                if (getter<ReturnType>::get(L, index, tracking)) {
                    return true;
                }

                handler(L, index, type::userdata, type_of(L, index), "value at this index does not properly reflect the desired type");
                return false;
            }
        };

        template <>
        struct getter<TRef<TEvent<ZString>::Sink>> {
            typedef TRef<TEvent<ZString>::Sink> ReturnType;
            static ReturnType get(lua_State* L, int index, record& tracking) {
                if (sol::stack::check_usertype<ReturnType>(L, index)) {
                    return getter<detail::as_value_tag<ReturnType>>{}.get(L, index, tracking);
                }
                if (sol::stack::check_usertype<TRef<SimpleModifiableValue<ZString>>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<SimpleModifiableValue<ZString>>>>{}.get(L, index, tracking);
                }
                return nullptr;
            }
        };

        template <>
        struct checker<TRef<TEvent<Point>::Sink>, type::userdata> {
            typedef TRef<TEvent<Point>::Sink> ReturnType;
            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                if (getter<ReturnType>::get(L, index, tracking)) {
                    return true;
                }

                handler(L, index, type::userdata, type_of(L, index), "value at this index does not properly reflect the desired type");
                return false;
            }
        };

        template <>
        struct getter<TRef<TEvent<Point>::Sink>> {
            typedef TRef<TEvent<Point>::Sink> ReturnType;
            static ReturnType get(lua_State* L, int index, record& tracking) {
                if (sol::stack::check_usertype<ReturnType>(L, index)) {
                    return getter<detail::as_value_tag<ReturnType>>{}.get(L, index, tracking);
                }
                if (sol::stack::check_usertype<TRef<SimpleModifiableValue<Point>>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<SimpleModifiableValue<Point>>>>{}.get(L, index, tracking);
                }
                return nullptr;
            }
        };

        //Image
        template <>
        struct checker<TRef<Image>> {
            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                if (getter<TRef<Image>>::get(L, index, tracking)) {
                    return true;
                }

                handler(L, index, type::userdata, type_of(L, index), "value at this index does not properly reflect the desired type");
                return false;
            }
        };

        template <>
        struct getter<TRef<Image>> {
            static TRef<Image> get(lua_State* L, int index, record& tracking) {
                if (sol::stack::check_usertype<TRef<Image>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<Image>>>{}.get(L, index, tracking);
                }
                if (sol::stack::check_usertype<TRef<ConstantImage>>(L, index)) {
                    return getter<detail::as_value_tag<TRef<ConstantImage>>>{}.get(L, index, tracking);
                }
                return nullptr;
            }
        };

        //list

        template <typename T>
        TRef<UiList<sol::object>> ConvertListTypeToGenericList(lua_State* L, TRef<UiList<T>> list) {
            return new MappedList<sol::object, T>(list, [L](T entry, TRef<Number> index) {
                return (sol::object)sol::make_object<T>(L, entry);
            });
        }

        template <>
        struct checker<TRef<UiList<sol::object>>, type::userdata> {
            typedef TRef<UiList<sol::object>> return_type;

            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                int absolute_index = lua_absindex(L, index);

                if (sol::stack::check_usertype<return_type>(L, absolute_index)) {
                    tracking.use(1);
                    return true;
                }
                if (stack::check_usertype<TRef<UiList<TRef<Image>>>>(L, absolute_index)) {
                    tracking.use(1);
                    return true;
                }
                if (stack::check_usertype<TRef<UiList<TRef<StringValue>>>>(L, absolute_index)) {
                    tracking.use(1);
                    return true;
                }
                if (stack::check_usertype<TRef<UiList<TRef<Boolean>>>>(L, absolute_index)) {
                    tracking.use(1);
                    return true;
                }
                if (stack::check_usertype<TRef<UiList<TRef<Number>>>>(L, absolute_index)) {
                    tracking.use(1);
                    return true;
                }
                if (stack::check_usertype<TRef<UiList<TRef<UiObjectContainer>>>>(L, absolute_index)) {
                    tracking.use(1);
                    return true;
                }
                handler(L, index, type::userdata, type::userdata, "Expected a list");
                return false;
            }
        };

        template <>
        struct getter<TRef<UiList<sol::object>>> {
            typedef TRef<UiList<sol::object>> return_type;

            static return_type get(lua_State* L, int index, record& tracking) {
                int absolute_index = lua_absindex(L, index);

                if (sol::stack::check_usertype<return_type>(L, absolute_index)) {
                    return getter<detail::as_value_tag<return_type>>{}.get(L, index, tracking);
                }
                if (stack::check_usertype<TRef<UiList<TRef<Image>>>>(L, absolute_index)) {
                    return ConvertListTypeToGenericList<TRef<Image>>(L, stack::get_usertype<TRef<UiList<TRef<Image>>>>(L, absolute_index, tracking));
                }
                if (stack::check_usertype<TRef<UiList<TRef<StringValue>>>>(L, absolute_index)) {
                    return ConvertListTypeToGenericList<TRef<StringValue>>(L, stack::get_usertype<TRef<UiList<TRef<StringValue>>>>(L, absolute_index, tracking));
                }
                if (stack::check_usertype<TRef<UiList<TRef<Number>>>>(L, absolute_index)) {
                    return ConvertListTypeToGenericList<TRef<Number>>(L, stack::get_usertype<TRef<UiList<TRef<Number>>>>(L, absolute_index, tracking));
                }
                if (stack::check_usertype<TRef<UiList<TRef<Boolean>>>>(L, absolute_index)) {
                    return ConvertListTypeToGenericList<TRef<Boolean>>(L, stack::get_usertype<TRef<UiList<TRef<Boolean>>>>(L, absolute_index, tracking));
                }
                if (stack::check_usertype<TRef<UiList<TRef<UiObjectContainer>>>>(L, absolute_index)) {
                    return ConvertListTypeToGenericList<TRef<UiObjectContainer>>(L, stack::get_usertype<TRef<UiList<TRef<UiObjectContainer>>>>(L, absolute_index, tracking));
                }

                throw std::runtime_error("Unknown type to cast");
            }
        };

        template <typename T>
        struct checker<TRef<UiList<TRef<T>>>, type::userdata> {
            typedef TRef<UiList<TRef<T>>> return_type;

            template <typename Handler>
            static bool check(lua_State* L, int index, Handler&& handler, record& tracking) {
                int absolute_index = lua_absindex(L, index);
                // Get the first element

                if (sol::stack::check_usertype<return_type>(L, absolute_index)) {
                    tracking.use(1);
                    return true;
                }

                // generic list container. We currently check entries of the initial value, not that useful maybe...
                if (sol::stack::check_usertype<TRef<UiList<sol::object>>>(L, absolute_index)) {
                    TRef<UiList<sol::object>> list = stack::get<TRef<UiList<sol::object>>>(L, absolute_index);
                    
                    for (sol::object entry : list->GetList()) {
                        if (entry.is<TRef<T>>() == false) {
                            handler(L, index, type::userdata, type::userdata, "Not all entries are of the correct type");
                            return false;
                        }
                    }
                    tracking.use(1);
                    return true;
                }

                //table
                record inner_tracking = record();
                if (stack::check<table>(L, absolute_index, sol::no_panic)) {
                    record inner_tracking = record();
                    table table_list = stack::get<table>(L, absolute_index, inner_tracking);

                    int count = table_list.size();

                    for (int i = 1; i <= count; ++i) {
                        sol::object entry = table_list.get<sol::object>(i);
                        if (entry.is<TRef<T>>() == false) {
                            handler(L, index, type::userdata, type::userdata, "Not all entries are of the correct type");
                            return false;
                        }
                    }

                    tracking.use(inner_tracking.used);
                    return true;
                }
                handler(L, index, type::userdata, type::userdata, "Either table or UiList");
                return false;
            }
        };

        template <typename T>
        struct getter<TRef<UiList<TRef<T>>>> {
            typedef TRef<UiList<TRef<T>>> return_type;

            static return_type get(lua_State* L, int index, record& tracking) {
                int absolute_index = lua_absindex(L, index);

                if (sol::stack::check_usertype<return_type>(L, absolute_index)) {
                    return getter<detail::as_value_tag<return_type>>{}.get(L, index, tracking);
                }

                // generic list container. Currently makes a copy of the list
                if (sol::stack::check_usertype<TRef<UiList<sol::object>>>(L, absolute_index)) {
                    TRef<UiList<sol::object>> list = stack::get_usertype<TRef<UiList<sol::object>>>(L, absolute_index, tracking);

                    return new MappedList<TRef<T>, sol::object>(list, [L](sol::object entry, TRef<Number> index) {
                        return entry.as<TRef<T>>();
                    });
                }

                //table. Makes a copy.
                if (stack::check<table>(L, absolute_index)) {
                    table table_list = stack::get<table>(L, absolute_index, tracking);

                    std::vector<TRef<T>> result_list = {};

                    int count = table_list.size();

                    for (int i = 1; i <= count; ++i) {
                        result_list.push_back(table_list.get<TRef<T>>(i));
                    }

                    return_type result = new UiList<TRef<T>>(result_list);
                    return result;
                }

                throw std::runtime_error("Unknown type to cast");
            }
        };
    }

}