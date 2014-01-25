#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef LC_DEBUG

#include "lc_math.h"

void RenderDebugPrimitives();

void AddDebugLine(const lcVector3& pt1, const lcVector3& pt2, const lcVector3& Color);
void ClearDebugLines();

void AddDebugQuad(const lcVector3& pt1, const lcVector3& pt2, const lcVector3& pt3, const lcVector3& pt4, const lcVector4& Color);
void ClearDebugQuads();

#endif // LC_DEBUG

#endif // _DEBUG_H_
