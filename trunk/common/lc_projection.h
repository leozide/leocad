#ifndef _LC_PROJECTION_H_
#define _LC_PROJECTION_H_

#include <stdlib.h>
#include "opengl.h"
#include "lc_math.h"

class Camera;

class lcProjection
{
public:

	enum Type
	{
		Ortho = 0,
		Projection,
		OUT_OF_RANGE,
		Cycle
	};

	lcProjection()
	{
		mType = Projection;
		mTransform = lcMatrix44Identity();
		mCamera = NULL;
	}

	inline void SetType(Type type)
	{
		if (Cycle == type)
		{
			type = (Type)((int)mType + 1);
			if (OUT_OF_RANGE == type)
				type = (Type)0;
		}

		if (type < OUT_OF_RANGE)
		{
			mType = type;
		}

		calculateTransform();
	}

	inline Type GetType() const
	{
		return mType;
	}

	inline void SetView(const Camera* pCamera, int width, int height)
	{
		setTransformInput(pCamera, width, height);
	}

	inline lcVector3 ProjectPoint(const lcMatrix44& WorldView, const lcVector3& Point) const
	{
		int viewport[4] = { 0, 0, mViewPixelWidth, mViewPixelHeight };
		return lcProjectPoint(Point, WorldView, mTransform, viewport);
	}

	inline lcVector3 UnprojectPoint(const lcMatrix44& WorldView, const lcVector3& Point) const
	{
		int viewport[4] = { 0, 0, mViewPixelWidth, mViewPixelHeight };
		return lcUnprojectPoint(Point, WorldView, mTransform, viewport);
	}

	inline void UnprojectPoints(const lcMatrix44& WorldView, lcVector3* Points, int NumPoints) const
	{
		if (NumPoints > 0)
		{
			int viewport[4] = { 0, 0, mViewPixelWidth, mViewPixelHeight };
			lcUnprojectPoints(Points, NumPoints, WorldView, mTransform, viewport);
		}
	}

	inline int ConstrainX(int x) const
	{ 
		if (x >= mViewPixelWidth) 
			return mViewPixelWidth - 1; 
		else 
			if (x <= 0) 
				return 1;
		return x;
	}

	inline int ConstrainY(int y) const
	{ 
		if (y >= mViewPixelHeight) 
			return mViewPixelHeight - 1; 
		else 
			if (y <= 0) 
				return 1; 
		return y;
	}

	operator lcMatrix44& ()
	{
		return mTransform;
	}

	const char* GetName() const
	{
		switch(mType)
		{
		case Ortho:
			return "Ortho";

		case Projection:
			return "Projection";

		default:
			return "";
		}
	}

private:

	void calculateTransform();
	void setTransformInput(const Camera* pCamera, int width, int height);

	Type mType;
	lcMatrix44 mTransform;
	const Camera* mCamera;
	int mViewPixelWidth;
	int mViewPixelHeight;
};

#endif // _LC_PROJECTION_H_
