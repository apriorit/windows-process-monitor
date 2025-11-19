#include "Utils.h"

namespace utils
{
    Event::Event(bool initialState, bool autoReset) : m_event(::CreateEventW(nullptr, !autoReset, initialState, nullptr))
    {
        if (!m_event)
        {
            throw std::runtime_error("Unable to create event");
        }
    }

    bool Event::Valid()
    {
        return !!m_event;
    }

    void Event::Set()
    {
        ::SetEvent(m_event);
    }

    void Event::Reset()
    {
        ::ResetEvent(m_event);
    }

    bool Event::IsSet()
    {
        return ::WaitForSingleObject(m_event, 0) == WAIT_OBJECT_0;
    }

    bool Event::Wait(DWORD timeout)
    {
        return ::WaitForSingleObject(m_event, timeout) == WAIT_OBJECT_0;
    }

    HANDLE Event::GetHandle()
    {
        return m_event;
    }
}