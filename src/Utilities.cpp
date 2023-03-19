#include "Utilities.h"

void printByte(int byte) {
    if (byte <= 0x0f) {
        Serial.print("0");  
    }
    Serial.print(byte, HEX);
    Serial.print(" ");
}

void printBinaryByte(int byte) {
    for(int i = 7; i>=4;i--) {
        Serial.print((char)('0' + ((byte>>i)&1)));
    }
    Serial.print(" ");
    for(int i = 3; i>=0;i--) {
        Serial.print((char)('0' + ((byte>>i)&1)));
    }
}