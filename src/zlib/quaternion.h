//////////////////////////////////////////////////////////////////////////////
//
// Quaternion
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Quaternion_h_
#define _Quaternion_h_

#include "vector.h"
#include "orientation.h"

//
// q = a + bi + cj + dk
// i*i = j*j = k*k = -1
// ij = k = -ji
// jk = i = -kj
// ki = j = -ik
//

class Quaternion {
private:
    float  m_a;
    Vector m_vec;

public:
    Quaternion(float a, float b, float c, float d) :
        m_a(a),
        m_vec(b, c, d)
    {
        ZAssert(m_a >= -1 && m_a <= 1);
    }

    Quaternion(const Vector& vec, float angle)
    {
        m_a   = cos(angle / 2);
        m_vec = vec.Normalize() * sin(angle / 2);
    }

    Quaternion(const Quaternion& quat) :
        m_a(quat.m_a),
        m_vec(quat.m_vec)
    {
    }

    Quaternion() :
        m_a(1),
        m_vec(0, 0, 0)
    {
    }

    Quaternion(const Orientation& o);

    float GetA() const { return m_a; }
    float GetX() const { return m_vec.X(); }
    float GetY() const { return m_vec.Y(); }
    float GetZ() const { return m_vec.Z(); }
    float GetRotation(Vector& vec) const;
    operator Orientation(void) const;

    Quaternion& Normalize(void);

    friend Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float alpha);
    friend bool operator==(const Quaternion& q1, const Quaternion& q2);
    friend bool operator!=(const Quaternion& q1, const Quaternion& q2);
};

#endif
