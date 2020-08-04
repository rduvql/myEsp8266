#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
// #include <Adafruit_Sensor.h>
// #include <DHT_U.h>
// #include <DHT.h>

#define PIN_PIR D8
#define PIN_LED D2
#define PIN_DHT D7

#define DHTTYPE DHT22

// DHT dht(PIN_DHT, DHTTYPE);


AsyncWebServer server(80);

const char* ssid = "changeme";
const char* password = "changeme";

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setup() {

    Serial.begin(9600);

      // Initialize SPIFFS
    if(!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_DHT, INPUT);

    // dht.begin();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.serveStatic("/", SPIFFS, "/"); //.setDefaultFile("/index.html");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html", false);
    });

    server.onNotFound(notFound);

    server.begin();

    Serial.println("setup --");
}

void loop() {

}

// void temperature() {

//     // int pir = digitalRead(PIN_PIR);

//     // digitalWrite(PIN_LED, pir);

//     // Serial.printf("%d", pir);

//     // delay(100);

//     delay(4000);

//     float h = dht.readHumidity();
//     // Read temperature as Celsius (the default)
//     float t = dht.readTemperature();

//     // Check if any reads failed and exit early (to try again).
//     if (isnan(h) || isnan(t)) {
//     Serial.println(F("Failed to read from DHT sensor!"));
//     return;
//     }

//     // Compute heat index in Celsius (isFahreheit = false)
//     float hic = dht.computeHeatIndex(t, h, false);

//     Serial.printf("%.2f °C, %.2f %% \n", t, h);
// }
