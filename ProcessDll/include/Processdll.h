#pragma once

#include "ProcessMonitor.h"

#ifdef ProcessDll_EXPORTS
#define PROCESSDLL_EXPORTS_API __declspec(dllexport) __stdcall
#else
#define PROCESSDLL_EXPORTS_API __declspec(dllimport) __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct DriverStatus
{
    enum class Type : int8_t
    {
        NotInstalled = 0,
        Installed = 1,
        Running = 2
    };
};

UINT
PROCESSDLL_EXPORTS_API
Init(PPROCESSES_CALLBACK callback);

UINT
PROCESSDLL_EXPORTS_API
Deinit();

UINT
PROCESSDLL_EXPORTS_API
Install();

UINT
PROCESSDLL_EXPORTS_API
GetStatus(int32_t* status);

UINT
PROCESSDLL_EXPORTS_API
Start();

UINT
PROCESSDLL_EXPORTS_API
Stop();

UINT
PROCESSDLL_EXPORTS_API
Uninstall();

#ifdef __cplusplus
}
#endif
