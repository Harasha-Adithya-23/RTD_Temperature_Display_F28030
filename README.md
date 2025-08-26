# RTD_Temperature_Display_F28030
Embedded Systems Project using TI F28030. Includes TM1637 7-segment display (I²C-like via bit-banging), UART communication, and MAX31865 RTD temperature sensor via SPI.
*************************************************************************************************************************************************
Implemented a 3-digit 7-segment display using TM1637 by simulating an I²C-like protocol via GPIO bit-banging.

Practiced UART communication: configured TX/RX, FIFO buffers, and interrupts; displayed received data on the 7-segment display; verified signals using a digital oscilloscope.

Interfaced MAX31865 with a 3-pin RTD sensor via SPI protocol; configured registers to read and process temperature data.
