#include <avr/io.h>

unsigned char SPI_tx(unsigned char data) {
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

unsigned char SPI_rx(void) {
	SPDR = 0xFF;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void SPI_Init(void) {
	SPCR = 0x52;	// Master, MSB first, SCK phase low, SCK idle low
	SPSR = 0x00;
}
