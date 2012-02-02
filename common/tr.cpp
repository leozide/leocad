// TR.cpp: implementation of the TiledRender class.
//
//////////////////////////////////////////////////////////////////////

#include <math.h>
#include "opengl.h"
#include "tr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TiledRender::TiledRender()
{
	m_TileWidth = 256;
	m_TileHeight = 256;
	m_TileBorder = 0;
	m_RowOrder = TR_BOTTOM_TO_TOP;
	m_CurrentTile = -1;
	m_ImageBuffer = 0;
	m_TileBuffer = 0;
}

TiledRender::~TiledRender()
{
}

void TiledRender::TileSize(int width, int height, int border)
{
	m_TileBorder = border;
	m_TileWidth = width;
	m_TileHeight = height;
	m_TileWidthNB = width - 2 * border;
	m_TileHeightNB = height - 2 * border;
}

void TiledRender::TileBuffer(TRenum format, TRenum type, void *image)
{
	m_TileFormat = format;
	m_TileType = type;
	m_TileBuffer = image;
}

void TiledRender::ImageSize(int width, int height)
{
	m_ImageWidth = width;
	m_ImageHeight = height;
}

void TiledRender::ImageBuffer(TRenum format, TRenum type, void *image)
{
	m_ImageFormat = format;
	m_ImageType = type;
	m_ImageBuffer = image;
}

void TiledRender::RowOrder(TRenum order)
{
	if (order == TR_TOP_TO_BOTTOM || order == TR_BOTTOM_TO_TOP)
		m_RowOrder = order;
}

int TiledRender::Get(TRenum param)
{
	switch (param) 
	{
	case TR_TILE_WIDTH:
		return m_TileWidth;
	case TR_TILE_HEIGHT:
		return m_TileHeight;
	case TR_TILE_BORDER:
		return m_TileBorder;
	case TR_IMAGE_WIDTH:
		return m_ImageWidth;
	case TR_IMAGE_HEIGHT:
		return m_ImageHeight;
	case TR_ROWS:
		return m_Rows;
	case TR_COLUMNS:
		return m_Columns;
	case TR_CURRENT_ROW:
		if (m_CurrentTile < 0)
            return -1;
		else
            return m_CurrentRow;
	case TR_CURRENT_COLUMN:
		if (m_CurrentTile < 0)
            return -1;
		else
            return m_CurrentColumn;
	case TR_CURRENT_TILE_WIDTH:
		return m_CurrentTileWidth;
	case TR_CURRENT_TILE_HEIGHT:
		return m_CurrentTileHeight;
	case TR_ROW_ORDER:
		return (int) m_RowOrder;
	default:
		return 0;
	}
}

void TiledRender::Ortho(double left, double right, double bottom, double top, double zNear, double zFar)
{
	if (m_CurrentTile < 0) 
	{
		m_Perspective = false;
		m_Left = left;
		m_Right = right;
		m_Bottom = bottom;
		m_Top = top;
		m_Near = zNear;
		m_Far = zFar;
	}
}

void TiledRender::Frustum(double left, double right, double bottom, double top, double zNear, double zFar)
{
	if (m_CurrentTile < 0) 
	{
		m_Perspective = true;
		m_Left = left;
		m_Right = right;
		m_Bottom = bottom;
		m_Top = top;
		m_Near = zNear;
		m_Far = zFar;
	}
}

void TiledRender::Perspective(double fovy, double aspect, double zNear, double zFar )
{
	double xmin, xmax, ymin, ymax;
	ymax = zNear * tan(fovy * 3.14159265 / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;
	Frustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

void TiledRender::BeginTile()
{
	GLint matrixMode;
	int tileWidth, tileHeight, tileWidthNB, tileHeightNB, border;
	double left, right, bottom, top;
	
	if (m_CurrentTile <= 0)
	{
		m_Columns = (m_ImageWidth + m_TileWidthNB - 1) / m_TileWidthNB;
		m_Rows = (m_ImageHeight + m_TileHeightNB - 1) / m_TileHeightNB;
		m_CurrentTile = 0;
		
		// Save user's viewport, will be restored after last tile rendered
		glGetIntegerv(GL_VIEWPORT, m_ViewportSave);
	}
	
	// which tile (by row and column) we're about to render
	if (m_RowOrder == TR_BOTTOM_TO_TOP)
	{
		m_CurrentRow = m_CurrentTile / m_Columns;
		m_CurrentColumn = m_CurrentTile % m_Columns;
	}
	else if (m_RowOrder==TR_TOP_TO_BOTTOM) 
	{
		m_CurrentRow = m_Rows - (m_CurrentTile / m_Columns) - 1;
		m_CurrentColumn = m_CurrentTile % m_Columns;
	}

	border = m_TileBorder;
	
	// Compute actual size of this tile with border
	if (m_CurrentRow < m_Rows-1)
		tileHeight = m_TileHeight;
	else
		tileHeight = m_ImageHeight - (m_Rows-1) * (m_TileHeightNB) + 2 * border;
	
	if (m_CurrentColumn < m_Columns-1)
		tileWidth = m_TileWidth;
	else
		tileWidth = m_ImageWidth - (m_Columns-1) * (m_TileWidthNB) + 2 * border;
	
	// tile size with No Border
	tileWidthNB = tileWidth - 2 * border;
	tileHeightNB = tileHeight - 2 * border;
	
	// Save tile size, with border
	m_CurrentTileWidth = tileWidth;
	m_CurrentTileHeight = tileHeight;
	
	glViewport(0, 0, tileWidth, tileHeight);  // tile size including border
	
	// save current matrix mode
	glGetIntegerv(GL_MATRIX_MODE, &matrixMode);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// compute projection parameters
	left = m_Left + (m_Right - m_Left)
        * (m_CurrentColumn * m_TileWidthNB - border) / m_ImageWidth;
	right = left + (m_Right - m_Left) * tileWidth / m_ImageWidth;
	bottom = m_Bottom + (m_Top - m_Bottom)
		* (m_CurrentRow * m_TileHeightNB - border) / m_ImageHeight;
	top = bottom + (m_Top - m_Bottom) * tileHeight / m_ImageHeight;
	
	if (m_Perspective)
		glFrustum(left, right, bottom, top, m_Near, m_Far);
	else
		glOrtho(left, right, bottom, top, m_Near, m_Far);
	
	// restore user's matrix mode
	glMatrixMode((GLenum)matrixMode);
}

int TiledRender::EndTile()
{
	GLint prevRowLength, prevSkipRows, prevSkipPixels;
	
	// be sure OpenGL rendering is finished
	glFlush();
	
	// save current glPixelStore values
	glGetIntegerv(GL_PACK_ROW_LENGTH, &prevRowLength);
	glGetIntegerv(GL_PACK_SKIP_ROWS, &prevSkipRows);
	glGetIntegerv(GL_PACK_SKIP_PIXELS, &prevSkipPixels);
	
	if (m_TileBuffer) 
	{
		int srcX = m_TileBorder;
		int srcY = m_TileBorder;
		int srcWidth = m_TileWidthNB;
		int srcHeight = m_TileHeightNB;
		glReadPixels(srcX, srcY, srcWidth, srcHeight,
			(GLenum)m_TileFormat, (GLenum)m_TileType, m_TileBuffer);
	}
	
	if (m_ImageBuffer) 
	{
		int srcX = m_TileBorder;
		int srcY = m_TileBorder;
		int srcWidth = m_CurrentTileWidth - 2 * m_TileBorder;
		int srcHeight = m_CurrentTileHeight - 2 * m_TileBorder;
		int destX = m_TileWidthNB * m_CurrentColumn;
		int destY = m_TileHeightNB * m_CurrentRow;
		
		// setup pixel store for glReadPixels
		glPixelStorei(GL_PACK_ROW_LENGTH, m_ImageWidth);
		glPixelStorei(GL_PACK_SKIP_ROWS, destY);
		glPixelStorei(GL_PACK_SKIP_PIXELS, destX);
		
		// read the tile into the final image
		glReadPixels(srcX, srcY, srcWidth, srcHeight,
			(GLenum)m_ImageFormat, (GLenum)m_ImageType, m_ImageBuffer);
	}
	
	// restore previous glPixelStore values
	glPixelStorei(GL_PACK_ROW_LENGTH, prevRowLength);
	glPixelStorei(GL_PACK_SKIP_ROWS, prevSkipRows);
	glPixelStorei(GL_PACK_SKIP_PIXELS, prevSkipPixels);
	
	// increment tile counter, return 1 if more tiles left to render
	m_CurrentTile++;
	if (m_CurrentTile >= m_Rows * m_Columns) 
	{
		// restore user's viewport
		glViewport(m_ViewportSave[0], m_ViewportSave[1],
			m_ViewportSave[2], m_ViewportSave[3]);
		m_CurrentTile = -1;  // all done
		return 0;
	}
	else
		return 1;
}
