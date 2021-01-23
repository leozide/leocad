#pragma once

struct lcColorListCell
{
	QRect Rect;
	int ColorIndex;
};

class lcQColorList : public QWidget
{
	Q_OBJECT

public:
	lcQColorList(QWidget* Parent = nullptr);
	~lcQColorList() = default;

	QSize sizeHint() const override;

	void setCurrentColor(int colorIndex);

signals:
	void colorChanged(int colorIndex);
	void colorSelected(int colorIndex);

protected slots:
	void ColorsLoaded();

protected:
	void UpdateCells();
	void UpdateRects();
	void SelectCell(size_t CellIndex);

	bool event(QEvent* Event) override;
	void paintEvent(QPaintEvent* PaintEvent) override;
	void resizeEvent(QResizeEvent* ResizeEvent) override;
	void mousePressEvent(QMouseEvent* MouseEvent) override;
	void mouseMoveEvent(QMouseEvent* MouseEvent) override;
	void keyPressEvent(QKeyEvent* KeyEvent) override;

	std::vector<lcColorListCell> mCells;
	std::vector<QRect> mGroupRects;

	size_t mCurrentCell = 0;
	quint32 mColorCode = 0;

	int mColumns = 0;
	int mRows = 0;
	int mWidth = 0;
	int mHeight = 0;
	int mPreferredHeight = 0;

	QPoint mDragStartPosition;
};

