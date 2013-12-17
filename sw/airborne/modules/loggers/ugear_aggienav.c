/*
 * Copyright (C) 2013 Michal Podhradsky
 * Utah State University, http://aggieair.usu.edu/
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
 * @file ugear_aggienav.c
 *
 * Ugear for aggienav
 *
 *
 * @author Michal Podhradsky <michal.podhradsky@aggiemail.usu.edu>
 */
#include "modules/loggers/ugear.h"

#ifdef USE_GX3
//#include "subsystems/ahrs/ahrs_gx3.h"
#include "subsystems/imu/imu_gx3.h"
#endif

static uint8_t UGEAR_GPS_buf[UGEAR_MESSAGE_SIZE_GPS];
static uint8_t UGEAR_IMU_buf[UGEAR_MESSAGE_SIZE_ATTITUDE];

void ugear_init(void) {
	/* Message Buffer Headers never change */
	UGEAR_IMU_buf[0] = UGEAR_MSG0;
	UGEAR_IMU_buf[1] = UGEAR_MSG1;
        UGEAR_IMU_buf[2] = UGEAR_HEADER_ATTITUDE;
        UGEAR_IMU_buf[3] = UGEAR_MESSAGE_SIZE_ATTITUDE;

	UGEAR_GPS_buf[0] = UGEAR_MSG0;
	UGEAR_GPS_buf[1] = UGEAR_MSG1;
        UGEAR_GPS_buf[2] = UGEAR_HEADER_GPS;
        UGEAR_GPS_buf[3] = UGEAR_MESSAGE_SIZE_GPS;
}

void ugear_periodic(void) {
  if (gps_ubx.new_data_available) {
    ugear_send_gps();
    gps_ubx.new_data_available = FALSE;
  }
    ugear_send_attitude();
}

void ugear_send_gps(void) {
  LED_TOGGLE(UGEAR_GPS_LED);
  static uint8_t cksum0, cksum1, gps_index;
  cksum0 = 0;
  cksum1 = 0;
  gps_index = UGEAR_DATA_INDEX;

#ifdef USE_CHIBIOS_RTOS
  chMtxLock(&gps_mutex_flag); //Lock gps struct
#endif
  //GPS ECEFCORD position, int32
  for (uint8_t i = 0;i<4;i++){
	  UGEAR_GPS_buf[gps_index] = 0xFF&(gps.ecef_pos.x>>(i*8));
	  gps_index++;
  }
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.ecef_pos.y>>(i*8));
    gps_index++;
  }
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.ecef_pos.z>>(i*8));
    gps_index++;
  }
  //GPS LLA, int32
  for (uint8_t i = 0;i<4;i++){
	  UGEAR_GPS_buf[gps_index] = 0xFF&(gps.lla_pos.lat>>(i*8));
	  gps_index++;
  }
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.lla_pos.lon>>(i*8));
    gps_index++;
  }
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.lla_pos.alt>>(i*8));
    gps_index++;
  }
  //GPS hmsl, int32
  for (uint8_t i = 0; i<4; i++){
   UGEAR_GPS_buf[gps_index] = 0xFF&(gps.hmsl>>(i*8));
   gps_index++;
  }
  //GPS ECEFCORD speed, int32
  for (uint8_t i = 0;i<4;i++){
	  UGEAR_GPS_buf[gps_index] = 0xFF&(gps.ecef_vel.x>>(i*8));
	  gps_index++;
  }
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.ecef_vel.y>>(i*8));
    gps_index++;
  }
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.ecef_vel.z>>(i*8));
    gps_index++;
  }
  //GPS pacc, uint32
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.pacc>>(i*8));
    gps_index++;
  }
  //GPS sacc, uint32
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.sacc>>(i*8));
    gps_index++;
  }
  //GPS tow, uint32
  for (uint8_t i = 0; i<4; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.tow>>(i*8));
    gps_index++;
  }
  //GPS pdop, uint16
  for (uint8_t i = 0; i<2; i++){
    UGEAR_GPS_buf[gps_index] = 0xFF&(gps.pdop>>(i*8));
    gps_index++;
  }
  //GPS num_sv, uint8
  UGEAR_GPS_buf[gps_index] = gps.fix;
  gps_index++;

  //GPS num_sv, uint8
  UGEAR_GPS_buf[gps_index] = gps.num_sv;
  gps_index++;
#ifdef USE_CHIBIOS_RTOS
  chMtxUnlock();//Unlock gps struct
#endif


  // calculate checksum & send
  ugear_cksum(UGEAR_MESSAGE_SIZE_GPS, (uint8_t *)UGEAR_GPS_buf, &cksum0, &cksum1);
  UGEAR_GPS_buf[UGEAR_MESSAGE_SIZE_GPS-2] = cksum0;
  UGEAR_GPS_buf[UGEAR_MESSAGE_SIZE_GPS-1] = cksum1;
  send_buf(UGEAR_MESSAGE_SIZE_GPS,UGEAR_GPS_buf);
}

void ugear_send_attitude(void) {
  //RunOnceEvery(100, LED_TOGGLE(UGEAR_ATTITUDE_LED));
  static uint8_t cksum0, cksum1, ugear_index;
  cksum0 = 0;
  cksum1 = 0;
  ugear_index = UGEAR_DATA_INDEX;

  // timestamp from GX3 in sec. float
  static float tmp;
#ifdef USE_CHIBIOS_RTOS
  chMtxLock(&states_mutex_flag);//Lock imu/ahrs/state struct
#endif

#ifdef USE_GX3
  //tmp = ((float)imu_gx3.gx3_time)/62500.0;
  //tmp = ((float)ahrs_impl.gx3_time)/62500.0;
#endif
  memcpy(&UGEAR_IMU_buf[ugear_index], &tmp, sizeof(float));
  ugear_index += sizeof(float);

  // System time
  tmp = (float)chTimeNow()/CH_FREQUENCY;
  memcpy(&UGEAR_IMU_buf[ugear_index], &tmp, sizeof(float));
  ugear_index += sizeof(float);

  // Acceleration, FIXME: in imu coordinate system
  memcpy(&UGEAR_IMU_buf[ugear_index], &imuf.accel.x, sizeof(float));
  ugear_index += sizeof(float);
  memcpy(&UGEAR_IMU_buf[ugear_index], &imuf.accel.y, sizeof(float));
  ugear_index += sizeof(float);
  memcpy(&UGEAR_IMU_buf[ugear_index], &imuf.accel.z, sizeof(float));
  ugear_index += sizeof(float);
  // Angular rates, body frame
  memcpy(&UGEAR_IMU_buf[ugear_index], &(stateGetBodyRates_f()->p), sizeof(float));
  ugear_index += sizeof(float);
  memcpy(&UGEAR_IMU_buf[ugear_index], &(stateGetBodyRates_f()->q), sizeof(float));
  ugear_index += sizeof(float);
  memcpy(&UGEAR_IMU_buf[ugear_index], &(stateGetBodyRates_f()->r), sizeof(float));
  ugear_index += sizeof(float);
  // Attitude, eulers, body frame
  memcpy(&UGEAR_IMU_buf[ugear_index], &(stateGetNedToBodyEulers_f()->phi), sizeof(float));
  ugear_index += sizeof(float);
  memcpy(&UGEAR_IMU_buf[ugear_index], &(stateGetNedToBodyEulers_f()->theta), sizeof(float));
  ugear_index += sizeof(float);
  memcpy(&UGEAR_IMU_buf[ugear_index], &(stateGetNedToBodyEulers_f()->psi), sizeof(float));
  ugear_index += sizeof(float);
#ifdef USE_CHIBIOS_RTOS
  chMtxUnlock();//Unlock imu/ahrs/state struct
#endif

  // calculate checksum & send
  ugear_cksum(UGEAR_MESSAGE_SIZE_ATTITUDE, (uint8_t *)UGEAR_IMU_buf, &cksum0, &cksum1 );
  UGEAR_IMU_buf[UGEAR_MESSAGE_SIZE_ATTITUDE-2] = cksum0;
  UGEAR_IMU_buf[UGEAR_MESSAGE_SIZE_ATTITUDE-1] = cksum1;
  send_buf(UGEAR_MESSAGE_SIZE_ATTITUDE,UGEAR_IMU_buf);
  //send_buf(50,UGEAR_IMU_buf);
}

void send_buf(uint16_t size, uint8_t *_buf){
#ifdef USE_CHIBIOS_RTOS
  uart_transmit_buffer(&UGEAR_PORT, _buf, size);
#else
  for (uint16_t i = 0;i<size;i++){
    uart_transmit(&UGEAR_PORT, _buf[i]);
  }
#endif
}

void ugear_cksum(uint16_t packet_length, uint8_t *buf, uint8_t *cksum0, uint8_t *cksum1 ) {
  static uint8_t c0, c1;
  static uint16_t i, size;
  c0 = 0;
  c1 = 0;
  i = 0;
  size = packet_length - 2;

  for ( i = 0; i < size; i++ ) {
  	c0 += (uint8_t)buf[i];
    	c1 += c0;
  }
  *cksum0 = c0;
  *cksum1 = c1;
}
