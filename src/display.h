/*
 * display.h
 *
 *  Created on: Apr 10, 2014
 *      Author: hp
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

void init_sdl ();
void wait_for_keypress ();
void display_buffer (int width, int height, char *buffer);

#define NCOLORS 256
SDL_Color colors[NCOLORS];

SDL_Surface *screen;

#endif /* DISPLAY_H_ */
