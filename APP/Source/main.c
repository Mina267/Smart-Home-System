#include "StdTypes.h"
#include "MemMap.h"
#include "Utils.h"

#include "FireSys.h"





int main(void)
{
	
	DIO_Init();
	LCD_Init();

	sei();
	

	FireSys_Init();
	while (1)
	{
		FireSys_Runnable();
		
	}
	return 0;
}
























