typedef struct dib_t {
	unsigned char *data;
	unsigned int stride;
	unsigned int width, height;
} dib_t;

void dib_init(dib_t *dib, unsigned int width, unsigned int height);
void dib_free(dib_t *dib);
