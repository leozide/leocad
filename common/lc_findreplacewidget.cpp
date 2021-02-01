#include "lc_global.h"
#include "lc_findreplacewidget.h"
#include "lc_qcolorpicker.h"
#include "lc_library.h"
#include "lc_mainwindow.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_model.h"
#include "lc_view.h"

lcFindReplaceWidget::lcFindReplaceWidget(QWidget* Parent, lcModel* Model, bool Replace)
	: QWidget(Parent)
{
	setAutoFillBackground(true);
	QPalette Palette = palette();
	Palette.setColor(QPalette::Window, QApplication::palette().color(QPalette::Button));
	setPalette(Palette);

	QGridLayout* Layout = new QGridLayout(this);
	Layout->setContentsMargins(5, 5, 5, 5);

	Layout->addWidget(new QLabel(tr("Find:")), 0, 0);

	lcQColorPicker* FindColorPicker = new lcQColorPicker(this, true);
	Layout->addWidget(FindColorPicker, 0, 1);

	mFindPartComboBox = new QComboBox(this);
	mFindPartComboBox->setEditable(true);
	mFindPartComboBox->setInsertPolicy(QComboBox::NoInsert);
	Layout->addWidget(mFindPartComboBox, 0, 2);

	QToolButton* FindNextButton = new QToolButton(this);
	FindNextButton->setAutoRaise(true);
	FindNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_FIND_NEXT]);
	Layout->addWidget(FindNextButton, 0, 3);

	QToolButton* FindAllButton = new QToolButton(this);
	FindAllButton ->setAutoRaise(true);
	FindAllButton ->setDefaultAction(gMainWindow->mActions[LC_EDIT_FIND_ALL]);
	Layout->addWidget(FindAllButton, 0, 4);

	connect(FindColorPicker, &lcQColorPicker::colorChanged, this, &lcFindReplaceWidget::FindColorIndexChanged);
	connect(mFindPartComboBox->lineEdit(), &QLineEdit::returnPressed, gMainWindow->mActions[LC_EDIT_FIND_NEXT], &QAction::trigger);
	connect(mFindPartComboBox->lineEdit(), &QLineEdit::textEdited, this, &lcFindReplaceWidget::FindTextEdited);
	connect(mFindPartComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &lcFindReplaceWidget::FindActivated);

	lcQColorPicker* ReplaceColorPicker = nullptr;

	if (Replace)
	{
		Layout->addWidget(new QLabel(tr("Replace:")), 1, 0);

		ReplaceColorPicker = new lcQColorPicker(this, true);
		Layout->addWidget(ReplaceColorPicker, 1, 1);

		mReplacePartComboBox = new QComboBox(this);
		Layout->addWidget(mReplacePartComboBox, 1, 2);

		QToolButton* ReplaceNextButton = new QToolButton(this);
		ReplaceNextButton->setAutoRaise(true);
		ReplaceNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_REPLACE_NEXT]);
		Layout->addWidget(ReplaceNextButton, 1, 3);

		QToolButton* ReplaceAllButton = new QToolButton(this);
		ReplaceAllButton->setAutoRaise(true);
		ReplaceAllButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_REPLACE_ALL]);
		Layout->addWidget(ReplaceAllButton, 1, 4);

		connect(ReplaceColorPicker, &lcQColorPicker::colorChanged, this, &lcFindReplaceWidget::ReplaceColorIndexChanged);
		connect(mReplacePartComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &lcFindReplaceWidget::ReplaceActivated);

		mReplacePartComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		mReplacePartComboBox->setMinimumContentsLength(1);

		lcPiecesLibrary* Library = lcGetPiecesLibrary();
		std::vector<PieceInfo*> SortedPieces;
		SortedPieces.reserve(Library->mPieces.size());

		for (const auto& PartIt : Library->mPieces)
			SortedPieces.push_back(PartIt.second);

		auto PieceCompare = [](PieceInfo* Info1, PieceInfo* Info2)
		{
			return strcmp(Info1->m_strDescription, Info2->m_strDescription) < 0;
		};

		std::sort(SortedPieces.begin(), SortedPieces.end(), PieceCompare);

		mReplacePartComboBox->addItem(QString(), QVariant::fromValue((void*)nullptr));
		for (PieceInfo* Info : SortedPieces)
			mReplacePartComboBox->addItem(QString::fromLatin1(Info->m_strDescription), QVariant::fromValue((void*)Info));

		ReplaceColorPicker->setCurrentColor(lcGetColorIndex(LC_COLOR_NOCOLOR));
		mReplacePartComboBox->setCurrentIndex(0);
	}

	lcPartsList PartsList;
	Model->GetPartsList(gDefaultColor, false, true, PartsList);

	for (const auto& PartIt : PartsList)
		mFindPartComboBox->addItem(QString::fromLatin1(PartIt.first->m_strDescription), QVariant::fromValue((void*)PartIt.first));
	mFindPartComboBox->model()->sort(0);

	lcPiece* Focus = dynamic_cast<lcPiece*>(Model->GetFocusObject());

	if (Focus)
	{
		lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

		Params.FindInfo = Focus->mPieceInfo;
		Params.FindColorIndex = Focus->GetColorIndex();

		FindColorPicker->setCurrentColor(Params.FindColorIndex);
		mFindPartComboBox->setCurrentIndex(mFindPartComboBox->findData(QVariant::fromValue((void*)Params.FindInfo)));

	}
	else
	{
		FindColorPicker->setCurrentColor(lcGetColorIndex(LC_COLOR_NOCOLOR));
		mFindPartComboBox->setEditText(QString());
	}

	mFindPartComboBox->setFocus();
	adjustSize();
	move(1, 1);
	show();
}

void lcFindReplaceWidget::FindColorIndexChanged(int ColorIndex)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.FindColorIndex = ColorIndex;
}

void lcFindReplaceWidget::FindTextEdited(const QString& Text)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.FindString = Text;
	Params.FindInfo = nullptr;
}

void lcFindReplaceWidget::FindActivated(int Index)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.FindString.clear();
	Params.FindInfo = (PieceInfo*)mFindPartComboBox->itemData(Index).value<void*>();
}

void lcFindReplaceWidget::ReplaceColorIndexChanged(int ColorIndex)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.ReplaceColorIndex = ColorIndex;
}

void lcFindReplaceWidget::ReplaceActivated(int Index)
{
	lcFindReplaceParams& Params = lcView::GetFindReplaceParams();

	Params.ReplacePieceInfo = (PieceInfo*)mReplacePartComboBox->itemData(Index).value<void*>();
}
