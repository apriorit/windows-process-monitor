#include "ProcessMonitor.h"
#include "Controls.h"

ProcessMonitor::ProcessMonitor(std::wstring_view driverName)
:   m_callback(nullptr),
    m_driverHandle(INVALID_HANDLE_VALUE),
    m_serviceName(driverName),
    m_terminateMonitoring(false)
{
}

ProcessMonitor::~ProcessMonitor()
{
    Reset();
}

UINT ProcessMonitor::Init(PPROCESSES_CALLBACK callback)
{
    m_terminateMonitoring = false;

    OpenDevice();

    AllowSelf();

    m_callback = callback;

    m_notifyEvent.Reset();

    HANDLE event = m_notifyEvent.GetHandle();
    IoControl(
        IOCTL_REGISTER_EVENT,
        &event,
        sizeof(event),
        nullptr,
        0);

    m_monitoringThread = std::make_shared<std::jthread>(std::bind(&ProcessMonitor::MonitoringThreadProc, this));

    return ERROR_SUCCESS;
}

void ProcessMonitor::MonitoringThreadProc()
{
    do
    {
        if (m_notifyEvent.Wait() && !m_terminateMonitoring)
        {
            Dispatch();
        }
    }
    while (!m_terminateMonitoring);
}

void ProcessMonitor::Dispatch()
{
    DWORD dispatchCount = 0;
    IoControl(
        IOCTL_GET_PROCESS_COUNT,
        nullptr,
        0,
        &dispatchCount,
        sizeof(dispatchCount));

    if (dispatchCount > 0)
    {
        DWORD bytesReturned = 0;
        // Get gray processes information.
        std::vector<PROCESS_INFO> gray(dispatchCount);
        IoControl(
            IOCTL_GET_PROCESS_LIST,
            nullptr,
            0,
            gray.data(),
            static_cast<DWORD>(gray.size() * sizeof(gray[0])),
            &bytesReturned);

        auto grayChecked = gray | std::views::filter([](auto& procInfo) { return wcslen(procInfo.ImagePath) != 0; });

        if (grayChecked.empty())
        {
            return;
        }

        std::vector<CHECK_LIST_ENTRY> checkList{};
        for (auto it = grayChecked.begin(); it != grayChecked.end(); ++it)
        {
            checkList.push_back(CHECK_LIST_ENTRY{ &*it, false });
        }

        // Callback procedure expect array of pointers, prepare it.
        std::vector<PCHECK_LIST_ENTRY> ptrs(checkList.size());
        for (unsigned i = 0; i < ptrs.size(); ++i)
        {
            ptrs[i] = &checkList[i];
        }

        // Execute callback.
        try
        {
            if (m_callback)
            {
                m_callback(ptrs.data(), static_cast<ULONG>(ptrs.size()));
            }
        }
        catch (...)
        {
            return;
        }

        for (size_t i = 0; i < checkList.size(); ++i)
        {
            if (wcslen(checkList[i].ProcessInfo->ImagePath) == 0)
            {
                throw std::runtime_error("Internal error. Process info don't have ImagePath");
            }

            if (checkList[i].AddToBlacklist)
            {
                Block(checkList[i].ProcessInfo);
            }
            else
            {
                Allow(checkList[i].ProcessInfo);
            }
        }

        IoControl(
            IOCTL_TERMINATE_ALL,
            nullptr,
            0,
            nullptr,
            0);
    }
    else
    {
        m_notifyEvent.Reset();
    }
}

UINT ProcessMonitor::Allow(PPROCESS_INFO ProcessInfo)
try
{
    IoControl(
        IOCTL_ALLOW,
        ProcessInfo,
        sizeof(ProcessInfo),
        nullptr,
        0);

    return 0;
}
catch (const std::exception&)
{
    return ::GetLastError();
}

UINT ProcessMonitor::Block(PPROCESS_INFO processInfo)
try
{
    IoControl(
        IOCTL_BLOCK,
        processInfo,
        sizeof(processInfo),
        nullptr,
        0);

    return ERROR_SUCCESS;
}
catch (const std::exception&)
{
    return ::GetLastError();
}

UINT ProcessMonitor::Reset()
{
    m_terminateMonitoring = true;
    m_notifyEvent.Set();

    m_monitoringThread.reset();

    if (m_driverHandle != INVALID_HANDLE_VALUE)
    {
        IoControl(
            IOCTL_RESET,
            nullptr,
            0,
            nullptr,
            0);

        ::CloseHandle(m_driverHandle);
        m_driverHandle = INVALID_HANDLE_VALUE;
    }

    return ERROR_SUCCESS;
}

void ProcessMonitor::OpenDevice()
{
    if (m_driverHandle != INVALID_HANDLE_VALUE)
    {
        return;
    }

    m_driverHandle = ::CreateFileW(
        std::format(L"\\\\.\\{}", m_serviceName).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (m_driverHandle == INVALID_HANDLE_VALUE)
    {
        throw std::runtime_error("Can't open driver device");
    }
}

void ProcessMonitor::IoControl(DWORD ioControlCode, LPVOID inBuffer, DWORD inBufferSize,
                               LPVOID outBuffer, DWORD outBufferSize, LPDWORD bytesReturned)
{
    DWORD tmp = 0;

    if (!::DeviceIoControl(
        m_driverHandle,
        ioControlCode,
        inBuffer,
        inBufferSize,
        outBuffer,
        outBufferSize,
        bytesReturned ? bytesReturned : &tmp,
        nullptr))
    {        
        throw std::runtime_error("DeviceIoControl() failed");
    }
}

void ProcessMonitor::AllowSelf()
{
    PROCESS_INFO info{};
    SecureZeroMemory(&info, sizeof(info));
    info.ProcessId = ::GetCurrentProcessId();

    std::wstring exePath = utils::GetModuleFileNameEx();
    ::memcpy(&info.ImagePath, exePath.c_str(), exePath.size() * 2);

    Allow(&info);
}