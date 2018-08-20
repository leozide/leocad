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

protected:
	lcMatrix44 GetViewMatrix() const;
	lcMatrix44 GetProjectionMatrix() const;

	View* mView;
	lcVector3 mIntersection;
	std::bitset<6> mIntersectionFlags;
};
