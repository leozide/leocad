#include "lc_global.h"
#include <stdlib.h>
#include "image.h"
#include "lc_file.h"

#ifdef LC_HAVE_PNGLIB

#include <png.h>

#define alpha_composite(composite, fg, alpha, bg) {			\
  unsigned short temp = ((unsigned short)(fg)*(unsigned short)(alpha) +	\
  (unsigned short)(bg)*(unsigned short)(255 - (unsigned short)(alpha)) + (unsigned short)128);	\
  (composite) = (unsigned char)((temp + (temp >> 8)) >> 8);			\
}

// =============================================================================

static void user_read_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	png_size_t check;

	// Read() returns 0 on error, so it is OK to store this in a png_size_t
	// instead of an int, which is what Read() actually returns.
	check = (png_size_t)((File*)png_get_io_ptr(png_ptr))->Read(data, length);

	if (check != length)
		png_error(png_ptr, "Read Error");
}

bool Image::LoadPNG(File& file)
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

	FreeData();

	file.Read(sig, 8);
	if (!png_check_sig(sig, 8))
		return false;	// bad signature

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;	// out of memory

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return false;	// out of memory
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}

	png_set_read_fn(png_ptr, (void*)&file, user_read_fn);
	//  png_init_io(png_ptr, f);
	png_set_sig_bytes(png_ptr, 8);	// we already read the 8 signature bytes

	png_read_info(png_ptr, info_ptr);  // read all PNG info up to image data
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_bKGD))
	{
		png_get_bKGD(png_ptr, info_ptr, &pBackground);

		if (setjmp(png_jmpbuf(png_ptr)))
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			return false;
		}

		// however, it always returns the raw bKGD data, regardless of any
		// bit-depth transformations, so check depth and adjust if necessary
		if (bit_depth == 16)
		{
			red   = pBackground->red >> 8;
			green = pBackground->green >> 8;
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
	{
		if (setjmp(png_jmpbuf(png_ptr)))
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			return false;
		}

		red = green = blue = 0;
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
		return false;
	}

	if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == NULL)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		free(image_data);
		return false;
	}

	// set the individual row_pointers to point at the correct offsets
	for (i = 0;  i < height;  ++i)
		row_pointers[i] = image_data + i*image_rowbytes;

	// now we can go ahead and just read the whole image
	png_read_image(png_ptr, row_pointers);

	// and we're done! (png_read_end() can be omitted if no processing of
	// post-IDAT text/time/etc. is desired)
	free(row_pointers);
	row_pointers = NULL;

	png_read_end(png_ptr, NULL);

	// done with PNG file, so clean up to minimize memory usage
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	if (!image_data)
		return false;

	// get our buffer set to hold data
	m_pData = (unsigned char*)malloc(width*height*image_channels);

	if (m_pData == NULL)
	{
		free (image_data);
		return false;
	}

	m_nWidth = width;
	m_nHeight = height;
	if (image_channels == 3)
		m_bAlpha = false;
	else
		m_bAlpha = true;

	for (row = 0; row < height; row++)
	{
		src = image_data + row*image_rowbytes;
		dest = m_pData + row*image_channels*width;

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
				*dest++ = a;
			}
		}
	}

	free(image_data);
	return true;
}

// =============================================================================

static void user_write_fn(png_structp png_ptr, png_bytep data, png_size_t length)
{
	png_uint_32 check;

	check = ((File*)png_get_io_ptr(png_ptr))->Write(data, length);
	if (check != length)
	{
		png_error(png_ptr, "Write Error");
	}
}

static void user_flush_fn(png_structp png_ptr)
{
	((File*)png_get_io_ptr(png_ptr))->Flush();
}

bool Image::SavePNG(File& file, bool transparent, bool interlaced, unsigned char* background) const
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytepp row_pointers = NULL;
	png_color_8 sig_bit;
	png_color_16 bg;
	int i;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		return false;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return false;
	}

	//  png_init_io(png_ptr, fp);
	png_set_write_fn(png_ptr, &file, user_write_fn, user_flush_fn);

	png_set_IHDR(png_ptr, info_ptr, m_nWidth, m_nHeight, 8, 
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

	if ((row_pointers = (png_bytepp)malloc(m_nHeight*sizeof(png_bytep))) == NULL)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return false;
	}

	// set the individual row_pointers to point at the correct offsets
	if (transparent)
	{
		unsigned char *buf, *src, *dst, alpha;
		dst = buf = (unsigned char*)malloc(m_nWidth*m_nHeight*4);
		src = m_pData;

		for (i = 0; i < m_nWidth*m_nHeight; i++)
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

		for (i = 0; i < m_nHeight; i++)
			row_pointers[i] = buf + i*m_nWidth*4;
		png_write_image(png_ptr, row_pointers);

		free(buf);
	}
	else
	{
		for (i = 0; i < m_nHeight; i++)
			row_pointers[i] = m_pData + i*m_nWidth*3;
		png_write_image(png_ptr, row_pointers);
	}

	free(row_pointers);

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return true;
}

#endif // LC_HAVE_PNGLIB
