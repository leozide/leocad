# Microsoft Developer Studio Project File - Name="LeoCAD" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=LeoCAD - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "LeoCAD.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "LeoCAD.mak" CFG="LeoCAD - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "LeoCAD - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "LeoCAD - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "LeoCAD - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../common" /I "../win" /D "NDEBUG" /D "WIN32" /D "LC_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 vfw32.lib jpeglib.lib 3dsftk.lib libpng.lib zlib.lib /nologo /subsystem:windows /map /machine:I386 /nodefaultlib:"libc.lib" /libpath:"./jpeglib/release" /libpath:"./3dsftk/release" /libpath:"./libpng/release" /libpath:"./zlib/release"

!ELSEIF  "$(CFG)" == "LeoCAD - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /Gi /GX /ZI /Od /I "../common" /I "../win" /D "_DEBUG" /D "WIN32" /D "LC_WINDOWS" /D "LC_DEBUG" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 vfw32.lib jpeglib.lib 3dsftk.lib libpng.lib zlib.lib gdi32.lib /nologo /subsystem:windows /map /debug /machine:I386 /nodefaultlib:"libcd.lib" /libpath:"./jpeglib/debug" /libpath:"./3dsftk/debug" /libpath:"./libpng/debug" /libpath:"./zlib/debug"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "LeoCAD - Win32 Release"
# Name "LeoCAD - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Aboutdlg.cpp
# End Source File
# Begin Source File

SOURCE=..\common\array.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\Arraydlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Barcmdui.cpp
# End Source File
# Begin Source File

SOURCE=.\basewnd.cpp
# End Source File
# Begin Source File

SOURCE=.\Bmpmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\Cadbar.cpp
# End Source File
# Begin Source File

SOURCE=.\Caddoc.cpp
# End Source File
# Begin Source File

SOURCE=.\Cadview.cpp
# End Source File
# Begin Source File

SOURCE=.\categdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Clrpick.cpp
# End Source File
# Begin Source File

SOURCE=.\Clrpopup.cpp
# End Source File
# Begin Source File

SOURCE=.\Colorlst.cpp
# End Source File
# Begin Source File

SOURCE=.\Disabtab.cpp
# End Source File
# Begin Source File

SOURCE=.\EdGrpDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Figdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Flatbar.cpp
# End Source File
# Begin Source File

SOURCE=.\glwindow.cpp
# End Source File
# Begin Source File

SOURCE=.\Groupdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\GrpTree.cpp
# End Source File
# Begin Source File

SOURCE=.\Htmldlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Imagedlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Ipedit.cpp
# End Source File
# Begin Source File

SOURCE=.\keyedit.cpp
# End Source File
# Begin Source File

SOURCE=.\Leocad.cpp
# End Source File
# Begin Source File

SOURCE=.\hlp\LeoCAD.hpj

!IF  "$(CFG)" == "LeoCAD - Win32 Release"

USERDEP__LEOCA="$(ProjDir)\hlp\AfxCore.rtf"	"$(ProjDir)\hlp\AfxPrint.rtf"	
# Begin Custom Build - Making help file...
OutDir=.\Release
ProjDir=.
TargetName=LeoCAD
InputPath=.\hlp\LeoCAD.hpj

"$(OutDir)\$(TargetName).hlp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	call "$(ProjDir)\makehelp.bat"

# End Custom Build

!ELSEIF  "$(CFG)" == "LeoCAD - Win32 Debug"

USERDEP__LEOCA="$(ProjDir)\hlp\AfxCore.rtf"	"$(ProjDir)\hlp\AfxPrint.rtf"	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Leocad.rc
# End Source File
# Begin Source File

SOURCE=.\Libdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Moddlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Piecebar.cpp
# End Source File
# Begin Source File

SOURCE=.\Piececmb.cpp
# End Source File
# Begin Source File

SOURCE=.\Pieceprv.cpp
# End Source File
# Begin Source File

SOURCE=.\Povdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Prefpage.cpp
# End Source File
# Begin Source File

SOURCE=.\Prefsht.cpp
# End Source File
# Begin Source File

SOURCE=.\Prevview.cpp
# End Source File
# Begin Source File

SOURCE=.\Print.cpp
# End Source File
# Begin Source File

SOURCE=.\Progdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Propspgs.cpp
# End Source File
# Begin Source File

SOURCE=.\Propssht.cpp
# End Source File
# Begin Source File

SOURCE=.\Rmodel.cpp
# End Source File
# Begin Source File

SOURCE=.\Seldlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Splitter.cpp
# End Source File
# Begin Source File

SOURCE=.\Stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Stepdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Steppop.cpp
# End Source File
# Begin Source File

SOURCE=.\system.cpp
# End Source File
# Begin Source File

SOURCE=.\Teropdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Terrctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\Terrdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Terrwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\texdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Tools.cpp
# End Source File
# Begin Source File

SOURCE=.\transdlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Wheelwnd.cpp
# End Source File
# Begin Source File

SOURCE=.\win_gl.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\ArrayDlg.h
# End Source File
# Begin Source File

SOURCE=.\Barcmdui.h
# End Source File
# Begin Source File

SOURCE=.\Bmpmenu.h
# End Source File
# Begin Source File

SOURCE=.\CADBar.h
# End Source File
# Begin Source File

SOURCE=.\CADDoc.h
# End Source File
# Begin Source File

SOURCE=.\CADView.h
# End Source File
# Begin Source File

SOURCE=.\categdlg.h
# End Source File
# Begin Source File

SOURCE=.\Clrpick.h
# End Source File
# Begin Source File

SOURCE=.\Clrpopup.h
# End Source File
# Begin Source File

SOURCE=.\ColorLst.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\Disabtab.h
# End Source File
# Begin Source File

SOURCE=.\EdGrpDlg.h
# End Source File
# Begin Source File

SOURCE=.\Figdlg.h
# End Source File
# Begin Source File

SOURCE=.\Flatbar.h
# End Source File
# Begin Source File

SOURCE=.\GroupDlg.h
# End Source File
# Begin Source File

SOURCE=.\GrpTree.h
# End Source File
# Begin Source File

SOURCE=.\HTMLDlg.h
# End Source File
# Begin Source File

SOURCE=.\ImageDlg.h
# End Source File
# Begin Source File

SOURCE=.\Ipedit.h
# End Source File
# Begin Source File

SOURCE=.\keyedit.h
# End Source File
# Begin Source File

SOURCE=.\LeoCAD.h
# End Source File
# Begin Source File

SOURCE=.\LibDlg.h
# End Source File
# Begin Source File

SOURCE=.\Light.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\ModDlg.h
# End Source File
# Begin Source File

SOURCE=.\Piecebar.h
# End Source File
# Begin Source File

SOURCE=.\PieceCmb.h
# End Source File
# Begin Source File

SOURCE=.\PieceInf.h
# End Source File
# Begin Source File

SOURCE=.\PiecePrv.h
# End Source File
# Begin Source File

SOURCE=.\Povdlg.h
# End Source File
# Begin Source File

SOURCE=.\PrefPage.h
# End Source File
# Begin Source File

SOURCE=.\PrefSht.h
# End Source File
# Begin Source File

SOURCE=.\PrevView.h
# End Source File
# Begin Source File

SOURCE=.\Print.h
# End Source File
# Begin Source File

SOURCE=.\ProgDlg.h
# End Source File
# Begin Source File

SOURCE=.\PropsPgs.h
# End Source File
# Begin Source File

SOURCE=.\PropsSht.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\RModel.h
# End Source File
# Begin Source File

SOURCE=.\SelDlg.h
# End Source File
# Begin Source File

SOURCE=.\Splitter.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\StepDlg.h
# End Source File
# Begin Source File

SOURCE=.\StepPop.h
# End Source File
# Begin Source File

SOURCE=.\TerOpDlg.h
# End Source File
# Begin Source File

SOURCE=.\Terrctrl.h
# End Source File
# Begin Source File

SOURCE=.\TerrDlg.h
# End Source File
# Begin Source File

SOURCE=.\Terrwnd.h
# End Source File
# Begin Source File

SOURCE=.\texdlg.h
# End Source File
# Begin Source File

SOURCE=.\Titletip.h
# End Source File
# Begin Source File

SOURCE=.\Tools.h
# End Source File
# Begin Source File

SOURCE=.\transdlg.h
# End Source File
# Begin Source File

SOURCE=.\WheelWnd.h
# End Source File
# Begin Source File

SOURCE=.\win_gl.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Angle.cur
# End Source File
# Begin Source File

SOURCE=.\res\animator.bmp
# End Source File
# Begin Source File

SOURCE=.\res\autopan.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Brick.cur
# End Source File
# Begin Source File

SOURCE=.\res\bulb.cur
# End Source File
# Begin Source File

SOURCE=.\res\CADDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\camera.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Camera.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00001.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00002.cur
# End Source File
# Begin Source File

SOURCE=.\res\cur00003.cur
# End Source File
# Begin Source File

SOURCE=.\res\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\res\delete.bmp
# End Source File
# Begin Source File

SOURCE=.\res\editor.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Eraser.cur
# End Source File
# Begin Source File

SOURCE=.\res\fullscr.bmp
# End Source File
# Begin Source File

SOURCE=.\res\fullscre.bmp
# End Source File
# Begin Source File

SOURCE=.\res\group.bmp
# End Source File
# Begin Source File

SOURCE=.\res\help.bmp
# End Source File
# Begin Source File

SOURCE=.\res\home.bmp
# End Source File
# Begin Source File

SOURCE=.\res\idr_main.ico
# End Source File
# Begin Source File

SOURCE=.\res\idr_part.ico
# End Source File
# Begin Source File

SOURCE=.\res\info.bmp
# End Source File
# Begin Source File

SOURCE=.\res\IToolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\LeoCAD.ico
# End Source File
# Begin Source File

SOURCE=.\res\LeoCAD.rc2
# End Source File
# Begin Source File

SOURCE=.\res\library.bmp
# End Source File
# Begin Source File

SOURCE=.\res\light.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Light.cur
# End Source File
# Begin Source File

SOURCE=.\res\mail.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Move.cur
# End Source File
# Begin Source File

SOURCE=.\res\Paint.cur
# End Source File
# Begin Source File

SOURCE=.\res\Pan.cur
# End Source File
# Begin Source File

SOURCE=.\res\pan_ne.cur
# End Source File
# Begin Source File

SOURCE=.\res\pan_nw.cur
# End Source File
# Begin Source File

SOURCE=.\res\pan_se.cur
# End Source File
# Begin Source File

SOURCE=.\res\pan_sw.cur
# End Source File
# Begin Source File

SOURCE=.\res\panall.cur
# End Source File
# Begin Source File

SOURCE=.\res\pandown.cur
# End Source File
# Begin Source File

SOURCE=.\res\panleft.cur
# End Source File
# Begin Source File

SOURCE=.\res\panright.cur
# End Source File
# Begin Source File

SOURCE=.\res\panup.cur
# End Source File
# Begin Source File

SOURCE=.\res\particon.bmp
# End Source File
# Begin Source File

SOURCE=.\res\photo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\piece.bmp
# End Source File
# Begin Source File

SOURCE=.\res\piecebar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\pieceed.bmp
# End Source File
# Begin Source File

SOURCE=.\res\preferen.bmp
# End Source File
# Begin Source File

SOURCE=.\res\preview.bmp
# End Source File
# Begin Source File

SOURCE=.\Resource.hm
# End Source File
# Begin Source File

SOURCE=.\res\roll.cur
# End Source File
# Begin Source File

SOURCE=.\res\Rotate.cur
# End Source File
# Begin Source File

SOURCE=.\res\rotx.cur
# End Source File
# Begin Source File

SOURCE=.\res\roty.cur
# End Source File
# Begin Source File

SOURCE=.\res\SelctGrp.cur
# End Source File
# Begin Source File

SOURCE=.\res\select.cur
# End Source File
# Begin Source File

SOURCE=.\res\split.cur
# End Source File
# Begin Source File

SOURCE=.\res\terrain.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\toolsbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ungroup.bmp
# End Source File
# Begin Source File

SOURCE=.\res\views.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Zoom.cur
# End Source File
# Begin Source File

SOURCE=.\res\zoomin.bmp
# End Source File
# Begin Source File

SOURCE=.\res\zoomout.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Zoomrect.cur
# End Source File
# End Group
# Begin Group "Help Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=".\$(ProjDir)\hlp\AfxCore.rtf"
# End Source File
# Begin Source File

SOURCE=".\$(ProjDir)\hlp\AfxCore.rtf"
# End Source File
# Begin Source File

SOURCE=.\hlp\AfxCore.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AfxOleSv.rtf
# End Source File
# Begin Source File

SOURCE=".\$(ProjDir)\hlp\AfxPrint.rtf"
# End Source File
# Begin Source File

SOURCE=".\$(ProjDir)\hlp\AfxPrint.rtf"
# End Source File
# Begin Source File

SOURCE=.\hlp\AfxPrint.rtf
# End Source File
# Begin Source File

SOURCE=.\hlp\AppExit.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Bullet.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw2.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurArw4.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\CurHelp.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCopy.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditCut.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditPast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\EditUndo.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileNew.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileOpen.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FilePrnt.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\FileSave.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpSBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\HlpTBar.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\LeoCAD.cnt
# End Source File
# Begin Source File

SOURCE=.\MakeHelp.bat
# End Source File
# Begin Source File

SOURCE=.\hlp\RecFirst.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecLast.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecNext.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\RecPrev.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmax.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\ScMenu.bmp
# End Source File
# Begin Source File

SOURCE=.\hlp\Scmin.bmp
# End Source File
# End Group
# Begin Group "Common Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=..\common\array.h
# End Source File
# Begin Source File

SOURCE=..\common\basewnd.h
# End Source File
# Begin Source File

SOURCE=..\Common\camera.h
# End Source File
# Begin Source File

SOURCE=..\common\console.h
# End Source File
# Begin Source File

SOURCE=..\common\curve.h
# End Source File
# Begin Source File

SOURCE=..\Common\defines.h
# End Source File
# Begin Source File

SOURCE=..\Common\file.h
# End Source File
# Begin Source File

SOURCE=..\Common\globals.h
# End Source File
# Begin Source File

SOURCE=..\common\glwindow.h
# End Source File
# Begin Source File

SOURCE=..\Common\group.h
# End Source File
# Begin Source File

SOURCE=..\Common\image.h
# End Source File
# Begin Source File

SOURCE=..\common\keyboard.h
# End Source File
# Begin Source File

SOURCE=..\common\lc_application.h
# End Source File
# Begin Source File

SOURCE=..\common\library.h
# End Source File
# Begin Source File

SOURCE=..\Common\light.h
# End Source File
# Begin Source File

SOURCE=..\common\mainwnd.h
# End Source File
# Begin Source File

SOURCE=..\Common\matrix.h
# End Source File
# Begin Source File

SOURCE=..\common\message.h
# End Source File
# Begin Source File

SOURCE=..\common\minifig.h
# End Source File
# Begin Source File

SOURCE=..\common\object.h
# End Source File
# Begin Source File

SOURCE=..\common\opengl.h
# End Source File
# Begin Source File

SOURCE=..\Common\piece.h
# End Source File
# Begin Source File

SOURCE=..\Common\pieceinf.h
# End Source File
# Begin Source File

SOURCE=..\common\preview.h
# End Source File
# Begin Source File

SOURCE=..\Common\project.h
# End Source File
# Begin Source File

SOURCE=..\Common\quant.h
# End Source File
# Begin Source File

SOURCE=..\common\str.h
# End Source File
# Begin Source File

SOURCE=..\common\system.h
# End Source File
# Begin Source File

SOURCE=..\Common\terrain.h
# End Source File
# Begin Source File

SOURCE=..\common\texfont.h
# End Source File
# Begin Source File

SOURCE=..\Common\texture.h
# End Source File
# Begin Source File

SOURCE=..\Common\Tr.h
# End Source File
# Begin Source File

SOURCE=..\Common\typedefs.h
# End Source File
# Begin Source File

SOURCE=..\Common\vector.h
# End Source File
# Begin Source File

SOURCE=..\common\view.h
# End Source File
# End Group
# Begin Group "Common Source Files"

# PROP Default_Filter "c;cpp"
# Begin Source File

SOURCE=..\common\algebra.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\camera.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\console.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\curve.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\debug.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\file.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\globals.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\group.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\im_bmp.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\im_gif.cpp
# ADD CPP /I "./jpeglib"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\im_jpg.cpp
# ADD CPP /I "./jpeglib"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\im_png.cpp
# ADD CPP /I "./libpng" /I "./zlib"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\image.cpp
# ADD CPP /I "./jpeglib"
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\keyboard.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\lc_application.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\library.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\light.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\mainwnd.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\matrix.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\message.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\minifig.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\object.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\opengl.cpp

!IF  "$(CFG)" == "LeoCAD - Win32 Release"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "LeoCAD - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Common\piece.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\pieceinf.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\preview.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\project.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\quant.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\str.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\terrain.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\texfont.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\texture.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\Tr.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\Common\vector.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\common\view.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Source File

SOURCE=..\docs\CHANGES.txt
# End Source File
# End Target
# End Project
