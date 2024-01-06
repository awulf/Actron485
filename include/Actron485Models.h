// Actron RS485 messages

#pragma once
#include <Arduino.h>

namespace Actron485 {

// Debugging

enum class PrintOutMode: uint8_t {
    StatusOnly,
    ChangedMessages,
    AllMessages,
};

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
    IndoorBoard1 = 0x01, // Unknown
    IndoorBoard2 = 0x02, // Infrequently updated indoor board status
    Stat1 = 0xA0, // Unknown
    Stat2 = 0xFE, // Unknown
    Stat3 = 0xE0 // Unknown
};

// Zone Control Messages

enum class ZoneMode {
    Off,
    On,
    Open,
    Ignore = -1 // Not part of the spec, used in this code whether to update
};

enum class ZoneMessageType {
    Normal,
    Config,
    InitZone
};

struct ZoneToMasterMessage {
    /// @brief struct has initialised data
    bool initialised;

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

enum class CompressorMode {
    Unknown,
    Idle,
    Cooling,
    Heating
};

struct MasterToZoneMessage {
    /// @brief struct has initialised data
    bool initialised;

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
    bool maybeAdjusting;

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
    static const uint8_t messageLength = 2;

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

    /// @brief Get the fan speed portion of the command
    /// @return Low, Medium, High, ESP (Auto)
    FanMode getFanSpeed();

    /// @brief Get the continuous portion of the command
    /// @return true if continuous, false otherwise
    bool isContinuous();

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
    Off =       0b00000000,
    OffAuto =   0b00000100,
    OffCool =   0b00000010,
    OffHeat =   0b00000001,
    FanOnly =   0b00010010,
    Auto =      0b00001100,
    Cool =      0b00001010,
    Heat =      0b00001001
};

struct OperatingModeCommand {
    static const uint8_t messageLength = 2;

    OperatingMode mode;

    /// @brief checks if the command is an on command
    /// @return true if on false if off
    bool onCommand();

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

    // Adjust Master to allow for the new temperature
    bool adjustMaster;
    
    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 2 bytes
    void parse(uint8_t data[2]);
    
    /// @brief generates the data from the variables in this struct
    /// @param data to write to, 2 bytes long
    void generate(uint8_t data[2]);
};

struct StateMessage {
    /// @brief struct has initialised data
    bool initialised;

    const static uint8_t stateMessageLength = 23;

    /// @brief setpoint temperature of all zones 1-8 indexed 0-7
    double zoneSetpoint[8];
    /// @brief false if off, true if on, zones 1-8 indexed 0-7
    bool zoneOn[8];

    /// @brief Average temperature of all active zones, where ever temperature sensors are
    double temperature;

    /// @brief System setpoint temperature, which also limits individual zone temperature set points
    double setpoint;

    /// @brief Mode the system is operating in
    OperatingMode operatingMode;
    /// @brief If system is off this is the last mode it operated in, thus can be used to turn the system back on to last used state
    OperatingMode lastOperatingMode;
    /// @brief if system is actively cooling/heating, or idle
    CompressorMode compressorMode;

    /// @brief System fan mode (not including continuous mode)
    FanMode fanMode;
    /// @brief True if system is in continuous mode
    bool continuousFan;
    /// @brief True if system fan is running, false if off
    bool fanActive;

    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 23 bytes
    void parse(uint8_t data[23]);
};

/// @brief Less Frequent State Message that should be sent by most Indoor Boards
struct StateMessage2 {
    /// @brief struct has initialised data
    bool initialised;

    const static uint8_t stateMessageLength = 18;

    /// @brief false if off, true if on, zones 1-8 indexed 0-7
    bool zoneOn[8];

    /// @brief Average temperature of all active zones, where ever temperature sensors are
    double temperature;

    /// @brief System setpoint temperature, which also limits individual zone temperature set points
    double setpoint;

    /// @brief Mode the system is operating in
    OperatingMode operatingMode;
    /// @brief If system is off this is the last mode it operated in, thus can be used to turn the system back on to last used state
    OperatingMode lastOperatingMode;
    /// @brief if system is actively cooling/heating, or idle
    CompressorMode compressorMode;

    /// @brief System fan mode (not including continuous mode)
    FanMode fanMode;
    /// @brief True if system is in continuous mode
    bool continuousFan;
    /// @brief True if system fan is running, false if off
    bool fanActive;

    /// @brief print state to printOut
    void print();

    /// @brief parse data provided
    /// @param data to read of 18 bytes
    void parse(uint8_t data[18]);
};

}