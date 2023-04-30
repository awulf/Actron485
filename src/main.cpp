#include "Utilities.h"
#include "Actron485.h"

#define RS485_SERIAL_BAUD 4800
#define RS485_SERIAL_MODE SERIAL_8N1
#define RXD GPIO_NUM_27 //Serial port RX2 pin assignment 
#define TXD GPIO_NUM_26 //Serial port TX2 pin assignment
#define WRITE_ENABLE GPIO_NUM_25 //Enable Out / Disable In

using namespace Actron485;

const size_t serialBufferSize = 64;
uint8_t serialBuffer[serialBufferSize];
unsigned long serialBufferReceivedTime = 0;
const unsigned long serialBufferBreak = 5; // milliseconds
uint8_t serialBufferIndex = 0;

// Zone number this controller belongs to
uint8_t ourZoneNumber = 3;
// If zone message hasn't been set
bool zoneStateInitialised = false;
// Zone message to send when requested
ZoneToMasterMessage zoneState;

// Zone control state, master controller can't control these
double temperature = 20.4;
double setpoint = 21.0;

// Zone own control requests. -1 (actronZoneModeIgnore) when not requesting (e.g. once request has been sent)
ZoneMode requestZoneMode = ZoneMode::Ignore;

uint8_t previousZoneWallPacket[8][5];
uint8_t previousZoneMasterPacket[8][7];

void serialWrite(bool enable) {
  if (enable) {
    digitalWrite(WRITE_ENABLE, HIGH); 
  } else {
    digitalWrite(WRITE_ENABLE, LOW); 
  }
}

void sendZoneInitResponse() {
    Serial.println("Send Zone Init");
    serialWrite(true);  
    delay(1);
    Serial1.write(0x00);
    Serial1.write(0xCC);
    Serial1.flush(true);
    serialWrite(false);
}

void sendZoneConfig() {
  Serial.println("Send Zone Config");
  ZoneToMasterMessage configMessage;
  configMessage.type = ZoneMessageType::Config;
  configMessage.zone = zoneState.zone;
  configMessage.mode = zoneState.mode;
  configMessage.setpoint = zoneState.setpoint;
  configMessage.temperature = 0;
  uint8_t data[5];
  configMessage.generate(data);
  serialWrite(true); 
  
  for (int i=0; i<5; i++) {
    Serial1.write(data[i]);
  }

  Serial1.flush(true);
  serialWrite(false);
}

void sendZoneMessage() {
  Serial.println("Send Zone Message");
  if (requestZoneMode != requestZoneMode) {
    zoneState.mode = requestZoneMode;
  }

  uint8_t data[5];
  zoneState.generate(data);

  serialWrite(true); 
  
  for (int i=0; i<5; i++) {
    Serial1.write(data[i]);
  }

  Serial1.flush(true);
  serialWrite(false);

  zoneState.print();
  Serial.println();
  printBinaryBytes(data, 5);
  Serial.println();
  Serial.println();
}

void processMasterMessage(MasterToZoneMessage masterMessage) {
  // Confirm our request to turn on/off the zone has been processed
  switch (requestZoneMode) {
    case ZoneMode::Ignore:
      break;
    case ZoneMode::Off:
      if (masterMessage.on == false) {
        requestZoneMode = ZoneMode::Ignore;
      }
      break;
    case ZoneMode::On:
    case ZoneMode::Open: // Master message doesn't show this case, so at this stage we can't tell we switch to open to on and vice versa, so assume it was successful anyway if on
      if (masterMessage.on == true) {
        requestZoneMode = ZoneMode::Ignore;
      }
  }

  // Update Zone on/off state based on this message (open is same as on, but the master message doesn't know the difference)
  if (masterMessage.on == true && zoneState.mode == ZoneMode::Off) {
    zoneState.mode = ZoneMode::On;
  } else if (masterMessage.on == false && zoneState.mode != ZoneMode::Off) {
    zoneState.mode = ZoneMode::Off;
  }
  
  // Clamp the Setpoint to the allowed range
  zoneState.setpoint = max(min(zoneState.setpoint, masterMessage.maxSetpoint), masterMessage.minSetpoint);
}

void sendData() {

  // serialWrite(true);  
  // delay(1);
  // Serial1.write(0x3D);
  // Serial1.write(0b00000010);
  // Serial1.flush(true);
  // serialWrite(false);

  // delay(2);

  // serialWrite(true);  
  // delay(1);
  // Serial1.write(0x03);
  // Serial1.flush(true);
  // serialWrite(false);

  // Serial.println("");
  // Serial.println(" SENT ");
  // Serial.println("");
}

void sendCommand() {

}

long endReceive = 0;

void decodeMasterToZone() {
    MasterToZoneMessage message = MasterToZoneMessage();
    message.parse(serialBuffer);

    uint8_t newData[7];
    message.generate(newData);

    // Check if changed
    bool print = false;
    
    int zone = message.zone;
    bool same = true;
    for (int i=0; i<7; i++) {
      same = same && previousZoneMasterPacket[zone][i] == serialBuffer[i];
      if (!same) {
        previousZoneMasterPacket[zone][i] = serialBuffer[i];
      }
    }
    print = zone == 3;//!same;

    if (!bytesEqual(serialBuffer, newData, 7)) {
      Serial.println("Error! Byte Miss Match!");
      message.print();
      Serial.println();

      // Binary
      printBinaryBytes(serialBuffer, 7);
      Serial.println();
      printBinaryBytes(newData, 7);
      Serial.println();
      Serial.println();

    } else if (print) {
      message.print();
      Serial.println();
      printBinaryBytes(newData, 7);
      Serial.println();
      Serial.println();
    }

    if (zone == ourZoneNumber) {
      processMasterMessage(message);
      if (!zoneStateInitialised) {
        // Send config first time
        sendZoneConfig();
        zoneStateInitialised = true;
      } else {
        sendZoneMessage();
      }
    }
}

void decodeZoneToMaster() {
    ZoneToMasterMessage message = ZoneToMasterMessage();
    message.parse(serialBuffer);

    // Check if changed
    bool print = false;
    
    int zone = message.zone;
    bool same = true;
    for (int i=0; i<5; i++) {
      same = same && previousZoneWallPacket[zone][i] == serialBuffer[i];
      if (!same) {
        previousZoneWallPacket[zone][i] = serialBuffer[i];
      }
    }
    print = zone == 3;//!same || message.type == actronZoneMessageTypeInitZone;

    uint8_t newData[5];
    message.generate(newData);

    if (!bytesEqual(serialBuffer, newData, 5)) {
      Serial.println("Error! Byte Miss Match!");
      message.print();
      Serial.println();

      // Binary
      printBinaryBytes(serialBuffer, 5);
      Serial.println();
      printBinaryBytes(newData, 5);
      Serial.println();
      Serial.println();

    } else if (print) {
      message.print();
      Serial.println();
      printBinaryBytes(newData, 7);
      Serial.println();
      Serial.println();
    }

    if (message.type == ZoneMessageType::InitZone && message.zone == ourZoneNumber) {
      sendZoneInitResponse();
      zoneStateInitialised = false;
    }    
}

bool zoneDecode() {
  // Zones Communication

  uint8_t byte = serialBuffer[0];
  if (serialBufferIndex == 7 && (byte & 0b11110000) == 0x80) {
    // Master -> Zone Wall
    decodeMasterToZone();
    return true;
  } else if (serialBufferIndex == 5 && (byte & 0b11110000) == 0xC0) {
    // Zone Wall -> Master
    decodeZoneToMaster();
    return true;
  }
  return false;
}

void setup() {
  // Setup zone state
  zoneState.mode = ZoneMode::Ignore;
  zoneState.type = ZoneMessageType::Normal;
  zoneState.zone = ourZoneNumber;
  zoneState.setpoint = setpoint; // TODO: read from memory
  zoneState.temperature = temperature; // TODO: read from sensor

  // Logging serial
  Serial.begin(115200);

  // Setup RS485 serial
  pinMode(WRITE_ENABLE, OUTPUT);
  serialWrite(false);
  Serial1.begin(4800, SERIAL_8N1, RXD, TXD);
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long now = millis();
  long serialLastReceivedTime = now-serialBufferReceivedTime;

  if (serialBufferIndex > 0 && serialLastReceivedTime > serialBufferBreak) {

    if (!zoneDecode()) {
      for (int j=0; j<serialBufferIndex; j++) {
        printByte(serialBuffer[j]);
      }
      Serial.println();
      // printBinaryBytes(serialBuffer, serialBufferIndex);
      // Serial.print(" (");
      // Serial.print(serialLastReceivedTime);
      // Serial.println("ms)");
      Serial.println();
      Serial.println();
    }

    if (serialBuffer[serialBufferIndex-2] == 0xFF && serialBuffer[serialBufferIndex-1] == 0xF9) {
      Serial.println("==========");
      sendData();
    }

    serialBufferIndex = 0;
  }

  while(Serial1.available() > 0 && serialBufferIndex < serialBufferSize) {
    uint8_t byte = Serial1.read();
    serialBuffer[serialBufferIndex] = byte;
    serialBufferIndex++;
    serialBufferReceivedTime = millis();
  }

}

