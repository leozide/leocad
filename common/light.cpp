// Light object.

#ifdef LC_WINDOWS
#include "stdafx.h"
#endif
#include <stdlib.h>
#include "boundbox.h"
#include "light.h"
#include "defines.h"

/////////////////////////////////////////////////////////////////////////////
// Camera construction/destruction

Light::Light()
{
	m_BoundingBox.Initialize(this, LC_LIGHT);
	m_TargetBoundingBox.Initialize(this, LC_LIGHT_TARGET);
	m_pNext = NULL;
	m_nState = 0;
}

Light::~Light()
{
	RemoveKeys();
}

void Light::RemoveKeys()
{

}

void Light::MinIntersectDist(CLICKLINE* pLine)
{
	double dist;

	if (m_nState & LC_LIGHT_HIDDEN)
		return;

	dist = m_BoundingBox.FindIntersectDist(pLine);

	if (dist < pLine->mindist)
	{
		pLine->mindist = dist;
		pLine->pClosest = &m_BoundingBox;
	}
	
	dist = m_TargetBoundingBox.FindIntersectDist(pLine);

	if (dist < pLine->mindist)
	{
		pLine->mindist = dist;
		pLine->pClosest = &m_TargetBoundingBox;
	}
}

void Light::UpdatePosition(unsigned short nTime, bool bAnimation)
{

}

/*
	glNewList (LC_LIGHT_LIST, GL_COMPILE);
	float radius = 0.2f;
	int slices = 6, stacks = 6;
	float rho, drho, theta, dtheta;
	float x, y, z;
	int j, imin, imax;
	drho = 3.1415926536f/(float)stacks;
	dtheta = 2.0f*3.1415926536f/(float)slices;

	// draw +Z end as a triangle fan
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0, 0.0, radius);
	for (j = 0; j <= slices; j++) 
	{
		theta = (j == slices) ? 0.0f : j * dtheta;
		x = (float)(-sin(theta) * sin(drho));
		y = (float)(cos(theta) * sin(drho));
		z = (float)(cos(drho));
		glVertex3f(x*radius, y*radius, z*radius);
	}
	glEnd();

	imin = 1;
	imax = stacks-1;

	for (i = imin; i < imax; i++)
	{
		rho = i * drho;
		glBegin(GL_QUAD_STRIP);
		for (j = 0; j <= slices; j++)
		{
			theta = (j == slices) ? 0.0f : j * dtheta;
			x = (float)(-sin(theta) * sin(rho));
			y = (float)(cos(theta) * sin(rho));
			z = (float)(cos(rho));
			glVertex3f(x*radius, y*radius, z*radius);
			x = (float)(-sin(theta) * sin(rho+drho));
			y = (float)(cos(theta) * sin(rho+drho));
			z = (float)(cos(rho+drho));
            glVertex3f(x*radius, y*radius, z*radius);
		}
		glEnd();
	}

	// draw -Z end as a triangle fan
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0.0, 0.0, -radius);
	rho = 3.1415926536f - drho;
	for (j = slices; j >= 0; j--)
	{
		theta = (j==slices) ? 0.0f : j * dtheta;
		x = (float)(-sin(theta) * sin(rho));
		y = (float)(cos(theta) * sin(rho));
        z = (float)(cos(rho));
        glVertex3f(x*radius, y*radius, z*radius);
	}
	glEnd();
	glEndList();

	glNewList (LC_TARGET_LIST, GL_COMPILE);
	glEnableClientState(GL_VERTEX_ARRAY);
	float box[24][3] = { { 0.2f, 0.2f, 0.2f },
		{ -0.2f, 0.2f, 0.2f },	{ -0.2f, -0.2f, 0.2f }, { 0.2f, -0.2f, 0.2f },
		{ 0.2f, 0.2f, -0.2f },	{ 0.2f, -0.2f, -0.2f },	{ -0.2f, -0.2f, -0.2f },
		{ -0.2f, 0.2f, -0.2f },	{ -0.2f, -0.2f, 0.2f }, { -0.2f, 0.2f, 0.2f },
		{ -0.2f, 0.2f, -0.2f },	{ -0.2f, -0.2f, -0.2f },{ 0.2f, -0.2f, -0.2f },
		{ 0.2f, 0.2f, -0.2f },	{ 0.2f, 0.2f, 0.2f },	{ 0.2f, -0.2f, 0.2f }, 
		{ 0.2f, 0.2f, -0.2f },	{ -0.2f, 0.2f, -0.2f },	{ -0.2f, 0.2f, 0.2f },
		{ 0.2f, 0.2f, 0.2f },	{ 0.2f, -0.2f, 0.2f },	{ -0.2f, -0.2f, 0.2f },
		{ -0.2f, -0.2f, -0.2f },{ 0.2f, -0.2f, -0.2f } };
	glVertexPointer (3, GL_FLOAT, 0, box);
	glDrawArrays(GL_QUADS, 0, 24);
	glEndList();
*/


/*
typedef enum { LK_POSITION, LK_TARGET } LK_TYPES;

typedef struct LIGHT_KEY {
	WORD		time;
	float		param[3];
	BYTE		type;
	LIGHT_KEY *next;
} LIGHT_KEY;

class CLight
{
public:
	CLight();
	CLight(float px, float py, float pz);
	~CLight();

protected:
	// Position
	LIGHT_KEY *m_Instructions;
	LIGHT_KEY *m_Animator;

	BOOL m_bDirectional;

public:
	void Render();
	void CalculateBoundingBox(WORD nTime, BOOL bAnimator);
	void CalculatePosition(WORD nTime, BOOL bAnimator, float pos[3], float target[3]);
	void ChangeKey(WORD nTime, BOOL bAnimator, BOOL bAddKey, float param[3], int nKeyType);
	void UpdateInformation(WORD nTime, BOOL bAnimator);
	void InterpolateKey(LIGHT_KEY* pStart, LIGHT_KEY* pEnd, WORD nTime, float key[3]);

	float m_fPosition[4];
};

static LIGHT_KEY* AddNode (LIGHT_KEY *node, WORD nTime, BYTE nType)
{
	LIGHT_KEY* newnode = (LIGHT_KEY*)malloc(sizeof(LIGHT_KEY));

	if (node)
	{
		newnode->next = node->next;
		node->next = newnode;
	}
	else
		newnode->next = NULL;

	newnode->type = nType;
	newnode->time = nTime;
	newnode->param[0] = newnode->param[1] = newnode->param[2] = 0;

	return newnode;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// new positional light
CLight::CLight(float px, float py, float pz)
{
	m_bHidden = FALSE;
	m_bSelected = TRUE;
	m_bFocused = TRUE;
	m_bTargetSelected = FALSE;
	m_bTargetFocused = FALSE;
	m_bDirectional = FALSE;
	m_bEnabled = TRUE;

	m_Animator = AddNode (NULL, 1, LK_POSITION);
	m_Animator->param[0] = px;
	m_Animator->param[1] = py;
	m_Animator->param[2] = pz;

	m_Instructions = AddNode (NULL, 1, LK_POSITION);
	m_Instructions->param[0] = px;
	m_Instructions->param[1] = py;
	m_Instructions->param[2] = pz;

	m_fPosition[0] = px;
	m_fPosition[1] = py;
	m_fPosition[2] = pz;
	m_fPosition[3] = 1;
}

void CLight::MinIntersectDist(CLICKLINE* Line)
{
	m_PositionBox.MinIntersectDist(Line, FALSE);
	if (m_bDirectional)
		m_TargetBox.MinIntersectDist(Line, FALSE);
}

void CLight::CalculateBoundingBox(WORD nTime, BOOL bAnimator)
{
	float pos[3], target[3];
	CalculatePosition(nTime, bAnimator, pos, target);
	m_PositionBox.CalculateBoundingBox(pos);
	if (m_bDirectional)
		m_TargetBox.CalculateBoundingBox(target);
}

void CLight::RemoveKeys()
{
	LIGHT_KEY *node = m_Instructions;
	
	while (node)
	{
		LIGHT_KEY *prev = node;
		node = node->next;
		free (prev);
	}

	node = m_Animator;
	
	while (node)
	{
		LIGHT_KEY *prev = node;
		node = node->next;
		free (prev);
	}
}

void CLight::CalculatePosition(WORD nTime, BOOL bAnimator, float pos[3], float target[3])
{
	LIGHT_KEY *node, *pp = NULL, *np = NULL, *pt = NULL, *nt = NULL;
	if (bAnimator)
		node = m_Animator;
	else
		node = m_Instructions;

	while (node && (!np || !nt))
	{
		if (node->time <= nTime)
		{
			if (node->type == LK_POSITION)
				pp = node;
			else
				pt = node;
		}
		else
		{
			if (node->type == LK_POSITION)
			{
				if (np == NULL)
					np = node;
			}
			else
			{
				if (nt == NULL)
					nt = node;
			}
		}

		node = node->next;
	}

	if (bAnimator && (np != NULL) && (pp->time != nTime))
		InterpolateKey(pp, np, nTime, pos);
	else
		memcpy (pos, pp->param, sizeof(float[3]));

	if (m_bDirectional)
	{
		if (bAnimator && (nt != NULL) && (pt->time != nTime))
			InterpolateKey(pt, nt, nTime, target);
		else
			memcpy (target, pt->param, sizeof(float[3]));
	}
}

void CLight::InterpolateKey(LIGHT_KEY* pStart, LIGHT_KEY* pEnd, WORD nTime, float key[3])
{
// USE KEY IN/OUT WEIGHTS
	float t = (float)(nTime - pStart->time)/(pEnd->time - pStart->time);
	key[0] = pStart->param[0] + (pEnd->param[0] - pStart->param[0])*t;
	key[1] = pStart->param[1] + (pEnd->param[1] - pStart->param[1])*t;
	key[2] = pStart->param[2] + (pEnd->param[2] - pStart->param[2])*t;
}

void CLight::ChangeKey(WORD nTime, BOOL bAnimator, BOOL bAddKey, float param[3], int nKeyType)
{
	LIGHT_KEY *node, *poskey = NULL, *newpos = NULL;
	if (bAnimator)
		node = m_Animator;
	else
		node = m_Instructions;

	while (node)
	{
		if ((node->time <= nTime) &&
			(node->type == nKeyType))
				poskey = node;

		node = node->next;
	}

	if (bAddKey)
	{
		if (poskey)
		{
			if (poskey->time != nTime)
				newpos = AddNode(poskey, nTime, nKeyType);
		}
		else
			newpos = AddNode(poskey, nTime, nKeyType);
	}

	if (newpos == NULL)
		newpos = poskey;

	newpos->param[0] = param[0];
	newpos->param[1] = param[1];
	newpos->param[2] = param[2];
}

void CLight::UpdateInformation(WORD nTime, BOOL bAnimator)
{
	if (m_bDirectional)
	{
///////
		m_fPosition[3] = 0;
	}
	else
	{
		CalculatePosition(nTime, bAnimator, m_fPosition, NULL);
		m_fPosition[3] = 1;
	}
//		CalculateBoundingBox();
}

void CLight::Render()
{
	if (m_bDirectional)
	{
	}
	else
	{
		// Draw a small ball at the light source.
		glPushMatrix();
		glTranslatef(m_fPosition[0], m_fPosition[1], m_fPosition[2]);
		glCallList(LC_LIGHT_LIST);
		glPopMatrix();
	}
}
*/
