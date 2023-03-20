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

    /// @brief debug print state to serial
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
    // Zone number. 0 -> 8
    int zone;

    // Temperature of the zone as thought of by the master controller
    double temperature;

    // Minimum allowed setpoint, as dictated by the master controller, range 16 -> 30°C
    double minSetpoint;

    // Maximum allowed setpoint, as dictated by the master controller, range 16 -> 30°C
    double maxSetpoint;

    // Set point of the zone, range 16 -> 30°C
    double setpoint;

    // AC is in compressor mode (e.g. heating or cooling)
    bool compressorMode;

    // Zone is on or off
    bool on;

    // Zone is in fan only mode
    bool fanMode;

    // Zone is being actively serviced by the compressor / zone requesting compressor
    bool compressorActive;

    // Position of the damper from 0 -> 5 (closed -> open)
    uint8_t damperPosition;

    /// @brief debug print state to serial
    void printToSerial();

    /// @brief parse data provided
    /// @param data to read of 7 bytes
    void parse(uint8_t data[7]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 5 bytes long
    void generate(uint8_t data[7]);

};