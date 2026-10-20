#include "lc_global.h"
#include "lc_shadowlibrary.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_file.h"

std::map<std::string, QString> lcShadowLibrary::mShadowFiles;
std::map<std::string, std::vector<lcShadowLine>> lcShadowLibrary::mShadowLines;
std::map<std::string, std::vector<lcConnectionCylinder>> lcShadowLibrary::mCylinders;

static const std::vector<lcConnectionCylinder> gNoCylinders;

bool lcShadowLibrary::SetPath(const QString& Path)
{
	mShadowFiles.clear();
	mShadowLines.clear();
	mCylinders.clear();

	const QDir Root(Path);

	for (const char* Folder : { "parts", "p" })
	{
		const QDir Dir(Root.filePath(QLatin1String(Folder)));
		QDirIterator It(Dir.absolutePath(), QStringList() << QLatin1String("*.dat"), QDir::Files, QDirIterator::Subdirectories);

		while (It.hasNext())
		{
			const QString FilePath = It.next();
			const std::string Name = NormalizeName(Dir.relativeFilePath(FilePath).toStdString());
			mShadowFiles.try_emplace(Name, FilePath);
		}
	}

	return !mShadowFiles.empty();
}

bool lcShadowLibrary::IsLoaded()
{
	return !mShadowFiles.empty();
}

std::string lcShadowLibrary::NormalizeName(const std::string& Name)
{
	std::string Result = lcPiecesLibrary::NormalizePieceDataName(Name.c_str());

	if (Result.size() < 4 || Result.compare(Result.size() - 4, 4, ".DAT") != 0)
		Result += ".DAT";

	return Result;
}

const std::vector<lcShadowLine>& lcShadowLibrary::GetShadowLines(const std::string& Name)
{
	const std::pair<std::map<std::string, std::vector<lcShadowLine>>::iterator, bool> LinesIt = mShadowLines.try_emplace(Name);
	std::vector<lcShadowLine>& Lines = LinesIt.first->second;

	if (!LinesIt.second)
		return Lines;

	const std::map<std::string, QString>::const_iterator FileIt = mShadowFiles.find(Name);

	if (FileIt == mShadowFiles.end())
		return Lines;

	QFile File(FileIt->second);

	if (!File.open(QIODevice::ReadOnly | QIODevice::Text))
		return Lines;

	while (!File.atEnd())
	{
		std::optional<lcShadowLine> Line = lcShadowParser::ParseLine(QString::fromUtf8(File.readLine()), Name);

		if (Line)
			Lines.push_back(std::move(*Line));
	}

	return Lines;
}

std::vector<lcConnectionCylinder> lcShadowLibrary::GetOwnCylinders(const std::string& Name)
{
	std::vector<lcConnectionCylinder> Cylinders;

	for (const lcShadowLine& Line : GetShadowLines(Name))
		if (Line.Type == lcShadowLineType::Cylinder)
			Cylinders.insert(Cylinders.end(), Line.Cylinders.begin(), Line.Cylinders.end());

	return Cylinders;
}

// The sub-files referenced by an LDraw file, with their transforms.
std::vector<std::pair<std::string, lcMatrix44>> lcShadowLibrary::ReadReferences(const std::string& Name)
{
	std::vector<std::pair<std::string, lcMatrix44>> References;
	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	lcLibraryPrimitive* Primitive = Library->FindPrimitive(Name.c_str());

	const lcResult<std::unique_ptr<lcFile>> File = Primitive ? Library->OpenPrimitiveFile(Primitive) : Library->OpenPieceDataFile(Name.c_str());

	if (!File)
		return References;

	char Line[1024];

	while (File.value()->ReadLine(Line, sizeof(Line)))
	{
		std::optional<std::pair<std::string, lcMatrix44>> Reference = lcShadowParser::ParseReference(Line);

		if (Reference)
			References.push_back(std::move(*Reference));
	}

	return References;
}

void lcShadowLibrary::AddPlacedCylinders(const std::vector<lcConnectionCylinder>& Cylinders, const lcMatrix44& Transform, std::vector<lcConnectionCylinder>& Result)
{
	for (lcConnectionCylinder Cylinder : Cylinders)
	{
		Cylinder.Transform = lcMul(Cylinder.Transform, Transform);

		if (lcShadowParser::NormalizeFrame(Cylinder))
			Result.push_back(std::move(Cylinder));
	}
}

void lcShadowLibrary::ApplyShadowLine(const lcShadowLine& Line, std::vector<lcConnectionCylinder>& Cylinders)
{
	switch (Line.Type)
	{
	case lcShadowLineType::Cylinder:
		Cylinders.insert(Cylinders.end(), Line.Cylinders.begin(), Line.Cylinders.end());
		break;

	case lcShadowLineType::Include:
		{
			const std::vector<lcConnectionCylinder> Included = GetOwnCylinders(NormalizeName(Line.Ref));

			for (const lcMatrix44& Placement : Line.Placements)
				AddPlacedCylinders(Included, Placement, Cylinders);
		}
		break;

	case lcShadowLineType::Clear:
		if (Line.Id.isEmpty())
			Cylinders.clear();
		else
			Cylinders.erase(std::remove_if(Cylinders.begin(), Cylinders.end(), [&Line](const lcConnectionCylinder& Cylinder) { return Line.Id.compare(Cylinder.Id, Qt::CaseInsensitive) == 0; }), Cylinders.end());
		break;
	}
}

const std::vector<lcConnectionCylinder>& lcShadowLibrary::GetCylinders(const char* PartName)
{
	if (mShadowFiles.empty())
		return gNoCylinders;

	const std::string Name = NormalizeName(PartName);

	// Insert first so a file that (indirectly) references itself can't recurse forever.
	const std::pair<std::map<std::string, std::vector<lcConnectionCylinder>>::iterator, bool> CylindersIt = mCylinders.try_emplace(Name);
	std::vector<lcConnectionCylinder>& Collected = CylindersIt.first->second;

	if (!CylindersIt.second)
		return Collected;

	std::vector<lcConnectionCylinder> Result;

	// Inherit the snap info of every sub-file, moved to where the sub-file is used.
	for (const auto& [ReferenceName, ReferenceTransform] : ReadReferences(Name))
		AddPlacedCylinders(GetCylinders(ReferenceName.c_str()), ReferenceTransform, Result);

	// Then the part's own shadow lines, in order, so SNAP_CLEAR only affects what came before it.
	for (const lcShadowLine& Line : GetShadowLines(Name))
		ApplyShadowLine(Line, Result);

	Collected = std::move(Result);
	return Collected;
}
