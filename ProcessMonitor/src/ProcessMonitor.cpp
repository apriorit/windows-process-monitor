#include "Pch.h"
#include "ProcessMonitor.h"
#include "Utils.h"
#include "Controls.h"

static UINT8 gProcessMonitor[sizeof(ProcessMonitor)];

NTSTATUS ProcessMonitor::Init()
{
    auto& instance = *new(gProcessMonitor) ProcessMonitor;
    return instance.m_activeProcesses.Init();
}

void ProcessMonitor::Uninit()
{
    reinterpret_cast<ProcessMonitor*>(gProcessMonitor)->~ProcessMonitor();
}

void ProcessMonitor::OnCreateProcess(const HANDLE parentPid, const HANDLE pid)
{
    if (m_shutdown) [[unlikely]]
    {
        return;
    }

    ProcessHelperPtr procHelper = new(PagedPool) ProcessHelper(parentPid, pid);
    if (procHelper)
    {
        if (!m_activeProcesses.Add(std::move(procHelper)))
        {
            return;
        }
    }
    else [[unlikely]]
    {
        ERR_FN(("Failed to allocate memory for ProcessHelper object\n"));
    }
}

void ProcessMonitor::OnLoadProcessImage(const HANDLE pid,
                                        const kf::USimpleString& nativeImageName,
                                        bool& terminateProcess)
{
    // Don't do anyting during shutdown stage.
    {
        std::shared_lock sharedLock(m_lock);
        if (m_shutdown) [[unlikely]]
        {
            return;
        }
    }

    ProcessHelperPtr procHelper = nullptr;
    // Explicit scope to access and release list
    {
        const ProcessHelperList& procList = m_activeProcesses.AquireList();
        utils::ScopedListReleaser<ProcessHelperHolder> procListGuard(m_activeProcesses);

        ProcessHelperList::const_iterator it = procList.find(pid);
        if (it != procList.end())
        {
            procHelper = it->second;
        }

        if (!procHelper)
        {
            return;
        }
    }

    // Check if process already have ImageName assigned. If assigned, then, 
    // current callback for dll image which is mapped in process address space.
    // Ignore it in this case.
    if (!procHelper->imageName.isEmpty())
    {
        return;
    }

    // Convert native image path to DOS path. Lowercase final path to optimize 
    // path checking speed on rules processing.
    kf::UString<PagedPool> dosName{};
    utils::KernelFileNameToDosName(nativeImageName, dosName);
    if (!dosName.isEmpty())
    {
        if (NTSTATUS status = dosName.toLowerCase(); !NT_SUCCESS(status)) [[unlikely]]
        {
            ERR_FN(("Failed to convert the process dos image name to lower case. Clean process record."));
            m_activeProcesses.Delete(pid);
            return;
        }

        procHelper->imageName.init(std::move(dosName));
    }
    else
    {
        procHelper->imageName.init(nativeImageName);
    }

    if (procHelper->imageName.charLength() == 0) [[unlikely]]
    {
        ERR_FN(("Failed to allocate memory for ImageName. Clean process record."));
        m_activeProcesses.Delete(pid);
        return;
    }

    // Artificail block to sync. access for mRulesLoaded & mInitialization flags.
    {
        std::shared_lock sharedLock(m_lock);

        if (!m_userNotifyEvent.Valid()) [[unlikely]]
        {
            return;
        }

        if (!procHelper->resumeEvent.Initialize(false, false)) [[unlikely]]
        {
            ERR_FN(("Failded to initialize resume event for ProcessId: %lu\n", HandleToULong(procHelper->pid)));
            return;
        }

        // Notify user mode library counterpart (if core in Normal state)
        m_userNotifyEvent.Set();
    }

    NTSTATUS waitStatus = STATUS_SUCCESS;
    unsigned waitTimeout = 4 * 60 * 1000;
    if (procHelper->resumeEvent.Wait(waitTimeout, &waitStatus) && waitStatus == STATUS_TIMEOUT)
    {
        Reset();
    }

    terminateProcess = procHelper->marker == ProcessMarker::Disallowed;
}

void ProcessMonitor::OnDeleteProcess(const HANDLE pid)
{
    m_activeProcesses.Delete(pid);
}

ULONG ProcessMonitor::QueryProcessesCount()
{
    const ProcessHelperList& procList = m_activeProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGuard(m_activeProcesses);

    ULONG count = 0;
    for (auto it = procList.begin(); it != procList.end(); ++it)
    {
        if (it->second->marker == ProcessMarker::Clean)
        {
            ++count;
        }
    }

    return count;
}

void ProcessMonitor::CopyProcInfo(_In_  const ProcessHelperPtr& procHelper, _Inout_ PPROCESS_INFO procInfo)
{
    procInfo->ProcessId = HandleToULong(procHelper->pid);

    memcpy(procInfo->ImagePath, procHelper->imageName.buffer(),
        (procHelper->imageName.charLength()) * sizeof(wchar_t));

    procInfo->ImagePath[procHelper->imageName.charLength()] = L'\0';
}

void ProcessMonitor::QueryProcesses(PROCESS_INFO procInfo[], ULONG capacity, ULONG& actualCount)
{
    const ProcessHelperList& procList = m_activeProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGuard(m_activeProcesses);

    actualCount = 0;
    for (auto it = procList.begin(); it != procList.end(); ++it)
    {
        const ProcessHelperPtr& procHelper = it->second;

        if (procHelper->marker == ProcessMarker::Clean)
        {
            CopyProcInfo(procHelper, &procInfo[actualCount]);

            ++actualCount;
            if (actualCount == capacity)
            {
                break;
            }
        }
    }
}

void ProcessMonitor::TerminateBlackMarked()
{
    // Terminate all black processes.
    const ProcessHelperList& procList = m_activeProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGuard(m_activeProcesses);
    for (auto it = procList.begin(); it != procList.end(); ++it)
    {
        const ProcessHelperPtr& procHelper = it->second;
        if (procHelper->marker == ProcessMarker::Disallowed)
        {
            if (procHelper->IsSuspended())
            {
                procHelper->resumeEvent.Set();
            }
            else
            {
                utils::ScheduleProcessTerminate(procHelper->pid);
            }
        }
    }
}

NTSTATUS ProcessMonitor::AppendRule(PPROCESS_INFO processInfo, bool blacklistRule)
{
    std::shared_lock sharedLock(m_lock);

    // Find process which match rule.
    const ProcessHelperList& procList = m_activeProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGuard(m_activeProcesses);
    for (auto it = procList.begin(); it != procList.end(); ++it)
    {
        const ProcessHelperPtr& procHelper = it->second;
        if (procHelper->pid == ULongToHandle(processInfo->ProcessId))
        {
            procHelper->marker = blacklistRule ? ProcessMarker::Disallowed : ProcessMarker::Allowed;
            // Any rule added after Initialization interpreted as user controlled action
            if (procHelper->IsSuspended())
            {
                // Resume suspended process 
                // (action will be applied to it depending of marker)
                procHelper->resumeEvent.Set();
            }
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS ProcessMonitor::ProcessIrp(PIRP irp)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ULONG_PTR bytesWritten = 0;

    PIO_STACK_LOCATION irpStack = ::IoGetCurrentIrpStackLocation(irp);

    ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    ULONG method = controlCode & 0x03;
    if (method != METHOD_BUFFERED)
    {
        return utils::CompleteIrp(irp, STATUS_INVALID_PARAMETER, bytesWritten);
    }

    ULONG inputBufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;

    ULONG outputBufferSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    PUCHAR buffer = static_cast<PUCHAR>(irp->AssociatedIrp.SystemBuffer);

    switch (controlCode)
    {
    case IOCTL_GET_PROCESS_COUNT:
    {
        if (outputBufferSize >= sizeof(ULONG))
        {
            *reinterpret_cast<PULONG>(buffer) = QueryProcessesCount();
            bytesWritten = sizeof(ULONG);
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    }
    case IOCTL_RESET:
    {
        Reset();
        status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_GET_PROCESS_LIST:
    {
        if (outputBufferSize >= sizeof(PROCESS_INFO))
        {
            ULONG entriesProcessed = 0;
            QueryProcesses(reinterpret_cast<PPROCESS_INFO>(buffer), outputBufferSize / sizeof(PROCESS_INFO), entriesProcessed);
            bytesWritten = entriesProcessed * sizeof(PROCESS_INFO);
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    }
    case IOCTL_TERMINATE_ALL:
    {
        TerminateBlackMarked();
        status = STATUS_SUCCESS;
        break;
    }
    case IOCTL_ALLOW:
    {
        if (inputBufferSize >= sizeof(PPROCESS_INFO))
        {
            status = AppendRule(reinterpret_cast<PPROCESS_INFO>(buffer), false);
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    }
    case IOCTL_BLOCK:
    {
        if (inputBufferSize >= sizeof(PPROCESS_INFO))
        {
            status = AppendRule(reinterpret_cast<PPROCESS_INFO>(buffer), true);
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        break;
    }
    case IOCTL_REGISTER_EVENT:
    {
        HANDLE event = nullptr;
#if defined(_WIN64)
        if (::IoIs32bitProcess(irp))
        {
            if (inputBufferSize == sizeof(UINT32))
            {
                UINT32 handle32{};
                handle32 = *reinterpret_cast<PUINT32>(buffer);
                event = LongToHandle(handle32);
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
        else
#endif
        {
            if (inputBufferSize == sizeof(HANDLE))
            {
                event = *reinterpret_cast<PHANDLE>(buffer);
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
        }

        std::unique_lock writeLock(m_lock);
        m_userNotifyEvent.Cleanup();
        status = m_userNotifyEvent.Initialize(event) ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;

        // Set control process flag
        if (m_userNotifyEvent.Valid())
        {
            const ProcessHelperList& procList = m_activeProcesses.AquireList();
            utils::ScopedListReleaser<ProcessHelperHolder> procListGuard(m_activeProcesses);

            ProcessHelperList::const_iterator proc = procList.find(::PsGetCurrentProcessId());
            ASSERT(proc != procList.end());

            // Look for suspended processes. If just one find. Signal event.
            for (auto it = procList.begin(); it != procList.end(); ++it)
            {
                const ProcessHelperPtr& procHelper = it->second;
                if (procHelper->IsSuspended())
                {
                    m_userNotifyEvent.Set();
                    break;
                }
            }
        }

        break;
    }
    default:
        status = STATUS_NOT_IMPLEMENTED;
    }

    return utils::CompleteIrp(irp, status, bytesWritten);
}

void ProcessMonitor::Cleanup()
{
    {
        std::unique_lock exclusiveLock(m_lock);
        m_shutdown = true;
    }

    Reset();

    m_activeProcesses.Clean();
}

void ProcessMonitor::Reset()
{
    TERSE_FN(("\n"));

    std::unique_lock writeLock(m_lock);

    if (m_userNotifyEvent.Valid())
    {
        m_userNotifyEvent.Set();
        m_userNotifyEvent.Cleanup();
    }

    const ProcessHelperList& procList = m_activeProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGuard(m_activeProcesses);
    for (auto it = procList.begin(); it != procList.end(); ++it)
    {
        const ProcessHelperPtr& procHelper = it->second;
        if (procHelper->IsSuspended())
        {
            procHelper->resumeEvent.Set();
        }
    }
}