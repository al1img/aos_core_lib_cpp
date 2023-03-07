/*
 * Copyright (C) 2023 Renesas Electronics Corporation.
 * Copyright (C) 2023 EPAM Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef AOS_THREAD_HPP_
#define AOS_THREAD_HPP_

#include <cstdint>
#include <pthread.h>

#include "aos/common/config/thread.hpp"
#include "aos/common/error.hpp"
#include "aos/common/noncopyable.hpp"
#include "aos/common/utils.hpp"

namespace aos {

/**
 * Default tread stack size.
 */
constexpr auto cDefaultThreadStackSize = AOS_CONFIG_THREAD_DEFAULT_STACK_SIZE;

/**
 * Defines callable interface used by thread.
 */
class CallableItf {
public:
    /**
     * Returns size of function.
     *
     * @return size_t.
     */
    virtual size_t Size() const = 0;

    /**
     * Call function operator.
     */
    virtual void operator()() const = 0;
};

/**
 * Thread function instance.
 *
 * @tparam T type of thread function instance.
 */
template <typename T>
class Function : public CallableItf {
public:
    // cppcheck-suppress noExplicitConstructor
    /**
     * Constructs thread function instance.
     *
     * @param functor functor used as function.
     * @param arg  optional argument that will be passed to functor.
     */
    Function(T& functor, void* arg = nullptr)
        : mFunctor(functor)
        , mArg(arg)
    {
    }

    /**
     * Implements call function operator.
     */
    void operator()() const override { mFunctor(mArg); }

    /**
     * Returns size of function.
     *
     * @return size_t.
     */
    size_t Size() const override { return sizeof(Function); };

    /**
     * Own placement new operator.
     *
     * @param size size of object.
     * @param mem pointer to memory region.
     * @return void*.
     */
    static void* operator new(size_t size, void* mem)
    {
        (void)size;

        return mem;
    }

private:
private:
    T     mFunctor;
    void* mArg;
};

/**
 * Aos thread.
 */
template <size_t cStackSize = cDefaultThreadStackSize>
class Thread : private NonCopyable {
public:
    /**
     * Constructs Aos thread instance and use lambda as argument.
     *
     * @param functor function to be called in thread.
     * @param arg optional argument that is passed to the thread function.
     */
    template <typename T>
    explicit Thread(T functor, void* arg = nullptr)
        : mPThread(0)
    {
        static_assert(AlignedSize(sizeof(T)) < AlignedSize(cStackSize), "not enough space to store functor");

        // cppcheck-suppress noDestructor // We don't need destructor since we create object on the static buffer.
        mCallable = new (mStack) Function<T>(functor, arg);
    }

    /**
     * Runs thread function.
     *
     * @return Error.
     */
    Error Run()
    {
        pthread_attr_t attr;

        auto ret = pthread_attr_init(&attr);
        if (ret != 0) {
            return ret;
        }

        ret = pthread_attr_setstack(
            &attr, &mStack[AlignedSize(mCallable->Size())], AlignedSize(cStackSize) - AlignedSize(mCallable->Size()));
        if (ret != 0) {
            return ret;
        }

        return pthread_create(&mPThread, &attr, ThreadFunction, this);
    }

    /**
     * Waits thread function is finished.
     *
     * @return Error.
     */
    Error Join() { return pthread_join(mPThread, nullptr); }

private:
    uint8_t      mStack[AlignedSize(cStackSize)];
    pthread_t    mPThread;
    CallableItf* mCallable;

    static void* ThreadFunction(void* arg)
    {
        (*static_cast<Thread*>(arg)->mCallable)();

        return nullptr;
    }
};

/**
 * Aos mutex.
 */
class Mutex : private NonCopyable {
public:
    /**
     * Constructs Aos mutex.
     */
    Mutex() { pthread_mutex_init(&mPMutex, nullptr); }

    /**
     * Destroys Aos mutex.
     */
    ~Mutex() { pthread_mutex_destroy(&mPMutex); }

    /**
     * Locks Aos mutex.
     *
     * @return Error.
     */
    Error Lock() { return pthread_mutex_lock(&mPMutex); }

    /**
     * Unlocks Aos mutex.
     *
     * @return Error.
     */
    Error Unlock() { return pthread_mutex_unlock(&mPMutex); }

    /**
     * Converts mutex to pthread_mutex_t pointer.
     *
     * @return pthread_mutex_t pointer.
     */

    explicit operator pthread_mutex_t*() { return &mPMutex; }

private:
    pthread_mutex_t mPMutex;
};

/**
 * Aos lock guard.
 */
class LockGuard : private NonCopyable {
public:
    /**
     * Creates lock guard instance.
     *
     * @param mutex mutex used to guard.
     */
    explicit LockGuard(Mutex& mutex)
        : mMutex(mutex)
    {
        mError = mMutex.Lock();
    }

    /**
     * Destroys lock guard instance.
     */
    ~LockGuard() { mMutex.Unlock(); }

    /**
     * Returns current lock guard error.
     *
     * @return Error.
     */
    Error GetError() { return mError; }

private:
    Mutex& mMutex;
    Error  mError;
};

/**
 * Aos unique lock.
 */
class UniqueLock : private NonCopyable {
public:
    /**
     * Creates unique lock instance.
     *
     * @param mutex mutex used to guard.
     */
    explicit UniqueLock(Mutex& mutex)
        : mMutex(mutex)
        , mIsLocked(false)
    {
        mError = Lock();
    }

    /**
     * Destroys unique lock instance.
     */
    ~UniqueLock()
    {
        if (mIsLocked) {
            Unlock();
        }
    }

    /**
     * Locks unique lock instance.
     */
    Error Lock()
    {
        mError = mMutex.Lock();
        if (!mError.IsNone()) {
            return mError;
        }

        mIsLocked = true;

        return mError;
    }

    /**
     * Unlocks unique lock instance.
     */
    Error Unlock()
    {
        mError = mMutex.Unlock();
        if (!mError.IsNone()) {
            return mError;
        }

        mIsLocked = false;

        return mError;
    }

    /**
     * Returns current unique lock error.
     *
     * @return Error.
     */
    Error GetError() { return mError; }

private:
    Mutex& mMutex;
    bool   mIsLocked;
    Error  mError;
};

/**
 * Aos conditional variable.
 */
class ConditionalVariable : private NonCopyable {
public:
    /**
     * Creates conditional variable.
     *
     * @param mutex conditional variable mutex.
     */
    explicit ConditionalVariable(Mutex& mutex)
        : mMutex(mutex)
    {
        mError = pthread_cond_init(&mCondVar, nullptr);
    }

    /**
     * Destroys conditional variable.
     */
    ~ConditionalVariable() { pthread_cond_destroy(&mCondVar); }

    /**
     * Blocks the current thread until the condition variable is awakened.
     *
     * @return Error.
     */
    Error Wait() { return mError = pthread_cond_wait(&mCondVar, static_cast<pthread_mutex_t*>(mMutex)); }

    /**
     * Blocks the current thread until the condition variable is awakened and predicate condition is met.
     *
     * @return Error.
     */
    template <typename T>
    Error Wait(T waitCondition)
    {
        while (!waitCondition()) {
            auto err = Wait();
            if (!err.IsNone()) {
                return err;
            }
        }

        return ErrorEnum::eNone;
    }

    /**
     * Notifies one waiting thread.
     *
     * @return Error.
     */
    Error NotifyOne() { return mError = pthread_cond_signal(&mCondVar); }

    /**
     * Notifies all waiting thread.
     *
     * @return Error.
     */
    Error NotifyAll() { return mError = pthread_cond_broadcast(&mCondVar); }

    /**
     * Returns current conditional variable error.
     *
     * @return Error.
     */
    Error GetError() { return mError; }

private:
    Mutex&         mMutex;
    pthread_cond_t mCondVar;
    Error          mError;
};

} // namespace aos

#endif
