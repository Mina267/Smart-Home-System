#include "StdTypes.h"
#include "Sensors.h"
#include "TEMP_Filter.h"


static u16 filter_temp;
static u16 Temp_Array[MAX_TEMP_READ];
static u16 TempSum;
static u8 TempP;


void TEMP_FilterInit(void)
{
	u16 temp = TEMP_Read();
	
	for (int TempIndex = FIRST_READ; TempIndex < MAX_TEMP_READ; TempIndex++)
	{
		Temp_Array[TempIndex] = temp;
		TempSum += temp;
	}
}
void TEMP_FilterRunnable(void)
{
	u16 temp = TEMP_Read();
	TempSum -= Temp_Array[TempP];
	Temp_Array[TempP] = temp;
	TempSum += Temp_Array[TempP];
	TempP = (TempP + 1) % MAX_TEMP_READ;
	filter_temp = TempSum / MAX_TEMP_READ;
	
}
u16 TEMP_FilterGetter(void)
{
	return filter_temp;
}