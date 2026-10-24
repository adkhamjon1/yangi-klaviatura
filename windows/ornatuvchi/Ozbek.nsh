;Language: Ozbek (1091) — Özbek klaviaturasi o'rnatuvchisi uchun.
;NSIS'dagi Uzbek.nsh o'rniga yozilgan; yetishmagan satr English.nsh dan olinadi.

!insertmacro LANGFILE "Ozbek" "Uzbek" "O'zbek" "O'zbek"

!ifdef MUI_WELCOMEPAGE
  ${LangFileString} MUI_TEXT_WELCOME_INFO_TITLE "$(^NameDA) o'rnatuvchisi"
  ${LangFileString} MUI_TEXT_WELCOME_INFO_TEXT "$(^NameDA) kompyuterga o'rnatiladi.$\r$\n$\r$\n$_CLICK"
!endif

!ifdef MUI_UNWELCOMEPAGE
  ${LangFileString} MUI_UNTEXT_WELCOME_INFO_TITLE "$(^NameDA): olib tashlash"
  ${LangFileString} MUI_UNTEXT_WELCOME_INFO_TEXT "$(^NameDA) kompyuterdan olib tashlanadi.$\r$\n$\r$\n$_CLICK"
!endif

!ifdef MUI_LICENSEPAGE
  ${LangFileString} MUI_TEXT_LICENSE_TITLE "Litsenziya shartnomasi"
  ${LangFileString} MUI_TEXT_LICENSE_SUBTITLE "$(^NameDA) o'rnatilishidan oldin shartnoma bilan tanishib chiqing."
  ${LangFileString} MUI_INNERTEXT_LICENSE_BOTTOM "Shartlarga rozi bo'lsangiz, «Roziman» tugmasini bosing. Shartnomasiz $(^NameDA) o'rnatilmaydi."
  ${LangFileString} MUI_INNERTEXT_LICENSE_BOTTOM_CHECKBOX "Shartlarga rozi bo'lsangiz, belgini qo'ying. Shartnomasiz $(^NameDA) o'rnatilmaydi. $_CLICK"
  ${LangFileString} MUI_INNERTEXT_LICENSE_BOTTOM_RADIOBUTTONS "Shartlarga rozi bo'lsangiz, birinchi variantni tanlang. Shartnomasiz $(^NameDA) o'rnatilmaydi. $_CLICK"
!endif

!ifdef MUI_UNLICENSEPAGE
  ${LangFileString} MUI_UNTEXT_LICENSE_TITLE "Litsenziya shartnomasi"
  ${LangFileString} MUI_UNTEXT_LICENSE_SUBTITLE "$(^NameDA) olib tashlanishidan oldin shartnoma bilan tanishib chiqing."
  ${LangFileString} MUI_UNINNERTEXT_LICENSE_BOTTOM "Shartlarga rozi bo'lsangiz, «Roziman» tugmasini bosing."
  ${LangFileString} MUI_UNINNERTEXT_LICENSE_BOTTOM_CHECKBOX "Shartlarga rozi bo'lsangiz, belgini qo'ying. $_CLICK"
  ${LangFileString} MUI_UNINNERTEXT_LICENSE_BOTTOM_RADIOBUTTONS "Shartlarga rozi bo'lsangiz, birinchi variantni tanlang. $_CLICK"
!endif

!ifdef MUI_LICENSEPAGE | MUI_UNLICENSEPAGE
  ${LangFileString} MUI_INNERTEXT_LICENSE_TOP "Matnni PageUp va PageDown tugmalari bilan aylantirish mumkin."
!endif

!ifdef MUI_COMPONENTSPAGE
  ${LangFileString} MUI_TEXT_COMPONENTS_TITLE "Tarkibiy qismlar"
  ${LangFileString} MUI_TEXT_COMPONENTS_SUBTITLE "O'rnatiladigan qismlarni tanlang."
!endif

!ifdef MUI_UNCOMPONENTSPAGE
  ${LangFileString} MUI_UNTEXT_COMPONENTS_TITLE "Tarkibiy qismlar"
  ${LangFileString} MUI_UNTEXT_COMPONENTS_SUBTITLE "Olib tashlanadigan qismlarni tanlang."
!endif

!ifdef MUI_COMPONENTSPAGE | MUI_UNCOMPONENTSPAGE
  ${LangFileString} MUI_INNERTEXT_COMPONENTS_DESCRIPTION_TITLE "Izoh"
  !ifndef NSIS_CONFIG_COMPONENTPAGE_ALTERNATIVE
    ${LangFileString} MUI_INNERTEXT_COMPONENTS_DESCRIPTION_INFO "Izohini ko'rish uchun sichqonchani qism ustiga olib boring."
  !else
    ${LangFileString} MUI_INNERTEXT_COMPONENTS_DESCRIPTION_INFO "Izohini ko'rish uchun qismni tanlang."
  !endif
!endif

!ifdef MUI_DIRECTORYPAGE
  ${LangFileString} MUI_TEXT_DIRECTORY_TITLE "O'rnatish papkasi"
  ${LangFileString} MUI_TEXT_DIRECTORY_SUBTITLE "$(^NameDA) o'rnatiladigan papkani tanlang."
!endif

!ifdef MUI_UNDIRECTORYPAGE
  ${LangFileString} MUI_UNTEXT_DIRECTORY_TITLE "Papka"
  ${LangFileString} MUI_UNTEXT_DIRECTORY_SUBTITLE "$(^NameDA) olib tashlanadigan papkani tanlang."
!endif

!ifdef MUI_INSTFILESPAGE
  ${LangFileString} MUI_TEXT_INSTALLING_TITLE "O'rnatilmoqda"
  ${LangFileString} MUI_TEXT_INSTALLING_SUBTITLE "$(^NameDA) o'rnatilguncha kuting."
  ${LangFileString} MUI_TEXT_FINISH_TITLE "O'rnatildi"
  ${LangFileString} MUI_TEXT_FINISH_SUBTITLE "O'rnatish muvaffaqiyatli tugadi."
  ${LangFileString} MUI_TEXT_ABORT_TITLE "O'rnatish to'xtatildi"
  ${LangFileString} MUI_TEXT_ABORT_SUBTITLE "O'rnatish oxiriga yetmadi."
!endif

!ifdef MUI_UNINSTFILESPAGE
  ${LangFileString} MUI_UNTEXT_UNINSTALLING_TITLE "Olib tashlanmoqda"
  ${LangFileString} MUI_UNTEXT_UNINSTALLING_SUBTITLE "$(^NameDA) olib tashlanguncha kuting."
  ${LangFileString} MUI_UNTEXT_FINISH_TITLE "Olib tashlandi"
  ${LangFileString} MUI_UNTEXT_FINISH_SUBTITLE "Olib tashlash muvaffaqiyatli tugadi."
  ${LangFileString} MUI_UNTEXT_ABORT_TITLE "Olib tashlash to'xtatildi"
  ${LangFileString} MUI_UNTEXT_ABORT_SUBTITLE "Olib tashlash oxiriga yetmadi."
!endif

!ifdef MUI_FINISHPAGE
  ${LangFileString} MUI_TEXT_FINISH_INFO_TITLE "$(^NameDA) o'rnatildi"
  ${LangFileString} MUI_TEXT_FINISH_INFO_TEXT "$(^NameDA) kompyuterga o'rnatildi.$\r$\n$\r$\nO'rnatuvchini yopish uchun «Tayyor» tugmasini bosing."
  ${LangFileString} MUI_TEXT_FINISH_INFO_REBOOT "O'rnatishni yakunlash uchun kompyuter qayta yoqilishi kerak. Hozir qayta yoqilsinmi?"
!endif

!ifdef MUI_UNFINISHPAGE
  ${LangFileString} MUI_UNTEXT_FINISH_INFO_TITLE "$(^NameDA) olib tashlandi"
  ${LangFileString} MUI_UNTEXT_FINISH_INFO_TEXT "$(^NameDA) kompyuterdan olib tashlandi.$\r$\n$\r$\nYopish uchun «Tayyor» tugmasini bosing."
  ${LangFileString} MUI_UNTEXT_FINISH_INFO_REBOOT "Olib tashlashni yakunlash uchun kompyuter qayta yoqilishi kerak. Hozir qayta yoqilsinmi?"
!endif

!ifdef MUI_FINISHPAGE | MUI_UNFINISHPAGE
  ${LangFileString} MUI_TEXT_FINISH_REBOOTNOW "Hozir qayta yoqish"
  ${LangFileString} MUI_TEXT_FINISH_REBOOTLATER "Keyinroq o'zim qayta yoqaman"
  ${LangFileString} MUI_TEXT_FINISH_RUN "Dasturni ishga tushirish"
  ${LangFileString} MUI_TEXT_FINISH_SHOWREADME "Qo'llanmani ochish"
  ${LangFileString} MUI_BUTTONTEXT_FINISH "&Tayyor"
!endif

!ifdef MUI_STARTMENUPAGE
  ${LangFileString} MUI_TEXT_STARTMENU_TITLE "Start menyusidagi papka"
  ${LangFileString} MUI_TEXT_STARTMENU_SUBTITLE "$(^NameDA) yorliqlari turadigan papkani tanlang."
  ${LangFileString} MUI_INNERTEXT_STARTMENU_TOP "Yorliqlar quyidagi papkaga qo'yiladi. Yangi papka uchun nomini yozing."
  ${LangFileString} MUI_INNERTEXT_STARTMENU_CHECKBOX "Yorliq qo'yilmasin"
!endif

!ifdef MUI_UNCONFIRMPAGE
  ${LangFileString} MUI_UNTEXT_CONFIRM_TITLE "$(^NameDA): olib tashlash"
  ${LangFileString} MUI_UNTEXT_CONFIRM_SUBTITLE "$(^NameDA) kompyuterdan olib tashlanadi."
!endif

!ifdef MUI_ABORTWARNING
  ${LangFileString} MUI_TEXT_ABORTWARNING "O'rnatish to'xtatilsinmi?"
!endif

!ifdef MUI_UNABORTWARNING
  ${LangFileString} MUI_UNTEXT_ABORTWARNING "Olib tashlash to'xtatilsinmi?"
!endif

!ifdef MULTIUSER_INSTALLMODEPAGE
  ${LangFileString} MULTIUSER_TEXT_INSTALLMODE_TITLE "Kim uchun o'rnatiladi"
  ${LangFileString} MULTIUSER_TEXT_INSTALLMODE_SUBTITLE "$(^NameDA) kimlar uchun o'rnatilishini tanlang."
  ${LangFileString} MULTIUSER_INNERTEXT_INSTALLMODE_TOP "$(^NameDA) faqat siz uchunmi yoki shu kompyuterning hamma foydalanuvchisi uchunmi? $(^ClickNext)"
  ${LangFileString} MULTIUSER_INNERTEXT_INSTALLMODE_ALLUSERS "Hamma foydalanuvchi uchun"
  ${LangFileString} MULTIUSER_INNERTEXT_INSTALLMODE_CURRENTUSER "Faqat men uchun"
!endif
