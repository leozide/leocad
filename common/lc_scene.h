#pragma once

#include "lc_mesh.h"
#include "lc_array.h"

enum class lcRenderPass
{
	UnlitOpaque,
	UnlitTranslucent,
	LitOpaque,
	LitTranslucent
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

	void SetDrawInterface(bool DrawInterface)
	{
		mDrawInterface = DrawInterface;
	}

	bool GetDrawInterface() const
	{
		return mDrawInterface;
	}

	void SetAllowWireframe(bool AllowWireframe)
	{
		mAllowWireframe = AllowWireframe;
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
		mInterfaceObjects.Add(Object);
	}

	void Draw(lcContext* Context) const;
	void DrawInterfaceObjects(lcContext* Context) const;

protected:
	void DrawPass(lcContext* Context, lcRenderPass RenderPass, int PrimitiveTypes) const;

	lcMatrix44 mViewMatrix;
	lcMatrix44 mActiveSubmodelTransform;
	lcPiece* mActiveSubmodelInstance;
	bool mDrawInterface;
	bool mAllowWireframe;

	lcArray<lcRenderMesh> mRenderMeshes;
	lcArray<int> mOpaqueMeshes;
	lcArray<int> mTranslucentMeshes;
	lcArray<const lcObject*> mInterfaceObjects;
};
