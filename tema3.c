#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <math.h>

#include "mpi.h"
#include "pic.c"

const float smooth[9] =
    {
        1.0 / 9, 1.0 / 9, 1.0 / 9,
        1.0 / 9, 1.0 / 9, 1.0 / 9,
        1.0 / 9, 1.0 / 9, 1.0 / 9
    };
const float blur[9] =
    {
        1.0 / 16, 1.0 / 8, 1.0 / 16,
        1.0 / 8, 1.0 / 4, 1.0 / 8,
        1.0 / 16, 1.0 / 8, 1.0 / 16
    };
const float sharpen[9] =
    {
        0.0, -2.0 / 3, 0.0,
        -2.0 / 3, 11.0 / 3, -2.0 / 3,
        0.0, -2.0 / 3, 0.0
    };
const float mean[9] =
    {
        -1.0, -1.0, -1.0,
        -1.0, -1.0, -1.0,
        -1.0, -1.0, -1.0
    };
const float emboss[9] =
    {
        0, 1, 0,
        0, 0, 0,
        0, -1, 0
    };

/* pnm_get_token: read token from PNM file in stream fp into "char tok[len]" */
char *pnm_get_token(FILE *fp, char *tok, int len) {
    char *t;
    int c;

    for (;;) {          /* skip over whitespace and comments */
    while (isspace(c = getc(fp)));
    if (c!='#') break;
    do c = getc(fp); while (c!='\n' && c!=EOF); /* gobble comment */
    if (c==EOF) break;
    }

    t = tok;
    if (c!=EOF) {
    do {
        *t++ = c;
        c = getc(fp);
    } while (!isspace(c) && c!='#' && c!=EOF && t-tok<len-1);
    if (c=='#') ungetc(c, fp);  /* put '#' back for next time */
    }
    *t = 0;
    return tok;
}

/* pnm_get_size: get size in pixels of PNM picture file */
int pnm_get_size(char *file, int *width, int *height) {
    char tok[20];
    FILE *fp;

    if ((fp = fopen(file, "r")) == NULL) {
    fprintf(stderr, "can't read PNM file %s\n", file);
    return 0;
    }
    pnm_get_token(fp, tok, sizeof tok);
    if (strcmp(tok, "P5") && strcmp(tok, "P6")) {
    fprintf(stderr,
        "%s is not a valid binary PGM or PPM file, bad magic#\n", file);
    fclose(fp);
    return 0;
    }
    if (sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", width) != 1 ||
    sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", height) != 1) {
        fprintf(stderr, "%s is not a valid PNM file: bad size\n", file);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

/*
 * pnm_read: read a PNM file into memory.
 * If opic!=0, then picture is read into opic->pix (after checking that
 * size is sufficient), else a new Pic is allocated.
 * Returns Pic pointer on success, 0 on failure.
 */
Pic *pnm_read(char *file, Pic *opic) {
    FILE *fp;
    char tok[20];
    int width, height, bpp, pvmax;
    Pic *p;

    /* open PNM file */
    if ((fp = fopen(file, "r")) == NULL) {
	fprintf(stderr, "can't read PNM file %s\n", file);
	return 0;
    }

    /* read PNM header */
    pnm_get_token(fp, tok, sizeof tok);
    if (strcmp(tok, "P5") && strcmp(tok, "P6")) {
	fprintf(stderr,
	    "%s is not a valid binary PGM or PPM file, bad magic#\n", file);
	fclose(fp);
	return 0;
    }
    bpp = strcmp(tok, "P6") ? 1 : 3;
	/* set bytes per pixel
	 * "P5" is header for PGM (grayscale), "P6" is header for PPM (color)*/
    if (sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", &width) != 1 ||
        sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", &height) != 1 ||
        sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", &pvmax) != 1) {
        fprintf(stderr, "%s is not a valid PNM file: bad size\n", file);
        fclose(fp);
        return 0;
    }

    if (pvmax!=255) {
	   fprintf(stderr, "%s does not have 8-bit components: pvmax=%d\n",
	       file, pvmax);
	   fclose(fp);
	   return 0;
    }

    p = pic_alloc(width, height, bpp, opic);
    printf("reading PNM file %s: %dx%d pixels, %d byte per pixel\n",
	file, p->width, p->height, p->bpp);

    if (fread(p->pix, p->width*p->bpp, p->height, fp) != p->height) {  /* read pixels */
    	fprintf(stderr, "premature EOF on file %s\n", file);
    	free(p);
    	fclose(fp);
    	return 0;
    }
    fclose(fp);
    return p;
}

int pnm_write(char *file, Pic *pic)
{
    FILE *pnm;

    if (pic->bpp!=1 && pic->bpp!=3) {
	fprintf(stderr, "pnm_write: can't write %d byte per pixel Pic\n",
	    pic->bpp);
	return 0;
    }

    /* Open the file for output */
    pnm = fopen(file, "w");
    if( !pnm )
	return 0;

    /* Always write a binary PNM file */
    fprintf(pnm, "P%c %d %d 255\n", pic->bpp==1 ? '5' : '6', pic->width, pic->height);
    /* "P5" is header for PGM (grayscale), "P6" is header for PPM (color) */
    
    if (fwrite(pic->pix, pic->width*pic->bpp, pic->height, pnm) != pic->height) {
	fprintf(stderr, "pnm_write: error writing %s\n", file);
	fclose(pnm);
	return 0;
    }

    fclose(pnm);
    return 1;
}

int main(int argc, char *argv[])
{
    int numtasks, rank;
    Pic *picture_read, *picture_write;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    const int items = 3;
    int block_len[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_CHAR, MPI_CHAR, MPI_CHAR};
    MPI_Datatype mpi_Pixel_color;
    
    MPI_Aint offsets[3];
    offsets[0] = offsetof(Pixel1_rgb, r);
    offsets[1] = offsetof(Pixel1_rgb, g);
    offsets[2] = offsetof(Pixel1_rgb, b);

    MPI_Type_create_struct(items, block_len, offsets, types, &mpi_Pixel_color);
    MPI_Type_commit(&mpi_Pixel_color);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    int width, height;
    if (rank == 0) {
        picture_read = pnm_read(argv[1], NULL);
        
        width = picture_read->width;
        height = picture_read->height;

        picture_write = pic_alloc(
            picture_read->width,
            picture_read->height,
            picture_read->bpp,
            NULL);
        
        for (int i = 1; i < numtasks; ++i) {
            MPI_Send(&picture_read->bpp, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&picture_read->width, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&picture_read->height, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        int bpp;
        MPI_Recv(&bpp, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&width, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        picture_write = pic_alloc(width, height, bpp, NULL);
    }
    
    int factor = (int)ceil((1.0 * height * width) / numtasks);
    int start = factor * rank;
    int end = (int)fmin(factor * (rank + 1), height * width);

    if (rank == 0) {
        for (int i = 1; i < numtasks; ++i) {
            if (picture_read->bpp == 3) {
                MPI_Send(
                    picture_read->pix,
                    picture_read->width * picture_read->height,
                    mpi_Pixel_color,
                    i, 0, MPI_COMM_WORLD);
            } else {
                MPI_Send(
                    picture_read->pix, 
                    picture_read->width * picture_read->height, 
                    MPI_CHAR, 
                    i, 0, MPI_COMM_WORLD);
            }
        }
        memcpy(picture_write->pix, picture_read->pix, (end - start + 1) * sizeof(Pixel1_rgb));
    }
    if (rank != 0) {
        if (picture_write->bpp == 3) {
            MPI_Recv(
                picture_write->pix, 
                picture_write->width * picture_write->height * sizeof(Pixel1_rgb), 
                mpi_Pixel_color, 0, 0, 
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Recv(
                picture_write->pix, 
                picture_write->width * picture_write->height * sizeof(char), 
                MPI_CHAR, 0, 0, 
                MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if (picture_write->bpp == 3) {
            MPI_Send(
                picture_write->pix + start,
                (end - start + 1) * sizeof(Pixel1_rgb),
                mpi_Pixel_color,
                0, 0, MPI_COMM_WORLD);
        } else {
            MPI_Send(
                picture_write->pix + start,
                (end - start + 1) * sizeof(char),
                MPI_CHAR,
                0, 0, MPI_COMM_WORLD);
        }
    } else {
        for (int i = 1; i < numtasks; ++i) {
            factor = (int)ceil((1.0 * width * height) / numtasks);
            start = factor * i;
            end = (int)fmin(factor * (i + 1), width * height);
            if (picture_write->bpp == 3) {
                MPI_Recv(
                    picture_write->pix + start,
                    (end - start + 1) * sizeof(Pixel1_rgb),
                    mpi_Pixel_color,
                    i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(
                    picture_write->pix + start, 
                    (end - start + 1) * sizeof(char), 
                    MPI_CHAR, 
                    i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }
    pnm_write(argv[2], picture_write);
    MPI_Finalize();
    return 0;
}
