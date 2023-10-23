#include "StdTypes.h"
#include "MemMap.h"
#include "Utils.h"
#include "Timers.h"
#include "DHT22.h"
#include "DHT22_Private.h"



#define TOP_OCRA	19999


static s8 i = 15;
static u8 Check_Response_flag = 0;
static s8 Read_data = 3;
static u16 Data_Time;
static DHTT22_Status Data_status =  DHT22_FINISH;
static volatile u16 Humidity;
static volatile u16 Temperature;
static volatile u16 Check_Sum;
static volatile u8 OVF_flag = 0;

static void Check_Data(void);
static void Check_Sum_Responce(void);
static void Read_SensorData(void);
static void Count_OV (void);


#include "LCD_interface.h"
#include "DIO_interface.h"
# define F_CPU 8000000
#include <util/delay.h>


void DHT22_Init(void)
{
	// InPut Free.
	CLR_BIT(DDRD, 6);
	CLR_BIT(PORTD, 6);
	Timer1_Init(TIMER1_NORMAL_MODE, TIMER1_SCALER_8);
	Timer1_ICU_InterruptEnable();
	Timer1_InputCaptureEdge(RISING);
	Timer1_ICU_SetCallBack(Check_Data);
	
	/*Timer1_OVF_InterruptEnable();
	Timer1_OVF_SetCallBack(Count_OV);*/
}
void DHT22_StartSignal(void)
{
	
	Humidity = 0;
	Temperature = 0;
	Check_Sum = 0;
	DIO_WritePin(PINC2,LOW);
	
/* trigger to ask for read */
/* Make DIO pin output */
	// OutPut Low
	SET_BIT(DDRD, 6);
	CLR_BIT(PORTD, 6);
	_delay_ms(25);
	// OutPut High.
	SET_BIT(PORTD, 6);
	_delay_us(29);
	// Clear Interrupt Flag.
	SET_BIT(TIFR, ICF1);
	/* Make DIO pin INPFREE for ICU read */
	// InPut Free.
	CLR_BIT(DDRD, 6);
	SET_BIT(PORTD, 6);
	Timer1_InputCaptureEdge(RISING);
	Check_Response_flag = 0;
	Data_status = DHT22_TAKE_READ;

	OVF_flag = 0;
	
	
	
}


DHTT22_Status DHT22_HumidityGetter(u16 *pHumidity)
{
	*pHumidity = Humidity;
	return Data_status;
}
DHTT22_Status DHT22_TemperatureGetter(u16 *pTemperature)
{
	*pTemperature = Temperature;
	return Data_status; 
}




static void Check_Data(void)
{
	static u16 Data_t1;
	static u16 Data_t2;
	static u8 Data_flag = 0;
	
	
	if ((Data_flag == 0) && (Data_status == DHT22_TAKE_READ))
	{
		Data_t1 = ICR1;
		Timer1_InputCaptureEdge(FALLING);
		Data_flag++;
	}
	else if (Data_status == DHT22_TAKE_READ)
	{
		Data_t2 = ICR1;
		Timer1_InputCaptureEdge(RISING);
		Data_flag = 0;
		Data_Time = (Data_t2 - Data_t1);
		
		if (Check_Response_flag == 1)
		{
			Read_SensorData();
		}
		else if (Data_Time <= MAX_WAIT_RESPONCE)
		{
			Check_Response_flag++;
		}
		else
		{
			Data_status = DHT22_ERROR;
		}
	}
	
}




static void Read_SensorData(void)
{
	static u16 data;
	if (Read_data)
	{
		

		if (Data_Time > MAX_WAIT_FOR_BIT)
		{
			Data_status = DHT22_ERROR;
			
		}
		if (Data_Time > MIN_ONE_BIT_TIME)
		{	
			SET_BIT(data, i);
			i--;
		}
		else
		{
			CLR_BIT(data, i);
			i--;
		}
		
		if (i == FINISH_READ_DATA)
		{
			
			if (Read_data == READ_HUMIDITY_DATA)
			{
				
				i = MOST_SIG_16BIT;
				Humidity = data;
				data = 0;
				Read_data = READ_TEMPERATURE_DATA;
			}
			else if (Read_data == READ_TEMPERATURE_DATA)
			{
				
				i = MOST_SIG_8BIT;
				Temperature = data;
				data = 0;
				Read_data = READ_CHECKSUM_DATA;
			}
			else if (Read_data == READ_CHECKSUM_DATA)
			{
				
				Check_Sum = data;
				data = 0;
				Read_data = 0;
				Read_data = READ_HUMIDITY_DATA;
				i = MOST_SIG_16BIT;
				Check_Sum_Responce();
				
			}
		}
	}
}


static void Check_Sum_Responce(void)
{
	u8 sum = (u8)Temperature + (u8)(Temperature >> 8) + (u8)Humidity + (u8)(Humidity >> 8);
	if (Check_Sum == sum)
	{
		Data_status = DHT22_FINISH;
	}
	else
	{
		Data_status = DHT22_ERROR;
	}
}


static void Count_OV (void)
{
	OVF_flag = 1;
	
}



/*

//DIO_WritePin(PINC2,LOW);
//DIO_WritePin(PINC2,HIGH);
//DIO_WritePin(PINC2,LOW);


// RED
DIO_WritePin(PINC1,HIGH);
DIO_WritePin(PINC1,LOW);

// BLUE
DIO_WritePin(PINC0,HIGH);
DIO_WritePin(PINC0,LOW);

*/