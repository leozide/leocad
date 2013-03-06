#include "lc_global.h"
#include <stdio.h>
#include <stdlib.h>
#include "quant.h"
#include "image.h"
#include "lc_file.h"

// ========================================================

bool Image::LoadBMP(lcFile& file)
{
	lcint32 bmWidth, bmHeight;
	lcuint16 bmPlanes, bmBitsPixel;
	lcuint8 m1, m2;
	typedef struct {
		unsigned char rgbBlue;
		unsigned char rgbGreen;
		unsigned char rgbRed;
		unsigned char rgbReserved;
	} RGBQUAD;
	lcint16 res1,res2;
	lcint32 filesize, pixoff;
	lcint32 bmisize, compression;
	lcint32 xscale, yscale;
	lcint32 colors, impcol, rc;
	lcuint32 sizeimage, m_bytesRead = 0;

	FreeData ();

	if (file.ReadU8(&m1, 1) != 1)
		return false;
	m_bytesRead++;

	if (file.ReadU8(&m2, 1) != 1)
		return false;
	m_bytesRead++;

	if ((m1 != 'B') || (m2 != 'M'))
		return false;

	rc = file.ReadS32(&filesize, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadS16(&res1, 1); m_bytesRead+=2;
	if (rc != 1) { return false; }

	rc = file.ReadS16(&res2, 1); m_bytesRead+=2;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&pixoff, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&bmisize, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&bmWidth, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&bmHeight, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadU16(&bmPlanes, 1); m_bytesRead+=2;
	if (rc != 1) { return false; }

	rc = file.ReadU16(&bmBitsPixel, 1); m_bytesRead+=2;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&compression, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadU32(&sizeimage, 1); m_bytesRead+=4;
	if (rc != 1) {return false; }

	rc = file.ReadS32(&xscale, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&yscale, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&colors, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	rc = file.ReadS32(&impcol, 1); m_bytesRead+=4;
	if (rc != 1) { return false; }

	if (colors == 0)
		colors = 1 << bmBitsPixel;

	RGBQUAD *colormap = NULL;

	if (bmBitsPixel != 24)
	{
		colormap = new RGBQUAD[colors];
		if (colormap == NULL)
			return false;

		int i;
		for (i = 0; i < colors; i++)
		{
			unsigned char r ,g, b, dummy;

			rc = file.ReadU8(&b, 1);
			m_bytesRead++;
			if (rc!=1)
			{
				delete [] colormap;
				return false;
			}

			rc = file.ReadU8(&g, 1); 
			m_bytesRead++;
			if (rc!=1)
			{
				delete [] colormap;
				return false;
			}

			rc = file.ReadU8(&r, 1); 
			m_bytesRead++;
			if (rc != 1)
			{
				delete [] colormap;
				return false;
			}

			rc = file.ReadU8(&dummy, 1); 
			m_bytesRead++;
			if (rc != 1)
			{
				delete [] colormap;
				return false;
			}

			colormap[i].rgbRed = r;
			colormap[i].rgbGreen = g;
			colormap[i].rgbBlue = b;
		}
	}

	if ((long)m_bytesRead > pixoff)
	{
		delete [] colormap;
		return false;
	}

	while ((long)m_bytesRead < pixoff)
	{
		lcuint8 dummy;
		file.ReadU8(&dummy, 1);
		m_bytesRead++;
	}

	int w = bmWidth;
	int h = bmHeight;

	// set the output params
	m_pData = (unsigned char*)malloc (w*h*3);
	long row_size = w * 3;

	if (m_pData != NULL) 
	{
		m_nWidth = w;
		m_nHeight = h;
		m_bAlpha = false;
		unsigned char* outbuf = m_pData;
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
						lcuint8 pixel[3];

						if (file.ReadU8(pixel, 3) ==3)
						{
							// we swap red and blue here
							*(outbuf + rowOffset + offset + 0) = pixel[2];		// r
							*(outbuf + rowOffset + offset + 1) = pixel[1];		// g
							*(outbuf + rowOffset + offset + 2) = pixel[0];		// b
						}
					}
					m_bytesRead += row_size;

					// read DWORD padding
					while ((m_bytesRead-pixoff)&3)
					{
						lcuint8 dummy;
						if (file.ReadU8(&dummy, 1) != 1)
						{
							FreeData ();
							return false;
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
							if (file.ReadU8(&inbyte, 1) != 1)
							{
								FreeData ();
								delete [] colormap;
								return false;
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
						lcuint8 dummy;
						if (file.ReadU8(&dummy, 1) != 1)
						{
							FreeData ();
							if (colormap)
								delete [] colormap;
							return false;
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
					c = file.ReadU8();

					if (c)
					{
						// encoded mode
						c1 = file.ReadU8();
						for (i = 0; i < c; x++, i++)
						{
							*pp = colormap[c1].rgbRed; pp++;
							*pp = colormap[c1].rgbGreen; pp++;
							*pp = colormap[c1].rgbBlue; pp++;
						}
					}
					else
					{
						// c==0x00, escape codes
						c = file.ReadU8();

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
							c = file.ReadU8();
							x += c;
							c = file.ReadU8();
							row += c;
							pp = outbuf + x*3 + (bmHeight-row-1)*bmWidth*3;
						}
						else // absolute mode
						{
							for (i = 0; i < c; x++, i++)
							{
								c1 = file.ReadU8();
								*pp = colormap[c1].rgbRed; pp++;
								*pp = colormap[c1].rgbGreen; pp++;
								*pp = colormap[c1].rgbBlue; pp++;
							}
					
							if (c & 1)
								file.ReadU8(); // odd length run: read an extra pad byte
						}
					}
				}
			}
			else if (bmBitsPixel == 4)
			{
				while (row < bmHeight)
				{
					c = file.ReadU8();

					if (c)
					{
						// encoded mode
						c1 = file.ReadU8();
						for (i = 0; i < c; x++, i++)
						{
							*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbRed; pp++;
							*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbGreen; pp++;
							*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbBlue; pp++;
						}
					}
					else
					{
						// c==0x00, escape codes
						c = file.ReadU8();

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
							c = file.ReadU8();
							x += c;
							c = file.ReadU8();
							row += c;
							pp = outbuf + x*3 + (bmHeight-row-1)*bmWidth*3;
						}
						else // absolute mode
						{
							for (i = 0; i < c; x++, i++)
							{
								if ((i&1) == 0)
									c1 = file.ReadU8();
								*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbRed; pp++;
								*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbGreen; pp++;
								*pp = colormap[(i&1) ? (c1 & 0x0f) : ((c1>>4)&0x0f)].rgbBlue; pp++;
							}
					
							if (((c&3) == 1) || ((c&3) == 2))
								file.ReadU8(); // odd length run: read an extra pad byte
						}
					}
				}
			}
		}

		if (colormap)
			delete [] colormap;
	}

	return true;
}

// ========================================================

bool Image::SaveBMP(lcFile& file, bool quantize) const
{
	lcuint16 bits;
	lcuint32 cmap, bfSize;
	lcuint8 pal[3][256], *colormappedbuffer = NULL;

	if (quantize)
	{
		colormappedbuffer = (unsigned char*)malloc(m_nWidth*m_nHeight);
		dl1quant (m_pData, colormappedbuffer, m_nWidth, m_nHeight, 256, true, pal);
		bits = 8;
		cmap = 256;
		bfSize = 1078 + m_nWidth*m_nHeight;
	}
	else
	{
		bits = 24;
		cmap = 0;
		bfSize = 54 + m_nWidth*m_nHeight*3;
	} 

	long byteswritten = 0;
	lcuint32 pixoff = 54 + cmap*4;
	lcuint16 res = 0;
	lcuint8 m1 ='B', m2 ='M';
	file.WriteU8(&m1, 1);       byteswritten++; // B
	file.WriteU8(&m2, 1);       byteswritten++; // M
	file.WriteU32(&bfSize, 1);  byteswritten+=4;// bfSize
	file.WriteU16(&res, 1);     byteswritten+=2;// bfReserved1
	file.WriteU16(&res, 1);     byteswritten+=2;// bfReserved2
	file.WriteU32(&pixoff, 1);  byteswritten+=4;// bfOffBits

	lcuint32 biSize = 40, compress = 0, size = 0;
	lcuint32 width = m_nWidth, height = m_nHeight, pixels = 0;
	lcuint16 planes = 1;
	file.WriteU32(&biSize, 1);   byteswritten+=4;// biSize
	file.WriteU32(&width, 1);    byteswritten+=4;// biWidth
	file.WriteU32(&height, 1);   byteswritten+=4;// biHeight
	file.WriteU16(&planes, 1);   byteswritten+=2;// biPlanes
	file.WriteU16(&bits, 1);     byteswritten+=2;// biBitCount
	file.WriteU32(&compress, 1); byteswritten+=4;// biCompression
	file.WriteU32(&size, 1);     byteswritten+=4;// biSizeImage
	file.WriteU32(&pixels, 1);   byteswritten+=4;// biXPelsPerMeter
	file.WriteU32(&pixels, 1);   byteswritten+=4;// biYPelsPerMeter
	file.WriteU32(&cmap, 1);     byteswritten+=4;// biClrUsed
	file.WriteU32(&cmap, 1);     byteswritten+=4;// biClrImportant

	if (quantize)
	{
		for (int i = 0; i < 256; i++) 
		{
			file.WriteU8(pal[2][i]);
			file.WriteU8(pal[1][i]);
			file.WriteU8(pal[0][i]);
			file.WriteU8(0);	// dummy
		}

		for (int row = 0; row < m_nHeight; row++) 
		{
			int pixbuf = 0;

			for (int col = 0; col < m_nWidth; col++) 
			{
				int offset = (m_nHeight-row-1) * width + col;	// offset into our color-mapped RGB buffer
				unsigned char pval = *(colormappedbuffer + offset);

				pixbuf = (pixbuf << 8) | pval;

				file.WriteU8(pixbuf);
				pixbuf = 0;
				byteswritten++;
			}

			// DWORD align
			while ((byteswritten - pixoff) & 3)
			{
				file.WriteU8(0);
				byteswritten++;
			}
		}

		free(colormappedbuffer);
	}
	else
	{
		unsigned long widthDW = (((m_nWidth*24) + 31) / 32 * 4);
		long row, row_size = m_nWidth*3;
		for (row = 0; row < m_nHeight; row++) 
		{
			unsigned char* buf = m_pData+(m_nHeight-row-1)*row_size;

			// write a row
			for (int col = 0; col < row_size; col += 3)
			{
				file.WriteU8(buf[col+2]);
				file.WriteU8(buf[col+1]);
				file.WriteU8(buf[col]);
			}
			byteswritten += row_size;	

			for (unsigned long count = row_size; count < widthDW; count++)
			{
				file.WriteU8(0);	// dummy
				byteswritten++;
			}
		}
	}

	return true;
}
