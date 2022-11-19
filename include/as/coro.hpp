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

namespace as::coro {

class Pool;
struct Task;

struct Promise {
    inline Task get_return_object();

    static std::suspend_always initial_suspend() { return {}; }
    static std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception()
    {
        exception = std::current_exception();
    }

    static void return_void() {}

    std::exception_ptr exception;
    Pool* pool = nullptr;
};

struct Task {
    using promise_type = Promise;

    static bool await_ready() { return false; }
    inline void await_suspend(
        std::coroutine_handle<Promise> suspendedHandle) const;
    void await_resume() {}

    std::coroutine_handle<Promise> handle;
};

class Pool {
    using Clock = std::chrono::high_resolution_clock;

public:
    [[nodiscard]] bool empty() const
    {
        return _chains.empty();
    }

    Pool& operator<<(Task task)
    {
        task.handle.promise().pool = this;
        _chains.emplace_back().push(task.handle);
        _chainByHandle.emplace(task.handle, _chains.size() - 1);
        return *this;
    }

    void subtask(
        std::coroutine_handle<Promise> oldHandle,
        std::coroutine_handle<Promise> newHandle)
    {
        std::cerr << "Pool::subtask: " << oldHandle.address() << " -> " <<
            newHandle.address() << "\n";
        auto chainIndex = _chainByHandle.at(oldHandle);
        _chains.at(chainIndex).push(newHandle);
        _chainByHandle.erase(oldHandle);
        _chainByHandle.emplace(newHandle, chainIndex);
    }

    void tick()
    {
        if (empty()) {
            return;
        }

        auto& chain = _chains.at(_index);
        chain.top().resume();
        if (auto e = chain.top().promise().exception; e) {
            std::rethrow_exception(e);
        }

        if (chain.top().done()) {
            std::cerr << "task " << chain.top().address() << " is done\n";
            _chainByHandle.erase(chain.top());
            chain.top().destroy();
            chain.pop();
            if (chain.empty()) {
                if (_index + 1 < _chains.size()) {
                    std::swap(chain, _chains.back());
                } else {
                    _index = 0;
                }
                _chains.resize(_chains.size() - 1);
            } else {
                _chainByHandle.emplace(chain.top(), _index);
                _index = (_index + 1) % _chains.size();
            }
        }
    }

    template <class Rep, class Period>
    void runFor(const std::chrono::duration<Rep, Period>& duration)
    {
        runUntil(Clock::now() +
            std::chrono::duration_cast<Clock::duration>(duration));
    }

    template <class C, class D>
    void runUntil(const std::chrono::time_point<C, D>& end)
    {
        while (Clock::now() < end) {
            tick();
        }
    }

private:
    std::vector<std::stack<std::coroutine_handle<Promise>>> _chains;
    std::map<std::coroutine_handle<Promise>, size_t> _chainByHandle;
    size_t _index = 0;
};

Task Promise::get_return_object()
{
    auto handle = std::coroutine_handle<Promise>::from_promise(*this);
    return Task{.handle = handle};
}

void Task::await_suspend(std::coroutine_handle<Promise> suspendedHandle) const
{
    std::cerr << "await_suspend: suspended " << suspendedHandle.address() <<
        ", starting " << handle.address() << "\n";

    Pool* pool = std::coroutine_handle<Promise>::from_address(
        suspendedHandle.address()).promise().pool;

    std::coroutine_handle<Promise>::from_address(
        handle.address()).promise().pool = pool;
    if (pool) {
        pool->subtask(suspendedHandle, handle);
    }
}

} // namespace as::coro
