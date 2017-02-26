#ifndef __PROCESS_DLL_H__
#define __PROCESS_DLL_H__

#include "ProcessMonitor.h"

#ifdef PROCESSDLL_EXPORTS
#define PROCESSDLL_EXPORTS_API __declspec(dllexport) __stdcall  
#else
#define PROCESSDLL_EXPORTS_API __declspec(dllimport) __stdcall 
#endif



#ifdef __cplusplus
extern "C" {
#endif

struct DriverStatus
{
    enum Type
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
GetStatus(__int32 * status);

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



#endif // __PROCESS_DLL_H__