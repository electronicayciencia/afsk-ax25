all: decode_ax25 nrzidec nrzienc

decode_ax25: decode_ax25.c
	gcc -lwiringPi -o decode_ax25 decode_ax25.c 

nrzienc: nrzienc.c
	gcc -lwiringPi -o nrzienc nrzienc.c

nrzidec: nrzidec.c
	gcc -lm -lwiringPi -o nrzidec nrzidec.c

clean: 
	rm -f decode_ax25 nrzidec nrzienc
