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

#include <pyblock.h>

#include <candev.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <candev.h>

static int fd;

static void init(python_block *block) {
  pysim_can_initialize();

  fd = pysim_can_open("dev/can1");

  pysim_can_register_msg(0x10);
}

static void inout(python_block *block) {
  uint8_t data[64];
  uint16_t len;

  pysim_can_read_msg(0x10, &len, data);
  printf("0x10: ");
  for (int i = 0; i < len; i++) {
    printf("%02x ", data[i]);
  }
  printf("\n");
}

static void end(python_block *block) {
}

void can_recv(int flag, python_block *block)
{
  if (flag==CG_OUT) {          /* get input */
    inout(block);
  }
  else if (flag==CG_END) {     /* termination */
    end(block);
  }
  else if (flag ==CG_INIT) {    /* initialisation */
    init(block);
  }
}
