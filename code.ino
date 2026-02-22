#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h> // New library for the direct address
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "project";
const char* password = "123456789";

#define DHTPIN D4
#define DHTTYPE DHT22
#define BUZZER_PIN D3
#define LED_PIN D7
#define LDR_PIN A0

Adafruit_SSD1306 display(128, 64, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);

float temp, hum;
int light;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Smart Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com">
  <style>
    body { font-family: 'Segoe UI', sans-serif; background: #0f0c29; background: linear-gradient(to bottom, #0f0c29, #302b63); color: white; text-align: center; margin:0; min-height: 100vh;}
    .header { padding: 40px 20px; background: rgba(255, 255, 255, 0.05); backdrop-filter: blur(10px); border-bottom: 1px solid rgba(255,255,255,0.1); }
    .container { display: flex; flex-wrap: wrap; justify-content: center; padding: 20px; }
    .card { background: rgba(255, 255, 255, 0.08); border-radius: 20px; padding: 25px; margin: 15px; width: 200px; border: 1px solid rgba(255,255,255,0.1); box-shadow: 0 8px 32px rgba(0,0,0,0.3); }
    .icon { font-size: 2.5rem; margin-bottom: 15px; }
    .value { font-size: 2.5rem; font-weight: bold; display: block; color: #00d2ff; }
    .label { font-size: 0.9rem; color: #aaa; text-transform: uppercase; letter-spacing: 1px; }
  </style>
</head>
<body>
  <div class="header"><h1>ENV MONITOR</h1></div>
  <div class="container">
    <div class="card"><i class="fas fa-thermometer-half icon" style="color:#ff4b2b;"></i><span class="label">Temp</span><span class="value" id="t">--</span><span class="label">°C</span></div>
    <div class="card"><i class="fas fa-tint icon" style="color:#00d2ff;"></i><span class="label">Humidity</span><span class="value" id="h">--</span><span class="label">%</span></div>
    <div class="card"><i class="fas fa-sun icon" style="color:#f9d423;"></i><span class="label">Light</span><span class="value" id="l">--</span><span class="label">ADC</span></div>
  </div>
  <script>
    setInterval(function(){
      fetch('/data').then(response => response.json()).then(data => {
        document.getElementById("t").innerHTML = data.temp;
        document.getElementById("h").innerHTML = data.hum;
        document.getElementById("l").innerHTML = data.light;
      });
    }, 2000);
  </script>
</body></html>)rawliteral";

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  dht.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  // START mDNS - This creates "env.local"
  if (MDNS.begin("env")) {
    MDNS.addService("http", "tcp", 80);
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"temp\":\""+String(temp,1)+"\",\"hum\":\""+String(hum,0)+"\",\"light\":\""+String(light)+"\"}";
    request->send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  MDNS.update(); // Keep mDNS service running
  temp = dht.readTemperature();
  hum = dht.readHumidity();
  light = analogRead(LDR_PIN);

  bool isAlert = (temp > 32.0 || light < 200);
  digitalWrite(LED_PIN, isAlert ? HIGH : LOW);
  digitalWrite(BUZZER_PIN, isAlert ? HIGH : LOW);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ADDRESS:");
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.println("env.local"); // Displaying the direct address
  
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("Temp: %.1f C\nHum:  %.0f %%\nLDR:  %d", temp, hum, light);
  display.display();
  
  delay(2000);
}