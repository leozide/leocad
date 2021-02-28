
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
				auto result = Remapping.insert(std::make_pair(parts.value(0).toStdString(),
												parts.value(1).toUtf8().data()));
				if (!result.second)
					qDebug() << "Duplicate" << parts.value(0);
			}
		}

		return true;
	}

	int lcGetBrickLinkColor(int ColorIndex)
	{
		struct lcBrickLinkEntry
		{
			int LDraw;
			int BrickLink;
		};

		lcBrickLinkEntry BrickLinkColors[] =
		{
			{   0,  11 }, // Black
			{   1,   7 }, // Blue
			{   2,   6 }, // Green
			{   3,  39 }, // Dark Turquoise
			{   4,   5 }, // Red
			{   5,  47 }, // Dark Pink
			{   6,   8 }, // Brown
			{   7,   9 }, // Light Gray
			{   8,  10 }, // Dark Gray
			{   9,  62 }, // Light Blue
			{  10,  36 }, // Bright Green
			{  11,  40 }, // Light Turquoise
			{  12,  25 }, // Salmon
			{  13,  23 }, // Pink
			{  14,   3 }, // Yellow
			{  15,   1 }, // White
			{  16,  -1 }, // None
			{  17,  38 }, // Light Green
			{  18,  33 }, // Light Yellow
			{  19,   2 }, // Tan
			{  20,  44 }, // Light Violet
			{  21,  46 }, // Glow in Dark Opaque
			{  22,  24 }, // Purple
			{  23, 109 }, // Dark Blue-Violet
			{  24,  -1 }, // None
			{  25,   4 }, // Orange
			{  26,  71 }, // Magenta
			{  27,  34 }, // Lime
			{  28,  69 }, // Dark Tan
			{  29, 104 }, // Bright Pink
			{  30, 157 }, // Medium Lavender
			{  31, 154 }, // Lavender
			{  32,  11 }, // Black
			{  33,  14 }, // Trans-Dark Blue
			{  34,  20 }, // Trans-Green
			{  35, 108 }, // Trans-Bright Green
			{  36,  17 }, // Trans-Red
			{  37,  50 }, // Trans-Dark Pink
			{  38,  18 }, // Trans-Neon Orange
			{  39, 113 }, // Trans-Very Lt Blue
			{  40,  13 }, // Trans-Black
			{  41,  74 }, // Trans-Medium Blue
			{  42,  16 }, // Trans-Neon Green
			{  43,  15 }, // Trans-Light Blue
			{  44, 114 }, // Trans-Light Purple
			{  45, 107 }, // Trans-Pink
			{  46,  19 }, // Trans-Yellow
			{  47,  12 }, // Trans-Clear
			{  52,  51 }, // Trans-Purple
			{  54, 121 }, // Trans-Neon Yellow
			{  57,  98 }, // Trans-Orange
			{  60,  57 }, // Chrome Antique Brass
			{  61,  52 }, // Chrome Blue
			{  62,  64 }, // Chrome Green
			{  63,  82 }, // Chrome Pink
			{  64, 122 }, // Chrome Black
			{  65,   3 }, // Yellow
			{  66,  19 }, // Trans-Yellow
			{  67,  12 }, // Trans-Clear
			{  68,  96 }, // Very Light Orange
			{  69,  93 }, // Light Purple
			{  70,  88 }, // Reddish Brown
			{  71,  86 }, // Light Bluish Gray
			{  72,  85 }, // Dark Bluish Gray
			{  73,  42 }, // Medium Blue
			{  74,  37 }, // Medium Green
			{  75, 116 }, // Speckle Black-Copper
			{  76, 117 }, // Speckle DBGray-Silver
			{  77,  56 }, // Light Pink
			{  78,  90 }, // Light Flesh
			{  79,  60 }, // Milky White
			{  80,  67 }, // Metallic Silver
			{  81,  70 }, // Metallic Green
			{  82,  65 }, // Metallic Gold
			{  83,  11 }, // Black
			{  84, 150 }, // Medium Dark Flesh
			{  85,  89 }, // Dark Purple
			{  86,  91 }, // Dark Flesh
			{  87,  77 }, // Pearl Dark Gray
			{  89,  97 }, // Blue - Violet
			{  92,  28 }, // Flesh
			{ 100,  26 }, // Light Salmon
			{ 110,  43 }, // Violet
			{ 112,  73 }, // Medium Violet
			{ 114, 100 }, // Glitter Trans-Dark Pink
			{ 115,  76 }, // Medium Lime
			{ 117, 101 }, // Glitter Trans-Clear
			{ 118,  41 }, // Aqua
			{ 120,  35 }, // Light Lime
			{ 125,  32 }, // Light Orange
			{ 128,  68 }, // Dark Orange
			{ 129, 102 }, // Glitter Trans-Purple
			{ 132, 111 }, // Speckle Black-Silver
			{ 133, 151 }, // Speckle Black-Gold
			{ 134,  84 }, // Copper
			{ 135,  66 }, // Pearl Light Gray
			{ 137,  78 }, // Metal Blue
			{ 142,  61 }, // Pearl Light Gold
			{ 148,  77 }, // Pearl Dark Gray
			{ 150, 119 }, // Pearl Very Light Gray
			{ 151,  99 }, // Very Light Bluish Gray
			{ 178,  81 }, // Flat Dark Gold
			{ 179,  95 }, // Flat Silver
			{ 183,  83 }, // Pearl White
			{ 191, 110 }, // Bright Light Orange
			{ 212, 105 }, // Bright Light Blue
			{ 216,  27 }, // Rust
			{ 226, 103 }, // Bright Light Yellow
			{ 232,  87 }, // Sky Blue
			{ 256,  11 }, // Black
			{ 272,  63 }, // Dark Blue
			{ 273,   7 }, // Blue
			{ 284, 114 }, // Trans-Light Purple
			{ 288,  80 }, // Dark Green
			{ 294, 118 }, // Glow In Dark Trans
			{ 297, 115 }, // Pearl Gold
			{ 308, 120 }, // Dark Brown
			{ 313,  72 }, // Maersk Blue
			{ 320,  59 }, // Dark Red
			{ 321, 153 }, // Dark Azure
			{ 322, 156 }, // Medium Azure
			{ 323, 152 }, // Light Aqua
			{ 324,   5 }, // Red
			{ 326, 158 }, // Yellowish Green
			{ 330, 155 }, // Olive Green
			{ 334,  21 }, // Chrome Gold
			{ 335,  58 }, // Sand Red
			{ 350,   4 }, // Orange
			{ 351,  94 }, // Medium Dark Pink
			{ 366,  29 }, // Earth Orange
			{ 373,  54 }, // Sand Purple
			{ 375,   9 }, // Light Gray
			{ 378,  48 }, // Sand Green
			{ 379,  55 }, // Sand Blue
			{ 383,  22 }, // Chrome Silver
			{ 406,  63 }, // Dark Blue
			{ 449,  24 }, // Purple
			{ 450, 106 }, // Fabuland Brown
			{ 462,  31 }, // Medium Orange
			{ 484,  68 }, // Dark Orange
			{ 490,  34 }, // Lime
			{ 493, -10 }, // Magnet
			{ 494, -11 }, // Electric_Contact_Alloy
			{ 495, -12 }, // Electric_Contact_Copper
			{ 496,  86 }, // Light Bluish Gray
			{ 503,  49 }, // Very Light Gray
			{ 504,  95 }, // Flat Silver
			{ 511,   1 }, // White
			{ 10001, 159 }, // Glow In Dark White
			{ 10002, 160 }, // Fabuland Orange
			{ 10003, 161 }, // Dark Yellow
			{ 10004, 162 }, // Glitter Trans-Light Blue
			{ 10005, 163 }, // Glitter Trans-Neon Green
			{ 10006, 164 }, // Trans-Light Orange
			{ 10007, 165 }, // Neon Orange
			{ 10008, 220 }, // Coral
			{ 10009, 166 }, // Neon Green
			{ 10010, 221 }  // Trans-Light Green
		};

		int ColorCode = gColorList[ColorIndex].Code;

		for (unsigned int Color = 0; Color < LC_ARRAY_COUNT(BrickLinkColors); Color++)
			if (BrickLinkColors[Color].LDraw == ColorCode)
				return BrickLinkColors[Color].BrickLink;

		return 0;
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
