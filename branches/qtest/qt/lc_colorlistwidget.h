#ifndef LC_COLORLISTWIDGET_H
#define LC_COLORLISTWIDGET_H

#include <QWidget>
#include "lc_colors.h"

class lcColorListWidget : public QWidget
{
	Q_OBJECT

public:
	lcColorListWidget(QWidget *parent = 0);
	~lcColorListWidget();

	QSize sizeHint() const;

	void setCurrentColor(int colorIndex);

signals:
	void colorChanged(int colorIndex);
	void colorSelected(int colorIndex);

protected:
	bool event(QEvent *event);
	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);

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
};

#endif // LC_COLORLISTWIDGET_H
