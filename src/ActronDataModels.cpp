#include "ActronDataModels.h"
#include "Utilities.h"

// ActronZoneToMasterMessage

void ActronZoneToMasterMessage::printToSerial() {

    // Decoded
    Serial.print("Zone: ");
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

    Serial.println();
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

        Serial.print(" Raw In: ");
        Serial.print(rawTempValue);

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

        Serial.print(" Raw Out: ");
        Serial.print(rawTempValue);

        // Byte 3
        data[2] = (mode != off ? 0b10000000 : 0b0) | (mode == open ? 0b01000000 : 0b0) | (rawTempValue >> 8 & 0b00000011);

        // Byte 4
        data[3] = (uint8_t)rawTempValue;

        // Byte 5, check/verify byte
        double sensorTemp = (250 - rawTemp) * 0.1; // Before master temp interpretation trickery
        const int c = -192;
        int8_t extraByte = data[2] & 0b11111101;
        int8_t checkByte = ((int8_t)(sensorTemp * 10) - (int8_t)(setpoint * 2)) + c + extraByte + (mode == open ? 128 : 0);
        data[4] = checkByte;

    } else if (type == config) {
        // Byte 3, config bit set to 1
        data[2] = (mode != off ? 0b10000000 : 0b0) | (mode == open ? 0b01000000 : 0b0) | 0b00100000;

        // Byte 4, temperature calibration offset x10, E.g. -32 * 0.1 -> -3.2. Min -3.2 Max 3.0°C
        data[3] = (int8_t) (temperature * 10);

        // Byte 5, check/verify byte
        int8_t checkByte = data[2] - data[3] - data[1] - 4;
        data[4] = checkByte;
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
