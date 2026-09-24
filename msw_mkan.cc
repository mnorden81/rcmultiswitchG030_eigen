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
// MSW-MKan - eigenstaendiges Projekt, abgeleitet von rcmultiswitchG030/msw30.cc
// (Wilhelm Meier, WMuCpp) fuer Martin Norden, 2026.
//
// Ziel: CRSF/ELRS-Multiswitch mit einer zusaetzlichen Kanal-Quelle ("MKan"),
// die je Ausgang eine ZWEITE Blink-Konfiguration aktiviert - komplett per
// CRSF-LUA-Menu konfigurierbar, kein Web-UI (anders als das ESP32-Projekt).
// Zielplattform: WeAct STM32G031F8 Board (64K Flash, HW_WEACT).
//
// Aenderungen gegenueber msw30.cc:
//  - Hardware fest auf HW_WEACT (kein Auswahlblock, keine anderen Boards).
//  - Feature-Reduktion (Morse/Pattern/Virtuals/Slave/Failsafe/Operate-Menu
//    AUS), um im LUA-Parameter-Budget (132, siehe crsf_cb.h) Platz fuer die
//    zweite Konfiguration je Ausgang zu schaffen.
//  - Neues Feature-Flag USE_MKAN aktiviert die dritte CRSF-Adresse in
//    eeprom.h/switch_cb.h/crsf_cb.h.
// ---------------------------------------------------------------------------

#ifndef HW_WEACT
#define HW_WEACT
#endif
#define HW_VERSION 11

// --- Feature-Auswahl ---------------------------------------------------
#define USE_MKAN            // zusaetzliche Kanal-Quelle mit 2. Konfiguration je Ausgang
#define USE_RESET_COMMAND   // "Reset all" im LUA-Menu (billig, 2 Parameter)
#define USE_TELEMETRY       // Telemetrie-Antwort default an

// bewusst AUS gelassen (Parameter-Budget fuer USE_MKAN freihalten):
// #define USE_MORSE
// #define USE_OPERATE_MENU
// #define USE_VIRTUALS
// #define USE_PATTERNS
// #define USE_SLAVE_COMMAND
// #define USE_FAILSAFE
// #define USE_COMMAND_DETECTION
// #define USE_BUTTON
// #define USE_IRDA
// #define USE_PWM_GLOBAL_MODULATION
// #define SERIAL_DEBUG
// #define CRSF_TX_OPENDRAIN
// #define CRSF_HALFDUPLEX
// #define USE_RESPONSE_SLOT
// #define USE_TP1

#define USE_IRDA_RX_INVERT // Polaritaet des IRDA-Sensors (falls bestueckt; sonst wirkungslos)

// debugging (nicht aendern)
// #define CRSF_LESS_BAUDRATES
#define NDEBUG

#define SW_VERSION 1 // MSW-MKan Firmware-Version 1

#include <cstdint>
#include <array>

#include "eeprom.h"
#include "devices_3.h"
#include "gfsm.h"

template<typename Config>
struct Storage {
    using debug = Config::debug;

    static inline void init() {
        std::memcpy(&eeprom, &eeprom_flash, sizeof(EEProm));
    }
    static inline void reset() {
        eeprom = EEProm{};
    }
    static inline void save() {
        if (const auto [ok, err] = Mcu::Stm32::savecfg(eeprom, eeprom_flash); ok) {
            IO::outl<debug>("# EEPROM OK");
        }
        else {
            IO::outl<debug>("# EEPROM NOK: ", err);
        }
    }
    __attribute__((__section__(".eeprom"))) static inline const EEProm eeprom_flash{};
    __attribute__((aligned (8))) static inline EEProm eeprom;
};

struct DevsConfig;

using devs = Devices<WeAct, DevsConfig>;

struct DevsConfig {
    using storage = Storage<devs>;
};

int main() {
    using gfsm = GFSM<devs>;
    using storage = devs::storage;

    storage::init();
    gfsm::init();
    gfsm::update(true);

    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_EnableIRQ(HardFault_IRQn);
    __enable_irq();

    while(true) {
        gfsm::periodic();
        devs::systemTimer::periodic([]{
            gfsm::ratePeriodic();
        });
    }
}

extern "C" {

void USART1_IRQHandler(){
    using crsf = devs::crsf;
    static_assert(crsf::number == 1);
    crsf::Isr::onTransferComplete([]{});
    crsf::Isr::onIdle([]{});
}

void HardFault_Handler() {
    while(true) {
    }
}
}
void __assert_func (const char *, int, const char *, const char *){
    while(true) {
    }
}
