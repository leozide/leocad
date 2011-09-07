@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by LEOCAD.HPJ. >"hlp\LeoCAD.hm"
echo. >>"hlp\LeoCAD.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\LeoCAD.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\LeoCAD.hm"
echo. >>"hlp\LeoCAD.hm"
echo // Prompts (IDP_*) >>"hlp\LeoCAD.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\LeoCAD.hm"
echo. >>"hlp\LeoCAD.hm"
echo // Resources (IDR_*) >>"hlp\LeoCAD.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\LeoCAD.hm"
echo. >>"hlp\LeoCAD.hm"
echo // Dialogs (IDD_*) >>"hlp\LeoCAD.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\LeoCAD.hm"
echo. >>"hlp\LeoCAD.hm"
echo // Frame Controls (IDW_*) >>"hlp\LeoCAD.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\LeoCAD.hm"
REM -- Make help for Project LEOCAD

	
echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\LeoCAD.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\LeoCAD.hlp" goto :Error
if not exist "hlp\LeoCAD.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\LeoCAD.hlp" Debug
if exist Debug\nul copy "hlp\LeoCAD.cnt" Debug
if exist Release\nul copy "hlp\LeoCAD.hlp" Release
if exist Release\nul copy "hlp\LeoCAD.cnt" Release
echo.
goto :done

:Error
echo hlp\LeoCAD.hpj(1) : error: Problem encountered creating help file

:done
echo.
