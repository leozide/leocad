#pragma once

#include "lc_mesh.h"

enum class lcHighlightRenderMode;

enum class lcRenderMeshState : int
{
	Default,
	Selected,
	Focused,
	Preview,
	Faded,
	Highlighted
};

struct lcRenderMesh
{
	lcMatrix44 WorldMatrix;
	lcMesh* Mesh;
	int ColorIndex;
	int LodIndex;
	lcRenderMeshState State;
};

struct lcTranslucentMeshInstance
{
	const lcMeshSection* Section;
	int RenderMeshIndex;
	float Distance;
};

class lcScene
{
public:
	lcScene();

	void SetActiveSubmodelInstance(lcPiece* ActiveSubmodelInstance, const lcMatrix44& ActiveSubmodelTransform)
	{
		mActiveSubmodelInstance = ActiveSubmodelInstance;
		mActiveSubmodelTransform = ActiveSubmodelTransform;
	}

	lcPiece* GetActiveSubmodelInstance() const
	{
		return mActiveSubmodelInstance;
	}

	const lcMatrix44& GetViewMatrix() const
	{
		return mViewMatrix;
	}

	void SetDrawInterface(bool DrawInterface)
	{
		mDrawInterface = DrawInterface;
	}

	bool GetDrawInterface() const
	{
		return mDrawInterface;
	}

	void SetRequireCompleteAssets(bool RequireCompleteAssets)
	{
		mRequireCompleteAssets = RequireCompleteAssets;
	}

	bool GetRequireCompleteAssets() const
	{
		return mRequireCompleteAssets;
	}

	void SetRequireGeneratedMeshes(bool RequireGeneratedMeshes)
	{
		mRequireGeneratedMeshes = RequireGeneratedMeshes;
	}

	bool GetRequireGeneratedMeshes() const
	{
		return mRequireGeneratedMeshes;
	}

	void MarkMissingAssets()
	{
		mMissingAssets = true;
	}

	bool HasMissingAssets() const
	{
		return mMissingAssets;
	}

	void SetDrawInsertPreview(bool DrawInsertPreview)
	{
		mDrawInsertPreview = DrawInsertPreview;
	}

	bool GetDrawInsertPreview() const
	{
		return mDrawInsertPreview;
	}

	void SetShadingMode(lcShadingMode ShadingMode)
	{
		mShadingMode = ShadingMode;
	}

	void SetLineSettings(bool DrawEdgeLines, bool DrawConditionalLines, float LineWidth)
	{
		mDrawEdgeLines = DrawEdgeLines;
		mDrawConditionalLines = DrawConditionalLines;
		mLineWidth = LineWidth;
	}

	void SetAllowLOD(bool AllowLOD)
	{
		mAllowLOD = AllowLOD;
	}

	void SetLODDistance(float Distance)
	{
		mMeshLODDistance = Distance;
	}

	lcMatrix44 ApplyActiveSubmodelTransform(const lcMatrix44& WorldMatrix) const
	{
		return !mActiveSubmodelInstance ? WorldMatrix : lcMul(WorldMatrix, mActiveSubmodelTransform);
	}

	void Begin(const lcMatrix44& ViewMatrix);
	void End();
	void AddMesh(lcMesh* Mesh, const lcMatrix44& WorldMatrix, int ColorIndex, lcRenderMeshState State);

	void AddInterfaceObject(const lcObject* Object)
	{
		mInterfaceObjects.emplace_back(Object);
	}

	void Draw(lcContext* Context, const std::function<void()>& PreTranslucentCallback) const;
	bool HasSelectedMeshes() const;
	bool HasHighlightedMeshes() const;
	bool CanReuseSceneDepth() const;
	void DrawSelectionMask(lcContext* Context) const;
	void DrawHighlightMask(lcContext* Context) const;
	void DrawOutlineOccluders(lcContext* Context) const;
	void SetHighlightRenderMode(lcHighlightRenderMode HighlightRenderMode)
	{
		mHighlightRenderMode = HighlightRenderMode;
	}

	lcHighlightRenderMode GetHighlightRenderMode() const
	{
		return mHighlightRenderMode;
	}

	void DrawInterfaceObjects(lcContext* Context) const;

protected:
	enum class lcOutlineGeometry
	{
		Selection,
		Highlight,
		Occluders
	};

	void DrawOutlineGeometry(lcContext* Context, lcOutlineGeometry Geometry) const;
	void DrawOpaqueMeshes(lcContext* Context, bool DrawLit, int PrimitiveTypes, bool DrawFaded, bool DrawNonFaded) const;
	void DrawTranslucentMeshes(lcContext* Context, bool DrawLit, bool DrawFadePrepass, bool DrawFaded, bool DrawNonFaded) const;
	void DrawDebugNormals(lcContext* Context, const lcMesh* Mesh) const;

	lcMatrix44 mViewMatrix;
	lcMatrix44 mActiveSubmodelTransform;
	lcPiece* mActiveSubmodelInstance = nullptr;
	lcShadingMode mShadingMode;
	bool mDrawInterface = false;
	lcHighlightRenderMode mHighlightRenderMode;
	bool mRequireCompleteAssets = false;
	bool mRequireGeneratedMeshes = false;
	bool mMissingAssets = false;
	bool mDrawInsertPreview = false;
	bool mAllowLOD = true;
	bool mDrawEdgeLines = true;
	bool mDrawConditionalLines = true;
	float mLineWidth = 1.0f;
	float mMeshLODDistance;

	lcVector4 mFadeColor;
	lcVector4 mHighlightColor;
	bool mHasFadedParts = false;
	bool mTranslucentFade = false;

	std::vector<lcRenderMesh> mRenderMeshes;
	std::vector<int> mOpaqueMeshes;
	std::vector<lcTranslucentMeshInstance> mTranslucentMeshes;
	std::vector<const lcObject*> mInterfaceObjects;
};
