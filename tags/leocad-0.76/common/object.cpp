// Base class for all drawable objects
//

#include "lc_global.h"
#include "object.h"

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "project.h"
#include "matrix.h"
#include "file.h"
#include "lc_application.h"
#include "lc_model.h"

#define LC_KEY_SAVE_VERSION 2 // LeoCAD 0.76

// =============================================================================
// lcObject class

lcObject::lcObject(LC_OBJECT_TYPE nType)
{
	m_Keys = NULL;

	m_nObjectType = nType;
	m_pKeyValues = NULL;

	m_Next = NULL;
}

lcObject::~lcObject()
{
	delete []m_pKeyValues;
	RemoveKeys ();
}

bool lcObject::FileLoad(File& file)
{
	u8 version;

	file.ReadByte(&version, 1);
	if (version > LC_KEY_SAVE_VERSION)
		return false;

	if (version == 1)
	{
		u16 time;
		float param[4];
		u8 type;
		u32 n;

		file.ReadLong(&n, 1);
		while (n--)
		{
			file.ReadShort(&time, 1);
			file.ReadFloat(param, 4);
			file.ReadByte(&type, 1);

			ChangeKey(time, true, param, type);
		}

		file.ReadLong(&n, 1);
		while (n--)
		{
			file.ReadShort(&time, 1);
			file.ReadFloat(param, 4);
			file.ReadByte(&type, 1);
		}
	}
	else
	{
		u32 time;
		float param[4];
		u32 type;
		u32 n;

		file.ReadLong(&n, 1);
		while (n--)
		{
			file.ReadLong(&time, 1);
			file.ReadFloat(param, 4);
			file.ReadLong(&type, 1);

			ChangeKey(time, true, param, type);
		}
	}

	return true;
}

void lcObject::FileSave(File& file) const
{
	u8 version = LC_KEY_SAVE_VERSION;
	LC_OBJECT_KEY *node;
	u32 n;

	file.WriteByte(&version, 1);

	for (n = 0, node = m_Keys; node; node = node->Next)
		n++;
	file.WriteLong(&n, 1);

	for (node = m_Keys; node; node = node->Next)
	{
		file.WriteLong(&node->Time, 1);
		file.WriteFloat(node->Value, 4);
		file.WriteLong(&node->Type, 1);
	}
}

// =============================================================================
// Key handling

static LC_OBJECT_KEY* AddNode(LC_OBJECT_KEY *node, u32 Time, int Type)
{
	LC_OBJECT_KEY* newnode = (LC_OBJECT_KEY*)malloc(sizeof(LC_OBJECT_KEY));

	if (node)
	{
		newnode->Next = node->Next;
		node->Next = newnode;
	}
	else
		newnode->Next = NULL;

	newnode->Type = Type;
	newnode->Time = Time;
	newnode->Value[0] = newnode->Value[1] = newnode->Value[2] = newnode->Value[3] = 0;

	return newnode;
}

void lcObject::RegisterKeys(float *values[], LC_OBJECT_KEY_INFO* info, int count)
{
	int i;

	m_pKeyValues = new float* [count];

	for (i = 0; i < count; i++)
		m_pKeyValues[i] = values[i];

	m_Keys = AddNode(NULL, 1, 0);

	for (i = count-1; i > 0; i--)
		AddNode(m_Keys, 1, i);

	m_pKeyInfo = info;
	m_nKeyInfoCount = count;
}

void lcObject::RemoveKeys()
{
	LC_OBJECT_KEY *node, *prev;

	for (node = m_Keys; node;)
	{
		prev = node;
		node = node->Next;
		free(prev);
	}
}

void lcObject::ChangeKey(u32 Time, bool AddKey, const float* Value, int KeyType)
{
	LC_OBJECT_KEY* node, *poskey = NULL, *newpos = NULL;
	node = m_Keys;

	while (node)
	{
		if ((node->Time <= Time) && (node->Type == KeyType))
			poskey = node;

		node = node->Next;
	}

	if (AddKey)
	{
		if (poskey)
		{
			if (poskey->Time != Time)
				newpos = AddNode(poskey, Time, KeyType);
		}
		else
			newpos = AddNode(poskey, Time, KeyType);
	}

	if (newpos == NULL)
		newpos = poskey;

	for (int i = 0; i < m_pKeyInfo[KeyType].Size; i++)
		newpos->Value[i] = Value[i];
}

void lcObject::CalculateKeys(u32 Time)
{
//	LC_OBJECT_KEY *Next[m_nKeyInfoCount], *prev[m_nKeyInfoCount], *node;
	LC_OBJECT_KEY *Next[32], *prev[32], *node;
	int i, empty = m_nKeyInfoCount;

	for (i = 0; i < m_nKeyInfoCount; i++)
	{
		Next[i] = NULL;
		prev[i] = NULL;
	}

	node = m_Keys;

	// Get the previous and next keys for each variable
	while (node && empty)
	{
		if (node->Time <= Time)
		{
			prev[node->Type] = node;
		}
		else
		{
			if (Next[node->Type] == NULL)
			{
				Next[node->Type] = node;
				empty--;
			}
		}

		node = node->Next;
	}

	// TODO: USE KEY IN/OUT WEIGHTS
	for (i = 0; i < m_nKeyInfoCount; i++)
	{
//		LC_OBJECT_KEY *n = Next[i];
		LC_OBJECT_KEY *p = prev[i];

		if (p == NULL) continue;
/*
		if (Animation && (n != NULL) && (p->Time != Time))
		{
			float t = (float)(Time - p->Time)/(n->Time - p->Time);

			for (int j = 0; j < m_pKeyInfo[i].Size; j++)
				m_pKeyValues[i][j] = p->Value[j] + (n->Value[j] - p->Value[j])*t;
		}
		else
		*/
			for (int j = 0; j < m_pKeyInfo[i].Size; j++)
				m_pKeyValues[i][j] = p->Value[j];
	}
}

void lcObject::CalculateSingleKey(u32 Time, bool Animation, int KeyType, float* Value) const
{
	LC_OBJECT_KEY *Next = NULL, *prev = NULL, *node;

	node = m_Keys;

	while (node)
	{
		if (node->Type == KeyType)
		{
			if (node->Time <= Time)
				prev = node;
			else
			{
				if (Next == NULL)
				{
					Next = node;
					break;
				}
			}
		}

		node = node->Next;
	}

	// TODO: USE KEY IN/OUT WEIGHTS
	if (Animation && (Next != NULL) && (prev->Time != Time))
	{
		float t = (float)(Time - prev->Time)/(Next->Time - prev->Time);

		for (int j = 0; j < m_pKeyInfo[KeyType].Size; j++)
			Value[j] = prev->Value[j] + (Next->Value[j] - prev->Value[j])*t;
	}
	else
		for (int j = 0; j < m_pKeyInfo[KeyType].Size; j++)
			Value[j] = prev->Value[j];
}

void lcObject::InsertTime(u32 Start, u32 Time)
{
	LC_OBJECT_KEY *node, *prev = NULL;
	bool end[32];
	int i;

	for (i = 0; i < m_nKeyInfoCount; i++)
		end[i] = false;

	u32 last = lcGetActiveProject()->IsAnimation() ? lcGetActiveProject()->m_ActiveModel->m_TotalFrames : LC_OBJECT_TIME_MAX;

	for (node = m_Keys; node != NULL; prev = node, node = node->Next)
	{
		// skip everything before the start time
		if ((node->Time < Start) || (node->Time == 1))
			continue;

		// there's already a key at the end, delete this one
		if (end[node->Type])
		{
			prev->Next = node->Next;
			free(node);
			node = prev;

			continue;
		}

		node->Time += Time;
		if (node->Time >= last)
		{
			node->Time = last;
			end[node->Type] = true;
		}
	}
}

void lcObject::RemoveTime(u32 Start, u32 Time)
{
	LC_OBJECT_KEY *node, *prev = NULL;

	node = m_Keys;

	for (; node != NULL; prev = node, node = node->Next)
	{
		// skip everything before the start time
		if ((node->Time < Start) || (node->Time == 1))
			continue;

		if (node->Time < (Start + Time))
		{
			// delete this key
			prev->Next = node->Next;
			free(node);
			node = prev;

			continue;
		}

		node->Time -= Time;
		if (node->Time < 1)
			node->Time = 1;
	}
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
