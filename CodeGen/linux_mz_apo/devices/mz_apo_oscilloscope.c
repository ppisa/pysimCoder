/*
COPYRIGHT (C) 2024  David Storek (storedav@fel.cvut.cz)

Description: C-code for printing osciloscope-like behavior onto LCD screen.

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

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>

/* supporting files for MZ_APO board */
#include "shared_parlcd.h"
#include "mzapo_circular_queue.h"

/* parameters index definition */
#define REALPAR_IDX_X_MIN_VAL 0
#define REALPAR_IDX_X_MAX_VAL 1
#define REALPAR_IDX_Y_MIN_VAL 2
#define REALPAR_IDX_Y_MAX_VAL 3
#define REALPAR_IDX_STRENGTH_MIN_VAL 4
#define REALPAR_IDX_STRENGTH_MAX_VAL 5
#define REALPAR_IDX_DECAY_RATE 6

#define INTPAR_IDX_LCD_X 0
#define INTPAR_IDX_LCD_Y 1
#define INTPAR_IDX_LCD_WIDTH 2
#define INTPAR_IDX_LCD_HEIGHT 3

#define INPUT_IDX_X_VAL 0
#define INPUT_IDX_Y_VAL 1
#define INPUT_IDX_STRENGTH_VAL 2

#define POINTS_BUFFER_SIZE 30

#define MASK_RED 0xf800
#define MASK_GREEN 0x07e0
#define MASK_BLUE 0x001f
#define COLOR_BACKGROUND 0x0000
#define COLOR_BORDER 0xffff
#define COLOR_LINE 0x07e0

struct Params {
  double x_min;
  double x_max;
  double y_min;
  double y_max;
  double strength_min;
  double strength_max;
  double decay_rate;

  int lcd_x;
  int lcd_y;
  int lcd_width;
  int lcd_height;
};

struct Input {
  double x;
  double y;
  double strength;
};

struct Point {
  int16_t x;
  int16_t y;
  float w;
};

struct draw_ctx {
  pthread_mutex_t draw_ctx_mutex;
  struct mzapo_cq points;
  struct Point last_drawn_point;
  struct Point * points_to_draw;

  float * values;
  uint16_t * color_pallete;
  int pallete_size;

  float decay_rate;

  uint16_t frame_x;
  uint16_t frame_y;
  uint16_t frame_width;
  uint16_t frame_height;
};

struct State {
  int lcd_user_id;

  struct draw_ctx * draw_context;
};

struct Params read_params(python_block * block) {
  struct Params pars;

  double * realPar = block -> realPar;
  pars.x_min = realPar[REALPAR_IDX_X_MIN_VAL];
  pars.x_max = realPar[REALPAR_IDX_X_MAX_VAL];
  pars.y_min = realPar[REALPAR_IDX_Y_MIN_VAL];
  pars.y_max = realPar[REALPAR_IDX_Y_MAX_VAL];
  pars.strength_min = realPar[REALPAR_IDX_STRENGTH_MIN_VAL];
  pars.strength_max = realPar[REALPAR_IDX_STRENGTH_MAX_VAL];
  pars.decay_rate = realPar[REALPAR_IDX_DECAY_RATE];

  int * intPar = block -> intPar;
  pars.lcd_x = intPar[INTPAR_IDX_LCD_X];
  pars.lcd_y = intPar[INTPAR_IDX_LCD_Y];
  pars.lcd_width = intPar[INTPAR_IDX_LCD_WIDTH];
  pars.lcd_height = intPar[INTPAR_IDX_LCD_HEIGHT];

  if (pars.lcd_width < 0 || pars.lcd_height < 0) {
    fprintf(stderr, "Display size must be positive!.\n");
    exit(1);
  }

  //  clip viewport to bounds of screen
  // first find slope of boundary values
  double x_slope = pars.lcd_width == 0 ? 0 : ((pars.x_max - pars.x_min) / pars.lcd_width);
  double x_q = pars.x_min - (x_slope * pars.lcd_x);
  double y_slope = pars.lcd_height == 0 ? 0 : ((pars.y_max - pars.y_min) / pars.lcd_height);
  double y_q = pars.y_min - (y_slope * pars.lcd_y);

  // clip left
  if (pars.lcd_x < 0) {
    pars.x_min = x_slope * 0 + x_q;
    pars.lcd_width -= 0 - pars.lcd_x;
    pars.lcd_width = pars.lcd_width < 0 ? 0 : pars.lcd_width;
    pars.lcd_x = 0;
  }
  // clip right
  if (pars.lcd_x + pars.lcd_width >= LCD_WIDTH) {
    pars.x_max = x_slope * LCD_WIDTH + x_q;
    pars.lcd_width -= pars.lcd_x + pars.lcd_width - LCD_WIDTH;
    pars.lcd_width = pars.lcd_width < 0 ? 0 : pars.lcd_width;
  }
  // clip top
  if (pars.lcd_y < 0) {
    pars.y_min = y_slope * 0 + y_q;
    pars.lcd_height -= 0 - pars.lcd_y;
    pars.lcd_height = pars.lcd_height < 0 ? 0 : pars.lcd_height;
    pars.lcd_y = 0;
  }
  // clip bottom
  if (pars.lcd_y + pars.lcd_height >= LCD_HEIGHT) {
    pars.y_max = y_slope * LCD_HEIGHT + y_q;
    pars.lcd_height -= pars.lcd_y + pars.lcd_height - LCD_HEIGHT;
    pars.lcd_height = pars.lcd_height < 0 ? 0 : pars.lcd_height;
  }

  return pars;
}

struct Input read_and_clip_input(python_block * block, struct Params * pars) {
  struct Input input;

  input.x = * (double * ) block -> u[INPUT_IDX_X_VAL];
  input.y = * (double * ) block -> u[INPUT_IDX_Y_VAL];
  input.strength = * (double * ) block -> u[INPUT_IDX_STRENGTH_VAL];

  input.x = fmin(fmax(input.x, pars -> x_min), pars -> x_max);
  input.y = fmin(fmax(input.y, pars -> y_min), pars -> y_max);
  input.strength = fmin(fmax(input.strength, pars -> strength_min), pars -> strength_max);

  return input;
}

uint16_t mix_colors(uint16_t c1, uint16_t c2, float weight) {
  uint16_t c1_r = c1 & MASK_RED;
  uint16_t c1_g = c1 & MASK_GREEN;
  uint16_t c1_b = c1 & MASK_BLUE;

  uint16_t c2_r = c2 & MASK_RED;
  uint16_t c2_g = c2 & MASK_GREEN;
  uint16_t c2_b = c2 & MASK_BLUE;

  uint16_t c_r = (uint16_t) roundf(((float)(c1_r) * (1.0f - weight)) + ((float)(c2_r) * weight));
  uint16_t c_g = (uint16_t) roundf(((float)(c1_g) * (1.0f - weight)) + ((float)(c2_g) * weight));
  uint16_t c_b = (uint16_t) roundf(((float)(c1_b) * (1.0f - weight)) + ((float)(c2_b) * weight));

  return (c_r & MASK_RED) | (c_g & MASK_GREEN) | (c_b & MASK_BLUE);
}

// function for line drawing
// Taken from https://zingl.github.io/bresenham.html
void bresenham(struct draw_ctx * ctx, int x0, int y0, int x1, int y1, float strength) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2; /* error value e_xy */

  for (;;) {
    /* loop */
    // setPixel(x0,y0);
    if (x0 >= 0 && x0 < ctx -> frame_width && y0 >= 0 && y0 < ctx -> frame_height) {
      ctx -> values[y0 * ctx -> frame_width + x0] = fmaxf(ctx -> values[y0 * ctx -> frame_width + x0], strength);
    }
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    } /* e_xy+e_x > 0 */
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    } /* e_xy+e_y < 0 */
  }
}

// function used to draw the oscilloscope curve. Runs in the LCD's thread.
void draw_function(void * _ctx) {
  struct draw_ctx * ctx = (struct draw_ctx * ) _ctx;

  // decay existing values
  for (int idx = 0; idx < ctx -> frame_width * ctx -> frame_height; idx++) {
    ctx -> values[idx] *= ctx -> decay_rate;
  }

  // gather all new points from queue
  int new_points_nr = 0;
  struct Point last_drawn_point = ctx -> last_drawn_point;

  pthread_mutex_lock( & ctx -> draw_ctx_mutex);
  while (cq_is_empty( & ctx -> points) == 0) {
    cq_pop( & ctx -> points, & ctx -> points_to_draw[new_points_nr++]);
  }
  if (new_points_nr > 0) {
    ctx -> last_drawn_point = ctx -> points_to_draw[new_points_nr - 1];
  }
  pthread_mutex_unlock( & ctx -> draw_ctx_mutex);

  // draw lines between new points
  for (int id = 0; id < new_points_nr; id++) {
    bresenham(ctx, last_drawn_point.x, last_drawn_point.y, ctx -> points_to_draw[id].x, ctx -> points_to_draw[id].y, last_drawn_point.w);
    last_drawn_point = ctx -> points_to_draw[id];
  }

  // draw values to lcd
  for (int y = 0; y < ctx -> frame_height; y++) {
    for (int x = 0; x < ctx -> frame_width; x++) {
      int idx = y * ctx -> frame_width + x;
      uint16_t final_color = ctx -> color_pallete[(int) round(ctx -> values[idx] * (ctx -> pallete_size - 1))];
      draw_funct_lcd_set_pixel(final_color, x + ctx -> frame_x, y + ctx -> frame_y);
    }
  }

  // draw frame around region to lcd
  uint16_t border_color[LCD_WIDTH];
  memset( & border_color[0], COLOR_BORDER, sizeof(uint16_t) * LCD_WIDTH);
  draw_funct_lcd_set_pixels( & border_color[0], ctx -> frame_x, ctx -> frame_y, ctx -> frame_width, 1);
  draw_funct_lcd_set_pixels( & border_color[0], ctx -> frame_x, ctx -> frame_y + ctx -> frame_height - 1, ctx -> frame_width, 1);
  draw_funct_lcd_set_pixels( & border_color[0], ctx -> frame_x, ctx -> frame_y, 1, ctx -> frame_height);
  draw_funct_lcd_set_pixels( & border_color[0], ctx -> frame_x + ctx -> frame_width - 1, ctx -> frame_y, 1, ctx -> frame_height);
}

/*
=================== Block functions ===================
*/

/*  INITIALIZATION FUNCTION  */
static void init(python_block * block) {
  struct State * state = (struct State * ) malloc(sizeof(struct State));
  /* Save memory map structure to ptrPar */
  block -> ptrPar = state;

  // Read parameters
  struct Params pars = read_params(block);

  // setup rendering context object
  struct draw_ctx * dctx = (struct draw_ctx * ) malloc(sizeof(struct draw_ctx));
  state -> draw_context = dctx;

  // setup its mutex
  int ret = pthread_mutex_init( & dctx -> draw_ctx_mutex, NULL);
  if (ret != 0) {
    printf("Can't initialize mutex for accessing oscilloscope's points buffer. Errcode: %d\n", ret);
    exit(-1);
  }

  // setup its points queue
  cq_init( & dctx -> points, POINTS_BUFFER_SIZE, sizeof(struct Point));

  // set its init point to center
  dctx -> last_drawn_point.x = pars.lcd_width / 2;
  dctx -> last_drawn_point.y = pars.lcd_height / 2;
  dctx -> last_drawn_point.w = 1.0f;

  // create buffer for storage of currently being drawn points
  dctx -> points_to_draw = (struct Point * ) malloc(sizeof(struct Point) * POINTS_BUFFER_SIZE);

  // set its values buffer
  dctx -> values = malloc(sizeof(float) * pars.lcd_width * pars.lcd_height);
  for (int idx = 0; idx < pars.lcd_width * pars.lcd_height; idx++) {
    dctx -> values[idx] = 0.0f;
  }

  //create collor pallete
  dctx -> pallete_size = 64; // Max bit depth per color is 6 bits for blue -> 64 values
  dctx -> color_pallete = malloc(sizeof(uint16_t) * dctx -> pallete_size);
  for (int id = 0; id < dctx -> pallete_size; id++) {
    dctx -> color_pallete[id] = mix_colors(COLOR_BACKGROUND, COLOR_LINE, (float) id / (float)(dctx -> pallete_size - 1));
  }

  dctx -> decay_rate = 0.8f;

  // copy parts of Params to it too
  dctx -> frame_x = pars.lcd_x;
  dctx -> frame_y = pars.lcd_y;
  dctx -> frame_width = pars.lcd_width;
  dctx -> frame_height = pars.lcd_height;

  // register block as user of LCD
  state -> lcd_user_id = lcd_register_user( & draw_function, dctx);
}

/*  INPUT/OUTPUT  FUNCTION  */
static void inout(python_block * block) {
  struct State * state = block -> ptrPar;
  struct Params pars = read_params(block);
  struct Input input = read_and_clip_input(block, & pars);

  // calculate relative positions of point based on input values
  double input_x_proportional = (input.x - pars.x_min) / (pars.x_max - pars.x_min);
  double input_y_proportional = (input.y - pars.y_min) / (pars.y_max - pars.y_min);
  double input_w_proportional = (input.strength - pars.strength_min) / (pars.strength_max - pars.strength_min);

  // clamp strength to [0, 1]
  input_w_proportional = fmax(fmin(input_w_proportional, 1.0), 0.0);

  // push point to list of points to be drawn
  struct draw_ctx * dctx = state -> draw_context;
  pthread_mutex_lock( & dctx -> draw_ctx_mutex);
  struct Point p = {
    .x = (int16_t) round(input_x_proportional * pars.lcd_width),
    .y = (int16_t) round(input_y_proportional * pars.lcd_height),
    .w = (float) input_w_proportional
  };
  cq_push( & dctx -> points, & p);
  dctx -> decay_rate = pars.decay_rate;
  pthread_mutex_unlock( & dctx -> draw_ctx_mutex);
}

/*  TERMINATION FUNCTION  */
static void end(python_block * block) {
  struct State * state = (struct State * ) block -> ptrPar;

  // unregister block as user of LCD
  struct draw_ctx * dctx = (struct draw_ctx * ) lcd_unregister_user(state -> lcd_user_id);

  // delete its color pallete
  free(dctx -> color_pallete);

  // delete its values buffer
  free(dctx -> values);

  // delete buffer for currently being drawn points
  free(dctx -> points_to_draw);

  // destroy its point queue
  cq_deinit( & dctx -> points);

  // deinit its mutex
  pthread_mutex_destroy( & dctx -> draw_ctx_mutex);

  // Delete the state as a whole
  free(state);
  state = NULL;
  block -> ptrPar = NULL;
}

void mz_apo_oscilloscope(int flag, python_block * block) {
  if (flag == CG_OUT) {
    /* input / output */
    inout(block);
  } else if (flag == CG_END) {
    /* termination */
    end(block);
  } else if (flag == CG_INIT) {
    /* initialisation */
    init(block);
  }
}
