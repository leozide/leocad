#pragma once

#include "lc_assettypes.h"

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

	std::optional<lcPieceDataRef> GetPickedPart() const
	{
		return mAccepted ? std::optional<lcPieceDataRef>(mPickedPiece) : std::nullopt;
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
	lcPieceDataRef mPickedPiece;
	bool mAccepted = false;
	int mHeightLimit = QWIDGETSIZE_MAX;
};

std::optional<lcPieceDataRef> lcShowPartSelectionPopup(lcPieceData* InitialPart, const std::vector<std::pair<lcPieceData*, std::string>>& CustomParts, int ColorIndex, QWidget* Parent, QPoint Position);
