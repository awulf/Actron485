#include "Utilities.h"

namespace Actron485 {

void printByte(uint8_t byte) {
    if (byte <= 0x0f) {
        printOut.print("0");  
    }
    printOut.print(byte, HEX);
    printOut.print(" ");
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

bool bytesEqual(uint8_t lhs[], uint8_t rhs[], uint8_t length) {
    for (int i=0; i<length; i++) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

bool copyBytes(uint8_t source[], uint8_t destination[], uint8_t length) {
    bool same = true;
    for (int i=0; i<length; i++) {
        same = same && source[i] == destination[i];
        destination[i] = source[i];
    }
    return !same;
}

}