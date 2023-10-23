#include "StdTypes.h"
#include "ADC_interface.h"
#include "Sensors.h"

u16 LIGHT_Read(void)
{
	u16 volt = ADC_ReadVolt(LDR_CH);
	return volt;
}

// Return temperature in C*10.
u16 TEMP_Read(void)
{
	u16 temp = ADC_ReadVolt(LM35_CH);
	return temp;
}



// Return press in KPA * 10.
u16 PRESS_Read(void)
{
	u16 adc = ADC_Read(MPX4115_CH);
	u16 press = (((u32)1000 * (adc - 55)) / 921) + 150;
	return press;
}





// Return temperature in C*10.
void TEMP_Convertion(void)
{
	ADC_StartConversion(LM35_CH);
}
u16 TEMP_GetRead(void)
{
	return READ_VOLT(ADC_GetRead());
}

// Return press in KPA * 10.
void PRESS_Conversion(void)
{
	ADC_StartConversion(MPX4115_CH);
}

u16 PRESS_GetRead(void)
{
	u16 adc = ADC_GetRead();
	u16 press = (((u32)1000 * (adc - 55)) / 921) + 150;
	return press;
}