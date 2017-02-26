#include "ProcessMonitor.h"
#include "controls.h"
#include <exception>

ProcessMonitor::ProcessMonitor(std::wstring driver_name)
:   m_callback(NULL),
    m_hDriverHandle(INVALID_HANDLE_VALUE),
    m_wsServiceName(driver_name),
    m_TerminateMonitoring(false)
{
}

ProcessMonitor::~ProcessMonitor(void)
{
    Reset();
}

UINT ProcessMonitor::Init(PPROCESSES_CALLBACK callback)
{
    m_TerminateMonitoring = false;
    OpenDevice();

	AllowSelf();
    
    m_callback = callback;

    m_NotifyEvent.Reset();
    HANDLE hEvent = m_NotifyEvent.GetHandle();
    IoControl(
        IOCTL_REGISTER_EVENT,
        &hEvent,
        sizeof(hEvent),
        NULL,
        0);

	m_hMonitoringThread = boost::shared_ptr<boost::thread>(
		new boost::thread(boost::bind(&ProcessMonitor::MonitoringThreadProc, this)));

    return ERROR_SUCCESS;
}

void ProcessMonitor::MonitoringThreadProc()
{
    do
    {
        if (m_NotifyEvent.Wait() && !m_TerminateMonitoring)
            Dispatch();
    }
    while (!m_TerminateMonitoring);
}

void ProcessMonitor::Dispatch()
{
    DWORD dispatchCount = 0;
    IoControl(
        IOCTL_GET_PROCESS_COUNT,
        NULL,
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
            NULL,
            0,
            &gray[0],
            static_cast<DWORD>(gray.size() * sizeof(gray[0])),
            &bytesReturned);

        // Adjust items count to actualy returned entries
        dispatchCount = bytesReturned/sizeof(gray[0]);

        std::vector<CHECK_LIST_ENTRY> checkList(dispatchCount);
        for (unsigned i = 0; i < checkList.size(); i++)
        {
            checkList[i].ProcessInfo = &gray[i];
            checkList[i].AddToBlacklist = false;
        }

        // Callback procedure expect array of pointers, prepare it.
        std::vector<PCHECK_LIST_ENTRY> ptrs(dispatchCount);
        for (unsigned i = 0; i < ptrs.size(); i++)
            ptrs[i] = &checkList[i];

        // Execute callback.
        try 
        {
            if (m_callback)
                m_callback(&ptrs[0], static_cast<ULONG>(ptrs.size()));
        }
        catch (...)
        {
        }

        for (unsigned i = 0; i < checkList.size(); i++)
        {
            if (wcslen(checkList[i].ProcessInfo->ImagePath) == 0)
                throw std::exception("Internal error. Process info don't have ImagePath");

            if (checkList[i].AddToBlacklist)
                Block(checkList[i].ProcessInfo);
            else
                Allow(checkList[i].ProcessInfo);
        }

        IoControl(
            IOCTL_TERMINATE_ALL,
            NULL,
            0,
            NULL,
            0);
    }
    else
    {
		m_NotifyEvent.Reset();
    }
}

UINT ProcessMonitor::Allow(PPROCESS_INFO ProcessInfo)
{
    try
    {
        IoControl(
            IOCTL_ALLOW,
            ProcessInfo,
            sizeof(ProcessInfo),
            NULL,
            0);
    }
    catch (const std::runtime_error &ex)
    {
        return GetLastError();

		ex;
    }
    return 0;
}

UINT ProcessMonitor::Block(PPROCESS_INFO ProcessInfo)
{
    try
    {
        IoControl(
            IOCTL_BLOCK,
            ProcessInfo,
            sizeof(ProcessInfo),
            NULL,
            0);
    }
    catch (const std::runtime_error &ex)
    {
        DWORD lastError = GetLastError();

        return lastError;

		ex;
    }

    return 0;
}


UINT ProcessMonitor::Reset()
{
    m_TerminateMonitoring = true;
    m_NotifyEvent.Set();
    if (m_hMonitoringThread)
        m_hMonitoringThread->join();

	if (m_hDriverHandle != INVALID_HANDLE_VALUE)
	{
		IoControl(
			IOCTL_RESET,
			NULL,
			0,
			NULL,
			0);
		CloseHandle(m_hDriverHandle);
		m_hDriverHandle = INVALID_HANDLE_VALUE;
	}

    return ERROR_SUCCESS;
}

void ProcessMonitor::OpenDevice()
{
	if (m_hDriverHandle != INVALID_HANDLE_VALUE)
		return;

    m_hDriverHandle = CreateFile(
        (L"\\\\.\\" + m_wsServiceName).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL);

    if (m_hDriverHandle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Can't open driver device");
}

void ProcessMonitor::IoControl(DWORD IoControlCode, LPVOID InBuffer, DWORD InBufferSize,
                               LPVOID OutBuffer, DWORD OutBufferSize, LPDWORD BytesReturned)
{
    DWORD tmp;
    if (!DeviceIoControl(
        m_hDriverHandle,
        IoControlCode,
        InBuffer,
        InBufferSize,
        OutBuffer,
        OutBufferSize,
        BytesReturned != NULL ? BytesReturned : &tmp,
        NULL))
    {        
        throw std::runtime_error("DeviceIoControl() failed");
    }
}

void ProcessMonitor::AllowSelf()
{
	PROCESS_INFO info;
	SecureZeroMemory(&info, sizeof(info));
	info.ProcessId = GetCurrentProcessId();
	std::wstring exePath = utils::GetModuleFileNameEx();
	::memcpy(&info.ImagePath, exePath.c_str(), exePath.size() * 2);
	Allow(&info);
}