#include "Utilities.h"

void printByte(uint8_t byte) {
    if (byte <= 0x0f) {
        Serial.print("0");  
    }
    Serial.print(byte, HEX);
    Serial.print(" ");
}

void printBinaryByte(uint8_t byte) {
    for(int i = 7; i>=4;i--) {
        Serial.print((char)('0' + ((byte>>i)&1)));
    }
    Serial.print(" ");
    for(int i = 3; i>=0;i--) {
        Serial.print((char)('0' + ((byte>>i)&1)));
    }
}

void printBytes(uint8_t bytes[], uint8_t length) {
    // Hex
    for (int i=0; i<length; i++) {
        printByte(bytes[i]);
    }
}

void printBinaryBytes(uint8_t bytes[], uint8_t length) {
    for (int i=0; i<length; i++) {
        printBinaryByte(bytes[i]);
        Serial.print("  ");
    }
}