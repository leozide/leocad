#pragma once

#include "lc_array.h"
#include "lc_math.h"
#include "lc_mesh.h"

#define LC_LIBRARY_VERTEX_UNTEXTURED 0x1
#define LC_LIBRARY_VERTEX_TEXTURED   0x2

enum lcMeshDataType
{
	LC_MESHDATA_HIGH,
	LC_MESHDATA_LOW,
	LC_MESHDATA_SHARED,
	LC_NUM_MESHDATA_TYPES
};

struct lcLibraryMeshVertex
{
	lcVector3 Position;
	lcVector3 Normal;
	float NormalWeight;
	lcVector2 TexCoord;
	quint32 Usage;
};

class lcLibraryMeshSection
{
public:
	lcLibraryMeshSection(lcMeshPrimitiveType PrimitiveType, quint32 Color, lcTexture* Texture)
		: mIndices(1024, 1024)
	{
		mPrimitiveType = PrimitiveType;
		mColor = Color;
		mTexture = Texture;
	}

	lcMeshPrimitiveType mPrimitiveType;
	quint32 mColor;
	lcTexture* mTexture;
	lcArray<quint32> mIndices;
};

enum class lcLibraryTextureMapType
{
	PLANAR,
	CYLINDRICAL,
	SPHERICAL
};

struct lcLibraryTextureMap
{
	lcTexture* Texture;

	union lcTextureMapParams
	{
		lcTextureMapParams()
		{
		}

		struct lcTextureMapPlanarParams
		{
			lcVector4 Planes[2];
		} Planar;

		struct lcTextureMapCylindricalParams
		{
			lcVector4 FrontPlane;
			float UpLength;
			lcVector4 Plane1;
			lcVector4 Plane2;
		} Cylindrical;

		struct lcTextureMapSphericalParams
		{
			lcVector4 FrontPlane;
			lcVector3 Center;
			lcVector4 Plane1;
			lcVector4 Plane2;
		} Spherical;
	} Params;

	float Angle1;
	float Angle2;
	lcLibraryTextureMapType Type;
	bool Fallback;
	bool Next;
};

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
	{
		mHasTextures = false;
		mHasStyleStud = false;

		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mVertices[MeshDataIdx].SetGrow(1024);
	}

	~lcLibraryMeshData()
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mSections[MeshDataIdx].DeleteAll();
	}

	lcLibraryMeshData(const lcLibraryMeshData&) = delete;
	lcLibraryMeshData(lcLibraryMeshData&&) = delete;
	lcLibraryMeshData& operator=(const lcLibraryMeshData&) = delete;
	lcLibraryMeshData& operator=(lcLibraryMeshData&&) = delete;

	bool IsEmpty() const
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			if (!mSections[MeshDataIdx].IsEmpty())
				return false;

		return true;
	}

	void RemoveAll()
	{
		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mVertices[MeshDataIdx].RemoveAll();

		for (int MeshDataIdx = 0; MeshDataIdx < LC_NUM_MESHDATA_TYPES; MeshDataIdx++)
			mSections[MeshDataIdx].RemoveAll();

		mHasTextures = false;
	}

	lcMesh* CreateMesh();
	lcLibraryMeshSection* AddSection(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, lcTexture* Texture);
	quint32 AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, bool Optimize);
	quint32 AddVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, bool Optimize);
	quint32 AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector2& TexCoord, bool Optimize);
	quint32 AddTexturedVertex(lcMeshDataType MeshDataType, const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoord, bool Optimize);
	void AddVertices(lcMeshDataType MeshDataType, int VertexCount, int* BaseVertex, lcLibraryMeshVertex** VertexBuffer);
	void AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, int IndexCount, quint32** IndexBuffer);
	void AddLine(lcMeshDataType MeshDataType, int LineType, quint32 ColorCode, bool WindingCCW, const lcVector3* Vertices, bool Optimize);
	void AddTexturedLine(lcMeshDataType MeshDataType, int LineType, quint32 ColorCode, bool WindingCCW, const lcLibraryTextureMap& Map, const lcVector3* Vertices, bool Optimize);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcLibraryTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void TestQuad(int* QuadIndices, const lcVector3* Vertices);
	void ResequenceQuad(int* QuadIndices, int a, int b, int c, int d);

	lcArray<lcLibraryMeshSection*> mSections[LC_NUM_MESHDATA_TYPES];
	lcArray<lcLibraryMeshVertex> mVertices[LC_NUM_MESHDATA_TYPES];
	bool mHasTextures;
	bool mHasStyleStud;
};

class lcMeshLoader
{
public:
	lcMeshLoader(lcLibraryMeshData& MeshData, bool Optimize, Project* CurrentProject, bool SearchProjectFolder);

	bool LoadMesh(lcFile& File, lcMeshDataType MeshDataType);

protected:
	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcArray<lcLibraryTextureMap>& TextureStack, lcMeshDataType MeshDataType);

	lcLibraryMeshData& mMeshData;
	bool mOptimize;
	Project* mCurrentProject;
	bool mSearchProjectFolder;
};
