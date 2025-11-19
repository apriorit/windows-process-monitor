#pragma once

#include "Common.h"
#include "Utils.h"

class ProcessMonitor
{
public:
    ProcessMonitor(std::wstring_view driverName);
    UINT Init(PPROCESSES_CALLBACK callback);
    UINT Reset();

    ~ProcessMonitor();

private:
    void MonitoringThreadProc();
    void OpenDevice();

    void Dispatch();

    void AllowSelf();

    UINT Block(PPROCESS_INFO processInfo);
    UINT Allow(PPROCESS_INFO processInfo);

    void IoControl(DWORD ioControlCode, LPVOID inBuffer, DWORD inBufferSize,
        LPVOID outBuffer, DWORD outBufferSize, LPDWORD bytesReturned = nullptr);

private:
    PPROCESSES_CALLBACK m_callback = nullptr;
    utils::Event m_notifyEvent{};
    HANDLE m_driverHandle = nullptr;
    std::wstring_view m_serviceName{};
    std::shared_ptr<std::jthread> m_monitoringThread{};
    bool m_terminateMonitoring = false;
};
