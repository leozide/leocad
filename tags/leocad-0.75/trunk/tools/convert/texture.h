
static unsigned char* ReadBMP (char* filename, unsigned short *width, unsigned short *height)
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
	unsigned long m_bytesRead = 0;
	unsigned char* outbuf;
	FILE *fp;
	
	fp = fopen(filename,"rb");
	if (fp == NULL)
		return NULL;
	else
	{
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

		//	I don't do RLE files
		if (compression != 0) // BI_RGB
		{
	    	fclose(fp);
	    	return NULL;
	    }

		if (colors == 0)
			colors = 1 << bmBitsPixel;

		RGBQUAD *colormap = NULL;

		switch (bmBitsPixel)
		{
		case 24:
			break;
			// read pallete 
		case 1:
		case 4:
		case 8:
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
			break;
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
		outbuf = (unsigned char*)malloc(w*h*3);
		long row_size = w * 3;
		long bufsize = (long)w * 3 * (long)h;

		if (outbuf != NULL) 
		{
			*width = w;
			*height = h;
			long row = 0;
			long rowOffset = 0;

			// read rows in reverse order
			for (row = 0; row < bmHeight; row++)
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
							free(outbuf);
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
								free(outbuf);
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
							free(outbuf);
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

		if (colormap)
			delete [] colormap;

		fclose(fp);
    }

//	VerticalFlip(image);
	return outbuf;
}





/*
#define WIDTHBYTES(bits)	(((bits) + 31) / 32 * 4)

static BOOL VertFlipBuf(BYTE* inbuf, UINT widthBytes, UINT height)
{   
	BYTE  *tb1, *tb2;
	ULONG off1 = 0, off2 = 0;

	if (inbuf == NULL)
		return FALSE;

	tb1 = (BYTE*)malloc(widthBytes);
	if (tb1 == NULL) 
		return FALSE;

	tb2 = (BYTE*)malloc(widthBytes);
	if (tb2 == NULL) 
	{
		free(tb1);
		return FALSE;
	}
	
	for (UINT row_cnt = 0; row_cnt < (height+1)/2; row_cnt++) 
	{
		off1 = row_cnt*widthBytes;
		off2 = ((height-1)-row_cnt)*widthBytes;   
		
		memcpy(tb1, inbuf+off1, widthBytes);
		memcpy(tb2, inbuf+off2, widthBytes);	
		memcpy(inbuf+off1, tb2, widthBytes);
		memcpy(inbuf+off2, tb1, widthBytes);
	}	

	free (tb1);
	free (tb2);

	return TRUE;
}        

static BYTE* MakeDwordAlignedBuf(BYTE *dataBuf, UINT widthPix,	UINT height, UINT *uiOutWidthBytes)
{
	if (dataBuf == NULL)
		return NULL;

	UINT uiWidthBytes = WIDTHBYTES(widthPix * 24);
	DWORD dwNewsize = (DWORD)((DWORD)uiWidthBytes * (DWORD)height);
	BYTE *pNew;

	pNew = (BYTE*)new BYTE[dwNewsize];
	if (pNew == NULL) 
		return NULL;
	
	// copy row-by-row
	UINT uiInWidthBytes = widthPix * 3;
	UINT uiCount;
	for (uiCount=0;uiCount < height;uiCount++)
	{
		BYTE* bpInAdd;
		BYTE* bpOutAdd;
		ULONG lInOff;
		ULONG lOutOff;

		lInOff = uiInWidthBytes * uiCount;
		lOutOff = uiWidthBytes * uiCount;

		bpInAdd = dataBuf + lInOff;
		bpOutAdd = pNew + lOutOff;

		memcpy(bpOutAdd,bpInAdd,uiInWidthBytes);
	}

	*uiOutWidthBytes = uiWidthBytes;
	return pNew;
}

static BOOL BGRFromRGB(BYTE *buf, UINT widthPix, UINT height)
{
	if (buf == NULL)
		return FALSE;

	UINT col, row;
	for (row=0;row<height;row++) {
		for (col=0;col<widthPix;col++) 
		{
			LPBYTE pRed, pGrn, pBlu;
			pRed = buf + row * widthPix * 3 + col * 3;
			pGrn = buf + row * widthPix * 3 + col * 3 + 1;
			pBlu = buf + row * widthPix * 3 + col * 3 + 2;

			// swap red and blue
			BYTE tmp;
			tmp = *pRed;
			*pRed = *pBlu;
			*pBlu = tmp;
		}
	}
	return TRUE;
}

static unsigned char* ReadBMP (char* fileName, UINT *width, UINT *height)
{
	BITMAP inBM;
	unsigned char m1,m2;
    unsigned long sizeimage;
    short res1,res2;
    long filesize, pixoff;
    long bmisize, compression;
    long xscale, yscale;
    long colors, impcol;
	FILE *fp;

	unsigned char *outBuf=NULL;
	unsigned long m_bytesRead = 0;
	*width=0;
	*height=0;
	
	fp=fopen(fileName,"rb");
	if (fp==NULL) {
		return NULL;
	} else {
	    long rc;
		rc=fread((unsigned char*)&(m1),1,1,fp); m_bytesRead+=1;
		if (rc==-1) {fclose(fp); return NULL;}

		rc=fread((unsigned char*)&(m2),1,1,fp); m_bytesRead+=1;
		if ((m1!='B') || (m2!='M')) {
			fclose(fp);
			return NULL;
        }
        
		rc=fread((long  *)&(filesize),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((int  *)&(res1),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((int  *)&(res2),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(pixoff),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(bmisize),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(inBM.bmWidth),4,1,fp);	 m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(inBM.bmHeight),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((int  *)&(inBM.bmPlanes),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((int  *)&(inBM.bmBitsPixel),2,1,fp); m_bytesRead+=2;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(compression),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(sizeimage),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(xscale),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(yscale),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(colors),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		rc=fread((long  *)&(impcol),4,1,fp); m_bytesRead+=4;
		if (rc!=1) {fclose(fp); return NULL;}

		////////////////////////////////////////////////////////////////////////////
		//	i don't do RLE files

		if (compression!=BI_RGB) {
	    	fclose(fp);
	    	return NULL;
	    }

		if (colors == 0) {
			colors = 1 << inBM.bmBitsPixel;
		}


		RGBQUAD *colormap = NULL;

		switch (inBM.bmBitsPixel) {
		case 24:
			break;
			// read pallete 
		case 1:
		case 4:
		case 8:
			colormap = new RGBQUAD[colors];
			if (colormap==NULL) {
				fclose(fp);
				return NULL;
			}

			int i;
			for (i=0;i<colors;i++) {
				unsigned char r,g,b, dummy;

				rc=fread((unsigned char*)&(b),1,1,fp);
				m_bytesRead++;
				if (rc!=1) {
					delete [] colormap;
					fclose(fp);
					return NULL;
				}

				rc=fread((unsigned char*)&(g),1,1,fp); 
				m_bytesRead++;
				if (rc!=1) {
					delete [] colormap;
					fclose(fp);
					return NULL;
				}

				rc=fread((unsigned char*)&(r),1,1,fp); 
				m_bytesRead++;
				if (rc!=1) {
					delete [] colormap;
					fclose(fp);
					return NULL;
				}


				rc=fread((unsigned char*)&(dummy),1,1,fp); 
				m_bytesRead++;
				if (rc!=1) {
					delete [] colormap;
					fclose(fp);
					return NULL;
				}

				colormap[i].rgbRed=r;
				colormap[i].rgbGreen=g;
				colormap[i].rgbBlue=b;
			}
			break;
		}


		if ((long)m_bytesRead>pixoff) {
			delete [] colormap;
			fclose(fp);
			return NULL;
		}

		while ((long)m_bytesRead<pixoff) {
			char dummy;
			fread(&dummy,1,1,fp);
			m_bytesRead++;
		}

		int w=inBM.bmWidth;
		int h=inBM.bmHeight;

		// set the output params
		*width=w;
		*height=h;

		long row_size = w * 3;
		long bufsize = (long)w * 3 * (long)h;

		outBuf=(unsigned char*) new unsigned char[bufsize];
		if (outBuf!=NULL) 
		{
			long row=0;
			long rowOffset=0;

			// read rows in reverse order
			for (row=inBM.bmHeight-1;row>=0;row--) {

				// which row are we working on?
				rowOffset=(long unsigned)row*row_size;						      

				if (inBM.bmBitsPixel==24) {

					for (int col=0;col<w;col++) {
						long offset = col * 3;
						char pixel[3];

						if (fread((void  *)(pixel),1,3,fp)==3) {
							// we swap red and blue here
							*(outBuf + rowOffset + offset + 0)=pixel[2];		// r
							*(outBuf + rowOffset + offset + 1)=pixel[1];		// g
							*(outBuf + rowOffset + offset + 2)=pixel[0];		// b
						}

					}

					m_bytesRead+=row_size;
					
					// read DWORD padding
					while ((m_bytesRead-pixoff)&3) {
						char dummy;
						if (fread(&dummy,1,1,fp)!=1) {
							delete [] outBuf;
							fclose(fp);
							return NULL;
						}

						m_bytesRead++;
					}
 
					
				} else {	// 1, 4, or 8 bit image

					////////////////////////////////////////////////////////////////
					// pixels are packed as 1 , 4 or 8 bit vals. need to unpack them

					int bit_count = 0;
					UINT mask = (1 << inBM.bmBitsPixel) - 1;

					unsigned char inbyte=0;

					for (int col=0;col<w;col++) {
						
						int pix=0;

						// if we need another byte
						if (bit_count <= 0) {
							bit_count = 8;
							if (fread(&inbyte,1,1,fp)!=1) {
								delete [] outBuf;
								delete [] colormap;
								fclose(fp);
								return NULL;
							}
							m_bytesRead++;
						}

						// keep track of where we are in the bytes
						bit_count -= inBM.bmBitsPixel;
						pix = ( inbyte >> bit_count) & mask;

						// lookup the color from the colormap - stuff it in our buffer
						// swap red and blue
						*(outBuf + rowOffset + col * 3 + 2) = colormap[pix].rgbBlue;
						*(outBuf + rowOffset + col * 3 + 1) = colormap[pix].rgbGreen;
						*(outBuf + rowOffset + col * 3 + 0) = colormap[pix].rgbRed;
					}

					// read DWORD padding
					while ((m_bytesRead-pixoff)&3) {
						char dummy;
						if (fread(&dummy,1,1,fp)!=1) {
							delete [] outBuf;
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

		if (colormap) {
			delete [] colormap;
		}

		fclose(fp);

    }

	return outBuf;
}
*/