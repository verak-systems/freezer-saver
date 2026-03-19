#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FreeRTOSConfig.h>
#include <Config.h>
#include <WiFi.h>
#include <PubSubClient.h>


#define TEMP_PIN 13
#define THERMISTOR_PIN 34

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

int status = WL_IDLE_STATUS;
WiFiClient client;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(12);
  Serial.println("DS18B20 ready");
  pinMode(THERMISTOR_PIN, INPUT);

  while (WiFi.status() != WL_CONNECTED){

    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (WiFi.status() != WL_CONNECTED){
      Serial.print("No Wifi Connection\n");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  Serial.print("Wifi connected!\n");
}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  int analogValue = analogRead(THERMISTOR_PIN);
  Serial.print("Thermistor ADC Value: ");
  Serial.println(analogValue);

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: sensor not found. Check wiring and pull-up resistor.");
  } else {
    Serial.print("Temperature: ");
    Serial.print(tempC, 1);
    Serial.println(" °C");
  }

  vTaskDelay(pdMS_TO_TICKS(5000));
}
