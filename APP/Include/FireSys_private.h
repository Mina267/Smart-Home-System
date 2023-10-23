
#ifndef FIRESYS_PRIVATE_H_
#define FIRESYS_PRIVATE_H_



// Smoke Percentage to Alarm fire
#define SMOKE_2ND_RANGE		10
#define SMOKE_3RD_RANGE		20
#define SMOKE_4RD_RANGE		30
#define SMOKE_ADDRESS		0X11
// Temp in C * 10
#define TEMP_HYST			50
#define TEMP_2ND_RANGE		100
#define TEMP_3RD_RANGE		200
#define TEMP_4RD_RANGE		300
#define TEMP_ADDRESS		0X10

/***************************** Temp System Private *************************/
#define SMOKE_PERC(read)	(((((u32)(read /10)) * (u32)100) / (u32)120))
#define DEGREE				1
#define START_CELL			10
#define FIRST_PASS_INDEX	0
#define START_PASS_ADDRESS	0X30


/**************************** CELLS private **********************/
#define CELL_OUT_LCD	16
#define FIRST_LINE		0
#define SECOND_LINE		1
#define FIRST_CELL		0
#define SECOND_CELL		1
#define LAST_CELL		15
#define PRELAST_CELL	14
#define ONE_CELL		1
#define FOUR_CELL		4
#define _12TH_CELL		12



typedef enum
{
	Fine,
	Heat,
	Fire,
	}FireSys_statue_t;

typedef enum
{
	PASS_CHANGE,
	PASS_ACCESS,
	}PASS_status;


typedef enum
{
	STR_NOMATCH,
	STR_MATCH,
	}STR_Status;



/*********** State Functions ****************/
static void Fine_state(void);
static void heat_state(void);
static void Fire_state(void);
static void Reset_System(void);

/*********** Utils Functions ****************/
static void Welcome(void);
static void print_Temp(void);
static void SirenSound(void);
static void print_Smoke(void);

/*********** Password Functions *************/
static void TakePassword(void);
static void Clear_pass(void);
static STR_Status Str_cmp(c8 *str);
static STR_Status TakePasswordToAccess(void);
static void EditPassword (void);
static void Send_Bluetooth_Runnable(void);
static void PassFrom_BlueTooth(void);

/********** HVAC Function ******************/
static void PUMP_Stop(void);
static void PUMP_Operate(void);
static void SmokeFan_Stop(void);
static void SmokeFan_Operate(void);

/********** Timer Function ******************/
static void Timer1_ScheduleFun(void);

/********** EEPROM Functions ****************/
static void Save_Password(void);
static void Read_Password(void);
static void Save_Temp(void);
static void Read_Temp(void);
static void Save_Smoke(void);
static void Read_Smoke(void);

/************ CFG Menu Function ***************/
static void EDIT_Menu(void);
static void Take_New_u8(void);
static void Menu_selection(void);
static void CFG_MenuStart(void);
static void EX_INT2_callBack(void);




#endif /* FIRESYS_PRIVATE_H_ */