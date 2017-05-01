#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include "metarikku_os.h"
#include "fat32.h"
#include "lcd.h"
#include "sd.h"

uint8_t textp_deleted[] PROGMEM = 		"DELETED  ";
uint8_t textp_notfound[] PROGMEM = 		"NOT FOUND";
uint8_t textp_saved[] PROGMEM = 		"SAVED !  ";
uint8_t textp_loaded[] PROGMEM = 		"LOADED ! ";
uint8_t textp_error[] PROGMEM = 		"ERROR !  ";

uint8_t filename[11];

extern uint8_t textp_sysmenu[];
extern uint8_t menu_system[];

void SavePatch(void) {
	sprintf(filename,"PATCH%03uKKU",PatchNum);
	if (SD_init()) {
		if (!getBootSectorData()) {
			cardPresent = 1;
			if (deleteFile(filename)) {
				LCD_XY(0,1);
				wrt(textp_deleted);
			} else {
				LCD_XY(0,1);
				wrt(textp_notfound);
			}
			_delay_ms(500);
			memcpy(&dataString,&Channels[CurCh],sizeof(chan));
			if (!writeFile(filename)) {
				LCD_XY(0,1);
				wrt(textp_saved);
				patchnb[CurCh] = PatchNum;
			} else {
				LCD_XY(0,1);
				wrt(textp_error);
			}
			_delay_ms(500);
		}
	}
}

void LoadPatch(void) {
	sprintf(filename,"PATCH%03uKKU",PatchNum);
	if (SD_init()) {
		if (!getBootSectorData()) {
			cardPresent = 1;
			if (!readFile(READ,filename)) {
				memcpy(&Channels[CurCh],&buffer,sizeof(chan));
				LCD_XY(0,1);
				wrt(textp_loaded);
				patchnb[CurCh] = PatchNum;
			} else {
				LCD_XY(0,1);
				wrt(textp_error);
			}
			_delay_ms(500);
		}
	}
}

void BtnH(void) {
	//uint8_t c;

	if (BtnAActive & 1) {
		CurCh++;
		if (CurCh == 6) CurCh = 0;
	}
	if (BtnAActive & 2) {
		CurOp++;
		if (CurOp == 4) CurOp = 0;
	}
	if (BtnAActive & 4) {
		if ((CurOpParam > 0) && (CurParam == OP)) CurOpParam--;
		CurParam = OP;
	}
	if (BtnAActive & 8) {
		if ((CurOpParam < 9) && (CurParam == OP)) CurOpParam++;
		CurParam = OP;
	}
	if (BtnAActive & 16) {
		if ((CurChParam > 0) && (CurParam == CH)) CurChParam--;
		CurParam = CH;
	}
	if (BtnAActive & 32) {
		if ((CurChParam < 5) && (CurParam == CH)) CurChParam++;
		CurParam = CH;
	}
	if (BtnAActive & 64) {
		// System
		// Disable encoder interrupt
		EIMSK = 0b00000000;
		OldCurParam = CurParam;
		CurParam = SYS;
		MenuSelection = 0;
		paramptr = &MenuSelection;
		parammax = 8;
		txtbptr = textp_sysmenu;
		txtmptr = menu_system;
		CurChoiceType = TEXT;
		refreshlcd = 1;
		// Enable encoder interrupt
		EIMSK = 0b00000001;
	}
	if (BtnAActive & 128) {
		// Validate
		if (CurParam == LFO) {
			// Quit LFO menu
			CurParam = OldCurParam;
		} else if (CurParam == PATCH) {
			CurParam = PATCHCH;
		} else if (CurParam == PATCHCH) {
			if (MenuSelection == 0) SavePatch();
			//if (MenuSelection == 1) NAME PATCH TODO
			if (MenuSelection == 2) LoadPatch();
			if (MenuSelection == 3) InitPatch(CurCh, 0);
		} else if (CurParam == SYS) {
			// System menu -> Sub-menus
			if (MenuSelection == 0) CurParam = PATCH;
			//if (MenuSelection == 1) CurParam = OldCurParam; //Quit
			if (MenuSelection == 2) CurParam = MIDI;
			//if (MenuSelection == 3) CurParam = OldCurParam; //Quit
			//if (MenuSelection == 4) CurParam = OldCurParam; //Quit
			//if (MenuSelection == 5) CurParam = OldCurParam; //Quit
			if (MenuSelection == 6) CurParam = DISP;
			//if (MenuSelection == 7) CurParam = OldCurParam; //Quit
			if (MenuSelection == 8) CurParam = DEBUG;
		} else if (CurParam == MIDI) {
			CurParam = MIDICH;
		} else if (CurParam == DISP) {
			CurParam = DISPCH;
		} else if (CurParam == CH) {
			if (CurChParam == 0) {
				// Enter LFO menu
				OldCurParam = CH;
				CurParam = LFO;
			}
		}

		/*if (CurParam == SYS) {
			// Validate sys menu selection
			if (SysMenu.val == 0) {
				// Save patch
				if (patchnb[CurCh] < 64)
					OpPatchNb.val = patchnb[CurCh];
				else
					OpPatchNb.val = 0;
				OpPatchNb.stuck = 1;
				CurParam = SYS_PATCH;
			} else if (SysMenu.val == 1) {
				// Load patch
				if (patchnb[CurCh] < 64)
					OpPatchNb.val = patchnb[CurCh];
				else
					OpPatchNb.val = 0;
				OpPatchNb.stuck = 1;
				CurParam = SYS_PATCH;
			} else if (SysMenu.val == 2) {
				// Init patch
				InitPatch(CurCh,0);
				CurParam = OldCurParam;	// Exit Sys mode: restore
			} else if (SysMenu.val == 3) {
				// Set bank
				OpBank.val = pbank;
				OpBank.stuck = 1;
				CurParam = SYS_BANK;
			} else if (SysMenu.val == 4) {
				// Set key mode
				OpChMode.val = ChMode;
				OpChMode.stuck = 1;
				CurParam = SYS_CHMODE;
			} else if (SysMenu.val == 5) {
				// Set unison detune
				UniDetune.stuck = 1;
				CurParam = SYS_UNIDET;
			} else if (SysMenu.val == 6) {
				// Set unison channels count
				UniChs.stuck = 1;
				CurParam = SYS_UNICHS;
			} else if ((SysMenu.val >= 7) && (SysMenu.val <= 12)) {
				// Set MIDI ch for each FM ch
				OpMIDICh.val = MIDICh[SysMenu.val-7];
				OpMIDICh.stuck = 1;
				CurParam = SYS_CH;
			}
		} else if (CurParam == SYS_PATCH) {
			if (SysMenu.val == 0) {
				// Save patch
				savepatch(OpPatchNb.val,CurCh,1);
				sei();
				chflash[0] = 30;	// User feedback (saving)
				chflash[1] = 30;
				chflash[2] = 30;
				chflash[3] = 30;
				chflash[4] = 30;
				chflash[5] = 30;
			} else if (SysMenu.val == 1) {
				// Load patch
				if ((ChMode == POLY) || (ChMode == UNISON)) {
					// Into channel 0 for Polyphony or Unison mode
					loadpatch(OpPatchNb.val,0,1);
				} else {
					// Into selected channel for Independant or Unison Multitimbral mode
					loadpatch(OpPatchNb.val,CurCh,1);
				}
			}
			CurParam = OldCurParam;	// Exit Sys mode: restore
		} else if (CurParam == SYS_BANK) {
			// Set bank
			pbank = OpBank.val;
			savesettings();
			sei();
			CurParam = OldCurParam;	// Exit Sys mode: restore
		} else if (CurParam == SYS_CHMODE) {
			// Set key mode
			ChMode = OpChMode.val;
			if (ChMode == POLY) {
				for (c=0;c<=5;c++)
					polynote[c] = 0;	// Free all polyphony slots
			}
			savesettings();
			sei();
			CurParam = OldCurParam;	// Exit Sys mode: restore
		} else if (CurParam == SYS_UNIDET) {
			// Set Unison detune
			savesettings();
			sei();
			CurParam = OldCurParam;	// Exit Sys mode: restore
		} else if (CurParam == SYS_UNICHS) {
			// Set Unison channel count
			savesettings();
			sei();
			CurParam = OldCurParam;	// Exit Sys mode: restore
		} else {
			// Enter sys menu
			OldCurParam = CurParam;	// Save
			CurParam = SYS;
		}*/
	}
	if (BtnBActive & 1) {
		// Cancel
		if (CurParam == LFO) {
			// Quit LFO menu
			CurParam = OldCurParam;
		} else if (CurParam == SYS) {
			// Quit System menu
			CurParam = OldCurParam;
		} else if (CurParam == PATCH) {
			// Quit Patch menu
			CurParam = OldCurParam;
		} else if (CurParam == MIDI) {
			// Quit MIDI menu
			CurParam = OldCurParam;
		} else if (CurParam == DISP) {
			// Quit Display menu
			CurParam = OldCurParam;
		}

		//TODO: Change !
		eeprom_write_block(&SystemParams,&EEPROM,sizeof(sysparams));
	}

	if ((BtnAActive & ~64) | BtnBActive) {
		//Don't do makepptr on System button, because it starts a menu directly
		makepptr();
		refreshlcd = 1;
	}

	Btnf = 0;
}
