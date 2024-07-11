#pragma once

class lcPiecesLibrary;

struct lcPartThumbnail
{
	QPixmap Pixmap;
	PieceInfo* Info;
	int ColorIndex;
	int Size;
	int ReferenceCount;
};

enum class lcPartThumbnailId : uint64_t
{
	Invalid = 0
};

class lcThumbnailManager : public QObject
{
	Q_OBJECT

public:
	lcThumbnailManager(lcPiecesLibrary* Library);
	~lcThumbnailManager();

	std::pair<lcPartThumbnailId, QPixmap> RequestThumbnail(PieceInfo* Info, int ColorIndex, int Size);
	void ReleaseThumbnail(lcPartThumbnailId ThumbnailId);

signals:
	void ThumbnailReady(lcPartThumbnailId ThumbnailId, QPixmap Pixmap);

protected slots:
	void PartLoaded(PieceInfo* Info);

protected:
	void DrawThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail);

	lcPiecesLibrary* mLibrary = nullptr;
	std::map<lcPartThumbnailId, lcPartThumbnail> mThumbnails;
	int mNextThumbnailId = 1;

	std::unique_ptr<lcView> mView;
	std::unique_ptr<lcModel> mModel;
};
