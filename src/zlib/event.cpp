#include "event.h"

/////////////////////////////////////////////////////////////////////////////
//
// Never Event Source
//
/////////////////////////////////////////////////////////////////////////////

class NeverEvent : public IEventSource {
public:
    void AddSink(IEventSink* psink)    {}
    void RemoveSink(IEventSink* psink) {}
};

static TRef<IEventSource> g_pnever = new NeverEvent();

IEventSource* IEventSource::GetNever()
{
    return g_pnever;
}

/////////////////////////////////////////////////////////////////////////////
//
// Event Source
//
/////////////////////////////////////////////////////////////////////////////

void EventSourceImpl::AddSink(IEventSink* psink)
{
    m_listSinks.PushFront(psink);
}

void EventSourceImpl::RemoveSink(IEventSink* psink)
{
    m_listSinks.Remove(psink);
}

void EventSourceImpl::Trigger()
{
    TRef<IEventSource> refThis = this;

    TList<TRef<IEventSink> >::Iterator iter(m_listSinks);

    while (!iter.End()) {
        TRef<IEventSink> value = iter.Value();
        if (!value->OnEvent(this)) {
            iter.Remove();
        } else {
            iter.Next();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// EventSinkDelegate
//
/////////////////////////////////////////////////////////////////////////////

class EventSinkDelegate : public IEventSink {
private:
    IEventSink* m_psink;

public:
    EventSinkDelegate(IEventSink* psink) :
        m_psink(psink)
    {
    }

    bool OnEvent(IEventSource* pevent)
    {
        return m_psink->OnEvent(pevent);
    }
};

TRef<IEventSink> IEventSink::CreateDelegate(IEventSink* psink)
{
    return new EventSinkDelegate(psink);
}

/////////////////////////////////////////////////////////////////////////////
//
// Integer Event Source
//
/////////////////////////////////////////////////////////////////////////////

void IntegerEventSourceImpl::AddSink(IEventSink* psink)
{
    m_listSinks.PushFront(psink);
}

void IntegerEventSourceImpl::RemoveSink(IEventSink* psink)
{
    m_listSinks.Remove(psink);
}

void IntegerEventSourceImpl::AddSink(IIntegerEventSink* psink)
{
    m_listIntegerSinks.PushFront(psink);
}

void IntegerEventSourceImpl::RemoveSink(IIntegerEventSink* psink)
{
    m_listIntegerSinks.Remove(psink);
}

void IntegerEventSourceImpl::Trigger(int value)
{
    {
        TList<TRef<IEventSink> >::Iterator iter(m_listSinks);

        while (!iter.End()) {
            if (!iter.Value()->OnEvent(this)) {
                iter.Remove();
            } else {
                iter.Next();
            }
        }
    }

    {
        TList<TRef<IIntegerEventSink> >::Iterator iter(m_listIntegerSinks);

        while (!iter.End()) {
            if (!iter.Value()->OnEvent(this, value)) {
                iter.Remove();
            } else {
                iter.Next();
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// IntegerEventSinkDelegate
//
/////////////////////////////////////////////////////////////////////////////

class IntegerEventSinkDelegate : public IIntegerEventSink {
private:
    IIntegerEventSink* m_psink;

public:
    IntegerEventSinkDelegate(IIntegerEventSink* psink) :
        m_psink(psink)
    {
    }

    bool OnEvent(IIntegerEventSource* pevent, int value)
    {
        return m_psink->OnEvent(pevent, value);
    }
};

TRef<IIntegerEventSink> IIntegerEventSink::CreateDelegate(IIntegerEventSink* psink)
{
    return new IntegerEventSinkDelegate(psink);
}
