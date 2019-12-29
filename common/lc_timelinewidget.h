#pragma once

class lcTimelineWidget : public QTreeWidget
{
	Q_OBJECT

public:
	lcTimelineWidget(QWidget* Parent);
	virtual ~lcTimelineWidget();

	void Update(bool Clear, bool UpdateItems);
	void UpdateSelection();

	void InsertStep();
	void RemoveStep();
	void MoveSelection();
	void SetCurrentStep();

public slots:
	void CurrentItemChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous);
	void ItemSelectionChanged();
	void CustomMenuRequested(QPoint Pos);

protected:
	virtual void dropEvent(QDropEvent* Event);
	virtual void mousePressEvent(QMouseEvent* Event);
	void UpdateModel();
	void UpdateCurrentStepItem();

	QMap<int, QIcon> mIcons;
	QMap<lcPiece*, QTreeWidgetItem*> mItems;
	QTreeWidgetItem* mCurrentStepItem;
	bool mIgnoreUpdates;
};

