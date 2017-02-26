/*
    This is a C++ run-time library for Windows kernel-mode drivers.
    Copyright (C) 2003 Bo Brantén.
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/
extern "C" {

#include <ntifs.h>
}
#include "exception"
#include "new.h"


std::exception::exception()
        : _m_what("")
{
}
std::exception::exception(char const * const & w)
    : _m_what(w)
{
}


#if (DDK_3790_USED==0)
std::exception::exception(const char * const & w, int)
    : _m_what(w)
{
}

#endif

std::exception::exception(const std::exception&e)
    : _m_what(e._m_what)
{
}
std::exception& std::exception::operator= (const std::exception& e)
{
    _m_what = e._m_what;
    return *this;
}
std::exception::~exception()
{
}
const char * std::exception::what() const
{
    return _m_what;
}


extern "C" {

#include "libcpp.h"


    
void __cdecl _invoke_watson()
{
    KeBugCheckEx(TRAP_CAUSE_UNKNOWN, 0,0,0, (ULONG_PTR)STATUS_UNSUCCESSFUL );
}

char * __cdecl __unDName(
                char * outputString,
                const char * name,
                int maxStringLength,
                void * (* pAlloc )(size_t),
                void (* pFree )(void *),
                unsigned short disableFlags)
{
    KeBugCheckEx(TRAP_CAUSE_UNKNOWN, 0,0,0, (ULONG_PTR)STATUS_UNSUCCESSFUL );
    outputString[0] = 0;
    return 0;
}

NTSYSAPI
VOID
NTAPI
RtlRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    );

NTSTATUS    initmt();
VOID    cleanupmt();

}

typedef void (__cdecl *func_t)();

//
// Data segment with pointers to C++ initializers.
//
#pragma data_seg(".CRT$XCA")
func_t xc_a[] = { 0 };
#pragma data_seg(".CRT$XCZ")
func_t xc_z[] = { 0 };
#pragma data_seg()

#if _MSC_FULL_VER >= 140050214 || defined (_M_IA64) || defined (_M_AMD64)
#pragma comment(linker, "/merge:.CRT=.rdata")
#else
#pragma comment(linker, "/merge:.CRT=.data")
#endif

#if defined (_M_IA64) || defined (_M_AMD64)
#pragma section(".CRT$XCA",read)
#pragma section(".CRT$XCZ",read)
#endif

//
// Simple class to keep track of functions registred to be called when
// unloading the driver. Since we only use this internaly from the load
// and unload functions it doesn't need to be thread safe.
//
class AtExitCall {
public:
    AtExitCall(func_t f) : m_func(f), m_next(m_exit_list) { m_exit_list = this; }
    ~AtExitCall() { m_func(); m_exit_list = m_next; }
    static void run() { while (m_exit_list) delete m_exit_list; }
private:
    func_t              m_func;
    AtExitCall*         m_next;
    static AtExitCall*  m_exit_list;
};

AtExitCall* AtExitCall::m_exit_list = 0;

//
// Calls functions the compiler has registred to call constructors
// for global and static objects.
//


NTSTATUS __cdecl libcpp_init()
{
    NTSTATUS status;
    status = initmt();
    if (!NT_SUCCESS(status)) 
    {
        return status;
    }

    for (func_t* f = xc_a; f < xc_z; f++) 
    {
        if (*f) 
        {
            (*f)();
        }
    }
    return STATUS_SUCCESS;
}

//
// Calls functions the compiler has registred to call destructors
// for global and static objects.
//
void __cdecl libcpp_exit()
{
    AtExitCall::run();
    cleanupmt();
}

extern "C" {

//
// The run-time support for RTTI uses malloc and free so we include them here.
//

void * __cdecl malloc(size_t size)
{
    return size ? ExAllocatePoolWithTag(NonPagedPool, size, '+MFS') : 0;
}

void __cdecl free(void *p)
{
    if (p) { ExFreePool(p); }
}

//
// Registers a function to be called when unloading the driver. If memory
// couldn't be allocated the function is called immediately since it never
// will be called otherwise.
//
int __cdecl atexit(func_t f)
{
    return (new AtExitCall(f) == 0) ? (*f)(), 1 : 0;
}

/*
 * The statement:
 *
 *   throw E();
 *
 * will be translated by the compiler to:
 *
 *   E e = E();
 *   _CxxThrowException(&e, ...);
 *
 * and _CxxThrowException is implemented as:
 *
 *   #define CXX_FRAME_MAGIC 0x19930520
 *   #define CXX_EXCEPTION   0xe06d7363
 *
 *   void _CxxThrowException(void *object, cxx_exception_type *type)
 *   {
 *       ULONG args[3];
 *
 *       args[0] = CXX_FRAME_MAGIC;
 *       args[1] = (ULONG) object;
 *       args[2] = (ULONG) type;
 *
 *       RaiseException(CXX_EXCEPTION, EXCEPTION_NONCONTINUABLE, 3, args);
 *   }
 *
 * so whats left for us to implement is RaiseException
 *
 */
extern "C" void CPPLIB_ThreadAddRef();

VOID
NTAPI
RaiseException (
    ULONG   ExceptionCode,
    ULONG   ExceptionFlags,
    ULONG   NumberParameters,
    PULONG_PTR  ExceptionInformation
    )
{
    EXCEPTION_RECORD ExceptionRecord = {
        ExceptionCode,
        ExceptionFlags & EXCEPTION_NONCONTINUABLE,
        NULL,
        RaiseException,
        NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS ? EXCEPTION_MAXIMUM_PARAMETERS : NumberParameters
    };

    RtlCopyMemory(
        ExceptionRecord.ExceptionInformation,
        ExceptionInformation,
        ExceptionRecord.NumberParameters * sizeof(ULONG_PTR)
        );

    RtlRaiseException(&ExceptionRecord);
}

void *__cdecl _encoded_null() {return 0;}
void __cdecl _lock() {}
void __cdecl _unlock() {}

} // extern "C"

void __cdecl
terminate(void) {
    ASSERT(FALSE);
    KeBugCheckEx('++CL', 0, 0, 0, 0);
}

void __cdecl
_inconsistency(void) {
    ASSERT(FALSE);
    terminate();
}

void __cdecl
unexpected(void) {
    ASSERT(FALSE);
    terminate();
}

#include "typeinfo.h"
const char* type_info::name() const {return raw_name();}

void* __cdecl operator new (size_t size)
{
    if (size == 0)//When the value of the expression in a direct-new-declarator is zero, 
        size = 4;//the allocation function is called to allocatean array with no elements.(ISO)

    PVOID ptr = ExAllocatePoolWithTag(NonPagedPool, (ULONG)size, '+NFS');
    if (ptr == NULL)
    {
        throw std::bad_alloc();

    }
    return ptr;
}

void __cdecl operator delete (void *ptr)
{
    if (ptr != NULL)
    {
        ExFreePool(ptr);
    }
}

void* __cdecl operator new[] (size_t size)
{
    if (size == 0)//When the value of the expression in a direct-new-declarator is zero, 
        size = 4;//the allocation function is called to allocatean array with no elements.(ISO)

    PVOID ptr = ExAllocatePoolWithTag(NonPagedPool, (ULONG)size, '2NFS');
    if (ptr == NULL)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

void __cdecl operator delete[] (void *ptr)
{
    if (ptr != NULL)
    {
        ExFreePool(ptr);
    }
}

void* __cdecl operator new (size_t size, POOL_TYPE pool)
{
    if (size == 0)//When the value of the expression in a direct-new-declarator is zero, 
        size = 4;//the allocation function is called to allocatean array with no elements.(ISO)

    PVOID ptr = ExAllocatePoolWithTag(pool, (ULONG)size, '+NFS');
    if (ptr == NULL)
    {
        throw std::bad_alloc();
    }
    return ptr;
}

void __cdecl operator delete(void *ptr, POOL_TYPE pool)
{
    if (ptr != NULL)
    {
        ExFreePool(ptr);
    }
}

void* __cdecl operator new[] (size_t size, POOL_TYPE pool)
{
    if (size == 0)//When the value of the expression in a direct-new-declarator is zero, 
        size = 4;//the allocation function is called to allocatean array with no elements.(ISO)

    PVOID ptr = ExAllocatePoolWithTag(pool, (ULONG)size, '2NFS');
    if (ptr == NULL)
    {
        throw std::bad_alloc();
    }
    return ptr;
}


void* __cdecl operator new (size_t size, const std::nothrow_t&)
{
    return ExAllocatePoolWithTag(NonPagedPool, (ULONG)size, '+NFS');
}

void* __cdecl operator new[] (size_t size, const std::nothrow_t&)
{
    return ExAllocatePoolWithTag(NonPagedPool, (ULONG)size, '2NFS');
}



extern "C" {
static ULONG NTAPI probe(CONST PVOID Buffer, ULONG Length, LOCK_OPERATION Operation)
{
    return FALSE;
}

ULONG NTAPI IsBadReadPtr(CONST PVOID Buffer, ULONG Length)
{
    return probe(Buffer, Length, IoReadAccess);
}

ULONG NTAPI IsBadWritePtr(PVOID Buffer, ULONG Length)
{
    return probe(Buffer, Length, IoWriteAccess);
}

ULONG NTAPI IsBadCodePtr(CONST PVOID Buffer)
{
    return probe(Buffer, 1, IoReadAccess);
}
}

LONG
__CxxUnhandledExceptionFilter(EXCEPTION_POINTERS *) 
{
    ASSERT(FALSE);
    return EXCEPTION_CONTINUE_SEARCH;
}

