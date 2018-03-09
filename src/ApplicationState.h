#pragma once

#include <stdint.h>
#include <functional>
#include <vector>
#include "Configuration.h"
#include "WeatherCondition.h"
#include "Weather.h"

class ApplicationState {
    
    using updater_fn = std::function<std::pair<char*, boolean>(void)>;

    using member_t = void*;

   public:
    ApplicationState();

    // states of the two led controller
    float led[2];

    // store temperature value as well
    float temperature;

    // stores the flag whenever the display is on or not
    bool display_is_on;

    // weather information
    common::WeatherCondition weatherCondition;

    common::Weather weather;

    void registerFn(updater_fn updater, member_t member, uint8_t size);

    template <typename T>
    void registerQueue(QueueHandle_t queue, member_t member, uint8_t size);

    void runUpdaters();

   private:
    std::vector<std::tuple<updater_fn, member_t, uint8_t>> updaters_;
};

template <typename T>
inline void ApplicationState::registerQueue(QueueHandle_t queue, member_t member, uint8_t size) {
    auto fn = [=]() -> std::pair<char*, bool> {
        static T data;
        if (xQueueReceive(queue, (void*)&data, (TickType_t)10)) {
            return std::make_pair(reinterpret_cast<char*>(&data), true);
        }
        return std::make_pair(reinterpret_cast<char*>(&data), false);
    };
    updaters_.push_back(std::make_tuple(fn, member, size));
}
