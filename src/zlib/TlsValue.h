#pragma once

#ifndef __TlsValue_h__
#define __TlsValue_h__

#include <mutex>

/////////////////////////////////////////////////////////////////////////////
// TlsValue.h : Declaration of the TlsValue template class.
//
/////////////////////////////////////////////////////////////////////////////
// TlsValue

template <class T>
class TlsValue
{
// Construction / Destruction
public:
  TlsValue(void)                         // Default Constructor (does nothing)
  {
  }
  TlsValue(const TlsValue& that)         // Copy Constructor
  {
    SetValue(that.GetValue());   
  }
  ~TlsValue()
  {
    TlsFree(m_dwSlot);
  }

// Attributes
public:
  T GetValue() const
  {
    return reinterpret_cast<T>(TlsGetValue(const_cast<TlsValue*> (this)->GetSlot()));
  }

  T SetValue(const T& value)
  {
    TlsSetValue(GetSlot(), reinterpret_cast<PVOID>(value));
    return value;
  }

// Operators
public:
  // Unary Operators
  T operator!()                          // Logical NOT
  {
    return !GetValue();
  }
  T operator~()                          // One's complement
  {
    return ~GetValue();
  }
  T operator+()                          // Unary plus
  {
    return +GetValue();
  }
  T operator++()                         // Pre-Increment
  {
    SetValue(GetValue() + 1);
    return *this;
  }
  T operator--()                         // Pre-Decrement
  {
    SetValue(GetValue() - 1);
    return *this;
  }
  T operator-()                          // Unary negation
  {
    return -GetValue();
  }
  T operator++(int)                      // Post-Increment
  {
    T temp(GetValue());
    SetValue(temp + 1);
    return temp;
  }
  T operator--(int)                      // Post-Decrement
  {
    T temp(GetValue());
    SetValue(temp - 1);
    return temp;
  }

  T operator%=(const T& value)           // Modulus/assignment 
  {
    SetValue(GetValue() % value);
    return *this;
  }
  T operator&=(const T& value)           // Bitwise AND/assignment
  {
    SetValue(GetValue() & value);
    return *this;
  }
  T operator*=(const T& value)           // Multiplication/assignment 
  {
    SetValue(GetValue() * value);
    return *this;
  }
  T operator+=(const T& value)           // Addition/assignment 
  {
    SetValue(GetValue() + value);
    return *this;
  }
  T operator-=(const T& value)           // Subtraction/assignment
  {
    SetValue(GetValue() - value);
    return *this;
  }
  T operator/=(const T& value)           // Division/assignment
  {
    SetValue(GetValue() / value);
    return *this;
  }
  T operator<<=(const T& value)          // Left shift/assignment 
  {
    SetValue(GetValue() << value);
    return *this;
  }
  T operator=(const TlsValue& that)      // Assignment
  {
    return SetValue(that.GetValue());
  }
  T operator=(const T& value)            // Assignment
  {
    return SetValue(value);
  }
  T operator>>=(const T& value)          // Right shift/assignment 
  {
    SetValue(GetValue() >> value);
    return *this;
  }
  T operator^=(const T& value)           // Exclusive OR/assignment
  {
    SetValue(GetValue() ^ value);
    return *this;
  }
  T operator|=(const T& value)           // Inclusive OR/assignment
  {
    SetValue(GetValue() | value);
    return *this;
  }

  // Casting Operator
  operator T() const
  {
    return GetValue();
  }


// Implementation
protected:
  static std::mutex &GetSyncObject()
  {
    // Shared by all instances, which is not a big deal since this is only
    // used when first allocating the TLS slot.
    static std::mutex s_cs;
    return s_cs;
  }
  uint32_t GetSlot()
  {
    if (m_dwSlot)                // Check for initialized slot
      return m_dwSlot;

	std::lock_guard<std::mutex> lock(GetSyncObject());
    if (!m_dwSlot)               // Check (again) for uninitialized slot
    {
      m_dwSlot = TlsAlloc();     // Allocate a TLS slot
      if (!m_dwSlot)             // Zero is valid, but not for us since zero
      {                          //   is used to indicate not-initialized
        m_dwSlot = TlsAlloc();   // Next index will not be zero
        TlsFree(0);              // Free the zero index
      }
    }
    return m_dwSlot;             // Return the (non-zero) slot
  }


// Data Members
protected:
  uint32_t m_dwSlot;
};


/////////////////////////////////////////////////////////////////////////////

typedef TlsValue<int32_t > tlsINT32;
typedef TlsValue<uint32_t> tlsUINT32;
typedef TlsValue<int16_t > tlsINT16;
typedef TlsValue<uint16_t> tlsUINT16;
typedef TlsValue<char16_t> tlsCHAR16;
typedef TlsValue<char    > tlsCHAR8;
typedef TlsValue<bool    > tlsBool;


/////////////////////////////////////////////////////////////////////////////

#endif // !__TlsValue_h__
