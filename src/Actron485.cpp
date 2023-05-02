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
        if (_requestZoneMode[zone-1] != ZoneMode::Ignore) {
            zoneMessage[zone-1].mode = _requestZoneMode[zone-1];
        }

        uint8_t data[zoneMessage[zone-1].messageLength];
        zoneMessage[zone-1].generate(data);

        serialWrite(true); 
        
        for (int i=0; i<5; i++) {
            Serial1.write(data[i]);
        }

        serialWrite(false);
    }

    void Controller::sendZoneConfigMessage(int zone) {
        if (zone <= 0 || zone > 8) {
            // Out of bounds
            return;
        }
        Serial.println("Send Zone Config");
        ZoneToMasterMessage configMessage;
        configMessage.type = ZoneMessageType::Config;
        configMessage.zone = zoneMessage[zone-1].zone;
        configMessage.mode = zoneMessage[zone-1].mode;
        configMessage.setpoint = zoneMessage[zone-1].setpoint;
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
        if (zoneControlled[zone-1] == false) {
            // We don't care about this, not us
            return;
        }

        // Confirm our request to turn on/off the zone has been processed
        switch (_requestZoneMode[zone-1]) {
            case ZoneMode::Ignore:
                break;
            case ZoneMode::Off:
                if (masterMessage.on == false) {
                    _requestZoneMode[zone-1] = ZoneMode::Ignore;
                }
                break;
            case ZoneMode::On:
            case ZoneMode::Open: // Master message doesn't show this case, so at this stage we can't tell we switch to open to on and vice versa, so assume it was successful anyway if on
                if (masterMessage.on == true) {
                    _requestZoneMode[zone-1] = ZoneMode::Ignore;
            }
        }

        // Update Zone on/off state based on this message (open is same as on, but the master message doesn't know the difference)
        if (masterMessage.on == true && zoneMessage[zone-1].mode == ZoneMode::Off) {
            zoneMessage[zone-1].mode = ZoneMode::On;
        } else if (masterMessage.on == false && zoneMessage[zone-1].mode != ZoneMode::Off) {
            zoneMessage[zone-1].mode = ZoneMode::Off;
        }
        
        // Clamp the Setpoint to the allowed range
        zoneMessage[zone-1].setpoint = max(min(zoneMessage[zone-1].setpoint, masterMessage.maxSetpoint), masterMessage.minSetpoint);

        ////////////////////////
        // Send our awaited status report/request/config

        // Do we want to send a config message this round?
        if (_sendZoneConfig[zone-1]) {
            sendZoneConfigMessage(zone-1);
            _sendZoneConfig[zone-1] = false;
        } else {
            sendZoneMessage(zone-1);
        }
    }
    
    void Controller::processZoneMessage(ZoneToMasterMessage zoneMessage) {
        if (zoneControlled[zoneMessage.zone] == false) {
            // We don't care about this, not us
            return;
        }

        // If we got a zone message from not us, its probably the master contoller
        // asking us to respond
        if (zoneMessage.type == ZoneMessageType::InitZone) {
            sendZoneInitMessage(zoneMessage.zone);
            _sendZoneConfig[zoneMessage.zone] = true;
        }
    }

    Controller::Controller(uint8_t rxPin, uint8_t txPin, uint8_t writeEnablePin) {
        if (writeEnablePin > 0) {
            pinMode(writeEnablePin, OUTPUT);
            serialWrite(false);
        }
        
        _rxPin = rxPin;
        _txPin = txPin;
        _writeEnablePin = writeEnablePin;
        Serial1.begin(4800, SERIAL_8N1, rxPin, txPin);
        _serial = &Serial1;
        setup();
    }

    void Controller::setup() {
        
    }

    bool Controller::sendQueuedCommand() {
        return false;
    }

    Controller::Controller(Stream &stream, uint8_t writeEnablePin) {
        _serial = &stream;
        _writeEnablePin = writeEnablePin;
        setup();
    }

    void Controller::loop() {
        unsigned long now = millis();
        long serialLastReceivedTime = now-_serialBufferReceivedTime;

        if (_serialBufferIndex > 0 && serialLastReceivedTime > _serialBufferBreak) {
            MessageType messageType = detectMessageType(_serialBuffer[0]);

            bool printChangesOnly = printOutMode == PrintOutMode::ChangedMessages;
            bool printAll = printOutMode == PrintOutMode::AllMessages;
            bool changed = false;

            bool popSendQueue = false;

            uint8_t zone = 0;

            switch (messageType) {
                case MessageType::Unknown:
                    printOut.println("Unknown Message received");
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
                        if (zoneControlled[command.zone]) {
                            zoneSetpoint[command.zone] = command.temperature;
                        }
                    }
                    break;
                case MessageType::ZoneWallController:
                    zone = _serialBuffer[0] & 0x0F;
                    if (0 < zone && zone <= 8) {
                        zoneMessage[zone-1].parse(_serialBuffer);
                        changed = copyBytes(_serialBuffer, zoneWallMessageRaw[zone-1], zoneMessage[zone-1].messageLength);

                        if (printAll || printChangesOnly && changed) {
                            zoneMessage[zone-1].print();
                            printOut.println();
                        }
                    }
                    break;
                case MessageType::ZoneMasterController:
                    zone = _serialBuffer[0] & 0x0F;
                    if (0 < zone && zone <= 8) {
                        masterToZoneMessage[zone-1].parse(_serialBuffer);
                        changed = copyBytes(_serialBuffer, zoneMasterMessageRaw[zone-1], masterToZoneMessage[zone-1].messageLength);

                        if (printAll || printChangesOnly && changed) {
                            masterToZoneMessage[zone-1].print();
                            printOut.println();
                        }
                    }
                    break;
                case MessageType::BoardComms1:
                    changed = copyBytes(_serialBuffer, boardComms1Message[boardComms1Index], _serialBufferIndex);
                    boardComms1MessageLength[boardComms1Index] = _serialBufferIndex;
                    boardComms1Index++;
                    break;
                case MessageType::BoardComms2:
                    changed = copyBytes(_serialBuffer, boardComms2Message, boardComms2MessageLength);
                    break;
                case MessageType::Stat1:
                    changed = copyBytes(_serialBuffer, stat1Message, stat1MessageLength);
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
                        processZoneMessage(zoneMessage[zone-1]);
                        break;
                    case MessageType::ZoneMasterController:
                        processMasterMessage(masterToZoneMessage[zone-1]);
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

}