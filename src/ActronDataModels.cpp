#include "ActronDataModels.h"
#include "Utilities.h"

///////////////////////////////////
// ActronZoneToMasterMessage

void ActronZoneToMasterMessage::printToSerial() {

    // Decoded
    Serial.print("C: Zone: ");
    Serial.print(zone);

    if (type == normal) {
        Serial.print(", Set Point: ");
        Serial.print(setpoint);

        Serial.print(", Temp: ");
        Serial.print(temperature);

        Serial.print("(Pre:");
        Serial.print(temperaturePreAdjustment);
        Serial.print(")");

        Serial.print(", Mode: ");
        switch (mode) {
            case off:
                Serial.print("Off");
                break;
            case on:
                Serial.print("On");
                break;
            case open:
                Serial.print("Open");
                break;
        }

    } else if (type == config) {
        Serial.print(", Temp Offset: ");
        Serial.print(temperature);
    }
}

void ActronZoneToMasterMessage::parse(uint8_t data[5]) {
    
    zone = data[0] & 0b00001111;

    setpoint = data[1] / 2.0;

    bool zoneOn = (data[2] & 0b10000000) == 0b10000000;
    bool openMode = (data[2] & 0b01000000) == 0b01000000;
    if (zoneOn && openMode) {
        mode = open;
    } else if (zoneOn) {
        mode = on;
    } else {
        mode = off;
    }

    bool configMessage = (data[2] & 0b00100000) == 0b00100000;
    if (configMessage) {
        type = config;
        
        // Offset temperature
        temperature = (double)((int8_t)data[3]) / 10.0;

    } else {
        type = normal;

        // Two last bits of data[2] are the leading bits for the raw temperature value
        uint8_t leadingBites = data[2] & 0b00000011;
        uint8_t negative = (leadingBites & 0b10) == 0b10;

        int16_t rawTempValue = (((negative ? 0b11111100 : 0x0) | leadingBites) << 8) | (uint16_t)data[3];
        int16_t rawTemp = rawTempValue + (negative ? 512 : -512); // A 512 offset

        temperaturePreAdjustment = (250 - rawTemp) * 0.1;
        temperature = zoneTempFromMaster(rawTemp);
    }
}

void ActronZoneToMasterMessage::generate(uint8_t data[5]) {
    // Byte 1, Start nibble C and zone nibble
    data[0] = 0b11000000 | zone;

    // Byte 2, Set Point Temp where Temp=Number/2. In 0.5° increments. 16->30
    data[1] = (uint8_t) (setpoint*2);

    if (type == normal) {
        int16_t rawTemp = zoneTempToMaster(temperature);
        int16_t rawTempValue = rawTemp - (rawTemp < 0 ? -512 : 512);

        // Byte 3
        data[2] = (mode != off ? 0b10000000 : 0b0) | (mode == open ? 0b01000000 : 0b0) | (rawTempValue >> 8 & 0b00000011);

        // Byte 4
        data[3] = (uint8_t)rawTempValue;

        // Byte 5, check/verify byte
        data[4] = data[2] - (data[2] << 1) - data[3] - data[1] - data[0] - 1;

    } else if (type == config) {
        // Byte 3, config bit set to 1
        data[2] = (mode != off ? 0b10000000 : 0b0) | (mode == open ? 0b01000000 : 0b0) | 0b00100000;

        // Byte 4, temperature calibration offset x10, E.g. -32 * 0.1 -> -3.2. Min -3.2 Max 3.0°C
        data[3] = (int8_t) (temperature * 10);

        // Byte 5, check/verify byte
        data[4] = data[2] - data[3] - data[1] - (data[0] & 0b1111) - 1;
    }
}

double ActronZoneToMasterMessage::zoneTempFromMaster(int16_t rawValue) {
    double temp;
    double out = 0;
    if (rawValue < -58) {
        // Temp High (above 30.8°C)
        out = 0.00007124*pow(rawValue, 2) - 0.1052*rawValue+24.5;
        temp = round(out * 10) / 10.0;
    } else if (rawValue > 81) {
        // Low Temp (below 16.9°C)
        out = (-0.00001457*pow(rawValue, 2) - 0.0988*rawValue+24.923);
        temp = round(out * 10) / 10.0;
    } else {
        temp = (250 - rawValue) * 0.1;
    }  
    return temp;
}

int16_t ActronZoneToMasterMessage::zoneTempToMaster(double temperature) {
    int16_t out = 0;
    if (temperature > 30.8) {
        out = (int16_t) round(-118.478*(sqrt(14.3372 + temperature)-6.23195));
    } else if (temperature < 16.9) {
        out = (int16_t) round(261.981*(sqrt(192.415 - temperature)-12.9419));
    } else {
        out = (int16_t) round(250 - temperature * 10);
    }
    return out;
}

///////////////////////////////////
// ActronMasterToZoneMessage

void ActronMasterToZoneMessage::printToSerial() {
    Serial.print("M: Zone: ");
    Serial.print(zone);

    Serial.print(", Set Point: ");
    Serial.print(setpoint);

    Serial.print(", Temp: ");
    Serial.print(temperature);

    Serial.print(", SP Range: ");
    Serial.print(minSetpoint);
    Serial.print("-");
    Serial.print(maxSetpoint);

    Serial.print(", Compr. Mode: ");
    Serial.print(compressorMode ? "On" : "Off");

    Serial.print(", Zone: ");
    Serial.print(on ? "On" : "Off");

    if (on) {
        if (fanMode) {
            Serial.print(", Fan Mode");
        } else if (compressorActive) {
            Serial.print(", Compr. Active");
        }
    }

    Serial.print(", Damper Pos: ");
    Serial.print((int)damperPosition/5*100);
    Serial.print("%");
}

void ActronMasterToZoneMessage::parse(uint8_t data[7]) {
    zone = data[0] & 0b00001111;

    // Temperature bits: [23][08 - 15]
    uint16_t zoneTempRaw = (uint16_t) data[1] | ((uint16_t) (data[2] & 0b1) << 8);
    temperature = zoneTempRaw * 0.1;

    minSetpoint = (data[3] & 0b00111111) / 2.0;
    maxSetpoint = (data[5] & 0b00111111) / 2.0;
    setpoint = (data[4] & 0b00111111) / 2.0;

    on = (data[2] & 0b01000000) == 0b01000000;

    compressorMode = (data[2] & 0b10000000) == 0b10000000;

    fanMode = (data[4] & 0x80) == 0x80;

    compressorActive = (data[5] & 0x80) == 0x80;

    damperPosition = (data[2] & 0b00011100) >> 2;
}

void ActronMasterToZoneMessage::generate(uint8_t data[7]) {
    // Byte 1, Start nibble 8 and zone nibble
    data[0] = 0b10000000 | zone;

    // Byte 2, lower part of zone temperature
    uint16_t zoneTempRaw = temperature * 10;
    data[1] = (uint8_t) zoneTempRaw;

    // Byte 3, first bit ac in compressor mode, second zone on. third unknown, 4-6 damper pos, 7 unknown, 8 first bit for zone temp
    data[2] = (compressorMode ? 0b10000000 : 0b0) | (on ? 0b01000000 : 0b0) | (/*Unknown*/0b0) | 
              (0b00011100 & (damperPosition << 2)) | (/*Unknown*/0b0) | (0b00000001 & (zoneTempRaw >> 8));
              // Bit 7 is probably for zone temperature, but we never see this range being used

    // Byte 4, unknown top bits, min setpoint lower 6
    data[3] = (/*Unknown*/0b0) | (/*Unknown*/0b0) | ((uint8_t)minSetpoint * 2) & 0b00111111;

    // Byte 5, first bit fan mode, second unknown, lower 6 setpoint
    data[4] = (fanMode ? 0b10000000 : 0b0) | (/*Unknown*/0b0) | ((uint8_t)setpoint * 2) & 0b00111111;

    // Byte 6, first bit compressor on for zone, second unknow, lower 6 max setpoint
    data[5] = (fanMode ? 0b10000000 : 0b0) | (/*Unknown*/0b0) | ((uint8_t)maxSetpoint * 2) & 0b00111111;

    // Byte 7, check byte
    data[6] = data[2] - (data[2] << 1) - data[5] - data[4] - data[3] - data[1] - data[0] - 1;
}