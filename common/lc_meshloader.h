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

struct lcMeshLoaderVertex
{
	lcVector3 Position;
	lcVector3 Normal;
	float NormalWeight;
	lcVector2 TexCoord;
	quint32 Usage;
};

struct lcMeshLoaderConditionalVertex
{
	lcVector3 Position[4];
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

enum class lcMeshLoaderTextureMapType
{
	Planar,
	Cylindrical,
	Spherical
};

struct lcMeshLoaderTextureMap
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
	lcMeshLoaderTextureMapType Type;
	bool Fallback;
	bool Next;
};

class lcMeshLoaderTypeData
{
public:
	lcMeshLoaderTypeData()
	{
		mVertices.SetGrow(1024);
		mConditionalVertices.SetGrow(1024);
	}

	lcMeshLoaderTypeData(const lcMeshLoaderTypeData&) = delete;
	lcMeshLoaderTypeData& operator=(const lcMeshLoaderTypeData&) = delete;

	bool IsEmpty() const
	{
		return mSections.empty();
	}

	void Clear()
	{
		mSections.clear();
		mVertices.RemoveAll();
		mConditionalVertices.RemoveAll();
	}

	lcLibraryMeshSection* AddSection(lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, lcTexture* Texture);

	quint32 AddVertex(const lcVector3& Position, bool Optimize);
	quint32 AddVertex(const lcVector3& Position, const lcVector3& Normal, bool Optimize);
	quint32 AddTexturedVertex(const lcVector3& Position, const lcVector2& TexCoord, bool Optimize);
	quint32 AddTexturedVertex(const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoord, bool Optimize);
	quint32 AddConditionalVertex(const lcVector3* Position, bool Optimize);

	void ProcessLine(int LineType, quint32 ColorCode, bool WindingCCW, lcVector3* Vertices, bool Optimize);
	void ProcessTexturedLine(int LineType, quint32 ColorCode, bool WindingCCW, const lcMeshLoaderTextureMap& Map, const lcVector3* Vertices, bool Optimize);

	void AddMeshData(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap);
	void AddMeshDataNoDuplicateCheck(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap);

	std::vector<std::unique_ptr<lcLibraryMeshSection>> mSections;
	lcArray<lcMeshLoaderVertex> mVertices;
	lcArray<lcMeshLoaderConditionalVertex> mConditionalVertices;
};

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
	{
		mHasTextures = false;
		mHasStyleStud = false;
	}

	lcLibraryMeshData(const lcLibraryMeshData&) = delete;
	lcLibraryMeshData& operator=(const lcLibraryMeshData&) = delete;

	bool IsEmpty() const
	{
		for (const lcMeshLoaderTypeData& Data : mData)
			if (!Data.IsEmpty())
				return false;

		return true;
	}

	void Clear()
	{
		for (lcMeshLoaderTypeData& Data : mData)
			Data.Clear();

		mHasTextures = false;
		mHasStyleStud = false;
	}

	lcMesh* CreateMesh();
	void AddVertices(lcMeshDataType MeshDataType, int VertexCount, int* BaseVertex, lcMeshLoaderVertex** VertexBuffer);
	void AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, int IndexCount, quint32** IndexBuffer);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);

	std::array<lcMeshLoaderTypeData, LC_NUM_MESHDATA_TYPES> mData;
	bool mHasTextures;
	bool mHasStyleStud;
};

class lcMeshLoader
{
public:
	lcMeshLoader(lcLibraryMeshData& MeshData, bool Optimize, Project* CurrentProject, bool SearchProjectFolder);

	bool LoadMesh(lcFile& File, lcMeshDataType MeshDataType);

protected:
	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcArray<lcMeshLoaderTextureMap>& TextureStack, lcMeshDataType MeshDataType);

	lcLibraryMeshData& mMeshData;
	bool mOptimize;
	Project* mCurrentProject;
	bool mSearchProjectFolder;
};
