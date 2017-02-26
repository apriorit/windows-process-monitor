#ifndef __UTILS_H__
#define __UTILS_H__

#include <ntifs.h>

#include "libcpp.h"
#include "debug.h"

#include "NtDefinitions.h"

namespace utils
{
    NTSTATUS 
        CompleteIrp(PIRP Irp, NTSTATUS Status, ULONG_PTR Info);

    VOID 
        ScheduleProcessTerminate(HANDLE ProcessId);

    KSTART_ROUTINE 
        SystemThreadToTerminateTagetProcess;

    template <class T>
    class ScopedListReleaser
    {
        T& mListHolder;

        ScopedListReleaser(const ScopedListReleaser&); // Restrict the copy constructor
        void operator=(const ScopedListReleaser&); // Restrict the assignment operator
    public:
        ScopedListReleaser(T& ListHolder): mListHolder(ListHolder)
        {
        }

        ~ScopedListReleaser()
        {
            mListHolder.ReleaseList();
        }
    };

    // Class that handles automatic deletion of handle values
    template<typename T, typename TFunc, TFunc DestroyFunc, T defaultValue>
    class GenericScopedHandle
    {
    public:
        // Initializes a new instance of the GenericScopedHandle class with an invalid handle value.
        GenericScopedHandle() : _handle(defaultValue)
        {
        }

        // Initializes a new instance of the GenericScopedHandle class with the specified handle value.
        GenericScopedHandle(T handle) : _handle(handle)
        {
        }

        // Close the handle when the object is deleted
        virtual ~GenericScopedHandle()
        {
            if( _handle != defaultValue )
            {
                DestroyFunc(_handle);
            }
        }

        // Assigns ownership of the specified handle to this object. If the object already
        // had ownership of a handle, that handle is destroyed.
        GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue>& operator=(T handle)
        {
            if( _handle != defaultValue )
            {
                DestroyFunc(_handle);
            }

            _handle = handle;
            return *this;
        }

        // Converts the object to the handle value.
        operator T() const
        {
            return _handle;
        }

        // Gets a reference to the handle value. Warning: this function can be used to change
        // the handle value without deleting the old handle value.
        T& UnsafeGetValue()
        {
            return _handle;
        }
    private:
        // Prevent assignment from another scopedhandle
        GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue>& operator=(const GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue> &right)
        {
            return *this;
        }

        // Prevent construction from another scopedhandle
        GenericScopedHandle(const GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue> &right)
        {
        }

        T _handle;
    };

    void KernelFileNameToDosName(IN PUNICODE_STRING KernelName, PUNICODE_STRING* DosName);

    typedef GenericScopedHandle<HANDLE, NTSTATUS (NTAPI*)(HANDLE), ZwClose, NULL> ScopedHandle;

    template <class StringType, class CharacterType>
    bool nothrow_string_assign(StringType* pstr, CharacterType* value, USHORT len)
    {
        bool result = false;
        // STLPort (w)string::assign() can throw exception (using RtlRaiseException()) 
        // if its can't allocate memory for new entry.
        __try
        {
            pstr->assign(value, len);
            result = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
        return result;
    }

    NTSTATUS GetSystemProcessInformation(IN OUT PSYSTEM_PROCESS_INFORMATION* SysProcInfo);

    NTSTATUS GetMainModuleImageBase(IN HANDLE ProcessId, IN OUT PVOID* ImageBase, IN OUT SIZE_T* ImageSize);

    NTSTATUS GetProcessImageFileName(IN HANDLE ProcessId, IN OUT PUNICODE_STRING* ImageName);
}

#endif //__UTILS_H__