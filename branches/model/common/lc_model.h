#ifndef LC_MODEL_H
#define LC_MODEL_H

#include "lc_array.h"
#include "lc_math.h"
#include "lc_object.h"

class View;

class lcModel
{
public:
	lcModel();
	~lcModel();

	void DeleteContents();
	void AddCamera(lcCamera* Camera);

	void Update(lcKeyTime Time);

	void RenderBackground(View* View);
	void RenderObjects(View* View);

	lcArray<lcPart*> mParts;
	lcArray<lcCamera*> mCameras;
	lcArray<lcLight*> mLights;
};

#endif // LC_MODEL_H
