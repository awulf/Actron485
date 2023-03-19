#pragma once
#include <Arduino.h>

enum ActronZoneMode {
    off,
    on,
    open
};

enum ActronZoneMessageType {
    normal,
    config
};

struct ActronZoneToMasterMessage {
    // Zone number. 0 -> 8
    int zone;
    // Zone setpoint temp, range 16 -> 30°C
    double setpoint;
    // Zone Temperature, range 0 -> 52°C
    // In config mode comes temperature offset, range: -3.2 -> +3.0°C
    double temperature; 
    // What it should be, but master controller adjust for thermistor characteristics
    double temperaturePreAdjustment;
    // Mode the zone is operating in
    ActronZoneMode mode;
    // Controller Message type
    ActronZoneMessageType type;

    void printToSerial();

    /// @brief parse data provided
    /// @param data to read of 5 bytes
    void parse(uint8_t data[5]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 5 bytes long
    void generate(uint8_t data[5]);

    /// @brief Given the raw encoded value converts to °C as master would interpet
    double zoneTempFromMaster(int16_t rawValue);

    /// @brief Given the the temperature returns the encoded value for master controller
    int16_t zoneTempToMaster(double temperature);
};

struct ActronMasterToZoneMessage {

};