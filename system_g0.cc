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

#include "stm32g0xx.h"

#define VECT_TAB_OFFSET  0x0U /*!< Vector Table base offset field.
                                   This value must be a multiple of 0x100. */
void SystemInit(){
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
}

// -----------------------------------------------------------------------
// Fix (2026, MSW-MKan-Build): startup_g031.s ruft vor main() explizit
// __libc_init_array() auf (fuehrt u.a. die C++-Global-/Static-Konstruktoren
// aus - fuer dieses stark templatebasierte Framework unverzichtbar).
// __libc_init_array() ruft intern zuerst _init() auf. In einem reinen
// Bare-Metal-Build ohne HAL-Crt-Objekte existiert _init() nicht - der
// Linker bricht sonst mit "undefined reference to _init" ab.
// In einem fruehen Testbuild wurde das faelschlich per Linker-Flag
// (--defsym=_init=0) "geloest": das setzt _init auf Adresse 0x0, wodurch
// der Aufruf beim Start auf Adresse 0 spring - Absturz vor main(), Chip
// bootet nicht, keine CRSF-Kommunikation. Die korrekte Loesung: echte,
// leere _init()/_fini()-Funktionen bereitstellen.
extern "C" void _init(void) {}
extern "C" void _fini(void) {}
