#!/usr/bin/env python3
"""Windows sinov VM'i (QEMU/KVM) — klaviaturani haqiqiy Windows'da sinash uchun.

    python3 windows/vm/vm.py ornat <windows.iso>   # bo'sh diskka Windows'ni avtomatik o'rnatadi va yoqadi
    python3 windows/vm/vm.py yoq                   # o'rnatilgan VM'ni yoqadi
    python3 windows/vm/vm.py yugur <papka>         # papkadagi buyruq.ps1 ni VM'da bajaradi, chiqishini beradi
    python3 windows/vm/vm.py surat <rasm.png>      # ekran surati
    python3 windows/vm/vm.py sichqon X Y [chap|ong|yoq]   # ekran nuqtasiga borib bosadi
    python3 windows/vm/vm.py tugma ret|esc|meta_l-r  # QEMU tugma nomlari, birga bosilsa «-» bilan
    python3 windows/vm/vm.py yoz "matn\n"           # ASCII matnni tugmalar bilan yozadi
    python3 windows/vm/vm.py ochir                 # Windows'ni o'chiradi

VM ishchi papkasi KLAV_VM (standart /var/tmp/klav-vm): disk, ISO'lar, COM1 jurnali, QMP soketi.
Tarmoq yo'q (yangilanishlar protsessorni band qilmaydi). Ichida windows/vm/agent.ps1 yuradi:
CD'dagi buyruq.ps1 ni bajaradi va chiqishini COM1 ga yozadi. QEMU systemd xizmati sifatida yuradi —
qizib ketsa `systemctl --user kill -s STOP klav-vm` bilan pauza qilinadi.
"""
import json
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

PAPKA = Path(os.environ.get('KLAV_VM', '/var/tmp/klav-vm'))
BU = Path(__file__).resolve().parent
DISK = PAPKA / 'disk.qcow2'
WIN_ISO = PAPKA / 'windows.iso'
ISH_ISO = PAPKA / 'ish.iso'
COM1 = PAPKA / 'com1.log'
QMP = PAPKA / 'qmp.sock'
XIZMAT = 'klav-vm'


def iso_yasa(manba, chiqish):
    """Papkadan CD obrazi; agent.ps1 va autounattend.xml doim qo'shiladi."""
    with tempfile.TemporaryDirectory(dir=PAPKA) as t:
        t = Path(t)
        if manba:
            shutil.copytree(manba, t, dirs_exist_ok=True)
        shutil.copy(BU / 'agent.ps1', t)
        shutil.copy(BU / 'autounattend.xml', t)
        for ps1 in t.glob('*.ps1'):  # PowerShell 5.1 BOM'siz UTF-8 ni ANSI deb o'qiydi
            b = ps1.read_bytes()
            if not b.startswith(b'\xef\xbb\xbf'):
                ps1.write_bytes(b'\xef\xbb\xbf' + b)
        (t / 'id.txt').write_text(str(time.time_ns()))
        vaqtincha = chiqish.with_suffix('.yangi')
        subprocess.run(['xorriso', '-as', 'mkisofs', '-quiet', '-J', '-R', '-V', 'ISH', '-o', str(vaqtincha), str(t)],
                       check=True)
        vaqtincha.replace(chiqish)
        return (t / 'id.txt').read_text()


def yoq():
    if subprocess.run(['systemctl', '--user', 'is-active', '--quiet', XIZMAT]).returncode == 0:
        print('VM allaqachon yoqiq')
        return
    if not ISH_ISO.exists():
        iso_yasa(None, ISH_ISO)
    QMP.unlink(missing_ok=True)
    qemu = ['qemu-system-x86_64', '-name', XIZMAT, '-machine', 'pc,accel=kvm', '-cpu', 'host', '-smp', '2', '-m', '2048',
            '-drive', f'file={DISK},if=none,id=disk,format=qcow2', '-device', 'ide-hd,drive=disk,bus=ide.0,unit=0',
            '-drive', f'file={WIN_ISO},if=none,id=win,media=cdrom,readonly=on',
            '-device', 'ide-cd,drive=win,bus=ide.1,unit=0,id=wincd',
            '-drive', f'file={ISH_ISO},if=none,id=ish,media=cdrom,readonly=on',
            '-device', 'ide-cd,drive=ish,bus=ide.1,unit=1,id=ishcd',
            '-boot', 'order=cd', '-vga', 'std', '-display', 'none', '-usb', '-device', 'usb-tablet', '-nic', 'none',
            '-rtc', 'base=localtime', '-serial', f'file:{COM1}', '-qmp', f'unix:{QMP},server=on,wait=off']
    # Xotira: 2 GB mehmon + QEMU o'zi; protsessor: bitta yadro (noutbuk qiziydi)
    subprocess.run(['systemd-run', '--user', '--unit', XIZMAT, '--collect', '-p', 'MemoryHigh=2500M', '-p', 'MemoryMax=2700M',
                    '-p', 'MemorySwapMax=256M', '-p', 'CPUQuota=100%', 'nice', '-n', '15', *qemu], check=True)
    for _ in range(50):
        if QMP.exists():
            break
        time.sleep(0.2)
    print('VM yoqildi')


def qmp(*buyruqlar):
    s = socket.socket(socket.AF_UNIX)
    s.connect(str(QMP))
    f = s.makefile('rw')
    json.loads(f.readline())
    natijalar = []
    for b in [{'execute': 'qmp_capabilities'}, *buyruqlar]:
        f.write(json.dumps(b) + '\n')
        f.flush()
        while True:
            j = json.loads(f.readline())
            if 'event' in j:
                continue
            if 'error' in j:
                raise RuntimeError(f"QMP {b['execute']}: {j['error']['desc']}")
            natijalar.append(j.get('return'))
            break
    s.close()
    return natijalar[1:]


def yugur(manba, kutish=900):
    """buyruq.ps1 ni VM'da bajaradi; COM1 dagi chiqishni qaytaradi."""
    if not (Path(manba) / 'buyruq.ps1').exists():
        sys.exit(f'{manba}/buyruq.ps1 yo\'q')
    boshi = COM1.stat().st_size if COM1.exists() else 0
    yangi = PAPKA / 'ish-keyingi.iso'
    id_ = iso_yasa(manba, yangi)
    yangi.replace(ISH_ISO)
    qmp({'execute': 'blockdev-change-medium', 'arguments': {'id': 'ishcd', 'filename': str(ISH_ISO), 'format': 'raw'}})
    oxiri = time.time() + kutish
    while time.time() < oxiri:
        matn = COM1.read_bytes()[boshi:].decode('utf-8', 'replace').replace('\r', '')
        if f'TUGADI {id_}' in matn:
            ichi = matn.split(f'BOSHLANDI {id_}', 1)[-1].split(f'TUGADI {id_}', 1)[0]
            print(ichi.strip('\n'))
            return
        time.sleep(2)
    sys.exit(f'{kutish} s ichida tugamadi; COM1: {COM1}')


def surat(chiqish):
    ppm = PAPKA / 'ekran.ppm'
    qmp({'execute': 'screendump', 'arguments': {'filename': str(ppm)}})
    subprocess.run(['convert', str(ppm), str(chiqish)], check=True)
    print(chiqish)


def sichqon(x, y, tugma='chap'):
    en, boy = ekran_olchami()
    hodisa = [{'type': 'abs', 'data': {'axis': 'x', 'value': round(x * 32767 / (en - 1))}},
              {'type': 'abs', 'data': {'axis': 'y', 'value': round(y * 32767 / (boy - 1))}}]
    buyruqlar = [{'execute': 'input-send-event', 'arguments': {'events': hodisa}}]
    if tugma != 'yoq':
        b = {'chap': 'left', 'ong': 'right'}[tugma]
        for bosildi in (True, False):
            buyruqlar.append({'execute': 'input-send-event',
                              'arguments': {'events': [{'type': 'btn', 'data': {'down': bosildi, 'button': b}}]}})
    qmp(*buyruqlar)


QCODE = {' ': 'spc', '\n': 'ret', '-': 'minus', '=': 'equal', '[': 'bracket_left', ']': 'bracket_right',
         ';': 'semicolon', "'": 'apostrophe', '`': 'grave_accent', '\\': 'backslash', ',': 'comma', '.': 'dot',
         '/': 'slash', '\t': 'tab'}
SHIFT = {'_': '-', '+': '=', '{': '[', '}': ']', ':': ';', '"': "'", '~': '`', '|': '\\', '<': ',', '>': '.', '?': '/',
         '!': '1', '@': '2', '#': '3', '$': '4', '%': '5', '^': '6', '&': '7', '*': '8', '(': '9', ')': '0'}


def yoz(matn):
    """ASCII matnni US tugmalari bilan yozadi (VM ichidagi oynaga)."""
    buyruqlar = []
    for c in matn:
        shift = c in SHIFT or c.isupper()
        asl = SHIFT.get(c, c.lower())
        kod = QCODE.get(asl, asl)
        keys = ([{'type': 'qcode', 'data': 'shift'}] if shift else []) + [{'type': 'qcode', 'data': kod}]
        buyruqlar.append({'execute': 'send-key', 'arguments': {'keys': keys}})
    for i in range(0, len(buyruqlar), 20):
        qmp(*buyruqlar[i:i + 20])
        time.sleep(0.3)


def ekran_olchami():
    ppm = PAPKA / 'olcham.ppm'
    qmp({'execute': 'screendump', 'arguments': {'filename': str(ppm)}})
    with open(ppm, 'rb') as f:
        f.readline()
        qator = f.readline().split()
        while qator and qator[0].startswith(b'#'):
            qator = f.readline().split()
    return int(qator[0]), int(qator[1])


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    PAPKA.mkdir(parents=True, exist_ok=True)
    buyruq, qolgan = sys.argv[1], sys.argv[2:]
    if buyruq == 'ornat':
        if DISK.exists():
            sys.exit(f'{DISK} bor — qayta o\'rnatish uchun avval o\'chiring')
        iso = Path(qolgan[0]).resolve()
        if iso != WIN_ISO:
            WIN_ISO.unlink(missing_ok=True)
            WIN_ISO.symlink_to(iso)
        subprocess.run(['qemu-img', 'create', '-q', '-f', 'qcow2', str(DISK), '40G'], check=True)
        iso_yasa(None, ISH_ISO)
        yoq()
    elif buyruq == 'yoq':
        yoq()
    elif buyruq == 'yugur':
        yugur(qolgan[0], int(qolgan[1]) if len(qolgan) > 1 else 900)
    elif buyruq == 'surat':
        surat(qolgan[0])
    elif buyruq == 'sichqon':
        sichqon(int(qolgan[0]), int(qolgan[1]), qolgan[2] if len(qolgan) > 2 else 'chap')
    elif buyruq == 'yoz':
        yoz(qolgan[0])
    elif buyruq == 'tugma':
        qmp({'execute': 'send-key', 'arguments': {'keys': [{'type': 'qcode', 'data': k} for k in qolgan[0].split('-')]}})
    elif buyruq == 'ochir':
        qmp({'execute': 'system_powerdown'})
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
