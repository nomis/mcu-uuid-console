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

#ifndef UUID_LOG_H_
#define UUID_LOG_H_

#include <stdarg.h>

#include <string>

namespace uuid {

namespace log {

enum class Level {
	OFF = -1,
	EMERG = 0,
	ALERT,
	CRIT,
	ERR,
	WARNING,
	NOTICE,
	INFO,
	DEBUG,
	TRACE,
	ALL,
};

enum class Facility {
	LPR,
};

static __attribute__((unused)) std::string format_timestamp_ms(int days_width, uint64_t timestamp_ms) { return ""; }
static __attribute__((unused)) char format_level_char(Level level) { return ' '; }
static __attribute__((unused)) const __FlashStringHelper *format_level_uppercase(Level level) { return F(""); }
static __attribute__((unused)) const __FlashStringHelper *format_level_lowercase(Level level) { return F(""); }

class Message {
public:
	Message(uint64_t uptime_ms, Level level, Facility facility, const __FlashStringHelper *name, const std::string &&text);
	~Message() = default;

	const uint64_t uptime_ms_;
	const Level level_;
	const Facility facility_;
	const __FlashStringHelper *name_;
	const std::string text_;
};

class Receiver {
public:
	virtual ~Receiver() = default;

	virtual void add_log_message(std::shared_ptr<Message> message) = 0;

protected:
	Receiver() = default;
};

class Logger {
public:
	Logger(const __FlashStringHelper *name __attribute__((unused)), Facility facility __attribute__((unused))) {};
	~Logger() = default;

	static void register_receiver(Receiver *receiver __attribute__((unused)), Level level __attribute__((unused))) {}
	static void unregister_receiver(Receiver *receiver __attribute__((unused))) {}

	static Level get_log_level(Receiver *receiver) { return Level::ALL; }

	static inline bool enabled(Level level) { return true; }
	void emerg(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void emerg(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void alert(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void alert(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void crit(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void crit(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void err(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void err(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void warning(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void warning(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void notice(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void notice(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void info(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void info(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void debug(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void debug(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void trace(const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void trace(const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
	void log(Level level __attribute__((unused)), Facility facility __attribute__((unused)), const char *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(format, ap); va_end(ap); }
	void log(Level level __attribute__((unused)), Facility facility __attribute__((unused)), const __FlashStringHelper *format __attribute__((unused)), ...) { va_list ap; va_start(ap, format); vprintf(reinterpret_cast<const char *>(format), ap); va_end(ap); }
};

} // namespace log

} // namespace uuid

#endif
