#ifndef _time_h_
#define _time_h_

/////////////////////////////////////////////////////////////////////////////
// ztime.h : Declaration and implementation of the Time class.
//

#include <cmath>
#include <cstdint>
#include <Windows.h>

#include "TlsValue.h"

/////////////////////////////////////////////////////////////////////////////
//
// Time
//
/////////////////////////////////////////////////////////////////////////////

class Time {
private:
    uint32_t m_dwTime;
    friend class DTime;

    static  tlsUINT32 s_dwPauseStart;
    static  tlsUINT32 s_dwNegativeOffset;

#ifdef _DEBUG_TRAINING
    static  tlsUINT32 s_dwLastTime;
    static  tlsUINT32 s_dwAccumulatedTime;
    static  tlsINT   s_iShift;
    static  tlsUINT32 s_dwLastClockTime;
#endif

public:
    static  float   fResolution (void) {return 1.0e3f;}

    static  void    Pause (void);
    static  bool    IsPaused (void);
    static  void    Continue (void);

#ifdef _DEBUG_TRAINING
    static  int     GetShift (void);
    static  void    SetShift (int iShift);
#endif

    static inline Time Now(void)
    {
      #ifdef _DEBUG_TRAINING
        // compute the amount of time elapsed since the last frame
        uint32_t   dwRealTime = timeGetTime ();
        assert (dwRealTime >= s_dwLastTime);
        uint32_t   dwDeltaTime = dwRealTime - s_dwLastTime;
        s_dwLastTime = dwRealTime;

        // compute a maximum allowable frame time
        uint32_t   dwMaxDeltaTime =  static_cast<uint32_t> (fResolution () * 0.25f); // 4 FPS
        dwDeltaTime = (dwDeltaTime > dwMaxDeltaTime) ? dwMaxDeltaTime : dwDeltaTime;

        // scale the elapsed time using the shift factor, and
        // accumulate it into the game clock
        uint32_t   dwShiftedTime = (s_iShift >= 0) ? (dwDeltaTime << s_iShift) : (dwDeltaTime >> -s_iShift);
        s_dwAccumulatedTime += dwShiftedTime;

        // compute the current time, accounting for whether or
        // not the clock is paused, and whether or not it has
        // been paused in the past.
        uint32_t   dwCurrentClockTime = ((s_dwPauseStart != 0) ? s_dwPauseStart : s_dwAccumulatedTime) - s_dwNegativeOffset;
        Time    now (dwCurrentClockTime);

        // check that time is strictly increasing
        if (s_dwLastClockTime != 0)
            assert (dwCurrentClockTime >= s_dwLastClockTime);
        s_dwLastClockTime = dwCurrentClockTime;
      #else
        uint32_t   dwCurrentClockTime = ((s_dwPauseStart != 0) ? s_dwPauseStart : timeGetTime()) - s_dwNegativeOffset;
        Time    now (dwCurrentClockTime);
      #endif

        return now;
    }

    inline Time(void)
    {
    }

    inline Time(uint32_t  dwTime) :
        m_dwTime(dwTime)
    {
    }

    inline Time(const Time& t) :
        m_dwTime(t.m_dwTime)
    {
    }

    inline Time Before(void) const
    {
        return Time(m_dwTime - 1);
    }

    inline Time After(void) const
    {
        return Time(m_dwTime + 0x7fffffff);
    }

    inline bool    operator >  (const Time     t) const
    {
        return ((int)(m_dwTime - t.m_dwTime) > 0);
    }

    inline bool    operator >= (const Time     t) const
    {
        return ((int)(m_dwTime - t.m_dwTime) >= 0);
    }

    inline bool    operator <  (const Time     t) const
    {
        return ((int)(m_dwTime - t.m_dwTime) < 0);
    }

    inline bool    operator <= (const Time     t) const
    {
        return ((int)(m_dwTime - t.m_dwTime) <= 0);
    }

    inline bool    operator == (const Time     t) const
    {
        return m_dwTime == t.m_dwTime;
    }

    inline bool    operator != (const Time     t) const
    {
        return m_dwTime != t.m_dwTime;
    }

    inline Time    operator =  (uint32_t  tick)
    {
        this->m_dwTime = tick;

        return *this;
    }

    inline Time    operator +  (float  d) const
    {
        Time   t(m_dwTime + (int)floor(d * fResolution () + 0.5));

        return t;
    }

    inline Time    operator += (float    d)
    {
        m_dwTime += (int)floor((d * fResolution ()) + 0.5);
        return *this;
    }

    inline Time    operator -  (float    d) const
    {
        Time   t(m_dwTime - (int)floor((d * fResolution ()) + 0.5));

        return t;
    }

    inline Time    operator -= (float    d)
    {
        m_dwTime -= (int)floor((d * fResolution ()) + 0.5);
        return *this;
    }

    inline float    operator -  (const Time     t) const
    {
        return ((float)((int)(m_dwTime - t.m_dwTime))) / fResolution ();
    }

    inline uint32_t   clock(void) const
    {
        return m_dwTime;
    }

    inline void     clock(uint32_t c)
    {
        m_dwTime = c;
    }
};

#endif
