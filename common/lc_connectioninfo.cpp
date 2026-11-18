#include "lc_global.h"
#include "lc_connectioninfo.h"
#include "lc_file.h"
#include "lc_library.h"
#include "lc_piecedata.h"
#include "lc_zipfile.h"

void lcConnectionInfo::Initialize(lcPiecesLibrary* Library)
{
	lcZipFile Archive;
	
	if (!Archive.OpenRead(QLatin1String(":/resources/connections.zip")))
		return;

	lcResult<void> LoadResult = LoadArchive(Archive, Library);
	
	if (!LoadResult)
		qWarning() << "Could not load bundled connection data:" << LoadResult.error();
}

bool lcConnectionInfo::ParseVector(const QJsonValue& Value, lcVector3& Vector)
{
	if (!Value.isArray())
		return false;

	const QJsonArray Array = Value.toArray();
	
	if (Array.size() != 3)
		return false;

	float Components[3];
	
	for (int ComponentIndex = 0; ComponentIndex < 3; ComponentIndex++)
	{
		if (!Array[ComponentIndex].isDouble())
			return false;

		const double Component = Array[ComponentIndex].toDouble();
		if (!std::isfinite(Component) || Component < -FLT_MAX || Component > FLT_MAX)
			return false;

		Components[ComponentIndex] = static_cast<float>(Component);
	}

	Vector = lcVector3LDrawToLeoCAD(lcVector3(Components[0], Components[1], Components[2]));
	
	return !Vector.IsNan();
}

bool lcConnectionInfo::ParsePart(const QByteArray& Data, const std::string& PartName, int ExpectedVersion, int ExpectedConnections, std::vector<lcConnectionPoint>& Connections)
{
	QJsonParseError ParseError;
	const QJsonDocument Document = QJsonDocument::fromJson(Data, &ParseError);
	
	if (ParseError.error != QJsonParseError::NoError || !Document.isObject())
		return false;

	const QJsonObject Root = Document.object();
	
	if (!Root.value(QStringLiteral("part")).isString() || Root.value(QStringLiteral("part")).toString().toStdString() != PartName ||
		!Root.value(QStringLiteral("version")).isDouble() || Root.value(QStringLiteral("version")).toInt(-1) != ExpectedVersion ||
		!Root.value(QStringLiteral("connections")).isArray())
		return false;

	const QJsonArray Array = Root.value(QStringLiteral("connections")).toArray();
	
	if (Array.size() != ExpectedConnections)
		return false;

	std::vector<lcConnectionPoint> ParsedConnections;
	ParsedConnections.reserve(static_cast<size_t>(Array.size()));
	std::unordered_set<std::string> ConnectionIds;

	for (const QJsonValue& ConnectionValue : Array)
	{
		if (!ConnectionValue.isObject())
			return false;

		const QJsonObject ConnectionObject = ConnectionValue.toObject();
		
		if (!ConnectionObject.value(QStringLiteral("id")).isString() || !ConnectionObject.value(QStringLiteral("type")).isString())
			return false;

		const std::string ConnectionId = ConnectionObject.value(QStringLiteral("id")).toString().toStdString();
		
		if (ConnectionId.empty() || !ConnectionIds.insert(ConnectionId).second)
			return false;

		lcConnectionPoint Connection;
		const QString ConnectionType = ConnectionObject.value(QStringLiteral("type")).toString();
		
		if (ConnectionType == QStringLiteral("stud_male"))
			Connection.Type = lcConnectionType::StudMale;
		else if (ConnectionType == QStringLiteral("stud_receiver"))
			Connection.Type = lcConnectionType::StudReceiver;
		else
			return false;

		if (!ParseVector(ConnectionObject.value(QStringLiteral("position")), Connection.Position) ||
			!ParseVector(ConnectionObject.value(QStringLiteral("direction")), Connection.Direction))
			return false;

		const float DirectionLength = Connection.Direction.Length();
		
		if (!std::isfinite(DirectionLength) || fabsf(DirectionLength - 1.0f) > 0.001f)
			return false;

		ParsedConnections.push_back(std::move(Connection));
	}

	Connections = std::move(ParsedConnections);
	
	return true;
}

lcResult<void> lcConnectionInfo::LoadArchive(lcZipFile& Archive, lcPiecesLibrary* Library)
{
	const std::optional<quint32> ManifestIndex = Archive.FindFileIndex("manifest.json");
	
	if (!ManifestIndex)
		return lcUnexpected(QStringLiteral("Connection archive is missing manifest.json."));

	lcMemFile ManifestFile;
	
	if (!Archive.ExtractFile(*ManifestIndex, ManifestFile, 1024 * 1024))
		return lcUnexpected(QStringLiteral("Could not read connection archive manifest."));
	
	const QByteArray ManifestBytes(reinterpret_cast<const char*>(ManifestFile.mBuffer), static_cast<int>(ManifestFile.mFileSize));
	QJsonParseError ParseError;
	const QJsonDocument ManifestDocument = QJsonDocument::fromJson(ManifestBytes, &ParseError);
	
	if (ParseError.error != QJsonParseError::NoError || !ManifestDocument.isObject())
		return lcUnexpected(QStringLiteral("Connection archive manifest is invalid JSON."));

	const QJsonObject Manifest = ManifestDocument.object();
	
	if (Manifest.value(QStringLiteral("format")).toString() != QStringLiteral("leocad-connections") ||
		!Manifest.value(QStringLiteral("version")).isDouble() ||
		Manifest.value(QStringLiteral("version")).toInt(-1) != 1 ||
		Manifest.value(QStringLiteral("coordinates")).toString() != QStringLiteral("ldraw_part_local") ||
		Manifest.value(QStringLiteral("units")).toString() != QStringLiteral("ldu") ||
		!Manifest.value(QStringLiteral("parts")).isObject())
		return lcUnexpected(QStringLiteral("Connection archive format or version is unsupported."));

	const QJsonObject PartEntries = Manifest.value(QStringLiteral("parts")).toObject();
	std::unordered_set<std::string> ExpectedArchiveEntries { "manifest.json" };
	int ConnectionTotal = 0;
	int MaleTotal = 0;
	int ReceiverTotal = 0;
	std::vector<std::pair<lcPieceData*, std::vector<lcConnectionPoint>>> PendingConnections;

	for (auto Entry = PartEntries.constBegin(); Entry != PartEntries.constEnd(); ++Entry)
	{
		if (!Entry.value().isObject() || !Entry.key().endsWith(QStringLiteral(".dat")))
			return lcUnexpected(QStringLiteral("Connection archive contains an invalid part entry."));

		const QJsonObject PartEntry = Entry.value().toObject();
		const QString ArchivePath = PartEntry.value(QStringLiteral("file")).toString();
		const int Count = PartEntry.value(QStringLiteral("connections")).toInt(-1);
		const QString ExpectedHash = PartEntry.value(QStringLiteral("sha256")).toString().toLower();
		const QJsonValue Version = PartEntry.value(QStringLiteral("version"));
		const bool SupportedVersion = Version.isUndefined() || (Version.isDouble() && Version.toDouble() == 1.0);
		const bool HashIsHex = ExpectedHash.size() == 64 && std::all_of(ExpectedHash.cbegin(), ExpectedHash.cend(), [](QChar Character)
		{
			return Character.isDigit() || (Character >= QLatin1Char('a') && Character <= QLatin1Char('f'));
		});
		
		if (ArchivePath != QStringLiteral("parts/") + Entry.key().left(Entry.key().size() - 4) + QStringLiteral(".json") || Count < 0 || !SupportedVersion || !HashIsHex)
			return lcUnexpected(QStringLiteral("Connection archive manifest has invalid metadata for %1.").arg(Entry.key()));

		const std::optional<quint32> PartIndex = Archive.FindFileIndex(ArchivePath.toLatin1().constData());
		
		if (!PartIndex)
			return lcUnexpected(QStringLiteral("Connection archive is missing %1.").arg(ArchivePath));

		lcMemFile PartFile;
		
		if (!Archive.ExtractFile(*PartIndex, PartFile, 16 * 1024 * 1024))
			return lcUnexpected(QStringLiteral("Could not read %1.").arg(ArchivePath));

		const QByteArray PartBytes(reinterpret_cast<const char*>(PartFile.mBuffer), static_cast<int>(PartFile.mFileSize));
		const QByteArray ActualHash = QCryptographicHash::hash(PartBytes, QCryptographicHash::Sha256).toHex();
		
		if (ActualHash != ExpectedHash.toLatin1())
			return lcUnexpected(QStringLiteral("Connection data hash mismatch for %1.").arg(Entry.key()));

		const std::string PartName = Entry.key().toStdString();
		std::vector<lcConnectionPoint> Connections;
		
		if (!ParsePart(PartBytes, PartName, 1, Count, Connections))
			return lcUnexpected(QStringLiteral("Connection data is invalid for %1.").arg(Entry.key()));

		for (const lcConnectionPoint& Connection : Connections)
			if (Connection.Type == lcConnectionType::StudMale)
				MaleTotal++;
			else
				ReceiverTotal++;

		ConnectionTotal += Count;
		ExpectedArchiveEntries.insert(ArchivePath.toStdString());

		lcPieceData* PieceData = Library->FindPieceData(PartName.c_str(), nullptr);
		
		if (PieceData && PieceData->IsLibraryPiece() && !PieceData->GetConnectionInfo())
			PendingConnections.emplace_back(PieceData, std::move(Connections));
	}

	const QJsonObject Counts = Manifest.value(QStringLiteral("counts")).toObject();
	
	if (Counts.value(QStringLiteral("parts")).toInt(-1) != PartEntries.size() ||
		Counts.value(QStringLiteral("connections")).toInt(-1) != ConnectionTotal ||
		Counts.value(QStringLiteral("stud_male")).toInt(-1) != MaleTotal ||
		Counts.value(QStringLiteral("stud_receiver")).toInt(-1) != ReceiverTotal ||
		ExpectedArchiveEntries.size() != Archive.mFiles.size())
		return lcUnexpected(QStringLiteral("Connection archive entries or manifest counts do not match."));

	for (const lcZipFileInfo& FileInfo : Archive.mFiles)
		if (ExpectedArchiveEntries.find(FileInfo.file_name) == ExpectedArchiveEntries.end())
			return lcUnexpected(QStringLiteral("Connection archive contains an unlisted entry."));
	
	for (auto& [PieceData, Connections] : PendingConnections)
		if (!PieceData->GetConnectionInfo())
			PieceData->SetConnectionInfo(new lcConnectionInfo(std::move(Connections)));

	return lcResult<void>();
}
