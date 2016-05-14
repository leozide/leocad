#include "lc_global.h"
#include "lc_category.h"
#include "lc_file.h"
#include "lc_profile.h"

lcArray<lcLibraryCategory> gCategories;

void lcResetDefaultCategories()
{
	lcResetCategories(gCategories);

	lcRemoveProfileKey(LC_PROFILE_CATEGORIES);
}

void lcLoadDefaultCategories(bool BuiltInLibrary)
{
	QByteArray Buffer = lcGetProfileBuffer(LC_PROFILE_CATEGORIES);

	if (Buffer.isEmpty() || !lcLoadCategories(Buffer, gCategories))
		lcResetCategories(gCategories, BuiltInLibrary);
}

void lcSaveDefaultCategories()
{
	QByteArray ByteArray;
	QTextStream Stream(&ByteArray, QIODevice::WriteOnly);

	lcSaveCategories(Stream, gCategories);

	lcSetProfileBuffer(LC_PROFILE_CATEGORIES, ByteArray);
}

void lcResetCategories(lcArray<lcLibraryCategory>& Categories, bool BuiltInLibrary)
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
		"Baseplate=^%Baseplate\n"
		"Brick=^%Brick\n"
		"Plate=^%Plate\n"
		"Slope=^%Slope\n"
		"Tile=^%Tile\n"
	};

	QByteArray Buffer;

	if (BuiltInLibrary)
		Buffer.append(BuiltInCategories, sizeof(BuiltInCategories));
	else
		Buffer.append(DefaultCategories, sizeof(DefaultCategories));

	lcLoadCategories(Buffer, Categories);
}

bool lcLoadCategories(const QString& FileName, lcArray<lcLibraryCategory>& Categories)
{
	QFile File(FileName);

	if (!File.open(QIODevice::ReadOnly))
		return false;

	QByteArray FileData = File.readAll();

	return lcLoadCategories(FileData, Categories);
}

bool lcLoadCategories(const QByteArray& Buffer, lcArray<lcLibraryCategory>& Categories)
{
	Categories.RemoveAll();

	QTextStream Stream(Buffer);

	for (QString Line = Stream.readLine(); !Line.isNull(); Line = Stream.readLine())
	{
		int Equals = Line.indexOf('=');

		if (Equals == -1)
			continue;

		QString Name = Line.left(Equals);
		QString Keywords = Line.mid(Equals + 1);

		lcLibraryCategory& Category = Categories.Add();

		Category.Name = Name.toLatin1().constData();
		Category.Keywords = Keywords.toLatin1().constData();
	}

	return true;
}

bool lcSaveCategories(const QString& FileName, const lcArray<lcLibraryCategory>& Categories)
{
	QFile File(FileName);

	if (!File.open(QIODevice::WriteOnly))
		return false;

	QTextStream Stream(&File);

	return lcSaveCategories(Stream, Categories);
}

bool lcSaveCategories(QTextStream& Stream, const lcArray<lcLibraryCategory>& Categories)
{
	QString Format("%1=%2\r\n");

	for (int CategoryIdx = 0; CategoryIdx < Categories.GetSize(); CategoryIdx++)
	{
		const lcLibraryCategory& Category = Categories[CategoryIdx];
		Stream << Format.arg((const char*)Category.Name, (const char*)Category.Keywords);
	}

	Stream.flush();

	return true;
}
