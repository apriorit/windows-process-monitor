#pragma once

#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <format>
#include <WinSvc.h>
#include <atlbase.h>

namespace utils
{
    class CLocalAllocGuard
    {
    public:
        explicit CLocalAllocGuard(void* memoryHandle) : m_memoryHandle(memoryHandle) {}

        ~CLocalAllocGuard()
        {
            if (m_memoryHandle)
            {
                ::LocalFree(m_memoryHandle);
            }
        }

        void* Release()
        {
            return std::exchange(m_memoryHandle, nullptr);
        }

    private:
        CLocalAllocGuard(const CLocalAllocGuard&);
        CLocalAllocGuard& operator=(const CLocalAllocGuard&) = delete;

    private:
        void* m_memoryHandle = nullptr;
    };

    class Event
    {
    public:
        Event(bool initialState = false, bool autoReset = false);

        bool Valid();
        void Set();
        void Reset();
        bool IsSet();
        bool Wait(DWORD timeout = INFINITE);
        HANDLE GetHandle();

    private:
        CHandle m_event{};
    };

    inline std::wstring GetWindowsDir()
    {
        WCHAR tmp[MAX_PATH] = {0};
        UINT len = ::GetWindowsDirectoryW(tmp, MAX_PATH);
        if (len == 0)
        {
            throw std::runtime_error("Can't retrive Windows directory path");
        }

        return tmp;
    }

    inline std::wstring GetModulePath()
    {
        wchar_t tmp[MAX_PATH] = {0};
        DWORD len = ::GetModuleFileNameW(nullptr, tmp, MAX_PATH);
        if (len == 0 || len == MAX_PATH)
        {
            return L"";
        }

        std::wstring sysPath(tmp);
        return sysPath.substr(0, sysPath.find_last_of('\\') + 1);
    }

    // Class that handles automatic deletion of handle values
    template<typename T, typename TFunc, TFunc DestroyFunc, T defaultValue>
    class GenericScopedHandle
    {
    public:
        // Initializes a new instance of the GenericScopedHandle class with an invalid handle value.
        GenericScopedHandle() : m_handle(defaultValue)
        {
        }

        // Initializes a new instance of the GenericScopedHandle class with the specified handle value.
        GenericScopedHandle(T handle) : m_handle(handle)
        {
        }

        // Close the handle when the object is deleted
        virtual ~GenericScopedHandle()
        {
            if (m_handle != defaultValue)
            {
                DestroyFunc(m_handle);
            }
        }

        // Assigns ownership of the specified handle to this object. If the object already
        // had ownership of a handle, that handle is destroyed.
        GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue>& operator=(T handle)
        {
            if (m_handle != defaultValue)
            {
                DestroyFunc(m_handle);
            }

            m_handle = handle;
            return *this;
        }

        // Converts the object to the handle value.
        operator T() const
        {
            return m_handle;
        }

        // Gets a reference to the handle value. Warning: this function can be used to change
        // the handle value without deleting the old handle value.
        T& UnsafeGetValue()
        {
            return m_handle;
        }

    private:
        // Prevent assignment from another scopedhandle
        GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue>& operator=(const GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue>& right)
        {
            return *this;
        }

        // Prevent construction from another scopedhandle
        GenericScopedHandle(const GenericScopedHandle<T, TFunc, DestroyFunc, defaultValue>& right) = delete;

    private:
        T m_handle{};
    };

    using ScopedServiceHandle = GenericScopedHandle<SC_HANDLE, BOOL (WINAPI*)(SC_HANDLE), CloseServiceHandle, nullptr>;

    inline std::wstring GetModuleFileNameEx()
    {
        WCHAR tmp[MAX_PATH] = {0};
        UINT len = ::GetModuleFileNameW(nullptr, tmp, MAX_PATH);
        if (len == 0 || len == MAX_PATH)
        {
            throw std::runtime_error("Can't retrive executable path");
        }

        return tmp;
    }

    inline std::wstring GetDriverPath(std::wstring_view driverName)
    {
        return std::format(L"{}\\System32\\Drivers\\{}.sys", utils::GetWindowsDir(), driverName);
    }
}
