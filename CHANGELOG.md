# Versionsgeschichte rcmultiswitchG030_eigen (MSW-MKan)

Das Projekt hat keine eigene Versionsnummer in der Firmware. Die Einträge folgen den Arbeitsschritten, neueste oben.

## 14.11.2026
- README: Anleitung zum Bauen und Flashen mit STM32CubeProgrammer.

## 16.10.2026
- Projektbeschreibung als Word-Dokument (`docs/Projektbeschreibung.docx`).

## 24.09.2026: erster Stand im Repo
- Abgeleitet von `rcmultiswitchG030` (WMuCpp, Wilhelm Meier): dritte CRSF-Adresse „MKan“ mit zweiter Blink-Konfiguration je Ausgang, alles im Lua-Menü einstellbar.
- Mit arm-none-eabi-gcc 13.2.1 kompiliert und gelinkt (ca. 26 KB Flash).
- Fix nach dem ersten Hardwaretest (23.09.): die Switch-Adresse (WM0) zeigte keine Wirkung mehr, nur MKan (WM1). Jetzt fest: MKan hat Vorrang, sonst gilt die Switch-Adresse.
- Fix nach dem ersten Flash-Test: der Chip startete nicht, weil `_init` per Linker-Flag auf Adresse 0 zeigte. Jetzt echte, leere `_init()`/`_fini()` in `system_g0.cc`.
