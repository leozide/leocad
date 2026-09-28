#include "lc_global.h"
#include "lc_thumbnailmanager.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "lc_view.h"
#include "lc_model.h"
#include "project.h"
#include "camera.h"

lcThumbnailManager::lcThumbnailManager(lcPiecesLibrary* Library)
	: QObject(Library), mLibrary(Library)
{
	connect(mLibrary, &lcPiecesLibrary::PartLoaded, this, &lcThumbnailManager::PartLoaded);
	connect(mLibrary, &lcPiecesLibrary::PartLoadFailed, this, &lcThumbnailManager::PartLoadFailed);
}

lcThumbnailManager::~lcThumbnailManager()
{
	Clear();
}

void lcThumbnailManager::Clear()
{
	mView.reset();
	mModel.reset();

	for (auto &[ThumbnailId, Thumbnail] : mThumbnails)
		ReleaseRequiredPieces(Thumbnail);

	mThumbnails.clear();
}

void lcThumbnailManager::RefreshPieces(const std::vector<PieceInfo*>& ChangedPieces)
{
	const std::unordered_set<PieceInfo*> Changed(ChangedPieces.begin(), ChangedPieces.end());
	std::vector<lcPartThumbnailId> ThumbnailIds;

	for (std::map<lcPartThumbnailId, lcPartThumbnail>::value_type& Entry : mThumbnails)
	{
		const lcPartThumbnail& Thumbnail = Entry.second;
		const bool RequiredChanged = std::any_of(Thumbnail.Required.begin(), Thumbnail.Required.end(), [&Changed](PieceInfo* Info)
		{
			return Changed.find(Info) != Changed.end();
		});

		if (Changed.find(Thumbnail.Info) == Changed.end() && !RequiredChanged)
			continue;

		Entry.second.Pixmap = QPixmap();
		ThumbnailIds.push_back(Entry.first);
	}

	for (lcPartThumbnailId ThumbnailId : ThumbnailIds)
	{
		const std::map<lcPartThumbnailId, lcPartThumbnail>::iterator It = mThumbnails.find(ThumbnailId);

		if (It != mThumbnails.end())
			UpdateThumbnail(ThumbnailId, It->second);
	}
}

std::pair<lcPartThumbnailId, QPixmap> lcThumbnailManager::RequestThumbnail(PieceInfo* Info, int ColorIndex, int Size, float DeviceScale)
{
	for (auto &[ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Info == Info && Thumbnail.ColorIndex == ColorIndex && Thumbnail.Size == Size && Thumbnail.DeviceScale == DeviceScale)
			return { ThumbnailId, Thumbnail.Pixmap };

	lcPartThumbnailId ThumbnailId = static_cast<lcPartThumbnailId>(mNextThumbnailId++);
	lcPartThumbnail& Thumbnail = mThumbnails[ThumbnailId];

	Thumbnail.Info = Info;
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
		ReleaseRequiredPieces(Thumbnail);

		mThumbnails.erase(ThumbnailIt);
	}
}

void lcThumbnailManager::RefreshRequiredPieces(lcPartThumbnail& Thumbnail)
{
	std::vector<PieceInfo*> Required{ Thumbnail.Info };

	if (Thumbnail.Info->IsModel())
	{
		std::vector<PieceInfo*> ModelRequired = Thumbnail.Info->GetModel()->GetRequiredPieces();
		Required.insert(Required.end(), ModelRequired.begin(), ModelRequired.end());
	}
	else if (Thumbnail.Info->IsProject())
	{
		const lcModel* Model = Thumbnail.Info->GetProject()->GetMainModel();

		if (Model)
		{
			std::vector<PieceInfo*> ModelRequired = Model->GetRequiredPieces();
			Required.insert(Required.end(), ModelRequired.begin(), ModelRequired.end());
		}
	}

	std::sort(Required.begin(), Required.end());
	Required.erase(std::unique(Required.begin(), Required.end()), Required.end());

	std::unordered_set<PieceInfo*> Previous(Thumbnail.Required.begin(), Thumbnail.Required.end());

	for (PieceInfo* Info : Required)
	{
		if (Previous.erase(Info) == 0)
			mLibrary->LoadPieceInfo(Info, lcPieceLoadFlag::None);
	}

	for (PieceInfo* Info : Previous)
		mLibrary->ReleasePieceInfo(Info);

	Thumbnail.Required = std::move(Required);
}

void lcThumbnailManager::ReleaseRequiredPieces(lcPartThumbnail& Thumbnail)
{
	for (PieceInfo* Info : Thumbnail.Required)
		mLibrary->ReleasePieceInfo(Info);

	Thumbnail.Required.clear();
}

void lcThumbnailManager::UpdateThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail)
{
	RefreshRequiredPieces(Thumbnail);

	bool Pending = false;

	for (const PieceInfo* Info : Thumbnail.Required)
	{
		if (Info->mState == lcPieceInfoState::Failed || Info->mState == lcPieceInfoState::Cancelled)
		{
			DrawFailedThumbnail(ThumbnailId, Thumbnail);
			return;
		}

		if (Info->mState != lcPieceInfoState::Loaded)
			Pending = true;
	}

	if (Pending)
		return;

	if (Thumbnail.Info->IsModel())
	{
		std::vector<lcModel*> UpdatedModels;
		Thumbnail.Info->GetModel()->UpdatePieceInfo(UpdatedModels);
	}
	else if (Thumbnail.Info->IsProject())
		Thumbnail.Info->GetProject()->UpdatePieceInfo(Thumbnail.Info);

	DrawThumbnail(ThumbnailId, Thumbnail);
}

void lcThumbnailManager::PartLoaded(PieceInfo* Info)
{
	std::vector<lcPartThumbnailId> Ready;

	for (const auto& [ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Pixmap.isNull() && std::find(Thumbnail.Required.begin(), Thumbnail.Required.end(), Info) != Thumbnail.Required.end())
			Ready.push_back(ThumbnailId);

	for (lcPartThumbnailId ThumbnailId : Ready)
	{
		const auto It = mThumbnails.find(ThumbnailId);

		if (It != mThumbnails.end() && It->second.Pixmap.isNull())
			UpdateThumbnail(ThumbnailId, It->second);
	}
}

void lcThumbnailManager::PartLoadFailed(PieceInfo* Info)
{
	std::vector<lcPartThumbnailId> Failed;

	for (const auto& [ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Pixmap.isNull() && std::find(Thumbnail.Required.begin(), Thumbnail.Required.end(), Info) != Thumbnail.Required.end())
			Failed.push_back(ThumbnailId);

	for (lcPartThumbnailId ThumbnailId : Failed)
	{
		const auto It = mThumbnails.find(ThumbnailId);

		if (It != mThumbnails.end() && It->second.Pixmap.isNull())
			UpdateThumbnail(ThumbnailId, It->second);
	}
}

void lcThumbnailManager::DrawFailedThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail)
{
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

	ReleaseRequiredPieces(Thumbnail);

	emit ThumbnailReady(ThumbnailId, Thumbnail.Pixmap);
}

void lcThumbnailManager::DrawThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail)
{
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
			return;
		}
	}

	mView->MakeCurrent();
	mView->BindRenderFramebuffer();

	const uint BackgroundColor = QApplication::palette().color(QPalette::Base).rgba();
	mView->SetBackgroundColorOverride(LC_RGBA(qRed(BackgroundColor), qGreen(BackgroundColor), qBlue(BackgroundColor), 0));

	PieceInfo* Info = Thumbnail.Info;
	mModel->SetPreviewPieceInfo(Info, Thumbnail.ColorIndex);

	const lcVector3 Center = (Info->GetBoundingBox().Min + Info->GetBoundingBox().Max) / 2.0f;
	const lcVector3 Position = Center + lcVector3(100.0f, -100.0f, 75.0f);

	mView->GetCamera()->SetViewpoint(Position, Center, lcVector3(0, 0, 1));
	mView->GetCamera()->m_fovy = 20.0f;
	mView->ZoomExtents();

	mView->OnDraw();

	mView->UnbindRenderFramebuffer();

	if (mView->HasMissingAssets())
	{
		RefreshRequiredPieces(Thumbnail);

		for (const PieceInfo* Required : Thumbnail.Required)
			if (Required->mState != lcPieceInfoState::Loaded && Required->mState != lcPieceInfoState::Failed && Required->mState != lcPieceInfoState::Cancelled)
				return;

		DrawFailedThumbnail(ThumbnailId, Thumbnail);
		return;
	}

	QImage Image = mView->GetRenderImage().convertToFormat(QImage::Format_ARGB32);
	const char* IconName = nullptr;

	if (Info->GetSynthInfo())
		IconName = ":/resources/part_flexible.png";
	else if (Info->GetTrainTrackInfo())
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

		Painter.drawImage(QPoint(0, 0), Icon);
		Painter.end();
	}

	Image.setDevicePixelRatio(Thumbnail.DeviceScale);

	float ScaledSize = Thumbnail.Size * Thumbnail.DeviceScale;

	Thumbnail.Pixmap = QPixmap::fromImage(Image).scaled(ScaledSize, ScaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	ReleaseRequiredPieces(Thumbnail);

	emit ThumbnailReady(ThumbnailId, Thumbnail.Pixmap);
}
