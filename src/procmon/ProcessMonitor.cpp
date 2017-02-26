#include "ProcessMonitor.h"
#include "utils.h"
#include "controls.h"

ProcessMonitor::ProcessMonitor() :  m_Shutdown(false)
{
}


VOID ProcessMonitor::OnCreateProcess(const HANDLE ParentPid, const HANDLE Pid)
{
    ProcessHelper* procHelper = new ProcessHelper(ParentPid, Pid);
    if (procHelper)
    {
        if (!m_ActiveProcesses.Add(procHelper))
            return;
    }
    else
    {
        ERR_FN(("Failed to allocate memory for ProcessHelper object\n"));
    }
}

VOID ProcessMonitor::OnLoadProcessImage(const HANDLE Pid, 
                                            const PUNICODE_STRING NativeImageName,
                                            const PVOID ImageBase,
                                            const SIZE_T ImageSize,
                                            PBOOLEAN TerminateProcess)
{
    // Don't do anyting during shutdown stage.
    {
        sync::AutoReadLock sharedLock(m_Lock);
        if (m_Shutdown)
            return;
    }

    ProcessHelper* procHelper = NULL;
    // Explicit scope to access and release list
    {
        ProcessHelperList procList = m_ActiveProcesses.AquireList();
        utils::ScopedListReleaser<ProcessHelperHolder> procListGaurd(m_ActiveProcesses);

        ProcessHelperList::iterator it = procList.find(Pid);
        if (it != procList.end())
            procHelper = it->second;

        if (!procHelper)
            return;
    }

    // Check if process already have ImageName assigned. If assigned, then, 
    // current callback for dll image which is mapped in process address space.
    // Ignore it in this case.
    if (!procHelper->ImageName.empty())
        return;

    // Convert native image path to DOS path. Lowercase final path to optimize 
    // path checking speed on rules processing.
    PUNICODE_STRING dosName = NULL;
    utils::KernelFileNameToDosName(NativeImageName, &dosName);
    if (dosName)
    {
        RtlDowncaseUnicodeString(dosName, dosName, FALSE);
        utils::nothrow_string_assign<std::wstring, WCHAR>(&procHelper->ImageName, dosName->Buffer, dosName->Length / sizeof(WCHAR));
        ExFreePool(dosName);
    }
    else
    {
        utils::nothrow_string_assign<std::wstring, WCHAR>(&procHelper->ImageName, NativeImageName->Buffer, NativeImageName->Length / sizeof(WCHAR));
    }

    if (procHelper->ImageName.size() == 0)
    {
        ERR_FN(("Failed to allocate memory for ImageName. Clean process record."));
        m_ActiveProcesses.Delete(Pid);
        return;
    }


    // Artificail block to sync. access for mRulesLoaded & mInitialization flags.
    {
        sync::AutoReadLock sharedLock(m_Lock);

        if (!m_UserNotifyEvent.Valid())
            return;

        if (!procHelper->ResumeEvent.Initialize(false, false))
        {
            ERR_FN(("Failded to initialize resume event for ProcessId: %d\n", procHelper->Pid));
            return;
        }

        // Notify user mode library counterpart (if core in Normal state)
        m_UserNotifyEvent.Set();
    }    

    NTSTATUS waitStatus = STATUS_SUCCESS;
    unsigned waitTimeout = 4*60*1000;
    if (procHelper->ResumeEvent.Wait(waitTimeout, &waitStatus) && waitStatus == STATUS_TIMEOUT)
        Reset();

    *TerminateProcess = procHelper->Marker == ProcessMarker::Disallowed;
}

VOID ProcessMonitor::OnDeleteProcess(const HANDLE Pid)
{
    m_ActiveProcesses.Delete(Pid);
}

ULONG ProcessMonitor::QueryProcessesCount()
{
    ProcessHelperList procList = m_ActiveProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGaurd(m_ActiveProcesses);

    ULONG count = 0;
    ProcessHelperList::iterator it;
    for (it = procList.begin(); it != procList.end(); it++)
        if (it->second->Marker == ProcessMarker::Clean)
            count++;

    return count;
}

void ProcessMonitor::CopyProcInfo(IN ProcessHelper* ProcHelper, IN OUT PPROCESS_INFO ProcInfo)
{
    ProcInfo->ProcessId = (ULONG)ProcHelper->GetPid();

    memcpy(ProcInfo->ImagePath, ProcHelper->ImageName.c_str(), 
        (ProcHelper->ImageName.size() + 1) * sizeof(wchar_t));
}

void ProcessMonitor::QueryProcesses(PROCESS_INFO ProcInfo[], ULONG Capacity, ULONG& ActualCount)
{
    ProcessHelperList procList = m_ActiveProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGaurd(m_ActiveProcesses);

    ActualCount = 0;
    ProcessHelperList::iterator it;
    for (it = procList.begin(); it != procList.end(); it++)
    {
        ProcessHelper* procHelper = it->second;

        if (procHelper->Marker == ProcessMarker::Clean)
        {
            CopyProcInfo(procHelper, &ProcInfo[ActualCount]);

            ActualCount++;
            if (ActualCount == Capacity)
                break;
        }
    }
}

void ProcessMonitor::TerminateBlackMarked()
{
    // Terminate all black processes.
    ProcessHelperList procList = m_ActiveProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGaurd(m_ActiveProcesses);
    ProcessHelperList::iterator it;
    for (it = procList.begin(); it != procList.end(); it++)
    {
        ProcessHelper* procHelper = it->second;
        if (procHelper->Marker == ProcessMarker::Disallowed)
        {
            if (procHelper->IsSuspended())
                procHelper->ResumeEvent.Set();
            else
                utils::ScheduleProcessTerminate(procHelper->Pid);
        }
    }
}

NTSTATUS ProcessMonitor::AppendRule(PPROCESS_INFO pInfo, bool BlacklistRule)
{
    sync::AutoReadLock sharedLock(m_Lock);

    // Find process which match rule.
    ProcessHelperList procList = m_ActiveProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGaurd(m_ActiveProcesses);
    ProcessHelperList::iterator it;
    for (it = procList.begin(); it != procList.end(); it++)
    {
        ProcessHelper* procHelper = it->second;
        if (procHelper->GetPid() == (HANDLE)(pInfo->ProcessId))
        {
            procHelper->Marker = BlacklistRule ? ProcessMarker::Disallowed : ProcessMarker::Allowed;
            // Any rule added after Initialization interpreted as user controlled action
            if (procHelper->IsSuspended())
            {
                // Resume suspended process 
                // (action will be applied to it depending of marker)
                procHelper->ResumeEvent.Set();
            }
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS ProcessMonitor::ProcessIrp(PIRP Irp)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    ULONG_PTR BytesWritten = 0;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    ULONG ControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    ULONG method = ControlCode & 0x03;
    if (method != METHOD_BUFFERED)
        return utils::CompleteIrp(Irp, STATUS_INVALID_PARAMETER, BytesWritten);

    ULONG InputBufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;

    ULONG OutputBufferSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    PUCHAR Buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

    switch (ControlCode) 
    {
    case IOCTL_GET_PROCESS_COUNT:
        {
            if (OutputBufferSize >= sizeof(ULONG))
            {
                *(PULONG)Buffer = QueryProcessesCount();
                BytesWritten = sizeof(ULONG);
                status = STATUS_SUCCESS;
            }
            else
                status = STATUS_INVALID_PARAMETER;
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
            if (OutputBufferSize >= sizeof(PROCESS_INFO))
            {
                ULONG entriesProcessed = 0;
                QueryProcesses((PPROCESS_INFO)Buffer, OutputBufferSize/sizeof(PROCESS_INFO), entriesProcessed);
                BytesWritten = entriesProcessed * sizeof(PROCESS_INFO);
                status = STATUS_SUCCESS;
            }
            else
                status = STATUS_INVALID_PARAMETER;
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
            if (InputBufferSize >= sizeof(PPROCESS_INFO))
                status = AppendRule((PPROCESS_INFO)Buffer, false);
            else
                status = STATUS_INVALID_PARAMETER;
            break;
        }
    case IOCTL_BLOCK:
        {
            if (InputBufferSize >= sizeof(PPROCESS_INFO))
                status = AppendRule((PPROCESS_INFO)Buffer, true);
            else
                status = STATUS_INVALID_PARAMETER;
            break;
        }
    case IOCTL_REGISTER_EVENT:
        {
            HANDLE  hEvent = NULL;
#if defined(_WIN64)
            if (IoIs32bitProcess(Irp))
            {
                if (InputBufferSize == sizeof(UINT32))
                {
                    UINT32  Handle32;
                    Handle32 = *((PUINT32)Buffer);
                    hEvent = LongToHandle(Handle32);
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
                if (InputBufferSize == sizeof(HANDLE))
                {
                    hEvent = *((PHANDLE)Buffer);
                }
                else
                {
                    status = STATUS_INVALID_PARAMETER;
                    break;
                }
            }

            sync::AutoWriteLock autoWriteLock(m_Lock);
            m_UserNotifyEvent.Cleanup();
            status = m_UserNotifyEvent.Initialize(hEvent) ? STATUS_SUCCESS : STATUS_INVALID_HANDLE;

            // Set control process flag
            if (m_UserNotifyEvent.Valid())
            {
                ProcessHelperList procList = m_ActiveProcesses.AquireList();
                utils::ScopedListReleaser<ProcessHelperHolder> procListGaurd(m_ActiveProcesses);

                ProcessHelperList::const_iterator proc = procList.find(PsGetCurrentProcessId());
                ASSERT(proc != procList.end());

                // Look for suspended processes. If just one find. Signal event.
                ProcessHelperList::iterator it2;
                for (it2 = procList.begin(); it2 != procList.end(); it2++)
                {
                    ProcessHelper* procHelper = it2->second;
                    if (procHelper->IsSuspended())
                    {
                        m_UserNotifyEvent.Set();
                        break;
                    }
                }
            }

            break;
        }
    default:
        status = STATUS_NOT_IMPLEMENTED;
    }

    return utils::CompleteIrp(Irp, status, BytesWritten);
}

VOID ProcessMonitor::Cleanup()
{
    {
        sync::AutoReadLock exclusiveLock(m_Lock);
        m_Shutdown = TRUE;
    }

    Reset();

    m_ActiveProcesses.Clean();
}

VOID ProcessMonitor::Reset()
{
    TERSE_FN(("\n"));

    sync::AutoWriteLock autoWriteLock(m_Lock);

    if (m_UserNotifyEvent.Valid())
    {
        m_UserNotifyEvent.Set();
        m_UserNotifyEvent.Cleanup();
    }


    ProcessHelperList procList = m_ActiveProcesses.AquireList();
    utils::ScopedListReleaser<ProcessHelperHolder> procListGaurd(m_ActiveProcesses);
    ProcessHelperList::iterator it;
    for (it = procList.begin(); it != procList.end(); it++)
    {        
        ProcessHelper* procHelper = it->second;
        if (procHelper->IsSuspended())
        {
            procHelper->ResumeEvent.Set();
        }
    }
}