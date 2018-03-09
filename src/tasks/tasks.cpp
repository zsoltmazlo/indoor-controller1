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
#include "ButtonEvent.h"

void tasks::display(void* args) {
    auto task = tasks::Display{};
    task(args);
}

void tasks::connection(void* args) {
    auto task = tasks::Connection{};
    task(args);
}

void tasks::button(void* args) {
    debug::printf("BTTN | Button event manager task is started.\n");

    // button events could fired very rapidly, thus we should have a big
    // queue for that events
    tasks::queues::buttonEventQueue = xQueueCreate(100, sizeof(ButtonEvent));

    // we will save 4 samples of the button measurement, and send a message
    // for each element
    uint8_t sample_count = 0;
    uint8_t samples = 0;
    ButtonEvent event;
    ButtonEvent prevEvent;

    pinMode(Configuration::button_pin, INPUT);
    
    TickType_t last_wake_time = xTaskGetTickCount();
    for(;;) {
        prevEvent = event;
        
        // get one sample
        samples <<= 1;
        samples &= 0x0F;
        samples |= digitalRead(Configuration::button_pin);

        switch(samples) {
            case 0x0F:
                event = ButtonEvent::EV_HIGH;
                break;

            case 0x00:
                event = ButtonEvent::EV_LOW;
                break;

            case 0x07:
            case 0x03:
            case 0x01:
                event = ButtonEvent::EV_RISING;
                break;

            case 0x08:
            case 0x0C:
            case 0x0E:
                event = ButtonEvent::EV_FALLING;
                break;
        }

        if( prevEvent != event ) {
            debug::printf("BTTN | Sending event: %d\n", event);
            xQueueSend(tasks::queues::buttonEventQueue, &event, (TickType_t)10);
        }

        vTaskDelayUntil(&last_wake_time, 5ms);
    }
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
    constexpr float fivePercent = (1 << Configuration::Led::pwm_resolution) * 0.05;

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

            f = (float)current_measurement / fivePercent * 5.0;
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

QueueHandle_t tasks::queues::buttonEventQueue;