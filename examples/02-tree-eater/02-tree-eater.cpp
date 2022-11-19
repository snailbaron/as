#include <as.hpp>

#include <SDL.h>
#include <SDL_main.h>

#include <chrono>
#include <cmath>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace co = as::coro;

using Clock = std::chrono::high_resolution_clock;
using namespace std::chrono_literals;

struct Vector {
    float x = 0.f;
    float y = 0.f;
};

Vector operator+(const Vector& lhs, const Vector& rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

Vector operator-(const Vector& lhs, const Vector& rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

Vector operator*(const Vector& v, float s)
{
    return {v.x * s, v.y * s};
}

float len(const Vector& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

Vector middle(const Vector& lhs, const Vector& rhs)
{
    return {(lhs.x + rhs.x) / 2, (lhs.y + rhs.y) / 2};
}

class Random {
public:
    bool coin() { return _coinDistribution(_randomEngine); }

    Vector vector()
    {
        return {_pointDistribution(_randomEngine),
            _pointDistribution(_randomEngine)};
    }

private:
    std::random_device _randomDevice;
    std::minstd_rand _randomEngine;
    std::uniform_int_distribution<int> _coinDistribution{0, 1};
    std::uniform_real_distribution<float> _pointDistribution{0.f, 10.f};
};

Random& rnd()
{
    static Random r;
    return r;
}

struct World {
    Vector eater;
    std::vector<Vector> trees;
};

template <class Rep, class Period>
co::Task wait(const std::chrono::duration<Rep, Period>& duration)
{
    auto end = Clock::now() + duration;
    while (Clock::now() < end) {
        co_await std::suspend_always{};
    }
}

co::Task moveTo(Vector& object, const Vector& target)
{
    static const float distancePerSecond = 1.f;

    const auto initialPosition = object;
    const auto start = Clock::now();
    const auto distanceToPass = len(target - object);
    for (;;) {
        auto secondsPassed =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                Clock::now() - start).count();
        auto passedDistance = secondsPassed * distancePerSecond;
        if (passedDistance >= distanceToPass) {
            break;
        }

        object = initialPosition +
            (target - initialPosition) * (passedDistance / distanceToPass);
        co_await std::suspend_always{};
    }
    object = target;
}

co::Task jump(Vector& object)
{
    auto initialPosition = object;
    auto start = Clock::now();
    auto end = start + 0.5s;
    for (auto now = Clock::now(); now < end; now = Clock::now()) {
        auto t =
            std::chrono::duration_cast<std::chrono::duration<float>>(
                now - start).count();
        object.y = initialPosition.y - 2 * t * (0.5f - t);
        co_await std::suspend_always{};
    }
    object = initialPosition;
}

co::Task think(World& world)
{
    while (world.trees.size() >= 2) {
        size_t t1 = 0;
        size_t t2 = 1;
        if (len(world.eater - world.trees.at(t1)) >
                len(world.eater - world.trees.at(t2))) {
            std::swap(t1, t2);
        }

        for (size_t t = 2; t < world.trees.size(); t++) {
            if (len(world.eater - world.trees.at(t)) <
                    len(world.eater - world.trees.at(t1))) {
                t2 = t1;
                t1 = t;
            } else if (len(world.eater - world.trees.at(t)) <
                    len(world.eater - world.trees.at(t2))) {
                t2 = t;
            }
        }

        auto m = middle(world.trees.at(t1), world.trees.at(t2));

        co_await moveTo(world.eater, m);
        co_await jump(world.eater);
        if (rnd().coin()) {
            co_await moveTo(world.eater, world.trees.at(t1));
            std::swap(world.trees.at(t1), world.trees.back());
        } else {
            co_await moveTo(world.eater, world.trees.at(t2));
            std::swap(world.trees.at(t2), world.trees.back());
        }
        world.trees.resize(world.trees.size() - 1);

        co_await jump(world.eater);
        co_await jump(world.eater);
        co_await jump(world.eater);
        co_await wait(1s);
    }

    if (world.trees.size() == 1) {
        co_await moveTo(world.eater, world.trees.front());
        world.trees.clear();
        co_await jump(world.eater);
    }
}

void sdlCheck(int statusCode)
{
    if (statusCode != 0) {
        throw std::runtime_error{SDL_GetError()};
    }
}

int main()
{
    try {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        sdlCheck(SDL_CreateWindowAndRenderer(1024, 768, 0, &window, &renderer));

        auto world = World{};
        for (int i = 0; i < 10; i++) {
            world.trees.push_back(rnd().vector());
        }

        auto pool = co::Pool{};
        pool << think(world);

        bool done = false;
        while (!done) {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) {
                    done = true;
                }
            }

            pool.tick();

            sdlCheck(SDL_SetRenderDrawColor(renderer, 30, 40, 30, 255));
            sdlCheck(SDL_RenderClear(renderer));

            auto toScreen = [] (const Vector& p) {
                return Vector{10 + p.x * 1004 / 10, 10 + p.y * 748 / 10};
            };

            for (const auto& tree : world.trees) {
                auto p = toScreen(tree);

                std::vector<SDL_Vertex> vertices{
                    SDL_Vertex{.position = {p.x - 5, p.y + 5}, .color = {0, 100, 0, 255}, .tex_coord = {}},
                    SDL_Vertex{.position = {p.x + 5, p.y + 5}, .color = {0, 100, 0, 255}, .tex_coord = {}},
                    SDL_Vertex{.position = {p.x, p.y - 10}, .color = {0, 100, 0, 255}, .tex_coord = {}},
                };
                sdlCheck(SDL_RenderGeometry(
                    renderer, nullptr, vertices.data(), 3, nullptr, 0));
            }

            {
                sdlCheck(SDL_SetRenderDrawColor(renderer, 200, 200, 200, 250));
                auto p = toScreen(world.eater);
                SDL_FRect rect{.x = p.x - 5, .y = p.y - 5, .w = 10, .h = 10};
                sdlCheck(SDL_RenderFillRectF(renderer, &rect));
            }

            SDL_RenderPresent(renderer);
        }

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        SDL_Quit();
    } catch (const std::exception& e) {
        return 1;
    }
}
