#include <Arduino.h>
#include <StrobeTransmitter.h>

volatile uint8_t _clkState, _frameType, _frameParity, _frameOneCount, _frameBitCount, _txOutBit, _txPin, _txReady;
volatile uint16_t _tmrDivisor;
volatile uint32_t _txBuffer;	
void COMPA_ISR();

StrobeTransmitter::StrobeTransmitter(uint8_t txPin){
	_clkState = 0;
	_frameOneCount = 0;
	_frameBitCount = 1;
	_txReady = 1;
	//Configure the transmit pin
	_txPin = txPin;
	pinMode(_txPin, OUTPUT);
	digitalWrite(_txPin, 0);
}

uint8_t StrobeTransmitter::begin(uint16_t txSpeed){
	//Validate transmit speed
	if(txSpeed == 0) return 0;
	//Calculate the divisor based on transmit speed
	//Clock is divided by 256 -> 62.5kHz
	_tmrDivisor = 62500/(2*txSpeed);
	//Disable interrupts
	noInterrupts();  
	//Configure timer registers
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	//Save the divisor into output compare register
	OCR1A = _tmrDivisor;
	//Setup timer for output compare
	TCCR1B |= (1 << WGM12);
	//Enable output compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	//Enable all interrupts
	interrupts();
	return 1;
}

void StrobeTransmitter::end(){
	noInterrupts();  
	//Set timer registers back to default
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	OCR1A = 0;
	TCCR1B = 0;
	TIMSK1 = 0;
	interrupts();
	//Set the pin back to default
	pinMode(_txPin, INPUT);
}

uint8_t StrobeTransmitter::transmit(uint8_t frameType, uint32_t txBuffer){
	//Validate the frame type
	if(frameType > 7) return 0;
	//Reset frame parity
	_frameParity = 0;
	//Signalize transmit in progress
	_txReady = 0;
	//Setup frame type and buffer
	_frameType = frameType;
	_txBuffer = txBuffer;
	  
	//Shift out the first bit (part of frame type)
	_txOutBit = (_frameType & 0x0001);
	digitalWrite(_txPin, _txOutBit^_clkState);
	//If the bit was 1, add to counter (for parity purposes)
	if(_txOutBit) _frameOneCount++;
	//Toggle the clock state
	_clkState = !_clkState;
	//Start Timer1 with 256 prescaler
	TCCR1B |= (1 << CS12);
 	return 1;
}

uint8_t StrobeTransmitter::transmitReady(){
	return _txReady;
}

void COMPA_ISR(){
	if(_frameBitCount < 4){
		//First 3 bits to shift out are packet type
		//Mask out first bit of frame type
		_txOutBit = (_frameType & 0x0001);
	}else if(_frameBitCount >= 4 && _frameBitCount < 36){
		//Than 32 bits of data are shifted out
		//Mask out first bit transmit buffer
		_txOutBit = (_txBuffer & 0x0001);
	}else if(_frameBitCount >= 36){
		//Last the parity is shifted out
		//Compute parity bit based on count of ones in frame
		if(((_frameOneCount/2) % 2)) _frameParity = 1;
		//Send the bit for transmission
		_txOutBit = _frameParity;  
	}

	//Shift out the bit
	if(_frameBitCount < 38) digitalWrite(_txPin, _txOutBit^_clkState);

	//If the bit was one, add 1 to the counter (for parity purposes)
	if(_txOutBit) _frameOneCount++;

	//On positive clock state, shift the buffers
	if(_clkState){
		if(_frameBitCount < 4){
			//If sending out frame type, shift frame type buffer
			_frameType >>= 1;
		}else if(_frameBitCount >= 4 && _frameBitCount < 36){
			//If sending out data, shift the transmit buffer
			_txBuffer >>= 1;
		}else if(_frameBitCount >= 36){
			//If sending out parity, shift parity
			_frameParity >>= 1;
		}

		//Add 1 to the "shifted out bits counter"
		_frameBitCount++;

		//If all of the bits are shifted out
		if(_frameBitCount >= 38){
			//Set the pin low
			digitalWrite(_txPin, 0);
		}

		//Wait two more cycles until the system gets ready for a new transmit
		if(_frameBitCount >= 40){
			//Stop the timer
			TCCR1B &= 0xf8;
			//Set bit count back to 1
			_frameBitCount = 1;
			//Set one count to 0
			_frameOneCount = 0;
			//Signalize transmit complete
			_txReady = 1;
		}
	}
	//Toggle the clock state
	_clkState = !_clkState;
}

ISR(TIMER1_COMPA_vect){
	COMPA_ISR();
}