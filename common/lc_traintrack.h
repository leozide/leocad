#pragma once

#include "lc_math.h"

class lcPiece;
class lcPiecesLibrary;
class PieceInfo;
struct lcInsertPieceInfo;

enum class lcTrainTrackConnectionSleeper
{
	None,
	HasSleeper,
	NeedsSleeper
};

struct lcTrainTrackConnectionType
{
	size_t Group;
	lcTrainTrackConnectionSleeper Sleeper;
};

struct lcTrainTrackConnection
{
	lcMatrix44 Transform;
	lcTrainTrackConnectionType Type;
};

class lcTrainTrackInfo
{
public:
	lcTrainTrackInfo(const std::vector<lcTrainTrackConnection> &Connections, int Color, bool Visible)
		: mConnections(Connections), mColorCode(Color), mVisible(Visible)
	{
	}

	int GetColorCode() const
	{
		return mColorCode;
	}

	bool IsVisible() const
	{
		return mVisible;
	}

	static void Initialize(lcPiecesLibrary* Library);
	static std::vector<lcInsertPieceInfo> GetInsertPieceInfo(lcPiece* CurrentPiece, PieceInfo* Info, lcPiece* MovingPiece, int ColorIndex, quint32 PreferredSection, bool AllowNewPieces, std::optional<lcVector3> ClosestPoint);
	static std::optional<lcMatrix44> GetConnectionTransform(PieceInfo* CurrentInfo, const lcMatrix44& CurrentTransform, quint32 CurrentConnectionIndex, PieceInfo* Info, quint32 NewConnectionIndex);
	static std::optional<lcMatrix44> CalculateTransformToConnection(const lcMatrix44& ConnectionTransform, PieceInfo* Info, quint32 ConnectionIndex);
	static int GetPieceConnectionIndex(const lcPiece* Piece1, int ConnectionIndex1, const lcPiece* Piece2);

	const std::vector<lcTrainTrackConnection>& GetConnections() const
	{
		return mConnections;
	}

	bool CanConnectTo(const lcTrainTrackConnectionType& ConnectionType, bool AllowNewPieces) const
	{
		for (const lcTrainTrackConnection& Connection : mConnections)
			if (AreConnectionsCompatible(Connection.Type, ConnectionType, AllowNewPieces))
				return true;

		return false;
	}

protected:
	static bool AreConnectionsCompatible(const lcTrainTrackConnectionType& a, const lcTrainTrackConnectionType& b, bool AllowNewPieces)
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
			return b.Sleeper == lcTrainTrackConnectionSleeper::HasSleeper || (AllowNewPieces && b.Sleeper == lcTrainTrackConnectionSleeper::NeedsSleeper);
		}

		return false;
	}

	std::vector<lcTrainTrackConnection> mConnections;
	int mColorCode = 16;
	bool mVisible = false;

	static std::map<size_t, PieceInfo*> mSleepers;
};
