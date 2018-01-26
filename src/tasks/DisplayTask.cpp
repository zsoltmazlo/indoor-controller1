#include "ApplicationState.h"
#include "Configuration.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logo.h"
#include "tasks.h"
#include "debug.h"

#include <ctime>

using namespace tasks::queues;
using namespace common;

ApplicationState appState;

WiFiUDP udpClient;
NTPClient ntpClient{udpClient, "0.hu.pool.ntp.org", 0};

static std::string formatDate(const char* fmt, uint64_t epoch) {
    char buf[20];
    time_t rawtime = static_cast<time_t>(epoch);
    auto t = std::localtime(&rawtime);
    std::strftime(buf, 20, fmt, t);
    return buf;
}

tasks::Display::Display() : display_{Configuration::Display::scl_pin, Configuration::Display::sda_pin, 16} {}

void tasks::Display::operator()(void* args) {
   debug::printf("DISP | Task started.\n");

    // before starting the display, we should initialize queues and register them
    for (uint8_t i = 0; i < 2; ++i) {
        ledQueue[i] = xQueueCreate(10, sizeof(float));
        appState.registerQueue<float>(ledQueue[i], &(appState.led[i]), sizeof(float));
    }
    displayQueue = xQueueCreate(10, sizeof(tasks::FrameSelectorMessage));
    temperatureQueue = xQueueCreate(10, sizeof(float));
    appState.registerQueue<float>(temperatureQueue, &(appState.temperature), sizeof(float));

    // by default, we'll generate the high voltage from the 3.3v line internally
    display_.begin(Configuration::Display::i2c_frequency, SSD1306_SWITCHCAPVCC);
    display_.clearDisplay();
    display_.dim(true);
    display_.drawBitmap(0, 0, logo, LOGO_GLCD_WIDTH, LOGO_GLCD_HEIGHT, 1);
    display_.display();

    // connect to network
    delay(Configuration::logo_display_time);
    display_.clearDisplay();
    ntpClient.begin();

    // also we should subscribe to the weather topic to get information
    ::Connection::instance->subscribe(Configuration::Topics::weather, [&](const std::string& msg) {
        debug::printf("WTHR | New information arrived!\n");
        appState.weather = common::Weather::parseFromJson(msg);
        debug::printf("WTHR | Current temperature: %0.2f\n", appState.weather.getTemperature());
    });

    // OVERVIEW FRAME
    display_.setFrame(0, [&](SSD1306<3>& display) {
        display.clearDisplay();
        display.showIndicators();
        // display_.centerredText(64, 0, "Overview");

        // clang-format off
        Display::MetricRow leds = {
            std::make_tuple("LED1", appState.led[0], "%"),
            std::make_tuple("LED2", appState.led[1], "%")
        };

        Display::MetricRow temp = {
            std::make_tuple("TEMP", appState.temperature, "C")
        };
        // clang-format on
        displayMetricRow(leds, 34, 0, 0);
        displayMetricRow(temp, 34, 88, 1);

        // display date and time
        display.text(2, 12, "%s", formatDate("%b %e", ntpClient.getEpochTime()).c_str());
        display.text(2, 24, "%s", formatDate("%A", ntpClient.getEpochTime()).c_str());
        display.setTextSize(2);
        display.text(68, 14, "%s", formatDate("%H.%M", ntpClient.getEpochTime()).c_str());
        display.setTextSize(1);
    });

    // WEATHER FRAME
    display_.setFrame(1, [&](SSD1306<3>& display) {
        display.clearDisplay();
        display.showIndicators();
        // display_.centerredText(64, 0, "Weather");

        // display weather mini logo
        auto title = showWeatherIcon(appState.weather.getWeatherCondition(), 8, 16);

        auto space_position = appState.weather.getCondition().find(" ");
        if (space_position != std::string::npos) {
            // if there is a space in the condition, the split it at that position
            auto part1 = appState.weather.getCondition().substr(0, space_position);
            auto part2 = appState.weather.getCondition().substr(space_position + 1);
            display.text(48, 16, "%s", part1.c_str());
            display.text(48, 28, "%s", part2.c_str());
        } else {
            display.text(48, 16, "%s", appState.weather.getCondition().c_str());
        }

        display.text(48, 40, "%s", formatDate("%H:%M", appState.weather.getSunrise()).c_str());
        display.text(88, 40, "%s", formatDate("%H:%M", appState.weather.getSunset()).c_str());
        display.text(48, 52, "%drH", appState.weather.getHumidity());
        display.text(88, 52, "%0.0fmB", appState.weather.getPressure());
        display.centerredText(24, 52, "%0.1fC", appState.weather.getTemperature());
    });

    // FORECAST FRAME
    display_.setFrame(2, [&](SSD1306<3>& display) {
        display.clearDisplay();
        display.showIndicators();
        // display_.centerredText(64, 0, "Forecast");

        for (uint8_t i = 0; i < appState.weather.getForecast().size(); ++i) {
            auto day = formatDate("%a", appState.weather.getForecast()[i].date);
            display.centerredText(i * 40 + 24, 10, "%s", day.c_str());
            showWeatherIcon(appState.weather.getForecast()[i].weatherCondition, i * 32 + (i + 1) * 8, 20);
            display.centerredText(i * 40 + 24, 54, "%0.1f", appState.weather.getForecast()[i].high);
        }
    });

    uint16_t count = Configuration::Display::frame_change_interval / Configuration::Display::frame_update_interval;
    TickType_t lastUpdate = xTaskGetTickCount();
    FrameSelectorMessage frameSelectorMessage;

    for (;;) {
        while (count > 0) {
            // update data
            ntpClient.update();
            appState.runUpdaters();

            // check if there is a message to show a frame now and which one if there is
            if (xQueueReceive(displayQueue, (void*)&frameSelectorMessage, (TickType_t)10)) {
                display_.setCurrentFrame(frameSelectorMessage.frameIndex);
                count = frameSelectorMessage.interval / Configuration::Display::frame_update_interval;
            }

            // run display frames to be the most fresh frame in the buffers
            display_.updateFrames();

            vTaskDelayUntil(&lastUpdate, Configuration::Display::frame_update_interval);
            --count;
        }

        // if count is 0, then we need to change the frame
        count = Configuration::Display::frame_change_interval / Configuration::Display::frame_update_interval;
        display_.changeFrame();
    }
}

void tasks::Display::displayMetricRow(const MetricRow& elements, uint8_t yOffset, uint8_t xOffset,
                                      uint8_t floatPrecision) {
    char buffer[16];
    display_.setTextSize(1);

    for (uint8_t i = 0; i < elements.size(); ++i) {
        // display graphic elements
        display_.fillRect(xOffset + i * 44, yOffset, 40, 12, 1);
        display_.drawRect(xOffset + i * 44, yOffset, 40, 28, 1);

        // display element name
        display_.setTextColor(0, 1);
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%s", std::get<0>(elements[i]).c_str());

        if (strlen(buffer) < 9) {
            display_.setCursor(xOffset + i * 44 + 21 - strlen(buffer) * 3, yOffset + 3);
        } else {
            display_.setCursor(xOffset + i * 44 + 16, yOffset + 3);
        }
        display_.printf("%s", buffer);

        // display element value and unit
        display_.setTextColor(1, 0);
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "%s%s", String(std::get<1>(elements[i]), floatPrecision).c_str(),
                std::get<2>(elements[i]).c_str());

        if (strlen(buffer) < 9) {
            display_.setCursor(xOffset + i * 44 + 21 - strlen(buffer) * 3, yOffset + 16);
        } else {
            display_.setCursor(xOffset + i * 44 + 16, yOffset + 16);
        }
        display_.printf("%s", buffer);
    }
}

std::string tasks::Display::showWeatherIcon(const WeatherCondition& condition, uint8_t x, uint8_t y) {
    switch (condition) {
        case WeatherCondition::ClearNight:
            display_.drawBitmap(x, y, icon_weather_night1, 32, 32, 1);
            return "clear";

        case WeatherCondition::Cloudy:
            display_.drawBitmap(x, y - 3, icon_weather_cloudy, 32, 32, 1);
            return "cloudy";

        case WeatherCondition::CloudyNight:
            display_.drawBitmap(x, y, icon_weather_night2, 32, 32, 1);
            return "cloudy";

        case WeatherCondition::HeavyRainy:
            display_.drawBitmap(x, y, icon_weather_rainy2, 32, 32, 1);
            return "heavy rain";

        case WeatherCondition::HeavySnowy:
            display_.drawBitmap(x, y, icon_weather_snowy2, 32, 32, 1);
            return "heavy snow";

        case WeatherCondition::Rainy:
            display_.drawBitmap(x, y, icon_weather_rainy1, 32, 32, 1);
            return "rainy";

        case WeatherCondition::Snowy:
            display_.drawBitmap(x, y, icon_weather_snowy1, 32, 32, 1);
            return "snowy";

        case WeatherCondition::Sunny:
            display_.drawBitmap(x, y, icon_weather_sunny, 32, 32, 1);
            return "sunny";

        case WeatherCondition::SunnyButCloudy:
            display_.drawBitmap(x, y, icon_weather_sunny_cloudy, 32, 32, 1);
            return "sunny/ cloudy";

        case WeatherCondition::Thunder:
            display_.drawBitmap(x, y, icon_weather_thunder, 32, 32, 1);
            return "thunder";
    }
}
