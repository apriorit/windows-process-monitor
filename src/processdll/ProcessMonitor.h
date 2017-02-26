#ifndef __PROCESS_MONITOR_H__
#define __PROCESS_MONITOR_H__

#include <string>
#include "common.h"
#include "utils.h"
#include <boost/thread.hpp>

class ProcessMonitor
{
    PPROCESSES_CALLBACK m_callback;
    utils::Event m_NotifyEvent;
    HANDLE m_hDriverHandle;
    std::wstring m_wsServiceName;
    boost::shared_ptr<boost::thread> m_hMonitoringThread;
    bool m_TerminateMonitoring;

    void MonitoringThreadProc();
    void OpenDevice();

    void Dispatch();

	void AllowSelf();

    UINT Block(PPROCESS_INFO ProcessInfo);
    UINT Allow(PPROCESS_INFO ProcessInfo);

    void IoControl(DWORD IoControlCode, LPVOID InBuffer, DWORD InBufferSize,
        LPVOID OutBuffer, DWORD OutBufferSize, LPDWORD BytesReturned = NULL);

public:
    ProcessMonitor(std::wstring driver_name);
    UINT Init(PPROCESSES_CALLBACK callback);
    UINT Reset();

    virtual ~ProcessMonitor(void);
};

#endif // __PROCESS_MONITOR_H__