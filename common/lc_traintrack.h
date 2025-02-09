#pragma once

#include "lc_math.h"

class lcPiece;
class lcPiecesLibrary;

struct lcTrainTrackConnection
{
	lcMatrix44 Transform;
	quint32 Type;
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

	static std::optional<lcMatrix44> GetPieceInsertTransform(lcPiece* CurrentPiece, PieceInfo* Info, quint32 Section);
	static std::optional<lcMatrix44> GetConnectionTransform(lcPiece* CurrentPiece, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex);
	static std::optional<lcMatrix44> CalculateTransformToConnection(const lcMatrix44& ConnectionTransform, PieceInfo* Info, quint32 ConnectionIndex);
	static int GetPieceConnectionIndex(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2);

	void AddConnection(const lcMatrix44 &Transform, quint32 Type)
	{
		mConnections.emplace_back(lcTrainTrackConnection{Transform, Type});
	}

	const std::vector<lcTrainTrackConnection>& GetConnections() const
	{
		return mConnections;
	}

	bool CanConnectTo(quint32 ConnectionType) const
	{
		for (const lcTrainTrackConnection& Connection : mConnections)
			if (Connection.Type == ConnectionType)
				return true;

		return false;
	}

protected:
	std::vector<lcTrainTrackConnection> mConnections;
};

void lcTrainTrackInit(lcPiecesLibrary* Library);
