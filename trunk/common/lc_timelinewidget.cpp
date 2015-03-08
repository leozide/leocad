#include "lc_global.h"
#include "lc_timelinewidget.h"
#include "lc_model.h"
#include "project.h"
#include "piece.h"
#include "pieceinf.h"

lcTimelineWidget::lcTimelineWidget(QWidget* Parent)
	: QTreeWidget(Parent)
{
	setSelectionMode(QAbstractItemView::ExtendedSelection);
	setDragEnabled(true);
	setDragDropMode(QAbstractItemView::InternalMove);
	setUniformRowHeights(true);
	setHeaderHidden(true);
}

lcTimelineWidget::~lcTimelineWidget()
{
}

void lcTimelineWidget::Update()
{
	clear();

	lcModel* Model = lcGetActiveModel();

	if (!Model)
		return;

	int Steps = Model->GetLastStep();
	QList<QTreeWidgetItem*> StepItems;
	StepItems.reserve(Steps);

	for (int Step = 0; Step < Steps; Step++)
	{
		QTreeWidgetItem* StepItem = new QTreeWidgetItem(this, QStringList(tr("Step %1").arg(Step + 1)));
		StepItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
		StepItems.append(StepItem);
	}

	const lcArray<lcPiece*>& Pieces = Model->GetPieces();

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Pieces[PieceIdx];
		QTreeWidgetItem* PieceItem = new QTreeWidgetItem(StepItems[Piece->GetStepShow() - 1], QStringList(Piece->mPieceInfo->m_strDescription));
		PieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		PieceItem->setData(0, Qt::UserRole, qVariantFromValue<uintptr_t>((uintptr_t)Piece));
	}

	insertTopLevelItems(0, StepItems);
	expandAll();
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
