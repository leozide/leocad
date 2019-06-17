;LeoCAD Setup Script
;Written by Leonardo Zide (based on one of the example scripts)

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "LeoCAD"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\LeoCAD"
  
  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\BT Software\LeoCAD" "InstallPath"

  Icon "setup.ico"

  !define MUI_ICON "setup.ico"
  !define MUI_UNICON "setup.ico"

  SetCompressor /SOLID lzma

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "LeoCAD" SecLeoCAD

  SetOutPath "$INSTDIR"

  File /r "appdir\*"

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
  ;WriteRegStr HKCU "Software\BT Software\LeoCAD" "InstallPath" $INSTDIR
  
  ; Overwrite old Pieces Library path.
  ;WriteRegStr HKCU "Software\BT Software\LeoCAD\Settings" "PiecesLibrary" $INSTDIR

  CreateShortCut "$SMPROGRAMS\LeoCAD.lnk" "$INSTDIR\LeoCAD.exe"

  ;Create uninstaller
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "DisplayName" "LeoCAD"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "URLUpdateInfo" "http://www.leocad.org"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "URLInfoAbout" "http://www.leocad.org"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "NoRepair" 1
  WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

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
  DeleteRegKey HKCU "Software\BT Software\LeoCAD\InstallPath"
  DeleteRegKey /ifempty HKCU "Software\BT Software\LeoCAD"

SectionEnd
