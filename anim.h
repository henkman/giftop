#ifndef ANIM_H
#define ANIM_H

#include <windows.h>
#include <gif_lib.h>
#include "dib.h"

typedef struct gifframe_t {
	dib_t image;
	unsigned int delayms;
} gifframe_t;

typedef struct gifanim_t {
	HDC hdc;
	unsigned int x, y;
	unsigned int current, count;
	gifframe_t *frames;
} gifanim_t;

void gifanim_init(gifanim_t *ga, GifFileType *gif, HDC hdc,
	unsigned int x, unsigned int y);
void gifanim_drawnext(gifanim_t *ga);
void gifanim_free(gifanim_t *ga);

#endif