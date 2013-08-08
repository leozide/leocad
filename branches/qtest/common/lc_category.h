#ifndef _LC_CATEGORY_H_
#define _LC_CATEGORY_H_

#include "str.h"
#include "array.h"

struct lcLibraryCategory
{
	String Name;
	String Keywords;
};

extern ObjArray<lcLibraryCategory> gCategories;

void lcResetDefaultCategories();
void lcLoadDefaultCategories(bool BuiltInLibrary = false);
void lcSaveDefaultCategories();

void lcResetCategories(ObjArray<lcLibraryCategory>& Categories, bool BuiltInLibrary = false);
bool lcLoadCategories(const char* FileName, ObjArray<lcLibraryCategory>& Categories);
bool lcLoadCategories(lcFile& File, ObjArray<lcLibraryCategory>& Categories);
bool lcSaveCategories(const char* FileName, const ObjArray<lcLibraryCategory>& Categories);
bool lcSaveCategories(lcFile& File, const ObjArray<lcLibraryCategory>& Categories);

#endif // _LC_CATEGORY_H_
