#ifndef _LC_LIBRARY_H_
#define _LC_LIBRARY_H_

#include "lc_mesh.h"
#include "array.h"

class PieceInfo;
class lcZipFile;

class lcLibraryMeshSection
{
public:
	lcLibraryMeshSection()
		: mIndices(1024, 1024)
	{
	}

	~lcLibraryMeshSection()
	{
	}

	lcuint32 mColorCode;
	bool mTriangles;
	ObjArray<lcuint32> mIndices;
};

class lcLibraryMeshData
{
public:
	lcLibraryMeshData()
		: mVertices(1024, 1024)
	{
	}

	~lcLibraryMeshData()
	{
		for (int SectionIdx = 0; SectionIdx < mSections.GetSize(); SectionIdx++)
			delete mSections[SectionIdx];
	}

	void AddLine(int LineType, lcuint32 ColorCode, const lcVector3* Vertices);
	void AddMeshData(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode);
	void AddMeshDataNoDuplicateCheck(const lcLibraryMeshData& Data, const lcMatrix44& Transform, lcuint32 CurrentColorCode);

	PtrArray<lcLibraryMeshSection> mSections;
	ObjArray<lcVector3> mVertices;
};

class lcLibraryPrimitive
{
public:
	char mName[LC_MAXPATH];
	lcuint32 mZipFileIndex;
	bool mLoaded;
	bool mStud;
	bool mSubFile;
	lcLibraryMeshData mMeshData;
};

class lcPiecesLibrary
{
public:
	lcPiecesLibrary();
	~lcPiecesLibrary();

	bool OpenArchive(const char* FileName);


	bool LoadPiece(const char* PieceName);
	bool LoadPiece(int PieceIndex);
	int FindPrimitiveIndex(const char* Name);
	bool LoadPrimitive(int PrimitiveIndex);
	bool ReadMeshData(lcFile& File, const lcMatrix44& CurrentTransform, lcuint32 CurrentColorCode, lcLibraryMeshData& MeshData);

	PtrArray<PieceInfo> mPieces;
	PtrArray<lcLibraryPrimitive> mPrimitives;

protected:
	lcZipFile* mZipFile;
};

#endif // _LC_LIBRARY_H_
