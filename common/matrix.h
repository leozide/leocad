// Matrix class
//

#ifndef _MATRIX_H_
#define _MATRIX_H_

class Matrix
{
public:
	Matrix();
	Matrix(const float *mat);
	Matrix(const float *rot, const float *pos);
	~Matrix() { };

	void FromPacked(const float *mat);
	void FromFloat(const float* mat);
	void FromLDraw(const float *f);
	void FromEulerAngles(float yaw, float pitch, float roll);
	void FromAxisAngle(const float *axis, float angle);

	void ToLDraw(float *f) const;
	void ToEulerAngles(float *rot) const;
	void ToAxisAngle(float *rot) const;

	void LoadIdentity();
	void Translate(float x, float y, float z);
	void Multiply(const Matrix& m1, const Matrix& m2);
	bool Invert();
	void Transpose3();
	float Determinant() const;

	void GetTranslation(float *x, float *y, float *z);
	void SetTranslation(float x, float y, float z);
	void GetTranslation(float pos[3]);
	void SetTranslation(float pos[3]);

	void TransformPoint(float out[], const float in[3]);
	void TransformPoints(float p[], int n);
	void CreateOld(float mx, float my, float mz, float rx, float ry, float rz);
	void Rotate(float angle, float x, float y, float z);
	void RotateCenter(float angle, float x, float y, float z, float px, float py, float pz);
	void CreatePerspective(float fovy, float aspect, float nearval, float farval);
	void CreateLookat(const float *eye, const float *target, const float *up);

public:
	float m[16];
};

#endif //_MATRIX_H_
