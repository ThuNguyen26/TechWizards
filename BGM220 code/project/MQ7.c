/*
 * MQ7.c
 *
 *  Created on: Sep 9, 2023
 *      Author: nthu8
 */
#include "MQ7.h"
#include <math.h>
void CO_intit(void){
	// Declare init structs
	  IADC_Init_t init = IADC_INIT_DEFAULT;
	  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
	  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
	  IADC_SingleInput_t initSingleInput = IADC_SINGLEINPUT_DEFAULT;

	  // Enable IADC0 and GPIO clock branches

	  /* Note: For EFR32xG21 radio devices, library function calls to
	   * CMU_ClockEnable() have no effect as oscillators are automatically turned
	   * on/off based on demand from the peripherals; CMU_ClockEnable() is a dummy
	   * function for EFR32xG21 for library consistency/compatibility.
	   */
	  CMU_ClockEnable(cmuClock_IADC0, true);
	  CMU_ClockEnable(cmuClock_GPIO, true);

	  // Reset IADC to reset configuration in case it has been modified by
	  // other code
	  IADC_reset(IADC0);

	  // Select clock for IADC
	  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);  // FSRCO - 20MHz

	  // Modify init structs and initialize
	  init.warmup = iadcWarmupKeepWarm;

	  // Set the HFSCLK prescale value here
	  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

	  // Configuration 0 is used by both scan and single conversions by default
	  // Use internal bandgap (supply voltage in mV) as reference
	  initAllConfigs.configs[0].reference = iadcCfgReferenceVddx;
	  initAllConfigs.configs[0].vRef = 1650;
	  initAllConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

	  // Divides CLK_SRC_ADC to set the CLK_ADC frequency
	  initAllConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
	                                             CLK_ADC_FREQ,
	                                             0,
	                                             iadcCfgModeNormal,
	                                             init.srcClkPrescale);

	  // Assign pins to positive and negative inputs in differential mode
	  initSingleInput.posInput   = IADC_INPUT_0_PORT_PIN;
	  initSingleInput.negInput   = iadcNegInputGnd;

	  // Initialize the IADC
	  IADC_init(IADC0, &init, &initAllConfigs);

	  // Initialize the Single conversion inputs
	  IADC_initSingle(IADC0, &initSingle, &initSingleInput);

	  // Allocate the analog bus for ADC0 inputs
	  GPIO->IADC_INPUT_0_BUS |= IADC_INPUT_0_BUSALLOC;
}

short get_value_ADC(void){
	IADC_command(IADC0, iadcCmdStartSingle);
	// Wait for conversion to be complete
	while((IADC0->STATUS & (_IADC_STATUS_CONVERTING_MASK | _IADC_STATUS_SINGLEFIFODV_MASK)) != IADC_STATUS_SINGLEFIFODV);
	// Get ADC result
	sample = IADC_pullSingleFifoResult(IADC0).data;
	singleResult = (sample * 3.3) / 0xFFF;
	return sample;
}
float getPPM(void){
	short adc = get_value_ADC();
	float ratio = (3.3 - singleResult) / singleResult;
	float ppm = (float)(coefficient_A * pow(ratio, coefficient_B));
	return ppm;
}

