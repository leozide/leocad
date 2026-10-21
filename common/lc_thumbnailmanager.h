#pragma once

#include "lc_assettypes.h"

struct lcPartThumbnail
{
	QPixmap Pixmap;
	lcPieceData* PieceData;
	std::vector<lcPieceDataRef> RequiredPieceData;
	int ColorIndex;
	int Size;
	float DeviceScale;
	int ReferenceCount;
};

enum class lcPartThumbnailId : uint32_t
{
	Invalid = 0
};

class lcThumbnailManager : public QObject
{
	Q_OBJECT

public:
	lcThumbnailManager(lcPiecesLibrary* Library);
	~lcThumbnailManager();

	std::pair<lcPartThumbnailId, QPixmap> RequestThumbnail(lcPieceData* PieceData, int ColorIndex, int Size, float DeviceScale);
	void ReleaseThumbnail(lcPartThumbnailId ThumbnailId);
	void Clear();
	void RefreshPieces(const std::vector<lcPieceData*>& ChangedPieces);

signals:
	void ThumbnailReady(lcPartThumbnailId ThumbnailId, QPixmap Pixmap);

protected slots:
	void PartLoaded(lcPieceData* PieceData);
	void PartLoadFailed(lcPieceData* PieceData);
	void GeneratedMeshSettled(lcPieceData* PieceData);

protected:
	void RefreshRequiredPieces(lcPartThumbnail& Thumbnail);
	void UpdateThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail);
	void DrawThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail);
	void DrawFailedThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail);
	void SchedulePendingThumbnails();

	lcPiecesLibrary* mLibrary = nullptr;
	std::map<lcPartThumbnailId, lcPartThumbnail> mThumbnails;
	int mNextThumbnailId = 1;

	std::unique_ptr<lcView> mView;
	std::unique_ptr<lcModel> mModel;
	std::map<lcPartThumbnailId, std::unique_ptr<lcModel>> mPendingGeneratedModels;
	bool mRetryScheduled = false;
};
