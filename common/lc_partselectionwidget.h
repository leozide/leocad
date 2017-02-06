#ifndef _LC_PARTSELECTIONWIDGET_H_
#define _LC_PARTSELECTIONWIDGET_H_

class lcPartSelectionListModel;
class lcPartSelectionListView;

class lcPartSelectionFilterModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	lcPartSelectionFilterModel(QObject* Parent);

	void SetFilter(const QString& Filter);
	void SetShowDecoratedParts(bool Show);

	bool GetShowDecoratedParts() const
	{
		return mShowDecoratedParts;
	}

protected:
	virtual bool filterAcceptsRow(int SourceRow, const QModelIndex& SourceParent) const;

	QByteArray mFilter;
	bool mShowDecoratedParts;
};

class lcPartSelectionItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	lcPartSelectionItemDelegate(QObject* Parent, lcPartSelectionListModel* ListModel, lcPartSelectionFilterModel* FilterModel)
		: QStyledItemDelegate(Parent), mListModel(ListModel), mFilterModel(FilterModel)
	{
	}

	virtual void paint(QPainter* Painter, const QStyleOptionViewItem& Option, const QModelIndex& Index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem& Option, const QModelIndex& Index) const;

protected:
	lcPartSelectionListModel* mListModel;
	lcPartSelectionFilterModel* mFilterModel;
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

	PieceInfo* GetPieceInfo(QModelIndex Index) const
	{
		return Index.isValid() ? mParts[Index.row()].first : NULL;
	}

	PieceInfo* GetPieceInfo(int Row) const
	{
		return mParts[Row].first;
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

	void Redraw();
	void SetColorIndex(int ColorIndex);
	void ToggleColorLocked();
	void SetCategory(int CategoryIndex);
	void SetModelsCategory();
	void SetCurrentModelCategory();
	void RequestPreview(int InfoIndex);
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
};

class lcPartSelectionListView : public QListView
{
	Q_OBJECT

public:
	lcPartSelectionListView(QWidget* Parent);

	virtual void startDrag(Qt::DropActions SupportedActions);

	PieceInfo* GetCurrentPart() const
	{
		return mListModel->GetPieceInfo(mFilterModel->mapToSource(currentIndex()));
	}

	lcPartSelectionListModel* GetListModel() const
	{
		return mListModel;
	}

	lcPartSelectionFilterModel* GetFilterModel() const
	{
		return mFilterModel;
	}

protected slots:
	void CustomContextMenuRequested(QPoint Pos);
	void SetNoIcons();
	void SetSmallIcons();
	void SetMediumIcons();
	void SetLargeIcons();
	void TogglePartNames();
	void ToggleDecoratedParts();
	void ToggleFixedColor();

protected:
	void SetIconSize(int Size);

	lcPartSelectionListModel* mListModel;
	lcPartSelectionFilterModel* mFilterModel;

//	QSize sizeHint() const;
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

	QTreeWidget* mCategoriesWidget;
	QTreeWidgetItem* mAllPartsCategoryItem;
	QTreeWidgetItem* mCurrentModelCategoryItem;
	QTreeWidgetItem* mModelsCategoryItem;
	QLineEdit* mFilterWidget;
	QAction* mFilterAction;
	lcPartSelectionListView* mPartsWidget;
	QSplitter* mSplitter;
};

#endif // _LC_PARTSELECTIONWIDGET_H_
