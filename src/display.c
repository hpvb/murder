#include <SDL/SDL.h>

#include "types.h"
#include "display.h"

void init_sdl() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(640, 480, 16, SDL_SWSURFACE);
	if (screen == NULL) {
		fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
		exit(1);
	}
}

void wait_for_keypress() {
	SDL_Event e;
	int done = 0;

	while (!done) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_KEYDOWN:
				done = 1;
				break;
			case SDL_QUIT:
				exit(0);
				break;
			default:
				break;
			}
		}
		SDL_Delay(100);
	}
}

void display_buffer(int width, int height, char *buffer) {
	SDL_Surface *surface;
	int x, y;
	char *inp, *outp;

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width * 2, height * 2, 8, 0,
			0, 0, 0);

	if (SDL_SetPalette(surface, SDL_LOGPAL, colors, 0, NCOLORS) < 0) {
		fprintf(stderr, "Couldn't set palette\n");
	}

	SDL_LockSurface(surface);

	inp = buffer;
	for (y = 0; y < height; ++y) {
		outp = surface->pixels + (((y * 2) * width) * 2);

		for (x = 0; x < width; ++x) {
			memset(outp, *inp, 2);
			memset(outp + (width * 2), *inp, 2);

			inp++;
			outp += 2;
		}
	}

	SDL_UnlockSurface(surface);

	if (SDL_BlitSurface(surface, NULL, screen, NULL) < 0) {
		fprintf(stderr, "BlitSurface error: %s\n", SDL_GetError());
	}

	SDL_Flip(screen);
	wait_for_keypress();

	SDL_FreeSurface(surface);
}
