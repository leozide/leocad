// Hide math stuff here.
//

static float Identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

// Perform a 4x4 matrix multiplication  (product = a x b).
// WARNING: (product != b) assumed
static __inline void matmul(float *product, const float *a, const float *b)
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

typedef struct
{
	float m[16];
} matrix;

static __inline void ConvertFromLDraw(matrix* m, float f[12])
{
	float trans[16] = { 1,0,0,0, 0,0,-1,0, 0,1,0,0, 0,0,0,1 };
	float t[16] = { 1,0,0,0, 0,0,1,0, 0,-1,0,0, 0,0,0,1 };
	m->m[0] = f[3];	m->m[1] = f[6];	m->m[2] = f[9];
	m->m[4] = f[4];	m->m[5] = f[7];	m->m[6] = f[10];
	m->m[8] = f[5];	m->m[9] = f[8];	m->m[10]= f[11];
	m->m[12]= f[0]/25;	m->m[13]= f[1]/25;	m->m[14]= f[2]/25;
	matmul (m->m, m->m, t);
	matmul (trans, trans, m->m);
	memcpy (&m[0], &trans[0], sizeof(m->m));
}

static __inline void LoadIdentity(matrix* m)
{
	memcpy (&m->m[0], &Identity, sizeof(float[16]));
}

static __inline void Multiply(matrix* m, matrix* m1, matrix* m2)
{
	matmul (m->m, m1->m, m2->m);
}

static __inline void TransformPoint(matrix* m, float out[], const float in[3])
{
	out[0] = m->m[0]*in[0] + m->m[4]*in[1] + m->m[8]*in[2] + m->m[12];
	out[1] = m->m[1]*in[0] + m->m[5]*in[1] + m->m[9]*in[2] + m->m[13];
	out[2] = m->m[2]*in[0] + m->m[6]*in[1] + m->m[10]*in[2] + m->m[14];
}

static __inline void TranslatePoint(matrix* m, float* in, float* t)
{
	in[0] = m->m[0]*t[0] + m->m[4]*t[1] + m->m[8]*t[2] + m->m[12];
	in[1] = m->m[1]*t[0] + m->m[5]*t[1] + m->m[9]*t[2] + m->m[13];
	in[2] = m->m[2]*t[0] + m->m[6]*t[1] + m->m[10]*t[2] + m->m[14];
}

static __inline void TransformPoints (matrix* m, float p[], int n)
{
	int i;
	float tmp[3];

	for (i = 0; i < n*3; i += 3)
	{
		tmp[0] = p[i];
		tmp[1] = p[i+1];
		tmp[2] = p[i+2];
		TransformPoint (m, &p[i], tmp);
	}
}

static __inline void ConvertPoints (float pts[], int count)
{
	float tmp;
	int i;

	for (i = 0; i < count; i++)
	{
		pts[3*i] /= 25;
		tmp = pts[3*i+1];
		pts[3*i+1] = pts[3*i+2]/25;
		pts[3*i+2] = -tmp/25;
	}
}

static __inline void Resequence(float v[4][3], int a, int b, int c, int d)
{
	float o[4][3];
	memcpy(o, v, sizeof(o));
	memcpy(v[0], o[a], sizeof(o[0]));
	memcpy(v[1], o[b], sizeof(o[0]));
	memcpy(v[2], o[c], sizeof(o[0]));
	memcpy(v[3], o[d], sizeof(o[0]));
}

static __inline void Sub3(float v[], float q1[], float q2[])
{
	v[0] = q1[0]-q2[0];
	v[1] = q1[1]-q2[1];
	v[2] = q1[2]-q2[2];
}

static __inline float Dot3(float q1[], float q2[])
{
	return q1[0]*q2[0]+q1[1]*q2[1]+q1[2]*q2[2];
}

static __inline void Cross3(float v[], float q1[], float q2[])
{
	v[0] = (q1[1]*q2[2]) - (q1[2]*q2[1]);
	v[1] = (q1[2]*q2[0]) - (q1[0]*q2[2]);
	v[2] = (q1[0]*q2[1]) - (q1[1]*q2[0]);
}

static __inline void TestQuads(float quad[4][3])
{
	float v01[3], v02[3], v03[3];
	float v12[3], v13[3], v23[3];
	float cp1[3], cp2[3];
	float dotA, dotB, dotC;
	int A, B, C;
	
	// Calculate A
	Sub3(v01, quad[1], quad[0]);
	Sub3(v02, quad[2], quad[0]);
	Sub3(v03, quad[3], quad[0]);
	Cross3(cp1, v01, v02);
	Cross3(cp2, v02, v03);
	dotA = Dot3(cp1, cp2);
	A = (dotA > 0.0f);
	
	if (A)
	{
		// 3 is in I, typical case, OK: 0123 D02 (convex/concave)
		// CONVEXINFO: quad is convex if (!B && !C): OK: 0123 D02/13 (convex)
	}
	else
	{
		// Calculate B and C (may be postponed/discarded)
		// NOTE: postponed !
		Sub3(v12, quad[2], quad[1]);
		Sub3(v13, quad[3], quad[1]);
		Sub3(v23, quad[3], quad[2]);
		Cross3(cp1, v12, v01);
		Cross3(cp2, v01, v13);
		dotB = Dot3(cp1, cp2);
		B = (dotB > 0.0f);
		Cross3(cp1, v02, v12);
		Cross3(cp2, v12, v23);
		dotC = -Dot3(cp1, cp2);
		C = (dotC > 0.0f);

		// 3 is in II, III, IV or V. Calculation of B and C could be postponed
		//	to here if CONVEXINFO (above) is not needed
		if (B)
		{
			// 3 is in II or III
			if (C)
			{
				// 3 is in II, OK: 0123 D13 (concave)
				Resequence(quad, 1, 2, 3, 0); // just to shift diagonal
			}
			else
			{
				// 3 is in III, bow-tie error: using 0312 D01/D23 (convex)
				Resequence(quad, 0, 3, 1, 2);
			}
		}
		else
		{
			// 3 is in IV or V
			if (C)
			{
				// 3 is in IV, bow-tie error: using 0132 D12/D03 (convex)
				Resequence(quad, 0, 1, 3, 2);
			}
			else
			{
				// 3 is in V, OK: 0123 D13 (concave)
				Resequence(quad, 1, 2, 3, 0); // just to shift diagonal
			}
		}
	}
	// The four vertices quad[0], quad[1], quad[2] and quad[3] now have
	// the correct sequence, the polygon can be divided by the diagonal 02
	// into two triangles, 012 and 230.
}

static __inline void FixQuads(float quad[])
{
	float t[4][3];
	memcpy(t, quad, sizeof(t));
	TestQuads(t);
	memcpy(quad, t, sizeof(t));
}

static __inline int FloatPointsClose (float pt1[], float pt2[])
{
	if (fabs(pt1[0] - pt2[0]) > 0.01)
		return 0;
	if (fabs(pt1[1] - pt2[1]) > 0.01)
		return 0;
	if (fabs(pt1[2] - pt2[2]) > 0.01)
		return 0;
	return 1;
}
