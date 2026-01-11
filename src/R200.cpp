#include "R200.h"

R200::R200(HardwareSerial &serial, int rxPin, int txPin, int baud)
{
    _rxPin = rxPin;
    _txPin = txPin;
    _baud = baud;
    _serial = &serial;
}

// Initiate R200 Reader with default baudrate
bool R200::begin()
{
    // Start Hardware Serial
    _serial->begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    delay(100); // Allow hardware to stabilize
    return true;
}

// Adjust transmission power (15 to 26 dBm)
// Command 0xB6
void R200::setTxPower(uint8_t dbm)
{
    if (dbm > 26)
        dbm = 26;
    else if (dbm < 15)
        dbm = 15;
    uint16_t powerVal = dbm * 100;

    // Param: 2 bytes (Value * 100)
    uint8_t params[2];
    params[0] = (powerVal >> 8) & 0xFF; // MSB
    params[1] = powerVal & 0xFF;        // LSB

    // Flush any existing data in buffer before sending command
    while (_serial->available())
        _serial->read();

    _sendCommand(0xB6, params, 2);
    delay(150); // Delay to ensure response is ready

    // Read response to verify
    Frame response = _readResponse();
    // Check if we got a valid response frame (frameType 0x01 = response, cmd 0xB6)
    if (response.frameType == 0x01 && response.cmd == 0xB6 && response.payload.size() > 0)
    {
        if (response.payload[0] == 0x00)
        {
            Serial.print("TX Power set successfully. Power: ");
            Serial.print(dbm);
            Serial.println(" dBm");
        }
        else
        {
            Serial.println("Warning: Failed to set TX Power.");
        }
    }
    else
    {
        Serial.println("Warning: Failed to set TX Power.");
    }
}

// Adjust sensitivity level (Command 0xF0)
// mixerGain: 0-6 (higher = more sensitive, 6 = maximum)
// ifGain: 0-7 (higher = more sensitive, 7 = maximum)
void R200::setSensitivity(uint8_t mixerGain, uint8_t ifGain, uint16_t threshold)
{
    // Validate inputs
    if (mixerGain > 6)
        mixerGain = 6;
    if (ifGain > 7)
        ifGain = 7;

    // Construct payload: Mixer_G (1byte) | IF_G (1byte) | Thrd (2bytes, big-endian)
    uint8_t params[4];
    params[0] = mixerGain;
    params[1] = ifGain;
    params[2] = (threshold >> 8) & 0xFF; // MSB
    params[3] = threshold & 0xFF;        // LSB

    // Flush any existing data in buffer before sending command
    while (_serial->available())
        _serial->read();

    _sendCommand(0xF0, params, 4);
    delay(150); // Delay to ensure response is ready

    // Read response to verify
    Frame response = _readResponse();
    // Check if we got a valid response frame (frameType 0x01 = response, cmd 0xF0)
    if (response.frameType == 0x01 && response.cmd == 0xF0 && response.payload.size() > 0)
    {
        if (response.payload[0] == 0x00)
        {
            Serial.print("Mixer Gain: ");
            Serial.print(mixerGain);
            Serial.print(" | Amplifier Gain: ");
            Serial.print(ifGain);
            Serial.print(" | Threshold: 0x");
            if (threshold < 0x1000)
                Serial.print("0");
            if (threshold < 0x100)
                Serial.print("0");
            if (threshold < 0x10)
                Serial.print("0");
            Serial.println(threshold, HEX);
        }
        else
        {
            Serial.println("Warning: Failed to set sensitivity.");
        }
    }
    else
    {
        Serial.println("Warning: Failed to set sensitivity.");
    }
}

// Send scan command and read the response frame
// Return the tags' EPC in JSON array format
String R200::scan()
{
    unsigned long start = millis();
    std::vector<String> foundTags;

    // Flush any existing data in buffer
    while (_serial->available())
        _serial->read();

    // Scan command (0x22)
    _sendCommand(0x22, NULL, 0);

    // Read tags for 500ms and return in JSON format
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();

    //
    while (millis() - start < 1000)
    {
        // Read a notification frame
        Frame response = _readResponse();

        // Receive valid frame
        if (response.cmd == 0x22 && response.payload.size() > 5)
        {
            String tag = "";
            // Extract EPC from payload
            // Start at index 3 (Skip RSSI, PC bytes)
            // End at size - 2 (Skip CRC bytes)
            for (size_t i = 3; i < response.payload.size() - 2; i++)
            {
                if (response.payload[i] < 0x10)
                    tag += "0";
                tag += String(response.payload[i], HEX);
            }
            tag.toUpperCase();

            // Ensure unique tags only
            bool exists = false;
            for (const String &t : foundTags)
            {
                if (t == tag)
                {
                    exists = true;
                    break;
                }
            }

            // Add to JSON array if new tag
            if (!exists)
            {
                foundTags.push_back(tag);
                array.add(tag);
            }
        }
    }

    // Construct JSON output
    String output;
    serializeJson(doc, output);

    if (array.size() == 0)
        return "[]";
    return output;
}

uint8_t R200::_calculateChecksum(const uint32_t data)
{
    return (uint8_t)(data & 0xFF);
}

// Construct frame to be sent to R200
void R200::_sendCommand(uint8_t cmd, const uint8_t *params, size_t len)
{
    // Frame: Header(AA) | Type(00) | Cmd | PL_MSB | PL_LSB | Params | Checksum | End(DD)

    // sum = Type + Cmd + PL_MSB + PL_LSB + Params[]
    // Calculate checksum
    uint32_t sum = 0;
    sum += 0x00; // Type
    sum += cmd;  // Command

    uint8_t pl_msb = (len >> 8) & 0xFF;
    uint8_t pl_lsb = len & 0xFF;
    sum += pl_msb;
    sum += pl_lsb;

    if (params != NULL && len > 0)
    {
        for (size_t i = 0; i < len; i++)
            sum += params[i];
    }

    uint8_t checksum = _calculateChecksum(sum);

    // Send frame to R200 for execution
    _serial->write(0xAA);
    _serial->write(0x00);
    _serial->write(cmd);
    _serial->write(pl_msb);
    _serial->write(pl_lsb);

    if (params != NULL && len > 0)
    {
        _serial->write(params, len);
    }

    _serial->write(checksum);
    _serial->write(0xDD);
}

// Read response frame from R200
Frame R200::_readResponse()
{

    if (_serial->available())
    {
        byte b = _serial->read();

        // Ignore invalid bytes until a header byte is found
        if (b != 0xAA)
            return Frame{};

        // Wait 50ms for Type, Cmd, PL_MSB, PL_LSB
        unsigned long timeout = millis();
        while (_serial->available() < 4)
        {
            // Incomplete frame
            if (millis() - timeout > 50)
                return Frame{};
        }

        // Read notification frame's Type, Cmd, PL_MSB, PL_LSB
        uint8_t type = _serial->read();
        uint8_t cmd = _serial->read();
        uint8_t len_msb = _serial->read();
        uint8_t len_lsb = _serial->read();
        uint16_t payloadLen = (len_msb << 8) | len_lsb;

        // Wait 100ms for Payload, Checksum, End
        timeout = millis();
        while (_serial->available() < payloadLen + 2)
        {
            // Incomplete frame
            if (millis() - timeout > 100)
                return Frame{};
        }

        // Read payload, checksum, end byte
        if (payloadLen > 64)
            payloadLen = 64;
        uint8_t payload[64];
        _serial->readBytes(payload, payloadLen);

        uint8_t checksum = _serial->read();
        uint8_t end = _serial->read();

        // Complete frame received
        if (end == 0xDD)
        {
            Frame f;
            f.frameType = type;
            f.cmd = cmd;

            // Copy array to vector
            for (int i = 0; i < payloadLen; i++)
            {
                f.payload.push_back(payload[i]);
            }

            // Debugging info
            // Serial.print("RX Cmd: "); Serial.println(cmd, HEX);
            // Serial.print("RX Type: "); Serial.println(type, HEX);
            // Serial.print("RX Payload Len: "); Serial.println(payloadLen);
            // Serial.print("RX Payload: ");
            // for (size_t i = 0; i < payloadLen; i++)
            // {
            //     Serial.print(payload[i], HEX); Serial.print(" ");
            // }
            // Serial.println();
            // Serial.print("RX Checksum: "); Serial.println(checksum, HEX);

            return f;
        }
    }
    // No valid tag found
    return Frame{};
}

// Compare two JSON arrays and return the difference in JSON format
// This difference would be the items to be added into the cart
String R200::getJsonDifference(String jsonOld, String jsonNew)
{
    // Parse both old and new JSON arrays for comparison
    DynamicJsonDocument doc1(2048);
    DynamicJsonDocument doc2(2048);
    deserializeJson(doc1, jsonOld);
    deserializeJson(doc2, jsonNew);

    // Convert JSON arrays to vectors for easier comparison
    std::vector<String> listOld;
    std::vector<String> listNew;

    // Populate JSON arrays into vectors
    for (JsonVariant v : doc1.as<JsonArray>())
        listOld.push_back(v.as<String>());
    for (JsonVariant v : doc2.as<JsonArray>())
        listNew.push_back(v.as<String>());

    // Compare both vectors to find missing tags
    DynamicJsonDocument resultDoc(2048);
    JsonArray missingArray = resultDoc.createNestedArray("Cart");
    // Find new tags array (not used)
    // JsonArray newArray = resultDoc.createNestedArray("new");

    // Find missing tags
    for (const String &oldTag : listOld)
    {
        bool found = false;
        for (const String &newTag : listNew)
        {
            if (oldTag == newTag)
            {
                found = true;
                break;
            }
        }
        if (!found)
            missingArray.add(oldTag);
    }

    // // Find new tags
    // for (const String &newTag : listNew)
    // {
    //     bool found = false;
    //     for (const String &oldTag : listOld)
    //     {
    //         if (newTag == oldTag)
    //         {
    //             found = true;
    //             break;
    //         }
    //     }
    //     if (!found)
    //         newArray.add(newTag);
    // }

    // Serialize result JSON
    String output;
    serializeJson(resultDoc, output);
    return output;
}

// Merge new tags from jsonNew into jsonOld
// Returns a JSON array containing all tags from jsonOld plus any new tags found in jsonNew
String R200::mergeNewTags(String jsonOld, String jsonNew)
{
    // Parse both old and new JSON arrays
    DynamicJsonDocument doc1(2048);
    DynamicJsonDocument doc2(2048);
    deserializeJson(doc1, jsonOld);
    deserializeJson(doc2, jsonNew);

    // Convert JSON arrays to vectors for easier comparison
    std::vector<String> listOld;
    std::vector<String> listNew;

    // Populate JSON arrays into vectors
    for (JsonVariant v : doc1.as<JsonArray>())
        listOld.push_back(v.as<String>());
    for (JsonVariant v : doc2.as<JsonArray>())
        listNew.push_back(v.as<String>());

    // Create merged result with all tags from old inventory
    DynamicJsonDocument mergedDoc(2048);
    JsonArray mergedArray = mergedDoc.to<JsonArray>();

    // Add all existing tags from old inventory
    for (const String &tag : listOld)
        mergedArray.add(tag);

    // Find and add new tags (tags in new inventory that are NOT in old inventory)
    for (const String &newTag : listNew)
    {
        bool exists = false;
        for (const String &oldTag : listOld)
        {
            if (newTag == oldTag)
            {
                exists = true;
                break;
            }
        }
        // If tag doesn't exist in old inventory, add it to merged array
        if (!exists)
        {
            mergedArray.add(newTag);
        }
    }

    // Serialize merged JSON
    String output;
    serializeJson(mergedDoc, output);
    return output;
}