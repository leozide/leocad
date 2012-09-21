//
// 4x4 Matrix class
//

#include "lc_global.h"
#include <memory.h>
#include <math.h>
#include <string.h>
#include "matrix.h"
#include "lc_math.h"

// =============================================================================
// static functions

static float Identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

// Perform a 4x4 matrix multiplication  (product = a x b).
// WARNING: (product != b) assumed
static void matmul (float *product, const float *a, const float *b)
{
  int i;

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]

  for (i = 0; i < 4; i++) 
  {
    float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
    P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
    P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
    P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
    P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
  }

#undef A
#undef B
#undef P
}

// Generate a 4x4 transformation matrix from rotation parameters.
static void rotation_matrix (double angle, float x, float y, float z, float m[] )
{
  float s, c, mag, xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;

  s = (float)sin (angle * LC_DTOR);
  c = (float)cos (angle * LC_DTOR);
  mag = (float)sqrt(x*x + y*y + z*z);

  if (mag == 0) 
  {
    // generate an identity matrix and return
    memcpy (m, Identity, sizeof(float[16]));
    return;
  }

  x /= mag;
  y /= mag;
  z /= mag;

  xx = x * x;
  yy = y * y;
  zz = z * z;
  xy = x * y;
  yz = y * z;
  zx = z * x;
  xs = x * s;
  ys = y * s;
  zs = z * s;
  one_c = 1.0f - c;

  m[0] = (one_c * xx) + c;
  m[4] = (one_c * xy) - zs;
  m[8] = (one_c * zx) + ys;
  m[12]= 0;

  m[1] = (one_c * xy) + zs;
  m[5] = (one_c * yy) + c;
  m[9] = (one_c * yz) - xs;
  m[13]= 0;

  m[2] = (one_c * zx) - ys;
  m[6] = (one_c * yz) + xs;
  m[10]= (one_c * zz) + c;
  m[14]= 0;

  m[3] = 0;
  m[7] = 0;
  m[11]= 0;
  m[15]= 1;
}

// =============================================================================
// Matrix class

Matrix::Matrix ()
{
  LoadIdentity();
}

// Create a matrix from axis-angle and a point
Matrix::Matrix (const float *rot, const float *pos)
{
	float tmp[4] = { rot[0], rot[1], rot[2], rot[3]*LC_DTOR };
	float q[4];
	float length, cosA, sinA;
	length = (float)sqrt(tmp[0]*tmp[0] + tmp[1]*tmp[1] + tmp[2]*tmp[2]);
	
	// if zero vector passed in, just return identity quaternion
	if (length < 1E-5)
	{
		q[0] = 0;
		q[1] = 0;
		q[2] = 0;
		q[3] = 1;
		return;
	}
	
	tmp[0] /= length;
	tmp[1] /= length;
	tmp[2] /= length;
	
	cosA = (float)cos(tmp[3] / 2.0f);
	sinA = (float)sin(tmp[3] / 2.0f);
	
	q[3] = cosA;
	q[0] = sinA * tmp[0];
	q[1] = sinA * tmp[1];
	q[2] = sinA * tmp[2];

	// Now calculate the matrix
	float s,xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;

	s = 2.0f / (q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3]);

	xs = q[0] * s;   ys = q[1] * s;   zs = q[2] * s;
	wx = q[3] * xs;  wy = q[3] * ys;  wz = q[3] * zs;
	xx = q[0] * xs;  xy = q[0] * ys;  xz = q[0] * zs;
	yy = q[1] * ys;  yz = q[1] * zs;  zz = q[2] * zs;

	m[0] = 1.0f - (yy + zz);
	m[4] = xy - wz;
	m[8] = xz + wy;
	m[12]= pos[0];
	
	m[1] = xy + wz;
	m[5] = 1.0f - (xx + zz);
	m[9] = yz - wx;
	m[13]= pos[1];
	
	m[2] = xz - wy;
	m[6] = yz + wx;
	m[10]= 1.0f - (xx + yy);
	m[14]= pos[2];
	
	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;
}

void Matrix::FromFloat (const float* mat)
{
  memcpy (&m[0], mat, sizeof(float[16]));
}

void Matrix::LoadIdentity ()
{
  memcpy (&m[0], &Identity, sizeof(float[16]));
}

float Matrix::Determinant() const
{
	return m[0]*m[5]*m[10] + m[1]*m[6]*m[8] + m[2]*m[4]*m[9] - m[0]*m[6]*m[9] - m[1]*m[4]*m[10] - m[2]*m[5]*m[8];
}

void Matrix::Multiply(const Matrix& m1, const Matrix& m2)
{
	matmul(m, m1.m, m2.m);
}

void Matrix::Rotate (float angle, float x, float y, float z)
{
  float rm[16];

  if (angle == 0.0)
    return;

  rotation_matrix(angle, x, y, z, rm);
  matmul(rm, rm, m);
  memcpy (&m[0], &rm[0], sizeof(rm));
}

void Matrix::RotateCenter (float angle, float x, float y, float z, float px, float py, float pz)
{
  m[12] -= px;
  m[13] -= py;
  m[14] -= pz;

  Rotate (angle, x, y, z);

  m[12] += px;
  m[13] += py;
  m[14] += pz;
}

void Matrix::Translate (float x, float y, float z)
{
  m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
  m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
  m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
  m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

void Matrix::SetTranslation (float x, float y, float z)
{
  m[12] = x;
  m[13] = y;
  m[14] = z;
  m[15] = 1;
}

void Matrix::GetTranslation (float* x, float* y, float* z)
{
  *x = m[12];
  *y = m[13];
  *z = m[14];
}

void Matrix::GetTranslation (float pos[3])
{
  pos[0] = m[12];
  pos[1] = m[13];
  pos[2] = m[14];
}

void Matrix::SetTranslation (float pos[3])
{
  m[12] = pos[0];
  m[13] = pos[1];
  m[14] = pos[2];
  m[15] = 1;
}

void Matrix::CreateOld(float mx, float my, float mz, float rx, float ry, float rz)
{
	LoadIdentity();
	Translate(mx, my, mz);
	
	float rm[16];
	rotation_matrix(rx, 1, 0, 0, rm);
	matmul(m, m, rm);
	rotation_matrix(ry, 0, 1, 0, rm);
	matmul(m, m, rm);
	rotation_matrix(rz, 0, 0, 1, rm);
	matmul(m, m, rm);
}

// Transform a point by a 4x4 matrix. out = m * in
void Matrix::TransformPoint(float out[], const float in[3])
{
	out[0] = m[0]*in[0] + m[4]*in[1] + m[8]*in[2] + m[12];
	out[1] = m[1]*in[0] + m[5]*in[1] + m[9]*in[2] + m[13];
	out[2] = m[2]*in[0] + m[6]*in[1] + m[10]*in[2] + m[14];
}

void Matrix::TransformPoints (float p[], int n)
{
	for (int i = 0; i < n*3; i += 3)
	{
		float tmp[3] = { p[i], p[i+1], p[i+2] };
		TransformPoint (&p[i], tmp);
	}
}

void Matrix::FromLDraw (const float *f)
{
	float trans[16] = { 1,0,0,0, 0,0,-1,0, 0,1,0,0, 0,0,0,1 };
	float t[16] = { 1,0,0,0, 0,0,1,0, 0,-1,0,0, 0,0,0,1 };

	m[0] = f[3];    m[1] = f[6];    m[2] = f[9];    m[3]  = 0.0f;
	m[4] = f[4];    m[5] = f[7];    m[6] = f[10];   m[7]  = 0.0f;
	m[8] = f[5];    m[9] = f[8];    m[10]= f[11];   m[11] = 0.0f;
	m[12]= f[0]/25; m[13]= f[1]/25; m[14]= f[2]/25; m[15] = 1.0f;

	matmul (m, m, t);
	matmul (trans, trans, m);
	memcpy (&m[0], &trans[0], sizeof(m));
}

void Matrix::ToLDraw (float *f) const
{
	float trans[16] = { 1,0,0,0, 0,0,-1,0, 0,1,0,0, 0,0,0,1 };
	float tmp[16] = { 1,0,0,0, 0,0,1,0, 0,-1,0,0, 0,0,0,1 };

	matmul(tmp, tmp, m);
	matmul (tmp, tmp, trans);

	f[0] = m[12]*25; f[1] = -m[14]*25; f[2] = m[13]*25;
	f[3] = tmp[0];   f[4] = tmp[4];    f[5] = tmp[8];
	f[6] = tmp[1];   f[7] = tmp[5];    f[8] = tmp[9];
	f[9] = tmp[2];   f[10]= tmp[6];    f[11]= tmp[10];
}

void Matrix::ToAxisAngle(float *rot) const
{
	Matrix tmp(*this);

	// Normalize.
	float inv;
	inv = 1.0f / sqrtf(tmp.m[0]*tmp.m[0] + tmp.m[1]*tmp.m[1] + tmp.m[2]*tmp.m[2]);
	tmp.m[0] *= inv; tmp.m[1] *= inv; tmp.m[2] *= inv;
	inv = 1.0f / sqrtf(tmp.m[4]*tmp.m[4] + tmp.m[5]*tmp.m[5] + tmp.m[6]*tmp.m[6]);
	tmp.m[4] *= inv; tmp.m[5] *= inv; tmp.m[6] *= inv;
	inv = 1.0f / sqrtf(tmp.m[8]*tmp.m[8] + tmp.m[9]*tmp.m[9] + tmp.m[10]*tmp.m[10]);
	tmp.m[8] *= inv; tmp.m[9] *= inv; tmp.m[10] *= inv;

	// Determinant should be 1 for rotation matrices.
	if (tmp.Determinant() < 0.0f)
	{
		tmp.m[0] *= -1.0f;
		tmp.m[1] *= -1.0f;
		tmp.m[2] *= -1.0f;
	}

	float fTrace = tmp.m[0] + tmp.m[5] + tmp.m[10];
	float fCos = 0.5f * (fTrace - 1.0f);

	rot[3] = acosf(fCos);  // in [0,PI]

	if (rot[3] > 0.01f)
	{
		if (fabs (LC_PI - rot[3]) > 0.01f)
		{
			rot[0] = tmp.m[6] - tmp.m[9];
			rot[1] = tmp.m[8] - tmp.m[2];
			rot[2] = tmp.m[1] - tmp.m[4];

			inv = 1.0f / sqrtf(rot[0]*rot[0] + rot[1]*rot[1] + rot[2]*rot[2]);

			rot[0] *= inv;
			rot[1] *= inv;
			rot[2] *= inv;
		}
		else
		{
			// angle is PI
			float fHalfInverse;
			if (tmp.m[0] >= tmp.m[5])
			{
				// r00 >= r11
				if (tmp.m[0] >= tmp.m[10])
				{
					// r00 is maximum diagonal term
					rot[0] = 0.5f * sqrtf(tmp.m[0] - tmp.m[5] - tmp.m[10] + 1.0f);
					fHalfInverse = 0.5f / rot[0];
					rot[1] = fHalfInverse * tmp.m[4];
					rot[2] = fHalfInverse * tmp.m[8];
				}
				else
				{
					// r22 is maximum diagonal term
					rot[2] = 0.5f * sqrtf(tmp.m[10] - tmp.m[0] - tmp.m[5] + 1.0f);
					fHalfInverse = 0.5f / rot[2];
					rot[0] = fHalfInverse * tmp.m[8];
					rot[1] = fHalfInverse * tmp.m[9];
				}
			}
			else
			{
				// r11 > r00
				if (tmp.m[5] >= tmp.m[10])
				{
					// r11 is maximum diagonal term
					rot[1] = 0.5f * sqrtf(tmp.m[5] - tmp.m[0] - tmp.m[10] + 1.0f);
					fHalfInverse  = 0.5f / rot[1];
					rot[0] = fHalfInverse * tmp.m[4];
					rot[2] = fHalfInverse * tmp.m[9];
				}
				else
				{
					// r22 is maximum diagonal term
					rot[2] = 0.5f * sqrtf(tmp.m[10] - tmp.m[0] - tmp.m[5] + 1.0f);
					fHalfInverse = 0.5f / rot[2];
					rot[0] = fHalfInverse * tmp.m[8];
					rot[1] = fHalfInverse * tmp.m[9];
				}
			}
		}
	}
	else
	{
		// The angle is 0 and the matrix is the identity.  Any axis will
		// work, so just use the z-axis.
		rot[0] = 0.0f;
		rot[1] = 0.0f;
		rot[2] = 1.0f;
	}

	rot[3] *= LC_RTOD;
}
