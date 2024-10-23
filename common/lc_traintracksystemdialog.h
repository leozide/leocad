#pragma once

#include "traintracksystem.h"
#include "lc_partselectionwidget.h"

class lcQColorPicker;

enum LC_TTS_EDITOR_STATES {
	LC_TTS_STATE_INSERT,
    LC_TTS_STATE_ROTATE,
    LC_TTS_STATE_NUMITEMS
};

namespace Ui
{
class lcTrainTrackSystemDialog;
}

class lcTrainTrackSystemDialog : public QDialog
{
	Q_OBJECT

public:
	explicit lcTrainTrackSystemDialog(QWidget* Parent);
	~lcTrainTrackSystemDialog();

	//MinifigWizard* mMinifigWizard;
	TrainTrackSystem* mTrainTrackSystem;

public slots:
	void mousePressOnPiece(lcPiece* piece);	
	void mouseRelease();

protected:
	
	Ui::lcTrainTrackSystemDialog* ui;

	//void mousePressEvent(QMouseEvent* Event) override;
	
	//bool eventFilter(QObject* Object, QEvent* Event) override;'	
	void keyPressEvent(QKeyEvent *event) override;


	enum LC_TTS_EDITOR_STATES editor_state; 

	lcView* mView;

	
private:

	lcPartSelectionListView* mPartsWidget;
	TrainTrackSection* selectedFromTrackSection = nullptr;
	int selectedFromConnectionNo = -1;
	bool newSectionInserted = false;
	enum LC_TTS_TYPES insertTrackType;
	int selectedToConnectionNo = 0;

	enum LC_TTS_TYPES selectedTrackTypeId;

	void SetInitialTrainTrackSystem();

};
