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

	std::pair<PieceInfo*, lcMatrix44> GetPieceInsertTransform(lcPiece* Piece, quint32 ConnectionIndex, lcTrainTrackType TrainTrackType) const;
	static std::optional<lcMatrix44> GetPieceInsertTransform(lcPiece* CurrentPiece, PieceInfo* Info);
	static std::optional<lcMatrix44> GetConnectionTransform(lcPiece* CurrentPiece, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex);
	static std::optional<lcMatrix44> CalculateTransformToConnection(const lcMatrix44& ConnectionTransform, PieceInfo* Info, quint32 ConnectionIndex);
	static int GetPieceConnectionIndex(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2);

	static quint32 EncodeTrackToolSection(quint32 ConnectionIndex, lcTrainTrackType TrainTrackType)
	{
		return ConnectionIndex | (static_cast<quint32>(TrainTrackType) << 8);
	}

	static std::pair<quint32, lcTrainTrackType> DecodeTrackToolSection(quint32 TrackToolSection)
	{
		quint32 ConnectionIndex = TrackToolSection & 0xff;
		lcTrainTrackType TrainTrackType = static_cast<lcTrainTrackType>((TrackToolSection >> 8) & 0xff);

		return { ConnectionIndex, TrainTrackType };
	}

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
