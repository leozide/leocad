#include "lc_global.h"
#include "lc_object.h"

#include <stdio.h>

lcObject::lcObject(u32 Type, int NumKeyTypes)
{
	m_Next = NULL;
	m_Parent = NULL;
	m_Children = NULL;

	m_Flags = Type;
	m_Keys = new lcObjectKeyArray[NumKeyTypes];

	LC_OBJECT_KEY Key;
	Key.Time = 1;
	Key.Value = Vector4(0, 0, 0, 0);

	for (int i = 0; i < NumKeyTypes; i++)
		m_Keys[i].Add(Key);
}

lcObject::~lcObject()
{
	delete[] m_Keys;
}

void lcObject::Move(u32 Time, bool AddKey, const Vector3& Delta)
{
	if (!IsSelected())
		return;

	ChangeKey(Time, AddKey, 0, m_ParentPosition + Delta);
}

void lcObject::SetPosition(u32 Time, bool AddKey, const Vector3& NewPosition)
{
	ChangeKey(Time, AddKey, 0, NewPosition);
}

void lcObject::ChangeKey(u32 Time, bool AddKey, int KeyType, const Vector4& Value)
{
	lcObjectKeyArray& Array = m_Keys[KeyType];
	int Prev = 0;

	// Find the previous key.
	for (int i = 0; i < Array.GetSize(); i++)
	{
		if (Array[i].Time > Time)
			break;

		Prev = i;
	}

	if (AddKey && (Array[Prev].Time != Time))
	{
		Array.InsertAt(Prev);
		Array[Prev].Time = Time;
	}

	Array[Prev].Value = Value;
}

void lcObject::ChangeKey(u32 Time, bool AddKey, int KeyType, const Vector3& Value)
{
	ChangeKey(Time, AddKey, KeyType, Vector4(Value));
}

void lcObject::ChangeKey(u32 Time, bool AddKey, int KeyType, const float Value)
{
	ChangeKey(Time, AddKey, KeyType, Vector4(Value, 0, 0, 0));
}

void lcObject::CalculateKey(u32 Time, int KeyType, Vector4* Value) const
{
	lcObjectKeyArray& Array = m_Keys[KeyType];
	int PrevIndex = 0;
	int NextIndex = 0;

	for (int Index = 0; Index < Array.GetSize(); Index++)
	{
		NextIndex = Index;

		if (Array[Index].Time > Time)
			break;

		PrevIndex = Index;
	}

	if (PrevIndex != NextIndex)
	{
		float t = (float)(Time - Array[PrevIndex].Time) / (Array[NextIndex].Time - Array[PrevIndex].Time);
		*Value = Array[PrevIndex].Value * t + Array[NextIndex].Value * (1.0f - t);
	}
	else
	{
		*Value = Array[PrevIndex].Value;
	}
}

void lcObject::CalculateKey(u32 Time, int KeyType, Vector3* Value) const
{
	Vector4 Tmp;
	CalculateKey(Time, KeyType, &Tmp);
	*Value = Vector3(Tmp);
}

void lcObject::CalculateKey(u32 Time, int KeyType, float* Value) const
{
	Vector4 Tmp;
	CalculateKey(Time, KeyType, &Tmp);
	*Value = Tmp[0];
}

void lcObject::SetUniqueName(lcObject* List, const String& Prefix)
{
	int Len = Prefix.GetLength();
	int Max = 0;

	for (lcObject* Object = List; Object; Object = Object->m_Next)
	{
		if (strncmp(Object->m_Name, Prefix, Len) == 0)
		{
			int i;

			if (sscanf((const char*)Object->m_Name + Len, " #%d", &i) == 1)
				Max = lcMax(Max, i);
		}
	}

	char Suffix[128];
	sprintf(Suffix, " #%.2d", Max + 1);

	m_Name = Prefix + Suffix;
}

void lcObject::InsertTime(u32 Time, u32 Frames)
{
	// FIXME: insert time
}

void lcObject::RemoveTime(u32 Time, u32 Frames)
{
	// FIXME: remove time
}
