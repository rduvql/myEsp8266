#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "LittleFS.h"
#include "Ticker.h"

#define FASTLED_ALLOW_INTERRUPTS 0
// #define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
// #define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// #define FASTLED_ESP8266_D1_PIN_ORDER
#include "FastLED.h"

#define ESP_ID 31
#define ESP_NAME_ID "esp31"
#define ESP_TAG "baie_vitre"

#define IS_LED true
#define IS_TEMP_SENSOR false

#define NUM_LEDS    2
#define BRIGHTNESS  10
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define TOPIC_LED_ACTION "esp/led/mainroom/action/+"
#define TOPIC_LED_ID_ACTION "esp/led/31/action/+"
#define TOPIC_LED_G_ACTION "esp/led/__global__/action/+"

#define TOPIC_LED_COLOR "esp/led/mainroom/color"
#define TOPIC_LED_ID_COLOR "esp/led/31/color"
#define TOPIC_LED_G_COLOR "esp/led/__global__/color"

#define TOPIC_ESP_PING "ping/esp"
#define TOPIC_ESP_LED_PONG_ID "pong/esp/led/mainroom/31"
#define TOPIC_ESP_TEMP_SENSOR_PONG_ID "pong/esp/temp_sensor/mainroom/31"

// 
// 
// 

// D2 is builtin led
#define PIN_LED D1

#define QOS0 0 // at most once
#define QOS1 1 // at least once
#define QOS2 2 // exactly once

// 
// 
// 

char *wifiSsid = "";
char *wifiPassword = "";
IPAddress wifiStaticIp(192, 168, 0, ESP_ID);
IPAddress wifiGatewayIp(192, 168, 0, 1);
IPAddress wifiSubnet(255, 255, 255, 0);
IPAddress wifiDns(8, 8, 8, 8);

char *mqttHost = "192.168.0.11"; // need to stay char*, string.c_str not working
uint16_t mqttPort = 1883;

AsyncMqttClient mqttClient;
AsyncWebServer webServer(80); // /!\ declare this variable AFTER mqttClient or else asyncWebServer is not reachable by browsers ...
AsyncWebSocket ws("/ws");

Ticker ticker;

CRGB leds[NUM_LEDS];
CRGB lastCRGB(0,0,0);

//
// UTILS
//

static bool eq(char *str1, char *str2) {
    return strcmp(str1, str2) == 0;
}
static bool startsWith(const std::string &str, const std::string &prefix) {
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}
static bool endsWith(const std::string &str, const std::string &suffix) {
    return str.size() >= suffix.size() && 0 == str.compare(str.size()-suffix.size(), suffix.size(), suffix);
}
static bool isTopicMatching(const std::string &str, const std::string &prefix, const std::string &suffix) {
    return startsWith(str, prefix) && endsWith(str, suffix);
}

//
// WIFI
//
void setupWifi() {
    Serial.println("[setupWifi] init");

    WiFi.config(wifiStaticIp, wifiGatewayIp, wifiSubnet, wifiDns);
    WiFi.hostname(ESP_NAME_ID);
    WiFi.mode(WIFI_STA);

    WiFi.begin(wifiSsid, wifiPassword);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        delay(500);
        Serial.println("[setupWifi] Failed!");
        return;
    }
    Serial.printf("[setupWifi] IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("[setupWifi] ok");
}


//
// µServer
//
void setupServer() {
    Serial.println("[setupServer] init");

    webServer.serveStatic("/", LittleFS, "/"); //.setDefaultFile("index.html");

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html", false);
    });

    webServer.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {

        // StaticJsonDocument<512> doc;
        // JsonObject object = doc.to<JsonObject>();
        // object["hello"] = "world";

        AsyncJsonResponse *response = new AsyncJsonResponse();
        JsonVariant &json = response->getRoot();
        
        uint32_t hfree; uint16_t hmax; uint8_t hfrag;
        ESP.getHeapStats(&hfree, &hmax, &hfrag);
        json["heap_free"] = hfree;
        json["heap_max"] = hmax;
        json["heap_frag"] = hfrag;
        json["cpu_mhz"] = ESP.getCpuFreqMHz();
        json["ssid"] = WiFi.SSID();

        response->setLength();
        request->send(response);
    });

    webServer.on("/wifi-disconnect", [](AsyncWebServerRequest *request) {
        request->send(200);
        WiFi.disconnect();
    });

    webServer.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    webServer.addHandler(&ws);

    webServer.begin();
    // ws.cleanupClients();

    Serial.println("[setupServer] ok");
}


//
// MQTT
//
void setupMQTT() {
    Serial.println("[setupMQTT] init");

    mqttClient.setServer(mqttHost, mqttPort);
    mqttClient.setClientId(ESP_NAME_ID);

    mqttClient.onMessage( [](char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

        Serial.printf("[mqttClient][onMessage]: topic: %s\n", topic);
        // Serial.println(payload);

        std::string _topic(topic);

        StaticJsonDocument<256> doc;

        doc["is_led"] = true;

        if(_topic == TOPIC_ESP_PING) {
            if(IS_LED) {
                mqttClient.publish(TOPIC_ESP_LED_PONG_ID, QOS0, false, "");
            }
            if(IS_TEMP_SENSOR) {
                mqttClient.publish(TOPIC_ESP_TEMP_SENSOR_PONG_ID, QOS0, false, "");
            }
        }

        if(isTopicMatching(_topic, "esp/led", "/action/on")) {
            Serial.println("turning on");
            // ws.textAll("turning on");
        }
        if(isTopicMatching(_topic, "esp/led", "/action/off")) {
            Serial.println("turning off");
            // ws.textAll("turning off");

            FastLED.clear();
            FastLED.show();
        }
        if(isTopicMatching(_topic, "esp/led", "/color")) {
            Serial.println("changing led color");
            Serial.printf("%s\n", payload); // weird payload with serial. sent 00ffff; received 00ffff@�x␂␌␂�␁␐

            int r, g, b;
            sscanf(payload, "%02x%02x%02x", &r, &g, &b);

            Serial.printf("r: %d, g: %d, b: %d\n", r, g, b);

            FastLED.showColor(CRGB(r,g,b));
        }
    });

    mqttClient.onConnect([](bool sessionPresent) {

        Serial.println("[mqttClient][onConnect] connected to mqtt server");

        mqttClient.subscribe(TOPIC_ESP_PING, QOS0);

        if(IS_LED) {
            Serial.println("[mqttClient][onConnect] subscribing to LED topics");

            mqttClient.subscribe(TOPIC_LED_ACTION, QOS0);
            mqttClient.subscribe(TOPIC_LED_ID_ACTION, QOS0);
            mqttClient.subscribe(TOPIC_LED_G_ACTION, QOS0);

            mqttClient.subscribe(TOPIC_LED_COLOR, QOS0);
            mqttClient.subscribe(TOPIC_LED_ID_COLOR, QOS0);
            mqttClient.subscribe(TOPIC_LED_G_COLOR, QOS0);
        }

        if(IS_TEMP_SENSOR) {
            // TODO
        }
    });

    mqttClient.connect();

    Serial.println("[setupMQTT] ok");
}


// 
// LEDs
// 
void setupLeds() {
    Serial.println("[setupLeds] init");

    FastLED.addLeds<LED_TYPE, PIN_LED, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);

    Serial.println("[setupLeds] complete.");
}


//
// TICKERS
//
void setupTickers() {

    ticker.attach(10, []() {
        Serial.println("[setupTickers] tick.");
    });
}

//
// MAIN
//
void setup() {

    Serial.begin(9600);
    Serial.println();
    Serial.println("[setup] init");

    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    // pinMode(PIN_LED, OUTPUT);
    // pinMode(PIN_PIR, INPUT);
    // pinMode(PIN_DHT, INPUT);

    // dht.begin();

    setupWifi();
    setupServer();
    setupMQTT();
    setupTickers();
    if(IS_LED) {
        setupLeds();
    }

    Serial.println("[setup] complete.");
}

void loop() {
    // delay(1000);
    // ws.textAll("loop");
}

// char* concat(char* str1, char* str2) {
//     char* concatenated = (char*)malloc(strlen(str1)+strlen(str2));
//     strcpy(str1, concatenated);
//     strcat(concatenated, str2);
//     return concatenated;
// }
