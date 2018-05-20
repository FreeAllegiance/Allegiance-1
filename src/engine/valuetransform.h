
#pragma once

#include "value.h"
#include "event.h"
#include <functional>

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
    static TRef<Number> Length(StringValue* a);
    static TRef<Boolean> Equals(StringValue* a, StringValue* b);

    static TRef<StringValue> Concat(StringValue* a, StringValue* b);
    static TRef<StringValue> Slice(StringValue* string, Number* start, Number* length);
};

template<class TransformedType, class OriginalType>
class TransformedValue : public TStaticValue<TransformedType> {

    std::function<TransformedType(OriginalType)> m_callback;

protected:
    TransformedType GetEvaluatedValue(OriginalType value) {
        return m_callback(value);
    }

public:
    TransformedValue(std::function<TransformedType(OriginalType)> callback, TStaticValue<OriginalType>* value) :
        m_callback(callback),
        TStaticValue(value)
    {}

    void Evaluate()
    {
        OriginalType value = ((TStaticValue<OriginalType>*)GetChild(0))->GetValue();

        TransformedType evaluated = GetEvaluatedValue(value);

        GetValueInternal() = evaluated;
    }
};

template<class TransformedType, class OriginalType, class OriginalType2>
class TransformedValue2 : public TStaticValue<TransformedType> {
    typedef std::function<TransformedType(OriginalType, OriginalType2)> CallbackType;
    CallbackType m_callback;

protected:
    TransformedType GetEvaluatedValue(OriginalType value, OriginalType2 value2) {
        return m_callback(value, value2);
    }

public:
    TransformedValue2(CallbackType callback, TStaticValue<OriginalType>* value, TStaticValue<OriginalType2>* value2) :
        m_callback(callback),
        TStaticValue(value, value2)
    {}

    void Evaluate()
    {
        OriginalType value = ((TStaticValue<OriginalType>*)GetChild(0))->GetValue();
        OriginalType2 value2 = ((TStaticValue<OriginalType2>*)GetChild(1))->GetValue();

        TransformedType evaluated = GetEvaluatedValue(value, value2);

        GetValueInternal() = evaluated;
    }
};

template<class TransformedType, class OriginalType, class OriginalType2, class OriginalType3>
class TransformedValue3 : public TStaticValue<TransformedType> {
    typedef std::function<TransformedType(OriginalType, OriginalType2, OriginalType3)> CallbackType;
    CallbackType m_callback;

public:
    TransformedValue3(CallbackType callback, TStaticValue<OriginalType>* value, TStaticValue<OriginalType2>* value2, TStaticValue<OriginalType3>* value3) :
        m_callback(callback),
        TStaticValue(value, value2, value3)
    {}

    void Evaluate()
    {
        OriginalType value = ((TStaticValue<OriginalType>*)GetChild(0))->GetValue();
        OriginalType2 value2 = ((TStaticValue<OriginalType2>*)GetChild(1))->GetValue();
        OriginalType3 value3 = ((TStaticValue<OriginalType3>*)GetChild(2))->GetValue();

        TransformedType evaluated = m_callback(value, value2, value3);

        GetValueInternal() = evaluated;
    }
};

template<class TransformedType, class OriginalType, class OriginalType2, class OriginalType3, class OriginalType4>
class TransformedValue4 : public TStaticValue<TransformedType> {
    typedef std::function<TransformedType(OriginalType, OriginalType2, OriginalType3, OriginalType4)> CallbackType;
    CallbackType m_callback;

public:
    TransformedValue4(CallbackType callback, TStaticValue<OriginalType>* value, TStaticValue<OriginalType2>* value2, TStaticValue<OriginalType3>* value3, TStaticValue<OriginalType4>* value4) :
        m_callback(callback),
        TStaticValue(value, value2, value3, value4)
    {}

    void Evaluate()
    {
        OriginalType value = ((TStaticValue<OriginalType>*)GetChild(0))->GetValue();
        OriginalType2 value2 = ((TStaticValue<OriginalType2>*)GetChild(1))->GetValue();
        OriginalType3 value3 = ((TStaticValue<OriginalType3>*)GetChild(2))->GetValue();
        OriginalType4 value4 = ((TStaticValue<OriginalType4>*)GetChild(3))->GetValue();

        TransformedType evaluated = m_callback(value, value2, value3, value4);

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