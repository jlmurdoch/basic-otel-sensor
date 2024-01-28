#include "neopixelSPI.h"

// Send 24 bits - one color, MSB->LSB
void sendSPIColor(SPIClass *SPI1, unsigned int data) {
  SPI1->transfer((data >> 16) & 0xFF);
  SPI1->transfer((data >> 8) & 0xFF);
  SPI1->transfer(data & 0xFF);
}

// Turn Neopixel off
void setNeopixelOff(SPIClass *SPI1) {
  SPI1->beginTransaction(SPISettings(spiClock, SPI_MSBFIRST, SPI_MODE0));
  sendSPIColor(SPI1, 0b100100100100100100100100); 
  sendSPIColor(SPI1, 0b100100100100100100100100); 
  sendSPIColor(SPI1, 0b100100100100100100100100); 
  SPI1->endTransaction();
}

// Turn Neopixel on and white
void setNeopixelOn(SPIClass *SPI1) {
  SPI1->beginTransaction(SPISettings(spiClock, SPI_MSBFIRST, SPI_MODE0));
  sendSPIColor(SPI1, 0b100100100100110100110100); 
  sendSPIColor(SPI1, 0b100100100100110100110100); 
  sendSPIColor(SPI1, 0b100100100100110100110100); 
  SPI1->endTransaction();
} 