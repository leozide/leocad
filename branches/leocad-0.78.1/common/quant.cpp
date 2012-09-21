///////////////////////////////////////
// DL1 Quantization

#include "lc_global.h"
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>

struct CUBE
{
	unsigned long r, g, b;
	unsigned long pixel_count;
	unsigned long pixels_in_cube;
	unsigned char children;
	unsigned char palette_index;
};

struct FCUBE
{
	unsigned char level;
	unsigned short index;
};

struct CLOSEST_INFO
{
	unsigned char palette_index, red, green, blue;
	unsigned long distance;
	unsigned long squares[255+255+1];
};

//#define FAST		// improves speed but uses a lot of memory
#define QUAL1		// slightly improves quality
//#define QUAL2 	// slightly improves quality

//#define DITHER1	// 1-val error diffusion dither
#define DITHER2 	// 2-val error diffusion dither
//#define DITHER4	// 4-val error diffusion dither (Floyd-Steinberg)

#define DITHER_MAX	20

static void dlq_finish();
static int	build_table(unsigned char *image, unsigned long pixels);
static void fixheap(unsigned long id);
static void reduce_table(int num_colors);
static void set_palette(int index, int level);
static void closest_color(int index, int level);
static int	quantize_image(unsigned char *in, unsigned char *out, int width, int height, int dither);
static int	bestcolor(int r, int g, int b);

static unsigned char palette[3][256];
static CUBE *rgb_table[6];
static unsigned short r_offset[256], g_offset[256], b_offset[256];
static CLOSEST_INFO c_info;
static int tot_colors, pal_index;
static unsigned long *squares;
static FCUBE *heap = NULL;
static short *dl_image = NULL;

bool dl1quant(unsigned char *inbuf, unsigned char *outbuf, int width, int height, int quant_to, int dither, unsigned char userpal[3][256])
{
	int i;

	// dlq_init
	for (i = 0; i < 6; i++)
		rgb_table[i]=NULL;

	tot_colors=0;
	pal_index=0;

	heap = NULL;
	dl_image = NULL;

	for (i = 0; i < 256; i++)
	{
		r_offset[i] = (i & 128) << 7 | (i & 64) << 5 | (i & 32) << 3 |
				  (i & 16)	<< 1 | (i & 8)	>> 1;
		g_offset[i] = (i & 128) << 6 | (i & 64) << 4 | (i & 32) << 2 |
				  (i & 16)	<< 0 | (i & 8)	>> 2;
		b_offset[i] = (i & 128) << 5 | (i & 64) << 3 | (i & 32) << 1 |
				  (i & 16)	>> 1 | (i & 8)	>> 3;
	}

	c_info.palette_index=0;
	c_info.red=0;
	c_info.green=0;
	c_info.blue=0;
	c_info.distance=0;

	for (i = (-255); i <= 255; i++)
		c_info.squares[i+255] = i*i;

	for (i = 0; i < 256; i++)
	{
		palette[0][i] = 0;
		palette[1][i] = 0;
		palette[2][i] = 0;
	}

	squares = c_info.squares + 255;

	// dlq_start
	rgb_table[0] = (CUBE*)calloc(sizeof(CUBE), 1);
	rgb_table[1] = (CUBE*)calloc(sizeof(CUBE), 8);
	rgb_table[2] = (CUBE*)calloc(sizeof(CUBE), 64);
	rgb_table[3] = (CUBE*)calloc(sizeof(CUBE), 512);
	rgb_table[4] = (CUBE*)calloc(sizeof(CUBE), 4096);
	rgb_table[5] = (CUBE*)calloc(sizeof(CUBE), 32768);

	for (i = 0; i <= 5; i++)
		if (rgb_table[i] == NULL)
		{
			dlq_finish();
			return false;
		}

	pal_index = 0;

	if (build_table(inbuf, width*height) == 0) 
	{
		dlq_finish();
		return false;
	}
	
	reduce_table(quant_to);
	set_palette(0, 0);
	
	if (quantize_image(inbuf, outbuf, width, height, dither) == 0) 
	{
		dlq_finish();
		return false;
	}

	dlq_finish();
	for (i = 0; i < 256; i++) 
	{
		userpal[0][i] = palette[0][i];
		userpal[1][i] = palette[1][i];
		userpal[2][i] = palette[2][i];
	}

	return true;
}

static void dlq_finish(void) 
{
	for (int i = 0;i < 6;i++)
	{
		if (rgb_table[i] != NULL)
		{
			free(rgb_table[i]);
			rgb_table[i] = NULL;
		}
	}

	if (heap != NULL) 
	{
		free(heap);
		heap = NULL;
	}
	
	if (dl_image != NULL)
	{
		free(dl_image);
		dl_image = NULL;
	}

	memset(&c_info, 0, sizeof(CLOSEST_INFO));

	tot_colors=pal_index=0;
}

// returns 1 on success, 0 on failure
static int build_table(unsigned char *image, unsigned long pixels)
{
	unsigned long i = 0, index = 0, cur_count = 0, head = 0, tail = 0;
	long j = 0;

	heap = (FCUBE *) malloc(sizeof(FCUBE) * 32769);
	if (heap == NULL)
		return 0;

#ifdef FAST
	dl_image = malloc(sizeof(short) * pixels);
	if (dl_image == NULL)
		return 0;
#endif

	for (i = 0; i < pixels; i++) 
	{
#ifdef FAST
		dl_image[i] = index = r_offset[image[0]] + g_offset[image[1]] + b_offset[image[2]];
#else
		index = r_offset[image[0]] + g_offset[image[1]] + b_offset[image[2]];
#endif
#ifdef QUAL1
		rgb_table[5][index].r += image[0];
		rgb_table[5][index].g += image[1];
		rgb_table[5][index].b += image[2];
#endif
		rgb_table[5][index].pixel_count++;
		image += 3;
	}

	tot_colors = 0;
	for (i = 0; i < 32768; i++) 
	{
		cur_count = rgb_table[5][i].pixel_count;
		if (cur_count) 
		{
			heap[++tot_colors].level = 5;
			heap[tot_colors].index = (unsigned short)i;
			rgb_table[5][i].pixels_in_cube = cur_count;
#ifndef QUAL1
			rgb_table[5][i].r = cur_count * (((i & 0x4000) >> 7 |
					(i & 0x0800) >> 5 | (i & 0x0100) >> 3 |
					(i & 0x0020) >> 1 | (i & 0x0004) << 1) + 4);
			rgb_table[5][i].g = cur_count * (((i & 0x2000) >> 6 |
					(i & 0x0400) >> 4 | (i & 0x0080) >> 2 |
					(i & 0x0010) >> 0 | (i & 0x0002) << 2) + 4);
			rgb_table[5][i].b = cur_count * (((i & 0x1000) >> 5 |
					(i & 0x0200) >> 3 | (i & 0x0040) >> 1 |
					(i & 0x0008) << 1 | (i & 0x0001) << 3) + 4);
#endif
			head = i;
			for (j = 4; j >= 0; j--) 
			{
				tail = head & 0x7;
				head >>= 3;
				rgb_table[j][head].pixels_in_cube += cur_count;
				rgb_table[j][head].children |= 1 << tail;
			}
		}
	}

	for (i = tot_colors; i > 0; i--)
		fixheap(i);

	return 1;
}

static void fixheap(unsigned long heapid) 
{
	unsigned char thres_level = heap[heapid].level;
	unsigned long thres_index = heap[heapid].index, index = 0;
	unsigned long half_totc = tot_colors >> 1;
	unsigned long thres_val = rgb_table[thres_level][thres_index].pixels_in_cube;

	while (heapid <= half_totc) 
	{
		index = heapid << 1;

		if (index < (unsigned long)tot_colors)
			if (rgb_table[heap[index].level][heap[index].index].pixels_in_cube
			  > rgb_table[heap[index+1].level][heap[index+1].index].pixels_in_cube)
			index++;

		if (thres_val <= rgb_table[heap[index].level][heap[index].index].pixels_in_cube)
			break;
		else {
			heap[heapid] = heap[index];
			heapid = index;
		}
	}
	heap[heapid].level = thres_level;
	heap[heapid].index = (unsigned short)thres_index;
}

static void reduce_table(int num_colors) 
{
	while (tot_colors > num_colors) 
	{
		unsigned char tmp_level = heap[1].level, t_level = (tmp_level - 1) > 0 ? (tmp_level - 1) : 0;
		unsigned long tmp_index = heap[1].index, t_index = tmp_index >> 3;

		if (rgb_table[t_level][t_index].pixel_count)
			heap[1] = heap[tot_colors--];
		else 
		{
			heap[1].level = t_level;
			heap[1].index = (unsigned short)t_index;
		}

		rgb_table[t_level][t_index].pixel_count += rgb_table[tmp_level][tmp_index].pixel_count;
		rgb_table[t_level][t_index].r += rgb_table[tmp_level][tmp_index].r;
		rgb_table[t_level][t_index].g += rgb_table[tmp_level][tmp_index].g;
		rgb_table[t_level][t_index].b += rgb_table[tmp_level][tmp_index].b;
		rgb_table[t_level][t_index].children &= ~(1 << (tmp_index & 0x7));

		fixheap(1);
	}
}

static void set_palette(int index, int level) 
{
	int i;

	if (rgb_table[level][index].children) 
		for (i = 7; i >= 0; i--) 
			if (rgb_table[level][index].children & (1 << i)) 
				set_palette((index << 3) + i, level + 1);

	if (rgb_table[level][index].pixel_count) 
	{
		unsigned long r_sum, g_sum, b_sum, sum;

		rgb_table[level][index].palette_index = pal_index;
		
		r_sum = rgb_table[level][index].r;
		g_sum = rgb_table[level][index].g;
		b_sum = rgb_table[level][index].b;
		
		sum = rgb_table[level][index].pixel_count;

		palette[0][pal_index] = (unsigned char)((r_sum + (sum >> 1)) / sum);
		palette[1][pal_index] = (unsigned char)((g_sum + (sum >> 1)) / sum);
		palette[2][pal_index] = (unsigned char)((b_sum + (sum >> 1)) / sum);

		pal_index++;
	}
}

static void closest_color(int index, int level) 
{
	int i;

	if (rgb_table[level][index].children) 
		for (i = 7; i >= 0; i--) 
			if (rgb_table[level][index].children & (1 << i))
				closest_color((index << 3) + i, level + 1);

	if (rgb_table[level][index].pixel_count) 
	{
		long dist, r_dist, g_dist, b_dist;
		unsigned char pal_num = rgb_table[level][index].palette_index;

		// Determine if this color is "closest".
		r_dist = palette[0][pal_num] - c_info.red;
		g_dist = palette[1][pal_num] - c_info.green;
		b_dist = palette[2][pal_num] - c_info.blue;
		
		dist = squares[r_dist] + squares[g_dist] + squares[b_dist];

		if (dist < (long)c_info.distance) 
		{
			c_info.distance = dist;
			c_info.palette_index = pal_num;
		}
	}
}

// returns 1 on success, 0 on failure
static int quantize_image(unsigned char *in, unsigned char *out, int width, int height, int dither) 
{
	if (!dither) 
	{
		unsigned long i = 0, pixels = width * height;
		unsigned short level = 0, index = 0;
		unsigned char tmp_r = 0, tmp_g = 0, tmp_b = 0, cube = 0;
		unsigned char *lookup = NULL;

		lookup = (unsigned char*)malloc(sizeof(char) * 32768);
		if (lookup == NULL)
			return 0;

		for (i = 0; i < 32768; i++)
			if (rgb_table[5][i].pixel_count) 
			{
			tmp_r = (unsigned char)((i & 0x4000) >> 7 | (i & 0x0800) >> 5 |
				(i & 0x0100) >> 3 | (i & 0x0020) >> 1 |
				(i & 0x0004) << 1);
			tmp_g = (unsigned char)((i & 0x2000) >> 6 | (i & 0x0400) >> 4 |
				(i & 0x0080) >> 2 | (i & 0x0010) >> 0 |
				(i & 0x0002) << 2);
			tmp_b = (unsigned char)((i & 0x1000) >> 5 | (i & 0x0200) >> 3 |
				(i & 0x0040) >> 1 | (i & 0x0008) << 1 |
				(i & 0x0001) << 3);
	#ifdef QUAL2
			lookup[i] = bestcolor(tmp_r, tmp_g, tmp_b);
	#else
			c_info.red	 = tmp_r + 4;
			c_info.green = tmp_g + 4;
			c_info.blue  = tmp_b + 4;
			level = 0;
			index = 0;
			for (;;) {
				cube = (tmp_r&128) >> 5 | (tmp_g&128) >> 6 | (tmp_b&128) >> 7;
				if ((rgb_table[level][index].children & (1 << cube)) == 0) {
				c_info.distance = (unsigned long)~0L;
				closest_color(index, level);
				lookup[i] = c_info.palette_index;
				break;
				}
				level++;
				index = (index << 3) + cube;
				tmp_r <<= 1;
				tmp_g <<= 1;
				tmp_b <<= 1;
			}
	#endif
			}

		for (i = 0; i < pixels; i++) 
		{
	#ifdef FAST
			out[i] = lookup[dl_image[i]];
	#else
			out[i] = lookup[r_offset[in[0]] + g_offset[in[1]] + b_offset[in[2]]];
			in += 3;
	#endif
		}

		free(lookup);

	} 
	else // dither
	{ 
	#if defined(DITHER2) || defined(DITHER4)
		long i = 0, j = 0;
		long r_pix = 0, g_pix = 0, b_pix = 0;
		long offset = 0, dir = 0;
		long odd_scanline = 0;
		long err_len = (width + 2) * 3;
		unsigned char *range_tbl = NULL, *range = NULL;
		short *lookup = NULL, *erowerr = NULL, *orowerr = NULL;
		short *thisrowerr = NULL, *nextrowerr = NULL;
		char *dith_max_tbl = NULL, *dith_max = NULL;

		lookup = (short*)malloc(sizeof(short) * 32768);
		erowerr = (short*)malloc(sizeof(short) * err_len);
		orowerr = (short*)malloc(sizeof(short) * err_len);
		range_tbl = (unsigned char*)malloc(3 * 256);
		range = range_tbl + 256;
		dith_max_tbl= (char*)malloc(512);
		dith_max = dith_max_tbl + 256;

		if (range_tbl == NULL || lookup == NULL || erowerr == NULL || orowerr == NULL || dith_max_tbl == NULL) 
		{
			if (range_tbl != NULL) 
			{
				free(range_tbl);
				range_tbl=NULL;
			}
			if (lookup != NULL) 
			{
				free(lookup);
				lookup=NULL;
			}
			if (erowerr != NULL) 
			{
				free(erowerr);
				erowerr=NULL;
			}
			if (orowerr != NULL) 
			{
				free(orowerr);
				orowerr=NULL;
			}
			if (dith_max_tbl != NULL) 
			{
				free(dith_max_tbl);
				dith_max_tbl=NULL;
			}
			return 0;
		}

		for (i = 0; i < err_len; i++)
			erowerr[i] = 0;

		for (i = 0; i < 32768; i++)
			lookup[i] = -1;

		for (i = 0; i < 256; i++) 
		{
			range_tbl[i] = 0;
			range_tbl[i + 256] = (unsigned char) i;
			range_tbl[i + 512] = 255;
		}

		for (i = 0; i < 256; i++) 
		{
			dith_max_tbl[i] = -DITHER_MAX;
			dith_max_tbl[i + 256] = DITHER_MAX;
		}
		for (i = -DITHER_MAX; i <= DITHER_MAX; i++)
			dith_max_tbl[i + 256] = (char)i;

		for (i = 0 ; i < height; i++) 
		{
			if (odd_scanline) 
			{
				dir = -1;
				in	+= (width - 1) * 3;
				out += (width - 1);
				thisrowerr = orowerr + 3;
				nextrowerr = erowerr + width * 3;
			}
			else 
			{
				dir = 1;
				thisrowerr = erowerr + 3;
				nextrowerr = orowerr + width * 3;
			}

			nextrowerr[0] = nextrowerr[1] = nextrowerr[2] = 0;
			
			for (j = 0; j < width; j++) 
			{
		#ifdef DITHER2
				r_pix = range[(thisrowerr[0] >> 1) + in[0]];
				g_pix = range[(thisrowerr[1] >> 1) + in[1]];
				b_pix = range[(thisrowerr[2] >> 1) + in[2]];
		#else
				r_pix = range[((thisrowerr[0] + 8) >> 4) + in[0]];
				g_pix = range[((thisrowerr[1] + 8) >> 4) + in[1]];
				b_pix = range[((thisrowerr[2] + 8) >> 4) + in[2]];
		#endif
				offset = (r_pix&248) << 7 | (g_pix&248) << 2 | b_pix >> 3;
				if (lookup[offset] < 0)
					lookup[offset] = bestcolor(r_pix, g_pix, b_pix);
				*out = (unsigned char)lookup[offset];
				r_pix = dith_max[r_pix - palette[0][lookup[offset]]];
				g_pix = dith_max[g_pix - palette[1][lookup[offset]]];
				b_pix = dith_max[b_pix - palette[2][lookup[offset]]];

		#ifdef DITHER2
				nextrowerr[0  ]  = (short)r_pix;
				thisrowerr[0+3] += (short)r_pix;
				nextrowerr[1  ]  = (short)g_pix;
				thisrowerr[1+3] += (short)g_pix;
				nextrowerr[2  ]  = (short)b_pix;
				thisrowerr[2+3] += (short)b_pix;
		#else
				two_val = r_pix * 2;
				nextrowerr[0-3]  = r_pix;
				r_pix += two_val;
				nextrowerr[0+3] += r_pix;
				r_pix += two_val;
				nextrowerr[0  ] += r_pix;
				r_pix += two_val;
				thisrowerr[0+3] += r_pix;
				two_val = g_pix * 2;
				nextrowerr[1-3]  = g_pix;
				g_pix += two_val;
				nextrowerr[1+3] += g_pix;
				g_pix += two_val;
				nextrowerr[1  ] += g_pix;
				g_pix += two_val;
				thisrowerr[1+3] += g_pix;
				two_val = b_pix * 2;
				nextrowerr[2-3]  = b_pix;
				b_pix += two_val;
				nextrowerr[2+3] += b_pix;
				b_pix += two_val;
				nextrowerr[2  ] += b_pix;
				b_pix += two_val;
				thisrowerr[2+3] += b_pix;
		#endif
				thisrowerr += 3;
				nextrowerr -= 3;
				in	+= dir * 3;
				out += dir;
			}

			if ((i % 2) == 1) 
			{
				in	+= (width + 1) * 3;
				out += (width + 1);
			}

			odd_scanline = !odd_scanline;
		}

		free(range_tbl);
		free(lookup);
		free(erowerr);
		free(orowerr);
		free(dith_max_tbl);
	#else
		long i = 0, j = 0; 
		long r_pix = 0, g_pix = 0, b_pix=0;
		long r_err = 0, g_err = 0, b_err=0;
		long offset = 0;
		BYTE *range_tbl = (BYTE*)malloc(3 * 256), *range = range_tbl + 256;
		short *lookup = (sshort *)malloc(sizeof(short) * 32768);

		if (range_tbl == NULL || lookup == NULL) 
		{
			if (range_tbl != NULL)
			free(range_tbl);
			if (lookup != NULL)
			free(lookup);
			return 0;
		}

		for (i = 0; i < 32768; i++)
			lookup[i] = -1;

		for (i = 0; i < 256; i++) 
		{
			range_tbl[i] = 0;
			range_tbl[i + 256] = (BYTE) i;
			range_tbl[i + 512] = 255;
		}

		for (i = 0; i < height; i++) 
		{
			r_err = g_err = b_err = 0;
			for (j = width - 1; j >= 0; j--) 
			{
				r_pix = range[(r_err >> 1) + in[0]];
				g_pix = range[(g_err >> 1) + in[1]];
				b_pix = range[(b_err >> 1) + in[2]];
				
				offset = (r_pix&248) << 7 | (g_pix&248) << 2 | b_pix >> 3;

				if (lookup[offset] < 0)
					lookup[offset] = bestcolor(r_pix, g_pix, b_pix);

				*out++ = (unsigned char)lookup[offset];

				r_err = r_pix - palette[0][lookup[offset]];
				g_err = g_pix - palette[1][lookup[offset]];
				b_err = b_pix - palette[2][lookup[offset]];

				in += 3;
			}
		}

		free(range_tbl);
		free(lookup);
	#endif
	}

	return 1;
}

static int bestcolor(int r, int g, int b) 
{
	unsigned long i = 0, bestcolor = 0, curdist = 0, mindist = 0;
	long rdist = 0, gdist = 0, bdist = 0;

	r = (r & 248) + 4;
	g = (g & 248) + 4;
	b = (b & 248) + 4;
	mindist = 200000;

	for (i = 0; i < (unsigned long)tot_colors; i++) 
	{
		rdist = palette[0][i] - r;
		gdist = palette[1][i] - g;
		bdist = palette[2][i] - b;
		curdist = squares[rdist] + squares[gdist] + squares[bdist];

		if (curdist < mindist) 
		{
			mindist = curdist;
			bestcolor = i;
		}
	}
	return bestcolor;
}
