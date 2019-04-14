#ifndef STROBE
#define STROBE

#include <Arduino.h>

class StrobeReceiver{
	public:
		StrobeReceiver(uint8_t rxPin);
		uint8_t begin(uint16_t rxSpeed);
		uint8_t begin(uint16_t rxSpeed, void (*receiveHandler)());
		uint8_t begin(uint16_t rxSpeed, void (*receiveHandler)(), void (*errorHandler)());
		void end();
		uint8_t receivedReady();
		void readData(uint8_t *frameType, uint32_t *rxBuffer);
};

#endif