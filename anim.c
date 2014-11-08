#include <windows.h>
#include <gif_lib.h>
#include <time.h>
#include "dib.h"
#include "anim.h"

static long DELAY_TIME_UNIT_MS = 1000L / 100L;

static void find_graphics_control_block(GraphicsControlBlock *gcb,
	SavedImage *image)
{
	int i;
	ExtensionBlock *eb;
	
	gcb->DelayTime = 0;
    gcb->TransparentColor = NO_TRANSPARENT_COLOR;
	for(i = 0, eb = image->ExtensionBlocks;
		i < image->ExtensionBlockCount;
		i++, eb++)
	{
		if(eb->Function == GRAPHICS_EXT_FUNC_CODE) {
			DGifExtensionToGCB(eb->ByteCount, eb->Bytes, gcb);
			break;
		}
	}
}

static void frame_from_gif(gifframe_t *frame, SavedImage *image,
	ColorMapObject *colormap)
{
	GraphicsControlBlock gcb;
	dib_t *dib;
	unsigned int oi, od, gap;
	GifColorType *color;
	unsigned char *pd;
	
	find_graphics_control_block(&gcb, image);
	frame->delayms = gcb.DelayTime == 0 ?
		DELAY_TIME_UNIT_MS :
		DELAY_TIME_UNIT_MS * gcb.DelayTime;
	dib = &(frame->image);
	dib_init(dib, image->ImageDesc.Width, image->ImageDesc.Height);
	if(colormap == NULL) {
		/* gif does not have global colormap, use the one of the image data */
		colormap = image->ImageDesc.ColorMap;
	}
	if(gcb.TransparentColor != NO_TRANSPARENT_COLOR) {
		color = &(colormap->Colors[gcb.TransparentColor]);
		color->Red = 0x00;
		color->Green = 0x00;
		color->Blue = 0x00;
	}
	gap = dib->stride-(dib->width*3);
	for(oi = 0, od = dib->stride*dib->height-gap-3;
		oi < dib->height*dib->width;
		oi++, od-=3)
	{
		/* 
			reading RasterBits incremental and
			writing to DIB decremental because DIBs are weird
		*/
		color = &(colormap->Colors[image->RasterBits[oi]]);
		pd = &(dib->data[od]);
		*(pd+0) = color->Blue;
		*(pd+1) = color->Green;
		*(pd+2) = color->Red;
		if(gap != 0 && od % dib->stride == 0) {
			/* ignore fill bytes */
			od -= gap;
		}
	}
}

void gifanim_init(gifanim_t *ga, GifFileType *gif, HDC hdc,
	unsigned int x, unsigned int y)
{
	int i;

	ga->hdc = hdc;
	ga->current = 0;
	ga->x = x;
	ga->y = y;
	ga->count = gif->ImageCount;
	ga->frames = malloc(sizeof(gifframe_t) * ga->count);
	for(i = 0; i < ga->count; i++) {
		frame_from_gif(
			&(ga->frames[i]),
			&(gif->SavedImages[i]),
			gif->SColorMap
		);
	}
}

void gifanim_drawnext(gifanim_t *ga)
{
	BITMAPINFO bi;
	gifframe_t *frame;
	struct timespec req;
	
	frame = &(ga->frames[ga->current]);
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = frame->image.width;
	bi.bmiHeader.biHeight = frame->image.height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	SetDIBitsToDevice(ga->hdc,
		ga->x, ga->y,
		frame->image.width, frame->image.height,
		0, 0,
		0, frame->image.height,
		frame->image.data, &bi,
		DIB_RGB_COLORS
	);
	req.tv_nsec = ((long)(frame->delayms)%1000L) * 1000000L;
	req.tv_sec = (long)(frame->delayms)/1000L;
	nanosleep(&req, NULL);
	ga->current = (ga->current + 1) % ga->count;
}

void gifanim_free(gifanim_t *ga)
{
	int i;
	
	for(i = 0; i < ga->count; i++) {
		dib_free(&(ga->frames[i].image));
	}
	free(ga->frames);
}
