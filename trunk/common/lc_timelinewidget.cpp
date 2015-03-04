#include "lc_global.h"
#include "lc_timelinewidget.h"
#include "lc_model.h"
#include "project.h"
#include "piece.h"
#include "pieceinf.h"

lcTimelineWidget::lcTimelineWidget(QWidget* Parent)
	: QTreeWidget(Parent)
{
	header()->hide();
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
		StepItems.append(new QTreeWidgetItem(this, QStringList(tr("Step %1").arg(Step + 1))));

	const lcArray<lcPiece*>& Pieces = Model->GetPieces();

	for (int PieceIdx = 0; PieceIdx < Pieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Pieces[PieceIdx];
		new QTreeWidgetItem(StepItems[Piece->GetStepShow() - 1], QStringList(Piece->mPieceInfo->m_strDescription));
	}

	insertTopLevelItems(0, StepItems);
}
