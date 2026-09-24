# MSW-MKan – CRSF-Multiswitch mit zweiter Konfiguration je Ausgang (WeAct G031F8)

Eigenständiges Projekt, abgeleitet von `rcmultiswitchG030` (WMuCpp-Framework
von Wilhelm Meier). Erweitert das Original um eine dritte CRSF-Adresse
("MKan"), die pro Ausgang eine zweite, unabhängige Blink-Konfiguration
aktiviert – komplett über das CRSF-LUA-Menü im Sender konfigurierbar.

## Geänderte/neue Dateien gegenüber dem Original

| Datei | Status |
|---|---|
| `msw_mkan.cc` | neu (Basis: `msw30.cc`, fest auf `HW_WEACT`, reduziertes Feature-Set) |
| `eeprom.h` | geändert (dritte Adresse `AdrIndex::MKan`, zweite Konfig. je `Output`) |
| `switch_cb.h` | geändert (Dispatch für `MKan`, lädt Konfig. 1/2 in den Blinker) |
| `crsf_cb.h` | geändert (LUA-Menü: "MKan Addr" + Konfig.-2-Felder je Ausgang) |
| alle übrigen Dateien | unverändert von `rcmultiswitchG030` übernommen |

**Nicht verändert wurde `rc/crsf_2.h`** (die CRSF/`SwitchDecoder`-Logik im
Framework): Sie matcht Adressen bereits generisch über ein Array
(`storage::eeprom.addresses`), die dritte Adresse fügt sich ohne
Framework-Änderung ein.

## Wichtige Einschränkung (bitte testen!)

PWM-Modus/-Duty/-Expo sind je Ausgang **einmal** vorhanden und gelten für
beide Konfigurationen. Nur Blink-Modus, Intervall-Zeiten und Blitz-Anzahl
sind je Konfiguration getrennt.

**Verhalten bei gleichzeitiger Ansteuerung über Switch UND MKan (fester
Stand nach Bugfix vom 23.9.):** MKan hat Vorrang. Ist ein Ausgang über die
MKan-Adresse „an", zeigt er Konfiguration 2 – unabhängig davon, was die
Switch-Adresse für denselben Ausgang gerade sagt. Ist MKan für diesen
Ausgang „aus", bestimmt die Switch-Adresse (Konfiguration 1) das Ergebnis.
„Zweites Szenario" übersteuert also gezielt den Normalbetrieb, wenn es
aktiv ist. Falls du lieber die umgekehrte Priorität (Switch übersteuert
MKan) oder „zuletzt geänderter Zustand gewinnt" willst, sag Bescheid.

## Parameter-Budget (LUA-Menü)

Die WeAct-Zielplattform limitiert das LUA-Menü intern auf 132 Einträge
(`etl::FixedVector<Param_t, 132>` in `crsf_cb.h`). Dieses Projekt nutzt davon
aktuell **123** (siehe Kommentar am Ende von `crsf_cb.h`). Deshalb sind
Morse, Pattern, Virtuals, Slave-Kommando, Failsafe und das Operate-Menü
bewusst deaktiviert. Überschreitet das Menü die Kapazität, bricht die
Kompilierung absichtlich hart ab (`fp()` ist eine nicht definierte Funktion –
das ist ein Compile-Time-Check aus dem Original-Framework, kein Bug).

## Kompilieren – **Arduino IDE funktioniert hier nicht**

Anders als deine ESP32-Projekte ist das hier **kein Arduino-Sketch**. Es ist
Bare-Metal-C++ (WMuCpp-Framework): direkter Registerzugriff, eigene
Template-Metaprogrammierung, kein Arduino-Core, kein `setup()`/`loop()`. Die
Arduino IDE kann so ein Projekt nicht öffnen, nicht kompilieren und nicht
flashen – es gibt keinen STM32G0-„Arduino core", der zu diesem Code passt
(auch STM32duino wäre ein anderes, inkompatibles Programmiermodell).

Du brauchst exakt dieselbe Toolchain, mit der du `rcmultiswitchG030` schon
baust:

1. **arm-none-eabi-gcc/g++** (Cross-Compiler für Cortex-M0+)
2. **GNU Make**
3. **STM32CubeG0**-Paket (CMSIS/HAL-Header von ST, kostenlos bei ST
   herunterladbar) – wird per relativem Pfad `../../../STM32CubeG0/...`
   eingebunden, du hast es also schon irgendwo liegen
4. **`Makefile.arm.include`** – eine Datei, die bei dir vermutlich eine
   Ebene über allen WMuCpp-Projekten liegt (ich konnte sie nicht einsehen,
   da sie nicht hochgeladen wurde) und die eigentlichen Compiler-/Linker-
   Regeln enthält
5. **st-flash** (oder STM32CubeProgrammer) zum Flashen über SWD/ST-Link

### Ordnerstruktur

Damit die relativen Pfade (`../../include_stm32`, `../../../STM32CubeG0`,
`../../Makefile.arm.include`) stimmen, muss `rcmultiswitchG030_mkan/` als
**Geschwisterordner** von `rcmultiswitchG030/` liegen – also z. B.:

```
<dein WMuCpp-Workspace>/
├── include_stm32/
├── Makefile.arm.include
├── rcmultiswitchG030/          (Original, unverändert)
└── rcmultiswitchG030_mkan/     (dieses Projekt)
```

### Bauen

```
cd rcmultiswitchG030_mkan
make weact
```

Das erzeugt `msw_mkan.bin` (und `msw_mkan.elf`).

### Flashen

```
st-flash --reset --connect-under-reset write msw_mkan.bin 0x8000000
```

(ST-Link an das WeAct-Board anschließen, wie bei deinen bisherigen G030-Builds.)

## Update: erfolgreich kompiliert (als Alternative zu Codespaces)

Ich habe in meiner Sandbox tatsächlich `arm-none-eabi-gcc` (13.2.1)
installiert, die STM32CubeG0-CMSIS-/HAL-Header direkt von ST (GitHub)
geladen und `make weact` hier laufen lassen. Ergebnis: **kompiliert und
gelinkt, `msw_mkan.bin` liegt bei**, Größe:

```
   text    data     bss     dec     hex
  20040    6136    1772   27948    6d2c
```

Flash-Bedarf (text+data) ≈ 26,2 KB von 64 KB – passt komfortabel auf die
WeAct G031F8.

Zwei Dinge musste ich dafür selbst rekonstruieren/anpassen, bitte beachten:

1. **`Makefile.arm.include` ist NICHT dein Original** – die Datei war nicht
   hochgeladen, ich habe sie selbst nachgebaut (Compiler-Flags,
   `-std=c++23`, Linker-Flags für newlib-nano). Zum Bauen bei dir in
   Codespaces/lokal weiter **deine eigene** `Makefile.arm.include`
   verwenden, nicht diese hier.
2. **Ein Bugfix in `rc/messagebuffer_2.h`** (WMuCpp-Framework, Zeile 185):
   dort steht `static_assert(m.message.size() == data.size());`, wobei `m`
   eine Laufzeit-Referenz (`mFifo.front()`) ist – das lässt sich mit
   GCC 13/C++23 nicht als Konstante auswerten und bricht die Kompilierung
   hart ab. Ich vermute einen Tippfehler im Original (`assert` statt
   `static_assert` gemeint) und habe das **nur in meiner lokalen Kopie**
   auf einen Laufzeit-`assert` umgestellt, um weiterzukommen. Diese Datei
   liegt nicht in deinem Projektordner, sondern in deinem `include_stm32`
   – falls deine Codespaces-Umgebung (vermutlich ein anderer GCC) das
   Original klaglos baut, betrifft dich das eventuell nie; falls nicht,
   ist das die Stelle, die du in deinem `include_stm32/rc/messagebuffer_2.h`
   anpassen müsstest (Zeile ~185: `static_assert` → `assert`).

Der Code ist damit ein ganzes Stück weiter geprüft als reines Code-Review –
er kompiliert und linkt tatsächlich mit einer realen Toolchain. **Auf
echter Hardware getestet ist er trotzdem nicht** – das kann ich hier nicht
leisten (kein ST-Link, keine Simulation). Bitte flashen und die neuen
LUA-Menüpunkte ("MKan Addr", "Intervall Mode/on/off/count 2") sorgfältig
gegenprüfen, insbesondere das Verhalten bei gleichzeitiger Ansteuerung über
Switch- und MKan-Adresse (siehe Einschränkung oben).

## Korrektur (nach Martins erstem Flash-Test: Chip startete nicht)

Die erste ausgelieferte `msw_mkan.bin` hatte einen von mir verursachten
Bug: Beim Linken gab es einen Fehler "undefined reference to `_init`"
(newlibs `__libc_init_array()`, das `startup_g031.s` vor `main()` aufruft
und das u. a. die C++-Global-/Static-Konstruktoren ausführt, ruft intern
zuerst `_init()` auf). Ich habe das fälschlich mit dem Linker-Flag
`--defsym=_init=0` "gelöst" – das setzt `_init` auf Adresse `0x0`. Beim
Boot sprang der Chip dadurch sofort auf Adresse 0 und stürzte ab, noch vor
`main()` – exakt das gemeldete Symptom (Chip startet nicht, kein CRSF).

**Fix:** In `system_g0.cc` echte, leere Funktionen `_init()`/`_fini()`
ergänzt (statt der Adress-0-Attrappe), Linker-Flag entfernt. Neu gebaut
und per `nm`/`objdump` verifiziert, dass `_init`/`_fini` jetzt auf echten
Code zeigen und die Vektortabelle (Stackpointer, Reset-Handler) plausibel
aussieht. Größe minimal kleiner (20024/6136/1772 statt 20040/6136/1772 -
der alte Fake-`_init`-Aufruf ist weg). Die neue `msw_mkan.bin` in diesem
Zip bzw. separat verschickt ist die korrigierte Version.
