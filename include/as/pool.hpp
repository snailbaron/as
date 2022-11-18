#pragma once

namespace as {

class Task {
public:
    virtual ~Task() = default;
};

class HeteroPool {
public:
    void tick();
};

} // namespace as
