// Matrix class
//

#include <memory.h>
#include <math.h>
#include <string.h>
#include "matrix.h"
#include "file.h"
#include "defines.h"

static float Identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

// Perform a 4x4 matrix multiplication  (product = a x b).
// WARNING: (product != b) assumed
static void matmul(float *product, const float *a, const float *b)
{
	int i;

//	#define M(row,col)  m[col*4+row]
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
static void rotation_matrix(double angle, float x, float y, float z, float m[] )
{
	float s, c, mag, xx, yy, zz, xy, yz, zx, xs, ys, zs, one_c;
	
	s = (float)sin (angle * DTOR);
	c = (float)cos (angle * DTOR);
	mag = (float)sqrt(x*x + y*y + z*z);
	
	if (mag == 0) 
	{
		// generate an identity matrix and return
		memcpy(m, Identity, sizeof(float[16]));
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
/*
double PointDistance (double x1, double y1, double z1, double x2, double y2, double z2)
{
	return sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)+(z2-z1)*(z2-z1));
}
*/
Matrix::Matrix()
{
	LoadIdentity();
}

// Expand from the .bin file
Matrix::Matrix(float* floats)
{
	m[0] = floats[0];
	m[1] = floats[1];
	m[2] = floats[2];
	m[3] = 0.0f;
	m[4] = floats[3];
	m[5] = floats[4];
	m[6] = floats[5];
	m[7] = 0.0f;
	m[8] = floats[6];
	m[9] = floats[7];
	m[10] = floats[8];
	m[11] = 0.0f;
	m[12] = floats[9];
	m[13] = floats[10];
	m[14] = floats[11];
	m[15] = 0.0f;
}

Matrix::Matrix(double mat[16])
{
	for (int i = 0; i < 16; i++)
		m[i] = (float)mat[i];
}

//	Create a matrix from axis-angle and a point
Matrix::Matrix(float rot[4], float pos[3])
{
	float tmp[4] = { rot[0], rot[1], rot[2], rot[3]*DTOR };
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

Matrix::~Matrix()
{
}

void Matrix::FromFloat(float* mat)
{
	memcpy (&m[0], mat, sizeof(float[16]));
}

void Matrix::LoadIdentity()
{
	memcpy (&m[0], &Identity, sizeof(float[16]));
}

void Matrix::Multiply(Matrix& m1, Matrix& m2)
{
	matmul (m, m1.m, m2.m);
}

void Matrix::Rotate(float angle, float x, float y, float z)
{
	if (angle == 0.0) return;
	float rm[16];
	rotation_matrix(angle, x, y, z, rm);
	matmul(rm, rm, m);
	memcpy (&m[0], &rm[0], sizeof(rm));

	for (int i = 0; i < 12; i++)
		if (fabs (m[i]) < .001f)
			m[i] = 0;
}

void Matrix::RotateCenter(float angle, float x, float y, float z, float px, float py, float pz)
{
	m[12] -= px;
	m[13] -= py;
	m[14] -= pz;

	Rotate(angle, x, y, z);

	m[12] += px;
	m[13] += py;
	m[14] += pz;
}

void Matrix::Translate(float x, float y, float z)
{
	m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

void Matrix::SetTranslation(float x, float y, float z)
{
	m[12] = x;
	m[13] = y;
	m[14] = z;
	m[15] = 1;
}

void Matrix::GetTranslation(float* x, float* y, float* z)
{
	*x = m[12];
	*y = m[13];
	*z = m[14];
}

void Matrix::GetTranslation(float pos[3])
{
	pos[0] = m[12];
	pos[1] = m[13];
	pos[2] = m[14];
}

void Matrix::SetTranslation(float pos[3])
{
	m[12] = pos[0];
	m[13] = pos[1];
	m[14] = pos[2];
	m[15] = 1;
}

void Matrix::Create(float mx, float my, float mz, float rx, float ry, float rz)
{
	LoadIdentity();
	Translate(mx, my, mz);
	Rotate(rx, 1, 0, 0);
	Rotate(ry, 0, 1, 0);
	Rotate(rz, 0, 0, 1);
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

void Matrix::ConvertFromLDraw(float f[12])
{
	float trans[16] = { 1,0,0,0, 0,0,-1,0, 0,1,0,0, 0,0,0,1 };
	float t[16] = { 1,0,0,0, 0,0,1,0, 0,-1,0,0, 0,0,0,1 };
	m[0] = f[3];	m[1] = f[6];	m[2] = f[9];
	m[4] = f[4];	m[5] = f[7];	m[6] = f[10];
	m[8] = f[5];	m[9] = f[8];	m[10]= f[11];
	m[12]= f[0]/25;	m[13]= f[1]/25;	m[14]= f[2]/25;
	matmul (m, m, t);
	matmul (trans, trans, m);
	memcpy (&m[0], &trans[0], sizeof(m));
}

void Matrix::ConvertToLDraw(float f[12])
{
	float trans[16] = { 1,0,0,0, 0,0,-1,0, 0,1,0,0, 0,0,0,1 };
	float tmp[16] = { 1,0,0,0, 0,0,1,0, 0,-1,0,0, 0,0,0,1 };
	matmul(tmp, tmp, m);
	matmul (tmp, tmp, trans);
	f[0] = m[12]*25;	f[1] = -m[14]*25;	f[2] = m[13]*25;
	f[3] = tmp[0];		f[4] = tmp[4];		f[5] = tmp[8];
	f[6] = tmp[1];		f[7] = tmp[5];		f[8] = tmp[9];
	f[9] = tmp[2];		f[10]= tmp[6];		f[11]= tmp[10];
}

void Matrix::ReadFromFile(File* F)
{
	float tmp[12];
	F->Read (&tmp, sizeof(tmp));
	m[0] = tmp[0];	m[1] = tmp[1];	m[2] = tmp[2];
	m[4] = tmp[3];	m[5] = tmp[4];	m[6] = tmp[5];
	m[8] = tmp[6];	m[9] = tmp[7];	m[10]= tmp[8];
	m[12]= tmp[9];	m[13]= tmp[10];	m[14]= tmp[11];
}

void Matrix::WriteToFile(File* F)
{
	float tmp[12];
	tmp[0] = m[0];	tmp[1] = m[1];	tmp[2] = m[2];
	tmp[3] = m[4];	tmp[4] = m[5];	tmp[5] = m[6];
	tmp[6] = m[8];	tmp[7] = m[9];	tmp[8] = m[10];
	tmp[9] = m[12];	tmp[10]= m[13];	tmp[11]= m[14];
	F->Write (&tmp, sizeof(tmp));
}

void Matrix::GetEulerAngles(float rot[3])
{
	double sinPitch, cosPitch, sinRoll, cosRoll, sinYaw, cosYaw;
	float colMatrix[4][4];

	colMatrix[0][0] = m[0];
	colMatrix[0][1] = m[4];
	colMatrix[0][2] = m[8];
	colMatrix[0][3] = m[12];
	
	colMatrix[1][0] = m[1];
	colMatrix[1][1] = m[5];
	colMatrix[1][2] = m[9];
	colMatrix[1][3] = m[13];
	
	colMatrix[2][0] = m[2];
	colMatrix[2][1] = m[6];
	colMatrix[2][2] = m[10];
	colMatrix[2][3] = m[14];

	colMatrix[3][0] = 0.0f;
	colMatrix[3][1] = 0.0f;
	colMatrix[3][2] = 0.0f;
	colMatrix[3][3] = 1.0f;

	sinPitch = -colMatrix[2][0];
	cosPitch = sqrt(1 - sinPitch*sinPitch);

	if (fabs(cosPitch) > 0.0005)
	{
		sinRoll = colMatrix[2][1] / cosPitch;
		cosRoll = colMatrix[2][2] / cosPitch;
		sinYaw = colMatrix[1][0] / cosPitch;
		cosYaw = colMatrix[0][0] / cosPitch;
	} 
	else
    {
	    sinRoll = -colMatrix[1][2];
		cosRoll = colMatrix[1][1];
		sinYaw = 0;
		cosYaw = 1;
    }

	rot[2] = (float)(RTOD*atan2 (sinYaw, cosYaw));
	rot[1] = (float)(RTOD*atan2 (sinPitch, cosPitch));
	rot[0] = (float)(RTOD*atan2 (sinRoll, cosRoll));

	if (rot[2] < 0) rot[2] += 360;
	if (rot[1] < 0) rot[1] += 360;
	if (rot[0] < 0) rot[0] += 360;
}


void Matrix::ToAxisAngle(float rot[4])
{
	double matrix[3][4];
	double q[4];
	matrix[0][0] = m[0];
	matrix[0][1] = m[4];
	matrix[0][2] = m[8];
	matrix[0][3] = m[12];
	
	matrix[1][0] = m[1];
	matrix[1][1] = m[5];
	matrix[1][2] = m[9];
	matrix[1][3] = m[13];
	
	matrix[2][0] = m[2];
	matrix[2][1] = m[6];
	matrix[2][2] = m[10];
	matrix[2][3] = m[14];
	
	double trace, s;
	int i, j, k;
	static int next[3] = {1, 2, 0};
	
	trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
	
	if (trace > 0.0) 
	{
		s = sqrt(trace + 1.0);
		q[3] = s * 0.5;
		s = 0.5 / s;
		
		q[0] = (matrix[2][1] - matrix[1][2]) * s;
		q[1] = (matrix[0][2] - matrix[2][0]) * s;
		q[2] = (matrix[1][0] - matrix[0][1]) * s;
	} 
	else 
	{
		i = 0;
		if (matrix[1][1] > matrix[0][0])
			i = 1;
		if (matrix[2][2] > matrix[i][i])
			i = 2;
		
		j = next[i];  
		k = next[j];
		
		s = sqrt( (matrix[i][i] - (matrix[j][j]+matrix[k][k])) + 1.0 );
		
		q[i] = s * 0.5;
		
		s = 0.5 / s;
		
		q[3] = (matrix[k][j] - matrix[j][k]) * s;
		q[j] = (matrix[j][i] + matrix[i][j]) * s;
		q[k] = (matrix[k][i] + matrix[i][k]) * s;
	}

	double cos_angle = q[3];
	rot[3] = (float)acos(cos_angle) * 2 * RTOD;
	double sin_angle = sqrt( 1.0 - cos_angle * cos_angle );
	if (fabs(sin_angle) < 1E-10)
		sin_angle = 1;

	rot[0] = (float)(q[0] / sin_angle);
	rot[1] = (float)(q[1] / sin_angle);
	rot[2] = (float)(q[2] / sin_angle);

	if (fabs(rot[3]) < 1E-10)
	{
		rot[0] = rot[1] = rot[3] = 0;
		rot[2] = 1;
	}
}

void Matrix::FromEuler(float roll, float pitch, float yaw)
{
	float  cosYaw, sinYaw, cosPitch, sinPitch, cosRoll, sinRoll;

	cosYaw = (float)cos(yaw*DTOR);
	sinYaw = (float)sin(yaw*DTOR);
	cosPitch = (float)cos(pitch*DTOR);
	sinPitch = (float)sin(pitch*DTOR);
	cosRoll = (float)cos(roll*DTOR);
	sinRoll = (float)sin(roll*DTOR);

	m[0] = cosYaw * cosPitch;
	m[4] = cosYaw * sinPitch * sinRoll - sinYaw * cosRoll;
	m[8] = cosYaw * sinPitch * cosRoll + sinYaw * sinRoll;
	m[12] = 0.0f;

	m[1] = sinYaw * cosPitch;
	m[5] = cosYaw * cosRoll + sinYaw * sinPitch * sinRoll;
	m[9] = sinYaw * sinPitch * cosRoll - cosYaw * sinRoll;
	m[13] = 0.0f;

	m[2] = -sinPitch;
	m[6] = cosPitch * sinRoll;
	m[10] = cosPitch * cosRoll;
	m[14] = 0.0f;

	m[3] = 0.0f;
	m[7] = 0.0f;
	m[11] = 0.0f;
	m[15] = 1.0f;
}

// Create a rotation matrix (angle is in degrees)
void Matrix::FromAxisAngle(float axis[3], float angle)
{
	if (angle == 0.0f)
		return;
	rotation_matrix(angle, axis[0], axis[1], axis[2], m);
}

bool Matrix::FromInverse(double* src)
{
/*
// This code is better !
	float det_1, pos, neg, temp;

#define ACCUMULATE	  \
	if (temp >= 0.0)  \
		pos += temp;  \
	else			  \
		neg += temp;

#define PRECISION_LIMIT (1.0e-15)

	// Calculate the determinant of submatrix A and determine if the
	// the matrix is singular as limited by the double precision
	// floating-point data representation.
	pos = neg = 0.0f;
	temp =	src[0] * src[5] * src[10];
	ACCUMULATE
	temp =	src[1] * src[6] * src[8];
	ACCUMULATE
	temp =	src[2] * src[4] * src[9];
	ACCUMULATE
	temp = -src[2] * src[5] * src[8];
	ACCUMULATE
	temp = -src[1] * src[4] * src[10];
	ACCUMULATE
	temp = -src[0] * src[6] * src[9];
	ACCUMULATE
	det_1 = pos + neg;
#define ABS(a) (a < 0) ? -a : a
	// Is the submatrix A singular ?
	if ((det_1 == 0.0) || (ABS(det_1 / (pos - neg)) < PRECISION_LIMIT))
		return false;

	// Calculate inverse(A) = adj(A) / det(A)
	det_1 = 1.0f / det_1;
	inverse[0] =  (src[5]*src[10] - src[6]*src[9])*det_1;
	inverse[4] = -(src[4]*src[10] - src[6]*src[8])*det_1;
	inverse[8] =  (src[4]*src[9]  - src[5]*src[8])*det_1;
	inverse[1] = -(src[1]*src[10] - src[2]*src[9])*det_1;
	inverse[5] =  (src[0]*src[10] - src[2]*src[8])*det_1;
	inverse[9] = -(src[0]*src[9]  - src[1]*src[8])*det_1;
	inverse[2] =  (src[1]*src[6]  - src[2]*src[5])*det_1;
	inverse[6] = -(src[0]*src[6]  - src[2]*src[4])*det_1;
	inverse[10] = (src[0]*src[5]  - src[1]*src[4])*det_1;

	// Calculate -C * inverse(A)
	inverse[12] = -(src[12]*inverse[0] + src[13]*inverse[4] + src[14]*inverse[8]);
	inverse[13] = -(src[12]*inverse[1] + src[13]*inverse[5] + src[14]*inverse[9]);
	inverse[14] = -(src[12]*inverse[2] + src[13]*inverse[6] + src[14]*inverse[10]);

	// Fill in last column
	inverse[3] = inverse[7] = inverse[11] = 0.0f;
	inverse[15] = 1.0f;

	return true;
*/
	float t;
	int i, j, k, swap;
	float tmp[4][4];

	for (i = 0; i < 16; i++)
		m[i] = 0.0;
	m[0] = m[5] = m[10] = m[15] = 1.0;

	for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
		tmp[i][j] = (float)src[i*4+j];

	for (i = 0; i < 4; i++) 
	{
		// look for largest element in column.
		swap = i;
		for (j = i + 1; j < 4; j++) 
			if (fabs(tmp[j][i]) > fabs(tmp[i][i])) 
				swap = j;

		if (swap != i) 
		{
			// swap rows.
			for (k = 0; k < 4; k++)
			{
				t = tmp[i][k];
				tmp[i][k] = tmp[swap][k];
				tmp[swap][k] = t;

				t = m[i*4+k];
				m[i*4+k] = m[swap*4+k];
				m[swap*4+k] = t;
			}
		}

		if (tmp[i][i] == 0) 
		{
			// The matrix is singular, which shouldn't happen.
			return false;
		}

		t = tmp[i][i];
		for (k = 0; k < 4; k++) 
		{
			tmp[i][k] /= t;
			m[i*4+k] /= t;
		}
		for (j = 0; j < 4; j++) 
		{
			if (j != i) 
			{
				t = tmp[j][i];
				for (k = 0; k < 4; k++) 
				{
					tmp[j][k] -= tmp[i][k]*t;
					m[j*4+k] -= m[i*4+k]*t;
				}
			}
		}
	}

	return true;
}
