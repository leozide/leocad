#ifndef _LC_APPLICATION_H_
#define _LC_APPLICATION_H_

#include "array.h"
#include "str.h"

class Project;
class lcPiecesLibrary;

class lcApplication
{
public:
	lcApplication();
	~lcApplication();

	bool Initialize(int argc, char *argv[], const char* LibraryInstallPath, const char* LibraryCachePath);
	void Shutdown();

	bool LoadPiecesLibrary(const char* LibPath, const char* LibraryInstallPath, const char* LibraryCachePath);

	void GetFileList(const char* Path, ObjArray<String>& FileList);
	void OpenURL(const char* URL);
	void SetClipboard(lcFile* Clipboard);
	void ExportClipboard(lcMemFile* Clipboard);

	Project* mProject;
	lcPiecesLibrary* m_Library;
	lcFile* mClipboard;

protected:
	void ParseIntegerArgument(int* CurArg, int argc, char* argv[], int* Value);
	void ParseStringArgument(int* CurArg, int argc, char* argv[], char** Value);
};

extern lcApplication* g_App;

inline lcPiecesLibrary* lcGetPiecesLibrary()
{
	return g_App->m_Library;
}

inline Project* lcGetActiveProject()
{
	return g_App->mProject;
}

#endif // _LC_APPLICATION_H_
