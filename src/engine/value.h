#ifndef _Value_h_
#define _Value_h_

//////////////////////////////////////////////////////////////////////////////
//
// NameSpaceInfo
//
//////////////////////////////////////////////////////////////////////////////

class INameSpace;
class INameSpaceInfo : public IObject {
public:
    virtual       INameSpace* GetNameSpace() = 0;
    virtual const ZString&    GetName()      = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Binary MDL Files
//
//////////////////////////////////////////////////////////////////////////////

class IMDLBinaryFile : public IObject {
public:
    virtual void   AddImport(const ZString& str)      = 0;
    virtual bool   AddImport(INameSpaceInfo* pnsInfo) = 0;

    virtual void   WriteString(const ZString& str)    = 0;
    virtual void   WriteReference(const ZString& str) = 0;
    virtual void   WriteApply()                       = 0;
    virtual ZFile* WriteBinary()                      = 0;
    virtual void   WriteList(uint32_t count)          = 0;
    virtual void   WriteNumber(float value)           = 0;
    virtual void   WriteBoolean(bool value)           = 0;
    virtual void   WritePair()                        = 0;
    virtual void   WriteEnd()                         = 0;
};

#define MDLMagic   (uint32_t(0xdebadf00))
#define MDLVersion (uint32_t(0x00010000))

const uint32_t ObjectEnd        = 0;
const uint32_t ObjectFloat      = 1;
const uint32_t ObjectString     = 2;
const uint32_t ObjectTrue       = 3;
const uint32_t ObjectFalse      = 4;
const uint32_t ObjectList       = 5;
const uint32_t ObjectApply      = 6;
const uint32_t ObjectBinary     = 7;
const uint32_t ObjectReference  = 8;
const uint32_t ObjectImport     = 9;
const uint32_t ObjectPair       = 10;

//////////////////////////////////////////////////////////////////////////////
//
// Forward Declarations
//
//////////////////////////////////////////////////////////////////////////////

class ValueList;

//////////////////////////////////////////////////////////////////////////////
//
// Value headers
//
//////////////////////////////////////////////////////////////////////////////

class Value : public IObject {
private:
    // vtable                             // 1 DWORD
    // m_count                            // 1 DWORD
    TVector<TRef<Value> > m_pchildren;    // 2 DWORDS
    TList<Value*>         m_listParents;  // 3 DWORDS
    TRef<INameSpaceInfo>  m_pnsInfo;      // 1 DWORD
    bool                  m_bChanged;     // 1 DWORD

    virtual void        InternalUpdate();
    virtual void        Evaluate();
    virtual TRef<Value> Fold();

    void RemoveParent(Value* pvalue);
    void AddParent(Value* pvalue);

protected:
    Value();
    Value(Value*);
    Value(Value*, Value*);
    Value(Value*, Value*, Value*);
    Value(Value*, Value*, Value*, Value*);

    #ifdef _DEBUG
        /*
        #define EnableDebugString
        ZString m_strDebug;
        virtual void UpdateDebugString();
        */
    #endif

    virtual void ChildChanged(Value* pvalue, Value* pvalueNew);

    void   Changed();
    void   AddChild(Value* pchild);
    void   SetChild(int index, Value* m_pvalueChild);
    Value* GetChild(int index) const { return m_pchildren[index];     }
    int    GetChildCount()     const { return m_pchildren.GetCount(); }

public:
    virtual ~Value();

    void ChangeTo(Value* pvalue);
    bool HasChanged() { return m_bChanged; }
    void Update();
    virtual bool DoFold();
    virtual bool IsConstant();
    virtual int  GetNodeCount();

    //
    // NameSpace
    //

    void SetNameSpaceInfo(INameSpaceInfo* pnsInfo);

    //
    // MDL Binary Files
    //

  private:
    void WriteInternal(IMDLBinaryFile* pfile);

  public:
    virtual void WriteChildren(IMDLBinaryFile* pfile);
    virtual void Write(IMDLBinaryFile* pfile);
    virtual void FillImportTable(IMDLBinaryFile* pfile);

    //
    // MDL Text Files
    //

    static ZString Indent(int indent);
    virtual ZString GetFunctionName();
    virtual ZString GetString(int indent);
            ZString GetChildString(int indent);

    friend class ValueList;
};

#define DefineSubclassMembers(Type) \
    Type(Value* pvalue0) : Value(pvalue0) {} \
    Type(Value* pvalue0, Value* pvalue1) : Value(pvalue0, pvalue1) {} \
    Type(Value* pvalue0, Value* pvalue1, Value* value2) : Value(pvalue0, pvalue1, value2) {} \
    Type(Value* pvalue0, Value* pvalue1, Value* value2, Value* pvalue3) : Value(pvalue0, pvalue1, value2, pvalue3) {}

//////////////////////////////////////////////////////////////////////////////
//
// Value List
//
//////////////////////////////////////////////////////////////////////////////

class ValueList : public Value {
public:
    class Site {
    private:
        virtual bool RemoveValue(Value* pvalue) = 0;
        friend class ValueList;
    };

private:
    Site*                         m_psite;
    TList<TRef<Value> >           m_list;
    TList<TRef<Value> >::Iterator m_iter;

    virtual void InternalUpdate();
    virtual void ChildChanged(Value* pvalue, Value* pvalueNew);

public:
    DefineCastMember(ValueList);

    ValueList(Site* psite);
    ~ValueList();

    void PushFront(Value* pvalue);
    void PushEnd(Value* pvalue);
    void Remove(Value* pvalue);

    int    GetCount();
    Value* GetFirst();
    Value* GetLast();
    Value* GetNext();
    Value* GetPrevious();
    Value* GetCurrent();
    Value* RemoveCurrent();
    bool   IsFirst();
    bool   IsLast();

    ZString GetString(int indent);

    //
    // Value members
    //

    bool DoFold();
    int  GetNodeCount();

    void Write(IMDLBinaryFile* pmdlFile);
    void FillImportTable(IMDLBinaryFile* pfile);
};

//////////////////////////////////////////////////////////////////////////////
//
// Static Value
//
//////////////////////////////////////////////////////////////////////////////

template<class StaticType>
class TStaticValue : public Value {
private:
    StaticType m_value;

protected:
    DefineSubclassMembers(TStaticValue);

    StaticType& GetValueInternal()
    {
        return m_value;
    }

    TStaticValue()
    {
    }

public:
    DefineCastMember(TStaticValue<StaticType>);

    TStaticValue(const StaticType& value) :
        m_value(value)
    {
    }

    TRef<TStaticValue<StaticType> > MakeConstant()
    {
        return new TStaticValue(m_value);
    }

    const StaticType& GetValue()
    {
        Update();
        return m_value;
    }

    bool IsConstant()
    {
        return true;
    }

    ZString GetString(int indent)
    {
        return ::GetString(indent, m_value);
    }

    void Write(IMDLBinaryFile* pmdlFile)
    {
        ::Write(pmdlFile, m_value);
    }

    ZString GetFunctionName()
    {
        return ::GetFunctionName(m_value);
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Constant Static Value
//
//////////////////////////////////////////////////////////////////////////////

template<class StaticType, class FunctionName>
class TModifiableValue : public TStaticValue<StaticType> {
public:
    TModifiableValue(const StaticType& value) :
        TStaticValue<StaticType>(value)
    {
    }

    void SetValue(const StaticType& value)
    {
        GetValueInternal() = value;
        Changed();
    }

    bool IsConstant()
    {
        return false;
    }

    ZString GetFunctionName()
    {
        return FunctionName::GetName();
    }

    ZString GetString(int indent)
    {
        return FunctionName::GetName() + "(" + ::GetString(indent, GetValue()) +")";
    }

    void Write(IMDLBinaryFile* pmdlFile)
    {
        ::Write(pmdlFile, GetValue());
        pmdlFile->WriteReference(FunctionName::GetName());
        pmdlFile->WriteApply();
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Wrap Value
//
//////////////////////////////////////////////////////////////////////////////

template<class StaticType>
class TWrapValue : public TStaticValue<StaticType> {
protected:
    //
    // Value members
    //

    void Evaluate()
    {
         GetValueInternal() = GetWrappedValue()->GetValue();
    }

public:
    TWrapValue(TStaticValue<StaticType>* pvalue) :
        TStaticValue<StaticType>(pvalue)
    {
    }

    void SetWrappedValue(TStaticValue<StaticType>* pvalue) { SetChild(0, pvalue); }
    TStaticValue<StaticType>* GetWrappedValue() { return TStaticValue<StaticType>::Cast(GetChild(0)); }
};

//////////////////////////////////////////////////////////////////////////////
//
// String Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const ZString& str);
void Write(IMDLBinaryFile* pmdlFile, const ZString& value);
ZString GetFunctionName(const ZString& value);

typedef TStaticValue<ZString> StringValue;

class ModifiableStringName {
public:
    static ZString GetName() { return "ModifiableString"; }
};

typedef TModifiableValue<ZString, ModifiableStringName> ModifiableString;

//////////////////////////////////////////////////////////////////////////////
//
// Color Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const Color& color);
void Write(IMDLBinaryFile* pmdlFile, const Color& color);
ZString GetFunctionName(const Color& value);

typedef TStaticValue<Color> ColorValue;

class ModifiableColorName {
public:
    static ZString GetName() { return "ModifiableColor"; }
};

typedef TModifiableValue<Color, ModifiableColorName> ModifiableColorValue;

//////////////////////////////////////////////////////////////////////////////
//
// Point Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const Point& vec);
void Write(IMDLBinaryFile* pmdlFile, const Point& value);
ZString GetFunctionName(const Point& value);

typedef TStaticValue<Point> PointValue;

class ModifiablePointName {
public:
    static ZString GetName() { return "ModifiablePoint"; }
};

typedef TModifiableValue<Point, ModifiablePointName> ModifiablePointValue;

//////////////////////////////////////////////////////////////////////////////
//
// WinPoint Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const WinPoint& vec);
void Write(IMDLBinaryFile* pmdlFile, const WinPoint& value);
ZString GetFunctionName(const WinPoint& value);

typedef TStaticValue<WinPoint> WinPointValue;

class ModifiableWinPointName {
public:
	static ZString GetName() { return "ModifiableWinPoint"; }
};

typedef TModifiableValue<WinPoint, ModifiableWinPointName> ModifiableWinPointValue;

//////////////////////////////////////////////////////////////////////////////
//
// Rect Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const Rect& vec);
void Write(IMDLBinaryFile* pmdlFile, const Rect& value);
ZString GetFunctionName(const Rect& value);

typedef TStaticValue<Rect> RectValue;

class ModifiableRectName {
public:
    static ZString GetName() { return "ModifiableRect"; }
};

typedef TModifiableValue<Rect, ModifiableRectName> ModifiableRectValue;
typedef TWrapValue<Rect> WrapRectValue;

//////////////////////////////////////////////////////////////////////////////
//
// Orientation Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const Orientation& orient);
void Write(IMDLBinaryFile* pmdlFile, const Orientation& value);
ZString GetFunctionName(const Orientation& value);

typedef TStaticValue<Orientation> OrientationValue;

class ModifiableOrientationName {
public:
    static ZString GetName() { return "ModifiableOrientation"; }
};

typedef TModifiableValue<Orientation, ModifiableOrientationName> ModifiableOrientationValue;

//////////////////////////////////////////////////////////////////////////////
//
// Vector Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, const Vector& vec);
void Write(IMDLBinaryFile* pmdlFile, const Vector& value);
ZString GetFunctionName(const Vector& value);

typedef TStaticValue<Vector> VectorValue;

class ModifiableVectorName {
public:
    static ZString GetName() { return "ModifiableVector"; }
};

typedef TModifiableValue<Vector, ModifiableVectorName> ModifiableVectorValue;

//////////////////////////////////////////////////////////////////////////////
//
// Number Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, float value);
void Write(IMDLBinaryFile* pmdlFile, float value);
ZString GetFunctionName(float value);

typedef TStaticValue<float> Number;

class ModifiableNumberName {
public:
    static ZString GetName() { return "ModifiableNumber"; }
};

typedef TModifiableValue<float, ModifiableNumberName> ModifiableNumber;
typedef TWrapValue<float> WrapNumber;

TRef<Number> Subtract(Number*, Number*);
TRef<Number> Add(Number*, Number*);
TRef<Number> Multiply(Number*, Number*);
TRef<Number> Divide(Number*, Number*);
TRef<Number> Sin(Number*);
TRef<Number> Cos(Number*);
TRef<Number> Or(Number*, Number*);
TRef<Number> And(Number*, Number*);
TRef<Number> XOr(Number*, Number*);
TRef<Number> Mod(Number*, Number*);
TRef<Number> Min(Number*, Number*);
TRef<Number> Max(Number*, Number*);

//////////////////////////////////////////////////////////////////////////////
//
// Boolean Value
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, bool value);
void Write(IMDLBinaryFile* pmdlFile, bool value);
ZString GetFunctionName(bool value);

typedef TStaticValue<bool> Boolean;

class ModifiableBooleanName {
public:
    static ZString GetName() { return "ModifiableBoolean"; }
};

typedef TModifiableValue<bool, ModifiableBooleanName> ModifiableBoolean;

TRef<Boolean> And(Boolean*, Boolean*);
TRef<Boolean> Or(Boolean*, Boolean*);
TRef<Boolean> Not(Boolean*);

//////////////////////////////////////////////////////////////////////////////
//
// List of statics as a Value
//
//////////////////////////////////////////////////////////////////////////////

template<class Type>
class TListValue : public Value {
public:
    typedef TList<Type, DefaultNoEquals> List;

private:
    List m_list;

public:
    List& GetList() { return m_list; }

    ZString GetFunctionName()
    {
        return ZString();
    }

    ZString GetString(int indent)
    {
        ZString str = "[\n";

        List::Iterator iter(m_list);

        while (!iter.End()) {
            str += Value::Indent(indent + 1) + ::GetString(indent, iter.Value());
            iter.Next();
            if (!iter.End()) {
                str += ",\n";
            } else {
                str += "\n";
            }
        }

        return str + Value::Indent(indent) + "]";
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Casts
//
//////////////////////////////////////////////////////////////////////////////

inline ZString GetString(IObject* pobject)
{
    return StringValue::Cast(pobject)->GetValue();
}

inline float GetNumber(IObject* pobject)
{
    return Number::Cast(pobject)->GetValue();
}

inline bool GetBoolean(IObject* pobject)
{
    return Boolean::Cast(pobject)->GetValue();
}

inline Color GetColor(IObject* pobject)
{
    return ColorValue::Cast(pobject)->GetValue();
}

inline Vector GetVector(IObject* pobject)
{
    return VectorValue::Cast(pobject)->GetValue();
}

//////////////////////////////////////////////////////////////////////////////
//
// Object List
//
//////////////////////////////////////////////////////////////////////////////

typedef TPointerListObject<IObject> IObjectList;

//////////////////////////////////////////////////////////////////////////////
//
// Object Lookup Table
//
//////////////////////////////////////////////////////////////////////////////

template<class Type>
class DoSetIndex {
public:
	// mdvalley: has to be void
    static void SetIndex(Type* pobject, int index)
    {
        pobject->SetIndex(index);
    }
};

class DontSetIndex {
public:
	// mdvalley: It has to be explicit. And it ain't an int.
    static void SetIndex(void* pobject, int index)
    {
    }
};

template<class Type, class SetIndexType = DontSetIndex>
class TLookup {
public:
	// mdvalley: explicit void now
    static void Parse(IObjectList* plist, int offset, TVector<TRef<Type> >& m_v)
    {
        int maxIndex = offset;

        plist->GetFirst();
        while (plist->GetCurrent() != NULL) {
            TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

            TRef<Number>  pindex;  CastTo(pindex,  ppair->GetFirst() );
            int index = (int)pindex->GetValue() + offset;

            if (index > maxIndex) {
                maxIndex = index;
            }

            plist->GetNext();
        }

        m_v.SetCount(maxIndex + 1);

        plist->GetFirst();
        while (plist->GetCurrent() != NULL) {
            TRef<IObjectPair> ppair; CastTo(ppair, plist->GetCurrent());

            TRef<Number>  pindex;  CastTo(pindex,  ppair->GetFirst() );
            TRef<Type>    pobject = (Type*)(ppair->GetSecond());
            int index = (int)pindex->GetValue() + offset;

            m_v.Set(index, pobject);
            SetIndexType::SetIndex(pobject, index);

            plist->GetNext();
        }
    }
};

#endif
