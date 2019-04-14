#include <StrobeReceiver.h>

//Initialize receiver on pin 2
//StrobeReceiver receiver(2);

void setup() {
  /*//Begin serial for debugging
  Serial.begin(9600);
  //Setup the receiver at 5bps (bits per second)
  receiver.begin(5);*/
}

void loop() {
  /*//If the system received some data
  if(receiver.receivedReady()){
    //Create storage variables
    uint8_t frameType;
    uint32_t rxBuffer;
    //Read the message
    receiver.readData(&frameType, &rxBuffer);
    //Write the data do serial
    Serial.print("------RECEIVED NEW FRAME------");
    Serial.println();
    Serial.print("Frame type: 0x");
    Serial.print(frameType, HEX);
    Serial.print("  |  Data: 0x");
    Serial.print(rxBuffer, HEX);
    Serial.println();
    Serial.println();
  }
  */
}
