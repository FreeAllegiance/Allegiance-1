#ifndef _TMap_H_
#define _TMap_H_

#include "zmath.h"
#include "tref.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Map Template
//
//////////////////////////////////////////////////////////////////////////////

template<class TKey, class TValue, class SinkFunctionType = NullFunc>
class TMap : public IObject {
public:
    class MapNode : public IObjectSingle {
    public:
        TKey m_key;
        TValue m_value;
        TRef<MapNode> m_pnode;

        MapNode(const TKey& key, const TValue& value, MapNode* pnode) :
            m_key(key),
            m_value(value),
            m_pnode(pnode)
        {}
    };

    TRef<MapNode> m_pnodes;
    int m_count;
    SinkFunctionType m_fnSink;

    SinkFunctionType& GetSink()
    {
        return m_fnSink;
    };

    MapNode* FindNode(const TKey& key) const
    {
        MapNode* pnode = m_pnodes;

        while (pnode && pnode->m_key >= key) {
            if (pnode->m_key == key) {
                return pnode;
            }
            pnode = pnode->m_pnode;
        }

        return NULL;
    }

    class Iterator : public IObject {
    private:
        TRef<MapNode> m_pnode;

    public:
        Iterator(TMap& map) :
            m_pnode(map.m_pnodes)
        {}

        virtual const TValue& Value()
        {
            return m_pnode->m_value;
        }

        virtual const TKey& Key()
        {
            return m_pnode->m_key;
        }

        virtual bool End()
        {
            return (m_pnode == NULL);
        }

        virtual bool Next()
        {
            if (m_pnode) {
                m_pnode = m_pnode->m_pnode;
            }

            return End();
        }
    };

    TMap() :
        m_pnodes(NULL),
        m_count(0)
    {}

    void Set(const TKey& key, const TValue& value)
    {
        TRef<MapNode>* ppnode = m_pnodes.Pointer();

        while ((*ppnode) && (*ppnode)->m_key > key) {
            ppnode = (*ppnode)->m_pnode.Pointer();
        }

        if ((*ppnode) && (*ppnode)->m_key == key) {
            (*ppnode)->m_value = value;
        } else {
            (*ppnode) = new MapNode(key, value, (*ppnode));
            ++m_count;
        }
        m_fnSink();
    }

    const bool Find(const TKey& key, TValue& value) const
    {
        MapNode* pnode = FindNode(key);
        if (pnode) {
            value = pnode->m_value;
        }
        return pnode != NULL;
    }

    const bool FindValue(const TValue& value, TKey& key)
    {
        MapNode* pnode = m_pnodes;

        while (pnode) {
            if (pnode->m_value == value) {
                key = pnode->m_key;
                return true;
            }
            pnode = pnode->m_pnode;
        }

        return false;
    }

    void Remove(const TKey& key)
    {
        TRef<MapNode>* ppnode = m_pnodes.Pointer();

        while (*ppnode) {
            if ((*ppnode)->m_key == key) {
                (*ppnode) = (*ppnode)->m_pnode;
                --m_count;
                m_fnSink();
                return;
            }
            ppnode = (*ppnode)->m_pnode.Pointer();
        }
    }

    void Remove(const TValue& value)
    {
        TRef<MapNode>* ppnode = m_pnodes.Pointer();

        while (*ppnode) {
            if ((*ppnode)->m_value == value) {
                (*ppnode) = (*ppnode)->m_pnode;
                --m_count;
                m_fnSink();
                return;
            }
            ppnode = (*ppnode)->m_pnode.Pointer();
        }
    }

    int Count() const
    {
        return m_count;
    }

    void SetEmpty()
    {
        m_count = 0;
        m_pnodes = NULL;
        m_fnSink();
    }

    bool IsEmpty() const
    {
        return (m_count == 0);
    }
};

#endif
