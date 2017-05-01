#define FAT_TESTING_ONLY

#define SD_CS_ASSERT     PORTB &= ~_BV(nSS)
#define SD_CS_DEASSERT   PORTB |= _BV(nSS)

#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define CRC_ON_OFF               59

#define ON     1
#define OFF    0

extern volatile unsigned long startBlock;
extern volatile unsigned long totalBlocks;
extern volatile unsigned char buffer[512];

unsigned char SD_init(void);
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg);
unsigned char SD_readSingleBlock(unsigned long startBlock);
unsigned char SD_readSingleBlockWord(unsigned long startBlock);
unsigned char SD_readMultipleBlock (unsigned long startBlock, unsigned long totalBlocks);
unsigned char SD_erase (unsigned long startBlock, unsigned long totalBlocks);
unsigned char SD_writeSingleBlock(unsigned long startBlock);
