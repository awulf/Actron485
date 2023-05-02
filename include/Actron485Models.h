// Actron RS485 messages

#pragma once
#include <Arduino.h>

namespace Actron485 {

// Message Type

enum class MessageType: uint8_t {
    Unknown = 0xFF,
    CommandMasterSetpoint = 0x3A,
    CommandFanMode = 0x3B,
    CommandOperatingMode = 0x3C,
    CommandZoneState = 0x3D,
    CustomCommandChangeZoneSetpoint = 0x3F,
    ZoneWallController = 0xC0, // 0xC{zone}
    ZoneMasterController = 0x80, // 0x8{zone}
    BoardComms1 = 0x01, // Unknown
    BoardComms2 = 0x02, // Unknown
    Stat1 = 0xA0, // Unknown
    Stat2 = 0xFE, // Unknown
    Stat3 = 0xE0 // Unknown
};

/// @brief Message type determined by the first byte
/// @param fistByte to from the message
/// @return message type
MessageType detectMessageType(uint8_t firstBit);

// Zone Control Messages

enum class ZoneMode {
    Off,
    On,
    Open,
    Ignore = -1 // Not part of the spec, used in this code wheather to update
};

enum class ZoneMessageType {
    Normal,
    Config,
    InitZone
};

struct ZoneToMasterMessage {
    static const uint8_t messageLength = 5;

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
    ZoneMode mode;
    
    // Controller Message type
    ZoneMessageType type;

    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 5 bytes
    void parse(uint8_t data[messageLength]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 5 bytes long
    void generate(uint8_t data[messageLength]);

    /// @brief Given the raw encoded value converts to °C as master would interpet
    double zoneTempFromMaster(int16_t rawValue);

    /// @brief Given the the temperature returns the encoded value for master controller
    int16_t zoneTempToMaster(double temperature);
};

enum class ZoneOperationMode {
    SystemOff,
    ZoneOff,
    FanOnly,
    Standby,
    Cooling,
    Heating
};

struct MasterToZoneMessage {
    static const uint8_t messageLength = 7;

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

    // Zone is heating
    bool heating;

    // A bit that turns 1 when sometimes turning off a zone or when all zones turn on for balancing
    bool maybeTurningOff;

    // Zone is being actively serviced by the compressor / zone requesting compressor
    bool compressorActive;

    // Position of the damper from 0 -> 5 (closed -> open)
    uint8_t damperPosition;

    // Interpreted from the provided data
    ZoneOperationMode operationMode;

    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 7 bytes
    void parse(uint8_t data[7]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 5 bytes long
    void generate(uint8_t data[7]);
};

// General AC Commands

struct MasterSetpointCommand {
    // In °C 16-30° in 0.5° increments
    double temperature;
    
    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 2 bytes
    void parse(uint8_t data[2]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 2 bytes long
    void generate(uint8_t data[2]);
};

enum class FanMode: uint8_t {
    Off = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Esp = 4,
    LowContinuous = 5,
    MediumContinuous = 6,
    HighContinuous = 7,
    EspContinuous = 8
};

struct FanModeCommand {
    static const uint8_t messageLength = 2;

    FanMode fanMode;

    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 2 bytes
    void parse(uint8_t data[2]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 2 bytes long
    void generate(uint8_t data[2]);
};

struct ZoneStateCommand {
    static const uint8_t messageLength = 2;

    bool zoneOn[8];

    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 2 bytes
    void parse(uint8_t data[2]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 2 bytes long
    void generate(uint8_t data[2]);
};

enum class OperatingMode: uint8_t {
    Off = 0b00000000,
    FanOnly = 0b00010000,
    Auto = 0b00001100,
    Cool = 0b00001010,
    Heat = 0b00001001
};

struct OperatingModeCommand {
    static const uint8_t messageLength = 2;

    OperatingMode mode;

    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 2 bytes
    void parse(uint8_t data[2]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 2 bytes long
    void generate(uint8_t data[2]);
};

/// @brief Custom command to this library, allows other controllers (e.g. a seperate zone wall controllers)
/// to change the temperature via direct communication
struct ZoneSetpointCustomCommand {
    static const uint8_t messageLength = 3;

    // In °C 16-30° in 0.5° increments
    double temperature;

    // Zone to change
    uint8_t zone;
    
    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 2 bytes
    void parse(uint8_t data[2]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 2 bytes long
    void generate(uint8_t data[2]);
};

}