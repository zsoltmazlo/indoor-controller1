#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <cmath>
#include <iostream>
#include <string>
#include "Configuration.h"
#include "debug.h"
#include "tasks/tasks.h"

using namespace tasks::queues;

// array must be defined somewhere to allocate memory for them
constexpr uint8_t Configuration::Led::potmeter_pin[2];
constexpr uint8_t Configuration::Led::pwm_pin[2];

void setup() {
    debug::init();

    pinMode(Configuration::led_pin, OUTPUT);
    digitalWrite(Configuration::led_pin, Configuration::is_led_active_low ? LOW : HIGH);

    Serial.begin(Configuration::serial_baud);
    Serial.setDebugOutput(true);

    // Local time zone information
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    // start button task immediately
    xTaskCreate(tasks::button, "button_task", 2048, NULL, 1, NULL);

    Connection::initialize(Configuration::mqtt_broker, Configuration::mqtt_port, Configuration::client_id);
    Connection::instance->setDebugStream(&Serial);

    delay(100ms);
    debug::printf("MAIN | MCU started\n     | Version: %s\n", Configuration::firmware_version);

    // start mqtt connection listener thread
    xTaskCreatePinnedToCore(tasks::connection, "connection_task", 8192, NULL, 1, NULL, 1);

    if (Configuration::Interfaces::display) {
        // start task for ssd1306 display
        xTaskCreatePinnedToCore(tasks::display, "display_task", 8192, NULL, 1, NULL, 0);
        delay(Configuration::logo_display_time);

        // send weather update message in a separated task
        xTaskCreatePinnedToCore(tasks::weather_update, "weather_updater_task", 2048, NULL, 1, NULL, 1);
    }

    if (Configuration::Interfaces::ledcontrol) {
        // starting tasks to read analog pin for leds
        xTaskCreatePinnedToCore(tasks::ledcontroller, "led0", 8192, (void *)0, 1, NULL, 1);
        xTaskCreatePinnedToCore(tasks::ledcontroller, "led1", 8192, (void *)1, 1, NULL, 1);
    }

    if (Configuration::Interfaces::temperature) {
        xTaskCreatePinnedToCore(tasks::temperature, "temperature", 8192, NULL, 1, NULL, 1);
    }

    // wait 5sec to start every task what should have been started
    vTaskDelay(5s);
}

void loop() {
    // main task should wait many many minutes
    vTaskDelay(10min);
}
