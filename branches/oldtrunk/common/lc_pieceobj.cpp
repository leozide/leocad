#include "lc_global.h"
#include "lc_pieceobj.h"

#include "lc_mesh.h"
#include "defines.h"
#include "lc_application.h"
#include "project.h"

lcPieceObject::lcPieceObject(u32 Type)
	: lcObject(Type, LC_PIECEOBJ_NUMKEYS)
{
	m_Color = 0;
	m_TimeShow = 1;
	m_TimeHide = LC_MAX_TIME;
	m_Mesh = NULL;

	ChangeKey(0, false, LC_PIECEOBJ_ROTATION, Vector4(0, 0, 1, 0));
}

lcPieceObject::lcPieceObject(const lcPieceObject* Object)
	: lcObject(Object->GetType(), LC_PIECEOBJ_NUMKEYS)
{
	// lcObject members.
	m_ParentPosition = Object->m_ParentPosition;
	m_WorldPosition = Object->m_WorldPosition;
	m_Name = "";
	m_Flags = Object->m_Flags;

	for (int i = 0; i < LC_PIECEOBJ_NUMKEYS; i++)
		m_Keys[i] = Object->m_Keys[i];

	// lcPieceObj members.
	m_Color = Object->m_Color;
	m_TimeShow = Object->m_TimeShow;
	m_TimeHide = Object->m_TimeHide;
	m_Mesh = NULL;

	m_ModelWorld = Object->m_ModelWorld;
	m_AxisAngle = Object->m_AxisAngle;
	m_BoundingBox = Object->m_BoundingBox;
}

lcPieceObject::~lcPieceObject()
{
	delete m_Mesh;
}

void lcPieceObject::Update(u32 Time)
{
	if (!IsVisible(Time))
		SetSelection(false, true);

	// Update key values.
	CalculateKey(Time, LC_PIECEOBJ_POSITION, &m_ParentPosition);
	CalculateKey(Time, LC_PIECEOBJ_ROTATION, &m_AxisAngle);

	if (m_Parent)
	{
		Matrix44 ModelParent = MatrixFromAxisAngle(m_AxisAngle);
		ModelParent.SetTranslation(m_ParentPosition);

		m_ModelWorld = Mul(ModelParent, ((lcPieceObject*)m_Parent)->m_ModelWorld);
		m_WorldPosition = Vector3(m_ModelWorld[3]);
	}
	else
	{
		m_ModelWorld = MatrixFromAxisAngle(m_AxisAngle);
		m_ModelWorld.SetTranslation(m_ParentPosition);
		m_WorldPosition = m_ParentPosition;
	}
}

void lcPieceObject::SetRotation(u32 Time, bool AddKey, const Vector4& NewRotation)
{
	ChangeKey(Time, AddKey, LC_PIECEOBJ_ROTATION, NewRotation);
}

void lcPieceObject::MergeBoundingBox(BoundingBox* Box)
{
	Vector3 Points[8];

	m_BoundingBox.GetPoints(Points);

	for (int i = 0; i < 8; i++)
		Box->AddPoint(Mul31(Points[i], m_ModelWorld));
}

void lcPieceObject::InsertTime(u32 Time, u32 Frames)
{
	if (m_TimeShow >= Time)
		m_TimeShow = min(m_TimeShow + Frames, LC_MAX_TIME);

	if (m_TimeHide >= Time)
		m_TimeHide = min(m_TimeHide + Frames, LC_MAX_TIME);

	if (m_TimeShow > (u32)lcGetActiveProject()->GetCurrentTime())
		SetSelection(false, true);

	lcObject::InsertTime(Time, Frames);
}

void lcPieceObject::RemoveTime(u32 Time, u32 Frames)
{
	if (m_TimeShow >= Time)
		m_TimeShow = max(m_TimeShow - Frames, 1);

	if (m_TimeHide != LC_MAX_TIME)
		m_TimeHide = max(m_TimeHide - Frames, 1);

	if (m_TimeHide < (u32)lcGetActiveProject()->GetCurrentTime())
		SetSelection(false, true);

	lcObject::RemoveTime(Time, Frames);
}
