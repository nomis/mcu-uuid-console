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

#ifndef UUID_CONSOLE_H_
#define UUID_CONSOLE_H_

#include <Arduino.h>
#include <stdarg.h>

#include <functional>
#include <list>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <uuid/common.h>
#include <uuid/log.h>

namespace uuid {

namespace console {

class Shell;

class Commands {
public:
	struct Completion {
		std::list<std::list<std::string>> help;
		std::list<std::string> replacement;
	};

	struct Execution {
		const __FlashStringHelper *error;
	};

	using command_function = std::function<void(Shell &shell, const std::vector<std::string> &arguments)>;
	using argument_completion_function = std::function<const std::set<std::string>(Shell &shell, const std::vector<std::string> &arguments)>;

	Commands() = default;
	~Commands() = default;

	static flash_string_vector no_arguments;
	static argument_completion_function no_argument_completion;

	void add_command(unsigned int context, unsigned int flags,
			const flash_string_vector &name, const flash_string_vector &arguments,
			command_function function, argument_completion_function arg_function);
	Execution execute_command(Shell &shell, unsigned int context, unsigned int flags, const std::list<std::string> &command_line);
	Completion complete_command(Shell &shell, unsigned int context, unsigned int flags, const std::list<std::string> &command_line);

private:
	class Command {
	public:
		Command(unsigned int context, unsigned int flags,
				const flash_string_vector name, const flash_string_vector arguments,
				command_function function, argument_completion_function arg_function);
		~Command();

		size_t minimum_arguments() const;
		size_t maximum_arguments() const;

		unsigned int context_;
		unsigned int flags_;
		const flash_string_vector name_;
		const flash_string_vector arguments_;
		command_function function_;
		argument_completion_function arg_function_;
	};

	struct Match {
		std::multimap<size_t,std::shared_ptr<const Command>> exact;
		std::multimap<size_t,std::shared_ptr<const Command>> partial;
	};

	/**
	 * Find commands by matching them against the command line.
	 * Each matching command is returned in a map grouped by size.
	 */
	Match find_command(unsigned int context, unsigned int flags, const std::list<std::string> &command_line);

	std::list<std::shared_ptr<Command>> commands_;
};

class Shell: public std::enable_shared_from_this<Shell>, public uuid::log::Receiver {
public:
	static constexpr size_t MAX_COMMAND_LINE_LENGTH = 80;
	static constexpr size_t MAX_LOG_MESSAGES = 20;

	using password_function = std::function<void(Shell &shell, bool completed, const std::string &password)>;
	using delay_function = std::function<void(Shell &shell)>;

	~Shell() override;

	static void loop_all();

	void start();
	virtual void add_log_message(std::shared_ptr<uuid::log::Message> message);
	uuid::log::Level get_log_level();
	void set_log_level(uuid::log::Level level);
	void loop_one();
	bool running();
	void stop();

	void enter_password(const __FlashStringHelper *prompt, password_function function);
	void delay_for(unsigned long ms, delay_function function);
	void delay_until(uint64_t ms, delay_function function);

	virtual void print(char data) = 0;
	virtual void print(const char *data) = 0;
	virtual void print(const std::string &data) = 0;
	virtual void print(const __FlashStringHelper *data) = 0;
	void println();
	void println(const char *data);
	void println(const std::string &data);
	void println(const __FlashStringHelper *data);
	void printf(const char *format, ...) /* __attribute__((format (printf, 2, 3))) */;
	void printf(const __FlashStringHelper *format, ...) /* __attribute__((format(printf, 2, 3))) */;
	void printfln(const char *format, ...) /* __attribute__((format (printf, 2, 3))) */;
	void printfln(const __FlashStringHelper *format, ...) /* __attribute__((format(printf, 2, 3))) */;

	static uuid::log::Logger logger_;
	unsigned int context_ = 0;
	unsigned int flags_ = 0;

protected:
	Shell() = default;
	Shell(std::shared_ptr<Commands> commands, int context, int flags = 0);

	virtual size_t maximum_command_line_length() const;
	virtual size_t maximum_log_messages() const;
	virtual int read_one_char() = 0;
	virtual void erase_current_line();
	virtual void erase_characters(size_t count);

	virtual void started();
	virtual void display_banner();
	virtual std::string hostname_text();
	virtual std::string context_text();
	virtual std::string prompt_prefix();
	virtual std::string prompt_suffix();
	virtual void end_of_transmission();
	virtual void stopped();

	void invoke_command(std::string line);

	std::list<std::string> parse_line(const std::string &line);
	std::string unparse_line(const std::list<std::string> &items);

private:
	enum class Mode {
		NORMAL,
		PASSWORD,
		DELAY,
	};

	Shell(const Shell&) = delete;
	Shell& operator=(const Shell&) = delete;

	void loop_normal();
	void loop_password();
	void loop_delay();

	void display_prompt();
	void output_logs();
	void process_command();
	void process_completion();
	void process_password(bool completed);

	void delete_buffer_word(bool display);

	void vprintf(const char *format, va_list ap);
	void vprintf(const __FlashStringHelper *format, va_list ap);

	static std::set<std::shared_ptr<Shell>> shells_;

	std::shared_ptr<Commands> commands_;
	bool stopped_ = false;
	Mode mode_ = Mode::NORMAL;
	std::string line_buffer_;
	char previous_ = 0;
	bool prompt_displayed_ = false;
	unsigned long log_message_id_ = 0;
	std::list<std::pair<unsigned long,std::shared_ptr<uuid::log::Message>>> log_messages_;
	const __FlashStringHelper *password_prompt_ = nullptr;
	password_function password_function_;
	uint64_t delay_time_ = 0;
	delay_function delay_function_;
};

class StreamConsole: virtual public Shell {
public:
	StreamConsole(std::shared_ptr<Commands> commands, Stream &stream, int context, int flags = 0);
	~StreamConsole() override = default;

	void print(char data) override;
	void print(const char *data) override;
	void print(const std::string &data) override;
	void print(const __FlashStringHelper *data) override;

protected:
	StreamConsole(Stream &stream);

	int read_one_char() override;

private:
	StreamConsole(const StreamConsole&) = delete;
	StreamConsole& operator=(const StreamConsole&) = delete;

	Stream &stream_;
};

} // namespace console

} // namespace uuid

#endif
