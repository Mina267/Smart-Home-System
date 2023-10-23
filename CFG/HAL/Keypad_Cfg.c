#include "StdTypes.h"
#include "DIO_interface.h"
#include "Keypad_interface.h"



								   	
const u8 KeysArray[ROWS][COLS] =   
{
	{'7', '8', '9', '/'},
	{'4', '5', '6', '*'},
	{'1', '2', '3', '-'},
	{'c', '0', '=', '+'}
};
								
const DIO_Pin_type RowIdx_Array[ROWS] =
										{
											PINB4, /* R1 PIN */
											PINB5, /* R2 PIN */
											PINB6, /* R3 PIN */
											PINB7, /* R4 PIN */
										};
										
const DIO_Pin_type ColIdx_Array[COLS] =
										{
											PIND2, /* C1 PIN */
											PIND3, /* C2 PIN */
											PIND4, /* C3 PIN */
											PIND5, /* C4 PIN */
										};


/*

{ 
	{'7', '8', '9', '/'},	
	{'4', '5', '6', '*'},	
	{'1', '2', '3', '-'},	
	{'c', '0', '=', '+'}	
};

{'7', '8', '9', 'S'},
{'4', '5', '6', 'y'}, 
{'1', '2', '3', '-'}, 
{'c', '0', '=', 'm'}
	
{
	{'7', '8', '9', 'X'},
	{'4', '5', '6', 'D'},
	{'1', '2', '3', 'N'},
	{'S', '0', 'K', 'N'}
};


		{'7', '8', '9', '/'},
		{'4', '1', '6', '*'},
		{'-', '2', '+', '-'},
		{'c', '0', '=', '+'}
	};

*/
