#include "ApplicationState.h"

ApplicationState::ApplicationState() : display_is_on(true), weatherCondition(common::WeatherCondition::HeavyRainy) {}

void ApplicationState::registerFn(updater_fn fn, member_t member, uint8_t size) {
    updaters_.push_back(std::make_tuple(fn, member, size));
}

void ApplicationState::runUpdaters() {
    for (auto updater : updaters_) {
        auto new_data = (std::get<0>(updater))();
        if (new_data.second) {
            memcpy(std::get<1>(updater), new_data.first, std::get<2>(updater));
        }
    }
}