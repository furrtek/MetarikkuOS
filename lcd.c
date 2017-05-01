#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"
#include "metarikku_os.h"
#include "io.h"

#define BIGFONTSTART 2

const uint8_t schars[] PROGMEM = {
	0x1E,0x13,0x11,0x11,0x11,0x1F,0x00,0x00,	// Card present
	0x1E,0x13,0x11,0x00,0x0A,0x04,0x0A,0x00,	// Card absent
	0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,	// Big font...
	0x1F,0x1F,0x03,0x03,0x03,0x03,0x1F,0x1F,
	0x1F,0x1F,0x18,0x18,0x18,0x18,0x1F,0x1F,
	0x1B,0x1B,0x1B,0x1B,0x1B,0x1B,0x1F,0x1F,
	0x1F,0x1F,0x1B,0x1B,0x1B,0x1B,0x1F,0x1F,
	0x1F,0x1F,0x1B,0x1B,0x1B,0x1B,0x1B,0x1B
};

uint8_t textp_yes[] PROGMEM =	"YES";
uint8_t textp_no[] PROGMEM =	"NO";

uint8_t textp_clear[] PROGMEM = "                ";
uint8_t textm_clear[] PROGMEM = "              ";

uint8_t bigfont[] PROGMEM = {
	5+BIGFONTSTART,3+BIGFONTSTART,
	0+BIGFONTSTART,0+BIGFONTSTART,
	1+BIGFONTSTART,2+BIGFONTSTART,
	1+BIGFONTSTART,1+BIGFONTSTART,
	3+BIGFONTSTART,0+BIGFONTSTART,
	2+BIGFONTSTART,1+BIGFONTSTART,
	2+BIGFONTSTART,3+BIGFONTSTART,
	1+BIGFONTSTART,0+BIGFONTSTART,
	4+BIGFONTSTART,3+BIGFONTSTART,
	4+BIGFONTSTART,1+BIGFONTSTART
};

void printvalmax(uint8_t v,uint8_t px) {
	uint8_t t;

	if (parammax) {
		//Units
		t = v%10;
		LCD_XY(px+2,0);
		wrtc(pgm_read_byte(&bigfont[t*2]));
		LCD_XY(px+2,1);
		wrtc(pgm_read_byte(&bigfont[(t*2)+1]));

		//Tens
		if (v > 9) {
			t = (v/10)%10;
			LCD_XY(px+1,0);
			wrtc(pgm_read_byte(&bigfont[t*2]));
			LCD_XY(px+1,1);
			wrtc(pgm_read_byte(&bigfont[(t*2)+1]));
		} else {
			LCD_XY(px+1,0);
			wrtc(' ');
			LCD_XY(px+1,1);
			wrtc(' ');
		}

		//Cents
		if (v > 99) {
			t = (v/100);
			LCD_XY(px,0);
			wrtc(pgm_read_byte(&bigfont[t*2]));
			LCD_XY(px,1);
			wrtc(pgm_read_byte(&bigfont[(t*2)+1]));
		} else {
			LCD_XY(px,0);
			wrtc(' ');
			LCD_XY(px,1);
			wrtc(' ');
		}
	} else {
		LCD_XY(px,0);
		wrtr("   ");
		LCD_XY(px,1);
		wrtr("   ");
	}
}

void LCD_Update(void) {
	uint8_t x;

	LCD_XY(0,0);
	wrt(textp_clear);

	if ((CurParam == CH) || (CurParam == PATCH)) {
		sprintf(message,"Ch%u",CurCh+1);
	} else if (CurParam == OP) {
		sprintf(message,"Ch%u%cOp%u",CurCh+1,0x7E,CurOp+1);
	} else {
		sprintf(message,"       ");
	}

	if (CurParam == PATCH) {
		LCD_XY(13,0);
	} else {
		LCD_XY(2,0);
	}
	wrtr(message);
	
	LCD_XY(0,0);
	if (cardPresent)
		wrtc(0);
	else
		wrtc(1);

	LCD_XY(0,1);
	wrt(textp_clear);
	if (CurChoiceType == TEXT) {
		if (CurParam == OP)
			LCD_XY(6,0);
		else
			LCD_XY(2,0);
	} else {
		LCD_XY(0,1);
	}
	wrt(txtbptr);

	//Bigfont print current/max
	if (CurChoiceType == NUMERIC) {
		printvalmax(parammax,13);
		x = 12;
		if (parammax <= 99) {
			x++;
			if (parammax <= 9) x++;
		}
		printvalmax((*paramptr),x-3);
		LCD_XY(x,1);
		wrtc('/');
	} else if (CurChoiceType == TEXT) {
		LCD_XY(2,1);
		wrt(textm_clear);
		LCD_XY(2,1);
		wrt((PGM_P)pgm_read_word(txtmptr + 2*(*paramptr)));
	} else if (CurChoiceType == YESNO) {
		LCD_XY(13,1);
		wrt(textm_clear);
		LCD_XY(13,1);
		if (*paramptr)
			wrt(textp_yes);
		else
			wrt(textp_no);
	}
}

void wrt(const uint8_t *FlashLoc) {
	uint8_t i,chr;
	PORTD |= _BV(LCD_RS);
	for(i=0;(chr = (uint8_t)pgm_read_byte(&FlashLoc[i]));i++) {
		com(chr);
	}
	PORTD &= ~_BV(LCD_RS);
}

// Passe 4 bits au LCD
void clockLCD(uint8_t arg) {
	PORTB = (PORTB & 0b11110000) | arg;
	_delay_us(50);
	PORTA |= _BV(PA1);
	_delay_us(50);
	PORTA &= ~_BV(PA1);
	_delay_us(50);
}

// Passe 8 bits au LCD
void com(uint8_t command) {
	clockLCD(command >> 4);
	_delay_us(500);
	clockLCD(command & 0x0F);
	_delay_us(500);
}

// Ecrit le string en RAM
void wrtr(char *cbuf) {
	uint8_t i, chr;
	PORTD |= _BV(LCD_RS);
	for(i=0;(chr = cbuf[i]);i++) {
		com(chr);
	}
	PORTD &= ~_BV(LCD_RS);
}

// Ecrit le caractere
void wrtc(char chr) {
	PORTD |= _BV(LCD_RS);
	com(chr);
	PORTD &= ~_BV(LCD_RS);
}

// Conversion hexa/ascii
uint8_t hex(uint8_t num) {
	if (num > 9) {
		num += 0x37;
	} else {
		num += 0x30;
	}
	return num;
}

void LCD_XY(uint8_t x, uint8_t y) {
	uint8_t adr;
	switch (y) {
		case 0:
			adr = 0x80;
	  		break;
		case 1:
			adr = 0xC0;
	  		break;
		case 2:
			adr = 0x94;
	  		break;
		case 3:
			adr = 0xD4;
	  		break;
		default:
			adr = 0x80;
	}
	adr += x;
	com(adr);
}

void LCD_Clear(void) {
	com(0b00000001);		// Clear
	LCD_XY(0,0);
}

void LCD_Init(void) {
	uint8_t c;

	clockLCD(0b0011);
	_delay_ms(10);
	clockLCD(0b0011);
	_delay_ms(10);
	clockLCD(0b0011);
	_delay_ms(10);
	clockLCD(0b0010);
	_delay_ms(10);
	com(0b00101000);		// 2 lines
	com(0b00000001);		// Clear
	com(0b00000010);		// Home
	com(0b00001100);		// Screen on, cursor off, blink off

	com(0b01000000);	// CGRAM

	for (c=0;c<8*8;c++)
		wrtc((uint8_t)pgm_read_byte(&schars[c]));
}

