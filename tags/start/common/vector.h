// Vector.h: interface for the Vector class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _VECTOR_H_
#define _VECTOR_H_

class Vector  
{
public:
	float X();
	float Y();
	float Z();
	float Dot(Vector& vec);
	float Angle(Vector& vec);
	void Add(float point[3]);
	void Add(float x, float y, float z);
	Vector& operator+=(Vector& add);
	Vector& operator*=(float scalar);
	Vector& operator=(Vector& source);
	Vector& operator=(float point[3]);
	bool operator==(Vector& vec);

	Vector& Cross(Vector& v1, Vector& v2);

	void ToFloat(float point[3]);
	void FromFloat(float point[3]);
	void FromFloat(float x, float y, float z);
	float Length();
	void Normalize();
	void Scale(float scale);

	Vector(float x, float y, float z);
	Vector(float point[3]);
	Vector(float p1[3], float p2[3]);
	Vector();
	virtual ~Vector();

protected:
	float m_fPoint[3];
};

#endif // _VECTOR_H_
