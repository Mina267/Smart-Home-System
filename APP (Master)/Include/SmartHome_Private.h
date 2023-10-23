
#ifndef SMARTHOME_PRIVATE_H_
#define SMARTHOME_PRIVATE_H_

/********* String received from user Sizes *************/
#define STR_RECIVED_SIZE		40
#define STR_COMMAND_SIZE		25
#define STR_TASK_SIZE			25

/********* General Command ******************************/
#define FIRST_CHAR				0

/********************** Smart home Tasks. **************************/
#define MAX_NUMOF_LED			3
#define MAX_LCD_STR				35

/* scheduler */
#define PRIORITY_0_		0
#define PRIORITY_1_		1
#define PRIORITY_2_		2
#define PRIORITY_3_		3
#define PRIORITY_4_		4
#define PRIORITY_5_		5
#define PRIORITY_6_		6
#define PRIORITY_7_		7
#define PRIORITY_8_		8

/********** Task functions ***********/
#define DOOR_OPEN			1
#define DOOR_CLOSE			0
#define CURTAIN_OPEN		1
#define CURTAIN_CLOSE		0
#define LDR_MODE_OFF		0
#define LDR_MODE_ON			1
#define LEDS_CLOSE			0
#define LEDS_OPEN			1
#define LDR_MODE_STATE		2
#define TEMP_CONTROL_ON		1
#define TEMP_CONTROL_OFF	0

extern const c8 *RGB_MSG[RGB_MSG_SIZE];
extern const c8 *RGB_Commands[RGB_COMMAND_SIZE];
extern const c8 *GeneralCommands_Array[GENERAL_COMMAND_SIZE];


// System Status enum.
typedef enum
{
	NO_MODE,
	RGB_MODE,
	SMARTHOME_MODE,
	}Sytem_mode_t;

// RGB Status mode enum.	
typedef enum
{
	RGB_NOMODE,
	RGB_SETCOLOR,
	RGB_SHOWROOM,
	}RGB_mode_status_t;	

	

	
// Smart Home Task Status.
typedef enum
{
	TASK_NOT_FOUND,
	TASK_FOUND,
	}TASKS_Status_t;
	
// General command enum.
typedef enum
{
	RESET,
	SELECT_SYSTEM,
	GENERAL_COMMAND_NUM,
	}GeneralCommand_t;	
	

	
	
/************* SPI Frame struct *****************/
/********** Frame send by master ****************/
typedef union
{
	u8 SPI_Array[SMARTHOME_FRAME_SIZE];
	struct
	{
		u8 WindowPostion;
		u8 FanSpeed;
		u8 AirConditiondegree;
		u8 DeviceStatus;
		};
	}Master_Frame_t;
	
/********* Frame Receive from slave ************/
typedef union
{
	u8 SPI_Array[SMARTHOME_FRAME_SIZE];
	struct
	{
		u8 HomeTemp;
		u8 DeviceStatus;
		};
	}Slave_Frame_t;
	
/*********** Save Task in EEPROM ********/
typedef union
{
	u8 TasksEEPROM_Array[MASTER_TASKS_NUM];
	struct
	{
		u8 PrevColor;
		u8 TV1;
		u8 TV2;
		u8 Red;
		u8 Grn;
		u8 Blu;
		u8 Room1;
		u8 Room2;
		u8 Room3;
		u8 Room4;
	};
	}Tasks_EEPROM_t;


/*************************************************************************************************/
/******************************** Select Operation Functions *************************************/
/*************************************************************************************************/

// Take System Operating Mode as default.
static void TakeSystemMode(void);
// Reset all System.
static void Sytem_Reset(void);
// Return to select system mode menu.
static void Sytem_SelectMode(void);
/***** UART receive data Task ******/
static void Receive_UART_Task(void);

/*************************************************************************************************/
/******************************** System General Command *****************************************/
/*************************************************************************************************/

// Can be use in any mode of system operate.
static void StrAnalysis_GeneralCommands(void);
// Take action according to general command order.
static void GeneralCommands_TakeAction(GeneralCommand_t GeneralCommandNumber);

/**************************************** - 1 - **************************************************/
/********************************* RGB mode Functions ********************************************/
/*************************************************************************************************/

// Function Operate as RGB mode Selected.
static void RGB_SystemMode(void);
// Take RGB mode User Want to operate with (SelectColor/ShowRoom).
static void TakeRGBSystemMode(void);
// Take user Command for RGB mode
static void RGB_colorTakeCommand(void);
// Execute Command received from user.
static void RGB_ExecuteCommand(void);
// Execute String command user Input of set color.
static void RGB_SetColorExecuteCommand(void);
// Execute String command user Input of show room.
static void RGB_ShowRoomExecuteCommand(void);
// Reset RGB as default
static void RGB_Reset(void);
// Back To select RGB mode;
static void RGB_Back(void);

/**************************************** - 2 - **************************************************/
/******************************** Smart Home Mode Functions **************************************/
/*************************************************************************************************/

// Function Operate as Smart Home mode Selected.
// Take Task User want to execute.
static void SmartHome_SystemMode(void);
// Search in task array for task order.
static TASKS_Status_t SearchForTaskOrder(u16 *TaskIndex);
// Execute Task of Specific order from array
static void SmartHome_ExecuteCommand(void);
// Reset Smart Home as default
static void SmartHome_Reset(void);
// Back To select system mode;
static void SmartHome_Back(void);

/**************** Smart Home Control Functions ***************/

// Enter Number of led to turn On
static void Leds_On(u16 LED);
// Enter Number of led to turn OFF
static void Leds_OFF(u16 LED);
// Enter Number of Room to turn its Led On.
// Turn all Rooms LED On but zero or no number.
static void RoomLeds_On(u16 Room);
// Enter Number of Room to turn its Led Off.
// Turn all Rooms LED OFF but zero or no number.
static void RoomLeds_OFF(u16 Room);
// Enter number of TV to turn on
// Turn all TVs on but zero or no number.
static void TV_ON(u16 n);
// Enter number of TV to turn off
// Turn all TVs off but zero or no number.
static void TV_OFF(u16 n);
// Write String on LCD
static void LCD_Write(u16 n);
// The position in degree or windows open
static void ControlWindow(u16 Positon);
// 0 for Close 1 for open
static void GarageDoor(u16 Status);
// Open and close window Curtain
static void WindowCurtain(u16 Status);
// 0 for Close 1 for open
static void OutDoorLeds(u16 Status);
// Fan Speed
static void Fan_ON(u16 Speed);
// Turn Fan off
static void Fan_OFF(u16 Speed);
// Set Degree of air Condition.
static void AirCondition_ON(u16 Degree);
// Close of air Condition.
static void AirCondition_OFF(u16 Degree);
/*
 * Send message when task in salve MCU executed.
 * Send message to user when task in salve is executed
 * Monitor task status in slave by SPI frame received from it.
 * Compare Order send from Master to status receive from salve
 * When status receive from slave change as master order Send Message to notify user. 
 */
static void ReceiveFrameStaus_responed(void);
// Make temp sensor control operation of Air condition and fan 
static void Temp_ControlMode(u16 mode);
/**** SPI transmit And Receive task ******************/
static void CallBack_transmitAndReceive_SPIframes(void);


/*************************************************************************************************/
/************************************* Utils Fuctions ********************************************/
/*************************************************************************************************/

// Send Wrong Choice MSG
static void SendWrongChoice_MSG(void);
// Send Wrong Command MSG
static void SendWrongCommand_MSG(void);
// Send MSG for RGB mode
static void SendRGB_MSG(u8 SelectMSG);

/*************************************************************************************************/
/************************************ EEPROM Fuctions ********************************************/
/*************************************************************************************************/

// Save system data in EEPROM for next start.
static void Save_SystemData(void);
// Read system data in EEPROM for restore system last status.
static void Read_SystemData(void);
/*
* Reset EEPROM For Original value  of system.
* Not all system status reset such as door condition open or close
*/
static void EEPROM_reset(void);
// Restore system according to last system order after restart.
static void Restore_System(void);


#endif /* SMARTHOME_PRIVATE_H_ */