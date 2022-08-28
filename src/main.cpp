#include <iostream>
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
// #include "Ticker.h"
#include "DHT.h"

#define FASTLED_ALLOW_INTERRUPTS 0
// #define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
// #define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// #define FASTLED_ESP8266_D1_PIN_ORDER
#include "FastLED.h"

// 
// 
// 

#define ESP_ID 31
#define ESP_NAME_ID "esp31"
#define ESP_TAG "baie_vitre"

#define IS_LED true
#define IS_DHT false

#define NUM_LEDS    10
#define BRIGHTNESS  16 // MAX 255
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define TOPIC_LED_ID "esp/led/31/+"
#define TOPIC_LED_ROOM "esp/led/mainroom/+"
#define TOPIC_LED_G "esp/led/__global__/+"

#define TOPIC_LED_ACTION_ON "/on"
#define TOPIC_LED_ACTION_OFF "/off"
#define TOPIC_LED_ACTION_COLOR "/color"
#define TOPIC_LED_ACTION_BRIGHTNESS "/brightness"

#define TOPIC_DHT_ID_TEMP "esp/dht/31/temp"

#define TOPIC_ESP_PING "ping/esp"
#define TOPIC_ESP_LED_PONG_ID "pong/esp/led/mainroom/31"
#define TOPIC_ESP_DHT_PONG_ID "pong/esp/dht/mainroom/31"

// 
// 
// 

// D2 is builtin led
#define PIN_LED D1
#define PIN_DHT D4

#define QOS0 0 // at most once
#define QOS1 1 // at least once
#define QOS2 2 // exactly once

// 
// 
// 

char *wifiSsid = "changeme";
char *wifiPassword = "changeme";
IPAddress wifiStaticIp(192, 168, 0, ESP_ID);
IPAddress wifiGatewayIp(192, 168, 0, 1);
IPAddress wifiSubnet(255, 255, 255, 0);
IPAddress wifiDns(8, 8, 8, 8);

char *mqttHost = "192.168.0.11"; // need to stay char*, string.c_str not working
uint16_t mqttPort = 1883;

AsyncMqttClient mqttClient;
AsyncWebServer webServer(80); // /!\ declare this variable AFTER mqttClient or else asyncWebServer is not reachable by browsers ...
AsyncWebSocket ws("/ws");

CRGB leds[NUM_LEDS];

DHT dht(PIN_DHT, DHT22);
char* dhtTemp = "0.00";
char* dhtHumidity = "0.00";

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

char* charConcat(char* str1, char* str2) {
    char* concatenated = (char*)malloc(strlen(str1)+strlen(str2));
    strcpy(str1, concatenated);
    strcat(concatenated, str2);
    return concatenated;
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
            if(IS_DHT) {
                mqttClient.publish(TOPIC_ESP_DHT_PONG_ID, QOS0, false, "");
                mqttClient.publish("esp/dht/31/temp", QOS0, false, dhtTemp);
                mqttClient.publish("esp/dht/31/humidity", QOS0, false, dhtHumidity);
            }
        }

        if(isTopicMatching(_topic, "esp/led", TOPIC_LED_ACTION_ON)) {
            Serial.println("turning on");
            FastLED.showColor(CRGB(255,255,255));
            // ws.textAll("turning on");
        }
        if(isTopicMatching(_topic, "esp/led", TOPIC_LED_ACTION_OFF)) {
            Serial.println("turning off");
            // ws.textAll("turning off");
            FastLED.clear();
            FastLED.show();
        }
        if(isTopicMatching(_topic, "esp/led", TOPIC_LED_ACTION_COLOR)) {
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

            mqttClient.subscribe(TOPIC_LED_ROOM, QOS0);
            mqttClient.subscribe(TOPIC_LED_ID, QOS0);
            mqttClient.subscribe(TOPIC_LED_G, QOS0);
        }

        // if(IS_DHT) {
        //     mqttClient.subscribe(TOPIC_DHT_ID_TEMP, QOS0);
        // }
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
    FastLED.showColor(CRGB(255,255,255));

    Serial.println("[setupLeds] complete.");
}

// 
// DHT
//
void setupDHT() {
    dht.begin();
}

// can't be called in interrupts
void dhtRead() {
    if(IS_DHT) {
        float tempCelcius = dht.readTemperature();
        float humidity = dht.readHumidity();

        if (isnan(humidity) || isnan(tempCelcius)) {
            Serial.println("Failed to read from DHT sensor!");
            return;
        } else {
            char c[50];
            char cc[50];

            sprintf(c, "%.2f", tempCelcius);
            sprintf(cc, "%.2f", humidity);

            dhtTemp = c;
            dhtHumidity = cc;

            Serial.printf("read temperature: %.2f, humidity: %.2f\n", tempCelcius, humidity);
            Serial.printf("saved temperature: %.2f, humidity: %.2f\n", dhtTemp, dhtHumidity);
        }
    }
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

    setupWifi();
    setupServer();
    setupMQTT();
    if(IS_LED) {
        setupLeds();
    }
    if(IS_DHT) {
        setupDHT();
        dhtRead();
    }

    Serial.println("[setup] complete.");
}

void loop() {
    delay(1000*60*2); //2 minutes
    dhtRead();
}
