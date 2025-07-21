#include "lc_global.h"
#include "lc_doublespinbox.h"
#include "lc_ladderwidget.h"
#include "lc_qutils.h"
#include "lc_mainwindow.h"

lcDoubleSpinBox::lcDoubleSpinBox(QWidget* Parent)
	: QDoubleSpinBox(Parent)
{
	lineEdit()->installEventFilter(this);

	connect(this, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &lcDoubleSpinBox::ValueChanged);
	connect(lineEdit(), &QLineEdit::returnPressed, this, &lcDoubleSpinBox::ReturnPressed);
}

void lcDoubleSpinBox::SetValue(double Value)
{
	mModified = false;

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

void lcDoubleSpinBox::ShowLadderWidget()
{
	lcLadderWidget* LadderWidget = new lcLadderWidget(this, mSnap);

	mLadderWidgetInitialValue = value();

	connect(LadderWidget, &lcLadderWidget::EditingCanceled, this, &lcDoubleSpinBox::LadderWidgetCanceled);

	LadderWidget->Show();
}

void lcDoubleSpinBox::LadderWidgetCanceled()
{
	setValue(mLadderWidgetInitialValue);
}

QString	lcDoubleSpinBox::textFromValue(double Value) const
{
	return lcFormatValueLocalized(Value);
}

void lcDoubleSpinBox::ValueChanged()
{
	mModified = true;
}

void lcDoubleSpinBox::CancelEditing()
{
	if (mModified)
	{
		emit EditingCanceled();
	}
}

void lcDoubleSpinBox::FinishEditing()
{
	if (mModified)
	{
		emit EditingFinished();
	}
}

void lcDoubleSpinBox::ReturnPressed()
{
	FinishEditing();
}

bool lcDoubleSpinBox::eventFilter(QObject* Object, QEvent* Event)
{
	if (mDragMode == DragMode::None && Event->type() == QEvent::MouseButtonPress && Object == lineEdit())
	{
		QMouseEvent* MouseEvent = reinterpret_cast<QMouseEvent*>(Event);

		if (MouseEvent->buttons() == Qt::MiddleButton)
			ShowLadderWidget();
	}

	return QDoubleSpinBox::eventFilter(Object, Event);
}

void lcDoubleSpinBox::mousePressEvent(QMouseEvent* MouseEvent)
{
	if (mDragMode == DragMode::Dragging)
	{
		if (MouseEvent->buttons() != Qt::LeftButton)
		{
			CancelEditing();

			mDragMode = DragMode::None;
		}
	}
	else if (mDragMode == DragMode::None)
	{
		if (MouseEvent->buttons() == Qt::LeftButton)
		{
			mDragMode = DragMode::Start;
			mLastPosition = GetGlobalMousePosition(MouseEvent);
		}
		else if (MouseEvent->buttons() == Qt::MiddleButton)
		{
			ShowLadderWidget();
		}
	}
	else
	{
		mDragMode = DragMode::None;
	}

	QDoubleSpinBox::mousePressEvent(MouseEvent);
}

void lcDoubleSpinBox::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	if (mDragMode == DragMode::Dragging)
	{
		QPoint Position = QCursor::pos();

		stepBy(mLastPosition.y() - Position.y());

		QScreen* Screen = QGuiApplication::screenAt(Position);

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
	}
	else if (mDragMode == DragMode::Start)
	{
		QPoint Position = GetGlobalMousePosition(MouseEvent);

		if (abs(mLastPosition.y() - Position.y()) > 3)
		{
			mDragMode = DragMode::Dragging;
			mLastPosition = QCursor::pos();
		}
	}

	QDoubleSpinBox::mouseMoveEvent(MouseEvent);
}

void lcDoubleSpinBox::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	mDragMode = DragMode::None;

	QDoubleSpinBox::mouseReleaseEvent(MouseEvent);
}

bool lcDoubleSpinBox::event(QEvent* Event)
{
	if (Event->type() == QEvent::KeyPress)
	{
		QKeyEvent* KeyEvent = static_cast<QKeyEvent*>(Event);

		if (KeyEvent->key() == Qt::Key_Escape)
		{
			CancelEditing();

			mDragMode = DragMode::None;

			clearFocus();
			KeyEvent->accept();

			return true;
		}
		else if (KeyEvent->key() == Qt::Key_Return || KeyEvent->key() == Qt::Key_Enter)
		{
			FinishEditing();

			KeyEvent->accept();

			return true;
		}
	}

	return QDoubleSpinBox::event(Event);
}

void lcDoubleSpinBox::focusInEvent(QFocusEvent* FocusEvent)
{
	mModified = false;

	QDoubleSpinBox::focusInEvent(FocusEvent);
}

void lcDoubleSpinBox::focusOutEvent(QFocusEvent* FocusEvent)
{
	FinishEditing();

	QDoubleSpinBox::focusOutEvent(FocusEvent);
}
