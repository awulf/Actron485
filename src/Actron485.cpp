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
        if (printOut) {
            printOut->println("Send Zone Message");
        }
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

        // Enforce, and set based on set point range limit, if we aren't currently adjusting the master set point
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
        if (printOut) {
            printOut->println();
            printOut->println();
        }
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
        if (printOut) {
            printOut->println("Send Zone Init");
        }
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

        // If we got a zone message from not us, its probably the master controller
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

    Controller::Controller(Stream &stream, uint8_t writeEnablePin) {
        _serial = &stream;
        _writeEnablePin = writeEnablePin;
        setup();
    }

    Controller::Controller() {
        setup();
    }

    void Controller::configure(Stream &stream, uint8_t writeEnablePin) {
        _serial = &stream;
        _writeEnablePin = writeEnablePin;
    }

    void Controller::configureLogging(Stream *stream) {
        printOut = stream;
    }

    void Controller::setup() {
        printOutMode = PrintOutMode::ChangedMessages;

        dataLastReceivedTime = 99999;

        // Set to ignore
        for (int i=0; i<8; i++) {
            _requestZoneMode[i] = ZoneMode::Ignore;
        }
    }

    bool Controller::sendQueuedCommand() {
        uint8_t data[7];
        int send = 0;

        // We can only send one command at a time, per sequence
        // start with the most important ones and work our way down
        if (sendOperatingModeCommand) {
            if (printOut) {
                printOut->print("Send: ");
            }
            sendOperatingModeCommand = false;
            nextOperatingModeCommand.generate(data);
            nextOperatingModeCommand.print();
            send = nextOperatingModeCommand.messageLength;
            
        } else if (sendZoneStateCommand) {
            if (printOut) {
                printOut->print("Send: ");
            }
            sendZoneStateCommand = false;
            nextZoneStateCommand.generate(data);
            nextZoneStateCommand.print();
            send = nextZoneStateCommand.messageLength;
            
        } else if (sendFanModeCommand) {
            if (printOut) {
                printOut->print("Send: ");
            }
            sendFanModeCommand = false;
            nextFanModeCommand.generate(data);
            nextFanModeCommand.print();
            send = nextFanModeCommand.messageLength;
            
        } else if (sendSetpointCommand) {
            if (printOut) {
                printOut->print("Send: ");
            }
            sendSetpointCommand = false;
            nextSetpointCommand.generate(data);
            nextSetpointCommand.print();
            send = nextSetpointCommand.messageLength;
            
        } else if (sendZoneSetpointCustomCommand) {
            if (printOut) {
                printOut->print("Send: ");
            }
            sendZoneSetpointCustomCommand = false;
            nextZoneSetpointCustomCommand.generate(data);
            nextZoneSetpointCustomCommand.print();
            send = nextZoneSetpointCustomCommand.messageLength;

        } else {
            for (int i=0; i<8; i++) {
                if (!sendMasterToZoneMessage[i]) {
                    continue;
                }
                
                if (printOut) {
                    printOut->print("Send: ");
                }
                sendMasterToZoneMessage[i] = false;
                nextMasterToZoneMessage[i].generate(data);
                nextMasterToZoneMessage[i].print();
                send = nextMasterToZoneMessage[i].messageLength;
                break;
            }
        }

        if (send > 0) {
            dataLastSentTime = millis();
            serialWrite(true); 
            
            for (int i=0; i<send; i++) {
                _serial->write(data[i]);
            }

            serialWrite(false);
        }
        
        return send > 0;
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
        } else if (firstByte == (uint8_t) MessageType::IndoorBoard1) {
            return MessageType::IndoorBoard1;
        } else if (firstByte == (uint8_t) MessageType::IndoorBoard2) {
            return MessageType::IndoorBoard2;
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
            bool printChangesOnly = printOutMode == PrintOutMode::ChangedMessages;
            bool printAll = (printOutMode == PrintOutMode::AllMessages);
            bool changed = false;

            uint8_t zone = 0;

            MessageType messageType = MessageType::Unknown;
            
            if ((now - dataLastSentTime) < 50) {
                // This will be a response to our command
                if (printOut) {
                    printOut->println("Response Message Received");
                }

            } else {
                messageType = detectActronMessageType(_serialBuffer[0]);
                
                switch (messageType) {
                    case MessageType::Unknown:
                        if (printOut) {
                            printOut->println("Unknown Message received");
                        }
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
                                setZoneSetpointTemperature(command.zone, command.temperature, command.adjustMaster);
                            }
                        }
                        break;
                    case MessageType::ZoneWallController:
                        zone = _serialBuffer[0] & 0x0F;
                        if (0 < zone && zone <= 8) {
                            zoneMessage[zindex(zone)].parse(_serialBuffer);
                            changed = copyBytes(_serialBuffer, zoneWallMessageRaw[zindex(zone)], zoneMessage[zindex(zone)].messageLength);
                            if (printOut && (printAll || (printChangesOnly && changed))) {
                                zoneMessage[zindex(zone)].print();
                                printOut->println();
                            }
                        }
                        break;
                    case MessageType::ZoneMasterController:
                        zone = _serialBuffer[0] & 0x0F;
                        if (0 < zone && zone <= 8) {
                            masterToZoneMessage[zindex(zone)].parse(_serialBuffer);
                            changed = copyBytes(_serialBuffer, zoneMasterMessageRaw[zindex(zone)], masterToZoneMessage[zindex(zone)].messageLength);

                            if (printOut && (printAll || (printChangesOnly && changed))) {
                                masterToZoneMessage[zindex(zone)].print();
                                printOut->println();
                            }
                        }
                        break;
                    case MessageType::IndoorBoard1:
                        changed = copyBytes(_serialBuffer, boardComms1Message[boardComms1Index], _serialBufferIndex);
                        boardComms1MessageLength[boardComms1Index] = _serialBufferIndex;
                        boardComms1Index = (boardComms1Index + 1)%2;
                        break;
                    case MessageType::IndoorBoard2:
                        changed = copyBytes(_serialBuffer, stateMessage2Raw, stateMessage2.stateMessageLength);
                        stateMessage2.parse(_serialBuffer);
                        statusLastReceivedTime = now;

                        if (printOut && (printAll || (printChangesOnly && changed))) {
                            stateMessage2.print();
                            printOut->println();
                        }
                        break;
                    case MessageType::Stat1:
                        changed = copyBytes(_serialBuffer, stateMessageRaw, stateMessage.stateMessageLength);
                        stateMessage.parse(_serialBuffer);
                        statusLastReceivedTime = now;

                        if (printOut && (printAll || (printChangesOnly && changed))) {
                            stateMessage.print();
                            printOut->println();
                        }
                        break;
                    case MessageType::Stat2:
                        changed = copyBytes(_serialBuffer, stat2Message, stat2MessageLength);
                        break;
                    case MessageType::Stat3:
                        changed = copyBytes(_serialBuffer, stat3Message, stat3MessageLength);
                        break;
                }
            }

            if (printOut && (printAll || (printChangesOnly && changed))) {
                printBytes(_serialBuffer, _serialBufferIndex);
                printOut->println();
                printOut->println();
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

            dataLastReceivedTime = now;

            _serialBufferIndex = 0;
        }

        // A gap send our message
        if ((now - dataLastReceivedTime) > 500 && (now - dataLastReceivedTime) < 1000 && (now - _lastQuietPeriodDetectedTime) > 900) {
            _lastQuietPeriodDetectedTime = now;
            // Reset board comms1 counter
            boardComms1Index = 0;
            if (printOut && printOutMode == PrintOutMode::AllMessages) {
                printOut->println("Time to Send");
            }
            sendQueuedCommand();
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

    bool Controller::receivingData() {
        return (millis() - dataLastReceivedTime) < 3000;
    }

    // Setup

    void Controller::setControlZone(uint8_t zone, bool control) {
        zoneControlled[zindex(zone)] = control;
    }

    bool Controller::getControlZone(uint8_t zone) {
        return zoneControlled[zindex(zone)];
    }

    // System Control

    void Controller::setSystemOn(bool on) {
        if (!receivingData()) {
            return;
        }

        OperatingMode currentMode = getOperatingMode();
        
        if (on) {
            switch (currentMode) {
                case OperatingMode::Off:
                    nextOperatingModeCommand.mode = OperatingMode::FanOnly;
                    sendOperatingModeCommand = true;
                    break;
                case OperatingMode::OffAuto:
                    nextOperatingModeCommand.mode = OperatingMode::Auto;
                    sendOperatingModeCommand = true;
                    break;
                case OperatingMode::OffHeat:
                    nextOperatingModeCommand.mode = OperatingMode::Heat;
                    sendOperatingModeCommand = true;
                    break;
                case OperatingMode::OffCool:
                    nextOperatingModeCommand.mode = OperatingMode::Cool;
                    sendOperatingModeCommand = true;
                    break;
            }
        } else {
            switch (currentMode) {
                case OperatingMode::FanOnly:
                    nextOperatingModeCommand.mode = OperatingMode::Off;
                    sendOperatingModeCommand = true;
                    break;
                case OperatingMode::Auto:
                    nextOperatingModeCommand.mode = OperatingMode::OffAuto;
                    sendOperatingModeCommand = true;
                    break;
                case OperatingMode::Heat:
                    nextOperatingModeCommand.mode = OperatingMode::OffHeat;
                    sendOperatingModeCommand = true;
                    break;
                case OperatingMode::Cool:
                    nextOperatingModeCommand.mode = OperatingMode::OffCool;
                    sendOperatingModeCommand = true;
                    break;
            }
        }
    }

    bool Controller::getSystemOn() {
        OperatingMode currentMode = getOperatingMode();

        switch (currentMode) {
            case OperatingMode::FanOnly:
                return true;
            case OperatingMode::Auto:
                return true;
            case OperatingMode::Heat:
                return true;
            case OperatingMode::Cool:
                return true;
            default:
                return false;
        }
    }

    void Controller::setFanSpeed(FanMode fanSpeed) {
        if (!receivingData()) {
            return;
        }

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
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.fanMode;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.fanMode;
        }
        return FanMode::Off;
    }

    FanMode Controller::getRunningFanSpeed() {
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.runningFanMode;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.runningFanMode;
        }
        return FanMode::Off;
    }

    void Controller::setFanSpeedAbsolute(FanMode fanSpeed) {
        if (!receivingData()) {
            return;
        }

        nextFanModeCommand.fanMode = fanSpeed;
        sendFanModeCommand = true;
    }

    void Controller::setContinuousFanMode(bool on) {
        if (!receivingData()) {
            return;
        }

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
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.continuousFan;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.continuousFan;
        }
        return false;
    }

    void Controller::setOperatingMode(OperatingMode mode) {
        if (!receivingData()) {
            return;
        }

        if (mode == OperatingMode::Off) {
            setSystemOn(false);
        } else {
            nextOperatingModeCommand.mode = mode;
            sendOperatingModeCommand = true;
        }
    }

    OperatingMode Controller::getOperatingMode() {
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.operatingMode;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.operatingMode;
        }
        return OperatingMode::Off;
    }

    void Controller::setMasterSetpoint(double temperature) {
        if (!receivingData()) {
            return;
        }

        nextSetpointCommand.temperature = temperature;
        sendSetpointCommand = true;
    }
    
    double Controller::getMasterSetpoint() {
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.setpoint;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.setpoint;
        }
        return 0;
    }

    double Controller::getMasterCurrentTemperature() {
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.temperature;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.temperature;
        }
        return 0;
    }

    CompressorMode Controller::getCompressorMode() {
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.compressorMode;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.compressorMode;
        }
        return CompressorMode::Unknown;
    }

    bool Controller::isFanIdle() {
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.fanActive == false;;
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.fanActive == false;;
        }
        return true;
    }

    /// Zone Control

    void Controller::setZoneOn(uint8_t zone, bool on) {
        if (!receivingData()) {
            return;
        }

        for (int i=0; i<8; i++) {
            nextZoneStateCommand.zoneOn[i] = (i == zindex(zone) ? on : getZoneOn(i+1));
        }
        sendZoneStateCommand = true;
    }

    bool Controller::getZoneOn(uint8_t zone) {
        if (stateMessage.initialised == true) {
            // Read from State Message
            return stateMessage.zoneOn[zindex(zone)];
        } else if (stateMessage2.initialised == true) {
            // Read from State 2 Message
            return stateMessage2.zoneOn[zindex(zone)];
        }
        return false;
    }

    void Controller::setZoneSetpointTemperatureCustom(uint8_t zone, double temperature, bool adjustMaster) {
        if (!receivingData()) {
            return;
        }

        if (zoneControlled[zindex(zone)] == true) {
            // Check if we need to adjust the master first
            if (adjustMaster) {
                double minAllowed = masterToZoneMessage[zindex(zone)].minSetpoint;
                double maxAllowed = masterToZoneMessage[zindex(zone)].maxSetpoint;
                double diff = 0;
                if (temperature<minAllowed) {
                    diff = minAllowed-temperature;
                } else if (temperature>maxAllowed) {
                    diff = maxAllowed-temperature;
                }
                // If the difference is not 0 adjust
                if (diff != 0) {
                    double newTemperature = getMasterSetpoint() - diff;
                    setMasterSetpoint(newTemperature);
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

    void Controller::setZoneSetpointTemperature(uint8_t zone, double temperature, bool adjustMaster) {
        if (!receivingData()) {
            return;
        }

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
                double newTemperature = getMasterSetpoint() - diff;
                setMasterSetpoint(newTemperature);
            }
        }

        if (zoneControlled[zindex(zone)] == true) {
            // We are directly controlling this
            zoneSetpoint[zindex(zone)] = temperature;
        } else {
            // Send the custom zone setpoint message, the official 
            nextMasterToZoneMessage[zindex(zone)] = masterToZoneMessage[zindex(zone)];
            nextMasterToZoneMessage[zindex(zone)].minSetpoint = temperature;
            nextMasterToZoneMessage[zindex(zone)].maxSetpoint = temperature;
            sendMasterToZoneMessage[zindex(zone)] = true;
        }
    }

    double Controller::getZoneSetpointTemperature(uint8_t zone) {
        return stateMessage.zoneSetpoint[zindex(zone)];
    }

    void Controller::setZoneCurrentTemperature(uint8_t zone, double temperature) {
        zoneTemperature[zindex(zone)] = temperature;
    }

    double Controller::getZoneCurrentTemperature(uint8_t zone) {
        return zoneMessage[zindex(zone)].temperature;
    }

}