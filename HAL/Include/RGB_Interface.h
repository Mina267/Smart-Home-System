

#ifndef RGB_INTERFACE_H_
#define RGB_INTERFACE_H_

typedef enum
{
	 RED = 0,
	 GREEN,
	 BLUE,
	 BLACK,
	 WHITE,
	 YELLOW,
	 CYAN,
	 MAGENTA,
	 SILVER,
	 GRAY,
	COLOR_NUM,
	}RGB_Color_type;

typedef enum
{
	RGB_NDONE,
	RGB_DONE
	}RGB_Status;
	
void RGB_Init(void);
void RGB_LedMakeColor(u8 red, u8 green, u8 blue);
void RGB_LedColor(RGB_Color_type Color);
void RGB_LedColorFullRange(RGB_Color_type Color);
RGB_Status RGB_ShowRoomSetColors(RGB_Color_type Color1, RGB_Color_type Color2);
void RGB_ShowRoom_Runnable (void);
RGB_Color_type RGB_LedColorIndex(c8 *StrColor);
void RGB_ShowRoom_Enable(void);
void RGB_ShowRoom_Disable(void);

#endif /* RGB_INTERFACE_H_ */