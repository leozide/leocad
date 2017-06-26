#include "lc_global.h"
#include "lc_lxf.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "piece.h"
#include "lc_file.h"

// Based on https://gitlab.com/sylvainls/lxf2ldr

#define LC_READ_XML_INT(attribute, variable) \
	variable = Reader.attributes().value(attribute).toInt(&Ok); \
	if (!Ok) \
		return false;
#define LC_READ_XML_FLOAT(attribute, variable) \
	variable = Reader.attributes().value(attribute).toFloat(&Ok); \
	if (!Ok) \
		return false;

inline QMatrix4x4 lcLDDRotationMatrix(double ax, double ay, double az, double Angle)
{
	QMatrix4x4 Matrix;
	Matrix.rotate(Angle, ax, ay, az);
	return Matrix;
}

struct lcLDDTransformation
{
	QVector3D Translation;
	QMatrix4x4 Rotation;

	lcLDDTransformation(const QVector3D& vtrans, const QMatrix4x4& vrot)
		: Translation(vtrans), Rotation(vrot)
	{
	}

	lcLDDTransformation()
	{
	}
};

struct lcLDDSubstitute
{
	QString LDrawFile;
	const lcLDDTransformation* Transformation;

	lcLDDSubstitute()
		: Transformation(nullptr)
	{
	}

	lcLDDSubstitute(const QString& vdatfile, const lcLDDTransformation *const vtransformation)
		: LDrawFile(vdatfile), Transformation(vtransformation)
	{
	}
};

struct lcLDDSimpleSubstitute
{
	QString LDrawFile;
	bool Overwrite;

	lcLDDSimpleSubstitute()
		: Overwrite(false)
	{
	}

	lcLDDSimpleSubstitute(const QString& vdatfile, bool voverwrite)
		: LDrawFile(vdatfile), Overwrite(voverwrite)
	{
	}
};

typedef QMap<int, lcLDDSimpleSubstitute> lcLDDColorMap;
typedef QMap<QString, lcLDDSubstitute> lcLDDDecorationMap;

struct DecorMatch
{
	int UseColor;
	QList<lcLDDColorMap> Colors;
	lcLDDDecorationMap Decorations;

	DecorMatch()
		: UseColor(0)
	{
	}

	DecorMatch(int vusecolor, const QList<lcLDDColorMap>& vcolors, const lcLDDDecorationMap& vdecorations)
		: UseColor(vusecolor), Colors(vcolors), Decorations(vdecorations)
	{
	}
};

struct lcLDDAssembly
{
	int id;
	QStringList Parts;

	lcLDDAssembly()
	{
	}

	lcLDDAssembly(int vid, const QStringList vparts)
		: id(vid), Parts(vparts)
	{
	}
};

static bool lcLDDReadLDrawXML(QMap<int, int>& MaterialTable, QMap<int, QString>& BrickTable, QMap<QString, lcLDDTransformation>& TransformationTable, QMap<int, lcLDDAssembly>& AssemblyTable)
{
	QString DiskFileName(lcGetPiecesLibrary()->mLibraryDir.absoluteFilePath(QLatin1String("ldraw.xml")));

	QXmlStreamReader Reader;
	QFile DiskFile(DiskFileName);

	if (DiskFile.open(QFile::ReadOnly | QFile::Text))
		Reader.setDevice(&DiskFile);
	else
	{
		QResource Resource(QLatin1String(":/resources/ldraw.xml"));

		if (Resource.isValid())
		{
			QByteArray Data;

			if (Resource.isCompressed())
				Data = qUncompress(Resource.data(), Resource.size());
			else
				Data = QByteArray::fromRawData((const char*)Resource.data(), Resource.size());

			Reader.addData(QString::fromUtf8(Data));
		}
	}

	if (!Reader.readNextStartElement() || Reader.name() != "LDrawMapping")
		return false;

	bool Ok;

	while (Reader.readNextStartElement())
	{
		if (Reader.name() == "Material")
		{
			int Lego, LDraw;
			LC_READ_XML_INT("lego", Lego);
			LC_READ_XML_INT("ldraw", LDraw);

			MaterialTable[Lego] = LDraw;
			Reader.skipCurrentElement();
		}
		else if (Reader.name() == "Brick")
		{
			int Lego;
			LC_READ_XML_INT("lego", Lego);

			BrickTable[Lego] = Reader.attributes().value("ldraw").toString().toLower();
			Reader.skipCurrentElement();
		}
		else if (Reader.name() == "Transformation")
		{
			float tx, ty, tz, ax, ay, az, Angle;
			LC_READ_XML_FLOAT("tx", tx);
			LC_READ_XML_FLOAT("ty", ty);
			LC_READ_XML_FLOAT("tz", tz);
			LC_READ_XML_FLOAT("ax", ax);
			LC_READ_XML_FLOAT("ay", ay);
			LC_READ_XML_FLOAT("az", az);
			LC_READ_XML_FLOAT("angle", Angle);

			TransformationTable[Reader.attributes().value("ldraw").toString().toLower()] = lcLDDTransformation(QVector3D(-tx, -ty, -tz), lcLDDRotationMatrix(ax, ay, az, -180.0 * Angle / M_PI));
			Reader.skipCurrentElement();
		}
		else if (Reader.name() == "Assembly")
		{
			int Lego;
			LC_READ_XML_INT("lego", Lego);

			QStringList Parts;
			while (Reader.readNextStartElement())
			{
				if (Reader.name() == "Part")
					Parts << Reader.attributes().value("ldraw").toString().toLower();

				Reader.skipCurrentElement();
			}
			AssemblyTable[Lego] = lcLDDAssembly(Lego, Parts);
		}
		else
		{
			Reader.skipCurrentElement();
		}
	}

	return !Reader.hasError();
}

static QString lcLDDGetLDrawPart(int LegoID, const QMap<int, QString>& BrickTable)
{
	if (BrickTable.contains(LegoID))
		return BrickTable.value(LegoID);
	return QString("%1.dat").arg(LegoID);
}

static const lcLDDTransformation lcLDDGetLDrawTransform(const QString& LDraw, const QMap<QString, lcLDDTransformation>& TransformationTable)
{
	return TransformationTable.value(LDraw);
}

static int lcLDDGetLDrawColor(int LegoColor, const QMap<int, int>& MaterialTable)
{
	if (MaterialTable.contains(LegoColor))
		return MaterialTable.value(LegoColor);
	return LegoColor;
}

static bool lcLDDReadDecors(QMap<int, DecorMatch>& DecorationTable)
{
	lcDiskFile DiskFile(lcGetPiecesLibrary()->mLibraryDir.absoluteFilePath(QLatin1String("decors_lxf2ldr.yaml")));
	lcMemFile MemFile;
	lcFile* File;

	if (DiskFile.Open(QIODevice::ReadOnly))
		File = &DiskFile;
	else
	{
		QResource Resource(":/resources/decors_lxf2ldr.yaml");

		if (Resource.isValid())
		{
			QByteArray Data;

			if (Resource.isCompressed())
				Data = qUncompress(Resource.data(), Resource.size());
			else
				Data = QByteArray::fromRawData((const char*)Resource.data(), Resource.size());

			MemFile.WriteBuffer(Data.constData(), Data.size());
			MemFile.Seek(0, SEEK_SET);
			File = &MemFile;
		}
	}

	char Line[1024];
	int CurrentLegoId = -1;
	bool ParsingColors = true;
	int UseColor = 0;
	QList<lcLDDColorMap> Colors;
	lcLDDDecorationMap Decorations;

	auto AddDecor = [&]()
	{
		if (CurrentLegoId == -1)
			return;

		DecorationTable[CurrentLegoId] = DecorMatch(UseColor, Colors, Decorations);

		UseColor = 0;
		Colors.clear();
		Decorations.clear();
		CurrentLegoId = -1;
	};

	while (File->ReadLine(Line, sizeof(Line)))
	{
		char* Comment = strchr(Line, '#');
		if (Comment)
			*Comment = 0;

		if (*Line == 0)
		{
			AddDecor();
			continue;
		}

		int Indent = 0;
		for (char* c = Line; *c == ' '; c++)
			Indent++;

		if (Indent == 0)
		{
			AddDecor();

			if (!strchr(Line, ':'))
				continue;

			CurrentLegoId = atoi(Line);
		}
		else if (Indent == 2)
		{
			char* Separator = strchr(Line, ':');
			if (!Separator)
				continue;
			*Separator = 0;
			Separator++;

			if (!strcmp(Line + 2, "usecolor"))
				UseColor = atoi(Separator);
			else if (!strcmp(Line + 2, "colors"))
				ParsingColors = true;
			else if (!strcmp(Line + 2, "decorations"))
				ParsingColors = false;
		}
		else if (Indent == 4)
		{
			if (ParsingColors)
			{
				if (Line[4] == '-')
					Colors << lcLDDColorMap();
			}
			else
			{
				static const QMap<QString, QMatrix4x4> RotationTable =
				{
					{ "x",     lcLDDRotationMatrix(1, 0, 0,   90) },
					{ "xx",    lcLDDRotationMatrix(1, 0, 0,  180) },
					{ "xxx",   lcLDDRotationMatrix(1, 0, 0,  -90) },
					{ "y",     lcLDDRotationMatrix(0, 1, 0,   90) },
					{ "yy",    lcLDDRotationMatrix(0, 1, 0,  180) },
					{ "yyy",   lcLDDRotationMatrix(0, 1, 0,  -90) },
					{ "y7p/6", lcLDDRotationMatrix(0, 1, 0,  210) },
					{ "z",     lcLDDRotationMatrix(0, 0, 1,   90) },
					{ "zz",    lcLDDRotationMatrix(0, 0, 1,  180) },
					{ "zzz",   lcLDDRotationMatrix(0, 0, 1,  -90) },
				};

				char* Separator = strchr(Line, ':');
				if (!Separator)
					continue;
				*Separator = 0;
				Separator++;

				char* Key = Line + 4;

				if (Key[0] != '\'')
					continue;
				Key++;

				char* Quote = strchr(Key, '\'');
				if (!Quote)
					continue;
				*Quote = 0;

				QStringList SubList = QString(Separator).trimmed().split(' ');
				QString LDrawFile = SubList.first().toLower();
				QString Rotation;
				if (SubList.size() > 1)
					Rotation = SubList.at(1);
				if (RotationTable.contains(Rotation))
					Decorations.insert(Key, lcLDDSubstitute(LDrawFile, new lcLDDTransformation(QVector3D(), RotationTable[Rotation])));
				else
					Decorations.insert(Key, lcLDDSubstitute(LDrawFile, 0));
			}
		}
		else if (Indent == 6)
		{
			if (ParsingColors)
			{
				char* Separator = strchr(Line, ':');
				if (!Separator)
					continue;
				*Separator = 0;
				Separator++;

				int LegoColor = atoi(Line + 4);

				QStringList SimpleList = QString(Separator).trimmed().split(' ');
				QString LDrawFile = SimpleList.first().toLower();
				bool Overwrite = false;
				if (SimpleList.size() > 1)
					Overwrite = SimpleList.at(1).toUpper() == QStringLiteral("OW");

				if (!Colors.empty())
					Colors.last().insert(LegoColor, lcLDDSimpleSubstitute(LDrawFile, Overwrite));
			}
		}
	}

	return true;
}

static void lcLDDGetDecorationSubstitute(int lego, const QString& decorations, const QList<int>& colors, int& usecolor, lcLDDSubstitute& substitute, const QMap<int, DecorMatch>& DecorationTable)
{
	if (!DecorationTable.contains(lego))
		return;

	const DecorMatch decor = DecorationTable.value(lego);
	usecolor = decor.UseColor;
	const int maxcol = qMin(colors.size(), decor.Colors.size());

	for (int i = 0; i < maxcol; ++i)
	{
		const int curcol = colors.at(i) == 0 ? colors.first() : colors.at(i);
		if (decor.Colors.at(i).contains(curcol))
		{
			const lcLDDSimpleSubstitute sub = decor.Colors.at(i).value(curcol);
			if (substitute.LDrawFile.isEmpty() || sub.Overwrite)
				substitute = lcLDDSubstitute(sub.LDrawFile, 0);
		}
	}

	if (decor.Decorations.contains(decorations))
		substitute = decor.Decorations.value(decorations);
}

bool lcImportLDDFile(const QString& FileData, lcArray<lcPiece*>& Pieces, lcArray<lcArray<lcPiece*>>& Groups)
{
	QMap<int, int> MaterialTable;
	QMap<int, QString> BrickTable;
	QMap<QString, lcLDDTransformation> TransformationTable;
	QMap<int, lcLDDAssembly> AssemblyTable;

	if (!lcLDDReadLDrawXML(MaterialTable, BrickTable, TransformationTable, AssemblyTable))
	{
		QMessageBox::information(gMainWindow, QApplication::tr("Error"), QApplication::tr("Error loading conversion data from ldraw.xml."));
		return false;
	}

	QMap<int, DecorMatch> DecorationTable;
	lcLDDReadDecors(DecorationTable);

	QXmlStreamReader Reader(FileData);
	Reader.readNext();

	if (!Reader.readNextStartElement() || Reader.name() != "LXFML")
		return false;

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	bool Ok;

	while (Reader.readNextStartElement())
	{
		if (Reader.name() == "Bricks")
		{
			while (Reader.readNextStartElement())
			{
				if (Reader.name() == "Brick")
				{
					int NumPieces = Pieces.GetSize();

					while (Reader.readNextStartElement())
					{
						if (Reader.name() == "Part")
						{
							int DesignId;
							LC_READ_XML_INT("designID", DesignId);

							const QString Decoration = Reader.attributes().value("decoration").toString();

							const QStringList ColorList = Reader.attributes().value("materials").toString().split(',');
							QList<int> PartColors;
							for (const QString& color_txt : ColorList)
							{
								PartColors << color_txt.toInt(&Ok);
								if (!Ok)
									return false;
							}

							QStringList Bones;
							while (Reader.readNextStartElement())
							{
								if (Reader.name() == "Bone")
									Bones << Reader.attributes().value("transformation").toString();
								Reader.skipCurrentElement();
							}

							if (Bones.isEmpty())
								return false;

							QList<QVector3D> Positions;
							QList<QMatrix4x4> Rotations;

							for (const QString& Bone : Bones)
							{
								const QStringList FloatList = Bone.split(',');
								
								if (FloatList.size() != 12)
									return false;

								float Matrix[12];
								
								for (int i = 0; i < FloatList.size(); ++i)
								{
									Matrix[i] = FloatList.at(i).toFloat(&Ok);
									if (!Ok)
										return false;
								}
								Positions << QVector3D(Matrix[9], Matrix[10], Matrix[11]);
								QMatrix4x4 Rotation(Matrix[0], Matrix[3], Matrix[6], 0, Matrix[1], Matrix[4], Matrix[7], 0, Matrix[2], Matrix[5], Matrix[8], 0, 0, 0, 0, 1);
								Rotation.optimize();
								Rotations << Rotation;
							}

							QString PartID = lcLDDGetLDrawPart(DesignId, BrickTable);
							const lcLDDTransformation LDrawTransform = lcLDDGetLDrawTransform(PartID, TransformationTable);

						    int UseColor = 0;
							lcLDDSubstitute Substitute;
							lcLDDGetDecorationSubstitute(DesignId, Decoration, PartColors, UseColor, Substitute, DecorationTable);
							if (!Substitute.LDrawFile.isEmpty())
								PartID = Substitute.LDrawFile;

							QList<int> LDrawColors;
							int ColorCode = lcLDDGetLDrawColor(PartColors.first(), MaterialTable);
							for (int Color : PartColors)
								LDrawColors << (Color ? lcLDDGetLDrawColor(Color, MaterialTable) : ColorCode);
							ColorCode = LDrawColors.at(UseColor);

							const lcLDDTransformation *const SubTransform = Substitute.Transformation;
							const static QMatrix4x4 Axes = lcLDDRotationMatrix(1, 0, 0, 180);
							const static QVector3D Zero;

							const QMatrix4x4 Rot = Rotations.first() * LDrawTransform.Rotation;
							const QMatrix4x4 ldr_rot = Axes * (SubTransform ? (Rot * SubTransform->Rotation) : Rot) * Axes;

							const QVector3D Move = (SubTransform ? SubTransform->Translation : Zero) + LDrawTransform.Translation;
							const QVector3D ldr_pos = Axes * (Positions.first() + (Rot * Move)) * 25;


							lcMatrix44 LDrawMatrix(lcVector4(ldr_rot(0, 0), ldr_rot(1, 0), ldr_rot(2, 0), 0.0f), lcVector4(ldr_rot(0, 1), ldr_rot(1, 1), ldr_rot(2, 1), 0.0f),
												   lcVector4(ldr_rot(0, 2), ldr_rot(1, 2), ldr_rot(2, 2), 0.0f), lcVector4(ldr_pos[0], ldr_pos[1], ldr_pos[2], 1.0f));

							float* Matrix = LDrawMatrix;
							lcMatrix44 Transform(lcVector4(Matrix[0], Matrix[2], -Matrix[1], 0.0f), lcVector4(Matrix[8], Matrix[10], -Matrix[9], 0.0f),
												 lcVector4(-Matrix[4], -Matrix[6], Matrix[5], 0.0f), lcVector4(Matrix[12], Matrix[14], -Matrix[13], 1.0f));

							PartID = PartID.toUpper();
							PartID.chop(4);
							PieceInfo* Info = Library->FindPiece(PartID.toLatin1().constData(), nullptr, true, false);

							lcPiece* Piece = new lcPiece(nullptr);
							Piece->SetPieceInfo(Info, false);
							Piece->Initialize(Transform, 1);
							Piece->SetColorCode(ColorCode);
							Pieces.Add(Piece);
						}
						else
						{
							Reader.skipCurrentElement();
						}
					}

					if (Pieces.GetSize() != NumPieces + 1)
					{
						lcArray<lcPiece*> Group;
						for (int PieceIdx = NumPieces; PieceIdx < Pieces.GetSize(); PieceIdx++)
							Group.Add(Pieces[PieceIdx]);
						Groups.Add(Group);
					}
				}
				else
					Reader.skipCurrentElement();
			}
		}
		else if (Reader.name() == "GroupSystems")
		{
			// todo: read ldd groups
			Reader.skipCurrentElement();
		}
		else
			Reader.skipCurrentElement();
	}

	return true;
}
