#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef LC_PROFILE

struct lcRenderStats
{
	int QuadCount;
	int TriCount;
	int LineCount;
	int RenderMS;
};

extern lcRenderStats g_RenderStats;

extern void lcRenderProfileStats(class View* view);

#endif

#ifdef LC_DEBUG

#include "algebra.h"

extern void RenderDebugPrimitives();

extern void AddDebugLine(const Vector3& pt1, const Vector3& pt2, const Vector3& Color);
extern void ClearDebugLines();

extern void AddDebugQuad(const Vector3& pt1, const Vector3& pt2, const Vector3& pt3, const Vector3& pt4, const Vector4& Color);
extern void ClearDebugQuads();

#endif // LC_DEBUG

#endif // _DEBUG_H_
