// Image I/O routines
//

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "image.h"
#include "quant.h"
#include "file.h"
extern "C" {
#include <jpeglib.h>
}

/////////////////////////////////////////////////////////////////////////////
// static declarations

static LC_IMAGE* OpenJPG(char* filename);
LC_IMAGE* OpenBMP(char* filename);
LC_IMAGE* OpenPNG(char* filename);
static LC_IMAGE* OpenGIF(File* file);

static bool SaveJPG(char* filename, LC_IMAGE* image, int quality, bool progressive);
bool SaveBMP(char* filename, LC_IMAGE* image, bool quantize);
bool SavePNG(char* filename, LC_IMAGE* image, bool transparent, bool interlaced, unsigned char* background);
static bool SaveGIF(File* file, LC_IMAGE* image, bool transparent, bool interlaced, unsigned char* background);

typedef struct bt_jpeg_error_mgr
{
	struct jpeg_error_mgr pub;	// "public" fields
	jmp_buf setjmp_buffer;		// for return to caller 
} bt_jpeg_error_mgr;

static void bt_jpeg_error_exit (j_common_ptr cinfo)
{
	bt_jpeg_error_mgr* myerr = (bt_jpeg_error_mgr*) cinfo->err;
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message) (cinfo, buffer);
//	MessageBox(NULL, buffer, "JPEG Fatal Error", MB_ICONSTOP);
	longjmp(myerr->setjmp_buffer, 1);
}

// stash a scanline
static void j_putRGBScanline(unsigned char* jpegline, int widthPix, unsigned char* outBuf, int row)
{
	int offset = row * widthPix * 3;
	int count;
	for (count = 0; count < widthPix; count++) 
	{
		unsigned char iRed, iBlu, iGrn;
		unsigned char *oRed, *oBlu, *oGrn;

		iRed = *(jpegline + count * 3 + 0);
		iGrn = *(jpegline + count * 3 + 1);
		iBlu = *(jpegline + count * 3 + 2);

		oRed = outBuf + offset + count * 3 + 0;
		oGrn = outBuf + offset + count * 3 + 1;
		oBlu = outBuf + offset + count * 3 + 2;

		*oRed = iRed;
		*oGrn = iGrn;
		*oBlu = iBlu;
	}
}

// stash a gray scanline
static void j_putGrayScanlineToRGB(unsigned char* jpegline, int widthPix, unsigned char* outBuf, int row)
{
	int offset = row * widthPix * 3;
	int count;
	for (count = 0; count < widthPix; count++) 
	{
		unsigned char iGray;
		unsigned char *oRed, *oBlu, *oGrn;

		// get our grayscale value
		iGray = *(jpegline + count);

		oRed = outBuf + offset + count * 3;
		oGrn = outBuf + offset + count * 3 + 1;
		oBlu = outBuf + offset + count * 3 + 2;

		*oRed = iGray;
		*oGrn = iGray;
		*oBlu = iGray;
	}
}

static LC_IMAGE* ResizeImage(LC_IMAGE* image)
{
	int i, j;
	long shifted_x, shifted_y;
	if (image == NULL)
		return NULL;

	shifted_x = image->width;
	for (i = 0; ((i < 16) && (shifted_x != 0)); i++)
		shifted_x = shifted_x >> 1;
	shifted_x = (i != 0) ? 1 << (i-1) : 1;

	shifted_y = image->height;
	for (i = 0; ((i < 16) && (shifted_y != 0)); i++)
		shifted_y = shifted_y >> 1;
	shifted_y = (i != 0) ? 1 << (i-1) : 1;

	if ((shifted_x == image->width) && (shifted_y == image->height))
		return image;

	LC_IMAGE* newimage = (LC_IMAGE*)malloc(shifted_x*shifted_y*3+sizeof(LC_IMAGE));
	newimage->width = (unsigned short)shifted_x;
	newimage->height = (unsigned short)shifted_y;
	newimage->bits = (unsigned char*)newimage + sizeof(LC_IMAGE);
	memset(newimage->bits, 0, shifted_x*shifted_y*3);

	float accumx, accumy;
	int stx, sty;
	unsigned char *oldbits = (unsigned char*)image->bits,
		*newbits = (unsigned char*)newimage->bits;

	for (j = 0; j < image->height; j++)
	{
		accumy = (float)newimage->height*j/(float)image->height;
		sty = (int)floor(accumy);

		for (i = 0; i < image->width; i++)
		{
			accumx = (float)newimage->width*i/(float)image->width;
			stx = (int)floor(accumx);

			newbits[(stx+sty*newimage->width)*3] = oldbits[(i+j*image->width)*3];
			newbits[(stx+sty*newimage->width)*3+1] = oldbits[(i+j*image->width)*3+1];
			newbits[(stx+sty*newimage->width)*3+2] = oldbits[(i+j*image->width)*3+2];
		}
	}

	free(image);
	return newimage;
}

/////////////////////////////////////////////////////////////////////////////
// functions

// Reads a file from disk
LC_IMAGE* OpenImage(char* filename)
{
	char ext[5];
	if (strlen(filename) != 0)
	{
		char *p = strrchr(filename, '.');
		if (p != NULL)
			strcpy (ext, p+1);
	}
	strlwr(ext);

	if ((strcmp(ext, "jpg") == 0) || (strcmp (ext, "jpeg") == 0))
		return ResizeImage(OpenJPG(filename));
	if (strcmp(ext, "bmp") == 0)
		return ResizeImage(OpenBMP(filename));
	if (strcmp(ext, "png") == 0)
		return ResizeImage(OpenPNG(filename));
	if ((strcmp (ext, "gif") == 0) || (strcmp (ext, "tmp") == 0))
	{
		FileDisk file;
		if (!file.Open(filename, "rb"))
			return NULL;
		LC_IMAGE* image = ResizeImage(OpenGIF(&file));
		file.Close();
		return image;
	}

//	MessageBox (NULL, "Unknown File Format", "Error", MB_ICONSTOP);

	return NULL;
}

LC_IMAGE* OpenImage(File* file, unsigned char format)
{
	if (format != LC_IMAGE_GIF)
		return NULL;
	return OpenGIF(file);
}

bool SaveImage(char* filename, LC_IMAGE* image, LC_IMAGE_OPTS* opts)
{
	char ext[5];
	if (strlen(filename) != 0)
	{
		char *p = strrchr(filename, '.');
		if (p != NULL)
			strcpy(ext, p+1);
	}
	strlwr(ext);

	if ((strcmp (ext, "jpg") == 0) || (strcmp (ext, "jpeg") == 0))
		return SaveJPG(filename, image, opts->quality, opts->interlaced);

	if (strcmp (ext, "gif") == 0)
	{
		FileDisk file;
		if (!file.Open(filename, "wb"))
			return false;

		bool ret = SaveGIF(&file, image, opts->transparent, opts->interlaced, opts->background);
		file.Close();
		return ret;
	}

	if (strcmp (ext, "bmp") == 0)
		return SaveBMP(filename, image, opts->truecolor == false);

	if (strcmp (ext, "png") == 0)
		return SavePNG(filename, image, opts->transparent, opts->interlaced, opts->background);

//	MessageBox (NULL, "Could not save file", "Error", MB_ICONSTOP);

	return false;
}

bool SaveImage(File* file, LC_IMAGE* image, LC_IMAGE_OPTS* opts)
{
	if (opts->format != LC_IMAGE_GIF)
		return false;
	return SaveGIF(file, image, opts->transparent, opts->interlaced, opts->background);
}

/////////////////////////////////////////////////////////////////////////////
// read functions

static LC_IMAGE* OpenJPG(char* filename)
{
	struct jpeg_decompress_struct cinfo;
	struct bt_jpeg_error_mgr jerr;
	FILE* infile = NULL;
	JSAMPARRAY buffer;	// Output row buffer
	int row_stride;		// physical row width in output buffer

	if ((infile = fopen(filename, "rb")) == NULL) 
		return NULL;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = bt_jpeg_error_exit;

	if (setjmp(jerr.setjmp_buffer)) 
	{
		jpeg_destroy_decompress(&cinfo);

		if (infile != NULL)
			fclose(infile);
		return NULL;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	// get our buffer set to hold data
	LC_IMAGE* image = (LC_IMAGE*)malloc(cinfo.output_width*cinfo.output_height*3 + sizeof(LC_IMAGE));

	if (image == NULL)
	{
//		MessageBox(NULL, "Error", "Cannot allocate memory", MB_ICONSTOP);
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return NULL;
	}

	image->width = cinfo.output_width;
	image->height = cinfo.output_height;
	image->bits = (char*)image + sizeof(LC_IMAGE);

	row_stride = cinfo.output_width * cinfo.output_components;
	buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	while (cinfo.output_scanline < cinfo.output_height) 
	{
		jpeg_read_scanlines(&cinfo, buffer, 1);

		if (cinfo.out_color_components == 3)
			j_putRGBScanline(buffer[0], image->width, (unsigned char*)image->bits, cinfo.output_scanline-1);
		else if (cinfo.out_color_components == 1)
			j_putGrayScanlineToRGB(buffer[0], image->width, (unsigned char*)image->bits, cinfo.output_scanline-1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);

	return image;
}

typedef struct
{
	unsigned char colormap[3][256];

	// State for GetCode and LZWReadByte
	char code_buf[256+4];
	int last_byte;		// # of bytes in code_buf
	int last_bit;		// # of bits in code_buf
	int cur_bit;		// next bit index to read
	bool out_of_blocks;	// true if hit terminator data block
	
	int input_code_size;	// codesize given in GIF file
	int clear_code,end_code;// values for Clear and End codes
	
	int code_size;		// current actual code size
	int limit_code;		// 2^code_size
	int max_code;		// first unused code value
	bool first_time;	// flags first call to LZWReadByte
	
	// Private state for LZWReadByte
	int oldcode;		// previous LZW symbol
	int firstcode;		// first byte of oldcode's expansion
	
	// LZW symbol table and expansion stack
	UINT16 FAR *symbol_head;	// => table of prefix symbols
	UINT8  FAR *symbol_tail;	// => table of suffix bytes
	UINT8  FAR *symbol_stack;	// => stack for symbol expansions
	UINT8  FAR *sp;				// stack pointer
	
	// State for interlaced image processing
	bool is_interlaced;		// true if have interlaced image
//	jvirt_sarray_ptr interlaced_image; // full image in interlaced order
	unsigned char* interlaced_image;
	JDIMENSION cur_row_number;	// need to know actual row number
	JDIMENSION pass2_offset;	// # of pixel rows in pass 1
	JDIMENSION pass3_offset;	// # of pixel rows in passes 1&2
	JDIMENSION pass4_offset;	// # of pixel rows in passes 1,2,3

	File* input_file;
	bool first_interlace;
	unsigned char* buffer;//JSAMPARRAY buffer;
	unsigned int width, height;
} gif_source_struct;

typedef gif_source_struct *gif_source_ptr;

// Macros for extracting header data --- note we assume chars may be signed
#define LM_to_uint(a,b)		((((b)&0xFF) << 8) | ((a)&0xFF))
#define BitSet(byte, bit)	((byte) & (bit))
#define INTERLACE		0x40	// mask for bit signifying interlaced image
#define COLORMAPFLAG	0x80	// mask for bit signifying colormap presence

#undef LZW_TABLE_SIZE
#define	MAX_LZW_BITS	12	// maximum LZW code size
#define LZW_TABLE_SIZE	(1<<MAX_LZW_BITS) // # of possible LZW symbols

static int GetDataBlock (gif_source_ptr sinfo, char *buf)
{
	int count = sinfo->input_file->GetChar();
	if (count > 0) 
		sinfo->input_file->Read(buf, count);
	return count;
}

static int GetCode (gif_source_ptr sinfo)
{
	register INT32 accum;
	int offs, ret, count;
	
	while ( (sinfo->cur_bit + sinfo->code_size) > sinfo->last_bit) {
		if (sinfo->out_of_blocks) 
			return sinfo->end_code;	// fake something useful
		sinfo->code_buf[0] = sinfo->code_buf[sinfo->last_byte-2];
		sinfo->code_buf[1] = sinfo->code_buf[sinfo->last_byte-1];
		if ((count = GetDataBlock(sinfo, &sinfo->code_buf[2])) == 0) {
			sinfo->out_of_blocks = true;
			return sinfo->end_code;	// fake something useful
		}
		sinfo->cur_bit = (sinfo->cur_bit - sinfo->last_bit) + 16;
		sinfo->last_byte = 2 + count;
		sinfo->last_bit = sinfo->last_byte * 8;
	}
	
	offs = sinfo->cur_bit >> 3;	// byte containing cur_bit
	accum = sinfo->code_buf[offs+2] & 0xFF;
	accum <<= 8;
	accum |= sinfo->code_buf[offs+1] & 0xFF;
	accum <<= 8;
	accum |= sinfo->code_buf[offs] & 0xFF;
	accum >>= (sinfo->cur_bit & 7);
	ret = ((int) accum) & ((1 << sinfo->code_size) - 1);
	
	sinfo->cur_bit += sinfo->code_size;
	return ret;
}

static int LZWReadByte (gif_source_ptr sinfo)
{
	register int code;	// current working code
	int incode;			// saves actual input code
	
	// First time, just eat the expected Clear code(s) and return next code,
	// which is expected to be a raw byte.
	if (sinfo->first_time)
	{
		sinfo->first_time = false;
		code = sinfo->clear_code;	// enables sharing code with Clear case
	}
	else
	{
		// If any codes are stacked from a previously read symbol, return them
		if (sinfo->sp > sinfo->symbol_stack)
			return (int) *(-- sinfo->sp);
		
		// Time to read a new symbol
		code = GetCode(sinfo);
		
	}
	
	if (code == sinfo->clear_code) 
	{
		sinfo->code_size = sinfo->input_code_size + 1;
		sinfo->limit_code = sinfo->clear_code << 1;	// 2^code_size
		sinfo->max_code = sinfo->clear_code + 2;	// first unused code value
		sinfo->sp = sinfo->symbol_stack;		// init stack to empty
		do
		{
			code = GetCode(sinfo);
		} while (code == sinfo->clear_code);
		
		if (code > sinfo->clear_code) 
			code = 0;	// use something valid
		sinfo->firstcode = sinfo->oldcode = code;
		return code;
	}
	
	if (code == sinfo->end_code) 
	{
		if (!sinfo->out_of_blocks)
		{
			char buf[256];
			while (GetDataBlock(sinfo, buf) > 0)
				;	// skip
			sinfo->out_of_blocks = true;
		}
		return 0;	// fake something usable
	}
	
	incode = code;	// save for a moment
	
	if (code >= sinfo->max_code)
	{	
		// special case for not-yet-defined symbol
		// code == max_code is OK; anything bigger is bad data
		if (code > sinfo->max_code)
			incode = 0;		// prevent creation of loops in symbol table
		// this symbol will be defined as oldcode/firstcode
		*(sinfo->sp++) = (UINT8) sinfo->firstcode;
		code = sinfo->oldcode;
	}
	
	while (code >= sinfo->clear_code)
	{
		*(sinfo->sp++) = sinfo->symbol_tail[code]; // tail is a byte value
		code = sinfo->symbol_head[code]; // head is another LZW symbol
	}
	sinfo->firstcode = code;	// save for possible future use
	
	if ((code = sinfo->max_code) < LZW_TABLE_SIZE)
	{
		sinfo->symbol_head[code] = sinfo->oldcode;
		sinfo->symbol_tail[code] = (UINT8) sinfo->firstcode;
		sinfo->max_code++;
		if ((sinfo->max_code >= sinfo->limit_code) &&
			(sinfo->code_size < MAX_LZW_BITS)) {
			sinfo->code_size++;
			sinfo->limit_code <<= 1;	// keep equal to 2^code_size
		}
	}
	
	sinfo->oldcode = incode;	// save last input symbol for future use
	return sinfo->firstcode;	// return first byte of symbol's expansion
}

static LC_IMAGE* OpenGIF(File* file)
{
	gif_source_ptr source;
	source = (gif_source_ptr)malloc (sizeof(gif_source_struct));
	source->input_file = file;

	char hdrbuf[10];
	unsigned int width, height;
	int colormaplen, aspectRatio;
	int c;

	source->input_file->Read(hdrbuf, 6);
	if ((hdrbuf[0] != 'G' || hdrbuf[1] != 'I' || hdrbuf[2] != 'F') ||
		((hdrbuf[3] != '8' || hdrbuf[4] != '7' || hdrbuf[5] != 'a') &&
		 (hdrbuf[3] != '8' || hdrbuf[4] != '9' || hdrbuf[5] != 'a')))
		return NULL;

	source->input_file->Read(hdrbuf, 7);
	width = LM_to_uint(hdrbuf[0],hdrbuf[1]);
	height = LM_to_uint(hdrbuf[2],hdrbuf[3]);
	source->height = height;
	source->width = width;
	colormaplen = 2 << (hdrbuf[4] & 0x07);
	aspectRatio = hdrbuf[6] & 0xFF;
	
	if (BitSet(hdrbuf[4], COLORMAPFLAG))
		for (int i = 0; i < colormaplen; i++) 
		{
			source->colormap[0][i] = source->input_file->GetChar();
			source->colormap[1][i] = source->input_file->GetChar();
			source->colormap[2][i] = source->input_file->GetChar();
		}
	
	for (;;) 
	{
		c = source->input_file->GetChar();
		
//		if (c == ';')
//			ERREXIT(cinfo, JERR_GIF_IMAGENOTFOUND);
		
		if (c == '!') 
		{
			int extlabel;
			char buf[256];

			extlabel = source->input_file->GetChar();
			while (GetDataBlock(source, buf) > 0)
			    ; // skip
			continue;
		}
		
		if (c != ',') 
			continue;
		
		source->input_file->Read(hdrbuf, 9);
		width = LM_to_uint(hdrbuf[4],hdrbuf[5]);
		height = LM_to_uint(hdrbuf[6],hdrbuf[7]);
		source->is_interlaced = (hdrbuf[8] & INTERLACE) != 0;
		
		if (BitSet(hdrbuf[8], COLORMAPFLAG)) 
		{
			colormaplen = 2 << (hdrbuf[8] & 0x07);
			for (int i = 0; i < colormaplen; i++) 
			{
				source->colormap[0][i] = source->input_file->GetChar();
				source->colormap[1][i] = source->input_file->GetChar();
				source->colormap[2][i] = source->input_file->GetChar();
			}
		}
		
		source->input_code_size = source->input_file->GetChar();
//		if (source->input_code_size < 2 || source->input_code_size >= MAX_LZW_BITS)
//			ERREXIT1(cinfo, JERR_GIF_CODESIZE, source->input_code_size);
		
		break;
	}
	
	source->symbol_head = (UINT16 FAR *) malloc(LZW_TABLE_SIZE * sizeof(UINT16));
	source->symbol_tail = (UINT8 FAR *) malloc (LZW_TABLE_SIZE * sizeof(UINT8));
	source->symbol_stack = (UINT8 FAR *) malloc (LZW_TABLE_SIZE * sizeof(UINT8));
	source->last_byte = 2;	// make safe to "recopy last two bytes"
	source->last_bit = 0;	// nothing in the buffer
	source->cur_bit = 0;	// force buffer load on first call
	source->out_of_blocks = false;
	source->clear_code = 1 << source->input_code_size;
	source->end_code = source->clear_code + 1;
	source->first_time = true;
	source->code_size = source->input_code_size + 1;
	source->limit_code = source->clear_code << 1;// 2^code_size
	source->max_code = source->clear_code + 2;	// first unused code value
	source->sp = source->symbol_stack;	// init stack to empty
	
	if (source->is_interlaced) 
	{
		source->first_interlace = true;
		source->interlaced_image = (unsigned char*)malloc(width*height);
	} 
	else
		source->first_interlace = false;
	
	source->buffer = (unsigned char*)malloc(width*3);
	LC_IMAGE* image = (LC_IMAGE*)malloc(width*height*3 + sizeof(LC_IMAGE));
	image->width = width;
	image->height = height;
	image->bits = (char*)image + sizeof(LC_IMAGE);
	unsigned char* buf = (unsigned char*)image->bits;

	for (unsigned long scanline = 0; scanline < height; scanline++) 
	{
		if (source->is_interlaced) 
		{
			if (source->first_interlace)
			{
				register JSAMPROW sptr;
				register JDIMENSION col;
				JDIMENSION row;
	
				for (row = 0; row < source->height; row++) 
				{
					sptr = &source->interlaced_image[row*source->width];
					for (col = source->width; col > 0; col--) 
						*sptr++ = (JSAMPLE) LZWReadByte(source);
				}
	
				source->first_interlace = false;
				source->cur_row_number = 0;
				source->pass2_offset = (source->height + 7) / 8;
				source->pass3_offset = source->pass2_offset + (source->height + 3) / 8;
				source->pass4_offset = source->pass3_offset + (source->height + 1) / 4;
			}

			register int c;
			register JSAMPROW sptr, ptr;
			register JDIMENSION col;
			JDIMENSION irow;
			
			// Figure out which row of interlaced image is needed, and access it.
			switch ((int) (source->cur_row_number & 7))
			{
			case 0:		// first-pass row
				irow = source->cur_row_number >> 3;
				break;
			case 4:		// second-pass row
				irow = (source->cur_row_number >> 3) + source->pass2_offset;
				break;
			case 2:		// third-pass row
			case 6:
				irow = (source->cur_row_number >> 2) + source->pass3_offset;
				break;
			default:	// fourth-pass row
				irow = (source->cur_row_number >> 1) + source->pass4_offset;
				break;
			}
			sptr = &source->interlaced_image[irow*source->width];
			ptr = source->buffer;
			for (col = source->width; col > 0; col--) 
			{
				c = GETJSAMPLE(*sptr++);
				*ptr++ = source->colormap[0][c];
				*ptr++ = source->colormap[1][c];
				*ptr++ = source->colormap[2][c];
			}
			source->cur_row_number++;	// for next time
		}
		else
		{
			register int c;
			register JSAMPROW ptr;
			register JDIMENSION col;
	
			ptr = source->buffer;
			for (col = source->width; col > 0; col--)
			{
				c = LZWReadByte(source);
				*ptr++ = source->colormap[0][c];
				*ptr++ = source->colormap[1][c];
				*ptr++ = source->colormap[2][c];
			}
		}

		memcpy (buf+(width*scanline*3), source->buffer, 3*width);
	}

	if (source->is_interlaced)
		free(source->interlaced_image);
	free(source->buffer);
	free(source->symbol_head);
	free(source->symbol_tail);
	free(source->symbol_stack);
	free(source);

	return image;
}

/////////////////////////////////////////////////////////////////////////////
// save functions

static bool SaveJPG (char* filename, LC_IMAGE* image, int quality, bool progressive)
{
	struct jpeg_compress_struct cinfo;
	FILE* outfile = NULL;
	int row_stride;	// physical row widthPix in image buffer
	struct bt_jpeg_error_mgr jerr;

	// allocate and initialize JPEG compression object
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = bt_jpeg_error_exit;

	if (setjmp(jerr.setjmp_buffer)) 
	{
		jpeg_destroy_compress(&cinfo);
		if (outfile != NULL)
			fclose(outfile);
		return false;
	}

	jpeg_create_compress(&cinfo);
	if ((outfile = fopen(filename, "wb")) == NULL) 
		return false;

	jpeg_stdio_dest(&cinfo, outfile);

	cinfo.image_width = image->width;
	cinfo.image_height = image->height;
	cinfo.input_components = 3;		
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	if (progressive) 
		jpeg_simple_progression(&cinfo);

	jpeg_start_compress(&cinfo, TRUE);
	row_stride = image->width * 3;

	while (cinfo.next_scanline < cinfo.image_height) 
	{
		unsigned char* outRow = (unsigned char*)image->bits + (cinfo.next_scanline * image->width * 3);
		jpeg_write_scanlines(&cinfo, &outRow, 1);
	}
	
	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);

	return true;
}


#undef LZW_TABLE_SIZE
#define	MAX_LZW_BITS 12
typedef INT16 code_int;
#define LZW_TABLE_SIZE	((code_int) 1 << MAX_LZW_BITS)
#define HSIZE 5003
typedef int hash_int;
#define MAXCODE(n_bits)	(((code_int) 1 << (n_bits)) - 1)
typedef INT32 hash_entry;
#define HASH_ENTRY(prefix,suffix)  ((((hash_entry) (prefix)) << 8) | (suffix))

typedef struct
{
	int n_bits;
	code_int maxcode;
	int init_bits;
	INT32 cur_accum;
	int cur_bits;
	code_int waiting_code;
	bool first_byte;
	code_int ClearCode;
	code_int EOFCode;
	code_int free_code;
	code_int *hash_code;
	hash_entry FAR *hash_value;
	int bytesinpkt;
	char packetbuf[256];
	File* output_file;
	void* buffer;//JSAMPARRAY buffer;
} gif_dest_struct;

typedef gif_dest_struct* gif_dest_ptr;

// Emit a 16-bit word, LSB first
static void put_word(File* output_file, unsigned int w)
{
	output_file->PutChar(w & 0xFF);
	output_file->PutChar((w >> 8) & 0xFF);
}

static void flush_packet(gif_dest_ptr dinfo)
{
	if (dinfo->bytesinpkt > 0) 
	{	
		dinfo->packetbuf[0] = (char) dinfo->bytesinpkt++;
		dinfo->output_file->Write(dinfo->packetbuf, dinfo->bytesinpkt);
		dinfo->bytesinpkt = 0;
	}
}

static void output(gif_dest_ptr dinfo, code_int code)
{
	dinfo->cur_accum |= ((INT32) code) << dinfo->cur_bits;
	dinfo->cur_bits += dinfo->n_bits;
	
	while (dinfo->cur_bits >= 8)
	{
	  (dinfo)->packetbuf[++(dinfo)->bytesinpkt] = (char) (dinfo->cur_accum & 0xFF);
	  if ((dinfo)->bytesinpkt >= 255)
	    flush_packet(dinfo);

		dinfo->cur_accum >>= 8;
		dinfo->cur_bits -= 8;
	}
	
	if (dinfo->free_code > dinfo->maxcode) {
		dinfo->n_bits++;
		if (dinfo->n_bits == MAX_LZW_BITS)
			dinfo->maxcode = LZW_TABLE_SIZE;
		else
			dinfo->maxcode = MAXCODE(dinfo->n_bits);
	}
}

// Accept and compress one 8-bit byte
static void compress_byte (gif_dest_ptr dinfo, int c)
{
	register hash_int i;
	register hash_int disp;
	register hash_entry probe_value;

	if (dinfo->first_byte) {	
		dinfo->waiting_code = c;
		dinfo->first_byte = FALSE;
		return;
	}
	
	i = ((hash_int) c << (MAX_LZW_BITS-8)) + dinfo->waiting_code;
	if (i >= HSIZE)
		i -= HSIZE;
	
	probe_value = HASH_ENTRY(dinfo->waiting_code, c);
	
	if (dinfo->hash_code[i] != 0) { 
		if (dinfo->hash_value[i] == probe_value) {
			dinfo->waiting_code = dinfo->hash_code[i];
			return;
		}
		if (i == 0)
			disp = 1;
		else
			disp = HSIZE - i;
		for (;;) {
			i -= disp;
			if (i < 0)
				i += HSIZE;
			if (dinfo->hash_code[i] == 0)
				break;
			if (dinfo->hash_value[i] == probe_value) {
				dinfo->waiting_code = dinfo->hash_code[i];
				return;
			}
		}
	}

	output(dinfo, dinfo->waiting_code);
	if (dinfo->free_code < LZW_TABLE_SIZE)
	{
		dinfo->hash_code[i] = dinfo->free_code++;
		dinfo->hash_value[i] = probe_value;
	}
	else
	{
		memset(dinfo->hash_code, 0, HSIZE * sizeof(code_int));
		dinfo->free_code = dinfo->ClearCode + 2;
		output(dinfo, dinfo->ClearCode);
		dinfo->n_bits = dinfo->init_bits;
		dinfo->maxcode = MAXCODE(dinfo->n_bits);
	}
	dinfo->waiting_code = c;
}

static bool SaveGIF(File* file, LC_IMAGE* image, bool transparent, bool interlaced, unsigned char* background)
{
	int InitCodeSize, FlagByte;
	int i;

	unsigned char pal[3][256];
	unsigned char* colormappedbuffer = (unsigned char*)malloc(image->width*image->height);
	dl1quant((unsigned char*)image->bits, colormappedbuffer, image->width, image->height, 256, true, pal);

	gif_dest_ptr dinfo;
	dinfo = (gif_dest_ptr) malloc (sizeof(gif_dest_struct));
	dinfo->output_file = file;
	dinfo->buffer = malloc(image->width*sizeof(JDIMENSION));
	dinfo->hash_code = (code_int*) malloc(HSIZE * sizeof(code_int));
	dinfo->hash_value = (hash_entry FAR*)malloc(HSIZE*sizeof(hash_entry));

	InitCodeSize = 8;
	// Write the GIF header.
	file->PutChar('G');
	file->PutChar('I');
	file->PutChar('F');
	file->PutChar('8');
	file->PutChar(transparent ? '9' : '7');
	file->PutChar('a');
	// Write the Logical Screen Descriptor
	put_word(file, (unsigned int)image->width);
	put_word(file, (unsigned int)image->height);
	FlagByte = 0x80;
	FlagByte |= (7) << 4; // color resolution
	FlagByte |= (7);	// size of global color table
	file->PutChar(FlagByte);
	file->PutChar(0); // Background color index
	file->PutChar(0); // Reserved (aspect ratio in GIF89)
	// Write the Global Color Map
	for (i = 0; i < 256; i++) 
	{
		file->PutChar(pal[0][i]);
		file->PutChar(pal[1][i]);
		file->PutChar(pal[2][i]);
	}

	// Write out extension for transparent colour index, if necessary.
	if (transparent) 
	{
		unsigned char index = 0;

		for (i = 0; i < 256; i++) 
			if (background[0] == pal[0][i] &&
				background[1] == pal[1][i] &&
				background[2] == pal[2][i])
			{
				index = i;
				break;
			}

		file->PutChar('!');
		file->PutChar(0xf9);
		file->PutChar(4);
		file->PutChar(1);
		file->PutChar(0);
		file->PutChar(0);
		file->PutChar(index);
		file->PutChar(0);
	}

	// Write image separator and Image Descriptor
	file->PutChar(',');
	put_word(file, 0);
	put_word(file, 0);
	put_word(file, (unsigned int)image->width); 
	put_word(file, (unsigned int)image->height);
	// flag byte: interlaced
	if (interlaced)
		file->PutChar(0x40);
	else
		file->PutChar(0x00);
	file->PutChar(InitCodeSize);// Write Initial Code Size byte
	
	// Initialize for LZW compression of image data
	dinfo->n_bits = dinfo->init_bits = InitCodeSize+1;
	dinfo->maxcode = MAXCODE(dinfo->n_bits);
	dinfo->ClearCode = ((code_int) 1 << (InitCodeSize));
	dinfo->EOFCode = dinfo->ClearCode + 1;
	dinfo->free_code = dinfo->ClearCode + 2;
	dinfo->first_byte = TRUE;	
	dinfo->bytesinpkt = 0;
	dinfo->cur_accum = 0;
	dinfo->cur_bits = 0;
	memset(dinfo->hash_code, 0, HSIZE * sizeof(code_int));
	output(dinfo, dinfo->ClearCode);

	int scanline = 0;
	int pass = 0;
	while (scanline < image->height)
	{
		memcpy(dinfo->buffer, colormappedbuffer+(scanline*image->width), image->width);

		register JSAMPROW ptr;
		register JDIMENSION col;

		ptr = (unsigned char*)dinfo->buffer;
		for (col = image->width; col > 0; col--) 
			compress_byte(dinfo, GETJSAMPLE(*ptr++));

		if (interlaced)
		{
			switch (pass)
			{
				case 0:
				{
					scanline += 8;
					if (scanline >= image->height)
					{
						pass++;
						scanline = 4;
					}
				} break;

				case 1:
				{
					scanline += 8;
					if (scanline >= image->height)
					{
						pass++;
						scanline = 2;
					}
				} break;

				case 2:
				{
					scanline += 4;
					if (scanline >= image->height)
					{
						pass++;
						scanline = 1;
					}
				} break;

				case 3:
				{
					scanline += 2;
				} break;
			}
		}
		else
			scanline++;
	}

	// Finish up at the end of the file.
	if (!dinfo->first_byte)
		output(dinfo, dinfo->waiting_code);
	output(dinfo, dinfo->EOFCode);
	if (dinfo->cur_bits > 0) 
	{
	(dinfo)->packetbuf[++(dinfo)->bytesinpkt] = (char) (dinfo->cur_accum & 0xFF);
	    if ((dinfo)->bytesinpkt >= 255)
	      flush_packet(dinfo);
	}

	flush_packet(dinfo);
	file->PutChar(0);
	file->PutChar(';');
	file->Flush();

	free(dinfo->buffer);
	free(dinfo->hash_code);
	free(dinfo->hash_value);
	free(dinfo);
	free(colormappedbuffer);
	return true;
}

#ifdef LC_WINDOWS
//#include <windows.h>
//#include <vfw.h>

#define AVIIF_KEYFRAME	0x00000010L // this frame is a key frame.

void SaveVideo(char* filename, LC_IMAGE** images, int count, float fps)
{
/*
	AVISTREAMINFO strhdr;
	PAVIFILE pfile = NULL;
	PAVISTREAM ps = NULL, psCompressed = NULL;
	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = { &opts };
	_fmemset(&opts, 0, sizeof(opts));

	// first let's make sure we are running on 1.1
	WORD wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a)
	{
		AfxMessageBox("Video for Windows 1.1 or later required", MB_OK|MB_ICONSTOP);
		return;
	}

	AVIFileInit();

	if (AVIFileOpen(&pfile, fn, OF_WRITE | OF_CREATE, NULL) == AVIERR_OK)
	{
		// Fill in the header for the video stream.
		_fmemset(&strhdr, 0, sizeof(strhdr));
		strhdr.fccType = streamtypeVIDEO;
		strhdr.fccHandler = 0;
		strhdr.dwScale = 1;
/////////////// set FPS
		strhdr.dwRate = 15;	 // 15 fps
		strhdr.dwSuggestedBufferSize  = plpbi[0]->biSizeImage;
		SetRect(&strhdr.rcFrame, 0, 0, (int) plpbi[0]->biWidth, (int) plpbi[0]->biHeight);

		// And create the stream.
		if (AVIFileCreateStream(pfile, &ps, &strhdr) == AVIERR_OK)
		if (AVISaveOptions(AfxGetMainWnd()->m_hWnd, 0, 1, &ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
		if (AVIMakeCompressedStream(&psCompressed, ps, &opts, NULL) == AVIERR_OK)
		if (AVIStreamSetFormat(psCompressed, 0, plpbi[0], plpbi[0]->biSize + plpbi[0]->biClrUsed * sizeof(RGBQUAD)) == AVIERR_OK)
		{
			float fPause = (float)AfxGetApp()->GetProfileInt("Default", "AVI Pause", 100)/100;
			int time = (int)(fPause * 15);
///////////// set FPS
			time = 1;
		
			for (int i = 0; i < nCount; i++) 
			{
				if (AVIStreamWrite(psCompressed, i * time, 1, 
					(LPBYTE) plpbi[i] +	plpbi[i]->biSize + plpbi[i]->biClrUsed * sizeof(RGBQUAD),
					plpbi[i]->biSizeImage, i%5 ? 0 : AVIIF_KEYFRAME, NULL, NULL) != AVIERR_OK)
					break;
			}
		}
	}
	
	// Now close the file
	if (ps) AVIStreamClose(ps);
	if (psCompressed) AVIStreamClose(psCompressed);
	if (pfile) AVIFileClose(pfile);
	AVIFileExit();
*/
}
#else
void SaveVideo(char* filename, LC_IMAGE** images, int count, float fps)
{
  //	SystemDoMessageBox("Format not supported under this platform.", LC_MB_OK|LC_MB_ERROR);
}
#endif
