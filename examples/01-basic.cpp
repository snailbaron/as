#include <as.hpp>

#include <chrono>
#include <coroutine>
#include <iostream>
#include <thread>

using namespace std::literals::chrono_literals;

coro::Task waitOneSecond(coro::Pool& pool)
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::high_resolution_clock;
    const auto end = Clock::now() + 1s;
    while (Clock::now() < end) {
        co_await std::suspend_always{};
    }
    std::cerr << "finished waiting\n";
}

coro::Task printWithWait(coro::Pool& pool)
{
    const auto startTime = std::chrono::high_resolution_clock::now();

    auto timestamp = [&startTime] {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::high_resolution_clock::now() - startTime).count();
    };

    std::cout << timestamp() << "s before" << std::endl;
    co_await waitOneSecond(pool);
    std::cout << timestamp() << "s after" << std::endl;
}

int main()
{
    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(2);

    coro::Pool pool;
    pool.run(printWithWait);
    while (!pool.empty()) {
        pool.runFor(100ms);
        std::cout << "." << std::endl;
    }
}
