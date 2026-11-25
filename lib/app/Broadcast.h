#pragma once
#include <cstddef>
#include <string>

namespace app {
struct BroadcastStatus {
    bool active = false;
    std::string ssid;
    bool open = false;
};

class Broadcast {
public:
    static constexpr std::size_t MaxSsidBytes = 32;
    virtual ~Broadcast() = default;
    virtual bool start(const std::string& ssid) = 0;
    virtual bool stop() = 0;
    virtual BroadcastStatus status() const = 0;
};
}
