extern uint8_t message[16];

void LCD_Init(void);
void clockLCD(uint8_t arg);
void com(uint8_t command);
void wrtr(char *cbuf);
void wrtc(char chr);
uint8_t hex(uint8_t num);
void LCD_XY(uint8_t x, uint8_t y);
void LCD_Clear(void);
void LCD_Update(void);
void wrt(const uint8_t *FlashLoc);
