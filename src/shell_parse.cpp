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

#include <list>
#include <string>

namespace uuid {

namespace console {

std::list<std::string> Shell::parse_line(const std::string &line) {
	std::list<std::string> items;
	bool string_escape_double = false;
	bool string_escape_single = false;
	bool char_escape = false;

	if (!line.empty()) {
		items.emplace_back("");
	}

	for (char c : line) {
		switch (c) {
		case ' ':
			if (string_escape_double || string_escape_single) {
				if (char_escape) {
					items.back().push_back('\\');
					char_escape = false;
				}
				items.back().push_back(' ');
			} else if (char_escape) {
				items.back().push_back(' ');
				char_escape = false;
			} else if (!items.back().empty()) {
				items.emplace_back("");
			}
			break;

		case '"':
			if (char_escape || string_escape_single) {
				items.back().push_back('"');
				char_escape = false;
			} else {
				string_escape_double = !string_escape_double;
			}
			break;

		case '\'':
			if (char_escape || string_escape_double) {
				items.back().push_back('\'');
				char_escape = false;
			} else {
				string_escape_single = !string_escape_single;
			}
			break;

		case '\\':
			if (char_escape) {
				items.back().push_back('\\');
				char_escape = false;
			} else {
				char_escape = true;
			}
			break;

		default:
			if (char_escape) {
				items.back().push_back('\\');
				char_escape = false;
			}
			items.back().push_back(c);
			break;
		}
	}

	return items;
}

std::string Shell::format_line(const std::list<std::string> &items) {
	std::string line;

	line.reserve(maximum_command_line_length());

	for (auto &item : items) {
		if (!line.empty()) {
			line += ' ';
		}

		for (char c : item) {
			switch (c) {
			case ' ':
			case '\"':
			case '\'':
			case '\\':
				line += '\\';
				break;
			}

			line += c;
		}
	}

	return line;
}

} // namespace console

} // namespace uuid
