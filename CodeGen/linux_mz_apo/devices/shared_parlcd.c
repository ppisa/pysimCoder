/*
COPYRIGHT (C) 2024  David Storek (storedav@fel.cvut.cz)

Description: C-code for managing access to LCD screen for multiple blocks.

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

#include "shared_parlcd.h"

#include <pthread.h>
#include <time.h>

#define MAX_REGISTERED_PAINTERS 4

/*LCD parameters and initialization are shared in whole program*/
mem_address_map_t * parlcd_mem_base = NULL;
uint16_t * FRAMEBUFFER = NULL;
size_t FB_SIZE = 0;

void _lcd_init() {
  parlcd_mem_base = mem_address_map_create(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
  /* Check for errors */
  if (parlcd_mem_base == NULL) {
    fprintf(stderr, "Error when accessing physical address.\n");
    exit(1);
  }
  printf("addresses: %d, %d \n", parlcd_mem_base -> regs_base_phys, (intptr_t) parlcd_mem_base -> regs_base_virt);
  parlcd_init(parlcd_mem_base);

  FB_SIZE = sizeof(uint16_t) * LCD_WIDTH * LCD_HEIGHT;
  FRAMEBUFFER = (uint16_t * ) malloc(FB_SIZE);
  memset(FRAMEBUFFER, 0, FB_SIZE);
}

void _lcd_uninit() {
  free(FRAMEBUFFER);
  FRAMEBUFFER = NULL;
  FB_SIZE = 0;

  mem_address_unmap_and_free(parlcd_mem_base);
}

void _lcd_reset() {
  parlcd_write_cr(parlcd_mem_base, 2);
  parlcd_delay(100);
  parlcd_write_cr(parlcd_mem_base, 0);
  parlcd_delay(100);
}

void _lcd_draw() {
  parlcd_write_cmd(parlcd_mem_base, 0x2c);
  for (int pix_id = 0; pix_id < LCD_WIDTH * LCD_HEIGHT; pix_id++) {
    parlcd_write_data(parlcd_mem_base, FRAMEBUFFER[pix_id]);
  }
}

enum registered_painter_status {
  NOT_SET = 0, SET = 1,
};

struct registered_painter {
  enum registered_painter_status status;
  void( * paint_funct)(void * ctx);
  void * ctx;
};

struct registered_painter painters[MAX_REGISTERED_PAINTERS];
int registered_painters_nr;

pthread_t painter_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int RUN_PAINTER = 0;
struct timespec paint_delay = {
  .tv_sec = 0,
  .tv_nsec = 100 * 1000 * 1000
}; // 100ms per frame

void * _thread_worker(void * ) {
  _lcd_init();
  int run = 1;
  while (run != 0) {
    pthread_mutex_lock( & mutex);
    for (int index = 0; index < MAX_REGISTERED_PAINTERS; index++) {
      struct registered_painter * painter = & painters[index];
      if (painter -> status == SET && painter -> paint_funct != NULL) {
        painter -> paint_funct(painter -> ctx);
      }
    }
    run = RUN_PAINTER;
    pthread_mutex_unlock( & mutex);
    _lcd_draw();
    clock_nanosleep(CLOCK_MONOTONIC, 0, & paint_delay, NULL);
  }

  _lcd_uninit();

  return NULL;
}

int lcd_register_user(void( * paint_funct)(void * ctx), void * ctx) {
  pthread_mutex_lock( & mutex);
  if (registered_painters_nr == 0) {

    memset( & painters[0], 0, sizeof(painters));
    RUN_PAINTER = 1;

    int res = pthread_create( & painter_thread, NULL, & _thread_worker, NULL);
    if (res != 0) {
      RUN_PAINTER = 0;
      pthread_mutex_unlock( & mutex);
      fprintf(stderr, "Can't create painter thread.");
      exit(1);
    }

  } else if (registered_painters_nr >= MAX_REGISTERED_PAINTERS) {
    RUN_PAINTER = 0;
    pthread_mutex_unlock( & mutex);
    fprintf(stderr, "Too many registered users of LCD. Max is %d", MAX_REGISTERED_PAINTERS);
    exit(1);
  }
  painters[registered_painters_nr].status = SET;
  painters[registered_painters_nr].paint_funct = paint_funct;
  painters[registered_painters_nr].ctx = ctx;

  int new_user_id = registered_painters_nr++;
  pthread_mutex_unlock( & mutex);

  return new_user_id;
}

void * lcd_unregister_user(int lcd_user_id) {

  pthread_mutex_lock( & mutex);
  void * ctx = painters[lcd_user_id].ctx;
  painters[lcd_user_id].status = NOT_SET;
  painters[lcd_user_id].paint_funct = NULL;
  painters[lcd_user_id].ctx = NULL;

  registered_painters_nr--;
  int should_stop = registered_painters_nr == 0;
  if (should_stop) {
    RUN_PAINTER = 0;
  }
  pthread_mutex_unlock( & mutex);

  if (should_stop) {
    void * result;
    pthread_join(painter_thread, & result);
  }

  return ctx;
}

void draw_funct_lcd_set_pixels(uint16_t * data, int x, int y, int width, int height) {
  /*Is expected to be called from wths file's worker thread, so no synchronizations.*/
  for (int row = 0; row < height; row++) {
    int fb_idx = (LCD_WIDTH * (row + y)) + x;
    int data_idx = row * width;
    memcpy( & FRAMEBUFFER[fb_idx], & data[data_idx], sizeof(uint16_t) * width);
  }
}

void draw_funct_lcd_set_pixel(uint16_t color, int x, int y) {
  /*Is expected to be called from wths file's worker thread, so no synchronizations.*/
  FRAMEBUFFER[LCD_WIDTH * y + x] = color;
}
