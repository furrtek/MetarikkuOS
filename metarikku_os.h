extern uint8_t MIDIBPut;
extern uint8_t MIDIBGet;
extern uint8_t MIDIBuffer[32];
volatile uint8_t cardPresent;
extern uint8_t refreshlcd;
extern uint8_t MenuSelection;
extern uint8_t message[16];
extern uint8_t PatchNum;
extern uint8_t SDAccess;

void IFH_Test();
void IFH_Run();
//void IFH_Save();
void IFH_Error();
void ResetIF();
void makepptr();
void setreg(uint8_t bank, uint8_t reg, uint8_t data);
void stfu();
uint8_t spicom(uint8_t val);
void savesettings();
void loadsettings();
void savepatch(uint8_t patch, uint8_t ch, uint8_t err);
void loadpatch(uint8_t patch, uint8_t ch, uint8_t err);
void NoteOff(uint8_t ch, uint8_t mnoteo);
void ProcessMIDI();
void RestoreDefaultSettings();
void InitPatch(uint8_t ch, uint8_t silent);
void updatenotes();

#define NOTEONFLASH 4		//Channel LED flash time each time a note on is received

typedef void (*FPointer) ();

typedef struct {
	uint8_t Data;
	uint8_t EOB;
} MIDIBData;

typedef struct {
	uint8_t TotalLevel;
	uint8_t AM;
	uint8_t Attack;
	uint8_t DecayA;
	uint8_t DecayB;
	uint8_t DecayLevel;
	uint8_t Release;
	uint8_t Scaling;
	uint8_t Multiply;
	uint8_t Detune;
} oper;

typedef struct {
	uint8_t PatchName[8];
	uint8_t Feedback;
	uint8_t Algo;
	uint8_t LR;
	uint8_t AMS;
	uint8_t FMS;
	oper Operator[4];
} chan;

extern uint8_t LFOSpeed;
extern uint8_t LFOen;

extern uint16_t note[12];
extern uint8_t param;
extern volatile uint8_t *paramptr;
extern uint8_t polynote[6];
extern volatile uint16_t testtmr;

enum chnmode {
   INDEP,
   POLY,
   UNISON,
   UNISONMT
};

enum choicetype {
   NUMERIC,
   TEXT,
   YESNO
};

enum lparam {
   CH,
   OP,
   LFO,
   SYS,
   PATCH,
   PATCHCH,	//Submenu
   MIDI,
   MIDICH,	//Submenu
   DISP,
   DISPCH,	//Submenu
   DEBUG
};

extern uint8_t eechk;
extern uint8_t OpBank;
extern uint8_t pbank;

extern enum choicetype CurChoiceType;

extern volatile uint8_t *txtbptr;
extern volatile uint8_t *txtmptr;

extern enum chnmode ChMode;
extern enum lparam CurParam;
extern enum lparam OldCurParam;
extern uint8_t parammax;
extern uint8_t MIDICh[6];
extern uint8_t UniDetune;
extern uint8_t UniChs;
extern uint8_t SysMenu;
extern uint8_t needtoplay[6];
extern uint8_t chflash[6];
extern uint8_t needtokill[6];
extern uint8_t unirotate;
extern int16_t mbend[6];
extern uint8_t LastSavedPatch;
extern uint8_t LastSavedPatchBank;

extern uint8_t patchnb[6];
extern uint8_t OpPatchNb;
extern uint8_t OpMIDICh;
extern uint8_t OpChMode;

extern uint8_t updpatch;
extern uint8_t paramlimit;

extern uint8_t midiclk;
extern uint8_t midipulse;
extern uint8_t midipulseto;

extern uint8_t CurOp;
extern uint8_t CurCh;
extern uint8_t CurOpParam;
extern uint8_t CurChParam;
extern uint8_t Volval;

extern uint8_t ifstep;
extern uint8_t BtnA,BtnB,OldBtnA,OldBtnB,Btnf,BtnAActive,BtnBActive;

extern uint8_t first[6];
extern uint8_t dacflash;
extern volatile uint8_t enc;

extern FPointer IFHandler;

extern chan Channels[6];

extern volatile uint8_t count;

typedef struct {
	uint8_t MasterVol;
	uint8_t SampleBank;
	uint8_t DACen;
	uint8_t CH36mode;
	uint8_t BigFont;
	uint8_t Brightness;
	uint8_t LFOSpeed;
	uint8_t LFOen;
	uint8_t Refresh;
	uint8_t MIDICh[6];
	enum chnmode ChMode;
	uint8_t CRC;
} sysparams;

sysparams SystemParams;

extern sysparams EEPROM;
