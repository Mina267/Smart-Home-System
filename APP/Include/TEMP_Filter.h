

#ifndef TEMP_FILTER_H_
#define TEMP_FILTER_H_
/**************** TEMP Filter Cfg **********************/
#define MAX_TEMP_READ	10


/****************** private ***************************/
#define FIRST_READ	0


/************************** Temp Func ****************/
void TEMP_FilterInit(void);
void TEMP_FilterRunnable(void);
u16 TEMP_FilterGetter(void);


#endif /* TEMP_FILTER_H_ */