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

	bool Initialize(int argc, char *argv[], const char* SysLibPath);
	void Shutdown();

	// Pieces library.
	bool LoadPiecesLibrary(const char* LibPath, const char* SysLibPath);
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

protected:
	void ParseIntegerArgument(int* CurArg, int argc, char* argv[], int* Value);
	void ParseStringArgument(int* CurArg, int argc, char* argv[], char** Value);

	Project* m_ActiveProject;
	PtrArray<Project> m_Projects;
	lcPiecesLibrary* m_Library;
};

extern lcApplication* g_App;
lcPiecesLibrary* lcGetPiecesLibrary();
Project* lcGetActiveProject();

#endif // _LC_APPLICATION_H_
