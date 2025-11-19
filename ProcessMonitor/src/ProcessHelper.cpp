#include "Pch.h"
#include "ProcessHelper.h"
#include "Debug.h"

NTSTATUS ProcessHelperHolder::Init()
{
    return m_processHelperList.initialize();
}

ProcessHelperHolder::~ProcessHelperHolder()
{
    ASSERT(m_processHelperList.empty());
}

ProcessHelperList& ProcessHelperHolder::AquireList()
{
    m_lock.lock_shared();
    return m_processHelperList;
}

_Requires_lock_held_(m_lock.m_resource)
void ProcessHelperHolder::ReleaseList()
{
    m_lock.unlock_shared();
}

bool ProcessHelperHolder::Add(ProcessHelperPtr&& procHlp)
{
    std::unique_lock writeLock(m_lock);

    auto result = m_processHelperList.emplace(std::make_pair(procHlp->pid, std::move(procHlp)));
    
    if (!result || !result.value().second) [[unlikely]]
    {
        ERR_FN(("Failed to insert map entry\n"));
    }
    
    return result && result.value().second;
}

void ProcessHelperHolder::Delete(HANDLE pid)
{
    std::unique_lock writeLock(m_lock);

    ProcessHelperList::const_iterator it = m_processHelperList.find(pid);
    if (it != m_processHelperList.end())
    {
        m_processHelperList.erase(it);
    }
}

void ProcessHelperHolder::Clean()
{
    std::unique_lock writeLock(m_lock);

    m_processHelperList.clear();
}
