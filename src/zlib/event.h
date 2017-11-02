#ifndef _message_H_
#define _message_H_

#include "tlist.h"
#include "tref.h"
#include "zstring.h"

/////////////////////////////////////////////////////////////////////////////
//
// Event
//
/////////////////////////////////////////////////////////////////////////////

class IEventSink;
class IEventSource : public IObject {
public:
    static IEventSource* GetNever();

    virtual void AddSink(IEventSink* psink)    = 0;
    virtual void RemoveSink(IEventSink* psink) = 0;
};

class IEventSink : public IObject {
public:
    static TRef<IEventSink> CreateDelegate(IEventSink* psink);

    virtual bool OnEvent(IEventSource* pevent) = 0;
};

/////////////////////////////////////////////////////////////////////////////
//
// Template Pair
//
/////////////////////////////////////////////////////////////////////////////

template<class FirstType, class SecondType>
class TPair {
private:
    FirstType  m_first;
    SecondType m_second;

public:
    TPair(const FirstType& first, const SecondType& second) :
        m_first(first),
        m_second(second)
    {
    }

    const FirstType&  GetFirst()  const { return m_first;  }
    const SecondType& GetSecond() const { return m_second; }
};

/////////////////////////////////////////////////////////////////////////////
//
// Template Events
//
/////////////////////////////////////////////////////////////////////////////

template<class Type>
class TEvent {
public:
    class Sink;
    class SourceImpl;
    class Source : public IObject {
    public:
        static TRef<SourceImpl> Create()
        {
            return new SourceImpl();
        }

        virtual void    AddSink(Sink* psink) = 0;
        virtual void RemoveSink(Sink* psink) = 0;
    };

    class Sink : public IObject {
    public:
        static TRef<Sink> CreateDelegate(Sink* psink)
        {
            return new Delegate(psink);
        }

        virtual bool OnEvent(Source* pevent, Type value) = 0;
    };

    class SourceImpl : public Source {
    private:
        TList<TRef<Sink> > m_listSinks;

    public:
        void AddSink(Sink* psink)
        {
            m_listSinks.PushFront(psink);
        }

        void RemoveSink(Sink* psink)
        {
            m_listSinks.Remove(psink);
        }

        void Trigger(Type value)
        {
            TList<TRef<Sink> >::Iterator iter(m_listSinks);

            while (!iter.End()) {
                if (!iter.Value()->OnEvent(this, value)) {
                    iter.Remove();
                } else {
                    iter.Next();
                }
            }
        }
    };

// KGJV // private:
    class Delegate : public Sink {
    private:
        Sink* m_psink;

    public:
        Delegate(Sink* psink) :
            m_psink(psink)
        {
        }

        bool OnEvent(Source* pevent, Type value)
        {
            return m_psink->OnEvent(pevent, value);
        }
    };
};

/////////////////////////////////////////////////////////////////////////////
//
// StringEvents
//
/////////////////////////////////////////////////////////////////////////////

typedef TEvent<ZString>         StringEvent;
typedef StringEvent::Source     IStringEventSource;
typedef StringEvent::Sink       IStringEventSink;
typedef StringEvent::SourceImpl StringEventSourceImpl;

/////////////////////////////////////////////////////////////////////////////
//
// Event Implementation
//
/////////////////////////////////////////////////////////////////////////////

class EventSourceImpl : public IEventSource {
private:
    TList<TRef<IEventSink> > m_listSinks;

public:
    void AddSink(IEventSink* psink);
    void RemoveSink(IEventSink* psink);
    void Trigger();
};

/////////////////////////////////////////////////////////////////////////////
//
// Integer Event
//
/////////////////////////////////////////////////////////////////////////////

/*
typedef TEvent<int>::Source     IIntegerEventSource;
typedef TEvent<int>::Sink       IIntegerEventSink;
typedef TEvent<int>::SourceImpl IntegerEventSourceImpl;
*/

class IIntegerEventSink;
class IIntegerEventSource : public IEventSource {
public:
    virtual void AddSink(IIntegerEventSink* psink)    = 0;
    virtual void RemoveSink(IIntegerEventSink* psink) = 0;
};

class IIntegerEventSink : public IObject {
public:
    static TRef<IIntegerEventSink> CreateDelegate(IIntegerEventSink* psink);

    virtual bool OnEvent(IIntegerEventSource* pevent, int value) = 0;
};

/////////////////////////////////////////////////////////////////////////////
//
// Integer Event Implementation
//
/////////////////////////////////////////////////////////////////////////////

class IntegerEventSourceImpl : public IIntegerEventSource {
private:
    TList<TRef<IEventSink> >        m_listSinks;
    TList<TRef<IIntegerEventSink> > m_listIntegerSinks;

public:
    void AddSink(IEventSink* psink);
    void RemoveSink(IEventSink* psink);

    void AddSink(IIntegerEventSink* psink);
    void RemoveSink(IIntegerEventSink* psink);
    void Trigger(int value);
};

/////////////////////////////////////////////////////////////////////////////
//
// Timer Event
//
/////////////////////////////////////////////////////////////////////////////

class ITimerEventSource : public IEventSource {
public:
    virtual void AddSink(IEventSink* psink, float when) = 0;
    virtual void RemoveSink(IEventSink* psink)          = 0;
};

/////////////////////////////////////////////////////////////////////////////
//
// EventTargetContainer
//
/////////////////////////////////////////////////////////////////////////////

class IAllegEventTarget : public IObject
{
public:
    virtual void Disconnect() = 0;
};

template<class ObjectClass>
class TIntegerEventTarget : public IIntegerEventSink, public IAllegEventTarget
{
private:

    typedef bool (ObjectClass::*PFNOnEventMember)(int value);

    ObjectClass* m_pobject;
    PFNOnEventMember m_pfn;
    TRef<IIntegerEventSource> m_psource;

public:
    TIntegerEventTarget(ObjectClass* pobject, 
            PFNOnEventMember pfn,
            IIntegerEventSource* psource) :
        m_pobject(pobject),
        m_pfn(pfn),
        m_psource(psource)
    {
        m_psource->AddSink(this);
    }

    void Disconnect()
    {
        m_psource->RemoveSink(this);
    }

    bool OnEvent(IIntegerEventSource* pevent, int value)
    {
        return (m_pobject->*m_pfn)(value);
    }
};

template<class ObjectClass>
class TEventTarget : public IEventSink, public IAllegEventTarget
{
private:

    typedef bool (ObjectClass::*PFNOnEventVoidMember)(void);

    ObjectClass* m_pobject;
    PFNOnEventVoidMember m_pfn;
    TRef<IEventSource> m_psource;

public:
    TEventTarget(ObjectClass* pobject, 
            PFNOnEventVoidMember pfn,
            IEventSource* psource) :
        m_pobject(pobject),
        m_pfn(pfn),
        m_psource(psource)
    {
        m_psource->AddSink(this);
    }

    TEventTarget(ObjectClass* pobject, 
            PFNOnEventVoidMember pfn,
            ITimerEventSource* psource,
            float when) :
        m_pobject(pobject),
        m_pfn(pfn),
        m_psource(psource)
    {
        psource->AddSink(this, when);
    }

    void Disconnect()
    {
        m_psource->RemoveSink(this);
    }

    bool OnEvent(IEventSource* pevent)
    {
        return (m_pobject->*m_pfn)();
    }
};


template<class DerivedClass>
class EventTargetContainer : public IObject 
{
private:
    typedef bool (DerivedClass::*PFNOnEventMember)(int value);
    typedef TIntegerEventTarget<DerivedClass> IntegerEventTarget;

    typedef bool (DerivedClass::*PFNOnEventVoidMember)(void);
    typedef TEventTarget<DerivedClass> EventTarget;
    
    TList<TRef<IAllegEventTarget> > m_lstTargets;

public:
    virtual ~EventTargetContainer()
    {
        TList<TRef<IAllegEventTarget> >::Iterator iter(m_lstTargets);

        while (!iter.End()) {
            iter.Value()->Disconnect();
            iter.Next();
        }
    }

    void AddEventTarget(PFNOnEventMember pfn, IIntegerEventSource* psource)
    {
        m_lstTargets.PushEnd(new IntegerEventTarget((DerivedClass*)this, pfn, psource));
    }

    void AddEventTarget(PFNOnEventVoidMember pfn, IEventSource* psource)
    {
        m_lstTargets.PushEnd(new EventTarget((DerivedClass*)this, pfn, psource));
    }

    void AddEventTarget(PFNOnEventVoidMember pfn, ITimerEventSource* psource, float when)
    {
        m_lstTargets.PushEnd(new EventTarget((DerivedClass*)this, pfn, psource, when));
    }
};

#endif
