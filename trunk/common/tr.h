// TR.h: interface for the TiledRender class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _TR_H_
#define _TR_H_

typedef enum {
	TR_TILE_WIDTH = 100,
	TR_TILE_HEIGHT,
	TR_TILE_BORDER,
	TR_IMAGE_WIDTH,
	TR_IMAGE_HEIGHT,
	TR_ROWS,
	TR_COLUMNS,
	TR_CURRENT_ROW,
	TR_CURRENT_COLUMN,
	TR_CURRENT_TILE_WIDTH,
	TR_CURRENT_TILE_HEIGHT,
	TR_ROW_ORDER,
	TR_TOP_TO_BOTTOM,
	TR_BOTTOM_TO_TOP
} TRenum;

class TiledRender
{
public:
	TiledRender();
	virtual ~TiledRender();

	void TileSize(int width, int height, int border);
	void TileBuffer(TRenum format, TRenum type, void *image);
	void ImageSize(int width, int height);
	void ImageBuffer(TRenum format, TRenum type, void *image);
	void RowOrder(TRenum order);
	void Ortho(double left, double right, double bottom, double top, double zNear, double zFar);
	void Frustum(double left, double right, double bottom, double top, double zNear, double zFar);
	void Perspective(double fovy, double aspect, double zNear, double zFar );
	void RasterPos3f(float x, float y, float z);
	int Get(TRenum param);
	int EndTile();
	void BeginTile();

	// Final image parameters
	int m_ImageWidth, m_ImageHeight;
	TRenum m_ImageFormat, m_ImageType;
	void *m_ImageBuffer;

	// Tile parameters
	int m_TileWidth, m_TileHeight;
	int m_TileWidthNB, m_TileHeightNB;
	int m_TileBorder;
	TRenum m_TileFormat, m_TileType;
	void *m_TileBuffer;

	// Projection parameters
	bool m_Perspective;
	double m_Left;
	double m_Right;
	double m_Bottom;
	double m_Top;
	double m_Near;
	double m_Far;

	// Misc
	TRenum m_RowOrder;
	int m_Rows, m_Columns;
	int m_CurrentTile;
	int m_CurrentTileWidth, m_CurrentTileHeight;
	int m_CurrentRow, m_CurrentColumn;

	GLint m_ViewportSave[4];
};

#endif // _TR_H_
