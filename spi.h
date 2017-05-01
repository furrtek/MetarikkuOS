unsigned char SPI_tx(unsigned char data);
unsigned char SPI_rx(void);
void SPI_Init(void);

#define SPI_SD             SPCR = 0x52
#define SPI_HIGH_SPEED     SPCR = 0x50; SPSR |= (1<<SPI2X)
