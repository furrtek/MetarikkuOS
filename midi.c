#include <avr/io.h>
#include <stdio.h>
#include "metarikku_os.h"
#include "lcd.h"
#include "ym2612.h"

uint8_t MIDIBRead, mnotep;
uint16_t f;
uint16_t note[12] = {617,653,692,733,777,823,872,924,979,1037,1099,1164};
uint8_t nrpn_lsb = 0,nrpn_msb = 1;

MIDIBData GetMIDIData() {
	MIDIBData MIDIData;
	if (MIDIBRead == MIDIBPut) {
		MIDIData.EOB = 1;	// End of buffer
		MIDIData.Data = 0;
		return MIDIData;
	} else {
		MIDIData.Data = MIDIBuffer[MIDIBRead];
		MIDIData.EOB = 0;
		MIDIBRead = (MIDIBRead+1) & 31;
		return MIDIData;
	}
}

void kill(uint8_t FMCh, uint8_t MIDINote, uint8_t MIDICmdCh) {
	uint8_t u;

	if (SystemParams.ChMode == INDEP) {
		needtokill[FMCh] = MIDINote | 0x80;
	} else {
		if (MIDICmdCh == SystemParams.MIDICh[0]) {
			if (SystemParams.ChMode == POLY) {
				for (u=0;u<=5;u++) {
					if (polynote[u] == (MIDINote | 0x80)) {
						needtokill[u] = MIDINote | 0x80;
						break;
					} 
				}
			}
			if ((SystemParams.ChMode == UNISON) || (SystemParams.ChMode == UNISONMT)) {
				needtokill[0] = MIDINote | 0x80;
			}
		}
	}
}

//TODO: Useless function...
void setParamVal(uint8_t *ptr,uint8_t val) {
	(*ptr) = val;
}

void ProcessMIDI() {
	uint16_t MIDICmdByte;
	uint8_t c,MIDIVel,MIDIPBL,MIDIPBM,MIDICtrl,MIDINote,MIDICmd,MIDICmdCh,FMCh;
//	uint8_t sysex_id,sysex_xx,sysex_yy;
	uint8_t opnb,oppr;
	MIDIBData MIDIData;

	while (MIDIBPut != MIDIBGet) {
		MIDIBRead = MIDIBGet;

		// Go through buffer to see if we got a MIDI command
		do {
			MIDIData = GetMIDIData();
			if (MIDIData.EOB) {
				MIDIBGet = MIDIBRead;
				return;	// No commands
			}
		} while ((MIDIData.Data & 0x80) != 0x80);

		MIDICmdByte = MIDIData.Data;
		MIDICmdCh = MIDICmdByte & 0x0F;
		FMCh = -1;
		// Match MIDI channel with FM channel
		for (c=0;c<6;c++) {
			if (SystemParams.MIDICh[c] == MIDICmdCh) {
				FMCh = c;
				break;
			}
		}

		MIDICmd = MIDICmdByte >> 4;

		if ((FMCh == -1) && (MIDICmd != 0xF)) {
			MIDIBGet = MIDIBRead;
			return;	// Channel isn't associated and command isn't SysEx: doesn't concern us
		}

		if (MIDICmd == 0x9) {
			// Note on
			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDINote = MIDIData.Data;

			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDIVel = MIDIData.Data;

/*
			LCD_XY(0,0);
			sprintf(message,"Note %u Vel %u  ",MIDINote,MIDIVel);
			wrtr(message);
*/

			/*if (!MIDIVel) {
				kill(FMCh,MIDINote,MIDICmdCh);
			} else {*/
				if (SystemParams.ChMode == POLY) {
					if (MIDICmdCh == SystemParams.MIDICh[0]) {
						chflash[unirotate] = NOTEONFLASH;
						needtoplay[unirotate++] = MIDINote | 0x80;
						if (unirotate == 6) unirotate = 0;
					}
				} else if (SystemParams.ChMode == INDEP) {
					needtoplay[FMCh] = MIDINote | 0x80;
					chflash[FMCh] = NOTEONFLASH;
				} else {
					if (MIDICmdCh == SystemParams.MIDICh[0]) {
						needtoplay[0] = MIDINote | 0x80;
						// Unison modes
						for (c=0;c<=UniChs+1;c++) {
							chflash[c] = NOTEONFLASH;
						}
					}
				}
			//}
			MIDIBGet = MIDIBRead;
		} else if (MIDICmd == 0x8) {
			// Note off
			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDINote = MIDIData.Data;

			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			// Ignore velocity for note off

			kill(FMCh,MIDINote,MIDICmdCh);
			MIDIBGet = MIDIBRead;
		} else if (MIDICmd == 0xB) {
			// Control change MIDI
			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDICtrl = MIDIData.Data;

			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDIVel = MIDIData.Data;

			first[FMCh] = 1;	// Need to resend patch next time

			// Patch bank change
			if ((MIDICtrl == 0) && (MIDIVel <= 3)) pbank = MIDIVel;

			// NRPN Number
			if (MIDICtrl == 98) nrpn_lsb = MIDIVel;
			if (MIDICtrl == 99) nrpn_msb = MIDIVel;
		
			// NRPN Value change
			if ((MIDICtrl == 6) && (nrpn_msb == 0)) {
				if (nrpn_lsb == 0) {			//0: Feedback
					setParamVal(&Channels[FMCh].Feedback,MIDIVel & 7);
				} else if (nrpn_lsb == 1) {	//1: Algo
					setParamVal(&Channels[FMCh].Algo,MIDIVel & 7);
				} else if (nrpn_lsb == 2) {	//2: Stereo
					setParamVal(&Channels[FMCh].LR,MIDIVel & 3);
				} else if (nrpn_lsb == 3) {	//3: AMS
					setParamVal(&Channels[FMCh].AMS,MIDIVel & 7);
				} else if (nrpn_lsb == 4) {	//4: FMS
					setParamVal(&Channels[FMCh].FMS,MIDIVel & 3);
				} else if (nrpn_lsb == 5) {	//5: LFO speed
					setParamVal(SystemParams.LFOSpeed,MIDIVel & 7);
				} else if (nrpn_lsb == 6) {	//6: LFO enable
					setParamVal(SystemParams.LFOen,MIDIVel & 1);
				} else if (nrpn_lsb == 7) {	//7: Unison detune
					setParamVal(&UniDetune,MIDIVel>>1);
				} else if (nrpn_lsb == 8) {	//8: Unison channels
					if (MIDIVel <= 4) setParamVal(&UniChs,MIDIVel);
				} else if ((nrpn_lsb >= 10) && (nrpn_lsb < 50)) {	//Per operator CC
					opnb = (nrpn_lsb-10)/10;
					oppr = (nrpn_lsb-10)%10;
					if (opnb <= 3) {
						if (oppr == 0) {		//10: Total level
							setParamVal(&Channels[FMCh].Operator[opnb].TotalLevel,MIDIVel);
						} else if (oppr == 1) {	//11: AM enable
							setParamVal(&Channels[FMCh].Operator[opnb].AM,MIDIVel & 1);
						} else if (oppr == 2) {	//12: Attack rate
							setParamVal(&Channels[FMCh].Operator[opnb].Attack,MIDIVel>>2);
						} else if (oppr == 3) {	//13: Decay 1 rate
							setParamVal(&Channels[FMCh].Operator[opnb].DecayA,MIDIVel>>2);
						} else if (oppr == 4) {	//14: Decay 2 rate
							setParamVal(&Channels[FMCh].Operator[opnb].DecayB,MIDIVel>>2);
						} else if (oppr == 5) {	//15: Decay 2 level
							setParamVal(&Channels[FMCh].Operator[opnb].DecayLevel,MIDIVel>>3);
						} else if (oppr == 6) {	//16: Release rate
							setParamVal(&Channels[FMCh].Operator[opnb].Release,MIDIVel>>3);
						} else if (oppr == 7) {	//17: Scaling
							setParamVal(&Channels[FMCh].Operator[opnb].Scaling,MIDIVel>>5);
						} else if (oppr == 8) {	//18: Multiply
							setParamVal(&Channels[FMCh].Operator[opnb].Multiply,MIDIVel>>3);
						} else if (oppr == 9) {	//19: Detune
							setParamVal(&Channels[FMCh].Operator[opnb].Detune,MIDIVel>>4);
						}
					}
				}
			} else if (MIDICtrl == 0x7B) {
				// Channel mode, all sounds off
				stfu();
			} else if (MIDICtrl == 0x79) {
				// Reset
				midiclk = 0;
			}
			MIDIBGet = MIDIBRead;
		} else if (MIDICmd == 0xC) {
			// Program change
			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDIVel = MIDIData.Data;

			/*if (MIDIVel <= 63) {
				//TODO: take patch bank into consideration !
				if (patchnb[FMCh] != MIDIVel) {	// Load patch only if different number
					loadpatch(MIDIVel,FMCh,1);	// Report error
					first[FMCh] = 1;	// Need to resend patch next time
				}
			}*/
			MIDIBGet = MIDIBRead;
		} else if (MIDICmd == 0xE) {
			// Pitch bend
			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDIPBL = MIDIData.Data;

			MIDIData = GetMIDIData();
			if (MIDIData.EOB)
				return;
			else
				MIDIPBM = MIDIData.Data;

			mbend[FMCh] = MIDIPBL;
			mbend[FMCh] += MIDIPBM<<7;
			mbend[FMCh] -= 8192;
			MIDIBGet = MIDIBRead;
		} else if (MIDICmdByte == 0xF8) {
			//MIDI clock
			if (midiclk < 24)
				midiclk++;
			else
				midiclk = 0;
			MIDIBGet = MIDIBRead;
		} else if (MIDICmdByte == 0xFA) {
			//MIDI start
			if (SystemParams.ChMode == POLY) {
				for (c=0;c<=5;c++)
					polynote[c] = 0;	// Free all polyphony slots
			}
			midiclk = 0;
			MIDIBGet = MIDIBRead;
		} else if (MIDICmdByte == 0xFC) {
			//MIDI stop
			stfu();
			MIDIBGet = MIDIBRead;
		} else if (MIDICmdByte == 0xF0) {
			//SYSEX
		} else {
			MIDIBGet = MIDIBRead;
		}
	}
}
