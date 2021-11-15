#pragma once

#include "lc_array.h"
#include "lc_math.h"
#include "lc_mesh.h"

class lcLibraryMeshData;
class lcMeshLoader;

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
};

struct lcMeshLoaderTexturedVertex
{
	lcVector3 Position;
	lcVector3 Normal;
	lcVector2 TexCoords;
};

struct lcMeshLoaderConditionalVertex
{
	lcVector3 Position[4];
};

enum class lcMeshLoaderMaterialType
{
	Solid,
	Planar,
	Cylindrical,
	Spherical
};

struct lcMeshLoaderMaterial
{
	lcMeshLoaderMaterialType Type = lcMeshLoaderMaterialType::Solid;
	quint32 Color = 16;
	lcVector3 Points[3] = {};
	float Angles[2] = {};
	char Name[256] = {};
};

class lcMeshLoaderSection
{
public:
	lcMeshLoaderSection(lcMeshPrimitiveType PrimitiveType, lcMeshLoaderMaterial* Material)
		: mMaterial(Material), mPrimitiveType(PrimitiveType), mIndices(1024, 1024)
	{
	}

	lcMeshLoaderMaterial* mMaterial;
	lcMeshPrimitiveType mPrimitiveType;
	lcArray<quint32> mIndices;
};

struct lcMeshLoaderFinalSection
{
	quint32 Color;
	lcMeshPrimitiveType PrimitiveType;
	char Name[256];
};

struct lcMeshLoaderTextureMap
{
	union lcTextureMapParams
	{
		lcTextureMapParams()
		{
		}

		struct lcTextureMapSphericalParams
		{
		} Spherical;
	} Params;

	lcMeshLoaderMaterialType Type;
	lcVector3 Points[3];
	float Angles[2];
	char Name[LC_MAXPATH];

	bool Fallback = false;
	bool Next = false;
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

	void SetMeshData(lcLibraryMeshData* MeshData)
	{
		mMeshData = MeshData;
	}

	lcMeshLoaderSection* AddSection(lcMeshPrimitiveType PrimitiveType, lcMeshLoaderMaterial* Material);

	quint32 AddVertex(const lcVector3& Position, bool Optimize);
	quint32 AddVertex(const lcVector3& Position, const lcVector3& Normal, bool Optimize);
	quint32 AddConditionalVertex(const lcVector3 (&Position)[4]);

	void ProcessLine(int LineType, lcMeshLoaderMaterial* Material, bool WindingCCW, lcVector3 (&Vertices)[4], bool Optimize);

	void AddMeshData(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap);
	void AddMeshDataNoDuplicateCheck(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap);

	std::vector<std::unique_ptr<lcMeshLoaderSection>> mSections;
	lcArray<lcMeshLoaderVertex> mVertices;
	lcArray<lcMeshLoaderConditionalVertex> mConditionalVertices;

protected:
	lcLibraryMeshData* mMeshData = nullptr;
};

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
	{
		mHasTextures = false;
		mHasStyleStud = false;

		for (lcMeshLoaderTypeData& Data : mData)
			Data.SetMeshData(this);
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

	void SetMeshLoader(lcMeshLoader* MeshLoader)
	{
		mMeshLoader = MeshLoader;
	}

	lcMesh* CreateMesh();
	void AddVertices(lcMeshDataType MeshDataType, int VertexCount, int* BaseVertex, lcMeshLoaderVertex** VertexBuffer);
	void AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, int IndexCount, quint32** IndexBuffer);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);

	lcMeshLoaderMaterial* GetMaterial(quint32 ColorCode);
	lcMeshLoaderMaterial* GetTexturedMaterial(quint32 ColorCode, const lcMeshLoaderTextureMap& TextureMap);

	std::array<lcMeshLoaderTypeData, LC_NUM_MESHDATA_TYPES> mData;
	bool mHasTextures;
	bool mHasStyleStud;

protected:
	lcMeshLoader* mMeshLoader = nullptr;
	std::vector<std::unique_ptr<lcMeshLoaderMaterial>> mMaterials;
	lcArray<lcMeshLoaderTexturedVertex> mTexturedVertices;

	void GenerateTexturedVertices();
	void GeneratePlanarTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data);
	void GenerateCylindricalTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data);
	void GenerateSphericalTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data);
	quint32 AddTexturedVertex(const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoords);

	template<typename IndexType>
	void WriteSections(lcMesh* Mesh, const lcArray<lcMeshLoaderFinalSection> (&FinalSections)[LC_NUM_MESH_LODS], int (&BaseVertices)[LC_NUM_MESHDATA_TYPES], int (&BaseConditionalVertices)[LC_NUM_MESHDATA_TYPES]);

	static void UpdateMeshBoundingBox(lcMesh* Mesh);
	template<typename IndexType>
	static void UpdateMeshSectionBoundingBox(const lcMesh* Mesh, const lcMeshSection& Section, lcVector3& SectionMin, lcVector3& SectionMax);
};

class lcMeshLoader
{
public:
	lcMeshLoader(lcLibraryMeshData& MeshData, bool Optimize, Project* CurrentProject, bool SearchProjectFolder);

	bool LoadMesh(lcFile& File, lcMeshDataType MeshDataType);

	Project* mCurrentProject;
	bool mSearchProjectFolder;

protected:
	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcMeshDataType MeshDataType);

	std::vector<lcMeshLoaderTextureMap> mTextureStack;

	lcLibraryMeshData& mMeshData;
	bool mOptimize;
};
