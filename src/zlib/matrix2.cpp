#include "matrix.h"

//////////////////////////////////////////////////////////////////////////////
//
// The Identity Matrix2
//
//////////////////////////////////////////////////////////////////////////////

Matrix2 g_matIdentity;

class InitializeTranform2 {
public:
    InitializeTranform2()
    {
        g_matIdentity.SetIdentity();
    }
} initialize;

const Matrix2& Matrix2::GetIdentity()
{
    return g_matIdentity;
}

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

Matrix2::Matrix2(
    float v00, float v01, float v02,
    float v10, float v11, float v12,
    float v20, float v21, float v22
) {
    m_m[0][0] = v00; m_m[0][1] = v01; m_m[0][2] = v02;
    m_m[1][0] = v10; m_m[1][1] = v11; m_m[1][2] = v12;
    m_m[2][0] = v20; m_m[2][1] = v21; m_m[2][2] = v22;
}

//////////////////////////////////////////////////////////////////////////////
//
// Matrix2 Accessors
//
//////////////////////////////////////////////////////////////////////////////

float Matrix2::GetScale() const
{
    return Point(m_m[0][0], m_m[1][0]).Length();
}

Point Matrix2::GetTranslate() const
{
    return Point(m_m[0][2], m_m[1][2]);
}

//////////////////////////////////////////////////////////////////////////////
//
// Matrix2
//
//////////////////////////////////////////////////////////////////////////////

TransformType Matrix2::GetType() const
{
    if (m_type == TransformUnknown) {
        ((Matrix2*)this)->CalcType();
    }
    return m_type;
}

void Matrix2::InvalidateType()
{
    m_type = TransformUnknown;
}

void Matrix2::CalcType()
{
    m_type = TransformIdentity;

    if (m_m[2][0] != 0 || m_m[2][1] != 0 || m_m[2][2] != 1) {
        m_type |= TransformPerspective;
    }

    if (m_m[0][1] != 0 || m_m[1][0] != 0) {
        m_type |= TransformRotate;
    }

    if (m_m[0][0] != 1 || m_m[1][1] != 1) {
        m_type |= TransformScale;
    }

    if (m_m[0][2] != 0 || m_m[1][2] != 0) {
        m_type |= TransformTranslate;
    }
}

Matrix2& Matrix2::SetIdentity()
{
    m_m[0][0] = 1; m_m[0][1] = 0; m_m[0][2] = 0;
    m_m[1][0] = 0; m_m[1][1] = 1; m_m[1][2] = 0;
    m_m[2][0] = 0; m_m[2][1] = 0; m_m[2][2] = 1;

    m_type = TransformIdentity;

    return *this;
}

void Matrix2::Set(int row, int col, float value)
{
    ZAssert(row >= 0 && row <= 2 && col >= 0 && col <= 2);

    m_m[row][col] = value;
    m_type = TransformUnknown;
}

//////////////////////////////////////////////////////////////////////////////
//
// Scales
//
//////////////////////////////////////////////////////////////////////////////

Matrix2& Matrix2::SetScale(float scale)
{
    SetIdentity();

    m_m[0][0] = scale;
    m_m[1][1] = scale;

    m_type = TransformScale;

    return *this;
}

Matrix2& Matrix2::Scale(float scale)
{
    m_m[0][0] *= scale; m_m[0][1] *= scale; m_m[0][2] *= scale;
    m_m[1][0] *= scale; m_m[1][1] *= scale; m_m[1][2] *= scale;

    m_type |= TransformScale;

    return *this;
}

Matrix2& Matrix2::PreScale(float scale)
{
    m_m[0][0] *= scale; m_m[0][1] *= scale;
    m_m[1][0] *= scale; m_m[1][1] *= scale;
    m_m[2][0] *= scale; m_m[2][1] *= scale;

    m_type |= TransformScale;

    return *this;
}

Matrix2& Matrix2::SetScale(const Point& vec)
{
    SetIdentity();

    m_m[0][0] = vec.X();
    m_m[1][1] = vec.Y();

    m_type = TransformScale;

    return *this;
}

Matrix2& Matrix2::Scale(const Point& vec)
{
    m_m[0][0] *= vec.X(); m_m[0][1] *= vec.X(); m_m[0][2] *= vec.X();
    m_m[1][0] *= vec.Y(); m_m[1][1] *= vec.Y(); m_m[1][2] *= vec.Y();

    m_type |= TransformScale;

    return *this;
}

Matrix2& Matrix2::PreScale(const Point& vec)
{
    m_m[0][0] *= vec.X(); m_m[0][1] *= vec.Y();
    m_m[1][0] *= vec.X(); m_m[1][1] *= vec.Y();
    m_m[2][0] *= vec.X(); m_m[2][1] *= vec.Y();

    m_type |= TransformScale;

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Translations
//
//////////////////////////////////////////////////////////////////////////////

Matrix2& Matrix2::SetTranslate(const Point& vec)
{
    SetIdentity();

    m_m[0][2] = vec.X();
    m_m[1][2] = vec.Y();

    m_type = TransformTranslate;

    return *this;
}

Matrix2& Matrix2::Translate(const Point& vec)
{
    m_m[0][2] += vec.X();
    m_m[1][2] += vec.Y();

    m_type |= TransformTranslate;

    return *this;
}

Matrix2& Matrix2::PreTranslate(const Point& vec)
{
    m_m[0][2] += m_m[0][0] * vec.X() + m_m[0][1] * vec.Y();
    m_m[1][2] += m_m[1][0] * vec.X() + m_m[1][1] * vec.Y();

    m_type |= TransformTranslate;

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Rotations
//
//////////////////////////////////////////////////////////////////////////////

Matrix2& Matrix2::SetRotate(float angle)
{
    SetIdentity();
    float c = cos(angle);
    float s = sin(angle);
    m_m[0][0] = c; m_m[0][1] = -s;
    m_m[1][0] = s; m_m[1][1] =  c;

    m_type = TransformRotate;

    return *this;
}

Matrix2& Matrix2::Rotate(float angle)
{
    Matrix2 m;
    m.SetRotate(angle);
    Multiply(m);

    return *this;
}

Matrix2& Matrix2::PreRotate(float angle)
{
    Matrix2 m;
    m.SetRotate(angle);
    PreMultiply(m);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Multiply (64 mul 48 add)
//
//////////////////////////////////////////////////////////////////////////////

Matrix2& Matrix2::SetMultiply(const Matrix2& m1, const Matrix2& m2)
{
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            m_m[row][col] =
                  m1.m_m[0][col] * m2.m_m[row][0]
                + m1.m_m[1][col] * m2.m_m[row][1]
                + m1.m_m[2][col] * m2.m_m[row][2];
        }
    }

    m_type = m1.GetType() | m2.GetType();

    return *this;
}

Matrix2& Matrix2::Multiply(const Matrix2& m2)
{
    Matrix2 m1(*this);
    SetMultiply(m1, m2);

    return *this;
}

Matrix2& Matrix2::PreMultiply(const Matrix2& m1)
{
    Matrix2 m2(*this);
    SetMultiply(m1, m2);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Inverse
// !!! pick inverse implementation based on Matrix2 type
//
//////////////////////////////////////////////////////////////////////////////

Matrix2& Matrix2::SetInverse(const Matrix2& m) // 36 mul 14 add 1 div
{
    SetIdentity();

    float denom = 1 /
        Determinant3(
            m.m_m[0][0], m.m_m[0][1], m.m_m[0][2],
            m.m_m[1][0], m.m_m[1][1], m.m_m[1][2],
            m.m_m[2][0], m.m_m[2][1], m.m_m[2][2]
        );

    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int rowNext = row == 2 ? 0 : row + 1;
            int colNext = col == 2 ? 0 : col + 1;
            int rowNextNext = rowNext == 2 ? 0 : rowNext + 1;
            int colNextNext = colNext == 2 ? 0 : colNext + 1;

            m_m[col][row] =
                  Determinant2(
                    m.m_m[rowNext][colNext],     m.m_m[rowNext][colNextNext],
                    m.m_m[rowNextNext][colNext], m.m_m[rowNextNext][colNextNext]
                  )
                * denom;
        }
    }

    m_type = m.m_type;
    return *this;
}

Matrix2& Matrix2::Inverse()
{
    Matrix2 mat(*this);
    SetInverse(mat);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Transpose
//
//////////////////////////////////////////////////////////////////////////////

Matrix2& Matrix2::SetTranspose(const Matrix2& m)
{
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            m_m[row][col] = m.m_m[col][row];
        }
    }

    return *this;
}

Matrix2& Matrix2::Transpose()
{
    for (int row = 0; row < 3; row++) {
        for (int col = row + 1; col < 3; col++) {
            float value = m_m[row][col];
            m_m[row][col] = m_m[col][row];
            m_m[col][row] = value;
        }
    }

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Transforms
//
//////////////////////////////////////////////////////////////////////////////

Point Matrix2::Transform(const Point& point) const
{
    return
        Point(
            ((m_m[0][0] * point.X()) + (m_m[0][1] * point.Y()) + m_m[0][2]),
            ((m_m[1][0] * point.X()) + (m_m[1][1] * point.Y()) + m_m[1][2])
        );
}

void Matrix2::Transform(const TVector<Point>& vSource, TVector<Point>& vDest) const
{
    int count = vSource.GetCount();
    vDest.SetCount(count);

    for (int index = 0; index < count; index++) {
        float x = vSource[index].X();
        float y = vSource[index].Y();

        vDest.Set(
            index,
            Point(
                m_m[0][0] * x + m_m[0][1] * y + m_m[0][2],
                m_m[1][0] * x + m_m[1][1] * y + m_m[1][2]
            )
        );
    }
}
