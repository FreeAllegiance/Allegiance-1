#include "orientation.h"
#include "matrix.h"
#include "zmath.h"
const Orientation Orientation::s_orientIdentity;

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

Orientation::Orientation()
{
    Reset();
}

Orientation::Orientation(const float m[3][3])
{
    for (int i = 0; (i < 3); i++)
        for (int j = 0; (j < 3); j++)
            m_r[i][j] = m[i][j];
}

Orientation::Orientation(const Orientation&  o)
{
    for (int i = 0; (i < 3); i++)
        for (int j = 0; (j < 3); j++)
            m_r[i][j] = o.m_r[i][j];
}

Orientation::Orientation(const Vector&   forwardAxis)
{
    Set(forwardAxis);
}

Orientation::Orientation(const Vector& forwardAxis, const Vector& upAxis)
{
    Set(forwardAxis, upAxis);
}

Orientation::Orientation(const Vector& axis, float theta)
{
    Matrix mat;
    mat.SetRotate(axis, theta);

    for (int i = 0; (i < 3); i++) {
        for (int j = 0; (j < 3); j++) {
            m_r[i][j] = mat[j][i];
        }
    }
}

Orientation::~Orientation()
{
}

Orientation&    Orientation::operator=(const Orientation& o)
{
    for (int i = 0; (i < 3); i++)
        for (int j = 0; (j < 3); j++)
            m_r[i][j] = o.m_r[i][j];

    return *this;
}

Orientation&    Orientation::operator=(const float r[3][3])
{
    for (int i = 0; (i < 3); i++)
        for (int j = 0; (j < 3); j++)
            m_r[i][j] = r[i][j];

    return *this;
}

Orientation     Orientation::operator*(const Orientation &o) const
{
    Orientation r;

    r.m_r[0][0] = m_r[0][0] * o.m_r[0][0] +
                  m_r[0][1] * o.m_r[1][0] +
                  m_r[0][2] * o.m_r[2][0];
    r.m_r[0][1] = m_r[0][0] * o.m_r[0][1] +
                  m_r[0][1] * o.m_r[1][1] +
                  m_r[0][2] * o.m_r[2][1];
    r.m_r[0][2] = m_r[0][0] * o.m_r[0][2] +
                  m_r[0][1] * o.m_r[1][2] +
                  m_r[0][2] * o.m_r[2][2];

    r.m_r[1][0] = m_r[1][0] * o.m_r[0][0] +
                  m_r[1][1] * o.m_r[1][0] +
                  m_r[1][2] * o.m_r[2][0];
    r.m_r[1][1] = m_r[1][0] * o.m_r[0][1] +
                  m_r[1][1] * o.m_r[1][1] +
                  m_r[1][2] * o.m_r[2][1];
    r.m_r[1][2] = m_r[1][0] * o.m_r[0][2] +
                  m_r[1][1] * o.m_r[1][2] +
                  m_r[1][2] * o.m_r[2][2];

    r.m_r[2][0] = m_r[2][0] * o.m_r[0][0] +
                  m_r[2][1] * o.m_r[1][0] +
                  m_r[2][2] * o.m_r[2][0];
    r.m_r[2][1] = m_r[2][0] * o.m_r[0][1] +
                  m_r[2][1] * o.m_r[1][1] +
                  m_r[2][2] * o.m_r[2][1];
    r.m_r[2][2] = m_r[2][0] * o.m_r[0][2] +
                  m_r[2][1] * o.m_r[1][2] +
                  m_r[2][2] * o.m_r[2][2];

    return r;
}

Orientation&    Orientation::operator*=(const Orientation &o)
{
    Orientation r(*this);

    m_r[0][0] = r.m_r[0][0] * o.m_r[0][0] +
                r.m_r[0][1] * o.m_r[1][0] +
                r.m_r[0][2] * o.m_r[2][0];
    m_r[0][1] = r.m_r[0][0] * o.m_r[0][1] +
                r.m_r[0][1] * o.m_r[1][1] +
                r.m_r[0][2] * o.m_r[2][1];
    m_r[0][2] = r.m_r[0][0] * o.m_r[0][2] +
                r.m_r[0][1] * o.m_r[1][2] +
                r.m_r[0][2] * o.m_r[2][2];

    m_r[1][0] = r.m_r[1][0] * o.m_r[0][0] +
                r.m_r[1][1] * o.m_r[1][0] +
                r.m_r[1][2] * o.m_r[2][0];
    m_r[1][1] = r.m_r[1][0] * o.m_r[0][1] +
                r.m_r[1][1] * o.m_r[1][1] +
                r.m_r[1][2] * o.m_r[2][1];
    m_r[1][2] = r.m_r[1][0] * o.m_r[0][2] +
                r.m_r[1][1] * o.m_r[1][2] +
                r.m_r[1][2] * o.m_r[2][2];

    m_r[2][0] = r.m_r[2][0] * o.m_r[0][0] +
                r.m_r[2][1] * o.m_r[1][0] +
                r.m_r[2][2] * o.m_r[2][0];
    m_r[2][1] = r.m_r[2][0] * o.m_r[0][1] +
                r.m_r[2][1] * o.m_r[1][1] +
                r.m_r[2][2] * o.m_r[2][1];
    m_r[2][2] = r.m_r[2][0] * o.m_r[0][2] +
                r.m_r[2][1] * o.m_r[1][2] +
                r.m_r[2][2] * o.m_r[2][2];

    return *this;
}

bool            Orientation::operator==(const Orientation &o) const
{
    return ((CosUp2(o.GetUp()) > 0.999) && (CosRight2(o.GetRight()) > 0.999));
}

Orientation& Orientation::Invert()
{
    float   t;

    t = m_r[0][1];
    m_r[0][1] = m_r[1][0];
    m_r[1][0] = t;

    t = m_r[0][2];
    m_r[0][2] = m_r[2][0];
    m_r[2][0] = t;

    t = m_r[2][1];
    m_r[2][1] = m_r[1][2];
    m_r[1][2] = t;

    return *this;
}

Vector Orientation::TimesInverse(const Vector& xyz) const
{
    return
        Vector(
            xyz.x * m_r[0][0] + xyz.y * m_r[0][1] + xyz.z * m_r[0][2],
            xyz.x * m_r[1][0] + xyz.y * m_r[1][1] + xyz.z * m_r[1][2],
            xyz.x * m_r[2][0] + xyz.y * m_r[2][1] + xyz.z * m_r[2][2]
        );
}

Orientation Orientation::TimesInverse(const Orientation& o) const
{
    Orientation r;

    r.m_r[0][0] = m_r[0][0] * o.m_r[0][0] +
                  m_r[0][1] * o.m_r[0][1] +
                  m_r[0][2] * o.m_r[0][2];
    r.m_r[0][1] = m_r[0][0] * o.m_r[1][0] +
                  m_r[0][1] * o.m_r[1][1] +
                  m_r[0][2] * o.m_r[1][2];
    r.m_r[0][2] = m_r[0][0] * o.m_r[2][0] +
                  m_r[0][1] * o.m_r[2][1] +
                  m_r[0][2] * o.m_r[2][2];

    r.m_r[1][0] = m_r[1][0] * o.m_r[0][0] +
                  m_r[1][1] * o.m_r[0][1] +
                  m_r[1][2] * o.m_r[0][2];
    r.m_r[1][1] = m_r[1][0] * o.m_r[1][0] +
                  m_r[1][1] * o.m_r[1][1] +
                  m_r[1][2] * o.m_r[1][2];
    r.m_r[1][2] = m_r[1][0] * o.m_r[2][0] +
                  m_r[1][1] * o.m_r[2][1] +
                  m_r[1][2] * o.m_r[2][2];

    r.m_r[2][0] = m_r[2][0] * o.m_r[0][0] +
                  m_r[2][1] * o.m_r[0][1] +
                  m_r[2][2] * o.m_r[0][2];
    r.m_r[2][1] = m_r[2][0] * o.m_r[1][0] +
                  m_r[2][1] * o.m_r[1][1] +
                  m_r[2][2] * o.m_r[1][2];
    r.m_r[2][2] = m_r[2][0] * o.m_r[2][0] +
                  m_r[2][1] * o.m_r[2][1] +
                  m_r[2][2] * o.m_r[2][2];

    return r;
}

void        Orientation::TurnTo(const Vector& target)
{
    float   dot = CosUp2(target);

    Vector  up = (dot < -0.99f)
                 ? GetForward()
                 : ((dot > 0.99f)
                    ? -GetForward()
                    : GetUp());

    Set(target, up);
}

float       Orientation::TurnTo(const Vector&   target,
                                float           maxTurn)
{
    float   remaining;

    float   cosTurn2 = CosForward2(target);
    if (cosTurn2 < 0.96f)
    {
        double  dcos = (cosTurn2 < 0.0f)
                       ? double(-sqrt(-cosTurn2))
                       : double(sqrt(cosTurn2));

        float   angle = (dcos <= -1.0)
                        ? pi
                        : ((dcos >= 1.0)
                           ? 0.0f
                           : (float)acos(dcos));

        if (angle <= maxTurn)
        {
            TurnTo(target);
            return 0.0f;
        }

        remaining = angle - maxTurn;
        if (dcos < -0.999)
        {
            /*
            ** Need to turn almost completely around ... do a simple yaw
            ** by the appropriate angle.
            */
            Yaw(maxTurn);
        }
        else
        {
            /*
            ** Get the vector normal to the target and the forward vector
            ** (which we'll use to rotate around).
            */
            Vector twist = -CrossProduct(GetBackward(), target);

            /*
            ** Twist about the axis perpendicular to both the target and forward
            ** vectors by the appropriate amount, but no more than maxTurn).
            */
            PostRotate(twist, maxTurn);
        }
    }
    else
    {
        //The target is almost directly in front of us (within 11 degrees or so)
        //which would make getting the twist axis above a little dicey
        //So ... instead ... get the yaw and pitch off of the angles with the right * up
        float yaw   = acos(CosRight(target)) - 0.5f * pi;
        float pitch = 0.5f * pi - acos(CosUp(target));

        float   angle = (float)sqrt(yaw * yaw + pitch * pitch);
        if (angle <= maxTurn)
        {
            TurnTo(target);
            return 0.0f;
        }

        remaining = angle - maxTurn;

        float f = maxTurn / angle;
        Yaw(yaw * f);
        Pitch(pitch * f);
    }

    Renormalize();
    return remaining;
}

void    Orientation::Reset()
{
    // set to identity.
    m_r[0][1] = m_r[0][2] = m_r[1][0] = m_r[1][2] = m_r[2][0] = m_r[2][1] = 0.0f; // mmf changed from 0.0F
    m_r[0][0] = m_r[1][1] = m_r[2][2] = 1.0f; // mmf changed from 0.0F
}

bool    Orientation::Set(const Vector& forwardAxis)
{
    bool    rc = true;
    float   forwardLength2 = forwardAxis.LengthSquared();

    if (forwardLength2 != 0.0f) // mmf changed from 0.0F
    {
        //
        // Construct an orientation matrix with the given forward axis
        // (everything else can be changed).
        //
        static const Vector xAxis(-1.0,  0.0, 0.0);
        static const Vector yAxis( 0.0, -1.0, 0.0);

        //
        // Try to find an upAxis that is perpendicular to the forward axis.
        //
        Vector upAxis = CrossProduct(xAxis, forwardAxis);

        //
        // Make sure upAxis is "good" (if forwardAxis is too close to the xAxis, round-off errors
        // will be significant).
        // |AxB| = |A|*|B|*sin(theta) => |AxB|^2 = |A|^2 * sin(theta)^2
        //
        if (upAxis.LengthSquared() < forwardLength2 * 0.01F)
        {
            // forwardAxis within ~ 5 degrees of xAxis ... try yAxis

            upAxis = CrossProduct(yAxis, forwardAxis);
        }

        Set(forwardAxis, upAxis);
    }
    else
    {
        // forwardAxis is 0,0,0 ... set to identity
        Reset();

        rc = false;
    }

    return rc;
}

bool Orientation::Set(const Vector& forwardAxis,
                      const Vector& upAxis)
{
    /*
    ** Take the cross-product to get the right vector
    */
    Vector rightAxis = CrossProduct(forwardAxis, upAxis);   //forward axis is negative of what it will be

    /*
    ** Normalize all three axes, set failure if any axis has a length
    ** of zero.
    */
    float lRight   = rightAxis.Length();
    float lForward = forwardAxis.Length();

    SetUp(CrossProduct(rightAxis, forwardAxis));
    float lUp = GetUp().Length();

    bool    rc = true;
    if ((lRight != 0.0f) && (lUp != 0.0f) && (lForward != 0.0f))
    {
        SetForward(forwardAxis / lForward);
        SetUp(GetUp() / lUp);
        SetRight(rightAxis / lRight);
    }
    else
    {
        Reset();
        rc = false;
    }

    return rc;
}

static  float    cosX(const Vector& axis,
                         const Vector& xyz)
{
    float    dot = axis * xyz;
    float    l2 = xyz.LengthSquared();

    if (l2 != 0.0f)
        return dot / (float)sqrt(l2);
    else
        return 0.0f; // mmf changed from 0.0F
}

static  float    cosX2(const Vector& axis,
                          const Vector& xyz)
{
    float    dot = axis * xyz;
    float    l2 = xyz.LengthSquared();

    if (l2)
        return dot >= 0.0f ? (dot*dot/l2) : (-dot*dot/l2); // mmf changed from 0.0F
    else
        return 0.0f; // mmf changed from 0.0F
}

float            Orientation::CosUp(const Vector&    xyz) const
{
    return cosX(GetUp(), xyz);
}

float            Orientation::CosRight(const Vector&    xyz) const
{
    return cosX(GetRight(), xyz);
}

float            Orientation::CosForward(const Vector&    xyz) const
{
    return -cosX(*((Vector*)&m_r[2][0]), xyz);
}

float            Orientation::CosUp2(const Vector&    xyz) const
{
    return cosX2(GetUp(), xyz);
}

float            Orientation::CosRight2(const Vector&    xyz) const
{
    return cosX2(GetRight(), xyz);
}

float            Orientation::CosForward2(const Vector&    xyz) const
{
    return -cosX2(*((Vector*)&m_r[2][0]), xyz);
}

Orientation& Orientation::PostPitch(float theta)
{
    /*
    **               [ 1      0     0]
    ** this = this * [ 0  cos t sin t]
    **               [ 0 -sin t cos t]
    **
    ** Do it as fast as possible.
    */
    float c = (float)cos(theta);
    float s = (float)sin(theta);

    float r[3][3] =
        {
            { m_r[0][0], c * m_r[0][1] - s * m_r[0][2], s * m_r[0][1] + c * m_r[0][2]},
            { m_r[1][0], c * m_r[1][1] - s * m_r[1][2], s * m_r[1][1] + c * m_r[1][2]},
            { m_r[2][0], c * m_r[2][1] - s * m_r[2][2], s * m_r[2][1] + c * m_r[2][2]}
        };

    *this = r;
    return  *this;
}

Orientation& Orientation::PostYaw(float theta)
{
    /*
    **               [cos t 0 -sin t]
    ** this = this * [    0 1      0]
    **               [sin t 0  cos t]
    **
    ** Do it as fast as possible.
    */
    float c = (float)cos(theta);
    float s = (float)sin(theta);

    float r[3][3] =
        {
            {c * m_r[0][0] + s * m_r[0][2], m_r[0][1], c * m_r[0][2] - s * m_r[0][0]},
            {c * m_r[1][0] + s * m_r[1][2], m_r[1][1], c * m_r[1][2] - s * m_r[1][0]},
            {c * m_r[2][0] + s * m_r[2][2], m_r[2][1], c * m_r[2][2] - s * m_r[2][0]}
        };

    *this = r;
    return  *this;
}

Orientation& Orientation::PostRoll(float theta)
{
    /*
    **               [ cos t sin t 0]
    ** this = this * [-sin t cos t 0]
    **               [     0     0 1]
    **
    ** Do it as fast as possible.
    */
    float   c = (float)cos(theta);
    float   s = (float)sin(theta);

    float r[3][3] =
        {
            {c * m_r[0][0] - s * m_r[0][1], s * m_r[0][0] + c * m_r[0][1], m_r[0][2]},
            {c * m_r[1][0] - s * m_r[1][1], s * m_r[1][0] + c * m_r[1][1], m_r[1][2]},
            {c * m_r[2][0] - s * m_r[2][1], s * m_r[2][0] + c * m_r[2][1], m_r[2][2]}
        };

    *this = r;
    return  *this;
}
Orientation& Orientation::Pitch(float theta)
{
    /*
    **        [ 1      0     0]
    ** this = [ 0  cos t sin t] * this
    **        [ 0 -sin t cos t]
    **
    ** Do it as fast as possible.
    */

    float c = (float)cos(theta);
    float s = (float)sin(theta);

    float r[3][3] =
        {
            {    m_r[0][0],                     m_r[0][1],                     m_r[0][2]},
            {c * m_r[1][0] + s * m_r[2][0], c * m_r[1][1] + s * m_r[2][1], c * m_r[1][2] + s * m_r[2][2]},
            {c * m_r[2][0] - s * m_r[1][0], c * m_r[2][1] - s * m_r[1][1], c * m_r[2][2] - s * m_r[1][2]},
        };

    *this = r;
    return  *this;
}

Orientation& Orientation::Yaw(float theta)
{
    /*
    **        [cos t 0 -sin t]
    ** this = [    0 1      0] * this
    **        [sin t 0  cos t]
    **
    ** Do it as fast as possible.
    */

	float c = (float)cos(theta);
    float s = (float)sin(theta);

    float r[3][3] =
        {
            {c * m_r[0][0] - s * m_r[2][0], c * m_r[0][1] - s * m_r[2][1], c * m_r[0][2] - s * m_r[2][2]},
            {    m_r[1][0],                     m_r[1][1],                     m_r[1][2]},
            {c * m_r[2][0] + s * m_r[0][0], c * m_r[2][1] + s * m_r[0][1], c * m_r[2][2] + s * m_r[0][2]},
        };

    *this = r;
    return  *this;
}

Orientation& Orientation::Roll(float theta)
{
    /*
    **        [ cos t sin t 0]
    ** this = [-sin t cos t 0] * this
    **        [     0     0 1]
    **
    ** Do it as fast as possible.
    */

    float   c = (float)cos(theta);
    float   s = (float)sin(theta);

    float r[3][3] =
        {
            {c * m_r[0][0] + s * m_r[1][0], c * m_r[0][1] + s * m_r[1][1], c * m_r[0][2] + s * m_r[1][2]},
            {c * m_r[1][0] - s * m_r[0][0], c * m_r[1][1] - s * m_r[0][1], c * m_r[1][2] - s * m_r[0][2]},
            {    m_r[2][0],                     m_r[2][1],                     m_r[2][2]}
        };

    *this = r;
    return  *this;
}

Orientation& Orientation::PreRotate(const Vector& axis, float theta)
{
    Orientation o(axis, theta);

    *this = o * *this;

    return *this;
}

Orientation& Orientation::PostRotate(const Vector&  axis,
                                     float          theta)
{
    Orientation o(axis, theta);

    *this *= o;

    return *this;
}
