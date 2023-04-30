#include <Arduino.h>

namespace Actron485 {

static Stream &printOut = Serial;

void printByte(uint8_t byte);
void printBinaryByte(uint8_t byte);
void printBytes(uint8_t bytes[], uint8_t length);
void printBinaryBytes(uint8_t bytes[], uint8_t length);
bool bytesEqual(uint8_t lhs[], uint8_t rhs[], uint8_t length);

}