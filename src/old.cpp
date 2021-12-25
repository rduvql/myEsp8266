// #include <ESP8266WiFi.h>

// WiFiServer server(80);
// const char* ssid = "ppixel";
// const char* password = "pistache";

// int ledPin = D1;

// void setup()
// {
//     // 115200
//     Serial.begin(9600);

//     server.begin();

//     pinMode(D1, OUTPUT);
//     pinMode(D4, OUTPUT);

//     WiFi.begin(ssid, password);
//     // IPAddress ip(192,168,1,200);
//     // IPAddress gateway(192,168,1,254);
//     // IPAddress subnet(255,255,255,0);
//     // WiFi.config(ip, gateway, subnet);

//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(300);
//         Serial.print(WiFi.status());
//         Serial.print(".");
//     }

//     Serial.println("");
//     Serial.println("http://" + WiFi.localIP().toString() + "/");
// }

// const char* HTTP200 = R"======(
// HTTP/1.1 200 OK
// Content-type:text/plain

// )======";

// const char* INDEX_HTML = R"======(
// HTTP/1.1 200 OK
// Content-type:text/html

// <span hidden> D1=ON;D2=OFF;D3=ON;D4=ON </span> <!DOCTYPE html> <html> <head> <meta charset='utf-8'> <meta http-equiv='X-UA-Compatible' content='IE=edge'> <title>Page Title</title> <meta name='viewport' content='width=device-width, initial-scale=1'> <style> body { margin: 0 auto; max-width: 50em; font-family: "Helvetica", "Arial", sans-serif; line-height: 1.5; padding: 4em 1em; color: #566b78; } h1, h2, strong { color: #333; } a { color: #e81c4f; } .switch { -webkit-appearance: none; -moz-appearance: none; appearance: none; position: absolute; width: 4em; height: 2em; font-size: 1em; border: 4px solid #dedede; border-radius: 2em; transition: all 0.2s ease; } .switch:after { position: absolute; content: ""; width: 1.4em; height: 1.4em; margin: 0.05em; border-radius: 50%; background-color: #dedede; transition: all 0.2s ease; } .switch:checked { border: 4px solid #7fc6a6; } .switch:checked:after { background-color: #7fc6a6; -webkit-transform: translate(2em, 0); transform: translate(2em, 0); } </style> <script> /** * @param input {HTMLInputElement} */ function request(input){ let status = input.checked ? "ON" :"OFF"; fetch(`/${input.id}=${status}`, { method: "POST" }).then((response) => { response.text().then(v => { console.log(v) }) }); } </script> </head> <body> <label for="D1"> D1 </label> <input id="D1" type="checkbox" class="switch" onclick="request(this)"/> </body> </html>

// )======";

// void loop()
// {
//     WiFiClient client = server.available();

//     if (!client) {
//         return;
//     }
//     while(!client.available()){
//         delay(1);
//     }

//     String request = client.readStringUntil('\r');
//     Serial.println(request);
//     client.flush();

//     if(request.equals("GET / HTTP/1.1")) {
//         client.print(INDEX_HTML);
//     }
//     else
//     {
//         String value = "";
//         if (request.indexOf("/D1=ON") != -1) {
//             digitalWrite(D1, HIGH); // allumer la led
//             digitalWrite(D4, HIGH); // allumer la led
//             value="on";
//         }
//         else if (request.indexOf("/D1=OFF") != -1)  {
//             digitalWrite(D1, LOW);
//             digitalWrite(D4, LOW);
//             value = "off";
//         }
//         client.print(HTTP200);
//         client.print("D1:"+value);
//     }

//     delay(1);
// }


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
