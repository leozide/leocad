#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include "typedefs.h"

#define alpha_composite(composite, fg, alpha, bg) {			\
  unsigned short temp = ((unsigned short)(fg)*(unsigned short)(alpha) +	\
  (unsigned short)(bg)*(unsigned short)(255 - (unsigned short)(alpha)) + (unsigned short)128);	\
  (composite) = (unsigned char)((temp + (temp >> 8)) >> 8);			\
}

// ========================================================

LC_IMAGE* OpenPNG(char* filename)
{
	unsigned char sig[8], red, green, blue;
	unsigned char *image_data = NULL;
	unsigned char *src, *dest;
	unsigned char r, g, b, a;
	unsigned long i, row;
	unsigned long image_rowbytes;
	png_color_16p pBackground;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_uint_32 width, height;
	png_bytepp row_pointers = NULL;
	int bit_depth, color_type;
	int image_channels;
	double gamma;
	FILE* f;

	f = fopen(filename, "rb");
	if (f == NULL)
		return NULL;

	fread(sig, 1, 8, f);
	if (!png_check_sig(sig, 8))
	{
		fclose(f);
		return NULL;	// bad signature
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fclose(f);
		return NULL;	// out of memory
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(f);
		return NULL;	// out of memory
	}

	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(f);
		return NULL;
	}

	png_init_io(png_ptr, f);
	png_set_sig_bytes(png_ptr, 8);	// we already read the 8 signature bytes

	png_read_info(png_ptr, info_ptr);  // read all PNG info up to image data
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		NULL, NULL, NULL);

	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(f);
		return NULL;
	}

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_bKGD))
	{
		png_get_bKGD(png_ptr, info_ptr, &pBackground);

		// however, it always returns the raw bKGD data, regardless of any
		// bit-depth transformations, so check depth and adjust if necessary
		if (bit_depth == 16)
		{
			red   = pBackground->red	>> 8;
			green = pBackground->green	>> 8;
			blue  = pBackground->blue	>> 8;
		}
		else if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		{
			if (bit_depth == 1)
				red = green = blue = pBackground->gray? 255 : 0;
			else if (bit_depth == 2)
				red = green = blue = (255/3) * pBackground->gray;
			else // bit_depth == 4
				red = green = blue = (255/15) * pBackground->gray;
		}
		else
		{
			red   = (unsigned char)pBackground->red;
			green = (unsigned char)pBackground->green;
			blue  = (unsigned char)pBackground->blue;
		}
	}
	else
	  red = green = blue = 0;

	// decode the image, all at once
	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(f);
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

	if (png_get_gAMA(png_ptr, info_ptr, &gamma))
		png_set_gamma(png_ptr, 2.2, gamma);

	// all transformations have been registered; now update info_ptr data,
	// get rowbytes and channels, and allocate image memory
	png_read_update_info(png_ptr, info_ptr);

	image_rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	image_channels = (int)png_get_channels(png_ptr, info_ptr);

	if ((image_data = (unsigned char*)malloc(image_rowbytes*height)) == NULL)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(f);
		return NULL;
	}

	if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == NULL)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		free(image_data);
		fclose(f);
		return NULL;
	}

	// set the individual row_pointers to point at the correct offsets
	for (i = 0;  i < height;  ++i)
		row_pointers[i] = image_data + i*image_rowbytes;

	// now we can go ahead and just read the whole image
	png_read_image(png_ptr, row_pointers);

	// and we're done!	(png_read_end() can be omitted if no processing of
	// post-IDAT text/time/etc. is desired)
	free(row_pointers);
	row_pointers = NULL;

	png_read_end(png_ptr, NULL);

	// done with PNG file, so clean up to minimize memory usage
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(f);

	if (!image_data)
		return NULL;

	// get our buffer set to hold data
	LC_IMAGE* image = (LC_IMAGE*)malloc(width*height*3 + sizeof(LC_IMAGE));

	if (image == NULL)
	{
		free(image_data);
		return NULL;
	}

	image->width = (unsigned short)width;
	image->height = (unsigned short)height;
	image->bits = (char*)image + sizeof(LC_IMAGE);

	for (row = 0; row < height; row++)
	{
		src = image_data + row*image_rowbytes;
		dest = (unsigned char*)image->bits + row*image_rowbytes;
		if (image_channels == 3)
		{
			for (i = width; i > 0; i--)
			{
				r = *src++;
				g = *src++;
				b = *src++;
				*dest++ = r;
				*dest++ = g;
				*dest++ = b;
			}
		}
		else // if (image_channels == 4)
		{
			for (i = width; i > 0; i--)
			{
				r = *src++;
				g = *src++;
				b = *src++;
				a = *src++;

				if (a == 255)
				{
					*dest++ = r;
					*dest++ = g;
					*dest++ = b;
				}
				else if (a == 0)
				{
					*dest++ = red;
					*dest++ = green;
					*dest++ = blue;
				}
				else
				{
					// this macro (copied from png.h) composites the
					// foreground and background values and puts the
					// result into the first argument; there are no
					// side effects with the first argument
					alpha_composite(*dest++, r, a, red);
					alpha_composite(*dest++, g, a, green);
					alpha_composite(*dest++, b, a, blue);
				}
			}
		}
	}

	free(image_data);
	return image;
}

// ========================================================

bool SavePNG(char* filename, LC_IMAGE* image, bool transparent, bool interlaced, unsigned char* background)
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytepp row_pointers = NULL;
	png_color_8 sig_bit;
	png_color_16 bg;
	int i;
	
	fp = fopen(filename, "wb");
	if (!fp)
		return false;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fclose(fp);
		return false;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		return false;
    }

	if (setjmp(png_ptr->jmpbuf))
	{
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		fclose(fp);
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, image->width, image->height, 8, 
		transparent ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
		interlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	bg.red = background[0];
	bg.green = background[1];
	bg.blue = background[2];
	png_set_bKGD(png_ptr, info_ptr, &bg);

	png_write_info(png_ptr, info_ptr);

	// Set the true bit depth of the image data
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	sig_bit.alpha = 8;

	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

	if ((row_pointers = (png_bytepp)malloc(image->height*sizeof(png_bytep))) == NULL)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		fclose(fp);
		return false;
	}

	// set the individual row_pointers to point at the correct offsets
	if (transparent)
	{
		unsigned char *buf, *src, *dst, alpha;
		dst = buf = (unsigned char*)malloc(image->width*image->height*4);
		src = (unsigned char*)image->bits;

		for (i = 0; i < image->width*image->height; i++)
		{
			if ((src[0] == background[0]) &&
				(src[1] == background[1]) &&
				(src[2] == background[2]))
				alpha = 0;
			else
				alpha = 255;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = alpha;
		}

		for (i = 0; i < image->height; i++)
			row_pointers[i] = buf + i*image->width*4;
	    png_write_image(png_ptr, row_pointers);

		free(buf);
	}
	else
	{
		for (i = 0; i < image->height; i++)
			row_pointers[i] = (unsigned char*)image->bits + i*image->width*3;
	    png_write_image(png_ptr, row_pointers);
	}

	free(row_pointers);

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);

	return true;
}

