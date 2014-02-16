#include "lc_global.h"
#include "opengl.h"
#include "debug.h"

#ifdef LC_DEBUG

#define LC_MAX_DEBUG_LINES 100

struct LC_DEBUG_LINE
{
	lcVector3 pt1;
	lcVector3 pt2;
	lcVector3 color;
};

static LC_DEBUG_LINE DebugLines[LC_MAX_DEBUG_LINES];
static int NumDebugLines;

void ClearDebugLines()
{
	NumDebugLines = 0;
}

void AddDebugLine(const lcVector3& pt1, const lcVector3& pt2, const lcVector3& Color)
{
	if (NumDebugLines == LC_MAX_DEBUG_LINES-1)
		return;

	DebugLines[NumDebugLines].pt1 = pt1;
	DebugLines[NumDebugLines].pt2 = pt2;
	DebugLines[NumDebugLines].color = Color;
	NumDebugLines++;
}

#define LC_MAX_DEBUG_QUADS 100

struct LC_DEBUG_QUAD
{
	lcVector3 pt1;
	lcVector3 pt2;
	lcVector3 pt3;
	lcVector3 pt4;
	lcVector4 color;
};

static LC_DEBUG_QUAD DebugQuads[LC_MAX_DEBUG_QUADS];
static int NumDebugQuads;

void ClearDebugQuads()
{
	NumDebugQuads = 0;
}

void AddDebugQuad(const lcVector3& pt1, const lcVector3& pt2, const lcVector3& pt3, const lcVector3& pt4, const lcVector4& Color)
{
	if (NumDebugQuads == LC_MAX_DEBUG_QUADS-1)
		return;

	DebugQuads[NumDebugQuads].pt1 = pt1;
	DebugQuads[NumDebugQuads].pt2 = pt2;
	DebugQuads[NumDebugQuads].pt3 = pt3;
	DebugQuads[NumDebugQuads].pt4 = pt4;
	DebugQuads[NumDebugQuads].color = Color;
	NumDebugQuads++;
}

void RenderDebugPrimitives()
{
	for (int i = 0; i < NumDebugLines; i++)
	{
		glVertexPointer(3, GL_FLOAT, 0, &DebugLines[i].pt1);
		glColor3fv((float*)&DebugLines[i].color);
		glDrawArrays(GL_LINES, 0, 2);
	}

	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	for (int i = 0; i < NumDebugQuads; i++)
	{
		glVertexPointer(3, GL_FLOAT, 0, &DebugQuads[i].pt1);
		glColor4fv((float*)&DebugQuads[i].color);
		glDrawArrays(GL_QUADS, 0, 4);
	}

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

#endif // LC_DEBUG
