// GLWindow.cpp : implementation file
//

#include "stdafx.h"
#include "LeoCAD.h"
#include "MFWnd.h"
#include "Tools.h"
#include "project.h"
#include "globals.h"
#include "Matrix.h"
#include "pieceinf.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMinifigWnd

MFPARTINFO CMinifigWnd::partinfo[MFW_PIECES] = { { "3624", "Police Hat", MF_HAT }, { "3626BP01", "Smiley Face", MF_HEAD },
	{ "973", "Plain Torso", MF_TORSO }, { "3838", "Airtanks", MF_NECK }, { "976", "Left Arm", MF_ARML }, 
	{ "975", "Right Arm", MF_ARMR }, { "977", "Hand", MF_HAND }, { "977", "Hand", MF_HAND }, { "3899", "Cup", MF_TOOL },
	{ "4528", "Frypan", MF_TOOL }, { "970", "Hips", MF_HIPS }, { "972", "Left Leg", MF_LEGL }, 
	{ "971", "Right Leg", MF_LEGR }, { "2599", "Flipper", MF_SHOE }, { "6120", "Ski", MF_SHOE }, 
	{ "4485", "Baseball Cap", MF_HAT }, { "3626B", "Plain Face", MF_HEAD }, 
	{ "3626BP02", "Woman Face", MF_HEAD }, { "973P11", "Dungarees", MF_TORSO }, { "973P47", "Castle Red/Gray Symbol", MF_TORSO }, 
	{ "973P51", "Blacktron II", MF_TORSO }, { "973P01", "Vertical Strips Red/Blue", MF_TORSO }, { "973P02", "Vertical Strips Blue/Red", MF_TORSO }, 
	{ "973P60", "Shell Logo", MF_TORSO }, { "973P61", "Gold Ice Planet Pattern", MF_TORSO }, 
	{ "4349", "Loudhailer", MF_TOOL }, { "3962", "Radio", MF_TOOL }, { "4529", "Saucepan", MF_TOOL },
	{ "3959", "Space Gun", MF_TOOL }, { "4360", "Space Laser Gun", MF_TOOL }, { "4479", "Metal Detector", MF_TOOL },
	{ "6246A", "Screwdriver", MF_TOOL }, { "6246B", "Hammer", MF_TOOL }, { "6246D", "Box Wrench", MF_TOOL }, { "6246E", "Open End Wrench", MF_TOOL },
	{ "3896", "Castle Helmet with Chin-Guard", MF_HAT }, { "3844", "Castle Helmet with Neck Protect", MF_HAT }, { "3833", "Construction Helmet", MF_HAT }, 
	{ "82359", "Skeleton Skull", MF_HEAD }, { "973P14", "'S' Logo", MF_TORSO }, { "973P16", "Airplane Logo", MF_TORSO }, 
	{ "973P52", "Blacktron I Pattern", MF_TORSO }, { "973P15", "Horizontal Stripes", MF_TORSO },  { "973P68", "Mtron Logo", MF_TORSO }, 
	{ "973P17", "Red V-Neck and Buttons", MF_TORSO }, { "973P63", "Robot Pattern", MF_TORSO }, { "973P18", "Suit and Tie ", MF_TORSO }, 
	{ "4736", "Jet-Pack with Stud On Front", MF_NECK }, { "4522", "Mallet", MF_TOOL }, { "6246C", "Power Drill", MF_TOOL },
	{ "4006", "Spanner/Screwdriver", MF_TOOL }, { "194", "Hose Nozzle", MF_TOOL }, { "2446", "Helmet", MF_HAT }, 	{ "3840", "Vest", MF_NECK },
	{ "970P63", "Hips with Robot Pattern", MF_HIPS }, { "972P63", "Left Leg with Robot Pattern", MF_LEGL }, { "971P63", "Right Leg with Robot Pattern", MF_LEGR },
	{ "2524", "Backpack Non-Opening", MF_NECK }, { "4497", "Spear", MF_TOOL }, { "37", "Knife", MF_TOOL }, { "38", "Harpoon", MF_TOOL },
	{ "3626BP03", "Pointed Moustache", MF_HEAD }, { "3626BP04", "Sunglasses", MF_HEAD }, { "3626BP05", "Grin and Eyebrows", MF_HEAD }, 
	{ "973P19", "Train Chevron", MF_TORSO }, { "973P31", "Pirate Strips (Red/Cream)", MF_TORSO }, { "973P32", "Pirate Strips (Blue/Cream)", MF_TORSO },
	{ "973P33", "Pirate Strips (Red/Black)", MF_TORSO }, { "973P41", "Castle Chainmail", MF_TORSO }, { "973P62", "Silver Ice Planet", MF_TORSO },
	{ "6131", "Wizard Hat", MF_HAT }, { "973P20", "Waiter", MF_TORSO }, { "973P49", "Forestman Blue Collar", MF_TORSO },
	{ "973P48", "Forestman Maroon Collar", MF_TORSO }, { "973P50", "Forestman Black Collar", MF_TORSO }, { "3841", "Pickaxe", MF_TOOL }
};

//	{ "770", "Shield Ovoid", MF_TOOL }, 
//	2447.DAT      Minifig Helmet Visor


CMinifigWnd::CMinifigWnd()
{
	m_pDC = NULL;
}

CMinifigWnd::~CMinifigWnd()
{
}

BEGIN_MESSAGE_MAP(CMinifigWnd, CWnd)
	//{{AFX_MSG_MAP(CMinifigWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMinifigWnd message handlers

int CMinifigWnd::InitGL() 
{
	m_pDC = new CClientDC(this);
	ASSERT(m_pDC != NULL);

	// Fill in the Pixel Format Descriptor
    PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd,0, sizeof(PIXELFORMATDESCRIPTOR));

    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);  
    pfd.nVersion = 1;
	pfd.dwFlags  =  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int nPixelFormat = ChoosePixelFormat(m_pDC->m_hDC, &pfd);
	if (nPixelFormat == 0)
		return -1 ;

	if (!SetPixelFormat(m_pDC->m_hDC, nPixelFormat, &pfd))
		return -1 ;

	m_pPal = new CPalette;

	if (CreateRGBPalette(m_pDC->m_hDC, &m_pPal))
	{
		m_pDC->SelectPalette(m_pPal, FALSE);
		m_pDC->RealizePalette();
	}
	else
	{
		delete m_pPal;
		m_pPal = NULL;
	}

    // Create a rendering context.
	m_hrc = wglCreateContext(m_pDC->m_hDC);
	if (!m_hrc)
		return -1;

	HDC oldDC = wglGetCurrentDC();
	HGLRC oldRC = wglGetCurrentContext();
	wglShareLists(oldRC, m_hrc);
	wglMakeCurrent (m_pDC->m_hDC, m_hrc);

	CRect rc;
	GetClientRect (&rc);

	double aspect = (float)rc.right/(float)rc.bottom;
	glViewport(0, 0, rc.right, rc.bottom);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, aspect, 1.0f, 50.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	gluLookAt (0, -9, 4, 0, 5, 1, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float *bg = project->GetBackgroundColor();
	glClearColor(bg[0], bg[1], bg[2], bg[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DITHER);
	glShadeModel (GL_FLAT);

	BYTE colors[15] = { 0, 6, 4, 22, 0, 0, 6, 6, 22, 22, 9, 9, 9, 22, 22 };
	float pos[15][3] = { {0,0,3.84f},{0,0,3.84f},{0,0,2.88f},{0,0,2.96f},{0,0,2.56f},{0,0,2.56f},{0.9f,-0.62f,1.76f},
		{-0.9f,-0.62f,1.76f},{0.92f,-0.62f,1.76f},{-0.92f,-0.62f,1.76f},{0,0,1.6f},{0,0,1.12f},{0,0,1.12f},{0.42f,0,0},{-0.42f,0,0} };

	for (int i = 0; i < 15; i++)
	{
		m_pFig->info[i] = NULL;
		m_pFig->colors[i] = colors[i];
		m_pFig->pos[i][0] = pos[i][0];
		m_pFig->pos[i][1] = pos[i][1];
		m_pFig->pos[i][2] = pos[i][2];
		m_pFig->rot[i][0] = 0;
		m_pFig->rot[i][1] = 0;
		m_pFig->rot[i][2] = 0;
	}

	for (i = 0; i < 13; i++)
	{
		if (i == 3 || i == 7 || i == 8 || i == 9)
			continue;

		PieceInfo* pInfo = project->FindPieceInfo(partinfo[i].name);
		if (pInfo == NULL)
			continue;

		if (i == 6)
		{
			m_pFig->info[6] = pInfo;
			m_pFig->info[7] = pInfo;
			pInfo->AddRef();
			pInfo->AddRef();
			m_pFig->rot[6][0] = 45;
			m_pFig->rot[6][2] = 90;
			m_pFig->rot[7][0] = 45;
			m_pFig->rot[7][2] = 90;
		}
		else
		{
			m_pFig->info[i] = pInfo;
			pInfo->AddRef();
		}
	}

	wglMakeCurrent (oldDC, oldRC);

	return 0;
}

BOOL CMinifigWnd::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CMinifigWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	DrawScene(); 
}

void CMinifigWnd::OnDestroy() 
{
	if (m_pPal)
	{
	    CPalette palDefault;
		palDefault.CreateStockObject(DEFAULT_PALETTE);
		m_pDC->SelectPalette(&palDefault, FALSE);
		delete m_pPal;
	}

	if (m_hrc)
		wglDeleteContext(m_hrc);
	if (m_pDC)
		delete m_pDC;

	CWnd::OnDestroy();
}

void CMinifigWnd::DrawScene() 
{
	if (m_pPal)
	{
		m_pDC->SelectPalette(m_pPal, FALSE);
		m_pDC->RealizePalette();
	}

	HDC oldDC = wglGetCurrentDC();
	HGLRC oldRC = wglGetCurrentContext();
	wglMakeCurrent (m_pDC->m_hDC, m_hrc);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (int i = 0; i < 15; i++)
	{
		if (m_pFig->info[i] == NULL)
			continue;

		glPushMatrix();
		Matrix mat;
		float rot[4];
		mat.CreateOld(0,0,0, m_pFig->rot[i][0], m_pFig->rot[i][1], m_pFig->rot[i][2]);
		mat.ToAxisAngle(rot);
		glTranslatef(m_pFig->pos[i][0], m_pFig->pos[i][1], m_pFig->pos[i][2]);
		glRotatef(rot[3], rot[0], rot[1], rot[2]);
		m_pFig->info[i]->RenderPiece(m_pFig->colors[i]);
		glPopMatrix();
	}

	glFinish();
	SwapBuffers(m_pDC->m_hDC);
	wglMakeCurrent (oldDC, oldRC);
}
