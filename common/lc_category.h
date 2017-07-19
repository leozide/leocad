#pragma once

#include "lc_array.h"

struct lcLibraryCategory
{
	QString Name;
	QByteArray Keywords;
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

