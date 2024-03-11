#include "Actron485Models.h"
#include "Utilities.h"

namespace Actron485 {

///////////////////////////////////
// Actron485::ZoneToMasterMessage

void ZoneToMasterMessage::print() {
    Serial.println((int)&printOut);
    if (!printOut) {
        return;
    }
    // Decoded
    printOut->print("C: Zone: ");
    printOut->print(zone);

    if (type == ZoneMessageType::Normal) {
        printOut->print(", Set Point: ");
        printOut->print(setpoint);

        printOut->print(", Temp: ");
        printOut->print(temperature);

        printOut->print("(Pre:");
        printOut->print(temperaturePreAdjustment);
        printOut->print(")");

        printOut->print(", Mode: ");
        switch (mode) {
            case ZoneMode::Off:
                printOut->print("Off");
                break;
            case ZoneMode::On:
                printOut->print("On");
                break;
            case ZoneMode::Open:
                printOut->print("Open");
                break;
        }

    } else if (type == ZoneMessageType::Config) {
        printOut->print(", Temp Offset: ");
        printOut->print(temperature);

    } else if (type == ZoneMessageType::InitZone) {
        printOut->print(", Init Zone");
        printOut->print(temperature);
    }
}

void ZoneToMasterMessage::parse(uint8_t data[5]) {
    initialised = true;

    zone = data[0] & 0b00001111;

    setpoint = data[1] / 2.0;

    bool zoneOn = (data[2] & 0b10000000) == 0b10000000;
    bool openMode = (data[2] & 0b01000000) == 0b01000000;
    if (zoneOn && openMode) {
        mode = ZoneMode::Open;
    } else if (zoneOn) {
        mode = ZoneMode::On;
    } else {
        mode = ZoneMode::Off;
    }

    bool configMessage = (data[2] & 0b00100000) == 0b00100000;
    bool initMessage = (data[2] & 0b00010000) == 0b00010000;
    if (configMessage) {
        type = ZoneMessageType::Config;
        
        // Offset temperature
        temperature = (double)((int8_t)data[3]) / 10.0;
    } else if (initMessage) {
        type = ZoneMessageType::InitZone;

    } else {
        type = ZoneMessageType::Normal;

        // Two last bits of data[2] are the leading bits for the raw temperature value
        uint8_t leadingBites = data[2] & 0b00000011;
        uint8_t negative = (leadingBites & 0b10) == 0b10;

        int16_t rawTempValue = (((negative ? 0b11111100 : 0x0) | leadingBites) << 8) | (uint16_t)data[3];
        int16_t rawTemp = rawTempValue + (negative ? 512 : -512); // A 512 offset

        temperaturePreAdjustment = (250 - rawTemp) * 0.1;
        temperature = zoneTempFromMaster(rawTemp);
    }
}

void ZoneToMasterMessage::generate(uint8_t data[5]) {
    // Byte 1, Start nibble C and zone nibble
    data[0] = 0b11000000 | zone;

    // Byte 2, Set Point Temp where Temp=Number/2. In 0.5° increments. 16->30
    data[1] = (uint8_t) (setpoint * 2.0);

    if (type == ZoneMessageType::Normal) {
        int16_t rawTemp = zoneTempToMaster(temperature);
        int16_t rawTempValue = rawTemp - (rawTemp < 0 ? -512 : 512);
        temperaturePreAdjustment = (250 - rawTemp) * 0.1; // Store for reference

        // Byte 3
        data[2] = (mode != ZoneMode::Off ? 0b10000000 : 0b0) | (mode == ZoneMode::Open ? 0b01000000 : 0b0) | (rawTempValue >> 8 & 0b00000011);

        // Byte 4
        data[3] = (uint8_t)rawTempValue;

        // Byte 5, check/verify byte
        data[4] = data[2] - (data[2] << 1) - data[3] - data[1] - data[0] - 1;

    } else if (type == ZoneMessageType::Config) {
        // Byte 3, config bit set to 1
        data[2] = (mode != ZoneMode::Off ? 0b10000000 : 0b0) | (mode == ZoneMode::Open ? 0b01000000 : 0b0) | 0b00100000;

        // Byte 4, temperature calibration offset x10, E.g. -32 * 0.1 -> -3.2. Min -3.2 Max 3.0°C
        data[3] = (int8_t) (temperature * 10.0);

        // Byte 5, check/verify byte
        data[4] = data[2] - data[3] - data[1] - (data[0] & 0b1111) - 1;

    }  else if (type == ZoneMessageType::InitZone) {
        // Byte 3, config bit set to 1
        data[2] = (mode != ZoneMode::Off ? 0b10000000 : 0b0) | (mode == ZoneMode::Open ? 0b01000000 : 0b0) | 0b00010001;

        // Byte 4, empty
        data[3] = 0;

        // Byte 5, check/verify byte
        data[4] = data[2] - (data[2] << 1) - data[3] - data[1] - data[0] - 1;
    }
}

double ZoneToMasterMessage::zoneTempFromMaster(int16_t rawValue) {
    double temp;
    double out = 0;
    if (rawValue < -58) {
        // Temp High (above 30.8°C)
        out = 0.00007124*pow(rawValue, 2) - 0.1052*rawValue+24.5;
        temp = round(out * 10) / 10.0;
    } else if (rawValue > 81) {
        // Low Temp (below 16.9°C)
        out = (-0.00001457*pow(rawValue, 2) - 0.0988*rawValue+24.923);
        temp = round(out * 10) / 10.0;
    } else {
        temp = (250 - rawValue) * 0.1;
    }  
    return temp;
}

int16_t ZoneToMasterMessage::zoneTempToMaster(double temperature) {
    int16_t out = 0;
    if (temperature > 30.8) {
        out = (int16_t) round(-118.478*(sqrt(14.3372 + temperature)-6.23195));
    } else if (temperature < 16.9) {
        out = (int16_t) round(261.981*(sqrt(192.415 - temperature)-12.9419));
    } else {
        out = (int16_t) round(250 - temperature * 10);
    }
    return out;
}

///////////////////////////////////
// Actron485::MasterToZoneMessage

void MasterToZoneMessage::print() {
    if (!printOut) {
        return;
    }

    printOut->print("M: Zone: ");
    printOut->print(zone);

    printOut->print(", Set Point: ");
    printOut->print(setpoint);

    printOut->print(", Temp: ");
    printOut->print(temperature);

    printOut->print(", SP Range: ");
    printOut->print(minSetpoint);
    printOut->print("-");
    printOut->print(maxSetpoint);

    printOut->print(", Zone: ");
    printOut->print(on ? "On" : "Off");

    if (on) {
        printOut->print(", Compr. Mode: ");
        printOut->print(compressorMode ? "On " : "Off");
        if (compressorMode) {
            printOut->print(heating ? "Heating" : "Cooling");
        }

        if (fanMode) {
            printOut->print(", Fan Mode");
        } else if (compressorActive) {
            printOut->print(", Compr. Active");
        }
    }

    printOut->print(", Damper Pos: ");
    printOut->print((int)((double)damperPosition/5.0*100.0));
    printOut->print("%");

    if (maybeAdjusting) {
        printOut->print(", ??Adjusting??");
    }
}

void MasterToZoneMessage::parse(uint8_t data[7]) {
    initialised = true;

    zone = data[0] & 0b00001111;

    // Temperature bits: [23][08 - 15]
    uint16_t zoneTempRaw = (uint16_t) data[1] | ((uint16_t) (data[2] & 0b1) << 8);
    temperature = zoneTempRaw * 0.1;

    minSetpoint = (data[3] & 0b00111111) / 2.0;
    maxSetpoint = (data[5] & 0b00111111) / 2.0;
    setpoint = (data[4] & 0b00111111) / 2.0;

    on = (data[2] & 0b01000000) == 0b01000000;

    maybeAdjusting = (data[2] & 0b00000010) == 0b00000010;

    compressorMode = (data[2] & 0b10000000) == 0b10000000;

    fanMode = (data[4] & 0x80) == 0x80;

    heating = (data[3] & 0b10000000) == 0b10000000;

    compressorActive = (data[5] & 0x80) == 0x80;

    damperPosition = (data[2] & 0b00011100) >> 2;

    if (!compressorMode && !fanMode) {
        operationMode = ZoneOperationMode::SystemOff;
    } else if (!on) {
        operationMode = ZoneOperationMode::ZoneOff;
    } else if (fanMode) {
        operationMode = ZoneOperationMode::FanOnly;
    } else if (!compressorActive) {
        operationMode = ZoneOperationMode::Standby;
    } else if (heating) {
        operationMode = ZoneOperationMode::Heating;
    } else {
        operationMode = ZoneOperationMode::Cooling;
    }
}

void MasterToZoneMessage::generate(uint8_t data[7]) {
    // Byte 1, Start nibble 8 and zone nibble
    data[0] = 0b10000000 | zone;

    // Byte 2, lower part of zone temperature
    uint16_t zoneTempRaw = (temperature * 10.0);
    data[1] = (uint8_t) zoneTempRaw;

    // Byte 3, first bit ac in compressor mode, second zone on. third unknown, 4-6 damper pos, 7 maybe turning off?, 8 first bit for zone temp
    data[2] = (compressorMode ? 0b10000000 : 0b0) | (on ? 0b01000000 : 0b0) | (/*Unknown*/0b0) | 
              (0b00011100 & (damperPosition << 2)) | (maybeAdjusting ? 0b00000010 : 0b0) | (0b00000001 & (zoneTempRaw >> 8));

    // Byte 4, heating mode, unknown, min setpoint lower 6
    data[3] = (heating ? 0b10000000 : 0b0) | (/*Unknown*/0b0) | ((uint8_t)(minSetpoint * 2.0)) & 0b00111111;

    // Byte 5, first bit fan mode, second unknown, lower 6 setpoint
    data[4] = (fanMode ? 0b10000000 : 0b0) | (/*Unknown*/0b0) | ((uint8_t)(setpoint * 2.0)) & 0b00111111;

    // Byte 6, first bit compressor on for zone, second unknow, lower 6 max setpoint
    data[5] = (compressorActive ? 0b10000000 : 0b0) | (/*Unknown*/0b0) | ((uint8_t)(maxSetpoint * 2.0)) & 0b00111111;

    // Byte 7, check byte
    data[6] = data[2] - (data[2] << 1) - data[5] - data[4] - data[3] - data[1] - data[0] - 1;
}

///////////////////////////////////
// Actron485::MasterSetpointCommand

void MasterSetpointCommand::print() {
    printOut->print("Command Master Temperature Setpoint: ");
    printOut->println(temperature);
}

void MasterSetpointCommand::parse(uint8_t data[2]) {
    temperature = ((double) data[1]) / 2.0;
}

void MasterSetpointCommand::generate(uint8_t data[2]) {
    data[0] = (uint8_t) MessageType::CommandMasterSetpoint;
    data[1] = (uint8_t) round(temperature * 2);
}

///////////////////////////////////
// Actron485::FanModeCommand

FanMode FanModeCommand::getFanSpeed() {
    switch (fanMode) {
        case FanMode::LowContinuous:
            return FanMode::Low;
        case FanMode::MediumContinuous:
            return FanMode::Medium;
        case FanMode::HighContinuous:
            return FanMode::High;
        case FanMode::EspContinuous:
            return FanMode::Esp;
        default:
            return fanMode;
    }
}

bool FanModeCommand::isContinuous() {
    switch (fanMode) {
        case FanMode::LowContinuous:
            return true;
        case FanMode::MediumContinuous:
            return true;
        case FanMode::HighContinuous:
            return true;
        case FanMode::EspContinuous:
            return true;
        default:
            return false;
    }
}

void FanModeCommand::print() {
    if (!printOut) {
        return;
    }

    printOut->print("Command: Fan Mode: ");
    switch (fanMode) {
        case FanMode::Off:
            printOut->println("Off");
            break;
        case FanMode::Low:
            printOut->println("Low");
            break;
        case FanMode::Medium:
            printOut->println("Medium");
            break;
        case FanMode::High:
            printOut->println("High");
            break;
        case FanMode::Esp:
            printOut->println("ESP");
            break;
        case FanMode::LowContinuous:
            printOut->println("Low Continuous");
            break;
        case FanMode::MediumContinuous:
            printOut->println("Medium Continuous");
            break;
        case FanMode::HighContinuous:
            printOut->println("High Continuous");
            break;
        case FanMode::EspContinuous:
            printOut->println("ESP Continuous");
            break;
    }
}

void FanModeCommand::parse(uint8_t data[2]) {
    fanMode = FanMode(data[1]);
}

void FanModeCommand::generate(uint8_t data[2]) {
    data[0] = (uint8_t) MessageType::CommandFanMode;
    data[1] = (uint8_t) fanMode;
}

///////////////////////////////////
// Actron485::ZoneStateCommand

void ZoneStateCommand::print() {
    if (!printOut) {
        return;
    }

    printOut->println("Command: Zone State: ");
    printOut->println("1 2 3 4 5 6 7 8");
    for (int i = 0; i<8; i++) {
        printOut->print(zoneOn[i] ? "1 " : "_ ");
    }
    printOut->println();
}

void ZoneStateCommand::parse(uint8_t data[2]) {
    for (int i=0; i<8; i++) {
        zoneOn[i] = (data[1] & (1 << i)) >> i;
    }
}

void ZoneStateCommand::generate(uint8_t data[2]) {
    data[0] = (uint8_t) MessageType::CommandZoneState;
    data[1] = 0;
    for (int i=0; i<8; i++) {
        data[1] = data[1] | (zoneOn[i] << i);
    }
}

///////////////////////////////////
// Actron485::OperatingModeCommand

bool OperatingModeCommand::onCommand() {
    switch (mode) {
        case OperatingMode::FanOnly:
            return true;
        case OperatingMode::Auto:
            return true;
        case OperatingMode::Cool:
            return true;
        case OperatingMode::Heat:
            return true;
    }
    return false;
}

void OperatingModeCommand::print() {
    if (!printOut) {
        return;
    }

    printOut->print("Command: Operating Mode: ");
    switch (mode) {
        case OperatingMode::Off:
            printOut->println("Off");
            break;
        case OperatingMode::FanOnly:
            printOut->println("Fan Only");
            break;
        case OperatingMode::Auto:
            printOut->println("Auto");
            break;
        case OperatingMode::Cool:
            printOut->println("Cool");
            break;
        case OperatingMode::Heat:
            printOut->println("Heat");
            break;
    }
}

void OperatingModeCommand::parse(uint8_t data[2]) {
    mode = OperatingMode(data[1]);
}

void OperatingModeCommand::generate(uint8_t data[2]) {
    data[0] = (uint8_t) MessageType::CommandOperatingMode;
    data[1] = (uint8_t) mode;
}

///////////////////////////////////
// Actron485::ZoneSetpointCustomCommand

void ZoneSetpointCustomCommand::print() {
    if (!printOut) {
        return;
    }

    printOut->print("Command: Zone ");
    printOut->print(zone);
    printOut->print(", Temperature Setpoint: ");
    printOut->print(temperature);
    if (adjustMaster) {
        printOut->print(", Force Master SP");
    }
    printOut->println();
}

void ZoneSetpointCustomCommand::parse(uint8_t data[3]) {
    zone = data[1];
    temperature = ((double) data[2]) / 2.0;
    adjustMaster = (bool) data[3];
}

void ZoneSetpointCustomCommand::generate(uint8_t data[3]) {
    data[0] = (uint8_t) MessageType::CustomCommandChangeZoneSetpoint;
    data[1] = zone;
    data[2] = (uint8_t) round(temperature * 2);
    data[3] = (uint8_t) adjustMaster;
}

///////////////////////////////////
// Actron485::StateMessage

void StateMessage::print() {
    if (!printOut) {
        return;
    }

    printOut->print("State: ");
    printOut->print("Operating Mode: ");
    switch (operatingMode) {
        case OperatingMode::Off:
            printOut->print("Off-Off");
            break;
        case OperatingMode::OffAuto:
            printOut->print("Off-Auto");
            break;
        case OperatingMode::OffCool:
            printOut->print("Off-Cool");
            break;
        case OperatingMode::OffHeat:
            printOut->print("Off-Heat");
            break;
        case OperatingMode::FanOnly:
            printOut->print("Fan Only");
            break;
        case OperatingMode::Auto:
            printOut->print("Auto");
            break;
        case OperatingMode::Cool:
            printOut->print("Cool");
            break;
        case OperatingMode::Heat:
            printOut->print("Heat");
            break;
    }
    printOut->print(" - ");
    switch (compressorMode) {
        case CompressorMode::Unknown:
            printOut->print("Unknown");
            break;
        case CompressorMode::Idle:
            printOut->print("Idle");
            break;
        case CompressorMode::Cooling:
            printOut->print("Cooling");
            break;
        case CompressorMode::Heating:
            printOut->print("Heating");
            break;
    }

    printOut->print(", Fan Mode: ");
    switch (fanMode) {
        case FanMode::Off:
            printOut->print("Off");
            break;
        case FanMode::Low:
            printOut->print("Low");
            break;
        case FanMode::Medium:
            printOut->print("Medium");
            break;
        case FanMode::High:
            printOut->print("High");
            break;
        case FanMode::Esp:
            printOut->print("ESP");
            switch (runningFanMode) {
                case FanMode::Low:
                    printOut->print("-Low");
                    break;
                case FanMode::Medium:
                    printOut->print("-Medium");
                    break;
                case FanMode::High:
                    printOut->print("-High");
                    break;
            }
            break;
    }
    if (continuousFan) {
        printOut->print(" Continuous");
    } 
    printOut->print(" - ");
    if (fanActive) {
        printOut->print("Active");
    } else {
        printOut->print("Idle");
    }

    printOut->print(", Setpoint: ");
    printOut->print(setpoint);

    printOut->print(", Temperature: ");
    printOut->print(temperature);

    printOut->print(", Zones:");
    for (int i=0; i<8; i++) {
        printOut->print(" ");
        printOut->print(i+1);
        printOut->print(":");
        printOut->print((zoneOn[i] ? "On" : "Off"));
    }
}

void StateMessage::parse(uint8_t data[StateMessage::stateMessageLength]) {
    initialised = true;

    for (int i=0; i<8; i++) {
        zoneSetpoint[i] = (double)data[i+3] / 2.0;
        zoneOn[i] = (data[11] & (1 << i)) >> i;
    }

    setpoint = (double)data[14] / 2.0;

    temperature = ((double) (((uint16_t)data[16] << 8) | data[17])) / 10.0;
    
    uint8_t fanModeRaw = (data[15] & 0b111100) >> 2;
    switch (fanModeRaw) {
        case 0b0000:
            runningFanMode = FanMode::Off;
            break;
        case 0b1000:
            runningFanMode = FanMode::Low;
            break;
        case 0b0100:
            runningFanMode = FanMode::Medium;
            break;
        case 0b0010:
            runningFanMode = FanMode::High;
            break;
    }
    
    if ((data[15] & 0b10) == 0b10) {
        fanMode = FanMode::Esp;
    } else {
        fanMode = runningFanMode;
    }

    continuousFan = (data[15] & 0b10000000) == 0b10000000;
    fanActive = (data[15] & 0b1) == 0; // 0 == Active, 1 == Idle

    uint8_t operatingModeRaw = (data[13] & 0b00011111);
    operatingMode = OperatingMode(operatingModeRaw);

    // Compressor mode
    uint8_t compressorModeRaw = (data[13] & 0b01100000) >> 5;
    switch (compressorModeRaw) {
        case 0:
            compressorMode = CompressorMode::Idle;
            break;
        case 1:
            compressorMode = CompressorMode::Heating;
            break;
        case 2:
            compressorMode = CompressorMode::Cooling;
            break;
        default:
            compressorMode = CompressorMode::Unknown;
            break;       
    }
}

///////////////////////////////////
// Actron485::StateMessage2

void StateMessage2::print() {
    if (!printOut) {
        return;
    }

    printOut->print("State 2: ");
    printOut->print("Operating Mode: ");
    switch (operatingMode) {
        case OperatingMode::Off:
            printOut->print("Off-Off");
            break;
        case OperatingMode::OffAuto:
            printOut->print("Off-Auto");
            break;
        case OperatingMode::OffCool:
            printOut->print("Off-Cool");
            break;
        case OperatingMode::OffHeat:
            printOut->print("Off-Heat");
            break;
        case OperatingMode::FanOnly:
            printOut->print("Fan Only");
            break;
        case OperatingMode::Auto:
            printOut->print("Auto");
            break;
        case OperatingMode::Cool:
            printOut->print("Cool");
            break;
        case OperatingMode::Heat:
            printOut->print("Heat");
            break;
    }
    printOut->print(" - ");
    switch (compressorMode) {
        case CompressorMode::Unknown:
            printOut->print("Unknown");
            break;
        case CompressorMode::Idle:
            printOut->print("Idle");
            break;
        case CompressorMode::Cooling:
            printOut->print("Cooling");
            break;
        case CompressorMode::Heating:
            printOut->print("Heating");
            break;
    }

    printOut->print(", Fan Mode: ");
    switch (fanMode) {
        case FanMode::Off:
            printOut->print("Off");
            break;
        case FanMode::Low:
            printOut->print("Low");
            break;
        case FanMode::Medium:
            printOut->print("Medium");
            break;
        case FanMode::High:
            printOut->print("High");
            break;
        case FanMode::Esp:
            printOut->print("ESP");
            switch (runningFanMode) {
                case FanMode::Low:
                    printOut->print("-Low");
                    break;
                case FanMode::Medium:
                    printOut->print("-Medium");
                    break;
                case FanMode::High:
                    printOut->print("-High");
                    break;
            }
            break;
    }
    if (continuousFan) {
        printOut->print(" Continuous");
    } 
    printOut->print(" - ");
    if (fanActive) {
        printOut->print("Active");
    } else {
        printOut->print("Idle");
    }

    printOut->print(", Setpoint: ");
    printOut->print(setpoint);

    printOut->print(", Temperature: ");
    printOut->print(temperature);

    printOut->print(", Zones:");
    for (int i=0; i<8; i++) {
        printOut->print(" ");
        printOut->print(i+1);
        printOut->print(":");
        printOut->print((zoneOn[i] ? "On" : "Off"));
    }
}

void StateMessage2::parse(uint8_t data[StateMessage::stateMessageLength]) {
    initialised = true;

    for (int i=0; i<8; i++) {
        zoneOn[i] = (data[6] & (1 << i)) >> i;
    }

    setpoint = (double)data[4] / 2.0;

    temperature = ((double) (((uint16_t)data[9] << 8) | data[10])) / 10.0;
    
    uint8_t fanModeRaw = (data[05] & 0b111100) >> 2;
    switch (fanModeRaw) {
        case 0b0000:
            runningFanMode = FanMode::Off;
            break;
        case 0b1000:
            runningFanMode = FanMode::Low;
            break;
        case 0b0100:
            runningFanMode = FanMode::Medium;
            break;
        case 0b0010:
            runningFanMode = FanMode::High;
            break;
    }
    
    if ((data[05] & 0b10) == 0b10) {
        fanMode = FanMode::Esp;
    } else {
        fanMode = runningFanMode;
    }

    continuousFan = (data[5] & 0b10000000) == 0b10000000;
    fanActive = (data[5] & 0b1) == 0; // 0 == Active, 1 == Idle

    uint8_t operatingModeRaw = (data[3] & 0b00011111);
    operatingMode = OperatingMode(operatingModeRaw);

    // Compressor mode
    uint8_t compressorModeRaw = (data[3] & 0b01100000) >> 5;
    switch (compressorModeRaw) {
        case 0:
            compressorMode = CompressorMode::Idle;
            break;
        case 1:
            compressorMode = CompressorMode::Heating;
            break;
        case 2:
            compressorMode = CompressorMode::Cooling;
            break;
        default:
            compressorMode = CompressorMode::Unknown;
            break;       
    }

}

}