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

protected:
	virtual bool filterAcceptsRow(int SourceRow, const QModelIndex& SourceParent) const;

	QByteArray mFilter;
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

	void Redraw();
	void SetCategory(int CategoryIndex);
	void SetModelsCategory();
	void RequestPreview(int InfoIndex);
	void SetIconSize(int Size);

protected slots:
	void PartLoaded(PieceInfo* Info);

protected:
	void ClearRequests();
	void DrawPreview(int InfoIndex);

	lcPartSelectionListView* mListView;
	QVector<QPair<PieceInfo*, QPixmap>> mParts;
	QList<int> mRequestedPreviews;
	int mIconSize;
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
	void SetSmallIcons();
	void SetMediumIcons();
	void SetLargeIcons();
	void SetText();

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

protected slots:
	void FilterChanged(const QString& Text);
	void CategoryChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous);
	void PartChanged(const QModelIndex& Current, const QModelIndex& Previous);

protected:
	virtual void resizeEvent(QResizeEvent* Event);

	QTreeWidget* mCategoriesWidget;
	QTreeWidgetItem* mModelsCategoryItem;
	QLineEdit* mFilterWidget;
	lcPartSelectionListView* mPartsWidget;
	QSplitter* mSplitter;
};

#endif // _LC_PARTSELECTIONWIDGET_H_
