#pragma once

#include "Types.h"

#define PROCMON_DRIVER_NAME L"ProcessMonitor"

inline constexpr std::wstring_view kProcmonDriverServiceName = PROCMON_DRIVER_NAME;
inline constexpr std::wstring_view kProcmonDriverBinaryName  = PROCMON_DRIVER_NAME L".sys";

#undef PROCMON_DRIVER_NAME

using PPROCESSES_CALLBACK = void(CALLBACK *)(PCHECK_LIST_ENTRY checkList[], ULONG checkListCount);

struct ProcessInfo
{
    unsigned int pid{};
    CString imageName{};
};
using RunningProcesses = std::vector<ProcessInfo>;

struct CheckProcessInfo
{
    ProcessInfo process{};
    bool isDeny = false;
};
using CheckedProcesses = std::vector<CheckProcessInfo>;