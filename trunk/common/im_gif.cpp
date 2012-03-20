#include "lc_global.h"
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "quant.h"
#include "file.h"
#include "config.h"

// =============================================================================

typedef struct
{
  unsigned char colormap[3][256];

  // State for GetCode and LZWReadByte
  char code_buf[256+4];
  int last_byte;	// # of bytes in code_buf
  int last_bit;		// # of bits in code_buf
  int cur_bit;		// next bit index to read
  bool out_of_blocks;	// true if hit terminator data block

  int input_code_size;	// codesize given in GIF file
  int clear_code,end_code;// values for Clear and End codes

  int code_size;	// current actual code size
  int limit_code;	// 2^code_size
  int max_code;		// first unused code value
  bool first_time;	// flags first call to LZWReadByte

  // Private state for LZWReadByte
  int oldcode;		// previous LZW symbol
  int firstcode;	// first byte of oldcode's expansion

  // LZW symbol table and expansion stack
  lcuint16 *symbol_head;	// => table of prefix symbols
  lcuint8  *symbol_tail;	// => table of suffix bytes
  lcuint8  *symbol_stack;	// => stack for symbol expansions
  lcuint8  *sp;				// stack pointer

  // State for interlaced image processing
  bool is_interlaced;		// true if have interlaced image
//	jvirt_sarray_ptr interlaced_image; // full image in interlaced order
  unsigned char* interlaced_image;
  lcuint32 cur_row_number;	// need to know actual row number
  lcuint32 pass2_offset;	// # of pixel rows in pass 1
  lcuint32 pass3_offset;	// # of pixel rows in passes 1&2
  lcuint32 pass4_offset;	// # of pixel rows in passes 1,2,3

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
  register lcint32 accum;
  int offs, ret, count;

  while ((sinfo->cur_bit + sinfo->code_size) > sinfo->last_bit)
  {
    if (sinfo->out_of_blocks) 
      return sinfo->end_code;	// fake something useful
    sinfo->code_buf[0] = sinfo->code_buf[sinfo->last_byte-2];
    sinfo->code_buf[1] = sinfo->code_buf[sinfo->last_byte-1];
    if ((count = GetDataBlock(sinfo, &sinfo->code_buf[2])) == 0)
    {
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
    *(sinfo->sp++) = (lcuint8) sinfo->firstcode;
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
    sinfo->symbol_tail[code] = (lcuint8) sinfo->firstcode;
    sinfo->max_code++;
    if ((sinfo->max_code >= sinfo->limit_code) &&
	(sinfo->code_size < MAX_LZW_BITS))
    {
      sinfo->code_size++;
      sinfo->limit_code <<= 1;	// keep equal to 2^code_size
    }
  }

  sinfo->oldcode = incode;	// save last input symbol for future use
  return sinfo->firstcode;	// return first byte of symbol's expansion
}

bool Image::LoadGIF (File& file)
{
  gif_source_ptr source;
  source = (gif_source_ptr)malloc (sizeof(gif_source_struct));
  source->input_file = &file;

  char hdrbuf[10];
  unsigned int width, height;
  int colormaplen, aspectRatio;
  int c;

  FreeData ();

  source->input_file->Read(hdrbuf, 6);
  if ((hdrbuf[0] != 'G' || hdrbuf[1] != 'I' || hdrbuf[2] != 'F') ||
      ((hdrbuf[3] != '8' || hdrbuf[4] != '7' || hdrbuf[5] != 'a') &&
       (hdrbuf[3] != '8' || hdrbuf[4] != '9' || hdrbuf[5] != 'a')))
    return false;

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

//    if (c == ';')
//      ERREXIT(cinfo, JERR_GIF_IMAGENOTFOUND);

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
//    if (source->input_code_size < 2 || source->input_code_size >= MAX_LZW_BITS)
//      ERREXIT1(cinfo, JERR_GIF_CODESIZE, source->input_code_size);

    break;
  }

  source->symbol_head = (lcuint16*) malloc(LZW_TABLE_SIZE * sizeof(lcuint16));
  source->symbol_tail = (lcuint8*) malloc (LZW_TABLE_SIZE * sizeof(lcuint8));
  source->symbol_stack = (lcuint8*) malloc (LZW_TABLE_SIZE * sizeof(lcuint8));
  source->last_byte = 2; // make safe to "recopy last two bytes"
  source->last_bit = 0;	 // nothing in the buffer
  source->cur_bit = 0;	 // force buffer load on first call
  source->out_of_blocks = false;
  source->clear_code = 1 << source->input_code_size;
  source->end_code = source->clear_code + 1;
  source->first_time = true;
  source->code_size = source->input_code_size + 1;
  source->limit_code = source->clear_code << 1; // 2^code_size
  source->max_code = source->clear_code + 2;	// first unused code value
  source->sp = source->symbol_stack;	        // init stack to empty

  if (source->is_interlaced) 
  {
    source->first_interlace = true;
    source->interlaced_image = (unsigned char*)malloc(width*height);
  } 
  else
    source->first_interlace = false;

  source->buffer = (unsigned char*)malloc(width*3);
  m_pData = (unsigned char*)malloc(width*height*3);
  m_nWidth = width;
  m_nHeight = height;
  m_bAlpha = false; // FIXME: create the alpha channel for transparent files
  unsigned char* buf = m_pData;

  for (unsigned long scanline = 0; scanline < height; scanline++) 
  {
    if (source->is_interlaced) 
    {
      if (source->first_interlace)
      {
        register lcuint8 *sptr;
        register lcuint32 col;
        lcuint32 row;

        for (row = 0; row < source->height; row++) 
        {
          sptr = &source->interlaced_image[row*source->width];
          for (col = source->width; col > 0; col--) 
            *sptr++ = (lcuint8) LZWReadByte(source);
        }

        source->first_interlace = false;
        source->cur_row_number = 0;
        source->pass2_offset = (source->height + 7) / 8;
        source->pass3_offset = source->pass2_offset + (source->height + 3) / 8;
        source->pass4_offset = source->pass3_offset + (source->height + 1) / 4;
      }

      register int c;
      register lcuint8 *sptr, *ptr;
      register lcuint32 col;
      lcuint32 irow;

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
        c = *sptr++;
        *ptr++ = source->colormap[0][c];
        *ptr++ = source->colormap[1][c];
        *ptr++ = source->colormap[2][c];
      }
      source->cur_row_number++;	// for next time
    }
    else
    {
      register int c;
      register lcuint8 *ptr;
      register lcuint32 col;

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

  return true;
}

// =============================================================================

#undef LZW_TABLE_SIZE
#define	MAX_LZW_BITS 12
typedef lcint16 code_int;
#define LZW_TABLE_SIZE	((code_int) 1 << MAX_LZW_BITS)
#define HSIZE 5003
typedef int hash_int;
#define MAXCODE(n_bits)	(((code_int) 1 << (n_bits)) - 1)
typedef lcint32 hash_entry;
#define HASH_ENTRY(prefix,suffix)  ((((hash_entry) (prefix)) << 8) | (suffix))

typedef struct
{
  int n_bits;
  code_int maxcode;
  int init_bits;
  lcint32 cur_accum;
  int cur_bits;
  code_int waiting_code;
  bool first_byte;
  code_int ClearCode;
  code_int EOFCode;
  code_int free_code;
  code_int *hash_code;
  hash_entry *hash_value;
  int bytesinpkt;
  char packetbuf[256];
  File* output_file;
  void* buffer;//JSAMPARRAY buffer;
} gif_dest_struct;

typedef gif_dest_struct* gif_dest_ptr;

// Emit a 16-bit word, LSB first
static void put_word(File& output_file, unsigned int w)
{
  output_file.PutChar(w & 0xFF);
  output_file.PutChar((w >> 8) & 0xFF);
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
  dinfo->cur_accum |= ((lcint32) code) << dinfo->cur_bits;
  dinfo->cur_bits += dinfo->n_bits;

  while (dinfo->cur_bits >= 8)
  {
    (dinfo)->packetbuf[++(dinfo)->bytesinpkt] = (char) (dinfo->cur_accum & 0xFF);
    if ((dinfo)->bytesinpkt >= 255)
      flush_packet(dinfo);

    dinfo->cur_accum >>= 8;
    dinfo->cur_bits -= 8;
  }

  if (dinfo->free_code > dinfo->maxcode)
  {
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

  if (dinfo->first_byte)
  {
    dinfo->waiting_code = c;
    dinfo->first_byte = false;
    return;
  }

  i = ((hash_int) c << (MAX_LZW_BITS-8)) + dinfo->waiting_code;
  if (i >= HSIZE)
    i -= HSIZE;

  probe_value = HASH_ENTRY(dinfo->waiting_code, c);

  if (dinfo->hash_code[i] != 0)
  {
    if (dinfo->hash_value[i] == probe_value)
    {
      dinfo->waiting_code = dinfo->hash_code[i];
      return;
    }
    if (i == 0)
      disp = 1;
    else
      disp = HSIZE - i;
    for (;;)
    {
      i -= disp;
      if (i < 0)
	i += HSIZE;
      if (dinfo->hash_code[i] == 0)
	break;
      if (dinfo->hash_value[i] == probe_value)
      {
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

bool Image::SaveGIF (File& file, bool transparent, bool interlaced, unsigned char* background) const
{
  int InitCodeSize, FlagByte, i;
  unsigned char pal[3][256];
  unsigned char* colormappedbuffer = (unsigned char*)malloc (m_nWidth*m_nHeight);
  dl1quant (m_pData, colormappedbuffer, m_nWidth, m_nHeight, 256, true, pal);

  gif_dest_ptr dinfo;
  dinfo = (gif_dest_ptr) malloc (sizeof(gif_dest_struct));
  dinfo->output_file = &file;
  dinfo->buffer = malloc(m_nWidth*sizeof(lcuint32));
  dinfo->hash_code = (code_int*) malloc(HSIZE * sizeof(code_int));
  dinfo->hash_value = (hash_entry*)malloc(HSIZE*sizeof(hash_entry));

  InitCodeSize = 8;
  // Write the GIF header.
  file.PutChar('G');
  file.PutChar('I');
  file.PutChar('F');
  file.PutChar('8');
  file.PutChar(transparent ? '9' : '7');
  file.PutChar('a');
  // Write the Logical Screen Descriptor
  put_word(file, (unsigned int)m_nWidth);
  put_word(file, (unsigned int)m_nHeight);
  FlagByte = 0x80;
  FlagByte |= (7) << 4; // color resolution
  FlagByte |= (7);	// size of global color table
  file.PutChar(FlagByte);
  file.PutChar(0); // Background color index
  file.PutChar(0); // Reserved (aspect ratio in GIF89)
  // Write the Global Color Map
  for (i = 0; i < 256; i++) 
  {
    file.PutChar(pal[0][i]);
    file.PutChar(pal[1][i]);
    file.PutChar(pal[2][i]);
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

    file.PutChar('!');
    file.PutChar(0xf9);
    file.PutChar(4);
    file.PutChar(1);
    file.PutChar(0);
    file.PutChar(0);
    file.PutChar(index);
    file.PutChar(0);
  }

  // Write image separator and Image Descriptor
  file.PutChar(',');
  put_word(file, 0);
  put_word(file, 0);
  put_word(file, (unsigned int)m_nWidth); 
  put_word(file, (unsigned int)m_nHeight);
  // flag byte: interlaced
  if (interlaced)
    file.PutChar(0x40);
  else
    file.PutChar(0x00);
  file.PutChar(InitCodeSize);// Write Initial Code Size byte

  // Initialize for LZW compression of image data
  dinfo->n_bits = dinfo->init_bits = InitCodeSize+1;
  dinfo->maxcode = MAXCODE(dinfo->n_bits);
  dinfo->ClearCode = ((code_int) 1 << (InitCodeSize));
  dinfo->EOFCode = dinfo->ClearCode + 1;
  dinfo->free_code = dinfo->ClearCode + 2;
  dinfo->first_byte = true;	
  dinfo->bytesinpkt = 0;
  dinfo->cur_accum = 0;
  dinfo->cur_bits = 0;
  memset(dinfo->hash_code, 0, HSIZE * sizeof(code_int));
  output(dinfo, dinfo->ClearCode);

  int scanline = 0;
  int pass = 0;
  while (scanline < m_nHeight)
  {
    memcpy(dinfo->buffer, colormappedbuffer+(scanline*m_nWidth), m_nWidth);

    register lcuint8 *ptr;
    register lcuint32 col;

    ptr = (unsigned char*)dinfo->buffer;
    for (col = m_nWidth; col > 0; col--) 
      compress_byte(dinfo, *ptr++);

    if (interlaced)
    {
      switch (pass)
      {
        case 0:
	{
	  scanline += 8;
	  if (scanline >= m_nHeight)
	  {
	    pass++;
	    scanline = 4;
	  }
	} break;

        case 1:
	{
	  scanline += 8;
	  if (scanline >= m_nHeight)
	  {
	    pass++;
	    scanline = 2;
	  }
	} break;

        case 2:
	{
	  scanline += 4;
	  if (scanline >= m_nHeight)
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
  file.PutChar(0);
  file.PutChar(';');
  file.Flush();

  free(dinfo->buffer);
  free(dinfo->hash_code);
  free(dinfo->hash_value);
  free(dinfo);
  free(colormappedbuffer);
  return true;
}
