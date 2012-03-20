#include "lc_global.h"
#include <setjmp.h>
#include <stdlib.h>
#include "config.h"
#include "image.h"
#include "file.h"

#ifdef LC_HAVE_JPEGLIB

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
// JPEG data source

// Expanded data source object for input using the File class
typedef struct
{
  struct jpeg_source_mgr pub;	// public fields

  File * infile;		// source stream
  JOCTET * buffer;		// start of buffer
  boolean start_of_file;	// have we gotten any data yet?
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096	// choose an efficiently fread'able size

// Initialize source --- called by jpeg_read_header
// before any data is actually read.
static void init_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  // We reset the empty-input-file flag for each image,
  // but we don't clear the input buffer.
  // This is correct behavior for reading a series of images from one source.
  src->start_of_file = TRUE;
}

// Fill the input buffer --- called whenever buffer is emptied.
static boolean fill_input_buffer (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
  size_t nbytes;

  nbytes = src->infile->Read (src->buffer, INPUT_BUF_SIZE);

  if (nbytes <= 0)
  {
//    if (src->start_of_file)	// Treat empty input file as fatal error
//      ERREXIT(cinfo, JERR_INPUT_EMPTY);
//    WARNMS(cinfo, JWRN_JPEG_EOF);

    // Insert a fake EOI marker
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }

  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;

  return TRUE;
}

// Skip data --- used to skip over a potentially large amount of
// uninteresting data (such as an APPn marker).

static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  // Just a dumb implementation for now.  Could use Seek() except
  // it doesn't work on pipes.  Not clear that being smart is worth
  // any trouble anyway --- large skips are infrequent.
  if (num_bytes > 0)
  {
    while (num_bytes > (long) src->pub.bytes_in_buffer)
    {
      num_bytes -= (long) src->pub.bytes_in_buffer;
      (void) fill_input_buffer(cinfo);
      // note we assume that fill_input_buffer will never return FALSE,
      // so suspension need not be handled.
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}

// Terminate source --- called by jpeg_finish_decompress
// after all data has been read.  Often a no-op.
static void term_source (j_decompress_ptr cinfo)
{
  // no work necessary here
}

// Prepare for input from a File object.
static void jpeg_file_src (j_decompress_ptr cinfo, File& infile)
{
  my_src_ptr src;

  // The source object and input buffer are made permanent so that a series
  // of JPEG images can be read from the same file by calling jpeg_stdio_src
  // only before the first one.  (If we discarded the buffer at the end of
  // one image, we'd likely lose the start of the next one.)
  // This makes it unsafe to use this manager and a different source
  // manager serially with the same JPEG object.  Caveat programmer.
  if (cinfo->src == NULL)
  {
    // first time for this JPEG object?
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof (my_source_mgr));
    src = (my_src_ptr) cinfo->src;
    src->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  INPUT_BUF_SIZE * sizeof(JOCTET));
  }

  src = (my_src_ptr) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; // use default method
  src->pub.term_source = term_source;
  src->infile = &infile;
  src->pub.bytes_in_buffer = 0; // forces fill_input_buffer on first read
  src->pub.next_input_byte = NULL; // until buffer loaded
}

// =============================================================================

bool Image::LoadJPG (File& file)
{
  struct jpeg_decompress_struct cinfo;
  struct bt_jpeg_error_mgr jerr;
  JSAMPARRAY buffer;	// Output row buffer
  int row_stride;	// physical row width in output buffer

  FreeData ();

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = bt_jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer)) 
  {
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_file_src(&cinfo, file);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  // get our buffer set to hold data
  m_pData = (unsigned char*)malloc(cinfo.output_width*cinfo.output_height*3);

  if (m_pData == NULL)
  {
//    MessageBox(NULL, "Error", "Cannot allocate memory", MB_ICONSTOP);
    jpeg_destroy_decompress(&cinfo);
    return false;
  }

  m_nWidth = cinfo.output_width;
  m_nHeight = cinfo.output_height;
  m_bAlpha = false;

  row_stride = cinfo.output_width * cinfo.output_components;
  buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  while (cinfo.output_scanline < cinfo.output_height) 
  {
    jpeg_read_scanlines(&cinfo, buffer, 1);

    if (cinfo.out_color_components == 3)
      j_putRGBScanline(buffer[0], m_nWidth, m_pData, cinfo.output_scanline-1);
    else if (cinfo.out_color_components == 1)
      j_putGrayScanlineToRGB(buffer[0], m_nWidth, m_pData, cinfo.output_scanline-1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return true;
}

// =============================================================================
// JPEG data destination

// Expanded data destination object for output using the File class
typedef struct
{
  struct jpeg_destination_mgr pub; // public fields

  File * outfile;   // target stream
  JOCTET * buffer;  // start of buffer
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

#define OUTPUT_BUF_SIZE  4096	// choose an efficiently fwrite'able size

// Initialize destination --- called by jpeg_start_compress
// before any data is actually written.
static void init_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  /* Allocate the output buffer --- it will be released when done with image */
  dest->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * sizeof(JOCTET));

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

// Empty the output buffer --- called whenever buffer fills up.
//
// In typical applications, this should write the entire output buffer
// (ignoring the current state of next_output_byte & free_in_buffer),
// reset the pointer & count to the start of the buffer, and return TRUE
// indicating that the buffer has been dumped.
static boolean empty_output_buffer (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  dest->outfile->Write (dest->buffer, OUTPUT_BUF_SIZE);
//  if (dest->outfile.Write (dest->buffer, OUTPUT_BUF_SIZE) != (size_t) OUTPUT_BUF_SIZE)
//    ERREXIT(cinfo, JERR_FILE_WRITE);

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

  return TRUE;
}

// Terminate destination --- called by jpeg_finish_compress
// after all data has been written.  Usually needs to flush buffer.
static void term_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

  // Write any data remaining in the buffer
  if (datacount > 0)
  {
    dest->outfile->Write (dest->buffer, datacount);
//    if (dest->outfile.Write (dest->buffer, datacount) != datacount)
//      ERREXIT(cinfo, JERR_FILE_WRITE);
  }
  dest->outfile->Flush ();

  // Make sure we wrote the output file OK
//  if (ferror(dest->outfile))
//    ERREXIT(cinfo, JERR_FILE_WRITE);
}

// Prepare for output to a File object.
static void jpeg_file_dest (j_compress_ptr cinfo, File& outfile)
{
  my_dest_ptr dest;

  // The destination object is made permanent so that multiple JPEG images
  // can be written to the same file without re-executing jpeg_stdio_dest.
  // This makes it dangerous to use this manager and a different destination
  // manager serially with the same JPEG object, because their private object
  // sizes may be different.  Caveat programmer.
  if (cinfo->dest == NULL)
  {
    // first time for this JPEG object?
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof (my_destination_mgr));
  }

  dest = (my_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outfile = &outfile;
}

// =============================================================================

bool Image::SaveJPG (File& file, int quality, bool progressive) const
{
  struct jpeg_compress_struct cinfo;
  struct bt_jpeg_error_mgr jerr;
  int row_stride;	// physical row widthPix in image buffer

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

  jpeg_file_dest(&cinfo, file);

  cinfo.image_width = m_nWidth;
  cinfo.image_height = m_nHeight;
  cinfo.input_components = 3;		
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);

  if (progressive) 
    jpeg_simple_progression(&cinfo);

  jpeg_start_compress (&cinfo, TRUE);
  row_stride = m_nWidth * 3;

  while (cinfo.next_scanline < cinfo.image_height) 
  {
    unsigned char* outRow = m_pData + (cinfo.next_scanline * m_nWidth * 3);
    jpeg_write_scanlines(&cinfo, &outRow, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  return true;
}

#endif // LC_HAVE_JPEGLIB
