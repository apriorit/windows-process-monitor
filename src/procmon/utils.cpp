#include "utils.h"


namespace utils
{
    NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS Status, ULONG_PTR Info)
    {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = Info;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    VOID ScheduleProcessTerminate(HANDLE ProcessId)
    {
        HANDLE threadHandle;
        OBJECT_ATTRIBUTES objAttr;
        TERSE_FN(("Schedule terminate process ProcessId: %d\n", ProcessId));
        InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
        NTSTATUS status = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, 
            &objAttr, NULL, NULL, SystemThreadToTerminateTagetProcess, ProcessId);
        if (status == STATUS_SUCCESS)
        {
            ZwClose(threadHandle);
        }
        else
        {
            ERR_FN(("Failed to thread to terminate ProcessId: %d\n", ProcessId));
        }
    }


    VOID SystemThreadToTerminateTagetProcess(IN PVOID StartContext)
    {
        NTSTATUS status;
        HANDLE processHandle = NULL;
        HANDLE processId = (HANDLE)StartContext;
        PEPROCESS process;

        status = PsLookupProcessByProcessId(processId, &process);
        if (status == STATUS_SUCCESS)
        {
            status = ObOpenObjectByPointer(process, OBJ_KERNEL_HANDLE, NULL, 
                PROCESS_ALL_ACCESS, NULL, KernelMode, &processHandle);
            ObDereferenceObject(process);
            if (status == STATUS_SUCCESS)
            {
                ZwTerminateProcess(processHandle, STATUS_SUCCESS);
                ZwClose(processHandle);
                TERSE_FN(("Terminated ProcessId: %d\n", processId));
            }
        }
        else
        {
            TERSE_FN(("Probably terminated ProcessId: %d\n", processId));
        }

        // Exit system thread
        PsTerminateSystemThread(STATUS_SUCCESS);
    }

    void KernelFileNameToDosName(IN PUNICODE_STRING KernelName, PUNICODE_STRING* DosName)
    {
        NTSTATUS status;
        OBJECT_ATTRIBUTES objAttr;
        HANDLE hFile = NULL;
        PFILE_OBJECT fileObj = NULL;
        IO_STATUS_BLOCK statusBlock;

        InitializeObjectAttributes(&objAttr, KernelName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
        status = ZwOpenFile(&hFile, SYNCHRONIZE | GENERIC_READ, &objAttr, &statusBlock, FILE_SHARE_READ, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
        if (status != STATUS_SUCCESS)
            return;

        ScopedHandle guard(hFile);
        status = ObReferenceObjectByHandle(hFile, 0, *IoFileObjectType, KernelMode, (PVOID*) &fileObj, NULL);
        if (status != STATUS_SUCCESS)
            return;

        IoQueryFileDosDeviceName(fileObj, (POBJECT_NAME_INFORMATION*)DosName);
        //if (status == STATUS_SUCCESS)
        //{
        //    ExFreePool(objInfo);
        //}

        ObDereferenceObject(fileObj);
    }

    NTSTATUS GetSystemProcessInformation(OUT PSYSTEM_PROCESS_INFORMATION* SysProcInfo)
    {
        NTSTATUS status;
        ULONG procInfoLen = 0;

        status = ZwQuerySystemInformation(SystemProcessInformation, 
            NULL, 0, &procInfoLen);

        procInfoLen += 512; // allocate more then needed to increase success chance from first call
        *SysProcInfo = NULL;
        while (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            if (*SysProcInfo)
                ExFreePool(*SysProcInfo);

            *SysProcInfo = (PSYSTEM_PROCESS_INFORMATION)ExAllocatePool(
                PagedPool, procInfoLen);

            if (*SysProcInfo)
                status = ZwQuerySystemInformation(SystemProcessInformation, 
                *SysProcInfo, procInfoLen, &procInfoLen);
        }

        if (!NT_SUCCESS(status) && *SysProcInfo)
        {
            ExFreePool(*SysProcInfo);
            *SysProcInfo = NULL;
        }

        return status;
    }

    NTSTATUS GetMainModuleImageBase(IN HANDLE ProcessId, IN OUT PVOID* ImageBase, IN OUT SIZE_T* ImageSize)
    {
        NTSTATUS status;
        PEPROCESS process;

        status = PsLookupProcessByProcessId(ProcessId, &process);
        if(!NT_SUCCESS(status)) 
            return status;

        KAPC_STATE apcState;
        KeStackAttachProcess((PKPROCESS)process, &apcState);
#pragma warning(disable: 6320)
        __try
        {
            PPEB peb = PsGetProcessPeb(process);
            if(peb == NULL)
            {
                status = STATUS_UNSUCCESSFUL;
                __leave;
            }

            PPEB_LDR_DATA ldr = peb->Ldr;
            if(ldr == NULL)
            {
                status = STATUS_UNSUCCESSFUL;
                __leave;
            }

            PLIST_ENTRY ldrHead = &ldr->InMemoryOrderModuleList;
            PLIST_ENTRY ldrNext = ldrHead->Flink;

            while (ldrNext != ldrHead)
            {
                PLDR_DATA_TABLE_ENTRY ldrEntry;
                ldrEntry = CONTAINING_RECORD(ldrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

                if(ldrEntry->DllBase == peb->ImageBaseAddress)
                {
                    *ImageBase = ldrEntry->DllBase;
                    *ImageSize = ldrEntry->SizeOfImage;
                    status = STATUS_SUCCESS;
                    __leave;
                }
                ldrNext = ldrEntry->InMemoryOrderLinks.Flink;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            ERR_FN(("PEB exception: 0x%X", status));    
        }
#pragma warning(default: 6320)
        KeUnstackDetachProcess(&apcState);

        ObDereferenceObject(process);

        return status;
    }

    NTSTATUS GetProcessImageFileName(IN HANDLE ProcessId, OUT PUNICODE_STRING* ImageName)
    {
        NTSTATUS status;
        HANDLE procHandle = NULL;
        OBJECT_ATTRIBUTES objAttr;
        CLIENT_ID clientId;

        *ImageName = NULL;

        clientId.UniqueProcess = ProcessId;
        clientId.UniqueThread = 0;

        InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

        status = ZwOpenProcess(&procHandle, PROCESS_ALL_ACCESS, &objAttr, &clientId);
        if (!NT_SUCCESS(status))
        {
            WARNING_FN(("Failed to open ProcessId: %d\n", ProcessId));
            return status;
        }

        ScopedHandle guard(procHandle);

        ULONG bufferLen = 0;
        status = ZwQueryInformationProcess(procHandle, ProcessImageFileName, NULL, 
            0, &bufferLen);
        if ((status == STATUS_INFO_LENGTH_MISMATCH) && (bufferLen > sizeof(UNICODE_STRING)))
        {
            *ImageName = (PUNICODE_STRING)ExAllocatePool(PagedPool, bufferLen);
            if (*ImageName)
            {
                status = ZwQueryInformationProcess(procHandle, ProcessImageFileName,
                    *ImageName, bufferLen, &bufferLen);
                if (!NT_SUCCESS(status))
                    ExFreePool(*ImageName);
            }
        }

        return status;
    }
}
