#pragma once

#include "lc_math.h"
#include "lc_context.h"
#include <bitset>

class lcGLWidget;
class View;
class lcPreviewWidget;

enum class lcViewSphereLocation
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight
};

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
	void UpdateSettings();
	lcMatrix44 GetViewMatrix() const;
	lcMatrix44 GetProjectionMatrix() const;
	std::bitset<6> GetIntersectionFlags(lcVector3& Intersection) const;

	lcGLWidget* const mWidget = nullptr;
	lcPreviewWidget* const mPreview = nullptr;
	View* const mView = nullptr;
	bool mIsPreview = false;

	int mSize = 1;
	bool mEnabled = true;
	lcViewSphereLocation mLocation = lcViewSphereLocation::TopRight;

	int mMouseDownX = 0;
	int mMouseDownY = 0;
	bool mMouseDown = false;

	lcVector3 mIntersection;
	std::bitset<6> mIntersectionFlags;

	static lcTexture* mTexture;
	static lcVertexBuffer mVertexBuffer;
	static lcIndexBuffer mIndexBuffer;
	static const float mRadius;
	static const float mHighlightRadius;
	static const int mSubdivisions;
};
