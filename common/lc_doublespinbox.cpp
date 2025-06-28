#include "lc_global.h"
#include "lc_doublespinbox.h"
#include "lc_ladderwidget.h"
#include "lc_qutils.h"
#include "lc_mainwindow.h"

lcDoubleSpinBox::lcDoubleSpinBox(QWidget* Parent)
	: QDoubleSpinBox(Parent)
{
	lineEdit()->installEventFilter(this);

	connect(lineEdit(), &QLineEdit::returnPressed, this, &lcDoubleSpinBox::ReturnPressed);
}

void lcDoubleSpinBox::SetValue(double Value)
{
	if (!hasFocus())
		mInitialValue = Value;

	setValue(Value);
}

void lcDoubleSpinBox::SetSnap(lcFloatPropertySnap Snap)
{
	mSnap = Snap;

	switch (Snap)
	{
	case lcFloatPropertySnap::Auto:
		setSingleStep(10.0);
		break;

	case lcFloatPropertySnap::PiecePositionXY:
		setSingleStep(gMainWindow->GetMoveXYSnap());
		break;

	case lcFloatPropertySnap::PiecePositionZ:
		setSingleStep(gMainWindow->GetMoveZSnap());
		break;

	case lcFloatPropertySnap::Position:
		setSingleStep(10.0);
		break;

	case lcFloatPropertySnap::Rotation:
		setSingleStep(gMainWindow->GetAngleSnap());
		break;
	}
}

QString	lcDoubleSpinBox::textFromValue(double Value) const
{
	return lcFormatValueLocalized(Value);
}

void lcDoubleSpinBox::CancelEditing()
{
	if (value() == mInitialValue)
		return;

	emit EditingCanceled();

	setValue(mInitialValue);
}

void lcDoubleSpinBox::FinishEditing()
{
	if (value() == mInitialValue)
		return;

	emit EditingFinished();

	mInitialValue = value();

	clearFocus();
}

void lcDoubleSpinBox::ReturnPressed()
{
	FinishEditing();
}

void lcDoubleSpinBox::HandleMousePressEvent(QMouseEvent* MouseEvent)
{
	if (MouseEvent->buttons() == Qt::LeftButton)
	{
		mDragMode = DragMode::Start;
		mLastPosition = GetGlobalMousePosition(MouseEvent);
	}
	else
	{
		if (mDragMode == DragMode::Value)
			CancelEditing();

		if (mDragMode != DragMode::None)
		{
			mDragMode = DragMode::None;
			return;
		}

		if (MouseEvent->buttons() == Qt::MiddleButton)
		{
			lcLadderWidget* LadderWidget = new lcLadderWidget(this, mSnap);

			connect(LadderWidget, &lcLadderWidget::EditingCanceled, this, &lcDoubleSpinBox::CancelEditing);
			connect(LadderWidget, &lcLadderWidget::EditingFinished, this, &lcDoubleSpinBox::FinishEditing);

			mInitialValue = value();

			LadderWidget->Show();
		}
	}
}

bool lcDoubleSpinBox::HandleMouseMoveEvent(QMouseEvent* MouseEvent, QObject* Object)
{
	if (mDragMode != DragMode::None)
	{
		if (mDragMode == DragMode::Start)
		{
			if (lineEdit()->selectionLength() > 0 && Object != this)
			{
				mDragMode = DragMode::None;
			}
			else
			{
				QPoint Position = GetGlobalMousePosition(MouseEvent);

				if (abs(mLastPosition.y() - Position.y()) > 3)
				{
					mDragMode = DragMode::Value;
					mLastPosition = QCursor::pos();
				}
			}
		}

		if (mDragMode == DragMode::Value)
		{
			QPoint Position = QCursor::pos();

			stepBy(mLastPosition.y() - Position.y());

			QScreen* Screen = QGuiApplication::screenAt(mapToGlobal(QPoint(width() / 2, height() / 2)));

			if (Screen)
			{
				int ScreenHeight = Screen->geometry().height();

				if (Position.y() <= 0)
				{
					Position.setY(ScreenHeight - 2);

					QCursor::setPos(Position);
				}
				else if (Position.y() >= ScreenHeight - 1)
				{
					Position.setY(1);

					QCursor::setPos(Position);
				}
			}

			mLastPosition = Position;

			MouseEvent->accept();

			return true;
		}
	}

	return false;
}

void lcDoubleSpinBox::HandleMouseReleaseEvent(QMouseEvent* MouseEvent)
{
	Q_UNUSED(MouseEvent);

	if (mDragMode == DragMode::Value)
		FinishEditing();

	mDragMode = DragMode::None;
}

bool lcDoubleSpinBox::eventFilter(QObject* Object, QEvent* Event)
{
	switch (Event->type())
	{
	case QEvent::MouseButtonPress:
		if (Object == lineEdit())
			HandleMousePressEvent(reinterpret_cast<QMouseEvent*>(Event));
		break;

	case QEvent::MouseMove:
		if (Object == lineEdit())
			HandleMouseMoveEvent(reinterpret_cast<QMouseEvent*>(Event), Object);
		break;

	case QEvent::MouseButtonRelease:
		if (Object == lineEdit())
			HandleMouseReleaseEvent(reinterpret_cast<QMouseEvent*>(Event));
		break;

	default:
		break;
	}

	return QDoubleSpinBox::eventFilter(Object, Event);
}

void lcDoubleSpinBox::mousePressEvent(QMouseEvent* MouseEvent)
{
	HandleMousePressEvent(MouseEvent);

	QDoubleSpinBox::mousePressEvent(MouseEvent);
}

void lcDoubleSpinBox::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	if (!HandleMouseMoveEvent(MouseEvent, this))
		QDoubleSpinBox::mouseMoveEvent(MouseEvent);
}

void lcDoubleSpinBox::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	HandleMouseReleaseEvent(MouseEvent);

	QDoubleSpinBox::mouseReleaseEvent(MouseEvent);
}

bool lcDoubleSpinBox::event(QEvent* Event)
{
	if (Event->type() == QEvent::ShortcutOverride)
	{
		QKeyEvent* KeyEvent = static_cast<QKeyEvent*>(Event);

		if (KeyEvent->key() == Qt::Key_Escape)
		{
			if (mDragMode == DragMode::Value)
				CancelEditing();

			mDragMode = DragMode::None;

			clearFocus();
			KeyEvent->accept();
		}
	}

	return QDoubleSpinBox::event(Event);
}

void lcDoubleSpinBox::focusOutEvent(QFocusEvent* FocusEvent)
{
	if (value() != mInitialValue)
		CancelEditing();

	QDoubleSpinBox::focusOutEvent(FocusEvent);
}
