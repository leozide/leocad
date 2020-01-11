#pragma once

struct lcLibraryCategory
{
	QString Name;
	QByteArray Keywords;
};

extern std::vector<lcLibraryCategory> gCategories;

void lcResetDefaultCategories();
void lcLoadDefaultCategories(bool BuiltInLibrary = false);
void lcSaveDefaultCategories();

void lcResetCategories(std::vector<lcLibraryCategory>& Categories, bool BuiltInLibrary = false);
bool lcLoadCategories(const QString& FileName, std::vector<lcLibraryCategory>& Categories);
bool lcLoadCategories(const QByteArray& Buffer, std::vector<lcLibraryCategory>& Categories);
bool lcSaveCategories(const QString& FileName, const std::vector<lcLibraryCategory>& Categories);
bool lcSaveCategories(QTextStream& Stream, const std::vector<lcLibraryCategory>& Categories);

bool lcMatchCategory(const char* PieceName, const char* Expression);
