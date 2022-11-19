#pragma once

#include <chrono>
#include <coroutine>
#include <exception>
#include <map>
#include <stack>
#include <vector>

namespace as::coro {

class Pool;
struct Task;

struct Promise {
    Task get_return_object();
    static std::suspend_always initial_suspend();
    static std::suspend_always final_suspend() noexcept;
    void unhandled_exception();
    static void return_void();

    std::exception_ptr exception;
    Pool* pool = nullptr;
};

struct Task {
    using promise_type = Promise;

    static bool await_ready();
    void await_suspend(std::coroutine_handle<Promise> suspendedHandle) const;
    void await_resume();

    std::coroutine_handle<Promise> handle;
};

class Pool {
    using Clock = std::chrono::high_resolution_clock;

public:
    Pool() = default;
    Pool(Pool&&) = default;
    Pool& operator=(Pool&&) = default;

    Pool(const Pool&) = delete;
    Pool& operator=(Pool&) = delete;

    ~Pool();

    [[nodiscard]] bool empty() const;
    Pool& operator<<(Task task);

    void tick();

    void clear();

    template <class C, class D>
    void runUntil(const std::chrono::time_point<C, D>& end)
    {
        while (Clock::now() <
                std::chrono::time_point_cast<Clock::duration>(end)) {
            tick();
        }
    }

    template <class Rep, class Period>
    void runFor(const std::chrono::duration<Rep, Period>& duration)
    {
        runUntil(Clock::now() +
            std::chrono::duration_cast<Clock::duration>(duration));
    }

private:
    friend struct Task;

    void subtask(
        std::coroutine_handle<Promise> oldHandle,
        std::coroutine_handle<Promise> newHandle);

    std::vector<std::stack<std::coroutine_handle<Promise>>> _chains;
    std::map<std::coroutine_handle<Promise>, size_t> _chainByHandle;
};

} // namespace as::coro
