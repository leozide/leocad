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

	invisibleRootItem()->setFlags(invisibleRootItem()->flags() & ~Qt::ItemIsDropEnabled);

	connect(this, SIGNAL(itemSelectionChanged()), SLOT(ItemSelectionChanged()));
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), SLOT(CustomMenuRequested(QPoint)));
}

lcTimelineWidget::~lcTimelineWidget()
{
}

void lcTimelineWidget::CustomMenuRequested(QPoint Pos)
{
	QMenu* Menu = new QMenu(this);

	lcObject* FocusObject = lcGetActiveModel()->GetFocusObject();

	if (FocusObject && FocusObject->IsPiece())
	{
		lcPiece* Piece = (lcPiece*)FocusObject;

		if (Piece->mPieceInfo->IsModel())
		{
			Menu->addAction(gMainWindow->mActions[LC_MODEL_EDIT_FOCUS]);
			Menu->addSeparator();
		}
	}

	QAction* InsertStepAction = Menu->addAction(gMainWindow->mActions[LC_VIEW_TIME_INSERT]->text(), this, SLOT(InsertStep()));
	InsertStepAction->setStatusTip(gMainWindow->mActions[LC_VIEW_TIME_INSERT]->statusTip());
	QAction* RemoveStepAction = Menu->addAction(gMainWindow->mActions[LC_VIEW_TIME_DELETE]->text(), this, SLOT(RemoveStep()));
	RemoveStepAction->setStatusTip(gMainWindow->mActions[LC_VIEW_TIME_DELETE]->statusTip());
	Menu->addSeparator();

	Menu->addAction(gMainWindow->mActions[LC_PIECE_HIDE_SELECTED]);
	Menu->addAction(gMainWindow->mActions[LC_PIECE_HIDE_UNSELECTED]);
	Menu->addAction(gMainWindow->mActions[LC_PIECE_UNHIDE_SELECTED]);
	Menu->addAction(gMainWindow->mActions[LC_PIECE_UNHIDE_ALL]);

	Menu->popup(viewport()->mapToGlobal(Pos));
}

void lcTimelineWidget::Update(bool Clear)
{
	if (mIgnoreUpdates)
		return;

	lcModel* Model = lcGetActiveModel();

	if (!Model)
	{
		mItems.clear();
		clear();
		return;
	}

	bool Blocked = blockSignals(true);

	if (Clear)
	{
		mItems.clear();
		clear();
	}

	int Steps = lcMax(Model->GetLastStep(), Model->GetCurrentStep());

	for (int TopLevelItemIdx = Steps; TopLevelItemIdx < topLevelItemCount(); )
	{
		QTreeWidgetItem* StepItem = topLevelItem(TopLevelItemIdx);

		while (StepItem->childCount())
		{
			QTreeWidgetItem* PieceItem = StepItem->child(0);
			lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();
			mItems.remove(Piece);
			delete PieceItem;
		}

		delete StepItem;
	}

	for (int TopLevelItemIdx = topLevelItemCount(); TopLevelItemIdx < Steps; TopLevelItemIdx++)
	{
		QTreeWidgetItem* StepItem = new QTreeWidgetItem(this, QStringList(tr("Step %1").arg(TopLevelItemIdx + 1)));
		StepItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
		addTopLevelItem(StepItem);
		StepItem->setExpanded(true);
	}

	const lcArray<lcPiece*>& Pieces = Model->GetPieces();
	QTreeWidgetItem* StepItem = NULL;
	int PieceItemIndex = 0;
	int Step = 0;

	for (int PieceIdx = 0; PieceIdx != Pieces.GetSize(); PieceIdx++)
	{
		lcPiece* Piece = Pieces[PieceIdx];

		while (Step != Piece->GetStepShow())
		{
			if (StepItem)
			{
				while (PieceItemIndex < StepItem->childCount())
				{
					QTreeWidgetItem* PieceItem = StepItem->child(PieceItemIndex);
					lcPiece* RemovePiece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();

					if (Pieces.FindIndex(RemovePiece) == -1)
					{
						mItems.remove(RemovePiece);
						delete PieceItem;
					}
					else
					{
						PieceItem->parent()->removeChild(PieceItem);
						topLevelItem(RemovePiece->GetStepShow() - 1)->addChild(PieceItem);
					}
				}
			}

			Step++;
			StepItem = topLevelItem(Step - 1);
			PieceItemIndex = 0;
		}

		QTreeWidgetItem* PieceItem = mItems.value(Piece);

		if (!PieceItem)
		{
			PieceItem = new QTreeWidgetItem(QStringList(Piece->mPieceInfo->m_strDescription));
			PieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
			PieceItem->setData(0, Qt::UserRole, qVariantFromValue<uintptr_t>((uintptr_t)Piece));
			StepItem->insertChild(PieceItemIndex, PieceItem);
			mItems[Piece] = PieceItem;
		}
		else
		{
			if (PieceItemIndex >= StepItem->childCount() || PieceItem != StepItem->child(PieceItemIndex))
			{
				if (PieceItem->parent() == StepItem)
					StepItem->removeChild(PieceItem);

				StepItem->insertChild(PieceItemIndex, PieceItem);
			}
		}

		QColor Color = palette().text().color();
		if (Piece->IsHidden())
			Color.setAlpha(128);
		PieceItem->setTextColor(0, Color);

		PieceItem->setSelected(Piece->IsSelected());
		PieceItemIndex++;
	}

	if (!StepItem)
		StepItem = topLevelItem(0);

	while (PieceItemIndex < StepItem->childCount())
	{
		QTreeWidgetItem* PieceItem = StepItem->child(PieceItemIndex);
		lcPiece* RemovePiece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();

		mItems.remove(RemovePiece);
		delete PieceItem;
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

void lcTimelineWidget::InsertStep()
{
	QTreeWidgetItem* CurrentItem = currentItem();

	if (!CurrentItem)
		return;

	if (CurrentItem->parent())
		CurrentItem = CurrentItem->parent();

	int Step = indexOfTopLevelItem(CurrentItem);

	if (Step == -1)
		return;

	lcGetActiveModel()->InsertStep(Step + 1);
}

void lcTimelineWidget::RemoveStep()
{
	QTreeWidgetItem* CurrentItem = currentItem();

	if (!CurrentItem)
		return;

	if (CurrentItem->parent())
		CurrentItem = CurrentItem->parent();

	int Step = indexOfTopLevelItem(CurrentItem);

	if (Step == -1)
		return;

	lcGetActiveModel()->RemoveStep(Step + 1);
}

void lcTimelineWidget::ItemSelectionChanged()
{
	lcArray<lcObject*> Selection;
	lcStep LastStep = 1;

	foreach (QTreeWidgetItem* PieceItem, selectedItems())
	{
		lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();
		LastStep = lcMax(LastStep, Piece->GetStepShow());
		Selection.Add(Piece);
	}

	lcPiece* CurrentPiece = NULL;
	QTreeWidgetItem* CurrentItem = currentItem();
	if (CurrentItem && CurrentItem->isSelected())
		CurrentPiece = (lcPiece*)CurrentItem->data(0, Qt::UserRole).value<uintptr_t>();

	bool Blocked = blockSignals(true);
	mIgnoreUpdates = true;
	lcModel* Model = lcGetActiveModel();
	if (LastStep > Model->GetCurrentStep())
		Model->SetCurrentStep(LastStep);
	Model->SetSelectionAndFocus(Selection, CurrentPiece, LC_PIECE_SECTION_POSITION);
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

	mIgnoreUpdates = true;
	lcGetActiveModel()->SetPieceSteps(PieceSteps);
	mIgnoreUpdates = false;
}
