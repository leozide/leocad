#include "lc_global.h"
#include "lc_model.h"

#include "object.h"
#include "piece.h"
#include "camera.h"
#include "light.h"
#include "group.h"
#include "lc_colors.h"
#include "lc_mesh.h"
#include "pieceinf.h"
#include "project.h"
#include "lc_application.h"
#include "library.h"
#include "system.h"
#include "console.h"
#include "view.h"

lcModel::lcModel()
{
//	m_Author = Sys_ProfileLoadString("Default", "User", "");

	m_Pieces = NULL;
	m_Cameras = NULL;
	m_Lights = NULL;
	m_Groups = NULL;

	m_CurFrame = 1;
	m_TotalFrames = 100;

	m_Mesh = NULL;
	m_PieceInfo = new PieceInfo();

	int Max = 0;

	for (int ModelIndex = 0; ModelIndex < lcGetActiveProject()->m_ModelList.GetSize(); ModelIndex++)
	{
		PieceInfo* Info = lcGetActiveProject()->m_ModelList[ModelIndex]->m_PieceInfo;
		int i;

		if (sscanf(Info->m_strName + 5, "%d", &i) == 1)
			Max = lcMax(Max, i);
	}

	sprintf(m_PieceInfo->m_strName, "MODEL%.3d", Max+1);

	m_PieceInfo->m_nFlags = LC_PIECE_MODEL;
	m_PieceInfo->m_Model = this;
}

lcModel::~lcModel()
{
	DeleteContents();
	delete m_PieceInfo;
}

void lcModel::SetName(const char* Name)
{
	m_Name = Name;
	strncpy(m_PieceInfo->m_strDescription, Name, sizeof(m_PieceInfo->m_strDescription));
	m_PieceInfo->m_strDescription[sizeof(m_PieceInfo->m_strDescription)-1] = 0;
}

void lcModel::DeleteContents()
{
	while (m_Pieces)
	{
		lcObject* Piece = m_Pieces;
		m_Pieces = (lcPiece*)m_Pieces->m_Next;
		delete Piece;
	}

	while (m_Cameras)
	{
		lcObject* Camera = m_Cameras;
		m_Cameras = (lcCamera*)m_Cameras->m_Next;
		delete Camera;
	}

	while (m_Lights)
	{
		lcObject* Light = m_Lights;
		m_Lights = (lcLight*)m_Lights->m_Next;
		delete Light;
	}

	while (m_Groups)
	{
		lcGroup* Group = m_Groups;
		m_Groups = m_Groups->m_Next;
		delete Group;
	}

	delete m_Mesh;
	m_Mesh = NULL;
	m_PieceInfo->m_Mesh = NULL;
}

bool lcModel::FileLoad(lcFile& file)
{
	return true;
}

void lcModel::FileSave(lcFile& file)
{
}

void lcModel::ImportLDraw(lcFile& file, int DefaultColor, const Matrix44& ParentWorld, const String& FilePath)
{
	char Buf[1024];

	while (file.ReadLine(Buf, 1024))
	{
		char* ptr = Buf;
		String LineType = GetToken(ptr);
		bool PieceHidden = false;

		if (LineType == "0")
		{
			String Token = GetToken(ptr);

			if (!Token.CompareNoCase("STEP"))
			{
				m_CurFrame++;
				continue;
			}

			if (Token == "!MLCAD")
			{
				Token = GetToken(ptr);

				if (Token == "HIDE")
				{
					PieceHidden = true;
					LineType = GetToken(ptr);
				}
			}
		}

		if (LineType == "1")
		{
			char tmp[LC_MAXPATH], pn[LC_MAXPATH];
			int Color;
			float Mat[12];

			if (sscanf(ptr, "%d %g %g %g %g %g %g %g %g %g %g %g %g %s[256]",
					   &Color, &Mat[0], &Mat[1], &Mat[2], &Mat[3], &Mat[4], &Mat[5], &Mat[6], 
					   &Mat[7], &Mat[8], &Mat[9], &Mat[10], &Mat[11], &tmp[0]) != 14)
				continue;

			Matrix44 ModelWorld = IdentityMatrix44();
			float* ModelMatrix = ModelWorld;

			ModelMatrix[12] =  Mat[0] / 25.0f;
			ModelMatrix[14] = -Mat[1] / 25.0f;
			ModelMatrix[13] =  Mat[2] / 25.0f;
			ModelMatrix[0] =  Mat[3];
			ModelMatrix[8] = -Mat[4];
			ModelMatrix[4] =  Mat[5];
			ModelMatrix[2] = -Mat[6];
			ModelMatrix[10] = Mat[7];
			ModelMatrix[6] = -Mat[8];
			ModelMatrix[1] =  Mat[9];
			ModelMatrix[9] = -Mat[10];
			ModelMatrix[5] =  Mat[11];

			ModelWorld = Mul(ModelWorld, ParentWorld);

			if (Color == 16) 
				Color = DefaultColor;
			else
				Color = lcConvertLDrawColor(Color);

			strcpy(pn, tmp);
			ptr = strrchr(tmp, '.');

			if (ptr != NULL)
				*ptr = 0;

			PieceInfo* Info = NULL;

			// See if it's a piece in the library.
			if (strlen(tmp) < 9)
			{
				char name[9];
				strcpy(name, tmp);
				_strupr(name);

				Info = lcGetPiecesLibrary()->FindPieceInfo(name);
			}

			// Check for one of the other models.
			if (!Info)
			{
				for (int ModelIndex = 0; ModelIndex < lcGetActiveProject()->m_ModelList.GetSize(); ModelIndex++)
				{
					lcModel* Model = lcGetActiveProject()->m_ModelList[ModelIndex];

					if (Model->m_Name == (const char*)pn)
					{
						Info = Model->m_PieceInfo;
						break;
					}
				}
			}

			if (Info)
			{
				lcPiece* Piece = new lcPiece(Info);

				Vector3 Pos = ModelWorld.GetTranslation();
				Vector4 Rot = MatrixToAxisAngle(ModelWorld);
				Piece->Initialize(Pos[0], Pos[1], Pos[2], m_CurFrame, Color);
				Piece->SetUniqueName(m_Pieces, Info->m_strDescription);
				AddPiece(Piece);
				Piece->ChangeKey(1, false, Rot, LC_PK_ROTATION);
				SystemPieceComboAdd(Info->m_strDescription);

				if (PieceHidden)
					Piece->Hide();

				continue;
			}

			// Try to read the file from disk.
			lcFileDisk tf;

			if (tf.Open(pn, "rt"))
			{
				// Read from the current directory.
				ImportLDraw(tf, Color, ModelWorld, FilePath);
			}
			else
			{
				// Try the file's directory.
				String Path = FilePath + pn;

				if (tf.Open(Path, "rt"))
				{
					// Read from the current directory.
					ImportLDraw(tf, Color, ModelWorld, FilePath);
				}
				else
				{
					console.PrintWarning("Could not find %s.\n", pn);
				}
			}
		}
	}
}

void lcModel::ExportLDraw(lcFile& file, bool ExportMPD, const Matrix44& ParentWorld, u32 Color)
{
	char buf[1024];

	String Name = m_Name;

	for (;;)
	{
		int Bad = Name.FindOneOf(" #%;/\\");
		if (Bad == -1)
			break;
		Name[Bad] = '_';
	}

	if (ExportMPD)
	{
		sprintf(buf, "0 FILE %s\r\n", (const char*)Name);
		file.Write(buf, strlen(buf));
	}

	if (!m_Description.IsEmpty())
		sprintf(buf, "0 %s\r\n", (const char*)m_Description);
	else
		sprintf(buf, "0 %s\r\n", (const char*)Name);
	file.Write(buf, strlen(buf));

	if (m_Author.IsEmpty())
		strcpy(buf, "0 Author: LeoCAD\r\n");
	else
		sprintf(buf, "0 Author: %s\r\n", (const char*)m_Author);
	file.Write(buf, strlen(buf));

	strcpy(buf, "\r\n");
	file.Write(buf, strlen(buf));

	u32 LastStep = GetLastStep();

	for (u32 Step = 1; Step <= LastStep; Step++)
	{
		for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		{
			if (Piece->m_TimeShow != Step)
				continue;

			u32 PieceColor = (Piece->m_Color == LC_COLOR_DEFAULT) ? Color : Piece->m_Color;
			const char* Name;
			const char* Ext = "";

			if (Piece->m_PieceInfo->m_nFlags & LC_PIECE_MODEL)
			{
				if (ExportMPD)
					Name = Piece->m_PieceInfo->m_Model->m_Name;
				else
				{
					Matrix44 ModelWorld = Mul(Piece->m_ModelWorld, ParentWorld);
					Piece->m_PieceInfo->m_Model->ExportLDraw(file, ExportMPD, ModelWorld, PieceColor);
					continue;
				}
			}
			else
			{
				Name = Piece->m_PieceInfo->m_strName;
				Ext = ".DAT";
			}

			if (Piece->IsHidden())
			{
				const char* HiddenText = " 0 !MLCAD HIDE";
				file.Write(HiddenText, strlen(HiddenText));
			}

			// Convert matrix to LDraw coordinates.
			Matrix44 ModelWorld = Mul(Piece->m_ModelWorld, ParentWorld);
			float* ModelMatrix = ModelWorld;
			float Mat[12];

			Mat[0] =  ModelMatrix[12] * 25.0f;
			Mat[1] = -ModelMatrix[14] * 25.0f;
			Mat[2] =  ModelMatrix[13] * 25.0f;
			Mat[3] =  ModelMatrix[0];
			Mat[4] = -ModelMatrix[8];
			Mat[5] =  ModelMatrix[4];
			Mat[6] = -ModelMatrix[2];
			Mat[7] =  ModelMatrix[10];
			Mat[8] = -ModelMatrix[6];
			Mat[9] =  ModelMatrix[1];
			Mat[10] = -ModelMatrix[9];
			Mat[11] =  ModelMatrix[5];

			sprintf(buf, " 1 %d %g %g %g %g %g %g %g %g %g %g %g %g %s%s\r\n",
			        g_ColorList[PieceColor].Code, Mat[0], Mat[1], Mat[2], Mat[3], Mat[4], Mat[5],
			        Mat[6], Mat[7], Mat[8], Mat[9], Mat[10], Mat[11], Name, Ext);
			file.Write(buf, strlen(buf));
		}

		if (Step != LastStep)
			file.Write("0 STEP\r\n", 8);
	}

	file.Write("0\r\n", 3);
}

bool lcModel::IsSubModel(const lcModel* Model) const
{
	for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		PieceInfo* Info = Piece->m_PieceInfo;

		if (!(Info->m_nFlags & LC_PIECE_MODEL))
			continue;

		if (Info->m_Model == Model || Info->m_Model->IsSubModel(Model))
			return true;
	}

	return false;
}

u32 lcModel::GetLastStep() const
{
	u32 Last = 1;

	for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		Last = lcMax(Last, Piece->m_TimeShow);

	return Last;
}

/*
void lcModel::GetPieceList(lcObjArray<struct LC_PIECELIST_ENTRY>& Pieces, int Color) const
{
	for (lcPieceObject* Piece = m_Pieces; Piece; Piece = (lcPieceObject*)Piece->m_Next)
		Piece->GetPieceList(Pieces, Color);
}
*/
void lcModel::SetActive(bool Active)
{
	if (Active)
	{
		for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		{
			if (!Piece->IsVisible(m_CurFrame))
				continue;

			Piece->UpdatePosition(m_CurFrame);
		}
		return;
	}
	else
	{
		UpdateMesh();
	}
}

void lcModel::UpdateMesh()
{
	u32 Time = lcMax(m_TotalFrames, LC_OBJECT_TIME_MAX);

	u32 VertexCount = 0;
	u32* SectionIndices = new u32[lcNumColors*2];
	memset(SectionIndices, 0, sizeof(u32)*lcNumColors*2);
	m_BoundingBox.Reset();

	// Update bounding box and count the number of vertices and indices needed.
	for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		if (!Piece->IsVisible(Time))
			continue;

		Piece->UpdatePosition(Time);
		Piece->MergeBoundingBox(&m_BoundingBox);

		lcMesh* Mesh = Piece->m_PieceInfo->m_Mesh;
		VertexCount += Mesh->m_VertexCount;

		for (int SectionIndex = 0; SectionIndex < Mesh->m_SectionCount; SectionIndex++)
		{
			lcMeshSection* Section = &Mesh->m_Sections[SectionIndex];
			int Color = (Section->ColorIndex == LC_COLOR_DEFAULT) ? Piece->m_Color : Section->ColorIndex;
			int Index = (Section->PrimitiveType == GL_TRIANGLES) ? Color * 2 : Color * 2 + 1;
			SectionIndices[Index] += Section->IndexCount;
		}
	}

	u32 SectionCount = 0;
	u32 IndexCount = 0;

	for (int i = 0; i < lcNumColors*2; i++)
	{
		if (SectionIndices[i])
		{
			SectionCount++;
			IndexCount += SectionIndices[i];
		}
	}

	delete m_Mesh;
	m_Mesh = new lcMesh(SectionCount, IndexCount, VertexCount, NULL);

	if (m_Mesh->m_IndexType == GL_UNSIGNED_INT)
		BuildMesh<u32>(SectionIndices);
	else
		BuildMesh<u16>(SectionIndices);

	delete[] SectionIndices;

	if (!m_Pieces)
	{
		m_BoundingBox = BoundingBox(Vector3(0, 0, 0), Vector3(0, 0, 0));
	}

	m_PieceInfo->CreateFromModel(this);
}

template<typename T>
void lcModel::BuildMesh(u32* SectionIndices)
{
	// Copy data from all meshes into the new mesh.
	lcMeshEditor<T> MeshEdit(m_Mesh);

	lcMeshSection** DstSections = new lcMeshSection*[lcNumColors*2];
	memset(DstSections, 0, sizeof(DstSections[0])*lcNumColors*2);

	for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		lcMesh* SrcMesh = Piece->m_PieceInfo->m_Mesh;

		void* SrcIndexBufer = SrcMesh->m_IndexBuffer->MapBuffer(GL_READ_ONLY_ARB);

		for (int i = 0; i < SrcMesh->m_SectionCount; i++)
		{
			lcMeshSection* SrcSection = &SrcMesh->m_Sections[i];

			int Color = (SrcSection->ColorIndex == LC_COLOR_DEFAULT) ? Piece->m_Color : SrcSection->ColorIndex;
			int SrcColor = (SrcSection->PrimitiveType == GL_TRIANGLES) ? Color * 2 : Color * 2 + 1;

			// Create a new section if needed.
			SectionIndices[SrcColor] -= SrcSection->IndexCount;
			int ReserveIndices = SectionIndices[SrcColor];
			if (DstSections[SrcColor])
			{
				MeshEdit.SetCurrentSection(DstSections[SrcColor]);
				DstSections[SrcColor]->Box += SrcSection->Box;
			}
			else
			{
				DstSections[SrcColor] = MeshEdit.StartSection(SrcSection->PrimitiveType, Color);
				DstSections[SrcColor]->Box = SrcSection->Box;
			}

			// Copy indices.
			if (SrcMesh->m_IndexType == GL_UNSIGNED_INT)
				MeshEdit.AddIndices32((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);
			else
				MeshEdit.AddIndices16((char*)SrcIndexBufer + SrcSection->IndexOffset, SrcSection->IndexCount);

			// Fix the indices to point to the right place after the vertex buffers are merged.
			MeshEdit.OffsetIndices(MeshEdit.m_CurIndex - SrcSection->IndexCount, SrcSection->IndexCount, MeshEdit.m_CurVertex);

			MeshEdit.EndSection(ReserveIndices);
		}

		SrcMesh->m_IndexBuffer->UnmapBuffer();

		// Transform and copy vertices.
		float* SrcVertexBuffer = (float*)SrcMesh->m_VertexBuffer->MapBuffer(GL_READ_ONLY_ARB);

		for (int i = 0; i < SrcMesh->m_VertexCount; i++)
		{
			float* SrcPtr = SrcVertexBuffer + 3 * i;
			Vector3 Vert(SrcPtr[0], SrcPtr[1], SrcPtr[2]);
			Vert = Mul31(Vert, Piece->m_ModelWorld);
			MeshEdit.AddVertex(Vert);
		}

		SrcMesh->m_VertexBuffer->UnmapBuffer();
	}

	delete[] DstSections;
}


/*
void lcModel::Update(u32 Time)
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		Piece->Update(Time);

	for (lcObject* Camera = m_Cameras; Camera; Camera = Camera->m_Next)
		Camera->Update(Time);

	for (lcObject* Light = m_Lights; Light; Light = Light->m_Next)
		Light->Update(Time);
}
*/
bool lcModel::AnyObjectsSelected() const
{
	for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		if (Piece->IsSelected())
			return true;

	for (lcCamera* Camera = m_Cameras; Camera; Camera = (lcCamera*)Camera->m_Next)
		if (Camera->IsEyeSelected() || Camera->IsTargetSelected())
			return true;

	for (lcLight* Light = m_Lights; Light; Light = (lcLight*)Light->m_Next)
		if (Light->IsEyeSelected() || Light->IsTargetSelected())
			return true;

	return false;
}

void lcModel::AddPiece(lcPiece* NewPiece)
{
	lcObject* Prev = NULL;
	lcObject* Next = m_Pieces;

	while (Next)
	{
		// TODO: sort pieces by vertex buffer.
//		if (Next->GetPieceInfo() > NewPiece->GetPieceInfo())
			break;

		Prev = Next;
		Next = Next->m_Next;
	}

	NewPiece->m_Next = Next;

	if (Prev)
		Prev->m_Next = NewPiece;
	else
		m_Pieces = NewPiece;
}

void lcModel::RemovePiece(lcPiece* Piece)
{
	lcObject* Next = m_Pieces;
	lcObject* Prev = NULL;

	while (Next)
	{
		if (Next == Piece)
		{
			if (Prev != NULL)
				Prev->m_Next = Piece->m_Next;
			else
				m_Pieces = (lcPiece*)Piece->m_Next;

			break;
		}

		Prev = Next;
		Next = Next->m_Next;
	}
}

void lcModel::InlineModel(lcModel* Model, const Matrix44& ModelWorld, u32 Color, u32 TimeShow, u32 TimeHide)
{
	u32 Time = lcMax(Model->m_TotalFrames, LC_OBJECT_TIME_MAX);

	for (lcPiece* SrcPiece = Model->m_Pieces; SrcPiece; SrcPiece = (lcPiece*)SrcPiece->m_Next)
	{
		lcPiece* Piece = new lcPiece(SrcPiece->m_PieceInfo);

		SrcPiece->UpdatePosition(Time);

		Matrix44 Mat = Mul(SrcPiece->m_ModelWorld, ModelWorld);

		Vector3 Pos = Mat.GetTranslation();
		Vector4 Rot = MatrixToAxisAngle(Mat);

		Piece->Initialize(Pos[0], Pos[1], Pos[2], TimeShow, (SrcPiece->m_Color == LC_COLOR_DEFAULT) ? Color : SrcPiece->m_Color);
		Piece->ChangeKey(TimeShow, false, Rot, LC_PK_ROTATION);
		Piece->UpdatePosition(m_CurFrame);
		Piece->m_TimeHide = TimeHide;

//			SelectAndFocusNone(false);
		Piece->SetUniqueName(m_Pieces, Piece->m_PieceInfo->m_strDescription);
		AddPiece(Piece);
//			Piece->Select(true, true, false);
//			lcPostMessage(LC_MSG_FOCUS_OBJECT_CHANGED, Piece);
//			UpdateSelection();
		SystemPieceComboAdd(Piece->m_PieceInfo->m_strDescription);
	}
}

bool lcModel::AnyPiecesSelected() const
{
	for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
		if ((Piece->IsVisible(m_CurFrame)) && Piece->IsSelected())
			return true;

	return false;
}
/*
void lcModel::SelectAllPieces(bool Select)
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (Piece->IsVisible(m_CurFrame))
			Piece->SetSelection(Select, true);
}

void lcModel::SelectInvertAllPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (Piece->IsVisible(m_CurFrame))
			Piece->SetSelection(!Piece->IsSelected(), true);
}

void lcModel::HideSelectedPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (Piece->IsSelected())
			Piece->SetVisible(false);
}

void lcModel::HideUnselectedPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		if (!Piece->IsSelected())
			Piece->SetVisible(false);
}

void lcModel::UnhideAllPieces()
{
	for (lcObject* Piece = m_Pieces; Piece; Piece = Piece->m_Next)
		Piece->SetVisible(true);
}

bool lcModel::RemoveSelectedPieces()
{
	lcObject* Piece = m_Pieces;
	bool Deleted = false;

	while (Piece)
	{
		if (Piece->IsSelected())
		{
			lcObject* Temp = Piece->m_Next;

			Deleted = true;
			RemovePiece((lcPieceObject*)Piece);
			delete Piece;
			Piece = Temp;
		}
		else
			Piece = Piece->m_Next;
	}

	return Deleted;
}
*/
void lcModel::AddCamera(lcCamera* Camera)
{
	lcObject* LastCamera = m_Cameras;

	while (LastCamera && LastCamera->m_Next)
		LastCamera = LastCamera->m_Next;

	if (LastCamera)
		LastCamera->m_Next = Camera;
	else
		m_Cameras = Camera;

	Camera->m_Next = NULL;
}

void lcModel::ResetCameras()
{
	// Delete all cameras.
	while (m_Cameras)
	{
		lcObject* Camera = m_Cameras;
		m_Cameras = (lcCamera*)m_Cameras->m_Next;
		delete Camera;
	}

	// Create new default cameras.
	for (int i = 0; i < 7; i++)
	{
		lcCamera* Camera = new lcCamera(i);

		AddCamera(Camera);
	}

/*
	lcObject* Last = NULL;
	for (int i = 0; i < LC_CAMERA_USER; i++)
	{
		lcCamera* Camera = new lcCamera();
		Camera->CreateCamera(i, true);
		Camera->Update(1);

		if (Last == NULL)
			m_Cameras = Camera;
		else
			Last->m_Next = Camera;

		Last = Camera;
	}
*/
}

lcCamera* lcModel::GetCamera(int Index) const
{
	lcObject* Camera = m_Cameras;

	while (Camera && Index--)
		Camera = Camera->m_Next;

	return (lcCamera*)Camera;
}

lcCamera* lcModel::GetCamera(const char* Name) const
{
	for (lcObject* Camera = m_Cameras; Camera; Camera = Camera->m_Next)
		if (Camera->m_Name == Name)
			return (lcCamera*)Camera;

	return NULL;
}

void lcModel::ZoomExtents(View* view, lcCamera* Camera, bool AddKeys)
{
	if (!m_Pieces)
		return;

	// Calculate a bounding box that includes all pieces and use its center as the camera target.
	lcObjArray<Vector3> Points;
	BoundingBox Box;
	Box.Reset();

	for (lcPiece* Piece = m_Pieces; Piece; Piece = (lcPiece*)Piece->m_Next)
	{
		if (!Piece->IsVisible(m_CurFrame))
			continue;

		Vector3 Corners[8];
		Piece->m_PieceInfo->m_BoundingBox.GetPoints(Corners);

		for (int i = 0; i < 8; i++)
		{
			Vector3 Point = Mul31(Corners[i], Piece->m_ModelWorld);
			Points.Add(Point);
			Box.AddPoint(Point);
		}
	}

	Vector3 Center = Box.GetCenter();

	// Update eye and target positions.
	Vector3 Eye = Camera->m_Position;
	Vector3 Target = Camera->m_TargetPosition;

	if (Camera->IsSide())
		Eye += Center - Target;

	Target = Center;

	Camera->ChangeKey(m_CurFrame, AddKeys, Eye, LC_CK_EYE);
	Camera->ChangeKey(m_CurFrame, AddKeys, Target, LC_CK_TARGET);
	Camera->UpdatePosition(m_CurFrame);

	float Aspect = (float)view->GetWidth()/(float)view->GetHeight();
	Matrix44 Projection = CreatePerspectiveMatrix(Camera->m_FOV, Aspect, Camera->m_NearDist, Camera->m_FarDist);

	Eye = ::ZoomExtents(Eye, Camera->m_WorldView, Projection, &Points[0], Points.GetSize());

	// Save new positions.
	Camera->ChangeKey(m_CurFrame, AddKeys, Eye, LC_CK_EYE);
	Camera->ChangeKey(m_CurFrame, AddKeys, Target, LC_CK_TARGET);
	Camera->UpdatePosition(m_CurFrame);
}

/*
void lcModel::AddLight(lcLight* Light)
{
	if (!m_Lights)
		m_Lights = Light;
	else
	{
		lcObject* Prev = m_Lights;

		while (Prev->m_Next)
			Prev = Prev->m_Next;

		Prev->m_Next = Light;
	}
}
*/
