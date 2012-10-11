// Terrain: a Bezier surface.
//

#include "lc_global.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "opengl.h"
#include "terrain.h"
#include "lc_file.h"
#include "camera.h"
#include "system.h"
#include "lc_texture.h"

/////////////////////////////////////////////////////////////////////////////
// Static functions

// Cubic Bezier patch matrix:
//  1  0  0  0
// -3  3  0  0
//  3 -6  3  0
// -1  3 -3  1
static float SolveBase(int i, float t)
{
	switch (i)
	{
	case 0: return (((-t)+3)*t-3)*t+1;
	case 1: return (((3*t)-6)*t+3)*t;
	case 2: return ((-3*t)+3)*t*t;
	case 3: return t*t*t;
	}
	return 0;
}

static float SolveDiff(int i, float t)
{
	switch (i)
	{
	case 0: return ((-3*t)+6)*t-3;
	case 1: return ((9*t)-12)*t+3;
	case 2: return ((-9*t)+6)*t;
	case 3: return 3*t*t;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// TerrainPatch functions

TerrainPatch::TerrainPatch ()
{
	vertex = NULL;
	normals = NULL;
	coords = NULL;
	index = NULL;
	steps = 10;
	visible = true;
}

TerrainPatch::~TerrainPatch ()
{
	FreeMemory ();
}

void TerrainPatch::InitBox(float minX, float maxX, float minY, float maxY, float minZ, float maxZ)
{
	// Now make each corner point for convenient culling.
	corners[0][0] = minX;
	corners[1][0] = minX;
	corners[2][0] = minX;
	corners[3][0] = minX;
	corners[4][0] = maxX;
	corners[5][0] = maxX;
	corners[6][0] = maxX;
	corners[7][0] = maxX;

	corners[0][1] = minY;
	corners[1][1] = minY;
	corners[2][1] = maxY;
	corners[3][1] = maxY;
	corners[4][1] = minY;
	corners[5][1] = minY;
	corners[6][1] = maxY;
	corners[7][1] = maxY;

	corners[0][2] = minZ;
	corners[1][2] = maxZ;
	corners[2][2] = minZ;
	corners[3][2] = maxZ;
	corners[4][2] = minZ;
	corners[5][2] = maxZ;
	corners[6][2] = minZ;
	corners[7][2] = maxZ;
}

bool TerrainPatch::BoxIsOutside(const float plane[4]) const
{
	float planeEqVal;

	for (int i = 0; i < 8; i++)
	{
		planeEqVal = plane[0] * corners[i][0] + plane[1] * corners[i][1] + plane[2] * corners[i][2] + plane[3];

		if (planeEqVal > 0)
			return false;
	}

	return true;
}

#define controlsX(b, a) control[(a*4+b)*3]
#define controlsY(b, a) control[(a*4+b)*3+1]
#define controlsZ(b, a) control[(a*4+b)*3+2]

void TerrainPatch::Tesselate(bool bNormals)
{
	FreeMemory();

	vertex = new float[steps*steps*3];
	if (bNormals)
		normals = new float[steps*steps*3];

	coords = new float[steps*steps*2];

	float invTotalSteps = 1.0f / (steps - 1);

	for (int stepU = 0; stepU < steps; stepU++)
	{
		// Generate the parameter for this step of the curve.
		float u = stepU * invTotalSteps;

		for (int stepV = 0; stepV < steps; stepV++)
		{
			// Generate the parameter for this step of the curve.
			float v = stepV * invTotalSteps;

			// This holds the point we're working on as we add control points' contributions to it.
			float curPt[3] = { 0, 0, 0 };
			float curNorm[3] = { 0, 0, 0 };
			float curUTan[3] = { 0, 0, 0 };
			float curVTan[3] = { 0, 0, 0 };

			// Generate a point on the curve for this step.
			for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
			{
				// Get a few basis function values and products thereof that we'll need.
				float bu = SolveBase(i, u);
				float bv = SolveBase(j, v);
				float dbu = SolveDiff(i, u);
				float dbv = SolveDiff(j, v);
				float bu_bv = bu * bv;
				float bu_dbv = bu * dbv;
				float dbu_bv = dbu * bv;

				// Add this control point's contribution onto the current point.
				curPt[0] += controlsX(i, j) * bu_bv;
				curPt[1] += controlsY(i, j) * bu_bv;
				curPt[2] += controlsZ(i, j) * bu_bv;

				// Add this point's contribution to our u-tangent.
				curUTan[0] += controlsX(i, j) * dbu_bv;
				curUTan[1] += controlsY(i, j) * dbu_bv;
				curUTan[2] += controlsZ(i, j) * dbu_bv;

				// Add this point's contribution to our v-tangent.
				curVTan[0] += controlsX(i, j) * bu_dbv;
				curVTan[1] += controlsY(i, j) * bu_dbv;
				curVTan[2] += controlsZ(i, j) * bu_dbv;
			}

			// Now get our normal as the cross-product of the u and v tangents.
			curNorm[0] = curVTan[1] * curUTan[2] - curVTan[2] * curUTan[1];
			curNorm[1] = curVTan[2] * curUTan[0] - curVTan[0] * curUTan[2];
			curNorm[2] = curVTan[0] * curUTan[1] - curVTan[1] * curUTan[0];

			// Normalize our normal (ouch!)
			float rInv = 1.0f / (float)sqrt(curNorm[0]*curNorm[0] + curNorm[1]*curNorm[1] + curNorm[2]*curNorm[2]);
			curNorm[0] *= rInv;
			curNorm[1] *= rInv;
			curNorm[2] *= rInv;

			// Store these.
			memcpy(&vertex[(stepU+(stepV*steps))*3], curPt, 3*sizeof(float));
			if (bNormals)
				memcpy(&normals[(stepU+(stepV*steps))*3], curNorm, 3*sizeof(float));

			coords[(stepU+(stepV*steps))*2] = u;
			coords[(stepU+(stepV*steps))*2+1] = v;
		}
	}

	index = new unsigned short[(steps-1)*(steps-1)*6];

	for (unsigned short i = 0; i < steps-1; i++)
	for (unsigned short j = 0; j < steps-1; j++)
	{
		index[(i*(steps-1)+j)*6]   = i*steps+j;
		index[(i*(steps-1)+j)*6+1] = (i+1)*steps+j;
		index[(i*(steps-1)+j)*6+2] = i*steps+j+1;
		index[(i*(steps-1)+j)*6+3] = (i+1)*steps+j;
		index[(i*(steps-1)+j)*6+4] = (i+1)*steps+j+1;
		index[(i*(steps-1)+j)*6+5] = i*steps+j+1;
	}
}

#undef controlsX
#undef controlsY
#undef controlsZ

void TerrainPatch::FreeMemory()
{
	if (vertex)
	{
		delete[] vertex;
		vertex = NULL;
	}
	
	if (normals)
	{
		delete[] normals;
		normals = NULL;
	}

	if (coords)
	{
		delete[] coords;
		coords = NULL;
	}

	if (index)
	{
		delete[] index;
		index = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Terrain construction/destruction

Terrain::Terrain()
{
	m_uPatches = 0;
	m_vPatches = 0;
	m_uSize = 50;
	m_vSize = 50;
	m_Patches = NULL;
	m_pControl = NULL;
	m_nOptions = 0;
	m_pTexture = new Texture();
}

Terrain::~Terrain()
{
	FreeMemory();
	delete m_pTexture;
}

/////////////////////////////////////////////////////////////////////////////
// Terrain functions

void Terrain::FileLoad(lcFile* file)
{
	lcuint8 ch;
	lcuint16 sh;
	lcint32 i, j;

	file->ReadU8(&ch, 1);
	file->ReadS32(&i, 1);
	file->ReadS32(&j, 1);
	file->ReadFloats(&m_uSize, 1);
	file->ReadFloats(&m_vSize, 1);
	file->ReadU32(&m_nOptions, 1);
	file->ReadFloats(m_fColor, 3);

	if (ch == 1)
	{
		file->ReadU8(&ch, 1);
		sh = ch;
	}
	else
		file->ReadU16(&sh, 1);

	if (sh > LC_MAXPATH)
		file->Seek (sh, SEEK_CUR);
	else
		file->ReadBuffer(&m_strTexture, sh);

	SetPatchCount(i, j);
	for (i = 0; i < GetCountU(); i++)
		for (j = 0; j < GetCountV(); j++)
			file->ReadFloats(&m_pControl[i][j*3+2], 1);
}

void Terrain::FileSave(lcFile* file)
{
  lcuint8 version = 2; // LeoCAD 0.70
  lcuint16 sh;

  file->WriteU8(&version, 1);
  file->WriteS32(&m_uPatches, 1);
  file->WriteS32(&m_vPatches, 1);
  file->WriteFloats(&m_uSize, 1);
  file->WriteFloats(&m_vSize, 1);
  file->WriteU32(&m_nOptions, 1);
  file->WriteFloats(m_fColor, 3);

  sh = strlen (m_strTexture);
  file->WriteU16(&sh, 1);
  file->WriteBuffer(m_strTexture, sh);

  for (int i = 0; i < GetCountU(); i++)
    for (int j = 0; j < GetCountV(); j++)
      file->WriteFloats(&m_pControl[i][j*3+2], 1);
}

void Terrain::FreeMemory()
{
	int i;

	if (m_Patches)
	{
		for (i = 0; i < m_uPatches; i++)
			delete[] m_Patches[i];

		delete[] m_Patches;
		m_Patches = NULL;
	}

	if (m_pControl)
	{
		for (i = 0; i < (m_uPatches*3)+1; i++)
			delete[] m_pControl[i];

		delete[] m_pControl;
		m_pControl = NULL;
	}
}

// Copy terrain info
Terrain& Terrain::operator= (const Terrain& source)
{
	FreeMemory();
	int i;

	m_nOptions = source.m_nOptions;
	strcpy(m_strTexture, source.m_strTexture);
	memcpy(m_fColor, source.m_fColor, sizeof(m_fColor));
	m_uPatches = source.m_uPatches;
	m_vPatches = source.m_vPatches;

	m_Patches = new TerrainPatch*[m_uPatches];
	for (i = 0; i < m_uPatches; i++)
		m_Patches[i] = new TerrainPatch[m_vPatches];

	int uCount = GetCountU(), vCount = GetCountV();

	m_pControl = new float*[uCount];
	for (i = 0; i < uCount; i++)
	{
		m_pControl[i] = new float[vCount*3];
		memcpy(m_pControl[i], source.m_pControl[i], vCount*3*sizeof(float));
	}

	m_uSize = source.m_uSize;
	m_vSize = source.m_vSize;

	SetControlPoints();
	Tesselate();

	return *this;
}

void Terrain::GetSize(float *uSize, float *vSize)
{
	*uSize = m_uSize;
	*vSize = m_vSize;
}

void Terrain::SetSize(float uSize, float vSize)
{
	m_uSize = uSize;
	m_vSize = vSize;

	int i, j, uCount = GetCountU(), vCount = GetCountV();

	for (i = 0; i < uCount; i++)
	for (j = 0; j < vCount; j++)
	{
		m_pControl[i][j*3]   = m_uSize * ((float)i/(uCount-1)-0.5f);
		m_pControl[i][j*3+1] = m_vSize * ((float)j/(vCount-1)-0.5f);
	}

	SetControlPoints();
	Tesselate();
}

void Terrain::GetPatchCount(int *uCount, int *vCount)
{
	*uCount = m_uPatches;
	*vCount = m_vPatches;
}

void Terrain::SetPatchCount(int uPatches, int vPatches)
{
	if (uPatches == m_uPatches && vPatches == m_vPatches)
		return;

	float** oldControl = m_pControl;
	int i, j, uCount = uPatches*3+1, vCount = vPatches*3+1;
	int uCountOld = m_uPatches != 0 ? m_uPatches*3+1 : 0, vCountOld = m_vPatches != 0 ? m_vPatches*3+1 : 0;

	// allocate new arrays
//	if (uPatches != m_uPatches)
		m_pControl = new float*[uCount];

	if (m_vPatches != vPatches)
	{
		for (i = 0; i < uCount; i++)
			m_pControl[i] = new float[vCount*3];
	}
	else
	{
		for (i = 0; (i < uCount) && (i < uCountOld); i++)
			m_pControl[i] = oldControl[i];
		for (i = uCountOld; i < uCount; i++)
			m_pControl[i] = new float[vCount*3];
	}

	// set the points
	for (i = 0; i < uCount; i++)
	{
		for (j = 0; j < vCount; j++)
		{
			m_pControl[i][j*3]   = m_uSize * ((float)i/(uCount-1)-0.5f);
			m_pControl[i][j*3+1] = m_vSize * ((float)j/(vCount-1)-0.5f);

			if (i < uCountOld && j < vCountOld)
				m_pControl[i][j*3+2] = oldControl[i][j*3+2];
			else
				m_pControl[i][j*3+2] = 0;
		}
	}

	if (m_vPatches != vPatches)
	{
		for (i = 0; i < uCountOld; i++)
			delete[] oldControl[i];
	}
	else
	{
		for (i = uCount; i < uCountOld; i++)
			delete[] oldControl[i];
	}

//	if ((uPatches != m_uPatches) && (oldControl != NULL))
		delete[] oldControl;

	if (m_Patches)
	{
		for (i = 0; i < m_uPatches; i++)
		{
			for (j = 0; j < m_vPatches; j++)
				m_Patches[i][j].FreeMemory();

			delete[] m_Patches[i];
		}

		delete[] m_Patches;
	}

	m_uPatches = uPatches;
	m_vPatches = vPatches;

	m_Patches = new TerrainPatch*[m_uPatches];
	for (i = 0; i < m_uPatches; i++)
		m_Patches[i] = new TerrainPatch[m_vPatches];

	SetControlPoints();
	Tesselate();
}

// Set the control points for each patch
void Terrain::SetControlPoints()
{
	int i, j;

	for (i = 0; i < m_uPatches; i++)
	for (j = 0; j < m_vPatches; j++)
	{
///////////
		m_Patches[i][j].FreeMemory();

		float minX = 9999999, maxX = -9999999, minY = 9999999, maxY = -9999999, minZ = 9999999, maxZ = -9999999;

		for (int a = 0; a < 4; a++)
		for (int b = 0; b < 4; b++)
		{
			m_Patches[i][j].control[(a*4+b)*3]   = m_pControl[i*(4-1)+a][(j*(4-1)+b)*3];
			m_Patches[i][j].control[(a*4+b)*3+1] = m_pControl[i*(4-1)+a][(j*(4-1)+b)*3+1];
			m_Patches[i][j].control[(a*4+b)*3+2] = m_pControl[i*(4-1)+a][(j*(4-1)+b)*3+2];

			minX = lcMin(minX, m_Patches[i][j].control[(a*4+b)*3]);
			maxX = lcMax(maxX, m_Patches[i][j].control[(a*4+b)*3]);
			minY = lcMin(minY, m_Patches[i][j].control[(a*4+b)*3+1]);
			maxY = lcMax(maxY, m_Patches[i][j].control[(a*4+b)*3+1]);
			minZ = lcMin(minZ, m_Patches[i][j].control[(a*4+b)*3+2]);
			maxZ = lcMax(maxZ, m_Patches[i][j].control[(a*4+b)*3+2]);
		}
		m_Patches[i][j].InitBox(minX, maxX, minY, maxY, minZ, maxZ);
	}
}

// Generate mesh and store for later use.
void Terrain::Tesselate()
{
	int i, j, a, steps = 10;
	float x, y, z, inv;

	for (i = 0; i < m_uPatches; i++)
		for (j = 0; j < m_vPatches; j++)
			m_Patches[i][j].Tesselate((m_nOptions & LC_TERRAIN_SMOOTH) != 0);

	if ((m_nOptions & LC_TERRAIN_SMOOTH) != 0)
	{
		// fix normals at +u
		for (i = 0; i < m_uPatches-1; i++)
		for (j = 0; j < m_vPatches; j++)
		for (a = 0; a < steps; a++)
		{
			x = m_Patches[i][j].normals[((steps-1)*steps+a)*3] + m_Patches[i+1][j].normals[a*3];
			y = m_Patches[i][j].normals[((steps-1)*steps+a)*3+1] + m_Patches[i+1][j].normals[a*3+1];
			z = m_Patches[i][j].normals[((steps-1)*steps+a)*3+2] + m_Patches[i+1][j].normals[a*3+2];

			inv = 1.0f / (float)sqrt(x*x + y*y + z*z);
			x *= inv;
			y *= inv;
			z *= inv;

			m_Patches[i][j].normals[((steps-1)*steps+a)*3] = x;
			m_Patches[i][j].normals[((steps-1)*steps+a)*3+1] = y;
			m_Patches[i][j].normals[((steps-1)*steps+a)*3+2] = z;
			m_Patches[i+1][j].normals[a*3] = x;
			m_Patches[i+1][j].normals[a*3+1] = y;
			m_Patches[i+1][j].normals[a*3+2] = z;
		}

		// and at +v
		for (i = 0; i < m_uPatches; i++)
		for (j = 0; j < m_vPatches-1; j++)
		for (a = 0; a < steps; a++)
		{
			x = m_Patches[i][j].normals[((steps-1)+a*steps)*3] + m_Patches[i][j+1].normals[(a*steps)*3];
			y = m_Patches[i][j].normals[((steps-1)+a*steps)*3+1] + m_Patches[i][j+1].normals[(a*steps)*3+1];
			z = m_Patches[i][j].normals[((steps-1)+a*steps)*3+2] + m_Patches[i][j+1].normals[(a*steps)*3+2];

			inv = 1.0f / (float)sqrt(x*x + y*y + z*z);
			x *= inv;
			y *= inv;
			z *= inv;

			m_Patches[i][j].normals[((steps-1)+a*steps)*3] = x;
			m_Patches[i][j].normals[((steps-1)+a*steps)*3+1] = y;
			m_Patches[i][j].normals[((steps-1)+a*steps)*3+2] = z;
			m_Patches[i][j+1].normals[(a*steps)*3] = x;
			m_Patches[i][j+1].normals[(a*steps)*3+1] = y;
			m_Patches[i][j+1].normals[(a*steps)*3+2] = z;
		}
	}
}

void Terrain::Render(Camera* pCam, float aspect)
{
	if (m_nOptions & LC_TERRAIN_FLAT)
	{
		const lcVector3& eye = pCam->mPosition;
		glPushMatrix();
		glTranslatef(eye[0], eye[1], 0);
		glScalef(pCam->m_zFar, pCam->m_zFar, 1);

		if (m_nOptions & LC_TERRAIN_TEXTURE)
		{
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			m_pTexture->MakeCurrent();
			glEnable(GL_TEXTURE_2D);

float tw = 15.0f, th = 15.0f;
//	tw = 2*pCam->m_zFar/m_nBackgroundSize;
//	th = 2*pCam->m_zFar/m_nBackgroundSize;

float tx, ty;
tx = (tw*eye[0])/(2*pCam->m_zFar);
ty = (th*eye[1])/(2*pCam->m_zFar);

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			float Quad[20] =
			{
				-1.0f, -1.0f, 0.0f, tx, ty,
				 1.0f, -1.0f, 0.0f, tx + tw, ty,
				 1.0f,  1.0f, 0.0f, tx + tw, ty + th,
				-1.0f,  1.0f, 0.0f, tx, ty + th
			};

			glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), Quad);
			glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), Quad + 3);

			glDrawArrays(GL_QUADS, 0, 4);

			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			glDisable(GL_TEXTURE_2D);
		}
		else
		{
			float Quad[12] =
			{
				-1.0f, -1.0f, 0.0f,
				 1.0f, -1.0f, 0.0f,
				 1.0f,  1.0f, 0.0f,
				-1.0f,  1.0f, 0.0f,
			};

			glColor4f(m_fColor[0], m_fColor[1], m_fColor[2], 1.0f);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Quad);

			glDrawArrays(GL_QUADS, 0, 4);

			glDisableClientState(GL_VERTEX_ARRAY);
		}

		glPopMatrix();
	}
	else
	{
		FindVisiblePatches(pCam, aspect);

		int i, j;
		glColor4f(m_fColor[0], m_fColor[1], m_fColor[2], 1.0f);
		glEnableClientState(GL_VERTEX_ARRAY);

		if (m_nOptions & LC_TERRAIN_TEXTURE)
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			m_pTexture->MakeCurrent();
			glEnable(GL_TEXTURE_2D);
		}

		if (m_nOptions & LC_TERRAIN_SMOOTH)
			glEnableClientState(GL_NORMAL_ARRAY);

		for (i = 0; i < m_uPatches; i++)
		for (j = 0; j < m_vPatches; j++)
		if (m_Patches[i][j].visible)
		{
			glVertexPointer(3, GL_FLOAT, 0, m_Patches[i][j].vertex);

			if (m_nOptions & LC_TERRAIN_SMOOTH)
				glNormalPointer(GL_FLOAT, 0, m_Patches[i][j].normals);

			if (m_nOptions & LC_TERRAIN_TEXTURE)
				glTexCoordPointer(2, GL_FLOAT, 0, m_Patches[i][j].coords);

			glDrawElements(GL_TRIANGLES, (m_Patches[i][j].steps-1)*(m_Patches[i][j].steps-1)*6, GL_UNSIGNED_SHORT, m_Patches[i][j].index);
		}

		if (m_nOptions & LC_TERRAIN_SMOOTH)
			glDisableClientState(GL_NORMAL_ARRAY);

		if (m_nOptions & LC_TERRAIN_TEXTURE)
		{
			glDisable(GL_TEXTURE_2D);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

void Terrain::FindVisiblePatches(Camera* pCam, float aspect)
{
	// Get camera position information.
	const lcVector3& eye = pCam->mPosition;

	// Get perspective information.
	float alpha = pCam->m_fovy / 2.0f * LC_DTOR;
	float halfFovY = pCam->m_fovy / 2.0f * LC_DTOR;
	float halfFovX = (float)atan(tan(halfFovY) * aspect);
	float beta = 2.0f * halfFovX;

	// Get vector stuff from the position.
	const lcVector3& nonOrthoTop = pCam->mUpVector;
	const lcVector3& target = pCam->mTargetPosition;
	float front[3] = { target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]};
	lcVector3 side;
	side[0] = nonOrthoTop[1]*front[2] - nonOrthoTop[2]*front[1];
	side[1] = nonOrthoTop[2]*front[0] - nonOrthoTop[0]*front[2];
	side[2] = nonOrthoTop[0]*front[1] - nonOrthoTop[1]*front[0];
	
	// Make sure our up vector is orthogonal.
	lcVector3 top;
	top[0] = front[1]*side[2] - front[2]*side[1];
	top[1] = front[2]*side[0] - front[0]*side[2];
	top[2] = front[0]*side[1] - front[1]*side[0];
	
	// Get our plane normals.
	lcMatrix44 mat;
	lcVector3 topNormal(-top[0], -top[1], -top[2]);
	mat = lcMatrix44FromAxisAngle(side, -alpha);
	topNormal = lcMul30(topNormal, mat);

	lcVector3 bottomNormal(top);
	mat = lcMatrix44FromAxisAngle(side, alpha);
	bottomNormal = lcMul30(bottomNormal, mat);

	lcVector3 rightNormal(side);
	mat = lcMatrix44FromAxisAngle(top, -beta);
	rightNormal = lcMul30(rightNormal, mat);

	lcVector3 leftNormal(-side[0], -side[1], -side[2]);
	mat = lcMatrix44FromAxisAngle(top, beta);
	leftNormal = lcMul30(leftNormal, mat);

	float nearNormal[3] = { front[0], front[1], front[2] };

	// Now calculate our plane offsets from the normals and the eye position.
	float topD = eye[0]*-topNormal[0] + eye[1]*-topNormal[1] + eye[2]*-topNormal[2];
	float bottomD = eye[0]*-bottomNormal[0] + eye[1]*-bottomNormal[1] + eye[2]*-bottomNormal[2];
	float leftD = eye[0]*-leftNormal[0] + eye[1]*-leftNormal[1] + eye[2]*-leftNormal[2];
	float rightD = eye[0]*-rightNormal[0] + eye[1]*-rightNormal[1] + eye[2]*-rightNormal[2];
	float nearD = eye[0]*-nearNormal[0] + eye[1]*-nearNormal[1] + eye[2]*-nearNormal[2];
	
	// For the far plane, find the point farDist away from the eye along the front vector.
	float farDist = pCam->m_zFar;
	float farPt[3] = { front[0], front[1], front[2] };
	float invR = farDist/(float)sqrt(farPt[0]*farPt[0]+farPt[1]*farPt[1]+farPt[2]*farPt[2]);
	farPt[0] = farPt[0]*invR;
	farPt[1] = farPt[1]*invR;
	farPt[2] = farPt[2]*invR;
	farPt[0] += eye[0];
	farPt[1] += eye[1];
	farPt[2] += eye[2];
	float farD = farPt[0]*nearNormal[0] + farPt[1]*nearNormal[1] + farPt[2]*nearNormal[2];
	
	// Now generate the planes
	invR = 1.0f/(float)sqrt(topNormal[0]*topNormal[0]+topNormal[1]*topNormal[1]+topNormal[2]*topNormal[2]);
	float topPlane[4] = { topNormal[0]*invR, topNormal[1]*invR, topNormal[2]*invR, topD*invR };
	invR = 1.0f/(float)sqrt(bottomNormal[0]*bottomNormal[0]+bottomNormal[1]*bottomNormal[1]+bottomNormal[2]*bottomNormal[2]);
	float bottomPlane[4] = { bottomNormal[0]*invR, bottomNormal[1]*invR, bottomNormal[2]*invR, bottomD*invR };
	invR = 1.0f/(float)sqrt(leftNormal[0]*leftNormal[0]+leftNormal[1]*leftNormal[1]+leftNormal[2]*leftNormal[2]);
	float leftPlane[4] = { leftNormal[0]*invR, leftNormal[1]*invR, leftNormal[2]*invR, leftD*invR };
	invR = 1.0f/(float)sqrt(rightNormal[0]*rightNormal[0]+rightNormal[1]*rightNormal[1]+rightNormal[2]*rightNormal[2]);
	float rightPlane[4] = { rightNormal[0]*invR, rightNormal[1]*invR, rightNormal[2]*invR, rightD*invR };
	invR = 1.0f/(float)sqrt(nearNormal[0]*nearNormal[0]+nearNormal[1]*nearNormal[1]+nearNormal[2]*nearNormal[2]);
	float nearPlane[4] = { nearNormal[0]*invR, nearNormal[1]*invR, nearNormal[2]*invR, nearD*invR };
	invR = 1.0f/(float)sqrt(-nearNormal[0]*-nearNormal[0]+-nearNormal[1]*-nearNormal[1]+-nearNormal[2]*-nearNormal[2]);
	float farPlane[4] = { -nearNormal[0]*invR, -nearNormal[1]*invR, -nearNormal[2]*invR, farD*invR };

	for (int i = 0; i < m_uPatches; i++)
	{
		for (int j = 0; j < m_vPatches; j++)
		{
			m_Patches[i][j].visible = true;

			if (m_Patches[i][j].BoxIsOutside(leftPlane))
			{
				m_Patches[i][j].visible = false;
				continue;
			}

			if (m_Patches[i][j].BoxIsOutside(rightPlane))
			{
				m_Patches[i][j].visible = false;
				continue;
			}

			if (m_Patches[i][j].BoxIsOutside(nearPlane))
			{
				m_Patches[i][j].visible = false;
				continue;
			}

			if (m_Patches[i][j].BoxIsOutside(farPlane))
			{
				m_Patches[i][j].visible = false;
				continue;
			}

			if (m_Patches[i][j].BoxIsOutside(bottomPlane))
			{
				m_Patches[i][j].visible = false;
				continue;
			}

			if (m_Patches[i][j].BoxIsOutside(topPlane))
			{
				m_Patches[i][j].visible = false;
				continue;
			}
		}
	}
}

void Terrain::LoadDefaults(bool bLinear)
{
	unsigned long rgb = Sys_ProfileLoadInt ("Default", "Floor", RGB (0,191,0));
	m_fColor[0] = (float)((unsigned char) (rgb))/255;
	m_fColor[1] = (float)((unsigned char) (((unsigned short) (rgb)) >> 8))/255;
	m_fColor[2] = (float)((unsigned char) ((rgb) >> 16))/255;

	m_uSize = 50;
	m_vSize = 50;

	strcpy (m_strTexture, Sys_ProfileLoadString ("Default", "FloorBMP", ""));
	m_pTexture->Unload();

	m_nOptions = LC_TERRAIN_FLAT;
	if (strlen(m_strTexture))
	{
		m_nOptions |= LC_TERRAIN_TEXTURE;
		LoadTexture(bLinear);
	}

	SetPatchCount(4, 4);

	for (int i = 0; i < 13; i++)
	for (int j = 0; j < 13; j++)
	{
		m_pControl[i][j*3]   = m_uSize * ((float)i/12-0.5f);
		m_pControl[i][j*3+1] = m_vSize * ((float)j/12-0.5f);
		m_pControl[i][j*3+2] = 0;
	}

	SetControlPoints();
	Tesselate();
}

void Terrain::LoadTexture(bool bLinear)
{
	m_pTexture->Unload();

	if ((m_nOptions & LC_TERRAIN_TEXTURE) == 0)
		return;

	if (m_pTexture->LoadFromFile(m_strTexture, bLinear) == false)
	{
//		AfxMessageBox("Could not load terrain texture.", MB_OK|MB_ICONEXCLAMATION);
		m_nOptions &= ~LC_TERRAIN_TEXTURE;
	}
}

void Terrain::GenerateRandom()
{
	srand((unsigned)time(NULL));

	int uCount = (m_uPatches*3)+1, vCount = (m_vPatches*3)+1;

	for (int i = 0; i < uCount; i++)
	for (int j = 0; j < vCount; j++)
	{
		m_pControl[i][j*3]   = m_uSize * ((float)i/(uCount-1)-0.5f);
		m_pControl[i][j*3+1] = m_vSize * ((float)j/(vCount-1)-0.5f);
		m_pControl[i][j*3+2] = (((float)rand()/(float)RAND_MAX) - 0.5f) * 8;
	}

	SetControlPoints();
	Tesselate();
}
