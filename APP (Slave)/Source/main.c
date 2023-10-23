#include "StdTypes.h"
#include "MemMap.h"
#include "Utils.h"

#include "SmartHome.h"





int main(void)
{
	
	DIO_Init();
	LCD_Init();

	sei();
	

	SmartHome_Init();
	while (1)
	{
		
	}
	return 0;
}
























