#ifndef __SYNC_H__
#define __SYNC_H__

#include <ntifs.h>
#include "new.h"

namespace sync
{
    #define INFINITE            0xFFFFFFFF  // Infinite timeout

    template <class _T> class _AutoLock
    {
    public:
        _AutoLock(_T& Lock): m_Lock(Lock)
        {
            m_Lock.Lock();
        }

        ~_AutoLock()
        {
            m_Lock.Unlock();
        }
    private:
        _T& m_Lock;

        // Restrict the copy constructor
        _AutoLock(const _AutoLock&);
        // Restrict the assignment operator
        void operator=(const _AutoLock&);
    };

    template <class _T> class _AutoReadLock
    {
    public:
        _AutoReadLock(_T& Lock): m_Lock(Lock)
        {
            m_Lock.LockRead();
        }

        ~_AutoReadLock()
        {
            m_Lock.UnlockRead();
        }
    private:
        _T& m_Lock;

        // Restrict the copy constructor
        _AutoReadLock(const _AutoReadLock&);
        // Restrict the assignment operator
        void operator=(const _AutoReadLock&);
    };


    template <class _T> class _AutoWriteLock
    {
    public:
        _AutoWriteLock(_T& Lock): m_Lock(Lock)
        {
            m_Lock.LockWrite();
        }

        ~_AutoWriteLock()
        {
            m_Lock.UnlockWrite();
        }
    private:
        _T& m_Lock;

        // Restrict the copy constructor
        _AutoWriteLock(const _AutoWriteLock&);
        // Restrict the assignment operator
        void operator=(const _AutoWriteLock&);
    };



    class KMutex
    {
    private:
        PKMUTEX m_Mutex;

    public:
        KMutex()
        {
            m_Mutex = new(NonPagedPool) KMUTEX();
            if (m_Mutex)
                KeInitializeMutex(m_Mutex, 0);
        }

        ~KMutex()
        {
            if (m_Mutex)
                delete m_Mutex;
        }

        bool Valid()
        {
            return (m_Mutex != NULL);
        }

        NTSTATUS Lock(KWAIT_REASON WaitReason = Executive,
            KPROCESSOR_MODE WaitMode = KernelMode,
            bool Alertable = false)
        {
            ASSERT(Valid());
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            return KeWaitForMutexObject(m_Mutex, WaitReason, WaitMode, Alertable, 0);
        }

        NTSTATUS TryLock(LONGLONG Timeout = 0,
            KWAIT_REASON WaitReason = Executive,
            KPROCESSOR_MODE WaitMode = KernelMode,
            bool Alertable = false)
        {
            ASSERT(Valid());
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            LARGE_INTEGER liTimeout;
            liTimeout.QuadPart = Timeout;
            KeWaitForMutexObject(m_Mutex, WaitReason, WaitMode, Alertable, &liTimeout);
        }

        void Unlock(bool NextCallIsWaitXXX = false)
        {
            ASSERT(Valid());
            KeReleaseMutex(m_Mutex, NextCallIsWaitXXX);
        }
    };

    class KFastMutex
    {
    public:
        KFastMutex()
        {
            ExInitializeFastMutex(&mFastMutex);
        }

        ~KFastMutex()
        {
        }

        void Lock()
        {
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            ExAcquireFastMutex(&mFastMutex);
        }

        void Unlock()
        {
            ExReleaseFastMutex(&mFastMutex);
        }
    private:
        FAST_MUTEX mFastMutex;

        // Restrict the copy constructor
        KFastMutex(const KFastMutex&);
        // Restrict the assignment operator
        void operator=(const KFastMutex&);
    };

    class KEResource
    {
    public:
        KEResource()
        {
            ExInitializeResourceLite(&mEResource);
        }

        ~KEResource()
        {
            ExDeleteResourceLite(&mEResource);
        }

        void LockRead()
        {
            KeEnterCriticalRegion();
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            ExAcquireResourceSharedLite(&mEResource, TRUE);
        }

        void LockWrite()
        {
            KeEnterCriticalRegion();
            ASSERT(!ExIsResourceAcquiredSharedLite(&mEResource));
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            ExAcquireResourceExclusiveLite(&mEResource, TRUE);
        }

        void UnlockRead()
        {
            ExReleaseResourceLite(&mEResource);
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            KeLeaveCriticalRegion();
        }

        void UnlockWrite()
        {
            ExReleaseResourceLite(&mEResource);
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            KeLeaveCriticalRegion();
        }
    private:
        ERESOURCE mEResource;

        // Restrict the copy constructor
        KEResource(const KEResource&);
        // Restrict the assignment operator
        void operator=(const KEResource&);
    };

    class Event
    {
    private:
        PKEVENT mEvent;
        bool mUserModeEvent;

    public:
        Event(): mEvent(NULL), mUserModeEvent(false)
        {
        }

        ~Event()
        {
            Cleanup();
        }

        bool Initialize(bool InitialState = false, bool AutoReset = false)
        {
            ASSERT(!Valid());
            mEvent = new(NonPagedPool) KEVENT();
            if (!Valid())
                return false;
            EVENT_TYPE eventType = AutoReset ? SynchronizationEvent : NotificationEvent;
            KeInitializeEvent(mEvent, eventType, InitialState);
            return true;
        }

        bool Initialize(HANDLE UserModeHandle)
        {
            // convert UserMode EventHandle to KernelMode presentation
            NTSTATUS status = ObReferenceObjectByHandle(UserModeHandle, 
                EVENT_MODIFY_STATE, *ExEventObjectType, UserMode, (PVOID*) &mEvent, NULL);
            mUserModeEvent = status == STATUS_SUCCESS; 
            return Valid();
        }

        void Cleanup()
        {
            if (!Valid())
                return;

            if (mUserModeEvent)
            {
                ObDereferenceObject(mEvent);
            }
            else
            {
                delete mEvent;
            }
            mEvent = NULL;
        }

        bool Valid()
        {
            return (mEvent != NULL);
        }

        void Set()
        {
            ASSERT(Valid());
            KeSetEvent(mEvent, IO_NO_INCREMENT, FALSE);
        }

        void Clear()
        {
            ASSERT(Valid());
            KeClearEvent(mEvent);
        }

        bool IsSet()
        {
            ASSERT(Valid());
            return (KeReadStateEvent(mEvent) != FALSE);
        }


        bool Wait(unsigned Timeout = INFINITE, PNTSTATUS WaitStatus = NULL)
        {
            ASSERT(Valid());
            LARGE_INTEGER timeout;
            PLARGE_INTEGER p;
            if (Timeout == INFINITE)
            {
                p = NULL;
            }
            else
            {
                timeout.QuadPart = Timeout * (10*1000/*milliseconds*/);
                timeout.QuadPart *= -1;
                p = &timeout;
            }
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            NTSTATUS status = KeWaitForSingleObject(mEvent, Executive, KernelMode, FALSE, p);
            if (WaitStatus)
                *WaitStatus = status;
            return NT_SUCCESS(status);
        }
    };

    typedef KFastMutex AtomicLock;
    typedef KEResource SharedLock;

    typedef _AutoLock<AtomicLock> AutoLock;
    typedef _AutoLock<KMutex> AutoMutex;
    typedef _AutoReadLock<SharedLock> AutoReadLock;
    typedef _AutoWriteLock<SharedLock> AutoWriteLock;
}

#endif //__SYNC_H__