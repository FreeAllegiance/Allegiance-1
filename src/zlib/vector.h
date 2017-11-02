//////////////////////////////////////////////////////////////////////////////
//
// Vector Value
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Vector_h_
#define _Vector_h_

#include "zassert.h"
#include "zstring.h"

class Vector {
private:
    static const Vector s_vectorZero;

public:
    //  : make these private again
    float x, y, z;

    Vector() {};
    Vector(float xArg, float yArg, float zArg) :
        x(xArg),
        y(yArg),
        z(zArg)
    {
    }

    static const Vector& GetZero()  { return s_vectorZero; }

    float X() const { return x; }
    float Y() const { return y; }
    float Z() const { return z; }

    Vector& SetX(float xArg) { x = xArg; return *this; }
    Vector& SetY(float yArg) { y = yArg; return *this; }
    Vector& SetZ(float zArg) { z = zArg; return *this; }

    bool IsZero() const
    {
        return (x == 0) && (y == 0) && (z == 0);
    }

    float LengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    float Length() const
    {
        return (float)sqrt(LengthSquared());
    }

    ZString GetString() const
    {
        return ZString("(") + x + ", " + y + ", " + z + ")";
    }

    Vector& operator+=(const Vector& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    Vector& operator-=(const Vector& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    Vector& operator*=(const float s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    Vector& operator/=(const float s)
    {
        *this = *this / s;
        return *this;
    }

    float   operator [](int i) const
    {
        assert (i >= 0);
        assert (i <= 2);
        return (&x)[i];
    }

    Vector Normalize() const
    {
        float lengthSquared = LengthSquared();

        if (lengthSquared == 1) {
            return *this;
        } else if (lengthSquared == 0) {
            ZError("divide by zero");
            return *this;
        } else {
            return *this / sqrt(lengthSquared);
        }
    }

    void SetNormalize()
    {
        *this = this->Normalize();
    }

    //
    // friend functions
    //

    friend bool operator==(const Vector& v1, const Vector& v2)
    {
        return
               v1.x == v2.x
            && v1.y == v2.y
            && v1.z == v2.z;
    }

    friend bool operator!=(const Vector& v1, const Vector& v2)
    {
        return !(v1 == v2);
    }

    friend Vector operator-(const Vector& v)
    {
        return Vector(-v.x, -v.y, -v.z);
    }

    friend Vector operator+(const Vector& v1, const Vector& v2)
    {
        return Vector(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
    }

    friend Vector operator-(const Vector& v1, const Vector& v2)
    {
        return Vector(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
    }

    friend Vector operator*(const Vector& v1, float s)
    {
        return Vector(v1.x * s, v1.y * s, v1.z * s);
    }

    friend Vector operator*(float s, const Vector& v1)
    {
        return v1 * s;
    }

    friend Vector operator/(const Vector& v1, float s)
    {
        float divisor = 1 / s;
        return Vector(v1.x * divisor, v1.y * divisor, v1.z * divisor);
    }

    friend float operator*(const Vector& v1, const Vector& v2)
    {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

	friend Vector CrossProduct(const Vector& v1, const Vector& v2)
// VS.Net 2003 port: friend definiton must be out of class to work with argument dep lookup
#if _MSC_VER < 1310
    {
        return
            Vector(
                (v1.y * v2.z) - (v2.y * v1.z),
                (v1.z * v2.x) - (v2.z * v1.x),
                (v1.x * v2.y) - (v2.x * v1.y)
            );
    }
#else
	;
#endif    
    Vector GetOrthogonalVector() const;

    static Vector RandomDirection(void);
    static Vector RandomPosition(float radius);
};

#endif
