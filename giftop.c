#include <windows.h>
#include <time.h>
#include <gif_lib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

typedef struct {
	unsigned char *data;
	unsigned stride;
	unsigned width, height;
} Dib;

typedef struct {
	Dib image;
	unsigned delayms;
} GifFrame;

typedef struct {
	HDC hdc;
	unsigned x, y;
	unsigned current, count;
	GifFrame *frames;
} GifAnim;

typedef struct Flags {
	char *gif;
	unsigned x, y;
} Flags;

static long DELAY_TIME_UNIT_MS = 1000L / 100L;

static void dib_init(Dib *dib, unsigned width, unsigned height) {
	dib->width = width;
	dib->height = height;
	dib->stride = (dib->width * 3 + 3) & ~3;
	dib->data = malloc(dib->stride * dib->height);
}

static void dib_free(Dib *dib) { free(dib->data); }

static void find_graphics_control_block(GraphicsControlBlock *gcb,
										SavedImage *image) {
	gcb->DelayTime = 0;
	gcb->TransparentColor = NO_TRANSPARENT_COLOR;
	ExtensionBlock *end = image->ExtensionBlocks + image->ExtensionBlockCount;
	for (ExtensionBlock *eb = image->ExtensionBlocks; eb != end; eb++) {
		if (eb->Function == GRAPHICS_EXT_FUNC_CODE) {
			DGifExtensionToGCB(eb->ByteCount, eb->Bytes, gcb);
			break;
		}
	}
}

static void frame_from_gif(GifFrame *frame, SavedImage *image,
						   ColorMapObject *colormap) {
	GraphicsControlBlock gcb;
	find_graphics_control_block(&gcb, image);
	frame->delayms = gcb.DelayTime == 0 ? DELAY_TIME_UNIT_MS
										: DELAY_TIME_UNIT_MS * gcb.DelayTime;
	if (image->ImageDesc.ColorMap) {
		colormap = image->ImageDesc.ColorMap;
	}
	if (gcb.TransparentColor != NO_TRANSPARENT_COLOR) {
		GifColorType *color = &(colormap->Colors[gcb.TransparentColor]);
		color->Red = 0x00;
		color->Green = 0x00;
		color->Blue = 0x00;
	}
	Dib *dib = &(frame->image);
	dib_init(dib, image->ImageDesc.Width, image->ImageDesc.Height);
	int oi = 0;
	for (int y = dib->height - 1; y >= 0; y--) {
		for (int x = 0; x < dib->width; x++, oi++) {
			GifColorType *color = &(colormap->Colors[image->RasterBits[oi]]);
			unsigned char *pd = &(dib->data[y * dib->stride + x * 3]);
			*(pd + 0) = color->Blue;
			*(pd + 1) = color->Green;
			*(pd + 2) = color->Red;
		}
	}
}

static void gifanim_init(GifAnim *ga, GifFileType *gif, HDC hdc, unsigned x,
						 unsigned y) {
	ga->hdc = hdc;
	ga->current = 0;
	ga->x = x;
	ga->y = y;
	ga->count = gif->ImageCount;
	ga->frames = malloc(sizeof(GifFrame) * ga->count);
	for (unsigned i = 0; i < ga->count; i++) {
		frame_from_gif(&(ga->frames[i]), &(gif->SavedImages[i]),
					   gif->SColorMap);
	}
}

static void gifanim_drawnext(GifAnim *ga) {
	GifFrame *frame = &(ga->frames[ga->current]);
	BITMAPINFO bi;
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = frame->image.width;
	bi.bmiHeader.biHeight = frame->image.height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	SetDIBitsToDevice(ga->hdc, ga->x, ga->y, frame->image.width,
					  frame->image.height, 0, 0, 0, frame->image.height,
					  frame->image.data, &bi, DIB_RGB_COLORS);
	struct timespec req;
	req.tv_nsec = ((long)(frame->delayms) % 1000L) * 1000000L;
	req.tv_sec = (long)(frame->delayms) / 1000L;
	nanosleep(&req, NULL);
	ga->current = (ga->current + 1) % ga->count;
}

static void gifanim_free(GifAnim *ga) {
	for (unsigned i = 0; i < ga->count; i++) {
		dib_free(&(ga->frames[i].image));
	}
	free(ga->frames);
}

static int find_desktop(HWND hwnd, LPARAM lParam) {
	if (FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL)) {
		HWND *phwnd = (HWND *)lParam;
		*phwnd = FindWindowEx(NULL, hwnd, "WorkerW", NULL);
	}
	return 1;
}

static HWND get_desktop(void) {
	HWND hwnd = 0;
	EnumWindows(find_desktop, (LPARAM)&hwnd);
	return hwnd;
}

static int running = 1;

static void sig_int(int signo) { running = 0; }

static void parse_args(Flags *flags, int argc, char **argv) {
	int c;
	while ((c = getopt(argc, argv, "g:x:y:")) != -1) {
		switch (c) {
		case 'g':
			flags->gif = optarg;
			break;
		case 'x':
			flags->x = strtoul(optarg, NULL, 10);
			break;
		case 'y':
			flags->y = strtoul(optarg, NULL, 10);
			break;
		default:
			break;
		}
	}
}

int main(int argc, char **argv) {
	int err;
	Flags flags = {0};
	parse_args(&flags, argc, argv);
	if (!flags.gif) {
		fprintf(stderr,
				"usage: giftop -g gif [-x x-position] [-y y-position]\n");
		return 0;
	}
	GifFileType *gif = DGifOpenFileName(flags.gif, &err);
	if (!gif) {
		fprintf(stderr, "failed to open gif");
		return 1;
	}
	if (DGifSlurp(gif) != GIF_OK) {
		fprintf(stderr, "failed to read gif");
		DGifCloseFile(gif, &err);
		return 1;
	}
	HWND desk = get_desktop();
	HDC hdc = GetDC(desk);
	GifAnim ga;
	gifanim_init(&ga, gif, hdc, flags.x, flags.y);
	DGifCloseFile(gif, &err);
	signal(SIGINT, sig_int);
	while (running) {
		gifanim_drawnext(&ga);
	}
	gifanim_free(&ga);
	ReleaseDC(desk, hdc);
	InvalidateRect(desk, NULL, 1);
	UpdateWindow(desk);
	return 0;
}