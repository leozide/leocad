#pragma once

#include "lc_math.h"
#include <bitset>

class View;

class lcViewCube
{
public:
	lcViewCube(View* View);

	void Draw();
	bool OnMouseMove();
	bool OnLeftButtonUp();
	bool OnLeftButtonDown();
	bool IsDragging() const;

protected:
	lcMatrix44 GetViewMatrix() const;
	lcMatrix44 GetProjectionMatrix() const;
	std::bitset<6> GetIntersectionFlags(lcVector3& Intersection) const;

	View* mView;
	lcVector3 mIntersection;
	std::bitset<6> mIntersectionFlags;
	int mMouseDownX;
	int mMouseDownY;
	bool mMouseDown;
};
