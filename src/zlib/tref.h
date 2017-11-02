#ifndef _tref_H_
#define _tref_H_

#include "StackTracer.h"

// BT - 9/17 - Hunting down mystery fedsrv crashes.

//////////////////////////////////////////////////////////////////////////////
//
// Base class for reference counted objects
//
//////////////////////////////////////////////////////////////////////////////

class IObjectSingle {
private:
	uint32_t m_count;

protected:
	typedef IObjectSingle QIType;


	void Internal_Release()
	{
		--m_count;
	}

public:
	IObjectSingle() :
		m_count(0)
	{
	}

	virtual ~IObjectSingle()
	{
	}


	uint32_t GetCount() const
	{
		return m_count;
	}

	virtual bool IsValid()
	{
		return true;
	}


#ifdef _DEBUG
	virtual
#endif
		uint32_t __stdcall AddRef()
	{
		return ++m_count;
	}

#ifdef _DEBUG
	virtual
#endif
		uint32_t __stdcall Release()
	{
		// BT - 9/17 - Hunting down mystery fedsrv crashes.
		__try
		{
			if (--m_count == 0) {
				delete this;
				return 0;
			}
		}
		__except (StackTracer::ExceptionFilter(GetExceptionInformation()))
		{
			StackTracer::OutputStackTraceToDebugF();
		}

		return m_count;
	}
};

//////////////////////////////////////////////////////////////////////////////
//
// Object Interface
//
//////////////////////////////////////////////////////////////////////////////

class IObject : virtual public IObjectSingle {
public:
};

//////////////////////////////////////////////////////////////////////////////
//
// Reference to a reference counted object
//
//////////////////////////////////////////////////////////////////////////////

template <class Type>
class TRef {
private:
	Type* m_pt;

public:
	//
	// constructors
	//

	TRef() :
		m_pt(NULL)
	{
	}

	TRef(const Type* pt)
	{
		m_pt = (Type*)pt;
		if (m_pt)
			m_pt->AddRef();
	}

	TRef(const TRef& tref) :
		m_pt(tref.m_pt)
	{
		if (m_pt)
			m_pt->AddRef();
	}

	template< class Other > TRef(const TRef<Other>& oref) :
		m_pt(static_cast<Other*>(oref))
	{
		if (m_pt)
			m_pt->AddRef();
	}


	//
	// assignment
	//

	TRef* Pointer()
	{
		return this;
	}

	Type** operator&()
	{
		if (m_pt) {
			m_pt->Release();
			m_pt = NULL;
		}
		return &m_pt;
	}

	TRef& operator=(const TRef& tref)
	{
		Type* ptOld = m_pt;
		m_pt = tref.m_pt;

		if (m_pt)
			m_pt->AddRef();

		if (ptOld)
			ptOld->Release();

		return *this;
	}
	/*
	TRef& operator=(const Type* ptNew)
	{
	Type* ptOld = m_pt;
	m_pt = (Type*)ptNew;

	if (m_pt)
	m_pt->AddRef();

	if (ptOld)
	ptOld->Release();

	return *this;
	}
	*/
	//
	// destructor
	//

	~TRef(void)
	{
		if (m_pt)
			m_pt->Release();
	}

	//
	// members
	//

	operator Type*(void) const { return m_pt; }
	Type& operator*(void) const { return *m_pt; }
	Type* operator->(void) const { return m_pt; }

	friend bool operator==(const TRef& t1, Type* pt)
	{
		return t1.m_pt == pt;
	}

	friend bool operator!=(const TRef& t1, Type* pt)
	{
		return t1.m_pt != pt;
	}


	// define comparison operators for use in associative containers
	inline friend bool operator<(const TRef& t1, Type* pt) { return t1.m_pt < pt; };
	inline friend bool operator>(const TRef& t1, Type* pt) { return t1.m_pt > pt; };
	inline friend bool operator<=(const TRef& t1, Type* pt) { return t1.m_pt <= pt; };
	inline friend bool operator>=(const TRef& t1, Type* pt) { return t1.m_pt >= pt; };
};

#endif
