#ifndef _matrix_h_
#define _matrix_h_

#include <cstdint>

#include "point.h"
#include "tvector.h"
#include "vertex.h"

//////////////////////////////////////////////////////////////////////////////
//
// Transform Types
//
//////////////////////////////////////////////////////////////////////////////

typedef uint32_t TransformType;

#define TransformUnknown    -1
#define TransformIdentity    0
#define TransformTranslate   1
#define TransformScale       2
#define TransformRotate      4
#define TransformPerspective 8

//////////////////////////////////////////////////////////////////////////////
//
// Determinants
//
//////////////////////////////////////////////////////////////////////////////

template<class Type>
inline Type Determinant2( // 2 mul 1 add
    Type m00, Type m01,
    Type m10, Type m11
) {
    return m00 * m11 - m10 * m01;
}

template<class Type>
inline Type Determinant3(  // 9 mul 5 add
    Type m00, Type m01, Type m02,
    Type m10, Type m11, Type m12,
    Type m20, Type m21, Type m22
) {
    Type m0 = m00 * Determinant2(m11, m12, m21, m22);
    Type m1 = m10 * Determinant2(m21, m22, m01, m02);
    Type m2 = m20 * Determinant2(m01, m02, m11, m12);

    return m0 + m1 + m2;
}

float Determinant(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33
);

//////////////////////////////////////////////////////////////////////////////
//
// Matrix2
//
//////////////////////////////////////////////////////////////////////////////
class Matrix;
class Matrix2 {
private:
    float m_m[3][3];
    TransformType m_type;

    void CalcType();

public:
    static const Matrix2& GetIdentity();

    Matrix2() : m_type(TransformUnknown) {}
    Matrix2(
        float v00, float v01, float v02,
        float v10, float v11, float v12,
        float v20 = 0, float v21 = 0, float v22 = 1
    );

    //
    // Types
    //

    TransformType GetType() const;
    void InvalidateType();

    //
    // operators
    //

    friend bool   operator==(const Matrix& m1, const Matrix& m2);
    friend bool   operator!=(const Matrix& m1, const Matrix& m2);
    friend Matrix operator*(const Matrix& m1, const Matrix& m2);

    //
    // Accessors
    //

    const float* operator[](int row) const { return m_m[row]; }
    float GetScale() const;
    Point GetTranslate() const;

    //
    // Set Methods
    //

    Matrix2& SetIdentity();

    void Set(int row, int col, float value);

    Matrix2& SetScale(float scale);
    Matrix2& Scale(float scale);
    Matrix2& PreScale(float scale);

    Matrix2& SetScale(const Point& point);
    Matrix2& Scale(const Point& point);
    Matrix2& PreScale(const Point& point);

    Matrix2& SetTranslate(const Point& point);
    Matrix2& Translate(const Point& point);
    Matrix2& PreTranslate(const Point& point);

    Matrix2& SetRotate(float angle);
    Matrix2& Rotate(float angle);
    Matrix2& PreRotate(float angle);

    Matrix2& SetMultiply(const Matrix2& m1, const Matrix2& m2);
    Matrix2& Multiply(const Matrix2& m2);
    Matrix2& PreMultiply(const Matrix2& m1);

    float Determinant() const;

    Matrix2& SetInverse(const Matrix2& m);
    Matrix2& Inverse();

    Matrix2& SetTranspose(const Matrix2& m);
    Matrix2& Transpose();

    //
    // Transforms
    //

    Point Transform(const Point& point) const;
    void  Transform(const TVector<Point>&, TVector<Point>&) const;
};

//////////////////////////////////////////////////////////////////////////////
//
// Matrix
//
//////////////////////////////////////////////////////////////////////////////
class Orientation;
class Vector;
class Matrix {
private:
    float m_m[4][4];
    TransformType m_type;

    void CalcType();

public:
    static const Matrix& GetIdentity();

    Matrix() : m_type(TransformUnknown) {}
    Matrix(const Matrix2& mat2);
    Matrix(
        float v00, float v01, float v02, float v03,
        float v10, float v11, float v12, float v13,
        float v20, float v21, float v22, float v23,
        float v30 = 0, float v31 = 0, float v32 = 0, float v33 = 1
    );
    Matrix(
        const Orientation& orientation,
        const Vector&      position,
        float              scale);

    //
    // Types
    //

    TransformType GetType() const;
    void InvalidateType();

    //
    // Operators
    //

    friend bool operator==(const Matrix& m1, const Matrix& m2);
    friend bool operator!=(const Matrix& m1, const Matrix& m2);
    friend Matrix operator*(const Matrix& m1, const Matrix& m2);

    //
    // Accessors
    //

    const float* operator[](int row) const { return m_m[row]; }
    float GetScale() const;
    Vector GetTranslate() const;

    //
    // Set Methods
    //

    Matrix& SetIdentity();

    void Set(int row, int col, float value);

    Matrix& SetLookAtFrom(const Vector& vecAt, const Vector& vecFrom, const Vector& vecUp);
    Matrix& LookAtFrom(const Vector& vecAt, const Vector& vecFrom, const Vector& vecUp);
    Matrix& PreLookAtFrom(const Vector& vecAt, const Vector& vecFrom, const Vector& vecUp);

    Matrix& SetScale(float scale);
    Matrix& Scale(float scale);
    Matrix& PreScale(float scale);

    Matrix& SetScale(const Vector& vec);
    Matrix& Scale(const Vector& vec);
    Matrix& PreScale(const Vector& vec);

    Matrix& SetTranslate(const Vector& vec);
    Matrix& Translate(const Vector& vec);
    Matrix& PreTranslate(const Vector& vec);

    Matrix& SetRotateX(float angle);
    Matrix& SetRotateY(float angle);
    Matrix& SetRotateZ(float angle);
    Matrix& SetRotate(const Vector& vec, float angle);
    Matrix& Rotate(const Vector& vec, float angle);
    Matrix& PreRotate(const Vector& vec, float angle);

    Matrix& SetMultiply(const Matrix& m1, const Matrix& m2);
    Matrix& Multiply(const Matrix& m2);
    Matrix& PreMultiply(const Matrix& m1);

    float Determinant() const;

    Matrix& SetInverse(const Matrix& m);
    Matrix& Inverse();

    Matrix& SetTranspose(const Matrix& m);
    Matrix& Transpose();

    //
    // Transforms
    //

    Point  Transform(const Point& point) const;
    HVector Transform(const HVector& vec) const;
    Vector Transform(const Vector& vec) const;
    Vector TransformDirection(const Vector& vec) const;

    void Transform(const TVector<Point>&, TVector<Point>&) const;
    void Transform(const TVector<HVector>&, TVector<HVector>&) const;
    void Transform(const TVector<Vector>&, TVector<Vector>&) const;
};

#endif
