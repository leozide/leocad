#include "lc_global.h"
#include "lc_findreplacewidget.h"
#include "lc_qcolorpicker.h"
#include "lc_library.h"
#include "lc_mainwindow.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_model.h"

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

	QCheckBox* FindColorCheckBox = new QCheckBox(this);
	Layout->addWidget(FindColorCheckBox, 0, 1);

	lcQColorPicker* FindColorPicker = new lcQColorPicker(this);
	Layout->addWidget(FindColorPicker, 0, 2);

	mFindPartComboBox = new QComboBox(this);
	mFindPartComboBox->setEditable(true);
	mFindPartComboBox->setInsertPolicy(QComboBox::NoInsert);
	Layout->addWidget(mFindPartComboBox, 0, 4);

	QToolButton* FindNextButton = new QToolButton(this);
	FindNextButton->setAutoRaise(true);
	FindNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_FIND_NEXT]);
	Layout->addWidget(FindNextButton, 0, 5);

	QToolButton* FindAllButton = new QToolButton(this);
	FindAllButton ->setAutoRaise(true);
	FindAllButton ->setDefaultAction(gMainWindow->mActions[LC_EDIT_FIND_ALL]);
	Layout->addWidget(FindAllButton, 0, 6);

	connect(FindColorCheckBox, &QCheckBox::toggled, [this](bool Checked)
	{
		mFindReplaceParams.MatchColor = Checked;
	});

	connect(FindColorPicker, &lcQColorPicker::colorChanged, [this](int ColorIndex)
	{
		mFindReplaceParams.ColorIndex = ColorIndex;
	});

	connect(mFindPartComboBox->lineEdit(), &QLineEdit::returnPressed, gMainWindow->mActions[LC_EDIT_FIND_NEXT], &QAction::trigger);
	connect(mFindPartComboBox->lineEdit(), &QLineEdit::textEdited, this, &lcFindReplaceWidget::FindTextEdited);
	connect(mFindPartComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &lcFindReplaceWidget::FindIndexChanged);

	QCheckBox* ReplaceColorCheckBox = nullptr;
	lcQColorPicker* ReplaceColorPicker = nullptr;
	QCheckBox* ReplacePartCheckBox = nullptr;
	QComboBox* ReplacePartComboBox = nullptr;

	if (Replace)
	{
		Layout->addWidget(new QLabel(tr("Replace:")), 1, 0);

		ReplaceColorCheckBox = new QCheckBox(this);
		Layout->addWidget(ReplaceColorCheckBox, 1, 1);

		ReplaceColorPicker = new lcQColorPicker(this);
		Layout->addWidget(ReplaceColorPicker, 1, 2);

		ReplacePartCheckBox = new QCheckBox(this);
		Layout->addWidget(ReplacePartCheckBox, 1, 3);

		ReplacePartComboBox = new QComboBox(this);
		Layout->addWidget(ReplacePartComboBox, 1, 4);

		QToolButton* ReplaceNextButton = new QToolButton(this);
		ReplaceNextButton->setAutoRaise(true);
		ReplaceNextButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_REPLACE_NEXT]);
		Layout->addWidget(ReplaceNextButton, 1, 5);

		QToolButton* ReplaceAllButton = new QToolButton(this);
		ReplaceAllButton->setAutoRaise(true);
		ReplaceAllButton->setDefaultAction(gMainWindow->mActions[LC_EDIT_REPLACE_ALL]);
		Layout->addWidget(ReplaceAllButton, 1, 6);

		connect(ReplaceColorCheckBox, &QCheckBox::toggled, [this](bool Checked)
		{
			mFindReplaceParams.ReplaceColor = Checked;
		});

		connect(ReplaceColorPicker, &lcQColorPicker::colorChanged, [this](int ColorIndex)
		{
			mFindReplaceParams.ReplaceColorIndex = ColorIndex;
		});

		connect(ReplacePartCheckBox, &QCheckBox::toggled, [this](bool Checked)
		{
			mFindReplaceParams.ReplaceInfo = Checked;
		});

		connect(ReplacePartComboBox, qOverload<int>(&QComboBox::currentIndexChanged), [this, ReplacePartComboBox](int Index)
		{
			mFindReplaceParams.ReplacePieceInfo = (PieceInfo*)ReplacePartComboBox->itemData(Index).value<void*>();
		});

		ReplacePartComboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		ReplacePartComboBox->setMinimumContentsLength(1);

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

		for (PieceInfo* Info : SortedPieces)
			ReplacePartComboBox->addItem(Info->m_strDescription, QVariant::fromValue((void*)Info));
	}

	lcPartsList PartsList;
	Model->GetPartsList(gDefaultColor, false, true, PartsList);

	for (const auto& PartIt : PartsList)
		mFindPartComboBox->addItem(QString::fromLatin1(PartIt.first->m_strDescription), QVariant::fromValue((void*)PartIt.first));
	mFindPartComboBox->model()->sort(0);

	lcPiece* Focus = dynamic_cast<lcPiece*>(Model->GetFocusObject());

	if (Focus)
	{
		mFindReplaceParams.FindInfo = Focus->mPieceInfo;

		mFindReplaceParams.ColorIndex = Focus->GetColorIndex();
		FindColorCheckBox->setChecked(true);
		FindColorPicker->setCurrentColor(mFindReplaceParams.ColorIndex);
		mFindPartComboBox->setCurrentIndex(mFindPartComboBox->findData(QVariant::fromValue((void*)mFindReplaceParams.FindInfo)));

		if (Replace)
		{
			ReplaceColorCheckBox->setChecked(true);
			ReplaceColorPicker->setCurrentColor(mFindReplaceParams.ColorIndex);
			ReplacePartCheckBox->setChecked(true);
			ReplacePartComboBox->setCurrentIndex(ReplacePartComboBox->findData(QVariant::fromValue((void*)mFindReplaceParams.FindInfo)));
		}
	}
	else
	{
		mFindPartComboBox->setEditText(QString());
	}

	adjustSize();
	move(1, 1);
	show();
}

void lcFindReplaceWidget::FindTextEdited(const QString& Text)
{
	mFindReplaceParams.FindString = Text;
	mFindReplaceParams.FindInfo = nullptr;
}

void lcFindReplaceWidget::FindIndexChanged(int Index)
{
	mFindReplaceParams.FindString.clear();
	mFindReplaceParams.FindInfo = (PieceInfo*)mFindPartComboBox->itemData(Index).value<void*>();
}
