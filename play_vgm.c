#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <string.h>
#include "fat32.h"
#include "sd.h"
#include "lcd.h"
#include "io.h"
#include "ym2612.h"
#include "metarikku_os.h"

#define VGMWAIT 15

unsigned char readVGM (char *fileName) {
	struct dir_Structure *dir;
	unsigned long fileSize;
	uint8_t vgmcmd, reg, data;
	uint16_t w;

	dir = findFiles (GET_FILE, fileName);
	if (!dir) {
		LCD_XY(0,0);
		wrtr("FILE NOT FOUND");
		for(;;) {}
	}

	LCD_XY(0,0);
	wrtr("VGM PLAY       ");
	LCD_XY(0,1);
	wrtr("CONTRA.DAT      ");

	cluster = (((unsigned long) dir->firstClusterHI) << 16) | dir->firstClusterLO;

	fileSize = dir->fileSize;

	ifi = 0;
	jfi = 0;

	dfirstSector = getFirstSector (cluster);
	SD_readSingleBlock(dfirstSector + jfi);

	while(1) {
		
		vgmcmd = getfilebyte();
		
		if (vgmcmd == 0x52) {
			reg = getfilebyte();
			data = getfilebyte();
			setreg(0, reg, data);
			if ((reg == 0x28) && (data & 0xF0)) {
				data &= 7;
				if (data > 2) data--;
				chflash[data] = NOTEONFLASH;
			}
		} else if (vgmcmd == 0x53) {
			reg = getfilebyte();
			data = getfilebyte();
			setreg(1, reg, data);
		} else if ((vgmcmd & 0xF0) == 0x80) {
			data = getfilebyte();
			setreg(0, 0x2A, data);
			for (w=0;w<(vgmcmd & 0x0F);w++)		//9
				_delay_us(9);
			if (data != 0x80) dacflash = NOTEONFLASH;
		} else if ((vgmcmd & 0xF0) == 0x70) {
			for (w=0;w<((vgmcmd & 0x0F)+1);w++)
				_delay_us(VGMWAIT);
		} else if (vgmcmd == 0x61) {
			w = getfilebyte() & 0xFF;
			w += getfilebyte()<<8;
			for (;w>0;w--)
				_delay_us(VGMWAIT);
		} else if (vgmcmd == 0x62) {
			// Wait
			for (w=735;w>0;w--)
				_delay_us(19);
		} else if (vgmcmd == 0x66) {
			stfu();
			return 0;
		} else {
			LCD_XY(0,1);
			sprintf(message,"UC? %X %X %X %X",vgmcmd, buffer[ifi+1], ifi, jfi);
			wrtr(message);
			return 0;
		}
	}
	return 0;
}
