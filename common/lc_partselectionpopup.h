#pragma once

class PieceInfo;
class lcPartSelectionWidget;
class lcPartSelectionListView;

class lcPartSelectionPopup : public QWidget
{
	Q_OBJECT

public:
	lcPartSelectionPopup(PieceInfo* InitialPart, QWidget* Parent);
	virtual ~lcPartSelectionPopup() = default;
	QSize sizeHint() const override;
	void SetHeightLimit(int Height);

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
	void ResizeToContents();

protected:
	void showEvent(QShowEvent* ShowEvent) override;
	void Close();
	void UpdateHeightLimit();
	QSize GetPartsViewSizeHint(const lcPartSelectionListView* PartsView, const QSize& PopupSize) const;

	lcPartSelectionWidget* mPartSelectionWidget = nullptr;
	PieceInfo* mInitialPart = nullptr;
	PieceInfo* mPickedPiece = nullptr;
	bool mAccepted = false;
	int mHeightLimit = QWIDGETSIZE_MAX;
};

std::optional<PieceInfo*> lcShowPartSelectionPopup(PieceInfo* InitialPart, const std::vector<std::pair<PieceInfo*, std::string>>& CustomParts, int ColorIndex, QWidget* Parent, QPoint Position);
