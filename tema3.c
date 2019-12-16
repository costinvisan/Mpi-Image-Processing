#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

#include "mpi.h"
#include "pic.c"

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
int pnm_get_size(char *file, int *nx, int *ny) {
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
    if (sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", nx) != 1 ||
    sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", ny) != 1) {
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
    int nx, ny, bpp, pvmax;
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
    if (sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", &nx) != 1 ||
        sscanf(pnm_get_token(fp, tok, sizeof tok), "%d", &ny) != 1 ||
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

    p = pic_alloc(nx, ny, bpp, opic);
    printf("reading PNM file %s: %dx%d pixels, %d byte per pixel\n",
	file, p->nx, p->ny, p->bpp);

    if (fread(p->pix, p->nx*p->bpp, p->ny, fp) != p->ny) {  /* read pixels */
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
    fprintf(pnm, "P%c %d %d 255\n", pic->bpp==1 ? '5' : '6', pic->nx, pic->ny);
    /* "P5" is header for PGM (grayscale), "P6" is header for PPM (color) */
    
    if (fwrite(pic->pix, pic->nx*pic->bpp, pic->ny, pnm) != pic->ny) {
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
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    Pic *pic = (Pic *)malloc(sizeof(Pic));
    Pic *p = pnm_read("Colectie Poze Intrare/Colectie Poze/PNM/baby-yoda.pnm", NULL);
    pnm_write("lol.pnm", p);
    MPI_Finalize();
    return 0;
}
 