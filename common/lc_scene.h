#ifndef _LC_SCENE_H_
#define _LC_SCENE_H_

#include "array.h"
#include "algebra.h"

class lcPieceObject;
class lcMesh;
struct lcMeshSection;

struct lcRenderSection
{
	Matrix44 ModelWorld;
	lcPieceObject* Owner;
	lcMesh* Mesh;
	lcMeshSection* Section;
	float Distance;
	int Color;
};

class lcScene
{
public:
	lcScene(int OpaqueSize, int OpaqueGrow, int TranslucentSize, int TranslucentGrow);
	~lcScene();

	void Render();

public:
	Matrix44 m_WorldView;
	ObjArray<lcRenderSection> m_OpaqueSections;
	ObjArray<lcRenderSection> m_TranslucentSections;
};

int SortRenderSectionsCallback(const lcRenderSection& a, const lcRenderSection& b, void* SortData);

#endif // _LC_SCENE_H_
