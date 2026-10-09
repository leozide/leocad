#pragma once

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

struct lcMeshLoaderTextureParams
{
	lcMeshLoaderMaterialType Type = lcMeshLoaderMaterialType::Solid;
	lcVector3 Points[3] = {};
	float Angles[2] = {};
	char Name[LC_MAXPATH] = {};
};

struct lcMeshLoaderMaterial : public lcMeshLoaderTextureParams
{
	quint32 Color = 16;
};

class lcMeshLoaderSection
{
public:
	lcMeshLoaderSection(lcMeshPrimitiveType PrimitiveType, lcMeshLoaderMaterial* Material)
		: mMaterial(Material), mPrimitiveType(PrimitiveType)
	{
		mIndices.reserve(1024);
	}

	lcMeshLoaderMaterial* mMaterial;
	lcMeshPrimitiveType mPrimitiveType;
	std::vector<quint32> mIndices;
};

struct lcMeshLoaderFinalSection
{
	quint32 Color;
	lcMeshPrimitiveType PrimitiveType;
	char Name[256];
};

struct lcMeshLoaderTextureMap : public lcMeshLoaderTextureParams
{
	bool Fallback = false;
	bool Next = false;
};

class lcMeshLoaderTypeData
{
public:
	lcMeshLoaderTypeData() = default;

	lcMeshLoaderTypeData(const lcMeshLoaderTypeData&) = delete;
	lcMeshLoaderTypeData& operator=(const lcMeshLoaderTypeData&) = delete;

	bool IsEmpty() const
	{
		return mSections.empty();
	}

	void Clear()
	{
		mSections.clear();
		mVertices.clear();
		mConditionalVertices.clear();
	}

	void SetMeshData(lcLibraryMeshData* MeshData)
	{
		mMeshData = MeshData;
	}

	lcMeshLoaderSection* AddSection(lcMeshPrimitiveType PrimitiveType, lcMeshLoaderMaterial* Material);

	quint32 AddVertex(const lcVector3& Position, bool Optimize);
	quint32 AddVertex(const lcVector3& Position, const lcVector3& Normal, float NormalWeight, bool Optimize);
	quint32 AddConditionalVertex(const lcVector3 (&Position)[4]);

	void ProcessLine(int LineType, lcMeshLoaderMaterial* Material, bool WindingCCW, lcVector3 (&Vertices)[4], bool Optimize);

	void AddMeshData(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap);
	void AddMeshDataNoDuplicateCheck(const lcMeshLoaderTypeData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap);

	std::vector<std::unique_ptr<lcMeshLoaderSection>> mSections;
	std::vector<lcMeshLoaderVertex> mVertices;
	std::vector<lcMeshLoaderConditionalVertex> mConditionalVertices;

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

		mMaterials.clear();
		mTexturedVertices.clear();
		mHasTextures = false;
		mHasStyleStud = false;
	}

	void SetMeshLoader(lcMeshLoader* MeshLoader)
	{
		mMeshLoader = MeshLoader;
	}

	lcMesh* CreateMesh(const std::function<lcTexture*(const char*)>& TextureLookup = {});
	void ResolveColors();
	lcMesh* CreateMeshResolved(const std::vector<bool>& ColorTranslucency, int DefaultColorIndex, const std::atomic_bool* Cancelled);
	QStringList GetTextureDependencies() const;
	void AddVertices(lcMeshDataType MeshDataType, size_t VertexCount, int* BaseVertex, lcMeshLoaderVertex** VertexBuffer);
	void AddIndices(lcMeshDataType MeshDataType, lcMeshPrimitiveType PrimitiveType, quint32 ColorCode, size_t IndexCount, quint32** IndexBuffer);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, quint32 CurrentColorCode, bool InvertWinding, bool InvertNormals, lcMeshLoaderTextureMap* TextureMap, lcMeshDataType OverrideDestIndex);

	lcMeshLoaderMaterial* GetMaterial(quint32 ColorCode);
	lcMeshLoaderMaterial* GetTexturedMaterial(quint32 ColorCode, const lcMeshLoaderTextureParams& TextureMap);

	std::array<lcMeshLoaderTypeData, LC_NUM_MESHDATA_TYPES> mData;
	bool mHasTextures;
	bool mHasStyleStud;

protected:
	lcMeshLoader* mMeshLoader = nullptr;
	std::vector<std::unique_ptr<lcMeshLoaderMaterial>> mMaterials;
	std::vector<lcMeshLoaderTexturedVertex> mTexturedVertices;

	void GenerateTexturedVertices();
	void GeneratePlanarTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data);
	void GenerateCylindricalTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data);
	void GenerateSphericalTexcoords(lcMeshLoaderSection* Section, const lcMeshLoaderTypeData& Data);
	quint32 AddTexturedVertex(const lcVector3& Position, const lcVector3& Normal, const lcVector2& TexCoords);

	template<typename IndexType>
	bool WriteSections(lcMesh* Mesh, const std::vector<lcMeshLoaderFinalSection> (&FinalSections)[LC_NUM_MESH_LODS], int (&BaseVertices)[LC_NUM_MESHDATA_TYPES], const std::function<lcTexture*(const char*)>& TextureLookup, const std::vector<bool>& ColorTranslucency, int DefaultColorIndex, const std::atomic_bool* Cancelled);
	lcMesh* CreateMeshInternal(const std::function<lcTexture*(const char*)>& TextureLookup, const std::vector<bool>& ColorTranslucency, int DefaultColorIndex, const std::atomic_bool* Cancelled);

	static bool UpdateMeshBoundingBox(lcMesh* Mesh, const std::atomic_bool* Cancelled);
	template<typename IndexType>
	static bool UpdateMeshSectionBoundingBox(const lcMesh* Mesh, const lcMeshSection& Section, lcVector3& SectionMin, lcVector3& SectionMax, const std::atomic_bool* Cancelled);
};

enum class lcMeshLoaderFlag
{
	None = 0,
	Optimize = 1 << 0,
	SearchProjectFolder = 1 << 1,
	RequireAllIncludes = 1 << 2
};

Q_DECLARE_FLAGS(lcMeshLoaderFlags, lcMeshLoaderFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(lcMeshLoaderFlags)

class lcMeshLoader
{
public:
	lcMeshLoader(lcLibraryMeshData& MeshData, Project* CurrentProject, lcMeshLoaderFlags Flags);

	bool LoadMesh(lcFile& File, lcMeshDataType MeshDataType);
	static std::vector<std::string> GetTextureReferences(const QByteArray& FileData);
	const QString& GetError() const { return mError; }
	void SetCancellationFlag(const std::atomic_bool* Cancelled) { mCancelled = Cancelled; }
	bool SearchProjectFolder() const { return mFlags.testFlag(lcMeshLoaderFlag::SearchProjectFolder); }

	Project* mCurrentProject;

protected:
	static bool ParseTextureName(const char* Text, char* Name, size_t NameSize, const char** EndOfName);
	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, quint32 CurrentColorCode, bool InvertWinding, lcMeshDataType MeshDataType);

	std::vector<lcMeshLoaderTextureMap> mTextureStack;

	lcLibraryMeshData& mMeshData;
	lcMeshLoaderFlags mFlags;
	const std::atomic_bool* mCancelled = nullptr;
	QString mError;

	Q_DECLARE_TR_FUNCTIONS(lcMeshLoader)
};
