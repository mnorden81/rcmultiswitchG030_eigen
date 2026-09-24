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
// Abgeleitet von rcmultiswitchG030/crsf_cb.h (Wilhelm Meier).
// Aenderung fuer MSW-MKan (2026, fuer Martin Norden):
//  - "MKan Addr" Parameter im Global-Menu ergaenzt (zweite CRSF-Adresse).
//  - addOutput<>() um eine zweite, flache Parameter-Gruppe
//    ("Intervall Mode 2/on 2/off 2/count 2") je Ausgang erweitert.
//  - Morse/Pattern/Virtuals/Slave/Failsafe/Operate-Menu bewusst entfernt,
//    um im 132-Parameter-Budget der WeAct-Zielplattform (etl::FixedVector)
//    Platz fuer die Konfiguration 2 zu schaffen. Stand nach dieser Datei:
//    ca. 124 von 132 Parametern belegt (siehe Kommentar am Dateiende).
// ---------------------------------------------------------------------------

#pragma once

#include "rc/crsf_2.h"
#include "eeprom.h"

template<typename Config>
struct CrsfCallback {
    using watchdog = Config::watchdog;
    using storage = Config::storage;
    using debug = Config::debug;

    using Param_t = RC::Protokoll::Crsf::V4::Parameter<uint8_t>;
    using PType = Param_t::Type;

    using switchCallback = Config::switchCallback;
    struct DecoderConfig {
        using debug = CrsfCallback::debug;
        using storage = CrsfCallback::storage;
        using callback = switchCallback;
    };
    using swDecoder = RC::Protokoll::Crsf::V4::Util::SwitchDecoder<DecoderConfig>;

    using bsws = Config::bswList;
    using pwms = Config::pwmList;
    using timer = Config::timer;
    using crsf = Config::crsf;
    using messageBuffer = crsf::messageBuffer;
    using telemetry = Config::telemetry;

    static inline constexpr const char* const title = "MultiSwitch-E@";
    using name_t = std::array<char, 32>;

    static inline constexpr void disableTelemetry()
    requires(!std::is_same_v<telemetry, void>) {
        IO::outl<debug>("# disable telemetry");
        telemetry::disableWithAutoOn();
    }
#ifdef USE_IRDA
    static inline void gotChannels() {
        messageBuffer::sendNext();
    }
#endif

    static inline constexpr void updateName(name_t& n) {
        strncpy(&n[0], title, n.size());
        auto r = std::to_chars(std::begin(n) + strlen(title), std::end(n), storage::eeprom.addresses[EEProm::AdrIndex::Switch]);
        *r.ptr++ = ':';
        r = std::to_chars(r.ptr, std::end(n), storage::eeprom.crsf_address);
        *r.ptr++ = '\0';
    }
    static inline void update() {
        updateName(mName);
    }
    static inline void setParameterValue(const uint8_t index, const auto data, const uint8_t paylength) {
        if (RC::Protokoll::Crsf::V4::Util::setParameter(params, index, data, paylength)) {
            storage::save();
        }
        update();
    }
    static inline Param_t parameter(const uint8_t index) {
        IO::outl<debug>("# cb param i: ", index);
        if (index < params.size()) {
            return params[index];
        }
        return {};
    }
    static inline bool isCommand(const uint8_t index) {
        IO::outl<debug>("# cb isCom i: ", index);
        if (index < params.size()) {
            return params[index].type == PType::Command;
        }
        return false;
    }
    static inline const char* name() {
        return &mName[0];
    }
    static inline uint32_t serialNumber() {
        if constexpr(!std::is_same_v<watchdog, void>) {
            makeWdgString();
        }
        return mSerialNumber;
    }
    static inline uint32_t hwVersion() {
        return mHWVersion;
    }
    static inline uint32_t swVersion() {
        return mSWVersion;
    }
    static inline uint8_t numberOfParameters() {
        return params.size() - 1;
    }
    static inline uint8_t protocolVersion() {
        return 0;
    }
    static inline void command(const auto payload, [[maybe_unused]] const uint8_t paylength) {
        swDecoder::process(payload, paylength);
    }
    static inline void serialize(const uint8_t index, auto& buffer, const RC::Protokoll::Crsf::V4::Lua::CmdStep step = RC::Protokoll::Crsf::V4::Lua::CmdStep::Idle) {
        params[index].serialize(buffer, params, step, index);
    }
    static inline void callbacks(const bool eepromMode = false) {
        const bool prevMode = mEepromMode;
        mEepromMode = eepromMode;
        for(const auto p: params) {
            if (p.cb) {
                p.cb(p.value());
            }
        }
        mEepromMode = prevMode;
    }
private:
    static inline bool resetAllParameter() {
		IO::outl<debug>("# reset all");
		storage::reset();
		return false;
	}
    static inline bool mEepromMode = false;
    static inline constexpr uint32_t mSerialNumber{1234};
    static inline constexpr uint32_t mHWVersion{HW_VERSION};
    static inline constexpr uint32_t mSWVersion{SW_VERSION};

    static inline std::array<char, 16> mWdgString = {};

    static inline void makeWdgString()
        requires(!std::is_same_v<watchdog, void>)
    {
        auto [ptr, e] = std::to_chars(std::begin(mWdgString), std::end(mWdgString), watchdog::pinResets());
        *ptr++ = ':';
        auto r = std::to_chars(ptr, std::end(mWdgString), watchdog::wdgResets());
        *r.ptr = '\0';
    }

    static inline constexpr auto mVersionString = [](){
        std::array<char, 16> s{};
        auto [ptr, e] = std::to_chars(std::begin(s), std::end(s), mHWVersion);
        *ptr++ = ':';
        auto r = std::to_chars(ptr, std::end(s), mSWVersion);
        *r.ptr = '\0';
        return s;
    }();
    static inline name_t mName = []{
        name_t name{};
        updateName(name);
        return name;
    }();
    static inline constexpr uint8_t addParent(auto& c, const Param_t& p) {
        c.push_back(p);
        return c.size() - 1;
    }
    static inline constexpr void addNode(auto& c, const Param_t& p) {
        c.push_back(p);
    }
    template<uint8_t index>
    static inline constexpr void addOutput(auto& p, const uint8_t parent, const char* const name) {
        const uint8_t parent2 = addParent(p, Param_t{parent, PType::Folder, name});
        addNode(p, Param_t{parent2, PType::Info, name, &mName[0]});
        addNode(p, Param_t{parent2, PType::Sel, "PWM Mode", "Off;On;Remote;Global/Indiv", &storage::eeprom.outputs[index].pwm, 0, 3, [](const uint8_t v){Meta::nth_element<index, bsws>::pwm(v); return true;}});
        addNode(p, Param_t{.parent = parent2, .type = PType::U8, .name = "PWM Duty", .value_ptr = &storage::eeprom.outputs[index].pwmDuty, .min = 1, .max = 99, .cb = [](const uint8_t v){Meta::nth_element<index, bsws>::duty((storage::eeprom.outputs[index].pwm == 2)?0:v); return true;}, .unitString = "%"});
        addNode(p, Param_t{parent2, PType::U8, "PWM Expo", nullptr, &storage::eeprom.outputs[index].pwmScale, 0, 100, [](const uint8_t v){Meta::nth_element<index, bsws>::expo(v); return true;}});
        // Konfiguration 1 (Switch-Adresse)
        addNode(p, Param_t{parent2, PType::Sel, "Intervall Mode", "Off;On;Morse;Pattern", &storage::eeprom.outputs[index].blink, 0, 2, [](const uint8_t v){Meta::nth_element<index, bsws>::blink(v); return true;}});
        addNode(p, Param_t{.parent = parent2, .type = PType::U8, .name = "Intervall(on)", .value_ptr = &storage::eeprom.outputs[index].blinkOnTime, .min = 1, .max = 255, .cb = [](const uint8_t v){Meta::nth_element<index, bsws>::on_dezi(v); return true;}, .unitString = "*50ms"});
        addNode(p, Param_t{.parent = parent2, .type = PType::U8, .name = "Intervall(off)", .value_ptr = &storage::eeprom.outputs[index].blinkOffTime, .min = 1, .max = 255, .cb = [](const uint8_t v){Meta::nth_element<index, bsws>::off_dezi(v); return true;}, .unitString = "*50ms"});
        addNode(p, Param_t{.parent = parent2, .type = PType::U8, .name = "Intervall(count)", .value_ptr = &storage::eeprom.outputs[index].flashCount, .min = 1, .max = 4, .cb = [](const uint8_t v){Meta::nth_element<index, bsws>::flash_count(v); return true;}});
#ifdef USE_MKAN
        // Konfiguration 2 (MKan-Adresse) - nur Werte, keine eigene Test-Taste
        // (Testen: "Test" unten schaltet unabhaengig von der Quelle).
        addNode(p, Param_t{parent2, PType::Sel, "Intervall Mode 2", "Off;On;Morse;Pattern", &storage::eeprom.outputs[index].blink2, 0, 2, [](const uint8_t v){return true;}});
        addNode(p, Param_t{.parent = parent2, .type = PType::U8, .name = "Intervall(on) 2", .value_ptr = &storage::eeprom.outputs[index].blinkOnTime2, .min = 1, .max = 255, .cb = [](const uint8_t){return true;}, .unitString = "*50ms"});
        addNode(p, Param_t{.parent = parent2, .type = PType::U8, .name = "Intervall(off) 2", .value_ptr = &storage::eeprom.outputs[index].blinkOffTime2, .min = 1, .max = 255, .cb = [](const uint8_t){return true;}, .unitString = "*50ms"});
        addNode(p, Param_t{.parent = parent2, .type = PType::U8, .name = "Intervall(count) 2", .value_ptr = &storage::eeprom.outputs[index].flashCount2, .min = 1, .max = 4, .cb = [](const uint8_t){return true;}});
#endif
        addNode(p, Param_t{parent2, PType::Sel, "Test", "Off;On", nullptr, 0, 1, [](const uint8_t v){Meta::nth_element<index, bsws>::on(v); return false;}});
    }
    static inline auto params = []{
        etl::FixedVector<Param_t, 132> p;
        addNode(p, Param_t{0, PType::Folder, ""}); // unvisible top folder
        addNode(p, Param_t{0, PType::Info, "Version(HW/SW)", &mVersionString[0]});
        addNode(p, Param_t{0, PType::Info, "Watchdog", &mWdgString[0]});
        auto parent = addParent(p, Param_t{0, PType::Folder, "Global"});
        addNode(p, Param_t{parent, PType::U8, "Switch Addr", nullptr, &storage::eeprom.addresses[EEProm::AdrIndex::Switch], 0, 255, [](const uint8_t){update(); return true;}});
#ifdef USE_MKAN
        addNode(p, Param_t{parent, PType::U8, "MKan Addr", nullptr, &storage::eeprom.addresses[EEProm::AdrIndex::MKan], 0, 255, [](const uint8_t){update(); return true;}});
#endif
#ifdef HW_WEACT
        addNode(p, Param_t{.parent = parent, .type = PType::U8, .name = "PWM f G1 (O4,7)", .value_ptr= &storage::eeprom.pwm1, .min = 1, .max = 200, .cb = [](const uint8_t v){Meta::nth_element<0, pwms>::freqCenties(v); return true;}, .def = 10, .unitString = " *100Hz"});
        addNode(p, Param_t{.parent = parent, .type = PType::U8, .name = "PWM f G2 (O5,6)", .value_ptr = &storage::eeprom.pwm2, .min = 1, .max = 200, .cb = [](const uint8_t v){Meta::nth_element<1, pwms>::freqCenties(v); return true;}, .def = 10, .unitString = " *100Hz"});
        addNode(p, Param_t{.parent = parent, .type = PType::U8, .name = "PWM f G3 (O0,1,2,3)", .value_ptr = &storage::eeprom.pwm3, .min = 1, .max = 200, .cb = [](const uint8_t v){Meta::nth_element<2, pwms>::freqCenties(v); return true;}, .def = 10, .unitString = " *100Hz"});
#endif
        addNode(p, Param_t{parent, PType::U8, "CRSF Addr", nullptr, &storage::eeprom.crsf_address, 0xc0, 0xcf, [](const uint8_t v){
                       crsf::address(std::byte{v});
                       return true;
                   }});

        addOutput<0>(p, 0, "Output 0");
        addOutput<1>(p, 0, "Output 1");
        addOutput<2>(p, 0, "Output 2");
        addOutput<3>(p, 0, "Output 3");
        addOutput<4>(p, 0, "Output 4");
        addOutput<5>(p, 0, "Output 5");
        addOutput<6>(p, 0, "Output 6");
        addOutput<7>(p, 0, "Output 7");

#ifdef USE_RESET_COMMAND
		parent = addParent(p, Param_t{0, PType::Folder, "Reset"});
        addNode(p, Param_t{parent, PType::Command, "Reset all", "Reset", nullptr, 0, 0, [](const uint8_t){
                               if (!mEepromMode) {
                                   return resetAllParameter();
                               }
                               return false;
                           }});
#endif
        if (p.size() >= p.capacity()) {
            void fp();
            fp(); // compile-time check (call to undefined function)
        }
        return p;
    }();
};

// Parameter-Bilanz (WeAct, 132er Budget), Stand dieser Datei:
//  3  (unsichtbarer Root, Version-Info, Watchdog-Info)
//  6  Global (Switch Addr, MKan Addr, 3x PWM-Freq-Gruppe, CRSF Addr)
// 112 8x Output (14 Parameter je Ausgang: 6 Basis + 4 Konfig.1 + 4 Konfig.2)
//  2  Reset (Folder + Command)
// ---
// 123 Parameter belegt, 9 Reserve bis 132 (etl::FixedVector<Param_t,132>).
// Bei Aktivierung weiterer Features (Morse/Pattern/Virtuals/Slave/Failsafe/
// Operate-Menu) muss dieses Budget neu gerechnet werden - der Compiler
// bricht sonst mit "undefined reference to fp()" hart ab (Zeile oben).
