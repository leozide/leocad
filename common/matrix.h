// Matrix class
//

#ifndef _MATRIX_H_
#define _MATRIX_H_

class File;

class Matrix
{
public:
	Matrix();
	Matrix(float* floats);
	Matrix(double mat[16]);
	Matrix(float rot[4], float pos[3]);
	~Matrix();

	void WriteToFile (File* F);
	void ReadFromFile (File* F);
	void Multiply (Matrix& m1, Matrix& m2);
	void ConvertToLDraw(float f[12]);
	void ConvertFromLDraw(float f[12]);
	void GetEulerAngles (float rot[3]);
	void LoadIdentity();
	void GetTranslation(float *x, float *y, float *z);
	void SetTranslation(float x, float y, float z);
	void GetTranslation(float pos[3]);
	void SetTranslation(float pos[3]);
	void TransformPoint(float out[], const float in[3]);
	void TransformPoints (float p[], int n);
	void Create (float mx, float my, float mz, float rx, float ry, float rz);
	void CreateOld(float mx, float my, float mz, float rx, float ry, float rz);
	void Rotate(float angle, float x, float y, float z);
	void RotateCenter(float angle, float x, float y, float z, float px, float py, float pz);
	void Translate(float x, float y, float z);
	void FromEuler(float yaw, float pitch, float roll);
	void ToAxisAngle(float rot[4]);
	void FromAxisAngle(float axis[3], float angle);
	void FromFloat(float* mat);
	bool FromInverse(double* src);
	void CreatePerspective (float fovy, float aspect, float nearval, float farval);
	void CreateLookat (float eye[3], float target[3], float up[3]);
	bool Invert ();

	float m[16];
};

#endif //_MATRIX_H_
