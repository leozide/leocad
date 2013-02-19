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

	QSize sizeHint() const;

protected:
	bool event(QEvent *event);
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	void SelectCell(int CellIdx);

	QRect mGroupRects[LC_NUM_COLORGROUPS];
	QRect* mCellRects;
	int* mCellColors;
	int mNumCells;

	int mColumns;
	int mRows;
	int mWidth;
	int mHeight;
	int mPreferredHeight;

	int mCurCell;
	QPoint mMouseDown;
	bool mTracking;
};

#endif // LC_COLORLISTWIDGET_H
