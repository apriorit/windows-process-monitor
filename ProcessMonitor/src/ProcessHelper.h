#pragma once

#include "Sync.h"

enum class ProcessMarker
{
    Clean,
    Disallowed,
    Allowed
};

struct ProcessHelper : public boost::intrusive_ref_counter<ProcessHelper>
{
    ProcessHelper(HANDLE parentPid, HANDLE pid) : parentPid(parentPid), pid(pid), marker(ProcessMarker::Clean)
    {
    }

    BOOLEAN IsSuspended() const { return resumeEvent.Valid() && !resumeEvent.IsSet(); }

    HANDLE parentPid{};
    HANDLE pid{};
    // Used to resume waiting process and select action for it
    sync::Event resumeEvent{};
    kf::UString<PagedPool> imageName{};
    ProcessMarker marker = ProcessMarker::Clean;
};

using ProcessHelperPtr = boost::intrusive_ptr<ProcessHelper>;

using ProcessHelperList = kf::map<HANDLE, ProcessHelperPtr, PagedPool>;

class ProcessHelperHolder
{
public:
    NTSTATUS Init();
    ~ProcessHelperHolder();

    ProcessHelperList& AquireList();
    _Requires_lock_held_(m_lock.m_resource)
    void ReleaseList();
    bool Add(ProcessHelperPtr&& procHlp);
    void Delete(HANDLE pid);
    void Clean();

private:
    ProcessHelperList m_processHelperList{};
    kf::EResource m_lock{};
};
