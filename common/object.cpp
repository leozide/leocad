#include "lc_global.h"
#include "lc_math.h"
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "project.h"
#include "object.h"
#include "lc_file.h"
#include "lc_application.h"

#define LC_KEY_SAVE_VERSION 1 // LeoCAD 0.73

lcObject::lcObject(lcObjectType ObjectType)
{
	m_pInstructionKeys = NULL;

	mObjectType = ObjectType;
	m_pKeyValues = NULL;
}

lcObject::~lcObject()
{
	delete []m_pKeyValues;
	RemoveKeys();
}

bool lcObject::FileLoad(lcFile& file)
{
  lcuint8 version = file.ReadU8();

  if (version > LC_KEY_SAVE_VERSION)
    return false;

  lcuint16 time;
  float param[4];
  lcuint8 type;
  lcuint32 n;

  file.ReadU32(&n, 1);
  while (n--)
  {
    file.ReadU16(&time, 1);
    file.ReadFloats(param, 4);
    file.ReadU8(&type, 1);

	ChangeKey(time, true, param, type);
  }

  file.ReadU32(&n, 1);
  while (n--)
  {
    file.ReadU16(&time, 1);
    file.ReadFloats(param, 4);
    file.ReadU8(&type, 1);
  }

  return true;
}

void lcObject::FileSave(lcFile& file) const
{
	LC_OBJECT_KEY *node;
	lcuint32 n;

	file.WriteU8(LC_KEY_SAVE_VERSION);

	for (n = 0, node = m_pInstructionKeys; node; node = node->next)
		n++;
	file.WriteU32(n);

	for (node = m_pInstructionKeys; node; node = node->next)
	{
		lcuint16 Step = lcMin(node->Step, 0xFFFFU);
		file.WriteU16(Step);
		file.WriteFloats(node->param, 4);
		file.WriteU8(node->type);
	}

	file.WriteU32(0);
}

// =============================================================================
// Key handling

static LC_OBJECT_KEY* AddNode(LC_OBJECT_KEY *node, lcStep Step, unsigned char nType)
{
  LC_OBJECT_KEY* newnode = (LC_OBJECT_KEY*)malloc(sizeof(LC_OBJECT_KEY));

  if (node)
  {
    newnode->next = node->next;
    node->next = newnode;
  }
  else
    newnode->next = NULL;

  newnode->type = nType;
  newnode->Step = Step;
  newnode->param[0] = newnode->param[1] = newnode->param[2] = newnode->param[3] = 0;

  return newnode;
}

void lcObject::RegisterKeys(float *values[], LC_OBJECT_KEY_INFO* info, int count)
{
  int i;

  m_pKeyValues = new float* [count];

  for (i = 0; i < count; i++)
    m_pKeyValues[i] = values[i];

  m_pInstructionKeys = AddNode(NULL, 1, 0);

  for (i = count-1; i > 0; i--)
	AddNode(m_pInstructionKeys, 1, i);

  m_pKeyInfo = info;
  m_nKeyInfoCount = count;
}

void lcObject::RemoveKeys()
{
  LC_OBJECT_KEY *node, *prev;

  for (node = m_pInstructionKeys; node;)
  {
    prev = node;
    node = node->next;
    free(prev);
  }
}

void lcObject::ChangeKey(lcStep Step, bool AddKey, const float *param, unsigned char nKeyType)
{
	LC_OBJECT_KEY *node, *poskey = NULL, *newpos = NULL;
	node = m_pInstructionKeys;

	while (node)
	{
		if ((node->Step <= Step) && (node->type == nKeyType))
			poskey = node;

		node = node->next;
	}

	if (AddKey)
	{
		if (poskey)
		{
			if (poskey->Step != Step)
				newpos = AddNode(poskey, Step, nKeyType);
		}
		else
			newpos = AddNode(poskey, Step, nKeyType);
	}

	if (newpos == NULL)
		newpos = poskey;

	for (int i = 0; i < m_pKeyInfo[nKeyType].size; i++)
		newpos->param[i] = param[i];
}

void lcObject::CalculateKeys(lcStep Step)
{
	LC_OBJECT_KEY* prev[32];

	for (LC_OBJECT_KEY* node = m_pInstructionKeys; node; node = node->next)
		if (node->Step <= Step)
			prev[node->type] = node;

	for (int i = 0; i < m_nKeyInfoCount; i++)
	{
		LC_OBJECT_KEY *p = prev[i];

		for (int j = 0; j < m_pKeyInfo[i].size; j++)
			m_pKeyValues[i][j] = p->param[j];
	}
}

void lcObject::InsertTime(lcStep Start, lcStep Time)
{
	LC_OBJECT_KEY *node, *prev = NULL;
	bool end[32];

	for (int i = 0; i < m_nKeyInfoCount; i++)
		end[i] = false;

	node = m_pInstructionKeys;

	for (; node != NULL; prev = node, node = node->next)
	{
		// skip everything before the start time
		if ((node->Step < Start) || (node->Step == 1))
			continue;

		// there's already a key at the end, delete this one
		if (end[node->type])
		{
			prev->next = node->next;
			free(node);
			node = prev;

			continue;
		}

		if (node->Step >= LC_STEP_MAX - Time)
		{
			node->Step = LC_STEP_MAX;
			end[node->type] = true;
		}
		else
			node->Step += Time;
	}
}

void lcObject::RemoveTime(lcStep Start, lcStep Time)
{
	LC_OBJECT_KEY *node = m_pInstructionKeys, *prev = NULL;

	for (; node != NULL; prev = node, node = node->next)
	{
		// skip everything before the start time
		if ((node->Step < Start) || (node->Step == 1))
			continue;

		if (node->Step < Start + Time)
		{
			// delete this key
			prev->next = node->next;
			free(node);
			node = prev;

			continue;
		}

		node->Step -= Time;
		if (node->Step < 1)
			node->Step = 1;
	}
}
