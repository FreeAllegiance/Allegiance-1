#ifndef _Orientation_h_
#define _Orientation_h_

#include "vector.h"

//////////////////////////////////////////////////////////////////////////////
//
// Orientation Value
//
//////////////////////////////////////////////////////////////////////////////

class Orientation {
private:
    float m_r[3][3]; 

    static const Orientation s_orientIdentity;
    friend class Matrix;

public:
    Orientation();
    Orientation(float r00, float r01, float r02,
                float r10, float r11, float r12,
                float r20, float r21, float r22)
    {
        m_r[0][0] = r00;
        m_r[0][1] = r01;
        m_r[0][2] = r02;
        m_r[1][0] = r10;
        m_r[1][1] = r11;
        m_r[1][2] = r12;
        m_r[2][0] = r20;
        m_r[2][1] = r21;
        m_r[2][2] = r22;
    }
    Orientation(const Orientation& o);
    Orientation(const float m[3][3]);
    Orientation& operator=(const float r[3][3]);
    Orientation(const Vector& vecForward);
    Orientation(const Vector& vecForward, const Vector& vecUp);
    Orientation(const Vector& axis, float angle);

    static const Orientation& GetIdentity()
    {
        return s_orientIdentity;
    }

    ~Orientation();

    Orientation& operator=(const Orientation&  o);
    Orientation& operator*=(const Orientation& o);

    Orientation operator*(const Orientation&  o) const;

    bool operator==(const Orientation& o)  const;  // fuzzy equality test: forward & up axes within 1 degree or so

    // turn the forward axis of *this towards target (by yaw * pitch), turning <= maxTurn

    void  TurnTo(const Vector& target);
    float TurnTo(const Vector& target, float maxTurn); //radians

    // invert the matrix by taking the transform

    Orientation& Invert();

    // multiply a vector by the inverse (without actually creating the inverse matrix): xyz * (*this)^-1
    //            orientation                                                           o * (*this)^-1

    //r = v * (o^-1)
    Vector      TimesInverse(const Vector& vec)    const;
    // r = this * (o^-1)
    Orientation TimesInverse(const Orientation& o) const;

    // Set the orientation equal to either the desired values

    void Reset();

    bool Set(const Vector& vecForward);
    bool Set(const Vector& vecForward, const Vector& vecUp);

    // Get the forward, up & right axes of the orientation
    // in-lined and in the header file for efficiency

    Vector GetForward() const
    {
        return Vector(-m_r[2][0], -m_r[2][1],-m_r[2][2]);
    }

    const Vector& GetBackward() const
    {
        assert (&(((Vector*)&m_r[2][0])->z) == &m_r[2][2]);
        return *((Vector*)&m_r[2][0]);
    }

    const Vector& GetUp() const
    {
        assert (&(((Vector*)&m_r[1][0])->z) == &m_r[1][2]);
        return *((Vector*)&m_r[1][0]);
    }

    const Vector& GetRight() const
    {
        assert (&(((Vector*)&m_r[0][0])->z) == &m_r[0][2]);
        return *((Vector*)&m_r[0][0]);
    }

    void SetForward(const Vector& vec)
    {
        ZAssert(vec.LengthSquared() != 0);

        m_r[2][0] = -vec.x;
        m_r[2][1] = -vec.y;
        m_r[2][2] = -vec.z;
    }

    void SetUp(const Vector& vec)
    {
        ZAssert(vec.LengthSquared() != 0);

        m_r[1][0] = vec.x;
        m_r[1][1] = vec.y;
        m_r[1][2] = vec.z;
    }

    void SetRight(const Vector& vec)
    {
        ZAssert(vec.LengthSquared() != 0);

        m_r[0][0] = vec.x;
        m_r[0][1] = vec.y;
        m_r[0][2] = vec.z;
    }

    // Return the cosine of the angle between vec and the given axis, requires a sqrt() call

    float CosForward(const Vector& vec) const;
    float      CosUp(const Vector& vec) const;
    float   CosRight(const Vector& vec) const;

    // Fast versions of the above that don't take a square-root.
    // cosX == sqrt(abs(cosX2)) & sgn(cosX) == sgn(cosX2)

    float CosForward2(const Vector& vec) const;
    float      CosUp2(const Vector& vec) const;
    float   CosRight2(const Vector& vec) const;

    // Rotate the coordinate system about the given axis by theta

    Orientation&  PostRoll(float theta);
    Orientation& PostPitch(float theta);
    Orientation&   PostYaw(float theta);

    Orientation&  Roll(float theta);
    Orientation& Pitch(float theta);
    Orientation&   Yaw(float theta);

    void        Renormalize(void)
    {
        Vector  forward = GetForward();
        Vector  up = GetUp();

        Set(forward, up);
    }

    // this = Orientation(axis, theta) * this

    Orientation& PreRotate(const Vector& axis, float theta);

    // this *= Orientation(axis, theta)

    Orientation& PostRotate(const Vector& axis, float theta);

    // Vector& operator*=(const Orientation& o);

    void        Scale(const Vector& xyz)
    {
        m_r[0][0] *= xyz.x; m_r[0][1] *= xyz.y; m_r[0][2] *= xyz.z;
        m_r[1][0] *= xyz.x; m_r[1][1] *= xyz.y; m_r[1][2] *= xyz.z;
        m_r[2][0] *= xyz.x; m_r[2][1] *= xyz.y; m_r[2][2] *= xyz.z;
    }

    const float* operator[](int index) const
    {
        return m_r[index];
    }

    friend Vector  operator * (const Vector& v, const Orientation& o);

};

inline Vector  operator * (const Vector& v, const Orientation& o)
{
    return
        Vector(
            v.x * o.m_r[0][0] + v.y * o.m_r[1][0] + v.z * o.m_r[2][0],
            v.x * o.m_r[0][1] + v.y * o.m_r[1][1] + v.z * o.m_r[2][1],
            v.x * o.m_r[0][2] + v.y * o.m_r[1][2] + v.z * o.m_r[2][2]
        );
}

#endif
