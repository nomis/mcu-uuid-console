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

#ifndef MOCK_UUID_COMMON_H_
#define MOCK_UUID_COMMON_H_

namespace uuid {

static __attribute__((unused)) std::string read_flash_string(const __FlashStringHelper *flash_str) {
	return reinterpret_cast<const char *>(flash_str);
}
using flash_string_vector = std::vector<const __FlashStringHelper*>;

uint64_t get_uptime_ms();

} // namespace uuid

#endif
