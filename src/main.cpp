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
    return;
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

  Serial.println();
}

// Given the raw encoded value converts to °C as master would interpet
double zoneTempFromMaster(int16_t rawValue) {
  double temp;
  double out = 0;
  if (rawValue < -58) {
    // Temp High (above 30.8°C)
    out = 0.00007124*pow(rawValue, 2) - 0.1052*rawValue+24.5;
    temp = round(out * 10) / 10.0;
  } else if (rawValue > 81) {
    // Low Temp  (below 16.9°C)
    out = (-0.00001457*pow(rawValue, 2) - 0.0988*rawValue+24.923);
    temp = round(out * 10) / 10.0;
  } else {
    temp = (250 - rawValue) * 0.1;
  }  
  return temp;
}

// Given the the temperature returns the encoded value for master controller
int16_t zoneTempToMaster(double temperature) {
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

void decodeZoneToMaster() {
  uint8_t zone = serialBuffer[0] & 0b00001111;
  double zoneSetTemp = serialBuffer[1] / 2.0;
  bool on = (serialBuffer[2] & 0b10000000) == 0b10000000;
  bool openMode = (serialBuffer[2] & 0b01000000) == 0b01000000;


  // Two last bits of byte[2] are the leading bits for the raw temperature value
  uint8_t leadingBites = serialBuffer[2] & 0b00000011;
  uint8_t negative = (leadingBites & 0b10) == 0b10;

  int16_t rawTempValue = (((negative ? 0b11111100 : 0x0) | leadingBites) << 8) | (uint16_t)serialBuffer[3];
  int16_t rawTemp = rawTempValue + (negative ? 512 : -512); // A 512 offset

  // uint8_t oldLeading = serialBuffer[2] & 0b00000001;
  // int16_t oldRawTemp = (((!prepend ? 0b11111111 : 0x0)) << 8) | (uint16_t)serialBuffer[3];
  // double oldTemp = (250 - oldRawTemp) * 0.1;

  double zoneTemp;
  double sensorTemp = (250 - rawTemp) * 0.1;
  if (rawTemp < -58) {
    // Temp High (above 30.8°C)
    double out = 0.00007124*pow(rawTemp, 2) - 0.1052*rawTemp+24.5;
    zoneTemp = round(out * 10) / 10.0;
  } else if (rawTemp > 81) {
    // Low Temp  (below 16.9°C)
    double out = (-0.00001457*pow(rawTemp, 2) - 0.0988*rawTemp+24.923);
    zoneTemp = round(out * 10) / 10.0;
  } else {
    zoneTemp = (250 - rawTemp) * 0.1;
  }  

  // Print Details
    if (zone != 3) {
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

  // printBinaryByte(rawTemp >> 8);
  // Serial.print(" ");
  // printBinaryByte(rawTemp);
  // Serial.print(", raw ");
  // Serial.print(rawTemp);
  // Serial.print(", old raw ");
  // Serial.print(oldRawTemp);
  // Serial.print(", old temp: ");
  // Serial.print(oldTemp);
  // Serial.println();

  Serial.print("Z: ");

  // Hex
  for (int i=0; i<count; i++) {
    printByte(serialBuffer[i]);
  }


  Serial.println();

  // Test Check Byte encoding
  const int x = -192;
  // int8_t extraByte = (on ? 0b10000000 : 0b0) | (openMode ? 0b01000000 : 0b0) | ((rawTempValue >> 8) & 0b00000001);
  //int8_t checkByte = ((int8_t)(sensorTemp * 10) - (int8_t)(zoneSetTemp * 2)) + x + extraByte + (openMode ? 128 : 0);
  int8_t checkByte = serialBuffer[2] - (serialBuffer[2] << 1) - serialBuffer[3] - serialBuffer[1] + 60;

  Serial.print("[");
  printBinaryByte(checkByte);
  Serial.print("]");
  Serial.print((int8_t)(serialBuffer[4]-checkByte));

    // Try with the new struct
  ActronZoneToMasterMessage zoneMessage = ActronZoneToMasterMessage();
  zoneMessage.parse(serialBuffer);

  uint8_t newData[5];
  zoneMessage.generate(newData);

  // Decoded
  Serial.println();
  Serial.print("Zone: ");
  Serial.print(zone);

  Serial.print(", Set Point: ");
  Serial.print(zoneSetTemp);

  Serial.print(", Temp: ");
  Serial.print(zoneTemp);

  Serial.print("(Pre:");
  Serial.print(sensorTemp);
  Serial.print(")");

  Serial.print(", Mode: ");
  Serial.print(on ? (openMode ? "Open" : "Auto") : "Off");

  Serial.println();

  zoneMessage.printToSerial();

    // Binary
  for (int i=0; i<count; i++) {
    printBinaryByte(serialBuffer[i]);
    Serial.print("  ");
  }
   Serial.println();
  for (int i=0; i<5; i++) {
    printBinaryByte(newData[i]);
    Serial.print("  ");
  }
  Serial.println();
   Serial.println();
}

bool zoneDecode() {
  // Zones Communication

  uint8_t byte = serialBuffer[0];
  if (serialBufferIndex == 7 && (byte & 0b11110000) == 0x80) {
    // Master -> Zone Wall
    //decodeMasterToZone();
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

