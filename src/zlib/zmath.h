//////////////////////////////////////////////////////////////////////////////
//
// floating point math functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _zmath_H_
#define _zmath_H_

#include <cmath>
#include <cstdint>

#include "float.h"

//////////////////////////////////////////////////////////////////////////////
//
// Constants
//
//////////////////////////////////////////////////////////////////////////////

const float pi    = 3.14159265359f;
const float sqrt2 = 1.41421356237f;

//  , fix these

const int c_maxInt = 0x7fffffff;
const int c_minInt = 0x80000000;

inline const float RadiansFromDegrees(float value) { return value * pi / 180.0f; }
inline const float DegreesFromRadians(float value) { return value * 180.0f / pi; }

//////////////////////////////////////////////////////////////////////////////
//
// Handy function for getting the next larger power of 2
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// Return the next larger multiple of size.  
//
//////////////////////////////////////////////////////////////////////////////

inline int NextMultipleOf(int size, int value)
{
    return ((value + size - 1) / size) * size;
}

//////////////////////////////////////////////////////////////////////////////
//
// Handy bit manipulation functions
//
//////////////////////////////////////////////////////////////////////////////

uint32_t CountBits(uint32_t dw);
uint32_t GetShift(uint32_t dw);
uint32_t NextPowerOf2(uint32_t x);

inline uint32_t PowerOf2(uint32_t exponent)
{
    return 1 << exponent;
}

inline uint32_t MakeMask(int bits, int shift)
{
    return (PowerOf2(bits) - 1) << shift;
}

//////////////////////////////////////////////////////////////////////////////
//
// functions
//
//////////////////////////////////////////////////////////////////////////////

inline bool odd(int value) 
{ 
    return value >= 0 ? value & 1 : !(value & 1);
}

template<class ValueType>
inline ValueType bound(ValueType value, ValueType min, ValueType max)
{
    return value < min ? min : (value > max ? max : value);
}

// KG- renamed swap to Swap to avoid conflict issue with VC9SP1 and future releases
template<class Type>
inline void Swap(Type& x, Type& y)
{
    Type temp(x);
    x = y;
    y = temp;
}

//////////////////////////////////////////////////////////////////////////////
//
// float functions
//
//////////////////////////////////////////////////////////////////////////////

template<class ValueType>
inline ValueType sign(ValueType x) { return x >= (ValueType)0 ? (ValueType)1 : (ValueType)-1; }

// VS.Net 2003 port
#if _MSC_VER < 1310
inline float abs(float x)              { return x < 0 ? -x : x;        }
#endif
inline float mod(float x, float limit) 
{ 
    if (limit == 0) {
        return 0;
    } else {
        return (float)fmod(x, limit); 
    }
}

// VS.Net 2003 port
#if _MSC_VER < 1310
    inline float floor(float x)            { return floorf(x);             }
    inline float pow(float x, float y)     { return powf(x, y);            }
    inline float cos(float x)              { return cosf(x);               }
    inline float sin(float x)              { return sinf(x);               }
    inline float tan(float x)              { return tanf(x);               }
    inline float atan(float x)             { return atanf(x);              }
    inline float atan2(float x, float y)   { return atan2f(x, y);          }
    inline float log(float x)              { return logf(x);               }

    inline float sqrt(float x)
    {
        ZAssert(x >= 0);
        return sqrtf(x);
    }

    inline float acos(float x)
    {
        ZAssert(x >= -1 && x <= 1);
        return acosf(x);
    }

    inline float asin(float x)
    {
        ZAssert(x >= -1 && x <= 1);
        return asinf(x);
    }
#endif


inline float random(float min, float max)
{
    return (((float)rand()) / RAND_MAX) * (max * (1.0f - FLT_EPSILON) - min) + min;
}

//////////////////////////////////////////////////////////////////////////////
//
// Interger functions
//
//////////////////////////////////////////////////////////////////////////////

inline int randomInt(int min, int max)
{
    return min + (rand() % (1 + max - min));
}

//////////////////////////////////////////////////////////////////////////////
//
// Interpolation
//
//////////////////////////////////////////////////////////////////////////////

inline float SmoothInterpolant(float value)
{
    return 0.5f - 0.5f * cos(pi * value);
}

template<class Type>
inline Type Interpolate(Type v1, Type v2, float value)
{
    return ((1 - value) * v1) + (value * v2);
}

//////////////////////////////////////////////////////////////////////////////
//
// TRange
//
//////////////////////////////////////////////////////////////////////////////

template<int count>
class TRange {
private:
    int m_value;

    void Validate()
    {
        while (m_value <  0    ) m_value += count;
        while (m_value >= count) m_value -= count;
    }

public:
    TRange(int value) :
        m_value(value)
    {
        Validate();
    }

    operator int()
    {
        return m_value;
    }

    TRange& operator=(int value)
    {
        m_value = value;
        Validate();
        return *this;
    }

    TRange operator++()
    {
        TRange old = *this;
        m_value++;
        Validate();
        return old;
    }

    TRange operator--()
    {
        TRange old = *this;
        m_value--;
        Validate();
        return old;
    }

    TRange& operator++(int)
    {
        m_value++;
        Validate();
        return *this;
    }

    TRange& operator--(int)
    {
        m_value--;
        Validate();
        return *this;
    }

    TRange& operator+=(int value)
    {
        m_value += value;
        Validate();
        return *this;
    }

    TRange& operator-=(int value)
    {
        m_value -= value;
        Validate();
        return *this;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// A function object which maps from {} -> {}
//
//////////////////////////////////////////////////////////////////////////////

class NullFunc
{
public:
    void operator () () {};
};

//////////////////////////////////////////////////////////////////////////////
//
// Fast floating point
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// Constants
//
//////////////////////////////////////////////////////////////////////////////

extern float g_0     ;
extern float g_1     ;
extern float g_0_5   ;
extern float g_255   ;
extern float g_Inv255;

//////////////////////////////////////////////////////////////////////////////
//
// Fast floating point
//
//////////////////////////////////////////////////////////////////////////////

#ifdef FLOATASM
    #define MakeIntMacro(value, result) _asm fld value _asm fistp result

    __forceinline int MakeInt(float value)
    {
        int result;
        MakeIntMacro(value, result);
        return result;
    }

    __forceinline int MakeInt(double value)
    {
        int result;
        MakeIntMacro(value, result);
        return result;
    }

#else
    __forceinline int MakeIntMacro(const float& value, int& result)
    {
        result = int(value);
    }

    __forceinline int MakeInt(float value)
    {
        return int(value);
    }

    __forceinline int MakeInt(double value)
    {
        return int(value);
    }
#endif

    __forceinline int FloorInt(float value)
    {
        return MakeInt(value - 0.5f);
    }

#endif
