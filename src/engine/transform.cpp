#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Transform
//
//////////////////////////////////////////////////////////////////////////////

bool Transform::IsIdentity()
{
    return GetValueInternal().GetType() == TransformIdentity;
}

TRef<Value> Transform::Fold()
{
    //if (true) {
    if (IsConstant()) {
        return new Transform(GetValue());
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
// Identity Transform
//
//////////////////////////////////////////////////////////////////////////////

TRef<Transform> g_ptransformIdentity = new MatrixTransform(Matrix::GetIdentity());

Transform* GetIdentityTransform()
{
    return g_ptransformIdentity;
}

//////////////////////////////////////////////////////////////////////////////
//
// Translate Transform
//
//////////////////////////////////////////////////////////////////////////////

TranslateTransform::TranslateTransform(const Vector& vec) :
    m_vec(vec)
{
}

void TranslateTransform::Evaluate()
{
    GetValueInternal().SetTranslate(m_vec);
}

void TranslateTransform::SetVector(const Vector& vec)
{
    m_vec = vec;
    Changed();
}

//////////////////////////////////////////////////////////////////////////////
//
// Rotate Transform
//
//////////////////////////////////////////////////////////////////////////////

RotateTransform::RotateTransform(const Vector& axis, float angle) :
    m_angle(angle),
    m_axis(axis)
{
}

void RotateTransform::Evaluate()
{
    GetValueInternal().SetRotate(m_axis, m_angle);
}

void RotateTransform::SetAngle(float angle)
{
    m_angle = angle;
    Changed();
}

void RotateTransform::SetAxis(const Vector& axis)
{
    m_axis = axis;
    Changed();
}

//////////////////////////////////////////////////////////////////////////////
//
// Scale Transform
//
//////////////////////////////////////////////////////////////////////////////

ScaleTransform::ScaleTransform(float scale) :
    m_vec(scale, scale, scale)
{
}

ScaleTransform::ScaleTransform(Number* pscale) :
	m_pScale(pscale),
	m_vec(pscale->GetValue(), pscale->GetValue(), pscale->GetValue())
{
}
                               
ScaleTransform::ScaleTransform(const Vector& vec) :
    m_vec(vec)
{
}

void ScaleTransform::Evaluate()
{
	if (m_pScale && m_pScale->HasChanged())
	{
		SetScale(m_pScale->GetValue());
	}
    GetValueInternal().SetScale(m_vec);
}

void ScaleTransform::SetScale(float scale)
{
    m_vec = Vector(scale, scale, scale);
    Changed();
}

//////////////////////////////////////////////////////////////////////////////
//
// Component Transform
//
//////////////////////////////////////////////////////////////////////////////

ComponentTransform::ComponentTransform() :
    m_scale(1),
    m_vecPos(0, 0, 0),
    m_vecForward(0, 0, -1),
    m_vecUp(0, 1, 0)
{
}

ComponentTransform::ComponentTransform(
    const Vector& vecPos,
    const Vector& vecForward,
    const Vector& vecUp,
    float scale
) :
    m_scale(scale),
    m_vecPos(vecPos),
    m_vecForward(vecForward),
    m_vecUp(vecUp)
{
	ZAssert(vecUp.Length() != 0);
	ZAssert(vecForward.Length() != 0);
}

void ComponentTransform::Evaluate()
{
    GetValueInternal().SetScale(m_scale);
    GetValueInternal().LookAtFrom(m_vecForward + m_vecPos, m_vecPos, m_vecUp);
}

void ComponentTransform::SetPosition(const Vector& vecPos)
{
    m_vecPos = vecPos;
    Changed();
}

void ComponentTransform::SetScale(float scale)
{
    m_scale = scale;
    Changed();
}

void ComponentTransform::SetOrientation(const Orientation& o)
{
    m_vecForward = o.GetForward();
    m_vecUp      = o.GetUp();

	ZAssert(m_vecUp.Length() != 0);
	ZAssert(m_vecForward.Length() != 0);

    Changed();
}

//////////////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////////////

void AnimateRotateTransform::Evaluate()
{
    TRef<VectorValue> paxis = GetAxis();

    GetValueInternal().SetRotate(
        paxis->GetValue(),
        GetAngle()->GetValue()
    );
}

