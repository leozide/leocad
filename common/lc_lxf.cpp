#include "lc_global.h"
#include "lc_lxf.h"
#include "lc_library.h"
#include "lc_application.h"
#include "lc_mainwindow.h"
#include "piece.h"
#include "lc_file.h"
#include <QDomDocument>

static bool lcLoadLDrawXML(std::map<int, int>& MaterialTable, std::map<int, std::string>& BrickTable, std::map<std::string, std::pair<lcVector3, lcVector4>>& TransformTable)
{
	QFile File(lcGetPiecesLibrary()->mLibraryDir.absoluteFilePath(QLatin1String("ldraw.xml")));
	QByteArray Data;

	if (File.open(QIODevice::ReadOnly))
		Data = File.readAll();
	else
	{
		QResource Resource(":/resources/ldraw.xml");

		if (Resource.isValid())
		{
			if (Resource.isCompressed())
				Data = qUncompress(Resource.data(), Resource.size());
			else
				Data = QByteArray::fromRawData((const char*)Resource.data(), Resource.size());
		}
	}

	if (Data.isEmpty())
		return false;

	QDomDocument Document;
	Document.setContent(QString::fromUtf8(Data));

	QDomElement Root = Document.documentElement();
	if (Root.tagName() != QLatin1String("LDrawMapping"))
		return false;

	for (QDomNode Node = Root.firstChild(); !Node.isNull(); Node = Node.nextSibling())
	{
		QDomElement Element = Node.toElement();
		QString ElementName = Element.tagName();

		if (ElementName == QLatin1String("Material"))
		{
			int LDrawColor = Element.attribute(QLatin1String("ldraw")).toInt();
			int LegoColor = Element.attribute(QLatin1String("lego")).toInt();

			MaterialTable[LegoColor] = LDrawColor;
		}
		else if (ElementName == QLatin1String("Brick"))
		{
			QString LDrawID = Element.attribute(QLatin1String("ldraw")).toUpper();
			LDrawID.chop(4);
			int LegoID = Element.attribute(QLatin1String("lego")).toInt();

			BrickTable.insert(std::make_pair(LegoID, std::move(LDrawID.toStdString())));
		}
		else if (ElementName == QLatin1String("Transformation"))
		{
			QString LDrawID = Element.attribute(QLatin1String("ldraw")).toUpper();
			LDrawID.chop(4);

			lcVector3 Translation;
			lcVector4 AxisAngle;

			Translation[0] = Element.attribute(QLatin1String("tx")).toFloat();
			Translation[1] = Element.attribute(QLatin1String("ty")).toFloat();
			Translation[2] = Element.attribute(QLatin1String("tz")).toFloat();
			AxisAngle[0] = Element.attribute(QLatin1String("ax")).toFloat();
			AxisAngle[1] = Element.attribute(QLatin1String("ay")).toFloat();
			AxisAngle[2] = Element.attribute(QLatin1String("az")).toFloat();
			AxisAngle[3] = Element.attribute(QLatin1String("angle")).toFloat();

			TransformTable.insert(std::make_pair(std::move(LDrawID.toStdString()), std::make_pair(Translation, AxisAngle)));
		}
	}

	return true;
}

bool lcImportLXFMLFile(const QString& FileData, lcArray<lcPiece*>& Pieces, lcArray<lcArray<lcPiece*>>& Groups)
{
	std::map<int, int> MaterialTable;
	std::map<int, std::string> BrickTable;
	std::map<std::string, std::pair<lcVector3, lcVector4>> TransformTable;

	if (!lcLoadLDrawXML(MaterialTable, BrickTable, TransformTable))
		return false;

	QDomDocument Document;
	Document.setContent(FileData);

	QDomElement Root = Document.documentElement();
	if (Root.tagName() != QLatin1String("LXFML"))
		return false;

	for (QDomNode SectionNode = Root.firstChild(); !SectionNode.isNull(); SectionNode = SectionNode.nextSibling())
	{
		QDomElement SectionElement = SectionNode.toElement();
		if (SectionElement.tagName() != QLatin1String("Bricks"))
			continue;

		for (QDomNode BrickNode = SectionElement.firstChild(); !BrickNode.isNull(); BrickNode = BrickNode.nextSibling())
		{
			QDomElement BrickElement = BrickNode.toElement();
			if (BrickElement.tagName() != QLatin1String("Brick"))
				continue;

			int NumBrickPieces = 0;

			for (QDomNode PartNode = BrickElement.firstChild(); !PartNode.isNull(); PartNode = PartNode.nextSibling())
			{
				QDomElement PartElement = PartNode.toElement();
				if (PartElement.tagName() != QLatin1String("Part"))
					continue;

				bool BoneFound = false;
				lcMatrix44 WorldMatrix;

				for (QDomNode BoneNode = PartElement.firstChild(); !BoneNode.isNull(); BoneNode = BoneNode.nextSibling())
				{
					QDomElement BoneElement = BoneNode.toElement();
					if (BoneElement.tagName() != QLatin1String("Bone"))
						continue;

					QString BoneTransform = BoneElement.attribute(QLatin1String("transformation"));
					QStringList BoneElements = BoneTransform.split(',');

					if (BoneElements.size() != 12)
						continue;

					WorldMatrix[0][0] = BoneElements[0].toFloat();
					WorldMatrix[0][1] = BoneElements[1].toFloat();
					WorldMatrix[0][2] = BoneElements[2].toFloat();
					WorldMatrix[0][3] = 0.0f;
					WorldMatrix[1][0] = BoneElements[3].toFloat();
					WorldMatrix[1][1] = BoneElements[4].toFloat();
					WorldMatrix[1][2] = BoneElements[5].toFloat();
					WorldMatrix[1][3] = 0.0f;
					WorldMatrix[2][0] = BoneElements[6].toFloat();
					WorldMatrix[2][1] = BoneElements[7].toFloat();
					WorldMatrix[2][2] = BoneElements[8].toFloat();
					WorldMatrix[3][3] = 0.0f;
					WorldMatrix[3][0] = BoneElements[9].toFloat();
					WorldMatrix[3][1] = BoneElements[10].toFloat();
					WorldMatrix[3][2] = BoneElements[11].toFloat();
					WorldMatrix[3][3] = 1.0f;

					BoneFound = true;
					break;
				}

				if (!BoneFound)
					continue;

				QString LegoID = PartElement.attribute(QLatin1String("designID"));
				int Material = PartElement.attribute(QLatin1String("materials")).split(',').first().toInt();

				PieceInfo* Info = nullptr;
				const auto BrickIt = BrickTable.find(LegoID.toInt());
				if (BrickIt != BrickTable.end())
					Info = lcGetPiecesLibrary()->FindPiece(BrickIt->second.c_str(), nullptr, true, false);
				else
					Info = lcGetPiecesLibrary()->FindPiece(LegoID.toLatin1(), nullptr, true, false);

				const auto ColorIt = MaterialTable.find(Material);
				int ColorCode = 16;
				if (ColorIt != MaterialTable.end())
					ColorCode = ColorIt->second;

				const auto TransformIt = TransformTable.find(BrickIt != BrickTable.end() ? BrickIt->second : LegoID.toStdString());
				if (TransformIt != TransformTable.end())
				{
					const lcVector4& AxisAngle = TransformIt->second.second;
					const lcVector3& TransformTranslation = TransformIt->second.first;

					lcMatrix44 Transform = lcMatrix44FromAxisAngle(lcVector3(AxisAngle[0], AxisAngle[1], AxisAngle[2]), -AxisAngle[3]);
					Transform.SetTranslation(lcMul30(-TransformTranslation, Transform));

					WorldMatrix = lcMul(Transform, WorldMatrix);
				}

				WorldMatrix = lcMatrix44(lcVector4(WorldMatrix[0][0], -WorldMatrix[0][1], -WorldMatrix[0][2], 0.0f), lcVector4(-WorldMatrix[1][0], WorldMatrix[1][1], WorldMatrix[1][2], 0.0f),
										 lcVector4(-WorldMatrix[2][0], WorldMatrix[2][1], WorldMatrix[2][2], 0.0f), lcVector4(WorldMatrix[3][0] * 25.0f, -WorldMatrix[3][1] * 25.0f, -WorldMatrix[3][2] * 25.0f, 1.0f));

				lcPiece* Piece = new lcPiece(nullptr);
				Piece->SetPieceInfo(Info, QString(), false);
				Piece->Initialize(lcMatrix44LDrawToLeoCAD(WorldMatrix), 1);
				Piece->SetColorCode(ColorCode);
				Pieces.Add(Piece);
				NumBrickPieces++;
			}
		
			if (NumBrickPieces > 1)
			{
				lcArray<lcPiece*> Group;
				for (int PieceIdx = Pieces.GetSize() - NumBrickPieces; PieceIdx < Pieces.GetSize(); PieceIdx++)
					Group.Add(Pieces[PieceIdx]);
				Groups.Add(Group);
			}
		}
	}

	return true;
}
