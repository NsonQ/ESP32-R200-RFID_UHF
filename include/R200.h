#ifndef R200_H
#define R200_H

#include <Arduino.h>
#include <vector>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// Structure to hold parsed frames internally
struct Frame
{
    uint8_t frameType;
    uint8_t cmd;
    std::vector<uint8_t> payload;
};

class R200
{
private:
    HardwareSerial *_serial;
    int _rxPin;
    int _txPin;
    long _baud;

    uint8_t _calculateChecksum(const uint32_t data);
    void _sendCommand(uint8_t cmd, const uint8_t *params, size_t len);
    Frame _readResponse();

public:
    R200(HardwareSerial &serial, int rxPin, int txPin, int baud = 115200);
    bool begin();
    void setTxPower(uint8_t dbm);
    bool setSensitivity(uint8_t mixerGain = 6, uint8_t ifGain = 7, uint16_t threshold = 0x0100);
    String scan();
    String getJsonDifference(String jsonOld, String jsonNew);
    String mergeNewTags(String jsonOld, String jsonNew);
};

#endif