#pragma once

typedef struct _PROCESS_INFO
{
    ULONG ProcessId{};
    WCHAR ImagePath[260]{};
} PROCESS_INFO, *PPROCESS_INFO;

typedef struct _CHECK_LIST_ENTRY
{
    PPROCESS_INFO ProcessInfo = nullptr;
    BOOLEAN AddToBlacklist = false;
} CHECK_LIST_ENTRY, *PCHECK_LIST_ENTRY;
