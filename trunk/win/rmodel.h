// RModel.h: interface for the CRModel class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RMODEL_H__2071A861_CBE3_11D2_8204_A9244A508F02__INCLUDED_)
#define AFX_RMODEL_H__2071A861_CBE3_11D2_8204_A9244A508F02__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

///////////////////////////////////////
// RAPID API RETURN VALUES
#define RAPID_OK						0 
#define RAPID_ERR_MODEL_OUT_OF_MEMORY	1 
#define RAPID_ERR_COLLIDE_OUT_OF_MEMORY	2
#define RAPID_ERR_UNPROCESSED_MODEL		3
#define RAPID_ERR_BUILD_OUT_OF_SEQUENCE	4
#define RAPID_ERR_BUILD_EMPTY_MODEL		5 

#define RAPID_BUILD_STATE_CONST		0	// "empty" state, after constructor
#define RAPID_BUILD_STATE_BEGIN		1	// after BeginModel()
#define RAPID_BUILD_STATE_ADDTRI	2	// after AddTri()
#define RAPID_BUILD_STATE_PROCESSED	3	// after EndModel()

// Find all pairwise intersecting triangles
#define RAPID_ALL_CONTACTS	1
// Just report one intersecting triangle pair, if there are any.
#define RAPID_FIRST_CONTACT	2

typedef struct
{
	int id1;
	int id2;
} collision_pair;

typedef struct
{
	int id;
	double p1[3], p2[3], p3[3];
} tri;

typedef struct box
{
	// placement in parent's space
	// box to parent space: x_m = pR*x_b + pT
	// parent to box space: x_b = pR.T()*(x_m - pT)
	double pR[3][3];
	double pT[3];
	
	// dimensions
	double d[3]; // this is "radius", that is, half the measure of a side length
	
	box *P;  // points to but does not "own".  
	box *N;
	
	tri *trp;
	
	int leaf() { return (!P && !N); } 
	double size() { return d[0]; } 
	
	int split_recurse(int *t, int n);
	int split_recurse(int *t); // specialized for leaf nodes
} box;

class CRModel  
{
public:
	CRModel();
	~CRModel();

	int BeginModel();
	int EndModel();
//	int AddTri(const double *p1, const double *p2, const double *p3, int id);
	int AddTri(float *p1, float *p2, float *p3, int id);

protected:
	box *b;
	int num_boxes_alloced;

	tri *tris;
	int num_tris;
	int num_tris_alloced;

	int build_state;

	int build_hierarchy();

	friend int RAPID_Collide(double R1[3][3], double T1[3], double s1, CRModel *RAPID_model1,
		       double R2[3][3], double T2[3], double s2, CRModel *RAPID_model2, int flag);
};

// The only global function that should be called
BOOL CollisionCheck(double R1[3][3], double T1[3], CRModel *RAPID_model1,
					double R2[3][3], double T2[3], CRModel *RAPID_model2, int flag = RAPID_FIRST_CONTACT);

#endif // !defined(AFX_RMODEL_H__2071A861_CBE3_11D2_8204_A9244A508F02__INCLUDED_)
