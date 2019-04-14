# Strobe

## Available functions
### Transmitter
| Function | Description | Returns |
|----------|-------------|---------|
| ```uint8_t begin(uint16_t speed);``` | Initializes the transmitter at provided speed (bits per second) | 0, 1 |
| ```void end();``` | Disables the transmitter | - |
| ```uint8_t transmit(uint8_t frameType, uint32_t txBuffer);``` | Trransmits the buffer with frame type | 0,1 |
| ```uint8_t transmitReady();``` | Signalizes whether another buffer can be transmitted | 0,1 |

### Receiver (only pins 2 and 3)
| Function | Description | Returns |
|----------|-------------|---------|
| ```uint8_t begin(uint16_t rxSpeed, *optional* void (*receiveHandler)(), *optional* void (*errorHandler)());``` | Initializes the receiver at provided speed (bits per second), two optional handle functions can be provided  | 0, 1 |
| ```void end();``` | Disables the receiver | - |
| ```void readData(uint8_t *frameType, uint32_t *rxBuffer);``` | Returns received data into provided variables | - |
| ```uint8_t receivedReady();``` | Signalizes whether new data is ready (gets cleared after readData() is called)  | 0,1 |


* Frame ty is in range of 0 to 7
