#ifndef _IMAGE_H_
#define _IMAGE_H_

class File;

#include "typedefs.h"

LC_IMAGE* OpenImage(char* filename);
LC_IMAGE* OpenImage(File* file, unsigned char format);
bool SaveImage(File* file, LC_IMAGE* image, LC_IMAGE_OPTS* opts);
bool SaveImage(char* filename, LC_IMAGE* image, LC_IMAGE_OPTS* opts);
void SaveVideo(char* filename, LC_IMAGE** images, int count, float fps);

#endif // _IMAGE_H_