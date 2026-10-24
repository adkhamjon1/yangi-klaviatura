# Sinov VM'idagi agent: foydalanuvchi sessiyasida yuradi (oddiy huquq), CD'dagi buyruq.ps1 ni
# bir marta bajaradi va chiqishini COM1 ga yozadi. Yangi buyruq — yangi id.txt bilan almashtirilgan CD.
$com = New-Object System.IO.Ports.SerialPort COM1, 115200
$com.Encoding = [System.Text.Encoding]::UTF8
$com.NewLine = "`n"
# Kirishda port hali tayyor bo'lmasligi mumkin: bir daqiqa qayta uriniladi, xato C:\agent\xato.txt ga
for ($i = 0; -not $com.IsOpen; $i++) {
    try { $com.Open() } catch {
        "$(Get-Date -Format s) $_" | Out-File -Append C:\agent\xato.txt
        if ($i -ge 30) { exit 1 }
        Start-Sleep -Seconds 2
    }
}
function Yoz([string]$s) { $com.WriteLine($s) }

Yoz "AGENT TAYYOR $([Environment]::OSVersion.VersionString)"
$oxirgi = ''
while ($true) {
    foreach ($d in 'D', 'E', 'F', 'G', 'H') {
        $buyruq = "$($d):\buyruq.ps1"
        if (-not (Test-Path $buyruq)) { continue }
        $id = Get-Content "$($d):\id.txt" -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $id -or $id -eq $oxirgi) { continue }
        $oxirgi = $id
        Yoz "BOSHLANDI $id"
        try {
            Set-Location "$($d):\"
            & $buyruq 2>&1 | ForEach-Object { Yoz "$_" }
        } catch {
            Yoz "XATO $_"
        }
        Yoz "TUGADI $id"
    }
    Start-Sleep -Seconds 3
}
