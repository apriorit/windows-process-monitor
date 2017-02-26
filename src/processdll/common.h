#ifndef __COMMON_H__
#define __COMMON_H__

#include <Windows.h>
#include "types.h"
#include <vector>
#include <atlstr.h>

#define SERVICE_NAME L"ProcessMonitor"

typedef
VOID
(CALLBACK *PPROCESSES_CALLBACK)(
    PCHECK_LIST_ENTRY CheckList[],
    ULONG CheckListCount
    );

struct ProcessInfo
{
	unsigned int pid;
	CString imageName;
};

typedef std::vector<ProcessInfo> RunningProcesses;

struct CheckProcessInfo
{
	ProcessInfo process;
	bool isDeny;
};

typedef std::vector<CheckProcessInfo> CheckedProcesses;

#endif // __COMMON_H__