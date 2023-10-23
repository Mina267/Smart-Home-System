

#ifndef SMARTHOME_H_
#define SMARTHOME_H_

/******* Task Struct ***********/
typedef struct
{
	c8 *str;
	void (*Fptr) (u16);
	}SmartHome_task_t;

/********* Sysytem Initialization *************/
void SmartHome_Init(void);
/******** system Runnable function ************/
void SmartHome_Runnable(void);


#endif /* SMARTHOME_H_ */