#ifndef _QUANT_H_
#define _QUANT_H_

bool dl1quant(unsigned char *inbuf, unsigned char *outbuf, int width, int height, int quant_to, int dither, unsigned char userpal[3][256]);

#endif // _QUANT_H_
