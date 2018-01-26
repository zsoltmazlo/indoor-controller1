#pragma once

#include <SSD1306.hpp>
#include <NTPClient.h>
#include <WiFi.h>
#include <vector>
#include <tuple>
#include "WeatherCondition.h"

namespace tasks {

namespace queues {

extern QueueHandle_t ledQueue[2];

extern QueueHandle_t temperatureQueue;

extern QueueHandle_t displayQueue;

}  // namespace queues

class Connection {
   public:
    void operator()(void* args);
};

struct FrameSelectorMessage {
    uint8_t frameIndex;
    uint64_t interval; 
};

class Display {
   public:
    Display();

    void operator()(void* args);

   private:

    using MetricRow = std::vector<std::tuple<std::string, float, std::string>>;

    void displayMetricRow(const MetricRow& elements, uint8_t yOffset, uint8_t xOffset = 0, uint8_t floatPrecision = 1);

    std::string showWeatherIcon(const common::WeatherCondition& condition, uint8_t x = 0, uint8_t y = 0);

    void overviewFrame();

    void weatherFrame();

    void forecastFrame();

    SSD1306<3> display_;

};

class DisplayFrame {

};

// wrapper functions need to be used because of the freeRTOS codebase: (sadly) only c-style function pointers could be
// used in the task creation, Functors or any other std::function instances are not.
void display(void* args);

void connection(void* args);

void ledcontroller(void* args);

void temperature(void* args);

void weather_update(void* args);

}  // namespace tasks