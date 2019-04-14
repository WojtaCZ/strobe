#ifndef STROBE
#define STROBE

#include <Arduino.h>

class StrobeTransmitter{
	public:
		StrobeTransmitter(uint8_t txPin);
		uint8_t begin(uint16_t txSpeed);
		void end();
		uint8_t transmit(uint8_t frameType, uint32_t txBuffer);
		uint8_t transmitReady();
};

#endif