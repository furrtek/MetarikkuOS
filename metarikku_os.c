// Master volume menu
// Digipot error detection
// VGM play menu
// Factory reset
// Patch CRC in KKU files
// Save patch mettre date par defaut (debug avec Winimage)
// Menu DEBUG
// Preset confirm to No each time a YESNO is needed ?
// Verifier tuning
// Gestion write protect SD

// Future versions:
// Patch naming
// CRC on EEPROM parameters
// Pitch global
// Choix bigfont/smallfont

#define DEVID 0x20802080
#define SWVERSIONMAJOR 0
#define SWVERSIONMINOR 1
#define VERSION "0.1"

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/pgmspace.h>

#include "spi.h"
#include "sd.h"
#include "fat32.h"
#include "lcd.h"
#include "io.h"
#include "midi.h"
#include "ym2612.h"
#include "metarikku_os.h"
#include "play_vgm.h"
#include "interface.h"
#include "lcdtext.h"
#include "buttons.h"

const uint8_t text_build[] PROGMEM = 	__DATE__;
const uint8_t text_splash[] PROGMEM =	"METARIKKU OS " VERSION;

sysparams  EEMEM EEPROM;

uint8_t chflash[6];
uint8_t dacflash;
uint8_t SDAccess = 0;
uint8_t PatchNum = 0;

const unsigned char bootldrinfo[8] __attribute__ ((section (".bootldrinfo"))) = {0x20, 0x80, 0x20, 0x80, SWVERSIONMAJOR << 8 | SWVERSIONMINOR, 0x00, 0x00};

volatile uint8_t *txtbptr;
volatile uint8_t *txtmptr;

volatile unsigned long startBlock;
volatile unsigned long totalBlocks;
volatile unsigned char buffer[512];
volatile unsigned long firstDataSector, rootCluster, totalClusters;
volatile unsigned int  bytesPerSector, sectorPerCluster, reservedSectorCount;

FPointer IFHandler = IFH_Test;

enum choicetype CurChoiceType;

volatile uint8_t testparam;

#define KEYDELAY 50

volatile uint8_t *paramptr;
uint8_t parammax;
uint8_t MIDIBuffer[32] = {0};

uint8_t MIDIBPut = 0;
uint8_t MIDIBGet = 0;
volatile uint8_t enc=0;
uint8_t message[16];
uint8_t ifstep = 0;
uint16_t volatile testtmr = 0;

chan Channels[6];
int16_t mbend[6] = {0};		// Pitch bend signed 14-bit value 0 centered for each channel
uint8_t patchnb[6] = {0}; 	// Last patch number loaded in each channel
uint8_t first[6] = {1,1,1,1,1,1};	// Patch reload flags (messy)
uint8_t polynote[6];
uint8_t needtoplay[6];
uint8_t needtokill[6];
uint8_t unirotate;
uint8_t LastSavedPatch;
uint8_t LastSavedPatchBank;

uint8_t midiclk = 0;	// MIDI clock pulse counter and LED timing
uint8_t midipulse = 0;
uint8_t midipulseto = 0;
uint8_t Confirm;

enum lparam CurParam = CH;		// Current parameter category
enum lparam OldCurParam;		// Last parameter category, used to restore after Sys menu access
uint8_t UniDetune;				// Unison detune
uint8_t UniChs;					// Unison number of channels 	
uint8_t MenuSelection;

uint8_t CurOp = 3;		//0~3	Operator 4
uint8_t CurCh = 0;		//0~5	Channel 1
uint8_t CurOpParam = 0;	//0~9	AM Enable
uint8_t CurChParam = 4;	//0~5	Stereo
uint8_t pbank = 0;		// Current patch bank

uint8_t refreshlcd = 0;

uint8_t BtnA,BtnB,OldBtnA,OldBtnB,Btnf,BtnAActive,BtnBActive;

volatile uint8_t debug = 0;

ISR(INT0_vect) {
	if (parammax) {
		sei();
		_delay_ms(1);
		cli();
		if (!(PIND & _BV(PD2))) {
			if (PIND & _BV(PD3)) {
				if ((*paramptr) != 0) (*paramptr)--;
			} else {
				if ((*paramptr) != parammax) (*paramptr)++;
			}
			refreshlcd = 1;
		}
		EIFR &= ~1;
	}
}

// MIDI receive, fill buffer
ISR(USART0_RX_vect) {
/*	MIDIBuffer[MIDIBPut] = UDR0;
	MIDIBPut = (MIDIBPut + 1) & 31;*/  //DEBUG
}

// UI update
ISR(TIMER0_COMPA_vect) {
	IFHandler();
}

void InitPatch(uint8_t ch, uint8_t silent) {
	uint8_t c;

	chan *chinit = &Channels[ch];

	chinit->Feedback = 0;
	chinit->Algo = 0;
	chinit->LR = 3;
	chinit->AMS = 0;
	chinit->FMS = 0;

	for (c=0;c<=3;c++) {
		chinit->Operator[c].TotalLevel = 0;
		chinit->Operator[c].AM = 0;
		chinit->Operator[c].Attack = 0;
		chinit->Operator[c].DecayA = 0;
		chinit->Operator[c].DecayB = 0;
		chinit->Operator[c].DecayLevel = 0;
		chinit->Operator[c].Release = 0;
		chinit->Operator[c].Scaling = 0;
		chinit->Operator[c].Multiply = 0;
		chinit->Operator[c].Detune = 0;
	}

	if (!silent) {
		chinit->Operator[3].TotalLevel = 127;
		chinit->Operator[3].DecayA = 31;
		chinit->Operator[3].DecayLevel = 15;
	}
}

void makepptr() {
	// Disable encoder interrupt
	EIMSK = 0b00000000;

	parammax = 0;	//Security in case something was buggy...

	if (CurParam == OP) {

		CurChoiceType = NUMERIC;	//NUMERIC by default in OP

		chan *chmod = &Channels[CurCh];

		if (CurOpParam == 0) {
			paramptr = &chmod->Operator[CurOp].AM;
			parammax = 1;
			txtbptr = textp_amen;
			CurChoiceType = YESNO;
		} else if (CurOpParam == 1) {
			paramptr = &chmod->Operator[CurOp].Detune;
			parammax = 7;
			txtbptr = textp_detune;
		} else if (CurOpParam == 2) {
			paramptr = &chmod->Operator[CurOp].Scaling;
			parammax = 3;
			txtbptr = textp_scaling;
		} else if (CurOpParam == 3) {
			paramptr = &chmod->Operator[CurOp].Multiply;
			parammax = 15;
			txtbptr = textp_multiply;
		} else if (CurOpParam == 4) {
			paramptr = &chmod->Operator[CurOp].TotalLevel;
			parammax = 127;
			txtbptr = textp_tl;
		} else if (CurOpParam == 5) {
			paramptr = &chmod->Operator[CurOp].Attack;
			parammax = 31;
			txtbptr = textp_ar;
		} else if (CurOpParam == 6) {
			paramptr = &chmod->Operator[CurOp].DecayA;
			parammax = 31;
			txtbptr = textp_d1;
		} else if (CurOpParam == 7) {
			paramptr = &chmod->Operator[CurOp].DecayLevel;
			parammax = 15;
			txtbptr = textp_dl;
		} else if (CurOpParam == 8) {
			paramptr = &chmod->Operator[CurOp].DecayB;
			parammax = 31;
			txtbptr = textp_d2;
		} else if (CurOpParam == 9) {
			paramptr = &chmod->Operator[CurOp].Release;
			parammax = 15;
			txtbptr = textp_rr;
		}

	} else if (CurParam == CH) {

		CurChoiceType = NUMERIC;	//NUMERIC by default in CH too

		chan *chmod = &Channels[CurCh];

		if (CurChParam == 0) {
			MenuSelection = 0;
			paramptr = &MenuSelection;
			parammax = 1;
			txtbptr = textp_lfomenu;
			txtmptr = menu_lfo;
			CurChoiceType = TEXT;
		} else if (CurChParam == 1) {
			paramptr = &chmod->Algo;
			parammax = 7;
			txtbptr = textp_algo;
		} else if (CurChParam == 2) {
			paramptr = &chmod->FMS;
			parammax = 7;
			txtbptr = textp_fms;
		} else if (CurChParam == 3) {
			paramptr = &chmod->AMS;
			parammax = 3;
			txtbptr = textp_ams;
		} else if (CurChParam == 4) {
			paramptr = &chmod->LR;
			parammax = 3;
			txtbptr = textp_stereo;
			txtmptr = choices_stereo;
			CurChoiceType = TEXT;
		} else if (CurChParam == 5) {
			paramptr = &chmod->Feedback;
			parammax = 7;
			txtbptr = textp_feedback;
		}

	} else if (CurParam == LFO) {

		txtbptr = (PGM_P)pgm_read_word(txtmptr + 2*MenuSelection);
		if (MenuSelection == 0) {
			paramptr = &SystemParams.LFOen;
			parammax = 1;
			CurChoiceType = YESNO;
		} else if (MenuSelection == 1) {
			paramptr = &SystemParams.LFOSpeed;
			parammax = 7;
			CurChoiceType = NUMERIC;
		}

	} else if (CurParam == SYS) {

		if (MenuSelection == 1) {
			txtbptr = textp_sampmenu;
			paramptr = &SystemParams.SampleBank;
			parammax = 99;
			CurChoiceType = NUMERIC;
		} else if (MenuSelection == 3) {
			txtbptr = textp_dacen;
			paramptr = &SystemParams.DACen;
			parammax = 1;
			CurChoiceType = YESNO;
		} else if (MenuSelection == 4) {
			txtbptr = textp_ch36mode;
			paramptr = &SystemParams.CH36mode;
			parammax = 1;
			txtmptr = choices_ch36mode;
			CurChoiceType = TEXT;
		} else if (MenuSelection == 5) {
			txtbptr = textp_polymode;
			paramptr = &SystemParams.ChMode;
			parammax = 3;
			txtmptr = choices_polymode;
			CurChoiceType = TEXT;
		} else if (MenuSelection == 7) {
			txtbptr = textp_sdformat;
			paramptr = &Confirm;
			parammax = 1;
			CurChoiceType = YESNO;
		}

	} else if (CurParam == PATCH) {

		txtbptr = textp_patchmenu;
		MenuSelection = 0;
		paramptr = &MenuSelection;
		parammax = 3;
		txtmptr = menu_patch;
		CurChoiceType = TEXT;

	} else if (CurParam == PATCHCH) {

		txtbptr = (PGM_P)pgm_read_word(txtmptr + 2*MenuSelection);
		if (MenuSelection == 0) {
			PatchNum = patchnb[CurCh];
			paramptr = &PatchNum;
			parammax = 255;
			CurChoiceType = NUMERIC;
		} else if (MenuSelection == 1) {
			// TODO: Name patch
		} else if (MenuSelection == 2) {
			PatchNum = patchnb[CurCh];
			paramptr = &PatchNum;
			parammax = 255;
			CurChoiceType = NUMERIC;
		} else if (MenuSelection == 3) {
			paramptr = &Confirm;
			parammax = 1;
			CurChoiceType = YESNO;
		}

	} else if (CurParam == MIDI) {

		txtbptr = textp_midichs;
		MenuSelection = 0;
		paramptr = &MenuSelection;
		parammax = 5;
		txtmptr = choices_midi;
		CurChoiceType = TEXT;

	} else if (CurParam == MIDICH) {

		txtbptr = (PGM_P)pgm_read_word(txtmptr + 2*MenuSelection);
		MenuSelection = 0;
		paramptr = &SystemParams.MIDICh[MenuSelection];
		parammax = 15;
		CurChoiceType = NUMERIC;

	} else if (CurParam == DISP) {

		txtbptr = textp_dispmenu;
		MenuSelection = 0;
		paramptr = &MenuSelection;
		parammax = 2;
		txtmptr = menu_display;
		CurChoiceType = TEXT;

	} else if (CurParam == DISPCH) {

		txtbptr = (PGM_P)pgm_read_word(txtmptr + 2*MenuSelection);
		if (MenuSelection == 0) {
			paramptr = &SystemParams.BigFont;
			parammax = 1;
			CurChoiceType = YESNO;
		} else if (MenuSelection == 1) {
			MenuSelection = 0;
			paramptr = &SystemParams.Refresh;
			parammax = 2;
			txtmptr = choices_refr;
			CurChoiceType = TEXT;
		} else if (MenuSelection == 2) {
			paramptr = &SystemParams.Brightness;
			parammax = 15;
			CurChoiceType = NUMERIC;
		}

	}

	// Enable encoder interrupt
	EIMSK = 0b00000001;
}

void updatenotes() {
	uint8_t c,mnotep,u;
	uint16_t f;

	for (c=0;c<=5;c++) {
		if (needtoplay[c] & 0x80) {
			mnotep = needtoplay[c] & 0x7F;
			f = ((mnotep / 12)<<11) + note[mnotep % 12];
			if (SystemParams.ChMode == INDEP) {
				f += mbend[c]>>4;
				playnote(1,c,f,c);
				first[c] = 0;
			} else if (SystemParams.ChMode == UNISON) {
				f += mbend[0]>>4;
				playnote(1,0,f,0);
				playnote(1,1,f+UniDetune,0);
				for (u=0;u<=UniChs;u++) {
					playnote(1,2+u,f+(UniDetune*(2+u)),0);
				}
				first[0] = 0;
			} else if (SystemParams.ChMode == UNISONMT) {
				f += mbend[0]>>4;
				playnote(1,0,f,0);
				playnote(1,1,f+UniDetune,1);
				for (u=0;u<=UniChs;u++) {
					playnote(1,2+u,f+(UniDetune*(2+u)),2+u);
				}
				first[0] = 0;
			} else if (SystemParams.ChMode == POLY) {
				// See if note is already being played somewhere
				for (u=0;u<=5;u++) {
					if (polynote[u] == (mnotep | 0x80)) break; 
				}
				if (u == 6) {
					// No, see if we can find a free spot
					for (u=0;u<=5;u++) {
						if (!polynote[u]) break; 
					}
					if (u != 6) {
						f += mbend[0]>>4;
						polynote[u] = mnotep + 0x80;
						playnote(1,u,f,0);
					}
				}
			}
			needtoplay[c] = mnotep;
		} else if (first[c]) {
			playnote(1,c,0,c);
			first[c] = 0;
		}
		if (needtokill[c] & 0x80) {
			NoteOff(c,needtokill[c] & 0x7F);
			needtokill[c] = 0;
		}
	}
}

int main(void) {
	//struct dir_Structure *dir;

	WDTCSR = (1<<WDCE) | (1<<WDE);
	WDTCSR = 0x00;

	DDRA = _BV(STEREO_EN) | _BV(LCD_E) | 0b11111100;
	DDRB = _BV(nSD_SS) | _BV(SD_MOSI) | _BV(SD_SCK) | 0b00001111;
	DDRC = _BV(YM_A0) | _BV(YM_A1) | _BV(IF_MR) | _BV(IF_CLK) | _BV(nYM_CS) | _BV(nYM_WR) | _BV(YMD_CLK) | _BV(YMD_D);
	DDRD = _BV(nYM_RST) | _BV(YM_PHI) | _BV(LCD_RS) | _BV(LEDPWM);

	PORTC = _BV(nYM_CS) | _BV(nYM_WR);
	PORTD = _BV(LEDPWM) | _BV(ENC1) | _BV(ENC2);

	// Interface refresh cycle timer
	TCCR0A = 0b00000010;	//CTC
	TCCR0B = 0b00000100;	//Prescaler = 100
	OCR0A = 78;				//TODO: EEPROM read byte (refresh rate)
	TIMSK0 = 0b00000010;	//Compare match A interrupt

	//f=16000000/8/256/n
	//n=16000000/8/256/f
	//60Hz n=130
	//50Hz n=156
	//100Hz n=78

	// YM2612 clock generation (8MHz on OC1B)
	TCCR1A = _BV(COM1B0);
	TCCR1B = _BV(WGM12) | _BV(CS10);
	TCCR1C = 0x00;
	OCR1A = 0;

	// Lighting PWM generation
	TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
	TCCR2B = _BV(CS22);
	OCR2A = 0xFF;
	
	EICRA = 0b00000001;
	EIMSK = 0b00000001;

	YM2612_Reset();

	// Load general paramaters
	eeprom_read_block(&SystemParams,&EEPROM,sizeof(sysparams));
	// Sanity check
	// SystemParams.MasterVol;
	if (SystemParams.SampleBank > 99) SystemParams.SampleBank = 0;
	if (SystemParams.DACen > 1) SystemParams.DACen = 1;
	if (SystemParams.CH36mode > 1) SystemParams.CH36mode = 1;
	if (SystemParams.BigFont > 1) SystemParams.BigFont = 1;
	if (SystemParams.Brightness > 15) SystemParams.Brightness = 15;
	if (SystemParams.LFOSpeed > 1) SystemParams.LFOSpeed = 0;
	if (SystemParams.LFOen > 1) SystemParams.LFOen = 1;
	if (SystemParams.Refresh > 2) SystemParams.Refresh = 2;
	if (SystemParams.MIDICh[0] > 15) SystemParams.MIDICh[0] = 0;
	if (SystemParams.MIDICh[1] > 15) SystemParams.MIDICh[1] = 1;
	if (SystemParams.MIDICh[2] > 15) SystemParams.MIDICh[2] = 2;
	if (SystemParams.MIDICh[3] > 15) SystemParams.MIDICh[3] = 3;
	if (SystemParams.MIDICh[4] > 15) SystemParams.MIDICh[4] = 4;
	if (SystemParams.MIDICh[5] > 15) SystemParams.MIDICh[5] = 5;
	if (SystemParams.ChMode > 3) SystemParams.ChMode = 0;
	// CRC

	// DIGIPOT STUFF TO MOVE TODO

	PORTC &= ~_BV(nYM_CS);		// /YMCS low

	SPCR = 0b01010010;	// Master, MSB first, SCK phase low, SCK idle low
	SPSR = 0x00;

	SPI_tx(0b00000000);	// Max
	SPI_tx(0b00000000);

	SPI_tx(0b00010000);
	SPI_tx(0b00000000);

	PORTC |= _BV(nYM_CS);		// /YMCS high

	LCD_Init();

	LCD_XY(0,0);
	wrt(text_splash);

	SPI_Init();
	if (SD_init()) {
		if (!getBootSectorData()) cardPresent = 1;
	}

/*	dir = findFiles (GET_FILE, "UPDATE  BIN");
	if(dir) {
		LCD_XY(0,1);
		wrtr("UPDATE DETECTED");
	}*/

	// USART for MIDI 31250 @ 16MHz
	UBRR0 = 31;
	UCSR0B = (1<<RXCIE0) | (1<<RXEN0);
	UCSR0C = (3<<UCSZ00);

	makepptr();

	//Disable MIDI TODO (interrupts)
	sei();
	//Enable MIDI TODO (interrupts)

	_delay_ms(800);

	IFHandler = IFH_Run;

	LCD_Clear();

	LCD_Update();
	
	for (;;) {

		if (Btnf) BtnH();

		// TODO: Put in encoder function call (saves time)
		OCR2A = SystemParams.Brightness*SystemParams.Brightness;

		//readVGM("SPACE   DAT");

		if (refreshlcd) {
			LCD_Update();
			refreshlcd = 0;
		}
	}
    return 0;
}
