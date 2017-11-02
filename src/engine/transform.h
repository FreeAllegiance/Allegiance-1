#ifndef _Transform_h_
#define _Transform_h_

//////////////////////////////////////////////////////////////////////////////
//
// Transform
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const Matrix& mat);
ZString GetString(int indent, const Matrix2& mat);

ZString GetFunctionName(const Matrix& value);
void Write(IMDLBinaryFile* pmdlFile, const Matrix& value);

class Transform : public TStaticValue<Matrix> {
protected:
    Transform(Value* pvalue0) :
        TStaticValue<Matrix>(pvalue0)
    {
    }

    Transform(Value* pvalue0, Value* pvalue1) :
        TStaticValue<Matrix>(pvalue0, pvalue1)
    {
    }

    TRef<Value> Fold();

public:
    Transform() :
        TStaticValue<Matrix>(Matrix::GetIdentity())
    {
    }

    Transform(const Matrix& mat) :
        TStaticValue<Matrix>(mat)
    {
    }

    bool IsIdentity();
    DefineCastMember(Transform);
};

//////////////////////////////////////////////////////////////////////////////
//
// MatrixTransform
//
//////////////////////////////////////////////////////////////////////////////

class MatrixTransform : public Transform {
public:
    MatrixTransform(const Matrix& mat) :
        Transform(mat)
    {
    }

    void SetValue(const Matrix& mat)
    {
        GetValueInternal() = mat;
    }

    ZString GetString(int indent)
    {
        return "MatrixTransform(" + ::GetString(indent, GetValue()) +")";
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Transform2
//
//////////////////////////////////////////////////////////////////////////////

class Transform2 : public TStaticValue<Matrix2> {
protected:
    Transform2(const Matrix2& mat) :
        TStaticValue<Matrix2>(mat)
    {
    }

    Transform2() :
        TStaticValue<Matrix2>(Matrix2::GetIdentity())
    {
    }

    Transform2(Value* pvalue0) :
        TStaticValue<Matrix2>(pvalue0)
    {
    }

    Transform2(Value* pvalue0, Value* pvalue1) :
        TStaticValue<Matrix2>(pvalue0, pvalue1)
    {
    }

public:
    DefineCastMember(Transform2);
};

class MatrixTransform2 : public Transform2 {
public:
    MatrixTransform2(const Matrix2& mat) :
        Transform2(mat)
    {
    }

    void SetValue(const Matrix2& mat)
    {
        GetValueInternal() = mat;
    }

    ZString GetString(int indent)
    {
        return "MatrixTransform2(" + ::GetString(indent, GetValue()) +")";
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Transform Values
//
//////////////////////////////////////////////////////////////////////////////

Transform*  GetIdentityTransform();
Transform2* GetIdentityTransform2();

//////////////////////////////////////////////////////////////////////////////
//
// Translate Transform
//
//////////////////////////////////////////////////////////////////////////////

class TranslateTransform : public Transform {
private:
    Vector m_vec;

    void Evaluate();

public:
    TranslateTransform(const Vector& vec);

    void SetVector(const Vector& vec);

    const Vector& GetVector() { return m_vec; }
};

//////////////////////////////////////////////////////////////////////////////
//
// Translate Transform2
//
//////////////////////////////////////////////////////////////////////////////

class TranslateTransform2 : public Transform2 {
private:
    void Evaluate();

public:
    TranslateTransform2(PointValue* ppoint);

    PointValue* GetTranslate() { return PointValue::Cast(GetChild(0)); }

    void SetTranslate(PointValue* ppoint);
    void SetTranslate(const Point& vec);

    //
    // Value members
    //

    ZString GetFunctionName() { return "TranslateTransform2"; }
};

//////////////////////////////////////////////////////////////////////////////
//
// Scale Transform2
//
//////////////////////////////////////////////////////////////////////////////

class ScaleTransform2 : public Transform2 {
private:
    void Evaluate();

public:
    ScaleTransform2(PointValue* ppoint);
	ScaleTransform2(const Point& point);

    PointValue* GetScale() { return PointValue::Cast(GetChild(0)); }

    void SetScale(PointValue* ppoint);
    void SetScale(const Point& vec);

    //
    // Value members
    //

    ZString GetFunctionName() { return "ScaleTransform2"; }
};

//////////////////////////////////////////////////////////////////////////////
//
// Rotate Transform2
//
//////////////////////////////////////////////////////////////////////////////

class RotateTransform2 : public Transform2 {
private:
    void Evaluate();

public:
    RotateTransform2(Number* pangle);

    Number* GetAngle() { return Number::Cast(GetChild(0)); }

    void SetAngle(Number* pangle);
    void SetAngle(float angle);

    //
    // Value members
    //

    ZString GetFunctionName() { return "RotateTransform2"; }
};

//////////////////////////////////////////////////////////////////////////////
//
// Rotate Transform
//
//////////////////////////////////////////////////////////////////////////////

class RotateTransform : public Transform {
private:
    Vector m_axis;
    float  m_angle;

    void Evaluate();

public:
    RotateTransform(const Vector& axis, float angle);

    void SetAngle(float angle);
    void SetAxis(const Vector& axis);

    float GetAngle()        { return m_angle; }
    const Vector& GetAxis() { return m_axis; }

    //
    // Value members
    //

    ZString GetFunctionName() { return "RotateTransform"; }
};

//////////////////////////////////////////////////////////////////////////////
//
// Scale Transform
//
//////////////////////////////////////////////////////////////////////////////

class ScaleTransform : public Transform {
private:
    Vector m_vec;
	TRef<Number> m_pScale;

    void Evaluate();

public:
    ScaleTransform(float scale);
	ScaleTransform(Number* pscale);
    ScaleTransform(const Vector& vec);

    void SetScale(float scale);

    float GetScale() { return m_vec.X(); }

    //
    // Value members
    //

    ZString GetFunctionName() { return "ScaleTransform"; }
};

//////////////////////////////////////////////////////////////////////////////
//
// Component Transform
//
//////////////////////////////////////////////////////////////////////////////

class ComponentTransform : public Transform {
private:
    float   m_scale;
    Vector  m_vecPos;
    Vector  m_vecForward;
    Vector  m_vecUp;

    void Evaluate();

public:
    ComponentTransform();
    ComponentTransform(
        const Vector& vecPos,
        const Vector& vecForward,
        const Vector& vecUp,
        float scale
    );

    virtual void SetOrientation(const Orientation& o);
    virtual void SetScale(float scale);
    virtual void SetPosition(const Vector& position);

    Vector GetPosition() { return m_vecPos;     }
    Vector GetForward()  { return m_vecForward; }
    Vector GetUp()       { return m_vecUp;      }
    float  GetScale()    { return m_scale;      }

    //
    // Value members
    //

    ZString GetFunctionName() { return "ComponentTransform"; }
};

//////////////////////////////////////////////////////////////////////////////
//
// Animate Rotate Transform
//
//////////////////////////////////////////////////////////////////////////////

class AnimateRotateTransform : public Transform {
private:
    void Evaluate();

public:
    AnimateRotateTransform(VectorValue* paxis, Number* pangle) :
        Transform(paxis, pangle)
    {
    }

    void  SetAxis(VectorValue* pvalue) { SetChild(0, pvalue); }
    void SetAngle(     Number* pvalue) { SetChild(1, pvalue); }

    VectorValue* GetAxis()  { return VectorValue::Cast(GetChild(0)); }
         Number* GetAngle() { return      Number::Cast(GetChild(1)); }

    //
    // Value members
    //

    ZString GetFunctionName() { return "AnimateRotateTransform"; }
};

#endif
