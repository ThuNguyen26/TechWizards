/*
 * Pzem.h
 *
 *  Created on: Sep 9, 2023
 *      Author: nthu8
 */

#ifndef PZEM_H_
#define PZEM_H_

#include <stdint.h>
#include <stdbool.h>
#include <sl_iostream.h>
typedef struct Pzem004
{
	float voltage;
	float current;
	float power;
	float energy;
	float freq;
	float pf;
	uint8_t data[18];
	uint8_t _addr;
	uint64_t _lastRead;
	bool _isConnected;
	sl_iostream_t* _stream;
}Pzem;
void Pzem_Init(Pzem *p, sl_iostream_t *stream, uint8_t addr);
uint16_t CRC16(const uint8_t *data, uint16_t len);
void setCRC(uint8_t *buf, uint16_t len);
bool checkCRC(const uint8_t *buf, uint16_t len);
uint16_t receive(Pzem *p, uint8_t *resp, uint16_t len);
bool sendCmd8(Pzem *p, uint8_t cmd, uint16_t rAddr, uint16_t val, bool check, uint16_t slave_addr);
void updateValues(Pzem *p);
bool resetEnergy(Pzem *p);

#endif /* PZEM_H_ */
