#include <Arduino.h>
#include <ArduinoJson.h>
#include "R200.h"

#define RX_PIN 16
#define TX_PIN 17

R200 reader(Serial2, RX_PIN, TX_PIN);

void setup()
{
  Serial.begin(115200);
  reader.begin();
}

void loop()
{
  // Scan for RFID tags
  String tagsJson = reader.scan();
  Serial.println("Scanned Tags: " + tagsJson);
}