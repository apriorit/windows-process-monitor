#include "Pch.h"
#include "Utils.h"

namespace utils
{
    NTSTATUS CompleteIrp(PIRP irp, NTSTATUS status, ULONG_PTR info)
    {
        irp->IoStatus.Status = status;
        irp->IoStatus.Information = info;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;
    }

    void ScheduleProcessTerminate(HANDLE processId)
    {
        TERSE_FN(("Schedule terminate process ProcessId: %lu\n", HandleToULong(processId)));

        OBJECT_ATTRIBUTES objAttr{};
        InitializeObjectAttributes(&objAttr, nullptr, OBJ_KERNEL_HANDLE, nullptr, nullptr);

        kf::Guard::Handle threadHandle{};
        NTSTATUS status = ::PsCreateSystemThread(&threadHandle.get(), THREAD_ALL_ACCESS,
            &objAttr, nullptr, nullptr, SystemThreadToTerminateTargetProcess, processId);

        if (!NT_SUCCESS(status)) [[unlikely]]
        {
            ERR_FN(("Failed to thread to terminate ProcessId: %lu\n", HandleToULong(processId)));
        }
    }

    void SystemThreadToTerminateTargetProcess(_In_ PVOID startContext)
    {
        HANDLE processId = static_cast<HANDLE>(startContext);
        kf::Guard::EProcess process{};

        NTSTATUS status = ::PsLookupProcessByProcessId(processId, &process.get());
        if (NT_SUCCESS(status)) [[likely]]
        {
            kf::Guard::Handle processHandle{};
            status = ::ObOpenObjectByPointer(process, OBJ_KERNEL_HANDLE, nullptr,
                PROCESS_ALL_ACCESS, nullptr, KernelMode, &processHandle.get());

            if (NT_SUCCESS(status)) [[likely]]
            {
                ::ZwTerminateProcess(processHandle, STATUS_SUCCESS);
                TERSE_FN(("Terminated ProcessId: %lu\n", HandleToULong(processId)));
            }
        }
        else [[unlikely]]
        {
            TERSE_FN(("Probably terminated ProcessId: %lu\n", HandleToULong(processId)));
        }

        // Exit system thread
        ::PsTerminateSystemThread(STATUS_SUCCESS);
    }

    void KernelFileNameToDosName(_In_ const kf::USimpleString& kernelName, _Out_ kf::UString<PagedPool>& dosName)
    {
        dosName.free();

        OBJECT_ATTRIBUTES objAttr{};
        InitializeObjectAttributes(&objAttr, const_cast<PUNICODE_STRING>(&kernelName.string()), OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);

        kf::Guard::Handle file{};
        IO_STATUS_BLOCK statusBlock{};
        NTSTATUS status = ::ZwOpenFile(&file.get(), SYNCHRONIZE | GENERIC_READ, &objAttr, &statusBlock, FILE_SHARE_READ, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(status)) [[unlikely]]
        {
            return;
        }

        kf::Guard::FileObject fileObj{};
        status = ::ObReferenceObjectByHandle(file, 0, *IoFileObjectType, KernelMode, reinterpret_cast<PVOID*>(&fileObj.get()), nullptr);
        if (!NT_SUCCESS(status)) [[unlikely]]
        {
            return;
        }

        POBJECT_NAME_INFORMATION dosFilePath = nullptr;
        SCOPE_EXIT{ ExFreePool(dosFilePath); };
        status = ::IoQueryFileDosDeviceName(fileObj, &dosFilePath);

        dosName.init(dosFilePath->Name);
    }

    NTSTATUS GetSystemProcessInformation(_Out_  kf::vector<std::byte, PagedPool>& sysProcInfo)
    {
        sysProcInfo.clear();

        ULONG procInfoLen = 0;
        NTSTATUS status = ::ZwQuerySystemInformation(SystemProcessInformation, nullptr, 0, &procInfoLen);

        procInfoLen += 512; // allocate more then needed to increase success chance from first call

        while (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            status = sysProcInfo.resize(procInfoLen);
            if (!NT_SUCCESS(status)) [[unlikely]]
            {
                return status;
            }

            status = ::ZwQuerySystemInformation(SystemProcessInformation, sysProcInfo.data(), procInfoLen, &procInfoLen);
        }

        if (!NT_SUCCESS(status)) [[unlikely]]
        {
            sysProcInfo.clear();
        }

        return status;
    }

    NTSTATUS GetMainModuleImageBase(_In_ HANDLE processId, _Inout_ PVOID* imageBase, _Inout_ SIZE_T* imageSize)
    {
        kf::Guard::EProcess process{};
        NTSTATUS status = ::PsLookupProcessByProcessId(processId, &process.get());
        if (!NT_SUCCESS(status)) [[unlikely]]
        {
            return status;
        }

        KAPC_STATE apcState{};
        ::KeStackAttachProcess(static_cast<PKPROCESS>(process), &apcState);
        [&] {
#pragma warning(disable: 6320)
            __try
            {
                PPEB peb = ::PsGetProcessPeb(process);
                if (!peb) [[unlikely]]
                {
                    status = STATUS_UNSUCCESSFUL;
                    __leave;
                }

                PPEB_LDR_DATA ldr = peb->Ldr;
                if (!ldr) [[unlikely]]
                {
                    status = STATUS_UNSUCCESSFUL;
                    __leave;
                }

                PLIST_ENTRY ldrHead = &ldr->InMemoryOrderModuleList;
                PLIST_ENTRY ldrNext = ldrHead->Flink;

                while (ldrNext != ldrHead)
                {
                    PLDR_DATA_TABLE_ENTRY ldrEntry = nullptr;
                    ldrEntry = CONTAINING_RECORD(ldrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

                    if (ldrEntry->DllBase == peb->ImageBaseAddress)
                    {
                        *imageBase = ldrEntry->DllBase;
                        *imageSize = ldrEntry->SizeOfImage;
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
        }();
#pragma warning(default: 6320)
        ::KeUnstackDetachProcess(&apcState);

        return status;
    }

    NTSTATUS GetProcessImageFileName(_In_ HANDLE processId, _Out_ kf::UString<PagedPool>& imageName)
    {
        imageName.free();

        OBJECT_ATTRIBUTES objAttr{};
        InitializeObjectAttributes(&objAttr, nullptr, OBJ_KERNEL_HANDLE, nullptr, nullptr);

        kf::Guard::Handle procHandle{};
        CLIENT_ID clientId{ .UniqueProcess = processId, .UniqueThread = 0 };

        NTSTATUS status = ::ZwOpenProcess(&procHandle.get(), PROCESS_ALL_ACCESS, &objAttr, &clientId);
        if (!NT_SUCCESS(status)) [[unlikely]]
        {
            WARNING_FN(("Failed to open ProcessId: %lu\n", HandleToULong(processId)));
            return status;
        }

        ULONG bufferLen = 0;
        status = ::ZwQueryInformationProcess(procHandle, ProcessImageFileName, nullptr, 0, &bufferLen);

        if ((status == STATUS_INFO_LENGTH_MISMATCH) && (bufferLen > sizeof(UNICODE_STRING))) [[likely]]
        {
            status = imageName.realloc(bufferLen);
            if (!NT_SUCCESS(status)) [[unlikely]]
            {
                return status;
            }

            status = ::ZwQueryInformationProcess(
                procHandle,
                ProcessImageFileName,
                imageName.buffer(),
                imageName.maxByteLength(),
                &bufferLen
            );

            imageName.setString(*reinterpret_cast<PCUNICODE_STRING>(imageName.buffer()));
        }

        return status;
    }
}
