#include "Processdll.h"
#include "ServiceController.h"

ProcessMonitor gProcessMonitor(kProcmonDriverServiceName);

UINT
PROCESSDLL_EXPORTS_API
Init(PPROCESSES_CALLBACK callback)
{
    return gProcessMonitor.Init(callback);
}

UINT
PROCESSDLL_EXPORTS_API
Deinit()
{
    return gProcessMonitor.Reset();
}

UINT
PROCESSDLL_EXPORTS_API
Install()
try
{
    bool installed = false;
    bool running = false;
    ServiceController::QueryStatus(kProcmonDriverServiceName, installed, running);

    if (installed)
    {
        return ERROR_SUCCESS;
    }

    if (!installed)
    {
        std::wstring fullPath = std::format(L"{}{}", utils::GetModulePath(), kProcmonDriverBinaryName);
        ServiceController::Install(kProcmonDriverServiceName, fullPath, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START);
    }

    return ERROR_SUCCESS;
}
catch (const std::exception&)
{
    return ERROR_SERVICE_NOT_FOUND;
}

UINT
PROCESSDLL_EXPORTS_API
GetStatus(int32_t* status)
try
{
    bool installed = false;
    bool running = false;
    ServiceController::QueryStatus(kProcmonDriverServiceName, installed, running);

    if (installed && running)
    {
        *status = static_cast<int32_t>(DriverStatus::Type::Running);
    }

    if (installed && !running)
    {
        *status = static_cast<int32_t>(DriverStatus::Type::Installed);
    }

    return ERROR_SUCCESS;
}
catch (const std::exception&)
{
    return ERROR_SERVICE_DOES_NOT_EXIST;
}

UINT
PROCESSDLL_EXPORTS_API
Start()
try
{
    bool installed = false;
    bool running = false;
    ServiceController::QueryStatus(kProcmonDriverServiceName, installed, running);

    if (installed && running)
    {
        return ERROR_SUCCESS;
    }

    if (!installed)
    {
        return ERROR_SERVICE_NOT_FOUND;
    }

    if (!running)
    {
        ServiceController::Start(kProcmonDriverServiceName);
    }

    return ERROR_SUCCESS;
}
catch (const std::exception&)
{
    return ERROR_SERVICE_NOT_FOUND;
}

UINT
PROCESSDLL_EXPORTS_API
Stop()
try
{
    bool installed = false;
    bool running = false;
    ServiceController::QueryStatus(kProcmonDriverServiceName, installed, running);

    if (!installed)
    {
        return ERROR_SERVICE_NOT_FOUND;
    }

    if (!running)
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }
        
    ServiceController::Stop(kProcmonDriverServiceName);
        
    return ERROR_SUCCESS;
}
catch (const std::exception&)
{
    return ERROR_SERVICE_NOT_FOUND;
}

UINT
PROCESSDLL_EXPORTS_API
Uninstall()
try
{
    bool installed = false;
    bool running = false;
    ServiceController::QueryStatus(kProcmonDriverServiceName, installed, running);
        
    if (!installed)
    {
        return ERROR_SERVICE_NOT_FOUND;
    }
        
    if (running)
    {
        return ERROR_SERVICE_EXISTS;
    }

    ServiceController::Remove(kProcmonDriverServiceName);

    return ERROR_SUCCESS;
}
catch (const std::exception&)
{
    return ERROR_SERVICE_NOT_FOUND;
}
