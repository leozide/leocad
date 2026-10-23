#include "lc_global.h"
#include "lc_thumbnailmanager.h"
#include "lc_library.h"
#include "lc_piecedata.h"
#include "lc_view.h"
#include "lc_model.h"
#include "piece.h"
#include "project.h"
#include "camera.h"

lcThumbnailManager::lcThumbnailManager(lcPiecesLibrary* Library)
	: QObject(Library), mLibrary(Library)
{
	connect(mLibrary, &lcPiecesLibrary::PartLoaded, this, &lcThumbnailManager::PartLoaded);
	connect(mLibrary, &lcPiecesLibrary::PartLoadFailed, this, &lcThumbnailManager::PartLoadFailed);
	connect(mLibrary, &lcPiecesLibrary::GeneratedMeshSettled, this, &lcThumbnailManager::GeneratedMeshSettled);
}

lcThumbnailManager::~lcThumbnailManager()
{
	Clear();
}

void lcThumbnailManager::Clear()
{
	mView.reset();
	mModel.reset();
	mPendingGeneratedModels.clear();

	mThumbnails.clear();
}

void lcThumbnailManager::RefreshPieces(const std::vector<lcPieceData*>& ChangedPieces)
{
	const std::unordered_set<lcPieceData*> Changed(ChangedPieces.begin(), ChangedPieces.end());
	std::vector<lcPartThumbnailId> ThumbnailIds;

	for (std::map<lcPartThumbnailId, lcPartThumbnail>::value_type& Entry : mThumbnails)
	{
		const lcPartThumbnail& Thumbnail = Entry.second;
		const bool RequiredChanged = std::any_of(Thumbnail.RequiredPieceData.begin(), Thumbnail.RequiredPieceData.end(), [&Changed](const lcPieceDataRef& RequiredData)
		{
			return Changed.find(RequiredData.Get()) != Changed.end();
		});

		if (Changed.find(Thumbnail.PieceData) == Changed.end() && !RequiredChanged)
			continue;

		Entry.second.Pixmap = QPixmap();
		mPendingGeneratedModels.erase(Entry.first);
		ThumbnailIds.push_back(Entry.first);
	}

	for (lcPartThumbnailId ThumbnailId : ThumbnailIds)
	{
		const std::map<lcPartThumbnailId, lcPartThumbnail>::iterator It = mThumbnails.find(ThumbnailId);

		if (It != mThumbnails.end())
			UpdateThumbnail(ThumbnailId, It->second);
	}
}

std::pair<lcPartThumbnailId, QPixmap> lcThumbnailManager::RequestThumbnail(lcPieceData* PieceData, int ColorIndex, int Size, float DeviceScale)
{
	for (auto &[ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.PieceData == PieceData && Thumbnail.ColorIndex == ColorIndex && Thumbnail.Size == Size && Thumbnail.DeviceScale == DeviceScale)
		{
			Thumbnail.ReferenceCount++;

			return { ThumbnailId, Thumbnail.Pixmap };
		}

	lcPartThumbnailId ThumbnailId = static_cast<lcPartThumbnailId>(mNextThumbnailId++);
	lcPartThumbnail& Thumbnail = mThumbnails[ThumbnailId];

	Thumbnail.PieceData = PieceData;
	Thumbnail.ColorIndex = ColorIndex;
	Thumbnail.Size = Size;
	Thumbnail.DeviceScale = DeviceScale;
	Thumbnail.ReferenceCount = 1;

	UpdateThumbnail(ThumbnailId, Thumbnail);

	return { ThumbnailId, Thumbnail.Pixmap };
}

void lcThumbnailManager::ReleaseThumbnail(lcPartThumbnailId ThumbnailId)
{
	auto ThumbnailIt = mThumbnails.find(ThumbnailId);

	if (ThumbnailIt == mThumbnails.end())
		return;

	lcPartThumbnail& Thumbnail = ThumbnailIt->second;

	Thumbnail.ReferenceCount--;

	if (Thumbnail.ReferenceCount == 0)
	{
		mPendingGeneratedModels.erase(ThumbnailId);

		mThumbnails.erase(ThumbnailIt);
	}
}

void lcThumbnailManager::RefreshRequiredPieces(lcPartThumbnail& Thumbnail)
{
	std::vector<lcPieceData*> Required{ Thumbnail.PieceData };

	if (Thumbnail.PieceData->IsModel())
	{
		std::vector<lcPieceDataRef> ModelPieceData = Thumbnail.PieceData->GetModel()->GetRequiredPieceData();
		for (const lcPieceDataRef& ModelData : ModelPieceData)
			Required.push_back(ModelData.Get());
	}
	else if (Thumbnail.PieceData->IsProject())
	{
		const lcModel* Model = Thumbnail.PieceData->GetProject()->GetMainModel();

		if (Model)
		{
			std::vector<lcPieceDataRef> ModelPieceData = Model->GetRequiredPieceData();
			for (const lcPieceDataRef& ModelData : ModelPieceData)
				Required.push_back(ModelData.Get());
		}
	}

	std::sort(Required.begin(), Required.end());
	Required.erase(std::unique(Required.begin(), Required.end()), Required.end());

	std::unordered_map<lcPieceData*, size_t> Previous;

	for (size_t Index = 0; Index < Thumbnail.RequiredPieceData.size(); Index++)
		Previous.emplace(Thumbnail.RequiredPieceData[Index].Get(), Index);

	std::vector<lcPieceDataRef> RequiredPieceData;
	RequiredPieceData.reserve(Required.size());

	for (lcPieceData* PieceData : Required)
	{
		const auto It = Previous.find(PieceData);

		if (It != Previous.end())
			RequiredPieceData.push_back(std::move(Thumbnail.RequiredPieceData[It->second]));
		else
		{
			RequiredPieceData.emplace_back(PieceData);
			mLibrary->RequestPieceDataLoad(PieceData, lcLoadPriority::Low);
		}
	}

	Thumbnail.RequiredPieceData = std::move(RequiredPieceData);
}

void lcThumbnailManager::UpdateThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail)
{
	RefreshRequiredPieces(Thumbnail);

	bool Pending = false;

	for (const lcPieceDataRef& RequiredData : Thumbnail.RequiredPieceData)
	{
		if (RequiredData->mState == lcAssetState::Failed || RequiredData->mState == lcAssetState::Unloaded)
		{
			DrawFailedThumbnail(ThumbnailId, Thumbnail);
			return;
		}

		if (RequiredData->mState != lcAssetState::Loaded)
			Pending = true;
	}

	if (Pending)
		return;

	if (Thumbnail.PieceData->IsModel())
	{
		std::vector<lcModel*> UpdatedModels;
		Thumbnail.PieceData->GetModel()->UpdatePieceData(UpdatedModels);
	}
	else if (Thumbnail.PieceData->IsProject())
		Thumbnail.PieceData->GetProject()->UpdatePieceData(Thumbnail.PieceData);

	DrawThumbnail(ThumbnailId, Thumbnail);
}

void lcThumbnailManager::PartLoaded(lcPieceData* PieceData)
{
	std::vector<lcPartThumbnailId> Ready;

	for (const auto& [ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Pixmap.isNull() && std::any_of(Thumbnail.RequiredPieceData.begin(), Thumbnail.RequiredPieceData.end(), [PieceData](const lcPieceDataRef& RequiredData)
		{
			return RequiredData == PieceData;
		}))
			Ready.push_back(ThumbnailId);

	for (lcPartThumbnailId ThumbnailId : Ready)
	{
		const auto It = mThumbnails.find(ThumbnailId);

		if (It != mThumbnails.end() && It->second.Pixmap.isNull())
			UpdateThumbnail(ThumbnailId, It->second);
	}
}

void lcThumbnailManager::PartLoadFailed(lcPieceData* PieceData)
{
	std::vector<lcPartThumbnailId> Failed;

	for (const auto& [ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Pixmap.isNull() && std::any_of(Thumbnail.RequiredPieceData.begin(), Thumbnail.RequiredPieceData.end(), [PieceData](const lcPieceDataRef& RequiredData)
		{
			return RequiredData == PieceData;
		}))
			Failed.push_back(ThumbnailId);

	for (lcPartThumbnailId ThumbnailId : Failed)
	{
		const auto It = mThumbnails.find(ThumbnailId);

		if (It != mThumbnails.end() && It->second.Pixmap.isNull())
			UpdateThumbnail(ThumbnailId, It->second);
	}
}

void lcThumbnailManager::GeneratedMeshSettled(lcPieceData*)
{
	std::vector<lcPartThumbnailId> Ready;

	for (const std::map<lcPartThumbnailId, std::unique_ptr<lcModel>>::value_type& Entry : mPendingGeneratedModels)
	{
		bool Pending = false;

		for (const lcPiece* Piece : Entry.second->GetRequiredSynthPieces())
			if (Piece->IsGeneratedMeshPending())
			{
				Pending = true;
				break;
			}

		if (!Pending)
			Ready.push_back(Entry.first);
	}

	for (lcPartThumbnailId ThumbnailId : Ready)
	{
		const std::map<lcPartThumbnailId, lcPartThumbnail>::iterator It = mThumbnails.find(ThumbnailId);

		if (It != mThumbnails.end())
			UpdateThumbnail(It->first, It->second);
	}
}

void lcThumbnailManager::SchedulePendingThumbnails()
{
	if (mRetryScheduled)
		return;

	mRetryScheduled = true;

	QTimer::singleShot(0, this, [this]()
	{
		mRetryScheduled = false;
		std::vector<lcPartThumbnailId> Pending;

		for (const std::map<lcPartThumbnailId, lcPartThumbnail>::value_type& Entry : mThumbnails)
			if (Entry.second.Pixmap.isNull())
				Pending.push_back(Entry.first);

		for (lcPartThumbnailId ThumbnailId : Pending)
		{
			const std::map<lcPartThumbnailId, lcPartThumbnail>::iterator It = mThumbnails.find(ThumbnailId);

			if (It != mThumbnails.end() && It->second.Pixmap.isNull())
				UpdateThumbnail(It->first, It->second);
		}
	});
}

void lcThumbnailManager::DrawFailedThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail)
{
	mPendingGeneratedModels.erase(ThumbnailId);

	const int Size = qMax(1, static_cast<int>(Thumbnail.Size * Thumbnail.DeviceScale));

	Thumbnail.Pixmap = QPixmap(Size, Size);
	Thumbnail.Pixmap.fill(Qt::transparent);

	QPainter Painter(&Thumbnail.Pixmap);
	QPen Pen(QColor(180, 40, 40));

	Pen.setWidth(qMax(2, Size / 12));
	Painter.setPen(Pen);

	const int Margin = qMax(3, Size / 5);

	Painter.drawLine(Margin, Margin, Size - Margin, Size - Margin);
	Painter.drawLine(Size - Margin, Margin, Margin, Size - Margin);
	Painter.end();

	Thumbnail.Pixmap.setDevicePixelRatio(Thumbnail.DeviceScale);

	Thumbnail.RequiredPieceData.clear();

	emit ThumbnailReady(ThumbnailId, Thumbnail.Pixmap);
	SchedulePendingThumbnails();
}

void lcThumbnailManager::DrawThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail)
{
	const std::map<lcPartThumbnailId, std::unique_ptr<lcModel>>::iterator PendingIt = mPendingGeneratedModels.find(ThumbnailId);
	const bool ReuseGeneratedPiece = PendingIt != mPendingGeneratedModels.end();

	if (ReuseGeneratedPiece)
	{
		for (const lcPiece* Piece : PendingIt->second->GetRequiredSynthPieces())
			if (Piece->IsGeneratedMeshPending())
				return;

		mView.reset();
		mModel = std::move(PendingIt->second);
		mPendingGeneratedModels.erase(PendingIt);
	}

	const int Width = Thumbnail.Size * 2 * Thumbnail.DeviceScale;
	const int Height = Thumbnail.Size * 2 * Thumbnail.DeviceScale;

	if (mView && mView->GetRenderImage().size() != QSize(Width, Height))
		mView.reset();

	if (!mView)
	{
		if (!mModel)
			mModel = std::unique_ptr<lcModel>(new lcModel(QString(), nullptr, true));
		mView = std::unique_ptr<lcView>(new lcView(lcViewType::PartsList, mModel.get()));

		mView->SetOffscreenContext();
		mView->MakeCurrent();
		mView->SetSize(Width, Height);

		if (!mView->BeginRenderToImage(Width, Height))
		{
			mView.reset();
			DrawFailedThumbnail(ThumbnailId, Thumbnail);
			return;
		}
	}

	mView->MakeCurrent();
	mView->BindRenderFramebuffer();

	const uint BackgroundColor = QApplication::palette().color(QPalette::Base).rgba();
	mView->SetBackgroundColorOverride(LC_RGBA(qRed(BackgroundColor), qGreen(BackgroundColor), qBlue(BackgroundColor), 0));

	lcPieceData* PieceData = Thumbnail.PieceData;

	if (!ReuseGeneratedPiece)
	{
		mModel->SetPreviewPieceData(PieceData, Thumbnail.ColorIndex);

		bool GeneratedPiecePending = false;

		for (const lcPiece* Piece : mModel->GetRequiredSynthPieces())
			if (Piece->IsGeneratedMeshPending())
			{
				GeneratedPiecePending = true;
				break;
			}

		if (GeneratedPiecePending)
		{
			mView->UnbindRenderFramebuffer();
			mView.reset();
			mPendingGeneratedModels.emplace(ThumbnailId, std::move(mModel));
			return;
		}
	}

	const lcVector3 Center = (PieceData->GetBoundingBox().Min + PieceData->GetBoundingBox().Max) / 2.0f;
	const lcVector3 Position = Center + lcVector3(100.0f, -100.0f, 75.0f);

	mView->GetCamera()->SetViewpoint(Position, Center, lcVector3(0, 0, 1));
	mView->GetCamera()->m_fovy = 20.0f;
	mView->ZoomExtents(lcGeometryBoundsMode::IncludeFallback, false);

	mView->OnDraw();

	mView->UnbindRenderFramebuffer();

	if (mView->HasMissingAssets())
	{
		RefreshRequiredPieces(Thumbnail);

		for (const lcPieceDataRef& RequiredData : Thumbnail.RequiredPieceData)
			if (RequiredData->IsLoading())
				return;

		DrawFailedThumbnail(ThumbnailId, Thumbnail);
		return;
	}

	QImage Image = mView->GetRenderImage().convertToFormat(QImage::Format_ARGB32);

	if (Image.isNull())
	{
		DrawFailedThumbnail(ThumbnailId, Thumbnail);
		return;
	}

	const char* IconName = nullptr;

	if (PieceData->GetSynthInfo())
		IconName = ":/resources/part_flexible.png";
	else if (PieceData->GetTrainTrackInfo())
		IconName = ":/resources/part_traintrack.png";

	if (IconName)
	{
		QPainter Painter(&Image);
		QImage Icon = QImage(IconName);
		uchar* ImageBits = Icon.bits();
		QRgb TextColor = QApplication::palette().color(QPalette::WindowText).rgba();
		int Red = qRed(TextColor);
		int Green = qGreen(TextColor);
		int Blue = qBlue(TextColor);

		for (int y = 0; y < Icon.height(); y++)
		{
			for (int x = 0; x < Icon.width(); x++)
			{
				QRgb& Pixel = ((QRgb*)ImageBits)[x];
				Pixel = qRgba(Red, Green, Blue, qAlpha(Pixel));
			}

			ImageBits += Icon.bytesPerLine();
		}

		Painter.setRenderHint(QPainter::SmoothPixmapTransform);
		Painter.scale(Thumbnail.DeviceScale, Thumbnail.DeviceScale);
		Painter.drawImage(QPoint(0, 0), Icon);
		Painter.end();
	}

	Image.setDevicePixelRatio(Thumbnail.DeviceScale);

	float ScaledSize = Thumbnail.Size * Thumbnail.DeviceScale;

	Thumbnail.Pixmap = QPixmap::fromImage(Image).scaled(ScaledSize, ScaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	Thumbnail.RequiredPieceData.clear();

	emit ThumbnailReady(ThumbnailId, Thumbnail.Pixmap);
	SchedulePendingThumbnails();
}
