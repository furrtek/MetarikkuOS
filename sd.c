#include "sd.h"
#include <avr/io.h>
#include <util/delay.h>
#include "spi.h"
#include "io.h"
#include "metarikku_os.h"
#include <stdio.h>

#define nSS		PB4
#define MOSI	PB5
#define MISO	PB6
#define SCK		PB7

unsigned char SD_init(void) {
	uint8_t i,response,retry=0 ;

	SPCR |=  (1<<SPR1) | (1<<SPR0); // Slow SPI clock
	SPSR &= ~(1<<SPI2X);

	PORTB |=  (1<<nSS);			// Warmup card deasserted
	PORTB |=  (1<<MOSI);
	for(i=0; i<10; i++)
		SPI_tx(0xFF);			// 74 pulses minimum

	PORTB &= ~(1<<nSS);			// SD assert

	while((response = SD_sendCommand(GO_IDLE_STATE,0)) != 1) {
		retry++;
		if (retry == 32) {
			//sprintf(message,"SD Idle fail, response = %x",response);
			//SerialWrite(message,1,ANSI_RED);
			return 0;
	    }
	    _delay_ms(5);
	}

	for (i=0;i<200;i++)			// Startup SD
		SPI_rx();

	retry = 0;

	SD_sendCommand(37,0);
	while (SD_sendCommand(SEND_OP_COND,0)) {
		retry++;
		if (!retry) return(0);
		SD_sendCommand(37,0); 
	}

	SD_sendCommand(SET_BLOCK_LEN,512);

	SPCR &= ~((1<<SPR1) | (1<<SPR0));	// Fast SPI clock
	SPSR |=  (1<<SPI2X);

	return 1;
}

unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg) {
	unsigned char response;
	uint16_t retry=0;

  	PORTB &= ~(1<<nSS);

  	SPI_tx(0xFF);			// Dummy
	SPI_tx(cmd | 0x40);
	SPI_tx(arg>>24);
	SPI_tx(arg>>16);
	SPI_tx(arg>>8);
	SPI_tx(arg);
	SPI_tx(0x95);			// Good CRC for command 0 only !
  	SPI_tx(0xFF);			// Dummy

	while((response = SPI_rx()) == 0xFF) {
	   	if(retry++ == 0xFF) {
			//SerialWrite("SD command timeout",1,ANSI_RED);
	   		break;
		}
	}

	return response;
}


unsigned char SD_readSingleBlock(unsigned long startBlock) {
	unsigned char response;
	unsigned int i;
	uint16_t retry = 0;

	SD_CS_ASSERT;

	SDAccess = 1;

	response = SD_sendCommand(READ_SINGLE_BLOCK,startBlock<<9); // *512
	if (response) {
  		SD_CS_DEASSERT;
		SDAccess = 0;
		//sprintf(message,"SD read block command error: %X",response);
		//SerialWrite(message,1,ANSI_RED);
		return 0;
	}

	if (!(SPSR&(1<<SPI2X)))
		_delay_ms(25);

	while((response = SPI_rx()) != 0xFE) {
	  	if(retry++ > 0x1FF){
	  		SD_CS_DEASSERT;
			SDAccess = 0;
			//sprintf(message,"SD start block error: %X",response);
			//SerialWrite(message,1,ANSI_RED);
			return 0;
		}
		_delay_us(50);	// 100
	}

	for(i=0;i<512;i++)
		buffer[i] = SPI_rx();

	SPI_rx(); 	// Ignore CRC
	SPI_rx();

	SD_CS_DEASSERT;

	SPI_rx(); 	// Extra 8 clock pulses

	SDAccess = 0;

	return 1;
}

unsigned char SD_writeSingleBlock(unsigned long startBlock) {
	unsigned char response;
	unsigned int i, retry=0;

	SDAccess = 1;

	response = SD_sendCommand(WRITE_SINGLE_BLOCK, startBlock<<9); //write a Block command

 	if(response != 0x00) {
		SDAccess = 0;
		return response; //check for SD status: 0x00 - OK (No flags set)
	}

	SD_CS_ASSERT;

	SPI_tx(0xFE);     		//Send start block token 0xFE (0x11111110)

	for(i=0; i<512; i++)    //send 512 bytes data
  		SPI_tx(buffer[i]);

	SPI_tx(0xff);		    //transmit dummy CRC (16-bit), CRC is ignored here
	SPI_tx(0xff);

	response = SPI_rx();

	if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
	{
  		SD_CS_DEASSERT;
		SDAccess = 0;
  		return response;
	}

	while(!SPI_rx()) {		//wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe) {
			SDAccess = 0;
			SD_CS_DEASSERT;
			return 1;
		}
	}

	SD_CS_DEASSERT;
	SPI_tx(0xff);   		//just spend 8 clock cycle delay before reasserting the CS line
	SD_CS_ASSERT;         	//re-asserting the CS line to verify if card is still busy

	while(!SPI_rx()) {		//wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe) {
			SD_CS_DEASSERT;
			SDAccess = 0;
			return 1;
		}
	}

	SD_CS_DEASSERT;
	SDAccess = 0;

	return 0;
}

