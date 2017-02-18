#ifndef _LC_MESH_H_
#define _LC_MESH_H_

#include "lc_math.h"

#define LC_MESH_FILE_ID      LC_FOURCC('M', 'E', 'S', 'H')
#define LC_MESH_FILE_VERSION 0x0112

enum lcMeshPrimitiveType
{
	LC_MESH_LINES,
	LC_MESH_TRIANGLES,
	LC_MESH_TEXTURED_LINES,
	LC_MESH_TEXTURED_TRIANGLES,
	LC_MESH_CONDITIONAL_LINES,
	LC_MESH_NUM_PRIMITIVE_TYPES
};

struct lcVertex
{
	lcVector3 Position;
	quint32 Normal;
};

struct lcVertexTextured
{
	lcVector3 Position;
	quint32 Normal;
	lcVector2 TexCoord;
};

struct lcMeshSection
{
	int ColorIndex;
	int IndexOffset;
	int NumIndices;
	lcMeshPrimitiveType PrimitiveType;
	lcTexture* Texture;
};

struct lcMeshLod
{
	lcMeshSection* Sections;
	int NumSections;
};

enum
{
	LC_MESH_LOD_HIGH,
	LC_MESH_LOD_LOW,
	LC_NUM_MESH_LODS
};

class lcMesh
{
public:
	lcMesh();
	~lcMesh();

	void Create(lcuint16 NumSections[LC_NUM_MESH_LODS], int NumVertices, int NumTexturedVertices, int NumIndices);
	void CreateBox();

	bool FileLoad(lcMemFile& File);
	bool FileSave(lcMemFile& File);

	template<typename IndexType>
	void ExportPOVRay(lcFile& File, const char* MeshName, const char** ColorTable);
	void ExportPOVRay(lcFile& File, const char* MeshName, const char** ColorTable);

	template<typename IndexType>
	void ExportWavefrontIndices(lcFile& File, int DefaultColorIndex, int VertexOffset);
	void ExportWavefrontIndices(lcFile& File, int DefaultColorIndex, int VertexOffset);

	template<typename IndexType>
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDist);
	bool MinIntersectDist(const lcVector3& Start, const lcVector3& End, float& MinDist);

	template<typename IndexType>
	bool IntersectsPlanes(const lcVector4 Planes[6]);
	bool IntersectsPlanes(const lcVector4 Planes[6]);

	int GetLodIndex(float Distance) const;

	lcMeshLod mLods[LC_NUM_MESH_LODS];
	lcBoundingBox mBoundingBox;
	float mRadius;

	void* mVertexData;
	int mVertexDataSize;
	void* mIndexData;
	int mIndexDataSize;
	int mVertexCacheOffset;
	int mIndexCacheOffset;

	int mNumVertices;
	int mNumTexturedVertices;
	int mIndexType;
};

enum lcRenderMeshState
{
	LC_RENDERMESH_NONE,
	LC_RENDERMESH_SELECTED,
	LC_RENDERMESH_FOCUSED
};

struct lcRenderMesh
{
	lcMatrix44 WorldMatrix;
	lcMesh* Mesh;
	float Distance;
	int ColorIndex;
	int LodIndex;
	lcRenderMeshState State;
};

extern lcMesh* gPlaceholderMesh;

#endif // _LC_MESH_H_
