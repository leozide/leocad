#pragma once

#include "lc_model.h"
#include "lc_thumbnailmanager.h"

struct lcPropertiesDialogOptions
{
	lcModelProperties Properties;
	lcPartsList PartsList;
	lcBoundingBox BoundingBox;
};

namespace Ui
{
class lcPropertiesDialog;
}

class lcPartsTableWidget : public QTableWidget
{
public:
	explicit lcPartsTableWidget(QWidget* Parent = nullptr);
	void FreezePartColumns();
	void scrollTo(const QModelIndex& Index, ScrollHint Hint = EnsureVisible) override;

protected:
	bool eventFilter(QObject* Object, QEvent* Event) override;
	void focusInEvent(QFocusEvent* Event) override;
	void resizeEvent(QResizeEvent* Event) override;
	QModelIndex moveCursor(CursorAction Action, Qt::KeyboardModifiers Modifiers) override;
	QItemSelectionModel::SelectionFlags selectionCommand(const QModelIndex& Index, const QEvent* Event = nullptr) const override;
	void setSelection(const QRect& Rect, QItemSelectionModel::SelectionFlags Command) override;

private:
	friend class lcFrozenPartsView;
	QItemSelectionModel::SelectionFlags TrackSelectionCommand(const QModelIndex& Index, const QEvent* Event, QItemSelectionModel::SelectionFlags Command) const;
	bool SelectSharedRange(QItemSelectionModel::SelectionFlags Command);
	void UpdateFrozenGeometry();
	void EnsureCountVisible(const QModelIndex& Index);
	QTableView* mFrozenColumn = nullptr;
	int GetFrozenWidth() const;
	int mPartColumnWidth = 0;
	mutable QPersistentModelIndex mSelectionAnchor;
	mutable QPersistentModelIndex mSelectionEnd;
};

class lcPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	lcPropertiesDialog(QWidget* Parent, lcPropertiesDialogOptions* Options);
	~lcPropertiesDialog();

	lcPropertiesDialogOptions* mOptions;

public slots:
	void accept() override;

protected slots:
	void ThumbnailReady(lcPartThumbnailId ThumbnailId, QPixmap Pixmap);
	void ExportBrickLink();
	void ExportCSV();

private:
	QString GetExportFileName(const QString& Extension, const QString& Title, const QString& Filter);

	Ui::lcPropertiesDialog* ui;
	std::vector<std::pair<lcPartThumbnailId, QTableWidgetItem*>> mThumbnails;
};
