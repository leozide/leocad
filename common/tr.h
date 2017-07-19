// TR.h: interface for the TiledRender class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class TiledRender
{
public:
	TiledRender();
	~TiledRender();

	void TileSize(int width, int height);
	void ImageSize(int width, int height);
	void Ortho(double left, double right, double bottom, double top, double zNear, double zFar);
	void Frustum(double left, double right, double bottom, double top, double zNear, double zFar);
	void Perspective(double fovy, double aspect, double zNear, double zFar );
	int EndTile();
	lcMatrix44 BeginTile();

	// Final image parameters
	int mImageWidth, mImageHeight;

	// Tile parameters
	int mTileWidth, mTileHeight;

	// Projection parameters
	bool mPerspective;
	double mLeft;
	double mRight;
	double mBottom;
	double mTop;
	double mNear;
	double mFar;

	// Misc
	int mRows, mColumns;
	int mCurrentTile;
	int mCurrentTileWidth, mCurrentTileHeight;
	int mCurrentRow, mCurrentColumn;

	GLint mViewportSave[4];
};

