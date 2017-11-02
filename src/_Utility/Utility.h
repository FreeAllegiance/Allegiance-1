/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	utility.h
**
**  Author: 
**
**  Description:
**      Header file for the utility library. Basically a set of commonly
**  used functions.
**
**  History:
*/
#ifndef utl_h
#define utl_h
/*
** Disable some stupid warning messages that we really don't need and
** shouldn't be there (actually, set them to level 4 so I can still see
** them if I want to).
*/
#define     _exposed

#include "CRC.h"

class   UTL;

class   List_utl;
class   Link_utl;

class   Lock_utl;

template<class T, class Sink = NullFunc> class Slist_utl;
template<class T, class Sink = NullFunc> class Slink_utl;

template<class T> class Mlist_utl;
template<class T> class Mlink_utl;

struct  FPOINT
{
    float   x;
    float   y;
};

class   Rotation;

// These constants correspond to the maximum length of a file name or a 
// player name. They should correspond to the maximum lengths in the SQL 
// database (for Name and FileName custom data types). Longer names are 
// handled via truncation. DB versions do not include the NULL terminator
const int c_cbFileNameDB    = 12; // keep in sync with 
const int c_cbDescriptionDB = 200;
const int c_cbNameDB        = 24;
const int c_cbLocAbrevDB    = 8;
const int c_cbFileName      = c_cbFileNameDB    + 1;
const int c_cbDescription   = c_cbDescriptionDB + 1;
const int c_cbName          = c_cbNameDB        + 1;
const int c_cbLocAbrev      = c_cbLocAbrevDB    + 1;
const int c_cbCDKey         = 1024 + 1; // BT - 9/11/2010 - widened to support longer auth tokens.
const int c_cbPassportName  = 256 + 1;

class _exposed List_utl
{
    public:
        /*
        ** Return the number of links in *this.
        */
        inline int    n(void) const
        {
            return m_n;
        }

    protected:
        List_utl(void);
        inline ~List_utl(void)
        {
        }

        /*
        ** Set the first link of *this to f.
        */
        void            first(Link_utl* f);
        /*
        ** Return the current first link in *this.
        */
        inline Link_utl*       first(void) const
        {
            return m_first;
        }
        /*
        ** Set the last link of *this to l.
        */
        void            last(Link_utl* l);

        /*
        ** Return the current last link in *this.
        */
        inline Link_utl*       last(void)  const
        {
            return m_last;
        }
        /*
        ** Return's the index'th element (index >= 0),
        ** or the list.n() + index'th element (index < 0).
        ** (note, list[-1] == list.last())
        */
        Link_utl*       operator [] (int index) const;

    private:
        Link_utl*       m_first;
        Link_utl*       m_last;

        int             m_n;

    friend class Link_utl;
};

class _exposed Link_utl
{
    public:
        /*
        ** Remove *this from whatever list it is attached to & return true,
        ** do nothing & return false if not attached.
        */
        bool    unlink(void);

    protected:
        Link_utl(void);
        ~Link_utl(void) 
        {
        }

        /*
        ** Return the pointer to whatever list *this is attached to.
        */
        inline List_utl*   list(void)  const
        {
            return m_list;
        }

        /*
        ** Insert l immediately after *this & return true.
        ** Do nothing & return false if *this is not part of a list.
        */
        bool        next(Link_utl*   l);

        /*
        ** Return the next element in the list.
        */
        inline Link_utl*   next(void)  const
        {
            return m_next;
        }

        /*
        ** Insert l immediately before *this & return true.
        ** Do nothing & return false if *this is not part of a list.
        */
        bool        txen(Link_utl*   l);

        /*
        ** Return the previous element in the list.
        */
        inline Link_utl*   txen(void)  const
        {
            return m_txen;
        }

    protected:
        List_utl*  m_list;
        Link_utl*  m_next;
        Link_utl*  m_txen;

    friend List_utl;
};

class _exposed Lock_utl
{
    public:
        Lock_utl(void);
        ~Lock_utl(void);

        /*
        ** Lock or unlock the list. Lock/unlock pairs can be nested.
        ** We are technically lying about the const attribute but that
        ** seems fair since neither lock nor unlock actually modify the
        ** contents of the thing that is locked.
        */
        void    lock(void)      const;
        void    unlock(void)    const;

    private:
        HANDLE                      m_lockedS;
        DWORD                       m_locking_threadID;
        int                         m_locks;
};

/*
** Templated list of T, single thread safe.
*/
template<class T, class Sink> class Slist_utl : public List_utl
{
    protected:
        Sink sink;

    public:

        Slist_utl() 
            {};

        Slist_utl(const Sink& _sink) 
            : sink(_sink) {}

        inline ~Slist_utl(void)
        {
            purge();
        }

        /*
        ** Clear the list.
        ** If deleteF is true, all links are deleted.
        ** If deleteF is false, they are simply unlinked.
        **
        ** Note: when a list's destructor is called, purge(true) is called.
        */
        void    purge(bool  deleteF = true);

        /*
        ** Create a new Slink which contains a copy of t and
        ** make it the first element of the list. Returns false
        ** iff unable to allocate a new link.
        */
        bool            first(const T&      t);
        bool            last(const T&       t);

        /*
        ** These are equivalent to the List_utl versions.
        */
        inline void            first(Slink_utl<T, Sink>* l)
        {
            List_utl::first(l);
            sink();
        }
        inline Slink_utl<T, Sink>*   first(void) const
        {
            return (Slink_utl<T, Sink>*)List_utl::first();
        }

        inline void            last(Slink_utl<T, Sink>*  l)
        {
            List_utl::last(l);
            sink();
        }
        inline Slink_utl<T, Sink>*   last(void) const
        {
            return (Slink_utl<T, Sink>*)List_utl::last();
        }

        inline Slink_utl<T, Sink>*    find(const T& e) const
        {
            Slink_utl<T, Sink>*  l;
            for (l = first(); ((l != NULL) && (l->data() != e)); l = l->next())
            {
                //Intentionally empty
            }

            return l;
        }

        inline Slink_utl<T, Sink>*   operator [](int index) const
        {
            return (Slink_utl<T, Sink>*)List_utl::operator [](index);
        }

    friend class Slink_utl<T, Sink>;
};

/*
** Templated list element of T, single thread safe. Each element
** contains a copy of T, not a pointer to T.
*/
template<class T, class Sink> class Slink_utl : public Link_utl
{
    public:
        /*
        ** Use the default constructor for the data member.
        */
        inline Slink_utl(void)
        {
        }

        /*
        ** Use the copy constructor from t for the data member.
        */
        inline Slink_utl(const T&  t)
        :
            m_data(t)
        {
        }

        inline ~Slink_utl(void)
        {
            if (list())
                unlink();
        }

        /*
        ** Return/set the data member of the link.
        */
        inline const T&        data(void)  const
        {
            return m_data;
        }
        inline T&              data(void)
        {
            return m_data;
        }
        inline void            data(const T&   t)
        {
            m_data = t;
            list()->sink();
        }

        /*
        ** Create a new Slink which contains a copy of t,
        ** insert it after or before *this, and return true.
        ** Do nothing and return false if *this is not attached to a list.
        */
        bool            next(const T&       t);
        bool            txen(const T&       t);

        /*
        ** These are equivalent to the List_utl versions.
        */
        inline Slist_utl<T, Sink>*   list(void)  const
        {
            return (Slist_utl<T, Sink>*)Link_utl::list();
        }

        inline bool            next(Slink_utl<T, Sink>*  l)
        {
            return Link_utl::next(l);
        }
        inline Slink_utl<T, Sink>*   next(void)  const
        {
            return (Slink_utl<T, Sink>*)Link_utl::next();
        }

        inline bool            txen(Slink_utl<T, Sink>*  l)
        {
            return Link_utl::txen(l);
        }
        inline Slink_utl<T, Sink>*   txen(void)  const
        {
            return (Slink_utl<T, Sink>*)Link_utl::txen();
        }

        __forceinline bool     unlink(void)
        {
            Slist_utl<T, Sink>* plist = list();
            bool result = Link_utl::unlink();
            plist->sink();
            return result;
        }

    private:
        T               m_data;
};

/*
** Templated list of T, multi-thread safe (if locked/unlocked properly).
*/
template<class T> class Mlist_utl : public List_utl, public Lock_utl
{
    public:
        inline Mlist_utl(void)
        {
        }
        inline ~Mlist_utl(void)
        {
            purge();
        }

        /*
        ** Clear the list.
        */
        void    purge(bool deleteF = true);    //locked/unlocked

        /*
        ** Create a new Slink which contains a copy of t and
        ** make it the first element of the list. Returns false
        ** if unable to allocate a new link.
        */
        bool            first(const T&      t);     //locked/unlocked
        bool            last(const T&       t);     //locked/unlocked

        /*
        ** These are equivalent to the List_utl versions.
        */
        void            first(Mlink_utl<T>* l);     //locked/unlocked
        Mlink_utl<T>*   first(void) const;
        void            last(Mlink_utl<T>*  l);     //locked/unlocked
        Mlink_utl<T>*   last(void) const;

        inline Mlink_utl<T>*    find(const T& e) const
        {
            lock();

            Mlink_utl<T>*  l;
            for (l = first(); ((l != NULL) && (l->data() != e)); l = l->next())
            {
                //Intentionally empty
            }

            unlock();

            return l;
        }

        Mlink_utl<T>*   operator [](int) const;

    friend class Mlink_utl<T>;
};

/*
** Templated list element of T, multi-thread safe. Each element
** contains a copy of T, not a pointer to T.
*/
template<class T> class Mlink_utl : public Link_utl
{
    public:
        /*
        ** Use the default constructor for the data member.
        */
        inline Mlink_utl(void)
        {
        }

        /*
        ** Use the copy constructor from t for the data member.
        */
        inline Mlink_utl(const T&  t)
        :
            m_data(t)
        {
        }

        inline ~Mlink_utl(void)
        {
            if (list())
                unlink();
        }

        /*
        ** Return the data member of the link.
        */
        inline const T&        data(void)  const
        {
            return m_data;
        }
        inline T&              data(void)
        {
            return m_data;
        }
        inline void            data(const T&   t)
        {
            m_data = t;
        }

        /*
        ** Create a new Mlink which contains a copy of t,
        ** insert it after or before *this, and return true.
        ** Do nothing and return false if *this is not attached to a list.
        */
        bool            next(const T&       t);
        bool            txen(const T&       t);

        /*
        ** These are equivalent to the List_utl versions.
        */
        inline Mlist_utl<T>*   list(void)  const
        {
            return (Mlist_utl<T>*)Link_utl::list();
        }

        inline bool            next(Mlink_utl<T>*  l)
        {
            return Link_utl::next(l);
        }
        inline Mlink_utl<T>*   next(void)  const
        {
            return (Mlink_utl<T>*)Link_utl::next();
        }

        inline bool            txen(Mlink_utl<T>*  l)
        {
            return Link_utl::txen(l);
        }
        inline Mlink_utl<T>*   txen(void)  const
        {
            return (Mlink_utl<T>*)Link_utl::txen();
        }

    private:
        T               m_data;
};

class Rotation
{
    public:
        Rotation(void);
        Rotation(float x, float y, float z, float a)
        :
            m_axis(x, y, z),
            m_angle(a)
        {
        }
        Rotation(const Vector& xyz, float a)
        :
            m_axis(xyz),
            m_angle(a)
        {
        }
        Rotation(const Rotation&);

        void    reset(void);

        void        set(const Vector& axis,
                        float         angle);

        Rotation&   operator = (const Rotation&);

        inline float    x(void) const
        {
            return m_axis.x;
        }

        inline float    y(void) const
        {
            return m_axis.y;
        }

        inline float    z(void) const
        {
            return m_axis.z;
        }

        inline float    angle(void) const
        {
            return m_angle;
        }

        inline const Vector&   axis(void) const
        {
            return m_axis;
        }

        void        axis(const Vector&  v);
        void        angle(float  r);
        void        x(float  t);
        void        y(float  t);
        void        z(float  t);

    private:
        Vector  m_axis;
        float   m_angle;
};

const int   c_cbFrameName = 20;
class   FrameDataUTL
{
    public:
        char    szName[c_cbFrameName];
        Vector  position;
        Vector  forward;

    friend bool operator!=(const FrameDataUTL& fd1, const FrameDataUTL& fd2)
    {
        return false;
    }
};

typedef Slist_utl<FrameDataUTL>    FrameList;
typedef Slink_utl<FrameDataUTL>    FrameLink;

const int   c_nAxes = 3;
const int   c_maxHitTests = 100;
const int   c_minNodeSize = 5;
const int   c_minRootSize = 10;

class   Interval;
class   BoundingBox;
class   Endpoint;
class   HitTest;
class   KDnode;
class   KDroot;
class   CollisionEntry;
class   CollisionQueue;

class   Interval
{
    public:
        float       values[2];      //lo & hi endpoints.
};

class   BoundingBox
{
    public:
        Interval    axes[c_nAxes];
};

class   Endpoint
{
    public:
        float       value;
        HitTest*    pHitTest;
        bool        highF;

        //Bubble-sort based algorith: use when the list is mostly sorted
        static void shortSort(Endpoint** lo,
                              Endpoint** hi);

        //Quick-sort based algorithm: the alternative to shortSort
        static void longSort(Endpoint** lo,
                             Endpoint** hi);
};

class   Transform44
{
    public:
        inline Transform44(void)
        :
            m_position(Vector::GetZero()),
            m_orientation(Orientation::GetIdentity()),
            m_modifiedF(true)
        {
        }

        inline const Vector&   GetPosition(void) const
        {
            return m_position;
        }

        inline void    SetPosition(const Vector& xyz)
        {
            assert(xyz.Length() < 100000);
            m_position = xyz;
            m_modifiedF = true;
        }

        inline const Orientation&  GetOrientation(void) const
        {
            return m_orientation;
        }

        inline void            SetOrientation(const Orientation& o)
        {
            m_orientation = o;
            m_modifiedF = true;
        }

        inline const   Matrix& GetMatrix(void)
        {
            if (m_modifiedF)
            {
                m_modifiedF = false;
                //m_matrix.SetTranslate(m_position);
                //m_matrix.SetOrientation(m_orientation);
            }

            return m_matrix;
        }

    private:
        Matrix          m_matrix;
        Vector          m_position;
        Orientation     m_orientation;
        bool            m_modifiedF;
};

typedef char   HitTestShape;

//These do not depend on orientation
const HitTestShape  c_htsPoint      = -4;
const HitTestShape  c_htsSphere     = -3;

//And these do, but there is a "closed" form solution for whether a point is inside
const HitTestShape  c_htsEllipse    = -2;
const HitTestShape  c_htsCone       = -1;

//but not for this ... the hts is actually the number of distinct
//convex hulls within the hit test
const HitTestShape  c_htsConvexHullMin = 0;
const HitTestShape  c_htsConvexHullMax = 127;

typedef void*   HitTestID;
HitTestID const c_htidStaticObject = (HitTestID)(-1);

class   MultiHullBase
{
    public:
        MultiHullBase(const Vector&    ellipseEquation,
                      float            originalRadius)
        :
            m_ellipseEquation(ellipseEquation),
            m_originalRadius(originalRadius)
        {
        }

        const Vector&  GetFrameOffset(const char*  pszFrameName) const
        {
            const FrameDataUTL*    pfd = GetFrame(pszFrameName);
            return pfd ? pfd->position : Vector::GetZero();
        }

        const Vector&  GetFrameForward(const char* pszFrameName) const
        {
            static const Vector z(0.0f, 0.0f, 1.0f);
            const FrameDataUTL*    pfd = GetFrame(pszFrameName);

            return pfd ? pfd->forward : z;
        }

        const FrameDataUTL*    GetFrame(const char* pszFrameName) const
        {
            for (FrameLink* pfl = m_frames.first();
                 (pfl != NULL);
                 pfl = pfl->next())
            {
                const FrameDataUTL&    fd = pfl->data();
                if (strcmp(pszFrameName, fd.szName) == 0)
                {
                    return &fd;
                }
            }

            return NULL;
        }

        const float GetOriginalRadius(void) const
        {
            return m_originalRadius;
        }

        const Vector& GetEllipseEquation(void) const
        {
            return m_ellipseEquation;
        }

    protected:
        FrameList           m_frames;
        float               m_originalRadius;
        Vector              m_ellipseEquation;
};

class   HitTest : public Transform44
{
    public:
        static MultiHullBase*   Load(const char*    pszFileName);

        static HitTest*         Create(const char*    pszFileName,
                                       IObject*       data,
                                       bool           staticF,
                                       HitTestShape   htsDefault = c_htsSphere);

        HitTest(IObject*        data,
                float           radius,
                bool            staticF,
                HitTestShape    shape)
        :
            m_cRefs(1),
            m_id(NULL),
            m_data(data),
            m_radius(radius),
            m_shape(shape),
            m_shapeTrue(shape),
            m_noHit(NULL),
            m_pvUseTrueShapeSelf(NULL),
            m_pvUseTrueShapeOther(NULL),
            m_deadF(true),
            m_deletedF(false),
            m_staticF(staticF),
            m_velocity(Vector::GetZero()),
            m_timeStart(0),
            m_timeStop(0),
            m_pkdrRoot(NULL)
        {
            assert (data);
            data->AddRef();

            m_endpoints[0][0].highF = 
                m_endpoints[1][0].highF = 
                m_endpoints[2][0].highF = false;

            m_endpoints[0][1].highF = 
                m_endpoints[1][1].highF = 
                m_endpoints[2][1].highF = true;

            m_endpoints[0][0].pHitTest = m_endpoints[0][1].pHitTest =
                m_endpoints[1][0].pHitTest = m_endpoints[1][1].pHitTest =
                m_endpoints[2][0].pHitTest = m_endpoints[2][1].pHitTest = this;

            //No addref on m_data ... this hit test is part of m_data
        }

        ~HitTest(void)
        {
            assert (m_data);
            m_data->Release();
        }

        ULONG __stdcall AddRef(void)
        {
            return ++m_cRefs;
        }

        ULONG __stdcall Release(void)
        {
            ULONG   cRefs = --m_cRefs;

            if (cRefs == 0)
                delete this;

            return cRefs;
        }

        void    SetStaticF(bool staticF)
        {
            m_staticF = staticF;
        }

        void    SetBB(Time start, Time stop, float dt);
        void    SetStopPosition(void)
        {
            m_stopPosition = GetPosition();
            if (m_tStop > 0.0f)
                m_stopPosition += m_tStop * m_velocity;

            assert (m_stopPosition.LengthSquared() >= 0.0f);
            assert (m_stopPosition.LengthSquared() < 1.0e12f);
        }
        void                    Move(float t)
        {
            if (!m_staticF)
            {
                SetPosition(GetPosition() + m_velocity * t);

                if (m_tStart <= t)
                    m_tStart = 0.0f;
                else
                    m_tStart -= t;

                m_tStop -= t;

                assert (GetPosition() * GetPosition() >= 0.0f);
            }
        }

        void                    AdjustTimes(float t)
        {
            if (!m_staticF)
            {
                if (m_tStart <= t)
                    m_tStart = 0.0f;
                else
                    m_tStart -= t;

                m_tStop -= t;
            }
        }
        void                    Move(void)
        {
            if (!m_staticF)
                SetPosition(m_stopPosition);
        }

        virtual void    UpdateStaticBB(void)
        {
            UpdateStaticBB(m_radius);
        }

        void    UpdateStaticBB(float r)
        {
            assert (m_staticF);

            //let's assume were using the xy & z axes.
            assert (c_nAxes == 3);
            for (int i = 0; (i < c_nAxes); i++)
            {
                float   v = GetPosition()[i];
 
                m_boundingBox.axes[i].values[0] = v - r;          
                m_boundingBox.axes[i].values[1] = v + r;          

                m_endpoints[i][0].value = m_boundingBox.axes[i].values[0];
                m_endpoints[i][1].value = m_boundingBox.axes[i].values[1];
            }
        }


        void    UpdateBB(void);

        void    Collide(HitTest*            pHitTest,
                        CollisionQueue*     pQueue);

        HitTestShape    GetShape(void) const
        {
            return m_shape;
        }
        HitTestShape    GetTrueShape(void) const
        {
            return m_shapeTrue;
        }

        virtual void            SetShape(HitTestShape   shape)
        {
            //Never upgrade to anything more complex than our true shape
            if (shape <= m_shapeTrue)
                m_shape = shape;
        }

        virtual const Vector*   GetEllipseEquation(void) const
        {
            return NULL;
        }

        float           GetRadius(void) const
        {
            return m_radius;
        }

        virtual void    SetRadius(float r)
        {
            m_radius = r;
        }

        virtual float    GetScale(void) const
        {
            return 1.0f;
        }

        bool            GetDeadF(void) const
        {
            return m_deadF;
        }
        void            SetDeadF(bool   d)
        {
            m_deadF = d;
        }

        bool            GetDeletedF(void) const
        {
            return m_deletedF;
        }
        void            SetDeletedF(bool   d)
        {
            m_deletedF = d;
        }

        IObject*        GetData(void) const
        {
            return m_data;
        }

        HitTest*        GetLastTest(void) const
        {
            return m_lastTest;
        }

        void            SetLastTest(HitTest*    ht)
        {
            m_lastTest = ht;
        }

        const Vector&   GetVelocity(void) const
        {
            return m_velocity;
        }
        void            SetVelocity(const Vector& v)
        {
			// mmf replaced asserts with log msg, rewrite to zero
			if (!(v.LengthSquared() < (100000.0f * 100000.0f))) {
				debugf("mmf Utility.h SetVelocity: v.LengthSquared debug build would have called assert and exited, commented out and set to zero for now\n");   
				m_velocity.x = 0.0f; m_velocity.y = 0.0f; m_velocity.z = 0.0f;
			} else {
			    m_velocity = v;
			}

		    if (!(v * v >= 0.0f)) {
				debugf("mmf Utility.h SetVelocity: v^2 debug build would have called assert and exited, commented out and set to zero for now\n");   
			    m_velocity.x = 0.0f; m_velocity.y = 0.0f; m_velocity.z = 0.0f;
			} else {
			    m_velocity = v;
			}
            
			// mmf orig code
			// assert (v * v >= 0.0f);
            // assert (v.LengthSquared() < (100000.0f * 100000.0f));
            // m_velocity = v;
			// mmf end orig code
        }

        HitTestID       GetID(void) const
        {
            return m_id;
        }
        void            SetID(HitTestID id)
        {
            m_id = id;
        }

        HitTest*        GetNoHit(void) const
        {
            return m_noHit;
        }

        void            SetUseTrueShapeSelf(void* pvUseTrueShapeSelf)
        {
            m_pvUseTrueShapeSelf = pvUseTrueShapeSelf;
        }

        void            SetUseTrueShapeOther(void* pvUseTrueShapeOther)
        {
            m_pvUseTrueShapeOther = pvUseTrueShapeOther;
        }

        void            SetNoHit(HitTest*   ht)
        {
            m_noHit = ht;
        }

        void            SetTimeStart(Time   t)
        {
            m_timeStart = t;
        }

        void            SetTimeStop(Time t)
        {
            m_timeStop = t;
        }

        static bool     HullCollide(float*        tCollision,
                                    float         tMax,
                                    HitTest*      phtHullA,
                                    HitTestShape* phtsA,
                                    HitTest*      phtHullB,
                                    HitTestShape* phtsB,
                                    const Vector& dP,
                                    const Vector& dV);  //Sense for velocity is reversed from position
        static bool     IntervalCollide(float               tStart,
                                        float               tStop,
                                        float               maxDeltaT,
                                        HitTest*            phtHullA,
                                        HitTestShape        htsA,
                                        HitTest*            phtHullB,
                                        HitTestShape        htsB,
                                        const Vector&       dP,
                                        const Vector&       dV,               //Sense for velocity is reversed from position
                                        const Orientation&  orientationA,
                                        float*              tCollision);

        Vector    GetMinExtreme(HitTestShape        hts,
                                const Vector&       position,
                                const Orientation&  orientation,
                                const Vector&       direction);

                                //position is assumed to be 0, 0, 0; orientation is the identity
        Vector    GetMaxExtreme(HitTestShape        hts,
                                const Vector&       direction);

        virtual Vector    GetCenter(HitTestShape  hts) const
        {
            assert (false); //Should never be called for anything except convex hulls
            return Vector::GetZero();
        }

        void        SetKDRoot(KDroot*   pkdr)
        {
            m_pkdrRoot = pkdr;
        }

        KDroot*     GetKDRoot(void) const
        {
            return m_pkdrRoot;
        }

        virtual const Vector   GetFrameOffset(const char*  pszFrameName) const
        {
            return Vector::GetZero();
        }
        virtual const Vector&  GetFrameForward(const char* pszFrameName) const
        {
            static const Vector z(0.0f, 0.0f, 1.0f);

            return z;
        }
        virtual const FrameDataUTL* GetFrame(const char* pszFrameName) const
        {
            return NULL;
        }

        float       GetTstop(void) const
        {
            return m_tStop;
        }

    protected:
        ULONG           m_cRefs;
        IObject*        m_data;

        void*           m_pvUseTrueShapeSelf;     //if m_useTrueShapeSelf != NULL and m_useTrueShapeSelf == pht->m_useTrueShapeOther
        void*           m_pvUseTrueShapeOther;    //then always use the true shape for purposes of collision detection & not the given shape 

        KDroot*         m_pkdrRoot;
        HitTest*        m_noHit;
        HitTestID       m_id;
        BoundingBox     m_boundingBox;
        Vector          m_stopPosition;
        Vector          m_velocity;
        float           m_radius;
        float           m_tStart;
        float           m_tStop;
        Time            m_timeStart;
        Time            m_timeStop;

        Endpoint        m_endpoints[c_nAxes][2];
        HitTest*        m_lastTest;

        HitTestShape    m_shape;
        HitTestShape    m_shapeTrue;

        bool            m_deletedF;
        bool            m_deadF;
        bool            m_activeF;
        bool            m_staticF;
              
    friend class KDnode;
    friend class KDroot;
};
typedef Slist_utl<HitTest*>  HitTestList;
typedef Slink_utl<HitTest*>  HitTestLink;

class   KDnode
{
    public:
        KDnode(KDnode*          parent);

        ~KDnode(void);

        void    reset(bool          highF);
        void    pivot(void);

        void    test(HitTest*           pHitTest,
                     CollisionQueue*    pQueue) const;

    protected:
        int         m_pivotAxis;
        float       m_pivotValue;

        KDnode*     m_parent;
        KDnode*     m_children[2];

        void        allocHitTestsEndpoints(int   n);

        int         m_nHitTests;
        int         m_maxHitTests;
        HitTest**   m_ppHitTests;
        Endpoint**  m_ppEndpoints[c_nAxes];
};

class   KDroot : public KDnode
{
    public:
        KDroot(bool bStatic);
        ~KDroot(void);

        void    addHitTest(HitTest*           pHitTest);
        void    deleteHitTest(HitTest*        pHitTest);

        void    flush(void);
        void    update(void);

    private:
        int     m_nAdded;
        int     m_nDeleted;
        bool    m_bStatic;
};

class   CollisionEntry
{
    public:
        float           m_tCollision;
        HitTest*        m_pHitTest1;
        HitTest*        m_pHitTest2;
        HitTestShape    m_hts1;
        HitTestShape    m_hts2;

        //Bubble-sort based algorith: use when the list is mostly sorted
        static void shortSort(CollisionEntry* lo,
                              CollisionEntry* hi);

        //Quick-sort based algorithm: the alternative to shortSort
        static void longSort(CollisionEntry* lo,
                             CollisionEntry* hi);
};

class   CollisionQueue
{
    public:
        CollisionQueue(void);
        CollisionQueue(int              m_maxCollisions,
                       CollisionEntry*  pCollisions);
        ~CollisionQueue(void);

        inline int     n(void) const
        {
            return m_nCollisions;
        }

        inline CollisionEntry& operator [] (int i)
        {
            return m_pCollisions[i];
        }

        void    sort(int                    start);
        void    flush(int                   start,
                      HitTest*              pHitTest1,
                      HitTest*              pHitTest2);
        void    purge(void);
        void    addCollision(float          tCollision,
                             HitTest*       pHitTest1,
                             HitTestShape   hts1,
                             HitTest*       pHitTest2,
                             HitTestShape   hts2);

    private:
        int             m_nCollisions;
        CollisionEntry* m_pCollisions;
        int             m_maxCollisions;
        bool            m_fDelete;
};

class UTL
{
    public:
		//Imago 7/10 #62
		static void SetServerVersion(const char * szVersion, DWORD dwCookie);
		static ZString GetServerVersion(DWORD dwCookie);
		//Imago 6/10 #2
		static void SetPrivilegedUsers(const char * szPrivilegedUsers, DWORD dwCookie);
		static ZString GetPrivilegedUsers(DWORD dwCookie);
		static bool PrivilegedUser(const char* szName, DWORD dwCookie); 

        //Get an artwork file, downloading if needed
        //  S_OK    ... file exists and has a non-zero length
        //  S_FALSE ... file exists, but has a length of zero
        //  E_FAIL  ... file does not exist.
        static void SetArtPath(const char* szArtwork);
        static HRESULT getFile(    const char*    name,
                                   const char*    extension,
                               OUT char*          artwork,
                                   bool           downloadF,
                                   bool           createF);

        //  Writes a blob to disk; returns true iff successful
        static bool SaveFile(      const char * szFilename, 
                                   const void *pData, 
                                   unsigned cLen, 
                             OUT   char * szErrorMsg, 
                                   bool bCreateAsTemp);

        static bool AppendFile(const char * szFileName, const void * data, unsigned bytes);

        static int SearchAndReplace(char * szDest, const char * szSource, const char * szNewWord, const char * szOldWord);

        static void SetUrlRoot(const char * szUrlRoot);

        //Like the stock strdup() except uses new [] and handles NULL
        static char*                strdup(const char*  s);

        //set allocated on demand strings, file names or names
        static void  putFileName(char*       name, const char*   newVal);
        static void  putName(char*           name, const char*   newVal);

        static const char*  artworkPath(void)
        {
            return s_artworkPath;
        }
        static LONG GetPathFromReg(IN  HKEY hkey,
                                   IN  const char * szSubKey, 
                                   OUT char * szPath);

        // converts char * of hex to int.  Assumes uppercase for 'A' to 'F'.
        static int hextoi(const char * pHex);

    private:
        static char     s_artworkPath[MAX_PATH];
        static char     s_szUrlRoot[MAX_PATH];
		static TMap<DWORD,ZString> m_PrivilegedUsersMap; //Imago 6/10
		static TMap<DWORD,ZString> m_ServerVersionMap; //Imago 7/10
};

class   BytePercentage
{
    public:
        enum
        {
            Divisor = 240   //Use 240 since it has so many nice factors (2, 3, 4, 5, 10, etc.) 
        };

        BytePercentage(void)
        {
        }

        BytePercentage(const BytePercentage& p)
        :
            m_percentage(p.m_percentage)
        {
        }

        BytePercentage(float f)
        :
            m_percentage((unsigned char)(f * (float)Divisor + 0.5f))
        {
            assert ((f * (float)Divisor + 0.5f) >= 0.0f);
            assert ((f * (float)Divisor + 0.5f) < (float)(Divisor + 1));
        }

        unsigned char   GetChar(void) const
        {
            return m_percentage;
        }
        void            SetChar(unsigned char p)
        {
            m_percentage = p;
        }

        BytePercentage& operator = (float f)
        {
			/*assert((f * (float)Divisor + 0.5f) >= 0.0f);
			assert((f * (float)Divisor + 0.5f) < (float)(Divisor + 1));*/

			// Fix for crash when a user drops a probe right when entering an aleph. 
			if ((f * (float)Divisor + 0.5f) < 0.0f)
				m_percentage = 0;

			else if ((f * (float)Divisor + 0.5f) >= (float)(Divisor + 1))
				m_percentage = Divisor;

			else
				m_percentage = (unsigned char)(f * (float)Divisor + 0.5f);

            return *this;
        }

        operator float (void) const
        {
            return ((float)m_percentage) / ((float)Divisor);
        }

    private:
        unsigned char   m_percentage;
};

extern const Rotation      c_rotationZero;

#include    "utility.hxx"
#undef  _exposed

// safe strcpy
static char * Strcpy(char * szDst, const char * szSrc)
{
  assert(szDst);
  return lstrcpyA(szDst, szSrc ? szSrc : "");
}
        
// safe strncpy
static char * Strncpy(char * szDst, const char * szSrc, size_t cb)
{
  assert(szDst);
  return strncpy(szDst, szSrc ? szSrc : "", cb);
}
        
// safe strcmp
static int Strcmp(const char * szDst, const char * szSrc)
{
  return lstrcmpA(szDst ? szDst : "", szSrc ? szSrc : "");
}

// safe strcat //Imago 7/15/09
static char * Strcat(const char * szDst, const char * szSrc)
{
  return lstrcatA(szDst ? szDst : "", szSrc ? szSrc : "");
}

#define IMPLIES(x, y) (!(x) || (y))
#define IFF(x, y) (!!(x) == !!(y))

#include "listwrappers.h"

// Class for temporary dll function use
template <class Func> // signature of function to get, so that Proc can be used without casting
class CTempFuncDll
{
public:
  CTempFuncDll(LPSTR szDLL, LPSTR szFunc)
  {
    hmod = LoadLibrary(szDLL);
    pfunc = (Func) ((hmod) ? GetProcAddress(hmod, szFunc) : 0);
  }
  ~CTempFuncDll()
  {
    FreeLibrary(hmod);
  }
  Func Call()
  {
    assert(pfunc);
    assert(!IsBadCodePtr((FARPROC)pfunc));
    return pfunc;
  }

private:
  HMODULE hmod;
  Func pfunc;
};


class CTempTimer_OptimizedOut // timers do nothing in optimized builds
{
public:
  CTempTimer_OptimizedOut(const char * szName, float dtWarning) {}
  void Start() {}
  bool Stop(const char* format = NULL, ...) {return false;}
  DWORD LastInterval() {return 0;}
};


class CTimer
{
public:
  CTimer(const char * szName, float dtWarning) : 
    m_szname(szName),
    m_time(0),
    m_dtWarning((DWORD)(1000.0f * dtWarning)),
    m_dtimemax(0),
    m_dtTotal(0)
  {
  }

  void Start()
  {
    m_time = Time::Now();
  }

  bool Stop(const char* format = NULL, ...) // returns whether this previous interval was a new record
  {
    m_dtimelast = Time::Now().clock() - m_time.clock();
    m_dtTotal += m_dtimelast;
    bool fExtra = false;
    bool fRecord = false;
    if (m_dtimelast > m_dtimemax)
    {
      debugf("Timer: %fs spent %s, %fs total (longest so far)\n", (float) m_dtimelast / 1000.f, m_szname, (float)m_dtTotal / 1000.f);
      m_dtimemax = m_dtimelast;
      fRecord = true;
      fExtra = true;
    }
    else if (m_dtimelast >= m_dtWarning)
    {
      debugf("Timer: %fs spent %s, %fs total\n", (float) m_dtimelast / 1000.0f, m_szname, (float)m_dtTotal / 1000.f);
      fExtra = true;
    }

    if (fExtra && format)
    {
      const size_t size = 256;
      char         bfr[size];
      va_list vl;
      va_start(vl, format);
      _vsnprintf(bfr, size, format, vl);
      va_end(vl);
      debugf("^--%s\n", bfr);
    }
    return fRecord;
  }

  DWORD LastInterval() // in seconds
  {
    return m_dtimelast;
  }

private:
  const char * m_szname;
  Time   m_time;
  DWORD  m_dtimelast;
  DWORD  m_dtimemax;
  DWORD  m_dtWarning;
  DWORD  m_dtTotal;
};

#ifdef NDEBUG
#define CTempTimer CTempTimer_OptimizedOut
#else
#define CTempTimer CTimer
#endif

#endif
