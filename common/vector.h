#ifndef _VECTOR_H_
#define _VECTOR_H_

class Vector
{
public:
	Vector();
	Vector(float x, float y, float z);
	Vector(const float *point);
	Vector(const float *p1, const float *p2);
	~Vector() { };

	float Dot(Vector& vec);
	float Angle(Vector& vec);
	Vector& Cross(Vector& v1, Vector& v2);
	Vector& operator+=(Vector& add);
	Vector& operator-=(Vector& sub);
	Vector& operator*=(float scalar);

	operator const float*() const
	{ return m_fPoint; }
	void ToFloat(float *point);
	float Length();
	void Normalize();

protected:
	float m_fPoint[3];
};

#endif // _VECTOR_H_
