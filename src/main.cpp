#include "Utilities.h"
#include "ActronDataModels.h"

#define RS485_SERIAL_BAUD 4800
#define RS485_SERIAL_MODE SERIAL_8N1
#define RXD GPIO_NUM_27 //Serial port RX2 pin assignment 
#define TXD GPIO_NUM_26 //Serial port TX2 pin assignment
#define WRITE_ENABLE GPIO_NUM_25 //Enable Out / Disable In

const size_t serialBufferSize = 64;
uint8_t serialBuffer[serialBufferSize];
unsigned long serialBufferReceivedTime = 0;
const unsigned long serialBufferBreak = 5; // milliseconds
uint8_t serialBufferIndex = 0;


uint8_t previousZonePacket[5];

void serialWrite(bool enable) {
  if (enable) {
    digitalWrite(WRITE_ENABLE, HIGH); 
  } else {
    digitalWrite(WRITE_ENABLE, LOW); 
  }
}

void sendData() {
  serialWrite(true);  
  delay(1);
  Serial1.write(0xC3);
  Serial1.write(0x31);
  Serial1.write(0x02);
  Serial1.write(0x0B);
  Serial1.write(0xFF);
  Serial1.flush(true);
  serialWrite(false);
  Serial.println("");
  Serial.println(" SENT ");
  Serial.println("");
}



long endReceive = 0;

void decodeMasterToZone() {
  int count = 7;

  uint8_t zone = serialBuffer[0] & 0b00001111;
  
  if (zone == 3) {
    // sendData();
  }

  if (zone != 3) {
    // return;
  }
  uint16_t zoneTempRaw =  (uint16_t) serialBuffer[1] | ((uint16_t) (serialBuffer[2] & 0b1) << 8);
  double zoneTemp = zoneTempRaw * 0.1;
  // Zone stats ? buffer[offset-2] & 0b11111110
  double zoneMinTemp = serialBuffer[3] / 2.0;
  double zoneSetTemp = (serialBuffer[4] & 0b00111111) / 2.0;
  double zoneMaxTemp = (serialBuffer[5] & 0b00111111) / 2.0;

  bool zoneOn = (serialBuffer[2] & 0b01000000) == 0b01000000;
  bool fanMode = (serialBuffer[4] & 0x80) == 0x80;
  bool zoneActive = (serialBuffer[5] & 0x80) == 0x80;
  
  uint8_t checkByte = serialBuffer[2] - (serialBuffer[2] << 1) - serialBuffer[5] - serialBuffer[4] - serialBuffer[3] - serialBuffer[1] - serialBuffer[0] - 1;
  // buffer[6] no idea

  // Print Details

  Serial.print("M: ");
  
  // Hex
  for (int i=0; i<count; i++) {
    printByte(serialBuffer[i]);
  }
  
  Serial.print("\t");

  // // Binary
  for (int i=0; i<count; i++) {
    printBinaryByte(serialBuffer[i]);
    Serial.print("  ");
  }

  // Decoded
  Serial.print("\tZone ");
  Serial.print(zone);

  Serial.print(", Temp: ");
  Serial.print(zoneTemp);

  Serial.print(", SetTemp: ");
  Serial.print(zoneSetTemp);

  Serial.print(", MinSetTemp: ");
  Serial.print(zoneMinTemp);

  Serial.print(", MaxTemp: ");
  Serial.print(zoneMaxTemp);

  Serial.print(", On: ");
  Serial.print(zoneOn ? "YES" : "NO");

  Serial.print(", Active: ");
  Serial.print(zoneActive ? "YES" : "NO");

  Serial.print(", FanMode: ");
  Serial.print(fanMode ? "YES" : "NO");

  Serial.print(", ???: ");

   printBinaryByte(checkByte);

  Serial.println();
}

void decodeZoneToMaster() {
    // Try with the new struct
  ActronZoneToMasterMessage zoneMessage = ActronZoneToMasterMessage();
  zoneMessage.parse(serialBuffer);

  if (zoneMessage.zone != 3) {
    return;
  }

  // Check if changed
  int count = 5;
  bool same = true;
  for (int i=0; i<count; i++) {
    same = same && previousZonePacket[i] == serialBuffer[i];
    if (!same) {
      previousZonePacket[i] = serialBuffer[i];
    }
  }

  if (same) {
    // Only show differences
    return;
  }

    uint8_t newData[5];
    zoneMessage.generate(newData);

    zoneMessage.printToSerial();

    // Binary
    printBinaryBytes(serialBuffer, count);
    Serial.println();
    printBinaryBytes(newData, count);
    Serial.println();
    Serial.println();
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
      // for (int j=0; j<serialBufferIndex; j++) {
      //   printByte(serialBuffer[j]);
      // }
      // Serial.print(" (");
      // Serial.print(serialLastReceivedTime);
      // Serial.println("ms)");
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

