

#ifndef RGB_PRIVATE_H_
#define RGB_PRIVATE_H_

typedef enum
{
	STR_NOMATCH = 0,
	STR_MATCH = 1,
}STR_Status;



#define RGB_NUM			3

#define RED_INDEX		0
#define GREEN_INDEX		1
#define BLUE_INDEX		2



extern const u8 RGB_ColorArray[COLOR_NUM][RGB_NUM];
extern const c8 *RGB_NamesColorArray[COLOR_NUM];
void CallBackFunc_timer2_ShowRoom(void);
static STR_Status Str_cmp(c8 *strReceive, c8 *StrExpected);
static void Str_toUpper(c8 *str);

#endif /* RGB_PRIVATE_H_ */