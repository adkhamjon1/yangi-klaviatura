; Özbek klaviaturasi — Windows o'rnatuvchisi (NSIS 3, Unicode).
; Yig'ish: bash windows/qur.sh  →  windows/build/ozbek-klaviaturasi-<versiya>.exe
;
; O'rnatadi: ozbek64.dll va ozbek32.dll (TSF matn xizmati, 64 va 32-bitli ilovalar uchun),
; Sozlamalar.exe, Start menyusidagi yorliq va «Dasturlar» ro'yxatidagi olib tashlash yozuvi.
; Uchala rejim (ÖZ, O'Z, ЎЗ) joriy foydalanuvchining til ro'yxatiga qo'shiladi.

Unicode true
ManifestDPIAware true
ManifestSupportedOS all
SetCompressor /SOLID lzma
RequestExecutionLevel admin

!ifndef VERSIYA
  !error "VERSIYA berilmagan: makensis -DVERSIYA=1.0.0"
!endif
!ifndef BUILD
  !define BUILD "../build"
!endif

!define NOM "Özbek klaviaturasi"
!define KALIT "YangiKlaviatura"
!define MUALLIF "adkhamjon1"
!define SAYT "https://github.com/adkhamjon1/yangi-klaviatura"
!define OLIB "Software\Microsoft\Windows\CurrentVersion\Uninstall\${KALIT}"
; tsf/umumiy.hpp dagi CLSID va profillar; «til:{CLSID}{profil}», ";" bilan
!define CLSID "{1B5150A6-39E7-42B1-B771-BBF2A214010B}"
!define TIPLAR "0443:${CLSID}{1090CF6A-22FC-486D-A2C4-566BFB57567F};0443:${CLSID}{45A97DD2-5AD2-40C8-9E36-A64C2860C9F5};0443:${CLSID}{064D6F63-6C7F-4EA4-AF76-E4C8FF0B2165}"
!define OLIB_TASHLA 1  ; InstallLayoutOrTip: ILOT_UNINSTALL

Name "${NOM}"
OutFile "${BUILD}/ozbek-klaviaturasi-${VERSIYA}.exe"
InstallDir "$PROGRAMFILES64\${KALIT}"
BrandingText "${NOM} ${VERSIYA}"
ShowInstDetails hide
ShowUninstDetails hide

!include MUI2.nsh
!include LogicLib.nsh
!include WinVer.nsh
!include x64.nsh
!include FileFunc.nsh

!define MUI_ICON "../tsf/ikonka/ilova.ico"
!define MUI_UNICON "../tsf/ikonka/ilova.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${BUILD}/yon.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${BUILD}/yon.bmp"
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING

!define MUI_WELCOMEPAGE_TEXT "$(^NameDA) kompyuterga o'rnatiladi.$\r$\n$\r$\nTil ro'yxatiga (Win+Probel) uchta rejim qo'shiladi:$\r$\n$\r$\nÖZ — yangi alifbo: ş, ç, ö, ğ harflari W, C, [ va ] tugmalarida; sh, ch, o', g' ham o'zi o'giriladi.$\r$\nO'Z — eski lotin yozuvi.$\r$\nЎЗ — kirillcha yozilgan so'z yangi lotinga o'giriladi.$\r$\n$\r$\n$_CLICK"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "Rejim Win+Probel bilan tanlanadi.$\r$\n$\r$\nOchiq turgan dasturda rejim ishlamasa, o'sha dasturni yopib, qayta oching."
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Sozlamalarni ochish"
!define MUI_FINISHPAGE_RUN_FUNCTION Sozlamalar
!insertmacro MUI_PAGE_FINISH

!define MUI_PAGE_HEADER_SUBTEXT "Sozlamalar va o'rganilgan so'zlar saqlanib qoladi."
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGEEX "." "Ozbek"

VIProductVersion "${VERSIYA}.0"
VIAddVersionKey /LANG=${LANG_OZBEK} "ProductName" "${NOM}"
VIAddVersionKey /LANG=${LANG_OZBEK} "FileDescription" "${NOM} o'rnatuvchisi"
VIAddVersionKey /LANG=${LANG_OZBEK} "CompanyName" "${MUALLIF}"
VIAddVersionKey /LANG=${LANG_OZBEK} "LegalCopyright" "© ${MUALLIF}"
VIAddVersionKey /LANG=${LANG_OZBEK} "FileVersion" "${VERSIYA}"
VIAddVersionKey /LANG=${LANG_OZBEK} "ProductVersion" "${VERSIYA}"

; Ishlab turgan dasturlar eski DLL'ni ushlab turadi: yangilashda u ustidan yozilmaydi,
; nomi o'zgartiriladi va kompyuter qayta yoqilganda o'chiriladi.
!macro Boshat FAYL
  ${If} ${FileExists} "$INSTDIR\${FAYL}"
    System::Call 'kernel32::GetTickCount() i .r9'
    Rename "$INSTDIR\${FAYL}" "$INSTDIR\${FAYL}.$9.eski"
    Delete /REBOOTOK "$INSTDIR\${FAYL}.$9.eski"
  ${EndIf}
!macroend

; regsvr32 ilova bitligiga mos olinadi: 64-bitli DLL — System32 dagisi, 32-bitli — SysWOW64 dagisi.
; Avval 32, keyin 64: profil ikonkasi 64-bitli DLL yo'lidan olinsin.
!macro Royxat UN
  ${If} ${RunningX64}
    ExecWait '"$WINDIR\SysWOW64\regsvr32.exe" /s ${UN} "$INSTDIR\ozbek32.dll"' $0
    ${DisableX64FSRedirection}
    ExecWait '"$SYSDIR\regsvr32.exe" /s ${UN} "$INSTDIR\ozbek64.dll"' $1
    ${EnableX64FSRedirection}
  ${Else}
    ExecWait '"$SYSDIR\regsvr32.exe" /s ${UN} "$INSTDIR\ozbek32.dll"' $0
    StrCpy $1 0
  ${EndIf}
!macroend

Function .onInit
  ${IfNot} ${AtLeastWin10}
    MessageBox MB_ICONSTOP|MB_OK "${NOM} Windows 10 yoki 11 da ishlaydi."
    Abort
  ${EndIf}
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
FunctionEnd

Function un.onInit
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
FunctionEnd

; Sozlamalar oddiy huquq bilan ochiladi (o'rnatuvchi administrator huquqida turibdi)
Function Sozlamalar
  Exec '"$WINDIR\explorer.exe" "$INSTDIR\Sozlamalar.exe"'
FunctionEnd

Section
  SetOutPath "$INSTDIR"
  !insertmacro Boshat "ozbek32.dll"
  !insertmacro Boshat "ozbek64.dll"
  !insertmacro Boshat "Sozlamalar.exe"
  File "${BUILD}/ozbek32.dll"
  ${If} ${RunningX64}
    File "${BUILD}/ozbek64.dll"
  ${EndIf}
  File "${BUILD}/Sozlamalar.exe"
  WriteUninstaller "$INSTDIR\olib-tashlash.exe"

  !insertmacro Royxat ""
  ; Wine'da RegisterProfile yozilmagan (E_NOTIMPL): -DSINOV_WINE qolgan qadamlarni sinash uchun xatoni o'tkazadi
  !ifndef SINOV_WINE
  ${If} $0 != 0
  ${OrIf} $1 != 0
    StrCpy $2 "$0, $1"
    !insertmacro Royxat "/u"
    MessageBox MB_ICONSTOP|MB_OK "Matn xizmati ro'yxatga olinmadi (regsvr32: $2). Antivirus to'sgan bo'lishi mumkin." /SD IDOK
    Abort
  ${EndIf}
  !endif

  StrCpy $2 "${TIPLAR}"
  System::Call 'input::InstallLayoutOrTip(w r2, i 0) i .r0'
  ${If} $0 == 0
    MessageBox MB_ICONEXCLAMATION|MB_OK "Rejimlar til ro'yxatiga qo'shilmadi. Sozlamalar oynasidagi «Yoqish» tugmasi ularni qo'shadi." /SD IDOK
  ${EndIf}

  SetShellVarContext all
  CreateShortCut "$SMPROGRAMS\${NOM}.lnk" "$INSTDIR\Sozlamalar.exe"

  WriteRegStr HKLM "${OLIB}" "DisplayName" "${NOM}"
  WriteRegStr HKLM "${OLIB}" "DisplayVersion" "${VERSIYA}"
  WriteRegStr HKLM "${OLIB}" "Publisher" "${MUALLIF}"
  WriteRegStr HKLM "${OLIB}" "URLInfoAbout" "${SAYT}"
  WriteRegStr HKLM "${OLIB}" "DisplayIcon" "$INSTDIR\Sozlamalar.exe"
  WriteRegStr HKLM "${OLIB}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${OLIB}" "UninstallString" '"$INSTDIR\olib-tashlash.exe"'
  WriteRegStr HKLM "${OLIB}" "QuietUninstallString" '"$INSTDIR\olib-tashlash.exe" /S'
  WriteRegDWORD HKLM "${OLIB}" "NoModify" 1
  WriteRegDWORD HKLM "${OLIB}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  WriteRegDWORD HKLM "${OLIB}" "EstimatedSize" $0
SectionEnd

; Foydalanuvchi sozlamalari (HKCU\Software\YangiKlaviatura) va lug'ati (%APPDATA%\YangiKlaviatura)
; qoladi: qayta o'rnatilganda o'rganilgan so'zlar yo'qolmaydi.
Section "Uninstall"
  StrCpy $2 "${TIPLAR}"
  System::Call 'input::InstallLayoutOrTip(w r2, i ${OLIB_TASHLA}) i .r0'
  !insertmacro Royxat "/u"

  Delete /REBOOTOK "$INSTDIR\ozbek64.dll"
  Delete /REBOOTOK "$INSTDIR\ozbek32.dll"
  Delete /REBOOTOK "$INSTDIR\Sozlamalar.exe"
  Delete /REBOOTOK "$INSTDIR\*.eski"
  Delete "$INSTDIR\olib-tashlash.exe"
  RMDir /REBOOTOK "$INSTDIR"

  SetShellVarContext all
  Delete "$SMPROGRAMS\${NOM}.lnk"
  DeleteRegKey HKLM "${OLIB}"
SectionEnd
