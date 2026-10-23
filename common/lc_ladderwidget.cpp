#include "lc_global.h"
#include "lc_ladderwidget.h"
#include "lc_objectproperty.h"
#include "lc_qutils.h"

lcLadderWidget::lcLadderWidget(QAbstractSpinBox* SpinBox, lcFloatPropertySnap Snap)
	: QWidget(SpinBox, Qt::Popup | Qt::Sheet), mSpinBox(SpinBox)
{
	mSpinBox->installEventFilter(this);

	CalculateSteps(Snap);
}

void lcLadderWidget::Show()
{
	int LineHeight = QFontMetrics(font()).height();
	int CellSize = LineHeight * 4;

	setFixedSize(CellSize, static_cast<int>(mSteps.size()) * CellSize);
	adjustSize();

	const QPoint CursorPosition = QCursor::pos();
	const QPoint Position = CursorPosition - QPoint(width() / 2, height() / 2);
	move(lcGetPopupPosition(Position, size(), CursorPosition));

	mLastMousePositionX = mapFromGlobal(QCursor::pos()).x();
	UpdateMousePosition();

	show();
	grabMouse();
}

void lcLadderWidget::CalculateSteps(lcFloatPropertySnap Snap)
{
	QDoubleSpinBox* SpinBox = qobject_cast<QDoubleSpinBox*>(mSpinBox);

	switch (Snap)
	{
	case lcFloatPropertySnap::Auto:
		if (SpinBox)
		{
			double Max = SpinBox->maximum();

			if (Max > 200.0)
				mSteps = { 1000.0, 100.0, 10.0, 1.0, 0.1 };
			else
				mSteps = { 30.0, 10.0, 1.0, 0.1, 0.01 };
		}
		break;

	case lcFloatPropertySnap::PiecePositionXY:
		mSteps = { 80.0, 40.0, 24.0, 20.0, 10.0, 8.0, 1.0 };
		break;

	case lcFloatPropertySnap::PiecePositionZ:
		mSteps = { 192.0, 96.0, 48.0, 24.0, 10.0, 8.0, 1.0 };
		break;

	case lcFloatPropertySnap::Position:
		mSteps = { 1000.0, 100.0, 50.0, 10.0, 5.0, 1.0, 0.1 };
		break;

	case lcFloatPropertySnap::Rotation:
		mSteps = { 45.0, 30.0, 22.5, 15.0, 5.0, 1.0, 0.1 };
		break;
	}

	if (mSteps.empty())
		mSteps = { 100.0, 10.0, 1.0, 0.1, 0.01 };
}

void lcLadderWidget::UpdateMousePosition()
{
	QPoint MousePosition = mapFromGlobal(QCursor::pos());

	if (rect().contains(MousePosition))
	{
		int CellHeight = height() / static_cast<int>(mSteps.size());
		int CurrentStep = MousePosition.y() / CellHeight;

		if (CurrentStep != mCurrentStep)
		{
			mCurrentStep = CurrentStep;

			update();
		}
	}

	if (mCurrentStep != -1)
	{
		QDoubleSpinBox* SpinBox = qobject_cast<QDoubleSpinBox*>(mSpinBox);

		if (SpinBox)
		{
			const int Sensitivity = 15;
			int Steps = (MousePosition.x() - mLastMousePositionX) / Sensitivity;

			if (Steps)
			{
				mLastMousePositionX = MousePosition.x();

				SpinBox->setValue(SpinBox->value() + mSteps[mCurrentStep] * Steps);

				update();
			}
		}
	}
}

void lcLadderWidget::CancelEditing()
{
	emit EditingCanceled();

	releaseMouse();
	deleteLater();
}

void lcLadderWidget::FinishEditing()
{
	emit EditingFinished();

	releaseMouse();
	deleteLater();
}

void lcLadderWidget::paintEvent(QPaintEvent* Event)
{
	Q_UNUSED(Event);

	QPainter Painter(this);

	int CellWidth = width();
	int CellHeight = height() / static_cast<int>(mSteps.size());

	Painter.fillRect(rect(), palette().brush(QPalette::Base));

	if (mCurrentStep != -1)
	{
		QRect Rect(0, mCurrentStep * CellHeight, CellWidth, CellHeight);

		Painter.fillRect(Rect, palette().brush(QPalette::Highlight));
	}

	Painter.setFont(font());
	Painter.setPen(palette().color(QPalette::Text));

	for (int Step = 0; Step < static_cast<int>(mSteps.size()); Step++)
	{
		QRect Rect(0, Step * CellHeight, CellWidth, CellHeight);
		QTextOption TextOption(Qt::AlignCenter);
		QString Text = lcFormatDoubleLocalized(mSteps[Step], mSpinBox->locale(), 'g', 6);

		if (Step == mCurrentStep && mSpinBox)
			Text = QString("%1\n\n(%2)").arg(Text, mSpinBox->text());

		Painter.drawText(Rect, Text, TextOption);
	}

	Painter.setPen(palette().color(QPalette::Shadow));

	QRect Rect(rect());
	Rect.adjust(0, 0, -1, -1);

	Painter.drawRect(Rect);

	for (int Step = 1; Step < static_cast<int>(mSteps.size()); Step++)
		Painter.drawLine(0, Step * CellHeight, CellWidth, Step * CellHeight);
}

bool lcLadderWidget::eventFilter(QObject* Object, QEvent* Event)
{
	if (Event->type() == QEvent::ShortcutOverride || Event->type() == QEvent::KeyPress)
	{
		QKeyEvent* KeyEvent = static_cast<QKeyEvent*>(Event);

		if (KeyEvent->key() == Qt::Key_Escape)
		{
			CancelEditing();

			KeyEvent->accept();

			return true;
		}
	}

	return QWidget::eventFilter(Object, Event);
}

void lcLadderWidget::mousePressEvent(QMouseEvent* MouseEvent)
{
	CancelEditing();

	QWidget::mousePressEvent(MouseEvent);
}

void lcLadderWidget::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	UpdateMousePosition();

	QWidget::mouseMoveEvent(MouseEvent);
}

void lcLadderWidget::mouseReleaseEvent(QMouseEvent* MouseEvent)
{
	FinishEditing();

	QWidget::mouseReleaseEvent(MouseEvent);
}
