


#ifndef SPI_INTERFACE_H_
#define SPI_INTERFACE_H_

typedef enum
{
	MASTER,
	SLAVE,
	}SPI_Mode_type;

typedef enum
{
	FOSC_4_ = 0,
	FOSC_16_,
	FOSC_64_,
	FOSC_128_,
	}SPI_clk_t;
	
typedef enum
{
	NO_DOUBLE_SPEED,
	DOUBLE_SPEED
	}SPI_DoubleSpeed_t;


void SPI_Init(SPI_Mode_type Mode, SPI_clk_t clk, SPI_DoubleSpeed_t speed);
void SPI_SlaveInit(void);
u8 SPI_SendReceive(u8 data);
u8 SPI_ReceivePerodic(u8 *Pdata);
void SPI_SendNoBlock(u8 data);
u8 SPI_ReceiveNoBlock(void);
void SPI_InterruptEnable(void);
void SPI_InterruptDisable(void);
void SPI_SetCallBack(void(*LocalFptr)(void));

#endif /* SPI_INTERFACE_H_ */