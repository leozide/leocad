// Vector.cpp: implementation of the Vector class.
//
//////////////////////////////////////////////////////////////////////

#include <math.h>
#include "vector.h"
#include "defines.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

Vector::Vector ()
{
  m_fPoint[0] = 0;
  m_fPoint[1] = 0;
  m_fPoint[2] = 0;
}

Vector::Vector (float x, float y, float z)
{
  m_fPoint[0] = x;
  m_fPoint[1] = y;
  m_fPoint[2] = z;
}

Vector::Vector (const float *point)
{
  m_fPoint[0] = point[0];
  m_fPoint[1] = point[1];
  m_fPoint[2] = point[2];
}

Vector::Vector (const float *p1, const float *p2)
{
  m_fPoint[0] = p2[0] - p1[0];
  m_fPoint[1] = p2[1] - p1[1];
  m_fPoint[2] = p2[2] - p1[2];
}

Vector::~Vector ()
{
}

//////////////////////////////////////////////////////////////////////
// Operations

bool Vector::operator==(Vector& vec)
{
  if (m_fPoint[0] != vec.m_fPoint[0])
    return false;
  if (m_fPoint[1] != vec.m_fPoint[1])
    return false;
  if (m_fPoint[2] != vec.m_fPoint[2])
    return false;
  return true;
}

Vector& Vector::operator*=(float scalar)
{
  m_fPoint[0] *= scalar;
  m_fPoint[1] *= scalar;
  m_fPoint[2] *= scalar;
  return *this;
}

Vector& Vector::operator+=(Vector& add)
{
  m_fPoint[0] += add.m_fPoint[0];
  m_fPoint[1] += add.m_fPoint[1];
  m_fPoint[2] += add.m_fPoint[2];
  return *this;
}

Vector& Vector::operator-=(Vector& sub)
{
  m_fPoint[0] -= sub.m_fPoint[0];
  m_fPoint[1] -= sub.m_fPoint[1];
  m_fPoint[2] -= sub.m_fPoint[2];
  return *this;
}

Vector& Vector::operator=(Vector& source)
{
  m_fPoint[0] = source.m_fPoint[0];
  m_fPoint[1] = source.m_fPoint[1];
  m_fPoint[2] = source.m_fPoint[2];
  return *this;
}

Vector& Vector::operator=(const float *point)
{
  m_fPoint[0] = point[0];
  m_fPoint[1] = point[1];
  m_fPoint[2] = point[2];
  return *this;
}

float Vector::Length()
{
  return (float)sqrt(m_fPoint[0]*m_fPoint[0] + m_fPoint[1]*m_fPoint[1] + m_fPoint[2]*m_fPoint[2]);
}

void Vector::Normalize()
{
  float inv = 1.0f/(float)sqrt(m_fPoint[0]*m_fPoint[0] + m_fPoint[1]*m_fPoint[1] + m_fPoint[2]*m_fPoint[2]);
  m_fPoint[0] *= inv;
  m_fPoint[1] *= inv;
  m_fPoint[2] *= inv;
}

void Vector::Add (const float *point)
{
  m_fPoint[0] += point[0];
  m_fPoint[1] += point[1];
  m_fPoint[2] += point[2];
}

void Vector::Add (float x, float y, float z)
{
  m_fPoint[0] += x;
  m_fPoint[1] += y;
  m_fPoint[2] += z;
}

void Vector::Scale(float scale)
{
  m_fPoint[0] *= scale;
  m_fPoint[1] *= scale;
  m_fPoint[2] *= scale;
}

Vector& Vector::Cross(Vector& v1, Vector& v2)
{
  m_fPoint[0] = v1.m_fPoint[1]*v2.m_fPoint[2] - v1.m_fPoint[2]*v2.m_fPoint[1];
  m_fPoint[1] = v1.m_fPoint[2]*v2.m_fPoint[0] - v1.m_fPoint[0]*v2.m_fPoint[2];
  m_fPoint[2] = v1.m_fPoint[0]*v2.m_fPoint[1] - v1.m_fPoint[1]*v2.m_fPoint[0];
  return *this;
}

float Vector::Angle(Vector& vec)
{
  double d, m1, m2;

  d = m_fPoint[0]*vec.m_fPoint[0]+m_fPoint[1]*vec.m_fPoint[1]+m_fPoint[2]*vec.m_fPoint[2];
  m1 = sqrt(m_fPoint[0]*m_fPoint[0]+m_fPoint[1]*m_fPoint[1]+m_fPoint[2]*m_fPoint[2]);
  m2 = sqrt(vec.m_fPoint[0]*vec.m_fPoint[0]+vec.m_fPoint[1]*vec.m_fPoint[1]+vec.m_fPoint[2]*vec.m_fPoint[2]);

  return (float) (RTOD * acos (d / (m1*m2)));
}

float Vector::Dot(Vector& vec)
{
  return m_fPoint[0]*vec.m_fPoint[0]+m_fPoint[1]*vec.m_fPoint[1]+m_fPoint[2]*vec.m_fPoint[2];
}

void Vector::ToFloat (float *point)
{
  point[0] = m_fPoint[0];
  point[1] = m_fPoint[1];
  point[2] = m_fPoint[2];
}

void Vector::FromFloat (const float *point)
{
  m_fPoint[0] = point[0];
  m_fPoint[1] = point[1];
  m_fPoint[2] = point[2];
}

void Vector::FromFloat(float x, float y, float z)
{
  m_fPoint[0] = x;
  m_fPoint[1] = y;
  m_fPoint[2] = z;
}
