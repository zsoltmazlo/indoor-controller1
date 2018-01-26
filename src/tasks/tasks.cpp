#include "tasks.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cmath>
#include "Configuration.h"
#include "debug.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Configuration.h"
#include "DS18B20.hpp"

void tasks::display(void* args) {
    auto task = tasks::Display{};
    task(args);
}

void tasks::connection(void* args) {
    auto task = tasks::Connection{};
    task(args);
}

void tasks::ledcontroller(void* args) {
    // argument will hold the led index which could use to get the pin number
    // and the queue as well
    uint32_t index = (uint32_t)args;
    uint8_t pin = Configuration::Led::potmeter_pin[index];
    uint8_t pwm = Configuration::Led::pwm_pin[index];

    debug::printf("LED%d | Led control task is started.\n      POTMETER pin: %d\n      PWM pin: %d\n", index, pin, pwm);
    pinMode(pin, INPUT);

    // start PWM handling (index could use as the analog channel)
    ledcSetup(index, Configuration::Led::pwm_frequency, Configuration::Led::pwm_resolution);
    ledcAttachPin(pwm, index);

    uint16_t previous_measurement = 10000, current_measurement;
    tasks::FrameSelectorMessage fsm;
    fsm.frameIndex = 0;
    fsm.interval = 5s;
    float f;
    constexpr uint16_t res = 1 << Configuration::Led::pwm_resolution;

    // subscribe for topic too (it is sad that std::to_string is not an option)
    char topic[16];
    sprintf(topic, Configuration::Topics::ledcontrol, index);
    ::Connection::instance->subscribe(topic, [&](const std::string& message) {
        debug::printf("LED%d | message received! %s\n", index, message.c_str());
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& object = jsonBuffer.parse(message.c_str());

        if (object.containsKey("value")) {
            f = object["value"].as<float>();
            xQueueSend(tasks::queues::ledQueue[index], &f, (TickType_t)10);
            xQueueSend(tasks::queues::displayQueue, &fsm, (TickType_t)10);
            ledcWrite(index, (uint16_t)(4096 * f / 100.0));
        }
    });

    for (;;) {
        current_measurement = analogRead(pin);
        if (std::abs(current_measurement - previous_measurement) > Configuration::Led::potmeter_threshold) {
            f = (float)current_measurement / res * 100.0;
            previous_measurement = current_measurement;

            // sending message
            xQueueSend(tasks::queues::ledQueue[index], &f, (TickType_t)10);
            xQueueSend(tasks::queues::displayQueue, &fsm, (TickType_t)10);

            // also write PWM value to the output as well
            ledcWrite(index, current_measurement);
            vTaskDelay(50ms);
        } else {
            // task can rest until 100ms
            vTaskDelay(100ms);
        }
    }
}

extern NTPClient ntpClient;

void tasks::temperature(void* args) {
    debug::printf("TEMP | Task started\n");

    float temp;
    for (;;) {
        // measure sensor value - also, we need to handle onewire communication as a critical section
        auto measurement =
            readSensorValue<Configuration::Temperature::sensor_pin, Configuration::Temperature::measurement_count,
                            Configuration::Temperature::measurement_resolution>();
        if (measurement.second) {
            temp = measurement.first;
            xQueueSend(tasks::queues::temperatureQueue, &temp, (TickType_t)10);

            // and send as an mqtt message as well
            auto ts = String(ntpClient.getEpochTime()) + "000";
            StaticJsonBuffer<200> jsonBuffer;
            JsonObject& root = jsonBuffer.createObject();
            root["timestamp"] = ts.c_str();
            root["temperature"] = temp;
            String output;
            root.printTo(output);
            ::Connection::instance->publish(Configuration::Topics::sensor, output.c_str());
        }

        // measurement every 5min
        vTaskDelay(Configuration::Temperature::measurement_interval);
    }
}

void tasks::weather_update(void* args) {
    static char message[100];
    sprintf(message, "{\"client\": \"%s\", \"forecast_days\": 3}", Configuration::client_id);

    for (;;) {
        if (::Connection::instance->isConnected()) {
            ::Connection::instance->publish(Configuration::Topics::weather_req, message);
        }
        // check weather information every 5 minutes
        vTaskDelay(5min);
    }
}

QueueHandle_t tasks::queues::ledQueue[2];

QueueHandle_t tasks::queues::temperatureQueue;

QueueHandle_t tasks::queues::displayQueue;