// Base class for all drawable objects
//

#include "lc_global.h"
#include "lc_math.h"
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "globals.h"
#include "project.h"
#include "object.h"
#include "matrix.h"
#include "lc_file.h"
#include "lc_application.h"

#define LC_KEY_SAVE_VERSION 1 // LeoCAD 0.73

// =============================================================================
// Object class

Object::Object(LC_OBJECT_TYPE nType)
{
  //  m_nState = 0;
  //  m_strName[0] = '\0';

  m_pAnimationKeys = NULL;
  m_pInstructionKeys = NULL;

  m_nObjectType = nType;
  m_pKeyValues = NULL;

  //  m_pParent = NULL;
  //  m_pNext = NULL;
  //  m_pNextRender = NULL;
}

Object::~Object()
{
  delete []m_pKeyValues;
  RemoveKeys();
}

bool Object::FileLoad(lcFile& file)
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

    ChangeKey(time, false, true, param, type);
  }

  file.ReadU32(&n, 1);
  while (n--)
  {
    file.ReadU16(&time, 1);
    file.ReadFloats(param, 4);
    file.ReadU8(&type, 1);

    ChangeKey(time, true, true, param, type);
  }

  return true;
}

void Object::FileSave(lcFile& file) const
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

  for (n = 0, node = m_pAnimationKeys; node; node = node->next)
    n++;
  file.WriteU32(n);

  for (node = m_pAnimationKeys; node; node = node->next)
  {
    file.WriteU16(node->time);
    file.WriteFloats(node->param, 4);
    file.WriteU8(node->type);
  }
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

void Object::RegisterKeys(float *values[], LC_OBJECT_KEY_INFO* info, int count)
{
  int i;

  m_pKeyValues = new float* [count];

  for (i = 0; i < count; i++)
    m_pKeyValues[i] = values[i];

  m_pAnimationKeys = AddNode(NULL, 1, 0);
  m_pInstructionKeys = AddNode(NULL, 1, 0);

  for (i = count-1; i > 0; i--)
  {
    AddNode(m_pAnimationKeys, 1, i);
    AddNode(m_pInstructionKeys, 1, i);
  }

  m_pKeyInfo = info;
  m_nKeyInfoCount = count;
}

void Object::RemoveKeys()
{
  LC_OBJECT_KEY *node, *prev;

  for (node = m_pInstructionKeys; node;)
  {
    prev = node;
    node = node->next;
    free(prev);
  }

  for (node = m_pAnimationKeys; node;)
  {
    prev = node;
    node = node->next;
    free(prev);
  }
}

void Object::ChangeKey(unsigned short nTime, bool bAnimation, bool bAddKey, const float *param, unsigned char nKeyType)
{
  LC_OBJECT_KEY *node, *poskey = NULL, *newpos = NULL;
  if (bAnimation)
    node = m_pAnimationKeys;
  else
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

void Object::CalculateKeys(unsigned short nTime, bool bAnimation)
{
//  LC_OBJECT_KEY *next[m_nKeyInfoCount], *prev[m_nKeyInfoCount], *node;
  LC_OBJECT_KEY *next[32], *prev[32], *node;
  int i, empty = m_nKeyInfoCount;

  for (i = 0; i < m_nKeyInfoCount; i++)
    next[i] = NULL;

  if (bAnimation)
    node = m_pAnimationKeys;
  else
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
    LC_OBJECT_KEY *n = next[i], *p = prev[i];

    if (bAnimation && (n != NULL) && (p->time != nTime))
    {
      float t = (float)(nTime - p->time)/(n->time - p->time);

      for (int j = 0; j < m_pKeyInfo[i].size; j++)
        m_pKeyValues[i][j] = p->param[j] + (n->param[j] - p->param[j])*t;
    }
    else
      for (int j = 0; j < m_pKeyInfo[i].size; j++)
        m_pKeyValues[i][j] = p->param[j];
  }
}

void Object::CalculateSingleKey(unsigned short nTime, bool bAnimation, int keytype, float *value) const
{
	LC_OBJECT_KEY *next = NULL, *prev = NULL, *node;

	if (bAnimation)
		node = m_pAnimationKeys;
	else
		node = m_pInstructionKeys;

	while (node)
	{
		if (node->type == keytype)
		{
			if (node->time <= nTime)
				prev = node;
			else
			{
				if (next == NULL)
				{
					next = node;
					break;
				}
			}
		}

		node = node->next;
	}

	// TODO: USE KEY IN/OUT WEIGHTS
	if (bAnimation && (next != NULL) && (prev->time != nTime))
	{
		float t = (float)(nTime - prev->time)/(next->time - prev->time);

		for (int j = 0; j < m_pKeyInfo[keytype].size; j++)
			value[j] = prev->param[j] + (next->param[j] - prev->param[j])*t;
	}
	else
		for (int j = 0; j < m_pKeyInfo[keytype].size; j++)
			value[j] = prev->param[j];
}

void Object::InsertTime(unsigned short start, bool animation, unsigned short time)
{
  LC_OBJECT_KEY *node, *prev = NULL;
  unsigned short last;
  bool end[32];
  int i;

  for (i = 0; i < m_nKeyInfoCount; i++)
    end[i] = false;

  if (animation)
  {
    node = m_pAnimationKeys;
    last = lcGetActiveProject()->GetTotalFrames();
  }
  else
  {
    node = m_pInstructionKeys;
    last = 255;
  }

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

void Object::RemoveTime(unsigned short start, bool animation, unsigned short time)
{
  LC_OBJECT_KEY *node, *prev = NULL;

  if (animation)
    node = m_pAnimationKeys;
  else
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
