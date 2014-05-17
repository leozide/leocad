#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "lc_math.h"
#include "lc_array.h"

enum lcObjectType
{
	LC_OBJECT_PIECE,
	LC_OBJECT_CAMERA,
	LC_OBJECT_LIGHT
};

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

struct lcObjectSection
{
	lcObject* Object;
	lcuint32 Section;
};

struct lcObjectRayTest
{
	lcCamera* ViewCamera;
	bool PiecesOnly;
	lcVector3 Start;
	lcVector3 End;
	float Distance;
	lcObjectSection ObjectSection;
};

struct lcObjectBoxTest
{
	lcCamera* ViewCamera;
	lcVector4 Planes[6];
	lcArray<lcObjectSection> ObjectSections;
};

class lcObject
{
public:
	lcObject(lcObjectType ObjectType);
	virtual ~lcObject();

public:
	bool IsPiece() const
	{
		return mObjectType == LC_OBJECT_PIECE;
	}

	bool IsCamera() const
	{
		return mObjectType == LC_OBJECT_CAMERA;
	}

	bool IsLight() const
	{
		return mObjectType == LC_OBJECT_LIGHT;
	}

	lcObjectType GetType() const
	{
		return mObjectType;
	}

	virtual bool IsSelected() const = 0;
	virtual bool IsSelected(lcuint32 Section) const = 0;
	virtual void SetSelected(bool Selected) = 0;
	virtual void SetSelected(lcuint32 Section, bool Selected) = 0;
	virtual bool IsFocused() const = 0;
	virtual bool IsFocused(lcuint32 Section) const = 0;
	virtual void SetFocused(lcuint32 Section, bool Focused) = 0;
	virtual lcuint32 GetFocusSection() const = 0;

	virtual lcVector3 GetSectionPosition(lcuint32 Section) const = 0;
	virtual void Move(unsigned short nTime, bool bAddKey, float dx, float dy, float dz) = 0;
	virtual void RayTest(lcObjectRayTest& ObjectRayTest) const = 0;
	virtual void BoxTest(lcObjectBoxTest& ObjectBoxTest) const = 0;
	virtual const char* GetName() const = 0;

protected:
	virtual bool FileLoad(lcFile& file);
	virtual void FileSave(lcFile& file) const;

public:
	void CalculateSingleKey(unsigned short nTime, int keytype, float *value) const;
	void ChangeKey(unsigned short time, bool addkey, const float *param, unsigned char keytype);
	virtual void InsertTime(unsigned short start, unsigned short time);
	virtual void RemoveTime(unsigned short start, unsigned short time);

	int GetKeyTypeCount() const
	{
		return m_nKeyInfoCount;
	}

	const LC_OBJECT_KEY_INFO* GetKeyTypeInfo(int index) const
	{
		return &m_pKeyInfo[index];
	}

	const float* GetKeyTypeValue(int index) const
	{
		return m_pKeyValues[index];
	}

protected:
	void RegisterKeys(float *values[], LC_OBJECT_KEY_INFO* info, int count);
	void CalculateKeys(unsigned short nTime);

private:
	void RemoveKeys();

	LC_OBJECT_KEY* m_pInstructionKeys;
	float **m_pKeyValues;

	LC_OBJECT_KEY_INFO *m_pKeyInfo;
	int m_nKeyInfoCount;

private:
	lcObjectType mObjectType;
};

#endif
