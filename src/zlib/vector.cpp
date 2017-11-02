#include "vector.h"
#include "zmath.h"

const Vector Vector::s_vectorZero(0.0f, 0.0f, 0.0f);  // mmf changed these to 0.0f from just 0

Vector Vector::GetOrthogonalVector() const
{
    float ax = fabs(x);
    float ay = fabs(y);
    float az = fabs(z);

    if (ax > ay) {
        if (ax > az) {
            //
            // x is biggest
            //

            return Vector(-z - y * z / x, z, x);
        }
    } else {
        if (ay > az) {
            //
            // y is biggest
            //

            return Vector(y, -x - z * x / y, x);
        }
    }

    //
    // z is biggest
    //

    return Vector(y, z, -y - x * y / z);
}

Vector Vector::RandomDirection(void)
{
    float sinPitch = random(-1.0f, 1.0f);
    float cosPitch = sqrt(1.0f - sinPitch * sinPitch);
    float yaw      = random(0.0f, 2.0f * pi);

    return 
        Vector(
            cos(yaw) * cosPitch,
            sin(yaw) * cosPitch,
            sinPitch
        );
}

Vector Vector::RandomPosition(float radius)
{
    float   rnd = random(0.0f, 1.0f);
    return  rnd == 0.0f
            ? Vector::GetZero()
            : Vector::RandomDirection() * (radius * pow(rnd, 1.0f/3.0f));
}

// VS.Net 2003 port: friend definition must be out of class/namespace to work with argument dep lookup
// see vector.h
#if _MSC_VER >= 1310
Vector CrossProduct(const Vector& v1, const Vector& v2)
    {
        return
            Vector(
                (v1.y * v2.z) - (v2.y * v1.z),
                (v1.z * v2.x) - (v2.z * v1.x),
                (v1.x * v2.y) - (v2.x * v1.y)
            );
    }
#endif    