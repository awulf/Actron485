#pragma once
#include <Arduino.h>
#include "Actron485Models.h"

/// moves zones 1-8 to array indexed 0-7
#define zindex(z) z-1

namespace Actron485 {

class Controller {

    Stream *_serial;

    uint8_t _writeEnablePin;
    uint8_t _rxPin;
    uint8_t _txPin;

    /// @brief Zone 1 - 8 (indexed 0-7), Zone own control requests. -1 (actronZoneModeIgnore) when not requesting (e.g. once request has been sent)
    ZoneMode _requestZoneMode[8];

    /// @brief If true, on next zone message, will send a configuration
    bool _sendZoneConfig[8];
    
    /// @brief Buffer size for ingesting serial messages
    static const size_t _serialBufferSize = 256;
    /// @brief Serial Buffer for ingesting
    uint8_t _serialBuffer[_serialBufferSize];
    /// @brief Last message received time
    unsigned long _serialBufferReceivedTime = 0;
    /// @brief Minimum time between bytes received to split up serial message in milliseconds
    static const unsigned long _serialBufferBreak = 5;
    /// @brief Index of current sequence being read
    uint8_t _serialBufferIndex = 0;

    /// Keeps track of if a response occurs after a set zone command is sent, so we know if we can discard our snapshot of the zone state
    bool _sendZoneStateCommandCleared = true;

    /// @brief system millis when there was a pause in receiving, a time when sending can occur
    unsigned long _lastQuietPeriodDetectedTime;

    /// @brief Bring up/down the serial write enable pin
    /// @param enable 
    void serialWrite(bool enable);

    /// @brief Attemps to send a zone message immediately for the given zone number
    /// @param zone 
    void sendZoneMessage(int zone);

    /// @brief Attempts to send a zone config message immediately for the given zone number
    /// @param zone 
    void sendZoneConfigMessage(int zone);

    /// @brief Attempts to send a zone init message immediately for the given zone number, should be sent
    /// straight after the master controller requests the zone
    /// @param zone 
    void sendZoneInitMessage(int zone);

    /// @brief Process the master to zone message received, adjusts stored zone parameters accordingly
    /// @param masterMessage to process
    void processMasterMessage(MasterToZoneMessage masterMessage);

    /// @brief Process the zone to master message, if initialisation message, will handle it if zone is configured
    /// @param zoneMessage 
    void processZoneMessage(ZoneToMasterMessage zoneMessage);

    /// @brief Common setup
    void setup();

    /// @brief Sends a queued command
    /// @returns true if a message was sent
    bool sendQueuedCommand();

    /// @brief Message Send Check
    /// @returns true if message length is as expected, prints error if printing enabled
    bool messageLengthCheck(int received, int expected, const char *name);

public:

    /// @brief initialise controller with serial pins. Supports rx & tx being the same pin if constrained with GPIOs.
    /// @param rxPin pin to receive on
    /// @param txPin pin to write to
    /// @param writeEnablePin for write enable, set to 0 if not used
    Controller(uint8_t rxPin, uint8_t txPin, uint8_t writeEnablePin);

    /// @brief initialise controller with a custom stream, e.g. if using single wire
    /// @param stream 
    /// @param writeEnablePin for write enable, set to 0 if not used
    Controller(Stream &stream, uint8_t writeEnablePin);

    /// @brief initialise unconfigured controller
    Controller();

    /// @brief configure after initialisation
    /// @param stream 
    /// @param writeEnablePin for write enable, set to 0 if not used
    void configure(Stream &stream, uint8_t writeEnablePin);

    /// @brief pass a different stream to send log messages to
    /// @param stream
    void configureLogging(Stream *stream);

    /// @brief Zone 1 - 8 (indexed 0-7), set bool to corresponding number to make the zone controlled by this controller
    bool zoneControlled[8];

    /// @brief Zone 1 - 8 (indexed 0-7), temperature setpoint, read from when controlling that zone
    double zoneSetpoint[8];

    /// @brief Zone 1 - 8 (indexed 0-7), current zone temperature, read from when controlling that zone
    double zoneTemperature[8];

    /// @brief Zone 1 - 8 (indexed 0-7), last zone to master message, either sent by ourselves, or other controllers on the bus
    ZoneToMasterMessage zoneMessage[8];

    /// @brief Zone 1 - 8 (indexed 0-7), last master to zone message
    MasterToZoneMessage masterToZoneMessage[8];

    /// @brief State of the AC control message (may not be available to all systems)
    StateMessage stateMessage;

    /// @brief State of the AC control message (more commonly available in systems, but updates less frequent)
    StateMessage2 stateMessage2;

    /// @brief State of the AC Ultima Controller
    UltimaState ultimaState;

    /// @brief system millis when data was last received
    unsigned long dataLastReceivedTime;
    /// @brief system millis when data was last sent
    unsigned long dataLastSentTime;
    /// @brief system millis when the last status message arrived (useful for comparing against a command sent time for updates)
    unsigned long statusLastReceivedTime;

    /// @brief Message type determined by the first byte
    /// @param firstByte to from the message
    /// @return message type
    MessageType detectActronMessageType(uint8_t firstByte);

    /// @brief Logging/printing mode
    PrintOutMode printOutMode;

    /// @brief Must be called with the main run loop
    void loop();

    /// @brief Pass message data to be processed, if passing data down manually, this can be used rather than calling loop
    /// @param data byte array
    /// @param length length of byte array
    void processMessage(uint8_t *data, uint8_t length);

    /// @brief Attempt to send any queued commands, will be rate limited and may not send, this can be used rather than calling loop
    /// also should only be called during the expected quiet time, otherwise there will be clashes on the 485 bus
    void attemptToSendQueuedCommand();

    //////////////////////
    // Queued Commands awaiting to be sent, when set will send one by one to the controller on each loop,
    // priority in the order listed in the declarations below

    /// @brief setpoint command, sent on next cycle
    OperatingModeCommand nextOperatingModeCommand;
    bool sendOperatingModeCommand;
    /// @brief zone state command, sent on next cycle, for self controlled zones best to use ZoneToMasterMessage
    ZoneStateCommand nextZoneStateCommand;
    bool sendZoneStateCommand;
    /// @brief setpoint command, sent on next cycle
    MasterSetpointCommand nextSetpointCommand;
    bool sendSetpointCommand;
    /// @brief fan mode command, sent on next cycle
    FanModeCommand nextFanModeCommand;
    bool sendFanModeCommand;
    /// @brief zone setpoint command
    ZoneSetpointCustomCommand nextZoneSetpointCustomCommand;
    bool sendZoneSetpointCustomCommand;
    /// @brief send a master message allows tricking zone wall controllers
    MasterToZoneMessage nextMasterToZoneMessage[8];
    bool sendMasterToZoneMessage[8];

    //////////////////////
    /// Below are last stored messages. Some of those assumed types, better understanding still required

    uint8_t zoneWallMessageRaw[8][5];
    uint8_t zoneMasterMessageRaw[8][7];

    // This message varies in length, and occurs up to two times per sequence
    uint8_t boardComms1Index; // records count per sequence
    uint8_t boardComms1MessageLength[2];
    uint8_t boardComms1Message[2][50];

    uint8_t stateMessage2Raw[StateMessage2::stateMessageLength];
    uint8_t stateMessageRaw[StateMessage::stateMessageLength];
    const static uint8_t stat2MessageLength = 19;
    uint8_t stat2Message[19];
    uint8_t ultimaStateMessageRaw[UltimaState::stateMessageLength];

    //////////////////////
    /// Convenient functions, that are the typical use for this module

    /// @brief Check if the system is receiving data, within 3seconds is considered valid
    /// @return true if we are receiving fresh data, false otherwise
    bool receivingData();

    /// @brief Number of commands that are pending to send, including zones (Ultima only)
    /// @return commands to be sent
    uint8_t totalPendingCommands();

    /// @brief Number of commands that are pending to send for the main controller only
    /// @return commands to be sent
    uint8_t totalPendingMainCommands();

    /// @brief If a zone control is pending to be sent (Ultima Only)
    /// @param zone to check
    /// @return commands to be sent
    bool isPendingZoneCommand(int zone); 

    // Setup

    /// @brief set the specified zone to be controlled by this module
    /// turning a zone on to be controlled will configure this module to report temperature reading and
    /// control the setpoint temperature.
    /// *** Only one zone controller can be active for a given zone ***
    /// This module can control multiple zones at once
    /// @param zone to set
    /// @param control true to enable, false otherwise
    void setControlZone(uint8_t zone, bool control);

    /// @brief get if the specified zone is controlled by this controller
    /// @param zone
    /// @return true if this module is controlling the specified zone
    bool getControlZone(uint8_t zone);

    // System Control
    // Generally if receivingData() is returning false sending commands are dropped as most commands
    // require up to date information to send the correct data

    /// @brief turn the ac system on/off, when turning on, should restore last operating mode
    /// @param on true to turn on, false to turn off
    void setSystemOn(bool on);

    /// @brief is system turned on/off
    /// @returns true if on, false otherwise
    bool getSystemOn();

    /// @brief set the system fan speed
    /// @param speed to set from Low, Medium, High, ESP (Auto), other options are ignored
    void setFanSpeed(FanMode fanSpeed);

    /// @brief set the system fan speed, adjusting continuous mode
    /// @param speed to set from Low, Medium, High, ESP (Auto), Low Cont., Medium Cont., High Cont., ESP (Auto) Cont., 
    void setFanSpeedAbsolute(FanMode fanSpeed);

    /// @brief get the set system fan speed
    /// @returns returns Off, Low, Medium, High, ESP (Auto)
    FanMode getFanSpeed();

    /// @brief get the running system fan speed
    /// @returns returns Off, Low, Medium, High
    FanMode getRunningFanSpeed();

    /// @brief set continuous fan mode
    /// @param on sets it to on, false to normal mode
    void setContinuousFanMode(bool on);

    /// @brief get if continuous fan mode is on or off
    /// @returns true if on, false otherwise
    bool getContinuousFanMode();

    /// @brief adjusts the operating mode of the system, can also be used to turn off the system
    /// turning off will try to preserve the state
    /// @param mode Off, Fan Only, Auto, Cool, Heat. Use OffAuto, OffCool, OffHeat to change modes without turning on the system
    void setOperatingMode(OperatingMode mode);
    
    /// @brief get the operating mode of the system, as expected when the on button is pressed if not on
    /// @return the operating mode: Fan Only, Auto, Cool, Heat
    OperatingMode getOperatingMode();

    /// @brief set the master setpoint temperature
    /// @param temperature to set in °C, in 0.5° increments
    void setMasterSetpoint(double temperature);

    /// @brief get the master setpoint temperature
    /// @returns temperature in °C
    double getMasterSetpoint();

    /// @brief get the master current temperature reading.
    /// This temperature may be the average of multiple sensors depending on configuration
    /// and operating zones
    /// @returns temperature in °C
    double getMasterCurrentTemperature();

    /// @brief if the compressor is idle, or active
    /// @return state of compressor
    CompressorMode getCompressorMode();

    /// @brief if the fan is idle, this can be if AC is on but not active, or off
    /// @return true if fan idle, false otherwise
    bool isFanIdle();

    /// Zone Control

    /// @brief adjust the specified zone to be
    /// @param zone to adjust
    /// @param on true, false for off
    void setZoneOn(uint8_t zone, bool on);

    /// @brief get zone on/off state
    /// @param zone to query
    /// @returns true if on, false otherwise
    bool getZoneOn(uint8_t zone);

    /// @brief adjust the zones setpoint to the specified temperature for Ultima systems
    /// *** This only works for zones controlled by this controller module ***
    /// If the zone is controlled by this device, it will be adjusted directly
    /// If the zone is controller by another device, but using this module, it will send a request
    /// @param zone to adjust
    /// @param temperature to set in °C, in 0.5° increments
    /// @param adjustMaster to adjust master to the allowed range
    void setZoneSetpointTemperatureCustom(uint8_t zone, double temperature, bool adjustMaster);

    /// @brief adjust the zones setpoint to the specified temperature for Ultima systems
    /// a hack by sending an out of sync message coercing the zone to change temperature
    /// If the zone is controlled by this device, it will be adjusted directly
    /// If the zone is controller by another device, but using this module, it will send a request
    /// @param zone to adjust
    /// @param temperature to set in °C, in 0.5° increments
    /// @param adjustMaster to adjust master to the allowed range
    void setZoneSetpointTemperature(uint8_t zone, double temperature, bool adjustMaster);

    /// @brief get zone set point temperature for Ultima systems
    /// @param zone to query
    /// @returns temperature in °C
    double getZoneSetpointTemperature(uint8_t zone);

    /// @brief set the current temperature for zones controlled by this module
    /// @param zone to adjust
    /// @param temperature to set in °C, in 0.1° increments
    void setZoneCurrentTemperature(uint8_t zone, double temperature);

    /// @brief get zone current temperature for Ultima systems
    /// @param zone to query
    /// @returns temperature in °C
    double getZoneCurrentTemperature(uint8_t zone);

    /// @brief get zone damper position
    /// @param zone to query
    /// @returns will vary between 0.0->1.0 (0-100%) for Ultima Systems. 0 or 1 for others.
    double getZoneDamperPosition(uint8_t zone);

};

}