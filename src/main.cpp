#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FreeRTOSConfig.h>
#include <Config.h>
#include <WiFi.h>
#include <PubSubClient.h>


#define TEMP_PIN 13
#define THERMISTER_PIN 2

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

int status = WL_IDLE_STATUS;
WiFiClient client;

void setup() {
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(12);
  Serial.println("DS18B20 ready");
  pinMode(THERMISTER_PIN, INPUT);

  while (status != WL_CONNECTED){
    status = WiFi.begin(SSID, PASSWORD);
    Serial.print("No Wifi Connection");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  Serial.print("Wifi connected!");


}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  int analogValue = analogRead(THERMISTER_PIN);
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
