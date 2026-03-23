#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FreeRTOSConfig.h>
#include <Config.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Secrets.h>
#include <string>


#define TEMP_PIN 13
#define THERMISTOR_PIN 34

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
PubSubClient mqttClient;

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
  Serial.print("Connecting to MQTT Broker");

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(BROKER, MQTT_PORT);

  while (!mqttClient.connect("esp32-temp", MQQT_USER, MQTT_PASS)){
    Serial.print(".");

  }

  Serial.print("Connected to MQTT Broker\n");
}

void loop() {
  if (!WiFi.isConnected() || !mqttClient.connected()){
    Serial.print("Wifi/MQTT connection Dropped\nRetrying connection");
    while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      WiFi.reconnect();
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while(!mqttClient.connected()){
      Serial.print(".");
      if (mqttClient.connect("esp32", MQQT_USER, MQTT_PASS)){break;}
    }

    Serial.print("RECONNECTED");
  }

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  int analogValue = analogRead(THERMISTOR_PIN);
  Serial.print("Thermistor ADC Value: ");
  Serial.println(analogValue);

  char value[20];

  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: sensor not found. Check wiring and pull-up resistor.");
    strcpy(value,"Temp Unavailble");

  } else {
    Serial.print("Temperature: ");
    Serial.print(tempC, 1);
    Serial.println(" °C");
    dtostrf(tempC, 4, 1, value);
    strcat(value, " °C");
  }

  mqttClient.publish(TEMP_TOPIC, value, true);

  vTaskDelay(pdMS_TO_TICKS(5000));

}
