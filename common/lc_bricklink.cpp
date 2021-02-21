
#include <locale.h>

#include "lc_colors.h"
#include "lc_file.h"
#include "lc_mainwindow.h"
#include "lc_model.h"
#include "pieceinf.h"
#include "project.h"

namespace {

	// The mapping LDraw to BL. Key is LDraw, Value is BL.
	//
	// The list is in *alphabetical* order for convenience.
	std::map<std::string, const char*> Remapping = {
		{"10201", "2436b"},
		{"10314", "6191"},
		{"10830", "10830c01"},
		{"10830p01", "10830c01"},
		{"12825", "2555"},
		{"14395", "2339"},
		{"14769p07", "14769pb011"},
		{"15672", "92946"},
		{"167", "6190"},
		{"2431p70", "2431pb499"},
		{"2436a", "2436"},
		{"2454b", "2454"},
		{"2476a", "2476"},
		{"2654a", "2654"},
		{"298", "4592"},
		{"2714", "2714a"},
		{"30039", "3070b"},
		{"3005pf0", "3005pb016"},
		{"30071", "3005"},
		{"30089a", "30089"},
		{"30009", "2362b"},
		{"3039pc8", "3039pb013"},
		{"3039pc9", "3039pb045"},
		{"30224", "x59"},
		{"30244", "2412b"},
		{"30258p01", "30258pb001"},
		{"30296p01", "30296px1"},
		{"30320", "30117"},
		{"30367", "553a"},
		{"3039p04", "3039px16"},
		{"3039p101", "3039ps1"},
		{"3040b", "3040"},
		{"3046", "3046a"},
		{"30499", "3684a"},
		{"30646", "30646a"},
		{"3068bp00", "3068bpb0116"},
		{"3069bp0i", "3069bpb436"},
		{"3069bp12", "3069p12"},
		{"3069p25", "3069bp25"},
		{"314d", "314"},
		{"3149d", "3149"},
		{"32064a", "32064"},
		{"32123a", "4265c"},
		{"3245a", "3245c"},
		{"32532", "40345"},
		{"32532b", "40345"},
		{"33299a", "33299"},
		{"3650c", "3650b"},
		{"3660b", "3660"},
		{"3665b", "3665"},
		{"3747", "3747a"},
		{"3815b", "970"},
		{"3816b", "971"},
		{"3817b", "972"},
		{"3818", "982"},
		{"3819", "981"},
		{"3820", "983"},
		{"4032a", "4032"},
		{"4287c", "4287"},
		{"4328", "x93"},
		{"4345b", "4345"},
		{"44042", "2356"},
		{"44237", "2456"},
		{"4476", "4476b"},
		{"4770", "4773"},
		{"482", "30553"},
		{"4865a", "4865"},
		{"50746", "54200"},
		{"51011", "42611"},
		{"55295", "6246b"},
		{"55299", "6246e"},
		{"55300", "6246d"},
		{"577", "64567"},
		{"577b", "64567"},
		{"59443", "6538c"},
		{"59900", "4589b"},
		{"6014c02", "6014bc05"},
		{"604547", "11402h"},
		{"604548", "11402c"},
		{"604549", "11402b"},
		{"604550", "11402a"},
		{"604551", "11402g"},
		{"604552", "11402i"},
		{"604553", "11402d"},
		{"604614", "11402f"},
		{"604615", "11402e"},
		{"60470a", "60470"},
		{"60475", "30241"},
		{"60475b", "30241b"},
		{"60616a", "60616"},
		{"60803", "57895"},
		{"60897", "4085d"},
		{"6141", "4073"},
		{"6143", "3941"},
		{"6188", "4589"},
		{"6269", "2343"},
		{"63965a", "63965"},
		{"71076a", "71075"},
		{"71137", "71137b"},
		{"72040", "71612cx1"},
		{"731", "731c02"},
		{"73603", "3403c01"},
		{"76385", "989"},
		{"88072", "4623b"},
		{"90194", "48183"},
		{"92410", "4532"},
		{"93221", "93221pb01"}, // LDraw has only one version
		{"93549", "93549pb01"},
		{"93888", "3007"}, //  Unoffical LDraw part
		{"95820", "30237b"},
		{"973p101", "973ps1"},
		{"975", "982"},
		{"976", "981"},
		{"977", "983"},
		{"u9026", "3062old"},
	};


	const char* BrickLinkRemap(const char* LDBrick)
	{
		auto Remapped = Remapping.find(LDBrick);
		if (Remapped != Remapping.end())
		{
			return Remapped->second;
		}
		return LDBrick;
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

			const char* Remapped = BrickLinkRemap(FileName);
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
