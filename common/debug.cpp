#include "opengl.h"
#include "debug.h"

#ifdef LC_DEBUG

#define LC_MAX_DEBUG_LINES 100

typedef struct
{
	Vector3 pt1;
	Vector3 pt2;
	Vector3 color;
} LC_DEBUG_LINE;

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

typedef struct
{
	Vector3 pt1;
	Vector3 pt2;
	Vector3 pt3;
	Vector3 pt4;
	Vector4 color;
} LC_DEBUG_QUAD;

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
	glBegin(GL_LINES);

	for (int i = 0; i < NumDebugLines; i++)
	{
		glColor3fv((float*)&DebugLines[i].color);
		glVertex3fv((float*)&DebugLines[i].pt1);
		glVertex3fv((float*)&DebugLines[i].pt2);
	}

	glEnd();

	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glBegin(GL_QUADS);

	for (int i = 0; i < NumDebugQuads; i++)
	{
		glColor4fv((float*)&DebugQuads[i].color);
		glVertex3fv((float*)&DebugQuads[i].pt1);
		glVertex3fv((float*)&DebugQuads[i].pt2);
		glVertex3fv((float*)&DebugQuads[i].pt3);
		glVertex3fv((float*)&DebugQuads[i].pt4);
	}

	glEnd();

	glDisable(GL_BLEND);
	glDepthMask(GL_TRUE);
}

#endif // LC_DEBUG
