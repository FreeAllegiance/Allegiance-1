//////////////////////////////////////////////////////////////////////////////
//
// Bit masks
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _mask_H_
#define _mask_H_

#include <cstdint>

//////////////////////////////////////////////////////////////////////////////
//
// This class is a type safe way of dealing with flag words see surface.h
// for example usage.
//
//////////////////////////////////////////////////////////////////////////////

template<class Type, class WordType>
class TBitMask 
{
private:
    WordType m_word;

public: //KGJV protected:
    TBitMask(WordType word) :
        m_word(word)
    {
    }

public:
    TBitMask() :
        m_word(0)
    {
    }

    TBitMask(const TBitMask& bm) :
        m_word(bm.m_word)
    {
    }

    void operator=(const TBitMask& bm)
    {
        m_word = bm.m_word;
    }

    friend TBitMask operator|(const TBitMask& bm1, const TBitMask& bm2)
    {
        return bm1.m_word | bm2.m_word;
    }

    friend bool operator==(const TBitMask& bm1, const TBitMask& bm2)
    {
        return bm1.m_word == bm2.m_word;
    }

    friend bool operator!=(const TBitMask& bm1, const TBitMask& bm2)
    {
        return bm1.m_word != bm2.m_word;
    }

    TBitMask& Set(const TBitMask& bm)
    {
        m_word |= bm.m_word;

        return *this;
    }

    TBitMask& Clear(const TBitMask& bm)
    {
        m_word &= ~bm.m_word;

        return *this;
    }

    TBitMask& Mask(const TBitMask& bm)
    {
        m_word &= bm.m_word;

        return *this;
    }

    bool Test(const TBitMask& bm) const
    {
        return (m_word & bm.m_word) != 0;
    }

    bool Test(const TBitMask& bm, const TBitMask& bmResult) const
    {
        return (m_word & bm.m_word) == bmResult.m_word;
    }

    //
    // Unsafe accessors needed for interface with Win32
    //

    void SetWord(WordType word)
    {
        m_word = word;
    }

    WordType GetWord() const
    {
        return m_word;
    }
};

class BitMask : public TBitMask<uint32_t, uint32_t> 
{
public:
    BitMask(uint32_t dw) : 
        TBitMask<uint32_t, uint32_t>(dw)
    {
    }

    BitMask() : 
        TBitMask<uint32_t, uint32_t>(0)
    {
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// This class implements a flag word with more than 32 bits.
//
//////////////////////////////////////////////////////////////////////////////

template<int nBits> class TLargeBitMask
{
    public:
        TLargeBitMask(void)
        {
            //ClearAll();
        }

        TLargeBitMask(const uint8_t*    bits)
        {
            Set(bits);
        }

        void    Set(const uint8_t* bits)
        {
            memcpy(&(m_bits[0]), bits, sizeof(m_bits));
        }

        void    ClearAll(void)
        {
            memset(&(m_bits[0]), 0, sizeof(m_bits));
        }

        void    SetAll(void)
        {
            memset(&(m_bits[0]), 0xff, sizeof(m_bits));
        }

        void    ClearBit(int    i)
        {
            int byte = i >> 3;
            int bit = i & 0x07;

            m_bits[byte] &= (0xff7f >> bit);    //pad the high byte with 1's
        }

        void    SetBit(int      i)
        {
            int byte = i >> 3;
            int bit = i & 0x07;

            m_bits[byte] |= (0x80 >> bit);
        }

        bool    GetBit(int      i) const
        {
            int byte = i >> 3;
            int bit = i & 0x07;

            return 0 != (m_bits[byte] & (0x80 >> bit));
        }

        bool    GetAllZero(void) const
        {
            for (int i = 0; (i < (nBits / 8)); i++)
            {
                if (m_bits[i] != 0)
                    return false;
            }

            return true;
        }
		// CHECK THIS, IT MIGHT OVERFLOW ! -KGJV
        void ToString(char* pszBytes, int cch) const
        {
          int cb = min(cch / 2, sizeof(m_bits));
          for (int i = 0; i < cb; ++i)
          {
            char szByte[3];
            sprintf(szByte, "%02X", m_bits[i]);
            CopyMemory(pszBytes + (i * 2), szByte, 2);
          }
        }

        bool FromString(const char* pszBits)
        {
          int cch = strlen(pszBits);
          if (cch % 2)
            return false;
          uint8_t bits[sizeof(m_bits)];
          ZeroMemory(bits, sizeof(bits));
          int cb = min(cch / 2, sizeof(m_bits));
          for (int i = 0; i < cb; ++i)
          {
            char szByte[3];
            CopyMemory(szByte, pszBits + (i * 2), 2);
            szByte[2] = '\0';
            long nBits = strtoul(szByte, NULL, 16);
            if ((0 == nBits || ULONG_MAX == nBits) && ERANGE == errno)
              return false;
            bits[i] = (uint8_t)nBits;
          }

          Set(bits);
          return true;
        }

        TLargeBitMask&   operator = (const TLargeBitMask& tbm)
        {
            memcpy(m_bits, tbm.m_bits, sizeof(m_bits));

            return *this;
        }

        TLargeBitMask&   operator |= (const TLargeBitMask& tbm)
        {
            for (int i = 0; (i < (nBits / 8)); i++)
                m_bits[i] |= tbm.m_bits[i];

            return *this;
        }

        TLargeBitMask    operator | (const TLargeBitMask& tbm) const
        {
            TLargeBitMask<nBits> r;
            for (int i = 0; (i < (nBits / 8)); i++)
                r.m_bits[i] = m_bits[i] | tbm.m_bits[i];

            return r;
        }

        TLargeBitMask&   operator &= (const TLargeBitMask& tbm)
        {
            for (int i = 0; (i < (nBits / 8)); i++)
                m_bits[i] &= tbm.m_bits[i];

            return *this;
        }

        TLargeBitMask    operator & (const TLargeBitMask& tbm) const
        {
            TLargeBitMask<nBits> r;
            for (int i = 0; (i < (nBits / 8)); i++)
                r.m_bits[i] = m_bits[i] & tbm.m_bits[i];

            return r;
        }

        bool        operator == (const TLargeBitMask&    tbm) const
        {
            bool    rc = true;
            for (int i = 0; (i < (nBits / 8)); i++)
            {
                if (m_bits[i] != tbm.m_bits[i])
                {
                    rc = false;
                    break;
                }
            }

            return rc;
        }

        bool        operator != (const TLargeBitMask&    tbm) const
        {
            bool    rc = true;
            for (int i = 0; (i < (nBits / 8)); i++)
            {
                if (m_bits[i] != tbm.m_bits[i])
                {
                    rc = false;
                    break;
                }
            }

            return !rc;
        }

        //Subset
        bool        operator <= (const TLargeBitMask& tbm) const
        {
            bool    rc = true;
            for (int i = 0; (i < (nBits / 8)); i++)
            {
                if ((m_bits[i] & tbm.m_bits[i]) != m_bits[i])
                {
                    rc = false;
                    break;
                }
            }

            return rc;
        }

        //Superset
        bool        operator >= (const TLargeBitMask& tbm) const
        {
            bool    rc = true;
            for (int i = 0; (i < (nBits / 8)); i++)
            {
                if ((m_bits[i] & tbm.m_bits[i]) != tbm.m_bits[i])
                {
                    rc = false;
                    break;
                }
            }

            return rc;
        }

        //"Difference": A - B == A & ~B
        //e.g. remove all the bits in B from A
        TLargeBitMask&   operator -= (const TLargeBitMask& tbm)
        {
            for (int i = 0; (i < (nBits / 8)); i++)
                m_bits[i] &= ~tbm.m_bits[i];

            return *this;
        }

        TLargeBitMask    operator - (const TLargeBitMask& tbm) const
        {
            TLargeBitMask<nBits> r;
            for (int i = 0; (i < (nBits / 8)); i++)
                r.m_bits[i] = m_bits[i] & ~tbm.m_bits[i];

            return r;
        }

    private:
        uint8_t    m_bits[((nBits - 1) / 8) + 1];

};

#endif
