#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
// Constructors
//
//////////////////////////////////////////////////////////////////////////////

#ifdef EnableDebugString
    void Value::UpdateDebugString()
    {
        m_strDebug = GetString(1);
    }
#endif

Value::Value() :
    m_bChanged(true),
    m_pchildren(0, 0)
{
}

Value::Value(Value* pvalue0) :
    m_bChanged(true),
    m_pchildren(1, 1)
{
    m_pchildren.Set(0, pvalue0);

    pvalue0->AddParent(this);
}

Value::Value(Value* pvalue0, Value* pvalue1) :
    m_bChanged(true),
    m_pchildren(2, 2)
{
    m_pchildren.Set(0, pvalue0);
    m_pchildren.Set(1, pvalue1);

    pvalue0->AddParent(this);
    pvalue1->AddParent(this);
}

Value::Value(Value* pvalue0, Value* pvalue1, Value* pvalue2) :
    m_bChanged(true),
    m_pchildren(3, 3)
{
    m_pchildren.Set(0, pvalue0);
    m_pchildren.Set(1, pvalue1);
    m_pchildren.Set(2, pvalue2);

    pvalue0->AddParent(this);
    pvalue1->AddParent(this);
    pvalue2->AddParent(this);
}

Value::Value(Value* pvalue0, Value* pvalue1, Value* pvalue2, Value* pvalue3) :
    m_bChanged(true),
    m_pchildren(4, 4)
{
    m_pchildren.Set(0, pvalue0);
    m_pchildren.Set(1, pvalue1);
    m_pchildren.Set(2, pvalue2);
    m_pchildren.Set(3, pvalue3);

    pvalue0->AddParent(this);
    pvalue1->AddParent(this);
    pvalue2->AddParent(this);
    pvalue3->AddParent(this);
}

//////////////////////////////////////////////////////////////////////////////
//
// Destructor
//
//////////////////////////////////////////////////////////////////////////////

Value::~Value()
{
    int count = m_pchildren.GetCount();
    for (int index = 0; index < count; index++) {
        m_pchildren[index]->RemoveParent(this);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Private functions
//
//////////////////////////////////////////////////////////////////////////////

void Value::Evaluate()
{
}

TRef<Value> Value::Fold()
{
    return NULL;
}

bool Value::DoFold()
{
    bool bAnyFold = false;

    //
    // Fold all of the children first
    //

    int count = m_pchildren.GetCount();
    for (int index = 0; index < count; index++) {
        if (m_pchildren[index]->DoFold()) {
            bAnyFold = true;
        }
    }

    //
    // Now fold this node
    //

    TRef<Value> pvalueFold = Fold();

    if (pvalueFold) {
        if (pvalueFold != this) {
            ChangeTo(pvalueFold);
        }
        return true;
    }

    return bAnyFold;
}

void Value::ChildChanged(Value* pvalue, Value* pvalueNew)
{
    if (pvalueNew != NULL) {
        int count = m_pchildren.GetCount();
        for (int index = 0; index < count; index++) {
            if (pvalue == m_pchildren[index]) {
                SetChild(index, pvalueNew);
            }
        }

        // !!! DoFold();
    }

    Changed();
}

void Value::InternalUpdate()
{
    m_bChanged = false;

    int count = m_pchildren.GetCount();
    for (int index = 0; index < count; index++) {
        if (m_pchildren[index]->HasChanged()) {
            m_pchildren[index]->InternalUpdate();
        }
    }

    Evaluate();

    #ifdef EnableDebugString
        UpdateDebugString();
    #endif
}

void Value::Update()
{
    if (HasChanged()) {
        InternalUpdate();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Protected functions
//
//////////////////////////////////////////////////////////////////////////////

void Value::Changed()
{
    #ifdef EnableDebugString
        UpdateDebugString();
    #endif

    if (!m_bChanged) {
        m_bChanged = true;

        TList<Value*>::Iterator iter(m_listParents);

        while (!iter.End()) {
            Value* pvalue = iter.Value();

            //  , get this to work
            //if (!pvalue->m_bChanged) {
                pvalue->ChildChanged(this, NULL);
            //}
            iter.Next();
        }
    }
}

void Value::ChangeTo(Value* pvalue)
{
    ZAssert(pvalue != this);
    TRef<Value> pvalueSave = this;

    while (m_listParents.GetCount() != 0) {
        m_listParents.GetFront()->ChildChanged(this, pvalue);
    };
}

bool Value::IsConstant()
{
    return false;
}

void Value::AddParent(Value* pvalue)
{
    m_listParents.PushFront(pvalue);
}

void Value::RemoveParent(Value* pvalue)
{
    ZAssert(m_listParents.Find(pvalue));
    m_listParents.Remove(pvalue);
}

void Value::SetChild(int index, Value* pvalueChild)
{
	// BT - 10/17 - If there are no elements in the list, then just add this element.
	// Fixing crash: 8975122	211205	allegiance.exe	allegiance.exe	tvector.h	215	4	0	Win32 StructuredException at 0044750A : UNKNOWN	2017-10-05 11:54:01	0x0004750A	10	UNKNOWN
	if (m_pchildren.GetCount() == 0)
	{
		AddChild(pvalueChild);
	}
	else if (m_pchildren[index] != pvalueChild) {
        m_pchildren[index]->RemoveParent(this);
        pvalueChild->AddParent(this);
        m_pchildren.Set(index, pvalueChild);
        Changed();
    }
}

void Value::AddChild(Value* pvalueChild)
{
    m_pchildren.PushEnd(pvalueChild);
    pvalueChild->AddParent(this);
}

//////////////////////////////////////////////////////////////////////////////
//
// Binary Files
//
//////////////////////////////////////////////////////////////////////////////

void Value::WriteInternal(IMDLBinaryFile* pfile)
{
    if (m_pnsInfo != NULL) {
        //
        // This value is top level in some NameSpace
        //

        pfile->WriteReference(m_pnsInfo->GetName());
    } else {
        Write(pfile);
    }
}

void Value::WriteChildren(IMDLBinaryFile* pfile)
{
    int count = m_pchildren.GetCount();
    for (int index = count - 1; index >= 0; index--) {
        m_pchildren[index]->WriteInternal(pfile);
    }
}

void Value::Write(IMDLBinaryFile* pfile)
{
    WriteChildren(pfile);
    pfile->WriteReference(GetFunctionName());
    pfile->WriteApply();
}

void Value::FillImportTable(IMDLBinaryFile* pfile)
{
    if (m_pnsInfo) {
        if (pfile->AddImport(m_pnsInfo)) {
            return;
        }
    }

    ZString str = GetFunctionName();

    if (!str.IsEmpty()) {
        pfile->AddImport(str);
    }

    int count = m_pchildren.GetCount();
    for (int index = count - 1; index >= 0; index--) {
        m_pchildren[index]->FillImportTable(pfile);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Public functions
//
//////////////////////////////////////////////////////////////////////////////

void Value::SetNameSpaceInfo(INameSpaceInfo* pnsInfo)
{
    m_pnsInfo = pnsInfo;
}

int Value::GetNodeCount()
{
    int nodes = 1;

    int count = m_pchildren.GetCount();
    for (int index = 0; index < count; index++) {
        nodes += m_pchildren[index]->GetNodeCount();
    }

    return nodes;
}

ZString Value::GetFunctionName()
{
    ZUnimplemented();

    return ZString();
}

ZString Value::GetChildString(int indent)
{
    if (m_pnsInfo != NULL) {
        return m_pnsInfo->GetName();
    } else {
        return GetString(indent);
    }
}

ZString Value::Indent(int indent)
{
    static char* pchSpaces = "                                                                ";

    ZAssert(indent * 2 < 64);

    return ZString(pchSpaces, indent * 2);
}

ZString Value::GetString(int indent)
{
    // name ( args )

    ZString str = GetFunctionName() + "(\n";

    int count = m_pchildren.GetCount();
    for (int index = 0; index < count; index++) {
        str += Indent(indent + 1) + m_pchildren[index]->GetChildString(indent + 1);

        if (index != count - 1) {
            str += ",\n";
        } else {
            str += "\n";
        }
    }

    return str + Indent(indent) + ")";
}

//////////////////////////////////////////////////////////////////////////////
//
// Value List
//
//////////////////////////////////////////////////////////////////////////////

ValueList::ValueList(Site* psite) :
    m_psite(psite),
    m_list(),
    m_iter(m_list)
{
    ZAssert(psite == NULL || (((DWORD)psite) > 0x10));
}

ValueList::~ValueList()
{
    Value* pvalue = GetFirst();
    while (pvalue) {
        pvalue->RemoveParent(this);
        pvalue = GetNext();
    }
}

void ValueList::PushFront(Value* pvalue)
{
    pvalue->AddParent(this);
    m_list.PushFront(pvalue);
    Changed();
}

void ValueList::PushEnd(Value* pvalue)
{
    pvalue->AddParent(this);
    m_list.PushEnd(pvalue);
    Changed();
}

void ValueList::Remove(Value* pvalueArg)
{
    TRef<Value> pvalue = pvalueArg;

    if (m_list.Remove(pvalue)) {
        pvalue->RemoveParent(this);
        Changed();
    }
}

bool ValueList::DoFold()
{
    //
    // Fold all of the children
    //

    bool bAnyFold = false;

    TRef<Value> pvalue = GetFirst();
    while (pvalue) {
        if (pvalue->DoFold()) {
            bAnyFold = true;
        }

        pvalue = GetNext();
    }

    return bAnyFold;
}

void ValueList::ChildChanged(Value* pvalue, Value* pvalueNew)
{
    ZAssert(m_list.Find(pvalue));

    if (pvalueNew) {
        pvalue->RemoveParent(this);
        pvalueNew->AddParent(this);
        if (m_psite != NULL && m_psite->RemoveValue(pvalueNew)) {
            m_list.Remove(pvalue);
        } else {
            m_list.Replace(pvalue, pvalueNew);
        }
    }

    Changed();
}

void ValueList::InternalUpdate()
{
    m_bChanged = false;

    Value* pvalue = GetFirst();
    while (pvalue) {
        if (pvalue->HasChanged()) {
            pvalue->InternalUpdate();
        }
        pvalue = GetNext();
    }
}

int ValueList::GetNodeCount()
{
    int nodes = 1;

    Value* pvalue = GetFirst();
    while (pvalue) {
        nodes += pvalue->GetNodeCount();
        pvalue = GetNext();
    }

    return nodes;
}

int ValueList::GetCount()
{
    return m_list.GetCount();
}

Value* ValueList::GetFirst()
{
    if (m_iter.First()) {
        return m_iter.Value();
    }

    return NULL;
}

Value* ValueList::GetLast()
{
    if (m_iter.Last()) {
        return m_iter.Value();
    }

    return NULL;
}

Value* ValueList::GetNext()
{
    if (m_iter.Next()) {
        return m_iter.Value();
    }

    return NULL;
}

Value* ValueList::GetPrevious()
{
    if (m_iter.Prev()) {
        return m_iter.Value();
    }

    return NULL;
}

Value* ValueList::GetCurrent()
{
    if (!m_iter.End()) {
        return m_iter.Value();
    } else {
        return NULL;
    }
}

Value* ValueList::RemoveCurrent()
{
    if (!m_iter.End()) {
        m_iter.Remove();
        if (!m_iter.End()) {
            return m_iter.Value();
        }
    }
    return NULL;
}

bool ValueList::IsFirst()
{
    return m_iter.IsFirst();
}

bool ValueList::IsLast()
{
    return m_iter.IsLast();
}

ZString ValueList::GetString(int indent)
{
    // [ args ]

    ZString str = "[\n";

    m_iter.First();

    while (!m_iter.End()) {
        str += Indent(indent + 1) + m_iter.Value()->GetChildString(indent + 1);

        m_iter.Next();

        if (m_iter.End()) {
            str += "\n";
        } else {
            str += ",\n";
        }
    }

    return str + Indent(indent) + "]";
}

void ValueList::Write(IMDLBinaryFile* pmdlFile)
{
    pmdlFile->WriteList(m_list.GetCount());

    m_iter.First();

    while (!m_iter.End()) {
        m_iter.Value()->Write(pmdlFile);
        pmdlFile->WriteEnd();
        m_iter.Next();
    }
}

void ValueList::FillImportTable(IMDLBinaryFile* pfile)
{
    m_iter.First();

    while (!m_iter.End()) {
        m_iter.Value()->FillImportTable(pfile);
        m_iter.Next();
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// Number Subclasses
//
//////////////////////////////////////////////////////////////////////////////

class SubtractNumber : public Number {
public:
    SubtractNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() - Get1()->GetValue();
    }
};

class AddNumber : public Number {
public:
    AddNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() + Get1()->GetValue();
    }
};

class MultiplyNumber : public Number {
public:
    MultiplyNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() * Get1()->GetValue();
    }
};

class DivideNumber : public Number {
public:
    DivideNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = Get0()->GetValue() / Get1()->GetValue();
    }
};

class SinNumber : public Number {
public:
    SinNumber(Number* pvalue0) :
        Number(pvalue0)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }

    void Evaluate()
    {
        GetValueInternal() = sin(Get0()->GetValue());
    }
};

class CosNumber : public Number {
public:
    CosNumber(Number* pvalue0) :
        Number(pvalue0)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }

    void Evaluate()
    {
        GetValueInternal() = cos(Get0()->GetValue());
    }
};

class OrNumber : public Number {
public:
    OrNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
            (float)(
                  ((DWORD)Get0()->GetValue())
                | ((DWORD)Get1()->GetValue())
            );
    }
};

class AndNumber : public Number {
public:
    AndNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
            (float)(
                  ((DWORD)Get0()->GetValue())
                & ((DWORD)Get1()->GetValue())
            );
    }
};

class XOrNumber : public Number {
public:
    XOrNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
            (float)(
                  ((DWORD)Get0()->GetValue())
                ^ ((DWORD)Get1()->GetValue())
            );
    }
};

class ModNumber : public Number {
public:
    ModNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = mod(Get0()->GetValue(), Get1()->GetValue());
    }
};

class MinNumber : public Number {
public:
    MinNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = min(Get0()->GetValue(), Get1()->GetValue());
    }
};

class MaxNumber : public Number {
public:
    MaxNumber(Number* pvalue0, Number* pvalue1) :
        Number(pvalue0, pvalue1)
    {
    }

    Number* Get0() { return Number::Cast(GetChild(0)); }
    Number* Get1() { return Number::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() = max(Get0()->GetValue(), Get1()->GetValue());
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Number
//
//////////////////////////////////////////////////////////////////////////////

TRef<Number> Subtract(Number* pvalue1, Number* pvalue2)
{
    return new SubtractNumber(pvalue1, pvalue2);
}

TRef<Number> Add(Number* pvalue1, Number* pvalue2)
{
    return new AddNumber(pvalue1, pvalue2);
}

TRef<Number> Multiply(Number* pvalue1, Number* pvalue2)
{
    return new MultiplyNumber(pvalue1, pvalue2);
}

TRef<Number> Divide(Number* pvalue1, Number* pvalue2)
{
    return new DivideNumber(pvalue1, pvalue2);
}

TRef<Number> Or(Number* pvalue1, Number* pvalue2)
{
    return new OrNumber(pvalue1, pvalue2);
}

TRef<Number> And(Number* pvalue1, Number* pvalue2)
{
    return new AndNumber(pvalue1, pvalue2);
}

TRef<Number> XOr(Number* pvalue1, Number* pvalue2)
{
    return new XOrNumber(pvalue1, pvalue2);
}

TRef<Number> Mod(Number* pvalue1, Number* pvalue2)
{
    return new ModNumber(pvalue1, pvalue2);
}

TRef<Number> Min(Number* pvalue1, Number* pvalue2)
{
    return new MinNumber(pvalue1, pvalue2);
}

TRef<Number> Max(Number* pvalue1, Number* pvalue2)
{
    return new MaxNumber(pvalue1, pvalue2);
}

TRef<Number> Sin(Number* pvalue)
{
    return new SinNumber(pvalue);
}

TRef<Number> Cos(Number* pvalue)
{
    return new CosNumber(pvalue);
}

//////////////////////////////////////////////////////////////////////////////
//
// Boolean
//
//////////////////////////////////////////////////////////////////////////////

class AndBoolean : public Boolean {
public:
    AndBoolean(Boolean* pvalue0, Boolean* pvalue1) :
        Boolean(pvalue0, pvalue1)
    {
    }

    Boolean* Get0() { return Boolean::Cast(GetChild(0)); }
    Boolean* Get1() { return Boolean::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
               Get0()->GetValue()
            && Get1()->GetValue();
    }
};

class OrBoolean : public Boolean {
public:
    OrBoolean(Boolean* pvalue0, Boolean* pvalue1) :
        Boolean(pvalue0, pvalue1)
    {
    }

    Boolean* Get0() { return Boolean::Cast(GetChild(0)); }
    Boolean* Get1() { return Boolean::Cast(GetChild(1)); }

    void Evaluate()
    {
        GetValueInternal() =
               Get0()->GetValue()
            || Get1()->GetValue();
    }
};

class NotBoolean : public Boolean {
public:
    NotBoolean(Boolean* pvalue0) :
        Boolean(pvalue0)
    {
    }

    Boolean* Get0() { return Boolean::Cast(GetChild(0)); }

    void Evaluate()
    {
        GetValueInternal() = !(Get0()->GetValue());
    }
};

TRef<Boolean> And(Boolean* pvalue1, Boolean* pvalue2)
{
    return new AndBoolean(pvalue1, pvalue2);
}

TRef<Boolean> Or(Boolean* pvalue1, Boolean* pvalue2)
{
    return new OrBoolean(pvalue1, pvalue2);
}

TRef<Boolean> Not(Boolean* pvalue1)
{
    return new NotBoolean(pvalue1);
}

//////////////////////////////////////////////////////////////////////////////
//
// GetString
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const ZString& str)
{
    //  , handle escape charaters within the string
    return "\"" + str + "\"";
}

ZString GetString(int indent, const Color& color)
{
    return
          "Color("
        + ZString(color.GetRed()  ) + ", "
        + ZString(color.GetGreen()) + ", "
        + ZString(color.GetBlue() ) + ")";
}

ZString GetString(int indent, const Point& point)
{
    return
          "Point("
        + ZString(point.X()) + ", "
        + ZString(point.Y()) + ")";
}

ZString GetString(int indent, const WinPoint& point)
{
	return
		"WinPoint("
		+ ZString(point.X()) + ", "
		+ ZString(point.Y()) + ")";
}

ZString GetString(int indent, const Rect& Rect)
{
    return
          "Rect("
        + ZString(Rect.XMin()) + ", "
        + ZString(Rect.YMin()) + ", "
        + ZString(Rect.XMax()) + ", "
        + ZString(Rect.YMax()) + ")";
}

ZString GetString(int indent, const Orientation& orient)
{
    return
          "Orientation("
        + GetString(indent, orient.GetUp()) + ", "
        + GetString(indent, orient.GetForward()) + ")";
}

ZString GetString(int indent, const Vector& vec)
{
    return
          "Vector("
        + ZString(vec.X()) + ", "
        + ZString(vec.Y()) + ", "
        + ZString(vec.Z()) + ")";
}

ZString GetString(int indent, float value)
{
    return ZString(value);
}

ZString GetString(int indent, bool value)
{
    return value ? "true" : "false";
}

ZString GetString(int indent, const Matrix& mat)
{
    ZString str;

    str = "Matrix(\n";
    str += Value::Indent(indent + 1) + ZString(mat[0][0]) + ", "+ ZString(mat[0][1]) + ", "+ ZString(mat[0][2]) + ", "+ ZString(mat[0][3]) + ",\n";
    str += Value::Indent(indent + 1) + ZString(mat[1][0]) + ", "+ ZString(mat[1][1]) + ", "+ ZString(mat[1][2]) + ", "+ ZString(mat[1][3]) + ",\n";
    str += Value::Indent(indent + 1) + ZString(mat[2][0]) + ", "+ ZString(mat[2][1]) + ", "+ ZString(mat[2][2]) + ", "+ ZString(mat[2][3]) + ",\n";
    str += Value::Indent(indent + 1) + ZString(mat[3][0]) + ", "+ ZString(mat[3][1]) + ", "+ ZString(mat[3][2]) + ", "+ ZString(mat[3][3]) + "\n";
    str += Value::Indent(indent) + ")";

    return str;
}

ZString GetString(int indent, const Matrix2& mat)
{
    return
          "Matrix2(\n"
        + Value::Indent(indent + 1) + ZString(mat[0][0]) + ", "+ ZString(mat[0][1]) + ", "+ ZString(mat[0][2]) + ",\n"
        + Value::Indent(indent + 1) + ZString(mat[1][0]) + ", "+ ZString(mat[1][1]) + ", "+ ZString(mat[1][2]) + ",\n"
        + Value::Indent(indent + 1) + ZString(mat[2][0]) + ", "+ ZString(mat[2][1]) + ", "+ ZString(mat[2][2]) + "\n"
        + Value::Indent(indent) + ")";
}

//////////////////////////////////////////////////////////////////////////////
//
// Write to binary file
//
//////////////////////////////////////////////////////////////////////////////

ZString GetFunctionName(const ZString& value)
{
    return ZString();
}

void Write(IMDLBinaryFile* pmdlFile, const ZString& value)
{
    pmdlFile->WriteString(value);
}

ZString GetFunctionName(float value)
{
    return ZString();
}

void Write(IMDLBinaryFile* pmdlFile, float value)
{
    pmdlFile->WriteNumber(value);
}

ZString GetFunctionName(bool value)
{
    return ZString();
}

void Write(IMDLBinaryFile* pmdlFile, bool value)
{
    pmdlFile->WriteBoolean(value);
}

ZString GetFunctionName(const Orientation& value)
{
    return "Orientation";
}

void Write(IMDLBinaryFile* pmdlFile, const Orientation& value)
{
    pmdlFile->WriteReference("Orientation");
    TRef<ZFile> pfile = pmdlFile->WriteBinary();
    pfile->Write((void*)&value, sizeof(value));
}

ZString GetFunctionName(const Rect& value)
{
    return "Rect";
}

void Write(IMDLBinaryFile* pmdlFile, const Rect& value)
{
    pmdlFile->WriteReference("Rect");
    TRef<ZFile> pfile = pmdlFile->WriteBinary();
    pfile->Write((void*)&value, sizeof(value));
}

ZString GetFunctionName(const Color& value)
{
    return "Color";
}

void Write(IMDLBinaryFile* pmdlFile, const Color& value)
{
    pmdlFile->WriteReference("Color");
    TRef<ZFile> pfile = pmdlFile->WriteBinary();
    pfile->Write((void*)&value, sizeof(value));
}

ZString GetFunctionName(const Point& value)
{
    return "Point";
}

ZString GetFunctionName(const WinPoint& value)
{
	return "WinPoint";
}

void Write(IMDLBinaryFile* pmdlFile, const Point& value)
{
    pmdlFile->WriteReference("Point");
    TRef<ZFile> pfile = pmdlFile->WriteBinary();
    pfile->Write((void*)&value, sizeof(value));
}

void Write(IMDLBinaryFile* pmdlFile, const WinPoint& value)
{
	pmdlFile->WriteReference("WinPoint");
	TRef<ZFile> pfile = pmdlFile->WriteBinary();
	pfile->Write((void*)&value, sizeof(value));
}

ZString GetFunctionName(const Vector& value)
{
    return "Vector";
}

void Write(IMDLBinaryFile* pmdlFile, const Vector& value)
{
    pmdlFile->WriteReference("Vector");
    TRef<ZFile> pfile = pmdlFile->WriteBinary();
    pfile->Write((void*)&value, sizeof(value));
}

ZString GetFunctionName(const Matrix& value)
{
    return "Matrix";
}

void Write(IMDLBinaryFile* pmdlFile, const Matrix& value)
{
    pmdlFile->WriteReference("Matrix");
    TRef<ZFile> pfile = pmdlFile->WriteBinary();
    pfile->Write((void*)&value, sizeof(value));
}
