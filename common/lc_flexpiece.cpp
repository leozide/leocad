#include "lc_global.h"
#include "lc_flexpiece.h"

#include "array.h"
#include "lc_scene.h"
#include "lc_mesh.h"
#include "pieceinf.h"

lcFlexiblePiece::lcFlexiblePiece(PieceInfo* Info)
	: lcPieceObject(LC_OBJECT_FLEXPIECE)
{
	m_PieceInfo = Info;

	if (m_PieceInfo)
		m_PieceInfo->AddRef();

	// FIXME: bounding box calculation
	m_BoundingBox = BoundingBox(Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[5]),
	                            Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[2]));

	for (int i = 0; i < 4; i++)
	{
		lcFlexiblePiecePoint* Point = new lcFlexiblePiecePoint(this);
		Point->SetPosition(1, true, Vector3(0.0f, 0.0f, 1.0f * i));

		Point->m_Next = m_Children;
		m_Children = Point;
	}
}

lcFlexiblePiece::lcFlexiblePiece(lcFlexiblePiece* Piece)
	: lcPieceObject(LC_OBJECT_FLEXPIECE)
{
	// lcObject members.
	m_Next = NULL;
	m_Parent = NULL;
	m_Children = NULL;

	m_ParentPosition = Piece->m_ParentPosition;
	m_WorldPosition = Piece->m_WorldPosition;
	m_Name = "";
	m_Flags = Piece->m_Flags;

	for (int i = 0; i < LC_PIECEOBJ_NUMKEYS; i++)
		m_Keys[i] = Piece->m_Keys[i];

	// lcPieceObj members.
	m_Color = Piece->m_Color;
	m_TimeShow = Piece->m_TimeShow;
	m_TimeHide = Piece->m_TimeHide;
	m_Mesh = NULL;

	m_ModelWorld = Piece->m_ModelWorld;
	m_AxisAngle = Piece->m_AxisAngle;
	m_BoundingBox = Piece->m_BoundingBox;

	// lcFlexiblePiece members.
	m_PieceInfo = Piece->m_PieceInfo;
	if (m_PieceInfo)
		m_PieceInfo->AddRef();

	// FIXME: copy correctly
	lcObject* Last = NULL;
	for (lcObject* Point = Piece->m_Children; Point; Point = Point->m_Next)
	{
		lcFlexiblePiecePoint* NewPoint = new lcFlexiblePiecePoint(this);
		NewPoint->SetPosition(1, true, Point->m_ParentPosition);

		if (Last)
			Last->m_Next = NewPoint;
		else
			m_Children = NewPoint;
		Last = NewPoint;
	}

}

lcFlexiblePiece::~lcFlexiblePiece()
{
	while (m_Children)
	{
		lcObject* Point = m_Children;
		m_Children = m_Children->m_Next;
		delete Point;
	}

	if (m_PieceInfo)
		m_PieceInfo->DeRef();
}

void lcFlexiblePiece::Move(u32 Time, bool AddKey, const Vector3& Delta)
{
	bool ChildSelected = false;

	for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
	{
		if (Point->IsSelected())
		{
			Point->Move(Time, AddKey, Delta);
			ChildSelected = true;
		}
	}

	if (!ChildSelected)
		lcObject::Move(Time, AddKey, Delta);
}

void lcFlexiblePiece::Update(u32 Time)
{
	lcPieceObject::Update(Time);

	for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
		Point->Update(Time);

	// FIXME: don't call buildmesh here
	BuildMesh();
}

void lcFlexiblePiece::AddToScene(lcScene* Scene, int Color)
{
	// FIXME: lcFlexiblePiece

	m_Mesh->AddToScene(Scene, m_ModelWorld, (m_Color == LC_COL_DEFAULT) ? Color : m_Color, this);

	for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
		Point->AddToScene(Scene, Color);
}

void lcFlexiblePiece::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	// FIXME: lcFlexiblePiece

	for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
		Point->ClosestRayIntersect(Ray);
}

bool lcFlexiblePiece::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	// FIXME: lcFlexiblePiece
	return false;
}

void lcFlexiblePiece::BuildMesh()
{
	delete m_Mesh;
	m_Mesh = NULL;

	ObjArray<Vector4> ControlPoints(16);
	for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
		ControlPoints.Add(Vector4(Point->m_ParentPosition, 1));

	int NumSections = 3;
	int NumSegments = 16;
	int NumSlices = 12;
	int NumIndices = 6 * (NumSlices) * (NumSegments-1) * NumSections;
	int NumVertices = NumSlices * NumSegments * NumSections;
	float r = 0.2f;

	m_Mesh = new lcMesh(1, NumIndices, NumVertices, NULL);

	lcMeshEditor<u16> MeshEdit(m_Mesh);
	MeshEdit.StartSection(GL_TRIANGLES, LC_COL_DEFAULT);
	// fixme: section bounding box

//	Matrix44 Blend(Vector4(-1, 3, -3, 1), Vector4(3, -6, 3, 0), Vector4(-3, 3, 0, 0), Vector4(1, 0, 0, 0));
//	Matrix44 Tangent(Vector4(0, 0, 0, 0), Vector4(-3, 9, -9, 3), Vector4(6,-12, 6, 0), Vector4(-3, 3, 0, 0));

	for (int i = 0; i < 3; i++)
	{
//		Matrix44 Segment(ControlPoints[i], ControlPoints[i+1], ControlPoints[i+2], ControlPoints[i+3]);

		for (int j = 0; j < NumSegments; j++)
		{
			float t = (float)j/(NumSegments-1);
//			Vector3 Pos = Mul31((Mul31(Vector3(t*t*t,t*t,t), Blend)), Segment);
//			Vector3 Tan = Mul31((Mul31(Vector3(0,t*t,t), Tangent)), Segment);
/*
			Matrix33 RotMat;
			Vector3 Z = Normalize(Tan);

			if (fabsf(Dot3(Z, Vector3(0, 0, 1))) < 0.99f)
			{
				Vector3 X = Normalize(Cross3(Vector3(0, 0, 1), Tan));
				Vector3 Y = Normalize(Cross3(Tan, X));

				RotMat = Matrix33(X, Y, Z);
			}
			else
			{
				RotMat = IdentityMatrix33();
			}
*/
Vector3 p0, p1, p2, p3;

if (i == 0)
{
			p0 = Vector3(ControlPoints[0]);
			p1 = Vector3(ControlPoints[0]);
			p2 = Vector3(ControlPoints[1]);
			p3 = Vector3(ControlPoints[2]);
}
else if (i == 1)
{
			p0 = Vector3(ControlPoints[0]);
			p1 = Vector3(ControlPoints[1]);
			p2 = Vector3(ControlPoints[2]);
			p3 = Vector3(ControlPoints[3]);
}
else if (i == 2)
{
			p0 = Vector3(ControlPoints[1]);
			p1 = Vector3(ControlPoints[2]);
			p2 = Vector3(ControlPoints[3]);
			p3 = Vector3(ControlPoints[3]);
}

			float t2 = t * t;
			float t3 = t2 * t;
			Vector3 Pos = 0.5f * ( ( 2.0f * p1 ) + ( -p0 + p2 ) * t + ( 2.0f * p0 - 5.0f * p1 + 4 * p2 - p3 ) * t2 + ( -p0 + 3.0f * p1 - 3.0f * p2 + p3 ) * t3 );
			Vector3 Tan = 0.5f * ( ( -p0 + p2 ) + 2 * ( 2.0f * p0 - 5.0f * p1 + 4 * p2 - p3 ) * t + 3 * ( -p0 + 3.0f * p1 - 3.0f * p2 + p3 ) * t2 );

			for (int k = 0; k < NumSlices; k++)
			{
				Vector3 Vertex(r * sinf(k * LC_2PI / NumSlices), r * cosf(k * LC_2PI / NumSlices), 0);

//				Vertex = Mul(Vertex, RotMat) + Pos;
				Vertex = Vertex + Pos;

				MeshEdit.AddVertex(Vertex);

				if (j != NumSegments-1)
				{
					if (k != NumSlices-1)
					{
						MeshEdit.AddIndex(j * NumSlices + k);
						MeshEdit.AddIndex(j * NumSlices + k + 1);
						MeshEdit.AddIndex((j+1) * NumSlices + k);
						MeshEdit.AddIndex(j * NumSlices + k + 1);
						MeshEdit.AddIndex((j+1) * NumSlices + k + 1);
						MeshEdit.AddIndex((j+1) * NumSlices + k);
					}
					else
					{
						MeshEdit.AddIndex(j * NumSlices + k);
						MeshEdit.AddIndex(j * NumSlices);
						MeshEdit.AddIndex((j+1) * NumSlices + k);
						MeshEdit.AddIndex(j * NumSlices);
						MeshEdit.AddIndex((j+1) * NumSlices);
						MeshEdit.AddIndex((j+1) * NumSlices + k);
					}
				}
			}
		}

		MeshEdit.OffsetIndices(6 * (NumSlices) * (NumSegments-1) * i, 6 * (NumSlices) * (NumSegments-1), NumSlices * NumSegments * i);
	}

	MeshEdit.EndSection();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME: split this into another file

#define LC_FLEXPIECE_POINT_SIZE 0.2f

lcFlexiblePiecePoint::lcFlexiblePiecePoint(lcFlexiblePiece* Parent)
	: lcObject(LC_OBJECT_FLEXPIECE_POINT, LC_FLEXPIECE_POINT_NUMKEYS)
{
	m_Parent = Parent;
}

lcFlexiblePiecePoint::~lcFlexiblePiecePoint()
{
}

void lcFlexiblePiecePoint::Update(u32 Time)
{
	CalculateKey(Time, LC_FLEXPIECE_POINT_POSITION, &m_ParentPosition);
	m_WorldPosition = Mul31(m_ParentPosition, ((lcPieceObject*)m_Parent)->m_ModelWorld);
}

void lcFlexiblePiecePoint::AddToScene(lcScene* Scene, int Color)
{
	// FIXME: lcFlexiblePiece

	Matrix44 ScaleMatrix(Vector4(LC_FLEXPIECE_POINT_SIZE, 0.0f, 0.0f, 0.0f), Vector4(0.0f, LC_FLEXPIECE_POINT_SIZE, 0.0f, 0.0f), Vector4(0.0f, 0.0f, LC_FLEXPIECE_POINT_SIZE, 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f));

	Matrix44 ModelWorld = IdentityMatrix44();
	ModelWorld.SetTranslation(m_WorldPosition);

	lcRenderSection RenderSection;

	RenderSection.Owner = (lcPieceObject*)this;
	RenderSection.ModelWorld = Mul(ScaleMatrix, ModelWorld);;
	RenderSection.Mesh = lcSphereMesh;
	RenderSection.Section = &lcSphereMesh->m_Sections[0];
	RenderSection.Color = 0;

	if (IsFocused())
		RenderSection.Color = LC_COL_FOCUSED;
	else if (IsSelected())
		RenderSection.Color = LC_COL_SELECTED;

	Scene->m_OpaqueSections.Add(RenderSection);
}

void lcFlexiblePiecePoint::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	float Dist = LinePointMinDistance(m_WorldPosition, Ray->Start, Ray->End);

	if (Dist < Ray->Dist)
	{
		Ray->Object = this;
		Ray->Dist = Dist;
	}

	// FIXME: lcFlexiblePiece
}

bool lcFlexiblePiecePoint::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	// FIXME: lcFlexiblePiece
	return false;
}
