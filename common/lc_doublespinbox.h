#pragma once

#include "lc_objectproperty.h"

class lcDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

public:
	lcDoubleSpinBox(QWidget* Parent);
	virtual ~lcDoubleSpinBox() = default;

	void SetValue(double Value);
	void SetSnap(lcFloatPropertySnap Snap);

	QString	textFromValue(double Value) const override;
	bool eventFilter(QObject* Object, QEvent* Event) override;

	void mousePressEvent(QMouseEvent* MouseEvent) override;
	void mouseMoveEvent(QMouseEvent* MouseEvent) override;
	void mouseReleaseEvent(QMouseEvent* MouseEvent) override;
	void focusOutEvent(QFocusEvent* FocusEvent) override;

signals:
	void EditingCanceled();
	void EditingFinished();

protected slots:
	void CancelEditing();
	void FinishEditing();
	void ReturnPressed();

protected:
	bool event(QEvent* Event) override;

	static QPoint GetGlobalMousePosition(QMouseEvent* MouseEvent)
	{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
		return MouseEvent->globalPos();
#else
		return MouseEvent->globalPosition().toPoint();
#endif
	}

	void HandleMousePressEvent(QMouseEvent* MouseEvent);
	bool HandleMouseMoveEvent(QMouseEvent* MouseEvent, QObject* Object);
	void HandleMouseReleaseEvent(QMouseEvent* MouseEvent);

	enum class DragMode
	{
		None,
		Start,
		Value
	};

	double mInitialValue = 0.0;
	QPoint mLastPosition;
	DragMode mDragMode = DragMode::None;
	lcFloatPropertySnap mSnap = lcFloatPropertySnap::Auto;
};
