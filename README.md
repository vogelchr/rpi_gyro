% LSM330DLC Gyro connected to a Raspberry PI

Two LSM330DLC gyroscopes are connected to the Raspberry PI GPIO
connector, with address decoding done my a 74AC11138 3-to-8 decoder.

The connectors are wired as follows:

		         1 O O
		           : :
		           : :
		3v3 Power  O O n/c (GPIO24)
		SPI MOSI   O O n/c (Ground)
		SPI MISO   O O GPIO 25
		SPI SCLK   O O SPI \CE0
		Ground     O O SPI \CE1

The "main" SPI signals are connected directly to the LSM330DLC.
The chip-select signals are connected through a 74AC11138 with
the following pins connected :

	Raspberry Pin 74AC11138    LSM330DLC
	------------- ------------ -----------
	\CE1          Pin 13, "C"  -
	\CE0          Pin 14, "B"  -
	GPIO 25       Pin 15, "A"  -
	-             Pin 2, "Y2"  ...?...
	-             Pin 3, "Y3"  ...?...
	-             Pin 5, "Y4"  ...?...
	-             Pin 6, "Y5"  ...?...

Enable-pins are set to always enable the low-active decoder
outputs. Truth table for the 74AC11138:

	A    B    C      Active-low output of
	GPIO \CE0 \CE1   decoder:
	---- ---- ------ --------------------
	1    1    1      \Y7 (SPI idle, not connected...)
	0    1    0      \Y2
	1    1    0      \Y3
	0    0    1      \Y4 
	1    0    1      \Y5


