#include "lc_global.h"
#include "lc_math.h"
#include "lc_projection.h"
#include "camera.h"

void lcProjection::setTransformInput(const Camera* pCamera, int width, int height)
{
	mViewPixelWidth = width;
	mViewPixelHeight = height;
	mCamera = pCamera;

	calculateTransform();
}

void lcProjection::calculateTransform()
{
	if (NULL == mCamera)
	{
		return;
	}

	float fAspect = (float)mViewPixelWidth/(float)mViewPixelHeight;

	switch(mType)
	{
		case Ortho:
		{
			// Compute the FOV/plane intersection radius.
			//                d               d
            //   a = 2 atan(------) => ~ a = --- => d = af
			//                2f              f
			float f = (mCamera->mPosition - mCamera->mOrthoTarget).Length();
			float d = ( mCamera->m_fovy * f) * (LC_PI / 180.0f);
			float r = d/2;

			float right = r * fAspect;
			mTransform = lcMatrix44Ortho(-right, right, -r, r, mCamera->m_zNear, mCamera->m_zFar * 4);

			return;
		}

		case Projection:
		default:
		{
			mTransform = lcMatrix44Perspective(mCamera->m_fovy, fAspect, mCamera->m_zNear, mCamera->m_zFar);
			return;
		}
	}
}
