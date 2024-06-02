/*
COPYRIGHT (C) 2024  David Storek (storedav@fel.cvut.cz)

Description: C-code implementing circular queue that overrider old elements when full.

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

#include "mzapo_circular_queue.h"

#include <stdlib.h>
#include <string.h>

void cq_init(struct mzapo_cq * queue, int max_elements, int elem_bytes) {
  queue -> data = malloc(max_elements * elem_bytes);
  queue -> max_elements = max_elements;
  queue -> elem_bytes = elem_bytes;
  queue -> index = 0;
  queue -> elements_nr = 0;
}

void cq_deinit(struct mzapo_cq * queue) {
  free(queue -> data);
  memset(queue, 0, sizeof(struct mzapo_cq));
}

void cq_push(struct mzapo_cq * queue, void * elem) {
  int pos_of_element = (queue -> index + queue -> elements_nr) % queue -> max_elements;
  memcpy(queue -> data + (pos_of_element * queue -> elem_bytes), elem, queue -> elem_bytes);
  queue -> elements_nr = queue -> elements_nr >= queue -> max_elements ? queue -> max_elements : queue -> elements_nr + 1;
}

void cq_pop(struct mzapo_cq * queue, void * dst) {
  memcpy(dst, queue -> data + (queue -> index * queue -> elem_bytes), queue -> elem_bytes);
  queue -> index = (queue -> index + 1) % queue -> max_elements;
  queue -> elements_nr--;
}

int cq_is_empty(struct mzapo_cq * queue) {
  return queue -> elements_nr == 0;
}
