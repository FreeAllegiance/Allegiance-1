#ifndef _TList_H_
#define _TList_H_

#include "tcompare.h"
#include "tref.h"
#include "zmath.h"

#pragma warning(disable:4291) //no matching operator delete found; memory will not be freed if initialization throws an exception

//////////////////////////////////////////////////////////////////////////////
//
//  List Template Generic Implementation
//
//////////////////////////////////////////////////////////////////////////////

class TListImpl : public IObject {
public:
    //
    // Types
    //

    class ListNodeImpl : public IObjectSingle {
    public:
        ListNodeImpl* m_pprev;
        TRef<ListNodeImpl> m_pnext;

        ListNodeImpl(ListNodeImpl* pprev, ListNodeImpl* pnext) :
            m_pprev(pprev),
            m_pnext(pnext)
        {}

        virtual ~ListNodeImpl();
    };

    class IteratorImpl : public IObject {
    protected:
        TListImpl&         m_list;
        TRef<ListNodeImpl> m_pnode;

        void RemoveImpl();
    public:
        IteratorImpl(TListImpl& list) :
            m_list(list),
            m_pnode(list.m_pfirst)
        {}

        bool End()
        {
            return (m_pnode == NULL);
        }

        bool First();
        bool Last();
        bool Next();
        bool Prev();
        bool IsFirst();
        bool IsLast();
    };

protected:
    friend class IteratorImpl;

    //
    // Data Members
    //

    TRef<ListNodeImpl> m_pfirst;
    TRef<ListNodeImpl> m_plast;
    int m_count;

    TListImpl() :
        m_pfirst(NULL),
        m_plast(NULL),
        m_count(0)
    {}

    //
    // Methods
    //

    void PushFrontImpl(ListNodeImpl* pnew);
    void PushEndImpl(ListNodeImpl* pnew);
    void InsertBeforeImpl(ListNodeImpl* pnode, ListNodeImpl* pnew);
    void InsertAfterImpl(ListNodeImpl* pnode, ListNodeImpl* pnew);

    void RemoveNode(ListNodeImpl* premove);
    void PopFrontImpl();
    void PopEndImpl();
    void SetEmptyImpl();

    ListNodeImpl* Get(int index) const;

public:
    int GetCount()
    {
        return m_count;
    }

    bool IsEmpty()
    {
        return (m_count == 0);
    }

    bool operator==(const TListImpl&);
};

//////////////////////////////////////////////////////////////////////////////
//
//  List Template
//
//////////////////////////////////////////////////////////////////////////////

template<
    class TValue,
    class EqualsFunctionType  = DefaultEquals,
    class CompareFunctionType = DefaultNoCompare,
    class SinkFunctionType = NullFunc
>
class TList : public TListImpl {
private:
    class ListNode : public TListImpl::ListNodeImpl {
    public:
        TValue m_value;

        ListNode(ListNodeImpl* pprev, ListNodeImpl* pnext) :
            TListImpl::ListNodeImpl(pprev, pnext)
        {}

        ListNode(const TValue& value, ListNodeImpl* pprev, ListNodeImpl* pnext) :
            TListImpl::ListNodeImpl(pprev, pnext),
            m_value(value)
        {}

        ListNode* GetNext()
        {
            return (ListNode*)(ListNodeImpl*)m_pnext;
        }

        ListNode* GetPrev()
        {
            return (ListNode*)(ListNodeImpl*)m_pprev;
        }
    };

    ListNode* FindInternal(const TValue& value)
    {
        ListNode* pfind = GetFirst();

        while (pfind) {
            if (m_fnEquals(((const TValue&)pfind->m_value), value)) {
                return pfind;
            }
            pfind = pfind->GetNext();
        }

        return NULL;
    }

    ListNode* GetFirst()
    {
        return (ListNode*)(ListNodeImpl*)m_pfirst;
    }

    ListNode* GetLast()
    {
        return (ListNode*)(ListNodeImpl*)m_plast;
    }

    SinkFunctionType m_fnSink;

    EqualsFunctionType m_fnEquals;
    CompareFunctionType m_fnCompare;

public:
    class Iterator : public TListImpl::IteratorImpl {
    public:
        Iterator(TList& list) :
            TListImpl::IteratorImpl(list)
        {}

        TValue& Value() { return ((ListNode*)(ListNodeImpl*)m_pnode)->m_value; }

        void Remove()
        {
            RemoveImpl();

            ((TList<TValue, EqualsFunctionType, CompareFunctionType, SinkFunctionType>&)m_list).GetSink()();
        }

        void InsertBefore(const TValue& value)
        {
            m_list.InsertBeforeImpl(m_pnode, new ListNodeImpl(value, NULL, NULL));
            ((TList<TValue, EqualsFunctionType, CompareFunctionType, SinkFunctionType>&)m_list).GetSink()();
        }

        void InsertAfter(TValue& value)
        {
            m_list.InsertAfterImpl(m_pnode, new ListNodeImpl(value, NULL, NULL));
            ((TList<TValue, EqualsFunctionType, CompareFunctionType, SinkFunctionType>&)m_list).GetSink()();
        }
    };

    TList()
    {
    }

    TList(EqualsFunctionType fnEquals, CompareFunctionType fnCompare)
        : m_fnEquals(fnEquals), m_fnCompare(fnCompare)
    {
    }

    TList(TList& list)
    {
        Iterator iter(list);

        while (!iter.End()) {
            PushEnd(iter.Value());
            iter.Next();
        }
    }

    void PushFront()
    {
        PushFrontImpl(new ListNode(NULL, GetFirst()));
        m_fnSink();
    }

    void PushFront(const TValue& value)
    {
        PushFrontImpl(new ListNode(value, NULL, GetFirst())); //Fix memory leak -Imago 8/2/09
        m_fnSink();
    }

    void PushEnd()
    {
        PushEndImpl(new ListNode(GetLast(), NULL));
        m_fnSink();
    }

    void PushEnd(const TValue& value)
    {
        PushEndImpl(new ListNode(value, GetLast(), NULL)); //Fix memory leak -Imago 8/2/09
        m_fnSink();
    }

    //
    // The compare function should return true if the first value
    // should be after the second value
    //

    void InsertSorted(const TValue& value)
    {
        ListNode* pnode = GetLast();

        while (pnode != NULL) {
            if (!m_fnCompare(((const TValue&)pnode->m_value), value)) {
                InsertAfterImpl(pnode, new ListNode(value, NULL, NULL));
                m_fnSink();
                return;
            }

            pnode = pnode->GetPrev();
        }

        PushFront(value);
        m_fnSink();
    }

    void InsertBefore(const TValue& valueReference, const TValue& valueNew)
    {
        InsertBeforeImpl(FindInternal(valueReference), new ListNode(valueNew, NULL, NULL));
        m_fnSink();
    }

    void InsertAfter(const TValue& valueReference, const TValue& valueNew)
    {
        InsertAfterImpl(FindInternal(valueReference), new ListNode(valueNew, NULL, NULL));
        m_fnSink();
    }

    TValue PopFront()
    {
        TValue value = GetFirst()->m_value;
        PopFrontImpl();
        m_fnSink();
        return value;
    }

    TValue PopEnd()
    {
        TValue value = GetLast()->m_value;
        PopEndImpl();
        m_fnSink();
        return value;
    }

    TValue& GetFront()
    {
        return GetFirst()->m_value;
    }

    TValue& GetEnd()
    {
        return GetLast()->m_value;
    }

    bool Find(const TValue& value)
    {
        return FindInternal(value) != NULL;
    }

    bool Remove(const TValue& value)
    {
        ListNode* premove = FindInternal(value);

        if (premove) {
            RemoveNode(premove);
            m_fnSink();
            return true;
        }

        return false;
    }

    bool Replace(const TValue& value, const TValue& valueNew)
    {
        ListNode* pfind = FindInternal(value);

        if (pfind) {
            pfind->m_value = valueNew;
        }

        m_fnSink();

        return false;
    }

    TValue& operator[](int index) const
    {
        return ((ListNode*)Get(index))->m_value;
    }

    void SetEmpty()
    {
        SetEmptyImpl();
        m_fnSink();
    }

    SinkFunctionType& GetSink()
    {
        return m_fnSink;
    };
};

//////////////////////////////////////////////////////////////////////////////
//
//  A List as an object
//
//////////////////////////////////////////////////////////////////////////////

template<class TValue>
class TListObject : public TList<TValue> {
public:
};

//////////////////////////////////////////////////////////////////////////////
//
//  List Template
//
//////////////////////////////////////////////////////////////////////////////

template<
    class TValue
>
class TPointerListObject : public IObject {
private:
    TList<TRef<TValue> >           m_list;

    typename TList<TRef<TValue> >::Iterator m_iter;

public:
    TPointerListObject() :
        m_list(),
        m_iter(m_list)
    {
    }

    void PushFront(TValue* pvalue) { m_list.PushFront(pvalue); }
    void PushEnd(TValue* pvalue)   { m_list.PushEnd(pvalue);   }
    void Remove(TValue* pvalue)    { m_list.Remove(pvalue);    }

    int  GetCount()                { return m_list.GetCount(); }
    bool IsFirst()                 { return m_iter.IsFirst();  }
    bool IsLast()                  { return m_iter.IsLast();   }

    TValue*  GetFirst()
    {
        if (m_iter.First()) {
            return m_iter.Value();
        }

        return NULL;
    }

    TValue*  GetLast()
    {
        if (m_iter.Last()) {
            return m_iter.Value();
        }

        return NULL;
    }

    TValue*  GetNext()
    {
        if (m_iter.Next()) {
            return m_iter.Value();
        }

        return NULL;
    }

    TValue*  GetPrevious()
    {
        if (m_iter.Prev()) {
            return m_iter.Value();
        }

        return NULL;
    }

    TValue*  GetCurrent()
    {
        if (!m_iter.End()) {
            return m_iter.Value();
        } else {
            return NULL;
        }
    }

    TValue* GetNth(int index)
    {
        GetFirst();
        while (index != 0) {
            GetNext();
            index--;
        }

        return GetCurrent();
    }

    TValue*  RemoveCurrent()
    {
        if (!m_iter.End()) {
            m_iter.Remove();
            if (!m_iter.End()) {
                return m_iter.Value();
            }
        }
        return NULL;
    }
};

#endif
