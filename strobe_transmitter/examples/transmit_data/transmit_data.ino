#include <StrobeTransmitter.h>

//Initialize transmitter on pin 4  
StrobeTransmitter transmitter(4);

void setup() {
  //Setup the transmitter at speed of 5bps (bits per second)
  transmitter.begin(5);
}

void loop() {
  //If the system is ready to transmit
  if(transmitter.transmitReady()){
    //Transmit a message
    //Packet type - 7
    //Message - 32b - 0x12345678
    transmitter.transmit(7, 0x12345678);
  }
  
}
