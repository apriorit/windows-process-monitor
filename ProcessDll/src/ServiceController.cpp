#include "ServiceController.h"
#include "Utils.h"

void ServiceController::Install(std::wstring_view serviceName, std::wstring_view sourceBinaryPath,
                                DWORD serviceType, DWORD startType, std::wstring_view dependencies)
{
    std::wstring path = utils::GetDriverPath(serviceName);

    if (serviceType == SERVICE_KERNEL_DRIVER && !::CopyFileW(sourceBinaryPath.data(), path.c_str(), false))
    {
        throw std::runtime_error("Can't copy file");
    }

    SC_HANDLE serviceControlManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!serviceControlManager)
    {
        throw std::runtime_error("Can't open SCM");
    }

    utils::ScopedServiceHandle scManagerHandleGuard(serviceControlManager);

    std::wstring displayName = std::format(L"{} Driver", serviceName);

    SC_HANDLE service = ::CreateServiceW(
        serviceControlManager,
        serviceName.data(),
        displayName.c_str(),
        SERVICE_ALL_ACCESS,
        serviceType,
        startType,
        SERVICE_ERROR_NORMAL,
        path.c_str(),
        nullptr,
        nullptr,
        dependencies.data(),
        nullptr,
        nullptr);

    if (!service)
    {
        throw std::runtime_error("Can't create service");
    }

    ::CloseServiceHandle(service);
}

void ServiceController::Remove(std::wstring_view serviceName)
{
    SC_HANDLE serviceControlManager = ::OpenSCManagerW(nullptr, nullptr, GENERIC_WRITE);
    if (!serviceControlManager)
    {
        throw std::runtime_error("Can't open SCM");
    }

    utils::ScopedServiceHandle scManagerHandleGuard(serviceControlManager);

    SC_HANDLE service = ::OpenServiceW(serviceControlManager, serviceName.data(), DELETE);
    if (!service)
    {
        throw std::runtime_error("Can't open service");
    }

    utils::ScopedServiceHandle serviceHandleGuard(service);

    if (!::DeleteService(service) && ::GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE)
    {
        throw std::runtime_error("Can't delete service");
    }

    std::wstring path = utils::GetDriverPath(serviceName);
    ::DeleteFileW(path.c_str());
}

void ServiceController::Start(std::wstring_view serviceName)
{
    SC_HANDLE serviceControlManager = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!serviceControlManager)
    {
        throw std::runtime_error("Can't open SCM");
    }

    utils::ScopedServiceHandle scManagerHandleGuard(serviceControlManager);

    SC_HANDLE service = ::OpenServiceW(serviceControlManager, serviceName.data(), SERVICE_START);
    if (!service)
    {
        throw std::runtime_error("Can't open service");
    }

    utils::ScopedServiceHandle serviceHandleGuard(service);
    if (!::StartServiceW(service, 0, nullptr))
    {
        throw std::runtime_error("Can't start service");
    }
}

void ServiceController::Stop(std::wstring_view serviceName)
{
    SC_HANDLE serviceControlManager = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!serviceControlManager)
    {
        throw std::runtime_error("Can't open SCM");
    }

    utils::ScopedServiceHandle scManagerHandleGuard(serviceControlManager);

    SC_HANDLE service = ::OpenServiceW(serviceControlManager, serviceName.data(), SERVICE_STOP);
    if (!service)
    {
        throw std::runtime_error("Can't open service");
    }

    utils::ScopedServiceHandle serviceHandleGuard(service);

    SERVICE_STATUS serviceStatus;
    if (!::ControlService(service, SERVICE_CONTROL_STOP, &serviceStatus) && ::GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
    {
        throw std::runtime_error("Can't stop service");
    }
}

void ServiceController::QueryStatus(std::wstring_view serviceName, bool& installed, bool& running)
{
    installed = false;
    running = false;

    SC_HANDLE serviceControlManager = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!serviceControlManager)
    {
        throw std::runtime_error("Can't open SCM");
    }

    utils::ScopedServiceHandle scManagerHandleGuard(serviceControlManager);

    SC_HANDLE service = ::OpenServiceW(serviceControlManager, serviceName.data(), SERVICE_QUERY_STATUS);
    installed = !!service;
    if (!installed)
    {
        return;
    }

    utils::ScopedServiceHandle serviceHandleGuard(service);

    SERVICE_STATUS serviceStatus{};
    if (!::QueryServiceStatus(service, &serviceStatus))
    {
        throw std::runtime_error("Can't query service config");
    }

    running = serviceStatus.dwCurrentState == SERVICE_RUNNING;
}
