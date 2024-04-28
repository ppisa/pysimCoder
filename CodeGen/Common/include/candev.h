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

#ifndef CANDEV_H
#define CANDEV_H

#include <stdbool.h>
#include <stdint.h>

struct pysim_can_ops {
  int (*open_device)(char *devpath);
  int (*send_frame)(int fd, uint32_t id, uint16_t len, uint8_t data[]);
  int (*close_device)(int fd);
};

int pysim_can_initialize(void);
int pysim_can_open(char *devpath);
int pysim_can_register_msg(uint32_t id);
int pysim_can_add_new_msg(int fd, uint32_t id, uint16_t len, uint8_t data[]);
int pysim_can_read_msg(uint32_t id, uint16_t *len, uint8_t data[]);
int pysim_can_send_msg(int fd, uint32_t id, uint16_t len, uint8_t data[]);

int pysim_can_target_register(struct pysim_can_ops *ops);

#endif /* CANDEV_H */
