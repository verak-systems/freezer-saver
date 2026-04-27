#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Config.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <Secrets.h>
#include <string>
#include <ArduinoJson.h>
#include <time.h>
#include <Wire.h>
#include <U8g2lib.h>
#include "twilio.hpp"

#define TEMP_PIN D7
#define THERMISTOR_PIN A0

// Board-specific I2C pins: SDA=D6(GPIO12), SCL=D5(GPIO14)
#define OLED_SDA D2
#define OLED_SCL D1

const float ALARM_TEMP = 28.0;
const int send_cap = 5;
int cur_send;
char msg[128];

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

const char *ntpServer = "pool.ntp.org";
const int estOffset_sec = -18000;
const char *unit = "°C";

int status = WL_IDLE_STATUS;
WiFiClient wifiClient;
PubSubClient mqttClient;
JsonDocument data;

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /*clock=*/12, /*data=*/14, U8X8_PIN_NONE);
const char fingerprint[] = "7C 92 41 AF E2 D2 67 A3 7B 92 C6 DA 05 31 CD E2 6A 1D 45 48";
const char *account_ssid = TWILIO_SSID;
const char *auth_token = TWILIO_TOKEN;

String master_num = MASTER_NUM;

String to_num = TO_NUM;
String from_num = FROM_NUM;

Twilio *twilio;
ESP8266WebServer twilio_server(8000);

 String response;

void handle_message()
{
  bool authorized = false;
  char command = '\0';

  Serial.println("Incoming connection!");

  for (int i = 0; i < twilio_server.args(); i++)
  {
    Serial.print(twilio_server.argName(i));
    Serial.print(": ");
    Serial.println(twilio_server.arg(i));

    if (twilio_server.argName(i) == "From" and twilio_server.arg(i) == master_num)
    {
      authorized = true;
    }
    else if (twilio_server.argName(i) == "Body")
    {
      if (twilio_server.arg(i) == "?" or twilio_server.arg(i) == "0" or twilio_server.arg(i) == "1")
      {
        command = twilio_server.arg(i)[0];
      }
    }

    String response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    if (command != '\0')
    {
      if (authorized)
      {
        switch (command)
        {
        case '?':
        default:
          response += "<Response><Message>"
                      "0 - Light off, 1 - Light On, "
                      "? - Help\n"
                      "The light is currently: ";
          response += digitalRead(LED_BUILTIN);
          response += "</Message></Response>";
          break;
        }
      }
      else
      {
        response += "<Response><Message>"
                    "Unauthorized!"
                    "</Message></Response>";
      }
    }
    else
    {
      response += "<Response><Message>"
                  "Look: a SMS response from an ESP8266!"
                  "</Message></Response>";
    }

    twilio_server.send(200, "application/xml", response);
  }
}

void setup()
{
  delay(100);
  Serial.begin(115200);
  sensors.begin();
  sensors.setResolution(12);
  Serial.println("\n\nDS18B20 ready");
  pinMode(THERMISTOR_PIN, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Wi-Fi disconnected -> Status: ");
    Serial.println(WiFi.status());
    delay(1000);
  }

  Serial.println("Wifi connected!\n");
  Serial.println(WiFi.localIP());

  twilio = new Twilio(account_ssid, auth_token, fingerprint);
 
  bool success = twilio->send_message(
    to_num,
    from_num,
    "Fridge monitor stared!",
    response
  );

  // twilio_server.on("/message", handle_message);
  twilio_server.begin();

  configTime(estOffset_sec, 0, ntpServer);

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14B_tr);
}

void loop()
{
  if (!WiFi.isConnected())
  {
    Serial.print("Wifi/MQTT connection Dropped\nRetrying connection");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      WiFi.reconnect();
      delay(1000);
    }

    Serial.print("RECONNECTED");
  }

  twilio_server.handleClient();

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C)
  {
    twilio->send_message(
    to_num,
    from_num,
    "Disconnected from wifi, reconnected now!",
    response
    );
  }
  else if (tempC > ALARM_TEMP && cur_send <= send_cap)
  {
    snprintf(msg, sizeof(msg), "FREEZER IS ABOVE %.2f", ALARM_TEMP);
    twilio->send_message(to_num, from_num, msg, response);
    cur_send++;
  }else if (tempC < ALARM_TEMP){
    cur_send--;
  }

  char val[15];
  dtostrf(tempC, 7, 3, val);
  strncat(val, unit, 13);

  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, "Fridge Temp Stats");
  u8g2.drawStr(0, 30, val);
  u8g2.sendBuffer();

  delay(5000);
}
