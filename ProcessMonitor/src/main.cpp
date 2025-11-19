#include "Pch.h"
#include "ProcessMonitor.h"

UNICODE_STRING gDeviceName       = RTL_CONSTANT_STRING(L"\\DosDevices\\ProcessMonitor");
UNICODE_STRING gSymbolicLinkName = RTL_CONSTANT_STRING(L"\\Device\\ProcessMonitor");

PDEVICE_OBJECT gDeviceObject = nullptr;

extern "C"
DRIVER_INITIALIZE DriverEntry;

DRIVER_UNLOAD DriverUnload;
__drv_dispatchType(IRP_MJ_CREATE)         DRIVER_DISPATCH DeviceOpenHandleRoutine;
__drv_dispatchType(IRP_MJ_CLOSE)          DRIVER_DISPATCH DeviceCloseHandleRoutine;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH DeviceControlRoutine;

NTSTATUS Initialize();
void     Cleanup();

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject,
                     _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    TERSE_FN(("Enter\n"));

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DeviceOpenHandleRoutine;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DeviceCloseHandleRoutine;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControlRoutine;

    NTSTATUS status = ::IoCreateDevice(DriverObject,       // pointer on DriverObject
        0,                                      // additional size of memory, for device extension
        &gDeviceName,                           // pointer to UNICODE_STRING
        FILE_DEVICE_UNKNOWN,                    // Device type
        0,                                      // Device characteristic
        false,                                  // "Exclusive" device
        &gDeviceObject);                        // pointer do device object

    if (NT_SUCCESS(status)) [[likely]]
    {
        status = ::IoCreateSymbolicLink(&gSymbolicLinkName, &gDeviceName);
    }

    if (NT_SUCCESS(status)) [[likely]]
    {
        status = Initialize();
    }

    if (!NT_SUCCESS(status)) [[unlikely]]
    {
        // Its safe to call, even if Initialize() failed
        Cleanup();
    }

    TERSE_FN(("Exit 0x%X\n", status));

    return status;
}

void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    TERSE_FN(("Enter\n"));

    ::IoDeleteSymbolicLink(&gSymbolicLinkName);
    ::IoDeleteDevice(gDeviceObject);

    Cleanup();

    TERSE_FN(("Exit\n"));

    return;
}

NTSTATUS DeviceControlRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return ProcessMonitor::getInstance()->ProcessIrp(Irp);
}

NTSTATUS DeviceOpenHandleRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return utils::CompleteIrp(Irp, STATUS_SUCCESS, 0);
}

NTSTATUS DeviceCloseHandleRoutine(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return utils::CompleteIrp(Irp, STATUS_SUCCESS, 0);
}

void LoadImageNotifyRoutine(_In_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo)
{   
    // The FullImageName parameter can be NULL in cases in which the operating 
    // system is unable to obtain the full name of the image at process creation time.
    if (!FullImageName)
    {
        return;
    }

    // Handle is zero if the newly loaded image is a driver
    if (!ProcessId || ImageInfo->SystemModeImage)
    {
        return;
    }

    TERSE_FN(("%wZ\n", FullImageName));

    bool terminate = false;
    ProcessMonitor::getInstance()->OnLoadProcessImage(ProcessId, kf::USimpleString{ *FullImageName }, terminate);

    if (terminate)
    {
        utils::ScheduleProcessTerminate(ProcessId);
    }
}

void CreateProcessNotifyRoutine(_In_ HANDLE ParentId, _In_ HANDLE ProcessId,
                                _In_ BOOLEAN Create)
{
    TERSE_FN(("pid: %lu, ppid: %lu\n", HandleToULong(ProcessId), HandleToULong(ParentId)));

    if (Create)
    {
        ProcessMonitor::getInstance()->OnCreateProcess(ParentId, ProcessId);
    }
    else
    {
        ProcessMonitor::getInstance()->OnDeleteProcess(ProcessId);
    }
}

void StartupProcessManagerList()
{
    kf::vector<std::byte, PagedPool> procInfos{};
    NTSTATUS status = utils::GetSystemProcessInformation(procInfos);
    if (!NT_SUCCESS(status)) [[unlikely]]
    {
        ERR_FN(("Can't retrive system process information list. NTSTATUS: 0x%X\n", status));
        return;
    }

    PSYSTEM_PROCESS_INFORMATION info = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(procInfos.data());
    for (;;)
    {
        kf::UString<PagedPool> imageName{};
        status = utils::GetProcessImageFileName(info->UniqueProcessId, imageName);
        if (NT_SUCCESS(status)) [[likely]]
        {
            PVOID imageBase = nullptr;
            SIZE_T imageSize = 0;
            status = utils::GetMainModuleImageBase(info->UniqueProcessId, &imageBase, &imageSize);

            if (NT_SUCCESS(status)) [[likely]]
            {
                ProcessMonitor::getInstance()->OnCreateProcess(info->InheritedFromUniqueProcessId, info->UniqueProcessId);

                bool terminate = false;
                ProcessMonitor::getInstance()->OnLoadProcessImage(info->UniqueProcessId, imageName, terminate);
            }
            else [[unlikely]]
            {
                ERR_FN(("Can't get ImageBase. ProcessId: %lu. NTSTATUS: 0x%X\n", HandleToULong(info->UniqueProcessId), status));
            }
        }
        else [[unlikely]]
        {
            ERR_FN(("Can't get image path. ProcessId: %lu. NTSTATUS: 0x%X\n", HandleToULong(info->UniqueProcessId), status));
        }

        if (info->NextEntryOffset == 0) [[unlikely]]
        {
            break;
        }

        info = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(reinterpret_cast<PUCHAR>(info) + info->NextEntryOffset);
    }
}

NTSTATUS Initialize()
{
    NTSTATUS status = ProcessMonitor::Init();

    if (NT_SUCCESS(status)) [[likely]]
    {
        status = ::PsSetCreateProcessNotifyRoutine(&CreateProcessNotifyRoutine, false);
    }

    if (NT_SUCCESS(status)) [[likely]]
    {
        status = ::PsSetLoadImageNotifyRoutine(&LoadImageNotifyRoutine);
    }

    if (NT_SUCCESS(status)) [[likely]]
    {
        StartupProcessManagerList();
    }

    return status;
}

void Cleanup()
{
    ::PsRemoveLoadImageNotifyRoutine(&LoadImageNotifyRoutine);
    ::PsSetCreateProcessNotifyRoutine(&CreateProcessNotifyRoutine, true);

    ProcessMonitor::getInstance()->Cleanup();
    ProcessMonitor::Uninit();
}