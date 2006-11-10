#ifndef _LC_OBJECT_H_
#define _LC_OBJECT_H_

// State flags.
#define LC_OBJECT_HIDDEN    0x01
#define LC_OBJECT_SELECTED  0x02
#define LC_OBJECT_FOCUSED   0x04

class lcObject
{
public:
	lcObject();
	virtual ~lcObject();

public:
	// Check if the object is the closest object that intersects a ray.
	virtual void ClosestOjectToRay(LC_CLICKLINE* Line) = 0;

	// Check if the object intersects the volume specified by a given set of planes.
	virtual bool IntersectsVolume(const class Vector4* Planes, int NumPlanes) = 0;

	// Select is the action (select/deselect), Focus means "add focus if selecting"
	// or "remove focus only if deselecting"
	virtual void Select(bool Select, bool Focus)
	{
		if (Select)
		{
			if (Focus)
				m_State |= (LC_OBJECT_FOCUSED|LC_OBJECT_SELECTED);
			else
				m_State |= LC_OBJECT_SELECTED;
		}
		else
		{
			if (Focus)
				m_State &= ~LC_OBJECT_FOCUSED;
			else
				m_State &= ~(LC_OBJECT_SELECTED|LC_OBJECT_FOCUSED);
		}
	}

	// Query if this object is currently selected.
	bool IsSelected() const
	{
		return ((m_State & LC_OBJECT_SELECTED) != 0);
	}

	// Query if this object is currently focused.
	bool IsFocused() const
	{
		return ((m_State & LC_OBJECT_FOCUSED) != 0);
	}

	// Flag this object as visible or hidden.
	virtual void SetVisible(bool Visible = true)
	{
		if (Visible)
			m_State &= ~LC_OBJECT_HIDDEN;
		else
		{
			m_State &= ~(LC_OBJECT_SELECTED|LC_OBJECT_FOCUSED);
			m_State |= LC_OBJECT_HIDDEN;
		}
	}

	// Query if this object is visible at a given time.
	virtual bool IsVisible(int Time)
	{
		return ((m_State & LC_OBJECT_HIDDEN) == 0);
	}

	// Return the object's name.
	const String& GetName() const
	{
		return m_Name;
	}

public:
	// Parenting information.
	Object* m_Next;
	Object* m_Parent;
	Object* m_Children;

	u32 m_State;
	String m_Name;
};

#endif // _LC_OBJECT_H_
