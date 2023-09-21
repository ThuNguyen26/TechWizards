/*
 * DHT22.h
 *
 *  Created on: Sep 9, 2023
 *      Author: nthu8
 */

#ifndef DHT22_H_
#define DHT22_H_

#include <stdint.h>
#include <em_gpio.h>
#include "ustimer.h"
typedef struct DHT_22
{
	GPIO_Port_TypeDef _port;
	uint8_t data[7];
	uint8_t _pin;
	uint64_t _lastreadtime;
	bool _lastresult;
	Ecode_t ustimer;
}DHT;
void init(DHT *d, GPIO_Port_TypeDef _port, uint8_t _pin);
float readTemperature(DHT *d, bool force);
float readHumidity(DHT *d, bool force);
bool read(DHT *d, bool force);
uint32_t expectPulse(DHT *d, bool level);

#endif /* DHT22_H_ */
