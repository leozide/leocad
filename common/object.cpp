// Base class for all drawable objects
//

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "object.h"
#include "matrix.h"
#include "vector.h"

// =============================================================================
// Static functions

// Returns in (A,B,C,D) the coefficientes of the plane with the three 
// succesive (in counterclockwise order) vertices p1,p2,p3. 
static void GetPolyCoeffs (float x1, float y1, float z1, float x2, float y2, float z2,
			   float x3, float y3, float z3, float *A, float *B, float *C, float *D)
{
  *A = ((y1-y2)*(z3-z2)) - ((z1-z2)*(y3-y2));
  *B = ((z1-z2)*(x3-x2)) - ((x1-x2)*(z3-z2)); 
  *C = ((x1-x2)*(y3-y2)) - ((y1-y2)*(x3-x2));
  *D = - ((*A)*x1) - ((*B)*y1) - ((*C)*z1);
}

// =============================================================================
// ClickLine structure

double LC_CLICKLINE::PointDistance (float *point)
{
  Vector op ((float)(point[0] - a1), (float)(point[1] - b1), (float)(point[2] - c1));
  Vector d ((float)a2, (float)b2, (float)c2);
  float len = d.Length ();
  d.Normalize ();
  float t = op.Dot (d);

  if (t > 0)
  {
    if (t >= len)
      t = 1;
    else
      t /= len;

    d.Scale (t*len);
    op -= d;
  }

  return op.Length ();
}

// =============================================================================
// Object class

Object::Object (LC_OBJECT_TYPE nType)
{
  //  m_nState = 0;
  //  m_strName[0] = '\0';
  m_nObjectType = nType;

  //  m_pParent = NULL;
  //  m_pNext = NULL;
  //  m_pNextRender = NULL;
}

Object::~Object ()
{

}

// =============================================================================
// BoundingBox stuff

// Find the distance from the object to the beginning of the "click line".
double Object::BoundingBoxIntersectDist (LC_CLICKLINE* pLine) const
{
  double x, y, z;

  if (BoundingBoxIntersectionbyLine (pLine->a1, pLine->b1, pLine->c1, pLine->a2, pLine->b2, pLine->c2, &x, &y, &z))
    return (float)sqrt ((pLine->a1-x)*(pLine->a1-x)+(pLine->b1-y)*(pLine->b1-y)+(pLine->c1-z)*(pLine->c1-z));

  return DBL_MAX;
}

// Returns TRUE if the specified point is inside the bounding box of this object.
bool Object::BoundingBoxPointInside(double x, double y, double z) const
{
  int i = 0;
  while (i < 6 && ((m_fBoxPlanes[0][i]*x + m_fBoxPlanes[1][i]*y + 
		    m_fBoxPlanes[2][i]*z + m_fBoxPlanes[3][i]) <= 0.001)) 
    i++;
  return (i == 6);
}

// Returns TRUE if the line is intersecting any of the planes of the bounding
// box and if this point is also inside this bounding box.
bool Object::BoundingBoxIntersectionbyLine (double a1, double b1, double c1, double a2, double b2,
					    double c2, double *x, double *y, double *z) const
{
  double curr_t = DBL_MAX;
  double t, t1, t2;

  for (int i = 0; i < 6; i++)
  {
    t1 = (m_fBoxPlanes[0][i]*a1 + m_fBoxPlanes[1][i]*b1 + m_fBoxPlanes[2][i]*c1 + m_fBoxPlanes[3][i]);
    t2 = (m_fBoxPlanes[0][i]*a2 + m_fBoxPlanes[1][i]*b2 + m_fBoxPlanes[2][i]*c2);

    if (t1!=0 && t2!=0)
    {
      t = -( t1 / t2 );
      if (t>=0)
      {
	*x=a1+a2*t;
	*y=b1+b2*t;
	*z=c1+c2*t;

	if (BoundingBoxPointInside(*x,*y,*z))
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

// For pieces
void Object::BoundingBoxCalculate (Matrix *mat, float Dimensions[6])
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

  GetPolyCoeffs (pts[3], pts[4], pts[5],  pts[0], pts[1], pts[2],  pts[6], pts[7], pts[8],
		 &m_fBoxPlanes[0][0], &m_fBoxPlanes[1][0], &m_fBoxPlanes[2][0], &m_fBoxPlanes[3][0]); //1,0,2
  GetPolyCoeffs (pts[9], pts[10],pts[11], pts[12],pts[13],pts[14], pts[15],pts[16],pts[17],
		 &m_fBoxPlanes[0][1], &m_fBoxPlanes[1][1], &m_fBoxPlanes[2][1], &m_fBoxPlanes[3][1]); //3,4,5
  GetPolyCoeffs (pts[15],pts[16],pts[17], pts[6], pts[7], pts[8],  pts[0], pts[1], pts[2],
		 &m_fBoxPlanes[0][2], &m_fBoxPlanes[1][2], &m_fBoxPlanes[2][2], &m_fBoxPlanes[3][2]); //5,2,0
  GetPolyCoeffs (pts[12],pts[13],pts[14], pts[9], pts[10],pts[11], pts[3], pts[4], pts[5],
		 &m_fBoxPlanes[0][3], &m_fBoxPlanes[1][3], &m_fBoxPlanes[2][3], &m_fBoxPlanes[3][3]); //4,3,1
  GetPolyCoeffs (pts[6], pts[7], pts[8],  pts[15],pts[16],pts[17], pts[12],pts[13],pts[14],
		 &m_fBoxPlanes[0][4], &m_fBoxPlanes[1][4], &m_fBoxPlanes[2][4], &m_fBoxPlanes[3][4]); //2,5,4
  GetPolyCoeffs (pts[0], pts[1], pts[2],  pts[3], pts[4], pts[5],  pts[9], pts[10],pts[11],
		 &m_fBoxPlanes[0][5], &m_fBoxPlanes[1][5], &m_fBoxPlanes[2][5], &m_fBoxPlanes[3][5]); //0,1,3
}

// Cameras
void Object::BoundingBoxCalculate (Matrix *mat)
{
  float normals[6][3] = {
    { 1,0,0 }, { 0,1,0 }, { 0,0,1 },
    { -1,0,0 }, { 0,-1,0 }, { 0,0,-1 } };
  float x,y,z,dist;

  if (IsCamera ())
    dist = 0.3f;
  else
    dist = 0.2f;

  mat->GetTranslation(&x,&y,&z);
  mat->SetTranslation(0,0,0);
  mat->TransformPoints(&normals[0][0], 6);

  for (int i = 0; i < 6; i++)
  {
    m_fBoxPlanes[0][i] = normals[i][0];
    m_fBoxPlanes[1][i] = normals[i][1];
    m_fBoxPlanes[2][i] = normals[i][2];

    float pt[3];
    pt[0] = dist*normals[i][0] + x;
    pt[1] = dist*normals[i][1] + y;
    pt[2] = dist*normals[i][2] + z;

    m_fBoxPlanes[3][i] = -(pt[0]*normals[i][0]+pt[1]*normals[i][1]+pt[2]*normals[i][2]);
  }
}

// Light
void Object::BoundingBoxCalculate (float pos[3])
{
  float pts[18] = {
     0.3f+pos[0],  0.3f+pos[1], -0.3f+pos[2],
    -0.3f+pos[0],  0.3f+pos[1], -0.3f+pos[2],
     0.3f+pos[0],  0.3f+pos[1],  0.3f+pos[2],
    -0.3f+pos[0], -0.3f+pos[1], -0.3f+pos[2],
    -0.3f+pos[0], -0.3f+pos[1],  0.3f+pos[2],
     0.3f+pos[0], -0.3f+pos[1],  0.3f+pos[2] };

  GetPolyCoeffs (pts[3], pts[4], pts[5],  pts[0], pts[1], pts[2],  pts[6], pts[7], pts[8],
		 &m_fBoxPlanes[0][0], &m_fBoxPlanes[1][0], &m_fBoxPlanes[2][0], &m_fBoxPlanes[3][0]); //1,0,2
  GetPolyCoeffs (pts[9], pts[10],pts[11], pts[12],pts[13],pts[14], pts[15],pts[16],pts[17],
		 &m_fBoxPlanes[0][1], &m_fBoxPlanes[1][1], &m_fBoxPlanes[2][1], &m_fBoxPlanes[3][1]); //3,4,5
  GetPolyCoeffs (pts[15],pts[16],pts[17], pts[6], pts[7], pts[8],  pts[0], pts[1], pts[2],
		 &m_fBoxPlanes[0][2], &m_fBoxPlanes[1][2], &m_fBoxPlanes[2][2], &m_fBoxPlanes[3][2]); //5,2,0
  GetPolyCoeffs (pts[12],pts[13],pts[14], pts[9], pts[10],pts[11], pts[3], pts[4], pts[5],
		 &m_fBoxPlanes[0][3], &m_fBoxPlanes[1][3], &m_fBoxPlanes[2][3], &m_fBoxPlanes[3][3]); //4,3,1
  GetPolyCoeffs (pts[6], pts[7], pts[8],  pts[15],pts[16],pts[17], pts[12],pts[13],pts[14],
		 &m_fBoxPlanes[0][4], &m_fBoxPlanes[1][4], &m_fBoxPlanes[2][4], &m_fBoxPlanes[3][4]); //2,5,4
  GetPolyCoeffs (pts[0], pts[1], pts[2],  pts[3], pts[4], pts[5],  pts[9], pts[10],pts[11],
		 &m_fBoxPlanes[0][5], &m_fBoxPlanes[1][5], &m_fBoxPlanes[2][5], &m_fBoxPlanes[3][5]); //0,1,3
}

