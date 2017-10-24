afsk-ax25
=========

Support files for a post about FSK in http://electronicayciencia.blogspot.com/



Files
-----

- nrzidec.c: Read a GPIO pin and writes 1's and 0's into stdout following a NRZI line code.

- nrzienc.c: Read 1's and 0's from stdin and toggles a GPIO pin following NRZI line code. Useful to test nrzidec.

- decode_ax25.c: Read 1's and 0's from stdin and tries to decode then as a AX25 packet.

- test_data: Directory with some raw binary AX25 packets to test decode_ax25.

- test_snd: Directory with some synthetic and real recorded frames.

- misc: Auxiliary files.


Build
-----

You will need a Raspberry and WiringPi library to compile nrzienc and nrzidec.
Copy files to Rapsberry and run 

	make


Usage
-----

In one terminal:

	./nrzidec | ./decode_ax25


In another terminal, just for testing you can do:

	./nrzienc < test_data/UI.dat

The intention is to use a hardware modem, like one based on TCM3105.


