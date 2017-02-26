#ifndef __TYPES_H__
#define __TYPES_H__

typedef struct _PROCESS_INFO
{
    ULONG ProcessId;
    WCHAR ImagePath[260];
} PROCESS_INFO, *PPROCESS_INFO;

typedef struct _CHECK_LIST_ENTRY
{
    PPROCESS_INFO ProcessInfo;
    BOOLEAN AddToBlacklist;
} CHECK_LIST_ENTRY, *PCHECK_LIST_ENTRY;

#endif // __TYPES_H__