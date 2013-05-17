#ifndef _LC_APPLICATION_H_
#define _LC_APPLICATION_H_

#include "array.h"

class Project;
class lcPiecesLibrary;

class lcApplication
{
public:
	lcApplication();
	~lcApplication();

	bool Initialize(int argc, char *argv[], const char* LibraryInstallPath, const char* LibraryCachePath);
	void Shutdown();
	void OpenURL(const char* URL);
	void SetClipboard(lcFile* Clipboard);
	void ExportClipboard(lcMemFile* Clipboard);

	// Pieces library.
	bool LoadPiecesLibrary(const char* LibPath, const char* LibraryInstallPath, const char* LibraryCachePath);
	lcPiecesLibrary* GetPiecesLibrary() const
	{
		return m_Library;
	}

	// Projects.
	void AddProject(Project* project);

	Project* GetActiveProject() const
	{
		return m_ActiveProject;
	}

	void SetActiveProject(Project* project)
	{
		m_ActiveProject = project;
	}

	Project* m_ActiveProject;
	PtrArray<Project> m_Projects;
	lcPiecesLibrary* m_Library;
	lcFile* mClipboard;

protected:
	void ParseIntegerArgument(int* CurArg, int argc, char* argv[], int* Value);
	void ParseStringArgument(int* CurArg, int argc, char* argv[], char** Value);
};

extern lcApplication* g_App;
lcPiecesLibrary* lcGetPiecesLibrary();
Project* lcGetActiveProject();

#endif // _LC_APPLICATION_H_
