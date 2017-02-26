#ifndef __UTILS_H__
#define __UTILS_H__

#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <WinSvc.h>

namespace utils
{
    class CLocalAllocGuard
    {
        void * h_;
        CLocalAllocGuard(const CLocalAllocGuard&);
        CLocalAllocGuard& operator=(const CLocalAllocGuard&);
    public:
        explicit CLocalAllocGuard(void * h)
            :h_(h){}
        ~CLocalAllocGuard()
        {
            if(h_) LocalFree(h_); 
        }
        void * release()
        {
            void * hRes = h_;
            h_ = 0;
            return hRes;
        }
    };

    class Event
    {
    public:
        Event(bool InitialState = false, bool AutoReset = false);       
        ~Event();

        bool Valid();
        void Set();
        void Reset();
        bool IsSet();
        bool Wait(unsigned Timeout = INFINITE);
        HANDLE GetHandle();

    private:
        HANDLE m_hEvent;
    };

    inline
    std::wstring GetWindowsDir()
    {
        WCHAR tmp[MAX_PATH] = {0};
        UINT len = GetWindowsDirectory(tmp, MAX_PATH);
        if (len == 0)
            throw std::runtime_error("Can't retrive Windows directory path");
        return tmp;
    }

    inline
    std::wstring GetModulePath()
    {
        wchar_t tmp[MAX_PATH] = {0};
        DWORD len = GetModuleFileNameW(NULL, tmp, MAX_PATH);
        if (len ==  0 || len == MAX_PATH)
            return L"";

        std::wstring sysPath(tmp);
        return sysPath.substr(0, sysPath.find_last_of('\\') + 1);
    }

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

    typedef GenericScopedHandle<SC_HANDLE, BOOL (WINAPI*)(SC_HANDLE), CloseServiceHandle, NULL> ScopedServiceHandle;

	inline
	std::wstring GetModuleFileNameEx()
	{
		WCHAR tmp[MAX_PATH] = {0};
		UINT len = GetModuleFileName(NULL, tmp, MAX_PATH);
		if (len ==  0 || len == MAX_PATH)
			throw std::runtime_error("Can't retrive executable path");
		return tmp;
	}
}

#endif // __UTILS_H__