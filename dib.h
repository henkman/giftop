typedef struct Dib {
	unsigned char *data;
	unsigned int stride;
	unsigned int width, height;
} Dib;

void dib_init(Dib *dib, unsigned int width, unsigned int height);
void dib_free(Dib *dib);
