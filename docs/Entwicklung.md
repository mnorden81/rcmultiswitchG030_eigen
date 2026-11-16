# MSW-MKan – Hinweise für die Entwicklung

Technische Notizen zum Projekt (früher `README_MKAN.md`). Bauen und Flashen Schritt für Schritt: [README.md](../README.md).

## Geänderte Dateien gegenüber `rcmultiswitchG030`

| Datei | Änderung |
|---|---|
| `msw_mkan.cc` | neu, Basis `msw30.cc`, fest auf `HW_WEACT`, reduziertes Feature-Set |
| `eeprom.h` | dritte Adresse `AdrIndex::MKan`, zweite Konfiguration je `Output` |
| `switch_cb.h` | Auswertung der MKan-Adresse, lädt Konfiguration 1 oder 2 in den Blinker |
| `crsf_cb.h` | Lua-Menü: „MKan Addr“ und Felder „… 2“ je Ausgang |
| `system_g0.cc` | leere `_init()`/`_fini()` |
| `Makefile.G031` | Zieldatei `msw_mkan` |
| übrige | unverändert übernommen |

**Nicht verändert wurde `rc/crsf_2.h`** (CRSF/`SwitchDecoder` im Framework): Adressen werden dort schon über ein Array (`storage::eeprom.addresses`) verglichen, die dritte Adresse passt ohne Änderung hinein.

## Priorität und Einschränkung

- PWM-Modus, Duty und Expo gibt es je Ausgang **einmal**, sie gelten für beide Konfigurationen. Nur Blink-Modus, Intervallzeiten und Blitz-Anzahl sind je Konfiguration getrennt.
- Bei gleichzeitiger Ansteuerung über Switch- und MKan-Adresse hat **MKan Vorrang**: Ist ein Ausgang über MKan an, gilt Konfiguration 2. Ist MKan für diesen Ausgang aus, entscheidet die Switch-Adresse (Konfiguration 1).

## Lua-Budget

Die WeAct-Zielplattform begrenzt das Lua-Menü auf 132 Einträge (`etl::FixedVector<Param_t, 132>` in `crsf_cb.h`). Das Projekt nutzt **123** (Kommentar am Ende von `crsf_cb.h`). Deshalb sind Morse, Pattern, Virtuals, Slave-Kommando, Failsafe und das Operate-Menü abgeschaltet. Wird das Menü zu groß, bricht die Kompilierung absichtlich ab (`fp()` ist nicht definiert, Compile-Time-Check aus dem Framework).

## Toolchain und Ordnerstruktur

Kein Arduino-Sketch, sondern Bare-Metal-C++ (WMuCpp). Gebraucht werden:

1. arm-none-eabi-gcc/g++ (Cortex-M0+)
2. GNU Make
3. STM32CubeG0 (CMSIS/HAL-Header von ST), eingebunden über `../../../STM32CubeG0/...`
4. `Makefile.arm.include` aus dem WMuCpp-Workspace (Compiler- und Linker-Regeln)
5. st-flash oder STM32CubeProgrammer (SWD/ST-Link)

Damit die relativen Pfade stimmen, liegt das Projekt als Geschwisterordner von `rcmultiswitchG030/`:

```
<WMuCpp-Workspace>/
├── include_stm32/
├── Makefile.arm.include
├── rcmultiswitchG030/          (Original, unverändert)
└── rcmultiswitchG030_mkan/     (dieses Projekt)
```

Bauen: `make weact` erzeugt `msw_mkan.bin` und `msw_mkan.elf`. Flashen per Kommandozeile:

```
st-flash --reset --connect-under-reset write msw_mkan.bin 0x8000000
```

Größe des Testbuilds (GCC 13.2.1): text 20024, data 6136, bss 1772, also ca. 26 KB von 64 KB Flash.

## Bekannte Stolperstelle im Framework

`include_stm32/rc/messagebuffer_2.h`, Zeile ~185: `static_assert(m.message.size() == data.size());` mit einer Laufzeit-Referenz bricht mit GCC 13 / C++23 ab. Im Testbuild wurde es lokal durch ein Laufzeit-`assert` ersetzt. Baut die eigene Toolchain das Original, ist nichts zu tun.
