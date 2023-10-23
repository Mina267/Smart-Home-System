#include "StdTypes.h"
#include "Utils.h"
#include "MemMap.h"
#include "DIO_interface.h"
#include "EX_Interrupt.h"
#include "SPI_Interface.h"
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
#include "Servo_Interface.h"
#include "TEMP_Filter.h"
#include "Sensors.h"
#include "EEPROM_intrface.h"
#include "Kernel_Interface.h"
#include "DHT22.h"


/***** Periodicity Acheive flags ***************/
static volatile bool_t ReadTemp_flag = FALSE;
static volatile bool_t ReadLDR_Flag = FALSE;
static volatile bool_t SPI_Frame_flag = FALSE;
static volatile bool_t Save_Flag = FALSE;
static volatile bool_t SmartHome_Flag = FALSE;


/*************** Sensor variables ***************/
static u16 HomeTemperature;
static u16 LDRread;

/*************** SPI frames *********************/
static Master_Frame_t ReceiveSPI_MasterFrame;
static Slave_Frame_t SendSPI_SlaveFrame;


/*************** Control mode flags *************/
static bool_t TempControlMode_flag = FALSE;
static bool_t OutDoorLedLDR_Control_Flag = FALSE;

/*************** EEPROM reset flag **************/
static bool_t Reset_flag = FALSE;


void SmartHome_Init(void)
{
	/************************* intializations *********************/
	DIO_Init();
	LCD_Init();
	ADC_Init( VREF_AVCC, ADC_SCALER_64);
	SPI_FrameSend_Init();
	EXI_Init();
	// Window Position
	Servo_Init();
	/************************ Draw Pattern ************************/
	u8 arr1[8] = {0x07, 0x05, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00};
	LCD_makeNewShape(arr1, DEGREE);
	
	// Garage door and curtain signal from a Proximity sensor
	EXI_SetCallBack(EX_INT0, EX_INT0_CallBack_CurtainProximity);
	EXI_SetCallBack(EX_INT1, EX_INT1_CallBack_GarageProximity);
	EXI_Enable(EX_INT0);
	EXI_Enable(EX_INT1);
	
	// Fan Speed Control
	TIMER0_Init(TIMER0_PHASECORRECT_MODE, TIMER0_SCALER_8);
	TIMER0_OC0Mode(OC0_NON_INVERTING);
	
	// Restore System Last status
	Read_SystemData();
	
	/***** Ensure motors stop *******************/
	MOTOR_dir(WINDOW_CURTION_MOTOR, M_STOP);
	MOTOR_dir(GRAGEDOOR_MOTOR, M_STOP);
	
	/* SPI transmit And Receive task */
	RTOS_Error_typeCreateTask(PRIORITY_1_, CallBack_transmitAndReceive_SPIframes, SPI_FRAME_PERIODICITY , 7);
	/* Running of smart home */
	RTOS_Error_typeCreateTask(PRIORITY_7_, SmartHome_Run, READ_TEMP_RUN_PERIODICITY , 1);
	/*  Read temp Task */
	RTOS_Error_typeCreateTask(PRIORITY_3_, HomeTemperatureRead_Task, SMARTHOME_RUN_PERIODICITY , 2);
	/* Read LDR Task */
	RTOS_Error_typeCreateTask(PRIORITY_4_, HomeLDR_Read_Task, READ_LDR_RUN_PERIODICITY , 3);
	/* EEPROM save data Task */
	RTOS_Error_typeCreateTask(PRIORITY_5_, SaveData_Task, SAVE_DATA_PERIODICITY , 4);
	
	
	
	/******* Start RTOS ******/
	RTOS_voidStart();
	
}




void SmartHome_Runnable(void)
{
	/********************* Send and Receive SPI frames **************************/
	if (SPI_Frame_flag)
	{
		SPI_SendFrame_Setter(SendSPI_SlaveFrame.SPI_Array);
		SPI_ReceiveFrame_Getter(ReceiveSPI_MasterFrame.SPI_Array);
		SPI_Frame_flag = FALSE;
	}	
	
	/********************* Read Temperature and Humidity SPI frames ***************/
	if (ReadTemp_flag)
	{
		HomeTemperature = TEMP_Read();
		SendSPI_SlaveFrame.HomeTemp = HomeTemperature / 10;
		print_Temp(HomeTemperature, 1, 10);
		ReadTemp_flag = FALSE;
	}
	
	/********************* Read LDR output SPI frames ******************************/
	if (ReadLDR_Flag)
	{
		LDRread = LIGHT_Read();
		ReadLDR_Flag = FALSE;
	}
	
	/********************* Save data of system *************************************/
	if (Save_Flag)
	{
		Save_SystemData();
		Save_Flag = FALSE;

	}
	
	/********************* Run system tasks *****************************************/
	if (SmartHome_Flag)
	{
		// Running of Tasks
		ControlWindow();
		GarageDoor();
		WindowCurtain();
		Temp_ControlMode();
		Fan_Operate();
		AirCondition();
		OutDoorLeds();
		LDR_Control();
		EEPROM_reset();
		
		SmartHome_Flag = FALSE;
	}
	
	

}

// Control Position of windows
static void ControlWindow(void)
{
	u8 WindowPositon = ReceiveSPI_MasterFrame.WindowPostion;
	Servo_Setposition2(WindowPositon * 10);
}


// 0 for Close 1 for open
static void GarageDoor(void)
{
	/* Read the status of Received SPI frame from master to see Action should do */
	u8 GarageDoor_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, GRAGEDOOR);
	u8 GarageDoor_Send = READ_BIT(SendSPI_SlaveFrame.DevicesStatus, GRAGEDOOR);

	/* If the current status of the garage door is closed and receive an order from the ?master to open.
	* start to open the door until the current status change to open.
	* Current status change by a signal from a Proximity sensor from an external interrupt.
	* set slave frame garage door status bit.
	* and the same thing for closing.
	*/
	if ((GarageDoor_Rec == OPEN) && (GarageDoor_Send == CLOSE))
	{
		// Operate DC motor to open the door
		MOTOR_dir(GRAGEDOOR_MOTOR, M_CW);
	}
	else if ((GarageDoor_Rec == CLOSE) && (GarageDoor_Send == OPEN))
	{
		// Operate DC motor to close the door
		MOTOR_dir(GRAGEDOOR_MOTOR, M_CCW);
	}
	
	
}


// Open and close window Curtain
static void WindowCurtain(void)
{
	/* Read the status of Received SPI frame from master to see Action should do */
	u8 WindowCurtain_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, WINDOW_CURTION);
	u8 WindowCurtain_Send = READ_BIT(SendSPI_SlaveFrame.DevicesStatus, WINDOW_CURTION);
	
	/* If the current status of the Curtain is closed and receive an order from the ?master to open.
	* start to open the door until the current status change to open.
	* Current status change by a signal from a Proximity sensor from an external interrupt.
	* set slave frame Curtain status bit.
	* and the same thing for closing.
	*/
	if ((WindowCurtain_Rec == OPEN) && (WindowCurtain_Send == CLOSE))
	{
		// Operate DC motor to open the Curtain.
		MOTOR_dir(WINDOW_CURTION_MOTOR, M_CW);
	}
	else if ((WindowCurtain_Rec == CLOSE) && (WindowCurtain_Send == OPEN))
	{
		// Operate DC motor to close the Curtain.
		MOTOR_dir(WINDOW_CURTION_MOTOR, M_CCW);
	}
}






// 0 for Close 1 for open
static void OutDoorLeds(void)
{
	/* Read the status of Received SPI frame from master to see Action should do */
	u8 OutDoorLeds_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, OUTDOORLEDS);
	
	/* See if system operate in LDR control outdoor LEDs mod */
	if (OutDoorLedLDR_Control_Flag)
	{
		/* applying hysteresis*/
		/* Min read to turn on out door LEDs */
		if (LDRread <= MIN_LIGHT)
		{
			DIO_WritePin(OUTDOORLEDS_PIN, HIGH);
			SET_BIT(SendSPI_SlaveFrame.DevicesStatus, OUTDOORLEDS);
		}
		/* Max read to turn off out door LEDs */
		else if (LDRread >= MAX_LIGHT)
		{
			DIO_WritePin(OUTDOORLEDS_PIN, LOW);
			CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, OUTDOORLEDS);
		}
	}
	//  Receive from SPI to turn on out door LEDs but LDR control must be close
	else if ((OutDoorLeds_Rec == OPEN) && (!OutDoorLedLDR_Control_Flag))
	{
		// High in Out door LEDs pin
		DIO_WritePin(OUTDOORLEDS_PIN, HIGH);
		// Set Out door LEDs in Device status Register.
		SET_BIT(SendSPI_SlaveFrame.DevicesStatus, OUTDOORLEDS);
	}
	//  Receive from SPI to turn off out door LEDs but LDR control must be close
	else if ((OutDoorLeds_Rec == CLOSE) && (!OutDoorLedLDR_Control_Flag))
	{
		// Low in Out door LEDs pin
		DIO_WritePin(OUTDOORLEDS_PIN, LOW);
		// Clear Out door LEDs in Device status Register.
		CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, OUTDOORLEDS);
	}
}


/* LDR control mode */
static void LDR_Control(void)
{
	/* Read the status of Received SPI frame from master to see Action should do */
	u8 LDR_Control_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, LDR_CONTROL);
	
	/* Received from SPI frame to Turn on  LDR control mode */
	if (LDR_Control_Rec == 1)
	{
		/* Flag to synchronization with  OutDoorLeds task */
		OutDoorLedLDR_Control_Flag = TRUE;
		/* Set LDR CONTROL in Device status Register. */
		SET_BIT(SendSPI_SlaveFrame.DevicesStatus, LDR_CONTROL);
	}
	/* Received from SPI frame to Turn on LDR control mode */
	else
	{
		/* Flag to synchronization with  OutDoorLeds task */
		OutDoorLedLDR_Control_Flag = FALSE;
		/* CLear LDR CONTROL in Device status Register. */
		CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, LDR_CONTROL);
	}
}


/* Set Fan speed control and on/off */
static void Fan_Operate(void)
{
	/* Check first if temperature control mode by temperature sensor operate or not */
	if (TempControlMode_flag)
	{
		/* applying hysteresis*/
		/* Read to turn on Fan */
		if (HomeTemperature >= OPERATE_TEMP)
		{
			SET_BIT(SendSPI_SlaveFrame.DevicesStatus, FAN);
			MOTOR_Speed_dir(FAN_MOTOR, DEFAULT_FAN_SPEED, M_CW);
		}
		/* Read to turn off Fan */
		if (HomeTemperature <= MIN_TEMP_STOP_OPERATE)
		{
			CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, FAN);
			MOTOR_Speed_dir(FAN_MOTOR, ZERO_SPEED, M_STOP);
		}
	}
	else
	{
		/* Read the status of Received SPI frame from master to see Action should do */
		u8 FAN_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, FAN);
		u8 Fan_Speed = ReceiveSPI_MasterFrame.FanSpeed;
		/* Open fan at speed received in frame */
		if (FAN_Rec == OPEN)
		{
			/* Set Bit Device status Register. */
			SET_BIT(SendSPI_SlaveFrame.DevicesStatus, FAN);
			/* Set motor speed and direction */
			MOTOR_Speed_dir(FAN_MOTOR, Fan_Speed, M_CW);
		}
		/* close fan */
		else
		{
			/* Clear Bit Device status Register. */
			CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, FAN);
			/* Stop motor */
			MOTOR_Speed_dir(FAN_MOTOR, ZERO_SPEED, M_STOP);
		}
	}
}



/* Set Degree of air Condition. */
static void AirCondition(void)
{
	/* Check first if temperature control mode by temperature sensor operate or not */
	if (TempControlMode_flag)
	{
		/* Applying hysteresis */
		/* Read to turn on Air Condition */
		if (HomeTemperature >= OPERATE_TEMP)
		{
			SET_BIT(SendSPI_SlaveFrame.DevicesStatus, AIRCONDITION);
			/* Print Air Condition degree on LCD */
			LCD_SetCursor(0, 0);
			LCD_WriteString("Air Condition ON ");
			print_Temp(DEFAULT_AIRCONITION_DGREE * 10, 1, 0);
		}
		/* Read to turn off Air Condition */
		if (HomeTemperature <= MIN_TEMP_STOP_OPERATE)
		{
			CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, AIRCONDITION);
			/* Print Air Condition state on LCD */
			LCD_SetCursor(0, 0);
			LCD_WriteString("Air ConditionOFF  ");
			LCD_ClearCell(1, 0, 6);
		}
	}
	else
	{
		/* Read the status of Received SPI frame from master to see Action should do */
		u8 AirCondition_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, AIRCONDITION);
		u8 AirConditiondegree = ReceiveSPI_MasterFrame.AirConditiondegree;
		/* Open Air Condition at degree received in frame */
		if (AirCondition_Rec == 1)
		{
			/* Set Bit Device status Register. */
			SET_BIT(SendSPI_SlaveFrame.DevicesStatus, AIRCONDITION);
			/* Print Air Condition degree on LCD */
			LCD_SetCursor(0, 0);
			LCD_WriteString("Air Condition ON ");
			print_Temp(AirConditiondegree * 10, 1, 0);
		}
		/* close Air Condition */
		else
		{
			/* Clear Bit Device status Register. */
			CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, AIRCONDITION);
			/* Print Air Condition state on LCD */
			LCD_SetCursor(0, 0);
			LCD_WriteString("Air ConditionOFF ");
			LCD_ClearCell(1, 0, 6);
		}
	}
}

// Make temp sensor control operation of Air condition and fan.
static void Temp_ControlMode(void)
{
	/* Read the status of Received SPI frame from master to see Action should do */
	u8 Temp_Control_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, TEMPCONTROL);
	/* Received from SPI frame to Turn on temp control mode */
	if (Temp_Control_Rec == OPEN)
	{
		/* Set Bit Device status Register. */
		SET_BIT(SendSPI_SlaveFrame.DevicesStatus, TEMPCONTROL);
		/* Flag to synchronization with  Fan & Air Condition tasks */
		TempControlMode_flag = TRUE;
	}
	/* Received from SPI frame to Turn on temp control mode */
	else if (Temp_Control_Rec == CLOSE)
	{
		/* Clear Bit Device status Register. */
		CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, TEMPCONTROL);
		/* Flag to synchronization with  Fan & Air Condition tasks */
		TempControlMode_flag = FALSE;
	}
}


/* Print Temp. in LCD */
static void print_Temp(u16 temp, u8 line, u8 cell)
{
	LCD_SetCursor(line, cell);
	LCD_WriteNumber(temp / 10);
	LCD_WriteChar('.');
	LCD_WriteNumber(temp % 10);
	LCD_WriteChar(DEGREE);
	LCD_WriteString("C ");
}



/*****************************************************************************************************************/
/************************************************* EEPROM Functions **********************************************/
/*****************************************************************************************************************/

/* Save system status in EEPROM to restore system last status in restart */
static void Save_SystemData(void)
{
	EEPROM_writeBusy(START_FRAMESLAVE_ADDRESS, SendSPI_SlaveFrame.DevicesStatus);
	u16 FrameAddress = START_FRAME_ADDRESS;
	for (u8 Index = FIRST_FRAME_INDEX; Index < SMARTHOME_FRAME_SIZE; Index++)
	{
		EEPROM_writeBusy(FrameAddress, ReceiveSPI_MasterFrame.SPI_Array[Index]);
		FrameAddress++;
	}
	
}

/* Read system last status from EEROM */
static void Read_SystemData(void)
{
	SendSPI_SlaveFrame.DevicesStatus = EEPROM_readBusy(START_FRAMESLAVE_ADDRESS);
	u16 FrameAddress = START_FRAME_ADDRESS;
	for (u8 Index = FIRST_FRAME_INDEX; Index < SMARTHOME_FRAME_SIZE; Index++)
	{
		ReceiveSPI_MasterFrame.SPI_Array[Index] = EEPROM_readBusy(FrameAddress);
		FrameAddress++;
	}
}

/*****************************************************************************************************************/
/********************************************** ISR CallBack Functions *******************************************/
/*****************************************************************************************************************/



/* External interrupt coming from Proximity Sensor when Curtain is closed and opened
*  When it close or open change status in SPI frame from open to close */
static void EX_INT0_CallBack_CurtainProximity(void)
{
	/* Read the status of Received SPI frame from master to see Action should do */
	u8 WindowCurtain_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, WINDOW_CURTION);
	
	/* If Curtain set to open then status now open */
	if (WindowCurtain_Rec == OPEN)
	{
		SET_BIT(SendSPI_SlaveFrame.DevicesStatus, WINDOW_CURTION);
	}
	/* If Curtain set to close then status now open */
	else if (WindowCurtain_Rec == CLOSE)
	{
		CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, WINDOW_CURTION);
	}
	/* Stop motor */
	MOTOR_dir(WINDOW_CURTION_MOTOR, M_STOP);
}


/* External interrupt coming from Proximity Sensor when Garage door is closed and opened
*  When it close or open change condition in SPI frame from open to close */
static void EX_INT1_CallBack_GarageProximity(void)
{
	/* Read the status of Received SPI frame from master to see Action should do */
	u8 GarageDoor_Rec = READ_BIT(ReceiveSPI_MasterFrame.DevicesStatus, GRAGEDOOR);
	
	/* If garage door set to open then status now open */
	if (GarageDoor_Rec == OPEN)
	{
		SET_BIT(SendSPI_SlaveFrame.DevicesStatus, GRAGEDOOR);
	}
	/* If garage door set to close then status now open */
	else if (GarageDoor_Rec == CLOSE)
	{
		CLR_BIT(SendSPI_SlaveFrame.DevicesStatus, GRAGEDOOR);
	}
	/* Stop motor */
	MOTOR_dir(GRAGEDOOR_MOTOR, M_STOP);
}







/********************* Read Temperature and Humidity SPI frames ***************/
static void HomeTemperatureRead_Task(void)
{
	ReadTemp_flag = TRUE;
}

/********************* Read LDR output SPI frames ******************************/
static void HomeLDR_Read_Task(void)
{
	ReadLDR_Flag = TRUE;
}

/********************* Save data of system *************************************/
static void SaveData_Task(void)
{
	/* Save new system status */
	Save_Flag = TRUE;
}

/********************* SPI transmit And Receive task ***************************/
static void CallBack_transmitAndReceive_SPIframes(void)
{
	SPI_Frame_flag = TRUE;
}


/********************* Run system tasks *****************************************/

static void SmartHome_Run(void)
{
	SmartHome_Flag = TRUE;
}


/********************* Reset system EEPROM **************************************/
static void EEPROM_reset(void)
{
	if (Reset_flag == TRUE)
	{
		EEPROM_writeBusy(START_FRAMESLAVE_ADDRESS, 0);
		u16 FrameAddress = START_FRAME_ADDRESS;
		for (u8 Index = FIRST_FRAME_INDEX; Index < SMARTHOME_FRAME_SIZE; Index++)
		{
			EEPROM_writeBusy(FrameAddress, 0);
			FrameAddress++;
		}
		Reset_flag = FALSE;
	}
}



/*
LCD_SetCursor(1, 0);
LCD_WriteBinary(ReceiveSPI_MasterFrame.SPI_Array[3]);
LCD_SetCursor(0, 0);
LCD_WriteNumber_3D(ReceiveSPI_MasterFrame.SPI_Array[2]);
LCD_WriteString(" ");
LCD_WriteNumber_3D(ReceiveSPI_MasterFrame.SPI_Array[1]);
LCD_WriteString(" ");
LCD_WriteNumber_3D(ReceiveSPI_MasterFrame.SPI_Array[0]);
LCD_WriteString(" ");


	LCD_SetCursor(1,0);
	LCD_WriteNumber(GarageDoor_Rec);
	LCD_SetCursor(1,3);
	LCD_WriteNumber(GarageDoor_Send);
*/


