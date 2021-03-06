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
 * @file arch/chibios/mcu_periph/uart_arch.h
 * UART/Serial driver implementation for ChibiOS arch
 *
 * ChibiOS has a high level Serial Driver, for Paparazzi it is more convenient
 * than pure UART driver (which needs callbacks etc.). This implementation is
 * asynchronous and the RX thread has to use event flags. See ChibiOS documen-
 * tation.
 */
#ifndef CHIBIOS_UART_ARCH_H
#define CHIBIOS_UART_ARCH_H

#include "mcu_periph/uart.h"
#include "hal.h"

#define B1200    1200
#define B2400    2400
#define B4800    4800
#define B9600    9600
#define B19200   19200
#define B38400   38400
#define B57600   57600
#define B115200  115200
#define B230400  230400
#define B921600  921600
#define B100000  100000

#define ch_uart_receive_downlink(_port, _flag, _callback, _arg) {   \
    if ((_flag & (SD_FRAMING_ERROR | SD_OVERRUN_ERROR |         \
                  SD_NOISE_ERROR)) != 0) {                      \
        if (_flag & SD_OVERRUN_ERROR) {                         \
            _port.ore++;                                        \
        }                                                       \
        if (_flag & SD_NOISE_ERROR) {                           \
            _port.ne_err++;                                     \
        }                                                       \
        if (_flag & SD_FRAMING_ERROR) {                         \
            _port.fe_err++;                                     \
        }                                                       \
    }                                                           \
    if (_flag & CHN_INPUT_AVAILABLE) {                         \
       msg_t charbuf;                                           \
       do {                                                     \
           charbuf = sdGetTimeout((SerialDriver*)_port.reg_addr, TIME_IMMEDIATE);\
          if ( charbuf != Q_TIMEOUT ) {                         \
             _callback(_arg, charbuf);                          \
          }                                                     \
       }                                                        \
       while (charbuf != Q_TIMEOUT);                            \
    }                                                           \
}


#endif /* STM32_UART_ARCH_H */