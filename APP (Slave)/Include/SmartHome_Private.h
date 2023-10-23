
#ifndef SMARTHOME_PRIVATE_H_
#define SMARTHOME_PRIVATE_H_



/********* Function marcos ******************************/
#define START_FRAME_ADDRESS			0X40
#define START_FRAMESLAVE_ADDRESS	0x10
#define FIRST_FRAME_INDEX			0
#define CLOSE						0
#define OPEN						1



/********* General Command ******************************/
#define FIRST_CHAR					0
#define DEGREE						1

/********** Scheduler **********************************/
#define PRIORITY_0_		0
#define PRIORITY_1_		1
#define PRIORITY_2_		2
#define PRIORITY_3_		3
#define PRIORITY_4_		4
#define PRIORITY_5_		5
#define PRIORITY_6_		6
#define PRIORITY_7_		7
#define PRIORITY_8_		8


/* ***** SPI Frames struct * *******/
typedef union
{
	u8 SPI_Array[SMARTHOME_FRAME_SIZE];
	struct
	{
		u8 WindowPostion;
		u8 FanSpeed;
		u8 AirConditiondegree;
		u8 DevicesStatus;
		};
	}Master_Frame_t;

typedef union
{
	u8 SPI_Array[SMARTHOME_FRAME_SIZE];
	struct
	{
		u8 HomeTemp;
		u8 DevicesStatus;
		};
	}Slave_Frame_t;
	
	

// The position in degree or windows open
static void ControlWindow(void);
// 0 for Close 1 for open
static void GarageDoor(void);
// External interrupt coming from Proximity Sensor when Garage door is closed and opened
// When it close or open change condition in SPI frame from open to close
static void EX_INT1_CallBack_GarageProximity(void);
// Open and close window Curtain
static void WindowCurtain(void);
// External interrupt coming from Proximity Sensor when Curtain is closed and opened
// When it close or open change condition in SPI frame from open to close
static void EX_INT0_CallBack_CurtainProximity(void);
// 0 for Close 1 for open
static void OutDoorLeds(void);
static void LDR_Control(void);
// Fan Speed
static void Fan_Operate(void);
// Set Degree of air Condition. Close of air Condition.
static void AirCondition(void);
// Make temp sensor control operation of Air condition and fan 
static void Temp_ControlMode(void);
// Print Temp. in LCD
static void print_Temp(u16 temp, u8 line, u8 cell);
/********************* Read Temperature and Humidity ***************************/
static void HomeTemperatureRead_Task(void);
/********************* Read LDR output *****************************************/
static void HomeLDR_Read_Task(void);
/********************* SPI transmit And Receive task ***************************/
static void CallBack_transmitAndReceive_SPIframes(void);
/********************* Save data of system *************************************/
static void SaveData_Task(void);
/********************* Run system tasks *****************************************/
static void SmartHome_Run(void);




/***************************************************************************************/
/************************** EEPROM *****************************************************/
/***************************************************************************************/

/********************* Save data of system *************************************/
static void Save_SystemData(void);
/********************* Read data of system *************************************/
static void Read_SystemData(void);
/********************* Reset system EEPROM **************************************/
static void EEPROM_reset(void);

#endif /* SMARTHOME_PRIVATE_H_ */