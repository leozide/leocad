///////////////////////////////////////
// DL1 Quantization

#ifndef _QUANT_H_
#define _QUANT_H_

typedef struct
{
	unsigned long r, g, b;
	unsigned long pixel_count;
	unsigned long pixels_in_cube;
	unsigned char children;
	unsigned char palette_index;
} CUBE;

typedef struct
{
	unsigned char level;
	unsigned short index;
} FCUBE;

typedef struct
{
	unsigned char palette_index, red, green, blue;
	unsigned long distance;
	unsigned long squares[255+255+1];
} CLOSEST_INFO;

bool dl1quant(unsigned char *inbuf, unsigned char *outbuf, int width, int height, int quant_to, int dither, unsigned char userpal[3][256]);

#endif // _QUANT_H_
