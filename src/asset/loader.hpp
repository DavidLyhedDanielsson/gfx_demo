#pragma once

#include <cassert>
#include <coroutine>
#include <exception>
#include <memory>

class Asset;

class Loader
{
  public:
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        std::unique_ptr<Asset> value;
        std::exception_ptr exception;

        // clang-format off
        Loader get_return_object() { return Loader(handle_type::from_promise(*this)); }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { this->exception = std::current_exception(); }
        // clang-format on
        std::suspend_always yield_value(Asset* value)
        {
            this->value = std::move(std::unique_ptr<Asset>(value));
            return {};
        }
        void return_void() {} // TODO: (force?) return Asset*
    };

    handle_type handle;
    uint32_t id = -1;

    Loader(handle_type handle): handle(handle) {}
    Loader(Loader&& other): handle(other.handle), id(other.id)
    {
        other.handle = nullptr;
    }
    Loader& operator=(Loader&& other)
    {
        this->handle = std::exchange(other.handle, nullptr);
        this->id = std::exchange(other.id, -1);
        return *this;
    }
    ~Loader()
    {
        if(handle)
            handle.destroy();
    }

    bool run()
    {
        assert(id != -1);
        handle();

        if(handle.promise().exception)
            std::rethrow_exception(handle.promise().exception);

        return !handle.done();
    }

    std::unique_ptr<Asset> getAsset()
    {
        return std::move(handle.promise().value);
    }
};
