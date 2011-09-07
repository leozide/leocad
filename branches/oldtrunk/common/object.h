#ifndef _OBJECT_H_
#define _OBJECT_H_

#if 0
class Object
{
public:
	Object(int nType);
	virtual ~Object();

public:
	// Move the object.
	virtual void Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz) = 0;

	// bSelecting is the action (add/remove), bFocus means "add focus if selecting"
	// or "remove focus only if deselecting"
	virtual void Select(bool bSelecting, bool bFocus) = 0;

/*
	virtual void UpdatePosition(unsigned short nTime, bool bAnimation) = 0;

	// Query functions
	virtual bool IsSelected() const
	{ return (m_nState & LC_OBJECT_SELECTED) != 0; };
	virtual bool IsFocused() const
	{ return (m_nState & LC_OBJECT_FOCUSED) != 0; };
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

protected:
	virtual bool FileLoad(File& file);
	virtual void FileSave(File& file) const;

	// Key handling stuff
public:
	void CalculateSingleKey(unsigned short nTime, int keytype, float *value) const;
	void ChangeKey(unsigned short time, bool addkey, const float *param, unsigned char keytype);
	virtual void InsertTime(u32 start, u32 time);
	virtual void RemoveTime(u32 start, u32 time);

protected:
	void CalculateKeys(unsigned short nTime);

private:
	void RemoveKeys();

	float **m_pKeyValues;

	// Bounding box stuff
protected:
	void BoundingBoxCalculate(float pos[3]);
	void BoundingBoxCalculate(const Matrix44& Mat, float Size);
	void BoundingBoxCalculate(const Matrix44& Mat, float Dimensions[6]);

private:
	bool BoundingBoxIntersectionbyLine(float a1, float b1, float c1, float a2, float b2, float c2,
	                                   float *x, float *y, float *z) const;
	bool BoundingBoxPointInside(float x, float y, float z) const;
	float m_fBoxPlanes[4][6];

	// Object type
	int m_nObjectType;
};

#endif // 0
#endif
