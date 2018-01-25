#pragma once

#include <ArduinoJson.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "WeatherCondition.h"

#include <Arduino.h>

namespace common {

struct WindInformation {
    float speed;
    uint16_t direction;
};

struct Forecast {
    uint64_t date;
    std::string condition;
    float low;
    float high;
    WeatherCondition weatherCondition;
};

class Weather {
   private:
    WeatherCondition weatherCondition_;
    std::string condition_;
    uint64_t sunset_;
    uint64_t sunrise_;
    uint8_t humidity_;
    float temperature_;
    float visibility_;
    float pressure_;
    WindInformation wind_;
    std::vector<Forecast> forecast_;

    bool isValid_;

    static const std::unordered_map<std::string, WeatherCondition> map;

   public:
    Weather()
        : condition_("no info"),
          sunset_(0),
          sunrise_(0),
          humidity_(0),
          temperature_(0.0),
          visibility_(0.0),
          pressure_(0.0),
          isValid_(false) {
        wind_.speed = 0;
        wind_.direction = 0;
        weatherCondition_ = WeatherCondition::ClearNight;
    }

    static Weather parseFromJson(const std::string& json) {
        DynamicJsonBuffer jsonBuffer{JSON_ARRAY_SIZE(3) + JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(4) +
                                     JSON_OBJECT_SIZE(9)};
        JsonObject& object = jsonBuffer.parseObject(json.c_str());

        Weather w;
        w.condition_ = object["condition"].as<char*>();
        w.sunset_ = object["sunset"].as<unsigned long>();
        w.sunrise_ = object["sunrise"].as<unsigned long>();
        w.humidity_ = object["humidity"].as<uint8_t>();
        w.temperature_ = object["temperature"].as<float>();
        w.visibility_ = object["visibility"].as<float>();
        w.pressure_ = object["pressure"].as<float>();

        WindInformation wind;
        wind.speed = object["wind"]["speed"].as<float>();
        wind.direction = object["wind"]["direction"].as<uint16_t>();
        w.wind_ = wind;

        for (auto fc : object["forecast"].as<JsonArray&>()) {
            Forecast forecast;
            forecast.condition = fc["condition"].as<char*>();
            forecast.date = fc["date"].as<unsigned long>();
            forecast.low = fc["low"].as<float>();
            forecast.high = fc["high"].as<float>();

            auto it = Weather::map.find(forecast.condition);
            if (it != Weather::map.end()) {
                forecast.weatherCondition = it->second;
            } else {
                forecast.weatherCondition = WeatherCondition::ClearNight;
            }

            w.forecast_.push_back(forecast);
        }

        auto it = Weather::map.find(w.condition_);
        if (it != Weather::map.end()) {
            w.weatherCondition_ = it->second;
        }

        w.isValid_ = true;
        return w;
    }

    WeatherCondition getWeatherCondition() const { return weatherCondition_; }

    const std::string& getCondition() const { return condition_; };

    uint64_t getSunset() const { return sunset_; };

    uint64_t getSunrise() const { return sunrise_; };

    uint8_t getHumidity() const { return humidity_; };

    float getTemperature() const { return temperature_; };

    float getVisibility() const { return visibility_; };

    float getPressure() const { return pressure_; };

    const WindInformation& getwind() const { return wind_; };

    const std::vector<Forecast>& getForecast() const { return forecast_; };

    bool isValid() const { return isValid_; }
};

}  // namespace common