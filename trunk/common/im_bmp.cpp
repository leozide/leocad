#include <stdio.h>
#include <stdlib.h>
#include "typedefs.h"
#include "quant.h"

// ========================================================

LC_IMAGE* OpenBMP (char* filename)
{
	int bmWidth;
	int bmHeight;
	unsigned char bmPlanes;
	unsigned char bmBitsPixel;
	typedef struct {
		unsigned char rgbBlue;
		unsigned char rgbGreen;
		unsigned char rgbRed;
		unsigned char rgbReserved;
	} RGBQUAD;
	unsigned char m1,m2;
	unsigned long sizeimage;
	short res1,res2;
	long filesize, pixoff;
	long bmisize, compression;
	long xscale, yscale;
	long colors, impcol;
	LC_IMAGE* image;
	unsigned long m_bytesRead = 0;
	FILE *fp;
	
	fp = fopen(filename,"rb");
	if (fp == NULL)
		return NULL;

	long rc;
	rc = fread(&m1, 1, 1, fp);
	m_bytesRead++;
	if (rc == -1)
	{
		fclose(fp);
		return NULL;
	}

	rc = fread(&m2, 1, 1, fp);
	m_bytesRead++;
	if ((m1!='B') || (m2!='M'))
	{
		fclose(fp);
		return NULL;
	}

	rc = fread((long*)&(filesize),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((int*)&(res1),2,1,fp); m_bytesRead+=2;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((int*)&(res2),2,1,fp); m_bytesRead+=2;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(pixoff),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(bmisize),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long  *)&(bmWidth),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(bmHeight),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((int*)&(bmPlanes),2,1,fp); m_bytesRead+=2;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((int*)&(bmBitsPixel),2,1,fp); m_bytesRead+=2;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(compression),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(sizeimage),4,1,fp); m_bytesRead+=4;
	if (rc != 1) {fclose(fp); return NULL; }

	rc = fread((long*)&(xscale),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(yscale),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(colors),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	rc = fread((long*)&(impcol),4,1,fp); m_bytesRead+=4;
	if (rc != 1) { fclose(fp); return NULL; }

	if (colors == 0)
		colors = 1 << bmBitsPixel;

	RGBQUAD *colormap = NULL;

	if (bmBitsPixel != 24)
	{
		colormap = new RGBQUAD[colors];
		if (colormap == NULL)
		{
			fclose(fp);
			return NULL;
		}

		int i;
		for (i = 0; i < colors; i++)
		{
			unsigned char r ,g, b, dummy;

			rc = fread(&b, 1, 1, fp);
			m_bytesRead++;
			if (rc!=1)
			{
				delete [] colormap;
				fclose(fp);
				return NULL;
			}

			rc = fread(&g, 1, 1, fp); 
			m_bytesRead++;
			if (rc!=1)
			{
				delete [] colormap;
				fclose(fp);
				return NULL;
			}

			rc = fread(&r, 1, 1, fp); 
			m_bytesRead++;
			if (rc != 1)
			{
				delete [] colormap;
				fclose(fp);
				return NULL;
			}


			rc = fread(&dummy, 1, 1, fp); 
			m_bytesRead++;
			if (rc != 1)
			{
				delete [] colormap;
				fclose(fp);
				return NULL;
			}

			colormap[i].rgbRed=r;
			colormap[i].rgbGreen=g;
			colormap[i].rgbBlue=b;
		}
	}

	if ((long)m_bytesRead > pixoff)
	{
		delete [] colormap;
		fclose(fp);
		return NULL;
	}

	while ((long)m_bytesRead < pixoff)
	{
		char dummy;
		fread(&dummy,1,1,fp);
		m_bytesRead++;
	}

	int w = bmWidth;
	int h = bmHeight;

	// set the output params
	image = (LC_IMAGE*)malloc(w*h*3 + sizeof(LC_IMAGE));
	long row_size = w * 3;

	if (image != NULL) 
	{
		image->width = w;
		image->height = h;
		image->bits = (char*)image + sizeof(LC_IMAGE);
		unsigned char* outbuf = (unsigned char*)image->bits;
		long row = 0;
		long rowOffset = 0;

		if (compression == 0) // BI_RGB
		{
			// read rows in reverse order
			for (row=bmHeight-1;row>=0;row--)
			{
				// which row are we working on?
				rowOffset = (long unsigned)row*row_size;						      

				if (bmBitsPixel == 24)
				{
					for (int col=0;col<w;col++)
					{
						long offset = col * 3;
						char pixel[3];

						if (fread((void*)(pixel),1,3,fp)==3)
						{
							// we swap red and blue here
							*(outbuf + rowOffset + offset + 0)=pixel[2];		// r
							*(outbuf + rowOffset + offset + 1)=pixel[1];		// g
							*(outbuf + rowOffset + offset + 2)=pixel[0];		// b
						}
					}
					m_bytesRead += row_size;

					// read DWORD padding
					while ((m_bytesRead-pixoff)&3)
					{
						char dummy;
						if (fread(&dummy,1,1,fp) != 1)
						{
							free(image);
							fclose(fp);
							return NULL;
						}
						m_bytesRead++;
					}
				}
				else
				{
					// pixels are packed as 1 , 4 or 8 bit vals. need to unpack them
					int bit_count = 0;
					unsigned long mask = (1 << bmBitsPixel) - 1;
					unsigned char inbyte = 0;

					for (int col=0;col<w;col++)
					{
						int pix = 0;

						// if we need another byte
						if (bit_count <= 0)
						{
							bit_count = 8;
							if (fread(&inbyte,1,1,fp) != 1)
							{
								free(image);
								delete [] colormap;
								fclose(fp);
								return NULL;
							}
							m_bytesRead++;
						}

						// keep track of where we are in the bytes
						bit_count -= bmBitsPixel;
						pix = ( inbyte >> bit_count) & mask;

						// lookup the color from the colormap - stuff it in our buffer
						// swap red and blue
						*(outbuf + rowOffset + col * 3 + 2) = colormap[pix].rgbBlue;
						*(outbuf + rowOffset + col * 3 + 1) = colormap[pix].rgbGreen;
						*(outbuf + rowOffset + col * 3 + 0) = colormap[pix].rgbRed;
					}

					// read DWORD padding
					while ((m_bytesRead-pixoff)&3)
					{
						char dummy;
						if (fread(&dummy,1,1,fp)!=1)
						{
							free(image);
							if (colormap)
								delete [] colormap;
							fclose(fp);
							return NULL;
						}
						m_bytesRead++;
					}
				}
			}
		}
		else
		{
			int i, x = 0;
			unsigned char c, c1 = 0, *pp;
			row = 0;
			pp = outbuf + (bmHeight-1)*bmWidth*3;

			if (bmBitsPixel == 8)
			{
				while (row < bmHeight)
				{
					c = getc(fp);

					if (c)
					{
						// encoded mode
						c1 = getc(fp);
						for (i = 0; i < c; x++, i++)
						{
							*pp = colormap[c1].rgbRed; pp++;
							*pp = colormap[c1].rgbGreen; pp++;
							*pp = colormap[c1].rgbBlue; pp++;
						}
					}
					else
					{
						// c==0x00,  escape codes
						c = getc(fp);

						if (c == 0x00) // end of line
						{
							row++;
							x = 0;
							pp = outbuf + (bmHeight-row-1)*bmWidth*3;
						}
						else if (c == 0x01)
							break; // end of pic
						else if (c == 0x02) // delta
						{
							c = getc(fp);
							x += c;
							c = getc(fp);
							row += c;
							pp = outbuf + x*3 + (bmHeight-row-1)*bmWidth*3;
						}
						else // absolute mode
						{
							for (i = 0; i < c; x++, i++)
							{
								c1 = getc(fp);
								*pp = colormap[c1].rgbRed; pp++;
								*pp = colormap[c1].rgbGreen; pp++;
								*pp = colormap[c1].rgbBlue; pp++;
							}
					
							if (c & 1)
								getc(fp); // odd length run: read an extra pad byte
						}
					}
				}
			}
			else if (bmBitsPixel == 4)
			{
				while (row < bmHeight)
				{
					c = getc(fp);

					if (c)
					{
						// encoded mode
						c1 = getc(fp);
						for (i = 0; i < c; x++, i++)
						{
							*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbRed; pp++;
							*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbGreen; pp++;
							*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbBlue; pp++;
						}
					}
					else
					{
						// c==0x00,  escape codes
						c = getc(fp);

						if (c == 0x00) // end of line
						{
							row++;
							x = 0;
							pp = outbuf + (bmHeight-row-1)*bmWidth*3;
						}
						else if (c == 0x01)
							break; // end of pic
						else if (c == 0x02) // delta
						{
							c = getc(fp);
							x += c;
							c = getc(fp);
							row += c;
							pp = outbuf + x*3 + (bmHeight-row-1)*bmWidth*3;
						}
						else // absolute mode
						{
							for (i = 0; i < c; x++, i++)
							{
							    if ((i&1) == 0)
									c1 = getc(fp);
								*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbRed; pp++;
								*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbGreen; pp++;
								*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbBlue; pp++;
							}
					
							if (((c&3) == 1) || ((c&3) == 2))
								getc(fp); // odd length run: read an extra pad byte
						}
					}
				}
			}
		}

		if (colormap)
			delete [] colormap;

		fclose(fp);
    }

	return image;
}

// ========================================================

bool SaveBMP(char* filename, LC_IMAGE* image, bool quantize)
{
	FILE *fp;	
	fp = fopen(filename, "wb");
	if (fp == NULL) 
		return false;

	unsigned short bits;
	unsigned long cmap, bfSize;
	unsigned char pal[3][256], *colormappedbuffer = NULL;

	if (quantize)
	{
		colormappedbuffer = (unsigned char*)malloc(image->width*image->height);
		dl1quant((unsigned char*)image->bits, colormappedbuffer, image->width, image->height, 256, true, pal);
		bits = 8;
		cmap = 256;
		bfSize = 1078 + image->width*image->height;
	}
	else
	{
		bits = 24;
		cmap = 0;
		bfSize = 54 + image->width*image->height*3;
	} 

	long byteswritten = 0;
	long pixoff = 54 + cmap*4;
	short res = 0;
	char m1 ='B', m2 ='M';
	fwrite(&m1, 1, 1, fp);		byteswritten++;	// B
	fwrite(&m2, 1, 1, fp);		byteswritten++; // M
	fwrite(&bfSize, 4, 1, fp);	byteswritten+=4;// bfSize
	fwrite(&res, 2, 1, fp);		byteswritten+=2;// bfReserved1
	fwrite(&res, 2, 1, fp);		byteswritten+=2;// bfReserved2
	fwrite(&pixoff, 4, 1, fp);	byteswritten+=4;// bfOffBits

	unsigned long biSize = 40, compress = 0, size = 0;
	long width = image->width, height = image->height, pixels = 0;
	unsigned short planes = 1;
	fwrite(&biSize, 4, 1, fp);	byteswritten+=4;// biSize
	fwrite(&width, 4, 1, fp);	byteswritten+=4;// biWidth
	fwrite(&height, 4, 1, fp);	byteswritten+=4;// biHeight
	fwrite(&planes, 2, 1, fp);	byteswritten+=2;// biPlanes
	fwrite(&bits, 2, 1, fp);	byteswritten+=2;// biBitCount
	fwrite(&compress, 4, 1, fp);byteswritten+=4;// biCompression
	fwrite(&size, 4, 1, fp);	byteswritten+=4;// biSizeImage
	fwrite(&pixels, 4, 1, fp);	byteswritten+=4;// biXPelsPerMeter
	fwrite(&pixels, 4, 1, fp);	byteswritten+=4;// biYPelsPerMeter
	fwrite(&cmap, 4, 1, fp);	byteswritten+=4;// biClrUsed
	fwrite(&cmap, 4, 1, fp);	byteswritten+=4;// biClrImportant

	if (quantize)
	{
		for (int i = 0; i < 256; i++) 
		{
			putc(pal[2][i], fp);
			putc(pal[1][i], fp);
			putc(pal[0][i], fp);
			putc(0, fp);	// dummy
		}

		for (int row = 0; row < image->height; row++) 
		{
			int pixbuf = 0;

			for (int col = 0; col < image->width; col++) 
			{
				int offset = (image->height-row-1) * width + col;	// offset into our color-mapped RGB buffer
				unsigned char pval = *(colormappedbuffer + offset);

				pixbuf = (pixbuf << 8) | pval;

				putc(pixbuf, fp);
				pixbuf = 0;
				byteswritten++;
			}

			// DWORD align
			while ((byteswritten - pixoff) & 3)
			{
				putc(0, fp);
				byteswritten++;
			}
		}

		free(colormappedbuffer);
	}
	else
	{
		unsigned long widthDW = (((image->width*24) + 31) / 32 * 4);
		long row, row_size = image->width*3;
		for (row = 0; row < image->height; row++) 
		{
			unsigned char* buf = (unsigned char*)image->bits+(image->height-row-1)*row_size;

			// write a row
			for (int col = 0; col < row_size; col += 3)
			{
				putc(buf[col+2], fp);
				putc(buf[col+1], fp);
				putc(buf[col], fp);
			}
			byteswritten += row_size;	

			for (unsigned long count = row_size; count < widthDW; count++)
			{
				putc(0, fp);	// dummy
				byteswritten++;
			}
		}
	}

	fclose(fp);
	return true;
}
