# VM ichida (vm.py yugur buni buyruq.ps1 sifatida beradi): o'rnatuvchini jimgina yuritadi,
# o'rnatilganini ko'rsatadi, keyin tsf_sinov 64 va 32-bitli jarayonda.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$setup = Get-ChildItem .\ozbek-klaviaturasi-*.exe | Select-Object -First 1
"== O'rnatish: $($setup.Name)"
$p = Start-Process -FilePath $setup.FullName -ArgumentList '/S' -Wait -PassThru
"o'rnatuvchi chiqish kodi: $($p.ExitCode)"
Get-ChildItem 'C:\Program Files\YangiKlaviatura' | ForEach-Object { "  $($_.Name)" }
"== Til ro'yxati"
Get-WinUserLanguageList | ForEach-Object { "  $($_.LanguageTag): $($_.InputMethodTips -join ', ')" }
foreach ($b in 64, 32) {
    "== tsf_sinov ($b-bit)"
    & ".\tsf_sinov$b.exe"
    "chiqish kodi: $LASTEXITCODE"
}
