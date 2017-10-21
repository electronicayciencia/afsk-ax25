/* Codifica un stream de 0s y 1s como NRZI (NRZS) en un puerto GPIO 
 *
 * echo '01001101...' | nrzienc
 *
 * gcc -l wiringPi -o nrzienc nrzienc.c
 *
 * Electrónica y Ciencia. http://electronicayciencia.blogspot.com.es/
 *
 * Reinoso G.  12/10/2017
 *
 */

#include <stdio.h>
#include <wiringPi.h>

#define bauds 1200
#define outpin 25


int main(void) {

	if (wiringPiSetup () == -1)
		return 1;

	fprintf(stderr, 
			"Encoding input as NRZI(S). Pin: %d, speed: %d bauds.\n", 
			outpin, bauds);

	piHiPri(99);
	pinMode(outpin, OUTPUT);

	int s = 0; // output status
	while (1) {
		unsigned char c = getchar();

		// 0: toogle, 1: maintain
		if (c == '0')
			s = 1 - s;

		// Well, not -1 but almost
		if (c == 255)
			break;

		//printf("%c", s == 1 ? 'm' : 's');

		digitalWrite(outpin, s);
		delayMicrosecondsHard((int)1.0e6/bauds);
	};

}
