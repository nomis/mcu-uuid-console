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

#include <memory>
#include <list>
#include <set>
#include <string>
#include <vector>

#ifndef __cpp_lib_make_unique
namespace std {

template<typename _Tp, typename... _Args>
inline unique_ptr<_Tp> make_unique(_Args&&... __args) {
	return unique_ptr<_Tp>(new _Tp(std::forward<_Args>(__args)...));
}

} // namespace std
#endif

namespace uuid {

namespace console {

Shell::Shell(std::shared_ptr<Commands> commands, unsigned int context, unsigned int flags)
		: commands_(commands), flags_(flags) {
	enter_context(context);
}

Shell::~Shell() {
	uuid::log::Logger::unregister_handler(this);
}

void Shell::start() {
	uuid::log::Logger::register_handler(this, uuid::log::Level::NOTICE);
	line_buffer_.reserve(maximum_command_line_length());
	display_banner();
	display_prompt();
	shells_.insert(shared_from_this());
	started();
};

void Shell::started() {

}

bool Shell::running() const {
	return !stopped_;
}

void Shell::stop() {
	if (running()) {
		stopped_ = true;
		stopped();
	}
}

void Shell::stopped() {

}

void Shell::loop_one() {
	output_logs();

	switch (mode_) {
	case Mode::NORMAL:
		loop_normal();
		break;

	case Mode::PASSWORD:
		loop_password();
		break;

	case Mode::DELAY:
		loop_delay();
		break;
	}
}

void Shell::loop_normal() {
	const int input = read_one_char();

	if (input < 0) {
		return;
	}

	const unsigned char c = input;

	switch (c) {
	case '\x03':
		// Interrupt (^C)
		line_buffer_.clear();
		println();
		prompt_displayed_ = false;
		display_prompt();
		break;

	case '\x04':
		// End of transmission (^D)
		if (line_buffer_.empty()) {
			end_of_transmission();
		}
		break;

	case '\x08':
	case '\x7F':
		// Backspace (^H)
		// Delete (^?)
		if (!line_buffer_.empty()) {
			erase_characters(1);
			line_buffer_.pop_back();
		}
		break;

	case '\x09':
		// Tab (^I)
		process_completion();
		break;

	case '\x0A':
		// Line feed (^J)
		if (previous_ != '\x0D') {
			process_command();
		}
		break;

	case '\x0C':
		// New page (^L)
		erase_current_line();
		display_prompt();
		break;

	case '\x0D':
		// Carriage return (^M)
		process_command();
		break;

	case '\x15':
		// Delete line (^U)
		erase_current_line();
		line_buffer_.clear();
		display_prompt();
		break;

	case '\x17':
		// Delete word (^W)
		delete_buffer_word(true);
		break;

	default:
		if (c >= '\x20' && c <= '\x7E') {
			// ASCII text
			if (line_buffer_.length() < maximum_command_line_length()) {
				line_buffer_.push_back(c);
				write((uint8_t)c);
			}
		}
		break;
	}

	previous_ = c;
}

Shell::PasswordData::PasswordData(const __FlashStringHelper *password_prompt, password_function password_function)
		: password_prompt_(password_prompt), password_function_(password_function) {

}

void Shell::loop_password() {
	const int input = read_one_char();

	if (input < 0) {
		return;
	}

	const unsigned char c = input;

	switch (c) {
	case '\x03':
		// Interrupt (^C)
		process_password(false);
		break;

	case '\x08':
	case '\x7F':
		// Backspace (^H)
		// Delete (^?)
		if (!line_buffer_.empty()) {
			line_buffer_.pop_back();
		}
		break;

	case '\x0A':
		// Line feed (^J)
		if (previous_ != '\x0D') {
			process_password(true);
		}
		break;

	case '\x0C':
		// New page (^L)
		erase_current_line();
		display_prompt();
		break;

	case '\x0D':
		// Carriage return (^M)
		process_password(true);
		break;

	case '\x15':
		// Delete line (^U)
		line_buffer_.clear();
		break;

	case '\x17':
		// Delete word (^W)
		delete_buffer_word(false);
		break;

	default:
		if (c >= '\x20' && c <= '\x7E') {
			// ASCII text
			if (line_buffer_.length() < maximum_command_line_length()) {
				line_buffer_.push_back(c);
			}
		}
		break;
	}

	previous_ = c;
}

Shell::DelayData::DelayData(uint64_t delay_time, delay_function delay_function)
		: delay_time_(delay_time), delay_function_(delay_function) {

}

void Shell::loop_delay() {
	auto *delay_data = reinterpret_cast<Shell::DelayData*>(mode_data_.get());

	if (uuid::get_uptime_ms() >= delay_data->delay_time_) {
		auto function_copy = delay_data->delay_function_;

		mode_ = Mode::NORMAL;
		mode_data_.reset();

		function_copy(*this);

		if (running()) {
			display_prompt();
		}
	}
}

bool Shell::exit_context() {
	if (context_.size() > 1) {
		context_.pop_back();
		return true;
	} else {
		return false;
	}
}

void Shell::enter_password(const __FlashStringHelper *prompt, password_function function) {
	if (mode_ == Mode::NORMAL) {
		mode_ = Mode::PASSWORD;
		mode_data_ = std::make_unique<Shell::PasswordData>(prompt, function);
	}
}

void Shell::delay_for(unsigned long ms, delay_function function) {
	delay_until(uuid::get_uptime_ms() + ms, function);
}

void Shell::delay_until(uint64_t ms, delay_function function) {
	if (mode_ == Mode::NORMAL) {
		mode_ = Mode::DELAY;
		mode_data_ = std::make_unique<Shell::DelayData>(ms, function);
	}
}

void Shell::delete_buffer_word(bool display) {
	size_t pos = line_buffer_.find_last_of(' ');

	if (pos == std::string::npos) {
		line_buffer_.clear();
		if (display) {
			erase_current_line();
			display_prompt();
		}
	} else {
		if (display) {
			erase_characters(line_buffer_.length() - pos);
		}
		line_buffer_.resize(pos);
	}
}

size_t Shell::maximum_command_line_length() const {
	return MAX_COMMAND_LINE_LENGTH;
}

void Shell::process_command() {
	std::list<std::string> command_line = parse_line(line_buffer_);

	line_buffer_.clear();
	println();
	prompt_displayed_ = false;

	if (!command_line.empty() && commands_) {
		auto execution = commands_->execute_command(*this, command_line);

		if (execution.error != nullptr) {
			println(execution.error);
		}
	}

	if (running()) {
		display_prompt();
	}
	::yield();
}

void Shell::process_completion() {
	std::list<std::string> command_line = parse_line(line_buffer_);

	if (!command_line.empty() && commands_) {
		auto completion = commands_->complete_command(*this, command_line);
		bool redisplay = false;

		if (!completion.help.empty()) {
			println();
			redisplay = true;

			for (auto &help : completion.help) {
				std::string help_line = format_line(help);

				println(help_line);
			}
		}

		if (!completion.replacement.empty()) {
			if (!redisplay) {
				erase_current_line();
				redisplay = true;
			}

			line_buffer_ = format_line(completion.replacement);
		}

		if (redisplay) {
			display_prompt();
		}
	}

	::yield();
}

void Shell::process_password(bool completed) {
	println();

	auto *password_data = reinterpret_cast<Shell::PasswordData*>(mode_data_.get());
	auto function_copy = password_data->password_function_;

	mode_ = Mode::NORMAL;
	mode_data_.reset();

	function_copy(*this, completed, line_buffer_);
	line_buffer_.clear();

	if (running()) {
		display_prompt();
	}
}

void Shell::invoke_command(std::string line) {
	if (!line_buffer_.empty()) {
		println();
		prompt_displayed_ = false;
	}
	if (!prompt_displayed_) {
		display_prompt();
	}
	line_buffer_ = line;
	print(line_buffer_);
	process_command();
}

} // namespace console

} // namespace uuid
