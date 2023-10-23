


#ifndef DHT22_H_
#define DHT22_H_

typedef enum
{
	DHT22_TAKE_READ,
	DHT22_FINISH,
	DHT22_ERROR,
	}DHTT22_Status;

void DHT22_Init(void);
void DHT22_StartSignal(void);
DHTT22_Status DHT22_HumidityGetter(u16 *pHumidity);
DHTT22_Status DHT22_TemperatureGetter(u16 *pTemperature);

#endif /* DHT22_H_ */