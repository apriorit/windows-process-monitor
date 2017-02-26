#ifndef __PROCESS_MONITOR_H__
#define __PROCESS_MONITOR_H__

#include "ProcessHelper.h"


#include "utils.h"
#include "new.h"
#include "sync.h"

#include "types.h"

class ProcessMonitor
{
public:
    ProcessMonitor();

    NTSTATUS ProcessIrp(PIRP Irp);

    VOID OnCreateProcess(const HANDLE ParentPid, const HANDLE Pid);
    VOID OnLoadProcessImage(const HANDLE Pid, 
        const PUNICODE_STRING NativeImageName,
        const PVOID ImageBase, const SIZE_T ImageSize, PBOOLEAN TerminateProcess);
    VOID OnDeleteProcess(const HANDLE Pid);

    VOID Cleanup();
    VOID Reset();

private:

    ProcessHelperHolder m_ActiveProcesses;

    ULONG QueryProcessesCount();
    void CopyProcInfo(IN ProcessHelper* ProcHelper, IN OUT PPROCESS_INFO ProcInfo);
    void QueryProcesses(PROCESS_INFO ProcInfo[], ULONG Capacity, ULONG& ActualCount);
    void TerminateBlackMarked();
    NTSTATUS AppendRule(PPROCESS_INFO pInfo, bool BlacklistRule);

    sync::SharedLock m_Lock;
    sync::Event m_UserNotifyEvent;

    BOOLEAN m_Shutdown;
};

#endif //__PROCESS_MONITOR_H__