/*
 * uuid-console - Microcontroller console shell
 * Copyright 2019  Simon Arlott
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ARDUINO_H_
#define ARDUINO_H_

#include <cstddef>
#include <cstring>

#define PROGMEM
#define PGM_P const char *
#define PSTR(s) (__extension__({static const char __c[] = (s); &__c[0];}))

class __FlashStringHelper;
#define FPSTR(string_literal) (reinterpret_cast<const __FlashStringHelper *>(string_literal))
#define F(string_literal) (FPSTR(PSTR(string_literal)))

#define vsnprintf_P vsnprintf

#define pgm_read_byte(addr) (*reinterpret_cast<const char *>(addr))

static __attribute__((unused)) void yield(void) {}

class Stream {
public:
	bool available() { return true; }
	int read() { return '\n'; }
	size_t write(char c) { return 1; }
	size_t write(const char *data) { return strlen(data); }
	size_t write(const char *buffer __attribute__((unused)), size_t size) { return size; }
	size_t print(const __FlashStringHelper *data) { return strlen(reinterpret_cast<const char *>(data)); }
};

#endif
