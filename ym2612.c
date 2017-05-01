#include "avr/io.h"
#include "util/delay.h"
#include "io.h"
#include <avr/interrupt.h>
#include "metarikku_os.h"

void YM2612_Reset() {
	PORTD &= ~_BV(nYM_RST);
	_delay_ms(1);
	PORTD |= _BV(nYM_RST);
	_delay_ms(10);
}

void stfu() {
	setreg(0,0x28,0x00);
	setreg(0,0x28,0x01);
	setreg(0,0x28,0x02);
	setreg(0,0x28,0x04);
	setreg(0,0x28,0x05);
	setreg(0,0x28,0x06);
	setreg(0,0xB4+0,0);
	setreg(0,0xB4+1,0);
	setreg(0,0xB4+2,0);
	setreg(1,0xB4+0,0);
	setreg(1,0xB4+1,0);
	setreg(1,0xB4+2,0);
	/*first[0] = 0;
	first[1] = 0;
	first[2] = 0;
	first[3] = 0;
	first[4] = 0;
	first[5] = 0;
	needtoplay[0] = 0;
	needtoplay[1] = 0;
	needtoplay[2] = 0;
	needtoplay[3] = 0;
	needtoplay[4] = 0;
	needtoplay[5] = 0;*/
}

void write_ym(uint8_t data) {
	uint8_t bits;
	
	//PORTC &= ~_BV(nYM_CS);		// /YMCS low

	for (bits=0;bits<8;bits++) {
		if (data & 1)			//Set data bit
			PORTC |= _BV(YMD_D);
		else
			PORTC &= ~_BV(YMD_D);
		PORTC |= _BV(YMD_CLK);		// Clock SR
		//_delay_us(1);
		PORTC &= ~_BV(YMD_CLK);
		//_delay_us(1);
		data = data>>1;
	}
	//_delay_us(1);
	PORTC &= ~_BV(nYM_WR);		// Write to 2612
	_delay_us(5);
	PORTC |= _BV(nYM_WR);
	_delay_us(5);

	//PORTC |= _BV(nYM_CS);		// /YMCS high
}

void setreg(uint8_t bank, uint8_t reg, uint8_t data) {
	if (!bank)
		PORTC &= ~_BV(YM_A1);
	else
		PORTC |= _BV(YM_A1);	// Set A1
	PORTC &= ~_BV(YM_A0);		// A0 low (select register)
	write_ym(reg);
	PORTC |= _BV(YM_A0);		// A0 high (write register)
	write_ym(data);
}

void playnote(uint8_t sendpatch, uint8_t chn, uint16_t f, uint8_t fromch) {
	uint8_t bank,ch;

	oper *op1ptr = &Channels[fromch].Operator[0];
	oper *op2ptr = &Channels[fromch].Operator[2];	// What the fuck ?
	oper *op3ptr = &Channels[fromch].Operator[1];
	oper *op4ptr = &Channels[fromch].Operator[3];

	if (chn < 3) {
		bank = 0;
		ch = chn;
	} else {
		bank = 1;
		ch = chn - 3;
		chn++;			// Shit hole (see 2612 datasheet...)
	}

	//if (f) setreg(0,0x28,0x00+chn);

	setreg(0,0x22,(SystemParams.LFOen<<3)+SystemParams.LFOSpeed);

	if (sendpatch) {
		setreg(bank,0x30+ch,(op1ptr->Detune<<4)+op1ptr->Multiply);
		setreg(bank,0x34+ch,(op2ptr->Detune<<4)+op2ptr->Multiply);
		setreg(bank,0x38+ch,(op3ptr->Detune<<4)+op3ptr->Multiply);
		setreg(bank,0x3C+ch,(op4ptr->Detune<<4)+op4ptr->Multiply);

		setreg(bank,0x40+ch,127-op1ptr->TotalLevel);
		setreg(bank,0x44+ch,127-op2ptr->TotalLevel);
		setreg(bank,0x48+ch,127-op3ptr->TotalLevel);
		setreg(bank,0x4C+ch,127-op4ptr->TotalLevel);

		if (f) {
			setreg(bank,0x50+ch,(op1ptr->Scaling<<6)+(31-op1ptr->Attack));
			setreg(bank,0x54+ch,(op2ptr->Scaling<<6)+(31-op2ptr->Attack));
			setreg(bank,0x58+ch,(op3ptr->Scaling<<6)+(31-op3ptr->Attack));
			setreg(bank,0x5C+ch,(op4ptr->Scaling<<6)+(31-op4ptr->Attack));

			setreg(bank,0x60+ch,(op1ptr->AM<<7)+(31-op1ptr->DecayA));
			setreg(bank,0x64+ch,(op2ptr->AM<<7)+(31-op2ptr->DecayA));
			setreg(bank,0x68+ch,(op3ptr->AM<<7)+(31-op3ptr->DecayA));
			setreg(bank,0x6C+ch,(op4ptr->AM<<7)+(31-op4ptr->DecayA));
		}
	}

	if (f) {
		setreg(bank,0x70+ch,(31-op1ptr->DecayB));
		setreg(bank,0x74+ch,(31-op2ptr->DecayB));
		setreg(bank,0x78+ch,(31-op3ptr->DecayB));
		setreg(bank,0x7C+ch,(31-op4ptr->DecayB));

		setreg(bank,0x80+ch,(op1ptr->DecayLevel<<4)+(15-op1ptr->Release));
		setreg(bank,0x84+ch,(op2ptr->DecayLevel<<4)+(15-op2ptr->Release));
		setreg(bank,0x88+ch,(op3ptr->DecayLevel<<4)+(15-op3ptr->Release));
		setreg(bank,0x8C+ch,(op4ptr->DecayLevel<<4)+(15-op4ptr->Release));
	}

	setreg(bank,0xB0+ch,(Channels[fromch].Feedback<<3)+Channels[fromch].Algo);
	setreg(bank,0xB4+ch,(Channels[fromch].LR<<6)+(Channels[fromch].AMS<<4)+Channels[fromch].FMS);

	if (f) {
		setreg(0,0x28,0x00+chn);

		setreg(bank,0xA4+ch,f >> 8);
		setreg(bank,0xA0+ch,f & 0xFF);

		setreg(0,0x28,0xF0+chn);
	}
}

void NoteOff(uint8_t ch, uint8_t mnoteo) {
	if (SystemParams.ChMode == INDEP) {
		if (mnoteo == needtoplay[ch]) {
			if (ch >= 3) ch++;	// Shit hole again
			setreg(0,0x28,ch);
		}
	} else if ((SystemParams.ChMode == UNISON) || (SystemParams.ChMode == UNISONMT)) {
		setreg(0,0x28,0x00);
		setreg(0,0x28,0x01);
		setreg(0,0x28,0x02);
		setreg(0,0x28,0x04);
		setreg(0,0x28,0x05);
		setreg(0,0x28,0x06);
	} else if (SystemParams.ChMode == POLY) {
		polynote[ch] = 0;	// Free slot
		if (ch >= 3) ch++;
		setreg(0,0x28,ch);
	}
}
