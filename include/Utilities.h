#include <Arduino.h>

namespace Actron485 {

static Stream *printOut = NULL;

void printByte(uint8_t byte);
void printBinaryByte(uint8_t byte);
void printBytes(uint8_t bytes[], uint8_t length);
void printBinaryBytes(uint8_t bytes[], uint8_t length);
bool bytesEqual(uint8_t lhs[], uint8_t rhs[], uint8_t length);

/// @brief Copy bytes across, check if there was a change at the same time
/// @param source to copy from
/// @param destination  to copy to
/// @param length length of array
/// @return 
bool copyBytes(uint8_t source[], uint8_t destination[], uint8_t length);

}