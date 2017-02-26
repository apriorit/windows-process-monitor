#include "ProcessMonitor.h"

UNICODE_STRING gDeviceName = RTL_CONSTANT_STRING(L"\\DosDevices\\PROCMON");
UNICODE_STRING gSymbolicLinkName = RTL_CONSTANT_STRING(L"\\Device\\PROCMON");

PDEVICE_OBJECT gDeviceObject = NULL;
ProcessMonitor * gProcessMonitor = 0;

extern "C" 
DRIVER_INITIALIZE DriverEntry;

DRIVER_UNLOAD DriverUnload;
__drv_dispatchType(IRP_MJ_CREATE) DRIVER_DISPATCH DeviceOpenHandleRoutine;
__drv_dispatchType(IRP_MJ_CLOSE) DRIVER_DISPATCH DeviceCloseHandleRoutine;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL) DRIVER_DISPATCH DeviceControlRoutine;

NTSTATUS Initialize();
VOID     Cleanup();

NTSTATUS DriverEntry(IN PDRIVER_OBJECT  DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status;

    TERSE_FN(("Enter\n"));

    status = libcpp_init();
    if (status != STATUS_SUCCESS)
        return status;

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = DeviceOpenHandleRoutine;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DeviceCloseHandleRoutine;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControlRoutine;

    status = IoCreateDevice(DriverObject,       // pointer on DriverObject
        0,                                      // additional size of memory, for device extension
        &gDeviceName,                           // pointer to UNICODE_STRING
        FILE_DEVICE_UNKNOWN,                    // Device type
        0,                                      // Device characteristic
        FALSE,                                  // "Exclusive" device
        &gDeviceObject);                        // pointer do device object

    if (status == STATUS_SUCCESS)
        status = IoCreateSymbolicLink(&gSymbolicLinkName, &gDeviceName);

    if (status == STATUS_SUCCESS)
        status = Initialize();

    if (status != STATUS_SUCCESS)
    {
        // Its safe to call, even if Initialize() failed
        Cleanup();

        libcpp_exit();       
    }
    TERSE_FN(("Exit 0x%X\n", status));
    return status;
}

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{    
    UNREFERENCED_PARAMETER(DriverObject);

    TERSE_FN(("Enter\n"));

    IoDeleteSymbolicLink(&gSymbolicLinkName);
    IoDeleteDevice(gDeviceObject);

    Cleanup();

    libcpp_exit();

    TERSE_FN(("Exit\n"));
    return;
}

NTSTATUS DeviceControlRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return gProcessMonitor->ProcessIrp(Irp);
}

NTSTATUS DeviceOpenHandleRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return utils::CompleteIrp(Irp, STATUS_SUCCESS, 0);
}

NTSTATUS DeviceCloseHandleRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return utils::CompleteIrp(Irp, STATUS_SUCCESS, 0);
}

void LoadImageNotifyRoutine(IN PUNICODE_STRING FullImageName,
                            IN HANDLE ProcessId, IN PIMAGE_INFO ImageInfo)
{   
    // The FullImageName parameter can be NULL in cases in which the operating 
    // system is unable to obtain the full name of the image at process creation time.
    if (!FullImageName)
        return;

    // Handle is zero if the newly loaded image is a driver
    if (!ProcessId || ImageInfo->SystemModeImage)
        return;

    TERSE_FN(("%wZ\n", FullImageName));
    BOOLEAN Terminate = FALSE;
    gProcessMonitor->OnLoadProcessImage(ProcessId,
        FullImageName, ImageInfo->ImageBase, ImageInfo->ImageSize, &Terminate);

    if (Terminate)
        utils::ScheduleProcessTerminate(ProcessId); 
}

void CreateProcessNotifyRoutine(IN HANDLE ParentId, IN HANDLE ProcessId,
                                IN BOOLEAN Create)
{
    if (Create)
        gProcessMonitor->OnCreateProcess(ParentId, ProcessId);
    else
        gProcessMonitor->OnDeleteProcess(ProcessId);    
}

void StartupProcessManagerList()
{
    PSYSTEM_PROCESS_INFORMATION procInfoBuf = NULL;
    NTSTATUS status = utils::GetSystemProcessInformation(&procInfoBuf);
    if (NT_SUCCESS(status))
    {
        PSYSTEM_PROCESS_INFORMATION info = procInfoBuf;
        for (;;)
        {
            PUNICODE_STRING imageName = NULL;
            status = utils::GetProcessImageFileName(info->UniqueProcessId, &imageName);
            if (NT_SUCCESS(status))
            {
                PVOID ImageBase = NULL;
                SIZE_T ImageSize = 0;
                status = utils::GetMainModuleImageBase(info->UniqueProcessId, &ImageBase, &ImageSize);

                if (NT_SUCCESS(status))
                {
                    gProcessMonitor->OnCreateProcess(info->InheritedFromUniqueProcessId, 
                        info->UniqueProcessId);

                    BOOLEAN Terminate = FALSE;
                    gProcessMonitor->OnLoadProcessImage(info->UniqueProcessId,
                        imageName, ImageBase, ImageSize, &Terminate);
                }
                else
                {
                    ERR_FN(("Can't get ImageBase. ProcessId: %d. NTSTATUS: 0x%X\n", info->UniqueProcessId, status));
                }

                ExFreePool(imageName);
            }
            else
            {
                ERR_FN(("Can't get image path. ProcessId: %d. NTSTATUS: 0x%X\n", info->UniqueProcessId, status));
            }

            if (info->NextEntryOffset == 0)
                break;

            info = (PSYSTEM_PROCESS_INFORMATION)(((PUCHAR)info) + info->NextEntryOffset);
        }
        ExFreePool(procInfoBuf);
    }
    else
    {
        ERR_FN(("Can't retrive system process information list. NTSTATUS: 0x%X\n", status));
    }
}

NTSTATUS Initialize()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    gProcessMonitor = new ProcessMonitor();
    if (gProcessMonitor == 0)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = PsSetCreateProcessNotifyRoutine(&CreateProcessNotifyRoutine, FALSE);

    if (status == STATUS_SUCCESS)
        status = PsSetLoadImageNotifyRoutine(&LoadImageNotifyRoutine);

    if (status == STATUS_SUCCESS)
    {
		StartupProcessManagerList();
    }
    return status;
}

VOID Cleanup()
{
    PsRemoveLoadImageNotifyRoutine(&LoadImageNotifyRoutine);
    PsSetCreateProcessNotifyRoutine(&CreateProcessNotifyRoutine, TRUE);

    if (gProcessMonitor)
    {
        gProcessMonitor->Cleanup();
        delete gProcessMonitor;
        gProcessMonitor = 0;
    }
}