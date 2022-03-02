#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include "stb_image.h"
#include "stb_image_write.h"

#include "dc_image.h"

//-------------------------------------------------------
// A few image helper functions
//-------------------------------------------------------

// Memory Allocation

byte **malloc2d(int rows, int cols) {
	int y;
	byte **ptr = (byte**)malloc(rows*sizeof(byte*));
	for (y=0; y<rows; y++)
		ptr[y] = (byte*)calloc(cols,sizeof(byte));
	return ptr;
}

byte ***malloc3d(int rows, int cols, int chan) {
	int y,x;
	byte ***ptr = (byte***)malloc(rows*sizeof(byte**));
	for (y=0; y<rows; y++) {
		ptr[y] = (byte**)malloc(cols*sizeof(byte*));
		for (x=0; x<cols; x++)
			ptr[y][x] = (byte*)calloc(chan,sizeof(byte));
	}
	return ptr;
}

void free2d(byte **data, int rows)
{
	int y;
	for (y=0; y<rows; y++)
		free(data[y]);
	free(data);
}

void free3d(byte ***data, int rows, int cols)
{
	int y,x;
	for (y=0; y<rows; y++) {
		for (x=0; x<cols; x++)
			free(data[y][x]);
		free(data[y]);
	}
	free(data);
}


// Loading / Saving Images

void SaveRgbPng(byte ***in, const char *fname, int rows, int cols)
{
	printf("SaveRgbaPng %s %d %d\n", fname, rows, cols);
	
	int y,x,c,i=0;
	byte *data = (byte*)malloc(cols*rows*3*sizeof(byte));
	for (y=0; y<rows; y++) {
		//printf("y %d rows %d cols %d\n", y, rows, cols);
		for (x=0; x<cols; x++) {
			for (c=0; c<3; c++) {
				data[i++] = in[y][x][c];
			}
		}
	}
	stbi_write_png(fname, cols, rows, 3, data, cols*3);
	free(data);
}

void SaveGrayPng(byte **in, const char *fname, int rows, int cols)
{
	printf("SaveGrayPng %s %d %d\n", fname, rows, cols);
	
	int y,x,c,i=0;
	byte *data = malloc(cols*rows*3*sizeof(byte));
	for (y=0; y<rows; y++) {
		for (x=0; x<cols; x++) {
			data[i++] = in[y][x];   // red
			data[i++] = in[y][x];   // green
			data[i++] = in[y][x];   // blue
//			data[i++] = 255;        // alpha
		}
	}
	stbi_write_png(fname, cols, rows, 3, data, cols*3);
	free(data);
}


byte ***LoadRgb(const char *fname, int *rows, int *cols, int *chan)
{
	printf("LoadRgba %s\n", fname);
	
	int y,x,c,i=0;	
	byte *data = stbi_load(fname, cols, rows, chan, 3);
	/*
	// Convert rgb to rgba
	if (*chan==3) {
		int N = *rows * *cols;
		printf("N %d\n", N);
		byte *rgb = data;
		data = malloc(N * 4 * sizeof(byte));
		for (i=0; i<N; i++) {
			//printf("i %d\n", i);
			data[4*i+0] = rgb[3*i+0];
			data[4*i+1] = rgb[3*i+1];
			data[4*i+2] = rgb[3*i+2];
			data[4*i+3] = 255;
		}
		free(rgb);
		printf("done convert\n");
	}
	*chan = 4;
	*/
	if (*chan != 3) {
		printf("error: expected 3 channels (red green blue)\n");
		exit(1);
	}
	
	i=0;
	byte ***img = malloc3d(*rows,*cols,*chan);
	for (y=0; y<*rows; y++)
		for (x=0; x<*cols; x++)
			for (c=0; c<*chan; c++)
				img[y][x][c] = data[i++];
	free(data);
	printf("done read\n");
	return img;
}

