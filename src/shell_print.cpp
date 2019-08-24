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

#include <uuid/console.h>

#include <Arduino.h>
#include <stdarg.h>

#include <string>

namespace uuid {

namespace console {

size_t Shell::print(const std::string &data) {
	return write(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
}

size_t Shell::println(const std::string &data) {
	size_t len = print(data);
	len += println();
	return len;
}

size_t Shell::printf(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	size_t len = vprintf(format, ap);
	va_end(ap);

	return len;
}

size_t Shell::printf(const __FlashStringHelper *format, ...) {
	va_list ap;

	va_start(ap, format);
	size_t len = vprintf(format, ap);
	va_end(ap);

	return len;
}

size_t Shell::printfln(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	size_t len = vprintf(format, ap);
	va_end(ap);

	len += println();
	return len;
}

size_t Shell::printfln(const __FlashStringHelper *format, ...) {
	va_list ap;

	va_start(ap, format);
	size_t len = vprintf(format, ap);
	va_end(ap);

	len += println();
	return len;
}

size_t Shell::vprintf(const char *format, va_list ap) {
	int len = ::vsnprintf(nullptr, 0, format, ap);
	if (len > 0) {
		std::string text(static_cast<std::string::size_type>(len), '\0');

		::vsnprintf(&text[0], text.capacity() + 1, format, ap);
		return print(text);
	} else {
		return 0;
	}
}

size_t Shell::vprintf(const __FlashStringHelper *format, va_list ap) {
	int len = ::vsnprintf_P(nullptr, 0, reinterpret_cast<PGM_P>(format), ap);
	if (len > 0) {
		std::string text(static_cast<std::string::size_type>(len), '\0');

		::vsnprintf_P(&text[0], text.capacity() + 1, reinterpret_cast<PGM_P>(format), ap);
		return print(text);
	} else {
		return 0;
	}
}

void Shell::erase_current_line() {
	print(F("\033[0G\033[K"));
}

void Shell::erase_characters(size_t count) {
	print(std::string(count, '\x08'));
	print(F("\033[K"));
}

} // namespace console

} // namespace uuid
