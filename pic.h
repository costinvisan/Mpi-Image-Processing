#ifndef PIC_H
#define PIC_H

#include <stdio.h>

#ifndef ALLOC
#define ALLOC(ptr, type, n) { \
    ptr = (type *)malloc((n)*sizeof(type)); \
    if (!ptr) { \
	fprintf(stderr, "pic_alloc: Can't allocate %d bytes of memory, aborting\n", n); \
	exit(1); \
    } \
}
#endif

typedef struct {char r, g, b;}	Pixel1_rgb;


typedef struct {		/* PICTURE */
    int width, height;			/* width & height, in pixels */
    int bpp;			/* bytes per pixel = 1, 3, or 4 */
    void *pix;		/* array of pixels */
				/* data is in row-major order,
				    i.e. it has the same memory layout as:
				    if 1 byte per pixel,  then array[ny][nx]
				    if 3 bytes per pixel, then array[ny][nx][3]
				    if 4 bytes per pixel, then array[ny][nx][4] */
} Pic;


/*
 * use the following macro to access the red, green, and blue channels of
 * a given pixel, for an image stored in a Pic structure.
 */
#define PIC_PIXEL(pic, x, y, chan) \
    (pic)->pix[((y)*(pic)->nx+(x))*(pic)->bpp+(chan)]
    /* returns channel chan of pixel (x,y) of picture pic */
    /* usually chan=0 for red, 1 for green, 2 for blue */
    
extern Pic *pic_alloc(int nx, int ny, int bytes_per_pixel, Pic *opic);
extern void pic_free(Pic *p);


#endif
