#include <Arduino.h>
#include <ArduinoJson.h>
#include "R200.h"

#define RX_PIN 16
#define TX_PIN 17

R200 reader(Serial2, RX_PIN, TX_PIN);
String previousInventory = "[]";
String currentInventory = "[]";

void setup()
{
  Serial.begin(115200);
  reader.begin();
  Serial.println("Press 1 for scan, Press 2 for scan and diff");
}

void loop()
{
  if (Serial.available())
  {
    char cmd = Serial.read();

    if (cmd == '1')
    {
      Serial.println("Scanning...");
      previousInventory = reader.scan();
      Serial.println("Result: " + previousInventory);
    }
    else if (cmd == '2')
    {
      Serial.println("Checking out...");
      currentInventory = reader.scan();
      String diff = reader.getJsonDifference(previousInventory, currentInventory);
      Serial.println(diff);
    }
  }
}