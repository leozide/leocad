#ifndef _MATRIX_H_
#define _MATRIX_H_

class Matrix
{
public:
	Matrix();
	Matrix(const float *rot, const float *pos);
	~Matrix() { };

	void FromFloat(const float* mat);
	void FromLDraw(const float *f);

	void ToLDraw(float *f) const;
	void ToAxisAngle(float *rot) const;

	void LoadIdentity();
	void Translate(float x, float y, float z);
	void Multiply(const Matrix& m1, const Matrix& m2);
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

public:
	float m[16];
};

#endif //_MATRIX_H_
