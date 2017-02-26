#ifndef __NT_DEFINITIONS_H__
#define __NT_DEFINITIONS_H__

extern "C"
{
    enum SYSTEM_INFORMATION_CLASS
    {
        SystemProcessInformation              =  5,  // SystemProcessesAndThreadsInformation
        SystemModuleInformation               = 11  
    };

    // Note: simplified structure representation
    typedef struct _SYSTEM_PROCESS_INFORMATION {
        ULONG NextEntryOffset;
        ULONG NumberOfThreads;
        UCHAR Reserved1[48];
        PVOID Reserved2[3];
        HANDLE UniqueProcessId;
        HANDLE InheritedFromUniqueProcessId;
        ULONG HandleCount;
        UCHAR Reserved4[4];
        PVOID Reserved5[11];
        SIZE_T PeakPagefileUsage;
        SIZE_T PrivatePageCount;
        LARGE_INTEGER Reserved6[6];
    } SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

    NTSTATUS
        NTAPI
        ZwQuerySystemInformation(
        __in       SYSTEM_INFORMATION_CLASS SystemInformationClass,
        __inout_bcount_opt(SystemInformationLength)    PVOID SystemInformation,
        __in       ULONG SystemInformationLength,
        __out_opt  PULONG ReturnLength
        );

    NTSTATUS
        NTAPI
        ZwQueryInformationProcess(
        __in       HANDLE ProcessHandle,
        __in       PROCESSINFOCLASS ProcessInformationClass,
        __out_bcount_opt(ProcessInformationLength)      PVOID ProcessInformation,
        __in       ULONG ProcessInformationLength,
        __out_opt  PULONG ReturnLength
        );

    PPEB
        PsGetProcessPeb (
        IN PEPROCESS Process
        );

    typedef struct _PEB_LDR_DATA {
        UCHAR Reserved1[8];
        PVOID Reserved2[3];
        LIST_ENTRY InMemoryOrderModuleList;
    } PEB_LDR_DATA, *PPEB_LDR_DATA;

    typedef struct _LDR_DATA_TABLE_ENTRY {
        PVOID Reserved1[2];
        LIST_ENTRY InMemoryOrderLinks;
        PVOID Reserved2[2];
        PVOID DllBase;
        PVOID EntryPoint; // Windows Server 2003 and Windows XP/2000:  The EntryPoint member is not supported.
        SIZE_T SizeOfImage;
        UNICODE_STRING FullDllName;
        UCHAR Reserved4[8];
        PVOID Reserved5[3];
        union {
            ULONG CheckSum;
            PVOID Reserved6;
        };
        ULONG TimeDateStamp;
    } LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

    typedef struct _RTL_USER_PROCESS_PARAMETERS {
        UCHAR          Reserved1[16];
        PVOID          Reserved2[10];
        UNICODE_STRING ImagePathName;
        UNICODE_STRING CommandLine;
    } RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

#ifndef _WIN64
    typedef struct _PEB {
        UCHAR Reserved1[2];
        UCHAR BeingDebugged;
        UCHAR Reserved2[1];
        PVOID Reserved3[1];
        PVOID ImageBaseAddress;
        PPEB_LDR_DATA Ldr;
        PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
        UCHAR Reserved4[104];
        PVOID Reserved5[52];
        PVOID PostProcessInitRoutine;
        UCHAR Reserved6[128];
        PVOID Reserved7[1];
        ULONG SessionId;
    } PEB, *PPEB;
#else
    typedef struct _PEB {
        UCHAR Reserved1[2];
        UCHAR BeingDebugged;
        UCHAR Reserved2[13];
        PVOID ImageBaseAddress;
        PPEB_LDR_DATA Ldr;
        PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
        UCHAR Reserved3[520];
        PVOID PostProcessInitRoutine;
        UCHAR Reserved4[136];
        ULONG SessionId;
    } PEB, *PPEB;
#endif

} // extern "C"

#endif //__NT_DEFINITIONS_H__