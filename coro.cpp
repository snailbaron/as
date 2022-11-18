#include <as/coro.hpp>

#include <exception>
#include <iostream>

namespace coro {

Task::Task(Pool& pool, std::coroutine_handle<Promise> handle)
    : _pool(pool)
    , _handle(handle)
{
}

std::coroutine_handle<Promise> Task::handle() const
{
    return _handle;
}

void Task::await_suspend(std::coroutine_handle<Promise> suspendedHandle)
{
    std::cerr << "await_suspend: suspended " << suspendedHandle.address() <<
        ", starting " << _handle.address() << "\n";
    _pool.subtask(suspendedHandle, _handle);
}

Promise::Promise(Pool& pool)
    : _pool(pool)
{ }

Task Promise::get_return_object()
{
    auto handle = std::coroutine_handle<Promise>::from_promise(*this);
    return Task{_pool, handle};
}

void Promise::unhandled_exception()
{
    exception = std::current_exception();
}

void Promise::return_void()
{
    std::cerr << "return_void\n";
}

void Pool::run(Task(*coroutine)(Pool&))
{
    auto task = coroutine(*this);
    _chains.emplace_back().push(task.handle());
    _chainByHandle.emplace(task.handle(), _chains.size() - 1);
}

void Pool::subtask(
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

void Pool::tick()
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

} // namespace coro
