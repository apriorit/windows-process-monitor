#include "processdll.h"
#include "ServiceController.h"

ProcessMonitor gProcessMonitor(L"procmon");

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
{
    try
    {
        bool installed = false;
        bool running = false;
        ServiceController::QueryStatus(SERVICE_NAME, installed, running);
        if (installed)
            return ERROR_SUCCESS;
        if (!installed)
        {
            std::wstring fullPath = utils::GetModulePath() + L"i386\\procmon.sys";
            ServiceController::Install(SERVICE_NAME, fullPath, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START);
        }
        return ERROR_SUCCESS;
    }
    catch(const std::exception& )
    {
        return ERROR_SERVICE_NOT_FOUND;
    }
}

UINT
PROCESSDLL_EXPORTS_API 
GetStatus(__int32 * status)
{
    try
    {
        bool installed = false;
        bool running = false;
        ServiceController::QueryStatus(SERVICE_NAME, installed, running);
        if (installed && running)
            *status = DriverStatus::Running;
        if (installed && !running)
            *status = DriverStatus::Installed;
        return ERROR_SUCCESS;
    }
    catch( const std::exception& )
    {
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }
}

UINT
PROCESSDLL_EXPORTS_API 
Start()
{
    try
    {
        bool installed = false;
        bool running = false;
        ServiceController::QueryStatus(SERVICE_NAME, installed, running);
        if (installed && running)
            return ERROR_SUCCESS;
        if (!installed)
        {
            return ERROR_SERVICE_NOT_FOUND;
        }
        if (!running)
        {
            ServiceController::Start(SERVICE_NAME);
        }
        return ERROR_SUCCESS;
    }
    catch(const std::exception& )
    {
        return ERROR_SERVICE_NOT_FOUND;
    }
}

UINT
PROCESSDLL_EXPORTS_API 
Stop()
{
    try
    {
        bool installed = false;
        bool running = false;
        ServiceController::QueryStatus(SERVICE_NAME, installed, running);
        if (!installed)
        {
            return ERROR_SERVICE_NOT_FOUND;
        }
        if (!running)
        {
            return ERROR_SERVICE_NOT_ACTIVE;
        }
        
        ServiceController::Stop(SERVICE_NAME);
        
        return ERROR_SUCCESS;
    }
    catch(const std::exception& )
    {
        return ERROR_SERVICE_NOT_FOUND;
    }
}

UINT
PROCESSDLL_EXPORTS_API 
Uninstall()
{
    try
    {
        bool installed = false;
        bool running = false;
        ServiceController::QueryStatus(SERVICE_NAME, installed, running);
        
        if (!installed)
        {
            return ERROR_SERVICE_NOT_FOUND;
        }
        
        if (running)
        {
            return ERROR_SERVICE_EXISTS;
        }

        ServiceController::Remove(SERVICE_NAME);

        return ERROR_SUCCESS;
    }
    catch(const std::exception& )
    {
        return ERROR_SERVICE_NOT_FOUND;
    }
}
