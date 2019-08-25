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

#include <string>

namespace uuid {

namespace console {

void Shell::display_banner() {

}

std::string Shell::hostname_text() {
	return "";
}

std::string Shell::context_text() {
	return "";
}

std::string Shell::prompt_prefix() {
	return "";
}

std::string Shell::prompt_suffix() {
	return "$";
}

void Shell::end_of_transmission() {

}

void Shell::display_prompt() {
	switch (mode_) {
	case Mode::DELAY:
	case Mode::BLOCKING:
		break;

	case Mode::PASSWORD:
		print(reinterpret_cast<Shell::PasswordData*>(mode_data_.get())->password_prompt_);
		break;

	case Mode::NORMAL:
		std::string hostname = hostname_text();
		std::string context = context_text();

		print(prompt_prefix());
		if (!hostname.empty()) {
			print(hostname);
			print(' ');
		}
		if (!context.empty()) {
			print(context);
			print(' ');
		}
		print(prompt_suffix());
		print(' ');
		print(line_buffer_);
		prompt_displayed_ = true;
		break;
	}
}

} // namespace console

} // namespace uuid