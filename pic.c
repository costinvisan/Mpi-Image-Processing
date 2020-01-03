#include <stdio.h>
#include <stdlib.h>
#include "pic.h"

Pic *pic_alloc(int width, int height, int bytes, Pic *pic) {
	Pic *p;
	int size = width * height * bytes;
	ALLOC(p, Pic, 1);
	p->width = width;
	p->height = height;
	p->bpp = bytes;
	if (pic && pic->width * pic->height * pic->bpp >= p->width * p->height * p->bpp) {
		p->pix = pic->pix;
	} else {
		if (bytes == 3) {
			ALLOC(p->pix, Pixel1_rgb, size);
		} else {
			ALLOC(p->pix, char, size);
		}
	}
	return p;
}