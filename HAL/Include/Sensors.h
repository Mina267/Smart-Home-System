

#ifndef SENSORS_H_
#define SENSORS_H_

#define LM35_CH		CH_0
#define MPX4115_CH	CH_7
#define LDR_CH		CH_0

u16 LIGHT_Read(void);
/* Return temperature in C*10. */
u16 TEMP_Read(void);
/* Return press in KPA * 10. */
u16 PRESS_Read(void);


/* Start Conversion on LM35 Channel */
void TEMP_Convertion(void);
/* Get read after conversion finished */
u16 TEMP_GetRead(void);

/* Start Conversion on Pressure sensor Channel */
void PRESS_Conversion(void);
/* Get read after conversion finished */
u16 PRESS_GetRead(void);

#endif /* SENSORS_H_ */