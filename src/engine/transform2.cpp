#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Identity Transform2
//
//////////////////////////////////////////////////////////////////////////////

class IdentityTransform2 : public Transform2 {
public:
    IdentityTransform2() :
        Transform2()
    {
        GetValueInternal().SetIdentity();
        AddRef();
    }
} g_identityTransform2;

Transform2* GetIdentityTransform2()
{
    return &g_identityTransform2;
}

//////////////////////////////////////////////////////////////////////////////
//
// Translate Transform2
//
//////////////////////////////////////////////////////////////////////////////

TranslateTransform2::TranslateTransform2(PointValue* ppoint) :
    Transform2(ppoint)
{
}

void TranslateTransform2::Evaluate()
{
    GetValueInternal().SetTranslate(GetTranslate()->GetValue());
}

void TranslateTransform2::SetTranslate(PointValue* ppoint)
{
    SetChild(0, ppoint);
}

void TranslateTransform2::SetTranslate(const Point& point)
{
    SetTranslate(new PointValue(point));
}

//////////////////////////////////////////////////////////////////////////////
//
// Scale Transform2
//
//////////////////////////////////////////////////////////////////////////////

ScaleTransform2::ScaleTransform2(PointValue* ppoint) :
    Transform2(ppoint)
{
}

ScaleTransform2::ScaleTransform2(const Point& point) :
	Transform2(new PointValue(point))
{
}

void ScaleTransform2::Evaluate()
{
    GetValueInternal().SetScale(GetScale()->GetValue());
}

void ScaleTransform2::SetScale(PointValue* ppoint)
{
    SetChild(0, ppoint);
}

void ScaleTransform2::SetScale(const Point& point)
{
    SetScale(new PointValue(point));
}

//////////////////////////////////////////////////////////////////////////////
//
// Rotate Transform2
//
//////////////////////////////////////////////////////////////////////////////

RotateTransform2::RotateTransform2(Number* pangle) :
    Transform2(pangle)
{
}

void RotateTransform2::Evaluate()
{
    GetValueInternal().SetRotate(GetAngle()->GetValue());
}

void RotateTransform2::SetAngle(Number* pangle)
{
    SetChild(0, pangle);
}

void RotateTransform2::SetAngle(float angle)
{
    SetAngle(new Number(angle));
}
