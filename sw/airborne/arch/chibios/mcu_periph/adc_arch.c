/*
 * Copyright (C) 2013 AggieAir, A Remote Sensing Unmanned Aerial System for Scientific Applications
 * Utah State University, http://aggieair.usu.edu/
 *
 * Michal Podhradsky (michal.podhradsky@aggiemail.usu.edu)
 * Calvin Coopmans (c.r.coopmans@ieee.org)
 *
 *
 * This file is part of paparazzi.
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * @brief ChibiOS arch dependent ADC files
 * @note Right now for STM32F1 only
 */

#include "mcu_periph/adc.h"

uint8_t adc_error_flag = 0;
ADCDriver* adcp_err = NULL;

/*
 * ADC defines for Lia board (STM32F105)
 * 5 Channels:
 * 1 - ADC1, Channel 13
 * 2 - ADC2, Channel 10
 * 3 - ADC3, Channel 11
 * 4 - ADC4, Channel 14
 * 5 - Temperature sensor, Channel 16
 *
 * Default buffer depth is equal to MAX_AV_NB_SAMPLE
 * This allows us to always have more or equal samples than the user requires
 *
 * @note: With ChibiOS/RT 2.6.2 and STM32F105 chip the channels have to be interleaved,
 * otherwise the current measurement is affected by the previous channel measured.
 * Most likely bug on ChibiOS ADC driver for this particular chip.
 *
 * For 5 actual channels we have to define (DUMMY stants for 1 dummy channel):
 * CH1 + DUMMY + CH2 + DUMMY + CH3 + DUMMY + CH4 + DUMMY + CH_TEMP = 9 channels
 * Then only every second channel is really read. Might be fixed in next version
 * of ChibiOS
 *
 * V_ref is 3.3V, ADC has 12bit resolution.
 */
#define ADC_NUM_CHANNELS        9
#define ADC_BUF_DEPTH   MAX_AV_NB_SAMPLE
#define ADC_V_REF_MV 3300
#define ADC_12_BIT_RESOLUTION 4096

static adcsample_t adc_samples[ADC_NUM_CHANNELS * ADC_BUF_DEPTH] = {0};

#ifdef USE_AD1
static struct adc_buf * adc1_buffers[ADC_NUM_CHANNELS] = {NULL};
#endif
#ifdef USE_AD2
#error ADC2_not implemented in ChibiOS(STM32F105/7 board)
#endif


/*
 * Callback, fired after half of the buffer is filled (i.e. half of the samples
 * is collected). Since we are assuming continuous ADC conversion, the ADC state is
 * never equal to ADC_COMPLETE.
 * @note: Averaging is done when the subsystems ask for ADC values
 */
void adc1callback(ADCDriver *adcp, adcsample_t *buffer, size_t n) {
  if (adcp->state != ADC_STOP) {
#ifdef USE_AD1
      chSysLockFromIsr();
      for(uint8_t channel = 0; channel < ADC_NUM_CHANNELS; channel+=2) {
          if (adc1_buffers[channel/2] != NULL) {
          adc1_buffers[channel/2]->sum = 0;
          adc1_buffers[channel/2]->av_nb_sample = n;
          for (uint8_t sample = 0; sample < n; sample++) {
                adc1_buffers[channel/2]->sum += buffer[channel + sample*ADC_NUM_CHANNELS];
            }
          adc1_buffers[channel/2]->sum = (adc1_buffers[channel/2]->sum)*ADC_V_REF_MV/ADC_12_BIT_RESOLUTION;
          }
      }
      chSysUnlockFromIsr();
#endif
  }
}

/*
 * ADC Error callback
 * @note: Fired if DMA error happens
 */
static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {
  chSysLockFromIsr();
  adcp_err = adcp;
  adc_error_flag = err;
  chSysUnlockFromIsr();
}

/*
 * Conversion config for ADC
 * @note: Continuous conversion
 * Including Temp sensor, Ts = 7 + 12.5 = 19.5us
 * ADCCLK = 14 MHz
 * Rest of ADcs Ts = 41.5c + 12.5c = 54.0us
 */
static const ADCConversionGroup adcgrpcfg = {
    TRUE,
    ADC_NUM_CHANNELS,
    adc1callback,
    adcerrorcallback,
    0, ADC_CR2_TSVREFE,
    ADC_SMPR1_SMP_AN10(ADC_SAMPLE_41P5) | ADC_SMPR1_SMP_AN11(ADC_SAMPLE_41P5) |
    ADC_SMPR1_SMP_AN13(ADC_SAMPLE_41P5) | ADC_SMPR1_SMP_AN14(ADC_SAMPLE_41P5) | ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_7P5),
    0,
    ADC_SQR1_NUM_CH(ADC_NUM_CHANNELS),
    ADC_SQR2_SQ9_N(ADC_CHANNEL_SENSOR) |ADC_SQR2_SQ7_N(ADC_CHANNEL_IN14),
    ADC_SQR3_SQ5_N(ADC_CHANNEL_IN11) |  ADC_SQR3_SQ3_N(ADC_CHANNEL_IN10)  | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN13)
};

/*
 * Link between ChibiOS ADC drivers and Paparazzi adc_buffers
 */
void adc_buf_channel(uint8_t adc_channel, struct adc_buf * s, uint8_t av_nb_sample){
  adc1_buffers[adc_channel] = s;
  if (av_nb_sample <= MAX_AV_NB_SAMPLE) {
      s->av_nb_sample = av_nb_sample;
  }
  else {
      s->av_nb_sample = MAX_AV_NB_SAMPLE;
  }
}

/*
 * Init ADC driver, buffers and start conversion in the background
 */
void adc_init( void ) {
  adcStart(&ADCD1, NULL);
  adcStartConversion(&ADCD1, &adcgrpcfg, adc_samples, ADC_BUF_DEPTH);
}
