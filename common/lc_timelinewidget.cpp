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

	Menu->addAction(gMainWindow->mActions[LC_TIMELINE_SET_CURRENT]);
	Menu->addAction(gMainWindow->mActions[LC_TIMELINE_INSERT]);
	Menu->addAction(gMainWindow->mActions[LC_TIMELINE_DELETE]);
	Menu->addAction(gMainWindow->mActions[LC_TIMELINE_MOVE_SELECTION]);

	Menu->addSeparator();

	Menu->addAction(gMainWindow->mActions[LC_PIECE_HIDE_SELECTED]);
	Menu->addAction(gMainWindow->mActions[LC_PIECE_HIDE_UNSELECTED]);
	Menu->addAction(gMainWindow->mActions[LC_PIECE_UNHIDE_SELECTED]);
	Menu->addAction(gMainWindow->mActions[LC_PIECE_UNHIDE_ALL]);

	Menu->popup(viewport()->mapToGlobal(Pos));
}

void lcTimelineWidget::Update(bool Clear, bool UpdateItems)
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

	lcStep LastStep = lcMax(Model->GetLastStep(), Model->GetCurrentStep());

	for (int TopLevelItemIdx = LastStep; TopLevelItemIdx < topLevelItemCount(); )
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

	for (unsigned int TopLevelItemIdx = topLevelItemCount(); TopLevelItemIdx < LastStep; TopLevelItemIdx++)
	{
		QTreeWidgetItem* StepItem = new QTreeWidgetItem(this, QStringList(tr("Step %1").arg(TopLevelItemIdx + 1)));
		StepItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
		addTopLevelItem(StepItem);
		StepItem->setExpanded(true);
	}

	const lcArray<lcPiece*>& Pieces = Model->GetPieces();
	QTreeWidgetItem* StepItem = nullptr;
	int PieceItemIndex = 0;
	lcStep Step = 0;

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
		bool UpdateItem = UpdateItems;

		if (StepItem)
		{
			if (!PieceItem)
			{
				PieceItem = new QTreeWidgetItem();
				PieceItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
				PieceItem->setData(0, Qt::UserRole, qVariantFromValue<uintptr_t>((uintptr_t)Piece));
				StepItem->insertChild(PieceItemIndex, PieceItem);
				mItems[Piece] = PieceItem;

				UpdateItem = true;
			}
			else
			{
				if (PieceItemIndex >= StepItem->childCount() || PieceItem != StepItem->child(PieceItemIndex))
				{
					QTreeWidgetItem* PieceParent = PieceItem->parent();

					if (PieceParent)
						PieceParent->removeChild(PieceItem);

					StepItem->insertChild(PieceItemIndex, PieceItem);
				}
			}
		}

		if (UpdateItem)
		{
			PieceItem->setText(0, Piece->mPieceInfo->m_strDescription);

			int ColorIndex = Piece->mColorIndex;
			if (!mIcons.contains(ColorIndex))
			{
				int Size = rowHeight(indexFromItem(PieceItem));

				QImage Image(Size, Size, QImage::Format_ARGB32);
				Image.fill(0);
				float* Color = gColorList[ColorIndex].Value;
				QPainter Painter(&Image);
				Painter.setPen(Qt::darkGray);
				Painter.setBrush(QColor::fromRgbF(Color[0], Color[1], Color[2]));
				Painter.drawEllipse(0, 0, Size - 1, Size - 1);

				mIcons[ColorIndex] = QIcon(QPixmap::fromImage(Image));
			}

			PieceItem->setIcon(0, mIcons[ColorIndex]);

			QColor Color = palette().text().color();
			if (Piece->IsHidden())
				Color.setAlpha(128);
			PieceItem->setTextColor(0, Color);
		}

		PieceItem->setSelected(Piece->IsSelected());
		PieceItemIndex++;
	}

	if (Step == 0)
	{
		Step = 1;
		StepItem = topLevelItem(0);
	}

	while (Step <= LastStep && StepItem)
	{
		while (PieceItemIndex < StepItem->childCount())
		{
			QTreeWidgetItem* PieceItem = StepItem->child(PieceItemIndex);
			lcPiece* RemovePiece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();

			mItems.remove(RemovePiece);
			delete PieceItem;
		}

		Step++;
		StepItem = topLevelItem(Step - 1);
		PieceItemIndex = 0;
	}

	blockSignals(Blocked);
}

void lcTimelineWidget::UpdateSelection()
{
	if (mIgnoreUpdates)
		return;

	QItemSelection ItemSelection;

	for (int TopLevelItemIdx = 0; TopLevelItemIdx < topLevelItemCount(); TopLevelItemIdx++)
	{
		QTreeWidgetItem* StepItem = topLevelItem(TopLevelItemIdx);

		for (int PieceItemIdx = 0; PieceItemIdx < StepItem->childCount(); PieceItemIdx++)
		{
			QTreeWidgetItem* PieceItem = StepItem->child(PieceItemIdx);
			lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();

			if (Piece && Piece->IsSelected())
			{
				QModelIndex Index = indexFromItem(PieceItem);
				ItemSelection.select(Index, Index);
			}
		}
	}

	bool Blocked = blockSignals(true);

	selectionModel()->select(ItemSelection, QItemSelectionModel::ClearAndSelect);

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

void lcTimelineWidget::MoveSelection()
{
	QTreeWidgetItem* CurrentItem = currentItem();

	if (!CurrentItem)
		return;

	if (CurrentItem->parent())
		CurrentItem = CurrentItem->parent();

	int Step = indexOfTopLevelItem(CurrentItem);

	if (Step == -1)
		return;

	QList<QTreeWidgetItem*> SelectedItems = selectedItems();

	for (QTreeWidgetItem* PieceItem : SelectedItems)
	{
		QTreeWidgetItem* Parent = PieceItem->parent();

		if (!Parent)
			continue;

		int ChildIndex = Parent->indexOfChild(PieceItem);
		CurrentItem->addChild(Parent->takeChild(ChildIndex));
	}

	UpdateModel();
}

void lcTimelineWidget::SetCurrentStep()
{
	QTreeWidgetItem* CurrentItem = currentItem();

	if (!CurrentItem)
		return;

	if (CurrentItem->parent())
		CurrentItem = CurrentItem->parent();

	int Step = indexOfTopLevelItem(CurrentItem);

	if (Step == -1)
		return;

	lcGetActiveModel()->SetCurrentStep(Step + 1);
}

void lcTimelineWidget::ItemSelectionChanged()
{
	lcArray<lcObject*> Selection;
	lcStep LastStep = 1;

	for (QTreeWidgetItem* PieceItem : selectedItems())
	{
		lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();
		if (Piece)
		{
			LastStep = lcMax(LastStep, Piece->GetStepShow());
			Selection.Add(Piece);
		}
	}

	lcPiece* CurrentPiece = nullptr;
	QTreeWidgetItem* CurrentItem = currentItem();
	if (CurrentItem && CurrentItem->isSelected())
		CurrentPiece = (lcPiece*)CurrentItem->data(0, Qt::UserRole).value<uintptr_t>();

	bool Blocked = blockSignals(true);
	mIgnoreUpdates = true;
	lcModel* Model = lcGetActiveModel();
	if (LastStep > Model->GetCurrentStep())
		Model->SetCurrentStep(LastStep);
	Model->SetSelectionAndFocus(Selection, CurrentPiece, LC_PIECE_SECTION_POSITION, false);
	mIgnoreUpdates = false;
	blockSignals(Blocked);
}

void lcTimelineWidget::dropEvent(QDropEvent* Event)
{
	QTreeWidget::dropEvent(Event);
	UpdateModel();
}

void lcTimelineWidget::mousePressEvent(QMouseEvent* Event)
{
	if (Event->button() == Qt::RightButton)
	{
		QItemSelection Selection = selectionModel()->selection();

		bool Blocked = blockSignals(true);
		QTreeWidget::mousePressEvent(Event);
		blockSignals(Blocked);

		selectionModel()->select(Selection, QItemSelectionModel::ClearAndSelect);
	}
	else
		QTreeWidget::mousePressEvent(Event);
}

void lcTimelineWidget::UpdateModel()
{
	QList<QPair<lcPiece*, lcStep>> PieceSteps;

	for (int TopLevelItemIdx = 0; TopLevelItemIdx < topLevelItemCount(); TopLevelItemIdx++)
	{
		QTreeWidgetItem* StepItem = topLevelItem(TopLevelItemIdx);

		for (int PieceItemIdx = 0; PieceItemIdx < StepItem->childCount(); PieceItemIdx++)
		{
			QTreeWidgetItem* PieceItem = StepItem->child(PieceItemIdx);
			lcPiece* Piece = (lcPiece*)PieceItem->data(0, Qt::UserRole).value<uintptr_t>();

			PieceSteps.append(QPair<lcPiece*, lcStep>(Piece, TopLevelItemIdx + 1));
		}
	}

	mIgnoreUpdates = true;
	lcGetActiveModel()->SetPieceSteps(PieceSteps);
	mIgnoreUpdates = false;
}
