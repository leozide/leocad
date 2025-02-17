#pragma once

#include "lc_math.h"

class lcPiece;
class lcPiecesLibrary;

struct lcTrainTrackConnectionType
{
	quint32 Group;
	int Format;
};

struct lcTrainTrackConnection
{
	lcMatrix44 Transform;
	lcTrainTrackConnectionType Type;
};

class lcTrainTrackInfo
{
public:
	lcTrainTrackInfo() = default;

	static std::optional<lcMatrix44> GetPieceInsertTransform(lcPiece* CurrentPiece, PieceInfo* Info, quint32 Section);
	static std::optional<lcMatrix44> GetConnectionTransform(lcPiece* CurrentPiece, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex);
	static std::optional<lcMatrix44> CalculateTransformToConnection(const lcMatrix44& ConnectionTransform, PieceInfo* Info, quint32 ConnectionIndex);
	static int GetPieceConnectionIndex(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2);

	void AddConnection(const lcMatrix44 &Transform, const lcTrainTrackConnectionType& Type)
	{
		mConnections.emplace_back(lcTrainTrackConnection{Transform, Type});
	}

	const std::vector<lcTrainTrackConnection>& GetConnections() const
	{
		return mConnections;
	}

	bool CanConnectTo(const lcTrainTrackConnectionType& ConnectionType) const
	{
		for (const lcTrainTrackConnection& Connection : mConnections)
			if (AreConnectionsCompatible(Connection.Type, ConnectionType))
				return true;

		return false;
	}

protected:
	static bool AreConnectionsCompatible(const lcTrainTrackConnectionType& a, const lcTrainTrackConnectionType& b)
	{
		return a.Group == b.Group && a.Format + b.Format == 0;
	}

	std::vector<lcTrainTrackConnection> mConnections;
};

void lcTrainTrackInit(lcPiecesLibrary* Library);
