# ESP32-R200-RFID UHF Firmware

Firmware for ESP32-based smart vending machine fridges using R200 UHF RFID reader. This firmware handles door locking/unlocking, RFID tag reading, and MQTT communication with the backend server.

## Project Description

This firmware runs on ESP32 microcontrollers installed in smart vending machine fridges. It provides:

- **MQTT Communication** - Real-time communication with backend server
- **RFID Tag Reading** - UHF RFID tag detection using R200 reader
- **Door Lock Control** - Electromagnetic lock control for fridge door
- **Cart Updates** - Publishes EPC tags of removed items to backend
- **Inventory Management** - Publishes current inventory when door closes

## Architecture

- **Platform**: ESP32 (Espressif)
- **Framework**: Arduino
- **RFID Reader**: R200 UHF RFID Module
- **Communication**: MQTT over WiFi
- **Lock Control**: Electromagnetic lock via GPIO

## Prerequisites

- **Development Environment**: Choose one:
  - Visual Studio Code with PlatformIO IDE extension
  - Arduino IDE with ESP32 board support
- ESP32 development board
- R200 UHF RFID reader module
- Electromagnetic lock (12V)
- MQTT broker (Mosquitto) running
- WiFi network access

## Hardware Requirements

- ESP32 development board (ESP32-DevKitC or similar)
- R200 UHF RFID reader module
- Electromagnetic lock (12V, controlled via relay)
- Power supply (5V for ESP32, 12V for lock)
- Relay module (for lock control)
- Antenna for R200 module
- Jumper wires and breadboard

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/NsonQ/ESP32-R200-RFID_UHF.git
cd ESP32-R200-RFID_UHF
```

### 2. Choose Development Environment

#### Option A: PlatformIO

1. Install Visual Studio Code: https://code.visualstudio.com/
2. Install PlatformIO IDE extension:
   - Open VS Code
   - Go to Extensions (Ctrl+Shift+X / Cmd+Shift+X)
   - Search for "PlatformIO IDE"
   - Click Install
3. PlatformIO will automatically install required tools on first use

#### Option B: Arduino IDE

1. Install Arduino IDE: https://www.arduino.cc/en/software
2. Add ESP32 Board Support:
   - Open Arduino IDE
   - Go to File → Preferences
   - Add this URL to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Boards Manager
   - Search for "esp32" and install "esp32 by Espressif Systems"
3. Install Required Libraries:
   - Go to Sketch → Include Library → Manage Libraries
   - Install the following libraries:
     - ArduinoJson (by Benoit Blanchon) - version 6.18.5 or compatible
     - PubSubClient (by Nick O'Leary) - version 2.8 or compatible
4. Select Board and COM port:
   - Tools → Board → esp32 → NodeMCU-32S
   - Tools → Port → Your COM port


### 3. Configure Environment

Copy `include/env_example.h` to `include/env.h` and update with your settings:

```cpp
#ifndef ENV_H
#define ENV_H

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Configuration
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
const int mqtt_port = 1883;

#endif
```

### 4. Update Fridge ID and MQTT Topics

For each fridge, update the following in `src/main.cpp`:
- MQTT Topics (lines 17-19):
  - `INVENTORY` - MQTT topic: `Fridge01/Inventory` (change "01" to your fridge ID)
  - `COMMAND` - MQTT topic: `Fridge01/Command` (change "01" to your fridge ID)
  - `CART` - MQTT topic: `Fridge01/Cart` (change "01" to your fridge ID)

### 5. Build and Upload

#### Using PlatformIO (VS Code)

1. Open the project folder in VS Code
2. Click the PlatformIO icon in the sidebar
3. Build: Click "Build" button or use `Ctrl+Alt+B` (Windows/Linux) / `Cmd+Alt+B` (Mac)
4. Upload: Click "Upload" button or use `Ctrl+Alt+U` (Windows/Linux) / `Cmd+Alt+U` (Mac)
5. Monitor: Click "Serial Monitor" button or use `Ctrl+Alt+S` (Windows/Linux) / `Cmd+Alt+S` (Mac)

#### Using Arduino IDE

1. Open `src/main.cpp` in Arduino IDE
2. Create additional tabs for other source files:
   - Create a new tab named `R200.cpp` and copy contents from `src/R200.cpp`
   - Create a new tab named `R200.h` and copy contents from `include/R200.h`
   - Create a new tab named `env.h` and copy contents from `include/env.h`
3. Ensure all files are in the same sketch folder
4. Select the correct board and port (see Step 2 Option B)
5. Click "Verify" (✓) to compile the code
6. Click "Upload" (→) to upload to ESP32
7. Open Serial Monitor (Tools → Serial Monitor) at 115200 baud rate to view output

## Hardware Connections

### ESP32 to R200 Module
- Connect R200 module via Serial (UART)
- Default pins: RX=16, TX=17 (configurable in R200.cpp)
- Power: 5V and GND

### ESP32 to Lock Control
- Lock control pin: GPIO 2 (configurable)
- Connect via relay module
- Relay controls 12V electromagnetic lock

### Power Supply
- ESP32: 5V via USB or external supply
- Lock: 12V external supply (via relay)

## MQTT Topics

### Subscribed Topics
- `Fridge{ID}/Command` - Receives "UNLOCK" command from backend

### Published Topics
- `Fridge{ID}/Cart` - Publishes JSON array of EPC tags when items are removed
- `Fridge{ID}/Inventory` - Publishes JSON array of EPC tags when door closes

## Operation Flow

1. **QR Code Scanned** → Backend sends "UNLOCK" to `Fridge{ID}/Command`
2. **Lock Unlocks** → Door opens, lock pin goes HIGH for 300ms
3. **Items Removed** → R200 detects removed items, publishes EPC tags to `Fridge{ID}/Cart`
4. **Door Closes** → R200 scans inventory, publishes all EPC tags to `Fridge{ID}/Inventory`
5. **Backend Processes** → Backend validates cart and checks out automatically

## Dependencies

### PlatformIO Libraries
- `ArduinoJson@^6.18.5` - JSON parsing and generation
- `PubSubClient@^2.8` - MQTT client library

### Built-in Libraries
- `WiFi` - ESP32 WiFi functionality
- `ArduinoJson` - JSON handling
- `PubSubClient` - MQTT communication

## Configuration

### platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_port = COM10  # Update with your COM port
lib_deps = 
    ArduinoJson@^6.18.5
    PubSubClient@^2.8
```

### Serial Monitor

Baud rate: 115200

Monitor output shows:
- WiFi connection status
- MQTT connection status
- RFID tag detections
- Lock control actions
- Error messages

## Troubleshooting

### WiFi Connection Issues
- Verify SSID and password in `env.h`
- Check WiFi signal strength
- Review serial monitor for connection errors

### MQTT Connection Issues
- Verify MQTT broker IP and port
- Check broker is accessible from ESP32 network
- Review MQTT client ID conflicts

### RFID Reading Issues
- Verify R200 module connections
- Check antenna connection
- Review serial output for R200 communication
- Ensure tags are within reading range

### Lock Control Issues
- Verify lock pin configuration
- Check relay module connections
- Test lock with direct power supply
- Review GPIO pin assignments

### Upload Issues
- Check COM port in `platformio.ini`
- Press BOOT button during upload if needed
- Verify ESP32 drivers are installed
- Try different USB cable

## Testing

### Serial Monitor Output

Expected output on startup:
```
WiFi connected
IP address: 192.168.x.x
Attempting MQTT connection...
connected
Subscribed to command topic
```

### MQTT Testing

Test unlock command:
```bash
mosquitto_pub -h <broker-ip> -t "Fridge01/Command" -m "UNLOCK"
```

Monitor cart updates:
```bash
mosquitto_sub -h <broker-ip> -t "Fridge01/Cart"
```

Monitor inventory updates:
```bash
mosquitto_sub -h <broker-ip> -t "Fridge01/Inventory"
```

## Project Structure

```
ESP32-R200-RFID_UHF/
├── include/
│   ├── env.h              # Configuration (create from env_example.h)
│   ├── env_example.h      # Configuration template
│   └── R200.h             # R200 RFID reader header
├── src/
│   ├── main.cpp           # Main firmware code
│   └── R200.cpp           # R200 RFID reader implementation
├── platformio.ini         # PlatformIO configuration
└── README.md              # This file
```

## Related Repositories

- **s-mart-backend** - FastAPI backend server (receives MQTT messages)
- **s-mart** - Flutter mobile application (triggers unlock commands)