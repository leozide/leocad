#include "lc_global.h"
#include "lc_timelinewidget.h"
#include "lc_model.h"
#include "project.h"
#include "piece.h"
#include "pieceinf.h"
#include "lc_mainwindow.h"

lcTimelineWidget::lcTimelineWidget(QWidget* Parent)
	: QTreeWidget(Parent)
{
	mIgnoreUpdates = false;

	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::InternalMove);
	setUniformRowHeights(true);
	setHeaderHidden(true);
	setContextMenuPolicy(Qt::CustomContextMenu);

	connect(this, SIGNAL(itemSelectionChanged()), SLOT(ItemSelectionChanged()));
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(CustomMenuRequested(QPoint)));
}

lcTimelineWidget::~lcTimelineWidget()
{
}

void lcTimelineWidget::CustomMenuRequested(QPoint Pos)
{
	QList<QTreeWidgetItem*> SelectedItems = selectedItems();

	if (!SelectedItems.isEmpty())
	{
		QMenu* Menu = new QMenu(this);

		Menu->addAction(gMainWindow->mActions[LC_PIECE_HIDE_SELECTED]);
		Menu->addAction(gMainWindow->mActions[LC_PIECE_HIDE_UNSELECTED]);
		Menu->addAction(gMainWindow->mActions[LC_PIECE_UNHIDE_ALL]);

		Menu->popup(viewport()->mapToGlobal(Pos));
	}
}

void lcTimelineWidget::Update()
{
	lcModel* Model = lcGetActiveModel();

	if (!Model)
	{
		mItems.clear();
		clear();
		return;
	}

	bool Blocked = blockSignals(true);

	int Steps = Model->GetLastStep();

	for (int TopLevelItemIdx = Steps; TopLevelItemIdx < topLevelItemCount(); TopLevelItemIdx++)
		delete topLevelItem(TopLevelItemIdx);

	for (int Step = topLevelItemCount(); Step < Steps; Step++)
	{
		QTreeWidgetItem* StepItem = new QTreeWidgetItem(this, QStringList(tr("Step %1").arg(Step + 1)));
		StepItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
		addTopLevelItem(StepItem);
		StepItem->setExpanded(true);
	}

	const lcArray<lcPiece*>& Pieces = Model->GetPieces();

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Pieces[PieceIdx];
		QTreeWidgetItem* PieceItem = mItems.value(Piece);
		QTreeWidgetItem* StepItem = topLevelItem(Piece->GetStepShow() - 1);

		if (PieceItem)
		{
			if (PieceItem->parent() != StepItem)
				StepItem->addChild(PieceItem);
		}
		else
		{
			PieceItem = new QTreeWidgetItem(StepItem, QStringList(Piece->mPieceInfo->m_strDescription));
			PieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
			PieceItem->setData(0, Qt::UserRole, qVariantFromValue<uintptr_t>((uintptr_t)Piece));
			StepItem->addChild(PieceItem);
			mItems[Piece] = PieceItem;
		}

		PieceItem->setSelected(Piece->IsSelected());
	}

	blockSignals(Blocked);
}

void lcTimelineWidget::UpdateSelection()
{
	if (mIgnoreUpdates)
		return;

	bool Blocked = blockSignals(true);

	for (int TopLevelItemIdx = 0; TopLevelItemIdx < topLevelItemCount(); TopLevelItemIdx++)
	{
		QTreeWidgetItem* StepItem = topLevelItem(TopLevelItemIdx);

		for (int PieceItemIdx = 0; PieceItemIdx < StepItem->childCount(); PieceItemIdx++)
		{
			QTreeWidgetItem* PieceItem = StepItem->child(PieceItemIdx);
			lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();

			PieceItem->setSelected(Piece->IsSelected());
		}
	}

	blockSignals(Blocked);
}

void lcTimelineWidget::ItemSelectionChanged()
{
	lcArray<lcObject*> Selection;

	foreach (QTreeWidgetItem* PieceItem, selectedItems())
	{
		lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();
		Selection.Add(Piece);
	}

	bool Blocked = blockSignals(true);
	mIgnoreUpdates = true;
	lcGetActiveModel()->SetSelection(Selection);
	mIgnoreUpdates = false;
	blockSignals(Blocked);
}

void lcTimelineWidget::dropEvent(QDropEvent* Event)
{
	QTreeWidget::dropEvent(Event);

	QList<QPair<lcPiece*, int>> PieceSteps;

	for (int TopLevelItemIdx = 0; TopLevelItemIdx < topLevelItemCount(); TopLevelItemIdx++)
	{
		QTreeWidgetItem* StepItem = topLevelItem(TopLevelItemIdx);

		for (int PieceItemIdx = 0; PieceItemIdx < StepItem->childCount(); PieceItemIdx++)
		{
			QTreeWidgetItem* PieceItem = StepItem->child(PieceItemIdx);
			lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();

			PieceSteps.append(QPair<lcPiece*, int>(Piece, TopLevelItemIdx + 1));
		}
	}

	lcGetActiveModel()->SetPieceSteps(PieceSteps);
}
