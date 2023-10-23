#include "StdTypes.h"
#include "DIO_interface.h"
#include "Timers.h"
#include "RGB_Cfg.h"
#include "RGB_Interface.h"
#include "RGB_Private.h"
#include "MemMap.h"

#include "LCD_interface.h"



static volatile bool_t EnableChange_Flag = TRUE;
static volatile bool_t ChangeColor_Flag = FALSE;
static volatile RGB_Color_type FirstColor;
static volatile RGB_Color_type SecondColor;
static u8 TmpColor[RGB_NUM];

void RGB_Init(void)
{
	TIMER0_Init(  TIMER0_PHASECORRECT_MODE, TIMER0_SCALER_256);
	TIMER0_OC0Mode(OC0_NON_INVERTING);
	OCR0 = 0;
	
	Timer1_Init(  TIMER1_PHASE_ICR_TOP_MODE, TIMER1_SCALER_256);
	Timer1_OCRA1Mode(OCRA_NON_INVERTING);
	Timer1_OCRB1Mode(OCRB_NON_INVERTING);
	ICR1 = 255;
	OCR1A = 0;
	OCR1B = 0;
	
}



void RGB_LedColor(RGB_Color_type Color)
{
	RED_PWM = RGB_ColorArray[Color][RED_INDEX];
	GREEN_PWM = RGB_ColorArray[Color][GREEN_INDEX];
	BLUE_PWM = RGB_ColorArray[Color][BLUE_INDEX];
}



void RGB_LedMakeColor(u8 red, u8 green, u8 blue)
{
	RED_PWM = red;
	GREEN_PWM = green;
	BLUE_PWM = blue;
}

RGB_Status RGB_ShowRoomSetColors(RGB_Color_type Color1, RGB_Color_type Color2)
{
	
	RGB_Status Status = RGB_NDONE;
	
	if (EnableChange_Flag)
	{
		/* Set First Color to RGB */
		RGB_LedColor(Color1);
		/* Save name of colors */
		FirstColor = Color1;
		SecondColor = Color2;
		
		/* save color in tmp to change in */
		TmpColor[RED_INDEX] = RGB_ColorArray[FirstColor][RED_INDEX];
		TmpColor[GREEN_INDEX] = RGB_ColorArray[FirstColor][GREEN_INDEX];
		TmpColor[BLUE_INDEX] = RGB_ColorArray[FirstColor][BLUE_INDEX];
		
		RGB_ShowRoom_Enable();
		
		/* Do not take another color until first finish */
		EnableChange_Flag = FALSE;
		/* Update status */
		Status = RGB_DONE;
	}
	
	return Status;
}


/* 19 us*/
void RGB_ShowRoom_Runnable (void)
{
	if (ChangeColor_Flag)
	{
		bool_t IsChange_Flag = FALSE;
		
		if (TmpColor[RED_INDEX] < RGB_ColorArray[SecondColor][RED_INDEX])
		{
			TmpColor[RED_INDEX]++;
			RED_PWM = TmpColor[RED_INDEX];
			IsChange_Flag = TRUE;
		}
		else if (TmpColor[RED_INDEX] > RGB_ColorArray[SecondColor][RED_INDEX])
		{
			TmpColor[RED_INDEX]--;
			RED_PWM = TmpColor[RED_INDEX];
			IsChange_Flag = TRUE;
		}
		
		if (TmpColor[GREEN_INDEX] < RGB_ColorArray[SecondColor][GREEN_INDEX])
		{
			TmpColor[GREEN_INDEX]++;
			GREEN_PWM = TmpColor[GREEN_INDEX];
			IsChange_Flag = TRUE;
		}
		else if (TmpColor[GREEN_INDEX] > RGB_ColorArray[SecondColor][GREEN_INDEX])
		{
			TmpColor[GREEN_INDEX]--;
			GREEN_PWM = TmpColor[GREEN_INDEX];
			IsChange_Flag = TRUE;
		}
		
		if (TmpColor[BLUE_INDEX] < RGB_ColorArray[SecondColor][BLUE_INDEX])
		{
			TmpColor[BLUE_INDEX]++;
			BLUE_PWM = TmpColor[BLUE_INDEX];
			IsChange_Flag = TRUE;
		}
		else if (TmpColor[BLUE_INDEX] > RGB_ColorArray[SecondColor][BLUE_INDEX])
		{
			TmpColor[BLUE_INDEX]--;
			BLUE_PWM = TmpColor[BLUE_INDEX];
			IsChange_Flag = TRUE;
		}
		
		if (IsChange_Flag == FALSE)
		{
			EnableChange_Flag = TRUE;
			RGB_ShowRoom_Disable();
		}	
	}
}



void RGB_ShowRoom_Enable(void)
{
	ChangeColor_Flag = TRUE;
}

void RGB_ShowRoom_Disable(void)
{
	ChangeColor_Flag = FALSE;
}



void CallBackFunc_timer2_ShowRoom(void)
{
	TCNT2 = 131;
	static u8 Cnt = 0;
	Cnt++;
	
	if (Cnt == SHOW_ROOM_TIME_MS / 2)
	{
		RGB_ShowRoom_Runnable();
	}
	
	

	
}






RGB_Color_type RGB_LedColorIndex(c8 *StrColor)
{
	
	RGB_Color_type ColorIndex;
	for (ColorIndex = 0; ColorIndex < COLOR_NUM; ColorIndex++)
	{
		STR_Status StrStatus = Str_cmp(StrColor, (c8*)RGB_NamesColorArray[ColorIndex]);
		if (StrStatus == STR_MATCH)
		{
			return ColorIndex;
		}
	}
	return COLOR_NUM;
}


static void Str_toUpper(c8 *str)
{
	for (u8 Index = 0; str[Index]; Index++)
	{
		if (str[Index] >= 'a' && str[Index] <= 'z')
		{
			str[Index] = str[Index] - 'a' + 'A';
		}
	}
}

static STR_Status Str_cmp(c8 *strReceive, c8 *StrExpected)
{
	Str_toUpper(strReceive);
	for (u8 Index = 0; strReceive[Index] || StrExpected[Index]; Index++)
	{
		if (strReceive[Index] != StrExpected[Index])
		{
			return STR_NOMATCH;
		}
	}
	return STR_MATCH;
}









void RGB_LedColorFullRange(RGB_Color_type Color)
{
	
	if (RGB_ColorArray[Color][RED_INDEX] == 255)
	{
		TIMER0_OC0Mode(OC0_DISCONNECTED);
		DIO_WritePin(RED_PIN, HIGH);
	}
	else
	{
		TIMER0_OC0Mode(OC0_INVERTING);
		RED_PWM = 255 - RGB_ColorArray[Color][RED_INDEX];
	}
	
	
	if (RGB_ColorArray[Color][GREEN_INDEX] == 255)
	{
		Timer1_OCRA1Mode(OC0_DISCONNECTED);
		DIO_WritePin(GREEN_PIN, HIGH);
	}
	else
	{
		Timer1_OCRA1Mode(OCRA_INVERTING);
		GREEN_PWM = 255 - RGB_ColorArray[Color][GREEN_INDEX];
	}
	
	
	if (RGB_ColorArray[Color][BLUE_INDEX] == 255)
	{
		Timer1_OCRB1Mode(OC0_DISCONNECTED);
		DIO_WritePin(BLUE_PIN, HIGH);
	}
	else
	{
		Timer1_OCRB1Mode(OCRA_INVERTING);
		BLUE_PWM = 255 - RGB_ColorArray[Color][BLUE_INDEX];
		
	}
}

