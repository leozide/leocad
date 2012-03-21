#include "lc_global.h"
#include <math.h>
#include "vector.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

Vector::Vector()
{
  m_fPoint[0] = 0;
  m_fPoint[1] = 0;
  m_fPoint[2] = 0;
}

Vector::Vector(float x, float y, float z)
{
  m_fPoint[0] = x;
  m_fPoint[1] = y;
  m_fPoint[2] = z;
}

Vector::Vector(const float *point)
{
  m_fPoint[0] = point[0];
  m_fPoint[1] = point[1];
  m_fPoint[2] = point[2];
}

Vector::Vector(const float *p1, const float *p2)
{
  m_fPoint[0] = p2[0] - p1[0];
  m_fPoint[1] = p2[1] - p1[1];
  m_fPoint[2] = p2[2] - p1[2];
}

//////////////////////////////////////////////////////////////////////
// Operations

Vector& Vector::operator*=(float scalar)
{
  m_fPoint[0] *= scalar;
  m_fPoint[1] *= scalar;
  m_fPoint[2] *= scalar;
  return *this;
}

Vector& Vector::operator+=(const Vector& add)
{
  m_fPoint[0] += add.m_fPoint[0];
  m_fPoint[1] += add.m_fPoint[1];
  m_fPoint[2] += add.m_fPoint[2];
  return *this;
}

Vector& Vector::operator-=(const Vector& sub)
{
  m_fPoint[0] -= sub.m_fPoint[0];
  m_fPoint[1] -= sub.m_fPoint[1];
  m_fPoint[2] -= sub.m_fPoint[2];
  return *this;
}

float Vector::Length()
{
  return (float)sqrt(m_fPoint[0]*m_fPoint[0] + m_fPoint[1]*m_fPoint[1] + m_fPoint[2]*m_fPoint[2]);
}

void Vector::Normalize()
{
  float inv = 1.0f / Length();
  m_fPoint[0] *= inv;
  m_fPoint[1] *= inv;
  m_fPoint[2] *= inv;
}

Vector& Vector::Cross(const Vector& v1, const Vector& v2)
{
  m_fPoint[0] = v1.m_fPoint[1]*v2.m_fPoint[2] - v1.m_fPoint[2]*v2.m_fPoint[1];
  m_fPoint[1] = v1.m_fPoint[2]*v2.m_fPoint[0] - v1.m_fPoint[0]*v2.m_fPoint[2];
  m_fPoint[2] = v1.m_fPoint[0]*v2.m_fPoint[1] - v1.m_fPoint[1]*v2.m_fPoint[0];
  return *this;
}

float Vector::Angle(const Vector& vec)
{
  double d, m1, m2;

  d = m_fPoint[0]*vec.m_fPoint[0]+m_fPoint[1]*vec.m_fPoint[1]+m_fPoint[2]*vec.m_fPoint[2];
  m1 = sqrt(m_fPoint[0]*m_fPoint[0]+m_fPoint[1]*m_fPoint[1]+m_fPoint[2]*m_fPoint[2]);
  m2 = sqrt(vec.m_fPoint[0]*vec.m_fPoint[0]+vec.m_fPoint[1]*vec.m_fPoint[1]+vec.m_fPoint[2]*vec.m_fPoint[2]);

  return (float)(RTOD * acos(d / (m1*m2)));
}

float Vector::Dot(const Vector& vec)
{
  return m_fPoint[0]*vec.m_fPoint[0]+m_fPoint[1]*vec.m_fPoint[1]+m_fPoint[2]*vec.m_fPoint[2];
}

void Vector::ToFloat(float *point)
{
  point[0] = m_fPoint[0];
  point[1] = m_fPoint[1];
  point[2] = m_fPoint[2];
}
