#include "StdTypes.h"
#include "SmartHome_Cfg.h"


/***************************** RGB MSG Array *****************************/
const c8 *RGB_MSG[RGB_MSG_SIZE] =
{
	"Color not change.",
	"Color Changed.",
	"Wrong color.",
	"Enter Color."
};

/***************************** RGB command Array *****************************/
const c8 *RGB_Commands[RGB_COMMAND_SIZE] =
{
	"BACK",
	"RESET"
};


/***************************** General command Array **************************/
const c8 *GeneralCommands_Array[GENERAL_COMMAND_SIZE] =
{	/* Command */
	"RESET",			/*	0	*/
	"SELECT SYSTEM",	/*	1	*/
};


