//  ---------------------------------------------------------------------------
//
//      Copyright (c) 1998-1999 Greg Roelofs.  All rights reserved.
//
//      This software is provided "as is," without warranty of any kind,
//      express or implied.  In no event shall the author or contributors
//      be held liable for any damages arising in any way from the use of
//      this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute
//      it freely, subject to the following restrictions:
//
//      1. Redistributions of source code must retain the above copyright
//         notice, disclaimer, and this list of conditions.
//      2. Redistributions in binary form must reproduce the above copyright
//         notice, disclaimer, and this list of conditions in the documenta-
//         tion and/or other materials provided with the distribution.
//      3. All advertising materials mentioning features or use of this
//         software must display the following acknowledgment:
//
//            This product includes software developed by Greg Roelofs
//            and contributors for the book, "PNG: The Definitive Guide,"
//            published by O'Reilly and Associates.
//
//  ---------------------------------------------------------------------------*/

#ifndef _LINUX
#include <setjmp.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include "typedefs.h"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef MAX
#define MAX(a,b)  ((a) > (b)? (a) : (b))
#define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

#define alpha_composite(composite, fg, alpha, bg) {			\
    ush temp = ((ush)(fg)*(ush)(alpha) +				\
                (ush)(bg)*(ush)(255 - (ush)(alpha)) + (ush)128);	\
    (composite) = (uch)((temp + (temp >> 8)) >> 8);			\
}

typedef unsigned char   uch;
typedef unsigned short  ush;
typedef unsigned long   ulg;

/* prototypes for public functions in readpng.c */
/*
void readpng_version_info(void);

int readpng_init(FILE *infile, long *pWidth, long *pHeight);

int readpng_get_bgcolor(uch *bg_red, uch *bg_green, uch *bg_blue);

uch *readpng_get_image(double display_exponent, int *pChannels,
                       ulg *pRowbytes);

void readpng_cleanup(int free_image_data);
*/

// ========================================================

static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

static png_uint_32 width, height;
static int bit_depth, color_type;
static uch *image_data = NULL;

static uch bg_red=0, bg_green=0, bg_blue=0;

static double display_exponent;

static ulg image_width, image_height, image_rowbytes;
static int image_channels;
static FILE *infile;

// ========================================================

// return value = 0 for success, 1 for bad sig, 2 for bad IHDR, 4 for no mem
static int readpng_init(FILE *infile, ulg *pWidth, ulg *pHeight)
{
	uch sig[8];

	// first do a quick check that the file really is a PNG image; could
	// have used slightly more general png_sig_cmp() function instead
	fread(sig, 1, 8, infile);
	if (!png_check_sig(sig, 8))
		return 1;	// bad signature


	// could pass pointers to user-defined error handlers instead of NULLs:
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return 4;	// out of memory

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return 4;	// out of memory
	}

	// we could create a second info struct here (end_info), but it's only
	// useful if we want to keep pre- and post-IDAT chunk info separated
	// (mainly for PNG-aware image editors and converters)


	// setjmp() must be called in every function that calls a PNG-reading
	// libpng function
	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return 2;
	}

	png_init_io(png_ptr, infile);
	png_set_sig_bytes(png_ptr, 8);	// we already read the 8 signature bytes

	png_read_info(png_ptr, info_ptr);  // read all PNG info up to image data

	// alternatively, could make separate calls to png_get_image_width(),
	// etc., but want bit_depth and color_type for later [don't care about
	// compression_type and filter_type => NULLs]
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	  NULL, NULL, NULL);
	*pWidth = width;
	*pHeight = height;

	// OK, that's all we need for now; return happy
	return 0;
}

// returns 0 if succeeds, 1 if fails due to no bKGD chunk, 2 if libpng error;
// scales values to 8-bit if necessary
static int readpng_get_bgcolor(uch *red, uch *green, uch *blue)
{
	png_color_16p pBackground;

	// setjmp() must be called in every function that calls a PNG-reading
	// libpng function
	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return 2;
	}

	if (!png_get_valid(png_ptr, info_ptr, PNG_INFO_bKGD))
		return 1;

	// it is not obvious from the libpng documentation, but this function
	// takes a pointer to a pointer, and it always returns valid red, green
	// and blue values, regardless of color_type:
	png_get_bKGD(png_ptr, info_ptr, &pBackground);

	// however, it always returns the raw bKGD data, regardless of any
	// bit-depth transformations, so check depth and adjust if necessary
	if (bit_depth == 16)
	{
		*red   = pBackground->red	>> 8;
		*green = pBackground->green >> 8;
		*blue  = pBackground->blue	>> 8;
	}
	else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	{
		if (bit_depth == 1)
			*red = *green = *blue = pBackground->gray? 255 : 0;
		else if (bit_depth == 2)
			*red = *green = *blue = (255/3) * pBackground->gray;
		else // bit_depth == 4
			*red = *green = *blue = (255/15) * pBackground->gray;
	}
	else
	{
		*red   = (uch)pBackground->red;
		*green = (uch)pBackground->green;
		*blue  = (uch)pBackground->blue;
	}

	return 0;
}

// display_exponent == LUT_exponent * CRT_exponent
static uch *readpng_get_image(double display_exponent, int *pChannels, ulg *pRowbytes)
{
	double	gamma;
	png_uint_32  i, rowbytes;
	png_bytepp	row_pointers = NULL;

	// setjmp() must be called in every function that calls a PNG-reading
	// libpng function
	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return NULL;
	}

	// expand palette images to RGB, low-bit-depth grayscale images to 8 bits,
	// transparency chunks to full alpha channel; strip 16-bit-per-sample
	// images to 8 bits per sample; and convert grayscale to RGB[A]
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	// unlike the example in the libpng documentation, we have *no* idea where
	// this file may have come from--so if it doesn't have a file gamma, don't
	// do any correction ("do no harm")
	if (png_get_gAMA(png_ptr, info_ptr, &gamma))
		png_set_gamma(png_ptr, display_exponent, gamma);

	// all transformations have been registered; now update info_ptr data,
	// get rowbytes and channels, and allocate image memory
	png_read_update_info(png_ptr, info_ptr);

	*pRowbytes = rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	*pChannels = (int)png_get_channels(png_ptr, info_ptr);

	if ((image_data = (uch *)malloc(rowbytes*height)) == NULL)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return NULL;
	}

	if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == NULL)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		free(image_data);
		image_data = NULL;
		return NULL;
	}

	// set the individual row_pointers to point at the correct offsets
	for (i = 0;  i < height;  ++i)
		row_pointers[i] = image_data + i*rowbytes;

	// now we can go ahead and just read the whole image
	png_read_image(png_ptr, row_pointers);

	// and we're done!	(png_read_end() can be omitted if no processing of
	// post-IDAT text/time/etc. is desired)
	free(row_pointers);
	row_pointers = NULL;

	png_read_end(png_ptr, NULL);

	return image_data;
}

static void readpng_cleanup(int free_image_data)
{
	if (free_image_data && image_data)
	{
		free(image_data);
		image_data = NULL;
	}

	if (png_ptr && info_ptr)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		png_ptr = NULL;
		info_ptr = NULL;
	}
}

LC_IMAGE* OpenPNG(char* filename)
{
	double LUT_exponent;		// just the lookup table
	double CRT_exponent = 2.2;	// just the monitor
	double default_display_exponent;	// whole display system

	char *p;
	int rc;
	int error = 0;


	// First set the default value for our display-system exponent, i.e.,
	// the product of the CRT exponent and the exponent corresponding to
	// the frame-buffer's lookup table (LUT), if any.  This is not an
	// exhaustive list of LUT values (e.g., OpenStep has a lot of weird
	// ones), but it should cover 99% of the current possibilities.  And
	// yes, these ifdefs are completely wasted in a Windows program...
#if defined(NeXT)
	LUT_exponent = 1.0 / 2.2;
	//	if (some_next_function_that_returns_gamma(&next_gamma))
	//		LUT_exponent = 1.0 / next_gamma;
#elif defined(sgi)
	LUT_exponent = 1.0 / 1.7;
	// there doesn't seem to be any documented function to get the
	// "gamma" value, so we do it the hard way
	infile = fopen("/etc/config/system.glGammaVal", "r");
	if (infile)
	{
		double sgi_gamma;

		fgets(tmpline, 80, infile);
		fclose(infile);
		sgi_gamma = atof(tmpline);
		if (sgi_gamma > 0.0)
			LUT_exponent = 1.0 / sgi_gamma;
	}
#elif defined(Macintosh)
	LUT_exponent = 1.8 / 2.61;
	//	if (some_mac_function_that_returns_gamma(&mac_gamma))
	//		LUT_exponent = mac_gamma / 2.61;				 
#else
	LUT_exponent = 1.0;   // assume no LUT:  most PCs
#endif

	// the defaults above give 1.0, 1.3, 1.5 and 2.2, respectively:
	default_display_exponent = LUT_exponent * CRT_exponent;

	// If the user has set the SCREEN_GAMMA environment variable as suggested
	// (somewhat imprecisely) in the libpng documentation, use that; otherwise
	// use the default value we just calculated.  Either way, the user may
	// override this via a command-line option.
	if ((p = getenv("SCREEN_GAMMA")) != NULL)
		display_exponent = atof(p);
	else
		display_exponent = default_display_exponent;

	if (!(infile = fopen(filename, "rb")))
	{
		printf("can't open PNG file [%s]\n", filename);
		++error;
	}
	else
	{
		if ((rc = readpng_init(infile, &image_width, &image_height)) != 0)
		{
			switch (rc)
			{
				case 1:
					printf("[%s] is not a PNG file: incorrect signature\n",filename);
					break;
				case 2:
					printf("[%s] has bad IHDR (libpng longjmp)\n", filename);
					break;
				case 4:
					printf("insufficient memory\n");
					break;
				default:
					printf("unknown readpng_init() error\n");
					break;
			}
			++error;
		}
		if (error)
			fclose(infile);
	}

	if (error)
		return NULL;

	// if the user didn't specify a background color on the command line,
	// check for one in the PNG file--if not, the initialized values of 0
	// (black) will be used

	if (readpng_get_bgcolor(&bg_red, &bg_green, &bg_blue) > 1)
	{
		readpng_cleanup(TRUE);
		printf("libpng error while checking for background color\n");
		return NULL;
	}

	// decode the image, all at once
	image_data = readpng_get_image(display_exponent, &image_channels,
	  &image_rowbytes);

	// done with PNG file, so clean up to minimize memory usage (but do NOT
	// nuke image_data!)
	readpng_cleanup(FALSE);
	fclose(infile);

	if (!image_data)
	{
		printf("unable to decode PNG image\n");
		return NULL;
	}

	// get our buffer set to hold data
	LC_IMAGE* image = (LC_IMAGE*)malloc(image_width*image_height*3 + sizeof(LC_IMAGE));

	if (image == NULL)
	{
//		MessageBox(NULL, "Error", "Cannot allocate memory", MB_ICONSTOP);
		free(image_data);
		return NULL;
	}

	image->width = (unsigned short)image_width;
	image->height = (unsigned short)image_height;
	image->bits = (char*)image + sizeof(LC_IMAGE);

	uch *src, *dest;
	uch r, g, b, a;
	ulg i, row;

	for (row = 0;  row < image_height;  ++row)
	{
		src = image_data + row*image_rowbytes;
		dest = (unsigned char*)image->bits + row*image_rowbytes;
		if (image_channels == 3)
		{
			for (i = image_width;  i > 0;  --i)
			{
				r = *src++;
				g = *src++;
				b = *src++;
				*dest++ = b;
				*dest++ = g;	/* note reverse order */
				*dest++ = r;
			}
		}
		else /* if (image_channels == 4) */
		{
			for (i = image_width;  i > 0;  --i)
			{
				r = *src++;
				g = *src++;
				b = *src++;
				a = *src++;

				if (a == 255)
				{
					*dest++ = b;
					*dest++ = g;
					*dest++ = r;
				}
				else if (a == 0)
				{
					*dest++ = bg_blue;
					*dest++ = bg_green;
					*dest++ = bg_red;
				}
				else
				{
					/* this macro (copied from png.h) composites the
					 * foreground and background values and puts the
					 * result into the first argument; there are no
					 * side effects with the first argument */
					alpha_composite(*dest++, b, a, bg_blue);
					alpha_composite(*dest++, g, a, bg_green);
					alpha_composite(*dest++, r, a, bg_red);
				}
			}
		}
	}

	free(image_data);
	return image;
}

