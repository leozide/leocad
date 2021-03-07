#pragma once

#include "lc_math.h"

enum lcMeshPrimitiveType
{
	LC_MESH_LINES = 0x01,
	LC_MESH_TRIANGLES = 0x02,
	LC_MESH_TEXTURED_TRIANGLES = 0x08,
	LC_MESH_CONDITIONAL_LINES = 0x10,
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

struct lcVertexConditional
{
	lcVector3 Position1;
	lcVector3 Position2;
	lcVector3 Position3;
	lcVector3 Position4;
};

struct lcMeshSection
{
	int ColorIndex;
	int IndexOffset;
	int NumIndices;
	lcMeshPrimitiveType PrimitiveType;
	lcTexture* Texture;
	lcBoundingBox BoundingBox;
	float Radius;
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

enum class lcMeshFlag
{
	HasDefault     = 0x01, // Mesh has triangles using the default color
	HasSolid       = 0x02, // Mesh has triangles using a solid color
	HasTranslucent = 0x04, // Mesh has triangles using a translucent color
	HasLines       = 0x08, // Mesh has lines
	HasTexture     = 0x10, // Mesh has sections using textures
	HasStyleStud   = 0x20  // Mesh has a stud that can have a logo applied
};

Q_DECLARE_FLAGS(lcMeshFlags, lcMeshFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(lcMeshFlags)

class lcMesh
{
public:
	lcMesh();
	~lcMesh();

	lcMesh(const lcMesh&) = delete;
	lcMesh(lcMesh&&) = delete;
	lcMesh& operator=(const lcMesh&) = delete;
	lcMesh& operator=(lcMesh&&) = delete;

	void Create(quint16 (&NumSections)[LC_NUM_MESH_LODS], int VertexCount, int TexturedVertexCount, int ConditionalVertexCount, int IndexCount);
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
	bool IntersectsPlanes(const lcVector4 (&Planes)[6]);
	bool IntersectsPlanes(const lcVector4 (&Planes)[6]);

	int GetLodIndex(float Distance) const;

	lcMeshLod mLods[LC_NUM_MESH_LODS];
	lcBoundingBox mBoundingBox;
	float mRadius;
	lcMeshFlags mFlags;

	void* mVertexData;
	int mVertexDataSize;
	void* mIndexData;
	int mIndexDataSize;
	int mVertexCacheOffset;
	int mIndexCacheOffset;

	int mNumVertices;
	int mNumTexturedVertices;
	int mConditionalVertexCount;
	int mIndexType;
};


