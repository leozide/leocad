#ifndef _LC_OBJECT_H_
#define _LC_OBJECT_H_

#include "str.h"
#include "array.h"
#include "algebra.h"

class lcScene;

// Forward declarations of all object types.
class lcObject;
class lcPieceObject;
class lcPiece;
class lcModelRef;
class lcCamera;
class lcCameraTarget;
class lcLight;
class lcLightTarget;

// Object types.
enum LC_OBJECT_TYPES
{
	LC_OBJECT_PIECE,
	LC_OBJECT_MODELREF,
	LC_OBJECT_CAMERA,
	LC_OBJECT_CAMERA_TARGET,
	LC_OBJECT_LIGHT,
	LC_OBJECT_LIGHT_TARGET,
	LC_OBJECT_TYPE_MASK = 0x0f
};

// State flags.
#define LC_OBJECT_HIDDEN         0x10
#define LC_OBJECT_SELECTED       0x20
#define LC_OBJECT_FOCUSED        0x40
#define LC_OBJECT_STATE_MASK     0xf0

// Structure to hold an object key.
struct LC_OBJECT_KEY
{
	Vector4 Value;
	u32 Time;
	u32 Tangent;
	u32 Pad0;
	u32 Pad1;
};

struct LC_CLICK_RAY
{
	Vector3 Start;
	Vector3 End;
	float Dist;
	const lcObject* Object;
};

typedef ObjArray<LC_OBJECT_KEY> lcObjectKeyArray;

// Base object class.
class lcObject
{
public:
	lcObject(u32 Type, int NumKeyTypes);
	virtual ~lcObject();

	// Return the object type.
	int GetType() const
	{
		return (m_Flags & LC_OBJECT_TYPE_MASK);
	}

	bool IsPieceObject() const
	{
		int Type = GetType();
		return (Type == LC_OBJECT_PIECE) || (Type == LC_OBJECT_MODELREF);
	}

	// Set or remove a flag.
	void SetFlag(u32 Flag, bool Set = true)
	{
		if (Set)
			m_Flags |= Flag;
		else
			m_Flags &= ~Flag;
	}

	// Check if a flag is set.
	bool IsFlagged(u32 Flag) const
	{
		return (m_Flags & Flag) != 0;
	}

	// Check if this object is hidden.
	bool IsHidden() const
	{
		return IsFlagged(LC_OBJECT_HIDDEN);
	}

	// Toggle selection.
	void SetSelection(bool Selection, bool Children)
	{
		if (Selection)
			m_Flags |= LC_OBJECT_SELECTED;
		else
			m_Flags &= ~(LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);

		if (Children)
			for (lcObject* Object = m_Children; Object; Object = Object->m_Next)
				Object->SetSelection(Selection, Children);
	}

	// Query if this object is currently selected.
	bool IsSelected() const
	{
		return ((m_Flags & LC_OBJECT_SELECTED) != 0);
	}

	// Toggle focus.
	void SetFocus(bool Focus, bool Children)
	{
		if (Focus)
			m_Flags |= (LC_OBJECT_SELECTED | LC_OBJECT_FOCUSED);
		else
			m_Flags &= ~LC_OBJECT_FOCUSED;

		if (Children)
			for (lcObject* Object = m_Children; Object; Object = Object->m_Next)
				Object->SetFocus(Focus, Children);
	}

	// Query if this object is currently focused.
	bool IsFocused() const
	{
		return ((m_Flags & LC_OBJECT_FOCUSED) != 0);
	}

	// Flag this object as visible or hidden.
	virtual void SetVisible(bool Visible = true)
	{
		if (Visible)
			m_Flags &= ~LC_OBJECT_HIDDEN;
		else
		{
			m_Flags &= ~(LC_OBJECT_SELECTED|LC_OBJECT_FOCUSED);
			m_Flags |= LC_OBJECT_HIDDEN;
		}
	}

	// Query if this object is visible at a given time.
	virtual bool IsVisible(u32 Time) const
	{
		return !IsFlagged(LC_OBJECT_HIDDEN);
	}

	// Creates a unique name for this object starting with an existing prefix.
	void SetUniqueName(lcObject* List, const String& Prefix);

	// Check if this object intersects a ray at a point closer to its start.
	virtual void ClosestRayIntersect(LC_CLICK_RAY* Ray) const = 0;

	// Check if this object intersects the volume specified by a given set of planes.
	virtual bool IntersectsVolume(const class Vector4* Planes, int NumPlanes) const = 0;

	// Move all keys after Time by Frames.
	virtual void InsertTime(u32 Time, u32 Frames);

	// Move all keys after Time by Frames.
	virtual void RemoveTime(u32 Time, u32 Frames);

	// Move this object by a given offset.
	virtual void Move(u32 Time, bool AddKey, const Vector3& Delta);

	// Move this object to a new position.
	void SetPosition(u32 Time, bool AddKey, const Vector3& NewPosition);

	// Change the previous key or add a new key.
	void ChangeKey(u32 Time, bool AddKey, int KeyType, const Vector4& Value);
	void ChangeKey(u32 Time, bool AddKey, int KeyType, const Vector3& Value);
	void ChangeKey(u32 Time, bool AddKey, int KeyType, const float Value);

	// Calculate a property value at a given time.
	void CalculateKey(u32 Time, int KeyType, Vector4* Value) const;
	void CalculateKey(u32 Time, int KeyType, Vector3* Value) const;
	void CalculateKey(u32 Time, int KeyType, float* Value) const;

	// Calculate all object properties at a given time.
	virtual void Update(u32 Time) = 0;

	// Render a preview of the object, not optmized.
	virtual void AddToScene(lcScene* Scene, const Matrix44& ParentWorld, int Color) = 0;

public:
	// Parenting information.
	lcObject* m_Next;
	lcObject* m_Parent;
	lcObject* m_Children;

	// Object data.
	Vector3 m_Position;
	String m_Name;
	u32 m_Flags;

	// Key frames.
	lcObjectKeyArray* m_Keys;
};

#endif // _LC_OBJECT_H_
