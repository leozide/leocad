#pragma once

class lcTimelineWidget : public QTreeWidget
{
	Q_OBJECT

public:
	lcTimelineWidget(QWidget* Parent);
	~lcTimelineWidget();

	void Update(bool Clear, bool UpdateItems);
	void UpdateSelection();

	void InsertStepBefore();
	void InsertStepAfter();
	void RemoveStep();
	void MoveSelection();
	void SetCurrentStep();

public slots:
	void CurrentItemChanged(QTreeWidgetItem* Current, QTreeWidgetItem* Previous);
	void ItemSelectionChanged();
	void CustomMenuRequested(QPoint Pos);
	void PreviewSelection(QTreeWidgetItem* Current);

protected:
	void dropEvent(QDropEvent* DropEvent) override;
	void mousePressEvent(QMouseEvent* MouseEvent) override;
	void mouseDoubleClickEvent(QMouseEvent* MouseEvent) override;
	void UpdateModel();
	void UpdateCurrentStepItem();

	QMap<int, QIcon> mIcons;
	QMap<lcPiece*, QTreeWidgetItem*> mItems;
	QTreeWidgetItem* mCurrentStepItem;
	bool mIgnoreUpdates;
};
