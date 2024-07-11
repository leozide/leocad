#include "lc_global.h"
#include "lc_thumbnailmanager.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "lc_view.h"
#include "lc_model.h"
#include "camera.h"

lcThumbnailManager::lcThumbnailManager(lcPiecesLibrary* Library)
	: QObject(Library), mLibrary(Library)
{
	connect(mLibrary, &lcPiecesLibrary::PartLoaded, this, &lcThumbnailManager::PartLoaded);
}

lcThumbnailManager::~lcThumbnailManager()
{
	for (auto &[ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Pixmap.isNull())
			mLibrary->ReleasePieceInfo(Thumbnail.Info);
}

std::pair<lcPartThumbnailId, QPixmap> lcThumbnailManager::RequestThumbnail(PieceInfo* Info, int ColorIndex, int Size)
{
	for (auto &[ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Info == Info && Thumbnail.ColorIndex == ColorIndex && Thumbnail.Size == Size)
			return { ThumbnailId, Thumbnail.Pixmap };

	lcPartThumbnailId ThumbnailId = static_cast<lcPartThumbnailId>(mNextThumbnailId++);
	lcPartThumbnail& Thumbnail = mThumbnails[ThumbnailId];

	Thumbnail.Info = Info;
	Thumbnail.ColorIndex = ColorIndex;
	Thumbnail.Size = Size;
	Thumbnail.ReferenceCount = 1;

	mLibrary->LoadPieceInfo(Info, false, false);

	if (Info->mState == lcPieceInfoState::Loaded)
		DrawThumbnail(ThumbnailId, Thumbnail);

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
		if (Thumbnail.Pixmap.isNull())
			mLibrary->ReleasePieceInfo(Thumbnail.Info);

		mThumbnails.erase(ThumbnailIt);
	}
}

void lcThumbnailManager::PartLoaded(PieceInfo* Info)
{
	for (auto& [ThumbnailId, Thumbnail] : mThumbnails)
		if (Thumbnail.Info == Info && Thumbnail.Pixmap.isNull())
			DrawThumbnail(ThumbnailId, Thumbnail);
}

void lcThumbnailManager::DrawThumbnail(lcPartThumbnailId ThumbnailId, lcPartThumbnail& Thumbnail)
{
	const int Width = Thumbnail.Size * 2;
	const int Height = Thumbnail.Size * 2;

	if (mView && (mView->GetWidth() != Width || mView->GetHeight() != Height))
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

	QImage Image = mView->GetRenderFramebufferImage().convertToFormat(QImage::Format_ARGB32);

	if (Info->GetSynthInfo())
	{
		QPainter Painter(&Image);
		QImage Icon = QImage(":/resources/flexible.png");
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

	Thumbnail.Pixmap = QPixmap::fromImage(Image).scaled(Thumbnail.Size, Thumbnail.Size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	mLibrary->ReleasePieceInfo(Info);

	emit ThumbnailReady(ThumbnailId, Thumbnail.Pixmap);
}
