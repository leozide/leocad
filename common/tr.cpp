#include "lc_global.h"
#include "tr.h"
#include "lc_math.h"

TiledRender::TiledRender()
{
	mTileWidth = 256;
	mTileHeight = 256;
	mCurrentTile = -1;
}

TiledRender::~TiledRender()
{
}

void TiledRender::TileSize(int width, int height)
{
	mTileWidth = width;
	mTileHeight = height;
}

void TiledRender::ImageSize(int width, int height)
{
	mImageWidth = width;
	mImageHeight = height;
}

void TiledRender::Ortho(double left, double right, double bottom, double top, double zNear, double zFar)
{
	if (mCurrentTile < 0)
	{
		mPerspective = false;
		mLeft = left;
		mRight = right;
		mBottom = bottom;
		mTop = top;
		mNear = zNear;
		mFar = zFar;
	}
}

void TiledRender::Frustum(double left, double right, double bottom, double top, double zNear, double zFar)
{
	if (mCurrentTile < 0)
	{
		mPerspective = true;
		mLeft = left;
		mRight = right;
		mBottom = bottom;
		mTop = top;
		mNear = zNear;
		mFar = zFar;
	}
}

void TiledRender::Perspective(double fovy, double aspect, double zNear, double zFar)
{
	double xmin, xmax, ymin, ymax;
	ymax = zNear * tan(fovy * 3.14159265 / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;
	Frustum(xmin, xmax, ymin, ymax, zNear, zFar);
}

lcMatrix44 TiledRender::BeginTile()
{
	int tileWidth, tileHeight;
	double left, right, bottom, top;

	if (mCurrentTile <= 0)
	{
		mColumns = (mImageWidth + mTileWidth - 1) / mTileWidth;
		mRows = (mImageHeight + mTileHeight - 1) / mTileHeight;
		mCurrentTile = 0;

		// Save user's viewport, will be restored after last tile rendered
		glGetIntegerv(GL_VIEWPORT, mViewportSave);
	}

	// which tile (by row and column) we're about to render
	mCurrentRow = mCurrentTile / mColumns;
	mCurrentColumn = mCurrentTile % mColumns;

	// Compute actual size of this tile with border
	if (mCurrentRow < mRows - 1)
		tileHeight = mTileHeight;
	else
		tileHeight = mImageHeight - (mRows - 1) * (mTileHeight);

	if (mCurrentColumn < mColumns - 1)
		tileWidth = mTileWidth;
	else
		tileWidth = mImageWidth - (mColumns - 1) * (mTileWidth);

	// Save tile size, with border
	mCurrentTileWidth = tileWidth;
	mCurrentTileHeight = tileHeight;

	glViewport(0, 0, tileWidth, tileHeight);  // tile size including border

	// compute projection parameters
	left = mLeft + (mRight - mLeft) * (mCurrentColumn * mTileWidth) / mImageWidth;
	right = left + (mRight - mLeft) * tileWidth / mImageWidth;
	bottom = mBottom + (mTop - mBottom) * (mCurrentRow * mTileHeight) / mImageHeight;
	top = bottom + (mTop - mBottom) * tileHeight / mImageHeight;

	if (mPerspective)
		return lcMatrix44Frustum(left, right, bottom, top, mNear, mFar);
	else
		return lcMatrix44Ortho(left, right, bottom, top, mNear, mFar);
}

int TiledRender::EndTile()
{
	// be sure OpenGL rendering is finished
	glFlush();

	// increment tile counter, return 1 if more tiles left to render
	mCurrentTile++;
	if (mCurrentTile >= mRows * mColumns)
	{
		// restore user's viewport
		glViewport(mViewportSave[0], mViewportSave[1], mViewportSave[2], mViewportSave[3]);
		mCurrentTile = -1;  // all done
		return 0;
	}
	else
		return 1;
}
