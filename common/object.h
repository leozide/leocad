#ifndef _OBJECT_H_
#define _OBJECT_H_

class File;
class Matrix44;
class Object;

typedef enum
{
	LC_OBJECT_PIECE,
	LC_OBJECT_CAMERA,
	LC_OBJECT_CAMERA_TARGET,
	LC_OBJECT_LIGHT,
	LC_OBJECT_LIGHT_TARGET,
	LC_OBJECT_CURVE,
	LC_OBJECT_CURVE_POINT,
} LC_OBJECT_TYPE;

// key handling
struct LC_OBJECT_KEY
{
	unsigned short  time;
	float           param[4];
	unsigned char   type;
	LC_OBJECT_KEY*  next;
};

struct LC_OBJECT_KEY_INFO
{
	const char *description;
	unsigned char size; // number of floats
	unsigned char type;
};

// Callback "closure" struct, used to make the necessary parameters known to
// the callback function.
struct LC_CLICKLINE
{
	float a1, b1, c1;
	float a2, b2, c2;
	float mindist;
	Object *pClosest;

	float PointDistance(float *point);

};

class Object
{
public:
	Object(LC_OBJECT_TYPE nType);
	virtual ~Object();

public:
	// Move the object.
	virtual void Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz) = 0;

	// Check if the object intersects the ray.
	virtual void MinIntersectDist(LC_CLICKLINE* pLine) = 0;

	// bSelecting is the action (add/remove), bFocus means "add focus if selecting"
	// or "remove focus only if deselecting"
	virtual void Select(bool bSelecting, bool bFocus) = 0;

	// Check if the object intersects the volume specified by a given set of planes.
	virtual bool IntersectsVolume(const class Vector4* Planes, int NumPlanes) = 0;

	Object* m_Next;
/*
	virtual void UpdatePosition(unsigned short nTime, bool bAnimation) = 0;
	virtual void CompareBoundingBox(float *box) { };

	// Query functions
	virtual bool IsSelected() const
	{ return (m_nState & LC_OBJECT_SELECTED) != 0; };
	virtual bool IsFocused() const
	{ return (m_nState & LC_OBJECT_FOCUSED) != 0; };
	virtual bool IsVisible(unsigned short nTime, bool bAnimation) const
	{ return (m_nState & LC_OBJECT_HIDDEN) == 0; }
	const char* GetName() const
	{ return m_strName; }


	// State change, most classes will have to replace these functions
	virtual void SetSelection(bool bSelect, void *pParam = NULL)
	{
		if (bSelect)
			m_nState |= LC_OBJECT_SELECTED;
		else
			m_nState &= ~(LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);
	};
	virtual void SetFocus(bool bFocus, void *pParam = NULL)
	{
		if (bFocus)
			m_nState |= (LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);
		else
			m_nState &= ~LC_OBJECT_FOCUSED;
	};
	virtual void SetVisible(bool bVisible)
	{
		if (bVisible)
			m_nState &= ~LC_OBJECT_HIDDEN;
		else
		{
			m_nState |= LC_OBJECT_HIDDEN;
			SetSelection(false, NULL);
		}
	}
	*/

	// determine the object type
	bool IsPiece() const
	{ return m_nObjectType == LC_OBJECT_PIECE; }
	bool IsCamera() const
	{ return m_nObjectType == LC_OBJECT_CAMERA; }
	bool IsLight() const
	{ return m_nObjectType == LC_OBJECT_LIGHT; }
	bool IsCurve() const
	{ return m_nObjectType == LC_OBJECT_CURVE; }

	LC_OBJECT_TYPE GetType() const
	{ return m_nObjectType; }

	virtual const char* GetName() const = 0;

protected:
	virtual bool FileLoad(File& file);
	virtual void FileSave(File& file) const;

	// Key handling stuff
public:
	void CalculateSingleKey(unsigned short nTime, int keytype, float *value) const;
	void ChangeKey(unsigned short time, bool addkey, const float *param, unsigned char keytype);
	virtual void InsertTime(u32 start, u32 time);
	virtual void RemoveTime(u32 start, u32 time);

	int GetKeyTypeCount() const
	{ return m_nKeyInfoCount; }
	const LC_OBJECT_KEY_INFO* GetKeyTypeInfo(int index) const
	{ return &m_pKeyInfo[index]; };
	const float* GetKeyTypeValue(int index) const
	{ return m_pKeyValues[index]; };

protected:
	void RegisterKeys(float *values[], LC_OBJECT_KEY_INFO* info, int count);
	void CalculateKeys(unsigned short nTime);

private:
	void RemoveKeys();

	LC_OBJECT_KEY* m_Keys;
	float **m_pKeyValues;

	LC_OBJECT_KEY_INFO *m_pKeyInfo;
	int m_nKeyInfoCount;


	// Bounding box stuff
protected:
	float BoundingBoxIntersectDist(LC_CLICKLINE* pLine) const;
	void BoundingBoxCalculate(float pos[3]);
	void BoundingBoxCalculate(const Matrix44& Mat, float Size);
	void BoundingBoxCalculate(const Matrix44& Mat, float Dimensions[6]);

private:
	bool BoundingBoxIntersectionbyLine(float a1, float b1, float c1, float a2, float b2, float c2,
	                                   float *x, float *y, float *z) const;
	bool BoundingBoxPointInside(float x, float y, float z) const;
	float m_fBoxPlanes[4][6];

	// Object type
private:
	LC_OBJECT_TYPE m_nObjectType;
};

#endif
