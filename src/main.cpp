#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <string.h>
#include "ArduinoJson.h"
#include "AsyncJson.h"

#define QOS0 0 // at most once
#define QOS1 1 // at least once
#define QOS2 2 // exactly once

// #define PIN_PIR D8
// #define PIN_LED D2
// #define PIN_DHT D7

// #define DHTTYPE DHT22
// DHT dht(PIN_DHT, DHTTYPE);

#define ESP_ID 31

#define TOPIC_ACTION "esp/led/mainroom/action"
#define TOPIC_COLOR "esp/led/mainroom/color"

#define TOPIC_G_ACTION "g/esp/led/action"
#define TOPIC_G_COLOR "g/esp/led/color"

char *wifiSsid = "changeme";
char *wifiPassword = "changeme";
IPAddress wifiStaticIp(192, 168, 1, ESP_ID);
IPAddress wifiGatewayIp(192, 168, 1, 1);
IPAddress wifiSubnet(255, 255, 255, 0);
IPAddress wifiDns(8, 8, 8, 8);

char *mqttHost = "192.168.1.11";
uint16_t mqttPort = 1883;

AsyncMqttClient mqttClient;
// /!\ declare this variable AFTER mqttClient or else asyncWebServer is not reachable by browsers ...
AsyncWebServer webServer(80);


bool eq(char *str1, char *str2) {
    return strcmp(str1, str2) == 0;
}


//
// WIFI
//
void setupWifi() {

    WiFi.config(wifiStaticIp, wifiGatewayIp, wifiSubnet, wifiDns);
    WiFi.hostname("esp8266_31");
    WiFi.mode(WIFI_STA);

    WiFi.begin(wifiSsid, wifiPassword);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {

        delay(500);
        Serial.printf("WiFi Failed!\n");
        return;
    }
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("wifi setup - ok");
}


//
// µServer
//
void setupServer() {
    
    webServer.serveStatic("/", SPIFFS, "/"); //.setDefaultFile("index.html");

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { 
        request->send(SPIFFS, "/index.html", "text/html", false); 
    });

    webServer.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {

        // StaticJsonDocument<512> doc;
        // JsonObject object = doc.to<JsonObject>();
        // object["hello"] = "world";
        
        AsyncJsonResponse * response = new AsyncJsonResponse();
        JsonVariant &json = response->getRoot();
        json["heap"] = ESP.getFreeHeap();
        json["ssid"] = WiFi.SSID();
        
        response->setLength();
        request->send(response);
    });

    webServer.onNotFound([](AsyncWebServerRequest *request) { 
        request->send(404, "text/plain", "Not found"); 
    });

    webServer.begin();
    Serial.println("server setup - ok");
}


//
// MQTT
//
void setupMQTT() {

    mqttClient.setServer(mqttHost, mqttPort);
    mqttClient.setClientId("esp 31");

    mqttClient.onMessage( [](char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

        Serial.println("message: ");
        Serial.println(topic);
        Serial.println(payload);

        if(eq(topic, TOPIC_ACTION) || eq(topic, TOPIC_G_ACTION)) {

            if(eq(payload, "on")) {

            }
            if(eq(payload, "off")) {

            }
            return;
        }

        if(eq(topic, TOPIC_COLOR) || eq(topic, TOPIC_G_COLOR)) {

            return;
        }
    });

    mqttClient.onConnect([](bool sessionPresent) {

        Serial.printf("subscribing to %s\n", TOPIC_ACTION);
        mqttClient.subscribe(TOPIC_ACTION, QOS0);
        mqttClient.subscribe(TOPIC_G_ACTION, QOS0);

        // char* colortopic = strcat(topic, "color");
        Serial.printf("subscribing to %s\n", TOPIC_COLOR);
        mqttClient.subscribe(TOPIC_COLOR, QOS0);
        mqttClient.subscribe(TOPIC_G_COLOR, QOS0); 
    });

    mqttClient.connect();

    Serial.println("mqtt setup - ok");
}


//
// MAIN
//
void setup() {

    Serial.begin(9600);
    Serial.println();
    Serial.println("Starting ...");

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // pinMode(PIN_LED, OUTPUT);
    // pinMode(PIN_PIR, INPUT);
    // pinMode(PIN_DHT, INPUT);

    // dht.begin();

    setupWifi();
    setupServer();
    setupMQTT();

    Serial.println("setup complete.");
}

void loop()
{
    mqttClient.publish("esp/led/mainroom/31", QOS0, true, "");
}

// char* concat(char* str1, char* str2) {
//     char* concatenated = (char*)malloc(strlen(str1)+strlen(str2));
//     strcpy(str1, concatenated);
//     strcat(concatenated, str2);
//     return concatenated;
// }
