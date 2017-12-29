#pragma once

#include "lc_context.h"

class lcPartSelectionListModel;
class lcPartSelectionListView;

class lcPartSelectionItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	lcPartSelectionItemDelegate(QObject* Parent, lcPartSelectionListModel* ListModel)
		: QStyledItemDelegate(Parent), mListModel(ListModel)
	{
	}

	virtual void paint(QPainter* Painter, const QStyleOptionViewItem& Option, const QModelIndex& Index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem& Option, const QModelIndex& Index) const;

protected:
	lcPartSelectionListModel* mListModel;
};

class lcPartSelectionListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	lcPartSelectionListModel(QObject* Parent);
	virtual ~lcPartSelectionListModel();

	virtual int rowCount(const QModelIndex& Parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& Index, int Role = Qt::DisplayRole) const;
	virtual QVariant headerData(int Section, Qt::Orientation Orientation, int Role = Qt::DisplayRole) const;
	virtual Qt::ItemFlags flags(const QModelIndex& Index) const;

	PieceInfo* GetPieceInfo(const QModelIndex& Index) const
	{
		return Index.isValid() ? mParts[Index.row()].first : nullptr;
	}

	PieceInfo* GetPieceInfo(int Row) const
	{
		return mParts[Row].first;
	}

	bool GetShowDecoratedParts() const
	{
		return mShowDecoratedParts;
	}

	int GetIconSize() const
	{
		return mIconSize;
	}

	bool GetShowPartNames() const
	{
		return mShowPartNames;
	}

	bool IsColorLocked() const
	{
		return mColorLocked;
	}

	bool IsListMode() const
	{
		return mListMode;
	}

	void Redraw();
	void SetColorIndex(int ColorIndex);
	void ToggleColorLocked();
	void ToggleListMode();
	void SetCategory(int CategoryIndex);
	void SetModelsCategory();
	void SetCurrentModelCategory();
	void SetFilter(const QString& Filter);
	void RequestPreview(int InfoIndex);
	void SetShowDecoratedParts(bool Show);
	void SetIconSize(int Size);
	void SetShowPartNames(bool Show);

protected slots:
	void PartLoaded(PieceInfo* Info);

protected:
	void ClearRequests();
	void DrawPreview(int InfoIndex);

	lcPartSelectionListView* mListView;
	QVector<QPair<PieceInfo*, QPixmap>> mParts;
	QList<int> mRequestedPreviews;
	int mIconSize;
	bool mColorLocked;
	int mColorIndex;
	bool mShowPartNames;
	bool mListMode;
	bool mShowDecoratedParts;
	QByteArray mFilter;
	std::pair<lcFramebuffer, lcFramebuffer> mRenderFramebuffer;
};

class lcPartSelectionListView : public QListView
{
	Q_OBJECT

public:
	lcPartSelectionListView(QWidget* Parent);

	virtual void startDrag(Qt::DropActions SupportedActions);

	PieceInfo* GetCurrentPart() const
	{
		return mListModel->GetPieceInfo(currentIndex());
	}

	lcPartSelectionListModel* GetListModel() const
	{
		return mListModel;
	}

	void UpdateViewMode();

public slots:
	void CustomContextMenuRequested(QPoint Pos);
	void SetNoIcons();
	void SetSmallIcons();
	void SetMediumIcons();
	void SetLargeIcons();
	void SetExtraLargeIcons();
	void TogglePartNames();
	void ToggleDecoratedParts();
	void ToggleListMode();
	void ToggleFixedColor();

protected:
	void SetIconSize(int Size);

	lcPartSelectionListModel* mListModel;
};

class lcPartSelectionWidget : public QWidget
{
	Q_OBJECT

public:
	lcPartSelectionWidget(QWidget* Parent);

	void Redraw();
	void SetDefaultPart();
	void UpdateModels();
	void UpdateCategories();
	void LoadState(QSettings& Settings);
	void SaveState(QSettings& Settings);
	void DisableIconMode();

	void SetColorIndex(int ColorIndex)
	{
		mPartsWidget->GetListModel()->SetColorIndex(ColorIndex);
	}

protected slots:
	void DockLocationChanged(Qt::DockWidgetArea Area);
	void FilterChanged(const QString& Text);
	void FilterTriggered();
	void CategoryChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous);
	void PartChanged(const QModelIndex& Current, const QModelIndex& Previous);

protected:
	virtual void resizeEvent(QResizeEvent* Event);
	virtual bool event(QEvent* Event);

	QTreeWidget* mCategoriesWidget;
	QTreeWidgetItem* mAllPartsCategoryItem;
	QTreeWidgetItem* mCurrentModelCategoryItem;
	QTreeWidgetItem* mModelsCategoryItem;
	QLineEdit* mFilterWidget;
	QAction* mFilterAction;
	lcPartSelectionListView* mPartsWidget;
	QSplitter* mSplitter;
};
