#include <stdio.h>
#include <stdlib.h>
#include "pic.h"

Pic *pic_alloc(int nx, int ny, int bytes, Pic *pic) {
	Pic *p;
	int size = nx * ny * bytes;
	ALLOC(p, Pic, 1);
	p->nx = nx;
	p->ny = ny;
	p->bpp = bytes;
	if (pic && pic->nx * pic->ny * pic->bpp >= p->nx * p->ny * p->bpp) {
		p->pix = pic->pix;
	} else {
		if (bytes == 3) {
			ALLOC(p->pix, Pixel1_rgb, size);
		} else {
			ALLOC(p->pix, Pixel1, size);
		}
	}
	return p;
}