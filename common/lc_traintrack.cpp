#include "lc_global.h"
#include "lc_traintrack.h"
#include "lc_library.h"
#include "pieceinf.h"
#include "piece.h"
#include "lc_application.h"

#include <iostream>
// todo:
// auto replace cross when going over a straight section
// redo gizmo
// add cross to gizmo
// rotate around connections shortcut
// shortcuts for changing active connection
// move config to json
// add other track types
// macros to encode/decode mTrackToolSection

void lcTrainTrackInit(lcPiecesLibrary* Library)
{
    QFile loadFile(":/resources/train_track_editor.json");

    if (!loadFile.open(QIODevice::ReadOnly))
        return;

    QByteArray trainTrackEditorConfig = loadFile.readAll();

    QJsonDocument loadDoc(QJsonDocument::fromJson(trainTrackEditorConfig));

    if (!loadDoc.object().contains("pieces") || !loadDoc["pieces"].isArray())
        return;

    if (!loadDoc.object().contains("releated_pieces_groups") || !loadDoc["releated_pieces_groups"].isArray())
        return;

    QJsonArray piecesGroupJson = loadDoc["releated_pieces_groups"].toArray();

    std::map<QString, lcRelatedPiecesGroup*> mapRelatedPiecesGroup;

    lcRelatedPiecesGroup* trainTrackGroupEmpty = new lcRelatedPiecesGroup();

    for (int groupIdx = 0; groupIdx < piecesGroupJson.size(); ++groupIdx) {

        QJsonObject groupObject = piecesGroupJson[groupIdx].toObject();

        QString groupName;
        QString pieceName;

        if (groupObject.contains("name") || groupObject["name"].isString()) {

            groupName = groupObject["name"].toString();

            lcRelatedPiecesGroup* trainTrackGroup = new lcRelatedPiecesGroup();

            mapRelatedPiecesGroup.insert(std::pair<QString, lcRelatedPiecesGroup*>(groupName, trainTrackGroup));

            if (groupObject.contains("filenames") || groupObject["filenames"].isArray()) 
            {
                QJsonArray filenamesObject = groupObject["filenames"].toArray();

                for (int pieceIdx = 0; pieceIdx < filenamesObject.size(); ++pieceIdx) 
                {
                    if(filenamesObject[pieceIdx].isString()) 
                    {
                        PieceInfo* Info = Library->FindPiece(filenamesObject[pieceIdx].toString().toStdString().c_str(), nullptr, false, false);
                        trainTrackGroup->AddRelatedPieceName(Info);
                    }
                }
            }
        }
    }

    QJsonArray tracksectionsJson = loadDoc["pieces"].toArray();

    std::map<QString, lcRelatedPiecesGroup*>::iterator mapRelatedPiecesGroupItr;

    for (int sectionIdx = 0; sectionIdx < tracksectionsJson.size(); ++sectionIdx) {
        QJsonObject sectionObject = tracksectionsJson[sectionIdx].toObject();

        if (sectionObject.contains("filename") || sectionObject["filename"].isString()) 
        {

            QString mFilename = sectionObject["filename"].toString();

            PieceInfo* Info = Library->FindPiece(mFilename.toStdString().c_str(), nullptr, false, false);

            if (Info)
            {
                if (sectionObject.contains("connections") || sectionObject["connections"].isArray()) 
                {
                    QJsonArray connectionsJson = sectionObject["connections"].toArray();

                    lcTrainTrackInfo* TrainTrackInfo = new lcTrainTrackInfo(Info);

                    for (int connIdx = 0; connIdx < connectionsJson.size(); ++connIdx) 
                    {
                            QJsonObject connObject = connectionsJson[connIdx].toObject();

                            double transX = 0, transY = 0, transZ = 0, rotateZ = 0;
                            QString connectGroup;

                            if (!sectionObject.contains("transform_x") || !sectionObject["transform_x"].isDouble())
                                transX = connObject["transform_x"].toDouble();

                            if (!sectionObject.contains("transform_y") || !sectionObject["transform_y"].isDouble())
                                transY = connObject["transform_y"].toDouble();

                            if (!sectionObject.contains("transform_z") || !sectionObject["transform_z"].isDouble())
                                transZ = connObject["transform_z"].toDouble();

                            if (!sectionObject.contains("rotate_z") || !sectionObject["rotate_z"].isDouble())
                                rotateZ = connObject["rotate_z"].toDouble();

                            if (!sectionObject.contains("connect_group") || !sectionObject["connect_group"].isString())
                                connectGroup = connObject["connect_group"].toString();

                            lcRelatedPiecesGroup* relatedPieces = nullptr;
                            mapRelatedPiecesGroupItr = mapRelatedPiecesGroup.find(connectGroup);

                            if(mapRelatedPiecesGroupItr != mapRelatedPiecesGroup.end()) 
                            {
                                relatedPieces = mapRelatedPiecesGroupItr->second;
                                TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(rotateZ * LC_DTOR), lcVector3(transX, transY, transZ)),relatedPieces});
                            } else {
                                TrainTrackInfo->AddConnection({lcMatrix44(lcMatrix33RotationZ(rotateZ * LC_DTOR), lcVector3(transX, transY, transZ)),trainTrackGroupEmpty});
                            }
                    }
                    Info->SetTrainTrackInfo(TrainTrackInfo);
                }
            }
        }
    }
}

std::pair<PieceInfo*, lcMatrix44> lcTrainTrackInfo::GetPieceInsertTransform(lcPiece* Piece, quint32 ConnectionIndex, int relatedPieceIdx) const
{
	if (ConnectionIndex >= mConnections.size())
		return { nullptr, lcMatrix44Identity() };

    PieceInfo* Info = mConnections[ConnectionIndex].relatedPiecesGroup->GetIndex(relatedPieceIdx);

	if (!Info)
		return { nullptr, lcMatrix44Identity() };

	lcTrainTrackInfo* TrainTrackInfo = Info->GetTrainTrackInfo();

	if (!TrainTrackInfo || TrainTrackInfo->mConnections.empty())
		return { nullptr, lcMatrix44Identity() };

	lcMatrix44 Transform;

    Transform = lcMatrix44AffineInverse(TrainTrackInfo->mConnections[0].Transform);
	Transform = lcMul(Transform, lcMatrix44RotationZ(LC_PI));

	Transform = lcMul(Transform, mConnections[ConnectionIndex].Transform);
	Transform = lcMul(Transform, Piece->mModelWorld);

	return { Info, Transform };
}

int lcTrainTrackInfo::GetPieceConnectionIndex(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2)
{
	const lcTrainTrackInfo* TrainTrackInfo1 = Piece1->mPieceInfo->GetTrainTrackInfo();
	const lcTrainTrackInfo* TrainTrackInfo2 = Piece2->mPieceInfo->GetTrainTrackInfo();

	const std::vector<lcTrainTrackConnection>& Connections2 = TrainTrackInfo2->GetConnections();
	lcMatrix44 Transform1 = lcMul(TrainTrackInfo1->GetConnections()[ConnectionIndex1].Transform, Piece1->mModelWorld);

	for (int ConnectionIndex2 = 0; ConnectionIndex2 < static_cast<int>(Connections2.size()); ConnectionIndex2++)
	{
		const lcTrainTrackConnection& Connection2 = Connections2[ConnectionIndex2];
		const lcMatrix44 Transform2 = lcMul(Connection2.Transform, Piece2->mModelWorld);

		if (lcLengthSquared(Transform1.GetTranslation() - Transform2.GetTranslation()) > 0.1f)
			continue;

		float Dot = lcDot3(Transform1[0], Transform2[0]);

		if (Dot < -0.99f && Dot > -1.01f)
			return ConnectionIndex2;
	}

	return -1;
}

std::optional<lcMatrix44> lcTrainTrackInfo::GetPieceInsertTransform(lcPiece* CurrentPiece, PieceInfo* Info)
{
	const lcTrainTrackInfo* CurrentTrackInfo = CurrentPiece->mPieceInfo->GetTrainTrackInfo();

	if (!CurrentTrackInfo || CurrentTrackInfo->GetConnections().empty())
		return std::nullopt;

	const quint32 FocusSection = CurrentPiece->GetFocusSection();
	quint32 ConnectionIndex = 0;

	if (FocusSection == LC_PIECE_SECTION_POSITION || FocusSection == LC_PIECE_SECTION_INVALID)
	{
		for (ConnectionIndex = 0; ConnectionIndex < CurrentTrackInfo->GetConnections().size(); ConnectionIndex++)
			if (!CurrentPiece->IsTrainTrackConnected(ConnectionIndex))
				break;
	}
	else
	{
		if (FocusSection < LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST)
			return std::nullopt;

		ConnectionIndex = FocusSection - LC_PIECE_SECTION_TRAIN_TRACK_CONNECTION_FIRST;
	}

	return GetConnectionTransform(CurrentPiece, ConnectionIndex, Info, 0);
}

std::optional<lcMatrix44> lcTrainTrackInfo::GetConnectionTransform(lcPiece* CurrentPiece, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex)
{
	if (!CurrentPiece || !Info)
		return std::nullopt;

	const lcTrainTrackInfo* CurrentTrackInfo = CurrentPiece->mPieceInfo->GetTrainTrackInfo();

	if (!CurrentTrackInfo || CurrentTrackInfo->GetConnections().empty())
		return std::nullopt;

	if (CurrentConnectionIndex >= CurrentTrackInfo->GetConnections().size())
		return std::nullopt;

	lcTrainTrackInfo* NewTrackInfo = Info->GetTrainTrackInfo();

	if (!NewTrackInfo || NewConnectionIndex >= NewTrackInfo->mConnections.size())
		return std::nullopt;

	lcMatrix44 Transform;

    Transform = lcMatrix44AffineInverse(NewTrackInfo->mConnections[NewConnectionIndex].Transform);
    Transform = lcMul(Transform, lcMatrix44RotationZ(LC_PI));

	Transform = lcMul(Transform, CurrentTrackInfo->GetConnections()[CurrentConnectionIndex].Transform);
	Transform = lcMul(Transform, CurrentPiece->mModelWorld);

	return Transform;
}
