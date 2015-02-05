#include <QtGui>
#include "lc_global.h"
#include "lc_qcolorlist.h"

lcQColorList::lcQColorList(QWidget *parent)
	: QWidget(parent)
{
	mCellRects = new QRect[gNumUserColors];
	mCellColors = new int[gNumUserColors];
	mNumCells = 0;

	mCurCell = 0;

	mColumns = 14;
	mRows = 0;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];

		for (int ColorIdx = 0; ColorIdx < Group->Colors.GetSize(); ColorIdx++)
			mCellColors[mNumCells++] = Group->Colors[ColorIdx];

		mRows += (Group->Colors.GetSize() + mColumns - 1) / mColumns;
	}

	mWidth = 0;
	mHeight = 0;

	QFontMetrics Metrics(font());
	int TextHeight = 0;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];

		mGroupRects[GroupIdx] = Metrics.boundingRect(rect(), Qt::TextSingleLine | Qt::AlignCenter, Group->Name);

		TextHeight += mGroupRects[GroupIdx].height();
	}

	mPreferredHeight = TextHeight + 10 * mRows;

	setFocusPolicy(Qt::StrongFocus);
}

lcQColorList::~lcQColorList()
{
	delete[] mCellRects;
	delete[] mCellColors;
}

QSize lcQColorList::sizeHint() const
{
	return QSize(200, mPreferredHeight);
}

void lcQColorList::setCurrentColor(int colorIndex)
{
	for (int CellIdx = 0; CellIdx < mNumCells; CellIdx++)
	{
		if (mCellColors[CellIdx] != colorIndex)
			continue;

		SelectCell(CellIdx);
		break;
	}
}

bool lcQColorList::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

		for (int CellIdx = 0; CellIdx < mNumCells; CellIdx++)
		{
			if (!mCellRects[CellIdx].contains(helpEvent->pos()))
				continue;

			lcColor* color = &gColorList[mCellColors[CellIdx]];
			QRgb rgb = qRgb(color->Value[0] * 255, color->Value[1] * 255, color->Value[2] * 255);

			QImage image(1, 1, QImage::Format_RGB888);
			image.setPixel(0, 0, rgb);

			QByteArray ba;
			QBuffer buffer(&ba);
			buffer.open(QIODevice::WriteOnly);
			image.save(&buffer, "PNG");

			int colorIndex = mCellColors[CellIdx];
			const char* format = "<table><tr><td style=\"vertical-align:middle\"><img height=16 src=\"data:image/png;base64,%1\"></td><td>%2 (%3)</td></tr></table>";
			QString text = QString(format).arg(QString(buffer.data().toBase64()), gColorList[colorIndex].Name, QString::number(gColorList[colorIndex].Code));

			QToolTip::showText(helpEvent->globalPos(), text);
			return true;
		}

		QToolTip::hideText();
		event->ignore();

		return true;
	}
	else if (event->type() == QEvent::ShortcutOverride)
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->modifiers() == Qt::NoModifier || keyEvent->modifiers() == Qt::KeypadModifier)
		{
			switch (keyEvent->key())
			{
			case Qt::Key_Left:
			case Qt::Key_Right:
			case Qt::Key_Up:
			case Qt::Key_Down:
				keyEvent->accept();
			default:
				break;
			}
		}
	}

	return QWidget::event(event);
}

void lcQColorList::mousePressEvent(QMouseEvent *event)
{
	for (int CellIdx = 0; CellIdx < mNumCells; CellIdx++)
	{
		if (!mCellRects[CellIdx].contains(event->pos()))
			continue;

		SelectCell(CellIdx);
		emit colorSelected(mCellColors[CellIdx]);

		break;
	}
}

void lcQColorList::keyPressEvent(QKeyEvent *event)
{
	int NewCell = mCurCell;

	if (event->key() == Qt::Key_Left)
	{
		if (mCurCell > 0)
			NewCell = mCurCell - 1;
	}
	else if (event->key() == Qt::Key_Right)
	{
		if (mCurCell < mNumCells - 1)
			NewCell = mCurCell + 1;
	}
	else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
	{
		if (mCurCell < 0 || mCurCell >= mNumCells)
			mCurCell = 0;

		int CurGroup = 0;
		int NumCells = 0;

		for (CurGroup = 0; CurGroup < LC_NUM_COLORGROUPS; CurGroup++)
		{
			int NumColors = gColorGroups[CurGroup].Colors.GetSize();

			if (mCurCell < NumCells + NumColors)
				break;

			NumCells += NumColors;
		}

		int Row = (mCurCell - NumCells) / mColumns;
		int Column = (mCurCell - NumCells) % mColumns;

		if (event->key() == Qt::Key_Up)
		{
			if (Row > 0)
				NewCell = mCurCell - mColumns;
			else if (CurGroup > 0)
			{
				int NumColors = gColorGroups[CurGroup - 1].Colors.GetSize();
				int NumColumns = NumColors % mColumns;

				if (NumColumns <= Column + 1)
					NewCell = mCurCell - NumColumns - mColumns;
				else
					NewCell = mCurCell - NumColumns;
			}
		}
		else if (event->key() == Qt::Key_Down)
		{
			int NumColors = gColorGroups[CurGroup].Colors.GetSize();

			if (mCurCell + mColumns < NumCells + NumColors)
				NewCell = mCurCell + mColumns;
			else
			{
				int NumColumns = NumColors % mColumns;

				if (NumColumns > Column)
				{
					if (mCurCell + NumColumns < mNumCells)
						NewCell = mCurCell + NumColumns;
				}
				else
					NewCell = mCurCell + mColumns + NumColumns;
			}
		}
	}
	else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	{
		emit colorSelected(mCellColors[mCurCell]);
	}

	if (NewCell != mCurCell)
		SelectCell(NewCell);
	else
		QWidget::keyPressEvent(event);
}

void lcQColorList::resizeEvent(QResizeEvent *event)
{
	if (mWidth == width() && mHeight == height())
		return;

	QFontMetrics Metrics(font());
	int TextHeight = 0;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];

		mGroupRects[GroupIdx] = Metrics.boundingRect(rect(), Qt::TextSingleLine | Qt::AlignCenter, Group->Name);

		TextHeight += mGroupRects[GroupIdx].height();
	}

	mPreferredHeight = TextHeight + 10 * mRows;

	float CellWidth = (float)(width() + 1) / (float)mColumns;
	float CellHeight = (float)(height() - TextHeight) / (float)mRows;

	int CurCell = 0;
	float GroupY = 0.0f;
	int TotalRows = 1;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];
		int CurColumn = 0;
		int NumRows = 0;

		mGroupRects[GroupIdx] = QRect(0, (int)GroupY, width(), mGroupRects[GroupIdx].height());
		GroupY += mGroupRects[GroupIdx].height();

		for (int ColorIdx = 0; ColorIdx < Group->Colors.GetSize(); ColorIdx++)
		{
			const int Left = CellWidth * CurColumn - 1;
			const int Right = (CurColumn + 1) * CellWidth - 1;
			const int Top = GroupY + CellHeight * NumRows;
			const int Bottom = (TotalRows != mRows) ? GroupY + CellHeight * (NumRows + 1) : height();

			mCellRects[CurCell] = QRect(Left, Top, Right - Left, Bottom - Top);

			CurColumn++;
			if (CurColumn == mColumns)
			{
				CurColumn = 0;
				NumRows++;
				TotalRows++;
			}

			CurCell++;
		}

		if (CurColumn != 0)
		{
			NumRows++;
			TotalRows++;
		}

		GroupY += NumRows * CellHeight;
	}

	mWidth = width();
	mHeight = height();

	QWidget::resizeEvent(event);
}

void lcQColorList::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);

	painter.fillRect(rect(), palette().brush(QPalette::Base));

	painter.setFont(font());
	painter.setPen(palette().color(QPalette::Text));

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];

		painter.drawText(mGroupRects[GroupIdx], Qt::TextSingleLine | Qt::AlignCenter, Group->Name);
	}

	painter.setPen(palette().color(QPalette::Shadow));

	for (int CellIdx = 0; CellIdx < mNumCells; CellIdx++)
	{
		lcColor* Color = &gColorList[mCellColors[CellIdx]];
		QColor CellColor(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255);

		painter.setBrush(CellColor);
		painter.drawRect(mCellRects[CellIdx]);
	}

	if (mCurCell < mNumCells)
	{
		lcColor* Color = &gColorList[mCellColors[mCurCell]];
		QColor EdgeColor(255 - Color->Value[0] * 255, 255 - Color->Value[1] * 255, 255 - Color->Value[2] * 255);
		QColor CellColor(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255);

		painter.setPen(EdgeColor);
		painter.setBrush(CellColor);

		QRect CellRect = mCellRects[mCurCell];
		CellRect.adjust(1, 1, -1, -1);
		painter.drawRect(CellRect);

		/*
		if (GetFocus() == this)
		{
			rc.DeflateRect(2, 2);
			dc.DrawFocusRect(rc);
		}
		*/
	}
}

void lcQColorList::SelectCell(int CellIdx)
{
	if (CellIdx < 0 || CellIdx >= mNumCells)
		return;

	if (CellIdx == mCurCell)
		return;

	update(mCellRects[mCurCell]);
	update(mCellRects[CellIdx]);
	mCurCell = CellIdx;

	emit colorChanged(mCellColors[mCurCell]);
}

#if 0

*/
void ColorPickerButton::focusInEvent(QFocusEvent *e)
{
    setFrameShadow(Raised);
    update();
    QFrame::focusOutEvent(e);
}

void ColorPickerButton::focusOutEvent(QFocusEvent *e)
{
    setFrameShadow(Raised);
    update();
    QFrame::focusOutEvent(e);
}

#endif
