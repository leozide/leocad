#pragma once

#include "lc_math.h"

class lcPiece;
class lcPiecesLibrary;

struct lcTrainTrackConnection
{
	lcMatrix44 Transform;
};

enum class lcTrainTrackType
{
	Straight,
	Left,
	Right,
	BranchLeft,
	BranchRight,
	Count
};

class lcTrainTrackInfo
{
public:
	lcTrainTrackInfo() = default;

	std::pair<PieceInfo*, lcMatrix44> GetPieceInsertPosition(lcPiece* Piece, quint32 ConnectionIndex, lcTrainTrackType TrainTrackType) const;
	std::optional<lcMatrix44> GetPieceInsertPosition(lcPiece* Piece, PieceInfo* Info) const;

	void AddConnection(const lcTrainTrackConnection& TrainTrackConnection)
	{
		mConnections.emplace_back(TrainTrackConnection);
	}

	const std::vector<lcTrainTrackConnection>& GetConnections() const
	{
		return mConnections;
	}

protected:
	std::vector<lcTrainTrackConnection> mConnections;
};

void lcTrainTrackInit(lcPiecesLibrary* Library);
