!include "MUI2.nsh"

; The name of the installer
Name "LeoCAD"

; The file to write
;OutFile "leocad-setup.exe"

; Request application privileges for Windows Vista
;RequestExecutionLevel admin

; Build Unicode installer
Unicode True

; The default installation directory
InstallDir $PROGRAMFILES64\LeoCAD

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\LeoCAD Software\LeoCAD" "InstallFolder"
  
;  Icon "setup.ico"

;  !define MUI_ICON "setup.ico"
;  !define MUI_UNICON "setup.ico"

SetCompressor /SOLID lzma


;Interface Settings

!define MUI_HEADERIMAGE
!define MUI_ABORTWARNING


;Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\LeoCAD.exe"
!insertmacro MUI_PAGE_FINISH
  
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
  
!insertmacro MUI_LANGUAGE "English"


;Installer Sections

Section "Application Files" SecLeoCAD

  SectionIn RO
  SetOutPath "$INSTDIR"

  File /r /x library.bin "appdir\*.*"

  ;Register file extension
  WriteRegStr HKCR ".lcd" "" "LeoCAD.Project"
  WriteRegStr HKCR ".lcd\ShellNew" "NullFile" ""
  WriteRegStr HKCR "LeoCAD.Project" "" "LeoCAD Project"
  WriteRegStr HKCR "LeoCAD.Project\DefaultIcon" "" "$INSTDIR\LeoCAD.exe,0"
  WriteRegStr HKCR "LeoCAD.Project\shell" "" "open"
  WriteRegStr HKCR "LeoCAD.Project\shell\open\command" "" '"$INSTDIR\LeoCAD.exe" "%1"'
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'

  IfFileExists "$INSTDIR\vc_redist.x64.exe" VcRedist64Exists PastVcRedist64Check
  VcRedist64Exists:
    ExecWait '"$INSTDIR\vc_redist.x64.exe"  /quiet /norestart'
  PastVcRedist64Check:

  ;Store installation folder
  ;WriteRegStr HKLM "Software\LeoCAD Software\LeoCAD" "InstallFolder" $INSTDIR
  
  CreateShortCut "$SMPROGRAMS\LeoCAD.lnk" "$INSTDIR\LeoCAD.exe"

  ;Create uninstaller
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "DisplayName" "LeoCAD"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "Publisher" "LeoCAD.org"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "DisplayIcon" '"$INSTDIR\LeoCAD.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "URLUpdateInfo" "https://www.leocad.org"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "URLInfoAbout" "https://www.leocad.org"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "NoRepair" 1

  !include "FileFunc.nsh"
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "EstimatedSize" "$0"

  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

Section "Parts Library" SecLibrary

  SetOutPath "$INSTDIR"

  File "appdir\library.bin"

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecLeoCAD ${LANG_ENGLISH} "Application Files (required)"
  LangString DESC_SecLibrary ${LANG_ENGLISH} "Library of parts that represent those produced by the LEGO company and created by the LDraw community"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecLeoCAD} $(DESC_SecLeoCAD)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecLibrary} $(DESC_SecLibrary)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$SMPROGRAMS\LeoCAD.lnk"
  Delete "$INSTDIR\Uninstall.exe"

  Delete "$INSTDIR\LeoCAD.exe"
  Delete "$INSTDIR\LeoCAD.hlp"
  Delete "$INSTDIR\LeoCAD.cnt"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\library.bin"
  Delete "$INSTDIR\povconsole32-sse2.exe"
  Delete "$INSTDIR\vc_redist.x64.exe"

  RMDir "$INSTDIR"

  DeleteRegKey HKCR ".lcd"
  DeleteRegKey HKCR "LeoCAD.Project"
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD"
  DeleteRegKey HKLM "Software\LeoCAD Software\LeoCAD\InstallFolder"
  DeleteRegKey /ifempty HKCU "Software\LeoCAD Software\LeoCAD"

SectionEnd
