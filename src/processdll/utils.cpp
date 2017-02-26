#include "utils.h"
#include <Windows.h>
#include <stdexcept>

namespace utils
{

Event::Event(bool InitialState, bool AutoReset)
{
    m_hEvent = CreateEvent(NULL, !AutoReset, InitialState, NULL);
    if (m_hEvent == NULL)
        throw std::runtime_error("Unable to create event");
}

Event::~Event()
{
    CloseHandle(m_hEvent);
}

bool Event::Valid()
{
    return m_hEvent != NULL;
}

void Event::Set()
{
    SetEvent(m_hEvent);
}

void Event::Reset()
{
    ResetEvent(m_hEvent);
}

bool Event::IsSet()
{
    return (WaitForSingleObject(m_hEvent, 0) == WAIT_OBJECT_0);
}

bool Event::Wait(unsigned Timeout)
{
    return (WaitForSingleObject(m_hEvent, Timeout) == WAIT_OBJECT_0);
}

HANDLE Event::GetHandle()
{
    return m_hEvent;
}

}