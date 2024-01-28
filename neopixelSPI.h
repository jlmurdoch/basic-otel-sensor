#include <SPI.h>

// Neopixel speed
static const int spiClock = 2400000;

// Send 24 bits - one color, MSB->LSB
void sendSPIColor(SPIClass *SPI1, unsigned int data);

// Turn Neopixel off
void setNeopixelOff(SPIClass *SPI1);

// Turn Neopixel on and white
void setNeopixelOn(SPIClass *SPI1);