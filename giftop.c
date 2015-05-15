#include <windows.h>
#include <gif_lib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "dib.h"
#include "anim.h"

static HWND get_desktop(void)
{
	HWND hwnd;
	hwnd = FindWindow("Progman", NULL);
	hwnd = FindWindowEx(hwnd, NULL, "SHELLDLL_DefView", NULL);
	return FindWindowEx(hwnd, NULL, "SysListView32", NULL);
}

static int running;

static void sig_int(int signo)
{
	running = 0;
}

typedef struct Flags {
	char *gif;
	int x, y;
} Flags;

static void parse_args(Flags *flags, int argc, char **argv)
{
	int c;

	while((c = getopt(argc, argv, "g:x:y:")) != -1) {
		switch(c) {
		case 'g':
			flags->gif = optarg;
			break;
		case 'x':
			flags->x = atoi(optarg);
			break;
		case 'y':
			flags->y = atoi(optarg);
			break;
		default:
			break;
		}
	}
}

int main(int argc, char **argv)
{
	Flags flags = {0};
	GifFileType *gif;
	GifAnim ga;
	int err;
	HWND desk;
	HDC hdc;

	parse_args(&flags, argc, argv);
	if(flags.gif == NULL) {
		fprintf(
			stderr,
			"usage: giftop -g gif [-x x-position] [-y y-position]\n"
		);
		return 0;
	}
	gif = DGifOpenFileName(flags.gif, &err);
	if(gif == NULL) {
		fprintf(stderr, "failed to open gif");
		return 1;
	}
	if(DGifSlurp(gif) != GIF_OK) {
		fprintf(stderr, "failed to read gif");
		DGifCloseFile(gif, &err);
		return 1;
	}
	desk = get_desktop();
	hdc = GetDC(desk);
	gifanim_init(&ga, gif, hdc, flags.x, flags.y);
	DGifCloseFile(gif, &err);
	signal(SIGINT, sig_int);
	running = 1;
	while(running) {
		gifanim_drawnext(&ga);
	}
	gifanim_free(&ga);
	ReleaseDC(desk, hdc);
	InvalidateRect(desk, NULL, 1);
	UpdateWindow(desk);
	return 0;
}
