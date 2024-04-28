/*
  COPYRIGHT (C) 2024 Michal Lenc (michallenc@seznam.cz)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#include <errno.h>
#include <rtems/error.h>
#include <rtems/mw_uid.h>
#include <rtems/rtems/intr.h>
#include <rtems/untar.h>
#include <rtems/score/atomic.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <candev.h>
#include <dev/can/can.h>
#include <dev/can/ctucanfd.h>
#include <dev/can/can-bittiming.h>
#include <dev/can/can-bus.h>
#include <dev/can/ctucanfd.h>

#ifndef CTUCANFD_WORKER_PRIORITY
#define CTUCANFD_WORKER_PRIORITY 121
#endif

static bool initialized = false;

static struct rtems_can_set_bittiming set_nominal_bittiming = {
  .type = RTEMS_CAN_BITTIME_NOMINAL,
  .from = RTEMS_CAN_BITTIME_FROM_BITRATE,
  .bittiming = {
    .bitrate = 500000,
  }
};

static struct rtems_can_set_bittiming set_data_bittiming = {
  .type = RTEMS_CAN_BITTIME_DATA,
  .from = RTEMS_CAN_BITTIME_FROM_BITRATE,
  .bittiming = {
    .bitrate = 2000000,
  }
};

rtems_task rtems_can_receiver(rtems_task_argument arg) {
  struct can_frame frame;
  int fd = (int)arg;
  int ret;

  while (true) {
    ret = read(fd, &frame, sizeof(struct can_frame));
    if (ret < 0) {
      printf("can_receiver: read failed %d\n", ret);
      break;
    }

    pysim_can_add_new_msg(fd, frame.header.can_id, frame.header.dlen, frame.data);
  }

  rtems_task_exit();
}

static int rtems_open_device(char *devpath) {
  rtems_id rx_id;
  int fd = open(devpath, O_RDWR);
  if (fd < 0) {
    printf("rtems_open_device: could not open %s\n", devpath);
    return -1;
  }

  ioctl( fd, RTEMS_CAN_SET_BITRATE, &set_nominal_bittiming);
  ioctl( fd, RTEMS_CAN_SET_BITRATE, &set_data_bittiming);
  ioctl( fd, RTEMS_CAN_CHIP_SET_MODE, CAN_CTRLMODE_FD );

  ioctl( fd, RTEMS_CAN_CHIP_START );

  rtems_task_create(
    rtems_build_name( 'C', 'A', 'N', 'R' ),
    120,
    RTEMS_MINIMUM_STACK_SIZE+0x1000,
    RTEMS_DEFAULT_MODES,
    RTEMS_DEFAULT_ATTRIBUTES,
    &rx_id
    );

  rtems_task_start( rx_id, rtems_can_receiver, fd );

  return fd;
}

static int rtems_send_frame(int fd, uint32_t id, uint16_t len, uint8_t data[]) {
  struct can_frame frame;
  int ret;

  memset( &frame, 0, sizeof( struct can_frame ) );

  frame.header.can_id = id;
  frame.header.dlen = len;
  memcpy(frame.data, data, len);

  ret = write(fd, &frame, can_framesize(&frame));
  if (ret != can_framesize(&frame)) {
    printf( "ERROR: write failed %d\n", ret );
  }

  return 0;
}

static int rtems_close_device(int fd) {
  close(fd);
  return 0;
}

int pysim_can_target_register(struct pysim_can_ops *ops) {
  struct rtems_can_bus *bus1;
  struct rtems_can_bus *bus2;
  int ret;

  if (initialized) {
    printf( "already initialized\n" );
    return 0;
  }

  bus1 = malloc( sizeof( struct rtems_can_bus ) );
  bus2 = malloc( sizeof( struct rtems_can_bus ) );

  *(volatile uint32_t *)(0x43C10000) = 0x01000000;

  bus1->chip = rtems_ctucanfd_initialize(
    0x43c30000,
    61,
    CTUCANFD_WORKER_PRIORITY,
    4,
    RTEMS_INTERRUPT_UNIQUE,
    100000000
  );
  if ( bus1->chip == NULL ) {
    printf( "ERROR: ctucanfd_initialize for 0x43c30000 failed!\n" );
    return -1;
  }

  bus2->chip = rtems_ctucanfd_initialize(
    0x43c70000,
    62,
    CTUCANFD_WORKER_PRIORITY,
    4,
    RTEMS_INTERRUPT_UNIQUE,
    100000000
  );
  if ( bus2->chip == NULL ) {
    printf( "ERROR: ctucanfd_initialize for 0x43c70000 failed!\n" );
    return -1;
  }

  ret = rtems_can_bus_register( bus1, "/dev/can0" );
  if ( ret != 0 ) {
    printf( "ERROR: can_bus_register failed\n" );
    return -1;
  }

  ret = rtems_can_bus_register( bus2, "/dev/can1" );
  if ( ret != 0 ) {
    printf( "ERROR: can_bus_register failed\n" );
    return -1;
  }

  ops->send_frame = rtems_send_frame;
  ops->close_device = rtems_close_device;
  ops->open_device = rtems_open_device;

  initialized = true;

  return 0;
}