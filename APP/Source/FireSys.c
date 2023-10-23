# define F_CPU 8000000
#include <util/delay.h>

#include "StdTypes.h"
#include "LCD_interface.h"
#include "Keypad_interface.h"
#include "MOTOR_interface.h"
#include "Sensors.h"
#include "Buzz_interface.h"
#include "LEDS.h"
#include "TEMP_Filter.h"
#include "FireSys.h"
#include "FireSys_private.h"
#include "EEPROM_intrface.h"
#include "Timers.h"
#include "MemMap.h"
#include "EX_Interrupt.h"
#include "Timers_Services.h"
#include "FireSys_Cfg.h"
#include "UART_Services.h"
#include "BCM.h"


/************** Sensor Read varaibles ****************/
static u16 temp;
static u8 Smoke;
/******* System status *******************************/
static FireSys_statue_t status = Fine;

/***** Smoke sensor start Read flag ******************/ 
static bool_t SmokeFlag = FALSE;
static volatile bool_t TakeSmokeRead_Flag = FALSE;
/************ One Excution function Flags ************/ 
static bool_t FireFlag = TRUE;
static bool_t HeatFlag = TRUE;
static bool_t FineFlag = TRUE;

/*********** Timer0 interrupt flags ******************/
static volatile bool_t tempRead_flag = FALSE;
static volatile bool_t SmokeRead_flag = FALSE;

/************** password Varaibles *******************/
static c8 Truepass[MAX_PASSWORD];
static c8 PASS[MAX_PASSWORD];
static volatile PASS_status Pass_status = PASS_ACCESS;
static volatile bool_t NewPass_Flag = FALSE;
static bool_t CorrectPass_flag = FALSE;
static volatile u8 WrongPass_Cnt = 0;
static bool_t clearAtDis_flag = FALSE;
static bool_t clearAtDis2_flag = TRUE;
static bool_t clearAtDis3_flag = FALSE;

/************** Siren Varaibles *******************/
static volatile bool_t SirenFlag = FALSE; 

/************ UART & Bluetooth Varaiables *********/
static volatile bool_t Warring_Flag = FALSE;
static volatile bool_t Alarm_Flag  = FALSE;

/************ CFG Menu Varaiables *********/
static u16 HeatTemp;
static u8 FireSmoke;
static volatile bool_t NewTemp_Flag = FALSE;
static volatile bool_t NewSmoke_Flag = FALSE;
static volatile bool_t EDITMenu_Flag = FALSE;
static volatile bool_t Menu_flag = FALSE;
static volatile bool_t CFG_MenuMess_flag = TRUE;
static volatile u8 num = 0;
static volatile bool_t wait_flag = TRUE; 
static volatile u8 cell = 10;








void FireSys_Init(void)
{
	
	
	
	/************************ Motors *******************************/
	TIMER0_Init(TIMER0_FASTPWM_MODE, TIMER0_SCALER_8);
	TIMER0_OC0Mode(OC0_INVERTING);
	PUMP_Stop();
	
	TIMER2_Init(TIMER2_FASTPWM_MODE, TIMER2_SCALER_8);
	TIMER2_OC2Mode(OC2_INVERTING);
	SmokeFan_Stop();
	
	/************************ Draw Pattern ************************/
	u8 arr1[8] = {0x07, 0x05, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00};
	LCD_makeNewShape(arr1, DEGREE);
	
	
	// Read Temp from EEPROM
	Read_Temp();
	// Read Smoke from EEPROM
	Read_Smoke();
	
	// Read password from EEPROM
	Read_Password();
	
	// Temperature Filter Start
	TEMP_FilterInit();
	
	// Timer to Block user for "" sec if user enter wrong password 5 times 
	// and Siren for 1 Sec on and two sec off
	Timer1_SetInterruptTime_s(1 ,Timer1_ScheduleFun);
	
	// Welcome Message
	Welcome();
	

	// External interrupt for Change password
	EXI_Init();
	EXI_Enable(EX_INT2);
	EXI_TriggerEdge(EX_INT2, FALLING_EDGE);
	EXI_SetCallBack(EX_INT2, EX_INT2_callBack);
	
	// UART Services to Warn Owner
	BCM_FrameReceive_Init();
	UART_ReceiveStringAshync_Init(0x0A0D);
	BCM_SendStringInit();
	
	// Enable Global Interrupts
	sei();
}



void FireSys_Runnable(void)
{
	// Start of CFG Menu 
	CFG_MenuStart();
	
	
	// Run of Temperature Filter
	TEMP_FilterRunnable();
	
	// Run of Send BCM
	BCM_SendStringRunnable();
	
	// Send Messages
	Send_Bluetooth_Runnable();
		
	// Read  Temperature and Print it.
	temp = TEMP_FilterGetter();
	print_Temp();
	
	
	// Check For System Status
	if (SmokeFlag && TakeSmokeRead_Flag)
	{
		Smoke = SMOKE_PERC(PRESS_Read());
		TakeSmokeRead_Flag = FALSE;
	}
	
	if (Smoke > FireSmoke)
	{
		status = Fire;
	}
	else if ((temp >= HeatTemp) && (status != Fire))
	{
		status = Heat;
		SmokeFlag = TRUE;
	}
	else if ((temp <= (HeatTemp - TEMP_HYST))  && (status != Fire))
	{
		status = Fine;
		SmokeFlag = FALSE;
		Smoke = 0;
	}
	
	
	
	switch (status)
	{
		// If System state is Fine
		case Fine:
		FireFlag = TRUE;
		HeatFlag = TRUE;
		Fine_state();
		break;
		
		// If System state is Heat
		case Heat:
		FireFlag = TRUE;
		FineFlag = TRUE;
		heat_state();
		break;
		
		// If System state is Fire
		case Fire:
		FineFlag = TRUE;
		HeatFlag = TRUE;
		Fire_state();
		
		// At Fire Start to take Password FromUser
		// If User enter to many wrong password Block Him For given time
		if ((Pass_status == PASS_ACCESS) && (WrongPass_Cnt < MAX_WRONGPASS))
		{
			LCD_SetCursor(SECOND_LINE, FIRST_CELL);
			LCD_WriteString("Password: ");
			
			// To Delete "Cannot take pass"
			if (clearAtDis2_flag)
			{
				LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
				clearAtDis2_flag = FALSE;
				
			}
			TakePassword();
			PassFrom_BlueTooth();
		}
		else
		{
			// To Print "Cannot take pass one time
			if (clearAtDis2_flag == FALSE)
			{
				LCD_SetCursor(SECOND_LINE, FIRST_CELL);
				LCD_WriteString("Cannot take pass");
				clearAtDis2_flag = TRUE;
			}
			
		}
		
		break;
	}
	
}

/************************ State Function *************************/

// Fine State Function
static void Fine_state(void)
{
	if (FineFlag == TRUE)
	{
		LCD_SetCursor(FIRST_LINE, 12);
		LCD_WriteString("Fine");
		LCD_ClearCell(FIRST_LINE, 8, 3);
		LEDS_Off(RED_LED);
		LEDS_Off(YEL_LED);
		PUMP_Stop();
		SmokeFan_Stop();
		BuzzOff();
		FineFlag = FALSE;
	}
	
}

// Heat State Function
static void heat_state(void)
{
	print_Smoke();
	
	if (HeatFlag == TRUE)
	{
		LCD_Clear();
		LCD_SetCursor(FIRST_LINE, 12);
		LCD_WriteString("Heat");
		LEDS_On(YEL_LED);
		LEDS_Off(RED_LED);
		PUMP_Stop();
		SmokeFan_Stop();
		BuzzOff();
		HeatFlag = FALSE;
	}
	
	
	
}

// Fire State Function
static void Fire_state(void)
{
	
	print_Smoke();
	SirenSound();
	PUMP_Operate();
	SmokeFan_Operate();
	
	if (FireFlag == TRUE)
	{
		LCD_Clear();
		NewPass_Flag = FALSE;
		Pass_status = PASS_ACCESS;
		WrongPass_Cnt = 0;
			
		LCD_SetCursor(FIRST_LINE, 12);
		LCD_WriteString("Fire");
		LEDS_On(RED_LED);
		LEDS_Off(YEL_LED);
		FireFlag = FALSE;
	}	
	
}

/********************** Print on LCD functions *********************/
static void Welcome(void)
{
	_delay_ms(300);
	LCD_SetCursor(FIRST_LINE, 3);
	LCD_WriteString("Welcome To");
	_delay_ms(600);
	LCD_SetCursor(SECOND_LINE, 3);
	LCD_WriteString("Fire System");
	_delay_ms(1500);
	LCD_Clear();
	
}

// Print Temp. in LCD
static void print_Temp(void)
{
	LCD_SetCursor(FIRST_LINE, FIRST_CELL);
	LCD_WriteNumber(temp / 10);
	LCD_WriteChar('.');
	LCD_WriteNumber(temp % 10);
	LCD_WriteChar(DEGREE);
	LCD_WriteString("C ");
}

// Print Smoke in LCD
static void print_Smoke(void)
{
	LCD_SetCursor(FIRST_LINE, 8);
	LCD_WriteNumber(Smoke);
	LCD_WriteString("% ");
}


/**************** Siren Functions *********************************************/


// Siren Sound
static void SirenSound(void)
{

	if (SirenFlag)
	{
		BuzzOn();
	}
	else if (!SirenFlag)
	{
		BuzzOff();
	}
	
}

/********************** Motor Functions ***************************************/
/*********** Pump ********************/
static void PUMP_Stop(void)
{
	MOTOR_Speed_dir(M1, 0 , M_STOP);
}

static void PUMP_Operate(void)
{
	if (temp < HeatTemp + TEMP_2ND_RANGE)
 	{
		MOTOR_Speed_dir(M1, 70 , M_CW);
	}
	else if ((temp >= HeatTemp + TEMP_2ND_RANGE) && (temp < HeatTemp + TEMP_3RD_RANGE))
	{
		MOTOR_Speed_dir(M1, 80 , M_CW);
	}
	else if ((temp >= HeatTemp + TEMP_3RD_RANGE) && (temp < HeatTemp + TEMP_4RD_RANGE))
	{
		MOTOR_Speed_dir(M1, 90 , M_CW);
	}
	else if (temp >= HeatTemp + TEMP_4RD_RANGE)
	{
		MOTOR_Speed_dir(M1, 100 , M_CW);
	}
	
	
}




/******************** Smoke Fan ******************/



static void SmokeFan_Stop(void)
{
	MOTOR_Speed_dir(M2, 0 , M_STOP);
}

static void SmokeFan_Operate(void)
{
	if (Smoke < FireSmoke +  SMOKE_2ND_RANGE)
	{
		MOTOR_Speed_dir(M2, 70 , M_CCW);
	}
	else if ((Smoke >= FireSmoke + SMOKE_2ND_RANGE) && (Smoke < FireSmoke + SMOKE_3RD_RANGE))
	{
		MOTOR_Speed_dir(M2, 80 , M_CCW);
	}
	else if ((Smoke >= FireSmoke + SMOKE_3RD_RANGE) && (Smoke < FireSmoke + SMOKE_4RD_RANGE))
	{
		MOTOR_Speed_dir(M2, 90 , M_CCW);
	}
	else if (Smoke >= FireSmoke + SMOKE_4RD_RANGE)
	{
		MOTOR_Speed_dir(M2, 100 , M_CCW);
	}
	
	
}



/*********************************** Password Functions ***************************************************/

/*************************** Edit And Read password Functions *********************/
static void Save_Password(void)
{
	u16 PassAddress = START_PASS_ADDRESS;
	for (u8 Index = FIRST_PASS_INDEX; Index < MAX_PASSWORD; Index++)
	{
		EEPROM_writeBusy(PassAddress, PASS[Index]);
		PassAddress++;
	}
}

static void Read_Password(void)
{
	u16 PassAddress = START_PASS_ADDRESS;
	for (u8 Index = FIRST_PASS_INDEX; Index < MAX_PASSWORD; Index++)
	{
		Truepass[Index] = EEPROM_readBusy(PassAddress);
		PassAddress++;
	}
}



static void Clear_pass(void)
{
	for (u8 Index = FIRST_PASS_INDEX; PASS[Index]; Index++)
	{
		PASS[Index] = 0;
	}
}

/********************************* Take Password Functions ***************************************/

static STR_Status Str_cmp(c8 *str)
{
	for (u8 Index = 0; Truepass[Index] || str[Index]; Index++)
	{
		if (Truepass[Index] != str[Index])
		{
			return STR_NOMATCH;
		}
	}
	return STR_MATCH;
}

static STR_Status TakePasswordToAccess(void)
{
	u8 result = Str_cmp(PASS);
	if (result == STR_MATCH)
	{
		LCD_SetCursor(1, 0);
		LCD_WriteString("CORRECT PASSWORD");
		_delay_ms(500);
		Reset_System();
	}
	else
	{
		WrongPass_Cnt++;
		LCD_SetCursor(1, 0);
		LCD_WriteString("WRONG PASSWORD!!");
		_delay_ms(500);
		LCD_SetCursor(1, 0);
		LCD_WriteString("                    ");
		
	}
	return result;
}

static void Reset_System(void)
{
	status = Fine;
	SmokeFlag = FALSE;
	Smoke = 0;
}

static void TakeNewPassword(void)
{
	
	
	if (CorrectPass_flag == FALSE)
	{
		
		STR_Status r = TakePasswordToAccess();
		if (r == STR_MATCH)
		{
			CorrectPass_flag = TRUE;
		}
	}
	else
	{
		
		LCD_SetCursor(SECOND_LINE, FIRST_CELL);
		LCD_WriteString("PASSWORD CHANGED!");
		_delay_ms(600);
		LCD_SetCursor(SECOND_LINE, FIRST_CELL);
		LCD_WriteString("                    ");
		Save_Password();
		Read_Password();
		
		Pass_status = PASS_ACCESS;
		NewPass_Flag = FALSE;
		CorrectPass_flag = FALSE;
		Menu_flag = FALSE;
	}
}


/************************************************************************************************/

static void TakePassword(void)
{
	static u8 key;
	static u8 PassIndex = 0;
	static u8 cell = 10;
	
	
	
	key = KEYPAD_GetKey();
	
	if ((key >= '0' && key <= '9')
	|| (key >= 'a' && key <= 'z')
	|| (key >= 'A' && key <= 'Z'))
	{
		if (PassIndex < MAX_PASSWORD - 1)
		{
			LCD_SetCursor(1, cell);
			LCD_WriteChar('*');
			BuzzSound();
			PASS[PassIndex] = key;
			PassIndex++;
			cell++;
		}
	}
	if (key == PASS_DELETE && START_CELL != cell)
	{
		PASS[PassIndex - 1] = 0;
		LCD_ClearCell(SECOND_LINE, cell - 1, 1);
		LCD_SetCursor(SECOND_LINE, cell - 1);
		cell--;
		PassIndex--;
	}
	
	if (key == PASS_DONE)
	{
		BuzzSound();
		switch(Pass_status)
		{
			case PASS_ACCESS:
			
			TakePasswordToAccess();
			break;
			
			case PASS_CHANGE:
			
			TakeNewPassword();
			
			break;
		}
		Clear_pass();
		PassIndex = 0;
		cell = 10;
		LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);// to remove
	}
}



static void EX_INT2_callBack(void)
{
	if (!wait_flag)
	{
		if (EDITMenu_Flag)
		{
			EDITMenu_Flag = FALSE;
			NewPass_Flag = FALSE;
			NewTemp_Flag = FALSE;
			NewSmoke_Flag = FALSE;
			Menu_flag = FALSE;
			cell = 10;
			
		}
		else
		{
			CFG_MenuMess_flag = TRUE;
			EDITMenu_Flag = TRUE;
		}
		wait_flag = TRUE;
	}
	
	
	
	
}


static void EditPassword (void)
{
	
	static bool_t ClearFlag = TRUE;
	if ((WrongPass_Cnt >= MAX_WRONGPASS) && (NewPass_Flag))
	{	
		LCD_SetCursor(SECOND_LINE, FIRST_CELL);
		if (!ClearFlag)
		{
			LCD_WriteString("Cannot take pass");
			clearAtDis_flag = TRUE;
		}
		
		ClearFlag = FALSE;
	}
	else if (NewPass_Flag)
	{
		if (clearAtDis_flag)
		{
			LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
			clearAtDis_flag = FALSE;
		}
		
		if (clearAtDis3_flag)
		{
			LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
			clearAtDis3_flag = FALSE;
		}
		
		if (status != Fire)
		{	
			if (CorrectPass_flag)
			{
				LCD_SetCursor(SECOND_LINE, FIRST_CELL);
				LCD_WriteString("New Pass: ");
			}
			else
			{
				LCD_SetCursor(SECOND_LINE, FIRST_CELL);
				LCD_WriteString("Old Pass: ");
			}
				
			Pass_status = PASS_CHANGE;
			TakePassword();
		}
		else
		{
			NewPass_Flag = FALSE;
		}
		ClearFlag = FALSE;
	}
	else if (!ClearFlag)
	{
		LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
		ClearFlag = TRUE;
		
	}

}







/**********************************************************************/
// Start Timer Counter to make interrupt take 1 sec

static void Timer1_ScheduleFun(void)
{
	
	// Make User wait time To enter New password
	static u8 Pass_cnt = 0;
	
	if (WrongPass_Cnt == MAX_WRONGPASS)
	{
		Pass_cnt++;
	}
	// In sec
	if (Pass_cnt == WAIT_TIME_FOR_RETAKE_PASS)
	{
		Pass_cnt = 0;
		WrongPass_Cnt = 0;
	}
	
	
	// Siren On for 1 Sec and off for 2 Sec.
	static u8 siren_cnt = 0;
	siren_cnt++;
	
	if ( siren_cnt == 1)
	{
		SirenFlag = TRUE;
		
	}
	else
	{
		SirenFlag = FALSE;
		if (siren_cnt >= 3)
		{
			siren_cnt = 0;
		}
	}
	
	// Take Smoke Read Every 1 Sec.
	if (SmokeFlag)
	{
		TakeSmokeRead_Flag = TRUE;
	}
	
	
	// Send Warring To User
	static u8 Warring_Cnt = 0;
	if (Warring_Cnt == 10)
	{
		Warring_Flag = TRUE;
		Warring_Cnt = 0;
	}
	if (status == Heat)
	{
		Warring_Cnt++;
	}
	
	
	// Send Alarm To User
	static u8 Alarm_Cnt = 0;
	if (Alarm_Cnt == 5)
	{
		Alarm_Flag = TRUE;
		Alarm_Cnt = 0;
	}
	if (status == Fire)
	{
		Alarm_Cnt++;
	}
	
	static u8 wait_cnt = 0;
	if (wait_cnt == 1)
	{
		wait_flag = FALSE;
	}
	if (wait_flag)
	{
		wait_cnt++;
	}
	
}





static void Save_Temp(void)
{
	EEPROM_writeBusy(TEMP_ADDRESS, HeatTemp);
	
}

static void Read_Temp(void)
{
	HeatTemp = 10 * EEPROM_readBusy(TEMP_ADDRESS);	
}




static void Save_Smoke(void)
{
	EEPROM_writeBusy(SMOKE_ADDRESS, FireSmoke);
	
}

static void Read_Smoke(void)
{
	FireSmoke = EEPROM_readBusy(SMOKE_ADDRESS);
}



static void CFG_MenuStart(void)
{
	if (status != Fire)
	{
		if (EDITMenu_Flag)
		{
			if (!Menu_flag)
			{
				EDIT_Menu();
				clearAtDis3_flag = TRUE;
			}
			else
			{
				Menu_selection();
			}
		}
		else
		{
			LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
		}
	}
}


static void EDIT_Menu(void)
{
	if (CFG_MenuMess_flag)
	{
		
		LCD_SetCursor(SECOND_LINE, FIRST_CELL);
		LCD_WriteString("    CFG Menu");
		_delay_ms(1500);
		LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
		CFG_MenuMess_flag = FALSE;
		
	}
	
	
	static u8 key;
	key = KEYPAD_GetKey();  
	LCD_SetCursor(SECOND_LINE, FIRST_CELL);
	LCD_WriteString("1-Pass 2-");
	LCD_WriteChar(DEGREE);
	LCD_WriteString("C 3- S");
	
	if (key == '1')
	{
		BuzzSound();
		NewPass_Flag = TRUE;
		NewTemp_Flag = FALSE;
		NewSmoke_Flag = FALSE;
		Menu_flag = TRUE;
	}
	else if (key == '2')
	{
		BuzzSound();
		NewPass_Flag = FALSE;
		NewTemp_Flag = TRUE;
		NewSmoke_Flag = FALSE;
		Menu_flag = TRUE;
		LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
	}
	else if (key == '3')
	{
		BuzzSound();
		NewPass_Flag = FALSE;
		NewTemp_Flag = FALSE;
		NewSmoke_Flag = TRUE;
		LCD_SetCursor(0,5);
		Menu_flag = TRUE;
		LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);
	}
}




// 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
//1  -   P  a  s  s     2  -  *  c     3 -  S


static void Take_New_u8(void)
{
	static u8 key = NO_KEY;
	static u8 startCell = 10;
	LCD_SetCursor(SECOND_LINE, cell);
	
	key = KEYPAD_GetKey();
	
	if ((key >= '0' && key <= '9'))
	{
		BuzzSound();
		LCD_WriteChar(key);
		num = num * 10 + (key - '0');
		cell++;
	}
	if (key == PASS_DELETE && startCell != cell)
	{
		BuzzSound();
		num = num / 10;
		LCD_ClearCell(SECOND_LINE, cell - 1, 1);
		LCD_SetCursor(SECOND_LINE, cell - 1);
		cell--;
	}
	
	
	
	if (key == PASS_DONE)
	{
		BuzzSound();
		cell = 10;
		if (NewTemp_Flag)
		{
			
			HeatTemp = num;
			Save_Temp();
			HeatTemp = 10 *  num;
			NewTemp_Flag = FALSE;
			Menu_flag = FALSE;
		}
		else if (NewSmoke_Flag)
		{
			FireSmoke = num;
			Save_Smoke();
			NewSmoke_Flag = FALSE;
			Menu_flag = FALSE;
		}
		num = 0;
		LCD_ClearCell(SECOND_LINE, FIRST_CELL, CELL_OUT_LCD);// to remove
	}
}

static void Menu_selection(void)
{
	if (NewTemp_Flag)
	{
		LCD_SetCursor(SECOND_LINE, FIRST_CELL);
		LCD_WriteString("New Deg:  ");
		LCD_SetCursor(SECOND_LINE, 15);
		LCD_WriteString("C");
		Take_New_u8();
	}
	if (NewSmoke_Flag)
	{
		LCD_SetCursor(SECOND_LINE, FIRST_CELL);
		LCD_WriteString("New %  :  ");
		LCD_SetCursor(SECOND_LINE, 15);
		LCD_WriteString("%");
		Take_New_u8();
		
	}
	else if (NewPass_Flag)
	{
		EditPassword();
	}
}


static void Send_Bluetooth_Runnable(void)
{
	
	if (Alarm_Flag)
	{
		BCM_SendStringSetter("ALarm! There is Fire!");
		
		Alarm_Flag = FALSE;
	}
	if (Warring_Flag)
	{
		BCM_SendStringSetter("Warring! Heat!");
		
		Warring_Flag = FALSE;
	}
}


static void PassFrom_BlueTooth(void)
{
	static u8 Rec_pass[MAX_PASSWORD];
	
	
	UART_Rec_status_t Rec_status = UART_ReceiveStringGetterAshync();

	if (Rec_status == FRAME_RECEIVED)
	{
		Rec_pass[MAX_PASSWORD - 1] = 0;
		STR_Status result = Str_cmp((c8 *)Rec_pass);
		
		if (result == STR_MATCH)
		{
			Reset_System();
		}
	}
	
	UART_ReceiveStringAshync((c8 *)Rec_pass);
}
