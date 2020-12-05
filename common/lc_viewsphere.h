#pragma once

#include "lc_math.h"
#include "lc_context.h"
#include <bitset>

class lcGLWidget;
class View;
class lcPreviewWidget;

class lcViewSphere
{
public:
	lcViewSphere(View *View);
	lcViewSphere(lcPreviewWidget *Preview);

	void Draw();
	bool OnMouseMove();
	bool OnLeftButtonUp();
	bool OnLeftButtonDown();
	bool IsDragging() const;

	static void CreateResources(lcContext* Context);
	static void DestroyResources(lcContext* Context);

protected:
	lcMatrix44 GetViewMatrix() const;
	lcMatrix44 GetProjectionMatrix() const;
	std::bitset<6> GetIntersectionFlags(lcVector3& Intersection) const;

	lcGLWidget* mWidget = nullptr;
	lcPreviewWidget* mPreview = nullptr;
	View* mView = nullptr;
	lcVector3 mIntersection;
	std::bitset<6> mIntersectionFlags;
	int mViewSphereSize = 1;
	int mMouseDownX = 0;
	int mMouseDownY = 0;
	bool mMouseDown = false;
	bool mIsPreview = false;

	static lcTexture* mTexture;
	static lcVertexBuffer mVertexBuffer;
	static lcIndexBuffer mIndexBuffer;
	static const float mRadius;
	static const float mHighlightRadius;
	static const int mSubdivisions;
};
