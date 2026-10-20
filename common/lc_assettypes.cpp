#include "lc_global.h"
#include "lc_assettypes.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_mesh.h"
#include "lc_piecedata.h"
#include "lc_texture.h"

template<>
void lcAssetRef<lcPieceData>::Retain(lcPieceData* Asset, lcPiecesLibrary* Library)
{
	Library->AddPieceDataReference(Asset);
}

template<>
void lcAssetRef<lcPieceData>::Release(lcPieceData* Asset, lcPiecesLibrary* Library)
{
	Library->ReleasePieceData(Asset);
}

template<>
void lcAssetRef<lcTexture>::Retain(lcTexture* Asset, lcPiecesLibrary* Library)
{
	Library->AddTextureReference(Asset);
}

template<>
void lcAssetRef<lcTexture>::Release(lcTexture* Asset, lcPiecesLibrary* Library)
{
	Library->ReleaseTexture(Asset);
}

template<typename T>
lcAssetRef<T>::lcAssetRef(T* Asset)
	: lcAssetRef(Asset, Asset ? lcGetPiecesLibrary() : nullptr)
{
}

template<typename T>
lcAssetRef<T>::lcAssetRef(T* Asset, lcPiecesLibrary* Library)
	: mAsset(Asset), mLibrary(Library)
{
	if (mAsset)
	{
		Q_ASSERT(mLibrary && QThread::currentThread() == mLibrary->thread());
		Retain(mAsset, mLibrary);
	}
}

template<typename T>
lcAssetRef<T>::lcAssetRef(const lcAssetRef& OtherRef)
	: lcAssetRef(OtherRef.mAsset, OtherRef.mLibrary)
{
}

template<typename T>
lcAssetRef<T>::lcAssetRef(lcAssetRef&& OtherRef) noexcept
	: mAsset(std::exchange(OtherRef.mAsset, nullptr)), mLibrary(std::exchange(OtherRef.mLibrary, nullptr))
{
}

template<typename T>
lcAssetRef<T>::~lcAssetRef()
{
	if (mAsset)
	{
		Q_ASSERT(mLibrary && QThread::currentThread() == mLibrary->thread());
		Release(mAsset, mLibrary);
	}
}

template<typename T>
lcAssetRef<T>& lcAssetRef<T>::operator=(const lcAssetRef& OtherRef)
{
	if (this != &OtherRef)
	{
		lcAssetRef CopyRef(OtherRef);
		Swap(CopyRef);
	}

	return *this;
}

template<typename T>
lcAssetRef<T>& lcAssetRef<T>::operator=(lcAssetRef&& OtherRef) noexcept
{
	if (this != &OtherRef)
	{
		lcAssetRef MovedRef(std::move(OtherRef));
		Swap(MovedRef);
	}

	return *this;
}

template<typename T>
bool lcAssetRef<T>::HasOtherReferences() const
{
	Q_ASSERT(!mAsset || (mLibrary && QThread::currentThread() == mLibrary->thread()));

	return mAsset && mAsset->GetRefCount() > 1;
}

template<typename T>
void lcAssetRef<T>::Reset()
{
	lcAssetRef EmptyRef;
	Swap(EmptyRef);
}

template<typename T>
T* lcAssetRef<T>::Release()
{
	mLibrary = nullptr;
	return std::exchange(mAsset, nullptr);
}

template<typename T>
void lcAssetRef<T>::Swap(lcAssetRef& OtherRef) noexcept
{
	std::swap(mAsset, OtherRef.mAsset);
	std::swap(mLibrary, OtherRef.mLibrary);
}

template<typename T>
lcAssetRef<T> lcAssetRef<T>::Adopt(T* Asset, lcPiecesLibrary* Library)
{
	Q_ASSERT(!Asset || (Library && QThread::currentThread() == Library->thread()));

	lcAssetRef AssetRef;
	AssetRef.mAsset = Asset;
	AssetRef.mLibrary = Library;
	return AssetRef;
}

template class lcAssetRef<lcPieceData>;
template class lcAssetRef<lcTexture>;

lcPieceSourceSnapshot::lcPieceSourceSnapshot()
	: ZipFileType(lcZipFileType::Count), StudStyle(lcStudStyle::Plain)
{
}

lcPieceBuildResult::lcPieceBuildResult() = default;
lcPieceBuildResult::~lcPieceBuildResult() = default;
lcPieceBuildResult::lcPieceBuildResult(lcPieceBuildResult&&) noexcept = default;
lcPieceBuildResult& lcPieceBuildResult::operator=(lcPieceBuildResult&&) noexcept = default;

lcTextureBuildResult::~lcTextureBuildResult() = default;
