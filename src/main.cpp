#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "R200.h"
#include "env.h"

#define RX_PIN 16
#define TX_PIN 17

// MQTT Topics
const char *INVENTORY = "Fridge01/Inventory";
const char *COMMAND = "Fridge01/Command";
const char *CART = "Fridge01/Cart";

R200 reader(Serial2, RX_PIN, TX_PIN);
WiFiClient espClient;
PubSubClient client(espClient);
String previousInventory = "[]";
String currentInventory = "[]";

void setup_wifi();
void reconnect();

void setup()
{
  Serial.begin(115200);
  reader.begin();
  reader.setTxPower(15);
  Serial.println("R200 Initialized");
  setup_wifi();
  Serial.println("WiFi Connected");
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("Press 1 for scan, Press 2 for scan and diff");
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (Serial.available())
  {
    char cmd = Serial.read();

    // Scan the initial inventory when the door is unlocked
    if (cmd == '1')
    {
      Serial.println("Scanning...");
      previousInventory = reader.scan();
      Serial.println("Result: " + previousInventory);
    }
    // Repeatedly scan
    // Publish the updated inventory and cart when the door is closed
    else if (cmd == '2')
    {
      Serial.println("Checking out...");
      currentInventory = reader.scan();
      String diff = reader.getJsonDifference(previousInventory, currentInventory);
      client.publish(CART, diff.c_str());
      Serial.println(diff);
    }
  }
}

// Function Implementations
// Connect to WiFi
void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);
  //  Serial.print("Set Wifi to 8.5 dBm");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

// Reconnect to MQTT broker
void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client"))
      Serial.println("connected");
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}