#pragma once

#include "lc_thumbnailmanager.h"

class lcPartSelectionListModel;
class lcPartSelectionListView;
class lcPartSelectionWidget;

enum class lcPartCategoryType
{
	AllParts,
	PartsInUse,
	Submodels,
	Palette,
	Category,
	Custom,
	Count
};

enum class lcPartCategoryRole
{
	Type = Qt::UserRole,
	Index
};

enum class lcPartFilterType
{
	FixedString,
	Wildcard,
	RegularExpression,
	Count
};

struct lcPartPalette
{
	QString Name;
	std::vector<std::string> Parts;
};

class lcPartSelectionItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	lcPartSelectionItemDelegate(QObject* Parent, lcPartSelectionListModel* ListModel)
		: QStyledItemDelegate(Parent), mListModel(ListModel)
	{
	}

	void paint(QPainter* Painter, const QStyleOptionViewItem& Option, const QModelIndex& Index) const override;
	QSize sizeHint(const QStyleOptionViewItem& Option, const QModelIndex& Index) const override;

protected:
	lcPartSelectionListModel* mListModel;
};

struct lcPartSelectionListModelEntry
{
	PieceInfo* Info = nullptr;
	QPixmap Pixmap;
	lcPartThumbnailId ThumbnailId = lcPartThumbnailId::Invalid;
};

class lcPartSelectionListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	lcPartSelectionListModel(QObject* Parent);
	~lcPartSelectionListModel();

	int rowCount(const QModelIndex& Parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& Index, int Role = Qt::DisplayRole) const override;
	QVariant headerData(int Section, Qt::Orientation Orientation, int Role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex& Index) const override;

	QModelIndex GetPieceInfoIndex(PieceInfo* Info) const;

	PieceInfo* GetPieceInfo(const QModelIndex& Index) const
	{
		return Index.isValid() ? mParts[Index.row()].Info : nullptr;
	}

	PieceInfo* GetPieceInfo(int Row) const
	{
		return mParts[Row].Info;
	}

	bool GetShowDecoratedParts() const
	{
		return mShowDecoratedParts;
	}

	bool GetShowPartAliases() const
	{
		return mShowPartAliases;
	}

	lcPartFilterType GetPartFilterType() const
	{
		return mPartFilterType;
	}

	bool GetCaseSensitiveFilter() const
	{
		return mCaseSensitiveFilter;
	}

	bool GetFileNameFilter() const
	{
		return mFileNameFilter;
	}

	bool GetPartDescriptionFilter() const
	{
		return mPartDescriptionFilter;
	}

	int GetIconSize() const
	{
		return mIconSize;
	}

	bool GetShowPartNames() const
	{
		return mShowPartNames;
	}

	int GetColorIndex() const
	{
		return mColorIndex;
	}

	bool IsColorLocked() const
	{
		return mColorLocked;
	}

	bool IsListMode() const
	{
		return mListMode;
	}

	void UpdateThumbnails();
	void SetColorIndex(int ColorIndex);
	void ToggleColorLocked();
	void ToggleListMode();
	void SetCategory(int CategoryIndex);
	void SetModelsCategory();
	void SetPaletteCategory(int SetIndex);
	void SetCurrentModelCategory();
	void SetCustomParts(const std::vector<PieceInfo*>& Parts);
	void SetFilter(const QString& Filter);
	void RequestThumbnail(int PartIndex);
	void SetShowDecoratedParts(bool Show);
	void SetShowPartAliases(bool Show);
	void SetPartFilterType(lcPartFilterType Option);
	void SetCaseSensitiveFilter(bool Option);
	void SetFileNameFilter(bool Option);
	void SetPartDescriptionFilter(bool Option);
	void SetIconSize(int Size);
	void SetShowPartNames(bool Show);

protected slots:
	void ThumbnailReady(lcPartThumbnailId ThumbnailId, QPixmap Pixmap);

protected:
	void ReleaseThumbnails();

	lcPartSelectionListView* mListView;
	std::vector<lcPartSelectionListModelEntry> mParts;
	lcPartFilterType mPartFilterType;
	int mIconSize;
	bool mColorLocked;
	int mColorIndex;
	bool mShowPartNames;
	bool mListMode;
	bool mShowDecoratedParts;
	bool mShowPartAliases;
	bool mCaseSensitiveFilter;
	bool mFileNameFilter;
	bool mPartDescriptionFilter;
	QByteArray mFilter;
};

class lcPartSelectionListView : public QListView
{
	Q_OBJECT

public:
	lcPartSelectionListView(QWidget* Parent, lcPartSelectionWidget* PartSelectionWidget);

	void startDrag(Qt::DropActions SupportedActions) override;

	void SetCategory(lcPartCategoryType Type, int Index);
	void SetCustomParts(const std::vector<PieceInfo*>& Parts);
	void SetCurrentPart(PieceInfo* Info);

	PieceInfo* GetCurrentPart() const
	{
		return mListModel->GetPieceInfo(currentIndex());
	}

	lcPartSelectionListModel* GetListModel() const
	{
		return mListModel;
	}

	lcPartSelectionWidget* GetPartSelectionWidget() const
	{
		return mPartSelectionWidget;
	}

	PieceInfo* GetContextInfo() const
	{
		return mContextInfo;
	}

	void UpdateInUseCategory();
	void UpdateViewMode();

signals:
	void PartPicked(PieceInfo* Info);

public slots:
	void CustomContextMenuRequested(QPoint Pos);
	void SetNoIcons();
	void SetSmallIcons();
	void SetMediumIcons();
	void SetLargeIcons();
	void SetExtraLargeIcons();
	void SetFixedStringFilter();
	void SetWildcardFilter();
	void SetRegularExpressionFilter();
	void TogglePartNames();
	void ToggleDecoratedParts();
	void TogglePartAliases();
	void ToggleCaseSensitiveFilter();
	void ToggleFileNameFilter();
	void TogglePartDescriptionFilter();
	void ToggleListMode();
	void ToggleFixedColor();

protected slots:
	void DoubleClicked(const QModelIndex& Index);

protected:
	void SetIconSize(int Size);
	void SetPartFilterType(lcPartFilterType Option);
	void keyPressEvent(QKeyEvent* KeyEvent) override;

	lcPartSelectionListModel* mListModel = nullptr;
	lcPartSelectionWidget* mPartSelectionWidget = nullptr;
	PieceInfo* mContextInfo = nullptr;
	lcPartCategoryType mCategoryType = lcPartCategoryType::AllParts;
	int mCategoryIndex = 0;
	std::map<std::pair<lcPartCategoryType, int>, int> mLastCategoryRow;
};

class lcPartSelectionWidget : public QWidget
{
	Q_OBJECT

public:
	lcPartSelectionWidget(QWidget* Parent);

	void UpdateThumbnails();
	void SetDefaultPart();
	void UpdateModels();
	void UpdateInUseCategory();
	void UpdateCategories();
	void LoadState(QSettings& Settings);
	void SaveState(QSettings& Settings);
	void DisableIconMode();
	void SetOrientation(Qt::Orientation Orientation);
	void SetCurrentPart(PieceInfo* Info);

	int GetColorIndex() const
	{
		return mPartsWidget->GetListModel()->GetColorIndex();
	}

	void SetColorIndex(int ColorIndex)
	{
		mPartsWidget->GetListModel()->SetColorIndex(ColorIndex);
	}

	const std::vector<lcPartPalette>& GetPartPalettes() const
	{
		return mPartPalettes;
	}

	PieceInfo* GetCurrentPart() const
	{
		return mPartsWidget->GetCurrentPart();
	}

	void FocusPartFilterWidget() const
	{
		mFilterWidget->setFocus();
		mFilterWidget->selectAll();
	}

signals:
	void PartPicked(PieceInfo* Info);
	void CurrentPartChanged(PieceInfo* Info);

public slots:
	void AddToPalette();
	void RemoveFromPalette();
	void DockLocationChanged(Qt::DockWidgetArea Area);

protected slots:
	void FilterChanged(const QString& Text);
	void FilterCategoriesChanged(const QString& Text);
	void FilterTriggered();
	void FilterCaseTriggered();
	void FilterCategoriesTriggered();
	void CategoryChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous);
	void PartViewSelectionChanged(const QModelIndex& Current, const QModelIndex& Previous);
	void PartViewPartPicked(PieceInfo* Info);
	void OptionsMenuAboutToShow();
	void EditPartPalettes();

protected:
	void LoadPartPalettes();
	void SavePartPalettes();

	void resizeEvent(QResizeEvent* Event) override;
	bool event(QEvent* Event) override;

	QTreeWidget* mCategoriesWidget = nullptr;
	QLineEdit* mFilterCategoriesWidget = nullptr;
	QAction* mFilterCategoriesAction = nullptr;
	QAction* mFilterCaseAction = nullptr;
	QLineEdit* mFilterWidget = nullptr;
	QAction* mFilterAction = nullptr;
	lcPartSelectionListView* mPartsWidget = nullptr;
	QSplitter* mSplitter = nullptr;
	QTreeWidgetItem* mAllPartsCategoryItem = nullptr;
	std::vector<lcPartPalette> mPartPalettes;
};
