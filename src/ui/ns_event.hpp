
#pragma once

#include "ui.h"
#include "items.hpp"

class EventToBoolean : public ModifiableBoolean, IEventSink {
    TRef<IEventSource> m_pEnableSource;
    TRef<IEventSource> m_pDisableSource;

public:
    EventToBoolean(const TRef<IEventSource>& pEnableSource, const TRef<IEventSource>& pDisableSource) :
        m_pEnableSource(pEnableSource),
        m_pDisableSource(pDisableSource),
        ModifiableBoolean(false)
    {
        pEnableSource->AddSink(this);
        pDisableSource->AddSink(this);
    }

    ~EventToBoolean() {
        m_pEnableSource->RemoveSink(this);
        m_pDisableSource->RemoveSink(this);
    }

    bool OnEvent(IEventSource* source) {
        if (source == m_pEnableSource) {
            SetValue(true);
        }
        else if (source == m_pDisableSource) {
            SetValue(false);
        }
        else {
            ZAssert(false);
        }
        return true;
    }
};

template<typename TypeResult>
class EventToMappedValue : public TWrapValue<TypeResult>, IEventSink {
    typedef TRef<TStaticValue<TypeResult>> TypeWrappedResult;

    std::map<TRef<IEventSource>, TypeWrappedResult> m_mapOptions;

public:
    EventToMappedValue(TypeWrappedResult tDefault, std::map<TRef<IEventSource>, TypeWrappedResult> mapOptions) :
        m_mapOptions(mapOptions),
        TWrapValue(tDefault)
    {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->AddSink(this);
        }
    }

    ~EventToMappedValue() {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->RemoveSink(this);
        }
    }

    bool OnEvent(IEventSource* source) {
        auto find = m_mapOptions.find(source);
        if (find == m_mapOptions.end()) {
            ZAssert(false);
        }
        else {
            SetWrappedValue(find->second);
        }
        return true;
    }
};

class EventToMappedImage : public WrapImage, IEventSink {
    typedef TRef<Image> TypeWrappedResult;

    std::map<TRef<IEventSource>, TypeWrappedResult> m_mapOptions;

public:
    EventToMappedImage(const TRef<Image>& pImage, std::map<TRef<IEventSource>, TypeWrappedResult> mapOptions) :
        m_mapOptions(mapOptions),
        WrapImage(pImage)
    {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->AddSink(this);
        }
    }

    ~EventToMappedImage() {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->RemoveSink(this);
        }
    }

    bool OnEvent(IEventSource* source) {
        auto find = m_mapOptions.find(source);
        if (find == m_mapOptions.end()) {
            ZAssert(false);
        }
        else {
            SetImage(find->second);
        }
        return true;
    }
};

template <typename TypeResult, typename TypeOriginal>
class EventOneToOne {
    typedef TRef<TStaticValue<TypeResult>> WrappedResult;
    typedef TRef<TStaticValue<TypeOriginal>> WrappedOriginal;

public:

    static auto CreateOnEventPropagatorFunction() {


        return [](const TRef<TEvent<TypeResult>::Sink>& pEventSink, const TRef<TEvent<TypeOriginal>::Source>& pEventSource, std::function<sol::object(WrappedOriginal)> transformer) {
            TypeOriginal original_default_value;
            TRef<SimpleModifiableValue<TypeOriginal>> start = new SimpleModifiableValue<TypeOriginal>(original_default_value);
            WrappedResult end = wrapValue<TypeResult>(transformer(start));

            TRef<TEvent<TypeResult>::Sink> sinkRefCounted = pEventSink;

            pEventSource->AddSink(new CallbackValueSink<TypeOriginal>([start, end, sinkRefCounted](TypeOriginal value) {
                start->SetValue(value);
                end->Update();
                sinkRefCounted->OnEvent(nullptr, end->GetValue());
                return true;
            }));
        };
    }
};

template <typename Type>
auto CreateSourceFilteringFunction() {
    typedef TEvent<Type> Event;

    return [](const TRef<Event::Source>& source, const TRef<Boolean>& active) {
        //this should be optimized so that the boolean value switches the link on off instead of filtering each entry
        //memory leak here when the source isn't cleaned up

        TRef<Event::SourceImpl> result = new Event::SourceImpl();
        source->AddSink(new CallbackValueSink<Type>([result, active](Type value) {
            if (active->GetValue()) {
                result->Trigger(value);
            }
            return true;
        }));

        return (TRef<Event::Source>)result;
    };
}

template <typename TypeResult>
class EventVoidToOne {
    typedef TRef<TStaticValue<TypeResult>> WrappedResult;

public:
    static auto CreateOnEventPropagatorFunction() {

        return [](const TRef<TEvent<TypeResult>::Sink>& pEventSink, const TRef<IEventSource>& pEventSource, std::function<sol::object()> transformer) {
            WrappedResult end = wrapValue<TypeResult>(transformer());

            TRef<TEvent<TypeResult>::Sink> sinkRefCounted = pEventSink;

            pEventSource->AddSink(new CallbackSink([end, sinkRefCounted]() {
                end->Update();
                sinkRefCounted->OnEvent(nullptr, end->GetValue());
                return true;
            }));
        };
    }
};

template <typename ... Types>
class EventVoidToMultiple {
    typedef ZString Type;
    typedef TRef<TStaticValue<Type>> WrappedType;
    typedef TEvent<Types...> EventType;

    typedef std::tuple<sol::object, sol::object, sol::object> TupleType;

public:

    static auto CreateOnEventPropagatorFunction() {


        return [](const TRef<EventType::Sink>& pEventSink, const TRef<IEventSource>& pEventSource, std::function<TupleType()> transformer) {

            WrappedType a, b, c;

            sol::object object_a, object_b, object_c;

            TupleType tuple = transformer();

            a = wrapValue<Type>(std::get<0>(tuple));
            b = wrapValue<Type>(std::get<1>(tuple));
            c = wrapValue<Type>(std::get<2>(tuple));

            TRef<EventType::Sink> sinkRefCounted = pEventSink;

            pEventSource->AddSink(new CallbackSink([a, b, c, sinkRefCounted]() {
                a->Update();
                b->Update();
                c->Update();
                sinkRefCounted->OnEvent(nullptr, a->GetValue(), b->GetValue(), c->GetValue());
                return true;
            }));
        };
    }
};

class EventNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["OnEvent"] = sol::overload(
            [](const TRef<IEventSink>& pEventSink, const TRef<IEventSource>& pEventSource) {
                pEventSource->AddSink(pEventSink);
            },
            EventOneToOne<bool, bool>::CreateOnEventPropagatorFunction(),
            EventOneToOne<bool, float>::CreateOnEventPropagatorFunction(),
            EventOneToOne<bool, Point>::CreateOnEventPropagatorFunction(),
            EventOneToOne<bool, ZString>::CreateOnEventPropagatorFunction(),
            EventOneToOne<float, bool>::CreateOnEventPropagatorFunction(),
            EventOneToOne<float, float>::CreateOnEventPropagatorFunction(),
            EventOneToOne<float, Point>::CreateOnEventPropagatorFunction(),
            EventOneToOne<float, ZString>::CreateOnEventPropagatorFunction(),
            EventOneToOne<Point, bool>::CreateOnEventPropagatorFunction(),
            EventOneToOne<Point, float>::CreateOnEventPropagatorFunction(),
            EventOneToOne<Point, Point>::CreateOnEventPropagatorFunction(),
            EventOneToOne<Point, ZString>::CreateOnEventPropagatorFunction(),
            EventOneToOne<ZString, bool>::CreateOnEventPropagatorFunction(),
            EventOneToOne<ZString, float>::CreateOnEventPropagatorFunction(),
            EventOneToOne<ZString, Point>::CreateOnEventPropagatorFunction(),
            EventOneToOne<ZString, ZString>::CreateOnEventPropagatorFunction(),
            EventVoidToOne<bool>::CreateOnEventPropagatorFunction(),
            EventVoidToOne<float>::CreateOnEventPropagatorFunction(),
            EventVoidToOne<Point>::CreateOnEventPropagatorFunction(),
            EventVoidToOne<ZString>::CreateOnEventPropagatorFunction(),
            EventVoidToMultiple<ZString, ZString, ZString>::CreateOnEventPropagatorFunction()
        );

        table["Get"] = [](const TRef<Image>& image, std::string string) {
            MouseEventImage* ptr = (MouseEventImage*)(Image*)image;
            return ptr->GetEventSource(string);
        };

        table["GetPoint"] = [](const TRef<Image>& image, std::string string) {
            MouseEventImage* ptr = (MouseEventImage*)(Image*)image;
            return ptr->GetPointEventSource(string);
        };

        table["Filter"] = sol::overload(
            [](const TRef<IEventSource>& pEventSource, const TRef<Boolean>& active) {
                //this should be optimized so that the boolean value switches the link on off instead of filtering each entry
                //memory leak here when the source isn't cleaned up

                TRef<EventSourceImpl> result = new EventSourceImpl();
                pEventSource->AddSink(new CallbackSink([result, active]() {
                    if (active->GetValue()) {
                        result->Trigger();
                    }
                    return true;
                }));

                return (TRef<IEventSource>)result;
            },
            CreateSourceFilteringFunction<float>(),
            CreateSourceFilteringFunction<bool>(),
            CreateSourceFilteringFunction<ZString>(),
            CreateSourceFilteringFunction<Point>()
        );

        table["ToBoolean"] = [](sol::table table, sol::object valueDefault) {
            std::map<TRef<IEventSource>, TRef<Boolean>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<const TRef<IEventSource>&>();
                mapOptions[mapKey] = wrapValue<bool>(value);
            });
            
            return (TRef<Boolean>)new EventToMappedValue<bool>(wrapValue<bool>(valueDefault), mapOptions);
        };

        table["ToNumber"] = [](sol::table table, sol::object valueDefault) {
            std::map<TRef<IEventSource>, TRef<Number>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<const TRef<IEventSource>&>();
                mapOptions[mapKey] = wrapValue<float>(value);
            });

            return (TRef<Number>)new EventToMappedValue<float>(wrapValue<float>(valueDefault), mapOptions);
        };

        table["ToString"] = [](sol::table table, sol::object valueDefault) {
            std::map<TRef<IEventSource>, TRef<StringValue>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<const TRef<IEventSource>&>();
                mapOptions[mapKey] = wrapString(value);
            });

            return (TRef<StringValue>)new EventToMappedValue<ZString>(wrapString(valueDefault), mapOptions);
        };

        table["ToImage"] = [](sol::table table, const TRef<Image>& valueDefault) {
            std::map<TRef<IEventSource>, TRef<Image>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<const TRef<IEventSource>&>();
                mapOptions[mapKey] = value.as<const TRef<Image>&>();
            });

            return (TRef<Image>)new EventToMappedImage(valueDefault, mapOptions);
        };

        m_pLua->set("Event", table);
    }
};