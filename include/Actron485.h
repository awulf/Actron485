#pragma once
#include <Arduino.h>
#include "Actron485Models.h"

namespace Actron485 {

class Controller {

    Stream *serial;
    uint8_t writeEnablePin;

    /// @brief Zone 1 - 8 (indexed 0-7), Zone own control requests. -1 (actronZoneModeIgnore) when not requesting (e.g. once request has been sent)
    ZoneMode requestZoneMode[8];

    /// @brief Buffer size for ingesting serial messages
    static const size_t serialBufferSize = 64;
    /// @brief Serial Buffer for ingesting
    uint8_t serialBuffer[serialBufferSize];
    /// @brief Last message received time
    unsigned long serialBufferReceivedTime = 0;
    /// @brief Minimum time between bytes received to split up serial message in milliseconds
    static const unsigned long serialBufferBreak = 5;
    /// @brief Index of current sequence being read
    uint8_t serialBufferIndex = 0;

    /// @brief Bring up/down the serial write enable pin
    /// @param enable 
    void serialWrite(bool enable);

    /// @brief Attemps to send a zone mesage immediatly for the given zone number
    /// @param zone 
    void sendZoneMessage(int zone);

    /// @brief Attempts to send a zone config message immdiatly for the given zone number
    /// @param zone 
    void sendZoneConfigMessage(int zone);

    /// @brief Processes the message received, stores and redirects accordingly
    /// @param data 
    void processMessage(uint8_t *data);

    /// @brief Process the master to zone message received, adjusts stored zone paramters accordingly
    /// @param masterMessage to process
    void processMasterMessage(MasterToZoneMessage masterMessage);

    /// @brief Process the zone to master message, if initialisation message, will handle it if zone is configured
    /// @param zoneMessage 
    void processZoneMessage(ZoneToMasterMessage zoneMessage);

public:

    /// @brief initialise controller with serial pins
    /// @param rxPin pin to receive on
    /// @param txPin pin to write to
    /// @param writeEnablePin for write enable, set to 0 if not used
    Controller(uint8_t rxPin, uint8_t txPin, uint8_t writeEnablePin);

    /// @brief initialise controller with a custom stream
    /// @param stream 
    Controller(Stream &stream);

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

    /// @brief Message type determined by the first byte
    /// @param fistByte to from the message
    /// @return message type
    MessageType messageType(uint8_t fistByte);

    /// @brief Call with the run loop
    void loop();
};

}