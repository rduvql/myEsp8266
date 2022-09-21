// pio upgrade --dev when init, restart PC if upload port not detected

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <time.h>
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
#define IS_DHT true

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

// D2 is builtin led
#define PIN_LED D1
#define PIN_DHT D3

#define QOS0 0 // at most once
#define QOS1 1 // at least once
#define QOS2 2 // exactly once

#define TIME_UTC_TZ 1
#define TIME_USE_DST 1 // daylight saving time
#define TIME_NTP_SERVER "pool.ntp.org"


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
// /!\ declare webServer AFTER mqttClient or else asyncWebServer is not working (dunno why)
AsyncWebServer webServer(80); 
AsyncWebSocket ws("/ws");

CRGB leds[NUM_LEDS];
CRGB lastLedRGB(255,255,255);

DHT dht(PIN_DHT, DHT22);
bool successfullRead = false;
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

    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }
    webServer.serveStatic("/", LittleFS, "/"); //.setDefaultFile("index.html");

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html", false);
    });

    webServer.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {

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

    webServer.on("/disconnect", [](AsyncWebServerRequest *request) {
        request->send(200);
        WiFi.disconnect();
    });

    webServer.on("/reboot", [](AsyncWebServerRequest *request) {
        request->send(200);
        ESP.restart();
    });

    webServer.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    webServer.addHandler(&ws); // ws.cleanupClients();
    webServer.begin();

    Serial.println("[setupServer] ok");
}


//
// MQTT
//
void setupMQTT() {
    Serial.println("[setupMQTT] init");

    mqttClient.setServer(mqttHost, mqttPort);
    mqttClient.setClientId(ESP_NAME_ID);

    mqttClient.onMessage( [](char *_topic, char *_payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

        std::string topic(_topic);

        char payload[len+1];
        payload[len] = '\0'; // => no null byte in _payload. sent 00ffff; received 00ffff@�x
        strncpy(payload, _payload, len);

        Serial.printf("[mqttClient][onMessage]: topic: %s | payload: %s\n", _topic, payload);

        if(topic == TOPIC_ESP_PING) {
            if(IS_LED) {
                mqttClient.publish(TOPIC_ESP_LED_PONG_ID, QOS0, false, "");
            }
            if(IS_DHT) {
                mqttClient.publish(TOPIC_ESP_DHT_PONG_ID, QOS0, false, "");
                mqttClient.publish("esp/dht/31/temp", QOS0, false, dhtTemp);
                mqttClient.publish("esp/dht/31/humidity", QOS0, false, dhtHumidity);
            }
        }

        if(isTopicMatching(topic, "esp/led", TOPIC_LED_ACTION_ON)) {
            FastLED.showColor(lastLedRGB);
        }
        if(isTopicMatching(topic, "esp/led", TOPIC_LED_ACTION_OFF)) {
            FastLED.showColor(CRGB(0,0,0));
        }
        if(isTopicMatching(topic, "esp/led", TOPIC_LED_ACTION_COLOR)) {
            int r, g, b;
            sscanf(payload, "%02x%02x%02x", &r, &g, &b); //x => hexadecimal int
            Serial.printf("r: %d, g: %d, b: %d\n", r, g, b);
            lastLedRGB = CRGB(r,g,b);
            FastLED.showColor(lastLedRGB);
        }
        if(isTopicMatching(topic, "esp/led", TOPIC_LED_ACTION_BRIGHTNESS)) {
            int brightness;
            sscanf(payload, "%d", &brightness);
            Serial.printf("%d\n", brightness);
            FastLED.setBrightness(brightness);
            FastLED.showColor(lastLedRGB);
        }
    });

    mqttClient.onConnect([](bool sessionPresent) {

        Serial.println("[mqttClient][onConnect] connected to mqtt server");
        mqttClient.subscribe(TOPIC_ESP_PING, QOS0);

        if(IS_LED) {
            Serial.println("[mqttClient][onConnect] subscribing to LED topics");
            mqttClient.subscribe(TOPIC_LED_ID, QOS0);
            mqttClient.subscribe(TOPIC_LED_ROOM, QOS0);
            mqttClient.subscribe(TOPIC_LED_G, QOS0);
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
    FastLED.showColor(lastLedRGB);

    Serial.println("[setupLeds] complete.");
}

void ledRainbow() {
    for (int i = 0; i <= 127; i++) {
        int  r = cubicwave8(i);
        // int  r = i;
        FastLED.showColor(CRGB(r,0,255-r));
        delay(20);
    }
    for (int i = 0; i <= 127; i++) {
        int g = cubicwave8(i);
        // int g = i;
        FastLED.showColor(CRGB(255-g,g,0));
        delay(20);
    }
    for (int i = 0; i <= 127; i++) {
        int b = cubicwave8(i);
        // int b = i;
        FastLED.showColor(CRGB(0,255-b,b));
        delay(20);
    }
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
            Serial.println("Failed to read from DHT sensor");
            return;
        } else {
            successfullRead = true;
            char dhtTemp[10];
            char dhtHumidity[10];

            sprintf(dhtTemp, "%.2f", tempCelcius);
            sprintf(dhtHumidity, "%.2f", humidity);

            Serial.printf("read temperature: %.2f, humidity: %.2f\n", tempCelcius, humidity);
            Serial.printf("saved temperature: %s, humidity: %s\n", dhtTemp, dhtHumidity);
        }
    }
}

//
// TIME
//
void setupTime() {
    configTime(TIME_UTC_TZ * 3600, TIME_USE_DST * 3600, TIME_NTP_SERVER);
    Serial.print("Waiting for time ...");
    while (time(nullptr) < 100) {
        Serial.print(".");
        delay(100);
    }
    time_t now = time(nullptr);
    Serial.printf(" => %d | %s\n", now, ctime(&now));
}

//
// MAIN
//
void setup() {
    // system_update_cpu_freq(SYS_CPU_160MHz);
    Serial.begin(9600);
    Serial.println();
    Serial.println("[setup] init");

    if(IS_LED) {
        setupLeds();
    }

    setupWifi();
    setupTime();
    setupServer();
    setupMQTT();

    if(IS_DHT) {
        setupDHT();
    }

    Serial.println("[setup] complete.");
}

void loop() {
    // delay(successfullRead ? 1000 * 60 * 10 : 5000);
    // dhtRead();
    ledRainbow();
}

// attachInterrupt()
// ICACHE_RAM_ATTR
// IRAM_ATTR
// With ICACHE_RAM_ATTR you put the function on the RAM.
// With ICACHE_FLASH_ATTR you put the function on the FLASH (to save RAM). 
// Interrupt functions should use the ICACHE_RAM_ATTR. 
// Function that are called often, should not use any cache attribute.

// wifi_set_sleep_type(MODEM_SLEEP_T);
// wifi_set_sleep_type(LIGHT_SLEEP_T)
// wifi_fpm_do_sleep()
// ESP.deepSleep(5e6); // need to wire D0 to RST