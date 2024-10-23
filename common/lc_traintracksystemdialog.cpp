#include "lc_traintracksystemdialog.h"
#include "ui_lc_traintracksystemdialog.h"    		        
#include "lc_viewwidget.h"
#include "lc_mainwindow.h"
#include "pieceinf.h"
#include "lc_view.h"
#include "piece.h"
#include "camera.h"
#include "lc_partselectionwidget.h"

lcTrainTrackSystemDialog::lcTrainTrackSystemDialog(QWidget* Parent)
	: QDialog(Parent), ui(new Ui::lcTrainTrackSystemDialog)
{

	gMainWindow->SetTool(lcTool::Pan);

	editor_state = LC_TTS_STATE_INSERT;
    
	ui->setupUi(this);

	QGridLayout* PreviewLayout = new QGridLayout(ui->trainTrackSystemFrame);
	PreviewLayout->setContentsMargins(0, 0, 0, 0);

	mTrainTrackSystem = new TrainTrackSystem();
	SetInitialTrainTrackSystem();
	mView = new lcView(lcViewType::View, mTrainTrackSystem->GetModel());

	lcViewWidget* ViewWidget = new lcViewWidget(nullptr, mView);
	ViewWidget->setMinimumWidth(1600);

	connect(ViewWidget, &lcViewWidget::mousePressOnPiece, this, &lcTrainTrackSystemDialog::mousePressOnPiece);	
	connect(ViewWidget, &lcViewWidget::mouseRelease, this, &lcTrainTrackSystemDialog::mouseRelease);

	PreviewLayout->addWidget(ViewWidget);

	mView->MakeCurrent();

	QGridLayout* PartSelector = new QGridLayout(ui->partSelector);
	PartSelector->setContentsMargins(0, 0, 0, 0);

	mPartsWidget = new lcPartSelectionListView(nullptr, nullptr);
	PartSelector->addWidget(mPartsWidget);
	
	mPartsWidget->SetParts(mTrainTrackSystem->getTrackTypes());

	mView->GetCamera()->SetViewpoint(lcVector3(0.0f, 0.0f, 90.0f));
	mView->ZoomExtents();
}


lcTrainTrackSystemDialog::~lcTrainTrackSystemDialog()
{
	delete mTrainTrackSystem;
	delete ui;
}


void lcTrainTrackSystemDialog::mousePressOnPiece(lcPiece* pieceClickedOn) {
	
	if(pieceClickedOn != nullptr) {
		
		if(mTrainTrackSystem->findTrackTypeByString(pieceClickedOn->mPieceInfo->mFileName) == -1) {
			
			PieceInfo* piecePartlistInf = mPartsWidget->GetCurrentPart();

			insertTrackType = mTrainTrackSystem->findTrackTypeByString(piecePartlistInf->mFileName);

			if(selectedFromTrackSection == nullptr) {
				
                std::vector<TrainTrackSection*> trackSections = mTrainTrackSystem->GetTrackSections();
				int found_con_no = 0;		
				
				for(TrainTrackSection* trackSection : trackSections) {									
					found_con_no = trackSection->FindConnectionNumber(pieceClickedOn->GetRotationCenter());
					if(found_con_no != -1) {
						selectedFromTrackSection = trackSection;
						selectedFromConnectionNo = found_con_no;
						break;
					}
				}
			}		
			if(selectedFromTrackSection != nullptr) {

				selectedToConnectionNo = 0;
				newSectionInserted = true;
				mTrainTrackSystem->addTrackSection(selectedFromTrackSection, selectedFromConnectionNo, insertTrackType, selectedToConnectionNo);
				mTrainTrackSystem->SetModel();
				mView->Redraw();
			}
		}
		else {

			if(mTrainTrackSystem->deleteTrackSection(pieceClickedOn) == true) {
				mTrainTrackSystem->SetModel();
				mView->Redraw();
			}
		}
	}
}

void lcTrainTrackSystemDialog::mouseRelease() {
	selectedFromTrackSection = nullptr;
	newSectionInserted = false;
}

void lcTrainTrackSystemDialog::keyPressEvent(QKeyEvent *event) {

	if(selectedFromTrackSection == nullptr) {

		lcModel* activeModel = mView->GetActiveModel();
        const std::vector<std::unique_ptr<lcPiece>>& Pieces = activeModel->GetPieces();
		for (const std::unique_ptr<lcPiece>& Piece : Pieces)	{
			if (Piece->IsFocused()) {
				
                std::vector<TrainTrackSection*> trackSections = mTrainTrackSystem->GetTrackSections();
                int found_con_no = 0;		
                
                for(TrainTrackSection* trackSection : trackSections) {									
                    found_con_no = trackSection->FindConnectionNumber(Piece->GetRotationCenter());
                    if(found_con_no != -1) {
                        selectedFromTrackSection = trackSection;
                        selectedFromConnectionNo = found_con_no;
                        break;
                    }
                }
				break;
			}
		}
	}
		
	if(selectedFromTrackSection != nullptr) {
		bool updateSection = true;
		switch(event->key()) {
			case Qt::Key_S: {
				insertTrackType = LC_TTS_STRAIGHT;
				selectedToConnectionNo = 0;
				break;
			}
			case Qt::Key_C: {
				insertTrackType = LC_TTS_CURVED;
				selectedToConnectionNo = 0;
				break;
			}
			case Qt::Key_X: {
				insertTrackType = LC_TTS_CROSS;
				selectedToConnectionNo = 0;
				break;
			}
			case Qt::Key_L: {
				insertTrackType = LC_TTS_LEFT_BRANCH;
				selectedToConnectionNo = 0;
				break;
			}
			case Qt::Key_R: {
				insertTrackType = LC_TTS_RIGHT_BRANCH;
				selectedToConnectionNo = 0;
				break;
			}
			case Qt::Key_Right: {
				if(newSectionInserted == true) {
					std::vector<TrainTrackSection*> trackSections = mTrainTrackSystem->GetTrackSections();
					TrainTrackSection* newTrackSection = trackSections[trackSections.size() - 1];
					
					selectedToConnectionNo++;
					if(selectedToConnectionNo >= newTrackSection->GetNoOfConnections())
						selectedToConnectionNo = 0;										
				}
				break;
			}
			case Qt::Key_Insert: {				
				selectedFromTrackSection = nullptr;
				selectedFromConnectionNo = -1;
				newSectionInserted = false;
				insertTrackType = LC_TTS_STRAIGHT;
				selectedToConnectionNo = 0;
				updateSection = false;	
				break;		
			}
			default: {
				updateSection = false;
			}
		}			

		if(updateSection == true) {

			if(newSectionInserted == true) {
				std::vector<TrainTrackSection*>* trackSections = mTrainTrackSystem->GetTrackSectionsPtr();                
                trackSections->erase(trackSections->begin() + trackSections->size() - 1);
			}
			mTrainTrackSystem->addTrackSection(selectedFromTrackSection, selectedFromConnectionNo, insertTrackType, selectedToConnectionNo);
			mTrainTrackSystem->SetModel();
			mView->Redraw();
			newSectionInserted = true;
		}
	}
}

void lcTrainTrackSystemDialog::SetInitialTrainTrackSystem() 
{
	int colorIndex = 8;
	int mCurrentStep = 1;
	mTrainTrackSystem->setColorIndex(colorIndex);
	mTrainTrackSystem->setCurrentStep(mCurrentStep);
	mTrainTrackSystem->addFirstTrackSection(lcMatrix44Identity(),LC_TTS_STRAIGHT);   //Idx: 0
}



















