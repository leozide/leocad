// Simple bounding box.
//

#include <float.h>
#include <math.h>
#include "opengl.h"
#include "boundbox.h"
#include "matrix.h"
#include "defines.h"

// Returns in (A,B,C,D) the coefficientes of the plane with the three 
// succesive (in counterclockwise order) vertices p1,p2,p3. 
static void GetPolyCoeffs(float x1, float y1, float z1, float x2, float y2, float z2,
						  float x3, float y3, float z3, float *A, float *B, float *C, float *D)
{
	*A = ((y1-y2)*(z3-z2)) - ((z1-z2)*(y3-y2));
	*B = ((z1-z2)*(x3-x2)) - ((x1-x2)*(z3-z2)); 
	*C = ((x1-x2)*(y3-y2)) - ((y1-y2)*(x3-x2));
	*D = - ((*A)*x1) - ((*B)*y1) - ((*C)*z1);
}

static void GetFrustumPlanes (float planes[6][4])
{
  // Storage for the Modelview, Projection and their multiplication (Frustum) matrix.
  float mv[16], pj[16], fm[16];

  glGetFloatv(GL_MODELVIEW_MATRIX, mv);
  glGetFloatv(GL_PROJECTION_MATRIX, pj);

  fm[0]  = pj[0] * mv[0]  + pj[4] * mv[1]  + pj[8]  * mv[2]  + pj[12] * mv[3];
  fm[4]  = pj[0] * mv[4]  + pj[4] * mv[5]  + pj[8]  * mv[6]  + pj[12] * mv[7];
  fm[8]  = pj[0] * mv[8]  + pj[4] * mv[9]  + pj[8]  * mv[10] + pj[12] * mv[11];
  fm[12] = pj[0] * mv[12] + pj[4] * mv[13] + pj[8]  * mv[14] + pj[12] * mv[15];
  fm[1]  = pj[1] * mv[0]  + pj[5] * mv[1]  + pj[9]  * mv[2]  + pj[13] * mv[3];
  fm[5]  = pj[1] * mv[4]  + pj[5] * mv[5]  + pj[9]  * mv[6]  + pj[13] * mv[7];
  fm[9]  = pj[1] * mv[8]  + pj[5] * mv[9]  + pj[9]  * mv[10] + pj[13] * mv[11];
  fm[13] = pj[1] * mv[12] + pj[5] * mv[13] + pj[9]  * mv[14] + pj[13] * mv[15];
  fm[2]  = pj[2] * mv[0]  + pj[6] * mv[1]  + pj[10] * mv[2]  + pj[14] * mv[3];
  fm[6]  = pj[2] * mv[4]  + pj[6] * mv[5]  + pj[10] * mv[6]  + pj[14] * mv[7];
  fm[10] = pj[2] * mv[8]  + pj[6] * mv[9]  + pj[10] * mv[10] + pj[14] * mv[11];
  fm[14] = pj[2] * mv[12] + pj[6] * mv[13] + pj[10] * mv[14] + pj[14] * mv[15];
  fm[3]  = pj[3] * mv[0]  + pj[7] * mv[1]  + pj[11] * mv[2]  + pj[15] * mv[3];
  fm[7]  = pj[3] * mv[4]  + pj[7] * mv[5]  + pj[11] * mv[6]  + pj[15] * mv[7];
  fm[11] = pj[3] * mv[8]  + pj[7] * mv[9]  + pj[11] * mv[10] + pj[15] * mv[11];
  fm[15] = pj[3] * mv[12] + pj[7] * mv[13] + pj[11] * mv[14] + pj[15] * mv[15];

  planes[0][0] = (fm[0] - fm[3]) * -1;
  planes[0][1] = (fm[4] - fm[7]) * -1;
  planes[0][2] = (fm[8] - fm[11]) * -1;
  planes[0][3] = (fm[12] - fm[15]) * -1;
  planes[1][0] = fm[0] + fm[3];
  planes[1][1] = fm[4] + fm[7];
  planes[1][2] = fm[8] + fm[11];
  planes[1][3] = fm[12] + fm[15];
  planes[2][0] = (fm[1] - fm[3]) * -1;
  planes[2][1] = (fm[5] - fm[7]) * -1;
  planes[2][2] = (fm[9] - fm[11]) * -1;
  planes[2][3] = (fm[13] - fm[15]) * -1;
  planes[3][0] = fm[1] + fm[3];
  planes[3][1] = fm[5] + fm[7];
  planes[3][2] = fm[9] + fm[11];
  planes[3][3] = fm[13] + fm[15];
  planes[4][0] = (fm[2] - fm[3]) * -1;
  planes[4][1] = (fm[6] - fm[7]) * -1;
  planes[4][2] = (fm[10] - fm[11]) * -1;
  planes[4][3] = (fm[14] - fm[15]) * -1;
  planes[5][0] = fm[2] + fm[3];
  planes[5][1] = fm[6] + fm[7];
  planes[5][2] = fm[10] + fm[11];
  planes[5][3] = fm[14] + fm[15];
}

bool BoxOutsideFrustum (float Dimensions[6])
{
  float d, planes[6][4], verts[8][3] = {
    { Dimensions[0], Dimensions[1], Dimensions[5] },
    { Dimensions[3], Dimensions[1], Dimensions[5] },
    { Dimensions[0], Dimensions[1], Dimensions[2] },
    { Dimensions[3], Dimensions[4], Dimensions[5] },
    { Dimensions[3], Dimensions[4], Dimensions[2] },
    { Dimensions[0], Dimensions[4], Dimensions[2] },
    { Dimensions[0], Dimensions[4], Dimensions[5] },
    { Dimensions[3], Dimensions[1], Dimensions[2] } };

  GetFrustumPlanes (planes);

  for (int i = 0; i < 6; i++)
    for (int j = 0; j < 8; j++)
    {
      d = verts[j][0]*planes[i][0] + verts[j][1]*planes[i][1] + verts[j][2]*planes[i][2] + planes[i][3];
      if (d < -0.001f)
	return true;
    }
  return false;
}

/////////////////////////////////////////////////////////////////////////////
// BoundingBox construction/destruction

BoundingBox::BoundingBox()
{

}

BoundingBox::~BoundingBox()
{

}

/////////////////////////////////////////////////////////////////////////////
// BoundingBox implementation

void BoundingBox::Initialize(void* pOwner, unsigned char nType)
{
	m_pOwner = pOwner;
	m_nParentType = nType;
}

// Find the distance from the object to the beginning of the "click line".
double BoundingBox::FindIntersectDist(CLICKLINE* pLine)
{
	double x, y, z;

	if (IntersectionbyLine(pLine->a1, pLine->b1, pLine->c1, pLine->a2, pLine->b2, pLine->c2, &x, &y, &z))
		return (float)sqrt((pLine->a1-x)*(pLine->a1-x)+(pLine->b1-y)*(pLine->b1-y)+(pLine->c1-z)*(pLine->c1-z));

	return DBL_MAX;
}

// For pieces
void BoundingBox::CalculateBoundingBox(Matrix *mat, float Dimensions[6])
{
	//   BASE       TOP
	// 1------3  .------4  ^ X
	// |      |  |      |  |
	// |      |  |      |  |   Y
	// 0------.  2------5  .--->

	float pts[18] = {
		Dimensions[0], Dimensions[1], Dimensions[5],
		Dimensions[3], Dimensions[1], Dimensions[5],
		Dimensions[0], Dimensions[1], Dimensions[2],
		Dimensions[3], Dimensions[4], Dimensions[5],
		Dimensions[3], Dimensions[4], Dimensions[2],
		Dimensions[0], Dimensions[4], Dimensions[2] };

	mat->TransformPoints(pts, 6);

	GetPolyCoeffs (pts[3], pts[4], pts[5],  pts[0], pts[1], pts[2],  pts[6], pts[7], pts[8],  &m_fPlanes[0][0], &m_fPlanes[1][0], &m_fPlanes[2][0], &m_fPlanes[3][0]); // (1,0,2)
	GetPolyCoeffs (pts[9], pts[10],pts[11], pts[12],pts[13],pts[14], pts[15],pts[16],pts[17], &m_fPlanes[0][1], &m_fPlanes[1][1], &m_fPlanes[2][1], &m_fPlanes[3][1]); // (3,4,5)
	GetPolyCoeffs (pts[15],pts[16],pts[17], pts[6], pts[7], pts[8],  pts[0], pts[1], pts[2],  &m_fPlanes[0][2], &m_fPlanes[1][2], &m_fPlanes[2][2], &m_fPlanes[3][2]); // (5,2,0)
	GetPolyCoeffs (pts[12],pts[13],pts[14], pts[9], pts[10],pts[11], pts[3], pts[4], pts[5],  &m_fPlanes[0][3], &m_fPlanes[1][3], &m_fPlanes[2][3], &m_fPlanes[3][3]); // (4,3,1)
	GetPolyCoeffs (pts[6], pts[7], pts[8],  pts[15],pts[16],pts[17], pts[12],pts[13],pts[14], &m_fPlanes[0][4], &m_fPlanes[1][4], &m_fPlanes[2][4], &m_fPlanes[3][4]); // (2,5,4)
	GetPolyCoeffs (pts[0], pts[1], pts[2],  pts[3], pts[4], pts[5],  pts[9], pts[10],pts[11], &m_fPlanes[0][5], &m_fPlanes[1][5], &m_fPlanes[2][5], &m_fPlanes[3][5]); // (0,1,3)
}

// Cameras
void BoundingBox::CalculateBoundingBox(Matrix *mat)
{
	float normals[6][3] = {
		{ 1,0,0 }, { 0,1,0 }, { 0,0,1 },
		{ -1,0,0 }, { 0,-1,0 }, { 0,0,-1 } };
	float x,y,z,dist;

	if (m_nParentType == LC_CAMERA)
		dist = 0.3f;
	else
		dist = 0.2f;

	mat->GetTranslation(&x,&y,&z);
	mat->SetTranslation(0,0,0);
	mat->TransformPoints(&normals[0][0], 6);

	for (int i = 0; i < 6; i++)
	{
		m_fPlanes[0][i] = normals[i][0];
		m_fPlanes[1][i] = normals[i][1];
		m_fPlanes[2][i] = normals[i][2];

		float pt[3];
		pt[0] = dist*normals[i][0] + x;
		pt[1] = dist*normals[i][1] + y;
		pt[2] = dist*normals[i][2] + z;

		m_fPlanes[3][i] = -(pt[0]*normals[i][0]+pt[1]*normals[i][1]+pt[2]*normals[i][2]);
	}
}

// Light
void BoundingBox::CalculateBoundingBox(float pos[3])
{
	float pts[18] = {
		 0.3f+pos[0],  0.3f+pos[1], -0.3f+pos[2],
		-0.3f+pos[0],  0.3f+pos[1], -0.3f+pos[2],
		 0.3f+pos[0],  0.3f+pos[1],  0.3f+pos[2],
		-0.3f+pos[0], -0.3f+pos[1], -0.3f+pos[2],
		-0.3f+pos[0], -0.3f+pos[1],  0.3f+pos[2],
		 0.3f+pos[0], -0.3f+pos[1],  0.3f+pos[2] };

	GetPolyCoeffs (pts[3], pts[4], pts[5],  pts[0], pts[1], pts[2],  pts[6], pts[7], pts[8],  &m_fPlanes[0][0], &m_fPlanes[1][0], &m_fPlanes[2][0], &m_fPlanes[3][0]); // (1,0,2)
	GetPolyCoeffs (pts[9], pts[10],pts[11], pts[12],pts[13],pts[14], pts[15],pts[16],pts[17], &m_fPlanes[0][1], &m_fPlanes[1][1], &m_fPlanes[2][1], &m_fPlanes[3][1]); // (3,4,5)
	GetPolyCoeffs (pts[15],pts[16],pts[17], pts[6], pts[7], pts[8],  pts[0], pts[1], pts[2],  &m_fPlanes[0][2], &m_fPlanes[1][2], &m_fPlanes[2][2], &m_fPlanes[3][2]); // (5,2,0)
	GetPolyCoeffs (pts[12],pts[13],pts[14], pts[9], pts[10],pts[11], pts[3], pts[4], pts[5],  &m_fPlanes[0][3], &m_fPlanes[1][3], &m_fPlanes[2][3], &m_fPlanes[3][3]); // (4,3,1)
	GetPolyCoeffs (pts[6], pts[7], pts[8],  pts[15],pts[16],pts[17], pts[12],pts[13],pts[14], &m_fPlanes[0][4], &m_fPlanes[1][4], &m_fPlanes[2][4], &m_fPlanes[3][4]); // (2,5,4)
	GetPolyCoeffs (pts[0], pts[1], pts[2],  pts[3], pts[4], pts[5],  pts[9], pts[10],pts[11], &m_fPlanes[0][5], &m_fPlanes[1][5], &m_fPlanes[2][5], &m_fPlanes[3][5]); // (0,1,3)
}

/////////////////////////////////////////////////////////////////////////////
// BoundingBox helpers

// Returns TRUE if the specified point is inside the bounding box of this object.
bool BoundingBox::PointInside(double x, double y, double z)
{
	int i = 0;
	while (i < 6 && ((m_fPlanes[0][i]*x + m_fPlanes[1][i]*y + 
		m_fPlanes[2][i]*z + m_fPlanes[3][i]) <= 0.001)) 
		i++;
	return (i == 6);
}

// Returns TRUE if the line is intersecting any of the planes of the bounding
// box and if this point is also inside this bounding box.
bool BoundingBox::IntersectionbyLine(double a1, double b1, double c1, double a2, double b2, double c2, double *x, double *y, double *z)
{
	double curr_t = DBL_MAX;
	double t, t1, t2;

	for (int i = 0; i < 6; i++)
	{
		t1 = (m_fPlanes[0][i]*a1 + m_fPlanes[1][i]*b1 + m_fPlanes[2][i]*c1 + m_fPlanes[3][i]);
		t2 = (m_fPlanes[0][i]*a2 + m_fPlanes[1][i]*b2 + m_fPlanes[2][i]*c2);

		if (t1!=0 && t2!=0)
		{
			t = -( t1 / t2 );
			if (t>=0)
			{
				*x=a1+a2*t;
				*y=b1+b2*t;
				*z=c1+c2*t;

				if (PointInside(*x,*y,*z))
					if (t < curr_t)
						curr_t = t;
			}
		}
	}
	
	if (curr_t != DBL_MAX)
	{
		*x=a1+a2*curr_t;
		*y=b1+b2*curr_t;
		*z=c1+c2*curr_t;
		return true;
	}
	else
		return false;
}
