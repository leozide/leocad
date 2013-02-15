#include <QtGui>
#include "lc_global.h"
#include "lc_colorlistwidget.h"

lcColorListWidget::lcColorListWidget(QWidget *parent)
	: QWidget(parent)
{
	mCellRects = new QRect[gNumUserColors];
	mCellColors = new int[gNumUserColors];
	mNumCells = 0;

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

	mPreferredHeight = TextHeight + 8 * mRows;
}

lcColorListWidget::~lcColorListWidget()
{
	delete[] mCellRects;
	delete[] mCellColors;
}

QSize lcColorListWidget::sizeHint() const
{
	return QSize(100, mPreferredHeight);
}

bool lcColorListWidget::event(QEvent *event)
{
	if (event->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

		for (int CellIdx = 0; CellIdx < mNumCells; CellIdx++)
		{
//			lcColor* Color = &gColorList[mCellColors[CellIdx]];
//			QColor CellColor(Color->Value[0] * 255, Color->Value[1] * 255, Color->Value[2] * 255);

			if (mCellRects[CellIdx].contains(helpEvent->pos()))
			{
				QToolTip::showText(helpEvent->globalPos(), gColorList[mCellColors[CellIdx]].Name);
				return true;
			}
		}

		QToolTip::hideText();
		event->ignore();

		return true;
	}

	return QWidget::event(event);
}

void lcColorListWidget::resizeEvent(QResizeEvent *event)
{
	int Width, Height;

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

	float CellWidth = (float)width() / (float)mColumns;
	float CellHeight = (float)(height() - TextHeight) / (float)mRows;

	int CurCell = 0;
	float CurY = 0.0f;

	for (int GroupIdx = 0; GroupIdx < LC_NUM_COLORGROUPS; GroupIdx++)
	{
		lcColorGroup* Group = &gColorGroups[GroupIdx];
		int CurColumn = 0;

		mGroupRects[GroupIdx] = QRect(0, (int)CurY, width(), mGroupRects[GroupIdx].height());
		CurY += mGroupRects[GroupIdx].height();

		for (int ColorIdx = 0; ColorIdx < Group->Colors.GetSize(); ColorIdx++)
		{
			int Left = CurColumn * CellWidth;
			int Right = (CurColumn + 1) * CellWidth;
			float Top = CurY;
			float Bottom = CurY + CellHeight;

			mCellRects[CurCell] = QRect(Left, Top, Right - Left, Bottom - Top);

			CurColumn++;
			if (CurColumn == mColumns)
			{
				CurColumn = 0;
				CurY += CellHeight;
			}

			CurCell++;
		}

		if (CurColumn != 0)
			CurY += CellHeight;
	}

	mWidth = width();
	mHeight = height();

	QWidget::resizeEvent(event);
}

void lcColorListWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.setFont(font());

	painter.setBrush(palette().brush(QPalette::Base));
	painter.setPen(palette().color(QPalette::Shadow)); // TODO: correct border
	painter.drawRect(rect());

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
}
