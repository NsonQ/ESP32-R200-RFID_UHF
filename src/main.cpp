#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "R200.h"
#include "env.h"

#define RX_PIN 16
#define TX_PIN 17
#define LOCK_PIN 21
#define LOCK_FEEDBACK_PIN 22

// MQTT Topics
const char *INVENTORY = "Fridge01/Inventory";
const char *COMMAND = "Fridge01/Command";
const char *CART = "Fridge01/Cart";

// Lock
// High means unlocked, Low means locked
int lastLockState = 0;
String diff = "[]";

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
  pinMode(LOCK_PIN, OUTPUT);
  pinMode(LOCK_FEEDBACK_PIN, INPUT_PULLUP);
  lastLockState = digitalRead(LOCK_FEEDBACK_PIN);
  Serial.println("WiFi Connected");
  client.setServer(mqtt_server, mqtt_port);
  Serial.println("Press 1 for scan, Press 2 for scan and diff");
}

void loop()
{
  int currentLockState = digitalRead(LOCK_FEEDBACK_PIN);

  delay(100); // Small debounce delay

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // Scan the initial inventory when the door is unlocked
  if (currentLockState == HIGH && lastLockState == LOW)
  {
    previousInventory = reader.scan();
    Serial.println("Initial Inventory: " + previousInventory);
    lastLockState = currentLockState;
  }
  // Repeatedly scan when the door is opening
  else if (currentLockState == HIGH && lastLockState == HIGH)
  {
    currentInventory = reader.scan();
    Serial.println("Current Inventory: " + currentInventory);
    delay(1000);
  }
  // Publish the updated inventory and cart when the door is closed
  else if (currentLockState == LOW && lastLockState == HIGH)
  {
    diff = reader.getJsonDifference(previousInventory, currentInventory);
    Serial.println("Cart: " + diff);
    client.publish(CART, diff.c_str());
    client.publish(INVENTORY, currentInventory.c_str());
    lastLockState = currentLockState;
  }

  if (Serial.available())
  {
    char cmd = Serial.read();

    if (cmd == 'o')
    {
      Serial.println("Door Unlocked");
      digitalWrite(LOCK_PIN, HIGH);
      delay(300);
      digitalWrite(LOCK_PIN, LOW);
    }
  }
}

// Function Implementations
// Connect to WiFi
void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  // WiFi.setTxPower(WIFI_POWER_8_5dBm);
  // Serial.print("Set Wifi to 8.5 dBm");
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