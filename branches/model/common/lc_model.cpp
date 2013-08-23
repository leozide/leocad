#include "lc_global.h"
#include "lc_model.h"
#include "lc_part.h"
#include "lc_camera.h"
#include "lc_light.h"

lcModel::lcModel()
{
}

lcModel::~lcModel()
{
	DeleteContents();
}

void lcModel::DeleteContents()
{
	for (int PartIdx = 0; PartIdx < mParts.GetSize(); PartIdx++)
		delete mParts[PartIdx];
	mParts.RemoveAll();

	for (int CameraIdx = 0; CameraIdx < mCameras.GetSize(); CameraIdx++)
		delete mCameras[CameraIdx];
	mCameras.RemoveAll();

	for (int LightIdx = 0; LightIdx < mLights.GetSize(); LightIdx++)
		delete mLights[LightIdx];
	mLights.RemoveAll();
}

