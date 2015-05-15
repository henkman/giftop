typedef struct GifFrame {
	Dib image;
	unsigned int delayms;
} GifFrame;

typedef struct GifAnim {
	HDC hdc;
	unsigned int x, y;
	unsigned int current, count;
	GifFrame *frames;
} GifAnim;

void gifanim_init(GifAnim *ga, GifFileType *gif, HDC hdc,
	unsigned int x, unsigned int y);
void gifanim_drawnext(GifAnim *ga);
void gifanim_free(GifAnim *ga);
