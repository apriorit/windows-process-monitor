#pragma once

#include "ProcessHelper.h"
#include "Utils.h"
#include "Sync.h"
#include "Types.h"

class ProcessMonitor : public kf::Singleton<ProcessMonitor>
{
public:
    static NTSTATUS Init();
    static void Uninit();
    NTSTATUS ProcessIrp(PIRP irp);

    void OnCreateProcess(const HANDLE parentPid, const HANDLE pid);
    void OnLoadProcessImage(const HANDLE pid, const kf::USimpleString& nativeImageName, bool& terminateProcess);
    void OnDeleteProcess(const HANDLE pid);

    void Cleanup();
    void Reset();

private:
    ULONG QueryProcessesCount();
    void CopyProcInfo(_In_ const ProcessHelperPtr& procHelper, _Inout_ PPROCESS_INFO procInfo);
    void QueryProcesses(PROCESS_INFO procInfo[], ULONG capacity, ULONG& actualCount);
    void TerminateBlackMarked();
    NTSTATUS AppendRule(PPROCESS_INFO info, bool blacklistRule);

private:
    ProcessHelperHolder m_activeProcesses{};
    kf::EResource m_lock{};
    sync::Event m_userNotifyEvent{};
    bool m_shutdown = false;
};
