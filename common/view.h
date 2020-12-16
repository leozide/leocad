#pragma once

#include "lc_glwidget.h"
#include "camera.h"
#include "lc_viewsphere.h"

class View : public lcGLWidget
{
	Q_OBJECT

public:
	View(lcModel* Model);
	~View();

	View(const View&) = delete;
	View(View&&) = delete;
	View& operator=(const View&) = delete;
	View& operator=(View&&) = delete;

	void Clear()
	{
		mModel = nullptr;
		mActiveSubmodelInstance = nullptr;
	}

	lcModel* GetModel() const
	{
		return mModel;
	}

	void SetTopSubmodelActive();
	void SetSelectedSubmodelActive();

	static void CreateResources(lcContext* Context);
	static void DestroyResources(lcContext* Context);

	void OnDraw() override;
	void OnInitialUpdate() override;
	void OnLeftButtonDown() override;
	void OnLeftButtonUp() override;
	void OnLeftButtonDoubleClick() override;
	void OnMiddleButtonDown() override;
	void OnMiddleButtonUp() override;
	void OnRightButtonDown() override;
	void OnRightButtonUp() override;
	void OnBackButtonUp() override;
	void OnForwardButtonUp() override;
	void OnMouseMove() override;
	void BeginDrag(lcDragState DragState) override;
	void EndDrag(bool Accept) override;

	void StartOrbitTracking();
	void CancelTrackingOrClearSelection();

	void SetProjection(bool Ortho);
	void LookAt();
	void MoveCamera(const lcVector3& Direction);
	void Zoom(float Amount);

	void RemoveCamera();
	void ShowContextMenu() const;

	lcVector3 GetMoveDirection(const lcVector3& Direction) const;
	lcMatrix44 GetPieceInsertPosition(bool IgnoreSelected, PieceInfo* Info) const;
	void GetRayUnderPointer(lcVector3& Start, lcVector3& End) const;
	lcObjectSection FindObjectUnderPointer(bool PiecesOnly, bool IgnoreSelected) const;
	lcArray<lcObject*> FindObjectsInBox(float x1, float y1, float x2, float y2) const;

	bool BeginRenderToImage(int Width, int Height);
	void EndRenderToImage();

	QImage GetRenderImage() const
	{
		return mRenderImage;
	}

protected:
	static void CreateSelectMoveOverlayMesh(lcContext* Context);

	void DrawSelectMoveOverlay();
	void DrawRotateOverlay();
	void DrawSelectZoomRegionOverlay();
	void DrawRotateViewOverlay();
	void DrawGrid();

	void UpdateTrackTool();
	bool IsTrackToolAllowed(lcTrackTool TrackTool, quint32 AllowedTransforms) const;
	lcTrackTool GetOverrideTrackTool(Qt::MouseButton Button) const;
	float GetOverlayScale() const;
	void StopTracking(bool Accept);
	void OnButtonDown(lcTrackButton TrackButton);
	lcMatrix44 GetTileProjectionMatrix(int CurrentRow, int CurrentColumn, int CurrentTileWidth, int CurrentTileHeight) const;

	lcDragState mDragState;
	bool mTrackToolFromOverlay;
	lcVector3 mMouseDownPosition;
	PieceInfo* mMouseDownPiece;
	QImage mRenderImage;
	std::pair<lcFramebuffer, lcFramebuffer> mRenderFramebuffer;
	lcViewSphere mViewSphere;

	lcVertexBuffer mGridBuffer;
	int mGridSettings[7];

	static lcVertexBuffer mRotateMoveVertexBuffer;
	static lcIndexBuffer mRotateMoveIndexBuffer;
};

