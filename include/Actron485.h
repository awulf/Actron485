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
    static const size_t _serialBufferSize = 64;
    /// @brief Serial Buffer for ingesting
    uint8_t _serialBuffer[_serialBufferSize];
    /// @brief Last message received time
    unsigned long _serialBufferReceivedTime = 0;
    /// @brief Minimum time between bytes received to split up serial message in milliseconds
    static const unsigned long _serialBufferBreak = 5;
    /// @brief Index of current sequence being read
    uint8_t _serialBufferIndex = 0;

    /// @brief Bring up/down the serial write enable pin
    /// @param enable 
    void serialWrite(bool enable);

    /// @brief Attemps to send a zone mesage immediatly for the given zone number
    /// @param zone 
    void sendZoneMessage(int zone);

    /// @brief Attempts to send a zone config message immdiatly for the given zone number
    /// @param zone 
    void sendZoneConfigMessage(int zone);

    /// @brief Attempts to send a zone init message immdiatly for the given zone number, should be sent
    /// straight after the master controller requests the zone
    /// @param zone 
    void sendZoneInitMessage(int zone);

    /// @brief Process the master to zone message received, adjusts stored zone paramters accordingly
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

public:

    /// @brief initialise controller with serial pins. Supports rx & tx being the same pin if constrained with GPIOs.
    /// @param rxPin pin to receive on
    /// @param txPin pin to write to
    /// @param writeEnablePin for write enable, set to 0 if not used
    Controller(uint8_t rxPin, uint8_t txPin, uint8_t writeEnablePin);

    /// @brief initialise controller with a custom stream
    /// @param writeEnablePin for write enable, set to 0 if not used
    /// @param stream 
    Controller(Stream &stream, uint8_t writeEnablePin);

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

    /// @brief State of the AC control message
    StateMessage stateMessage;

    /// @brief Message type determined by the first byte
    /// @param firstByte to from the message
    /// @return message type
    MessageType detectActronMessageType(uint8_t firstByte);

    /// @brief Logging/printing mode
    PrintOutMode printOutMode;

    /// @brief Must be called with the main run loop
    void loop();

    //////////////////////
    // Queued Commands awaiting to be sent, when set will send one by one to the controller on each loop,
    // priority in the order listed in the declarations below

    /// @brief setpoint command, sent on next cycle
    OperatingModeCommand queuedOperatingModeCommand;
    /// @brief zone state command, sent on next cycle, for self controlled zones best to use ZoneToMasterMessage
    ZoneStateCommand queuedZoneStateCommand;
    /// @brief setpoint command, sent on next cycle
    MasterSetpointCommand queueSetpointCommand;
    /// @brief fan mode command, sent on next cycle
    FanModeCommand queueFanModeCommand;

    //////////////////////
    /// Below are last stored messages. Some of those assumed types, better understanding still required

    uint8_t zoneWallMessageRaw[8][5];
    uint8_t zoneMasterMessageRaw[8][7];

    // This message varies in length, and occurs up to two times per sequence
    uint8_t boardComms1Index; // records count per sequence
    uint8_t boardComms1MessageLength[2];
    uint8_t boardComms1Message[2][50];

    const static uint8_t boardComms2MessageLength = 18;
    uint8_t boardComms2Message[boardComms2MessageLength];
    uint8_t stateMessageRaw[StateMessage::stateMessageLength];
    const static uint8_t stat2MessageLength = 19;
    uint8_t stat2Message[19];
    const static uint8_t stat3MessageLength = 32;
    uint8_t stat3Message[32];
};

}