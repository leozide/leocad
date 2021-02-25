#include "lc_global.h"
#include "lc_qcolorlist.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_colors.h"

void lcDrawNoColorRect(QPainter& Painter, const QRect& Rect)
{
	Painter.setBrush(Qt::black);
	Painter.drawRect(Rect);

	const int SquareSize = 3;
	int Column = 0;

	for (;;)
	{
		int x = Rect.left() + 1 + Column * SquareSize;

		if (x >= Rect.right())
			break;

		int Row = Column & 1;

		for (;;)
		{
			int y = Rect.top() + 1 + Row * SquareSize;

			if (y >= Rect.bottom())
				break;

			QRect GridRect(x, y, SquareSize, SquareSize);

			if (GridRect.right() > Rect.right())
				GridRect.setRight(Rect.right());

			if (GridRect.bottom() > Rect.bottom())
				GridRect.setBottom(Rect.bottom());

			Painter.fillRect(GridRect, Qt::white);

			Row += 2;
		}

		Column++;
	}
}

lcQColorList::lcQColorList(QWidget* Parent, bool AllowNoColor)
	: QWidget(Parent), mAllowNoColor(AllowNoColor)
{
	setFocusPolicy(Qt::StrongFocus);

	UpdateCells();

	connect(lcGetPiecesLibrary(), &lcPiecesLibrary::ColorsLoaded, this, &lcQColorList::ColorsLoaded);
}

void lcQColorList::UpdateCells()
{
	mCells.clear();
	mGroups.clear();

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];
		lcColorListGroup ListGroup;

		for (int ColorIndex : Group->Colors)
		{
			mCells.emplace_back(lcColorListCell{ QRect(), ColorIndex });
			ListGroup.Name = Group->Name;
			ListGroup.Cells.emplace_back(mCells.size());
		}

		mGroups.emplace_back(std::move(ListGroup));
	}

	if (mAllowNoColor)
	{
		mCells.emplace_back(lcColorListCell{ QRect(), lcGetColorIndex(LC_COLOR_NOCOLOR) });
		mGroups[LC_COLORGROUP_SPECIAL].Cells.emplace_back(mCells.size());
	}

	mColumns = 14;
	mRows = 0;

	for (const lcColorListGroup& Group : mGroups)
		mRows += ((int)Group.Cells.size() + mColumns - 1) / mColumns;

	QFontMetrics Metrics(font());
	int TextHeight = 0;

	for (lcColorListGroup& Group : mGroups)
	{
		Group.Rect = Metrics.boundingRect(rect(), Qt::TextSingleLine | Qt::AlignCenter, Group.Name);

		TextHeight += Group.Rect.height();
	}

	mPreferredHeight = TextHeight + 10 * mRows;

	setMinimumHeight(TextHeight + 5 * mRows);
}

void lcQColorList::UpdateRects()
{
	QFontMetrics Metrics(font());
	int TextHeight = 0;

	for (lcColorListGroup& Group : mGroups)
	{
		Group.Rect = Metrics.boundingRect(rect(), Qt::TextSingleLine | Qt::AlignCenter, Group.Name);

		TextHeight += Group.Rect.height();
	}

	mPreferredHeight = TextHeight + 10 * mRows;

	float CellWidth = (float)(width() + 1) / (float)mColumns;
	float CellHeight = (float)(height() - TextHeight) / (float)mRows;

	while (CellWidth / CellHeight > 1.5f)
	{
		mColumns++;
		mRows = 0;

		for (const lcColorListGroup& Group : mGroups)
			mRows += ((int)Group.Cells.size() + mColumns - 1) / mColumns;

		CellWidth = (float)(width() + 1) / (float)mColumns;
		CellHeight = (float)(height() - TextHeight) / (float)mRows;

		if (mRows <= LC_NUM_COLORGROUPS)
			break;
	}

	while (CellHeight / CellWidth > 1.5f)
	{
		mColumns--;
		mRows = 0;

		for (const lcColorListGroup& Group : mGroups)
			mRows += ((int)Group.Cells.size() + mColumns - 1) / mColumns;

		CellWidth = (float)(width() + 1) / (float)mColumns;
		CellHeight = (float)(height() - TextHeight) / (float)mRows;

		if (mColumns <= 5)
			break;
	}

	int CurCell = 0;
	float GroupY = 0.0f;
	int TotalRows = 1;

	for (lcColorListGroup& Group : mGroups)
	{
		int CurColumn = 0;
		int NumRows = 0;

		Group.Rect = QRect(0, (int)GroupY, width(), Group.Rect.height());
		GroupY += Group.Rect.height();

		for (size_t ColorIdx = 0; ColorIdx < Group.Cells.size(); ColorIdx++)
		{
			const int Left = CellWidth * CurColumn - 1;
			const int Right = (CurColumn + 1) * CellWidth - 1;
			const int Top = GroupY + CellHeight * NumRows;
			const int Bottom = (TotalRows != mRows) ? GroupY + CellHeight * (NumRows + 1) : height() - 1;

			mCells[CurCell].Rect = QRect(Left, Top, Right - Left, Bottom - Top);

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
}

void lcQColorList::ColorsLoaded()
{
	UpdateCells();
	UpdateRects();

	setCurrentColor(lcGetColorIndex(mColorCode));

	update();
}

QSize lcQColorList::sizeHint() const
{
	return QSize(200, mPreferredHeight);
}

void lcQColorList::setCurrentColor(int ColorIndex)
{
	for (size_t CellIndex = 0; CellIndex < mCells.size(); CellIndex++)
	{
		if (mCells[CellIndex].ColorIndex == ColorIndex)
		{
			SelectCell(CellIndex);
			break;
		}
	}
}

bool lcQColorList::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

		for (size_t CellIndex = 0; CellIndex < mCells.size(); CellIndex++)
		{
			if (!mCells[CellIndex].Rect.contains(helpEvent->pos()))
				continue;

			lcColor* color = &gColorList[mCells[CellIndex].ColorIndex];
			QColor rgb(color->Value[0] * 255, color->Value[1] * 255, color->Value[2] * 255);

			QImage image(16, 16, QImage::Format_RGB888);
			image.fill(rgb);
			QPainter painter(&image);
			painter.setPen(Qt::darkGray);
			if (color->Code != LC_COLOR_NOCOLOR)
				painter.drawRect(0, 0, image.width() - 1, image.height() - 1);
			else
				lcDrawNoColorRect(painter, QRect(0, 0, image.width() - 1, image.height() - 1));
			painter.end();

			QByteArray ba;
			QBuffer buffer(&ba);
			buffer.open(QIODevice::WriteOnly);
			image.save(&buffer, "PNG");
			buffer.close();

			int colorIndex = mCells[CellIndex].ColorIndex;
			QString text;
			if (color->Code != LC_COLOR_NOCOLOR)
			{
				const char* format = "<table><tr><td style=\"vertical-align:middle\"><img src=\"data:image/png;base64,%1\"/></td><td>%2 (%3)</td></tr></table>";
				text = QString(format).arg(QString(buffer.data().toBase64()), gColorList[colorIndex].Name, QString::number(gColorList[colorIndex].Code));
			}
			else
			{
				const char* format = "<table><tr><td style=\"vertical-align:middle\"><img src=\"data:image/png;base64,%1\"/></td><td>%2</td></tr></table>";
				text = QString(format).arg(QString(buffer.data().toBase64()), gColorList[colorIndex].Name);
			}

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

void lcQColorList::mousePressEvent(QMouseEvent* MouseEvent)
{
	for (size_t CellIndex = 0; CellIndex < mCells.size(); CellIndex++)
	{
		if (!mCells[CellIndex].Rect.contains(MouseEvent->pos()))
			continue;

		SelectCell(CellIndex);
		emit colorSelected(mCells[CellIndex].ColorIndex);

		break;
	}

	mDragStartPosition = MouseEvent->pos();
}

void lcQColorList::mouseMoveEvent(QMouseEvent* MouseEvent)
{
	if (!(MouseEvent->buttons() & Qt::LeftButton))
		return;

	if ((MouseEvent->pos() - mDragStartPosition).manhattanLength() < QApplication::startDragDistance())
		return;

	QMimeData* MimeData = new QMimeData;
	MimeData->setData("application/vnd.leocad-color", QString::number(mCells[mCurrentCell].ColorIndex).toLatin1());

	QDrag* Drag = new QDrag(this);
	Drag->setMimeData(MimeData);

	Drag->exec(Qt::CopyAction);
}

void lcQColorList::keyPressEvent(QKeyEvent *event)
{
	size_t NewCell = mCurrentCell;

	if (event->key() == Qt::Key_Left)
	{
		if (mCurrentCell > 0)
			NewCell = mCurrentCell - 1;
	}
	else if (event->key() == Qt::Key_Right)
	{
		if (mCurrentCell < mCells.size() - 1)
			NewCell = mCurrentCell + 1;
	}
	else if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)
	{
		if (mCurrentCell >= mCells.size())
			mCurrentCell = 0;

		size_t CurGroup = 0;
		size_t NumCells = 0;

		for (CurGroup = 0; CurGroup < mGroups.size(); CurGroup++)
		{
			int NumColors = (int)mGroups[CurGroup].Cells.size();

			if (mCurrentCell < NumCells + NumColors)
				break;

			NumCells += NumColors;
		}

		size_t Row = (mCurrentCell - NumCells) / mColumns;
		size_t Column = (mCurrentCell - NumCells) % mColumns;

		if (event->key() == Qt::Key_Up)
		{
			if (Row > 0)
				NewCell = mCurrentCell - mColumns;
			else if (CurGroup > 0)
			{
				size_t NumColors = mGroups[CurGroup - 1].Cells.size();
				size_t NumColumns = NumColors % mColumns;

				if (NumColumns < Column + 1)
					NewCell = mCurrentCell - NumColumns - mColumns;
				else
					NewCell = mCurrentCell - NumColumns;
			}
		}
		else if (event->key() == Qt::Key_Down)
		{
			int NumColors = (int)mGroups[CurGroup].Cells.size();

			if (mCurrentCell + mColumns < NumCells + NumColors)
				NewCell = mCurrentCell + mColumns;
			else
			{
				size_t NumColumns = NumColors % mColumns;

				if (NumColumns > Column)
				{
					if (mCurrentCell + NumColumns < mCells.size())
						NewCell = mCurrentCell + NumColumns;
				}
				else
					NewCell = mCurrentCell + mColumns + NumColumns;
			}
		}
	}
	else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	{
		emit colorSelected(mCells[mCurrentCell].ColorIndex);
	}

	if (NewCell != mCurrentCell)
		SelectCell(NewCell);
	else
		QWidget::keyPressEvent(event);
}

void lcQColorList::resizeEvent(QResizeEvent* Event)
{
	if (mWidth == width() && mHeight == height())
		return;

	UpdateRects();

	mWidth = width();
	mHeight = height();

	QWidget::resizeEvent(Event);
}

void lcQColorList::paintEvent(QPaintEvent* Event)
{
	Q_UNUSED(Event);

	QPainter Painter(this);

	Painter.fillRect(rect(), palette().brush(QPalette::Window));

	Painter.setFont(font());
	Painter.setPen(palette().color(QPalette::Text));

	for (const lcColorListGroup& Group : mGroups)
		Painter.drawText(Group.Rect, Qt::TextSingleLine | Qt::AlignLeft, Group.Name);

	Painter.setPen(palette().color(QPalette::Shadow));

	for (size_t CellIndex = 0; CellIndex < mCells.size(); CellIndex++)
	{
		const lcColor* Color = &gColorList[mCells[CellIndex].ColorIndex];

		const QRect& Rect = mCells[CellIndex].Rect;

		if (Color->Code != LC_COLOR_NOCOLOR)
		{
			QColor CellColor(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255);

			Painter.setBrush(CellColor);
			Painter.drawRect(Rect);
		}
		else
			lcDrawNoColorRect(Painter, Rect);
	}

	if (mCurrentCell < mCells.size())
	{
		const lcColor* Color = &gColorList[mCells[mCurrentCell].ColorIndex];
		QColor EdgeColor(255 - Color->Value[0] * 255, 255 - Color->Value[1] * 255, 255 - Color->Value[2] * 255);

		Painter.setPen(EdgeColor);
		Painter.setBrush(Qt::NoBrush);

		QRect CellRect = mCells[mCurrentCell].Rect;
		CellRect.adjust(1, 1, -1, -1);
		Painter.drawRect(CellRect);

		/*
		if (GetFocus() == this)
		{
			rc.DeflateRect(2, 2);
			dc.DrawFocusRect(rc);
		}
		*/
	}
}

void lcQColorList::SelectCell(size_t CellIndex)
{
	if (CellIndex >= mCells.size())
		return;

	if (CellIndex == mCurrentCell)
		return;

	mCurrentCell = CellIndex;
	mColorCode = lcGetColorCode(mCells[CellIndex].ColorIndex);

	emit colorChanged(mCells[mCurrentCell].ColorIndex);
	update();
}
