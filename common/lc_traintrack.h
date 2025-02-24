#pragma once

#include "lc_math.h"

class lcPiece;
class lcPiecesLibrary;
class PieceInfo;

enum class lcTrainTrackConnectionSleeper
{
	None,
	HasSleeper,
	NeedsSleeper
};

struct lcTrainTrackConnectionType
{
	quint32 Group;
	lcTrainTrackConnectionSleeper Sleeper;
};

struct lcTrainTrackConnection
{
	lcMatrix44 Transform;
	lcTrainTrackConnectionType Type;
};

struct lcTrainTrackInsert
{
	PieceInfo* Info;
	lcMatrix44 Transform;
	int ColorCode;
};

class lcTrainTrackInfo
{
public:
	lcTrainTrackInfo() = default;

	static void Initialize(lcPiecesLibrary* Library);
	static std::vector<lcTrainTrackInsert> GetPieceInsertTransforms(lcPiece* CurrentPiece, PieceInfo* Info, quint32 Section);
	static std::optional<lcMatrix44> GetConnectionTransform(PieceInfo* CurrentInfo, const lcMatrix44& CurrentTransform, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex);
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
		if (a.Group != b.Group)
			return false;

		switch (a.Sleeper)
		{
		case lcTrainTrackConnectionSleeper::None:
			return b.Sleeper == lcTrainTrackConnectionSleeper::None;

		case lcTrainTrackConnectionSleeper::HasSleeper:
			return b.Sleeper == lcTrainTrackConnectionSleeper::NeedsSleeper;

		case lcTrainTrackConnectionSleeper::NeedsSleeper:
			return b.Sleeper == lcTrainTrackConnectionSleeper::HasSleeper || b.Sleeper == lcTrainTrackConnectionSleeper::NeedsSleeper;
		}

		return false;
	}

	std::vector<lcTrainTrackConnection> mConnections;
	static std::map<quint32, PieceInfo*> mSleepers;
};
