#include "tlist.h"
#include "zassert.h"
//////////////////////////////////////////////////////////////////////////////
//
// TListImpl::IteratorImpl
//
//////////////////////////////////////////////////////////////////////////////

TListImpl::ListNodeImpl::~ListNodeImpl()
{}

//////////////////////////////////////////////////////////////////////////////
//
// TListImpl::IteratorImpl
//
//////////////////////////////////////////////////////////////////////////////

bool TListImpl::IteratorImpl::First()
{
    m_pnode = m_list.m_pfirst;
    return !End();
}

bool TListImpl::IteratorImpl::Last()
{
    m_pnode = m_list.m_plast;
    return !End();
}

bool TListImpl::IteratorImpl::Next()
{
    if (m_pnode) {
        m_pnode = m_pnode->m_pnext;
    }
    return !End();
}

bool TListImpl::IteratorImpl::Prev()
{
    if (m_pnode) {
        m_pnode = m_pnode->m_pprev;
    }
    return !End();
}

bool TListImpl::IteratorImpl::IsFirst()
{
    return m_pnode == m_list.m_pfirst;
}

bool TListImpl::IteratorImpl::IsLast()
{
    return m_pnode == m_list.m_plast;
}

void TListImpl::IteratorImpl::RemoveImpl()
{
    if (m_pnode) {
        TRef<ListNodeImpl> pnodeRemove = m_pnode;
        m_pnode = m_pnode->m_pnext;
        m_list.RemoveNode(pnodeRemove);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// TListImpl
//
//////////////////////////////////////////////////////////////////////////////

bool TListImpl::operator==(const TListImpl&)
{
    ZUnimplemented();
    return false;
}

void TListImpl::RemoveNode(ListNodeImpl* premove)
{
    TRef<ListNodeImpl> psave = premove;

    if (premove->m_pnext)
        premove->m_pnext->m_pprev = premove->m_pprev;
    else
        m_plast = premove->m_pprev;

    if (premove->m_pprev)
        premove->m_pprev->m_pnext = premove->m_pnext;
    else
        m_pfirst = premove->m_pnext;

    --m_count;
}

void TListImpl::PushFrontImpl(ListNodeImpl* pnew)
{
    if (m_pfirst)
        m_pfirst->m_pprev = pnew;

    m_pfirst = pnew;

    if (m_plast == NULL)
        m_plast = pnew;

    ++m_count;
}

void TListImpl::PushEndImpl(ListNodeImpl* pnew)
{
    if (m_plast)
        m_plast->m_pnext = pnew;

    m_plast = pnew;

    if (m_pfirst == NULL)
        m_pfirst = pnew;

    ++m_count;
}

void TListImpl::InsertBeforeImpl(ListNodeImpl* pnode, ListNodeImpl* pnew)
{
    pnew->m_pnext = pnode;
    pnew->m_pprev = pnode->m_pprev;

    if (pnode->m_pprev) {
        pnode->m_pprev->m_pnext = pnew;
    } else {
        m_pfirst = pnew;
    }

    pnode->m_pprev = pnew;

    ++m_count;
}

void TListImpl::InsertAfterImpl(ListNodeImpl* pnode, ListNodeImpl* pnew)
{
    pnew->m_pnext = pnode->m_pnext;
    pnew->m_pprev = pnode;

    if (pnode->m_pnext) {
        pnode->m_pnext->m_pprev = pnew;
    } else {
        m_plast = pnew;
    }

    pnode->m_pnext = pnew;

    ++m_count;
}

void TListImpl::PopFrontImpl()
{
    ZAssert(m_pfirst != NULL);

    m_pfirst = m_pfirst->m_pnext;
    if (m_pfirst) {
        m_pfirst->m_pprev = NULL;
    } else {
        m_plast = NULL;
    }

    m_count--;
}

void TListImpl::PopEndImpl()
{
    ZAssert(m_plast != NULL);

    m_plast = m_plast->m_pprev;
    if (m_plast) {
        m_plast->m_pnext = NULL;
    } else {
        m_pfirst = NULL;
    }

    m_count--;
}

void TListImpl::SetEmptyImpl()
{
    m_pfirst = NULL;
    m_plast = NULL;
    m_count = 0;
}

TListImpl::ListNodeImpl* TListImpl::Get(int index) const
{
    ZAssert(index >= 0 && index < m_count);

    ListNodeImpl* pnode = m_pfirst;
    while (index > 0)
    {
        pnode = pnode->m_pnext;
        index--;
    }

    return pnode;
}
