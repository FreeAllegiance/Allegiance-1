
#pragma once

#include "value.h"
#include "event.h"
#include <functional>
#include <utility>

class NumberTransform {
public:
    static TRef<StringValue> ToString(Number* a, int decimals);
    static TRef<Boolean> Equals(Number* a, Number* b);

    static TRef<Number> Add(Number* a, Number* b);
    static TRef<Number> Subtract(Number* a, Number* b);
    static TRef<Number> Divide(Number* a, Number* b);
    static TRef<Number> Multiply(Number* a, Number* b);
    static TRef<Number> Min(Number* a, Number* b);
    static TRef<Number> Max(Number* a, Number* b);
    static TRef<Number> Clamp(Number* a, Number* b, Number* c);
    static TRef<Number> Mod(Number* a, Number* b);

    static TRef<Number> Round(Number* a, int decimals);
    static TRef<Number> Sin(Number* a);
    static TRef<Number> Cos(Number* a);
    static TRef<Number> Sqrt(Number* a);
    static TRef<Number> Power(Number* a, Number* power);
};

class PointTransform {
public:
    static TRef<PointValue> Create(Number* a, Number* b);

    static TRef<Number> X(PointValue* point);
    static TRef<Number> Y(PointValue* point);

    static TRef<PointValue> Scale(PointValue* a, PointValue* b);
};

class ColorTransform {
public:
    static TRef<ColorValue> Create(Number* r, Number* g, Number* b, Number* a);
};

class RectTransform {
public:
    static TRef<RectValue> Create(Number* xmin, Number* ymin, Number* xmax, Number* ymax);
    static TRef<RectValue> Create(Number* width, Number* height);
    static TRef<RectValue> Create(PointValue* pPoint);

    static TRef<Number> XMin(RectValue* pRect);
    static TRef<Number> YMin(RectValue* pRect);
    static TRef<Number> XMax(RectValue* pRect);
    static TRef<Number> YMax(RectValue* pRect);
};

class BooleanTransform {
public:
    static TRef<Boolean> Equals(Boolean* a, Boolean* b);

    static TRef<Boolean> And(Boolean* a, Boolean* b);
    static TRef<Boolean> Or(Boolean* a, Boolean* b);
    static TRef<Boolean> Not(Boolean* a);

    static TRef<Number> ToNumber(Boolean* a);
    static TRef<Number> Count(std::vector<TRef<Boolean>> a);
};

class StringTransform {
public:
    static TRef<Number> ToNumber(StringValue* string);
    static TRef<Boolean> IsNumber(StringValue* string);

    static TRef<Number> Length(StringValue* a);
    static TRef<Boolean> Equals(StringValue* a, StringValue* b);

    static TRef<StringValue> Concat(StringValue* a, StringValue* b);
    static TRef<StringValue> Slice(StringValue* string, Number* start, Number* length);
};

template<typename... Types>
class CallbackWhenChanged : public Value {

    std::function<void(Types...)> m_callback;

public:
    CallbackWhenChanged(std::function<void(Types...)> callback, TStaticValue<Types>*... values) :
        m_callback(callback),
        Value(values...)
        //Value(std::forward<TStaticValue<Types>*>(values)...)
    {}

    template<int... Indices>
    void GetEvaluatedValue(std::index_sequence<Indices...>) {
        m_callback(((TStaticValue<Types>*)GetChild(Indices))->GetValue()...);
    }

    void Evaluate() override
    {
        GetEvaluatedValue(std::make_index_sequence<sizeof...(Types)>{});
    }
};

template<class TransformedType, class... Types>
class TransformedValue : public TStaticValue<TransformedType> {

    std::function<TransformedType(Types...)> m_callback;

protected:
    template<int... Indices>
    TransformedType GetEvaluatedValue(std::index_sequence<Indices...>) {
        return m_callback(((TStaticValue<Types>*)GetChild(Indices))->GetValue()...);
    }

public:
    TransformedValue(std::function<TransformedType(Types...)> callback, TStaticValue<Types>*... values) :
        m_callback(callback),
        TStaticValue<TransformedType>(values...)
    {}

    void Evaluate() override
    {
        TransformedType evaluated = GetEvaluatedValue(std::make_index_sequence<sizeof...(Types)>{});

        GetValueInternal() = evaluated;
    }
};

template <typename Type>
class SimpleModifiableValue : public TStaticValue<Type>, public TEvent<Type>::Sink {
public:
    SimpleModifiableValue(const Type& value) :
        TStaticValue<Type>(value)
    {
    }

    void SetValue(const Type& value) {
        TStaticValue<Type>::GetValueInternal() = value;
        TStaticValue<Type>::Changed();
    }

    bool OnEvent(typename TEvent<Type>::Source* pevent, Type value) override {
        SetValue(value);
        return true;
    }
};

template <typename TypeResult, typename TypeEntry>
class VectorTransformValue : public TStaticValue<TypeResult>
{
    typedef TRef<TStaticValue<TypeEntry>> TypeVectorEntry;
    typedef std::function<TypeResult(const std::vector<TypeEntry>&)> CallbackType;

    CallbackType m_callback;
    int m_count;

public:
    VectorTransformValue(CallbackType callback, std::vector<TypeVectorEntry> vector) :
        m_callback(callback),
        m_count(vector.size()),
        TStaticValue()
    {
        
        for (std::vector<TypeVectorEntry>::iterator it = vector.begin(); it != vector.end(); ++it) {
            TypeVectorEntry entry = *it;
            AddChild(entry);
        }
    }

    void Evaluate()
    {
        std::vector<TypeEntry> temp = {};
        for (int i = 0; i < m_count; ++i) {
            TypeEntry value = ((TStaticValue<TypeEntry>*)GetChild(i))->GetValue();
            temp.push_back(value);
        }

        TypeResult evaluated = m_callback(temp);

        GetValueInternal() = evaluated;
    }


};