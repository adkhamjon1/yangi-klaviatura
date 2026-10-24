#!/usr/bin/env node
// Sxema fayllarini xarita.mjs dan yasaydi.
//   node qur.mjs           — fayllarni yozadi
//   node qur.mjs --check   — fayllar xaritaga mosligini tekshiradi (o'zgartirmaydi)
import { readFileSync, writeFileSync, mkdirSync, existsSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { NOM, QISQA, LINUX_ID, XARITA } from './xarita.mjs';

const ILDIZ = dirname(fileURLToPath(import.meta.url));
const TEKSHIR = process.argv.includes('--check');

for (const [k, b] of Object.entries(XARITA)) {
  if (!/^(A[B-E]\d\d|TLDE|BKSL)$/.test(k)) throw new Error(`xarita.mjs: noma'lum tugma ${k}`);
  if (b.length !== 4) throw new Error(`xarita.mjs: ${k} da 4 ta belgi bo'lishi kerak`);
}

const harfmi = (c) => c.toLowerCase() !== c.toUpperCase();

// ---------- Linux (XKB) ----------

const KEYSYM = {
  'ş': 'scedilla', 'Ş': 'Scedilla', 'ç': 'ccedilla', 'Ç': 'Ccedilla',
  'ö': 'odiaeresis', 'Ö': 'Odiaeresis', 'ğ': 'gbreve', 'Ğ': 'Gbreve',
  '[': 'bracketleft', ']': 'bracketright', '{': 'braceleft', '}': 'braceright',
  ';': 'semicolon', ':': 'colon', "'": 'apostrophe', '"': 'quotedbl',
  '\\': 'backslash', '|': 'bar', ',': 'comma', '.': 'period', '/': 'slash',
  '<': 'less', '>': 'greater', '?': 'question', '-': 'minus', '_': 'underscore',
  '=': 'equal', '+': 'plus', '`': 'grave', '~': 'asciitilde',
};
const keysym = (c) => {
  if (/^[A-Za-z0-9]$/.test(c)) return c;
  return KEYSYM[c] ?? `U${c.codePointAt(0).toString(16).toUpperCase().padStart(4, '0')}`;
};

function linuxSymbols() {
  const qatorlar = Object.entries(XARITA).map(([k, b]) => {
    const tur = harfmi(b[0]) ? 'FOUR_LEVEL_SEMIALPHABETIC' : 'FOUR_LEVEL';
    return `    key <${k}> { type[Group1] = "${tur}", [ ${b.map(keysym).join(', ')} ] };`;
  });
  return `// ${NOM} — o'zbek lotin alifbosi (2026): Ö Ğ Ş Ç.
// Bu fayl qur.mjs yasagan, qo'lda tahrirlanmaydi — xarita.mjs ni o'zgartiring.
default partial alphanumeric_keys
xkb_symbols "basic" {
    include "us(basic)"
    name[Group1] = "${NOM}";

${qatorlar.join('\n')}

    include "level3(ralt_switch)"
};
`;
}

function linuxRegistry() {
  return `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE xkbConfigRegistry SYSTEM "xkb.dtd">
<!-- ozbek-klaviatura: qur.mjs yasagan -->
<xkbConfigRegistry version="1.1">
  <layoutList>
    <layout>
      <configItem>
        <name>${LINUX_ID}</name>
        <shortDescription>${QISQA}</shortDescription>
        <description>${NOM}</description>
        <countryList>
          <iso3166Id>UZ</iso3166Id>
        </countryList>
        <languageList>
          <iso639Id>uzb</iso639Id>
        </languageList>
      </configItem>
    </layout>
  </layoutList>
</xkbConfigRegistry>
`;
}

// ---------- yozish / tekshirish ----------

// IBus dvigateli uchun: yangi harf → tugmaning asl belgisi
function ibusXarita() {
  const juftlar = Object.values(XARITA).flatMap((b) => [[b[0], b[2]], [b[1], b[3]]]);
  return `# qur.mjs yasagan — xarita.mjs ni o'zgartiring.
# Yangi harf → tugmaning asl belgisi («Yangi harflar tugmalarda» o'chiq bo'lganda).
TUGMA_ASLI = ${JSON.stringify(Object.fromEntries(juftlar), null, 4).replace(/\n}$/, ',\n}')}
`;
}

const FAYLLAR = [
  [`linux/symbols/${LINUX_ID}`, linuxSymbols],
  ['linux/rules/evdev.xml', linuxRegistry],
  ['linux/ibus/xarita.py', ibusXarita],
];

let eskirgan = 0;
for (const [yol, yasa] of FAYLLAR) {
  const toliq = join(ILDIZ, yol);
  const yangi = Buffer.from(yasa());
  const bor = existsSync(toliq) ? readFileSync(toliq) : null;
  if (bor && bor.equals(yangi)) continue;
  if (TEKSHIR) {
    console.error(`✗ ${yol} xaritaga mos emas — node qur.mjs`);
    eskirgan++;
    continue;
  }
  mkdirSync(dirname(toliq), { recursive: true });
  writeFileSync(toliq, yangi);
  console.log(`✓ ${yol}`);
}
if (TEKSHIR) {
  if (eskirgan) process.exit(1);
  console.log(`✓ ${FAYLLAR.length} fayl xaritaga mos`);
}
