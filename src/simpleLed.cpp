// #include <iostream>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <ESP8266WiFi.h>
// #include <ESPAsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include <AsyncMqttClient.h>
// #include "ArduinoJson.h"
// #include "AsyncJson.h"
// #include "LittleFS.h"
// // #include "Ticker.h"
// #include "DHT.h"

// // #define FASTLED_ALLOW_INTERRUPTS 0
// // #define FASTLED_INTERRUPT_RETRY_COUNT 0
// #define FASTLED_ESP8266_RAW_PIN_ORDER
// // #define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// // #define FASTLED_ESP8266_D1_PIN_ORDER
// #include "FastLED.h"

// #define NUM_LEDS    10
// #define BRIGHTNESS  255 // MAX 255
// #define LED_TYPE    WS2812B
// #define COLOR_ORDER GRB

// // D2 is builtin led
// #define PIN_LED D1

// CRGB leds[NUM_LEDS];

// //
// // LEDs
// //
// void setupLeds() {
//     Serial.println("[setupLeds] init");

//     FastLED.addLeds<LED_TYPE, PIN_LED, COLOR_ORDER>(leds, NUM_LEDS);
//     FastLED.setBrightness(25);

//     Serial.println("[setupLeds] complete.");
// }


// //
// // MAIN
// //
// void setup() {

//     Serial.begin(9600);
//     Serial.println();
//     Serial.println("[setup] init");

//     setupLeds();

//     Serial.println("[setup] complete.");
// }

// void loop() {
//     delay(1000);
//     Serial.println(" r");
//     FastLED.showColor(CRGB(255,0,0));
//     delay(1000);
//     Serial.println(" g");
//     FastLED.showColor(CRGB(0,255,0));
//     delay(1000);
//     Serial.println(" b");
//     FastLED.showColor(CRGB(0,0,255));
// }
