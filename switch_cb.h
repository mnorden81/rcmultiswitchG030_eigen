/*
 * WMuCpp - Bare Metal C++
 * Copyright (C) 2019 - 2026 Wilhelm Meier <wilhelm.wm.meier@googlemail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// ---------------------------------------------------------------------------
// Abgeleitet von rcmultiswitchG030/switch_cb.h (Wilhelm Meier).
// Aenderung fuer MSW-MKan (2026, fuer Martin Norden):
//  - setIndex() bekommt einen dritten Zweig fuer AdrIndex::MKan.
//  - Bevor ein Ausgang geschaltet wird, wird die passende Parameter-
//    Konfiguration (1 oder 2) in das zugehoerige BlinkerWithPwm-Objekt
//    geladen (ueber dessen bereits vorhandene Setter blink()/on_dezi()/
//    off_dezi()/flash_count() - dieselben, die auch das LUA-Menu benutzt).
//    Das Blinker-Framework (blinker.h) selbst wird NICHT veraendert.
//
// BUGFIX (nach Martins Test: WM0/Switch zeigte gar keine Wirkung mehr,
// nur WM1/MKan):
// Die urspruengliche Fassung hat bei JEDER eingehenden CRSF-Set-Nachricht
// (egal ob Switch- oder MKan-Adresse) fuer ALLE 8 Ausgaenge sofort die
// zugehoerige Konfiguration in den (pro Ausgang nur EINMAL vorhandenen)
// Blinker geladen und den Zustand gesetzt - unabhaengig davon, ob sich
// etwas geaendert hat. Das Multiswitch-Protokoll sendet den Zustand aber
// offenbar periodisch erneut (nicht nur bei Aenderung). Wurden beide
// Adressen fuer denselben Ausgang aktiv genutzt, hat also staendig die
// zuletzt verarbeitete Nachricht die andere ueberschrieben - ohne feste
// Regel, rein zufaellig je nach Verarbeitungsreihenfolge. In der Praxis
// hat dabei WM1 (MKan) staendig gewonnen, WM0 (Switch) wirkte tot.
//
// Fix: Es wird jetzt der zuletzt empfangene Ein/Aus-Zustand JE QUELLE
// getrennt gespeichert (switchOn[]/mkanOn[]) und daraus eine feste,
// deterministische Prioritaet gebildet:
//   MKan aktiv (on)  -> Konfiguration 2 gewinnt, unabhaengig von Switch
//   MKan inaktiv     -> Switch (Konfiguration 1) bestimmt den Ausgang
// "Zweites Szenario" (MKan) uebersteuert also gezielt den Normalbetrieb,
// wenn es aktiv ist - das duerfte dem beabsichtigten Anwendungsfall
// entsprechen. Falls du es umgekehrt willst (Switch uebersteuert MKan)
// oder "letzter Zustand gewinnt", sag Bescheid, dann passe ich es an.
// ---------------------------------------------------------------------------

#pragma once

#include <array>
#include "eeprom.h"

template<typename Config>
struct SwitchCallback {
    using storage = Config::storage;
    using debug = Config::debug;
    using crsf = Config::crsf;
    using bsws = Config::bsws;

    static inline void baud(const uint8_t port, const uint32_t b) {
        for(const auto& bd: RC::Protokoll::Crsf::V4::baudrates) {
            if (bd == b) {
                IO::outl<debug>("# baud: ", b, " p: ", port);
                crsf::baud(b);
                return;
            }
        }
        IO::outl<debug>("# wrong baud: ", b);
    }

    static inline void prop(const uint8_t adrIndex, const uint8_t channel, const uint8_t duty) {
        uint8_t dutyLimited = std::min(duty, uint8_t{100});
        if ((adrIndex == EEProm::AdrIndex::Switch) && (channel < 8)) {
            IO::outl<debug>("# prop: ", channel, " duty: ", dutyLimited);
            Meta::visitAt<bsws>(channel, [&]<typename SW>(Meta::Wrapper<SW>){
                SW::duty(dutyLimited);
            });
        }
    }
    static inline void set(const uint8_t adrIndex, const uint8_t sw) {
        IO::outl<debug>("# set: ", sw);
        for(uint8_t i = 0; i < 8; ++i) {
            const uint8_t mask = (0x01 << i);
            setIndex(adrIndex, i, (sw & mask));
        }
    }
    static inline void setIndex(const uint8_t adrIndex, const uint8_t swIndex, const bool on) {
        if (swIndex >= 8) return;
        if (adrIndex == EEProm::AdrIndex::Switch) {
            switchOn[swIndex] = on;
        }
#ifdef USE_MKAN
        else if (adrIndex == EEProm::AdrIndex::MKan) {
            mkanOn[swIndex] = on;
        }
#endif
        else {
            return;
        }
        applyAndSet(swIndex);
    }
#ifdef USE_SLAVE_COMMAND
    static inline void slaveSet(const uint8_t p, const uint8_t master) {
        if (master != storage::eeprom.master) {
            return;
        }
        for(uint8_t i = 0; i < 8; ++i) {
            if (isSlave(i)) {
                const uint8_t mask = (1 << i);
                Meta::visitAt<bsws>(i, [&]<typename SW>(Meta::Wrapper<SW>){
                    SW::event((p & mask) ? SW::Event::On : SW::Event::Off);
                });
            }
        }
    }
#endif
    private:
    static inline bool isSlave([[maybe_unused]] const uint8_t swIndex) {
#ifdef USE_SLAVE_COMMAND
        return (storage::eeprom.outputs[swIndex].slaveEnable > 0);
#else
        return false;
#endif
    }
    // Letzter bekannter Ein/Aus-Zustand je Quelle und Ausgang (siehe Bugfix-
    // Kommentar oben). Default: aus.
    static inline std::array<bool, 8> switchOn{};
#ifdef USE_MKAN
    static inline std::array<bool, 8> mkanOn{};
#endif

    // Ermittelt aus switchOn/mkanOn den fuer diesen Ausgang aktuell
    // gueltigen Zustand + Konfiguration und wendet beides an. Wird bei
    // JEDER Aenderung (Switch ODER MKan) fuer den betroffenen Ausgang neu
    // aufgerufen - dadurch ist das Ergebnis unabhaengig von der
    // Verarbeitungsreihenfolge der beiden Adressen.
    static inline void applyAndSet(const uint8_t swIndex) {
        if (swIndex >= storage::eeprom.outputs.size()) return;
        const auto& o = storage::eeprom.outputs[swIndex];
#ifdef USE_MKAN
        if (mkanOn[swIndex]) {
            Meta::visitAt<bsws>(swIndex, [&]<typename SW>(Meta::Wrapper<SW>){
                SW::blink(o.blink2);
                SW::on_dezi(o.blinkOnTime2);
                SW::off_dezi(o.blinkOffTime2);
                SW::flash_count(o.flashCount2);
            });
            setIndex(swIndex, true);
            return;
        }
#endif
        Meta::visitAt<bsws>(swIndex, [&]<typename SW>(Meta::Wrapper<SW>){
            SW::blink(o.blink);
            SW::on_dezi(o.blinkOnTime);
            SW::off_dezi(o.blinkOffTime);
            SW::flash_count(o.flashCount);
        });
        setIndex(swIndex, switchOn[swIndex]);
    }
    static inline void setIndex(const uint8_t swIndex, const bool on) {
        if (isSlave(swIndex)) {
            return;
        }
        Meta::visitAt<bsws>(swIndex, [&]<typename SW>(Meta::Wrapper<SW>){
                                SW::event(on ? SW::Event::On : SW::Event::Off);
                            });
    }
};
