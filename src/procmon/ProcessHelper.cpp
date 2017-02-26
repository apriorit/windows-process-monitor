#include "ProcessHelper.h"
#include "debug.h"

ProcessHelperHolder::~ProcessHelperHolder()
{
    ASSERT(mProcessHelperList.empty());
}

ProcessHelperList& ProcessHelperHolder::AquireList()
{
    m_Lock.LockRead();
    return mProcessHelperList;
}

VOID ProcessHelperHolder::ReleaseList()
{
    m_Lock.UnlockRead();
}

BOOLEAN ProcessHelperHolder::Add(ProcessHelper* ProcHlp)
{
    sync::AutoWriteLock autoWriteLock(m_Lock);

    std::pair<ProcessHelperList::iterator, BOOLEAN> result = mProcessHelperList.insert(std::make_pair(ProcHlp->GetPid(), ProcHlp));
    
    if (!result.second)
    {
        delete ProcHlp;
        ERR_FN(("Failed to insert map entry\n"));
    }
    
    return result.second;
}

VOID ProcessHelperHolder::Delete(HANDLE Pid)
{
    sync::AutoWriteLock autoWriteLock(m_Lock);

    ProcessHelperList::iterator it = mProcessHelperList.find(Pid);
    if (it != mProcessHelperList.end())
    {
        delete it->second;
        mProcessHelperList.erase(it);
    }
}

VOID ProcessHelperHolder::Clean()
{
    sync::AutoWriteLock autoWriteLock(m_Lock);

    ProcessHelperList::iterator it;
    for (it = mProcessHelperList.begin(); it != mProcessHelperList.end(); it++)
        delete it->second;

    mProcessHelperList.clear();
}
