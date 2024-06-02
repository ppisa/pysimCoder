/*******************************************************************
  Implementation of circular queue that writes over old elements when full.

  (C) Copyright 2024 by David Storek
      e-mail:   dstorek@fel.cvut.cz
      license:  any combination of GPL, LGPL, MPL or BSD licenses

 *******************************************************************/

#ifndef MZAPO_CIRCULAR_QUEUE
#define MZAPO_CIRCULAR_QUEUE

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
  #endif

  struct mzapo_cq {
    void * data;
    int max_elements;
    int elem_bytes;
    int index;
    int elements_nr;
  };

  void cq_init(struct mzapo_cq * queue, int size, int elem_bytes);

  void cq_push(struct mzapo_cq * queue, void * elem);

  void cq_pop(struct mzapo_cq * queue, void * dst);

  int cq_is_empty(struct mzapo_cq * queue);

  void cq_deinit(struct mzapo_cq * queue);

  #ifdef __cplusplus
} /* extern "C"*/
#endif

#endif /*MZAPO_CIRCULAR_QUEUE*/