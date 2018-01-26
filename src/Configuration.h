#pragma once

#include <WiFi.h>
#include <pins_arduino.h>
#include <stdint.h>
#include <MqttConnection.hpp>

#define CLIENT_ID "chthonious"  // dictys cyllarus chthonious

using Connection = MqttConnection<4, WiFiClient>;

constexpr unsigned long long operator"" khz(unsigned long long khz) { return khz * 1000; }

constexpr unsigned long long operator"" min(unsigned long long s) { return s * 1000 * 60; }

constexpr unsigned long long operator"" s(unsigned long long s) { return s * 1000; }

constexpr unsigned long long operator"" ms(unsigned long long ms) { return ms; }

constexpr unsigned long long operator"" _bit(unsigned long long bit) { return bit; }

struct Configuration {
    static constexpr char* firmware_version = "1.0.0b";
    static constexpr char* client_id = CLIENT_ID;
    static constexpr char* ssid = "lsmx49";
    static constexpr char* pwd = "";
    static constexpr bool show_mac_address = false;
    static constexpr char* mqtt_broker = "192.168.1.3";
    static constexpr uint16_t mqtt_port = 1883;
    static constexpr uint8_t led_pin = LED_BUILTIN;
    static constexpr bool is_led_active_low = false;
    static constexpr uint16_t serial_baud = 9600;
    static constexpr bool lowenergy_mode = true;

    static constexpr uint16_t logo_display_time = 4s;

    struct Topics {
        static constexpr char* time_req = "time/request";
        static constexpr char* time_res = CLIENT_ID "/time";
        static constexpr char* sensor = CLIENT_ID "/sensor";
        static constexpr char* remote = CLIENT_ID "/remote";
        static constexpr char* weather = CLIENT_ID "/weather";
        static constexpr char* weather_req = "weather/request";
        static constexpr char* ledcontrol = CLIENT_ID "/led%d";
    };

    struct Interfaces {
        static constexpr bool temperature = true;
        static constexpr bool tvremote = false;
        static constexpr bool display = true;
        static constexpr bool ledcontrol = true;
    };

    struct Display {
        static constexpr int8_t scl_pin = 4;  //-1; //4;
        static constexpr int8_t sda_pin = 5;  //-1; // 5;
        static constexpr uint32_t i2c_frequency = 400khz;
        static constexpr uint16_t frame_change_interval = 10s;
        static constexpr uint16_t frame_update_interval = 100ms;
    };

    struct Temperature {
        static constexpr uint8_t sensor_pin = 16;
        static constexpr uint32_t measurement_interval = 10min;
        static constexpr uint8_t measurement_count = 5;
        static constexpr uint8_t measurement_resolution = 12_bit;
    };

    struct Led {
        static constexpr uint16_t pwm_frequency = 10khz;
        static constexpr uint16_t pwm_resolution = 12_bit;
        static constexpr uint8_t pwm_pin[2] = {14, 12};

        static constexpr uint16_t potmeter_threshold = 100;
        static constexpr uint8_t potmeter_pin[2] = {36, 39};
    };
};
