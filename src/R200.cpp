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

// Adjust transmission power (15 to 30 dBm)
// Command 0xB6
void R200::setTxPower(uint8_t dbm)
{
    if (dbm > 30)
        dbm = 30;
    else if (dbm < 15)
        dbm = 15;
    uint16_t powerVal = dbm * 100;

    // Param: 2 bytes (Value * 100)
    uint8_t params[2];
    params[0] = (powerVal >> 8) & 0xFF; // MSB
    params[1] = powerVal & 0xFF;        // LSB

    _sendCommand(0xB6, params, 2);
    delay(50);
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
    while (millis() - start < 500)
    {
        // Try to read a frame
        String tag = _readResponse();

        // Receive valid tag
        if (tag != "")
        {
            // Check for duplicates
            bool exists = false;
            for (const String &t : foundTags)
            {
                if (t == tag)
                {
                    exists = true;
                    break;
                }
            }

            // Add new tag
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
// Return EPC string if tag found, else return empty string
String R200::_readResponse()
{

    if (_serial->available())
    {
        byte b = _serial->read();

        // Ignore invalid bytes until a header byte is found
        if (b != 0xAA)
            return "";

        // Wait 50ms for Type, Cmd, PL_MSB, PL_LSB
        unsigned long timeout = millis();
        while (_serial->available() < 4)
        {
            // Incomplete frame
            if (millis() - timeout > 50)
                return "";
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
                return "";
        }

        // Read payload, checksum, end byte
        if (payloadLen > 64)
            payloadLen = 64;
        uint8_t payload[64];
        _serial->readBytes(payload, payloadLen);

        uint8_t checksum = _serial->read();
        uint8_t end = _serial->read();

        // Debugging Output
        Serial.print("Notification Frame Type: ");
        Serial.print(type, HEX);
        Serial.print(" | Cmd: ");
        Serial.print(cmd, HEX);
        if (cmd == 0xFF && payloadLen > 0)
        {
            Serial.print(" | ERROR: ");
            Serial.print(payload[0], HEX);
        }
        else
        {
            Serial.println("");
        }

        // Validate notification frame's type
        if (type == 0x02 && payloadLen > 5)
        {
            String epcString = "";
            // EPC is from byte 3 to (Length - 2)
            for (int i = 3; i < payloadLen - 2; i++)
            {
                if (payload[i] < 0x10)
                    epcString += "0";
                epcString += String(payload[i], HEX);
            }
            epcString.toUpperCase();
            return epcString;
        }
    }
    // No valid tag found
    return "";
}