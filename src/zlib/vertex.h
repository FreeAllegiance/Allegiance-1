//////////////////////////////////////////////////////////////////////////////
//
// 3D Verticies
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _HVector_H_
#define _HVector_H_

#include "vector.h"

class HVector {
public:
    float x, y, z, w;

    HVector() {}

    HVector(float xArg, float yArg, float zArg, float wArg = 1)
    {
        x = xArg;
        y = yArg;
        z = zArg;
        w = wArg;
    }

    HVector(const HVector& hvec) :
        x(hvec.x),
        y(hvec.y),
        z(hvec.z),
        w(hvec.w)
    {
    }

    HVector(const Vector& vec) :
        x(vec.x),
        y(vec.y),
        z(vec.z),
        w(1)
    {
    }

    float X() const { return x; }
    float Y() const { return y; }
    float Z() const { return z; }
    float W() const { return w; }

    void Set(float xArg, float yArg, float zArg, float wArg = 1) { x = xArg; y = yArg; z = zArg; w = wArg; }
    void SetX(float xArg) { x = xArg; }
    void SetY(float yArg) { y = yArg; }
    void SetZ(float zArg) { z = zArg; }
    void SetW(float wArg) { w = wArg; }

    HVector& operator+=(const HVector& hvec)
    {
        x += hvec.x;
        y += hvec.y;
        z += hvec.z;
        w += hvec.w;
        return *this;
    }

    HVector& operator-=(const HVector& hvec)
    {
        x -= hvec.x;
        y -= hvec.y;
        z -= hvec.z;
        w -= hvec.w;
        return *this;
    }

    friend HVector operator+(const HVector& hvec1, const HVector& hvec2)
    {
        return
            HVector(
                hvec1.x + hvec2.x,
                hvec1.y + hvec2.y,
                hvec1.z + hvec2.z,
                hvec1.w + hvec2.w
            );
    }

    friend HVector operator-(const HVector& hvec1, const HVector& hvec2)
    {
        return
            HVector(
                hvec1.x - hvec2.x,
                hvec1.y - hvec2.y,
                hvec1.z - hvec2.z,
                hvec1.w - hvec2.w
            );
    }

    friend HVector operator*(const HVector& v1, float s)
    {
        return HVector(v1.x * s, v1.y * s, v1.z * s, v1.w * s);
    }

    friend HVector operator*(float s, const HVector& v1)
    {
        return v1 * s;
    }

    friend float operator*(const HVector& hvec1, const HVector& hvec2)
    {
        return
              hvec1.x * hvec2.x
            + hvec1.y * hvec2.y
            + hvec1.z * hvec2.z
            + hvec1.w * hvec2.w;
    }

    friend bool operator==(const HVector& hvec1, const HVector& hvec2)
    {
        return hvec1.x == hvec2.x && hvec1.y == hvec2.y && hvec1.z == hvec2.z && hvec1.w == hvec2.w;
    }

    friend bool operator!=(const HVector& hvec1, const HVector& hvec2)
    {
        return hvec1.x != hvec2.x || hvec1.y != hvec2.y || hvec1.z != hvec2.z || hvec1.w != hvec2.w;
    }
};

#endif
