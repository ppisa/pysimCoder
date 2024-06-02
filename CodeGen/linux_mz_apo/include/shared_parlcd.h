/*******************************************************************
  Simple interface to parallel LCD screen

  parlcd.h      - Connected LCD access
  (C) Copyright 2024 by David Storek
      e-mail:   storedav@fel.cvut.cz
      license:  any combination of GPL, LGPL, MPL or BSD licenses

 *******************************************************************/

#ifndef PARLCD_H
#define PARLCD_H

#include <stdint.h>
#include <mzapo_regs.h>
#include <phys_address_access.h>
#include <mzapo_parlcd.h>


#define LCD_WIDTH                  480
#define LCD_HEIGHT                 320

/*Expected to be called from block's init()*/
int lcd_register_user(void(*draw_function)(void* ctx), void* ctx);

/*Expected to be called from block's deinit()*/
void* lcd_unregister_user(int token);

/*This function has to be called from inside of draw_function*/
void draw_funct_lcd_set_pixels(uint16_t* data, int x, int y, int width, int height);

/*This function has to be called from inside of draw_function*/
void draw_funct_lcd_set_pixel(uint16_t color, int x, int y);

#endif  /*PARLCD_H*/
