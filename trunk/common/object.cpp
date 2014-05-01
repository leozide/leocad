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
    file.WriteU16(node->time);
    file.WriteFloats(node->param, 4);
    file.WriteU8(node->type);
  }

  file.WriteU32(0);
}

// =============================================================================
// Key handling

static LC_OBJECT_KEY* AddNode(LC_OBJECT_KEY *node, unsigned short nTime, unsigned char nType)
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
  newnode->time = nTime;
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

void lcObject::ChangeKey(unsigned short nTime, bool bAddKey, const float *param, unsigned char nKeyType)
{
  LC_OBJECT_KEY *node, *poskey = NULL, *newpos = NULL;
  node = m_pInstructionKeys;

  while (node)
  {
    if ((node->time <= nTime) &&
	(node->type == nKeyType))
      poskey = node;

    node = node->next;
  }

  if (bAddKey)
  {
    if (poskey)
    {
      if (poskey->time != nTime)
	newpos = AddNode(poskey, nTime, nKeyType);
    }
    else
      newpos = AddNode(poskey, nTime, nKeyType);
  }

  if (newpos == NULL)
    newpos = poskey;

  for (int i = 0; i < m_pKeyInfo[nKeyType].size; i++)
    newpos->param[i] = param[i];
}

void lcObject::CalculateKeys(unsigned short nTime)
{
//  LC_OBJECT_KEY *next[m_nKeyInfoCount], *prev[m_nKeyInfoCount], *node;
  LC_OBJECT_KEY *next[32], *prev[32], *node;
  int i, empty = m_nKeyInfoCount;

  for (i = 0; i < m_nKeyInfoCount; i++)
    next[i] = NULL;

  node = m_pInstructionKeys;

  // Get the previous and next keys for each variable
  while (node && empty)
  {
    if (node->time <= nTime)
    {
      prev[node->type] = node;
    }
    else
    {
      if (next[node->type] == NULL)
      {
        next[node->type] = node;
        empty--;
      }
    }

    node = node->next;
  }

  // TODO: USE KEY IN/OUT WEIGHTS
  for (i = 0; i < m_nKeyInfoCount; i++)
  {
	LC_OBJECT_KEY *p = prev[i];

	for (int j = 0; j < m_pKeyInfo[i].size; j++)
	  m_pKeyValues[i][j] = p->param[j];
  }
}

void lcObject::CalculateSingleKey(unsigned short nTime, int keytype, float *value) const
{
	LC_OBJECT_KEY *prev = NULL, *node;

	node = m_pInstructionKeys;

	while (node)
	{
		if (node->type == keytype)
		{
			if (node->time <= nTime)
				prev = node;
			else
				break;
		}

		node = node->next;
	}

	for (int j = 0; j < m_pKeyInfo[keytype].size; j++)
		value[j] = prev->param[j];
}

void lcObject::InsertTime(unsigned short start, unsigned short time)
{
  LC_OBJECT_KEY *node, *prev = NULL;
  unsigned short last;
  bool end[32];
  int i;

  for (i = 0; i < m_nKeyInfoCount; i++)
    end[i] = false;

  node = m_pInstructionKeys;
  last = 255;

  for (; node != NULL; prev = node, node = node->next)
  {
    // skip everything before the start time
    if ((node->time < start) || (node->time == 1))
      continue;

    // there's already a key at the end, delete this one
    if (end[node->type])
    {
      prev->next = node->next;
      free(node);
      node = prev;

      continue;
    }

    node->time += time;
    if (node->time >= last)
    {
      node->time = last;
      end[node->type] = true;
    }
  }
}

void lcObject::RemoveTime(unsigned short start, unsigned short time)
{
  LC_OBJECT_KEY *node, *prev = NULL;

  node = m_pInstructionKeys;

  for (; node != NULL; prev = node, node = node->next)
  {
    // skip everything before the start time
    if ((node->time < start) || (node->time == 1))
      continue;

    if (node->time < (start + time))
    {
      // delete this key
      prev->next = node->next;
      free(node);
      node = prev;

      continue;
    }

    node->time -= time;
    if (node->time < 1)
      node->time = 1;
  }
}
