#pragma once

namespace sync
{
    #define INFINITE            0xFFFFFFFF  // Infinite timeout

    template <class T>
    class GenericAutoLock
    {
    public:
        GenericAutoLock(T& Lock): m_lock(Lock)
        {
            m_lock.Lock();
        }

        ~GenericAutoLock()
        {
            m_lock.Unlock();
        }

    private:
        // Restrict the copy constructor
        GenericAutoLock(const GenericAutoLock&);
        // Restrict the assignment operator
        void operator=(const GenericAutoLock&) = delete;

    private:
        T& m_lock;
    };

    class KMutex
    {
    public:
        KMutex()
        {
            m_mutex = kf::make_unique<KMUTEX, NonPagedPoolNx>();
            if (m_mutex)
            {
                ::KeInitializeMutex(m_mutex.get(), 0);
            }
        }

        bool Valid()
        {
            return !!m_mutex;
        }

        NTSTATUS Lock(KWAIT_REASON waitReason = Executive,
            KPROCESSOR_MODE waitMode = KernelMode,
            bool alertable = false)
        {
            ASSERT(Valid());
            ASSERT(::KeGetCurrentIrql() <= APC_LEVEL);
            return ::KeWaitForMutexObject(m_mutex.get(), waitReason, waitMode, alertable, nullptr);
        }

        NTSTATUS TryLock(LONGLONG timeout = 0,
            KWAIT_REASON waitReason = Executive,
            KPROCESSOR_MODE waitMode = KernelMode,
            bool alertable = false)
        {
            ASSERT(Valid());
            ASSERT(::KeGetCurrentIrql() <= APC_LEVEL);

            LARGE_INTEGER waitTimeout{};
            waitTimeout.QuadPart = timeout;
            ::KeWaitForMutexObject(m_mutex.get(), waitReason, waitMode, alertable, &waitTimeout);
        }

        void Unlock(bool nextCallIsWait = false)
        {
            ASSERT(Valid());

#pragma warning(push)
#pragma warning(disable : 28160)
            ::KeReleaseMutex(m_mutex.get(), nextCallIsWait);
#pragma warning(pop)
        }

    private:
        std::unique_ptr<KMUTEX> m_mutex = nullptr;
    };

    class KFastMutex
    {
    public:
        KFastMutex()
        {
            ::ExInitializeFastMutex(&m_fastMutex);
        }

        ~KFastMutex() = default;

        void Lock()
        {
            ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
            ::ExAcquireFastMutex(&m_fastMutex);
        }

        _Requires_lock_held_(m_fastMutex)
        void Unlock()
        {
            ::ExReleaseFastMutex(&m_fastMutex);
        }

    private:
        // Restrict the copy constructor
        KFastMutex(const KFastMutex&);
        // Restrict the assignment operator
        void operator=(const KFastMutex&) = delete;

    private:
        FAST_MUTEX m_fastMutex{};
    };

    class Event
    {
    public:
        Event(): m_event(nullptr), m_userModeEvent(false)
        {
        }

        ~Event()
        {
            Cleanup();
        }

        bool Initialize(bool initialState = false, bool autoReset = false)
        {
            ASSERT(!Valid());

            m_event = kf::make_unique<KEVENT, NonPagedPoolNx>();
            if (!Valid()) [[unlikely]]
            {
                return false;
            }

            EVENT_TYPE eventType = autoReset ? SynchronizationEvent : NotificationEvent;
            ::KeInitializeEvent(m_event.get(), eventType, initialState);
            return true;
        }

        bool Initialize(HANDLE userModeHandle)
        {
            // convert UserMode EventHandle to KernelMode presentation
            NTSTATUS status = ::ObReferenceObjectByHandle(userModeHandle,
                EVENT_MODIFY_STATE, *ExEventObjectType, UserMode, reinterpret_cast<PVOID*>(&m_event), nullptr);

            m_userModeEvent = NT_SUCCESS(status);

            return Valid();
        }

        void Cleanup()
        {
            if (!Valid())
            {
                return;
            }

            if (m_userModeEvent)
            {
                ::ObDereferenceObject(m_event.get());
                m_event.release();
            }
            else
            {
                m_event.reset();
            }

            ASSERT(!m_event);
        }

        bool Valid() const
        {
            return !!m_event.get();
        }

        void Set()
        {
            ASSERT(Valid());
            ::KeSetEvent(m_event.get(), IO_NO_INCREMENT, false);
        }

        void Clear()
        {
            ASSERT(Valid());
            ::KeClearEvent(m_event.get());
        }

        bool IsSet() const
        {
            ASSERT(Valid());
            return !!::KeReadStateEvent(m_event.get());
        }

        bool Wait(unsigned timeout = INFINITE, PNTSTATUS waitStatus = nullptr)
        {
            ASSERT(Valid());

            LARGE_INTEGER waitTimeout{};
            PLARGE_INTEGER waitTimeoutPtr = nullptr;
            if (timeout == INFINITE)
            {
                waitTimeoutPtr = nullptr;
            }
            else
            {
                waitTimeout.QuadPart = timeout * (10 * 1000 /*milliseconds*/);
                waitTimeout.QuadPart *= -1;
                waitTimeoutPtr = &waitTimeout;
            }

            ASSERT(::KeGetCurrentIrql() <= APC_LEVEL);
            NTSTATUS status = ::KeWaitForSingleObject(m_event.get(), Executive, KernelMode, false, waitTimeoutPtr);
            if (waitStatus)
            {
                *waitStatus = status;
            }

            return NT_SUCCESS(status);
        }

    private:
        std::unique_ptr<KEVENT> m_event = nullptr;
        bool m_userModeEvent = false;
    };

    using AtomicLock = KFastMutex;

    using AutoLock = GenericAutoLock<AtomicLock>;
    using AutoMutex = GenericAutoLock<KMutex>;
}
