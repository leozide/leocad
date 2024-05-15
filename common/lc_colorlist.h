#pragma once

struct lcColorListCell
{
	QRect Rect;
	int ColorIndex;
};

struct lcColorListGroup
{
	QRect Rect;
	QString Name;
	std::vector<size_t> Cells;
};

void lcDrawNoColorRect(QPainter& Painter, const QRect& Rect);

class lcColorList : public QWidget
{
	Q_OBJECT

public:
	lcColorList(QWidget* Parent = nullptr, bool AllowNoColor = false);
	~lcColorList() = default;

	QSize sizeHint() const override;

	void SetCurrentColor(int colorIndex);

signals:
	void ColorChanged(int colorIndex);
	void ColorSelected(int colorIndex);

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
	std::vector<lcColorListGroup> mGroups;

	size_t mCurrentCell = 0;
	quint32 mColorCode = 0;

	int mColumns = 0;
	int mRows = 0;
	int mWidth = 0;
	int mHeight = 0;
	int mPreferredHeight = 0;
	bool mAllowNoColor;

	QPoint mDragStartPosition;
};

