#ifndef _LC_TIMELINEWIDGET_H_
#define _LC_TIMELINEWIDGET_H_

class lcTimelineWidget : public QTreeWidget
{
public:
	lcTimelineWidget(QWidget* Parent);
	virtual ~lcTimelineWidget();

	void Update();
};

#endif // _LC_TIMELINEWIDGET_H_
