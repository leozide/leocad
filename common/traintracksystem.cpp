#include "piece.h"
#include "traintracksystem.h"
#include "lc_model.h"
#include "lc_application.h"
#include "lc_library.h"
#include <QString>

TrainTrackConnectionPoint::TrainTrackConnectionPoint(float angle, float x, float y) {
	this->angle = angle;
	xy_coordinate.x = x;
	xy_coordinate.y = y;
}



void TrainTrackType::Load(lcPiecesLibrary* Library, char const* filename) {
	pieceInfo = Library->FindPiece(filename, nullptr, false, false);
}

void TrainTrackType::AddConnection(TrainTrackConnectionPoint *connectionPoint) {
    connectionPoints.push_back(connectionPoint);
}

TrainTrackConnectionPoint* TrainTrackType::GetConnection(int index) {
	return connectionPoints[index];	
}

std::vector<TrainTrackConnectionPoint *>& TrainTrackType::GetConnections() {
	return connectionPoints;	
}

int TrainTrackType::GetNoOfConnections() {
	return connectionPoints.size();
}



TrainTrackSection::TrainTrackSection(TrainTrackType *trackType, enum LC_TTS_TYPES trackTypeId, lcMatrix44 location, int mCurrentStep, int mColorIndex) {
	this->trackType = trackType;
	this->trackTypeId = trackTypeId;
	this->location = location;
	this->mCurrentStep = mCurrentStep;
	this->mColorIndex = mColorIndex;
}


lcMatrix44 TrainTrackSection::GetConnectionLocation(int connectionNo) {
	
	lcMatrix44 mat44;
	lcMatrix44 matOffset;

	TrainTrackConnectionPoint* connection = trackType->GetConnections()[connectionNo];
	
	mat44 = lcMatrix44Identity();
	mat44 =	lcMatrix44RotationZ(LC_DTOR * connection->angle);

	matOffset = lcMatrix44Identity();
	matOffset.SetTranslation(lcVector3(connection->xy_coordinate.x, connection->xy_coordinate.y,0)); 

	matOffset = lcMul(mat44, matOffset);

	return lcMul(matOffset,location);
}

int TrainTrackSection::GetNoOfConnections() {
	return trackType->GetNoOfConnections();	
}

int TrainTrackSection::FindConnectionNumber(lcVector3 v3searchLocation) {
	float precisionErrorNumber = 0.001;

	lcMatrix44 matOffset;

	for(int con_no = 0; con_no < trackType->GetNoOfConnections(); con_no++) {

		lcMatrix44 matSelection = GetConnectionLocation(con_no);
		
		matOffset = lcMatrix44Identity();
		matOffset.SetTranslation(lcVector3(-10, 0, 8)); 

		lcVector3 v3Selection = lcMul(matOffset,matSelection).GetTranslation();

		if(abs(v3Selection.x - v3searchLocation.x) < precisionErrorNumber &&
			abs(v3Selection.y - v3searchLocation.y) < precisionErrorNumber &&
			abs(v3Selection.z - v3searchLocation.z) < precisionErrorNumber) 
		{
			return con_no;
		}
	}
	return -1;
}



const char* TrainTrackSystem::mTrackTypeFilenames[LC_TTS_NUMITEMS] =
{
	"74746.dat", 	// LC_TTS_STRAIGHT,
	"74747.dat", 	// LC_TTS_CURVED,
	"32087.dat", 	// LC_TTS_CROSS,
	"2861c04.dat", 	// LC_TTS_LEFT_BRANCH,
	"2859c04.dat" 	// LC_TTS_RIGHT_BRANCH	
};


TrainTrackSystem::TrainTrackSystem()
	: mModel(new lcModel(QString(), nullptr, false))
{
	lcPiecesLibrary* Library = lcGetPiecesLibrary();

	float xHalfBendOff = sin(LC_DTOR * 11.25) * 800;
	float yHalfBendOff = ((cos(LC_DTOR * 11.25) * 800) - 800);

	float x_left_branch = 320 + 320 + 0 + (-(sin(LC_DTOR * 22.5)*800));
	float y_left_branch = 0 + 0 + 320 + ((cos(LC_DTOR * 22.5)*800)-800);

	float x_right_branch = 320 + 320 + 0 + (-(sin(LC_DTOR * 22.5)*800));
	float y_right_branch = 0 + 0 - 320 - ((cos(LC_DTOR * 22.5)*800)-800);

	TrainTrackType *trackType;

	trainTrackTypes.reserve(LC_TTS_NUMITEMS);

	// straight
	trackType = new TrainTrackType();
	trackType->Load(Library,mTrackTypeFilenames[LC_TTS_STRAIGHT]);
	trackType->AddConnection(new TrainTrackConnectionPoint(0,160,0));
	trackType->AddConnection(new TrainTrackConnectionPoint(180,-160,0));
	trainTrackTypes[LC_TTS_STRAIGHT] = trackType;

	// curved
	trackType = new TrainTrackType();
	trackType->Load(Library,mTrackTypeFilenames[LC_TTS_CURVED]);
	trackType->AddConnection(new TrainTrackConnectionPoint(-11.25,xHalfBendOff,yHalfBendOff));
	trackType->AddConnection(new TrainTrackConnectionPoint(-168.75,-xHalfBendOff,yHalfBendOff));
	trainTrackTypes[LC_TTS_CURVED] = trackType;

	// Crossing
	trackType = new TrainTrackType();
	trackType->Load(Library,mTrackTypeFilenames[LC_TTS_CROSS]);
	trackType->AddConnection(new TrainTrackConnectionPoint(0,160,0));
	trackType->AddConnection(new TrainTrackConnectionPoint(90,0,160));
	trackType->AddConnection(new TrainTrackConnectionPoint(180,-160,0));
	trackType->AddConnection(new TrainTrackConnectionPoint(-90,0,-160));
	trainTrackTypes[LC_TTS_CROSS] = trackType;

	// Left branch
	trackType = new TrainTrackType();
	trackType->Load(Library,mTrackTypeFilenames[LC_TTS_LEFT_BRANCH]);
	trackType->AddConnection(new TrainTrackConnectionPoint(0,320,0));
	trackType->AddConnection(new TrainTrackConnectionPoint(22.50,x_left_branch,y_left_branch));	
	trackType->AddConnection(new TrainTrackConnectionPoint(180,-320,0));
	trainTrackTypes[LC_TTS_LEFT_BRANCH] = trackType;

	// Right branch
	trackType = new TrainTrackType();
	trackType->Load(Library,mTrackTypeFilenames[LC_TTS_RIGHT_BRANCH]);
	trackType->AddConnection(new TrainTrackConnectionPoint(0,320,0));
	trackType->AddConnection(new TrainTrackConnectionPoint(180,-320,0));
	trackType->AddConnection(new TrainTrackConnectionPoint(-22.50,x_right_branch,y_right_branch));		
	trainTrackTypes[LC_TTS_RIGHT_BRANCH] = trackType;
		
	// Create map for lookup track type by name
	for(int i = 0; i < LC_TTS_NUMITEMS; i++) trackTypesByName.insert({mTrackTypeFilenames[i], static_cast<LC_TTS_TYPES>(i)});
}


TrainTrackSystem::~TrainTrackSystem() {}

void TrainTrackSystem::setColorIndex(int mColorIndex) {
	this->mColorIndex = mColorIndex;
}

void TrainTrackSystem::setCurrentStep(int mCurrentStep) {
	this->mCurrentStep = mCurrentStep;
}

void TrainTrackSystem::addFirstTrackSection(lcMatrix44 position, enum LC_TTS_TYPES trackTypeId) {
	TrainTrackType* trainTrackType = trainTrackTypes[trackTypeId];
	trackSections.push_back(new TrainTrackSection(trainTrackType, trackTypeId,position, mCurrentStep, mColorIndex));
}

void TrainTrackSystem::addTrackSection(int fromTrackSectionIdx, int fromTrackSectionConnectionIdx, enum LC_TTS_TYPES toTrackType, int toTrackTypeConnectionIdx) 
{
	TrainTrackSection* fromTrackSection = trackSections[fromTrackSectionIdx];
	TrainTrackType* fromTrack = trainTrackTypes[fromTrackSection->trackTypeId];
	TrainTrackConnectionPoint * fromConnectIdx = fromTrack->GetConnection(fromTrackSectionConnectionIdx);
	TrainTrackType* toTrack = trainTrackTypes[toTrackType];
	TrainTrackConnectionPoint * toConnectIdx = toTrack->GetConnection(toTrackTypeConnectionIdx);

	lcMatrix44 currentLocation = fromTrackSection->location;
	lcMatrix44 mat44;
	lcMatrix44 matOffset;

	// calculate position on exiting connection point
	mat44 = lcMatrix44Identity();
	mat44 =	lcMatrix44RotationZ(LC_DTOR * fromConnectIdx->angle);

	matOffset = lcMatrix44Identity();
	matOffset.SetTranslation(lcVector3(fromConnectIdx->xy_coordinate.x, fromConnectIdx->xy_coordinate.y,0));

	matOffset = lcMul(mat44, matOffset);

	currentLocation = lcMul(matOffset,currentLocation);

	// calculate position on entering new track section
	mat44 = lcMatrix44Identity();
	mat44 =	lcMatrix44RotationZ(LC_DTOR * (180 - toConnectIdx->angle));

	matOffset = lcMatrix44Identity();
	matOffset.SetTranslation(lcVector3(-toConnectIdx->xy_coordinate.x, -toConnectIdx->xy_coordinate.y,0));

	matOffset = lcMul(matOffset,mat44);

	currentLocation = lcMul(matOffset,currentLocation);

	trackSections.push_back(new TrainTrackSection(toTrack, toTrackType,currentLocation, mCurrentStep, mColorIndex));
}


void TrainTrackSystem::addTrackSection(TrainTrackSection* fromTrackSection, int fromTrackSectionConnectionIdx, enum LC_TTS_TYPES toTrackType, int toTrackTypeConnectionIdx) 
{
	TrainTrackType* fromTrack = trainTrackTypes[fromTrackSection->trackTypeId];	
	TrainTrackConnectionPoint * fromConnectIdx = fromTrack->GetConnection(fromTrackSectionConnectionIdx);
	TrainTrackType* toTrack = trainTrackTypes[toTrackType];
	TrainTrackConnectionPoint * toConnectIdx = toTrack->GetConnection(toTrackTypeConnectionIdx);

	lcMatrix44 currentLocation = fromTrackSection->location;
	lcMatrix44 mat44;
	lcMatrix44 matOffset;

	// calculate position on exiting connection point
	mat44 = lcMatrix44Identity();
	mat44 =	lcMatrix44RotationZ(LC_DTOR * fromConnectIdx->angle);

	matOffset = lcMatrix44Identity();
	matOffset.SetTranslation(lcVector3(fromConnectIdx->xy_coordinate.x, fromConnectIdx->xy_coordinate.y,0)); 

	matOffset = lcMul(mat44, matOffset);

	currentLocation = lcMul(matOffset,currentLocation);

	// calculate position on entering new track section
	mat44 = lcMatrix44Identity();
	mat44 =	lcMatrix44RotationZ(LC_DTOR * (180 - toConnectIdx->angle));

	matOffset = lcMatrix44Identity();
	matOffset.SetTranslation(lcVector3(-toConnectIdx->xy_coordinate.x, -toConnectIdx->xy_coordinate.y,0)); 

	matOffset = lcMul(matOffset,mat44);

	currentLocation = lcMul(matOffset,currentLocation);	

	trackSections.push_back(new TrainTrackSection(toTrack, toTrackType,currentLocation, mCurrentStep, mColorIndex));				
}

bool TrainTrackSystem::deleteTrackSection(lcPiece* piece)
{	
	float precisionErrorNumber = 0.001;
	lcVector3 vec3PieceDel = piece->GetRotationCenter();

	for(unsigned long i = 0; i < trackSections.size(); i++) {
		
		lcVector3 vec3PieceCur = trackSections[i]->location.GetTranslation(); 

		if(abs(vec3PieceDel.x - vec3PieceCur.x) < precisionErrorNumber &&
			abs(vec3PieceDel.y - vec3PieceCur.y) < precisionErrorNumber &&
			abs(vec3PieceDel.z - vec3PieceCur.z) < precisionErrorNumber) 
		{
            trackSections.erase(trackSections.begin() + i);
			return true;
		}
	}	
	return false;
}

std::vector<PieceInfo *> TrainTrackSystem::getTrackTypes()
{
    std::vector<PieceInfo*> trackTypes;
	for(int i = 0; i < LC_TTS_NUMITEMS; i++) {
		trackTypes.push_back(trainTrackTypes[i]->pieceInfo);		
	}
	return trackTypes;	
} 

std::vector<TrainTrackSection*> TrainTrackSystem::GetTrackSections()
{
	return trackSections;
}

std::vector<TrainTrackSection*>* TrainTrackSystem::GetTrackSectionsPtr()
{
	return &trackSections;
}

std::vector<lcPiece*> TrainTrackSystem::GetPieces()
{
    std::vector<lcPiece*> pieces;
	PieceInfo* pieceInfo;

	lcPiecesLibrary* Library = lcGetPiecesLibrary();
	PieceInfo* pieceInfoSel = Library->FindPiece("3023.dat", nullptr, false, false);
	
	TrainTrackSection* trackSection;	
	lcPiece* piece;
		
	for(unsigned long i = 0; i < trackSections.size(); i++)
    {
		trackSection = trackSections[i];
		pieceInfo = trainTrackTypes[trackSection->trackTypeId]->pieceInfo;
		piece = new lcPiece(pieceInfo);
		piece->Initialize(trackSection->location, trackSection->mCurrentStep);
		piece->SetColorIndex(trackSection->mColorIndex);
        pieces.push_back(piece);
		
		// Add 1 x 2 plate on each connecton point, for detect selection
		lcMatrix44 mat44;
		lcMatrix44 matOffset;

		for(int con_no = 0; con_no < trackSection->GetNoOfConnections(); con_no++) 
        {
			mat44 = trackSection->GetConnectionLocation(con_no);

			matOffset = lcMatrix44Identity();
			matOffset.SetTranslation(lcVector3(-10, 0, 8)); 

			mat44 = lcMul(matOffset,mat44);

			matOffset = lcMatrix44Identity();
			matOffset =	lcMatrix44RotationZ(LC_DTOR * 90);

			mat44 = lcMul(matOffset,mat44);

			piece = new lcPiece(pieceInfoSel);
			piece->Initialize(mat44, trackSection->mCurrentStep);
			piece->SetColorIndex(trackSection->mColorIndex);
			//pieces.Add(piece);
            pieces.push_back(piece);		
		}
	}
	return pieces;
}	

lcModel* TrainTrackSystem::GetModel()
{
    std::vector<lcPiece*> trackSystem = this->GetPieces();
	mModel->SetTrainTrackSystem(trackSystem);
	return mModel.get();
}

void TrainTrackSystem::SetModel()
{
    std::vector<lcPiece*> trackSystem = this->GetPieces();
	mModel->SetTrainTrackSystem(trackSystem);	
}


LC_TTS_TYPES TrainTrackSystem::findTrackTypeByString(const char *tracktypeName) 
{
	std::string str_search = tracktypeName;
	std::map<std::string ,LC_TTS_TYPES>::iterator search = trackTypesByName.find(str_search);
    if (search != trackTypesByName.end()) {
		return search->second;
	}		
    return static_cast<LC_TTS_TYPES>(-1);
}
