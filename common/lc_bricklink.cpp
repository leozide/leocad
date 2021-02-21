
#include <locale.h>

#include "lc_colors.h"
#include "lc_file.h"
#include "lc_library.h"
#include "lc_mainwindow.h"
#include "lc_model.h"
#include "pieceinf.h"
#include "project.h"

namespace {

	std::map<std::string, std::string> Remapping;

	std::string BrickLinkRemap(const char* LDBrick)
	{
		auto Remapped = Remapping.find(LDBrick);
		if (Remapped != Remapping.end())
		{
			return Remapped->second;
		}
		return LDBrick;
	}

	bool LoadRemapping()
	{
		if (!Remapping.empty())
		{
			return false;
		}

		QFile File(lcGetPiecesLibrary()->mLibraryDir.absoluteFilePath(QLatin1String("ldraw2bl.txt")));
		QByteArray Data;

		if (File.open(QIODevice::ReadOnly))
			Data = File.readAll();
		else
		{
			QFile DefaultFile(":/resources/ldraw2bl.txt");

			if (DefaultFile.open(QIODevice::ReadOnly))
				Data = DefaultFile.readAll();
		}

		if (Data.isEmpty())
			return false;

		QTextStream Stream(Data);
		QString Line;
		while (!Stream.atEnd())
		{
			Line = Stream.readLine(1024);
			if (!Line.isEmpty() > 0)
			{
				// Commented line
				if (Line.startsWith('!'))
				{
					continue;
				}
				auto parts = Line.trimmed().split(QLatin1Char(' '));
				if (parts.size() < 2)
				{
					qDebug() << "BL remappings: Invalid line" << Line;
					continue;
				}
				Remapping.insert(std::make_pair(parts.value(0).toStdString(),
												parts.value(1).toUtf8().data()));
			}
		}

		return true;
	}
}

class Item
{
public:
	Item(const std::string& id, int color, int count)
		: mId(id), mColor(color), mCount(count)
		{
		}
	void AddToCount(int count)
		{
			mCount += count;
		}
	std::string mId;
	int mColor;
	int mCount;
};

void ExportBrickLink(const Project& project)
{
	lcPartsList PartsList;

	auto Models = project.GetModels();
	if (!Models.IsEmpty())
		Models[0]->GetPartsList(gDefaultColor, true, false, PartsList);

	if (PartsList.empty())
	{
		QMessageBox::information(gMainWindow, project.tr("LeoCAD"), project.tr("Nothing to export."));
		return;
	}

	QString SaveFileName = project.GetExportFileName(QString(), "xml", project.tr("Export BrickLink"), project.tr("XML Files (*.xml);;All Files (*.*)"));

	if (SaveFileName.isEmpty())
		return;

	if (Remapping.empty())
	{
		if (!LoadRemapping())
		{
			qDebug() << "Loading BL remappings failed.";
		}
	}

	lcDiskFile BrickLinkFile(SaveFileName);
	char Line[1024];

	if (!BrickLinkFile.Open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(gMainWindow, project.tr("LeoCAD"), project.tr("Could not open file '%1' for writing.").arg(SaveFileName));
		return;
	}

	std::map<std::string, Item> Inventory;

	BrickLinkFile.WriteLine("<INVENTORY>\n");

	for (const auto& PartIt : PartsList)
	{
		const PieceInfo* Info = PartIt.first;

		for (const auto& ColorIt : PartIt.second)
		{

			char FileName[LC_PIECE_NAME_LEN];
			strcpy(FileName, Info->mFileName);
			char* Ext = strchr(FileName, '.');
			if (Ext)
				*Ext = 0;

			std::string Remapped = BrickLinkRemap(FileName);
			int Color = lcGetBrickLinkColor(ColorIt.first);
			std::string key(Remapped);
			key += "-" + std::to_string(Color);
			auto iter = Inventory.find(key);
			if (iter == Inventory.end())
			{
				Inventory.emplace(std::make_pair(key, Item(Remapped, Color, ColorIt.second)));
			}
			else
			{
				iter->second.AddToCount(ColorIt.second);
			}
		}
	}

	for (const auto& Item : Inventory)
	{
		BrickLinkFile.WriteLine("  <ITEM>\n");
		BrickLinkFile.WriteLine("    <ITEMTYPE>P</ITEMTYPE>\n");
		sprintf(Line, "    <ITEMID>%s</ITEMID>\n", Item.second.mId.c_str());
		BrickLinkFile.WriteLine(Line);

		sprintf(Line, "    <MINQTY>%d</MINQTY>\n", Item.second.mCount);
		BrickLinkFile.WriteLine(Line);

		if (Item.second.mColor)
		{
			sprintf(Line, "    <COLOR>%d</COLOR>\n", Item.second.mColor);
			BrickLinkFile.WriteLine(Line);
		}

		BrickLinkFile.WriteLine("  </ITEM>\n");
	}

	BrickLinkFile.WriteLine("</INVENTORY>\n");
}
