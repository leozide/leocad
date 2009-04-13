#ifndef _DEBUG_H_
#define _DEBUG_H_

#if LC_PROFILE

struct lcRenderStats
{
	int TriCount;
	int LineCount;
	int RenderMS;
};

extern lcRenderStats g_RenderStats;

void lcRenderProfileStats(class View* view);

#endif

#if LC_DEBUG

#include "algebra.h"

void RenderDebugPrimitives();

void AddDebugLine(const Vector3& pt1, const Vector3& pt2, const Vector3& Color);
void ClearDebugLines();

void AddDebugQuad(const Vector3& pt1, const Vector3& pt2, const Vector3& pt3, const Vector3& pt4, const Vector4& Color);
void ClearDebugQuads();

#endif // LC_DEBUG

#endif // _DEBUG_H_
