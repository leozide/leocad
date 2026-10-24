#pragma once

#include "lc_view.h"

class lcPreview;

class lcPreviewDockWidget : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcPreviewDockWidget(QMainWindow* Parent);

	bool SetCurrentPiece(lcPieceData* PieceData, int ColorCode);
	void ClearPreview();
	void UpdatePreview();
	void RefreshDescription();
	void RefreshModel(const lcModel* Model);
	void RefreshAfterAssetChange(const std::vector<lcPieceData*>& ChangedPieces);
	void RebindPieceData(lcPieceData* Previous, lcPieceData* Replacement);

protected slots:
	void SetPreviewLock();
	void OptionsMenuAboutToShow();

protected:
	void RefreshPendingAssets();

	QAction* mLockAction;
	QToolBar* mToolBar;
	QLabel* mLabel;
	lcPreview* mPreview;
	lcViewWidget* mViewWidget;
	bool mRefreshingModel = false;
	bool mAssetRefreshPending = false;
	bool mRefitAfterAssetLoad = true;
};

class lcPreview : public lcView
{
public:
	lcPreview();

	QString GetDescription() const
	{
		return mDescription;
	}

	bool IsModel() const
	{
		return mIsModel;
	}

	void ClearPreview();
	void UpdatePreview();
	void RefreshDescription();
	bool SetCurrentPiece(lcPieceData* PieceData, int ColorCode);

protected:
	std::unique_ptr<Project> mLoader;

	QString mDescription;
	bool mIsModel = false;
};
