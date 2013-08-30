#ifndef LC_MODEL_H
#define LC_MODEL_H

#include "lc_array.h"
#include "lc_math.h"

class View;
class lcCheckpoint;
struct lcObjectHitTest;
struct lcObjectSection;

enum lcActionType
{
	LC_ACTION_CREATE_CAMERA
//	LC_NUM_ACTIONS
};

class lcModel
{
public:
	lcModel();
	~lcModel();

	void RenderBackground(View* View) const;
	void RenderObjects(View* View) const;

	void ToggleSelection(const lcObjectSection& ObjectSection);
	void AddToSelection(const lcArray<lcObjectSection>& ObjectSections);
	void SetSelection(const lcArray<lcObjectSection>& ObjectSections);
	void ToggleFocus(const lcObjectSection& ObjectSection);
	void SetFocus(const lcObjectSection& ObjectSection);

	void FindClosestObject(lcObjectHitTest& HitTest) const;
	void FindObjectsInBox(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections) const;

//	void GetRenderMeshes(View* View, bool PartsOnly, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes) const;

	void SetCurrentTime(lcTime Time);

	void BeginCameraTool(const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3& UpVector);
	void UpdateCameraTool(const lcVector3& Distance);
	void EndCameraTool(bool Accept);
//	void MoveSelectedObjects(const lcVector3& Distance);

protected:
	void DeleteContents();
	void Update(lcTime Time);

	void BeginCheckpoint(const char* Name);
	void EndCheckpoint(bool Accept);
	void ApplyCheckpoint(lcCheckpoint* Checkpoint);
	void RevertCheckpoint(lcCheckpoint* Checkpoint);

	lcCheckpoint* mCurrentCheckpoint;
	lcArray<lcCheckpoint*> mUndoCheckpoints;
	lcArray<lcCheckpoint*> mRedoCheckpoints;

	lcObject* mFocusObject;
	lcArray<lcObject*> mSelectedObjects;

	lcArray<lcObject*> mObjects;

//	lcArray<lcPart*> mParts;
//	lcArray<lcCamera*> mCameras;
//	lcArray<lcLight*> mLights;
};

#endif // LC_MODEL_H
