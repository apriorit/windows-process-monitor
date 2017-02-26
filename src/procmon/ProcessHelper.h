#ifndef __PROCESS_HELPER_H__
#define __PROCESS_HELPER_H__

#include "sync.h"
#include <map>
#include <string>

enum ProcessMarker
{
    Clean,
    Disallowed,
    Allowed
};

struct ProcessHelper
{
    HANDLE ParentPid;
    HANDLE Pid;

    // Used to resume waiting process and select action for it
    sync::Event ResumeEvent;
    std::wstring ImageName;

    ProcessMarker Marker;

    ProcessHelper(HANDLE ParentPid, HANDLE Pid): 
    ParentPid(ParentPid),
        Pid(Pid),
		Marker(ProcessMarker::Clean)
    {
    }

    BOOLEAN IsSuspended() { return ResumeEvent.Valid() ? !ResumeEvent.IsSet() : false; }

    const HANDLE GetPid() const { return Pid; }
};

typedef std::map<HANDLE, ProcessHelper*> ProcessHelperList;

class ProcessHelperHolder
{
    ProcessHelperList mProcessHelperList;
    sync::SharedLock m_Lock;
public:
    ~ProcessHelperHolder();

    ProcessHelperList& AquireList();
    VOID ReleaseList();
    BOOLEAN Add(ProcessHelper* ProcHlp);
    VOID Delete(HANDLE Pid);
    VOID Clean();
};

#endif // __PROCESS_HELPER_H__