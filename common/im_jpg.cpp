#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "image.h"
extern "C" {
#include <jpeglib.h>
}

typedef struct bt_jpeg_error_mgr
{
  struct jpeg_error_mgr pub;  // "public" fields
  jmp_buf setjmp_buffer;      // for return to caller 
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

// =============================================================================

LC_IMAGE* OpenJPG (char* filename)
{
  struct jpeg_decompress_struct cinfo;
  struct bt_jpeg_error_mgr jerr;
  FILE* infile = NULL;
  JSAMPARRAY buffer;	// Output row buffer
  int row_stride;	// physical row width in output buffer

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
//    MessageBox(NULL, "Error", "Cannot allocate memory", MB_ICONSTOP);
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

// =============================================================================

bool SaveJPG (char* filename, LC_IMAGE* image, int quality, bool progressive)
{
  struct jpeg_compress_struct cinfo;
  struct bt_jpeg_error_mgr jerr;
  int row_stride;	// physical row widthPix in image buffer
  FILE* outfile;

  // allocate and initialize JPEG compression object
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = bt_jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer)) 
  {
    //    jpeg_destroy_compress(&cinfo);
    //    if (outfile != NULL)
    //      fclose(outfile);
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
