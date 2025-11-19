#pragma once

#include "Debug.h"
#include "NtDefinitions.h"

namespace utils
{
    NTSTATUS CompleteIrp(PIRP irp, NTSTATUS status, ULONG_PTR info);

    void ScheduleProcessTerminate(HANDLE processId);

    KSTART_ROUTINE SystemThreadToTerminateTargetProcess;

    template <class T>
    class ScopedListReleaser
    {
    public:
        ScopedListReleaser(T& listHolder): m_listHolder(listHolder)
        {
        }

        ~ScopedListReleaser()
        {
            m_listHolder.ReleaseList();
        }

    private:
        ScopedListReleaser(const ScopedListReleaser&); // Restrict the copy constructor
        void operator=(const ScopedListReleaser&) = delete; // Restrict the assignment operator

    private:
        T& m_listHolder;
    };

    void KernelFileNameToDosName(_In_ const kf::USimpleString& kernelName, _Out_ kf::UString<PagedPool>& dosName);

    NTSTATUS GetSystemProcessInformation(_Out_ kf::vector<std::byte, PagedPool>& sysProcInfo);

    NTSTATUS GetMainModuleImageBase(_In_ HANDLE processId, _Inout_ PVOID* imageBase, _Inout_ SIZE_T* imageSize);

    NTSTATUS GetProcessImageFileName(_In_ HANDLE processId, _Out_ kf::UString<PagedPool>& imageName);
}
