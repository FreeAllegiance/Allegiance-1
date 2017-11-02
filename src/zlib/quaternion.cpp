#include "quaternion.h"

//////////////////////////////////////////////////////////////////////////////
//
// Quaternions
//
//////////////////////////////////////////////////////////////////////////////

float Quaternion::GetRotation(Vector& vec) const
{
    if (m_vec.X() == 0.0f && m_vec.Y() == 0.0f && m_vec.Z() == 0.0f) {
        vec   = Vector(0, 0, 1);
        return 0;
    } else {
        vec = m_vec.Normalize();
        return 2 * acos(m_a);
    }
}

bool operator==(const Quaternion& q1, const Quaternion& q2)
{
    return q1.m_a == q2.m_a && q1.m_vec == q2.m_vec;
}

bool operator!=(const Quaternion& q1, const Quaternion& q2)
{
    return q1.m_a != q2.m_a || q1.m_vec != q2.m_vec;
}


// rotation matrix to quaternion
Quaternion::Quaternion(const Orientation& o)
{
    float trace = o[0][0] + o[1][1] + o[2][2];

    //if (0 < trace && !Is0(trace))
    if (trace > 0.001f)
    {
        float scale = (float)sqrt(1.0f + trace);
        m_a = scale * 0.5f;

        scale = -0.5f / scale;
        m_vec.x = scale * (o[2][1] - o[1][2]);
        m_vec.y = scale * (o[0][2] - o[2][0]);
        m_vec.z = scale * (o[1][0] - o[0][1]);
    }
    else
    {
        int i,j,k;
        if (o[1][1] > o[0][0])
            if (o[2][2] > o[1][1])
                i = 2, j = 0, k = 1;
            else
                i = 1, j = 2, k = 0;
        else
            if (o[2][2] > o[0][0])
                i = 2, j = 0, k = 1;
            else
                i = 0, j = 1, k = 2;

        float dojk = o[j][k] - o[k][j];

        float scale = (dojk >= 0.0f)
                      ? float( sqrt(1.0f + o[i][i] - o[j][j] - o[k][k]))
                      : float(-sqrt(1.0f + o[i][i] - o[j][j] - o[k][k]));

        ((float*)(&m_vec.x))[i] = scale * 0.5f;

        scale = 0.5f / scale;
        m_a = scale * dojk;

        ((float*)(&m_vec.x))[j] = scale * (o[j][i] + o[i][j]);
        ((float*)(&m_vec.x))[k] = scale * (o[k][i] + o[i][k]);
    }
	
	if (!(m_a >= 0.0f)) {
		debugf("mmf Zlib quaternion.cpp:85 commented out assert(m_a >= 0.0f)");
	}
    //assert (m_a >= 0.0f);
}

//Quaternion to orientation
Quaternion::operator Orientation(void) const
{
    float   m[3][3];

    double  s = m_a;
    double  x = m_vec.x;
    double  y = m_vec.y;
    double  z = m_vec.z;

    double i2 = 2.0 * x;  //Which is faster? multiplication or addition
    double j2 = 2.0 * y;
    double k2 = 2.0 * z;

    double l2 = s*s + x*x + y*y + z*z;
    assert (l2 > 0.0);

    //Fudge factor for a not perfectly normalized quaternion
    //Note everything used to compute the matrix is the product of
    //two quaternion elements, so don't bother with the sqrt

    {
        double   ii2 = (x * i2) / l2;
        double   jj2 = (y * j2) / l2;
        double   kk2 = (z * k2) / l2;

        m[0][0] = float(1.0 - jj2 - kk2);
        m[1][1] = float(1.0 - kk2 - ii2);
        m[2][2] = float(1.0 - ii2 - jj2);
    }
    {
        double   ij2 = (x * j2) / l2;
        double   sk2 = (s * k2) / l2;

        m[0][1] = float(ij2 + sk2);
        m[1][0] = float(ij2 - sk2);
    }
    {
        double   jk2 = (y * k2) / l2;
        double   si2 = (s * i2) / l2;

        m[1][2] = float(jk2 + si2);
        m[2][1] = float(jk2 - si2);
    }
    {
        double   ki2 = (z * i2) / l2;
        double   sj2 = (s * j2) / l2;

        m[0][2] = float(ki2 - sj2);
        m[2][0] = float(ki2 + sj2);
    }

    Orientation o(m);
    o.Renormalize();

    return o;
}

Quaternion Slerp(const Quaternion& q1, const Quaternion& q2, float alpha)
{
	/*
	** Do spherical linear interpolation to the target quaternion.
	** Use the algorithm from SIGGRAPH '85, pp 245-254.
	*/
    double  cTheta = q1.m_a   * q2.m_a +
                     q1.m_vec * q2.m_vec;

    double  l1 = q1.m_a   * q1.m_a +
                 q1.m_vec * q1.m_vec;

    double  l2 = q2.m_a   * q2.m_a +
                 q2.m_vec * q2.m_vec;

    float   beta = 1.0f - alpha;
    if (cTheta < 0.0f)
    {
        //Pretend q2 == -q2 (which is an equivalent quaternion but easier on the
        //math).
        alpha  = -alpha;
        cTheta = -cTheta;
    }
    cTheta /= sqrt(l1 * l2);

    assert (cTheta >= 0.0f);

    Quaternion  q;
	if (cTheta > 0.999)
	{
		/*
		** cTheta is near +1, therefore sin theta is small.
		** Our best hope is to use the approximation that
		** sin(theta) ~= theta for small thetas.
		**
		** One reason for the funny test above is that (because
		** of round off errors) we can get cTheta > 1.0 which
		** acos() doesn't like at all.
		*/
        q.m_a   = q1.m_a   * beta + q2.m_a   * alpha;
        q.m_vec = q1.m_vec * beta + q2.m_vec * alpha;

        q.Normalize();
	}
	else
	{
		double  theta = acos(cTheta);
		double  sTheta = sin(theta);

		float  s1 = (float)(sin(beta  * theta) / sTheta);
		float  s2 = (float)(sin(alpha * theta) / sTheta);

        q.m_a   = q1.m_a   * s1 + q2.m_a   * s2;
        q.m_vec = q1.m_vec * s1 + q2.m_vec * s2;
	}

    return q;
}

Quaternion& Quaternion::Normalize(void)
{
    float s = m_vec * m_vec + m_a * m_a;

    if (s == 0.0f)
    {
        m_a = 0.0f;
        m_vec.SetX(0.0f);
        m_vec.SetY(0.0f);
        m_vec.SetZ(1.0f);
    }
    else
    {
        s = (float)(1.0 / sqrt(s));
        m_a   *= s;
        m_vec *= s;
    }

    return *this;
}
