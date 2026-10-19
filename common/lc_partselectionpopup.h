#pragma once

class lcPieceData;
class lcPartSelectionWidget;
class lcPartSelectionListView;

class lcPartSelectionPopup : public QWidget
{
	Q_OBJECT

public:
	lcPartSelectionPopup(lcPieceData* InitialPart, QWidget* Parent);
	virtual ~lcPartSelectionPopup() = default;
	QSize sizeHint() const override;
	void SetHeightLimit(int Height);

	std::optional<lcPieceData*> GetPickedPart() const
	{
		return mAccepted ? std::optional<lcPieceData*>(mPickedPiece) : std::nullopt;
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
	lcPieceData* mInitialPart = nullptr;
	lcPieceData* mPickedPiece = nullptr;
	bool mAccepted = false;
	int mHeightLimit = QWIDGETSIZE_MAX;
};

std::optional<lcPieceData*> lcShowPartSelectionPopup(lcPieceData* InitialPart, const std::vector<std::pair<lcPieceData*, std::string>>& CustomParts, int ColorIndex, QWidget* Parent, QPoint Position);
