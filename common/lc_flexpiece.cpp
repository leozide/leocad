#include "lc_global.h"
#include "lc_flexpiece.h"

#include "array.h"
#include "lc_scene.h"
#include "lc_mesh.h"
#include "lc_colors.h"
#include "pieceinf.h"
#include "typedefs.h"

lcFlexiblePiece::lcFlexiblePiece(PieceInfo* Info)
	: lcPieceObject(LC_OBJECT_FLEXPIECE)
{
	m_PieceInfo = Info;

	if (m_PieceInfo)
		m_PieceInfo->AddRef();

	// FIXME: bounding box calculation
	m_BoundingBox = BoundingBox(Vector3(m_PieceInfo->m_fDimensions[3], m_PieceInfo->m_fDimensions[4], m_PieceInfo->m_fDimensions[5]),
	                            Vector3(m_PieceInfo->m_fDimensions[0], m_PieceInfo->m_fDimensions[1], m_PieceInfo->m_fDimensions[2]));

	lcObject* Last = NULL;
	for (int i = 0; i < 4; i++)
	{
		lcFlexiblePiecePoint* Point = new lcFlexiblePiecePoint(this);
		Point->SetPosition(1, true, Vector3(0.0f, 0.0f, 1.0f * i));

		if (i == 1 || i == 2)
			Point->SetFlag(LC_FLEXPIECE_POINT_TANGENT);

		if (Last)
			Last->m_Next = Point;
		else
			m_Children = Point;
		Last = Point;
	}
}

lcFlexiblePiece::lcFlexiblePiece(const lcFlexiblePiece* Piece)
	: lcPieceObject(Piece)
{
	// lcFlexiblePiece members.
	m_PieceInfo = Piece->m_PieceInfo;
	if (m_PieceInfo)
		m_PieceInfo->AddRef();

	// FIXME: copy correctly
	lcObject* Last = NULL;
	for (lcObject* Point = Piece->m_Children; Point; Point = Point->m_Next)
	{
		lcFlexiblePiecePoint* NewPoint = new lcFlexiblePiecePoint(this);

		for (int i = 0; i < LC_FLEXPIECE_POINT_NUMKEYS; i++)
			NewPoint->m_Keys[i] = Point->m_Keys[i];

		NewPoint->m_WorldPosition = Point->m_WorldPosition;
		NewPoint->m_ParentPosition = Point->m_ParentPosition;

		NewPoint->m_Flags = Point->m_Flags;

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

void lcFlexiblePiece::GetPieceList(ObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const
{
	LC_PIECELIST_ENTRY Entry;

	Entry.Info = m_PieceInfo;
	Entry.Color = (m_Color == LC_COLOR_DEFAULT) ? Color : m_Color;

	Pieces.Add(Entry);
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
	m_Mesh->AddToScene(Scene, m_ModelWorld, (m_Color == LC_COLOR_DEFAULT) ? Color : m_Color, this);

	bool AddChildren = IsSelected();

	if (!AddChildren)
	{
		for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
		{
			if (Point->IsSelected())
			{
				AddChildren = true;
				break;
			}
		}
	}

	if (AddChildren)
		for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
			Point->AddToScene(Scene, Color);
}

void lcFlexiblePiece::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	bool HitPoint = false;

	bool CheckChildren = IsSelected();

	if (!CheckChildren)
	{
		for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
		{
			if (Point->IsSelected())
			{
				CheckChildren = true;
				break;
			}
		}
	}

	if (CheckChildren)
	{
		for (lcObject* Point = m_Children; Point; Point = Point->m_Next)
		{
			Point->ClosestRayIntersect(Ray);

			if (Ray->Object == Point)
				HitPoint = true;
		}
	}

	if (!HitPoint)
	{
		Matrix44 WorldModel = RotTranInverse(m_ModelWorld);
		Vector3 Start = Mul31(Ray->Start, WorldModel);
		Vector3 End = Mul31(Ray->End, WorldModel);

		// Check the bounding box distance first.
		float Dist;
		if (!BoundingBoxRayMinIntersectDistance(m_BoundingBox, Start, End, &Dist) || (Dist >= Ray->Dist))
			return;

		// Check mesh.
		if (m_Mesh->ClosestRayIntersect(Start, End, &Dist) || (Dist >= Ray->Dist))
		{
			Ray->Object = this;
			Ray->Dist = Dist;
		}
	}
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

	int NumSections = 3;
	int NumSegments = 16;
	int NumSlices = 12;
	int NumIndices = 6 * (NumSlices) * (NumSegments-1) * NumSections;
	int NumVertices = NumSlices * NumSegments * NumSections;
	float r = 0.2f;

	m_Mesh = new lcMesh(1, NumIndices, NumVertices, NULL);

	lcMeshEditor<u16> MeshEdit(m_Mesh);
	lcMeshSection* Section = MeshEdit.StartSection(GL_TRIANGLES, LC_COLOR_DEFAULT);
	Section->Box.Reset();

	// Calculate the initial rotation matrix for the curve points.
	Vector3 Z = Normalize(m_Children->m_Next->m_ParentPosition - m_Children->m_ParentPosition);

	// Build the Y vector of the matrix.
	Vector3 UpVector;

	if (fabsf(Z[0]) < 0.001f && fabsf(Z[1]) < 0.001f)
		UpVector = Vector3(-Z[2], 0, 0);
	else
		UpVector = Vector3(0, 0, 1);

	// Calculate X vector.
	Vector3 X = Cross3(UpVector, Z);

	// Calculate real Y vector.
	Vector3 Y = Cross3(Z, X);

	Matrix33 LastMat = Matrix33(Normalize(X), Normalize(Y), Normalize(Z));

	lcFlexiblePiecePoint* PrevPoint = (lcFlexiblePiecePoint*)m_Children;
	lcFlexiblePiecePoint* Point = (lcFlexiblePiecePoint*)m_Children;
	lcFlexiblePiecePoint* NextPoint = (lcFlexiblePiecePoint*)m_Children->m_Next;
	int Segment = 0;

	while (NextPoint)
	{
		Matrix33 StartMat, EndMat;

		Vector3 p0 = PrevPoint->m_ParentPosition;
		Vector3 p1 = Point->m_ParentPosition;
		Vector3 p2 = NextPoint->m_ParentPosition;
		Vector3 p3 = NextPoint->m_Next ? NextPoint->m_Next->m_ParentPosition : NextPoint->m_ParentPosition;

		// Calculate the start and end orientation for this curve segment.
		if (Point->IsFlagged(LC_FLEXPIECE_POINT_TANGENT))
			StartMat = LastMat;
		else
			StartMat = Point->m_ModelParent;

		if (NextPoint->IsFlagged(LC_FLEXPIECE_POINT_TANGENT))
		{
			// Make the tangent at this point be parallel to a vector between the point before and after it.
			Vector3 Tan = p3 - p1;

			Tan.Normalize();
			float Dot = Dot3(StartMat[2], Tan);

			if (Dot < 0.999f)
			{
				Vector3 Axis = Cross3(StartMat[2], Tan);
				float Angle = acosf(Dot);

				Matrix33 Rot = MatrixFromAxisAngle(Axis, Angle);
				EndMat = Mul(StartMat, Rot);
			}
			else
				EndMat = StartMat;
		}
		else
			EndMat = NextPoint->m_ModelParent;

		float Dot = Dot3(StartMat[2], EndMat[2]);
		Vector3 Axis;
		float Angle;

		if (Dot < 0.999f)
		{
			Axis = Cross3(StartMat[2], EndMat[2]);
			Angle = acosf(Dot);
		}
		else
		{
			Axis = Vector3(0, 0, 1);
			Angle = 0;
		}

		// Calculate the coefficients of a bezier curve that passes through the 2 end points
		// for this segment and has the tangent vectors calculated above.
		p0 = p1;
		p3 = p2;

		float Len = Length(p1-p2) * 0.5f;
		p1 = p1 + StartMat[2] * Len;
		p2 = p2 - EndMat[2] * Len;

		Matrix44 Points = Matrix44(Vector4(p0, 1), Vector4(p1, 1), Vector4(p2, 1), Vector4(p3, 1));

		for (int j = 0; j < NumSegments; j++)
		{
			static Matrix44 Blend(Vector4(-1, 3, -3, 1), Vector4(3, -6, 3, 0), Vector4(-3, 3, 0, 0), Vector4(1, 0, 0, 0));  
			static Matrix44 Tangent(Vector4(0, 0, 0, 0), Vector4(-3, 9, -9, 3), Vector4(6,-12, 6, 0), Vector4(-3, 3, 0, 0));  

			float t = (float)j/(NumSegments-1);

			Vector3 Pos = Vector3(Mul4(Mul4(Vector4(t*t*t,t*t,t,1), Blend), Points));  
			Vector3 Tan = Vector3(Mul4(Mul4(Vector4(0,t*t,t,1), Tangent), Points));  

			Tan.Normalize();
			float Dot = Dot3(LastMat[2], Tan);

			if (Dot < 0.999f)
			{
				Vector3 Axis = Cross3(LastMat[2], Tan);
				float Angle = acosf(Dot);

				Matrix33 Rot = MatrixFromAxisAngle(Axis, Angle);
				LastMat = Mul(LastMat, Rot);
			}

			int IndexOffset = NumSlices * NumSegments * Segment;

			for (int k = 0; k < NumSlices; k++)
			{
				Vector3 Vertex(r * sinf(k * LC_2PI / NumSlices), r * cosf(k * LC_2PI / NumSlices), 0);
				Vertex = Mul(Vertex, LastMat) + Pos;
				MeshEdit.AddVertex(Vertex);
				Section->Box.AddPoint(Vertex);

				if (j != NumSegments-1)
				{
					if (k != NumSlices-1)
					{
						MeshEdit.AddIndex(IndexOffset +    j  * NumSlices + k);
						MeshEdit.AddIndex(IndexOffset +    j  * NumSlices + k + 1);
						MeshEdit.AddIndex(IndexOffset + (j+1) * NumSlices + k);
						MeshEdit.AddIndex(IndexOffset +    j  * NumSlices + k + 1);
						MeshEdit.AddIndex(IndexOffset + (j+1) * NumSlices + k + 1);
						MeshEdit.AddIndex(IndexOffset + (j+1) * NumSlices + k);
					}
					else
					{
						MeshEdit.AddIndex(IndexOffset +    j  * NumSlices + k);
						MeshEdit.AddIndex(IndexOffset +    j  * NumSlices);
						MeshEdit.AddIndex(IndexOffset + (j+1) * NumSlices + k);
						MeshEdit.AddIndex(IndexOffset +    j  * NumSlices);
						MeshEdit.AddIndex(IndexOffset + (j+1) * NumSlices);
						MeshEdit.AddIndex(IndexOffset + (j+1) * NumSlices + k);
					}
				}
			}
		}

		LastMat = EndMat;

		PrevPoint = Point;
		Point = NextPoint;
		NextPoint = (lcFlexiblePiecePoint*)Point->m_Next;

		Segment++;
	}

	MeshEdit.EndSection();

	m_BoundingBox = Section->Box;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME: split this into another file

#define LC_FLEXPIECE_POINT_SIZE 0.3f

lcFlexiblePiecePoint::lcFlexiblePiecePoint(lcFlexiblePiece* Parent)
	: lcObject(LC_OBJECT_FLEXPIECE_POINT, LC_FLEXPIECE_POINT_NUMKEYS)
{
	m_Parent = Parent;

	ChangeKey(0, false, LC_PIECEOBJ_ROTATION, Vector4(0, 0, 1, 0));
}

lcFlexiblePiecePoint::~lcFlexiblePiecePoint()
{
}

void lcFlexiblePiecePoint::Update(u32 Time)
{
	Vector4 AxisAngle;

	CalculateKey(Time, LC_FLEXPIECE_POINT_POSITION, &m_ParentPosition);
	CalculateKey(Time, LC_PIECEOBJ_ROTATION, &AxisAngle);

	m_ModelParent = MatrixFromAxisAngle(AxisAngle);
	m_ModelParent.SetTranslation(m_ParentPosition);

	m_ModelWorld = Mul(m_ModelParent, ((lcPieceObject*)m_Parent)->m_ModelWorld);
	m_WorldPosition = Vector3(m_ModelWorld[3]);
}

void lcFlexiblePiecePoint::AddToScene(lcScene* Scene, int Color)
{
	// FIXME: lcFlexiblePiece

	Matrix44 ScaleMatrix(Vector4(LC_FLEXPIECE_POINT_SIZE, 0.0f, 0.0f, 0.0f), Vector4(0.0f, LC_FLEXPIECE_POINT_SIZE, 0.0f, 0.0f), Vector4(0.0f, 0.0f, LC_FLEXPIECE_POINT_SIZE, 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f));

	lcRenderSection RenderSection;

	RenderSection.Owner = (lcPieceObject*)this;
	RenderSection.ModelWorld = Mul(ScaleMatrix, m_ModelWorld);
	RenderSection.Mesh = lcSphereMesh;
	RenderSection.Section = &lcSphereMesh->m_Sections[0];
	RenderSection.Color = 1;

	if (IsFocused())
		RenderSection.Color = LC_COLOR_FOCUS;
	else if (IsSelected())
		RenderSection.Color = LC_COLOR_SELECTION;

	Scene->m_OpaqueSections.Add(RenderSection);
}

void lcFlexiblePiecePoint::ClosestRayIntersect(LC_CLICK_RAY* Ray) const
{
	float Dist = LinePointMinDistance(m_WorldPosition, Ray->Start, Ray->End);

	if (Dist < Ray->Dist && Dist < LC_FLEXPIECE_POINT_SIZE)
	{
		Ray->Object = this;
		Ray->Dist = Dist;
	}
}

bool lcFlexiblePiecePoint::IntersectsVolume(const Vector4* Planes, int NumPlanes) const
{
	// FIXME: lcFlexiblePiece
	return false;
}
