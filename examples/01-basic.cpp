#include <as.hpp>

#include <chrono>
#include <coroutine>
#include <iostream>
#include <thread>

using namespace std::literals::chrono_literals;

namespace co = as::coro;

co::Task waitOneSecond()
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::high_resolution_clock;
    const auto end = Clock::now() + 1s;
    while (Clock::now() < end) {
        co_await std::suspend_always{};
    }
    std::cerr << "finished waiting\n";
}

co::Task printWithWait()
{
    const auto startTime = std::chrono::high_resolution_clock::now();

    auto timestamp = [&startTime] {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
            std::chrono::high_resolution_clock::now() - startTime).count();
    };

    std::cout << timestamp() << "s before" << std::endl;
    co_await waitOneSecond();
    std::cout << timestamp() << "s after" << std::endl;
}

int main()
{
    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(2);

    co::Pool pool;
    pool << printWithWait();
    while (!pool.empty()) {
        std::cout << "." << std::endl;
        pool.runFor(100ms);
    }
}
