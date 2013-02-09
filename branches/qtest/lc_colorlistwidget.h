#ifndef LC_COLORLISTWIDGET_H
#define LC_COLORLISTWIDGET_H

#include <QWidget>
#include "lc_global.h" // TODO: wrong include
#include "lc_colors.h"

class lcColorListWidget : public QWidget
{
	Q_OBJECT

public:
	lcColorListWidget(QWidget *parent = 0);
	~lcColorListWidget();

protected:
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);

	QRect mGroupRects[LC_NUM_COLORGROUPS];
	QRect* mCellRects;
	int* mCellColors;
	int mNumCells;

	int mColumns;
	int mRows;
	int mWidth;
	int mHeight;
};

#endif // LC_COLORLISTWIDGET_H
