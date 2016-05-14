#ifndef _LC_CATEGORY_H_
#define _LC_CATEGORY_H_

#include "str.h"
#include "lc_array.h"

struct lcLibraryCategory
{
	String Name;
	String Keywords;
};

extern lcArray<lcLibraryCategory> gCategories;

void lcResetDefaultCategories();
void lcLoadDefaultCategories(bool BuiltInLibrary = false);
void lcSaveDefaultCategories();

void lcResetCategories(lcArray<lcLibraryCategory>& Categories, bool BuiltInLibrary = false);
bool lcLoadCategories(const QString& FileName, lcArray<lcLibraryCategory>& Categories);
bool lcLoadCategories(const QByteArray& Buffer, lcArray<lcLibraryCategory>& Categories);
bool lcSaveCategories(const QString& FileName, const lcArray<lcLibraryCategory>& Categories);
bool lcSaveCategories(QTextStream& Stream, const lcArray<lcLibraryCategory>& Categories);

#endif // _LC_CATEGORY_H_
