//
// Math and Linear Algebra stuff.
//

#include "algebra.h"

// ============================================================================
// 4x4 Matrix class.

void Matrix44::CreateLookAt(const Point3& Eye, const Point3& Target, const Vector3& Up)
{
	Vector3 x, y, z;

	// Z = Eye - Target
	z = Eye - Target;

  // X = Y Cross Z
	x = Cross3(Up, z);

  // Y = Z Cross X
	y = Cross3(z, x);

	// Normalize everything.
	x.Normalize();
	y.Normalize();
	z.Normalize();

	m_Rows[0] = Float4(x.GetX(), y.GetX(), z.GetX(), 0.0f);
	m_Rows[1] = Float4(x.GetY(), y.GetY(), z.GetY(), 0.0f);
	m_Rows[2] = Float4(x.GetZ(), y.GetZ(), z.GetZ(), 0.0f);
	m_Rows[3] = m_Rows[0]*-Eye.GetX() + m_Rows[1]*-Eye.GetY() + m_Rows[2]*-Eye.GetZ();
}
