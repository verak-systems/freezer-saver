#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Config.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Secrets.h>
#include <string>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include <U8g2lib.h>


#define TEMP_PIN D7
#define THERMISTOR_PIN A0

// Board-specific I2C pins: SDA=D6(GPIO12), SCL=D5(GPIO14)
#define OLED_SDA D2
#define OLED_SCL D1

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

const char* ntpServer = "pool.ntp.org";
const int  estOffset_sec = -18000;
const char* unit = "°C";

int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
PubSubClient mqttClient;
JsonDocument data;

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /*clock=*/12, /*data=*/14, U8X8_PIN_NONE);


String buildJson(JsonDocument& doc, char* device, char* key, float value, char* unit){
  doc["device"] = device;
  doc[key] = value;
  doc["unit"] = unit;
  // TODO: Add dst check somewhere, or would this be better done on the rpi?
  struct tm timeinfo;
  char timestamp[32];

  if(getLocalTime(&timeinfo)){
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);
  }
  else{
    strcpy(timestamp, "Unavailable");
  }

  doc["timestamp"] = timestamp;

  String payload;
  serializeJson(doc, payload);

  return payload;
}

void setup() {
  delay(100);
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(12);
  Serial.println("\n\nDS18B20 ready");
  pinMode(THERMISTOR_PIN, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED){

    Serial.print("Wi-Fi disconnected -> Status: ");
    Serial.println(WiFi.status());
    delay(1000);
  }

  Serial.print("Wifi connected!\n");

  configTime(estOffset_sec, 0, ntpServer);

  Serial.print("Connecting to MQTT Broker\n");

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(BROKER, MQTT_PORT);

  while (!mqttClient.connect("esp32-Pub", MQQT_USER, MQTT_PASS)){
    Serial.print(".");
  }

  Serial.print("Connected to MQTT Broker\n");

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
}

void loop() {
  if (!WiFi.isConnected() || !mqttClient.connected()){
    Serial.print("Wifi/MQTT connection Dropped\nRetrying connection");
    while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      WiFi.reconnect();
      delay(1000);
    }

    while(!mqttClient.connected()){
      Serial.print(".");
      if (mqttClient.connect("esp32-Pub", MQQT_USER, MQTT_PASS)){break;}
      delay(1000);
    }

    Serial.print("RECONNECTED");
  }

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  int analogValue = analogRead(THERMISTOR_PIN);
  Serial.print("Thermistor ADC Value: ");
  Serial.println(analogValue);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: sensor not found. Check wiring and pull-up resistor.");
    data['status'] = "Unavaible";
  } else {
    
  }

  String payload = buildJson(data, "ESP32", "digitalTemp", tempC, "°C");
  mqttClient.publish(TEMP_TOPIC, payload.c_str(), true);
  data.clear();

  char val[15];
  

  dtostrf(tempC, 7, 3, val);

  strncat(val, unit, 13);

  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Fridge Temp Stats");
  u8g2.drawStr(0, 30, val);
  u8g2.sendBuffer();

  delay(5000);
}
