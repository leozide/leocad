# Microsoft Developer Studio Project File - Name="3dsftk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=3dsftk - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "3dsftk.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "3dsftk.mak" CFG="3dsftk - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "3dsftk - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "3dsftk - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "3dsftk - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "3dsftk - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "3dsftk - Win32 Release"
# Name "3dsftk - Win32 Debug"
# Begin Source File

SOURCE=.\Source\3dsambm.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsambm.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsaset.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsaset.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsbbox.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsbbox.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsbgnd.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsbgnd.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dscamm.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dscamm.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dscamr.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dscamr.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dserr.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dserr.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsfile.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsfile.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsftkst.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dshier.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dshier.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsiobj.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsiobj.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dskey.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dskey.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dslites.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dslites.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmatr.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmatr.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmatx.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmatx.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmobj.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmobj.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmset.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsmset.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsobjm.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsobjm.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsomnm.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsomnm.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsprim.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsprim.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsrange.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsrobj.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsrobj.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dssptm.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dssptm.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsstrlf.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsstrlf.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dstype.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsutil.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsutil.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsvers.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsvers.h
# End Source File
# Begin Source File

SOURCE=.\Source\3dsvprt.c
# End Source File
# Begin Source File

SOURCE=.\Source\3dsvprt.h
# End Source File
# Begin Source File

SOURCE=.\Source\Assert.c
# End Source File
# Begin Source File

SOURCE=.\Source\Chunk3ds.c
# End Source File
# Begin Source File

SOURCE=.\Source\Chunk3ds.h
# End Source File
# Begin Source File

SOURCE=.\Source\Chunkinf.h
# End Source File
# Begin Source File

SOURCE=.\Source\Dbase3ds.c
# End Source File
# Begin Source File

SOURCE=.\Source\Dbase3ds.h
# End Source File
# Begin Source File

SOURCE=.\Source\Dumpchnk.c
# End Source File
# Begin Source File

SOURCE=.\Source\Dumpchnk.h
# End Source File
# Begin Source File

SOURCE=.\Source\Dumpsrc.c
# End Source File
# Begin Source File

SOURCE=.\Source\Dumpsrc.h
# End Source File
# Begin Source File

SOURCE=.\Source\Dumpstr.c
# End Source File
# Begin Source File

SOURCE=.\Source\Dumpstr.h
# End Source File
# Begin Source File

SOURCE=.\Source\Ftkhead.h
# End Source File
# Begin Source File

SOURCE=.\Source\Ftktail.h
# End Source File
# Begin Source File

SOURCE=.\Source\Kdata3ds.c
# End Source File
# Begin Source File

SOURCE=.\Source\Kdata3ds.h
# End Source File
# Begin Source File

SOURCE=.\Source\Kfutils.c
# End Source File
# Begin Source File

SOURCE=.\Source\Kfutils.h
# End Source File
# Begin Source File

SOURCE=.\Source\Makeswap.c
# End Source File
# Begin Source File

SOURCE=.\Source\Mdata3ds.c
# End Source File
# Begin Source File

SOURCE=.\Source\Mdata3ds.h
# End Source File
# Begin Source File

SOURCE=.\Source\Seekchil.c
# End Source File
# Begin Source File

SOURCE=.\Source\Seekchil.h
# End Source File
# Begin Source File

SOURCE=.\Source\Smartall.c
# End Source File
# Begin Source File

SOURCE=.\Source\Smartall.h
# End Source File
# Begin Source File

SOURCE=.\Source\Strcmpi.c
# End Source File
# Begin Source File

SOURCE=.\Source\Strcmpi.h
# End Source File
# Begin Source File

SOURCE=.\Source\Strdup.c
# End Source File
# Begin Source File

SOURCE=.\Source\swapbyte.c
# End Source File
# Begin Source File

SOURCE=.\Source\Swapbyte.h
# End Source File
# Begin Source File

SOURCE=.\Source\Xdata.c
# End Source File
# Begin Source File

SOURCE=.\Source\Xdata.h
# End Source File
# End Target
# End Project
