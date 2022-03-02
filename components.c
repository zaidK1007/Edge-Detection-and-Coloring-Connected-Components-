#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "dc_image.h"



#define MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define MAX(a,b)  ( (a) > (b) ? (a) : (b) )
#define ABS(x)    ( (x) <= 0 ? 0-(x) : (x) )


//--------------------------------------------------
//--------------------------------------------------
// You must modify this disjoint set implementation
//--------------------------------------------------
//--------------------------------------------------

struct DisjointSet;

typedef struct DisjointSet {
	int r,g,b;
	int x,y;
	int rank;
	struct DisjointSet *parent;
} DisjointSet;



DisjointSet *DisjointSetFindRoot(DisjointSet *curr)
{
	// Add code here
	if(curr->parent == curr)
		return curr;
	else{
	curr->parent = DisjointSetFindRoot(curr->parent);
	return curr->parent;  
	}
}

void DisjointSetUnion(DisjointSet *a, DisjointSet *b)
{

	// Add code here
	if(a == b) return;
	DisjointSet *a_parent, *b_parent;
	a_parent = DisjointSetFindRoot(a);
	b_parent = DisjointSetFindRoot(b);

	if(a_parent == b_parent) return;
	if(a_parent->rank == b_parent->rank){
		a_parent->parent = b_parent;
		b_parent->rank += 1;
	}
	if(a_parent->rank < b_parent->rank){
		a_parent->parent = b_parent;
	}	
	else{
		b_parent->parent = a_parent;
	}
}

//--------------------------------------------------
//--------------------------------------------------
// The following "run" function runs the entire algorithm
//  for a single vision file
//--------------------------------------------------
//--------------------------------------------------

float normal_pdf(float x, float s)
{
    float pi = 1.0 / (s * sqrt(2.0*M_PI));
    float a = x / s;
    return (pi) * exp(-0.5f * a * a);
}

void run(const char *infile, const char *outpre, int canny_thresh, int canny_blur, float set_thresh, float sigma) 
{
	int y,x,i,j;
	int rows, cols, chan;
	// float set_thresh = 4.5;
	//-----------------
	// Read the image    [y][x][c]   y number rows   x cols  c 3
	//-----------------
	byte ***img = LoadRgb(infile, &rows, &cols, &chan);
	printf("img %p rows %d cols %d chan %d\n", img, rows, cols, chan);
	
	

	char str[4096];
	sprintf(str, "out/%s_1_img.png", outpre);
	SaveRgbPng(img, str, rows, cols);
	
	//-----------------
	// Convert to Grayscale
	//-----------------
	byte **gray = malloc2d(rows, cols);
	for (y=0; y<rows; y++){
		for (x=0; x<cols; x++) {
			int r = img[y][x][0];   // red
			int g = img[y][x][1];   // green
			int b = img[y][x][2];   // blue
			gray[y][x] =  (r+g+b) / 3;
		}
	}

	sprintf(str, "out/%s_2_gray.png", outpre);
	SaveGrayPng(gray, str, rows, cols);

	//-----------------
	// Box Blur   ToDo: Gaussian Blur is better
	//-----------------
	
	// Box blur is separable, so separately blur x and y
	int k_x=canny_blur, k_y=canny_blur;
	
	// blur in the x dimension
	byte **blurx = (byte**)malloc2d(rows, cols);
	for (y=0; y<rows; y++) {
		for (x=0; x<cols; x++) {
			
			// Start and end to blur
			int minx = x-k_x/2;      // k_x/2 left of pixel
			int maxx = minx + k_x;   // k_x/2 right of pixel
			minx = MAX(minx, 0);     // keep in bounds
			maxx = MIN(maxx, cols);
			
			// average blur it
			int x2;
			int total = 0;
			int count = 0;
			for (x2=minx; x2<maxx; x2++) {
				total += gray[y][x2];    // use "gray" as input
				count++;
			}
			blurx[y][x] = total / count; // blurx is output
		}
	}
	
	sprintf(str, "out/%s_3_blur_just_x.png", outpre);
	SaveGrayPng(blurx, str, rows, cols);
	
	// blur in the y dimension
	byte **blur = (byte**)malloc2d(rows, cols);
	for (y=0; y<rows; y++) {
		for (x=0; x<cols; x++) {
			
			// Start and end to blur
			int miny = y-k_y/2;      // k_x/2 left of pixel
			int maxy = miny + k_y;   // k_x/2 right of pixel
			miny = MAX(miny, 0);     // keep in bounds
			maxy = MIN(maxy, rows);
			
			// average blur it
			int y2;
			int total = 0;
			int count = 0;
			for (y2=miny; y2<maxy; y2++) {
				total += blurx[y2][x];    // use blurx as input
				count++;
			}
			blur[y][x] = total / count;   // blur is output
		}
	}
	
	sprintf(str, "out/%s_3_blur.png", outpre);
	SaveGrayPng(blur, str, rows, cols);
	
	
	// //-----------------
	// Take the "Sobel" (magnitude of derivative)
	//  (Actually we'll make up something similar)
	//-----------------
	
	byte **sobel = (byte**)malloc2d(rows, cols);
	
	for (y=0; y<rows; y++) {
		for (x=0; x<cols; x++) {
			int mag=0;
			
			if (y>0)      mag += ABS((int)blur[y-1][x] - (int)blur[y][x]);
			if (x>0)      mag += ABS((int)blur[y][x-1] - (int)blur[y][x]);
			if (y<rows-1) mag += ABS((int)blur[y+1][x] - (int)blur[y][x]);
			if (x<cols-1) mag += ABS((int)blur[y][x+1] - (int)blur[y][x]);
			
			int out = 3*mag;
			sobel[y][x] = MIN(out,255);
		}
	}
	
	
	sprintf(str, "out/%s_4_sobel.png", outpre);
	SaveGrayPng(sobel, str, rows, cols);
	
	//-----------------
	// Non-max suppression
	//-----------------
	byte **nonmax = malloc2d(rows, cols);    // note: *this* initializes to zero!
	
	for (y=1; y<rows-1; y++)
	{
		for (x=1; x<cols-1; x++)
		{
			// Is it a local maximum
			int is_y_max = (sobel[y][x] > sobel[y-1][x] && sobel[y][x]>=sobel[y+1][x]);
			int is_x_max = (sobel[y][x] > sobel[y][x-1] && sobel[y][x]>=sobel[y][x+1]);
			if (is_y_max || is_x_max)
				nonmax[y][x] = sobel[y][x];
			else
				nonmax[y][x] = 0;
		}
	}
	
	sprintf(str, "out/%s_5_nonmax.png", outpre);
	SaveGrayPng(nonmax, str, rows, cols);
	
	//-----------------
	// Final Threshold
	//-----------------
	byte **edges = malloc2d(rows, cols);    // note: *this* initializes to zero!
	int count_sets = 0;
	for (y=0; y<rows; y++) {
		for (x=0; x<cols; x++) {
			if (nonmax[y][x] > canny_thresh) {
				edges[y][x] = 255;
				count_sets++;
			}
			else
				edges[y][x] = 0;
		}
	}
	
	sprintf(str, "out/%s_6_edges.png", outpre);
	SaveGrayPng(edges, str, rows, cols);

	DisjointSet sets[count_sets];
	//-----------------
	// Making Disjoint Set
	//-----------------
	i=0;
	for(y=0;y<rows;y++) {
		for(x=0;x<cols;x++) {
			if(edges[y][x] == 255) {
				sets[i].x = x;
				sets[i].y = y;
				sets[i].rank = 0;
				sets[i].parent = &sets[i];
				i++;
			}
		}
	}

	for(i=0;i<count_sets;i++) {
		for(j=i+1;j<count_sets;j++) {
			if(hypot(sets[i].y - sets[j].y, sets[i].x -sets[j].x)<set_thresh) {
				DisjointSetUnion(&sets[i],&sets[j]);	
			}
			
		}
	}
	int ran_r = 25,ran_g = 125,ran_b = 85;
	for(i=0;i<count_sets;i++) {
		if(DisjointSetFindRoot(&sets[i]) == &sets[i]){
			sets[i].r = ran_r;
			sets[i].g = ran_g;
			sets[i].b = ran_b;
			ran_r+=90;
			ran_g+=35;
			ran_b+=50;
		}
	}

	for(i=0;i<count_sets;i++) {
		sets[i].r = sets[i].parent->r;
		sets[i].g = sets[i].parent->g;
		sets[i].b = sets[i].parent->b;
	}	

	
	byte ***color = malloc3d(rows,cols,chan);
	for (i=0; i<count_sets; i++) {
			color[sets[i].y][sets[i].x][0] = sets[i].r;
			color[sets[i].y][sets[i].x][1] = sets[i].g;
			color[sets[i].y][sets[i].x][2] = sets[i].b;
		}

	sprintf(str, "out/%s_7_color.png", outpre);
	SaveRgbPng(color, str, rows, cols);

	float xj=0,yj=0;
	float s_scalar=0.0,scalar=0.0;
	// byte ***smooth = malloc3d(rows,cols,chan);

	for(i=0;i<count_sets;i++) {
		float xj=0.0,yj=0.0;
		float s_scalar=0.0,scalar=0.0;
		for(j=0;j<count_sets;j++) {
			if(sets[i].parent == sets[j].parent){
				scalar = normal_pdf(hypot(sets[i].y - sets[j].y,sets[i].x - sets[j].x),sigma);
				xj += scalar*sets[j].x;
				yj += scalar*sets[j].y;
				s_scalar += scalar;
			}	
		}
		if(s_scalar!=0) {
		int p,q;
		xj = xj/s_scalar;
		yj = yj/s_scalar;
		p = (int)xj;
		q = (int)yj;
		color[q][p][0] = 255;
		color[q][p][1] = 0;
		color[q][p][2] = 0;
	}
		
	}

	sprintf(str, "out/%s_8_smooth.png", outpre);
	SaveRgbPng(color, str, rows, cols);
}

	
int main()
{
	system("mkdir out");
	
	//
	// Main simply calls the run function 
	//  with different parameters for each image
	//
	run("puppy.jpg", "puppy", 45, 25, 4.5, 12.0); // 4.5 6
	run("pentagon.png", "pentagon", 45, 10, 2.5, 10.0); // 4
	run("tiger.jpg", "tiger", 45, 10, 4.0, 5.0); // 4
	run("houses.jpg", "houses", 45, 10, 2.0, 3.0); // 2  	
	printf("Done!\n");

	return 0;
}
