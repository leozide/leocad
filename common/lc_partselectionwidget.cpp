#include "lc_global.h"
#include "lc_partselectionwidget.h"
#include "lc_partpalettedialog.h"
#include "lc_profile.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "lc_library.h"
#include "lc_thumbnailmanager.h"
#include "project.h"
#include "pieceinf.h"
#include "lc_glextensions.h"
#include "lc_category.h"

Q_DECLARE_METATYPE(QList<int>)

void lcPartSelectionItemDelegate::paint(QPainter* Painter, const QStyleOptionViewItem& Option, const QModelIndex& Index) const
{
	mListModel->RequestThumbnail(Index.row());
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
	mShowPartAliases = lcGetProfileInt(LC_PROFILE_PARTS_LIST_ALIASES);

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

	connect(lcGetPiecesLibrary()->GetThumbnailManager(), &lcThumbnailManager::ThumbnailReady, this, &lcPartSelectionListModel::ThumbnailReady);
}

lcPartSelectionListModel::~lcPartSelectionListModel()
{
	ReleaseThumbnails();
}

void lcPartSelectionListModel::UpdateThumbnails()
{
	beginResetModel();

	ReleaseThumbnails();

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetColorIndex(int ColorIndex)
{
	if (mColorLocked || ColorIndex == mColorIndex)
		return;

	UpdateThumbnails();

	mColorIndex = ColorIndex;
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
	beginResetModel();

	ReleaseThumbnails();

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	std::vector<PieceInfo*> SingleParts, GroupedParts;

	if (CategoryIndex != -1)
		Library->GetCategoryEntries(CategoryIndex, false, SingleParts, GroupedParts);
	else
	{
		Library->GetParts(SingleParts);

		lcModel* ActiveModel = gMainWindow->GetActiveModel();

		for (size_t PartIndex = 0; PartIndex < SingleParts.size(); )
		{
			PieceInfo* Info = SingleParts[PartIndex];

			if (!Info->IsModel() || !Info->GetModel()->IncludesModel(ActiveModel))
				PartIndex++;
			else
				SingleParts.erase(SingleParts.begin() + PartIndex);
		}
	}

	auto lcPartSortFunc=[](const PieceInfo* a, const PieceInfo* b)
	{
		return strcmp(a->m_strDescription, b->m_strDescription) < 0;
	};

	std::sort(SingleParts.begin(), SingleParts.end(), lcPartSortFunc);

	mParts.resize(SingleParts.size());

	for (size_t PartIndex = 0; PartIndex < SingleParts.size(); PartIndex++)
		mParts[PartIndex].Info = SingleParts[PartIndex];

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetModelsCategory()
{
	beginResetModel();

	ReleaseThumbnails();
	mParts.clear();

	const std::vector<std::unique_ptr<lcModel>>& Models = lcGetActiveProject()->GetModels();
	lcModel* ActiveModel = gMainWindow->GetActiveModel();

	for (const std::unique_ptr<lcModel>& Model : Models)
		if (!Model->IncludesModel(ActiveModel))
			mParts.emplace_back().Info = Model->GetPieceInfo();

	auto lcPartSortFunc = [](const lcPartSelectionListModelEntry& a, const lcPartSelectionListModelEntry& b)
	{
		return strcmp(a.Info->m_strDescription, b.Info->m_strDescription) < 0;
	};

	std::sort(mParts.begin(), mParts.end(), lcPartSortFunc);

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetPaletteCategory(int SetIndex)
{
	beginResetModel();

	ReleaseThumbnails();
	mParts.clear();

	lcPartSelectionWidget* PartSelectionWidget = mListView->GetPartSelectionWidget();
	const std::vector<lcPartPalette>& Palettes = PartSelectionWidget->GetPartPalettes();
	std::vector<PieceInfo*> PartsList = lcGetPiecesLibrary()->GetPartsFromSet(Palettes[SetIndex].Parts);

	auto lcPartSortFunc = [](const PieceInfo* a, const PieceInfo* b)
	{
		return strcmp(a->m_strDescription, b->m_strDescription) < 0;
	};

	std::sort(PartsList.begin(), PartsList.end(), lcPartSortFunc);

	mParts.reserve(PartsList.size());

	for (PieceInfo* Info : PartsList)
		mParts.emplace_back().Info = Info;

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetCurrentModelCategory()
{
	beginResetModel();

	ReleaseThumbnails();
	mParts.clear();

	lcModel* ActiveModel = gMainWindow->GetActiveModel();
	lcPartsList PartsList;

	if (ActiveModel)
		ActiveModel->GetPartsList(gDefaultColor, true, true, PartsList);

	for (const auto& PartIt : PartsList)
		mParts.emplace_back().Info = (PieceInfo*)PartIt.first;

	auto lcPartSortFunc = [](const lcPartSelectionListModelEntry& a, const lcPartSelectionListModelEntry& b)
	{
		return strcmp(a.Info->m_strDescription, b.Info->m_strDescription) < 0;
	};

	std::sort(mParts.begin(), mParts.end(), lcPartSortFunc);

	endResetModel();

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetFilter(const QString& Filter)
{
	mFilter = Filter.toLatin1();

	for (size_t PartIdx = 0; PartIdx < mParts.size(); PartIdx++)
	{
		PieceInfo* Info = mParts[PartIdx].Info;
		bool Visible;

		if (!mShowDecoratedParts && Info->IsPatterned() && !Info->IsProjectPiece())
			Visible = false;
		else if (!mShowPartAliases && Info->m_strDescription[0] == '=')
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

		mListView->setRowHidden((int)PartIdx, !Visible);
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
		PieceInfo* Info = mParts[InfoIndex].Info;

		switch (Role)
		{
		case Qt::DisplayRole:
			if (!mIconSize || mShowPartNames || mListMode)
				return QVariant(QString::fromLatin1(Info->m_strDescription));
			break;

		case Qt::ToolTipRole:
			return QVariant(QString("%1 (%2)").arg(QString::fromLatin1(Info->m_strDescription), QString::fromLatin1(Info->mFileName)));

		case Qt::DecorationRole:
			if (mIconSize && !mParts[InfoIndex].Pixmap.isNull())
				return QVariant(mParts[InfoIndex].Pixmap);
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

QModelIndex lcPartSelectionListModel::GetPieceInfoIndex(PieceInfo* Info) const
{
	if (Info)
	{
		for (int PartIndex = 0; PartIndex < static_cast<int>(mParts.size()); PartIndex++)
			if (mParts[PartIndex].Info == Info)
				return index(PartIndex, 0);
	}

	return QModelIndex();
}

void lcPartSelectionListModel::ReleaseThumbnails()
{
	lcThumbnailManager* ThumbnailManager = lcGetPiecesLibrary()->GetThumbnailManager();

	for (lcPartSelectionListModelEntry& Part : mParts)
	{
		ThumbnailManager->ReleaseThumbnail(Part.ThumbnailId);
		Part.ThumbnailId = lcPartThumbnailId::Invalid;
		Part.Pixmap = QPixmap();
	}
}

void lcPartSelectionListModel::RequestThumbnail(int PartIndex)
{
	if (!mIconSize || !mParts[PartIndex].Pixmap.isNull() || mParts[PartIndex].ThumbnailId != lcPartThumbnailId::Invalid)
		return;

	PieceInfo* Info = mParts[PartIndex].Info;
	auto [ThumbnailId, Thumbnail] = lcGetPiecesLibrary()->GetThumbnailManager()->RequestThumbnail(Info, mColorIndex, mIconSize);

	mParts[PartIndex].ThumbnailId = ThumbnailId;

	if (!Thumbnail.isNull())
	{
		mParts[PartIndex].Pixmap = Thumbnail;

		emit dataChanged(index(PartIndex, 0), index(PartIndex, 0), { Qt::DecorationRole });
	}
}

void lcPartSelectionListModel::ThumbnailReady(lcPartThumbnailId ThumbnailId, QPixmap Pixmap)
{
	for (int PartIndex = 0; PartIndex < static_cast<int>(mParts.size()); PartIndex++)
	{
		if (mParts[PartIndex].ThumbnailId == ThumbnailId)
		{
			mParts[PartIndex].Pixmap = Pixmap;

			emit dataChanged(index(PartIndex, 0), index(PartIndex, 0), { Qt::DecorationRole });

			break;
		}
	}
}

void lcPartSelectionListModel::SetShowDecoratedParts(bool Show)
{
	if (Show == mShowDecoratedParts)
		return;

	mShowDecoratedParts = Show;

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetShowPartAliases(bool Show)
{
	if (Show == mShowPartAliases)
		return;

	mShowPartAliases = Show;

	SetFilter(mFilter);
}

void lcPartSelectionListModel::SetIconSize(int Size)
{
	if (Size == mIconSize)
		return;

	mIconSize = Size;

	beginResetModel();

	ReleaseThumbnails();

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

lcPartSelectionListView::lcPartSelectionListView(QWidget* Parent, lcPartSelectionWidget* PartSelectionWidget)
	: QListView(Parent)
{
	mPartSelectionWidget = PartSelectionWidget;
	mCategoryType = lcPartCategoryType::AllParts;
	mCategoryIndex = 0;

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

	QModelIndex Index = indexAt(Pos);
	mContextInfo = Index.isValid() ? mListModel->GetPieceInfo(Index.row()) : nullptr;

	QMenu* SetMenu = Menu->addMenu(tr("Add to Palette"));

	const std::vector<lcPartPalette>& Palettes = mPartSelectionWidget->GetPartPalettes();

	if (!Palettes.empty())
	{
		for (const lcPartPalette& Palette : Palettes)
			SetMenu->addAction(Palette.Name, mPartSelectionWidget, SLOT(AddToPalette()));
	}
	else
	{
		QAction* Action = SetMenu->addAction(tr("None"));
		Action->setEnabled(false);
	}

	QAction* RemoveAction = Menu->addAction(tr("Remove from Palette"), mPartSelectionWidget, SLOT(RemoveFromPalette()));
	RemoveAction->setEnabled(mCategoryType == lcPartCategoryType::Palette);

	Menu->exec(viewport()->mapToGlobal(Pos));
	delete Menu;
}

void lcPartSelectionListView::SetCategory(lcPartCategoryType Type, int Index)
{
	mCategoryType = Type;
	mCategoryIndex = Index;

	switch (Type)
	{
	case lcPartCategoryType::AllParts:
		mListModel->SetCategory(-1);
		break;
	case lcPartCategoryType::PartsInUse:
		mListModel->SetCurrentModelCategory();
		break;
	case lcPartCategoryType::Submodels:
		mListModel->SetModelsCategory();
		break;
	case lcPartCategoryType::Palette:
		mListModel->SetPaletteCategory(Index);
		break;
	case lcPartCategoryType::Category:
		mListModel->SetCategory(Index);
		break;
	case lcPartCategoryType::Count:
		break;
	}

	setCurrentIndex(mListModel->index(0, 0));
}

void lcPartSelectionListView::SetCurrentPart(PieceInfo* Info)
{
	QModelIndex Index = mListModel->GetPieceInfoIndex(Info);

	if (Index.isValid())
	{
		setCurrentIndex(Index);
		scrollTo(Index, QAbstractItemView::EnsureVisible);
	}	
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

void lcPartSelectionListView::TogglePartAliases()
{
	bool Show = !mListModel->GetShowPartAliases();
	mListModel->SetShowPartAliases(Show);
	lcSetProfileInt(LC_PROFILE_PARTS_LIST_ALIASES, Show);
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
	: QWidget(Parent)
{
	mSplitter = new QSplitter(this);
	mSplitter->setOrientation(Qt::Vertical);
	mSplitter->setChildrenCollapsible(false);

	QWidget* CategoriesGroupWidget = new QWidget(mSplitter);

	QVBoxLayout* CategoriesLayout = new QVBoxLayout();
	CategoriesLayout->setContentsMargins(0, 0, 0, 0);
	CategoriesGroupWidget->setLayout(CategoriesLayout);

	QHBoxLayout* FilterCategoriesLayout = new QHBoxLayout();
	FilterCategoriesLayout->setContentsMargins(0, 0, 0, 0);
	CategoriesLayout->addLayout(FilterCategoriesLayout);

	mFilterCategoriesWidget = new QLineEdit(CategoriesGroupWidget);
	mFilterCategoriesWidget->setPlaceholderText(tr("Filter Categories"));
	FilterCategoriesLayout->addWidget(mFilterCategoriesWidget);

	mFilterCaseAction = new QAction();
	mFilterCaseAction->setIcon(QIcon(":/resources/case.png"));
	mFilterCaseAction->setToolTip(tr("Match Case"));
	mFilterCaseAction->setCheckable(true);
	mFilterCaseAction->setChecked(false);
	connect(mFilterCaseAction, SIGNAL(triggered()), this, SLOT(FilterCaseTriggered()));

	QToolButton* FilterCaseButton = new QToolButton();
	FilterCaseButton->setDefaultAction(mFilterCaseAction);
	FilterCategoriesLayout->addWidget(FilterCaseButton);

	mCategoriesWidget = new QTreeWidget(mSplitter);
	mCategoriesWidget->setHeaderHidden(true);
	mCategoriesWidget->setUniformRowHeights(true);
	mCategoriesWidget->setRootIsDecorated(false);

	CategoriesLayout->addWidget(mCategoriesWidget);

	QWidget* PartsGroupWidget = new QWidget(mSplitter);

	QVBoxLayout* PartsLayout = new QVBoxLayout();
	PartsLayout->setContentsMargins(0, 0, 0, 0);
	PartsGroupWidget->setLayout(PartsLayout);

	QHBoxLayout* SearchLayout = new QHBoxLayout();
	SearchLayout->setContentsMargins(0, 0, 0, 0);
	PartsLayout->addLayout(SearchLayout);

	mFilterWidget = new QLineEdit(PartsGroupWidget);
	mFilterWidget->setPlaceholderText(tr("Search Parts"));
	SearchLayout->addWidget(mFilterWidget);

	QToolButton* OptionsButton = new QToolButton();
	OptionsButton->setIcon(QIcon(":/resources/gear_in.png"));
	OptionsButton->setToolTip(tr("Options"));
	OptionsButton->setPopupMode(QToolButton::InstantPopup);
	SearchLayout->addWidget(OptionsButton);

	QMenu* OptionsMenu = new QMenu(this);
	OptionsButton->setMenu(OptionsMenu);
	connect(OptionsMenu, SIGNAL(aboutToShow()), this, SLOT(OptionsMenuAboutToShow()));

	mPartsWidget = new lcPartSelectionListView(PartsGroupWidget, this);
	PartsLayout->addWidget(mPartsWidget);

	QHBoxLayout* Layout = new QHBoxLayout(this);
	Layout->setContentsMargins(0, 0, 0, 0);
	Layout->addWidget(mSplitter);
	setLayout(Layout);

	connect(mPartsWidget, &QListView::doubleClicked, this, &lcPartSelectionWidget::PartViewDoubleClicked);
	connect(mPartsWidget->selectionModel(), &QItemSelectionModel::currentChanged, this, &lcPartSelectionWidget::PartViewSelectionChanged);
	connect(mFilterWidget, &QLineEdit::textChanged, this, &lcPartSelectionWidget::FilterChanged);
	connect(mCategoriesWidget, &QTreeWidget::currentItemChanged, this, &lcPartSelectionWidget::CategoryChanged);
	connect(mFilterCategoriesWidget, &QLineEdit::textChanged, this, &lcPartSelectionWidget::FilterCategoriesChanged);

	LoadPartPalettes();
	UpdateCategories();

	mSplitter->setStretchFactor(0, 0);
	mSplitter->setStretchFactor(1, 1);
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
		Sizes = { Length / 3, 2 * Length / 3 };
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

void lcPartSelectionWidget::SetCurrentPart(PieceInfo* Info)
{
	mCategoriesWidget->setCurrentItem(mAllPartsCategoryItem);
	mPartsWidget->SetCurrentPart(Info);
	mPartsWidget->setFocus();
}

void lcPartSelectionWidget::SetOrientation(Qt::Orientation Orientation)
{
	mSplitter->setOrientation(Orientation);

	int Length = mSplitter->orientation() == Qt::Horizontal ? mSplitter->width() : mSplitter->height();
	QList<int> Sizes = { Length / 3, 2 * Length / 3 };

	mSplitter->setSizes(Sizes);
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
	QDockWidget* DockWidget = qobject_cast<QDockWidget*>(parent());

	if (DockWidget && DockWidget->isFloating())
	{
		if (Event->size().width() > Event->size().height())
			mSplitter->setOrientation(Qt::Horizontal);
		else
			mSplitter->setOrientation(Qt::Vertical);
	}

	QWidget::resizeEvent(Event);
}

void lcPartSelectionWidget::FilterCategoriesChanged(const QString& Text)
{
	if (mFilterCategoriesAction)
	{
		if (Text.isEmpty())
		{
			delete mFilterCategoriesAction;
			mFilterCategoriesAction = nullptr;
		}
	}
	else
	{
		if (!Text.isEmpty())
		{
			mFilterCategoriesAction = mFilterCategoriesWidget->addAction(QIcon(":/stylesheet/close.svg"), QLineEdit::TrailingPosition);
			connect(mFilterCategoriesAction, &QAction::triggered, this, &lcPartSelectionWidget::FilterCategoriesTriggered);
		}
	}

	bool Hide = true;
	Qt::CaseSensitivity MatchCase = mFilterCaseAction->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
	mCategoriesWidget->setUpdatesEnabled(false);
	for (int CategoryIdx = 0; CategoryIdx < mCategoriesWidget->topLevelItemCount(); CategoryIdx++)
	{
		QTreeWidgetItem* CategoryItem = mCategoriesWidget->topLevelItem(CategoryIdx);
		Hide = false;
		if (!CategoryItem->text(0).contains(Text, MatchCase))
			Hide = true;
		CategoryItem->setHidden(Hide);
	}
	mCategoriesWidget->setUpdatesEnabled(true);
	mCategoriesWidget->update();
}

void lcPartSelectionWidget::FilterChanged(const QString& Text)
{
	if (mFilterAction)
	{
		if (Text.isEmpty())
		{
			delete mFilterAction;
			mFilterAction = nullptr;
		}
	}
	else
	{
		if (!Text.isEmpty())
		{
			mFilterAction = mFilterWidget->addAction(QIcon(":/stylesheet/close.svg"), QLineEdit::TrailingPosition);
			connect(mFilterAction, &QAction::triggered, this, &lcPartSelectionWidget::FilterTriggered);
		}
	}

	mPartsWidget->GetListModel()->SetFilter(Text);
}

void lcPartSelectionWidget::FilterCategoriesTriggered()
{
	mFilterCategoriesWidget->clear();
}

void lcPartSelectionWidget::FilterCaseTriggered()
{
	if (!mFilterCategoriesWidget->text().isEmpty())
		FilterCategoriesChanged(mFilterCategoriesWidget->text());
}

void lcPartSelectionWidget::FilterTriggered()
{
	mFilterWidget->clear();
}

void lcPartSelectionWidget::CategoryChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous)
{
	Q_UNUSED(Previous);

	if (!Current)
		return;

	int Type = Current->data(0, static_cast<int>(lcPartCategoryRole::Type)).toInt();
	int Index = Current->data(0, static_cast<int>(lcPartCategoryRole::Index)).toInt();

	mPartsWidget->SetCategory(static_cast<lcPartCategoryType>(Type), Index);
}

void lcPartSelectionWidget::PartViewSelectionChanged(const QModelIndex& Current, const QModelIndex& Previous)
{
	Q_UNUSED(Current);
	Q_UNUSED(Previous);

	emit CurrentPartChanged(mPartsWidget->GetCurrentPart());
}

void lcPartSelectionWidget::PartViewDoubleClicked(const QModelIndex& Index)
{
	PieceInfo* Info = mPartsWidget->GetListModel()->GetPieceInfo(Index.row());

	if (Info)
		emit PartPicked(Info);
}

void lcPartSelectionWidget::OptionsMenuAboutToShow()
{
	QMenu* Menu = (QMenu*)sender();
	Menu->clear();

	Menu->addAction(tr("Edit Palettes..."), this, SLOT(EditPartPalettes()));
	Menu->addSeparator();

	lcPartSelectionListModel* ListModel = mPartsWidget->GetListModel();

	if (gSupportsFramebufferObject)
	{
		QActionGroup* IconGroup = new QActionGroup(Menu);

		QAction* NoIcons = Menu->addAction(tr("No Icons"), mPartsWidget, SLOT(SetNoIcons()));
		NoIcons->setCheckable(true);
		NoIcons->setChecked(ListModel->GetIconSize() == 0);
		IconGroup->addAction(NoIcons);

		QAction* SmallIcons = Menu->addAction(tr("Small Icons"), mPartsWidget, SLOT(SetSmallIcons()));
		SmallIcons->setCheckable(true);
		SmallIcons->setChecked(ListModel->GetIconSize() == 32);
		IconGroup->addAction(SmallIcons);

		QAction* MediumIcons = Menu->addAction(tr("Medium Icons"), mPartsWidget, SLOT(SetMediumIcons()));
		MediumIcons->setCheckable(true);
		MediumIcons->setChecked(ListModel->GetIconSize() == 64);
		IconGroup->addAction(MediumIcons);

		QAction* LargeIcons = Menu->addAction(tr("Large Icons"), mPartsWidget, SLOT(SetLargeIcons()));
		LargeIcons->setCheckable(true);
		LargeIcons->setChecked(ListModel->GetIconSize() == 96);
		IconGroup->addAction(LargeIcons);

		QAction* ExtraLargeIcons = Menu->addAction(tr("Extra Large Icons"), mPartsWidget, SLOT(SetExtraLargeIcons()));
		ExtraLargeIcons->setCheckable(true);
		ExtraLargeIcons->setChecked(ListModel->GetIconSize() == 192);
		IconGroup->addAction(ExtraLargeIcons);

		Menu->addSeparator();
	}

	if (ListModel->GetIconSize() != 0 && !ListModel->IsListMode())
	{
		QAction* PartNames = Menu->addAction(tr("Show Part Names"), mPartsWidget, SLOT(TogglePartNames()));
		PartNames->setCheckable(true);
		PartNames->setChecked(ListModel->GetShowPartNames());
	}

	QAction* DecoratedParts = Menu->addAction(tr("Show Decorated Parts"), mPartsWidget, SLOT(ToggleDecoratedParts()));
	DecoratedParts->setCheckable(true);
	DecoratedParts->setChecked(ListModel->GetShowDecoratedParts());

	QAction* PartAliases = Menu->addAction(tr("Show Part Aliases"), mPartsWidget, SLOT(TogglePartAliases()));
	PartAliases->setCheckable(true);
	PartAliases->setChecked(ListModel->GetShowPartAliases());

	if (ListModel->GetIconSize() != 0)
	{
		QAction* ListMode = Menu->addAction(tr("List Mode"), mPartsWidget, SLOT(ToggleListMode()));
		ListMode->setCheckable(true);
		ListMode->setChecked(ListModel->IsListMode());

		QAction* FixedColor = Menu->addAction(tr("Lock Color"), mPartsWidget, SLOT(ToggleFixedColor()));
		FixedColor->setCheckable(true);
		FixedColor->setChecked(ListModel->IsColorLocked());
	}
}

void lcPartSelectionWidget::EditPartPalettes()
{
	lcPartPaletteDialog Dialog(this, mPartPalettes);

	if (Dialog.exec() != QDialog::Accepted)
		return;

	SavePartPalettes();
	UpdateCategories();
}

void lcPartSelectionWidget::UpdateThumbnails()
{
	mPartsWidget->GetListModel()->UpdateThumbnails();
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

void lcPartSelectionWidget::LoadPartPalettes()
{
	QByteArray Buffer = lcGetProfileBuffer(LC_PROFILE_PART_PALETTES);
	QJsonDocument Document = QJsonDocument::fromJson(Buffer);

	if (Document.isNull())
		Document = QJsonDocument::fromJson((QString("{ \"Version\":1, \"Palettes\": { \"%1\": [] } }").arg(tr("Favorites"))).toUtf8());

	QJsonObject RootObject = Document.object();
	mPartPalettes.clear();

	int Version = RootObject["Version"].toInt(0);
	if (Version != 1)
		return;

	QJsonObject PalettesObject = RootObject["Palettes"].toObject();

	for (QJsonObject::const_iterator ElementIt = PalettesObject.constBegin(); ElementIt != PalettesObject.constEnd(); ElementIt++)
	{
		if (!ElementIt.value().isArray())
			continue;

		lcPartPalette Palette;
		Palette.Name = ElementIt.key();

		QJsonArray Parts = ElementIt.value().toArray();

		for (const QJsonValue& Part : Parts)
			Palette.Parts.emplace_back(Part.toString().toStdString());

		mPartPalettes.emplace_back(std::move(Palette));
	}
}

void lcPartSelectionWidget::SavePartPalettes()
{
	QJsonObject RootObject;

	RootObject["Version"] = 1;
	QJsonObject PalettesObject;

	for (const lcPartPalette& Palette : mPartPalettes)
	{
		QJsonArray Parts;

		for (const std::string& PartId : Palette.Parts)
			Parts.append(QString::fromStdString(PartId));

		PalettesObject[Palette.Name] = Parts;
	}

	RootObject["Palettes"] = PalettesObject;

	QByteArray Buffer = QJsonDocument(RootObject).toJson();
	lcSetProfileBuffer(LC_PROFILE_PART_PALETTES, Buffer);
}

void lcPartSelectionWidget::AddToPalette()
{
	PieceInfo* Info = mPartsWidget->GetContextInfo();
	if (!Info)
		return;

	QString SetName = ((QAction*)sender())->text();

	std::vector<lcPartPalette>::iterator SetIt = std::find_if(mPartPalettes.begin(), mPartPalettes.end(), [&SetName](const lcPartPalette& Set)
	{
		return Set.Name == SetName;
	});

	if (SetIt == mPartPalettes.end())
		return;

	std::string PartId = lcGetPiecesLibrary()->GetPartId(Info);
	std::vector<std::string>& Parts = SetIt->Parts;

	if (std::find(Parts.begin(), Parts.end(), PartId) == Parts.end())
	{
		Parts.emplace_back(PartId);
		SavePartPalettes();
	}
}

void lcPartSelectionWidget::RemoveFromPalette()
{
	PieceInfo* Info = mPartsWidget->GetContextInfo();
	if (!Info)
		return;

	QTreeWidgetItem* CurrentItem = mCategoriesWidget->currentItem();
	if (!CurrentItem || CurrentItem->data(0, static_cast<int>(lcPartCategoryRole::Type)) != static_cast<int>(lcPartCategoryType::Palette))
		return;

	int SetIndex = CurrentItem->data(0, static_cast<int>(lcPartCategoryRole::Index)).toInt();
	lcPartPalette& Palette = mPartPalettes[SetIndex];

	std::string PartId = lcGetPiecesLibrary()->GetPartId(Info);
	std::vector<std::string>::iterator PartIt = std::find(Palette.Parts.begin(), Palette.Parts.end(), PartId);

	if (PartIt != Palette.Parts.end())
	{
		Palette.Parts.erase(PartIt);
		mPartsWidget->SetCategory(lcPartCategoryType::Palette, SetIndex);
		SavePartPalettes();
	}
}

void lcPartSelectionWidget::UpdateCategories()
{
	QTreeWidgetItem* CurrentItem = mCategoriesWidget->currentItem();
	lcPartCategoryType CurrentType = lcPartCategoryType::Count;
	int CurrentIndex = -1;

	if (CurrentItem)
	{
		CurrentType = static_cast<lcPartCategoryType>(CurrentItem->data(0, static_cast<int>(lcPartCategoryRole::Type)).toInt());
		CurrentIndex = CurrentItem->data(0, static_cast<int>(lcPartCategoryRole::Index)).toInt();
		CurrentItem = nullptr;
	}

	mCategoriesWidget->clear();

	mAllPartsCategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(tr("All Parts")));
	mAllPartsCategoryItem->setData(0, static_cast<int>(lcPartCategoryRole::Type), static_cast<int>(lcPartCategoryType::AllParts));

	if (CurrentType == lcPartCategoryType::AllParts && CurrentIndex == 0)
		CurrentItem = mAllPartsCategoryItem;

	QTreeWidgetItem* CurrentModelCategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(tr("In Use")));
	CurrentModelCategoryItem->setData(0, static_cast<int>(lcPartCategoryRole::Type), static_cast<int>(lcPartCategoryType::PartsInUse));

	if (CurrentType == lcPartCategoryType::PartsInUse && CurrentIndex == 0)
		CurrentItem = CurrentModelCategoryItem;

	QTreeWidgetItem* SubmodelsCategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(tr("Submodels")));
	SubmodelsCategoryItem->setData(0, static_cast<int>(lcPartCategoryRole::Type), static_cast<int>(lcPartCategoryType::Submodels));

	if (CurrentType == lcPartCategoryType::Submodels && CurrentIndex == 0)
		CurrentItem = SubmodelsCategoryItem;

	for (int PaletteIdx = 0; PaletteIdx < static_cast<int>(mPartPalettes.size()); PaletteIdx++)
	{
		const lcPartPalette& Set = mPartPalettes[PaletteIdx];
		QTreeWidgetItem* PaletteCategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(Set.Name));
		PaletteCategoryItem->setData(0, static_cast<int>(lcPartCategoryRole::Type), static_cast<int>(lcPartCategoryType::Palette));
		PaletteCategoryItem->setData(0, static_cast<int>(lcPartCategoryRole::Index), PaletteIdx);

		if (CurrentType == lcPartCategoryType::Palette && CurrentIndex == PaletteIdx)
			CurrentItem = PaletteCategoryItem;
	}

	for (int CategoryIdx = 0; CategoryIdx < static_cast<int>(gCategories.size()); CategoryIdx++)
	{
		QTreeWidgetItem* CategoryItem = new QTreeWidgetItem(mCategoriesWidget, QStringList(gCategories[CategoryIdx].Name));
		CategoryItem->setData(0, static_cast<int>(lcPartCategoryRole::Type), static_cast<int>(lcPartCategoryType::Category));
		CategoryItem->setData(0, static_cast<int>(lcPartCategoryRole::Index), CategoryIdx);

		if (CurrentType == lcPartCategoryType::Category && CurrentIndex == CategoryIdx)
			CurrentItem = CategoryItem;
	}

	if (CurrentItem)
		mCategoriesWidget->setCurrentItem(CurrentItem);
}

void lcPartSelectionWidget::UpdateModels()
{
	QTreeWidgetItem* CurrentItem = mCategoriesWidget->currentItem();

	if (CurrentItem && CurrentItem->data(0, static_cast<int>(lcPartCategoryRole::Type)) == static_cast<int>(lcPartCategoryType::Submodels))
		mPartsWidget->SetCategory(lcPartCategoryType::Submodels, 0);
}
