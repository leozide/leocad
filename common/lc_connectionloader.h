#pragma once

#include "lc_connection.h"

class lcConnectionLoader
{
public:
	explicit lcConnectionLoader(const lcPiecesLibrary& Library);
	lcConnections Load(lcFile& File, const QString& FilePath, const std::set<std::string>& UnavailableTextures);

private:
	using lcConnectionProperties = std::map<QByteArray, QByteArray>;

	enum class lcConnectionScalePolicy { None, Axial, Radial, AxialOrRadial };
	enum class lcConnectionMirrorPolicy { None, CorrectX, CorrectZ };
	enum class lcConnectionTransformKind { Declaration, Include, Reference };

	struct lcConnectionRecord
	{
		lcConnection Data;
		std::vector<QByteArray> Ids;
		lcConnectionScalePolicy Scaling = lcConnectionScalePolicy::None;
		lcConnectionMirrorPolicy Mirroring = lcConnectionMirrorPolicy::None;
	};

	using lcConnectionRecords = std::vector<lcConnectionRecord>;
	using lcConnectionTokens = QList<QByteArray>;

	struct lcConnectionGrid
	{
		qlonglong Count[3]{ 1, 1, 1 };
		lcVector3 Step = lcVector3(0.0f, 0.0f, 0.0f);
		lcVector3 Center = lcVector3(0.0f, 0.0f, 0.0f);
	};

	enum class lcGeometryTexture { None, Mapped, Fallback };
	using lcGeometryReference = std::pair<QString, lcGeometryTexture>;

	struct lcGeometryLine
	{
		QByteArray Text;
		lcGeometryTexture Texture;
	};

	struct lcPieceGeometrySource
	{
		lcPieceGeometrySource(const QString& FilePath, const lcGeometryReference& SourceReference,
			std::vector<lcGeometryLine> SourceLines, const lcMatrix44& ParentTransform)
			: Path(FilePath), Reference(SourceReference), Lines(std::move(SourceLines)), Transform(ParentTransform)
		{
		}

		QString Path;
		lcGeometryReference Reference;
		std::vector<lcGeometryLine> Lines;
		lcMatrix44 Transform;
		lcConnectionRecords Connections;
		size_t NextLine = 0;
	};

	static lcConnectionTokens SplitWords(const QByteArray& Text);
	static QByteArray GetProperty(const lcConnectionProperties& Values, const char* Key, const char* Default = "");
	static bool ReadProperties(const QByteArray& Text, int Offset, lcConnectionProperties& Values);
	static bool ReadNumber(const QByteArray& Text, float& Number);
	static bool ReadNumbers(const lcConnectionTokens& Tokens, int Offset, int Count, float* Numbers);
	static lcMatrix44 MakeFrame(const float (&Position)[3], const float (&Orientation)[9]);
	static bool ReadFrame(const lcConnectionProperties& Values, lcMatrix44& Frame);
	static bool ReadGrid(const QByteArray& Text, lcConnectionGrid& Grid);
	static lcConnectionGender ReadGender(const lcConnectionProperties& Values, const char* Key);
	static bool ReadCylinder(const lcConnectionProperties& Values, lcConnectionRecord& Item);
	static bool ReadGeneric(const QByteArray& Kind, const lcConnectionProperties& Values, lcConnectionRecord& Item);
	static bool ReadShape(const QByteArray& Kind, const lcConnectionProperties& Values, lcConnectionRecord& Item);
	static bool TransformRecord(lcConnectionRecord& Item, const lcMatrix44& Parent, lcConnectionTransformKind Kind);
	static QString NormalizeReference(QString Path);

	lcConnectionRecords ReadGeometry(lcFile& File, const QString& Path, const std::set<std::string>& UnavailableTextures);
	std::vector<lcGeometryLine> ReadGeometryLines(lcFile& File, lcGeometryTexture InheritedTexture,
		const std::set<std::string>& UnavailableTextures) const;
	void ReadConnectionDirectives(lcFile& File, lcConnectionRecords& Result, bool AllowIncludes);
	void ReadConnectionDirective(const QByteArray& Line, lcConnectionRecords& Result, bool AllowIncludes);
	void Include(const lcConnectionProperties& Values, const lcMatrix44& Frame, const lcConnectionGrid& Grid, lcConnectionRecords& Result);
	void PlaceGrid(const lcConnectionGrid& Grid, const lcMatrix44& Frame, const lcConnectionRecord* Connections, size_t Count, lcConnectionRecords& Result);
	void Append(lcConnectionRecords& Result, const lcConnectionRecords& Included, const lcMatrix44& Frame,
		const QByteArray& Id, const QByteArray& Slide, lcConnectionTransformKind Kind);

	const lcPiecesLibrary& mLibrary;
	std::map<QString, lcConnectionRecords> mConnectionIncludes;
};
