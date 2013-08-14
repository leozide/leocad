#include "lc_global.h"
#include "lc_category.h"
#include "lc_file.h"
#include "lc_profile.h"

ObjArray<lcLibraryCategory> gCategories;

void lcResetDefaultCategories()
{
	lcResetCategories(gCategories);

	lcRemoveProfileKey(LC_PROFILE_CATEGORIES);
}

void lcLoadDefaultCategories(bool BuiltInLibrary)
{
	lcMemFile File;

	lcGetProfileBuffer(LC_PROFILE_CATEGORIES, File);

	if (!File.GetLength() || !lcLoadCategories(File, gCategories))
		lcResetCategories(gCategories, BuiltInLibrary);
}

void lcSaveDefaultCategories()
{
	lcMemFile File;

	lcSaveCategories(File, gCategories);

	lcSetProfileBuffer(LC_PROFILE_CATEGORIES, File);
}

void lcResetCategories(ObjArray<lcLibraryCategory>& Categories, bool BuiltInLibrary)
{
	const char DefaultCategories[] =
	{
		"Animal=^%Animal | ^%Bone\n"
		"Antenna=^%Antenna\n"
		"Arch=^%Arch\n"
		"Bar=^%Bar\n"
		"Baseplate=^%Baseplate | ^%Platform\n"
		"Boat=^%Boat\n"
		"Brick=^%Brick\n"
		"Container=^%Container | ^%Box | ^Chest | ^%Storage | ^Mailbox\n"
		"Door and Window=^%Door | ^%Window | ^%Glass | ^%Freestyle | ^%Gate | ^%Garage | ^%Roller\n"
		"Electric=^%Electric\n"
		"Hinge and Bracket=^%Hinge | ^%Bracket | ^%Turntable\n"
		"Hose=^%Hose | ^%String\n"
		"Minifig=^%Minifig\n"
		"Miscellaneous=^%Arm | ^%Barrel | ^%Brush | ^%Claw | ^%Cockpit | ^%Conveyor | ^%Crane | ^%Cupboard | ^%Fence | ^%Jack | ^%Ladder | ^%Motor | ^%Rock | ^%Rope | ^%Sheet | ^%Sports | ^%Staircase | ^%Stretcher | ^%Tap | ^%Tipper | ^%Trailer | ^%Umbrella | ^%Winch\n"
		"Other=^%Ball | ^%Belville | ^%Die | ^%Duplo | ^%Fabuland | ^%Figure | ^%Homemaker | ^%Maxifig | ^%Microfig | ^%Mursten | ^%Scala | ^%Znap\n"
		"Panel=^%Panel | ^%Castle Wall | ^%Castle Turret\n"
		"Plant=^%Plant\n"
		"Plate=^%Plate\n"
		"Round=^%Cylinder | ^%Cone | ^%Dish | ^%Dome | ^%Hemisphere | ^%Round\n"
		"Sign and Flag=^%Flag | ^%Roadsign | ^%Streetlight | ^%Flagpost | ^%Lamppost | ^%Signpost\n"
		"Slope=^%Slope | ^%Roof\n"
		"Space=^%Space\n"
		"Sticker=^%Sticker\n"
		"Support=^%Support\n"
		"Technic=^%Technic | ^%Rack\n"
		"Tile=^%Tile\n"
		"Train=^%Train | ^%Monorail | ^%Magnet\n"
		"Tyre and Wheel=^%Tyre | %^Wheel | %^Wheels | ^%Castle Wagon\n"
		"Vehicle=^%Bike | ^%Canvas | ^%Car | ^%Excavator | ^%Exhaust | ^%Forklift | ^%Grab Jaw | ^%Landing | ^%Motorcycle | ^%Plane | ^%Propellor | ^%Tail | ^%Tractor | ^%Vehicle | ^%Wheelbarrow\n"
		"Windscreen=^%Windscreen\n"
		"Wedge=^%Wedge\n"
		"Wing=^%Wing\n"
	};

	const char BuiltInCategories[] =
	{
		"Brick=^%Brick\n"
		"Plate=^%Plate\n"
	};

	lcMemFile File;

	if (BuiltInLibrary)
		File.WriteBuffer(BuiltInCategories, sizeof(BuiltInCategories));
	else
		File.WriteBuffer(DefaultCategories, sizeof(DefaultCategories));
	File.Seek(0, SEEK_SET);

	lcLoadCategories(File, Categories);
}

bool lcLoadCategories(const char* FileName, ObjArray<lcLibraryCategory>& Categories)
{
	lcDiskFile File;

	if (!File.Open(FileName, "rt"))
		return false;

	return lcLoadCategories(File, Categories);
}

bool lcLoadCategories(lcFile& File, ObjArray<lcLibraryCategory>& Categories)
{
	Categories.RemoveAll();

	char Line[1024];

	while (File.ReadLine(Line, sizeof(Line)))
	{
		char* Key = strchr(Line, '=');

		if (!Key)
			continue;

		*Key = 0;
		Key++;

		char* NewLine = strchr(Key, '\n');
		if (NewLine)
			*NewLine = 0;

		lcLibraryCategory& Category = Categories.Add();

		Category.Name = Line;
		Category.Keywords = Key;
	}

	return true;
}

bool lcSaveCategories(const char* FileName, const ObjArray<lcLibraryCategory>& Categories)
{
	lcDiskFile File;

	if (!File.Open(FileName, "wt"))
		return false;

	return lcSaveCategories(File, Categories);
}

bool lcSaveCategories(lcFile& File, const ObjArray<lcLibraryCategory>& Categories)
{
	char Line[1024];

	for (int CategoryIdx = 0; CategoryIdx < Categories.GetSize(); CategoryIdx++)
	{
		lcLibraryCategory& Category = Categories[CategoryIdx];

		sprintf(Line, "%s=%s\n", (const char*)Category.Name, (const char*)Category.Keywords);

		File.WriteLine(Line);
	}

	return true;
}
