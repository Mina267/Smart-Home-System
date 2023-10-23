


#ifndef SMARTHOME_CFG_H_
#define SMARTHOME_CFG_H_


/********************************** Task *******************************************/
#define TASK_NUM				20
#define MASTER_TASKS_NUM		10

/***************************** RGB MSG Array ***************************************/
#define  RGB_MSG_SIZE			4
/********* RGB MSG Array Indexs **************/
#define  RGB_COLOR_NOTCHG_MSG	0
#define  RGB_COLOR_CHG_MSG		1
#define  RGB_WRG_COLOR_MSG		2
#define  RGB_ENTER_COLOR_MSG	3

/***************************** RGB command Array **********************************/
#define RGB_COMMAND_SIZE		2
/********* RGB command Array Indexs ***********/
#define RGB_BACK_CMD			0
#define RGB_RESET_CMD			1

/************************* General command Array **********************************/
#define GENERAL_COMMAND_SIZE	2


/*********************** Room LEDs lighting Dio Pins ******************************/
#define ROOM1_LEDS				PINC5
#define ROOM2_LEDS				PINC6
#define ROOM3_LEDS				PINC7
#define ROOM4_LEDS				PINB0

/******************** TV relays. Realys connected to TVs **************************/
#define TV_NUM					2
#define _1_TV_RELAY				_1_RELAY
#define _2_TV_RELAY				_2_RELAY

/****************************** SPI Frame Size ************************************/
#define SMARTHOME_FRAME_SIZE	4

/************************** SPI Device Status bits ********************************/
#define MOTORS_STATUS			3
#define FAN						0
#define GRAGEDOOR				1
#define WINDOW_CURTION			2
#define AIRCONDITION			3
#define TEMPCONTROL				4
#define OUTDOORLEDS				5
#define LDR_CONTROL				6

/******************************* EEPROM CFG ***************************************/
#define START_SYSTEM_ADDRESS		0X20
#define START_FRAME_ADDRESS			0X10
#define START_SLAVE_ADDRESS			0X5
#define FIRST_FRAME_INDEX			0
#define FIRST_SYSTEM_INDEX			0

/*********************** TIME in scheduler time 1ms *******************************/
#define BCM_RECEIVE_PERIODICITY		500
#define BCM_SEND_PERIODICITY		10
#define SHOWROOM_PERIODICITY		10
#define SMARTHOME_RUN_PERIODICITY	20
#define SPI_FRAME_PERIODICITY		303
#define SAVE_DATA_PERIODICITY		5000
#define FRAME_RESPONED_PERIODICITY	500

#endif /* SMARTHOME_CFG_H_ */