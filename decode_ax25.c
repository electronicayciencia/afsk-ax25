/* This is a proof of concept to decode a stream of 1's and 0's 
 * into a AX.25 protocol for some cases. 
 *
 * It uses unbuffered I/O to read the streaming from stdin.
 *
 * Electronica y Ciencia. http://electronicayciencia.blogspot.com.es/
 *
 * Reinoso G.  09/10/2017
 */

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>

#define DEBUG 0


typedef struct {
	char call[7]; //ASCIIZ
	int  ssid;
	int  last;
	int  hbit;
} ax25_addr_t;


uint8_t data[2048];   // package readed
int data_length = 0;  // package length in bytes


/* Print 8 binary digits */
void p_binary(uint8_t i) {
	// Binary representation
	int j;
	for (j = 0; j <= 7; j++) {
		printf("%s", i & 0x80 ? "1" : "0");
		i <<= 1;
	}
}


/* Read 7 bytes of the message and fill adress strunture */
ax25_addr_t read_addr(char *msg) {
	ax25_addr_t addr = {0};

	// Callsign
	int i;
	for (i = 0; i <= 5; i ++) {
		unsigned char c = msg[i] >> 1;
		if (c != ' ') addr.call[i] = c;
	}

	// SSID, extension bit and hbit
	unsigned char c = msg[6];
	addr.hbit = (c >> 7) & 1;
	addr.ssid = (c >> 1) & 0b1111;
	addr.last = c & 1;

	return addr;
}


/* Calculate AX25 CRC and comapre to FCS. 
 * Yep, it's like magic */
int check_ax25_crc(uint8_t *msg, int len) {
	/* CRC Parameters are:
	 *  Order:         16 bits
	 *  Polynom:       1021
	 *  Initial value: 0xffff
	 *  Final XOR:     0xffff
	 *  Reverse data bytes:           yes
	 *  Reverse CRC before final XOR: yes
	 *
	 * Simpler equivalent:
	 *  Reverted polynom: 0x8408
	 *  Proceed rotating rigth
	 */
	
	int i;
	uint16_t crc  = 0xFFFF;
	uint16_t poly = 0x8408;

	for (i = 0; i < len-2; i++) {
		uint16_t b = msg[i];

		crc ^= b;

		int j;
		for (j = 0; j <= 7; j++) {

			if (crc & 0x0001)
				crc = (crc >> 1) ^ poly;
			else
				crc = crc >> 1;
		}
	}

	crc ^= 0xFFFF;

	// Assume FCS is in the last two bytes of the AX25 packet
	uint16_t fcs = (msg[len-1] << 8) + msg[len-2];
	if (DEBUG) printf("CRC: %x\tFCS: %x\n", crc, fcs);

	return (crc == fcs);
}


/* Decode and pretty print AX.25 frame. 
 * Currently only SABM and UI are supported. */
void decode_ax25(uint8_t *msg, int len) {
	int i;
	int pointer = 0;
	uint8_t c;

	if (DEBUG) {
		printf("Decoding %d bytes\n", len);
	
		for (i = 0; i < len; i++) {
			p_binary(msg[i]);
			printf("  %02x\n", msg[i]);
		}
	}
	 	
	// Check FCS
	if(!check_ax25_crc(msg, len)) {
		if (DEBUG) puts("Bad CRC");
		if (!DEBUG) return;
	}
	else {
		if (DEBUG) printf("VALID FRAME!!! --------------- \n\n");
	}

	// Print date and time
	char datetime[100];
	time_t now = time(0);
	strftime(datetime, 100, "%H:%M:%S", localtime(&now));
	printf("(%s) ", datetime);

	// Print all addresses
	ax25_addr_t addr;
	do {
		addr = read_addr(msg + pointer);
		pointer += 7;

		printf("%s-%d", addr.call, addr.ssid);
		if (addr.hbit) putchar('*');
		if (!addr.last) printf(" <- ");

	} while (!addr.last && pointer < len-(3+7));

	printf(": ");


	// Control
	c = msg[pointer++] & 0b1111; 

	// It's a SABM packet
	if (c == 0b1111) {
		printf("SABM\n");
	}

	// It's a UI packet
	else if (c == 0b0011) {
		printf("UI\n");
		// skip proto
		pointer ++;

		// do not print FCS (2 bytes) non printable chars
		for (i = pointer; i <= len-3; i++) {
			c = msg[i];
			putchar(c > 0x20 ? c : ' ');
		}
		printf("\n");
	}

	// It's not a SABM or UI packet  ;-)
	else {
		p_binary(c);
		printf(" - packet type not supported\n");
	}

	printf("\n");
}


/* Append new bits to the message in LSB first. 
 * Undo bit stuffing.
 * Group octets.
 * Detect start / stop flags.  */
void new_bit(char b) {
	static int bit = 0;
	static uint8_t last_octet = 0;
	static int last_ones = 0;
	static last_flag = 0;

	if (b == 1)
		last_ones++;

	// Undo bit stuffing
	if (b == 0 && last_ones == 5) {
		last_ones = 0;
		return;
	}

	// 7 ones in a row is not valid: reset
	if (last_ones > 6) {
		bit         = 0;
		data_length = 0;
		last_octet  = 0;
		last_ones   = 0;
		if (DEBUG) puts("Reset / Abort!");
	}

	// 6 ones an then 0, separation mark
	if (b == 0 && last_ones == 6) {
		if (data_length > 10) {
			decode_ax25(data, data_length);
		}
		else if (data_length > 0) {
			if (DEBUG) puts("Package too short");
		}

		bit         = 0;
		data_length = 0;
		last_ones   = 0;
		return;
	}

	if (b == 0)
		last_ones = 0;

	// Retrieve bits LSB first
	last_octet = (last_octet >> 1) + (b<<7);

	bit++;

	if (bit == 8) {
		data[data_length++] = last_octet;
		bit = 0;
	}

	// Prevent overflow
	if (data_length >= sizeof data) {
		data_length = 0;
	}
}



int main(void) {
	int i;
	uint8_t c;

	struct termios old_tio, new_tio;
	
	fprintf(stderr, 
			"Decoding input binary stream as AX.25 packets. Debug level: %d.\n", 
			DEBUG);


	// get stdin settings
	tcgetattr(STDIN_FILENO,&old_tio);
	new_tio=old_tio;

	// disable canonical mode (buffered i/o) and local echo
	new_tio.c_lflag &=(~ICANON & ~ECHO);
	tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);

	do {
		c=getchar();
		new_bit(c == '1' ? 1 : 0);
	} while(c!=255);
	
	// restore stdin settings
	tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);

	return 0;
}

