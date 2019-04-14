#include <Arduino.h>
#include <StrobeReceiver.h>

volatile uint8_t _frameType, _frameParity, _frameOneCount, _frameChangeCount, _tmrStartOffset, _frameBit, _frameBitOld, _rxPin, _frameError, _rxReady;
volatile uint16_t _tmrDivisor;
volatile uint32_t _rxBuffer;
volatile void (*_errorHandler)();
volatile void (*_receiveHandler)(); 
void COMPA_ISR();
void PIN_ISR();


StrobeReceiver::StrobeReceiver(uint8_t rxPin){
	_tmrStartOffset = 1;
	_rxReady = 0;
	//Configure the receive pin
	_rxPin = rxPin;
}

uint8_t StrobeReceiver::begin(uint16_t rxSpeed){
	//Validate receive speed
	if(rxSpeed == 0) return 0;
	//Assign event handlers
	_errorHandler = 0;
	_receiveHandler = 0;
	//Calculate the divisor based on receive speed
	//Clock is divided by 256 -> 62.5kHz
	_tmrDivisor = 62500/(2*rxSpeed);
	//Disable interrupts
	noInterrupts();  
	//Configure timer registers
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	//Save the divisor into output compare register
	//Only half - first pass will offset the timer to be in center of a pulse
	//OCR1A = (_tmrDivisor/2);
	//Setup timer for output compare
	TCCR1B |= (1 << WGM12);
	//Enable output compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	//Enable all interrupts
	interrupts();
	//Setup pin as input
	pinMode(_rxPin, INPUT);
	//Attach interrupt to the pin
	attachInterrupt(digitalPinToInterrupt(_rxPin), PIN_ISR, RISING);
	return 1;
}

uint8_t StrobeReceiver::begin(uint16_t rxSpeed, void (*receiveHandler)()){
	//Validate receive speed
	if(rxSpeed == 0) return 0;
	//Assign event handlers
	_errorHandler = 0;
	_receiveHandler = receiveHandler;
	//Calculate the divisor based on receive speed
	//Clock is divided by 256 -> 62.5kHz
	_tmrDivisor = 62500/(2*rxSpeed);
	//Disable interrupts
	noInterrupts();  
	//Configure timer registers
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	//Save the divisor into output compare register
	//Only half - first pass will offset the timer to be in center of a pulse
	//OCR1A = (_tmrDivisor/2);
	//Setup timer for output compare
	TCCR1B |= (1 << WGM12);
	//Enable output compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	//Enable all interrupts
	interrupts();
	//Setup pin as input
	pinMode(_rxPin, INPUT);
	//Attach interrupt to the pin
	attachInterrupt(digitalPinToInterrupt(_rxPin), PIN_ISR, RISING);
	return 1;
}

uint8_t StrobeReceiver::begin(uint16_t rxSpeed, void (*receiveHandler)(), void (*errorHandler)()){
	//Validate receive speed
	if(rxSpeed == 0) return 0;
	//Assign event handlers
	_errorHandler = errorHandler;
	_receiveHandler = receiveHandler;
	//Calculate the divisor based on receive speed
	//Clock is divided by 256 -> 62.5kHz
	_tmrDivisor = 62500/(2*rxSpeed);
	//Disable interrupts
	noInterrupts();  
	//Configure timer registers
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	//Setup timer for output compare
	TCCR1B |= (1 << WGM12);
	//Enable output compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	//Enable all interrupts
	interrupts();
	//Setup pin as input
	pinMode(_rxPin, INPUT);
	//Attach interrupt to the pin
	attachInterrupt(digitalPinToInterrupt(_rxPin), PIN_ISR, RISING);
	return 1;
}

void StrobeReceiver::readData(uint8_t *frameType, uint32_t *rxBuffer){
	//Return received data
	*frameType = _frameType;
	*rxBuffer = _rxBuffer;

	_rxReady = 0;
}

uint8_t StrobeReceiver::receivedReady(){
 	return _rxReady;
}

void PIN_ISR(){
	//Clear buffer, frame type, frame parity and receive complete
	_rxBuffer = 0;
	_frameType = 0;
	_frameParity = 0;
	_rxReady = 0;
	//Prepare the timer offset into OCR1A
	OCR1A = (_tmrDivisor/2);
	//Detach the interrupt
	detachInterrupt(digitalPinToInterrupt(_rxPin));
	//Start the timer
	TCCR1B |= (1 << CS12);
}

void COMPA_ISR(){
	//If the timer was offset aleready
	if(_tmrStartOffset){
		//Put the full divisor into OCR1A
		OCR1A = _tmrDivisor;
		_tmrStartOffset = 0;
	}

	//Add one to the "pin change counter"
	_frameChangeCount++;
	//Save the old and new bits
	_frameBitOld = _frameBit;
	_frameBit = digitalRead(_rxPin);

	//Every odd bit change
	if((_frameChangeCount % 2) == 0){
		//If the change was from 1 to 0 -> Manchester coded 1
		if(_frameBitOld && !_frameBit){      
			if(_frameChangeCount <= 6){
				//First 3 bits are frame type (they get shifted into frame type)
				//Shift one into the byte
				_frameType >>=1;
				_frameType |= 0x80;
				_frameOneCount++;
			}else if(_frameChangeCount > 6 && _frameChangeCount <= 70){
				//Than 32bits of data get shifted into the data buffer
				//Shift one into the byte
				_rxBuffer >>= 1;
				_rxBuffer |= 0x80000000;
				_frameOneCount++;
			}else if(_frameChangeCount > 70){
				//Last bit gets shifted into parity
				//Shift one into the byte
				_frameParity >>=1;
				_frameParity |= 0x80;
			}
		//Else if the change was from 0 to 1 -> Manchester coded 0
		}else if(!_frameBitOld && _frameBit){
			if(_frameChangeCount <= 6){
				//First 3 bits are frame type (they get shifted into frame type)
				//Shift zero into the byte
				_frameType >>=1;
			}else if(_frameChangeCount > 6 && _frameChangeCount <= 70){
				//Than 32bits of data get shifted into the data buffer
				//Shift zero into the byte
				_rxBuffer >>= 1;
			}else if(_frameChangeCount > 70){
				//Last bit gets shifted into parity
				//Shift zero into the byte
				_frameParity >>=1;
			}
		//If there was neither high to low, nor low to high change where it should have been, its a frame error
		}else if(_frameChangeCount <= 72){
			//Turn off the timer
			TCCR1B &= ~(1<< CS12);
			//Clear bit change count
			_frameChangeCount = 0;
			//Clear the number of received ones (for parity purposes)
			_frameOneCount = 0;
			//Prepare the timer for offset
			_tmrStartOffset = 1; 
			//Signalize a frame error (type 1)
			_frameError = 1;
			
			//If error handler was provided, execute it    
			if(_errorHandler != 0) (*_errorHandler)();
			  
			//Clear pin interrupt flag to prevent interrupt from false fireing
			if(digitalPinToInterrupt(_rxPin) == 0) EIFR |= bit(INTF0);
			else if(digitalPinToInterrupt(_rxPin) == 1) EIFR |= bit(INTF1);
			//Attach the interrupt
			attachInterrupt(digitalPinToInterrupt(_rxPin), PIN_ISR, RISING);  
		}
	}

	//If received all 36 bits
	if(_frameChangeCount == 72){
		//Align frame type and parity to be propertly represented as numbers
		_frameType >>= 5;
		_frameParity >>= 7;
		 
		//If receive handler was provided, execute it   
		if(_receiveHandler != 0) (*_receiveHandler)();
		   
		//Check the parity and signalize a frame error type 2 - parity error
		if(!(_frameOneCount % 2) && !_frameParity || _frameParity && (_frameOneCount % 2)){
			_frameError = 0;
		}else{
			_frameError = 2;
			//If error handler was provided, execute it    
			if(_errorHandler != 0) (*_errorHandler)();
		}
	  
	}

	//Wait four more cycles to prevent interrupt from false fireing
	if(_frameChangeCount == 76){
		//Turn off the timer
		TCCR1B &= ~(1<< CS12);
		//Clear bit change count
		_frameChangeCount = 0;
		//Clear the number of received ones (for parity purposes)
		_frameOneCount = 0;
		//Prepare the timer for offset
		_tmrStartOffset = 1; 
		//Clear the frame error
		_frameError = 0;
		//Signalize complete receive
		_rxReady = 1;
		//Clear pin interrupt flag to prevent interrupt from false fireing
		if(digitalPinToInterrupt(_rxPin) == 0) EIFR |= bit(INTF0);
		else if(digitalPinToInterrupt(_rxPin) == 1) EIFR |= bit(INTF1);
		//Attach the interrupt
		attachInterrupt(digitalPinToInterrupt(_rxPin), PIN_ISR, RISING);  
	}
}

ISR(TIMER1_COMPA_vect){
	COMPA_ISR();
}