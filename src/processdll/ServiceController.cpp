#include "ServiceController.h"
#include "utils.h"

void ServiceController::Install(const std::wstring& ServiceName, const std::wstring& SourceBinaryPath, 
                                DWORD ServiceType, DWORD StartType, LPCTSTR lpDependencies)
{
    std::wstring path = utils::GetWindowsDir() + L"\\System32\\Drivers\\" + ServiceName + L".sys";

    if (ServiceType == SERVICE_KERNEL_DRIVER && !CopyFile(SourceBinaryPath.c_str(), path.c_str(), FALSE))
        throw std::runtime_error("Can't copy file");

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager == NULL)
        throw std::runtime_error("Can't open SCM");
    utils::ScopedServiceHandle guard(hSCManager);

    std::wstring displayName = ServiceName + L" Driver";

    SC_HANDLE hService = CreateService(
        hSCManager,
        ServiceName.c_str(),
        displayName.c_str(),
        SERVICE_ALL_ACCESS,
        ServiceType,
        StartType,
        SERVICE_ERROR_NORMAL,
        path.c_str(),
        NULL, NULL, lpDependencies, NULL, NULL);

    if (hService == NULL)
        throw std::runtime_error("Can't create service");

    CloseServiceHandle(hService);
}

void ServiceController::Remove(const std::wstring& ServiceName)
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, GENERIC_WRITE);
    if (hSCManager == NULL)
        throw std::runtime_error("Can't open SCM");
    utils::ScopedServiceHandle guard1(hSCManager);   

    SC_HANDLE hService = OpenService(hSCManager, ServiceName.c_str(), DELETE);
    if (hService == NULL)
        throw std::runtime_error("Can't open service");
    utils::ScopedServiceHandle guard2(hService);

    if (DeleteService(hService) == FALSE && GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE)
        throw std::runtime_error("Can't delete service");

    std::wstring path =  utils::GetWindowsDir() + L"\\System32\\Drivers\\" + ServiceName + L".sys";
    DeleteFile(path.c_str());
}

void ServiceController::Start(const std::wstring& ServiceName)
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
        throw std::runtime_error("Can't open SCM");
    utils::ScopedServiceHandle guard1(hSCManager);

    SC_HANDLE hService = OpenService(hSCManager, ServiceName.c_str(), SERVICE_START);
    if (hService == NULL) 
        throw std::runtime_error("Can't open service");
    utils::ScopedServiceHandle guard2(hService);

    if (StartService(hService, 0, NULL) == FALSE)
        throw std::runtime_error("Can't start service");
}

void ServiceController::Stop(const std::wstring& ServiceName)
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
        throw std::runtime_error("Can't open SCM");
    utils::ScopedServiceHandle guard1(hSCManager);

    SC_HANDLE hService = OpenService(hSCManager, ServiceName.c_str(), SERVICE_STOP);
    if (hService == NULL)
        throw std::runtime_error("Can't open service");
    utils::ScopedServiceHandle guard2(hService);

    SERVICE_STATUS serviceStatus;
    if (ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus) == FALSE )
        if (GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
            throw std::runtime_error("Can't stop service");
}

void ServiceController::QueryStatus(const std::wstring& ServiceName, bool& Installed, bool& Running)
{
    Installed = false;
    Running = false;

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager == NULL)
        throw std::runtime_error("Can't open SCM");
    utils::ScopedServiceHandle guard1(hSCManager);

    SC_HANDLE hService = OpenService(hSCManager, ServiceName.c_str(), SERVICE_QUERY_STATUS);
    Installed = hService != NULL;
    if (!Installed)
        return;
    utils::ScopedServiceHandle guard2(hService);

    SERVICE_STATUS serviceStatus;
    if (QueryServiceStatus(hService, &serviceStatus) == FALSE)
        throw std::runtime_error("Can't query service config");
    Running = serviceStatus.dwCurrentState == SERVICE_RUNNING;
}
