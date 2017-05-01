#include "avr/io.h"
#include "util/delay.h"
#include <avr/interrupt.h>
#include "metarikku_os.h"

uint8_t ringLight;
uint8_t blink = 0;

void IncIF(void) {
	PORTC |= _BV(PC3);	// Inc 4017
	_delay_us(2);
	PORTC &= ~_BV(PC3);
	_delay_us(3);
}

void ResetIF(void) {
	PORTC |= _BV(PC2);	// Reset 4017 counter
	_delay_us(5);
	PORTC &= ~_BV(PC2);
	_delay_us(5);
}

void IFH_Test() {
	if (ifstep == 0) {
		PORTA &= ~0b11111100;	// Blank
		DDRA &= ~0b11111100;	// Input time !
		ResetIF();
	} else if (ifstep == 1) {
		IncIF();
	} else if (ifstep == 2) {
		DDRA |= 0b11111100;		// Output time !
		PORTA &= ~0b11111100;	// Blank
		IncIF();
	//	ResetIF();
		PORTA |= blink<<2;
	//	DDRA = 3;
	//	count = PINA;
	} else if (ifstep == 3) {
		DDRA |= 0b11111100;
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA |= blink<<2;
	} else if (ifstep == 4) {
		// OP Sel
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA |= blink<<2;
	} else if (ifstep == 5) {
		// Ch Sel
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA |= blink<<2;
	} else if (ifstep == 6) {
		// Env param
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA |= blink<<2;
	} else if (ifstep == 7) {
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA |= blink<<2;

		if (!(testtmr & 3)) {
			if (!blink) blink = 1;
			blink <<= 1;
			if (blink == 64) blink = 1;
		}
		testtmr++;

		/*if (testtmr == 10<<3) {
			if (!(Btn & 0x60)) {	// Press SYS + OP on startup = factory reset
				// Factory reset
				cli();
				InitPatch(0,1);
				for (b=0;b<4;b++) {
					pbank = b;
					for (p=0;p<64;p++)
						savepatch(p,0,1);
				}
				RestoreDefaultSettings();
				sei();
				testtmr = 0;
				errorcode = 4;
				//IFHandler = IFH_Error;
			} else {
				//IFHandler = IFH_Run;
			}
		}*/
	}

	ifstep++;
	if (ifstep >= 8) ifstep = 0;
}

/*void IFH_Save() {
	uint8_t blink;

	blink = ((testtmr>>3)&1);

	DigitA = DigitB = 8;

	if (ifstep == 0) {
		DDRD |= 0b00111100;		// Output time !
		PORTD = (PORTD & 3) | 0b00111100;	// Blank 7-Seg
		ResetIF();
		if (blink) PORTD = (PORTD & 3) | (DigitA<<2);
	} else if (ifstep == 1) {
		PORTD = (PORTD & 3) | 0b00111100;	// Blank 7-Seg
		IncIF();
		if (blink) PORTD = (PORTD & 3) | (DigitB<<2);
		testtmr++;
	}

	ifstep++;
	if (ifstep >= 2) ifstep = 0;
}*/
/*
void IFH_Error() {
	uint8_t blink;

	blink = ((testtmr>>4)&1);

	if (ifstep == 0) {
		DDRD |= 0b11111100;		// Output time !
		PORTD = (PORTD & 3) | 0b00111100;	// Blank 7-Seg
		ResetIF();
		if (blink) PORTD = (PORTD & 3) | (0<<2);
	} else if (ifstep == 1) {
		PORTD = (PORTD & 3) | 0b00111100;	// Blank 7-Seg
		IncIF();
		if (blink) PORTD = (PORTD & 3) | (errorcode<<2);
		testtmr++;
		if (testtmr == 10<<5) IFHandler = IFH_Run;
	}

	ifstep++;
	if (ifstep >= 2) ifstep = 0;
}
*/

void IFH_Run() {
	uint8_t c,ringVal;

	ProcessMIDI();
	updatenotes();

	if (ifstep == 0) {
		if (parammax)
			ringVal = (((uint16_t)(*paramptr)<<8) / (parammax))>>5;
		else
			ringVal = 0;

		ringLight = 0;
		for (c=0;c<ringVal;c++) {
			ringLight = ringLight<<1;
			ringLight |= 1;
		}

		PORTA &= ~0b11111100;	// Blank
		DDRA &= ~0b11111100;	// Input time !
		PORTA |= 0b11111100;	// Pullups
		ResetIF();
		BtnA = (PINA >> 2) & 0x3F;
		IncIF();				// IncIF here instead of at the beginning of ifstep1 (hack because
								// capacity at input (buttons) causes problems (push CH 0 -> pushes SYS 7)
	} else if (ifstep == 1) {
		// Read buttons
		BtnA |= (PINA & 0xC) << 4;
		BtnB = (PINA & 0x10) >> 4;

		if (!Btnf) {
			BtnA ^= 0xFF;
			BtnAActive = (OldBtnA ^ BtnA) & BtnA;
			OldBtnA = BtnA;
			BtnB ^= 0xFF;
			BtnBActive = (OldBtnB ^ BtnB) & BtnB;
			OldBtnB = BtnB;
			Btnf = 1;
		}
	} else if (ifstep == 2) {
		DDRA |= 0b11111100;		// Output time !
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA = ((ringLight & 0x0F)<<2);
		if ((CurOpParam < 2) && (CurParam == OP)) PORTA |= (1<<CurOpParam)<<6;
	} else if (ifstep == 3) {
		// MSB of pot lights + AM + detune
		// adpppp--
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA = ((ringLight & 0xF0)>>2);
		if ((CurOpParam > 1) && (CurOpParam < 4) && (CurParam == OP)) PORTA |= (1<<(CurOpParam-2))<<6;
	} else if (ifstep == 4) {
		// LED Op indication + Algo + Stereo
		// ooooas--
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA = (PORTA & 3) | (8>>CurOp)<<4;
		if ((CurChParam < 2) && ((CurParam == CH) || (CurParam == LFO))) PORTA |= (1<<CurChParam)<<2;
	} else if (ifstep == 5) {
		// LED Channel indication
		// cccccc--
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		PORTA = (PORTA & 3) | (32>>CurCh)<<2;
		if (chflash[0]) {
			chflash[0]--;
			PORTA |= 128;
		}
		if (chflash[1]) {
			chflash[1]--;
			PORTA |= 64;
		}
		if (chflash[2]) {
			chflash[2]--;
			PORTA |= 32;
		}
		if (chflash[3]) {
			chflash[3]--;
			PORTA |= 16;
		}
		if (chflash[4]) {
			chflash[4]--;
			PORTA |= 8;
		}
		if (chflash[5]) {
			chflash[5]--;
			PORTA |= 4;
		}
	} else if (ifstep == 6) {
		// LED envelope parameters
		// pppppp--
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		if ((CurOpParam > 3) && (CurParam == OP)) PORTA = (PORTA & 3) | (1<<(CurOpParam-4))<<2;
	} else if (ifstep == 7) {
		// LED system parameters
		// ssssss--
		PORTA &= ~0b11111100;	// Blank
		IncIF();
		if (dacflash) {
			dacflash--;
			PORTA |= 0b00000100;
		}
		PORTA |= (SDAccess & 1) << 3;
		/*if (midiclk <= 4) {
			if (midipulseto < 0x10) {
				midipulse = 1;
				midipulseto++;
			} else {
				midipulse = 0;
			}
		} else {
			midipulse = 0;
			midipulseto = 0;
		}
		PORTD = (PORTD & 3) | (midipulse<<4)<<2;
		*/
		if ((CurChParam > 1) && (CurParam == CH)) PORTA |= (1<<(CurChParam-2))<<4;
		/*
		if ((CurParam == SYS) || (CurParam == SYS_PATCH) || (CurParam == SYS_BANK) || (CurParam == SYS_CH) || (CurParam == SYS_UNIDET) || (CurParam == SYS_CHMODE) || (CurParam == SYS_UNICHS)) PORTD |= (1<<5)<<2;*/
	}/* else if (ifstep == 8) {
		PORTA &= ~0b11111100;	// Blank
		IncIF();
	} else if (ifstep == 9) {
		PORTA &= ~0b11111100;	// Blank
		IncIF();
	}*/

	ifstep++;
	if (ifstep >= 8) ifstep = 0;	// Interface Read/Refresh loop
}
