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

#include <memory>
#include <string>

namespace uuid {

namespace console {

StreamConsole::StreamConsole(std::shared_ptr<Commands> commands, Stream *stream, int context, int flags)
		: Shell(commands, context, flags), stream_(stream) {

}

int StreamConsole::read() {
	if (stream_->available()) {
		return stream_->read();
	}

	return -1;
}

void StreamConsole::print(char data) {
	stream_->write(data);
}

void StreamConsole::print(const char *data) {
	stream_->write(data);
}

void StreamConsole::print(const std::string &data) {
	stream_->write(data.data(), data.length());
}

void StreamConsole::print(const __FlashStringHelper *data) {
	stream_->print(data);
}

} // namespace console

} // namespace uuid