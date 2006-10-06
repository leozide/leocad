#include "opengl.h"
#include "debug.h"
#include "view.h"
#include "texfont.h"
#include "lc_application.h"
#include "project.h"

#ifdef LC_PROFILE

lcRenderStats g_RenderStats;

void lcRenderProfileStats(View* view)
{
	TexFont* Font = lcGetActiveProject()->GetFont();

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, view->GetWidth(), 0, view->GetHeight(), -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375, 0.375, 0.0);

	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	Font->MakeCurrent();
	glEnable(GL_ALPHA_TEST);

	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);

	char str[256];

	float Left = (float)view->GetWidth() - 125;
	float Top = (float)view->GetHeight() - 6;

	sprintf(str, "Quads: %d", g_RenderStats.QuadCount);
	Font->PrintText(Left, Top, 0.0f, str);
	Top -= Font->GetFontHeight();

	sprintf(str, "Tris: %d", g_RenderStats.TriCount);
	Font->PrintText(Left, Top, 0.0f, str);
	Top -= Font->GetFontHeight();

	sprintf(str, "Lines: %d", g_RenderStats.LineCount);
	Font->PrintText(Left, Top, 0.0f, str);
	Top -= Font->GetFontHeight();

	sprintf(str, "Render: %d ms", g_RenderStats.RenderMS);
	Font->PrintText(Left, Top, 0.0f, str);
	Top -= Font->GetFontHeight();

	glEnd();

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);
}

#endif

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

	for (i = 0; i < NumDebugQuads; i++)
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
