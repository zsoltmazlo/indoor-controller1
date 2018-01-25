#include "Weather.h"

const std::unordered_map<std::string, common::WeatherCondition> common::Weather::map = {
    {"Cloudy", WeatherCondition::Cloudy},
    {"Partly Cloudy", WeatherCondition::Cloudy},
    {"Mostly Cloudy", WeatherCondition::Cloudy},

    {"Showers", WeatherCondition::HeavyRainy},
    {"Scattered Showers", WeatherCondition::Rainy},
    {"Rain", WeatherCondition::Rainy},

    {"Rain And Snow", WeatherCondition::Snowy},
    {"Snow", WeatherCondition::Snowy},
    {"Snow Showers", WeatherCondition::Snowy},

    {"Clear", WeatherCondition::Sunny},
    {"Sunny", WeatherCondition::Sunny},
    {"Mostly Clear", WeatherCondition::SunnyButCloudy},
    {"Mostly Sunny", WeatherCondition::SunnyButCloudy},
    {"Scattered Thunderstorms", WeatherCondition::Thunder},
    {"Thunderstorm", WeatherCondition::Thunder}

};  // Breezy: windy