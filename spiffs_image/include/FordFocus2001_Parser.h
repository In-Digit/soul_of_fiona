#ifndef FORD_FOCUS_2001_PARSER_H
#define FORD_FOCUS_2001_PARSER_H

#include <Arduino.h>

class FordFocus2001_Parser {
public:
    static float parseELM327Response(const String& rawResponse, const String& pid);
    static float parseTemperature(uint8_t dataByte);
    static float parseSpeed(uint8_t dataByte);
    static float parseMAF(uint8_t highByte, uint8_t lowByte);
    static float parseRPM(uint8_t highByte, uint8_t lowByte);
    static float parsePercentage(uint8_t dataByte);
    static float parseFuelTrim(uint8_t dataByte);
    static bool isValidResponse(const String& response);
    static String extractDataBytes(const String& rawResponse, const String& pid);
    static uint8_t hexCharToByte(char c);
    static uint8_t parseHexByte(const String& hexStr, int startPos);

private:
    static String cleanResponse(const String& rawResponse);
    static int findDataStart(const String& cleanHex, const String& pidCode);
};

#endif