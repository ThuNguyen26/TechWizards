/*
 * DHT22.c
 *
 *  Created on: Sep 9, 2023
 *      Author: nthu8
 */
#include "sl_sleeptimer.h"
#include "em_gpio.h"
#include "ustimer.h"
#include "em_cmu.h"
#include "DHT22.h"

void init(DHT *d, GPIO_Port_TypeDef _port, uint8_t _pin){
	 d->_lastreadtime = sl_sleeptimer_get_tick_count64();
	 USTIMER_Init();
	 d->_port = _port;
	 d->_pin = _pin;
}
float readTemperature(DHT *d, bool force){
    float temp = 0;
      if (read(d, force))
      {
        temp = ((uint32_t)(d->data[2] & 0x7F)) << 8 | d->data[3];
        temp *= 0.1;
        if (d->data[2] & 0x80)
        {
          temp *= -1;
        }
      }
      return temp;
}
float readHumidity(DHT *d, bool force){
    float hm = 0;
      if (read(d, force))
      {
        hm = ((uint32_t)d->data[0]) << 8 | d->data[1];
        hm *= 0.1;
      }
      return hm;
}
uint32_t expectPulse(DHT *d, bool level)
{
  uint32_t count = 0;
  while (GPIO_PinInGet(d->_port, d->_pin) == level)
  {
    if(count++ >= 1500)// * CMU_ClockFreqGet(cmuClock_SYSCLK))
    	return 0xffffffffu;
  }
  return count;
}
bool read(DHT *d, bool force){
    uint64_t currenttime = sl_sleeptimer_get_tick_count64();
    if(!force && ((currenttime - d->_lastreadtime) < 66000)){
        return d->_lastresult;
    }
    d->_lastreadtime = sl_sleeptimer_get_tick_count64();
    d->data[0] = d->data[1] = d->data[2] = d->data[3] = d->data[4] = 0;
    GPIO_PinModeSet(d->_port, d->_pin, gpioModeInputPull, 1);
    d->ustimer = USTIMER_Delay(1000);
    GPIO_PinModeSet(d->_port, d->_pin, gpioModePushPull, 0);
    d->ustimer = USTIMER_Delay(1100);
    uint32_t cycles[80];
    {
        GPIO_PinModeSet(d->_port, d->_pin, gpioModeInputPull, 1);
        d->ustimer = USTIMER_DelayIntSafe(55);
        if(expectPulse(d, 0) == 0xffffffffu){
        	d->_lastresult = false;
        	return d->_lastresult;
        }
        if(expectPulse(d, 1) == 0xffffffffu){
        	d->_lastresult = false;
        	return d->_lastresult;
        }
        for (int i = 0; i < 80; i += 2){
            cycles[i] = expectPulse(d, 0);
            cycles[i + 1] = expectPulse(d, 1);
            if(cycles[i] == 0xffffffffu || cycles[i+1] == 0xffffffffu){
            		d->_lastresult = false;
            		return d->_lastresult;
            }
        }
    }
    for (int i = 0; i < 40; ++i)
    {
        uint32_t lowCycles = cycles[2 * i];
        uint32_t highCycles = cycles[2 * i + 1];
        if ((lowCycles == 0xffffffffu) || (highCycles == 0xffffffffu))
        {
        		d->_lastresult = false;
        		return d->_lastresult;
        }
        d->data[i / 8] <<= 1;
        if (highCycles > lowCycles)
        {
          d->data[i / 8] |= 1;
        }
    }
    if (d->data[4] == ((d->data[0] + d->data[1] + d->data[2] + d->data[3]) & 0xFF)){
        d->_lastresult = true;
        return d->_lastresult;
    }
    else
    {
        d->_lastresult = false;
        return d->_lastresult;
    }
}

