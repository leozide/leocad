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
		"Boat=^%Boat | ^%Sail\n"
		"Brick=^%Brick\n"
		"Container=^%Container | ^%Box | ^Chest | ^%Storage | ^Mailbox\n"
		"Door and Window=^%Door | ^%Window | ^%Glass | ^%Freestyle | ^%Gate | ^%Garage | ^%Roller\n"
		"Electric=^%Battery | ^%Electric\n"
		"Hinge and Bracket=^%Hinge | ^%Bracket | ^%Turntable\n"
		"Hose=^%Hose | ^%Rubber | ^%String\n"
		"Minifig=^%Minifig\n"
		"Miscellaneous=^%Arm | ^%Barrel | ^%Brush | ^%Bucket | ^%Cardboard | ^%Claw | ^%Cockpit | ^%Cocoon | ^%Conveyor | ^%Crane | ^%Cupboard | ^%Fence | ^%Gold | ^%Handle | ^%Hook | ^%Jack | ^%Key | ^%Ladder | ^%Medical | ^%Motor | ^%Rock | ^%Rope | ^%Slide | ^%Sheet | ^%Snow | ^%Sports | ^%Spring | ^%Staircase | ^%Stretcher | ^%Tap | ^%Tipper | ^%Trailer | ^%Umbrella | ^%Winch\n"
		"Other=^%Ball | ^%Belville | ^%BigFig | ^%Die | ^%Duplo | ^%Fabuland | ^%Figure | ^%Homemaker | ^%Maxifig | ^%Microfig | ^%Mursten | ^%Quatro | ^%Scala | ^%Znap\n"
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
		"Tyre and Wheel=^%Tyre | %^Wheel | %^Wheels | ^%Castle Wagon | ^%Axle\n"
		"Vehicle=^%Bike | ^%Canvas | ^%Car | ^%Excavator | ^%Exhaust | ^%Forklift | ^%Grab Jaw | ^%Jet | ^%Landing | ^%Motorcycle | ^%Plane | ^%Propellor | ^%Tail | ^%Tractor | ^%Vehicle | ^%Wheelbarrow\n"
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
		Stream << Format.arg(Category.Name, QString::fromLatin1(Category.Keywords));
	}

	Stream.flush();

	return true;
}

bool lcMatchCategory(const char* PieceName, const char* Expression)
{
	// Check if we need to split the test expression.
	const char* p = Expression;

	while (*p)
	{
		if (*p == '!')
		{
			return !lcMatchCategory(PieceName, p + 1);
		}
		else if (*p == '(')
		{
//			const char* Start = p;
			int c = 0;

			// Skip what's inside the parenthesis.
			do
			{
				if (*p == '(')
						c++;
				else if (*p == ')')
						c--;
				else if (*p == 0)
					return false; // Mismatched parenthesis.

				p++;
			}
			while (c);

			if (*p == 0)
				break;
		}
		else if ((*p == '|') || (*p == '&'))
		{
			std::string LeftStr(Expression, (p - Expression) - 1);
			std::string RightStr(p + 1);

			if (*p == '|')
				return lcMatchCategory(PieceName, LeftStr.c_str()) || lcMatchCategory(PieceName, RightStr.c_str());
			else
				return lcMatchCategory(PieceName, LeftStr.c_str()) && lcMatchCategory(PieceName, RightStr.c_str());
		}

		p++;
	}

	if (strchr(Expression, '('))
	{
		p = Expression;
		while (*p)
		{
			if (*p == '(')
			{
				const char* Start = p;
				int c = 0;

				// Extract what's inside the parenthesis.
				do
				{
					if (*p == '(')
							c++;
					else if (*p == ')')
							c--;
					else if (*p == 0)
						return false; // Mismatched parenthesis.

					p++;
				}
				while (c);

				std::string SubExpression(Start + 1, p - Start - 2);
				return lcMatchCategory(PieceName, SubExpression.c_str());
			}

			p++;
		}
	}

	const char* SearchStart = Expression;
	while (isspace(*SearchStart))
		SearchStart++;

	const char* SearchEnd = SearchStart + strlen(SearchStart) - 1;
	while (SearchEnd >= SearchStart && isspace(*SearchEnd))
		SearchEnd--;

	// Testing a simple case.
	std::string Search;
	if (SearchStart != SearchEnd)
		Search = std::string(SearchStart, SearchEnd - SearchStart + 1);
	const char* Word = Search.c_str();

	// Check for modifiers.
	bool WholeWord = 0;
	bool Begin = 0;

	for (;;)
	{
		if (Word[0] == '^')
			WholeWord = true;
		else if (Word[0] == '%')
			Begin = true;
		else
			break;

		Word++;
	}

	const char* Result = strcasestr(PieceName, Word);

	if (!Result)
		return false;

	if (Begin && (Result != PieceName))
	{
		if ((Result != PieceName + 1) || ((Result[-1] != '_') && (Result[-1] != '~')))
			return false;
	}

	if (WholeWord)
	{
		char End = Result[strlen(Word)];

		if ((End != 0) && (End != ' '))
			return false;

		if ((Result != PieceName) && ((Result[-1] == '_') || (Result[-1] == '~')))
			Result--;

		if ((Result != PieceName) && (Result[-1] != ' '))
			return false;
	}

	return true;
}
