#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

String endpoint = "http://192.168.0.11:3000/";
const char * wifi_ssid = "UniFiBP";
const char * wifi_password = "foobar88";

int encoderPinA = D1;
int encoderPinB = D2;
int buttonPin = D3;

long lastEncoderValue = 0;
byte state = 0; // used to read the encoder
volatile int encoder = 10;
int bump[] = {0, 0, -1, 1};

boolean isButtonPushDown(void)
{
  if (!digitalRead(buttonPin))
  {
    delay(50);
    if (!digitalRead(buttonPin))
      return true;
  }
  return false;
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(wifi_ssid, wifi_password);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);
  pinMode(buttonPin, INPUT);

  digitalWrite(encoderPinA, HIGH);
  digitalWrite(encoderPinB, HIGH);

  attachInterrupt(digitalPinToInterrupt(D1), updateEncoder, RISING);
}

void loop()
{
  if (isButtonPushDown())
  {
    sendRequest("button");
  }
  if (lastEncoderValue != encoder)
  {
    sendRequest(String(encoder));
    lastEncoderValue = encoder;
  }
}

void updateEncoder()
{
  state = 0;                                // start with 0
  state = state + digitalRead(encoderPinA); // add the state of pin A
  state <<= 1;                              // shift left
  state = state + digitalRead(encoderPinB); // add the state of pin B
  if (state == 0)
    return;                        // ignore this value - keybounce
  encoder = encoder + bump[state]; // added direction of turn to state
  if (encoder < 0)
    encoder = 100; // lower limit of any menu roll back to top
  if (encoder > 99)
    encoder = 0; // upper limit of any menu roll back to bottom
  Serial.printf("Rot: %d\n", encoder);
  Serial.flush();
}

ESP8266WiFiMulti WiFiMulti;

void sendRequest(String value)
{
  if ((WiFiMulti.run() == WL_CONNECTED))
  {

    WiFiClient client;

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, endpoint + value))
    {
      Serial.print("[HTTP] GET...\n");
      int httpCode = http.GET();
      if (httpCode > 0)
      {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String payload = http.getString();
          Serial.println(payload);
        }
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
    else
    {
      Serial.printf("[HTTP} Unable to connect\n");
    }
  }
}