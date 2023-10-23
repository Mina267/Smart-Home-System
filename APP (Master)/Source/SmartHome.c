#include "StdTypes.h"
#include "Utils.h"
#include "DIO_interface.h"
#include "UART_Interface.h"
#include "SPI_Interface.h"
#include "SPI_Service.h"
#include "EX_Interrupt.h"
#include "EEPROM_intrface.h"
#include "ADC_interface.h"
#include "Timers.h"
#include "Timers_Services.h"
#include "BCM.h"
#include "SPI_Service.h"
#include "MOTOR_interface.h"
#include "LEDS.h"
#include "RELAY_Interface.h"
#include "LCD_interface.h"
#include "RGB_Interface.h"
#include "SmartHome_Cfg.h"
#include "SmartHome_Private.h"
#include "SmartHome.h"
#include "Kernel_Interface.h"
#include "String_Analysis.h"



/**************** System Variables ************************/
static Sytem_mode_t SystemMode = NO_MODE;
static	bool_t	SelectSysModeMSG_flag = TRUE;
static volatile BCM_RecStr_status_t Rec_status= BCM_NO_STR_TORECIVE;

/************* 1- RGB System variables ********************/
static RGB_mode_status_t RGB_Mode = RGB_NOMODE;
static RGB_Color_type PrevColor = BLACK;
static bool_t RGBSelectMSG_flag = TRUE;
static bool_t SetRGBColorMSG_flag = TRUE;
static bool_t ShowRoomRGBColorMSG_flag = TRUE;

/************* 2- Smart Home System variables *************/
static bool_t SH_EnterTaskMSG_flag = TRUE;
static c8 LCD_Str[MAX_LCD_STR];
static bool_t TempControlMode_flag = FALSE;
static bool_t OutDoorLedMSG_Flag = FALSE;
static bool_t WindowCurtainMSG_Flag = FALSE;
static bool_t GarageDoorMSG_Flag = FALSE;

/**************** String received from user Variables *******************/
static c8 Str_Recived[STR_RECIVED_SIZE];
static c8 StrCommand[STR_COMMAND_SIZE];
static c8 StrTask[STR_TASK_SIZE];
static u32 NumOrder;

/************************ EEPROM ***************************/
Tasks_EEPROM_t Task_save;
static volatile bool_t SaveInEEPROM_Flag = FALSE;
static bool_t SaveNewData_flag = FALSE;

/********************** Smart home Tasks TEXT. **************************/
static const c8 WRG_RoomMSG[] = "Wrong Room!.";
static const c8 WRG_TVmsg[] = "Wrong TV!.";
static const c8 AlreadyOn_MSG[] = "Already On";
static const c8 AlreadyOFF_MSG[] = "Already OFF";
static const c8 ErrorTempControlOn_MSG[] = "Error!Temp Ctrl On";
static const c8 LCRCntrlOn_MSG[] = "LDR!CtrlOn.";
static const c8 Tmpctrlon_MSG[] = "Temp ctrl On.";
static const c8 Tmpctrloff_MSG[] = "Temp ctrl Off.";

/**************************** Task Array **************************************/


/*********** Master Tasks array ***************************/
static const SmartHome_task_t TasksArray[TASK_NUM] = {
	{"LEDON", Leds_On},
	{"LEDOFF", Leds_OFF},
	{"ROOMON", RoomLeds_On},
	{"ROOMOFF", RoomLeds_OFF},
	{"TVON", TV_ON},
	{"TVOFF", TV_OFF},
	{"LCD", LCD_Write},
	{"WINDOW" ,ControlWindow},
	{"GARAGEDOOR" ,GarageDoor},
	{"CURTAIN" ,WindowCurtain},
	{"OUTDOORLEDS" ,OutDoorLeds},	
	{"FANON" ,Fan_ON},
	{"FANOFF" ,Fan_OFF},
	{"ACON" ,AirCondition_ON},
	{"ACOFF" ,AirCondition_OFF},
	{"TEMPCONTROL", Temp_ControlMode}
};

/************ Frame Send & receive from SPI ****************/
static Master_Frame_t SendSPI_MasterFrame;
static Slave_Frame_t ReceiveSPI_SlaveFrame;





void SmartHome_Init(void)
{
	/******* Intialization ******/
	// UART
	UART_Init(UART_9600_BAUDRATE);
	// SPI Master
	SPI_sendFrameInit();
	SPI_Init(MASTER, FOSC_4_, DOUBLE_SPEED);
	// Basic communication protocol.
	BCM_ReceiveStringAshync_Init();
	BCM_SendStringInit();
	// RGB LEDs
	RGB_Init();
	RGB_LedColor(BLACK);

	/******* EEPROM ******/
	// Read system data from EEPROM
	Read_SystemData();
	// Restore System Last status
	Restore_System();
	
	/******* Create Tasks ******/
	
	/* UART send data Task 6.5us */
	RTOS_Error_typeCreateTask(PRIORITY_0_, BCM_SendStringRunnable, BCM_SEND_PERIODICITY , 3);
	/* UART receive data Task 55 us */
	RTOS_Error_typeCreateTask(PRIORITY_1_, Receive_UART_Task, BCM_RECEIVE_PERIODICITY , 0);
	/* SPI transmit And Receive task 40 us */
	RTOS_Error_typeCreateTask(PRIORITY_2_, CallBack_transmitAndReceive_SPIframes, SPI_FRAME_PERIODICITY , 57);
	/* Running of smart home 25us */
	RTOS_Error_typeCreateTask(PRIORITY_3_, SmartHome_Runnable, SMARTHOME_RUN_PERIODICITY , 1);
	/* Running of RGB ShowRoom 19 us */
	RTOS_Error_typeCreateTask(PRIORITY_4_, RGB_ShowRoom_Runnable, SHOWROOM_PERIODICITY , 6);
	/* Response to user from receive SPI frame throw UART 5.5us */
	RTOS_Error_typeCreateTask(PRIORITY_7_, ReceiveFrameStaus_responed, FRAME_RESPONED_PERIODICITY , 2);
	/* EEPROM save data Task 45ms */
	RTOS_Error_typeCreateTask(PRIORITY_8_, Save_SystemData, SAVE_DATA_PERIODICITY , 4);
	
	/******* Start RTOS ******/
	RTOS_voidStart();

	
}

/* SPI transmit And Receive task */
static void CallBack_transmitAndReceive_SPIframes(void)
{
	
	SPI_SendReceiveFrameSynch(SendSPI_MasterFrame.SPI_Array, ReceiveSPI_SlaveFrame.SPI_Array);
	
}

/* UART receive data Task */
static void Receive_UART_Task(void)
{
	Rec_status = BCM_ReceiveStringAshync(Str_Recived);
}

/* Running of smart home */
void SmartHome_Runnable(void)
{
	
	switch (SystemMode)
	{
		case NO_MODE:
		TakeSystemMode();
		break;
		
		case RGB_MODE:
		RGB_SystemMode();
		break;
		
		case SMARTHOME_MODE:
		SmartHome_SystemMode();
		break;
	}
		
}




/*************************************************************************************************/
/******************************** Select Operation Functions *************************************/
/*************************************************************************************************/

// Take System Operating Mode as default.
static void TakeSystemMode(void)
{
	if (SelectSysModeMSG_flag)
	{
		BCM_SendStringSetter("1-RGB/2-Home");
		SelectSysModeMSG_flag = FALSE;
	}
	
	
	if (Rec_status == BCM_STR_RECEVIED)
	{
		Rec_status = BCM_STR_READED;
		if (Str_cmp(Str_Recived, "1") == STR_MATCH)
		{
			
			SystemMode = RGB_MODE;
		}
		else if (Str_cmp(Str_Recived, "2") == STR_MATCH)
		{
			SystemMode = SMARTHOME_MODE;
		}
		else
		{
			SendWrongChoice_MSG();
		}
	}
}

// Reset all System.
static void Sytem_Reset(void)
{
	// Disable send Message.
	BCM_SendString_Disable();
	
	// Select operation variables Reset
	SystemMode = NO_MODE;
	SelectSysModeMSG_flag = TRUE;
	
	// RGB Reset
	RGB_Reset();
	
	// Smart Home Reset
	SmartHome_Reset();
	
	// EEPROM Reset
	EEPROM_reset();
	
	// Enable send Message.
	BCM_SendString_Enable();
}
// Return to select system mode menu.
static void Sytem_SelectMode(void)
{
	// Select operation variables Reset
	SystemMode = NO_MODE;
	SelectSysModeMSG_flag = TRUE;
	
	// RGB Back
	RGB_Back();
	
	// Smart Home Back
	SmartHome_Back();
}



/************************************************ - 1 - **********************************************/
/***************************************** RGB Mode Functions ****************************************/
/*****************************************************************************************************/


// Function Operate as RGB mode Selected.
static void RGB_SystemMode(void)
{
	// Switch according to mode selected
	
	
	if (RGB_Mode == RGB_NOMODE)
	{
		TakeRGBSystemMode();
	}
	else
	{
		RGB_colorTakeCommand();
	}
	
}

// Take RGB mode User Want to operate with (SelectColor/ShowRoom).
static void TakeRGBSystemMode(void)
{
	// To Send MSG only one time
	if (RGBSelectMSG_flag)
	{
		BCM_SendStringSetter("1-Color/2-ShowRoom");
		RGBSelectMSG_flag = FALSE;
	}
	
	// Wait to take user choice which mode want to select from RGB mode
	if (Rec_status == BCM_STR_RECEVIED)
	{
		// Variable to synchronize with receive function from UART that message read.
		Rec_status = BCM_STR_READED;
		
		// Change RGB mode according user selection
		if (Str_cmp(Str_Recived, "1") == STR_MATCH)
		{
			// Set RGB mode to Set color directly mode.
			RGB_Mode = RGB_SETCOLOR;
		}
		else if (Str_cmp(Str_Recived, "2") == STR_MATCH)
		{
			// Set RGB mode to Set color in showRoom mode.
			RGB_Mode = RGB_SHOWROOM;
		}
		else
		{
			// If user Enter Wrong choice send to user wrong Choice MSG.
			SendWrongChoice_MSG();
		}
	}
}


// Take user Command for RGB mode
static void RGB_colorTakeCommand(void)
{
	// To Send MSG only one time
	if (SetRGBColorMSG_flag)
	{
		SendRGB_MSG(RGB_ENTER_COLOR_MSG);
		SetRGBColorMSG_flag = FALSE;
	}
	
	
	if (Rec_status == BCM_STR_RECEVIED)
	{
		// Variable to synchronize with receive function from UART that message read
		Rec_status = BCM_STR_READED;
		// Check if user first Enter General command by check for '/'.
		if (Str_Recived[FIRST_CHAR] == '/')
		{
			StrAnalysis_GeneralCommands();
		}
		else
		{
			// Analysis MSG receive from user.
			RGB_CommandStatus_t CommandStatus = StrAnalysisRGBMode(Str_Recived, StrCommand, StrTask);
			// If user enter wrong Command.
			if (CommandStatus == RGB_WEONG_COMMAND)
			{
				SendWrongCommand_MSG();
			}
			// Execute Command received from user.
			else
			{
				RGB_ExecuteCommand();
				// Save new status that executed in EEPROM.
				SaveNewData_flag = TRUE;
			}
			
		}
	}
}

// Execute Command received from user.
static void RGB_ExecuteCommand(void)
{
	// If user Want To Go back or Reset RGB Setting
	// By Enter "RGB_BACK" or "RGB_RESET"
	if (Str_cmp(StrTask, (c8*)RGB_Commands[RGB_RESET_CMD]) == STR_MATCH)
	{
		RGB_Reset();
	}
	else if (Str_cmp(StrTask, (c8*)RGB_Commands[RGB_BACK_CMD]) == STR_MATCH)
	{
		RGB_Back();
	}
	// Set color of RGB User Entered.
	else if (RGB_Mode == RGB_SETCOLOR)
	{
		RGB_SetColorExecuteCommand();
	}
	// Show Room color of RGB User Entered.
	else if (RGB_Mode == RGB_SHOWROOM)
	{
		RGB_ShowRoomExecuteCommand();
	}
	
	// Return RGB color MSG to True again to resend MSG
	SetRGBColorMSG_flag = TRUE;
	// Save previous color in task array to save in EEPROM
	Task_save.PrevColor = PrevColor;
}

/************************************** RGB set Color ********************************************/

// Execute String command user Input of set color.
static void RGB_SetColorExecuteCommand(void)
{
	// Convert User color name string to its index in RGB Drive.
	RGB_Color_type color = RGB_LedColorIndex(StrTask);
	// If this color is not Existing in RGB color "Send Wrong! Color."
	if (color == COLOR_NUM)
	{
		// Return color to previous value
		color = PrevColor;
		SendRGB_MSG(RGB_WRG_COLOR_MSG);
	}
	else
	{
		SendRGB_MSG(RGB_COLOR_CHG_MSG);
		RGB_LedColor(color);
	}
	
	// assign previous color to showRoom uses and To save in EEPROM when system start again.
	PrevColor = color;
	
}


/************************************** RGB Show Room Color ********************************************/


// Execute String command user Input of show room.
static void RGB_ShowRoomExecuteCommand(void)
{
	// Convert User color name string to its index in RGB Drive.
	RGB_Color_type color = RGB_LedColorIndex(StrTask);
	// If this color is not Existing in RGB color "Send Wrong! Color."
	if (color == COLOR_NUM)
	{
		// Return color to previous value.
		color = PrevColor;
		SendRGB_MSG(RGB_WRG_COLOR_MSG);
	}
	
	/* SHOW ROOM Function */
	RGB_Status showStatus = RGB_ShowRoomSetColors(PrevColor ,color);
	// If show Room did not finish previous Work.
	if ((showStatus == RGB_NDONE) && (color != PrevColor))
	{
		SendRGB_MSG(RGB_COLOR_NOTCHG_MSG);
	}
	else if (color != PrevColor)
	{
		SendRGB_MSG(RGB_COLOR_CHG_MSG);
	}
	
	// assign previous color to showRoom uses and To save in EEPROM when system start again.
	PrevColor = color;
}



// Reset RGB as default
static void RGB_Reset(void)
{
	RGB_LedColor(BLACK);
	RGB_Mode = RGB_NOMODE;
	PrevColor = BLACK;
	Task_save.PrevColor = PrevColor;
	RGBSelectMSG_flag = TRUE;
	SetRGBColorMSG_flag = TRUE;
	ShowRoomRGBColorMSG_flag = TRUE;
}
// Back To select RGB mode;
static void RGB_Back(void)
{
	RGB_Mode = RGB_NOMODE;
	RGBSelectMSG_flag = TRUE;
	SetRGBColorMSG_flag = TRUE;
	ShowRoomRGBColorMSG_flag = TRUE;
	
}



/********************************************** - 2 - ************************************************/
/************************************ Smart Home Mode Functions **************************************/
/*****************************************************************************************************/



// Function Operate as Smart Home mode Selected.
// Take Task User want to execute.
static void SmartHome_SystemMode(void)
{
	// To Send MSG only one time
	if (SH_EnterTaskMSG_flag)
	{
		BCM_SendStringSetter("Enter Task.");
		SH_EnterTaskMSG_flag = FALSE;
	}
	
	if (Rec_status == BCM_STR_RECEVIED)
	{
		// Variable to synchronize with receive function from UART that message read
		Rec_status = BCM_STR_READED;
		// Check if user first Enter General command by check for '/'.
		if (Str_Recived[FIRST_CHAR] == '/')
		{
			StrAnalysis_GeneralCommands();
		}
		else
		{
			// To show "Enter task" MSG again.
			SH_EnterTaskMSG_flag = TRUE;
			// Analysis MSG receive from user.
			SH_CommandStatus_t CommandStatus = StrAnalysis_SmartHomeMode(Str_Recived, StrCommand, &NumOrder, LCD_Str);
			// If user enter wrong Command.
			if (CommandStatus == SH_WEONG_COMMAND)
			{
				SendWrongCommand_MSG();
			}
			// Execute Task of Specific order from array
			else
			{
				SmartHome_ExecuteCommand();
				// Save new status that executed in EEPROM.
				SaveNewData_flag = TRUE;
			}
			
		}
	}
}

// Search in task array for task order.
static TASKS_Status_t SearchForTaskOrder(u16 *TaskIndex)
{
	// Convert String to upper case to compare string receive ceaseless.
	Str_toUpper(StrCommand);
	TASKS_Status_t taskStatus = TASK_NOT_FOUND;
	u8 Index;
	for (Index = 0; Index < TASK_NUM; Index++)
	{
		// If task receive found in task array return TASK INDEX.
		if (Str_cmp(StrCommand, (c8*)TasksArray[Index].str) == STR_MATCH)
		{
			*TaskIndex = Index;
			taskStatus = TASK_FOUND;
		}
	}
	return taskStatus;
	
}

// Execute Task of Specific order from array
static void SmartHome_ExecuteCommand(void)
{
	u16 TaskIndex;
	// Search in task array for task order.
	TASKS_Status_t taskStatus = SearchForTaskOrder(&TaskIndex);
	// If Task found in array.
	if (taskStatus == TASK_FOUND)
	{
		// Execute task function from Array.
		TasksArray[TaskIndex].Fptr(NumOrder);
		BCM_SendStringSetter("Task Executed.");
	}
	// If Task NOT found in array.
	else if (taskStatus == TASK_NOT_FOUND)
	{
		BCM_SendStringSetter("Wrong! Task.");
	}
}


// Reset Smart Home as default
static void SmartHome_Reset(void)
{
	// To Send MSG again
	SH_EnterTaskMSG_flag = TRUE;
	// Reset SPI Frame.
	SendSPI_MasterFrame.AirConditiondegree = 0;
	SendSPI_MasterFrame.FanSpeed = 0;
	SendSPI_MasterFrame.DeviceStatus = 0;
	SendSPI_MasterFrame.WindowPostion = 0;
	// close all LEDs
	Leds_OFF(0);
	// close all room 
	RoomLeds_OFF(0);
	// close all TV
	TV_OFF(0);
	// Clear LCD
	LCD_Clear();
	// Turn off temperature Control mode.
	Temp_ControlMode(0);
	// Turn off LDR Control mode.
	CLR_BIT(SendSPI_MasterFrame.DeviceStatus, LDR_CONTROL);
	// Close Window.
	ControlWindow(0);
	// Turn off Out Door LEDs.
	OutDoorLeds(0);
	// Close AirCondition.
	AirCondition_OFF(0);
	// Close Fan.
	Fan_OFF(0);
	// Delete commands
	Str_Recived[FIRST_CHAR] = NULL;
	StrCommand[FIRST_CHAR] = NULL;
	StrTask[FIRST_CHAR] = NULL;
	
}
// Back To select system mode.
static void SmartHome_Back(void)
{
	SH_EnterTaskMSG_flag = TRUE;
	Str_Recived[FIRST_CHAR] = NULL;
	StrCommand[FIRST_CHAR] = NULL;
	StrTask[FIRST_CHAR] = NULL;
}




/********************** Smart home Tasks Functions. **************************/


// Enter Number of led to turn On
static void Leds_On(u16 LED)
{
	LEDS_types ledColor = LED - 1;
	// Select Led to turn it on. 
	// Save it in task Save array For EEPROM.
	if ((LED <= MAX_NUMOF_LED) && LED != 0)
	{
		LEDS_On(ledColor);
		switch(ledColor)
		{
			case RED_LED:
			BCM_SendStringSetter("RedON.");
			Task_save.Red = 1;
			break;
			
			case GRN_LED:
			BCM_SendStringSetter("GrnON.");
			Task_save.Grn = 1;
			break;
			
			case BLU_LED:
			BCM_SendStringSetter("BlueON.");
			Task_save.Blu = 1;
			break;
			
			default:
			;
		}
	}
	// Select All Led to turn it on.
	else if (LED == 0)
	{
		LEDS_On(RED_LED);
		LEDS_On(GRN_LED);
		LEDS_On(BLU_LED);
		BCM_SendStringSetter("LEDs ON.");
		Task_save.Red = 1;
		Task_save.Grn = 1;
		Task_save.Blu = 1;
	}
	// Wrong Led number.
	else
	{
		BCM_SendStringSetter("Wrong!Led.");
	}
}





// Enter Number of led to turn OFF
static void Leds_OFF(u16 LED)
{
	LEDS_types ledColor = LED - 1;
	// Enter Number of led to turn Off
	// Save it in task Save array For EEPROM.
	if ((LED <= MAX_NUMOF_LED) && LED != 0)
	{
		LEDS_Off(ledColor);
		switch(ledColor)
		{
			case RED_LED:
			BCM_SendStringSetter("RedOFF.");
			Task_save.Red = 0;
			break;
			
			case GRN_LED:
			BCM_SendStringSetter("GrnOFF.");
			Task_save.Grn = 0;
			break;
			
			case BLU_LED:
			BCM_SendStringSetter("BlueOFF.");
			Task_save.Blu = 0;
			break;
			
			default:
			;
		}
	}
	// Select All Led to turn it off.
	else if (LED == 0)
	{
		LEDS_Off(RED_LED);
		LEDS_Off(GRN_LED);
		LEDS_Off(BLU_LED);
		BCM_SendStringSetter("LEDs OFF.");
		Task_save.Red = 0;
		Task_save.Grn = 0;
		Task_save.Blu = 0;
	}
	// Wrong Led number.
	else
	{
		BCM_SendStringSetter("Wrong!Led.");
	}
}


// Enter Number of Room to turn its Led On.
// Turn all Rooms LED On but zero or no number.
static void RoomLeds_On(u16 Room)
{
	// Number of Room to turn On.
	// Save it in task Save array For EEPROM.
	switch(Room)
	{
		// All rooms light on.
		case 0:
		DIO_WritePin(ROOM1_LEDS, HIGH);
		DIO_WritePin(ROOM2_LEDS, HIGH);
		DIO_WritePin(ROOM3_LEDS, HIGH);
		DIO_WritePin(ROOM4_LEDS, HIGH);
		BCM_SendStringSetter("Rooms On.");
		Task_save.Room1 = 1;
		Task_save.Room2 = 1;
		Task_save.Room3 = 1;
		Task_save.Room4 = 1;
		break;
		
		case 1:
		DIO_WritePin(ROOM1_LEDS, HIGH);
		BCM_SendStringSetter("Room1 On.");
		Task_save.Room1 = 1;
		break;
		
		case 2:
		DIO_WritePin(ROOM2_LEDS, HIGH);
		BCM_SendStringSetter("Room2 On.");
		Task_save.Room2 = 1;
		break;
		
		case 3:
		DIO_WritePin(ROOM3_LEDS, HIGH);
		BCM_SendStringSetter("Room3 On.");
		Task_save.Room3 = 1;
		break;
		
		case 4:
		DIO_WritePin(ROOM4_LEDS, HIGH);
		BCM_SendStringSetter("Room4 On.");
		Task_save.Room4 = 1;
		break;
		
		// Wrong Room.
		default:
		BCM_SendStringSetter((c8*)WRG_RoomMSG);
	}
}


// Enter Number of Room to turn its Led Off.
// Turn all Rooms LED OFF but zero or no number.
static void RoomLeds_OFF(u16 Room)
{
	// Number of Room to turn Off.
	// Save it in task Save array For EEPROM.
	switch(Room)
	{
		// All rooms light off.
		case 0:
		DIO_WritePin(ROOM1_LEDS, LOW);
		DIO_WritePin(ROOM2_LEDS, LOW);
		DIO_WritePin(ROOM3_LEDS, LOW);
		DIO_WritePin(ROOM4_LEDS, LOW);
		Task_save.Room1 = 0;
		Task_save.Room2 = 0;
		Task_save.Room3 = 0;
		Task_save.Room4 = 0;

		BCM_SendStringSetter("Rooms Off.");
		break;
		
		case 1:
		DIO_WritePin(ROOM1_LEDS, LOW);
		BCM_SendStringSetter("Room1 Off.");
		Task_save.Room1 = 0;
		break;
		
		case 2:
		DIO_WritePin(ROOM2_LEDS, LOW);
		BCM_SendStringSetter("Room2 Off.");
		Task_save.Room2 = 0;
		break;
		
		case 3:
		DIO_WritePin(ROOM3_LEDS, LOW);
		BCM_SendStringSetter("Room3 Off.");
		Task_save.Room3 = 0;
		break;
		
		case 4:
		DIO_WritePin(ROOM4_LEDS, LOW);
		BCM_SendStringSetter("Room 4 Off.");
		Task_save.Room4 = 0;
		break;
		
		// Wrong Room.
		default:
		BCM_SendStringSetter((c8*)WRG_RoomMSG);
	}
}





// Enter number of TV to turn on
// Turn all TVs on but zero or no number.
static void TV_ON(u16 TV)
{
	RELAY_t Tv_Relay = TV;
	
	// Check if TV. number within Range.
	// Save it in task Save array For EEPROM.
	if ((TV > 0) && (TV <= TV_NUM))
	{
		// 
		RELAY_On(Tv_Relay);
		switch(Tv_Relay)
		{
			case _1_TV_RELAY:
			BCM_SendStringSetter("TV1On.");
			Task_save.TV1 = 1;
			break;
			
			case _2_TV_RELAY:
			BCM_SendStringSetter("TV2On.");
			Task_save.TV2 = 1;
			break;
			
			default:
			;
		}
	}
	// Turn all TVs on.
	else if (TV == 0)
	{
		RELAY_On(RELAY_ALL);
		BCM_SendStringSetter("TVs On.");
		Task_save.TV1 = 1;
		Task_save.TV2 = 1;
	}
	// Wrong TV. number.
	else
	{
		BCM_SendStringSetter((c8*)WRG_TVmsg);
	}
}

// Enter number of TV to turn off
// Turn all TVs off but zero or no number.
static void TV_OFF(u16 TV)
{
	RELAY_t Tv_Relay = TV;
	
	// Check if TV. number within Range.
	// Save it in task Save array For EEPROM.
	if ((TV > 0) && (TV <= TV_NUM))
	{
		RELAY_Off(Tv_Relay);
		switch(Tv_Relay)
		{
			case _1_TV_RELAY:
			BCM_SendStringSetter("TV1Off.");
			Task_save.TV1 = 0;
			break;
			
			case _2_TV_RELAY:
			BCM_SendStringSetter("TV2Off.");
			Task_save.TV2 = 0;
			break;
			
			default:
			;
		}
	}
	// Turn all TVs off.
	else if (TV == 0)
	{
		RELAY_Off(RELAY_ALL);
		BCM_SendStringSetter("TVs Off.");
		Task_save.TV1 = 0;
		Task_save.TV2 = 0;
	}
	// Wrong TV. number.
	else
	{
		BCM_SendStringSetter((c8*)WRG_TVmsg);
	}
	
	
}



// Write String on LCD
static void LCD_Write(u16 n)
{
	if (n == 0)
	{
		// Clear LCD and write new string.
		LCD_Clear();
		LCD_WriteStringOverFlow(0 ,0, LCD_Str);
	}
	
}

// The position in degree or windows open
static void ControlWindow(u16 Positon)
{
	// If position of window less than 180 degree.
	if (Positon <= 180)
	{
		SendSPI_MasterFrame.WindowPostion = Positon;
	}
	// If position of window more than 180 degree adjust to 180.
	else
	{
		SendSPI_MasterFrame.WindowPostion = 180;
	}
	
}




// 0 for Close 1 for open
static void GarageDoor(u16 Status)
{	
	// Detect the Status of the Garage door from received salve if it opens or close.?
	u8 GarageDoor_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, GRAGEDOOR);
	if (Status == DOOR_CLOSE)
	{
		// Close door will it already Close.
		if (GarageDoor_rec == DOOR_CLOSE)
		{
			BCM_SendStringSetter((c8*)AlreadyOFF_MSG);
		}
		// Close door
		else
		{
			CLR_BIT(SendSPI_MasterFrame.DeviceStatus, GRAGEDOOR);
			GarageDoorMSG_Flag = TRUE;
		}
	}
	else if (Status == DOOR_OPEN)
	{
		// Open door will it already open.
		if (GarageDoor_rec == DOOR_OPEN)
		{
			BCM_SendStringSetter((c8*)AlreadyOn_MSG);
		}
		// Open door
		else
		{
			SET_BIT(SendSPI_MasterFrame.DeviceStatus, GRAGEDOOR);
			GarageDoorMSG_Flag = TRUE;
		}
	}
}



// Open and close window Curtain
static void WindowCurtain(u16 Status)
{
	// Detect the Status of the CURTAIN from received salve if it opens or close.?
	u8 WindowCurtain_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, WINDOW_CURTION);
	if (Status == CURTAIN_CLOSE)
	{
		// Close Curtain will it already Close.
		if (WindowCurtain_rec == CURTAIN_CLOSE)
		{
			BCM_SendStringSetter((c8*)AlreadyOFF_MSG);
		}
		// Close Curtain.
		else
		{
			CLR_BIT(SendSPI_MasterFrame.DeviceStatus, WINDOW_CURTION);
			WindowCurtainMSG_Flag = TRUE;
		}
	}
	else if (Status == CURTAIN_OPEN)
	{
		// OPen Curtain will it already OPen.
		if (WindowCurtain_rec == CURTAIN_OPEN)
		{
			BCM_SendStringSetter((c8*)AlreadyOn_MSG);
		}
		// Open Curtain.
		else
		{
			SET_BIT(SendSPI_MasterFrame.DeviceStatus, WINDOW_CURTION);
			WindowCurtainMSG_Flag = TRUE;
		}
	}
}







// Status 0 Turn outdoor led off
// Status 1 Turn outdoor led on
// Status 2 Turn LDR control mode on/off.
static void OutDoorLeds(u16 Status)
{
	// Read status of outdoor led from received salve frame if it on/off.
	u8 Outdoorled_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, OUTDOORLEDS);
	// Read status of LDR control from received salve frame if it on/off.
	u8 LDR_control_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, LDR_CONTROL);
	
	// If user want to turn outdoor led on and LDR control mode on.
	// Send error MSG that user can not control outdoor LEDs will LDR mode on.
	if ((LDR_control_rec == LDR_MODE_ON) && (Status != LDR_MODE_STATE))
	{
		BCM_SendStringSetter((c8*)LCRCntrlOn_MSG);
	}
	// User want to close Outdoor LEDs
	else if (Status == LEDS_CLOSE)
	{
		// Close Outdoor LEDs will it already Close.
		if (Outdoorled_rec == LEDS_CLOSE)
		{
			BCM_SendStringSetter((c8*)AlreadyOFF_MSG);
		}
		// close Outdoor LEDs
		else
		{
			CLR_BIT(SendSPI_MasterFrame.DeviceStatus, OUTDOORLEDS);
			OutDoorLedMSG_Flag = TRUE;
		}
	}
	// User want to open Outdoor LEDs
	else if (Status == LEDS_OPEN)
	{
		// OPen Outdoor LEDs will it already OPen.
		if (Outdoorled_rec == LEDS_OPEN)
		{
			BCM_SendStringSetter((c8*)AlreadyOn_MSG);
		}
		// open Outdoor LEDs
		else
		{
			SET_BIT(SendSPI_MasterFrame.DeviceStatus, OUTDOORLEDS);
			OutDoorLedMSG_Flag = TRUE;
		}
	}
	// Open LDR control mode
	else if (Status == LDR_MODE_STATE)
	{
		if (LDR_control_rec == LDR_MODE_OFF)
		{
			// Turn on Outdoor LEDs set its bit in SPI frame send to slave.
			SET_BIT(SendSPI_MasterFrame.DeviceStatus, LDR_CONTROL);
			BCM_SendStringSetter((c8*)LCRCntrlOn_MSG);
		}
		else
		{
			// Turn off Outdoor LEDs Clear its bit in SPI frame send to slave.
			CLR_BIT(SendSPI_MasterFrame.DeviceStatus, LDR_CONTROL);
			BCM_SendStringSetter("LCD CtrlOFF.");
		}
	}
	
}




// Fan Speed
static void Fan_ON(u16 Speed)
{
	// Read status of Temp Control Mode from received salve frame if it on/off.
	u8 TempControlMode_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, TEMPCONTROL);
	// If Temp Control Mode is on refuse to change Fan speed or turn it on.
	if (TempControlMode_rec == TEMP_CONTROL_ON)
	{
		BCM_SendStringSetter((c8*) ErrorTempControlOn_MSG);
	}
	// Change Fan Speed
	else
	{
		// Turn on fan set its bit in SPI frame send to slave.
		SET_BIT(SendSPI_MasterFrame.DeviceStatus, FAN);
		
		// Set speed on fan max = 100 %.
		// And set speed of fan in SPI frame send to slave to speed variable.
		if (Speed <= 100)
		{
			SendSPI_MasterFrame.FanSpeed = Speed;
		}
		else
		{
			SendSPI_MasterFrame.FanSpeed = 100;
		}
	}
	
	
}


// Turn Fan off
static void Fan_OFF(u16 Speed)
{
	// Read status of Temp Control Mode from received salve frame if it on/off.
	u8 TempControlMode_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, TEMPCONTROL);
	// If Temp Control Mode is on refuse to turn off fan.
	if (TempControlMode_rec == TEMP_CONTROL_ON)
	{
		BCM_SendStringSetter((c8*) ErrorTempControlOn_MSG);
	}
	else
	{
		// If air Fan already on do nothing
		if (READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, FAN) == 0)
		{
			BCM_SendStringSetter((c8*)AlreadyOFF_MSG);
		}
		else if (Speed == 0)
		{
			// Turn off Fan Clear its bit in SPI frame send to slave.
			// And set speed of fan in SPI frame send to slave to zero.
			CLR_BIT(SendSPI_MasterFrame.DeviceStatus, FAN);
			SendSPI_MasterFrame.FanSpeed = 0;
		}
	}
}

// Set Degree of air Condition.
static void AirCondition_ON(u16 Degree)
{
	// Read status of Temp Control Mode from received salve frame if it on/off.
	u8 TempControlMode_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, TEMPCONTROL);
	// If Temp Control Mode is on refuse to change Air Condition degree or turn it on.
	if (TempControlMode_rec == TEMP_CONTROL_ON)
	{
		BCM_SendStringSetter((c8*) ErrorTempControlOn_MSG);
	}
	else
	{
		// Turn on Air Condition by set its bit in SPI frame send to slave.
		SET_BIT(SendSPI_MasterFrame.DeviceStatus, AIRCONDITION);
		// Set air condition degree Max = 28, Min = 18. 
		if ((Degree >= 18) && (Degree <= 28))
		{
			SendSPI_MasterFrame.AirConditiondegree = Degree;
		}
		else if ((Degree < 18))
		{
			SendSPI_MasterFrame.AirConditiondegree = 18;
		}
		else
		{
			SendSPI_MasterFrame.AirConditiondegree = 28;
		}
	}
}

// Close of air Condition.
static void AirCondition_OFF(u16 Degree)
{
	// Read status of Temp Control Mode from received salve frame if it on/off.
	u8 TempControlMode_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, TEMPCONTROL);
	// If Temp Control Mode is on refuse to turn it off.
	if (TempControlMode_rec == TEMP_CONTROL_ON)
	{
		BCM_SendStringSetter((c8*) ErrorTempControlOn_MSG);
	}
	else
	{
			
		if (Degree == 0)
		{
			// If air condition already on do nothing
			if (READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, AIRCONDITION) == 0)
			{
				BCM_SendStringSetter((c8*)AlreadyOFF_MSG);
			}
			else
			{
				// Turn on Air Condition by clear its bit in SPI frame send to slave.
				CLR_BIT(SendSPI_MasterFrame.DeviceStatus, AIRCONDITION);
				SendSPI_MasterFrame.AirConditiondegree = 0;
			}
		}
	}
}


// Make temp sensor control operation of Air condition and fan
static void Temp_ControlMode(u16 mode)
{
	// Read status of Temp Control Mode from received salve frame if it on/off.
	u8 TempControlMode_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, TEMPCONTROL); 
	
	// Turn on Temperature control mode.
	if (mode == TEMP_CONTROL_ON)
	{
		// If TEMP CONTROL ON already on do nothing
		if (TempControlMode_rec == TEMP_CONTROL_ON)
		{
			BCM_SendStringSetter((c8*)AlreadyOn_MSG);
		}
		else
		{
			// Turn on temp control mode.
			TempControlMode_flag = TRUE;
			SET_BIT(SendSPI_MasterFrame.DeviceStatus, TEMPCONTROL);
			BCM_SendStringSetter((c8*)Tmpctrlon_MSG);
		}
		
	}
	// Turn off Temperature control mode.
	else if (mode == TEMP_CONTROL_OFF)
	{
		// If TEMP CONTROL Off already on do nothing
		if (TempControlMode_rec == TEMP_CONTROL_OFF)
		{
			BCM_SendStringSetter((c8*)AlreadyOFF_MSG);
		}
		else
		{
			// Turn off temp control mode.
			TempControlMode_flag = FALSE;
			CLR_BIT(SendSPI_MasterFrame.DeviceStatus, TEMPCONTROL);
			BCM_SendStringSetter((c8*)Tmpctrloff_MSG);
		}
		
	}
}



/*
 * Send message when task in salve MCU executed.
 * Send message to user when task in salve is executed
 * Monitor task status in slave by SPI frame received from it.
 * Compare Order send from Master to status receive from salve
 * When status receive from slave change as master order Send Message to notify user. 
 */
static void ReceiveFrameStaus_responed(void)
{
	DIO_WritePin(PINA0, HIGH);
	// message for out door LEDs Status.
	if (OutDoorLedMSG_Flag)
	{
		
		u8 Outdoorled_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, OUTDOORLEDS);
		u8 Outdoorled_Send = READ_BIT(SendSPI_MasterFrame.DeviceStatus, OUTDOORLEDS);
		// Compare and Decided.
		if ((Outdoorled_rec == 1) && (Outdoorled_Send == 1))
		{
			BCM_SendStringSetter("OutLeds On");
			OutDoorLedMSG_Flag = FALSE;
		}
		else if ((Outdoorled_rec == 0) && (Outdoorled_Send == 0))
		{
			BCM_SendStringSetter("OutLeds Off");
			OutDoorLedMSG_Flag = FALSE;
		}
	}
	if (WindowCurtainMSG_Flag)
	{
		u8 WindowCurtain_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, WINDOW_CURTION);
		u8 WindowCurtain_Send = READ_BIT(SendSPI_MasterFrame.DeviceStatus, WINDOW_CURTION);
		// Compare and Decided.
		if ((WindowCurtain_rec == 1) && (WindowCurtain_Send == 1))
		{
			BCM_SendStringSetter("Curtain Open");
			WindowCurtainMSG_Flag = FALSE;
		}
		else if ((WindowCurtain_rec == 0) && (WindowCurtain_Send == 0))
		{
			BCM_SendStringSetter("Curtain close");
			WindowCurtainMSG_Flag = FALSE;
		}
	}

	if (GarageDoorMSG_Flag)
	{
		u8 GarageDoor_rec = READ_BIT(ReceiveSPI_SlaveFrame.DeviceStatus, GRAGEDOOR);
		u8 GarageDoor_Send = READ_BIT(SendSPI_MasterFrame.DeviceStatus, GRAGEDOOR);
		// Compare and Decided.
		if ((GarageDoor_rec == 1) && (GarageDoor_Send == 1))
		{
			BCM_SendStringSetter("Garage Open");
			GarageDoorMSG_Flag = FALSE;
		}
		else if ((GarageDoor_rec == 0) && (GarageDoor_Send == 0))
		{
			BCM_SendStringSetter("Garage close");
			GarageDoorMSG_Flag = FALSE;
		}
	}
	
	DIO_WritePin(PINA0, LOW);
	
}


/*************************************************************************************************/
/******************************** System General Command *****************************************/
/*************************************************************************************************/
// Can be use in any mode of system operate.
static void StrAnalysis_GeneralCommands(void)
{
	GeneralCommand_t CommandIndex;
	bool_t WrongCommad_flag = TRUE;
	// Search for general command in array.
	for (CommandIndex = 0; CommandIndex < GENERAL_COMMAND_NUM; CommandIndex++)
	{
		STR_Status StrStatus = Str_cmp(Str_Recived + 1 , (c8*)GeneralCommands_Array[CommandIndex]);
		// If command found execute command.
		if (StrStatus == STR_MATCH)
		{
			WrongCommad_flag = FALSE;
			BCM_SendStringSetter("Gnrl command Executed");
			GeneralCommands_TakeAction(CommandIndex);
		}
	}
	
	// If command not found send error message.
	if (WrongCommad_flag)
	{
		SendWrongCommand_MSG();
	}
}


// Take action according to general command order.
static void GeneralCommands_TakeAction(GeneralCommand_t GeneralCommandNumber)
{
	switch (GeneralCommandNumber)
	{
		case RESET:
		Sytem_Reset();
		break;
		
		case SELECT_SYSTEM:
		Sytem_SelectMode();
		break;
		
		case GENERAL_COMMAND_NUM:
		break;
	}
}



/*************************************************************************************************/
/**************************** Utils Fuctions *****************************************************/
/*************************************************************************************************/



// Send Wrong Choice MSG
static void SendWrongChoice_MSG(void)
{
	BCM_SendStringSetter("Wrong! Choice");
}
// Send Wrong Command MSG
static void SendWrongCommand_MSG(void)
{
	BCM_SendStringSetter("Wrong! Command");
}
// Send MSG for RGB mode
static void SendRGB_MSG(u8 SelectMSG)
{
	BCM_SendStringSetter((c8*)RGB_MSG[SelectMSG]);
}



/***************************** Save system in EEPROM Functions **********************************/



// Save system data in EEPROM for next start.
static void Save_SystemData(void)
{
	// Save system data when there is new data to save.
	if (SaveNewData_flag)
	{
		// Save SPI frame for slave status.
		EEPROM_writeBusy(START_SLAVE_ADDRESS, ReceiveSPI_SlaveFrame.DeviceStatus);
		
		// Save Master SPI order frame.
		u16 FrameAddress = START_FRAME_ADDRESS;
		for (u8 Index = FIRST_FRAME_INDEX; Index < SMARTHOME_FRAME_SIZE; Index++)
		{
			EEPROM_writeBusy(FrameAddress, SendSPI_MasterFrame.SPI_Array[Index]);
			FrameAddress++;
		}
		
		// Save last Task command.
		u16 SystemAddress = START_SYSTEM_ADDRESS;
		for (u8 Index = FIRST_SYSTEM_INDEX; Index < MASTER_TASKS_NUM; Index++)
		{
			EEPROM_writeBusy(SystemAddress, Task_save.TasksEEPROM_Array[Index]);
			SystemAddress++;
		}
		SaveNewData_flag = FALSE;
	}
	
}


// Read system data in EEPROM for restore system last status.
static void Read_SystemData(void)
{
	// Load SPI frame for slave status.
	ReceiveSPI_SlaveFrame.DeviceStatus = EEPROM_readBusy(START_SLAVE_ADDRESS);
	
	// Load Master SPI order frame.
	u16 FrameAddress = START_FRAME_ADDRESS;
	for (u8 Index = FIRST_FRAME_INDEX; Index < SMARTHOME_FRAME_SIZE; Index++)
	{
		SendSPI_MasterFrame.SPI_Array[Index] = EEPROM_readBusy(FrameAddress); 
		FrameAddress++;
	}
	
	// Load last Task command.
	u16 SystemAddress = START_SYSTEM_ADDRESS;
	for (u8 Index = FIRST_SYSTEM_INDEX; Index < MASTER_TASKS_NUM; Index++)
	{
		Task_save.TasksEEPROM_Array[Index] = EEPROM_readBusy(SystemAddress);
		SystemAddress++;
	}
	
	
	
}

/* 
* Reset EEPROM For Original value  of system. 
* Not all system status reset such as door condition open or close
*/
static void EEPROM_reset(void)
{
	
	EEPROM_writeBusy(START_SLAVE_ADDRESS, 0);
	

	u16 SystemAddress = START_SYSTEM_ADDRESS;
	for (u8 Index = FIRST_SYSTEM_INDEX; Index < MASTER_TASKS_NUM; Index++)
	{
		EEPROM_writeBusy(SystemAddress, 0);
		SystemAddress++;
	}
}

// Restore system according to last system order after restart.
static void Restore_System(void)
{
	// Disable send Message.
	BCM_SendString_Disable();
	
	PrevColor = Task_save.PrevColor;
	RGB_LedColor(PrevColor);
	
	// TVs
	if (Task_save.TV1 == 1)
	{
		TV_ON(1);
	}
	if (Task_save.TV2 == 1)
	{
		TV_ON(2);
	}
	
	// LEDs
	if (Task_save.Red == 1)
	{
		Leds_On(1);
	}
	if (Task_save.Grn== 1)
	{
		Leds_On(2);
	}
	if (Task_save.Blu == 1)
	{
		Leds_On(3);
	}
	
	// Rooms
	if (Task_save.Room1 == 1)
	{
		RoomLeds_On(1);
	}
	if (Task_save.Room3 == 1)
	{
		RoomLeds_On(2);
	}
	if (Task_save.Room3 == 1)
	{
		RoomLeds_On(3);
	}
	if (Task_save.Room4 == 1)
	{
		RoomLeds_On(4);
	}
	// Enable send Message.
	BCM_SendString_Enable();

	
}