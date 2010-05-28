#include <stdio.h>
#include "lc_global.h"
#include "lc_viewpoint.h"

#include "lc_application.h"
#include "view.h"

lcViewpoint::lcViewpoint()
{
	mFOV = 30;
	mNearDist = 1.0f;
	mFarDist = 500.0f;
}

lcViewpoint::~lcViewpoint()
{
}

void lcViewpoint::SetDefault(int Viewpoint)
{
	Vector3 Positions[] = { Vector3(0.0f, -50.0f, 0.0f), Vector3(0.0f, 50.0f, 0.0f), Vector3(-50.0f, 0.0f, 0.0f), Vector3(50.0f, 0.0f, 0.0f), 
	                        Vector3(0.0f, 0.0f, 50.0f), Vector3(0.0f, 0.0f, -50.0f), Vector3(-10.0f, -10.0f, 5.0f) };
	float Rolls[] = { 0.0f, 0.0f, -LC_PI/2, -LC_PI/2, 0.0f, 0.0f, 0.0f };

	SetPosition(1, false, Positions[Viewpoint]);
	SetTarget(1, false, Vector3(0, 0, 0));
	SetRoll(1, false, Rolls[Viewpoint]);

	CalculateMatrices();
}

void lcViewpoint::CalculateMatrices()
{
	Vector3 Z = Normalize(mPosition - mTarget);

	// Build the Y vector of the matrix.
	Vector3 UpVector;

	if (fabsf(Z[0]) < 0.001f && fabsf(Z[1]) < 0.001f)
		UpVector = Vector3(-Z[2], 0, 0);
	else
		UpVector = Vector3(0, 0, 1);

	// Calculate X vector.
	Vector3 X = Cross(UpVector, Z);

	// Calculate real Y vector.
	Vector3 Y = Cross(Z, X);

	// Apply the roll rotation and recalculate X and Y.
	Matrix33 RollMat = MatrixFromAxisAngle(Z, mRoll);
	Y = Normalize(Mul(Y, RollMat));
	X = Normalize(Cross(Y, Z));

	// Build matrices.
	Vector4 Row0 = Vector4(X[0], Y[0], Z[0], 0.0f);
	Vector4 Row1 = Vector4(X[1], Y[1], Z[1], 0.0f);
	Vector4 Row2 = Vector4(X[2], Y[2], Z[2], 0.0f);
	Vector4 Row3 = Vector4(Vector3(Row0 * -mPosition[0] + Row1 * -mPosition[1] + Row2 * -mPosition[2]), 1.0f);

	mWorldView = Matrix44(Row0, Row1, Row2, Row3);
	mViewWorld = RotTranInverse(mWorldView);
}

void lcViewpoint::Zoom(u32 Time, bool AddKey, int MouseX, int MouseY)
{
	float Sensitivity = 2.0f / (LC_MAX_MOUSE_SENSITIVITY+1 - g_App->m_MouseSensitivity);
	float dy = MouseY * Sensitivity;

	/*
	if (IsOrtho())
	{
		// TODO: have a different option to change the FOV.
		m_FOV += dy;
		m_FOV = lcClamp(m_FOV, 0.001f, 179.999f);
	}
	else
	*/
	{
		Vector3 Delta = Vector3(mViewWorld[2]) * dy;

		// TODO: option to move eye, target or both
		SetPosition(Time, AddKey, mPosition + Delta);
		SetTarget(Time, AddKey, mTarget + Delta);
	}

	CalculateMatrices();
}

void lcViewpoint::Pan(u32 Time, bool AddKey, int MouseX, int MouseY)
{
	float Sensitivity = 2.0f / (LC_MAX_MOUSE_SENSITIVITY+1 - g_App->m_MouseSensitivity);
	float dx = MouseX * Sensitivity;
	float dy = MouseY * Sensitivity;

	Vector3 Delta = Vector3(mViewWorld[0]) * -dx + Vector3(mViewWorld[1]) * -dy;

	SetPosition(Time, AddKey, mPosition + Delta);
	SetTarget(Time, AddKey, mTarget + Delta);

	CalculateMatrices();
}

void lcViewpoint::Orbit(u32 Time, bool AddKey, int MouseX, int MouseY)
{
	float Sensitivity = 2.0f / (LC_MAX_MOUSE_SENSITIVITY+1 - g_App->m_MouseSensitivity);
	float dx = MouseX * Sensitivity;
	float dy = MouseY * Sensitivity;

	Vector3 Dir = mPosition - mTarget;

	// The X axis of the mouse always corresponds to Z in the world.
	if (fabsf(dx) > 0.01f)
	{
		float AngleX = -dx * LC_DTOR;
		Matrix33 RotX = MatrixFromAxisAngle(Vector4(0, 0, 1, AngleX));

		Dir = Mul(Dir, RotX);
	}

	// The Y axis will be the side vector.
	if (fabsf(dy) > 0.01f)
	{
		float AngleY = dy * LC_DTOR;
		Matrix33 RotY = MatrixFromAxisAngle(Vector4(mWorldView[0][0], mWorldView[1][0], mWorldView[2][0], AngleY));

		Dir = Mul(Dir, RotY);
	}

	SetPosition(Time, AddKey, Dir + mTarget);

	CalculateMatrices();
}

void lcViewpoint::Rotate(u32 Time, bool AddKey, int MouseX, int MouseY)
{
	float Sensitivity = 2.0f / (LC_MAX_MOUSE_SENSITIVITY+1 - g_App->m_MouseSensitivity);
	float dx = MouseX * Sensitivity;
	float dy = MouseY * Sensitivity;

	Vector3 Dir = mTarget - mPosition;

	// The X axis of the mouse always corresponds to Z in the world.
	if (fabsf(dx) > 0.01f)
	{
		float AngleX = -dx * LC_DTOR;
		Matrix33 RotX = MatrixFromAxisAngle(Vector4(0, 0, 1, AngleX));

		Dir = Mul(Dir, RotX);
	}

	// The Y axis will the side vector of the camera.
	if (fabsf(dy) > 0.01f)
	{
		float AngleY = dy * LC_DTOR;
		Matrix33 RotY = MatrixFromAxisAngle(Vector4(mWorldView[0][0], mWorldView[1][0], mWorldView[2][0], AngleY));

		Dir = Mul(Dir, RotY);
	}

	SetTarget(Time, AddKey, Dir + mPosition);

	CalculateMatrices();
}

void lcViewpoint::Roll(u32 Time, bool AddKey, int MouseX, int MouseY)
{
	float Sensitivity = 2.0f / (LC_MAX_MOUSE_SENSITIVITY+1 - g_App->m_MouseSensitivity);
	float dx = MouseX * Sensitivity;

	float NewRoll = mRoll + dx / 100;

	SetRoll(Time, AddKey, NewRoll);

	CalculateMatrices();
}

void lcViewpoint::ZoomExtents(u32 Time, bool AddKey, View* view, lcObjArray<Vector3>& Points)
{
	float Aspect = (float)view->GetWidth()/(float)view->GetHeight();
	Matrix44 Projection = CreatePerspectiveMatrix(mFOV, Aspect, mNearDist, mFarDist);

	Vector3 Position = ::ZoomExtents(mPosition, mWorldView, Projection, &Points[0], Points.GetSize());

	SetPosition(Time, AddKey, Position);

	CalculateMatrices();
}
