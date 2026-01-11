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
#define GREEN_LED_PIN 18
#define RED_LED_PIN 4
#define BLUE_LED_PIN 3

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
void callback(char *topic, byte *payload, unsigned int length);

void setup()
{
  // Initialise Serial and R200 Module
  Serial.begin(115200);
  reader.begin();

  // Set sensitivity and transmission power
  // Higher TX power (24 dBm = near maximum) for better range
  reader.setTxPower(26);
  // Maximum sensitivity: mixerGain=6 (max), ifGain=7 (max), threshold=0x0080 (lower = more sensitive)
  // Lower threshold allows detection of weaker signals from tags
  reader.setSensitivity(3, 6, 0x0100);

  // Initialize GPIO pins
  pinMode(LOCK_PIN, OUTPUT);
  pinMode(LOCK_FEEDBACK_PIN, INPUT_PULLUP);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, HIGH);
  lastLockState = digitalRead(LOCK_FEEDBACK_PIN);

  // Connect to WiFi
  setup_wifi();

  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
  Serial.println("Blue LED ON - System Active");
  digitalWrite(BLUE_LED_PIN, HIGH);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // Read the lock state
  int currentLockState = digitalRead(LOCK_FEEDBACK_PIN);
  delay(100);

  // Scan the initial inventory when the door is unlocked
  if (currentLockState == HIGH && lastLockState == LOW)
  {
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    previousInventory = reader.scan();
    Serial.println("Initial Inventory: " + previousInventory);
    lastLockState = currentLockState;
  }
  // Repeatedly scan and publish the cart when the door is opening
  else if (currentLockState == HIGH && lastLockState == HIGH)
  {
    currentInventory = reader.scan();
    Serial.println("Current Inventory: " + currentInventory);

    // Update previousInventory with any newly discovered tags
    // (tags that appear in currentInventory but were missed in initial scan)
    String updatedPrevious = reader.mergeNewTags(previousInventory, currentInventory);
    if (updatedPrevious != previousInventory)
    {
      Serial.println("New tags discovered, updating previous inventory.");
      previousInventory = updatedPrevious;
    }

    diff = reader.getJsonDifference(previousInventory, currentInventory);
    Serial.println("Cart: " + diff);
    client.publish(CART, diff.c_str());
    // delay(1000);
  }
  // Publish the updated inventory the door is closed
  else if (currentLockState == LOW && lastLockState == HIGH)
  {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    Serial.println("Door Closed. Publishing latest inventory...");
    Serial.println("Latest Inventory: " + currentInventory);
    client.publish(INVENTORY, currentInventory.c_str());
    lastLockState = currentLockState;
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
    {
      Serial.println("connected");
      client.subscribe(COMMAND);
      Serial.println("Subscribed to command topic");
    }

    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Callback function to handle incoming MQTT messages
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  // Create a string from the payload for easy comparison
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.println(message);

  // Check if the message is the unlock command
  if (String(topic) == COMMAND)
  {
    if (message == "UNLOCK")
    {
      Serial.println("MQTT Command: Door Unlocked");
      digitalWrite(LOCK_PIN, HIGH);
      delay(300);
      digitalWrite(LOCK_PIN, LOW);
    }
  }
}