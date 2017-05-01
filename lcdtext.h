#include <avr/pgmspace.h>
#include <inttypes.h>

/*
uint8_t textp_nocard[] =			"NO CARD !";
uint8_t textp_nosamp[] =			"NO SAMPLES !";
uint8_t textp_nopatch[] =			"NO PATCHES !";
*/
								   //0123456789ABCDEF
//Operator parameters
uint8_t textp_freq[] PROGMEM = 		"FREQUENCY";	//Numeric todo

uint8_t textp_amen[] PROGMEM = 		"AM ENABLE";	//Yes/no  ok
uint8_t textp_detune[] PROGMEM = 	"DETUNE";		//Numeric ok
uint8_t textp_scaling[] PROGMEM = 	"SCALING";		//Numeric ok
uint8_t textp_multiply[] PROGMEM = 	"MULTIPLY";		//Numeric ok
uint8_t textp_tl[] PROGMEM = 		"TOTAL LVL";	//Numeric ok
uint8_t textp_ar[] PROGMEM = 		"ATTACK";		//Numeric ok
uint8_t textp_d1[] PROGMEM = 		"DECAY 1";		//Numeric ok
uint8_t textp_dl[] PROGMEM = 		"DECAY LVL";	//Numeric ok
uint8_t textp_d2[] PROGMEM = 		"DECAY 2";		//Numeric ok
uint8_t textp_rr[] PROGMEM = 		"RELEASE";		//Numeric ok

//Channel parameters
uint8_t textp_algo[] PROGMEM = 		"ALGORITHM";	//Numeric ok
uint8_t textp_stereo[] PROGMEM = 	"STEREO";		//Text    ok
uint8_t textp_ams[] PROGMEM = 		"AM INTENS";	//Numeric ok
uint8_t textp_fms[] PROGMEM = 		"FM INTENS";	//Numeric ok
uint8_t textp_lfomenu[] PROGMEM = 	"LFO MENU";		//Menu    ok
uint8_t textp_feedback[] PROGMEM = 	"FEEDBACK";		//Numeric ok

//Stereo parameter choices
uint8_t textp_st0[] PROGMEM = 	"NOTHING";
uint8_t textp_st1[] PROGMEM = 	"LEFT ONLY";
uint8_t textp_st2[] PROGMEM = 	"RIGHT ONLY";
uint8_t textp_st3[] PROGMEM = 	"LEFT & RIGHT";
uint8_t *choices_stereo[4] PROGMEM = {textp_st0,
									textp_st1,
									textp_st2,
									textp_st3};

//LFO menu
uint8_t textp_lfoen[] PROGMEM =		"LFO ENABLE";	//Yes/no
uint8_t textp_lfospeed[] PROGMEM = 	"LFO SPEED";	//Numeric
uint8_t *menu_lfo[2] PROGMEM = {		textp_lfoen,
									textp_lfospeed};

//System menu
uint8_t textp_sysmenu[] PROGMEM =	"SYSTEM MENU";

uint8_t textp_patchmenu[] PROGMEM =	"PATCH MENU";	//Menu
uint8_t textp_sampmenu[] PROGMEM =	"SAMPLE BNK";	//Numeric
uint8_t textp_midichs[] PROGMEM =	"MIDI MENU";	//Menu
uint8_t textp_dacen[] PROGMEM =		"DAC ENABLE";	//Yes/no
uint8_t textp_ch36mode[] PROGMEM =	"CH3/6 MODE";	//Text (see textp_ch36*)
uint8_t textp_polymode[] PROGMEM =	"POLY MODE";	//Text (see textp_poly*)
uint8_t textp_dispmenu[] PROGMEM =	"DISP MENU";	//Text (see textp_refr*)
uint8_t textp_sdformat[] PROGMEM =	"CARD FORMAT";	//Yes/no
uint8_t textp_debugmenu[] PROGMEM =	"DEBUG";		//Menu
uint8_t *menu_system[9] PROGMEM = {	textp_patchmenu,
									textp_sampmenu,
									textp_midichs,
									textp_dacen,
									textp_ch36mode,
									textp_polymode,
									textp_dispmenu,
									textp_sdformat,
									textp_debugmenu};

//CH3 & 6 mode
uint8_t textp_ch36norm[] PROGMEM =	"NORMAL";		//Validate
uint8_t textp_ch36freq[] PROGMEM =	"DEFINE FREQ";	//Validate
uint8_t *choices_ch36mode[2] PROGMEM = {textp_ch36norm,
									textp_ch36freq};

//Debug menu
uint8_t textp_facreset[] PROGMEM =	"FACTORY RESET";//Yes/no
uint8_t textp_midib[] PROGMEM =		"MIDI VIEWER";	//Validate
uint8_t textp_fwrev[] PROGMEM =		"FIRMWARE REV";	//Validate
uint8_t textp_softreset[] PROGMEM =	"SOFT RESET";	//Yes/no
uint8_t *menu_debug[4] PROGMEM = {textp_facreset,textp_midib,textp_fwrev,textp_softreset};

//Patch menu
uint8_t textp_patchs[] PROGMEM =	"PATCH SAVE";	//Yes/no
uint8_t textp_patchn[] PROGMEM =	"PATCH NAME";	//Special menu
uint8_t textp_patchl[] PROGMEM =	"PATCH LOAD";	//Yes/no
uint8_t textp_patchi[] PROGMEM =	"PATCH INIT";	//Yes/no
uint8_t *menu_patch[4] PROGMEM = {textp_patchs,textp_patchn,textp_patchl,textp_patchi};

//Display menu
uint8_t textp_bigfont[] PROGMEM =	"BIG FONT";		//Yes/no
uint8_t textp_refr[] PROGMEM =		"REFRESH";		//Text
uint8_t textp_bright[] PROGMEM =	"BRIGHTNESS";	//Numeric
uint8_t *menu_display[3] PROGMEM = {textp_bigfont,textp_refr,textp_bright};

//Refresh rate
uint8_t textp_refr50[] PROGMEM =	"50Hz";			//Validate
uint8_t textp_refr60[] PROGMEM =	"60Hz";			//Validate
uint8_t textp_refr100[] PROGMEM =	"100Hz";		//Validate
uint8_t *choices_refr[3] PROGMEM = {textp_refr50,textp_refr60,textp_refr100};

//MIDI channels
uint8_t textp_midich1[] PROGMEM = 	"MIDI CH1";		//Numeric
uint8_t textp_midich2[] PROGMEM = 	"MIDI CH2";		//Numeric
uint8_t textp_midich3[] PROGMEM = 	"MIDI CH3";		//Numeric
uint8_t textp_midich4[] PROGMEM = 	"MIDI CH4";		//Numeric
uint8_t textp_midich5[] PROGMEM = 	"MIDI CH5";		//Numeric
uint8_t textp_midich6[] PROGMEM = 	"MIDI CH6";		//Numeric
uint8_t *choices_midi[6] PROGMEM = {	textp_midich1,
									textp_midich2,
									textp_midich3,
									textp_midich4,
									textp_midich5,
									textp_midich6};

//Poly modes
uint8_t textp_poly0[] PROGMEM =		"SINGLE";		//Validate
uint8_t textp_poly1[] PROGMEM =		"POLYPHONIC";	//Validate
uint8_t textp_poly2[] PROGMEM =		"UNISON";		//Validate
uint8_t textp_poly3[] PROGMEM =		"UNISON MULTI";	//Validate
uint8_t *choices_polymode[4] PROGMEM = {textp_poly0,
									textp_poly1,
									textp_poly2,
									textp_poly3};
