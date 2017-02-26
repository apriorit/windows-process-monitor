#include <ntifs.h>
#include "ptd.h"

typedef struct _PREALLOCATED_POOL 
{
    LIST_ENTRY    FreeEntriesHead;
    KSPIN_LOCK    Lock;
    ULONG    EntrySize;
    ULONG    EntriesCount;
    PVOID    Buffer;
} PREALLOCATED_POOL, *PPREALLOCATED_POOL;

ULONG    PoolSizes[] = {64, 512, 1024};
#define POOLS_COUNT (sizeof(PoolSizes) / sizeof(PoolSizes[0]))
static PREALLOCATED_POOL Pools[POOLS_COUNT];


#ifdef _M_AMD64

void jumpToCatchCode_stub(long long x);
void __CxxCallCatchBlock();
void _CxxThrowException();
void jumpToCatchCode();
void CPPLIB_Throw();

long long CPPLIB_DisableKernelDefence();
void CPPLIB_EnableKernelDefence(long long lOldDef);




static unsigned char g_mask[] = {0x41, 0x5F, 0x41, 0x5E, 0x41, 0x5D, 0x41, 0x5C};
static unsigned char g_mask2[] = {0x48, 0x89, 0x5c, 0x24, 0x10};


static NTSTATUS patch(unsigned char * pData,
                  unsigned char * pMask, 
                  int maskSize,
                  unsigned char * pTarget)
{
    int i =0, j =0;
    
    for(i = 0; i<0x1000; ++i, ++pData)
    {
        if (pData[0] == pMask[0])
        {
            if (memcmp(pData, pMask, maskSize)==0)
            {
                long long oldCr0 = CPPLIB_DisableKernelDefence();
                long long delta = 0;

                *pData = 0xE8; 
                ++pData;
                delta = (long long)pTarget - (long long)pData - 4;
                *(unsigned __int32*)pData = (unsigned __int32)delta;

                CPPLIB_EnableKernelDefence(oldCr0);
                return STATUS_SUCCESS;
            }
        }
       
    }
    return STATUS_UNSUCCESSFUL;
}




static NTSTATUS init_x64()
{
    NTSTATUS status = 0;
    
    jumpToCatchCode_stub(0);

    status = patch((unsigned  char* )__CxxCallCatchBlock,
                   g_mask,
                   sizeof(g_mask),
                   (unsigned  char* )jumpToCatchCode);
    if (!NT_SUCCESS(status))
        return status;

    status = patch((unsigned  char* )_CxxThrowException,
                   g_mask2,
                   sizeof(g_mask2),
                   (unsigned  char* )CPPLIB_Throw);
    return status;
}

#endif

static
NTSTATUS
InitializePool(PPREALLOCATED_POOL Pool, ULONG EntrySize, ULONG EntriesCount) {
    ULONG bufferSize = EntrySize * EntriesCount;
    ULONG i;

    
    InitializeListHead(&Pool->FreeEntriesHead);
    Pool->EntrySize = EntrySize;
    Pool->EntriesCount = EntriesCount;
    Pool->Buffer = ExAllocatePoolWithTag(NonPagedPool, bufferSize, 'INFS');
    if (Pool->Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    KeInitializeSpinLock(&Pool->Lock);

    for (i = 0; i < EntriesCount; i++) {
        InsertTailList(&Pool->FreeEntriesHead, (PLIST_ENTRY)((ULONG_PTR)Pool->Buffer + i * EntrySize));
    }

    return STATUS_SUCCESS;
}

static
VOID
CleanupPool(PPREALLOCATED_POOL Pool) {
    if (Pool->Buffer != NULL)
        ExFreePool(Pool->Buffer);

    RtlZeroMemory(Pool, sizeof(PREALLOCATED_POOL));
}

static
PVOID
AllocateMemory(ULONG Size) 
{
    PVOID ptr = NULL;
    ULONG i;

    for (i = 0; i < POOLS_COUNT; i++) 
    {
        KIRQL oldIrql;

        KeAcquireSpinLock(&Pools[i].Lock, &oldIrql);
        if (Size <= Pools[i].EntrySize && !IsListEmpty(&Pools[i].FreeEntriesHead)) 
        {
            PLIST_ENTRY entry = RemoveHeadList(&Pools[i].FreeEntriesHead);
            KeReleaseSpinLock(&Pools[i].Lock, oldIrql);
            return entry;
        }
        KeReleaseSpinLock(&Pools[i].Lock, oldIrql);
    }
    return NULL;
}

static
VOID
FreeMemory(PVOID Ptr) 
{
    ULONG i;

    for (i = 0; i < POOLS_COUNT; i++) 
    {
        ULONG_PTR begin = (ULONG_PTR)Pools[i].Buffer;
        ULONG_PTR end = begin + Pools[i].EntrySize * Pools[i].EntriesCount;
        ULONG_PTR ptr = (ULONG_PTR)Ptr;

        if (ptr < end && ptr >= begin) 
        {
            KIRQL    oldIrql;
            KeAcquireSpinLock(&Pools[i].Lock, &oldIrql);
            InsertHeadList(&Pools[i].FreeEntriesHead, (PLIST_ENTRY)Ptr);
            KeReleaseSpinLock(&Pools[i].Lock, oldIrql);
            return;
        }
    }

    ExFreePool(Ptr);
}

typedef struct _GENERIC_TABLE_ENTRY {
    PETHREAD    Thread;
    int RefCount;
    PVOID    Ptd;
    
} GENERIC_TABLE_ENTRY, *PGENERIC_TABLE_ENTRY;

typedef struct _LIBCPP64_GLOBALS
{
    KSPIN_LOCK    Lock;
    RTL_GENERIC_TABLE    PtdMap;
    PGENERIC_TABLE_ENTRY    CachedEntry;
}
LIBCPP64_GLOBALS, *PLIBCPP64_GLOBALS;

static LIBCPP64_GLOBALS    Globals;



static
VOID
FreePtdForThread(PETHREAD Thread)
{
    KIRQL    oldIrql;
    PGENERIC_TABLE_ENTRY pFoundEntry = 0;

    BOOLEAN newElement = FALSE;
    GENERIC_TABLE_ENTRY    keyEntry = {Thread, 0, NULL};
    void * pPtd = 0;

    KeAcquireSpinLock(&Globals.Lock, &oldIrql);


    // query cache
    if (Globals.CachedEntry && Globals.CachedEntry->Thread == Thread)
    {
        // cache works
        pFoundEntry = Globals.CachedEntry;
    }
    else
    {
        //  query map
        pFoundEntry = RtlLookupElementGenericTable(&Globals.PtdMap, &keyEntry);
    }
    if (pFoundEntry)
    {
        if (!--pFoundEntry->RefCount)
        {
            pPtd = pFoundEntry->Ptd;
            RtlDeleteElementGenericTable(&Globals.PtdMap, pFoundEntry);
            if (Globals.CachedEntry == pFoundEntry)
            {
                Globals.CachedEntry = 0;
            }
            FreeMemory(pPtd);
        }
    }

    KeReleaseSpinLock(&Globals.Lock, oldIrql);
}


static
PGENERIC_TABLE_ENTRY
QueryTableEntryForThread(PETHREAD Thread, int bNeedAddRef)
{
    KIRQL    oldIrql;
    GENERIC_TABLE_ENTRY    tableEntry = {Thread, 1, NULL};

    NTSTATUS status = STATUS_SUCCESS;
    PGENERIC_TABLE_ENTRY pFoundEntry = 0;
        
    BOOLEAN newElement = FALSE;
    KeAcquireSpinLock(&Globals.Lock, &oldIrql);

    // query cache
    if (Globals.CachedEntry && Globals.CachedEntry->Thread == Thread)
    {
        // cache works
        pFoundEntry = Globals.CachedEntry;
        if (bNeedAddRef)
        {
            ++pFoundEntry->RefCount;
        }
        goto exit;
    }

    //  query or create
    {
        pFoundEntry = RtlInsertElementGenericTable(&Globals.PtdMap,
                                                   &tableEntry,
                                                   sizeof(tableEntry),
                                                   &newElement);
        if (!pFoundEntry)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto fail;
        }

        if (!newElement)
        {
            // open existing element
            if (bNeedAddRef)
            {
                ++pFoundEntry->RefCount;
            }
            goto exit;
        }
    }

    // need to create new element
    // allocating ptd
    {
        tableEntry.Ptd = AllocateMemory(sizeof(struct _tiddata));
        if (tableEntry.Ptd == NULL)
        {
            // out of memory - delete element
            RtlDeleteElementGenericTable(&Globals.PtdMap, pFoundEntry);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto fail;
        }
        pFoundEntry->Ptd = tableEntry.Ptd;
        RtlZeroMemory(tableEntry.Ptd, sizeof(struct _tiddata));
    }

    Globals.CachedEntry = pFoundEntry;
    KeReleaseSpinLock(&Globals.Lock, oldIrql);

    return pFoundEntry;

fail:

    if (tableEntry.Ptd != NULL)
    {
        FreeMemory(tableEntry.Ptd);
    }
    KeReleaseSpinLock(&Globals.Lock, oldIrql);
    return 0;

exit:
    KeReleaseSpinLock(&Globals.Lock, oldIrql);
    return pFoundEntry;
}

static
PVOID
QueryPtdForThread(PETHREAD Thread, int bNeedAddRef)
{
    PGENERIC_TABLE_ENTRY pEntry = QueryTableEntryForThread(Thread, bNeedAddRef);
    if (!pEntry)
        return 0;
    return pEntry->Ptd;
}

static
RTL_GENERIC_COMPARE_RESULTS
PtdMapCompareRoutine(
    PRTL_GENERIC_TABLE  Table,
    PVOID  FirstStruct,
    PVOID  SecondStruct
    )
{
    PGENERIC_TABLE_ENTRY    firstEntry = (PGENERIC_TABLE_ENTRY)FirstStruct;
    PGENERIC_TABLE_ENTRY    secondEntry = (PGENERIC_TABLE_ENTRY)SecondStruct;

    if (firstEntry->Thread < secondEntry->Thread)
    {
        return GenericLessThan;
    }
    else if (firstEntry->Thread > secondEntry->Thread)
    {
        return GenericGreaterThan;
    }
    else
    {
        return GenericEqual;
    }
}

static
PVOID
PtdMapAllocateRoutine(
    PRTL_GENERIC_TABLE  Table,
    CLONG  ByteSize
    )
{
    return AllocateMemory(ByteSize);
}

static
VOID
PtdMapFreeRoutine(
    PRTL_GENERIC_TABLE  Table,
    PVOID  Buffer
    )
{
    FreeMemory(Buffer);
}

// PUBLIC MODULE INTERFACE
NTSTATUS
initmt()
{
    ULONG i;
    NTSTATUS status;
    KEVENT    threadStartedEvent;

    status = 0;

#ifdef _M_AMD64
    status = init_x64();
    if (!NT_SUCCESS(status))
        return status;
#endif

    RtlZeroMemory(&Pools, sizeof(Pools));
    for (i = 0; i < POOLS_COUNT; i++)
    {
        status = InitializePool(&Pools[i], PoolSizes[i], 1024);
        if (!NT_SUCCESS(status))
        {
            ULONG j;
            for (j = 0; j < i; j++)
            {
                CleanupPool(&Pools[j]);
            }
            return status;
        }
    }

    RtlZeroMemory(&Globals, sizeof(Globals));
    KeInitializeSpinLock(&Globals.Lock);
    RtlInitializeGenericTable(&Globals.PtdMap, PtdMapCompareRoutine, PtdMapAllocateRoutine, PtdMapFreeRoutine, NULL);

    return STATUS_SUCCESS;
}

VOID
cleanupmt()
{
    NTSTATUS status;
    KIRQL    oldIrql;
    ULONG i;
    PGENERIC_TABLE_ENTRY    tableEntry;

    KeAcquireSpinLock(&Globals.Lock, &oldIrql);

    for (tableEntry = RtlEnumerateGenericTable(&Globals.PtdMap, TRUE);
         tableEntry != NULL;
         tableEntry = RtlEnumerateGenericTable(&Globals.PtdMap, TRUE))
    {
        BOOLEAN result;
        GENERIC_TABLE_ENTRY    tempEntry = *tableEntry;

        KeBugCheck(1);
        result = RtlDeleteElementGenericTable(&Globals.PtdMap, &tempEntry);

        FreeMemory(tempEntry.Ptd);
    }

    KeReleaseSpinLock(&Globals.Lock, oldIrql);

    for (i = 0; i < POOLS_COUNT; i++)
    {
        CleanupPool(&Pools[i]);
    }
}


PVOID __cdecl
_getptd()
{
    return QueryPtdForThread(PsGetCurrentThread(), 0);
}


void CPPLIB_ThreadAddRef()
{
    QueryPtdForThread(PsGetCurrentThread(), 1);
}

void CPPLIB_ThreadRelease()
{
    PETHREAD pThread = PsGetCurrentThread();
    FreePtdForThread(pThread);
}
