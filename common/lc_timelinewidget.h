#ifndef _LC_TIMELINEWIDGET_H_
#define _LC_TIMELINEWIDGET_H_

class lcTimelineWidget : public QTreeWidget
{
	Q_OBJECT

public:
	lcTimelineWidget(QWidget* Parent);
	virtual ~lcTimelineWidget();

	void Update();
	void UpdateSelection();

public slots:
	void InsertStep();
	void RemoveStep();
	void ItemSelectionChanged();
	void CustomMenuRequested(QPoint Pos);

protected:
	virtual void dropEvent(QDropEvent* Event);

	QMap<lcPiece*, QTreeWidgetItem*> mItems;
	bool mIgnoreUpdates;
};

#endif // _LC_TIMELINEWIDGET_H_
