#ifndef _LC_APPLICATION_H_
#define _LC_APPLICATION_H_

#include "lc_array.h"

class Project;
class PiecesLibrary;
class PiecePreview;

#define LC_MAX_MOUSE_SENSITIVITY 20

class lcApplication
{
public:
	lcApplication();
	~lcApplication();

	bool Initialize(int argc, char* argv[], const char* SysLibPath);
	void Shutdown();

	// Pieces library.
	bool LoadPiecesLibrary(const char* LibPath, const char* SysLibPath);
	PiecesLibrary* GetPiecesLibrary() const
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

public:
	PiecePreview* m_PiecePreview;
	u32 m_SelectedColor;
	int m_MouseSensitivity;

protected:
	void ParseIntegerArgument(int* CurArg, int argc, char* argv[], int* Value);
	void ParseStringArgument(int* CurArg, int argc, char* argv[], char** Value);

	Project* m_ActiveProject;
	lcPtrArray<Project> m_Projects;
	PiecesLibrary* m_Library;
};

extern lcApplication* g_App;
PiecesLibrary* lcGetPiecesLibrary();
Project* lcGetActiveProject();

#endif // _LC_APPLICATION_H_
