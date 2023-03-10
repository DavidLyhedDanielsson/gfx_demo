#pragma once

#include <condition_variable>
#include <mutex>

template<typename T>
class ConcurrentData
{
    mutable std::mutex mutex;
    T data;

  public:
    template<typename F>
    void modify(F func)
    {
        std::lock_guard lock(mutex);
        func(data);
    }

    template<typename F>
    void view(F func) const
    {
        std::lock_guard lock(mutex);
        func(std::cref(data));
    }
};

enum class NotifyType : int32_t
{
    NOTIFY_NONE = 0,
    NOTIFY_ONE,
    NOTIFY_ALL,
};

template<typename T>
class WaitableConcurrentData
{
  private:
    mutable std::mutex mutex;
    std::condition_variable condition;
    T data;

  public:
    template<typename F>
    void modify(F func)
    {
        std::lock_guard lock(mutex);
        notify(func(data));
    }

    template<typename F>
    void hold(F func)
    {
        std::unique_lock lock(mutex);
        notify(func(lock, condition, data));
    }

    template<typename F>
    void view(F func) const
    {
        std::lock_guard lock(mutex);
        func(std::cref(data));
    }

    void notify(NotifyType notifyType)
    {
        switch(notifyType)
        {
            case NotifyType::NOTIFY_NONE: break;
            case NotifyType::NOTIFY_ONE: condition.notify_one(); break;
            case NotifyType::NOTIFY_ALL: condition.notify_all(); break;
        }
    }
};