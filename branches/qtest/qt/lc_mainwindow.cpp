#include "lc_global.h"
#include "lc_mainwindow.h"
#include "lc_global.h"
#include "lc_library.h"
#include "lc_application.h"
#include "pieceinf.h"
#include "project.h"
#include "preview.h"
#include "ui_lc_mainwindow.h"

static int PiecesSortFunc(const PieceInfo* a, const PieceInfo* b, void* SortData)
{
	if (a->IsSubPiece())
	{
		if (b->IsSubPiece())
		{
			return strcmp(a->m_strDescription, b->m_strDescription);
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if (b->IsSubPiece())
		{
			return -1;
		}
		else
		{
			return strcmp(a->m_strDescription, b->m_strDescription);
		}
	}

	return 0;
}

lcMainWindow::lcMainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::lcMainWindow)
{
	ui->setupUi(this);

	lcPiecesLibrary* Lib = lcGetPiecesLibrary();

	QList<QTreeWidgetItem *> items;
	for (int i = 0; i < Lib->mCategories.GetSize(); i++)
	{
		QTreeWidgetItem* Category = new QTreeWidgetItem((QTreeWidget*)0, QStringList((const char*)Lib->mCategories[i].Name));
		items.append(Category);
		/*
		PtrArray<PieceInfo> SinglePieces, GroupedPieces;

		Lib->GetCategoryEntries(i, true, SinglePieces, GroupedPieces);

		SinglePieces += GroupedPieces;
		SinglePieces.Sort(PiecesSortFunc, NULL);

		for (int j = 0; j < SinglePieces.GetSize(); j++)
		{
			PieceInfo* Info = SinglePieces[j];
			QTreeWidgetItem* Item = new QTreeWidgetItem(Category, QStringList(Info->m_strDescription));

			if (GroupedPieces.FindIndex(Info) != -1)
			{
				PtrArray<PieceInfo> Patterns;
				Lib->GetPatternedPieces(Info, Patterns);

				for (int k = 0; k < Patterns.GetSize(); k++)
				{
					PieceInfo* child = Patterns[k];

					if (!Lib->PieceInCategory(child, Lib->mCategories[i].Keywords))
					continue;

					const char* desc = child->m_strDescription;
					int len = strlen(Info->m_strDescription);

					if (!strncmp(child->m_strDescription, Info->m_strDescription, len))
						desc += len;

					QTreeWidgetItem* Pattern = new QTreeWidgetItem(Item, QStringList(desc));
				}
			}
		}
*/
	}
	ui->pieceList->insertTopLevelItems(0, items);

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* Info = Library->FindPiece("3005", false);

	if (!Info)
		Info = Library->mPieces[0];

	if (Info)
	{
		lcGetActiveProject()->SetCurrentPiece(Info);
		PiecePreview* Preview = (PiecePreview*)ui->piecePreview->mWindow;
		Preview->SetCurrentPiece(Info);
	}

}

lcMainWindow::~lcMainWindow()
{
	delete ui;
}
