#include "FordFocus2001_Parser.h"

float FordFocus2001_Parser::parseELM327Response(const String &rawResponse, const String &pid) {
    if (!isValidResponse(rawResponse)) return 0.0;

    // Специальная обработка для ATRV (напряжение)
    if (pid == "ATRV") {
        // Ответ может быть вида "14.0V", "14.0", "14.0>", с пробелами и т.д.
        String cleaned = rawResponse;
        cleaned.replace("V", "");
        cleaned.replace("v", "");
        cleaned.replace(">", "");
        cleaned.trim();
        // Используем strtof для безопасного преобразования
        char *endptr;
        float val = strtof(cleaned.c_str(), &endptr);
        if (endptr == cleaned.c_str()) return 0.0; // ничего не спарсилось
        return val;
    }

    String cleanHex = cleanResponse(rawResponse);
    String pidCode = pid.substring(2); // "0110" -> "10"
    int dataStart = findDataStart(cleanHex, pidCode);
    if (dataStart == -1) return 0.0;

    if (pid == "0105" || pid == "010F" || pid == "010D" ||
        pid == "0104" || pid == "0111" || pid == "0106" || pid == "0107") {
        if (dataStart + 2 <= cleanHex.length()) {
            uint8_t dataByte = parseHexByte(cleanHex, dataStart);
            if (pid == "0105" || pid == "010F") return parseTemperature(dataByte);
            if (pid == "010D") return parseSpeed(dataByte);
            if (pid == "0104" || pid == "0111") return parsePercentage(dataByte);
            if (pid == "0106" || pid == "0107") return parseFuelTrim(dataByte);
        }
    } else if (pid == "010C" || pid == "0110" || pid == "0142") {
        if (dataStart + 4 <= cleanHex.length()) {
            uint8_t highByte = parseHexByte(cleanHex, dataStart);
            uint8_t lowByte = parseHexByte(cleanHex, dataStart + 2);
            if (pid == "010C") return parseRPM(highByte, lowByte);
            if (pid == "0110") return parseMAF(highByte, lowByte);
            if (pid == "0142") {
                uint16_t raw = (highByte << 8) | lowByte;
                return raw / 1000.0;
            }
        }
    }
    return 0.0;
}

float FordFocus2001_Parser::parseTemperature(uint8_t dataByte) {
    return dataByte - 40.0;
}

float FordFocus2001_Parser::parseSpeed(uint8_t dataByte) {
    return dataByte;
}

float FordFocus2001_Parser::parseMAF(uint8_t highByte, uint8_t lowByte) {
    uint16_t raw = (highByte << 8) | lowByte;
    return raw / 100.0;
}

float FordFocus2001_Parser::parseRPM(uint8_t highByte, uint8_t lowByte) {
    uint16_t raw = (highByte << 8) | lowByte;
    return raw / 4.0;
}

float FordFocus2001_Parser::parsePercentage(uint8_t dataByte) {
    return (100.0 * dataByte) / 255.0;
}

float FordFocus2001_Parser::parseFuelTrim(uint8_t dataByte) {
    return (dataByte - 128.0) * 0.78125;
}

bool FordFocus2001_Parser::isValidResponse(const String &response) {
    if (response.indexOf("NO DATA") >= 0 ||
        response.indexOf("?") >= 0 ||
        response.indexOf("UNABLE") >= 0 ||
        response.indexOf("ERROR") >= 0) {
        return false;
    }
    return response.length() >= 4;
}

String FordFocus2001_Parser::extractDataBytes(const String &rawResponse, const String &pid) {
    String cleanHex = cleanResponse(rawResponse);
    String pidCode = pid.substring(2);
    int dataStart = findDataStart(cleanHex, pidCode);
    if (dataStart == -1) return "";
    int dataLength = (pid == "010C" || pid == "0110" || pid == "0142") ? 4 : 2;
    if (dataStart + dataLength <= cleanHex.length()) {
        return cleanHex.substring(dataStart, dataStart + dataLength);
    }
    return "";
}

String FordFocus2001_Parser::cleanResponse(const String &rawResponse) {
    String result = rawResponse;
    result.replace("SEARCHING...", "");
    result.replace(" ", "");
    result.replace("\r", "");
    result.replace("\n", "");
    result.replace(">", "");  // Удаляем символ '>' в конце ответов
    result.toUpperCase();
    return result;
}

int FordFocus2001_Parser::findDataStart(const String &cleanHex, const String &pidCode) {
    String pattern = "41" + pidCode;
    int pos = cleanHex.indexOf(pattern);
    if (pos != -1) return pos + pattern.length();
    pos = cleanHex.indexOf(pidCode);
    if (pos != -1) return pos + pidCode.length();
    return -1;
}

uint8_t FordFocus2001_Parser::hexCharToByte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

uint8_t FordFocus2001_Parser::parseHexByte(const String &hexStr, int startPos) {
    if (startPos + 2 > hexStr.length()) return 0;
    char high = hexStr.charAt(startPos);
    char low = hexStr.charAt(startPos + 1);
    return (hexCharToByte(high) << 4) | hexCharToByte(low);
}