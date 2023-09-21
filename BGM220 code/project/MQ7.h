/*
 * CO.h
 *
 *  Created on: Sep 9, 2023
 *      Author: nthu8
 */

#ifndef MQ7_H_
#define MQ7_H_

#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_iadc.h"

// Set CLK_ADC to 10MHz
#define CLK_SRC_ADC_FREQ          20000000 // CLK_SRC_ADC
#define CLK_ADC_FREQ              10000000 // CLK_ADC - 10MHz max in normal mode

#define IADC_INPUT_0_PORT_PIN     iadcPosInputPortBPin0;

#define IADC_INPUT_0_BUS          BBUSALLOC
#define IADC_INPUT_0_BUSALLOC     GPIO_BBUSALLOC_BEVEN0_ADC0

#define coefficient_A 19.32
#define coefficient_B -0.64
#define R_Load 10.0
/*******************************************************************************
 ***************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/

static volatile short sample;
static volatile double singleResult; // Volts
short get_value_ADC(void);
void CO_intit(void);
float getPPM(void);

#endif /* MQ7_H_ */
