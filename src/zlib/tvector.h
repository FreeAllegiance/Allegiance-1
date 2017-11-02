#ifndef _TVector_H_
#define _TVector_H_

#include "tcompare.h"
#include "zassert.h"

#pragma warning(disable:4291) //no matching operator delete found; memory will not be freed if initialization throws an exception

//////////////////////////////////////////////////////////////////////////////
//
// Vector Template
//
//////////////////////////////////////////////////////////////////////////////

template<
    class TValue,
    class EqualsFunctionType  = DefaultNoEquals,
    class CompareFunctionType = DefaultNoCompare
>
class TVector {
protected:
    TValue* m_pvalue;
    int     m_count;
    int     m_countAlloc;

    EqualsFunctionType m_fnEquals;
    CompareFunctionType m_fnCompare;

public:
/*    class Iterator {
    private:
        TVector& m_vec;
        int      m_index;

    public:
        Iterator(TVector& vec) :
            m_vec(vec),
            m_index(0)
        {
        }

        const TValue& Value() { return m_vec[m_index];     }
        void First()    { m_index = 0;               }
        void Last()
        {
            if (m_count != 0) {
                m_index = m_count - 1;
            }
        }

        void Next()
        {
            ZAssert(m_index < m_count);

            m_index++;
        }

        void Prev()
        {
            ZAssert(m_index > 0);
            m_index--;
        }

        bool End()      { return m_index == m_count;     }
        bool IsFirst()  { return m_index == 0;           }
        bool IsLast()   { return m_index == m_count - 1; }
    };
*/
    TVector(int count = 0) :
        m_count(count),
        m_countAlloc(count)
    {
        ZAssert(m_countAlloc >= 0);

        if (m_countAlloc == 0) {
            m_pvalue = NULL;
        } else {
            m_pvalue = new TValue[m_countAlloc];
        }
    }

    TVector(int countAlloc, int initCount) :
        m_count(initCount),
        m_countAlloc(countAlloc)
    {
        ZAssert(m_countAlloc >= 0);
        ZAssert(m_count <= m_countAlloc);

        if (m_countAlloc == 0) {
            m_pvalue = NULL;
        } else {
            m_pvalue = new TValue[m_countAlloc]; //Fix memory leak -Imago 8/2/09
        }
    }

    TVector(const TVector& tvec) :
        m_count(tvec.GetCount()),
        m_countAlloc(tvec.GetCount())
    {
        if (m_countAlloc == 0) {
            m_pvalue = NULL;
        } else {
            m_pvalue = new TValue[m_countAlloc];

            for (int index = 0; index < m_count; index++) {
                m_pvalue[index] = tvec[index];
            }
        }
    }

    TVector(int count, TValue* pvalue, bool bStatic = false) :
        m_count(count)
    {
        ZAssert(count >= 0);

        if (bStatic) {
            m_countAlloc = 0;
            m_pvalue     = pvalue;
        } else {
            m_countAlloc = m_count;
            m_pvalue     = new TValue[m_count];

            for (int index = 0; index < m_count; index++) {
                m_pvalue[index] = pvalue[index];
            }
       }
    }

    ~TVector()
    {
        if (m_countAlloc != 0) {
            delete[] m_pvalue;
        }
    }

    void SetCount(int count)
    {
        if (m_countAlloc == 0) {
            if (count > m_count) {
                m_countAlloc = count;

                TValue* m_pvalueNew = new TValue[m_countAlloc]; //Fix memory leak -Imago 8/2/09

                for(int index = 0; index < m_count; index++) {
                    m_pvalueNew[index] = m_pvalue[index];
                }

                m_pvalue = m_pvalueNew;
            }
        } else if (count > m_countAlloc) {
            while (m_countAlloc < count) m_countAlloc *= 2;

            TValue* m_pvalueNew = new TValue[m_countAlloc]; //Fix memory leak -Imago 8/2/09

            for(int index = 0; index < m_count; index++) {
                m_pvalueNew[index] = m_pvalue[index];
            }

            delete[] m_pvalue;
            m_pvalue = m_pvalueNew;
        }

        m_count = count;
    }

    void Reserve(int count)
    {
        ZAssert(count > 0);

        if (count <= 0)
          return;

        if (m_countAlloc == 0) {
            m_pvalue = new TValue[count];
        } else if (count > m_countAlloc) {
            TValue* m_pvalueNew = new TValue[count];

            for(int index = 0; index < m_count; index++) {
                m_pvalueNew[index] = m_pvalue[index];
            }

            delete[] m_pvalue;
            m_pvalue = m_pvalueNew;
        }

        m_countAlloc = count;
    }

    void SetWritable()
    {
        if (m_countAlloc == 0 && m_count != 0) {
            m_countAlloc = m_count;

            TValue* m_pvalueNew = new TValue[m_countAlloc];

            for(int index = 0; index < m_count; index++) {
                m_pvalueNew[index] = m_pvalue[index];
            }

            m_pvalue = m_pvalueNew;
        }
    }

    void operator=(const TVector& tvec)
    {
        SetCount(tvec.GetCount());
        SetWritable();

        for (int index = 0; index < m_count; index++) {
            m_pvalue[index] = tvec.m_pvalue[index];
        }
    }

    const TValue& operator[](int index) const
    {
        ZAssert(index >= 0 && index < m_count);

        return m_pvalue[index];
    }

    TValue& Get(int index)
    {
        ZAssert(index >= 0 && index < m_count);

        SetWritable();
        return m_pvalue[index];
    }

    void Set(int index, const TValue& value)
    {
        ZAssert(index >= 0 && index < m_count);

        SetWritable();
        m_pvalue[index] = value;
    }

    void SetEmpty()
    {
        if (m_countAlloc != 0) {
            delete[] m_pvalue;
        }

        m_pvalue     = NULL;
        m_count      = 0;
        m_countAlloc = 0;
    }

    void SetAll(const TValue& value)
    {
        SetWritable();

        for (int index = 0; index < m_count; index++) {
            m_pvalue[index] = value;
        }
    }

    TValue& GetFront() const
    {
        ZAssert(m_count > 0);
        return m_pvalue[0];
    }

    TValue& GetEnd() const
    {
        ZAssert(m_count > 0);
        return m_pvalue[m_count - 1];
    }

    void Remove(int index)
    {
        ZAssert(index >= 0 && index < m_count);

        SetWritable();

        for (int i = index; i < m_count - 1; i++) {
            m_pvalue[i] = m_pvalue[i+1];
        }

        m_count--;
    }

    int Remove(const TValue& value)
    {
        int index = Find(value);

        if (index != -1) {
            Remove(index);
        }

        return index;
    }

    void Insert(int index, const TValue& value)
    {
        ZAssert(index >= 0 && index <= m_count);

        SetCount(m_count + 1);
        SetWritable();

        for (int i = m_count - 1; i > index; i--) {
            m_pvalue[i] = m_pvalue[i-1];
        }
        m_pvalue[index] = value;
    }

    void Insert(int index)
    {
        ZAssert(index >= 0 && index <= m_count);

        SetCount(m_count + 1);
        SetWritable();

        for (int i = m_count - 1; i > index; i--) {
            m_pvalue[i] = m_pvalue[i-1];
        }
    }

    void PushFront(const TValue& value)
    {
        Insert(0, value);
    }

    void PushFront()
    {
        Insert(0);
    }

    void PushEnd(const TValue& value)
    {
        Insert(GetCount(), value);
    }

    void PushEnd()
    {
        Insert(GetCount());
    }

    template<class Type>
    void PushEnd(const TVector<Type>& values)
    {
        int count     = m_count;
        int pushCount = values.GetCount();

        SetCount(count + pushCount);
        SetWritable();

        for (int index = 0; index < pushCount; index++) {
            m_pvalue[index + count] = values[index];
        }
    }

    int Find(const TValue& value)
    {
        for (int index = 0; index < m_count; index++) {
            if (m_fnEquals(((const TValue&)m_pvalue[index]), value)) {
                return index;
            }
        }

        return -1;
    }

    int GetCount() const
    {
        return m_count;
    }

    void Sort(int startArg, int endArg)
    {
        endArg--;
        if (startArg < endArg) {
            int start = startArg;
            int end   = endArg;
            TValue valueMid = m_pvalue[(start + end) / 2];

            do {
                while (start < end && !m_fnCompare(m_pvalue[start], valueMid)) {
                    start++;
                }
                while (start < end && !m_fnCompare(valueMid, m_pvalue[end])) {
                    end--;
                }

                if (start < end) {
                    TValue valueTemp = m_pvalue[start];
                    m_pvalue[start] = m_pvalue[end];
                    m_pvalue[end] = valueTemp;
                    start++;
                    end--;
                }
            } while (start < end);

            Sort(startArg, start);
            Sort(start + 1, endArg + 1);
        }
    }

    void Sort()
    {
        SetWritable();
        Sort(0, m_count);
    }
};

#endif
