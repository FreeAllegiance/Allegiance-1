#include "matrix.h"
#include "orientation.h"
#include "zmath.h"

//////////////////////////////////////////////////////////////////////////////
//
// The Identity Matrix
//
//////////////////////////////////////////////////////////////////////////////

Matrix g_matIdentity;

class InitializeTransform {
public:
    InitializeTransform()
    {
        g_matIdentity.SetIdentity();
    }
} initialize;

const Matrix& Matrix::GetIdentity()
{
    return g_matIdentity;
}

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

Matrix::Matrix(const Matrix2& mat2) :
    m_type(TransformUnknown)
{
    m_m[0][0] = mat2[0][0]; m_m[0][1] = mat2[0][1]; m_m[0][2] = 0; m_m[0][3] = mat2[0][2];
    m_m[1][0] = mat2[1][0]; m_m[1][1] = mat2[1][1]; m_m[1][2] = 0; m_m[1][3] = mat2[1][2];
    m_m[2][0] =          0; m_m[2][1] =          0; m_m[2][2] = 1; m_m[2][3] =          0;
    m_m[3][0] = mat2[2][0]; m_m[3][1] = mat2[2][1]; m_m[3][2] = 0; m_m[3][3] = mat2[2][2];
}

Matrix::Matrix(
    float v00, float v01, float v02, float v03,
    float v10, float v11, float v12, float v13,
    float v20, float v21, float v22, float v23,
    float v30, float v31, float v32, float v33
) :
    m_type(TransformUnknown)
{
    m_m[0][0] = v00; m_m[0][1] = v01; m_m[0][2] = v02; m_m[0][3] = v03;
    m_m[1][0] = v10; m_m[1][1] = v11; m_m[1][2] = v12; m_m[1][3] = v13;
    m_m[2][0] = v20; m_m[2][1] = v21; m_m[2][2] = v22; m_m[2][3] = v23;
    m_m[3][0] = v30; m_m[3][1] = v31; m_m[3][2] = v32; m_m[3][3] = v33;
}

Matrix::Matrix(
    const Orientation& o,
    const Vector&      p,
    float              s
) :
    m_type(TransformUnknown)
{
    //
    // note that orientations are transposed
    //

    m_m[0][0] = o.m_r[0][0] * s; m_m[0][1] = o.m_r[1][0] * s; m_m[0][2] = o.m_r[2][0] * s; m_m[0][3] = p.x;
    m_m[1][0] = o.m_r[0][1] * s; m_m[1][1] = o.m_r[1][1] * s; m_m[1][2] = o.m_r[2][1] * s; m_m[1][3] = p.y;
    m_m[2][0] = o.m_r[0][2] * s; m_m[2][1] = o.m_r[1][2] * s; m_m[2][2] = o.m_r[2][2] * s; m_m[2][3] = p.z;
    m_m[3][0] = 0.0;             m_m[3][1] = 0.0;             m_m[3][2] = 0.0;             m_m[3][3] = 1.0f;
}

//////////////////////////////////////////////////////////////////////////////
//
// Matrix Accessors
//
//////////////////////////////////////////////////////////////////////////////

float Matrix::GetScale() const
{
    return Vector(m_m[0][0], m_m[1][0], m_m[2][0]).Length();
}

Vector Matrix::GetTranslate() const
{
    return Vector(m_m[0][3], m_m[1][3], m_m[2][3]);
}

//////////////////////////////////////////////////////////////////////////////
//
// Determinants
//
//////////////////////////////////////////////////////////////////////////////

float Determinant(  // 40 mul 23 add
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33
) {
    return
        m00 * Determinant3(
            m11, m12, m13,
            m21, m22, m23,
            m31, m32, m33
        )
      + m10 * Determinant3(
            m21, m22, m23,
            m31, m32, m33,
            m01, m02, m03
        )
      + m20 * Determinant3(
            m31, m32, m33,
            m01, m02, m03,
            m11, m12, m13
        )
      + m30 * Determinant3(
            m01, m02, m03,
            m11, m12, m13,
            m21, m22, m23
        );
}

//
// alternate Determinate implementation
//

float Determinant(const float m[4][4], int startRow, int startCol, int size);

float Determinant(const float m[4][4], int size)
{
    if (size == 2) {
        return m[0][0] * m[1][1] - m[0][1] * m[1][0];
    } else {
        float sum = 0;

        for(int index = 0; index < size; index++) {
            int nextIndex = index == size ? 0 : index + 1;
            sum += m[index][0] * Determinant(m, nextIndex, 1, size);
        }

        return sum;
    }

}

float Determinant(const float m[4][4], int startRow, int startCol, int size)
{
    float mm[4][4];

    for (int row = 0; row < size - 1; row++) {
        for (int col = 0; col < size - 1; col++) {
            int fromRow = startRow + row; if (fromRow >= size) fromRow -= size;
            int fromCol = startCol + col; if (fromCol >= size) fromCol -= size;

            mm[row][col] = m[fromRow][fromCol];
        }
    }

    return Determinant(mm, size - 1);
}

//////////////////////////////////////////////////////////////////////////////
//
// Matrix
//
//////////////////////////////////////////////////////////////////////////////

TransformType Matrix::GetType() const
{
    if (m_type == TransformUnknown) {
        ((Matrix*)this)->CalcType();
    }
    return m_type;
}

void Matrix::InvalidateType()
{
    m_type = TransformUnknown;
}

void Matrix::CalcType()
{
    m_type = TransformIdentity;

    if (m_m[3][0] != 0 || m_m[3][1] != 0 || m_m[3][2] != 0 || m_m[3][3] != 1) {
        m_type |= TransformPerspective;
    }

    if (
        m_m[0][1] != 0 || m_m[0][2] != 0 ||
        m_m[1][0] != 0 || m_m[1][2] != 0 ||
        m_m[2][0] != 0 || m_m[2][1] != 0
       ) {
        m_type |= TransformRotate;
    }

    if (m_m[0][0] != 1 || m_m[1][1] != 1 || m_m[2][2] != 1) {
        m_type |= TransformScale;
    }

    if (m_m[0][3] != 0 || m_m[1][3] != 0 || m_m[2][3] != 0) {
        m_type |= TransformTranslate;
    }
}

Matrix& Matrix::SetIdentity()
{
    m_m[0][0] = 1; m_m[0][1] = 0; m_m[0][2] = 0; m_m[0][3] = 0;
    m_m[1][0] = 0; m_m[1][1] = 1; m_m[1][2] = 0; m_m[1][3] = 0;
    m_m[2][0] = 0; m_m[2][1] = 0; m_m[2][2] = 1; m_m[2][3] = 0;
    m_m[3][0] = 0; m_m[3][1] = 0; m_m[3][2] = 0; m_m[3][3] = 1;

    m_type = TransformIdentity;

    return *this;
}

void Matrix::Set(int row, int col, float value)
{
    ZAssert(row >= 0 && row <= 3 && col >= 0 && col <= 3);

    m_m[row][col] = value;
    m_type = TransformUnknown;
}

//////////////////////////////////////////////////////////////////////////////
//
// Scales
//
//////////////////////////////////////////////////////////////////////////////

Matrix& Matrix::SetScale(float scale)
{
    SetIdentity();

    m_m[0][0] = scale;
    m_m[1][1] = scale;
    m_m[2][2] = scale;

    m_type = TransformScale;

    return *this;
}

Matrix& Matrix::Scale(float scale)
{
    m_m[0][0] *= scale; m_m[0][1] *= scale; m_m[0][2] *= scale; m_m[0][3] *= scale;
    m_m[1][0] *= scale; m_m[1][1] *= scale; m_m[1][2] *= scale; m_m[1][3] *= scale;
    m_m[2][0] *= scale; m_m[2][1] *= scale; m_m[2][2] *= scale; m_m[2][3] *= scale;

    m_type |= TransformScale;

    return *this;
}

Matrix& Matrix::PreScale(float scale)
{
    m_m[0][0] *= scale; m_m[0][1] *= scale; m_m[0][2] *= scale;
    m_m[1][0] *= scale; m_m[1][1] *= scale; m_m[1][2] *= scale;
    m_m[2][0] *= scale; m_m[2][1] *= scale; m_m[2][2] *= scale;
    m_m[3][0] *= scale; m_m[3][1] *= scale; m_m[3][2] *= scale;

    m_type |= TransformScale;

    return *this;
}

Matrix& Matrix::SetScale(const Vector& vec)
{
    SetIdentity();

    m_m[0][0] = vec.X();
    m_m[1][1] = vec.Y();
    m_m[2][2] = vec.Z();

    m_type = TransformScale;

    return *this;
}

Matrix& Matrix::Scale(const Vector& vec)
{
    m_m[0][0] *= vec.X(); m_m[0][1] *= vec.X(); m_m[0][2] *= vec.X(); m_m[0][3] *= vec.X();
    m_m[1][0] *= vec.Y(); m_m[1][1] *= vec.Y(); m_m[1][2] *= vec.Y(); m_m[1][3] *= vec.Y();
    m_m[2][0] *= vec.Z(); m_m[2][1] *= vec.Z(); m_m[2][2] *= vec.Z(); m_m[2][3] *= vec.Z();

    m_type |= TransformScale;

    return *this;
}

Matrix& Matrix::PreScale(const Vector& vec)
{
    m_m[0][0] *= vec.X(); m_m[0][1] *= vec.Y(); m_m[0][2] *= vec.Z();
    m_m[1][0] *= vec.X(); m_m[1][1] *= vec.Y(); m_m[1][2] *= vec.Z();
    m_m[2][0] *= vec.X(); m_m[2][1] *= vec.Y(); m_m[2][2] *= vec.Z();
    m_m[3][0] *= vec.X(); m_m[3][1] *= vec.Y(); m_m[3][2] *= vec.Z();

    m_type |= TransformScale;

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Translations
//
//////////////////////////////////////////////////////////////////////////////

Matrix& Matrix::SetTranslate(const Vector& vec)
{
    SetIdentity();

    m_m[0][3] = vec.X();
    m_m[1][3] = vec.Y();
    m_m[2][3] = vec.Z();

    m_type = TransformTranslate;

    return *this;
}

Matrix& Matrix::Translate(const Vector& vec)
{
    float x = vec.X();
    float y = vec.Y();
    float z = vec.Z();

    m_m[0][0] = m_m[0][0] + x * m_m[3][0];
    m_m[0][1] = m_m[0][1] + x * m_m[3][1];
    m_m[0][2] = m_m[0][2] + x * m_m[3][2];
    m_m[0][3] = m_m[0][3] + x * m_m[3][3];

    m_m[1][0] = m_m[1][0] + y * m_m[3][0];
    m_m[1][1] = m_m[1][1] + y * m_m[3][1];
    m_m[1][2] = m_m[1][2] + y * m_m[3][2];
    m_m[1][3] = m_m[1][3] + y * m_m[3][3];

    m_m[2][0] = m_m[2][0] + z * m_m[3][0];
    m_m[2][1] = m_m[2][1] + z * m_m[3][1];
    m_m[2][2] = m_m[2][2] + z * m_m[3][2];
    m_m[2][3] = m_m[2][3] + z * m_m[3][3];

    m_type |= TransformTranslate;

    return *this;
}

Matrix& Matrix::PreTranslate(const Vector& vec)
{
    m_m[0][3] += m_m[0][0] * vec.X() + m_m[0][1] * vec.Y() + m_m[0][2] * vec.Z();
    m_m[1][3] += m_m[1][0] * vec.X() + m_m[1][1] * vec.Y() + m_m[1][2] * vec.Z();
    m_m[2][3] += m_m[2][0] * vec.X() + m_m[2][1] * vec.Y() + m_m[2][2] * vec.Z();
    m_m[3][3] += m_m[3][0] * vec.X() + m_m[3][1] * vec.Y() + m_m[3][2] * vec.Z();

    m_type |= TransformTranslate;

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Rotations
//
//////////////////////////////////////////////////////////////////////////////

Matrix& Matrix::SetRotateX(float angle)
{
    SetIdentity();
    float c = cos(angle);
    float s = sin(angle);
    m_m[1][1] = c; m_m[1][2] = -s;
    m_m[2][1] = s; m_m[2][2] =  c;

    m_type = TransformRotate;

    return *this;
}

Matrix& Matrix::SetRotateY(float angle)
{
    SetIdentity();
    float c = cos(angle);
    float s = sin(angle);
    m_m[2][2] = c; m_m[2][0] = -s;
    m_m[0][2] = s; m_m[0][0] =  c;

    m_type = TransformRotate;

    return *this;
}

Matrix& Matrix::SetRotateZ(float angle)
{
    SetIdentity();
    float c = cos(angle);
    float s = sin(angle);
    m_m[0][0] = c; m_m[0][1] = -s;
    m_m[1][0] = s; m_m[1][1] =  c;

    m_type = TransformRotate;

    return *this;
}

Matrix& Matrix::SetRotate(const Vector& vec, float angle)
{
    if (vec.Y() == 0 && vec.Z() == 0) {
        return SetRotateX(angle * sign(vec.X()));
    } else if (vec.X() == 0 && vec.Z() == 0) {
        return SetRotateY(angle * sign(vec.Y()));
    } else if (vec.X() == 0 && vec.Y() == 0) {
        return SetRotateZ(angle * sign(vec.Z()));
    } else {
        Vector vecUnit = vec.Normalize();

        float x = vecUnit.X();
        float y = vecUnit.Y();
        float z = vecUnit.Z();

        float c = cos(angle);
        float s = sin(angle);

        float x2 = x * x;
        float y2 = y * y;
        float z2 = z * z;
        float xy = x * y * (1 - c);
        float yz = y * z * (1 - c);
        float zx = z * x * (1 - c);

        m_m[0][0] = x2 + c * (1 - x2); m_m[0][1] =        xy - z * s; m_m[0][2] =        zx + y * s; m_m[0][3] = 0;
        m_m[1][0] =        xy + z * s; m_m[1][1] = y2 + c * (1 - y2); m_m[1][2] =        yz - x * s; m_m[1][3] = 0;
        m_m[2][0] =        zx - y * s; m_m[2][1] =        yz + x * s; m_m[2][2] = z2 + c * (1 - z2); m_m[2][3] = 0;
        m_m[3][0] =                 0; m_m[3][1] =                 0; m_m[3][2] =                 0; m_m[3][3] = 1;
    }

    m_type = TransformRotate;

    return *this;
}

Matrix& Matrix::Rotate(const Vector& vec, float angle)
{
    Matrix m;
    m.SetRotate(vec, angle);
    Multiply(m);

    return *this;
}

Matrix& Matrix::PreRotate(const Vector& vec, float angle)
{
    Matrix m;
    m.SetRotate(vec, angle);
    PreMultiply(m);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Look at from
//
//////////////////////////////////////////////////////////////////////////////

Matrix& Matrix::SetLookAtFrom(const Vector& vecAt, const Vector& vecFrom, const Vector& vecUp)
{
    Vector vecZAxis = (vecFrom - vecAt).Normalize();

    if (vecZAxis == vecUp) {
        SetIdentity();
        m_m[0][3] = vecFrom.X();
        m_m[1][3] = vecFrom.Y();
        m_m[2][3] = vecFrom.Z();
    } else {
        Vector vecXAxis = CrossProduct(vecUp, vecZAxis).Normalize();
        Vector vecYAxis = CrossProduct(vecZAxis, vecXAxis);

        m_m[0][0] = vecXAxis.X();
        m_m[0][1] = vecYAxis.X();
        m_m[0][2] = vecZAxis.X();
        m_m[0][3] =  vecFrom.X();

        m_m[1][0] = vecXAxis.Y();
        m_m[1][1] = vecYAxis.Y();
        m_m[1][2] = vecZAxis.Y();
        m_m[1][3] =  vecFrom.Y();

        m_m[2][0] = vecXAxis.Z();
        m_m[2][1] = vecYAxis.Z();
        m_m[2][2] = vecZAxis.Z();
        m_m[2][3] =  vecFrom.Z();

        m_m[3][0] = 0;
        m_m[3][1] = 0;
        m_m[3][2] = 0;
        m_m[3][3] = 1;

        m_type = TransformRotate | TransformTranslate;
    }

    return *this;
}

Matrix& Matrix::LookAtFrom(const Vector& vecAt, const Vector& vecFrom, const Vector& vecUp)
{
    Matrix m;
    m.SetLookAtFrom(vecAt, vecFrom, vecUp);
    Multiply(m);

    return *this;
}

Matrix& Matrix::PreLookAtFrom(const Vector& vecAt, const Vector& vecFrom, const Vector& vecUp)
{
    Matrix m;
    m.SetLookAtFrom(vecAt, vecFrom, vecUp);
    PreMultiply(m);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Multiply (64 mul 48 add)
//
//////////////////////////////////////////////////////////////////////////////

Matrix& Matrix::SetMultiply(const Matrix& m1, const Matrix& m2)
{
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            m_m[row][col] =
                  m1.m_m[0][col] * m2.m_m[row][0]
                + m1.m_m[1][col] * m2.m_m[row][1]
                + m1.m_m[2][col] * m2.m_m[row][2]
                + m1.m_m[3][col] * m2.m_m[row][3];
        }
    }

    m_type = m1.GetType() | m2.GetType();

    return *this;
}

Matrix& Matrix::Multiply(const Matrix& m2)
{
    Matrix m1(*this);
    SetMultiply(m1, m2);

    return *this;
}

Matrix& Matrix::PreMultiply(const Matrix& m1)
{
    Matrix m2(*this);
    SetMultiply(m1, m2);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Inverse
//
//////////////////////////////////////////////////////////////////////////////

Matrix& Matrix::SetInverse(const Matrix& mStart)
{
    const int size = 4;
    bool bSwap = false;

    Matrix m(mStart);
    SetIdentity();

    for (int row = 0; row < size; row++) {
        //
        // pick the largest row in the rowth column
        //

        float max    = fabs(m.m_m[row][row]);
        int   rowMax = row;

        for (int rowIndex = row + 1; rowIndex < size; rowIndex++) {
            if (fabs(m.m_m[rowIndex][row]) > max) {
                max = fabs(m.m_m[rowIndex][row]);
                rowMax = rowIndex;
            }
        }

        //
        // swap this row with the largest row
        //

        if (rowMax != row) {
            //
            // swap rows
            //

            bSwap = true;
            for (int col = 0; col < size; col++) {
                Swap(m.m_m[row][col], m.m_m[rowMax][col]);
                Swap(  m_m[row][col],   m_m[rowMax][col]);
            }
        }

        //
        // scale to get a one in the diagonal
        //

        float scale = m.m_m[row][row];

        if (scale != 1.0f) {
            float rscale = 1.0f / scale;

            int col;
            m.m_m[row][row] = 1;
            for (col = row + 1; col < size; col++) {
                m.m_m[row][col] *= rscale;
            }

            if (bSwap) {
                for (col = 0; col < size; col++) {
                    m_m[row][col] *= rscale;
                }
            } else {
                m_m[row][row] = rscale;
                for (col = 0; col < row; col++) {
                    m_m[row][col] *= rscale;
                }
            }
        }

        //
        // get zeros in the rowth column of each row
        // by subtracting a multiple of row from row2
        //

        for (int row2 = 0; row2 < size; row2++) {
            if (row2 != row) {
                float scale = m.m_m[row2][row];

                if (scale != 0) {
                    int col;

                    m.m_m[row2][row] = 0;
                    for (col = row + 1; col < size; col++) {
                        m.m_m[row2][col] -= scale * m.m_m[row][col];
                    }

                    if (bSwap) {
                        for (col = 0; col < size; col++) {
                            m_m[row2][col] -= scale * m_m[row][col];
                        }
                    } else {
                        for (col = 0; col <= row; col++) {
                            m_m[row2][col] -= scale * m_m[row][col];
                        }
                    }
                }
            }
        }
    }

    m_type = TransformUnknown;

    return *this;
}

Matrix& Matrix::Inverse()
{
    Matrix mat(*this);
    SetInverse(mat);

    return *this;
}

//////////////////////////////////////////////////////////////////////////////
//
// Transpose
//
//////////////////////////////////////////////////////////////////////////////

Matrix& Matrix::SetTranspose(const Matrix& m)
{
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            m_m[row][col] = m.m_m[col][row];
        }
    }

    return *this;
}

Matrix& Matrix::Transpose()
{
    for (int row = 0; row < 3; row++) {
        for (int col = row + 1; col < 4; col++) {
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

bool operator==(const Matrix& m1, const Matrix& m2)
{
    for (int row = 0; row < 3; row++) {
        for (int col = row + 1; col < 4; col++) {
            if (m1.m_m[row][col] != m2.m_m[row][col]) {
                return false;
            }
        }
    }

    return true;
}

bool operator!=(const Matrix& m1, const Matrix& m2)
{
    for (int row = 0; row < 3; row++) {
        for (int col = row + 1; col < 4; col++) {
            if (m1.m_m[row][col] == m2.m_m[row][col]) {
                return false;
            }
        }
    }

    return true;
}

Matrix operator*(const Matrix& m1, const Matrix& m2)
{
    Matrix m(m1);

    m.Multiply(m2);

    return m;
}

//////////////////////////////////////////////////////////////////////////////
//
// Transforms
//
//////////////////////////////////////////////////////////////////////////////

Point Matrix::Transform(const Point& point) const
{
    float   x = ((m_m[0][0] * point.X()) + (m_m[0][1] * point.Y()) + m_m[0][3]);
    float   y = ((m_m[1][0] * point.X()) + (m_m[1][1] * point.Y()) + m_m[1][3]);
    Point   p(x, y);

    return p;
}

HVector Matrix::Transform(const HVector& vec) const
{
    float x = vec.X();
    float y = vec.Y();
    float z = vec.Z();
    float w = vec.W();

    return
        HVector(
            m_m[0][0] * x + m_m[0][1] * y + m_m[0][2] * z + m_m[0][3] * w,
            m_m[1][0] * x + m_m[1][1] * y + m_m[1][2] * z + m_m[1][3] * w,
            m_m[2][0] * x + m_m[2][1] * y + m_m[2][2] * z + m_m[2][3] * w,
            m_m[3][0] * x + m_m[3][1] * y + m_m[3][2] * z + m_m[3][3] * w
        );
}

Vector Matrix::Transform(const Vector& vec) const
{
    //#ifndef FLOATASM
        float x = vec.X();
        float y = vec.Y();
        float z = vec.Z();

        return
            Vector(
                m_m[0][0] * x + m_m[0][1] * y + m_m[0][2] * z + m_m[0][3],
                m_m[1][0] * x + m_m[1][1] * y + m_m[1][2] * z + m_m[1][3],
                m_m[2][0] * x + m_m[2][1] * y + m_m[2][2] * z + m_m[2][3]
            );
    /*
    #else
        float* pm = m_m;
        Vector result;

        __asm {
            mov     esi, vec
            lea     edi, result
            mov     eax, pm

            fld     [esi].x         \\ x
            fld     [esi].y         \\ y x
            fld     [esi].z         \\ z y x
                                
            fld     [eax]           \\ m00 z y x
            fmul    st(3)           \\ m00*x z y x
            fld     [eax + 4]       \\ m01   m00*x z y x
            fmul    st(3)           \\ m01*y m00*x z y x
            fld     [eax + 8]       \\ m02   m01*y m00*x z y x
            fmul    st(3)           \\ m02*z mo1*y m00*x z y x
            fld     [eax + 12]      \\ m03   m02*z mo1*y m00*x z y x
            faddp   st(3), st(0)    \\ m02*z mo1*y m00*x+m03 z y x
            faddp   st(1), st(0)    \\ m02*z+mo1*y m00*x+m03 z y x


            fld     [eax + 16 + 0]  \\ m10 m02*z+mo1*y m00*x+m03 z y x
            fmul    st(5)           \\ m10*x m02*z+mo1*y m00*x+m03 z y x
            fxch    st(1)           \\ m02*z+mo1*y m10*x m00*x+m03 z y x
            faddp   st(2), st(0)    \\ m10*x m02*z+mo1*y+m00*x+m03 z y x
            fld     [eax + 16 + 4]  \\ m11 m10*x m02*z+mo1*y+m00*x+m03 z y x
            fmul    st(4)           \\ m11*y m10*x m02*z+mo1*y+m00*x+m03 z y x
            fld     [eax + 16]      \\ m12 m11*4 m10*x m02*z+mo1*y+m00*x+m03 z y x
            fmul    st(4)           \\ m12*z m11*4 m10*x m02*z+mo1*y+m00*x+m03 z y x
            fxch    st(3)           \\ m02*z+mo1*y+m00*x+m03 m11*4 m10*x m12*z z y x
            fstp    [edi].x         \\ m11*4 m10*x m12*z z y x
            fadd

        }
    #endif
    */
}

Vector Matrix::TransformDirection(const Vector& vec) const
{
     float x = vec.X();
     float y = vec.Y();
     float z = vec.Z();

     return
         Vector(
             m_m[0][0] * x + m_m[0][1] * y + m_m[0][2] * z,
             m_m[1][0] * x + m_m[1][1] * y + m_m[1][2] * z,
             m_m[2][0] * x + m_m[2][1] * y + m_m[2][2] * z
         );
}

void Matrix::Transform(const TVector<Point>& vSource, TVector<Point>& vDest) const
{
    int count = vSource.GetCount();
    vDest.SetCount(count);

    for (int index = 0; index < count; index++) {
        float x = vSource[index].X();
        float y = vSource[index].Y();

        vDest.Set(
            index,
            Point(
                m_m[0][0] * x + m_m[0][1] * y + m_m[0][3],
                m_m[1][0] * x + m_m[1][1] * y + m_m[1][3]
            )
        );
    }
}

void Matrix::Transform(const TVector<HVector>& vSource, TVector<HVector>& vDest) const
{
    int count = vSource.GetCount();
    vDest.SetCount(count);

    for (int index = 0; index < count; index++) {
        float x = vSource[index].X();
        float y = vSource[index].Y();
        float z = vSource[index].Z();

        vDest.Set(
            index,
            HVector(
                m_m[0][0] * x + m_m[0][1] * y + m_m[0][2] * z + m_m[0][3],
                m_m[1][0] * x + m_m[1][1] * y + m_m[1][2] * z + m_m[1][3],
                m_m[2][0] * x + m_m[2][1] * y + m_m[2][2] * z + m_m[2][3]
            )
        );
    }
}

void Matrix::Transform(const TVector<Vector>& vSource, TVector<Vector>& vDest) const
{
    int count = vSource.GetCount();
    vDest.SetCount(count);

    for (int index = 0; index < count; index++) {
        float x = vSource[index].X();
        float y = vSource[index].Y();
        float z = vSource[index].Z();

        vDest.Set(
            index,
            Vector(
                m_m[0][0] * x + m_m[0][1] * y + m_m[0][2] * z + m_m[0][3],
                m_m[1][0] * x + m_m[1][1] * y + m_m[1][2] * z + m_m[1][3],
                m_m[2][0] * x + m_m[2][1] * y + m_m[2][2] * z + m_m[2][3]
            )
        );
    }
}
