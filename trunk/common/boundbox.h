//
//	boundbox.h
////////////////////////////////////////////////////

#ifndef _BOUNDBOX_H_
#define _BOUNDBOX_H_

bool BoxOutsideFrustum (float Dimensions[6]);

class Matrix;
class BoundingBox;

// Callback "closure" struct, used to make the necessary parameters known to
// the callback function.
typedef struct {
	double a1,b1,c1;
	double a2,b2,c2;
	double mindist;
	BoundingBox *pClosest;
} CLICKLINE;

class BoundingBox
{
public:
	BoundingBox();
	~BoundingBox();

	int GetOwnerType()
		{ return m_nParentType; }
	void* GetOwner()
		{ return m_pOwner; }

	void Initialize(void* pOwner, unsigned char nType);
	double FindIntersectDist(CLICKLINE* pLine);
	void CalculateBoundingBox(float pos[3]);
	void CalculateBoundingBox(Matrix *mat);
	void CalculateBoundingBox(Matrix *mat, float Dimensions[6]);

protected:
	void* m_pOwner;
	unsigned char m_nParentType;
	float m_fPlanes[4][6];

	bool IntersectionbyLine(double a1, double b1, double c1, double a2, double b2, double c2, double *x, double *y, double *z);
	bool PointInside(double x, double y, double z);
};

#endif // _BOUNDBOX_H_
