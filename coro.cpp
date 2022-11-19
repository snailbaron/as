#include <as/coro.hpp>

namespace as::coro {

Task Promise::get_return_object()
{
    auto handle = std::coroutine_handle<Promise>::from_promise(*this);
    return Task{.handle = handle};
}

std::suspend_always Promise::initial_suspend()
{
    return {};
}

std::suspend_always Promise::final_suspend() noexcept
{
    return {};
}

void Promise::unhandled_exception()
{
    exception = std::current_exception();
}

void Promise::return_void()
{
}

bool Task::await_ready()
{
    return false;
}

void Task::await_suspend(std::coroutine_handle<Promise> suspendedHandle) const
{
    Pool* pool = std::coroutine_handle<Promise>::from_address(
        suspendedHandle.address()).promise().pool;

    std::coroutine_handle<Promise>::from_address(
        handle.address()).promise().pool = pool;
    if (pool) {
        pool->subtask(suspendedHandle, handle);
    }
}

void Task::await_resume()
{
}

Pool::~Pool()
{
    clear();
}

[[nodiscard]] bool Pool::empty() const
{
    return _chains.empty();
}

Pool& Pool::operator<<(Task task)
{
    task.handle.promise().pool = this;
    _chains.emplace_back().push(task.handle);
    _chainByHandle.emplace(task.handle, _chains.size() - 1);
    return *this;
}

void Pool::subtask(
    std::coroutine_handle<Promise> oldHandle,
    std::coroutine_handle<Promise> newHandle)
{
    auto chainIndex = _chainByHandle.at(oldHandle);
    _chains.at(chainIndex).push(newHandle);
    _chainByHandle.erase(oldHandle);
    _chainByHandle.emplace(newHandle, chainIndex);
}

void Pool::tick()
{
    for (size_t i = 0; i < _chains.size(); ) {
        auto& chain = _chains.at(i);

        chain.top().resume();
        if (auto e = chain.top().promise().exception; e) {
            std::rethrow_exception(e);
        }

        if (chain.top().done()) {
            _chainByHandle.erase(chain.top());
            while (chain.top().done()) {
                chain.top().destroy();
                chain.pop();
                if (chain.empty()) {
                    break;
                }
                _chainByHandle.emplace(chain.top(), i);
                chain.top().resume();
                if (auto e = chain.top().promise().exception; e) {
                    std::rethrow_exception(e);
                }
            }
        }

        if (chain.empty()) {
            std::swap(chain, _chains.back());
            _chains.resize(_chains.size() - 1);
        } else {
            _chainByHandle.emplace(chain.top(), i);
            i++;
        }
    }
}

void Pool::clear()
{
    _chainByHandle.clear();
    for (auto& chain : _chains) {
        while (!chain.empty()) {
            if (!chain.top().done()) {
                chain.top().destroy();
            }
            chain.pop();
        }
    }
}

} // namespace as::coro
