#include "lc_global.h"
#include "lc_colorlist.h"
#include "lc_application.h"
#include "lc_library.h"
#include "lc_colors.h"

lcColorList::lcColorList(QWidget* Parent, bool AllowNoColor)
	: QWidget(Parent), mAllowNoColor(AllowNoColor)
{
	setFocusPolicy(Qt::StrongFocus);

	UpdateCells();

	connect(lcGetPiecesLibrary(), &lcPiecesLibrary::ColorsLoaded, this, &lcColorList::ColorsLoaded);
}

void lcColorList::UpdateCells()
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

	mPreferredHeight = GetHeightForWidth(width() > 0 ? width() : 200, &mColumns, &mRows);
}

int lcColorList::GetHeightForWidth(int Width, int* Columns, int* Rows) const
{
	QFontMetrics Metrics(font());
	int TextHeight = 0;

	for (const lcColorListGroup& Group : mGroups)
		TextHeight += qMax(Metrics.boundingRect(Group.Name).height(), 16) + 4;

	// Tiles are always square: the width sets the tile size and the rows set the height.
	const int TargetCellSize = Metrics.height() * 3 / 2;
	const int ColumnCount = qMax(5, Width / qMax(TargetCellSize, 1));
	int RowCount = 0;

	for (const lcColorListGroup& Group : mGroups)
		if (!Group.Collapsed)
			RowCount += ((int)Group.Cells.size() + ColumnCount - 1) / ColumnCount;

	if (Columns)
		*Columns = ColumnCount;

	if (Rows)
		*Rows = RowCount;

	return TextHeight + qCeil((float)Width / (float)ColumnCount * RowCount);
}

void lcColorList::UpdateRects()
{
	QFontMetrics Metrics(font());

	for (lcColorListGroup& Group : mGroups)
	{
		Group.Rect = Metrics.boundingRect(rect(), Qt::TextSingleLine | Qt::AlignCenter, Group.Name);
		Group.Rect.setHeight(qMax(Group.Rect.height(), 16) + 4);
	}

	mPreferredHeight = GetHeightForWidth(width(), &mColumns, &mRows);

	const float CellWidth = (float)width() / (float)mColumns;
	const float CellHeight = CellWidth;

	if (height() != mPreferredHeight)
	{
		setFixedHeight(mPreferredHeight);
		emit HeightChanged();
	}

	int CurCell = 0;
	float GroupY = 0.0f;

	for (lcColorListGroup& Group : mGroups)
	{
		int CurColumn = 0;
		int NumRows = 0;

		Group.Rect = QRect(0, (int)GroupY, width(), Group.Rect.height());
		GroupY += Group.Rect.height();

		// Collapsed groups keep their header but hide their tiles.
		if (Group.Collapsed)
		{
			for (size_t ColorIdx = 0; ColorIdx < Group.Cells.size(); ColorIdx++)
				mCells[CurCell++].Rect = QRect();

			continue;
		}

		for (size_t ColorIdx = 0; ColorIdx < Group.Cells.size(); ColorIdx++)
		{
			const int Left = CellWidth * CurColumn;
			const int Right = (CurColumn + 1 == mColumns) ? width() - 1 : (CurColumn + 1) * CellWidth;
			const int Top = GroupY + CellHeight * NumRows;
			const int Bottom = GroupY + CellHeight * (NumRows + 1);

			mCells[CurCell].Rect = QRect(Left, Top, Right - Left, Bottom - Top);

			CurColumn++;
			if (CurColumn == mColumns)
			{
				CurColumn = 0;
				NumRows++;
			}

			CurCell++;
		}

		if (CurColumn != 0)
			NumRows++;

		GroupY += NumRows * CellHeight;
	}
}

void lcColorList::ToggleGroupCollapsed(int GroupIndex)
{
	if (GroupIndex < 0 || GroupIndex >= static_cast<int>(mGroups.size()))
		return;

	mGroups[GroupIndex].Collapsed = !mGroups[GroupIndex].Collapsed;

	UpdateRects();
	update();
}

void lcColorList::ColorsLoaded()
{
	UpdateCells();
	UpdateRects();

	SetCurrentColor(lcGetColorIndex(mColorCode));

	update();
}

QSize lcColorList::sizeHint() const
{
	return QSize(200, mPreferredHeight);
}

void lcColorList::SetCurrentColor(int ColorIndex)
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

bool lcColorList::event(QEvent* Event)
{
	if (Event->type() == QEvent::ToolTip)
	{
		QHelpEvent* HelpEvent = static_cast<QHelpEvent*>(Event);

		for (size_t CellIndex = 0; CellIndex < mCells.size(); CellIndex++)
		{
			if (!mCells[CellIndex].Rect.contains(HelpEvent->pos()))
				continue;

			QString Text = lcGetColorToolTip(mCells[CellIndex].ColorIndex);
			QToolTip::showText(HelpEvent->globalPos(), Text);

			return true;
		}

		for (const lcColorListGroup& Group : mGroups)
		{
			if (!Group.Rect.contains(HelpEvent->pos()))
				continue;

			QToolTip::showText(HelpEvent->globalPos(), tr("Expand or collapse the %1 colors.").arg(Group.Name));

			return true;
		}

		QToolTip::hideText();
		Event->ignore();

		return true;
	}
	else if (Event->type() == QEvent::ShortcutOverride)
	{
		QKeyEvent* KeyEvent = static_cast<QKeyEvent*>(Event);

		if (KeyEvent->modifiers() == Qt::NoModifier || KeyEvent->modifiers() == Qt::KeypadModifier)
		{
			switch (KeyEvent->key())
			{
			case Qt::Key_Left:
			case Qt::Key_Right:
			case Qt::Key_Up:
			case Qt::Key_Down:
				KeyEvent->accept();
				break;

			default:
				break;
			}
		}
	}

	return QWidget::event(Event);
}

void lcColorList::mousePressEvent(QMouseEvent* MouseEvent)
{
	for (lcColorListGroup& Group : mGroups)
	{
		if (!Group.Rect.contains(MouseEvent->pos()))
			continue;

		ToggleGroupCollapsed(static_cast<int>(&Group - mGroups.data()));

		return;
	}

	for (size_t CellIndex = 0; CellIndex < mCells.size(); CellIndex++)
	{
		if (!mCells[CellIndex].Rect.contains(MouseEvent->pos()))
			continue;

		SelectCell(CellIndex);
		emit ColorSelected(mCells[CellIndex].ColorIndex);

		break;
	}

	mDragStartPosition = MouseEvent->pos();
}

void lcColorList::mouseMoveEvent(QMouseEvent* MouseEvent)
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

void lcColorList::keyPressEvent(QKeyEvent *event)
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
		emit ColorSelected(mCells[mCurrentCell].ColorIndex);
	}

	if (NewCell != mCurrentCell)
		SelectCell(NewCell);
	else
		QWidget::keyPressEvent(event);
}

void lcColorList::resizeEvent(QResizeEvent* Event)
{
	if (mWidth == width() && mHeight == height())
		return;

	UpdateRects();

	mWidth = width();
	mHeight = height();

	QWidget::resizeEvent(Event);
}

void lcColorList::paintEvent(QPaintEvent* Event)
{
	Q_UNUSED(Event);

	QPainter Painter(this);

	Painter.fillRect(rect(), palette().brush(QPalette::Window));

	Painter.setFont(font());
	Painter.setPen(palette().color(QPalette::Text));

	// Group headers match the collapsible headers of the properties panel.
	const QColor ArrowColor = palette().color(QPalette::Text);

	for (const lcColorListGroup& Group : mGroups)
	{
		const int TextWidth = Painter.fontMetrics().horizontalAdvance(Group.Name);
		const int ArrowLeft = Group.Rect.center().x() - (TextWidth + 20) / 2;
		const int ArrowTop = Group.Rect.center().y() - 8;

		Painter.setPen(Qt::NoPen);
		Painter.setBrush(ArrowColor);

		if (Group.Collapsed)
			Painter.drawPolygon(QPolygon({ QPoint(ArrowLeft + 6, ArrowTop + 4), QPoint(ArrowLeft + 13, ArrowTop + 8), QPoint(ArrowLeft + 6, ArrowTop + 12) }));
		else
			Painter.drawPolygon(QPolygon({ QPoint(ArrowLeft + 4, ArrowTop + 6), QPoint(ArrowLeft + 12, ArrowTop + 6), QPoint(ArrowLeft + 8, ArrowTop + 13) }));

		Painter.setPen(ArrowColor);
		Painter.drawText(QRect(ArrowLeft + 20, Group.Rect.top(), TextWidth + 1, Group.Rect.height()), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, Group.Name);
	}

	Painter.setPen(palette().color(QPalette::Shadow));

	for (size_t CellIndex = 0; CellIndex < mCells.size(); CellIndex++)
	{
		const lcColor* Color = &gColorList[mCells[CellIndex].ColorIndex];

		const QRect& Rect = mCells[CellIndex].Rect;

		if (Rect.isNull())
			continue;

		if (Color->Code != LC_COLOR_NOCOLOR)
		{
			QColor CellColor(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255);

			Painter.setBrush(CellColor);
			Painter.drawRect(Rect);
		}
		else
			lcDrawNoColorRect(Painter, Rect);
	}

	if (mCurrentCell < mCells.size() && !mCells[mCurrentCell].Rect.isNull())
	{
		const lcColor* Color = &gColorList[mCells[mCurrentCell].ColorIndex];
		QColor EdgeColor(255 - Color->Value[0] * 255, 255 - Color->Value[1] * 255, 255 - Color->Value[2] * 255);

		Painter.setPen(EdgeColor);
		Painter.setBrush(Qt::NoBrush);

		QRect CellRect = mCells[mCurrentCell].Rect;
		CellRect.adjust(1, 1, -1, -1);
		Painter.drawRect(CellRect);
	}
}

void lcColorList::SelectCell(size_t CellIndex)
{
	if (CellIndex >= mCells.size())
		return;

	if (CellIndex == mCurrentCell)
		return;

	mCurrentCell = CellIndex;
	mColorCode = lcGetColorCode(mCells[CellIndex].ColorIndex);

	emit ColorChanged(mCells[mCurrentCell].ColorIndex);
	update();
}
