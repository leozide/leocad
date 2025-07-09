#pragma once

class PieceInfo;
class lcPartSelectionWidget;

class lcPartSelectionPopup : public QWidget
{
	Q_OBJECT

public:
	lcPartSelectionPopup(PieceInfo* InitialPart, QWidget* Parent);
	virtual ~lcPartSelectionPopup() = default;

	std::optional<PieceInfo*> GetPickedPart() const
	{
		return mAccepted ? std::optional<PieceInfo*>(mPickedPiece) : std::nullopt;
	}

	lcPartSelectionWidget* GetPartSelectionWidget() const
	{
		return mPartSelectionWidget;
	}

protected slots:
	void Accept();
	void Reject();

protected:
	void showEvent(QShowEvent* ShowEvent) override;
	void Close();

	lcPartSelectionWidget* mPartSelectionWidget = nullptr;
	PieceInfo* mInitialPart = nullptr;
	PieceInfo* mPickedPiece = nullptr;
	bool mAccepted = false;
};

std::optional<PieceInfo*> lcShowPartSelectionPopup(PieceInfo* InitialPart, const std::vector<std::pair<PieceInfo*, std::string>>& CustomParts, int ColorIndex, QWidget* Parent, QPoint Position);
