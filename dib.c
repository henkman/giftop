#include <stdlib.h>
#include "dib.h"

void dib_init(Dib *dib, unsigned int width, unsigned int height)
{
	dib->width = width;
	dib->height = height;
	dib->stride = (dib->width * 3 + 3) &~ 3;
	dib->data = malloc(dib->stride * dib->height);
}

void dib_free(Dib *dib)
{
	free(dib->data);
}
