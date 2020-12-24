#pragma once

#include "view.h"

class lcPreview;

class lcPreviewDockWidget : public QMainWindow
{
	Q_OBJECT

public:
	explicit lcPreviewDockWidget(QMainWindow* Parent = nullptr);

	bool SetCurrentPiece(const QString& PartType, int ColorCode);
	void ClearPreview();
	void UpdatePreview();

protected slots:
	void SetPreviewLock();

protected:
	QAction* mLockAction;
	QToolBar* mToolBar;
	QLabel* mLabel;
	lcPreview* mPreview;
	lcViewWidget* mViewWidget;
};

class lcPreview : public View
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
	bool SetCurrentPiece(const QString& PartType, int ColorCode);

protected:
	std::unique_ptr<Project> mLoader;

	QString mDescription;
	bool mIsModel = false;
};
