#pragma once

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
	virtual bool event(QEvent* Event) override;
	virtual void paintEvent(QPaintEvent* PaintEvent) override;
	virtual void resizeEvent(QResizeEvent* ResizeEvent) override;
	virtual void mousePressEvent(QMouseEvent* MouseEvent) override;
	virtual void mouseMoveEvent(QMouseEvent* MouseEvent) override;
	virtual void keyPressEvent(QKeyEvent* KeyEvent) override;

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
	QPoint mDragStartPosition;
};

