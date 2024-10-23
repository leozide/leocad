#pragma once

#include "lc_library.h"
#include <vector>
#include <map>
#include <string>
#include <memory>


enum LC_TTS_TYPES {
    LC_TTS_STRAIGHT,
    LC_TTS_CURVED,
    LC_TTS_CROSS,
    LC_TTS_LEFT_BRANCH,
    LC_TTS_RIGHT_BRANCH,
    LC_TTS_NUMITEMS
};

class TrainTrackConnectionPoint {
    public:
        TrainTrackConnectionPoint(float angle, float x, float y);
        float angle;
        lcVector2 xy_coordinate;
};


class TrainTrackType {
    public:
        
        void Load(lcPiecesLibrary* Library, char const* filename);
        void AddConnection(TrainTrackConnectionPoint *connectionPoint);        
        TrainTrackConnectionPoint *GetConnection(int index);

        //lcArray<TrainTrackConnectionPoint *>& GetConnections();
        std::vector<TrainTrackConnectionPoint *>& GetConnections();
        PieceInfo* pieceInfo;
        int GetNoOfConnections();        
    private:
        char const* filename;
        //lcArray<TrainTrackConnectionPoint *> connectionPoints;
        std::vector<TrainTrackConnectionPoint *> connectionPoints;
        
};

class TrainTrackSection {
    public:
        TrainTrackSection(TrainTrackType *trackType, enum LC_TTS_TYPES trackTypeId, lcMatrix44 location, int mCurrentStep, int mColorIndex);
        lcMatrix44 GetConnectionLocation(int connectionNo);
        int GetNoOfConnections();
        int FindConnectionNumber(lcVector3 searchLocation);
        TrainTrackType *trackType;
        enum LC_TTS_TYPES trackTypeId;        
        lcMatrix44 location;
        int mCurrentStep;
        int mColorIndex;
};


class TrainTrackSystem
{
    public:
        TrainTrackSystem();
        ~TrainTrackSystem();
	    void setColorIndex(int colorIndex);
	    void setCurrentStep(int mCurrentStep);

        void addFirstTrackSection(lcMatrix44 position, enum LC_TTS_TYPES trackTypeId);
        void addTrackSection(int fromTrackSectionIdx, int fromTrackSectionConnectionIdx, enum LC_TTS_TYPES toTrackType, int toTrackTypeConnectionIdx);
        void addTrackSection(TrainTrackSection* fromTrackSection, int fromTrackSectionConnectionIdx, enum LC_TTS_TYPES toTrackType, int toTrackTypeConnectionIdx);
        bool deleteTrackSection(lcPiece* piece);

        //lcArray<PieceInfo*> getTrackTypes();
        std::vector<PieceInfo *> getTrackTypes();

        //lcArray<TrainTrackSection*> GetTrackSections();
        std::vector<TrainTrackSection *> GetTrackSections();
        //lcArray<TrainTrackSection*>* GetTrackSectionsPtr();
        std::vector<TrainTrackSection *>* GetTrackSectionsPtr();

        //lcArray<lcPiece* > GetPieces();
        std::vector<lcPiece *> GetPieces();

        lcModel* GetModel();
        void SetModel();
        LC_TTS_TYPES findTrackTypeByString(const char *tracktypeName); 
    private:

        //lcArray<TrainTrackType*> trainTrackTypes;
        std::vector<TrainTrackType *> trainTrackTypes;

        static const char* mTrackTypeFilenames[LC_TTS_NUMITEMS];
        //lcArray<TrainTrackSection*> trackSections; 
        std::vector<TrainTrackSection *> trackSections;

        int mColorIndex;
        int mCurrentStep;

        std::unique_ptr<lcModel> mModel;

        std::map<std::string ,LC_TTS_TYPES> trackTypesByName;
        
};



