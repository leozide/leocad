#include "lc_global.h"
#include "lc_renderdialog.h"
#include "ui_lc_renderdialog.h"
#include "project.h"
#include "lc_application.h"

lcRenderDialog::lcRenderDialog(QWidget* Parent)
	: QDialog(Parent),
    ui(new Ui::lcRenderDialog)
{
	mProcess = nullptr;
	mMapFile = NULL;
	mBuffer = nullptr;

	ui->setupUi(this);
	connect(&mUpdateTimer, SIGNAL(timeout()), this, SLOT(Update()));
	mUpdateTimer.start(100);
}

lcRenderDialog::~lcRenderDialog()
{
	CloseSharedMemory();

	delete ui;
}

void lcRenderDialog::on_RenderButton_clicked()
{
	if (mProcess)
	{
// todo: cancel
return;
	}

	QString FileName = QDir(QDir::tempPath()).absoluteFilePath("leocad-render.pov");

	if (!lcGetActiveProject()->ExportPOVRay(FileName))
		return;

	QStringList Arguments;

	Arguments.append(QString::fromLatin1("+I%1").arg(FileName));
//	Arguments.append("-pause");
//	Arguments.append("+IC:\\Users\\leo\\Projects\\leocad\\test.pov");
	Arguments.append("+OC:\\Users\\leo\\Projects\\leocad\\test.png");
	Arguments.append("+W800");
	Arguments.append("+H600");

	Arguments.append("+Q11");
	Arguments.append("+R3");
	Arguments.append("+A0.1");
	Arguments.append("+J0.5");

	/*
	if (!LGEOPath.isEmpty())
	{
		Arguments.append(QString::fromLatin1("+L%1lg/").arg(LGEOPath));
		Arguments.append(QString::fromLatin1("+L%1ar/").arg(LGEOPath));
	}
	Arguments.append(QString::fromLatin1("/EXIT"));
	*/

	//+LC:\Users\leo\Documents\POV-Ray\v3.7\include +IC:\Users\leo\Projects\leocad\test.pov +OC:\Users\leo\Projects\leocad\test.png
	Arguments.append("+LC:\\Users\\leo\\Documents\\POV-Ray\\v3.7\\include");
	QString POVRayPath = "C:\\Users\\leo\\Projects\\povray\\windows\\vs2015\\bin32\\povconsole32-sse2.exe";

	mProcess = new QProcess(this);
	mProcess->start(POVRayPath, Arguments);
}

void lcRenderDialog::Update()
{
	if (mProcess)
	{
		if (mProcess->state() == QProcess::NotRunning)
		{
			QString Output = mProcess->readAllStandardError() + mProcess->readAllStandardOutput();
			QMessageBox::information(this, "LeoCAD", Output);
			delete mProcess;
			mProcess = nullptr;
		}
	}

	if (mMapFile == NULL)
	{
		mMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "leocad-povray");

		if (!mMapFile)
			return;

		mBuffer = MapViewOfFile(mMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		if (!mBuffer)
		{
			CloseHandle(mMapFile);
			mMapFile = NULL;
			return;
		}

		uint32_t* Header = (uint32_t*)mBuffer;
		if (Header[0] != 1)
		{
			CloseSharedMemory();
			return;
		}
	}

	if (!mBuffer)
		return;

	struct lcSharedMemoryHeader
	{
		uint32_t Version;
		uint32_t Width;
		uint32_t Height;
		uint32_t PixelsWritten;
		uint32_t PixelsRead;
	};

	lcSharedMemoryHeader* Header = (lcSharedMemoryHeader*)mBuffer;

	int Width = Header->Width;
	int Height = Header->Height;
	int PixelsRead = Header->PixelsWritten;

//		if (width != expected) ...

	QImage Image(Width, Height, QImage::Format_ARGB32);
	uint8_t* Pixels = (uint8_t*)(Header + 1);

	for (int y = 0; y < Height; y++)
	{
		for (int x = 0; x < Width; x++)
		{
			Image.setPixelColor(x, y, QColor::fromRgb(Pixels[0], Pixels[1], Pixels[2], Pixels[3]));
			Pixels += 4;
		}
	}

	Header->PixelsRead = PixelsRead;

	ui->label->setPixmap(QPixmap::fromImage(Image));
}

void lcRenderDialog::CloseSharedMemory()
{
	if (mBuffer)
	{
		UnmapViewOfFile(mBuffer);
		mBuffer = NULL;
	}

	if (mMapFile)
	{
		CloseHandle(mMapFile);
		mMapFile = NULL;
	}
}
