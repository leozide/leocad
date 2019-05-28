#include "lc_global.h"
#include "lc_partselectionwidget.h"
#include "lc_profile.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "lc_library.h"
#include "lc_model.h"
#include "project.h"
#include "pieceinf.h"
#include "view.h"
#include "lc_glextensions.h"

 Q_DECLARE_METATYPE(QList<int>)

void lcPartSelectionItemDelegate::paint(QPainter* Painter, const QStyleOptionViewItem& Option, const QModelIndex& Index) const
{
	mListModel->RequestPreview(Index.row());
	QStyledItemDelegate::paint(Painter, Option, Index);
}

QSize lcPartSelectionItemDelegate::sizeHint(const QStyleOptionViewItem& Option, const QModelIndex& Index) const
{
	QSize Size = QStyledItemDelegate::sizeHint(Option, Index);
	int IconSize = mListModel->GetIconSize();

	if (IconSize)
	{
		QWidget* Widget = (QWidget*)parent();
		const int PixmapMargin = Widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &Option, Widget) + 1;
		int PixmapWidth = IconSize + 2 * PixmapMargin;
		Size.setWidth(qMin(PixmapWidth, Size.width()));
	}

	return Size;
}

lcPartSelectionListModel::lcPartSelectionListModel(QObject* Parent)
	: QAbstractListModel(Parent)
{
	mListView = (lcPartSelectionListView*)Parent;
	mIconSize = 0;
	mShowPartNames = lcGetProfileInt(LC_PROFILE_PARTS_LIST_NAMES);
	mListMode = lcGetProfileInt(LC_PROFILE_PARTS_LIST_LISTMODE);
	mShowDecoratedParts = lcGetProfileInt(LC_PROFILE_PARTS_LIST_DECORATED);

	int ColorCode = lcGetProfileInt(LC_PROFILE_PARTS_LIST_COLOR);
	if (ColorCode == -1)
	{
		mColorIndex = gMainWindow->mColorIndex;
		mColorLocked = false;
	}
	else
	{
		mColorIndex = lcGetColorIndex(ColorCode);
		mColorLocked = true;
	}

	connect(lcGetPiecesLibrary(), SIGNAL(PartLoaded(PieceInfo*)), this, SLOT(PartLoaded(PieceInfo*)));
}

lcPartSelectionListModel::~lcPartSelectionListModel()
{
	ClearRequests();
}

void lcPartSelectionListModel::ClearRequests()
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	for (int RequestIdx : mRequestedPreviews)
	{
		PieceInfo* Info = mParts[RequestIdx].first;
		Library->ReleasePieceInfo(Info);
	}

	mRequestedPreviews.clear();
}

void lcPartSelectionListModel::Redraw()
{
	ClearRequests();

	beginResetModel();

	for (size_t PartIdx = 0; PartIdx < mParts.size(); PartIdx++)
		mParts[PartIdx].second = QPixmap();

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetColorIndex(int ColorIndex)
{
	if (mColorLocked || ColorIndex == mColorIndex)
		return;

	mColorIndex = ColorIndex;
	Redraw();
}

void lcPartSelectionListModel::ToggleColorLocked()
{
	mColorLocked = !mColorLocked;

	SetColorIndex(gMainWindow->mColorIndex);
	lcSetProfileInt(LC_PROFILE_PARTS_LIST_COLOR, mColorLocked ? lcGetColorCode(mColorIndex) : -1);
}

void lcPartSelectionListModel::ToggleListMode()
{
	mListMode = !mListMode;

	mListView->UpdateViewMode();
	lcSetProfileInt(LC_PROFILE_PARTS_LIST_LISTMODE, mListMode);
}

void lcPartSelectionListModel::SetCategory(int CategoryIndex)
{
	ClearRequests();

	beginResetModel();

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	lcArray<PieceInfo*> SingleParts, GroupedParts;

	if (CategoryIndex != -1)
		Library->GetCategoryEntries(CategoryIndex, false, SingleParts, GroupedParts);
	else
	{
		Library->GetParts(SingleParts);

		lcModel* ActiveModel = gMainWindow->GetActiveModel();

		for (int PartIdx = 0; PartIdx < SingleParts.GetSize(); )
		{
			PieceInfo* Info = SingleParts[PartIdx];

			if (!Info->IsModel() || !Info->GetModel()->IncludesModel(ActiveModel))
				PartIdx++;
			else
				SingleParts.RemoveIndex(PartIdx);
		}
	}

	auto lcPartSortFunc=[](PieceInfo* const& a, PieceInfo* const& b)
	{
		return strcmp(a->m_strDescription, b->m_strDescription);
	};

	SingleParts.Sort(lcPartSortFunc);
	mParts.resize(SingleParts.GetSize());

	for (int PartIdx = 0; PartIdx < SingleParts.GetSize(); PartIdx++)
		mParts[PartIdx] = QPair<PieceInfo*, QPixmap>(SingleParts[PartIdx], QPixmap());

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetModelsCategory()
{
	ClearRequests();

	beginResetModel();

	mParts.clear();

	const lcArray<lcModel*>& Models = lcGetActiveProject()->GetModels();
	lcModel* ActiveModel = gMainWindow->GetActiveModel();

	for (int ModelIdx = 0; ModelIdx < Models.GetSize(); ModelIdx++)
	{
		lcModel* Model = Models[ModelIdx];

		if (!Model->IncludesModel(ActiveModel))
			mParts.emplace_back(QPair<PieceInfo*, QPixmap>(Model->GetPieceInfo(), QPixmap()));
	}

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetCurrentModelCategory()
{
	ClearRequests();

	beginResetModel();

	mParts.clear();

	lcModel* ActiveModel = gMainWindow->GetActiveModel();
	lcPartsList PartsList;
	ActiveModel->GetPartsList(gDefaultColor, true, PartsList);

	for (const auto& PartIt : PartsList)
		mParts.emplace_back(QPair<PieceInfo*, QPixmap>((PieceInfo*)PartIt.first, QPixmap()));

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetFilter(const QString& Filter)
{
	mFilter = Filter.toLatin1();

	for (size_t PartIdx = 0; PartIdx < mParts.size(); PartIdx++)
	{
		PieceInfo* Info = mParts[PartIdx].first;
		bool Visible;

		if (!mShowDecoratedParts && Info->IsPatterned())
			Visible = false;
		else if (mFilter.isEmpty())
			Visible = true;
		else
		{
			char Description[sizeof(Info->m_strDescription)];
			char* Src = Info->m_strDescription;
			char* Dst = Description;

			for (;;)
			{
				*Dst = *Src;

				if (*Src == ' ' && *(Src + 1) == ' ')
					Src++;
				else if (*Src == 0)
					break;

				Src++;
				Dst++;
			}

			Visible = strcasestr(Description, mFilter) || strcasestr(Info->mFileName, mFilter);
		}

		mListView->setRowHidden(PartIdx, !Visible);
	}
}

int lcPartSelectionListModel::rowCount(const QModelIndex& Parent) const
{
	Q_UNUSED(Parent);

	return (int)mParts.size();
}

QVariant lcPartSelectionListModel::data(const QModelIndex& Index, int Role) const
{
	size_t InfoIndex = Index.row();

	if (Index.isValid() && InfoIndex < mParts.size())
	{
		PieceInfo* Info = mParts[InfoIndex].first;

		switch (Role)
		{
		case Qt::DisplayRole:
			if (!mIconSize || mShowPartNames || mListMode)
				return QVariant(QString::fromLatin1(Info->m_strDescription));
			break;

		case Qt::ToolTipRole:
			return QVariant(QString("%1 (%2)").arg(QString::fromLatin1(Info->m_strDescription), QString::fromLatin1(Info->mFileName)));

		case Qt::DecorationRole:
			if (!mParts[InfoIndex].second.isNull() && mIconSize)
				return QVariant(mParts[InfoIndex].second);
			else
				return QVariant(QColor(0, 0, 0, 0));

		default:
			break;
		}
	}

	return QVariant();
}

QVariant lcPartSelectionListModel::headerData(int Section, Qt::Orientation Orientation, int Role) const
{
	Q_UNUSED(Section);
	Q_UNUSED(Orientation);

	return Role == Qt::DisplayRole ? QVariant(QLatin1String("Image")) : QVariant();
}

Qt::ItemFlags lcPartSelectionListModel::flags(const QModelIndex& Index) const
{
	Qt::ItemFlags DefaultFlags = QAbstractListModel::flags(Index);

	if (Index.isValid())
		return Qt::ItemIsDragEnabled | DefaultFlags;
	else
		return DefaultFlags;
}

void lcPartSelectionListModel::RequestPreview(int InfoIndex)
{
	if (!mIconSize || !mParts[InfoIndex].second.isNull())
		return;

	if (std::find(mRequestedPreviews.begin(), mRequestedPreviews.end(), InfoIndex) != mRequestedPreviews.end())
		return;

	PieceInfo* Info = mParts[InfoIndex].first;
	lcGetPiecesLibrary()->LoadPieceInfo(Info, false, false);

	if (Info->mState == LC_PIECEINFO_LOADED)
		DrawPreview(InfoIndex);
	else
		mRequestedPreviews.push_back(InfoIndex);
}

void lcPartSelectionListModel::PartLoaded(PieceInfo* Info)
{
	for (size_t PartIdx = 0; PartIdx < mParts.size(); PartIdx++)
	{
		if (mParts[PartIdx].first == Info)
		{
			auto PreviewIt = std::find(mRequestedPreviews.begin(), mRequestedPreviews.end(), PartIdx);
			if (PreviewIt != mRequestedPreviews.end())
			{
				mRequestedPreviews.erase(PreviewIt);
				DrawPreview(PartIdx);
			}
			break;
		}
	}
}

void lcPartSelectionListModel::DrawPreview(int InfoIndex)
{
	View* ActiveView = gMainWindow->GetActiveView();
	if (!ActiveView)
		return;

	ActiveView->MakeCurrent();
	lcContext* Context = ActiveView->mContext;
	int Width = mIconSize * 2;
	int Height = mIconSize * 2;

	if (mRenderFramebuffer.first.mWidth != Width || mRenderFramebuffer.first.mHeight != Height)
	{
		Context->DestroyRenderFramebuffer(mRenderFramebuffer);
		mRenderFramebuffer = Context->CreateRenderFramebuffer(Width, Height);
	}

	if (!mRenderFramebuffer.first.IsValid())
		return;

	float Aspect = (float)Width / (float)Height;
	Context->SetViewport(0, 0, Width, Height);

	Context->SetDefaultState();
	Context->BindFramebuffer(mRenderFramebuffer.first);

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = mParts[InfoIndex].first;

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	lcMatrix44 ProjectionMatrix, ViewMatrix;

	Info->ZoomExtents(20.0f, Aspect, ProjectionMatrix, ViewMatrix);

	Context->SetProjectionMatrix(ProjectionMatrix);

	lcScene Scene;
	Scene.SetAllowWireframe(false);
	Scene.Begin(ViewMatrix);

	Info->AddRenderMeshes(Scene, lcMatrix44Identity(), mColorIndex, lcRenderMeshState::NORMAL, false);

	Scene.End();

	Scene.Draw(Context);

	mParts[InfoIndex].second = QPixmap::fromImage(Context->GetRenderFramebufferImage(mRenderFramebuffer)).scaled(mIconSize, mIconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	Library->ReleasePieceInfo(Info);

	Context->ClearFramebuffer();
	Context->ClearResources();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	QVector<int> Roles;
	Roles.append(Qt::DecorationRole);
	emit dataChanged(index(InfoIndex, 0), index(InfoIndex, 0), Roles);
#else
	emit dataChanged(index(InfoIndex, 0), index(InfoIndex, 0));
#endif
}

void lcPartSelectionListModel::SetShowDecoratedParts(bool Show)
{
	if (Show == mShowDecoratedParts)
		return;

	mShowDecoratedParts = Show;

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetIconSize(int Size)
{
	if (Size == mIconSize)
		return;

	mIconSize = Size;

	beginResetModel();

	for (size_t PartIdx = 0; PartIdx < mParts.size(); PartIdx++)
		mParts[PartIdx].second = QPixmap();

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetShowPartNames(bool Show)
{
	if (Show == mShowPartNames)
		return;

	mShowPartNames = Show;

	beginResetModel();
	endResetModel();

	SetFilter(mFilter);
}

lcPartSelectionListView::lcPartSelectionListView(QWidget* Parent)
	: QListView(Parent)
{
	setUniformItemSizes(true);
	setResizeMode(QListView::Adjust);
	setWordWrap(false);
	setDragEnabled(true);
	setContextMenuPolicy(Qt::CustomContextMenu);

	mListModel = new lcPartSelectionListModel(this);
	setModel(mListModel);
	lcPartSelectionItemDelegate* ItemDelegate = new lcPartSelectionItemDelegate(this, mListModel);
	setItemDelegate(ItemDelegate);

	connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(CustomContextMenuRequested(QPoint)));

	SetIconSize(lcGetProfileInt(LC_PROFILE_PARTS_LIST_ICONS));
}

void lcPartSelectionListView::CustomContextMenuRequested(QPoint Pos)
{
	QMenu* Menu = new QMenu(this);

	if (gSupportsFramebufferObjectARB || gSupportsFramebufferObjectEXT)
	{
		QActionGroup* IconGroup = new QActionGroup(Menu);

		QAction* NoIcons = Menu->addAction(tr("No Icons"), this, SLOT(SetNoIcons()));
		NoIcons->setCheckable(true);
		NoIcons->setChecked(mListModel->GetIconSize() == 0);
		IconGroup->addAction(NoIcons);

		QAction* SmallIcons = Menu->addAction(tr("Small Icons"), this, SLOT(SetSmallIcons()));
		SmallIcons->setCheckable(true);
		SmallIcons->setChecked(mListModel->GetIconSize() == 32);
		IconGroup->addAction(SmallIcons);

		QAction* MediumIcons = Menu->addAction(tr("Medium Icons"), this, SLOT(SetMediumIcons()));
		MediumIcons->setCheckable(true);
		MediumIcons->setChecked(mListModel->GetIconSize() == 64);
		IconGroup->addAction(MediumIcons);

		QAction* LargeIcons = Menu->addAction(tr("Large Icons"), this, SLOT(SetLargeIcons()));
		LargeIcons->setCheckable(true);
		LargeIcons->setChecked(mListModel->GetIconSize() == 96);
		IconGroup->addAction(LargeIcons);

		QAction* ExtraLargeIcons = Menu->addAction(tr("Extra Large Icons"), this, SLOT(SetExtraLargeIcons()));
		ExtraLargeIcons->setCheckable(true);
		ExtraLargeIcons->setChecked(mListModel->GetIconSize() == 192);
		IconGroup->addAction(ExtraLargeIcons);

		Menu->addSeparator();
	}

	if (mListModel->GetIconSize() != 0 && !mListModel->IsListMode())
	{
		QAction* PartNames = Menu->addAction(tr("Show Part Names"), this, SLOT(TogglePartNames()));
		PartNames->setCheckable(true);
		PartNames->setChecked(mListModel->GetShowPartNames());
	}

	QAction* DecoratedParts = Menu->addAction(tr("Show Decorated Parts"), this, SLOT(ToggleDecoratedParts()));
	DecoratedParts->setCheckable(true);
	DecoratedParts->setChecked(mListModel->GetShowDecoratedParts());

	if (mListModel->GetIconSize() != 0)
	{
		QAction* ListMode = Menu->addAction(tr("List Mode"), this, SLOT(ToggleListMode()));
		ListMode->setCheckable(true);
		ListMode->setChecked(mListModel->IsListMode());

		QAction* FixedColor = Menu->addAction(tr("Lock Preview Color"), this, SLOT(ToggleFixedColor()));
		FixedColor->setCheckable(true);
		FixedColor->setChecked(mListModel->IsColorLocked());
	}

	Menu->popup(viewport()->mapToGlobal(Pos));
}

void lcPartSelectionListView::SetNoIcons()
{
	SetIconSize(0);
}

void lcPartSelectionListView::SetSmallIcons()
{
	SetIconSize(32);
}

void lcPartSelectionListView::SetMediumIcons()
{
	SetIconSize(64);
}

void lcPartSelectionListView::SetLargeIcons()
{
	SetIconSize(96);
}

void lcPartSelectionListView::SetExtraLargeIcons()
{
	SetIconSize(192);
}

void lcPartSelectionListView::TogglePartNames()
{
	bool Show = !mListModel->GetShowPartNames();
	mListModel->SetShowPartNames(Show);
	lcSetProfileInt(LC_PROFILE_PARTS_LIST_NAMES, Show);
}

void lcPartSelectionListView::ToggleDecoratedParts()
{
	bool Show = !mListModel->GetShowDecoratedParts();
	mListModel->SetShowDecoratedParts(Show);
	lcSetProfileInt(LC_PROFILE_PARTS_LIST_DECORATED, Show);
}

void lcPartSelectionListView::ToggleListMode()
{
	mListModel->ToggleListMode();
}

void lcPartSelectionListView::ToggleFixedColor()
{
	mListModel->ToggleColorLocked();
}

void lcPartSelectionListView::UpdateViewMode()
{
	setViewMode(mListModel->GetIconSize() && !mListModel->IsListMode() ? QListView::IconMode : QListView::ListMode);
	setWordWrap(mListModel->IsListMode());
	setDragEnabled(true);
}

void lcPartSelectionListView::SetIconSize(int Size)
{
	setIconSize(QSize(Size, Size));
	lcSetProfileInt(LC_PROFILE_PARTS_LIST_ICONS, Size);
	mListModel->SetIconSize(Size);
	UpdateViewMode();

	int Width = Size + 2 * frameWidth() + 6;
	if (verticalScrollBar())
		Width += verticalScrollBar()->sizeHint().width();
	int Height = Size + 2 * frameWidth() + 2;
	if (horizontalScrollBar())
		Height += horizontalScrollBar()->sizeHint().height();
	setMinimumSize(Width, Height);
}

void lcPartSelectionListView::startDrag(Qt::DropActions SupportedActions)
{
	Q_UNUSED(SupportedActions);

	PieceInfo* Info = GetCurrentPart();

	if (!Info)
		return;

	QByteArray ItemData;
	QDataStream DataStream(&ItemData, QIODevice::WriteOnly);
	DataStream << QString(Info->mFileName);

	QMimeData* MimeData = new QMimeData;
	MimeData->setData("application/vnd.leocad-part", ItemData);

	QDrag* Drag = new QDrag(this);
	Drag->setMimeData(MimeData);

	Drag->exec(Qt::CopyAction);
}

lcPartSelectionWidget::lcPartSelectionWidget(QWidget* Parent)
	: QWidget(Parent), mFilterAction(nullptr)
{
	mSplitter = new QSplitter(this);
	mSplitter->setOrientation(Qt::Vertical);

	mCategoriesWidget = new QTreeWidget(mSplitter);
	mCategoriesWidget->setHeaderHidden(true);
	mCategoriesWidget->setUniformRowHeights(true);
	mCategoriesWidget->setRootIsDecorated(false);

	QWidget* PartsGroupWidget = new QWidget(mSplitter);

	QVBoxLayout* PartsLayout = new QVBoxLayout();
	PartsLayout->setContentsMargins(0, 0, 0, 0);
	PartsGroupWidget->setLayout(PartsLayout);

	mFilterWidget = new QLineEdit(PartsGroupWidget);
	mFilterWidget->setPlaceholderText(tr("Search Parts"));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
	mFilterAction = mFilterWidget->addAction(QIcon(":/resources/parts_search.png"), QLineEdit::TrailingPosition);
	connect(mFilterAction, SIGNAL(triggered()), this, SLOT(FilterTriggered()));
#endif
	PartsLayout->addWidget(mFilterWidget);

	mPartsWidget = new lcPartSelectionListView(PartsGroupWidget);
	PartsLayout->addWidget(mPartsWidget);

	QHBoxLayout* Layout = new QHBoxLayout(this);
	Layout->setContentsMargins(0, 0, 0, 0);
	Layout->addWidget(mSplitter);
	setLayout(Layout);

	connect(mPartsWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(PartChanged(const QModelIndex&, const QModelIndex&)));
	connect(mFilterWidget, SIGNAL(textChanged(const QString&)), this, SLOT(FilterChanged(const QString&)));
	connect(mCategoriesWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(CategoryChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

	UpdateCategories();

	mSplitter->setStretchFactor(0, 0);
	mSplitter->setStretchFactor(1, 1);

	connect(Parent, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)), this, SLOT(DockLocationChanged(Qt::DockWidgetArea)));
}

bool lcPartSelectionWidget::event(QEvent* Event)
{
	if (Event->type() == QEvent::ShortcutOverride)
	{
		QKeyEvent* KeyEvent = (QKeyEvent*)Event;
		int Key = KeyEvent->key();

		if (KeyEvent->modifiers() == Qt::NoModifier && Key >= Qt::Key_A && Key <= Qt::Key_Z)
			Event->accept();

		switch (Key)
		{
		case Qt::Key_Down:
		case Qt::Key_Up:
		case Qt::Key_Left:
		case Qt::Key_Right:
		case Qt::Key_Home:
		case Qt::Key_End:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		case Qt::Key_Asterisk:
		case Qt::Key_Plus:
		case Qt::Key_Minus:
			Event->accept();
			break;
		}
	}

	return QWidget::event(Event);
}

void lcPartSelectionWidget::LoadState(QSettings& Settings)
{
	QList<int> Sizes = Settings.value("PartSelectionSplitter").value<QList<int>>();

	if (Sizes.size() != 2)
	{
		int Length = mSplitter->orientation() == Qt::Horizontal ? mSplitter->width() : mSplitter->height();
		Sizes << Length / 3 << 2 * Length / 3;
	}

	mSplitter->setSizes(Sizes);
}

void lcPartSelectionWidget::SaveState(QSettings& Settings)
{
	QList<int> Sizes = mSplitter->sizes();
	Settings.setValue("PartSelectionSplitter", QVariant::fromValue(Sizes));
}

void lcPartSelectionWidget::DisableIconMode()
{
	mPartsWidget->SetNoIcons();
}

void lcPartSelectionWidget::DockLocationChanged(Qt::DockWidgetArea Area)
{
	if (Area == Qt::LeftDockWidgetArea || Area == Qt::RightDockWidgetArea)
		mSplitter->setOrientation(Qt::Vertical);
	else
		mSplitter->setOrientation(Qt::Horizontal);
}

void lcPartSelectionWidget::resizeEvent(QResizeEvent* Event)
{
	if (((QDockWidget*)parent())->isFloating())
	{
		if (Event->size().width() > Event->size().height())
			mSplitter->setOrientation(Qt::Horizontal);
		else
			mSplitter->setOrientation(Qt::Vertical);
	}

	QWidget::resizeEvent(Event);
}

void lcPartSelectionWidget::FilterChanged(const QString& Text)
{
	if (mFilterAction)
	{
		if (Text.isEmpty())
			mFilterAction->setIcon(QIcon(":/resources/parts_search.png"));
		else
			mFilterAction->setIcon(QIcon(":/resources/parts_cancel.png"));
	}

	mPartsWidget->GetListModel()->SetFilter(Text);
}

void lcPartSelectionWidget::FilterTriggered()
{
	mFilterWidget->clear();
}

void lcPartSelectionWidget::CategoryChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous)
{
	Q_UNUSED(Previous);
	lcPartSelectionListModel* ListModel = mPartsWidget->GetListModel();

	if (Current == mModelsCategoryItem)
		ListModel->SetModelsCategory();
	else if (Current == mCurrentModelCategoryItem)
		ListModel->SetCurrentModelCategory();
	else if (Current == mAllPartsCategoryItem)
		ListModel->SetCategory(-1);
	else
		ListModel->SetCategory(mCategoriesWidget->indexOfTopLevelItem(Current) - 2);

	mPartsWidget->setCurrentIndex(ListModel->index(0, 0));
}

void lcPartSelectionWidget::PartChanged(const QModelIndex& Current, const QModelIndex& Previous)
{
	Q_UNUSED(Current);
	Q_UNUSED(Previous);

	gMainWindow->SetCurrentPieceInfo(mPartsWidget->GetCurrentPart());
}

void lcPartSelectionWidget::Redraw()
{
	mPartsWidget->GetListModel()->Redraw();
}

void lcPartSelectionWidget::SetDefaultPart()
{
	for (int CategoryIdx = 0; CategoryIdx < mCategoriesWidget->topLevelItemCount(); CategoryIdx++)
	{
		QTreeWidgetItem* CategoryItem = mCategoriesWidget->topLevelItem(CategoryIdx);

		if (CategoryItem->text(0) == "Brick")
		{
			mCategoriesWidget->setCurrentItem(CategoryItem);
			break;
		}
	}
}

void lcPartSelectionWidget::UpdateCategories()
{
	int CurrentIndex = mCategoriesWidget->indexOfTopLevelItem(mCategoriesWidget->currentItem());

	mCategoriesWidget->clear();

	mAllPartsCategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(tr("All Parts")));
	mCurrentModelCategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(tr("Parts In Use")));

	for (int CategoryIdx = 0; CategoryIdx < gCategories.GetSize(); CategoryIdx++)
		new QTreeWidgetItem(mCategoriesWidget, QStringList(gCategories[CategoryIdx].Name));

	mModelsCategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(tr("Submodels")));

	if (CurrentIndex != -1)
		mCategoriesWidget->setCurrentItem(mCategoriesWidget->topLevelItem(CurrentIndex));
}

void lcPartSelectionWidget::UpdateModels()
{
	if (mCategoriesWidget->currentItem() == mModelsCategoryItem)
		mPartsWidget->GetListModel()->SetModelsCategory();
}
