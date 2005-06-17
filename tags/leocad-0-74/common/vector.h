// Vector.h: interface for the Vector class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _VECTOR_H_
#define _VECTOR_H_

class Vector  
{
 public:
  Vector ();
  Vector (float x, float y, float z);
  Vector (const float *point);
  Vector (const float *p1, const float *p2);
  virtual ~Vector ();

  float X ()
    { return m_fPoint[0]; }
  float Y ()
    { return m_fPoint[1]; }
  float Z ()
    { return m_fPoint[2]; }

  float Dot (Vector& vec);
  float Angle (Vector& vec);
  Vector& Cross (Vector& v1, Vector& v2);
  void Add (const float *point);
  void Add (float x, float y, float z);
  Vector& operator+=(Vector& add);
  Vector& operator-=(Vector& sub);
  Vector& operator*=(float scalar);
  Vector& operator=(Vector& source);
  Vector& operator=(const float *point);
  bool operator==(Vector& vec);

  operator const float* () const
    { return m_fPoint; }
  void ToFloat (float *point);
  void FromFloat (const float *point);
  void FromFloat (float x, float y, float z);
  float Length ();
  void Normalize ();
  void Scale (float scale);

 protected:
  float m_fPoint[3];
};

#endif // _VECTOR_H_
