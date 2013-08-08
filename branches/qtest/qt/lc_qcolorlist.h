#ifndef _LC_QCOLORLIST_H_
#define _LC_QCOLORLIST_H_

#include <QWidget>
#include "lc_colors.h"

class lcQColorList : public QWidget
{
	Q_OBJECT

public:
	lcQColorList(QWidget *parent = 0);
	~lcQColorList();

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

#endif // _LC_QCOLORLIST_H_
