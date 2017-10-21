/* DeCodifica señales en GPIO como un stream de 0s y 1s NRZS 
 *
 * nrzienc | decode_ax25
 *
 * gcc -lm -lwiringPi -o nrzidec nrzidec.c
 *
 * Electrónica y Ciencia. http://electronicayciencia.blogspot.com.es/
 *
 * Reinoso G.  13/10/2017
 *
 */

#include <stdio.h>
#include <wiringPi.h>
#include <sys/time.h>
#include <math.h>

#define bauds 1200
#define inpin 24

struct timeval tlast;

static void isr_srv(void) {
	struct timeval tnow;
	gettimeofday(&tnow, NULL);

	unsigned int tdiff = 1e6 * (tnow.tv_sec - tlast.tv_sec)
	                         +  tnow.tv_usec - tlast.tv_usec;

	float symbs = tdiff / (1e6/bauds);

	//printf("Took %uus (%3.1f symbs)\n", tdiff, symbs);

	if (symbs > 0.5 && symbs < 8) {
		int i;
		for (i = round(symbs) - 1; i > 0; i--)
			putchar('1');

		putchar('0');
	}
	else if (symbs >= 8) {
		printf("11111110"); // abort
	}


	tlast = tnow;
}


int main(void) {
	char gpiocmd[32];

	if (wiringPiSetup () == -1)
		return 1;

	fprintf(stderr, 
			"Decoding pin %d as NRZI(S) data. Speed: %d bauds.\n", 
			inpin, bauds);

	piHiPri(99);
	pinMode(inpin, INPUT);

	setbuf(stdout, NULL);
	gettimeofday(&tlast, NULL);
	wiringPiISR(inpin, INT_EDGE_BOTH, &isr_srv);

	while (1) {
		delayMicroseconds(1e5);
	};
}
