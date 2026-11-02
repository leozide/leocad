!include "MUI2.nsh"
!include "LogicLib.nsh"

!define LDRAW_PROGID "LeoCAD.LDraw"
!define FILE_ASSOCIATIONS_KEY "Software\LeoCAD Software\LeoCAD\FileAssociations"

!macro RegisterLDrawExtension Extension
  ReadRegStr $0 HKLM "Software\Classes\${Extension}" ""
  ; Preserve the original association when reinstalling over LeoCAD.
  ${If} $0 != "${LDRAW_PROGID}"
    WriteRegStr HKLM "${FILE_ASSOCIATIONS_KEY}" "${Extension}" "$0"
  ${EndIf}
  WriteRegStr HKLM "Software\Classes\${Extension}" "" "${LDRAW_PROGID}"
  WriteRegStr HKLM "Software\Classes\${Extension}\OpenWithProgids" "${LDRAW_PROGID}" ""
!macroend

!macro UnregisterLDrawExtension Extension
  ReadRegStr $0 HKLM "Software\Classes\${Extension}" ""
  ; Leave a different application's association in place.
  ${If} $0 == "${LDRAW_PROGID}"
    ReadRegStr $1 HKLM "${FILE_ASSOCIATIONS_KEY}" "${Extension}"
    ${If} $1 == ""
      DeleteRegValue HKLM "Software\Classes\${Extension}" ""
    ${Else}
      WriteRegStr HKLM "Software\Classes\${Extension}" "" "$1"
    ${EndIf}
  ${EndIf}
  DeleteRegValue HKLM "Software\Classes\${Extension}\OpenWithProgids" "${LDRAW_PROGID}"
  DeleteRegKey /ifempty HKLM "Software\Classes\${Extension}\OpenWithProgids"
  DeleteRegKey /ifempty HKLM "Software\Classes\${Extension}"
!macroend

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

  IfFileExists "$INSTDIR\vc_redist.x64.exe" VcRedist64Exists PastVcRedist64Check
  VcRedist64Exists:
    ExecWait '"$INSTDIR\vc_redist.x64.exe"  /quiet /norestart'
  PastVcRedist64Check:

  ;Store installation folder
  ;WriteRegStr HKLM "Software\LeoCAD Software\LeoCAD" "InstallFolder" $INSTDIR
  
  CreateShortCut "$SMPROGRAMS\LeoCAD.lnk" "$INSTDIR\LeoCAD.exe"

  ;Create uninstaller
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "DisplayName" "LeoCAD"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD" "DisplayVersion" $%VERSION%
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

Section /o "Associate .ldr and .mpd files" SecFileAssociations

  WriteRegStr HKLM "Software\Classes\${LDRAW_PROGID}" "" "LDraw Model"
  WriteRegStr HKLM "Software\Classes\${LDRAW_PROGID}\DefaultIcon" "" '$\"$INSTDIR\LeoCAD.exe$\",0'
  WriteRegStr HKLM "Software\Classes\${LDRAW_PROGID}\shell" "" "open"
  WriteRegStr HKLM "Software\Classes\${LDRAW_PROGID}\shell\open\command" "" '$\"$INSTDIR\LeoCAD.exe$\" $\"%1$\"'
  !insertmacro RegisterLDrawExtension ".ldr"
  !insertmacro RegisterLDrawExtension ".mpd"
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'

SectionEnd

Section "Parts Library" SecLibrary

  SetOutPath "$INSTDIR"

  File "appdir\library.bin"

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecLeoCAD ${LANG_ENGLISH} "Application Files (required)"
  LangString DESC_SecFileAssociations ${LANG_ENGLISH} "Associate LDraw models (.ldr and .mpd) with LeoCAD."
  LangString DESC_SecLibrary ${LANG_ENGLISH} "Library of parts that represent those produced by the LEGO company and created by the LDraw community"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecLeoCAD} $(DESC_SecLeoCAD)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecFileAssociations} $(DESC_SecFileAssociations)
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
  ; An older uninstaller must not remove a newer installation's registration.
  ReadRegStr $0 HKLM "Software\Classes\${LDRAW_PROGID}\shell\open\command" ""
  ${If} $0 == '$\"$INSTDIR\LeoCAD.exe$\" $\"%1$\"'
    !insertmacro UnregisterLDrawExtension ".ldr"
    !insertmacro UnregisterLDrawExtension ".mpd"
    DeleteRegKey HKLM "Software\Classes\${LDRAW_PROGID}"
    DeleteRegKey HKLM "${FILE_ASSOCIATIONS_KEY}"
  ${EndIf}
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\LeoCAD"
  DeleteRegKey HKLM "Software\LeoCAD Software\LeoCAD\InstallFolder"
  DeleteRegKey /ifempty HKCU "Software\LeoCAD Software\LeoCAD"

SectionEnd
