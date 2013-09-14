#ifndef LC_MODEL_H
#define LC_MODEL_H

#include "lc_array.h"
#include "lc_math.h"

class View;
class PieceInfo;
class lcCheckpoint;
struct lcObjectHitTest;
struct lcObjectSection;
struct lcRenderMesh;

enum lcActionType
{
	LC_ACTION_CREATE_PIECE,
	LC_ACTION_CREATE_CAMERA,
	LC_ACTION_MOVE_OBJECTS,
	LC_ACTION_ZOOM_CAMERA,
	LC_ACTION_PAN_CAMERA,
	LC_ACTION_ORBIT_CAMERA,
	LC_ACTION_ROLL_CAMERA,
	LC_ACTION_ZOOM_EXTENTS,
	LC_ACTION_ZOOM_REGION
};

class lcModel
{
public:
	lcModel();
	~lcModel();

	lcObject* GetFocusObject() const
	{
		return mFocusObject;
	}

	const lcArray<lcObject*>& GetSelectedObjects() const
	{
		return mSelectedObjects;
	}

	lcCamera* GetCamera(int CameraIndex);
	void GetCameras(lcArray<lcCamera*>& Cameras);

	void RenderBackground(View* View) const;
	void RenderScene(View* View, bool RenderInterface) const;
	void GetRenderMeshes(View* View, lcArray<lcRenderMesh>& OpaqueMeshes, lcArray<lcRenderMesh>& TranslucentMeshes, lcArray<lcObject*>& InterfaceObjects) const;

	void InvertSelection();
	void InvertSelection(const lcObjectSection& ObjectSection);
	void AddToSelection(const lcArray<lcObjectSection>& ObjectSections);
	void SetSelection(const lcArray<lcObjectSection>& ObjectSections);
	void ClearSelectionOrSetFocus(const lcObjectSection& ObjectSection);
	void SetFocus(const lcObjectSection& ObjectSection);

	void FindClosestObject(lcObjectHitTest& HitTest) const;
	void FindObjectsInBox(const lcVector4* BoxPlanes, lcArray<lcObjectSection>& ObjectSections) const;

	void AddPiece(PieceInfo* Part, int ColorIndex, const lcVector3& Position, const lcVector4& AxisAngle, lcTime Time);

	void BeginCameraTool(const lcVector3& Position, const lcVector3& TargetPosition, const lcVector3& UpVector);
	void UpdateCameraTool(const lcVector3& Distance, lcTime Time, bool AddKeys);
	void EndCameraTool(bool Accept);

	void BeginMoveTool();
	void UpdateMoveTool(const lcVector3& Distance, lcTime Time, bool AddKeys);
	void EndMoveTool(bool Accept);

	void BeginZoomTool();
	void UpdateZoomTool(float Distance, lcTime Time, bool AddKeys);
	void EndZoomTool(bool Accept);

	void BeginPanTool();
	void UpdatePanTool(float DistanceX, float DistanceY, lcTime Time, bool AddKeys);
	void EndPanTool(bool Accept);

	void BeginOrbitTool(const lcVector3& Center);
	void UpdateOrbitTool(float AngleX, float AngleY, lcTime Time, bool AddKeys);
	void EndOrbitTool(bool Accept);

	void BeginRollTool();
	void UpdateRollTool(float Angle, lcTime Time, bool AddKeys);
	void EndRollTool(bool Accept);

	void ZoomExtents(View* View, const lcVector3& Center, const lcVector3* Points, lcTime Time, bool AddKeys);
	void ZoomRegion(View* View, float Left, float Right, float Bottom, float Top, lcTime Time, bool AddKeys);

protected:
	void DeleteContents();
	void Update(lcTime Time);

	void BeginCheckpoint(lcActionType ActionType);
	void EndCheckpoint(bool Accept, bool SaveCheckpoint);
	void ApplyCheckpoint(lcCheckpoint* Checkpoint);
	void RevertCheckpoint(lcCheckpoint* Checkpoint);

	lcCheckpoint* mCurrentCheckpoint;
	lcArray<lcCheckpoint*> mUndoCheckpoints;
	lcArray<lcCheckpoint*> mRedoCheckpoints;

	lcObject* mFocusObject;
	lcArray<lcObject*> mSelectedObjects;

	lcArray<lcObject*> mObjects;
};

#endif // LC_MODEL_H
