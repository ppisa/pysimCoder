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
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <candev.h>

bool initialized = false;

struct pysim_can_msg {
  uint32_t id;
  uint16_t dlc;
  uint16_t read;
  uint8_t data[64];
  struct pysim_can_msg *next;
};

static struct pysim_can_ops can_ops;
struct pysim_can_msg *can_msg_rx = NULL;

int pysim_can_register_msg(uint32_t id) {
  struct pysim_can_msg *msg = (struct pysim_can_msg *)malloc(sizeof(struct pysim_can_msg));
  if (msg == NULL) {
    return ENOMEM;
  }
  msg->next = can_msg_rx;
  can_msg_rx = msg;
  msg->id = id;
  memset(msg->data, 0, sizeof(*msg->data));
  msg->read = 0;
  msg->dlc = 0;

  return 0;
}

int pysim_can_add_new_msg(int fd, uint32_t id, uint16_t len, uint8_t data[]) {
  struct pysim_can_msg *msg = can_msg_rx;
  while (msg != NULL) {
    if (msg->id == id) {
      memcpy(msg->data, data, len);
      msg->dlc = len;
      msg->read = 0;
    }
    msg = msg->next;
  }

  return 0;
}

int pysim_can_read_msg(uint32_t id, uint16_t *len, uint8_t data[]) {
  struct pysim_can_msg *msg = can_msg_rx;
  while (msg != NULL) {
    if ((msg->id == id) && (msg->read == 0)) {
      msg->read = 1;
      memcpy(data, msg->data, msg->dlc);
      *len = msg->dlc;
      return 0;
    }

    msg = msg->next;
  }

  /* No message of desired ID found. */

  return ENOMSG;
}

int pysim_can_send_msg(int fd, uint32_t id, uint16_t len, uint8_t data[]) {
  if (can_ops.send_frame == NULL) {
    return -1;
  }

  return can_ops.send_frame(fd, id, len, data);
}

int pysim_can_open(char *devpath) {
  if (can_ops.open_device == NULL) {
    return -1;
  }

  return can_ops.open_device(devpath);
}

int pysim_can_initialize() {
  if (initialized) {
    return 0;
  }

  memset(&can_ops, 0, sizeof(struct pysim_can_ops));

  pysim_can_target_register(&can_ops);

  initialized = true;

  return 0;
}
