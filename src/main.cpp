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


uint8_t previousZonePacket[7];

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
    ActronMasterToZoneMessage message = ActronMasterToZoneMessage();
    message.parse(serialBuffer);

    uint8_t newData[7];
    message.generate(newData);

    // Check if changed
    bool print = false;
    if (message.zone == 3) {
        bool same = true;
        for (int i=0; i<7; i++) {
          same = same && previousZonePacket[i] == serialBuffer[i];
          if (!same) {
            previousZonePacket[i] = serialBuffer[i];
          }
        }
        print = !same;
    }


    if (!bytesEqual(serialBuffer, newData, 7)) {
      Serial.println("Error! Byte Miss Match!");
      message.printToSerial();
      Serial.println();

      // Binary
      printBinaryBytes(serialBuffer, 7);
      Serial.println();
      printBinaryBytes(newData, 7);
      Serial.println();
      Serial.println();

    } else if (print) {
      message.printToSerial();
      Serial.println();
      printBinaryBytes(newData, 7);
      Serial.println();
      Serial.println();

    }
}

void decodeZoneToMaster() {
    ActronZoneToMasterMessage message = ActronZoneToMasterMessage();
    message.parse(serialBuffer);

    uint8_t newData[5];
    message.generate(newData);

    if (!bytesEqual(serialBuffer, newData, 5)) {
      Serial.println("Error! Byte Miss Match!");
      message.printToSerial();
      Serial.println();

      // Binary
      printBinaryBytes(serialBuffer, 5);
      Serial.println();
      printBinaryBytes(newData, 5);
      Serial.println();
      Serial.println();
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

