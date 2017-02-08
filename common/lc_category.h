#ifndef _LC_CATEGORY_H_
#define _LC_CATEGORY_H_

#include "lc_array.h"

struct lcLibraryCategory
{
	std::string Name;
	std::string Keywords;
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

bool lcMatchCategory(const char* PieceName, const char* Expression);

#endif // _LC_CATEGORY_H_
