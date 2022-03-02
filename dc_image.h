#ifndef dc_image_h
#define dc_image_h



#define byte unsigned char

//-------------------------------------------------------
// A few image helper functions
//-------------------------------------------------------

// Memory Allocation

byte **malloc2d(int rows, int cols);

byte ***malloc3d(int rows, int cols, int chan);

void free2d(byte **data, int rows);

void free3d(byte ***data, int rows, int cols);


// Loading / Saving Images

void SaveRgbPng(byte ***in, const char *fname, int rows, int cols);

void SaveGrayPng(byte **in, const char *fname, int rows, int cols);

byte ***LoadRgb(const char *fname, int *rows, int *cols, int *chan);


#endif // dc_image_h
