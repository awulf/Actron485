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

    }

    void Controller::sendZoneConfigMessage(int zone) {
        Serial.println("Send Zone Config");
        ZoneToMasterMessage configMessage;
        configMessage.type = ZoneMessageType::Config;
        configMessage.zone = zoneMessage[zone].zone;
        configMessage.mode = zoneMessage[zone].mode;
        configMessage.setpoint = zoneMessage[zone].setpoint;
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

    void Controller::processMessage(uint8_t *data) {

    }

    void Controller::processMasterMessage(MasterToZoneMessage masterMessage) {

    }

    
    void Controller::processZoneMessage(ZoneToMasterMessage zoneMessage) {

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
    }

    void Controller::setup() {

    }

    bool Controller::sendQueuedCommand() {
        return false;
    }

    Controller::Controller(Stream &stream, uint8_t writeEnablePin) {
        _serial = &stream;
        _writeEnablePin = writeEnablePin;
    }

    void Controller::loop() {
        unsigned long now = millis();
        long serialLastReceivedTime = now-_serialBufferReceivedTime;

        if (_serialBufferIndex > 0 && serialLastReceivedTime > _serialBufferBreak) {
            MessageType messageType = detectMessageType(_serialBuffer[0]);

            switch (messageType) {
                case MessageType::Unknown:
                    break;
                case MessageType::CommandMasterSetpoint:
                    break;
                case MessageType::CommandFanMode:
                    break;
                case MessageType::CommandOperatingMode:
                    break;
                case MessageType::CommandZoneState:
                    break;
                case MessageType::ZoneWallController:
                    break;
                case MessageType::ZoneMasterController:
                    break;
                case MessageType::BoardComms1:
                    break;
                case MessageType::BoardComms2:
                    break;
                case MessageType::Stat1:
                    break;
                case MessageType::Stat2:
                    break;
                case MessageType::Stat3:
                    // Last message in the sequence, our chance to send a message
                    printOut.println("==========");
                    sendQueuedCommand();
                    break;
            }

            // if (!zoneDecode()) {
            //     for (int j=0; j<_serialBufferIndex; j++) {
            //         printByte(_serialBuffer[j]);
            //     }
            //     Serial.println();
            //     // printBinaryBytes(serialBuffer, serialBufferIndex);
            //     // Serial.print(" (");
            //     // Serial.print(serialLastReceivedTime);
            //     // Serial.println("ms)");
            //     Serial.println();
            //     Serial.println();
            // }

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