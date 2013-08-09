/*
Based on ChibiOS/testhal/STM32F0xx/ADC,

PA2(TX) and PA3(RX) are routed to USART2 38400-8-N-1.

PC0 and PC1 are analog inputs
Connect PC0 to 3.3V and PC1 to GND for analog measurements.

*/
#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "chprintf.h"


#define ADC_GRP1_NUM_CHANNELS   1
#define ADC_GRP1_BUF_DEPTH      8

#define ADC_GRP2_NUM_CHANNELS   4
#define ADC_GRP2_BUF_DEPTH      16

#define SHELL_WA_SIZE   THD_WA_SIZE(1024)

static adcsample_t samples1[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
static adcsample_t samples2[ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH];

/*
 * ADC streaming callback.
 */
float pc0 = 0;
float pc0_volt = 0;
float pc1_volt = 0;
float pc0_temp = 0;

size_t nx = 0, ny = 0;
static void adccallback(ADCDriver *adcp, adcsample_t *buffer, size_t n) {
    (void)adcp;
    if (samples2 == buffer) {
	nx += n;
    }
    else {
	ny += n;
    }

    pc0 = (float) samples2[0];
    pc0_volt = (pc0 / 4095) * 3.3;
    pc0_temp = pc0_volt * 100;
}

static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {
    (void)adcp;
    (void)err;
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 1 channel, SW triggered.
 * Channels:    IN10.
 */
static const ADCConversionGroup adcgrpcfg1 = {
    FALSE,
    ADC_GRP1_NUM_CHANNELS,
    NULL,
    adcerrorcallback,
    ADC_CFGR1_RES_12BIT,                              /* CFGRR1 */
    ADC_TR(0, 0),                                     /* TR */
    ADC_SMPR_SMP_1P5,                                 /* SMPR */
    ADC_CHSELR_CHSEL10                                /* CHSELR */
};

/*
 * ADC conversion group.
 * Mode:        Continuous, 16 samples of 8 channels, SW triggered.
 * Channels:    IN10, IN11, Sensor, VRef.
 */
static const ADCConversionGroup adcgrpcfg2 = {
    TRUE,
    ADC_GRP2_NUM_CHANNELS,
    adccallback,
    adcerrorcallback,
    ADC_CFGR1_RES_12BIT,                              /* CFGRR1 */
    ADC_TR(0, 0),                                     /* TR */
    ADC_SMPR_SMP_28P5,                                /* SMPR */
    ADC_CHSELR_CHSEL10 | ADC_CHSELR_CHSEL11 |
    ADC_CHSELR_CHSEL16 | ADC_CHSELR_CHSEL17           /* CHSELR */
};

/*
 * Red LEDs blinker thread, times are in milliseconds.
 */
static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *arg) {
  (void)arg;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palSetPad(GPIOC, GPIOC_LED4);
    chThdSleepMilliseconds(500);
    palClearPad(GPIOC, GPIOC_LED4);
    chThdSleepMilliseconds(500);
  }
}

static void cmd_temp(BaseSequentialStream *chp, int argc, char *argv[]) {
    chprintf(chp, "Temp: %.2f \n\r", pc0_temp);  
}

static void cmd_volt(BaseSequentialStream *chp, int argc, char *argv[]) {
    chprintf(chp, "PC0 DCV: %.2f \n\r", pc0_volt);  
    chprintf(chp, "PC1 DCV: %.2f \n\r", pc1_volt);  
}

static void cmd_ledOn(BaseSequentialStream *chp, int argc, char *argv[]) {
    chprintf(chp, "PC9 Led on \n\r");  
    palSetPad(GPIOC, GPIOC_LED3); // led on
}

static void cmd_ledOff(BaseSequentialStream *chp, int argc, char *argv[]) {
    chprintf(chp, "PC9 Led off \n\r");  
    palClearPad(GPIOC, GPIOC_LED3); // led off
}
static void cmd_adcl(BaseSequentialStream *chp, int argc, char *argv[]) {
}

static const ShellCommand shCmds[] = {
    {"temp",   cmd_temp},
    {"volt",   cmd_volt},
    {"ledon",  cmd_ledOn},
    {"ledoff", cmd_ledOff},
    {"adcl",   cmd_adcl},
    {NULL, NULL}
};

static const ShellConfig shCfg = {
    (BaseSequentialStream *)&SD2,
    shCmds
};



/*
 * Application entry point.
 */
int main(void) {

    Thread *sh = NULL;

    halInit();
    chSysInit();

    sdStart(&SD2, NULL);

    palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(1));      /* USART1 TX.       */
    palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(1));      /* USART1 RX.       */

    /*
    * Setting up analog inputs used by the demo.
    */
    palSetGroupMode(GPIOC, PAL_PORT_BIT(0) | PAL_PORT_BIT(1),
		  0, PAL_MODE_INPUT_ANALOG);

    /*
    * Creates the blinker thread.
    */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    /*
    * Activates the ADC1 driver and the temperature sensor.
    */
    adcStart(&ADCD1, NULL);
    adcSTM32SetCCR(ADC_CCR_VBATEN | ADC_CCR_TSEN | ADC_CCR_VREFEN);

    /*
    * Linear conversion.
    */
    adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
    chThdSleepMilliseconds(1000);

    /*
    * Starts an ADC continuous conversion.
    */
    adcStartConversion(&ADCD1, &adcgrpcfg2, samples2, ADC_GRP2_BUF_DEPTH);

    shellInit();

    for (;;) {
    if (!sh)
      sh = shellCreate(&shCfg, SHELL_WA_SIZE, NORMALPRIO);
    else if (chThdTerminated(sh)) {
      chThdRelease(sh);
      sh = NULL;
    }
    chThdSleepMilliseconds(1000);
    }
}
