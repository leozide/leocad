#pragma once

#include "lc_math.h"

class lcPiece;
class lcPiecesLibrary;
class lcTrainTrackInfo;
class lcRelatedPiecesGroup;

struct lcTrainTrackConnection
{
	lcMatrix44 Transform;
    lcRelatedPiecesGroup *relatedPiecesGroup;
};


class lcRelatedPiecesGroup
{
public:

    void AddRelatedPieceName(PieceInfo* info)
    {
        relatedPieces.push_back(info);
    }

    PieceInfo* GetIndex(int index)
    {
        return relatedPieces[index];
    }

    int Size()
    {
        return relatedPieces.size();
    }

protected:
    std::vector<PieceInfo*> relatedPieces;
};


class lcTrainTrackInfo
{
public:
	lcTrainTrackInfo() = default;

    lcTrainTrackInfo(PieceInfo* pieceInfo)
    {
        mPieceInfo = pieceInfo;
    }

	std::pair<PieceInfo*, lcMatrix44> GetPieceInsertTransform(lcPiece* Piece, quint32 ConnectionIndex, int relatedPieceIdx) const;
	static std::optional<lcMatrix44> GetPieceInsertTransform(lcPiece* CurrentPiece, PieceInfo* Info);
	static std::optional<lcMatrix44> GetConnectionTransform(lcPiece* CurrentPiece, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex);
	static int GetPieceConnectionIndex(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2);

	void AddConnection(const lcTrainTrackConnection& TrainTrackConnection)
	{
		mConnections.emplace_back(TrainTrackConnection);
	}

	const std::vector<lcTrainTrackConnection>& GetConnections() const
	{
		return mConnections;
	}

    PieceInfo* GetPieceInfo() {
        return mPieceInfo;
    }

protected:

    PieceInfo* mPieceInfo = nullptr;
	std::vector<lcTrainTrackConnection> mConnections;
};

void lcTrainTrackInit(lcPiecesLibrary* Library);
