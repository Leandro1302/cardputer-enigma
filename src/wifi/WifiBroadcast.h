#pragma once
#include "Broadcast.h"

class WifiBroadcast final : public app::Broadcast {
public:
    bool start(const std::string& ssid) override;
    bool stop() override;
    app::BroadcastStatus status() const override;
private:
    bool initialized_ = false;
    bool started_ = false;
};
