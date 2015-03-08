#ifndef _LC_TIMELINEWIDGET_H_
#define _LC_TIMELINEWIDGET_H_

class lcTimelineWidget : public QTreeWidget
{
public:
	lcTimelineWidget(QWidget* Parent);
	virtual ~lcTimelineWidget();

	void Update();

protected:
	virtual void dropEvent(QDropEvent* Event);
};

#endif // _LC_TIMELINEWIDGET_H_
