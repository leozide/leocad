#pragma once

enum class lcFloatPropertySnap;

class lcLadderWidget: public QWidget
{
	Q_OBJECT

public:
	lcLadderWidget(QAbstractSpinBox* SpinBox, lcFloatPropertySnap Snap);
	virtual ~lcLadderWidget() = default;

	void Show();

	bool eventFilter(QObject* Object, QEvent* Event) override;
	void mousePressEvent(QMouseEvent* MouseEvent) override;
	void mouseMoveEvent(QMouseEvent* MouseEvent) override;
	void mouseReleaseEvent(QMouseEvent* MouseEvent) override;

signals:
	void EditingCanceled();
	void EditingFinished();

protected slots:
	void CancelEditing();
	void FinishEditing();

protected:
	void CalculateSteps(lcFloatPropertySnap Snap);
	void UpdateMousePosition();

	void paintEvent(QPaintEvent* PaintEvent) override;

	QPointer<QAbstractSpinBox> mSpinBox = nullptr;
	std::vector<double> mSteps;
	int mCurrentStep = -1;
	int mLastMousePositionX = 0;
};
