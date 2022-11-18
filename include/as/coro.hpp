#pragma once

#include <chrono>
#include <concepts>
#include <coroutine>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stack>
#include <utility>
#include <vector>

namespace coro {

class Pool;
class Promise;

class Task {
public:
    using promise_type = Promise;

    [[nodiscard]] std::coroutine_handle<Promise> handle() const;
    Task(Pool& pool, std::coroutine_handle<Promise> handle);
    static bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<Promise> suspendedHandle);
    void await_resume() {}

private:
    Pool& _pool;
    std::coroutine_handle<Promise> _handle;
};

class Promise {
public:
    explicit Promise(Pool& pool);

    Task get_return_object();

    static std::suspend_always initial_suspend() { return {}; }
    static std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception();

    void return_void();

    std::exception_ptr exception;

private:
    Pool& _pool;
};

class Pool {
    using Clock = std::chrono::high_resolution_clock;

public:
    [[nodiscard]] bool empty() const
    {
        return _chains.empty();
    }

    void run(Task(*coroutine)(Pool&));

    void subtask(
        std::coroutine_handle<Promise> oldHandle,
        std::coroutine_handle<Promise> newHandle);

    void tick();

    template <class Rep, class Period>
    void runFor(const std::chrono::duration<Rep, Period>& duration)
    {
        auto end = Clock::now() + duration;
        while (Clock::now() < end) {
            tick();
        }
    }

    template <class C, class D>
    void runUntil(const std::chrono::time_point<C, D>& endTimePoint)
    {
        auto end = std::chrono::time_point_cast<Clock::duration>(endTimePoint);
        while (Clock::now() < end) {
            tick();
        }
    }

private:
    std::vector<std::stack<std::coroutine_handle<Promise>>> _chains;
    std::map<std::coroutine_handle<Promise>, size_t> _chainByHandle;
    size_t _index = 0;
};

} // namespace coro
