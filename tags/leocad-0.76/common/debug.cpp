#include "lc_global.h"
#include "opengl.h"
#include "debug.h"

#ifdef LC_DEBUG

#define LC_MAX_DEBUG_LINES 100

struct LC_DEBUG_LINE
{
	Vector3 pt1;
	Vector3 pt2;
	Vector3 color;
};

static LC_DEBUG_LINE DebugLines[LC_MAX_DEBUG_LINES];
static int NumDebugLines;

void ClearDebugLines()
{
	NumDebugLines = 0;
}

void AddDebugLine(const Vector3& pt1, const Vector3& pt2, const Vector3& Color)
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
	Vector3 pt1;
	Vector3 pt2;
	Vector3 pt3;
	Vector3 pt4;
	Vector4 color;
};

static LC_DEBUG_QUAD DebugQuads[LC_MAX_DEBUG_QUADS];
static int NumDebugQuads;

void ClearDebugQuads()
{
	NumDebugQuads = 0;
}

void AddDebugQuad(const Vector3& pt1, const Vector3& pt2, const Vector3& pt3, const Vector3& pt4, const Vector4& Color)
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
	glEnableClientState(GL_VERTEX_ARRAY);

	for (int i = 0; i < NumDebugLines; i++)
	{
		glColor3fv((float*)&DebugLines[i].color);
		glVertexPointer(3, GL_FLOAT, sizeof(DebugLines[i].pt1), &DebugLines[i].pt1);
		glDrawArrays(GL_LINES, 0, 2);
	}

	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	for (int q = 0; q < NumDebugQuads; q++)
	{
		glColor4fv((float*)&DebugQuads[q].color);
		glVertexPointer(3, GL_FLOAT, sizeof(DebugQuads[q].pt1), &DebugQuads[q].pt1);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

#endif // LC_DEBUG
