#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
// #include "OTA.h"
#include "credentials.h"
#include <FastLED.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

#define NUM_LEDS 300
#define DATA_PIN 15
#define TOPIC "device/" DEVICEID

CRGB leds[NUM_LEDS];

int value = 0;
char json_c[250];
bool state = false;
float brightness;
long color_spectrumRGB;
String hexColor;

bool boot = false;

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]");

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    json_c[i] = payload[i];
  }
  Serial.println();

  StaticJsonDocument<250> doc;
  DeserializationError error = deserializeJson(doc, json_c);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }
  else
  {
    state = doc["on"];
    brightness = doc["brightness"];
    brightness /= 100;
    color_spectrumRGB = doc["color"]["spectrumRGB"];
    hexColor = String(color_spectrumRGB, HEX);
    Serial.println(hexColor);
    long number = (long)strtol(&hexColor[0], NULL, 16);
    int r = number >> 16;
    int g = number >> 8 & 0xFF;
    int b = number & 0xFF;
    Serial.println((String)r + "," + g + "," + b);
    if (state)
    {
      fill_solid(leds, NUM_LEDS, CRGB(r * brightness, g * brightness, b * brightness));
      FastLED.show();
    }
    else if (!state)
    {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
    }
  }
}
void requestData()
{
  Serial.println("Control");
  client.publish("device/control", "{\"id\" : \"" DEVICEID "\",\"param\" : \"\",\"value\" : \"\",\"intent\" : \"request\"}");
}
void reconnect()
{
  while (!client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(DEVICEID))
    {
      Serial.println("connected");
      client.subscribe(TOPIC);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  requestData();
}
void connectmqtt()
{
  client.connect(DEVICEID); // ESP will connect to mqtt broker with DEVICEID
  {
    Serial.println("connected to MQTT");
    Serial.print("TOPIC: ");
    Serial.println(TOPIC);
    client.subscribe(TOPIC);

    if (!client.connected())
    {
      reconnect();
    }
    requestData();
  }
}
void setup()
{
  Serial.begin(115200);
  Serial.println("Version: v1.0\nAuthor: Sergi Castro (sergicastro2001@gmail.com)");
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }
  // delay(3000);
  Serial.print("Wifi Connected with ip: ");
  Serial.println(WiFi.localIP());
  client.setServer(MQTTBROKER, 1883); //connecting to mqtt server
  client.setCallback(callback);
  //delay(5000);
  connectmqtt();

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();
}
