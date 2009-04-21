// Light object.

#include "lc_global.h"
#include "light.h"

#include "algebra.h"
#include "lc_colors.h"
#include "matrix.h"

static LC_OBJECT_KEY_INFO light_key_info[LC_LK_COUNT] =
{
	{ "Light Position", 3, LC_LK_POSITION },
	{ "Light Target", 3, LC_LK_TARGET },
	{ "Ambient Color", 3, LC_LK_AMBIENT_COLOR },
	{ "Diffuse Color", 3, LC_LK_DIFFUSE_COLOR },
	{ "Specular Color", 3, LC_LK_SPECULAR_COLOR },
	{ "Constant Attenuation", 1, LC_LK_CONSTANT_ATTENUATION },
	{ "Linear Attenuation", 1, LC_LK_LINEAR_ATTENUATION },
	{ "Quadratic Attenuation", 1, LC_LK_QUADRATIC_ATTENUATION },
	{ "Spot Cutoff", 1, LC_LK_SPOT_CUTOFF },
	{ "Spot Exponent", 1, LC_LK_SPOT_EXPONENT }
};

// =============================================================================
// LightTarget class

LightTarget::LightTarget(lcLight* Parent)
	: lcObject(LC_OBJECT_LIGHT_TARGET)
{
	m_Parent = Parent;
	m_Name = Parent->m_Name + ".Target";
}

LightTarget::~LightTarget()
{
}

void LightTarget::ClosestLineIntersect(lcClickLine& ClickLine) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
	Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

	Matrix44 WorldLight = ((lcLight*)m_Parent)->m_WorldLight;
	WorldLight.SetTranslation(Mul30(-((lcLight*)m_Parent)->m_TargetPosition, WorldLight));

	Vector3 Start = Mul31(ClickLine.Start, WorldLight);
	Vector3 End = Mul31(ClickLine.End, WorldLight);

	float Dist;
	if (BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) && (Dist < ClickLine.Dist))
	{
		ClickLine.Object = this;
		ClickLine.Dist = Dist;
	}
}

bool LightTarget::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	BoundingBox Box;
	Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
	Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

	// Transform the planes to local space.
	Vector4* LocalPlanes = new Vector4[NumPlanes];
	int i;

	Matrix44 WorldLight = ((lcLight*)m_Parent)->m_WorldLight;
	WorldLight.SetTranslation(Mul30(-((lcLight*)m_Parent)->m_TargetPosition, WorldLight));

	for (i = 0; i < NumPlanes; i++)
	{
		LocalPlanes[i] = Vector4(Mul30(Vector3(Planes[i]), WorldLight));
		LocalPlanes[i][3] = Planes[i][3] - Dot3(Vector3(WorldLight[3]), Vector3(LocalPlanes[i]));
	}

	bool Intersect = BoundingBoxIntersectsVolume(Box, LocalPlanes, NumPlanes);

	delete[] LocalPlanes;

	return Intersect;
}

void LightTarget::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	m_Parent->SelectTarget(bSelecting, bFocus, bMultiple);
}

// =============================================================================
// Light class

// New positional light
lcLight::lcLight(float px, float py, float pz)
	: lcObject(LC_OBJECT_LIGHT)
{
	Initialize();

	float pos[] = { px, py, pz }, target[] = { 0, 0, 0 };

	ChangeKey(1, true, pos, LC_LK_POSITION);
	ChangeKey(1, true, target, LC_LK_TARGET);

	UpdatePosition(1);
}

// New directional light
lcLight::lcLight(float px, float py, float pz, float tx, float ty, float tz)
	: lcObject(LC_OBJECT_LIGHT)
{
	Initialize();

	float pos[] = { px, py, pz }, target[] = { tx, ty, tz };

	ChangeKey(1, true, pos, LC_LK_POSITION);
	ChangeKey(1, true, target, LC_LK_TARGET);

	m_Target = new LightTarget(this);

	UpdatePosition(1);
}

void lcLight::Initialize()
{
	m_bEnabled = true;
	m_nState = 0;
	m_Target = NULL;
	m_Name = "";

	m_AmbientColor[3] = 1.0f;
	m_DiffuseColor[3] = 1.0f;
	m_SpecularColor[3] = 1.0f;

	float *values[] = { m_Position, m_TargetPosition, m_AmbientColor, m_DiffuseColor, m_SpecularColor,
	                    &m_ConstantAttenuation, &m_LinearAttenuation, &m_QuadraticAttenuation, &m_SpotCutoff, &m_SpotExponent };
	RegisterKeys(values, light_key_info, LC_LK_COUNT);

	// set the default values
	float ambient[] = { 0, 0, 0 }, diffuse[] = { 0.8f, 0.8f, 0.8f }, specular[] = { 1, 1, 1 };
	float constant = 1, linear = 0, quadratic = 0, cutoff = 30, exponent = 0;

	ChangeKey(1, true, ambient, LC_LK_AMBIENT_COLOR);
	ChangeKey(1, true, diffuse, LC_LK_DIFFUSE_COLOR);
	ChangeKey(1, true, specular, LC_LK_SPECULAR_COLOR);
	ChangeKey(1, true, &constant, LC_LK_CONSTANT_ATTENUATION);
	ChangeKey(1, true, &linear, LC_LK_LINEAR_ATTENUATION);
	ChangeKey(1, true, &quadratic, LC_LK_QUADRATIC_ATTENUATION);
	ChangeKey(1, true, &cutoff, LC_LK_SPOT_CUTOFF);
	ChangeKey(1, true, &exponent, LC_LK_SPOT_EXPONENT);
}

lcLight::~lcLight()
{
	delete m_Target;
}

void lcLight::CreateName(const lcLight* pLight)
{
	int i, max = 0;

	for (; pLight; pLight = (lcLight*)pLight->m_Next)
	{
		if (strncmp(pLight->m_Name, "Light ", 6) == 0)
		{
			if (sscanf((char*)pLight->m_Name + 6, " #%d", &i) == 1)
			{
				if (i > max)
					max = i;
			}
		}
	}

	char Name[256];
	sprintf(Name, "Light #%.2d", max+1);
	m_Name = Name;
}

void lcLight::Select(bool bSelecting, bool bFocus, bool bMultiple)
{
	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_LIGHT_FOCUSED|LC_LIGHT_SELECTED);

			if (m_Target != NULL)
				m_Target->Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_LIGHT_SELECTED;

		if (bMultiple == false)
			if (m_Target != NULL)
				m_Target->Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_LIGHT_FOCUSED);
		else
			m_nState &= ~(LC_LIGHT_SELECTED|LC_LIGHT_FOCUSED);
	}
}

void lcLight::SelectTarget(bool bSelecting, bool bFocus, bool bMultiple)
{
	// TODO: the target should handle this

	if (bSelecting == true)
	{
		if (bFocus == true)
		{
			m_nState |= (LC_LIGHT_TARGET_FOCUSED|LC_LIGHT_TARGET_SELECTED);

			Select(false, true, bMultiple);
		}
		else
			m_nState |= LC_LIGHT_TARGET_SELECTED;

		if (bMultiple == false)
			Select(false, false, bMultiple);
	}
	else
	{
		if (bFocus == true)
			m_nState &= ~(LC_LIGHT_TARGET_FOCUSED);
		else
			m_nState &= ~(LC_LIGHT_TARGET_SELECTED|LC_LIGHT_TARGET_FOCUSED);
	}
}

void lcLight::ClosestLineIntersect(lcClickLine& ClickLine) const
{
	if (m_Target)
	{
		BoundingBox Box;
		Box.m_Max = Vector3(0.2f, 0.2f, 0.2f);
		Box.m_Min = Vector3(-0.2f, -0.2f, -0.2f);

		Vector3 Start = Mul31(ClickLine.Start, m_WorldLight);
		Vector3 End = Mul31(ClickLine.End, m_WorldLight);

		float Dist;
		if (BoundingBoxRayMinIntersectDistance(Box, Start, End, &Dist) && (Dist < ClickLine.Dist))
		{
			ClickLine.Object = this;
			ClickLine.Dist = Dist;
		}

		m_Target->ClosestLineIntersect(ClickLine);
	}
	else
	{
		float Dist;
		if (SphereRayMinIntersectDistance(m_Position, 0.2f, ClickLine.Start, ClickLine.End, &Dist))
		{
			ClickLine.Object = this;
			ClickLine.Dist = Dist;
		}
	}
}

bool lcLight::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	if (m_Target)
	{
		BoundingBox Box;
		Box.m_Max = Vector3(0.3f, 0.3f, 0.3f);
		Box.m_Min = Vector3(-0.3f, -0.3f, -0.3f);

		// Transform the planes to local space.
		Vector4* LocalPlanes = new Vector4[NumPlanes];
		int i;

		for (i = 0; i < NumPlanes; i++)
		{
			LocalPlanes[i] = Vector4(Mul30(Vector3(Planes[i]), m_WorldLight));
			LocalPlanes[i][3] = Planes[i][3] - Dot3(Vector3(m_WorldLight[3]), Vector3(LocalPlanes[i]));
		}

		bool Intersect = BoundingBoxIntersectsVolume(Box, LocalPlanes, NumPlanes);

		delete[] LocalPlanes;

		if (!Intersect)
			Intersect = m_Target->IntersectsVolume(Planes, NumPlanes);

		return Intersect;
	}
	else
	{
		return SphereIntersectsVolume(m_Position, 0.2f, Planes, NumPlanes);
	}
}

void lcLight::Move(u32 Time, bool AddKey, const Vector3& Delta)
{
	if (IsEyeSelected())
	{
		m_Position += Delta;

		ChangeKey(Time, AddKey, m_Position, LC_LK_POSITION);
	}

	if (IsTargetSelected())
	{
		m_TargetPosition += Delta;

		ChangeKey(Time, AddKey, m_TargetPosition, LC_LK_TARGET);
	}
}

void lcLight::UpdatePosition(u32 Time)
{
	CalculateKeys(Time);

	if (m_Target != NULL)
	{
		Vector3 frontvec = m_TargetPosition - m_Position;
		Vector3 up(1, 1, 1);
		float len = Length(frontvec);

		if (fabs(frontvec[0]) < fabs(frontvec[1]))
		{
			if (fabs(frontvec[0]) < fabs(frontvec[2]))
				up[0] = -(up[1]*frontvec[1] + up[2]*frontvec[2]);
			else
				up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
		}
		else
		{
			if (fabs(frontvec[1]) < fabs(frontvec[2]))
				up[1] = -(up[0]*frontvec[0] + up[2]*frontvec[2]);
			else
				up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
		}

		m_WorldLight = CreateLookAtMatrix(m_Position, m_TargetPosition, up);
	}
	else
	{
		m_WorldLight = IdentityMatrix44();
		m_WorldLight.SetTranslation(-m_Position);
	}
}

void lcLight::Render(float fLineWidth)
{
	if (m_Target != NULL)
	{
		if (IsEyeSelected())
		{
			glLineWidth(fLineWidth*2);
			lcSetColor((m_nState & LC_LIGHT_FOCUSED) != 0 ? LC_COLOR_FOCUS : LC_COLOR_SELECTION);
			DrawCone();
			glLineWidth(fLineWidth);
		}
		else
		{
			glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
			DrawCone();
		}

		if (IsTargetSelected())
		{
			glLineWidth(fLineWidth*2);
			lcSetColor((m_nState & LC_LIGHT_TARGET_FOCUSED) != 0 ? LC_COLOR_FOCUS : LC_COLOR_SELECTION);
			DrawTarget();
			glLineWidth(fLineWidth);
		}
		else
		{
			glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
			DrawTarget();
		}

		glColor4f(0.5f, 0.8f, 0.5f, 1.0f);

		float Line[2][3] = 
		{
			{ m_Position[0], m_Position[1], m_Position[2] },
			{ m_TargetPosition[0], m_TargetPosition[1], m_TargetPosition[2] }
		};

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, Line);
		glDrawArrays(GL_LINES, 0, 2);

		if (IsSelected())
		{
			Matrix44 projection, modelview;
			Vector3 frontvec = m_TargetPosition - m_Position;
			Vector3 up(1, 1, 1);
			float len = Length(frontvec);

			if (fabs(frontvec[0]) < fabs(frontvec[1]))
			{
				if (fabs(frontvec[0]) < fabs(frontvec[2]))
					up[0] = -(up[1]*frontvec[1] + up[2]*frontvec[2]);
				else
					up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
			}
			else
			{
				if (fabs(frontvec[1]) < fabs(frontvec[2]))
					up[1] = -(up[0]*frontvec[0] + up[2]*frontvec[2]);
				else
					up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
			}

			glPushMatrix();

			modelview = CreateLookAtMatrix(m_Position, m_TargetPosition, up);
			modelview = RotTranInverse(modelview);
			glMultMatrixf(modelview);

			projection = CreatePerspectiveMatrix(2*m_SpotCutoff, 1.0f, 0.01f, len);
			projection = Inverse(projection);
			glMultMatrixf(projection);

			// Draw the light cone.
			float Verts[16][3] =
			{
				{  0.5f,   1.0f,  1.0f },
				{  1.0f,   0.5f,  1.0f },
				{  1.0f,  -0.5f,  1.0f },
				{  0.5f,  -1.0f,  1.0f },
				{ -0.5f,  -1.0f,  1.0f },
				{ -1.0f,  -0.5f,  1.0f },
				{ -1.0f,   0.5f,  1.0f },
				{ -0.5f,   1.0f,  1.0f },
				{  1.0f,   1.0f, -1.0f },
				{  0.75f,  0.75f, 1.0f },
				{ -1.0f,   1.0f, -1.0f },
				{ -0.75f,  0.75f, 1.0f },
				{ -1.0f,  -1.0f, -1.0f },
				{ -0.75f, -0.75f, 1.0f },
				{  1.0f,  -1.0f, -1.0f },
				{  0.75f, -0.75f, 1.0f }
			};

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, Verts);
			glDrawArrays(GL_LINE_LOOP, 0, 8);
			glDrawArrays(GL_LINES, 8, 8);

			glPopMatrix();
		}
	}
	else
	{
		glPushMatrix();
		glTranslatef(m_Position[0], m_Position[1], m_Position[2]);

		if (IsEyeSelected())
		{
			glLineWidth(fLineWidth*2);
			lcSetColor((m_nState & LC_LIGHT_FOCUSED) != 0 ? LC_COLOR_FOCUS : LC_COLOR_SELECTION);
			DrawSphere();
			glLineWidth(fLineWidth);
		}
		else
		{
			glColor4f(0.5f, 0.8f, 0.5f, 1.0f);
			DrawSphere();
		}

		glPopMatrix();
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

void lcLight::DrawCone()
{
	glPushMatrix();
	glTranslatef(m_Position[0], m_Position[1], m_Position[2]);

	Vector3 frontvec = m_TargetPosition - m_Position;
	Vector3 up(1, 1, 1);
	float len = Length(frontvec);

	if (fabs(frontvec[0]) < fabs(frontvec[1]))
	{
		if (fabs(frontvec[0]) < fabs(frontvec[2]))
			up[0] = -(up[1]*frontvec[1] + up[2]*frontvec[2]);
		else
			up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
	}
	else
	{
		if (fabs(frontvec[1]) < fabs(frontvec[2]))
			up[1] = -(up[0]*frontvec[0] + up[2]*frontvec[2]);
		else
			up[2] = -(up[0]*frontvec[0] + up[1]*frontvec[1]);
	}

	Matrix44 mat = CreateLookAtMatrix(m_Position, m_TargetPosition, up);
	mat = RotTranInverse(mat);
	mat.SetTranslation(Vector3(0, 0, 0));

	glMultMatrixf(mat);

	glEnableClientState(GL_VERTEX_ARRAY);
	float verts[16*3];
	for (int i = 0; i < 8; i++)
	{
		verts[i*6] = verts[i*6+3] = (float)cos((float)i/4 * PI) * 0.3f;
		verts[i*6+1] = verts[i*6+4] = (float)sin((float)i/4 * PI) * 0.3f;
		verts[i*6+2] = 0.3f;
		verts[i*6+5] = -0.3f;
	}
	glVertexPointer(3, GL_FLOAT, 0, verts);
	glDrawArrays(GL_LINES, 0, 16);
	glVertexPointer(3, GL_FLOAT, 6*sizeof(float), verts);
	glDrawArrays(GL_LINE_LOOP, 0, 8);
	glVertexPointer(3, GL_FLOAT, 6*sizeof(float), &verts[3]);
	glDrawArrays(GL_LINE_LOOP, 0, 8);

	float Lines[4][3] =
	{
		{ -0.5f, -0.5f, -0.3f },
		{  0.5f, -0.5f, -0.3f },
		{  0.5f,  0.5f, -0.3f },
		{ -0.5f,  0.5f, -0.3f }
	};

	glVertexPointer(3, GL_FLOAT, 0, Lines);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glTranslatef(0, 0, -len);
}

void lcLight::DrawTarget()
{
	glEnableClientState(GL_VERTEX_ARRAY);
	float box[24][3] =
	{
		{  0.2f,  0.2f,  0.2f }, { -0.2f,  0.2f,  0.2f },
		{ -0.2f,  0.2f,  0.2f }, { -0.2f, -0.2f,  0.2f },
		{ -0.2f, -0.2f,  0.2f }, {  0.2f, -0.2f,  0.2f },
		{  0.2f, -0.2f,  0.2f }, {  0.2f,  0.2f,  0.2f },
		{  0.2f,  0.2f, -0.2f }, { -0.2f,  0.2f, -0.2f },
		{ -0.2f,  0.2f, -0.2f }, { -0.2f, -0.2f, -0.2f },
		{ -0.2f, -0.2f, -0.2f }, {  0.2f, -0.2f, -0.2f },
		{  0.2f, -0.2f, -0.2f }, {  0.2f,  0.2f, -0.2f },
		{  0.2f,  0.2f,  0.2f }, {  0.2f,  0.2f, -0.2f },
		{ -0.2f,  0.2f,  0.2f }, { -0.2f,  0.2f, -0.2f },
		{ -0.2f, -0.2f,  0.2f }, { -0.2f, -0.2f, -0.2f },
		{  0.2f, -0.2f,  0.2f }, {  0.2f, -0.2f, -0.2f }
	};
	glVertexPointer(3, GL_FLOAT, 0, box);
	glDrawArrays(GL_LINES, 0, 24);
	glPopMatrix();
}

void lcLight::DrawSphere()
{
	glEnableClientState(GL_VERTEX_ARRAY);

	const float radius = 0.2f;
	const int slices = 6, stacks = 6;
	float rho, drho, theta, dtheta;
	int i, j, imin, imax;
	drho = 3.1415926536f/(float)stacks;
	dtheta = 2.0f*3.1415926536f/(float)slices;

	// draw +Z end as a triangle fan
	float Cap[slices+2][3];

	Cap[0][0] = 0.0f;
	Cap[0][1] = 0.0f;
	Cap[0][2] = radius;

	for (j = 0; j <= slices; j++) 
	{
		theta = (j == slices) ? 0.0f : j * dtheta;
		Cap[j+1][0] = (float)(-sin(theta) * sin(drho)) * radius;
		Cap[j+1][1] = (float)(cos(theta) * sin(drho)) * radius;
		Cap[j+1][2] = (float)(cos(drho)) * radius;
	}

	glVertexPointer(3, GL_FLOAT, 0, Cap);
	glDrawArrays(GL_TRIANGLE_FAN, 0, slices+2);

	imin = 1;
	imax = stacks-1;

	float Center[(slices+1)*2][3];
	glVertexPointer(3, GL_FLOAT, 0, Center);

	for (i = imin; i < imax; i++)
	{
		rho = i * drho;

		for (j = 0; j <= slices; j++)
		{
			theta = (j == slices) ? 0.0f : j * dtheta;
			Center[j*2][0] = (float)(-sin(theta) * sin(rho)) * radius;
			Center[j*2][1] = (float)(cos(theta) * sin(rho)) * radius;
			Center[j*2][2] = (float)(cos(rho)) * radius;
			Center[j*2+1][0] = (float)(-sin(theta) * sin(rho+drho)) * radius;
			Center[j*2+1][1] = (float)(cos(theta) * sin(rho+drho)) * radius;
			Center[j*2+1][2] = (float)(cos(rho+drho)) * radius;
		}

		glDrawArrays(GL_TRIANGLE_STRIP, 0, (slices+1)*2);
	}

	// draw -Z end as a triangle fan
	Cap[0][0] = 0.0f;
	Cap[0][1] = 0.0f;
	Cap[0][2] = -radius;

	rho = 3.1415926536f - drho;
	for (j = slices; j >= 0; j--)
	{
		theta = (j==slices) ? 0.0f : j * dtheta;
		Cap[j+1][0] = (float)(-sin(theta) * sin(rho)) * radius;
		Cap[j+1][1] = (float)(cos(theta) * sin(rho)) * radius;
		Cap[j+1][2] = (float)(cos(rho)) * radius;
	}

	glVertexPointer(3, GL_FLOAT, 0, Cap);
	glDrawArrays(GL_TRIANGLE_FAN, 0, slices+2);
}

void lcLight::Setup(int index)
{
	GLenum light = (GLenum)(GL_LIGHT0+index);

	if (!m_bEnabled)
	{
		glDisable(light);
		return;
	}

	bool Omni = (m_Target == NULL);
	bool Spot = (m_Target != NULL) && (m_SpotCutoff != 180.0f);

	glEnable(light);

	glLightfv(light, GL_AMBIENT, m_AmbientColor);
	glLightfv(light, GL_DIFFUSE, m_DiffuseColor);
	glLightfv(light, GL_SPECULAR, m_SpecularColor);

	Vector4 Position(m_Position);

	if (Omni || Spot)
	{
		glLightf(light, GL_CONSTANT_ATTENUATION, m_ConstantAttenuation);
		glLightf(light, GL_LINEAR_ATTENUATION, m_LinearAttenuation);
		glLightf(light, GL_QUADRATIC_ATTENUATION, m_QuadraticAttenuation);

		Position[3] = 1.0f;
		glLightfv(light, GL_POSITION, Position);
	}
	else
	{
		glLightf(light, GL_CONSTANT_ATTENUATION, 1.0f);
		glLightf(light, GL_LINEAR_ATTENUATION, 0.0f);
		glLightf(light, GL_QUADRATIC_ATTENUATION, 0.0f);

		Position[3] = 0.0f;
		glLightfv(light, GL_POSITION, Position);
	}

	if (Omni)
	{
		Vector3 dir(0.0f, 0.0f, 0.0f);

		glLightf(light, GL_SPOT_CUTOFF, 180.0f);
		glLightf(light, GL_SPOT_EXPONENT, m_SpotExponent);
		glLightfv(light, GL_SPOT_DIRECTION, dir);
	}
	else
	{
		Vector3 dir = m_TargetPosition - m_Position;
		dir = Normalize(dir);

		glLightf(light, GL_SPOT_CUTOFF, m_SpotCutoff);
		glLightf(light, GL_SPOT_EXPONENT, m_SpotExponent);
		glLightfv(light, GL_SPOT_DIRECTION, dir);
	}
}
