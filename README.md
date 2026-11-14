# rcmultiswitchG030_eigen (MSW-MKan)

CRSF-Multiswitch mit 8 Ausgängen für das WeAct-Board **STM32G031F8**. Abgeleitet von `rcmultiswitchG030` aus Wilhelm Meiers [WMuCpp-Framework](https://github.com/wimalopaan/wmucpp) (bare-metal C++, kein Arduino).

Neu gegenüber dem Original ist eine dritte CRSF-Adresse **„MKan“**: Ist ein Ausgang über die MKan-Adresse eingeschaltet, nutzt er eine zweite, unabhängige Blink-Konfiguration. Alles ist über das CRSF-Lua-Menü am Sender einstellbar.

Projektbeschreibung als Word-Dokument: [`docs/Projektbeschreibung.docx`](docs/Projektbeschreibung.docx) · Entwicklungsnotizen: [`README_MKAN.md`](README_MKAN.md)

## Funktionen

- 8 Ausgänge mit Blink-Modi, Intervallzeiten und Blitz-Anzahl
- Adressen: „Switch Addr“, „MKan Addr“ und die CRSF-Geräteadresse (0xC0–0xCF)
- Je Ausgang Konfiguration 1 (Switch-Adresse) und Konfiguration 2 (MKan-Adresse)
- Priorität: MKan an → Konfiguration 2, sonst entscheidet die Switch-Adresse
- PWM-Modus, Duty und Expo je Ausgang einmal (gelten für beide Konfigurationen)
- PWM-Frequenz in drei Gruppen: G1 (O4, O7), G2 (O5, O6), G3 (O0–O3)

Wegen des Lua-Budgets (123 von 132 Einträgen) sind Morse, Pattern, Virtuals, Slave-Kommando, Failsafe und das Operate-Menü abgeschaltet.

## Bauen und Flashen

Dieses Projekt ist **kein** Arduino-Sketch. Es gibt keine fertige Firmware-Datei im Repo und keinen automatischen Build bei GitHub, die Datei `msw_mkan.bin` muss einmal selbst gebaut werden. Danach wird sie mit einem ST-Link-Programmer auf das WeAct-Board geschrieben.

### 1. Firmware bauen (`msw_mkan.bin`)

Das Projekt braucht den WMuCpp-Baum, weil die Pfade relativ sind. Der Ordner liegt als Geschwister von `rcmultiswitchG030`:

```
<WMuCpp>/
├── include_stm32/
├── Makefile.arm.include
├── rcmultiswitchG030/
└── rcmultiswitchG030_mkan/     (dieses Repo)
```

Toolchain: `arm-none-eabi-gcc`, GNU Make, STM32CubeG0 (als `../../../STM32CubeG0`). Unter Windows am einfachsten in WSL (Ubuntu). Im Ordner dieses Repos:

```
make weact
```

Ergebnis: `msw_mkan.bin` (Testbuild mit GCC 13.2: ca. 26 KB von 64 KB Flash). Hinweis zu GCC 13 siehe [Offene Punkte](#offene-punkte).

### 2. Flashen mit STM32CubeProgrammer (Windows, ohne Kommandozeile)

**Einmalig einrichten**

1. **STM32CubeProgrammer** von st.com herunterladen und installieren (kostenlos, Anmeldung bei ST nötig). Der ST-Link-Treiber wird mit installiert.
2. Einen **ST-Link** (z. B. ST-Link V2 oder Clone) bereitlegen.

**Verkabeln** (ST-Link → WeAct STM32G031F8, SWD-Stiftleiste am Board):

| ST-Link | WeAct-Board |
|---|---|
| 3.3V | 3V3 |
| GND | GND |
| SWDIO | DIO (SWDIO) |
| SWCLK | CLK (SWCLK) |
| RST (nur falls nötig, siehe unten) | NRST / R |

Das Board dabei **nicht** gleichzeitig über den Empfänger mit 5 V versorgen, oder 3.3V vom ST-Link weglassen und nur GND, SWDIO, SWCLK verbinden.

**Flashen**

1. ST-Link in den USB-Port stecken, STM32CubeProgrammer starten.
2. Rechts oben **ST-LINK** auswählen, **Connect** klicken. Unten im Log steht dann die erkannte Device ID (STM32G03x/G04x).
3. Links auf das Symbol **Erasing & Programming** (zweites Symbol, Pfeil nach unten) klicken.
4. Bei **File path** mit **Browse** die Datei `msw_mkan.bin` wählen, **Start address** `0x08000000`.
5. Haken bei **Verify programming** und **Run after programming** setzen, dann **Start Programming** klicken.
6. Meldung „File download complete“ abwarten, oben rechts **Disconnect** klicken, ST-Link abziehen. Fertig.

**Wenn „Connect“ nicht klappt** („No STM32 target found“): RST-Leitung anschließen und rechts unter **Mode** „Under reset“ wählen, dann erneut **Connect**.

**Alternativ per Kommandozeile** (stlink-tools):

```
st-flash --reset --connect-under-reset write msw_mkan.bin 0x8000000
```

## Stand

- Kompiliert und gelinkt; `_init`-Fehler nach dem ersten Flash-Test behoben (`system_g0.cc`)
- Hardwaretest der neuen Lua-Punkte und der Priorität MKan vor Switch steht aus

## Offene Punkte

- Im wmucpp-Fork gibt es einen zweiten MKan-Stand (Branch `mkan-zweite-konfiguration`, hinter `USE_MKAN`). Einer der beiden sollte gewinnen.
- `rc/messagebuffer_2.h` (Framework, Zeile ~185): `static_assert` bricht mit GCC 13/C++23, im Testbuild lokal durch `assert` ersetzt.
- Die fertige `msw_mkan.bin` liegt nicht im Repo.

## Dateien

| Datei | Änderung gegenüber `rcmultiswitchG030` |
|---|---|
| `msw_mkan.cc` | neu, Basis `msw30.cc`, fest auf `HW_WEACT` |
| `eeprom.h` | dritte Adresse `AdrIndex::MKan`, zweite Konfiguration je Ausgang |
| `switch_cb.h` | Auswertung der MKan-Adresse |
| `crsf_cb.h` | Lua-Menü: „MKan Addr“ und Felder „… 2“ je Ausgang |
| `system_g0.cc` | leere `_init()`/`_fini()` |
| `Makefile.G031` | Zieldatei `msw_mkan` |
| übrige | unverändert übernommen |
