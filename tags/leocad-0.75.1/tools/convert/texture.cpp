#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include "texture.h"

void ConvertFile(char* szFilename, FILE* idx, FILE* bin, unsigned long* binoff)
{
	long i, j;
	unsigned short width, height;
	unsigned char* buf = ReadBMP(szFilename, &width, &height);
	unsigned char bt;
	unsigned short sh;
	char* p;
	p = strrchr(szFilename, '.');
	*p = 0;
	p = strrchr(szFilename, '\\');
	strupr(p);
	p++;

	char name[9];
	memset(name, 0, 9);
	strcpy(name, p);

	fwrite(name, 8, 1, idx);
	sh = (unsigned short)width;
	fwrite(&sh, sizeof(sh), 1, idx);
	sh = (unsigned short)height;
	fwrite(&sh, sizeof(sh), 1, idx);

	if (strcmp("SYSFONT", name) == 0)
	{
		bt = 0; // luminance
		fwrite(&bt, 1, 1, idx);

		// Store top->bottom only this one
		for (i = 0; i < height; i++)
		{
			unsigned char* b = buf+(height-i-1)*width*3;

			for (j = 0; j < width; j++)
			{
				if (b[j*3] > 0 || b[j*3+1] > 0 || b[j*3+2] > 0)
					bt = 0;
				else
					bt = 255;
				fwrite(&bt, 1, 1, bin);
			}
		}

		fwrite(binoff, sizeof(*binoff), 1, idx);
		*binoff += width*height;
	}

	if (strcmp("SPACE", name) == 0)
	{
		bt = 2; // RGBA
		fwrite(&bt, 1, 1, idx);
		for (i = 0; i < width*height; i++)
		{
			if (buf[i*3] == 255 && buf[i*3+1] == 0 && buf[i*3+2] == 255)
			{
				buf[i*3] = buf[i*3+1] = buf[i*3+2] = 255;
				bt = 0;
			}
			else
				bt = 255;

			fwrite(&buf[i*3], 3, 1, bin);
			fwrite(&bt, 1, 1, bin);
		}

		fwrite(binoff, sizeof(*binoff), 1, idx);
		*binoff += width*height*4;
	}

	free (buf);
}

static char* filepath = "f:\\ldraw\\textures\\";
static char* filenames[] = { "SYSFONT", "SPACE" };
static int filecount = 2;

int main(int argc, char *argv[])
{
	char tmp[260];
	FILE *bin, *idx;
	unsigned long binoff;
	unsigned short count = 0;

	char strbin[32] = "LeoCAD texture data file\0\0\0\0\0\0\0";
	char stridx[32] = "LeoCAD texture index file\0\0\0\0\0\0";
	unsigned char bt;

	bin = fopen("TEXTURES.BIN", "wb");
	idx = fopen("TEXTURES.IDX", "wb");
	fwrite(strbin, 32, 1, bin);
	binoff = 32;
	fwrite(stridx, 32, 1, idx);
	bt = 1; // version
	fwrite(&bt, 1, 1, idx);
	bt = 3; // last update
	fwrite(&bt, 1, 1, idx);

	for (int i = 0; i < filecount; i++)
	{
		strcpy(tmp, filepath);
		strcat(tmp, filenames[i]);
		strcat(tmp, ".bmp");
		count++;
		ConvertFile(tmp, idx, bin, &binoff);
	}

	fclose(bin);
	fwrite(&binoff, sizeof(binoff), 1, idx);
	fwrite(&count, sizeof(count), 1, idx);
	fclose(idx);

	return 0;
}