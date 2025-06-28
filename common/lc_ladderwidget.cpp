#include "lc_global.h"
#include "lc_ladderwidget.h"

lcLadderWidget::lcLadderWidget(QAbstractSpinBox* SpinBox)
	: QWidget(nullptr, Qt::Popup | Qt::Sheet), mSpinBox(SpinBox)
{
	mSpinBox->installEventFilter(this);
}

void lcLadderWidget::Show()
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
	QScreen* Screen = screen();
	const QRect Desktop = Screen ? Screen->geometry() : QRect();
#else
	const QRect Desktop = QApplication::desktop()->geometry();
#endif

	mSteps = { 100, 10, 1 };
	int LineHeight = QFontMetrics(font()).height();
	int CellSize = LineHeight * 4;

	setFixedSize(CellSize, static_cast<int>(mSteps.size()) * CellSize);
	adjustSize();

	int HalfWidth = width() / 2;
	int HalfHeight = height() / 2;
	QPoint Position = QCursor::pos() - QPoint(HalfWidth, HalfHeight);

	if (Position.x() - HalfWidth < Desktop.left())
		Position.setX(Desktop.left() + HalfWidth);

	if (Position.y() - HalfHeight < Desktop.top())
		Position.setY(Desktop.top() + HalfHeight);

	if ((Position.x() + HalfHeight) > Desktop.width())
		Position.setX(Desktop.width() - HalfHeight);

	if (Position.y() + HalfHeight > Desktop.bottom())
		Position.setY(Desktop.bottom() - HalfHeight);
	
	move(Position);

	mLastMousePositionX = mapFromGlobal(QCursor::pos()).x();
	UpdateMousePosition();

	show();
	grabMouse();
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
			int Steps = (MousePosition.x() - mLastMousePositionX) / 10;

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
		QString Text = QString::number(mSteps[Step]);

		if (Step == mCurrentStep)
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
	if (Event->type() == QEvent::ShortcutOverride)
	{
		QKeyEvent* KeyEvent = static_cast<QKeyEvent*>(Event);

		if (KeyEvent->key() == Qt::Key_Escape)
		{
			CancelEditing();

			KeyEvent->accept();
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
