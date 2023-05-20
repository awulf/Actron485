#include "Actron485.h"
#include "Utilities.h"

namespace Actron485 {
 
    void Controller::serialWrite(bool enable) {
        if (enable) {
            if (_rxPin == _txPin) {
                pinMatrixOutDetach(_rxPin, false, false);
                pinMode(_txPin, OUTPUT);
                pinMatrixOutAttach(_txPin, U1TXD_OUT_IDX, false, false);
            }

            if (_writeEnablePin > 0) {
                digitalWrite(_writeEnablePin, HIGH); 
            }

        } else {
            _serial->flush();

            if (_writeEnablePin > 0) {
                digitalWrite(_writeEnablePin, LOW); 
            }

            if (_rxPin == _txPin) {
                pinMatrixOutDetach(_txPin, false, false);
                pinMode(_rxPin, INPUT);
                pinMatrixOutAttach(_rxPin, U1RXD_IN_IDX, false, false);
            }
        }
    }

    void Controller::sendZoneMessage(int zone) {
        printOut.println("Send Zone Message");
        if (zone <= 0 || zone > 8) {
            // Out of bounds
            return;
        }

        // If we have mode change (off/on/open) request pending, send it now
        if (_requestZoneMode[zindex(zone)] != ZoneMode::Ignore) {
            zoneMessage[zindex(zone)].mode = _requestZoneMode[zindex(zone)];
        }

        zoneMessage[zindex(zone)].type = ZoneMessageType::Normal;
        zoneMessage[zindex(zone)].temperature = zoneTemperature[zindex(zone)];

        // Enforce, and set based on setpoint range limit, if we aren't currently adjusting the master setpoint
        if (!sendSetpointCommand) {
            zoneSetpoint[zindex(zone)] = max(min(zoneSetpoint[zindex(zone)], masterToZoneMessage[zindex(zone)].maxSetpoint), masterToZoneMessage[zindex(zone)].minSetpoint);
        }
        zoneMessage[zindex(zone)].setpoint = zoneSetpoint[zindex(zone)];
        
        uint8_t data[zoneMessage[zindex(zone)].messageLength];
        zoneMessage[zindex(zone)].generate(data);

        serialWrite(true); 
        
        for (int i=0; i<5; i++) {
            Serial1.write(data[i]);
        }

        serialWrite(false);

        zoneMessage[zindex(zone)].print();
        printOut.println();
        printOut.println();
    }

    void Controller::sendZoneConfigMessage(int zone) {
        if (zone <= 0 || zone > 8) {
            // Out of bounds
            return;
        }
        Serial.println("Send Zone Config");
        ZoneToMasterMessage configMessage;
        configMessage.type = ZoneMessageType::Config;
        configMessage.zone = zoneMessage[zindex(zone)].zone;
        configMessage.mode = zoneMessage[zindex(zone)].mode;
        configMessage.setpoint = zoneMessage[zindex(zone)].setpoint;
        configMessage.temperature = 0;
        uint8_t data[configMessage.messageLength];
        configMessage.generate(data);
        serialWrite(true); 
        
        for (int i=0; i<configMessage.messageLength; i++) {
            _serial->write(data[i]);
        }

        serialWrite(false);
    }

    void Controller::sendZoneInitMessage(int zone) {
        printOut.println("Send Zone Init");
        serialWrite(true);  
        _serial->write((uint8_t) 0x00);
        _serial->write((uint8_t) 0xCC);
        serialWrite(false);
    }

    void Controller::processMasterMessage(MasterToZoneMessage masterMessage) {
        uint8_t zone = masterMessage.zone;
        if (zone <= 0 || zone > 8) {
            // Out of bounds
            return;
        }
        if (zoneControlled[zindex(zone)] == false) {
            // We don't care about this, not us
            return;
        }

        // Confirm our request to turn on/off the zone has been processed
        switch (_requestZoneMode[zindex(zone)]) {
            case ZoneMode::Ignore:
                break;
            case ZoneMode::Off:
                if (masterMessage.on == false) {
                    _requestZoneMode[zindex(zone)] = ZoneMode::Ignore;
                }
                break;
            case ZoneMode::On:
            case ZoneMode::Open: // Master message doesn't show this case, so at this stage we can't tell we switch to open to on and vice versa, so assume it was successful anyway if on
                if (masterMessage.on == true) {
                    _requestZoneMode[zindex(zone)] = ZoneMode::Ignore;
            }
        }

        // If zone is set to 0, we need to copy some values from master
        bool copyZoneSate = (zoneMessage[zindex(zone)].zone == 0);

        // Update Zone on/off state based on this message (open is same as on, but the master message doesn't know the difference)
        if (masterMessage.on == true && zoneMessage[zindex(zone)].mode == ZoneMode::Off || copyZoneSate) {
            zoneMessage[zindex(zone)].mode = ZoneMode::On;
        } else if (masterMessage.on == false && zoneMessage[zindex(zone)].mode != ZoneMode::Off || copyZoneSate) {
            zoneMessage[zindex(zone)].mode = ZoneMode::Off;
        }
        
        if (copyZoneSate) {
            zoneMessage[zindex(zone)].zone = zone;
            zoneMessage[zindex(zone)].temperature = masterMessage.temperature;
            zoneMessage[zindex(zone)].setpoint = masterMessage.setpoint;
        }

        ////////////////////////
        // Send our awaited status report/request/config

        // Do we want to send a config message this round?
        if (_sendZoneConfig[zindex(zone)]) {
            sendZoneConfigMessage(zindex(zone));
            _sendZoneConfig[zindex(zone)] = false;
        } else {
            sendZoneMessage(zone);
        }
    }
    
    void Controller::processZoneMessage(ZoneToMasterMessage zoneMessage) {
        if (zoneControlled[zindex(zoneMessage.zone)] == false) {
            // We don't care about this, not us
            return;
        }

        // If we got a zone message from not us, its probably the master contoller
        // asking us to respond
        if (zoneMessage.type == ZoneMessageType::InitZone) {
            sendZoneInitMessage(zoneMessage.zone);
            _sendZoneConfig[zindex(zoneMessage.zone)] = true;
        }
    }

    Controller::Controller(uint8_t rxPin, uint8_t txPin, uint8_t writeEnablePin) {
        _rxPin = rxPin;
        _txPin = txPin;
        _writeEnablePin = writeEnablePin;

         Serial1.begin(4800, SERIAL_8N1, rxPin, txPin);
        _serial = &Serial1;

        if (writeEnablePin > 0) {
            pinMode(writeEnablePin, OUTPUT);
            serialWrite(false);
        }
        
        setup();
    }

    void Controller::setup() {
        printOutMode = PrintOutMode::ChangedMessages;

        // Set to ignore
        for (int i=0; i<8; i++) {
            _requestZoneMode[i] = ZoneMode::Ignore;
        }
    }

    bool Controller::sendQueuedCommand() {
        uint8_t data[5];
        int send = 0;

        // We can only send one command at a time, per sequence
        // start with the most important ones and work our way down
        if (sendOperatingModeCommand) {
            printOut.print("Send: ");
            sendOperatingModeCommand = false;
            nextOperatingModeCommand.generate(data);
            nextOperatingModeCommand.print();
            send = nextOperatingModeCommand.messageLength;
            
        } else if (sendZoneStateCommand) {
            printOut.print("Send: ");
            sendZoneStateCommand = false;
            nextZoneStateCommand.generate(data);
            nextZoneStateCommand.print();
            send = nextZoneStateCommand.messageLength;
            
        } else if (sendFanModeCommand) {
            printOut.print("Send: ");
            sendFanModeCommand = false;
            nextFanModeCommand.generate(data);
            nextFanModeCommand.print();
            send = nextFanModeCommand.messageLength;
            
        } else if (sendSetpointCommand) {
            printOut.print("Send: ");
            sendSetpointCommand = false;
            nextSetpointCommand.generate(data);
            nextSetpointCommand.print();
            send = nextSetpointCommand.messageLength;
            
        } else if (sendZoneSetpointCustomCommand) {
            printOut.print("Send: ");
            sendZoneSetpointCustomCommand = false;
            nextZoneSetpointCustomCommand.generate(data);
            nextZoneSetpointCustomCommand.print();
            send = nextZoneSetpointCustomCommand.messageLength;
        }

        if (send > 0) {
            serialWrite(true); 
            
            for (int i=0; i<send; i++) {
                _serial->write(data[i]);
            }

            serialWrite(false);
        }
        
        return send > 0;
    }

    Controller::Controller(Stream &stream, uint8_t writeEnablePin) {
        _serial = &stream;
        _writeEnablePin = writeEnablePin;
        setup();
    }

    ///////////////////////////////////
    // Message type

    MessageType Controller::detectActronMessageType(uint8_t firstByte) {
        if (firstByte == (uint8_t) MessageType::CommandMasterSetpoint) {
            return MessageType::CommandMasterSetpoint;
        } else if (firstByte == (uint8_t) MessageType::CommandFanMode) {
            return MessageType::CommandFanMode;
        } else if (firstByte == (uint8_t) MessageType::CommandOperatingMode) {
            return MessageType::CommandOperatingMode;
        } else if (firstByte == (uint8_t) MessageType::CommandZoneState) {
            return MessageType::CommandZoneState;
        } else if (firstByte == (uint8_t) MessageType::BoardComms1) {
            return MessageType::BoardComms1;
        } else if (firstByte == (uint8_t) MessageType::BoardComms2) {
            return MessageType::BoardComms2;
        } else if (firstByte == (uint8_t) MessageType::Stat1) {
            return MessageType::Stat1;
        } else if (firstByte == (uint8_t) MessageType::Stat2) {
            return MessageType::Stat2;
        } else if (firstByte == (uint8_t) MessageType::Stat3) {
            return MessageType::Stat3;
        } else if ((firstByte & (uint8_t) MessageType::ZoneWallController) == (uint8_t) MessageType::ZoneWallController) {
            return MessageType::ZoneWallController;
        } else if ((firstByte & (uint8_t) MessageType::ZoneMasterController) == (uint8_t) MessageType::ZoneMasterController) {
            return MessageType::ZoneMasterController;
        }

        return MessageType::Unknown;
    }

    void Controller::loop() {
        unsigned long now = millis();
        long serialLastReceivedTime = now-_serialBufferReceivedTime;

        if (_serialBufferIndex > 0 && serialLastReceivedTime > _serialBufferBreak) {
            MessageType messageType = detectActronMessageType(_serialBuffer[0]);

            bool printChangesOnly = printOutMode == PrintOutMode::ChangedMessages;
            bool printAll = (printOutMode == PrintOutMode::AllMessages);
            bool changed = false;

            bool popSendQueue = false;

            uint8_t zone = 0;

            switch (messageType) {
                case MessageType::Unknown:
                    printOut.println("Unknown Message received");
                    changed = true;
                    break;
                case MessageType::CommandMasterSetpoint:
                    // We don't care about this command
                    break;
                case MessageType::CommandFanMode:
                    // We don't care about this command
                    break;
                case MessageType::CommandOperatingMode:
                    // We don't care about this command
                    break;
                case MessageType::CommandZoneState:
                    // We don't care about this command
                    break;
                case MessageType::CustomCommandChangeZoneSetpoint:
                    {
                        ZoneSetpointCustomCommand command;
                        command.parse(_serialBuffer);
                        if (zoneControlled[zindex(command.zone)]) {
                            zoneSetpoint[zindex(command.zone)] = command.temperature;
                        }
                    }
                    break;
                case MessageType::ZoneWallController:
                    zone = _serialBuffer[0] & 0x0F;
                    if (0 < zone && zone <= 8) {
                        zoneMessage[zindex(zone)].parse(_serialBuffer);
                        changed = copyBytes(_serialBuffer, zoneWallMessageRaw[zindex(zone)], zoneMessage[zindex(zone)].messageLength);
                        if (printAll || printChangesOnly && changed) {
                            zoneMessage[zindex(zone)].print();
                            printOut.println();
                        }
                    }
                    break;
                case MessageType::ZoneMasterController:
                    zone = _serialBuffer[0] & 0x0F;
                    if (0 < zone && zone <= 8) {
                        masterToZoneMessage[zindex(zone)].parse(_serialBuffer);
                        changed = copyBytes(_serialBuffer, zoneMasterMessageRaw[zindex(zone)], masterToZoneMessage[zindex(zone)].messageLength);

                        if (printAll || printChangesOnly && changed) {
                            masterToZoneMessage[zindex(zone)].print();
                            printOut.println();
                        }
                    }
                    break;
                case MessageType::BoardComms1:
                    changed = copyBytes(_serialBuffer, boardComms1Message[boardComms1Index], _serialBufferIndex);
                    boardComms1MessageLength[boardComms1Index] = _serialBufferIndex;
                    boardComms1Index = (boardComms1Index + 1)%2;
                    break;
                case MessageType::BoardComms2:
                    changed = copyBytes(_serialBuffer, boardComms2Message, boardComms2MessageLength);
                    break;
                case MessageType::Stat1:
                    changed = copyBytes(_serialBuffer, stateMessageRaw, stateMessage.stateMessageLength);
                    stateMessage.parse(_serialBuffer);

                    if (printAll || printChangesOnly && changed) {
                        stateMessage.print();
                        printOut.println();
                    }
                    break;
                case MessageType::Stat2:
                    changed = copyBytes(_serialBuffer, stat2Message, stat2MessageLength);
                    break;
                case MessageType::Stat3:
                    changed = copyBytes(_serialBuffer, stat3Message, stat3MessageLength);

                    // Reset board comms1 counter
                    boardComms1Index = 0;

                    // Last message in the sequence, our chance to send a message        
                    popSendQueue = true;
                    break;
            }

            if (printAll || printChangesOnly && changed) {
                printBytes(_serialBuffer, _serialBufferIndex);
                printOut.println();
                printOut.println();
            }

            // We need to process after printing, else the logs appear out of order
            if (0 < zone && zone <= 8) {
                switch (messageType) {
                    case MessageType::ZoneWallController:
                        processZoneMessage(zoneMessage[zindex(zone)]);
                        break;
                    case MessageType::ZoneMasterController:
                        processMasterMessage(masterToZoneMessage[zindex(zone)]);
                        break;
                }
            }

            if (popSendQueue) {
                printOut.println("==========");
                printOut.println();
                sendQueuedCommand();
            }

            _serialBufferIndex = 0;
        }

        while(_serial->available() > 0 && _serialBufferIndex < _serialBufferSize) {
            uint8_t byte = _serial->read();
            _serialBuffer[_serialBufferIndex] = byte;
            _serialBufferIndex++;
            _serialBufferReceivedTime = millis();
        }
    }

     //////////////////////
    /// Convenient functions, that are the typical use for this module

    // Setup

    void Controller::setControlZone(uint8_t zone, bool control) {
        zoneControlled[zindex(zone)] = control;
    }

    bool Controller::getControlZone(uint8_t zone) {
        return zoneControlled[zindex(zone)];
    }

    // System Control

    void Controller::setSystemOn(bool on) {
        if (stateMessage.operatingMode == OperatingMode::Off && on) {
            // If all off, pressing on turns on fan mode
            nextOperatingModeCommand.mode = OperatingMode::FanOnly;
        } else if (stateMessage.operatingMode == OperatingMode::FanOnly && on == false) {
            // If fan mode, turning off, sets to fully off
            nextOperatingModeCommand.mode = OperatingMode::Off;
        } else {
            // Set the on/off bit
            nextOperatingModeCommand.mode = OperatingMode(((uint8_t) stateMessage.operatingMode) & (on ? 0b11111111 : 0b11110111));
        }
        sendOperatingModeCommand = true;
    }

    bool Controller::getSystemOn() {
        return stateMessage.operatingMode != OperatingMode::Off;
    }

    void Controller::setFanSpeed(FanMode fanSpeed) {
        bool continuous = getContinuousFanMode();
        switch (fanSpeed) {
            case FanMode::Low:
                nextFanModeCommand.fanMode = continuous ? FanMode::LowContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
            case FanMode::Medium:
                nextFanModeCommand.fanMode = continuous ? FanMode::MediumContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
            case FanMode::High:
                nextFanModeCommand.fanMode = continuous ? FanMode::HighContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
            case FanMode::Esp:
                nextFanModeCommand.fanMode = continuous ? FanMode::EspContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
        }
    }

    FanMode Controller::getFanSpeed() {
        return stateMessage.fanMode;
    }

    void Controller::setContinuousFanMode(bool on) {
        FanMode fanSpeed = getFanSpeed();
        switch (fanSpeed) {
            case FanMode::Low:
                nextFanModeCommand.fanMode = on ? FanMode::LowContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
            case FanMode::Medium:
                nextFanModeCommand.fanMode = on ? FanMode::MediumContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
            case FanMode::High:
                nextFanModeCommand.fanMode = on ? FanMode::HighContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
            case FanMode::Esp:
                nextFanModeCommand.fanMode = on ? FanMode::EspContinuous : fanSpeed;
                sendFanModeCommand = true;
                break;
        }
    }

    bool Controller::getContinuousFanMode() {
        return stateMessage.continuousFan;
    }

    void Controller::setOperatingMode(OperatingMode mode) {
        OperatingMode currentMode = stateMessage.operatingMode;
        if (mode == OperatingMode::Off) {
            setSystemOn(false);
        } else {
            nextOperatingModeCommand.mode = mode;
            sendOperatingModeCommand = true;
        }
    }

    OperatingMode Controller::getOperatingMode() {
        return stateMessage.lastOperatingMode;
    }

    void Controller::setMasterSetpoint(double temperature) {
        nextSetpointCommand.temperature = temperature;
        sendSetpointCommand = true;
    }
    
    double Controller::getMasterSetpoint() {
        return stateMessage.setpoint;
    }

    bool Controller::isSystemIdle() {
        return stateMessage.systemActive == false;
    }

    bool Controller::isFanIdle() {
        return stateMessage.fanActive == false;
    }

    /// Zone Control

    void Controller::setZoneOn(uint8_t zone, bool on) {
        for (int i=0; i<8; i++) {
            nextZoneStateCommand.zoneOn[i] = (i == zindex(zone) ? on : stateMessage.zoneOn[i]);
        }
        sendZoneStateCommand = true;
    }

    bool Controller::getZoneOnState(uint8_t zone) {
        return stateMessage.zoneOn[zindex(zone)];
    }

    void Controller::setZoneSetpointTemperature(uint8_t zone, double temperature, bool adjustMaster) {
        if (zoneControlled[zindex(zone)] == true) {
            // Check if we need to adjust the master first
            if (adjustMaster) {
                double minAllowed = masterToZoneMessage[zindex(zone)].minSetpoint;
                double maxAllowed = masterToZoneMessage[zindex(zone)].maxSetpoint;
                double diff = 0;
                if (temperature<minAllowed) {
                    diff = minAllowed-temperature;
                } else if (temperature>maxAllowed) {
                    diff = temperature-maxAllowed;
                }
                // If the difference is not 0 adjust
                if (diff != 0) {
                    double newTempeature = getMasterSetpoint() + diff;
                    setMasterSetpoint(newTempeature);
                }
            }
            zoneSetpoint[zindex(zone)] = temperature;

        } else {
            // Send the custom zone setpoint message, the official 
            nextZoneSetpointCustomCommand.temperature = temperature;
            nextZoneSetpointCustomCommand.adjustMaster = adjustMaster;
            nextZoneSetpointCustomCommand.zone = zone;
            sendZoneSetpointCustomCommand = true;
        }
    }

    double Controller::getZoneSetpointTempeature(uint8_t zone) {
        return stateMessage.zoneSetpoint[zindex(zone)];
    }

    void Controller::setZoneCurrentTemperature(uint8_t zone, double temperature) {
        zoneTemperature[zindex(zone)] = temperature;
    }

    double Controller::getZoneCurrentTemperature(uint8_t zone) {
        return zoneMessage[zindex(zone)].temperature;
    }

}