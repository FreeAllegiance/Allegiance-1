
#include "valuetransform.h"


template <typename Type>
class EqualsTransform : public TransformedValue2<bool, Type, Type> {
    typedef TRef<TStaticValue<Type>> WrappedType;

public:
    EqualsTransform(WrappedType const& a, WrappedType const& b) :
        TransformedValue2([](ZString a, ZString b) {
        return a == b;
    }, a, b)
    {
    }
};

class SubtractNumber : public Number {
public:
    SubtractNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() - Get1()->GetValue();
    }
};

class AddNumber : public Number {
public:
    AddNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() + Get1()->GetValue();
    }
};

class MultiplyNumber : public Number {
public:
    MultiplyNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() * Get1()->GetValue();
    }
};

class DivideNumber : public Number {
public:
    DivideNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() / Get1()->GetValue();
    }
};

class SinNumber : public Number {
public:
    SinNumber(Number* pvalue0) :
        Number(pvalue0)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }

    void Evaluate()
    {
        GetValueInternal() = sin(Get0()->GetValue());
    }
};

class CosNumber : public Number {
public:
    CosNumber(Number* pvalue0) :
        Number(pvalue0)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }

    void Evaluate()
    {
        GetValueInternal() = cos(Get0()->GetValue());
    }
};

class ModNumber : public Number {
public:
    ModNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = mod(Get0()->GetValue(), Get1()->GetValue());
    }
};

class MinNumber : public Number {
public:
    MinNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = std::min(Get0()->GetValue(), Get1()->GetValue());
    }
};

class MaxNumber : public Number {
public:
    MaxNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = std::max(Get0()->GetValue(), Get1()->GetValue());
    }
};

TRef<StringValue> NumberTransform::ToString(Number* pNumber, int decimals)
{
    return new TransformedValue<ZString, float>([decimals](float a) {
        float multiplier = pow(10, decimals);

        int output_decimals = decimals >= 0 ? decimals : 0;

        std::string s(16, '\0');
        std::string format = std::string("%.") + std::to_string(output_decimals) + "f";
        auto written = std::snprintf(&s[0], s.size(), format.c_str(), round(a * multiplier) / multiplier);
        s.resize(written);

        //return ZString(format.c_str());
        return ZString(s.c_str());
        //return round(a * multiplier) / multiplier;
    }, pNumber);
}

TRef<Boolean> NumberTransform::Equals(Number* a, Number* b)
{
    return new EqualsTransform<float>(a, b);
}

TRef<Number> NumberTransform::Round(Number* pNumber, int decimals)
{
    return new TransformedValue<float, float>([decimals](float a) {
        float multiplier = pow(10, decimals);
        return round(a * multiplier) / multiplier;
    }, pNumber);
}

TRef<Number> NumberTransform::Subtract(Number* pvalue1, Number* pvalue2)
{
    return new SubtractNumber(pvalue1, pvalue2);
}

TRef<Number> NumberTransform::Add(Number* pvalue1, Number* pvalue2)
{
    return new AddNumber(pvalue1, pvalue2);
}

TRef<Number> NumberTransform::Multiply(Number* pvalue1, Number* pvalue2)
{
    return new MultiplyNumber(pvalue1, pvalue2);
}

TRef<Number> NumberTransform::Divide(Number* pvalue1, Number* pvalue2)
{
    return new DivideNumber(pvalue1, pvalue2);
}

TRef<Number> NumberTransform::Mod(Number* pvalue1, Number* pvalue2)
{
    return new ModNumber(pvalue1, pvalue2);
}

TRef<Number> NumberTransform::Min(Number* pvalue1, Number* pvalue2)
{
    return new MinNumber(pvalue1, pvalue2);
}

TRef<Number> NumberTransform::Max(Number* pvalue1, Number* pvalue2)
{
    return new MaxNumber(pvalue1, pvalue2);
}

TRef<Number> NumberTransform::Sin(Number* pvalue)
{
    return new SinNumber(pvalue);
}

TRef<Number> NumberTransform::Cos(Number* pvalue)
{
    return new CosNumber(pvalue);
};

TRef<Number> NumberTransform::Sqrt(Number* pvalue)
{
    return new TransformedValue<float, float>([](float value) {
        return sqrt(value);
    }, pvalue);
};

TRef<Number> NumberTransform::Power(Number* pvalue, Number* power)
{
    return new TransformedValue2<float, float, float>([](float value, float power) {
        return pow(value, power);
    }, pvalue, power);
};


// ### Point ###

class PointV : public PointValue {
public:
    PointV(Number* px, Number* py) :
        PointValue(px, py)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
            Point(
                Get0()->GetValue(),
                Get1()->GetValue()
            );
    }
};

class PointX : public Number {
private:
    PointValue* GetPoint() { return PointValue::Cast(GetChild(0)); }

public:
    PointX(PointValue* ppoint) :
        Number(ppoint)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = GetPoint()->GetValue().X();
    }
};

class PointY : public Number {
private:
    PointValue* GetPoint() { return PointValue::Cast(GetChild(0)); }

public:
    PointY(PointValue* ppoint) :
        Number(ppoint)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = GetPoint()->GetValue().Y();
    }
};

TRef<PointValue> PointTransform::Create(Number* x, Number* y) {
    return (PointValue*)new PointV(
        x,
        y
    );
};

TRef<Number> PointTransform::X(PointValue* ppoint) {
    return new PointX(ppoint);
};

TRef<Number> PointTransform::Y(PointValue* ppoint) {
    return new PointY(ppoint);
};

TRef<PointValue> PointTransform::Scale(PointValue* a, PointValue* b) {
    return new TransformedValue2<Point, Point, Point>([](Point a, Point b) {
        return Point(a.X() * b.X(), a.Y() * b.Y());
    }, a, b);
};


// ### Color

TRef<ColorValue> ColorTransform::Create(Number* r, Number* g, Number* b, Number* a) {
    return new TransformedValue4<Color, float, float, float, float>(
        [](float r, float g, float b, float a) {
            return Color(r, g, b, a);
        },
        r,
        g,
        b,
        a
    );
};


// ### Rect

TRef<RectValue> RectTransform::Create(Number* xmin, Number* ymin, Number* xmax, Number* ymax) {
    return new TransformedValue4<Rect, float, float, float, float>(
        [](float xmin, float ymin, float xmax, float ymax) {
            return Rect(xmin, ymin, xmax, ymax);
        },
        xmin,
        ymin,
        xmax,
        ymax
    );
};

TRef<RectValue> RectTransform::Create(Number* width, Number* height) {
    return new TransformedValue2<Rect, float, float>(
        [](float width, float height) {
            return Rect(0, 0, width, height);
        },
        width,
        height
    );
};

TRef<RectValue> RectTransform::Create(PointValue* pPoint) {
    return new TransformedValue<Rect, Point>(
        [](Point point) {
            return Rect(0, 0, point.X(), point.Y());
        },
        pPoint
    );
};

TRef<Number> RectTransform::XMin(RectValue* pRect) {
    return new TransformedValue<float, Rect>(
        [](Rect rect) {
            return rect.XMin();
        },
        pRect
    );
};

TRef<Number> RectTransform::YMin(RectValue* pRect) {
    return new TransformedValue<float, Rect>(
        [](Rect rect) {
            return rect.YMin();
        },
        pRect
    );
};

TRef<Number> RectTransform::XMax(RectValue* pRect) {
    return new TransformedValue<float, Rect>(
        [](Rect rect) {
            return rect.XMax();
        },
        pRect
    );
};

TRef<Number> RectTransform::YMax(RectValue* pRect) {
    return new TransformedValue<float, Rect>(
        [](Rect rect) {
            return rect.YMax();
        },
        pRect
    );
};

// ### Boolean

class AndBoolean : public Boolean {
public:
    AndBoolean(Boolean* pvalue0, Boolean* pvalue1) :
        Boolean(pvalue0, pvalue1)
    {
    }

    Boolean* Get0() { return Boolean::Cast(GetChild(0)); }
    Boolean* Get1() { return Boolean::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
            Get0()->GetValue()
            && Get1()->GetValue();
    }
};

class OrBoolean : public Boolean {
public:
    OrBoolean(Boolean* pvalue0, Boolean* pvalue1) :
        Boolean(pvalue0, pvalue1)
    {
    }

    Boolean* Get0() { return Boolean::Cast(GetChild(0)); }
    Boolean* Get1() { return Boolean::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
            Get0()->GetValue()
            || Get1()->GetValue();
    }
};

class NotBoolean : public Boolean {
public:
    NotBoolean(Boolean* pvalue0) :
        Boolean(pvalue0)
    {
    }

    Boolean* Get0() { return Boolean::Cast(GetChild(0)); }

    void Evaluate()
    {
        GetValueInternal() = !(Get0()->GetValue());
    }
};

TRef<Boolean> BooleanTransform::Equals(Boolean* a, Boolean* b)
{
    return new EqualsTransform<bool>(a, b);
}

TRef<Boolean> BooleanTransform::And(Boolean* pvalue1, Boolean* pvalue2)
{
    return new AndBoolean(pvalue1, pvalue2);
}

TRef<Boolean> BooleanTransform::Or(Boolean* pvalue1, Boolean* pvalue2)
{
    return new OrBoolean(pvalue1, pvalue2);
}

TRef<Boolean> BooleanTransform::Not(Boolean* pvalue1)
{
    return new NotBoolean(pvalue1);
}

TRef<Number> BooleanTransform::ToNumber(Boolean* pvalue1)
{
    return new TransformedValue<float, bool>([](bool bValue) {
        return bValue ? 1.0f : 0.0f;
    }, pvalue1);
}

TRef<Number> BooleanTransform::Count(std::vector<TRef<Boolean>> a)
{
    return new VectorTransformValue<float, bool>([](const std::vector<bool>& vector) {
        int count = 0;
        for (bool b : vector) {
            if (b) {
                ++count;
            }
        }
        return (float)count;
    }, a);
}

// ### String

class ConcatenatedString : public StringValue {
public:
    ConcatenatedString(StringValue* pvalue1, StringValue* pvalue2) :
        StringValue(pvalue1, pvalue2)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = ((StringValue*)GetChild(0))->GetValue()
            + ((StringValue*)GetChild(1))->GetValue();
    }

};

TRef<Number> StringTransform::Length(StringValue* a)
{
    return (TRef<Number>)new TransformedValue<float, ZString>([](ZString str) {
        return (float)str.GetLength();
    }, a);
}

TRef<Boolean> StringTransform::Equals(StringValue* a, StringValue* b)
{
    return new EqualsTransform<ZString>(a, b);
}

TRef<StringValue> StringTransform::Concat(StringValue* a, StringValue* b)
{
    return new ConcatenatedString(a, b);
}

TRef<StringValue> StringTransform::Slice(StringValue* string, Number* start, Number* length)
{
    return new TransformedValue3<ZString, ZString, float, float>([](ZString string, float fstart, float flength) {
        int start = (int)fstart;
        int length = (int)flength;

        int string_length = string.GetLength();

        if (start < 0) {
            //negative start values means from end of string
            start = string_length + start;
        }

        if (length < 0) {
            //negative length values means up to the last n characters
            length = string_length + length;
        }

        //validation
        if (start < 0) {
            start = 0;
        }

        if (start >= string_length) {
            return ZString("");
        }

        if (length <= 0) {
            return ZString("");
        }

        if (length > string_length - start) {
            length = string_length - start;
        }

        ZString strSlice = string.Middle(start, length);
        return strSlice;
    }, string, start, length);
}