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

#include <cstdarg>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <uuid/common.h>
#include <uuid/log.h>

namespace uuid {

namespace console {

class Shell;

/**
 * Container of commands for use by a Shell.
 *
 * These should normally be stored in a std::shared_ptr and reused.
 */
class Commands {
public:
	/**
	 * Result of a command completion operation.
	 *
	 * Each space-delimited parameter is a separate string.
	 */
	struct Completion {
		std::list<std::list<std::string>> help; /*!< Suggestions for matching commands. */
		std::list<std::string> replacement; /*!< Replacement matching full or partial command string. */
	};

	/**
	 * Result of a command execution operation.
	 */
	struct Execution {
		const __FlashStringHelper *error; /*!< Error message if the command could not be executed. */
	};

	/**
	 * Function to handle a command.
	 *
	 * @param[in] shell Shell instance that is executing the command.
	 * @param[in] arguments Command line arguments.
	 */
	using command_function = std::function<void(Shell &shell, const std::vector<std::string> &arguments)>;
	/**
	 * Function to obtain completions for a command line.
	 *
	 * @param[in] shell Shell instance that has a command line matching
	 *                  this command.
	 * @param[in] arguments Command line arguments prior to (but
	 *                      excluding) the argument being completed.
	 * @return Set of possible values for the next argument on the
	 *         command line.
	 */
	using argument_completion_function = std::function<const std::set<std::string>(Shell &shell, const std::vector<std::string> &arguments)>;

	/**
	 * Construct a new container of commands for use by a Shell.
	 *
	 * This should normally be stored in a std::shared_ptr and reused.
	 */
	Commands() = default;
	~Commands() = default;

	/**
	 * Parameter to specify that a command has no arguments.
	 *
	 * @return An empty flash_string_vector.
	 */
	static inline flash_string_vector no_arguments() { return flash_string_vector{}; }
	/**
	 * Parameter to specify that a command does not support argument
	 * completion.
	 *
	 * @return A std::function that returns an empty std::set.
	 */
	static inline argument_completion_function no_argument_completion() {
		return [] (Shell &shell __attribute__((unused)),
				const std::vector<std::string> &arguments __attribute__((unused)))
				-> const std::set<std::string> {
			return std::set<std::string>{};
		};
	}

	/**
	 * Add a command to the list of commands in this container.
	 *
	 * @param[in] context Shell context in which this command is
	 *                    available.
	 * @param[in] flags Shell flags that must be set for this command
	 *                  to be available.
	 * @param[in] name Name of the command as a std::vector of flash
	 *                 strings.
	 * @param[in] arguments Help text for arguments that the command
	 *                      accepts as a std::vector of flash strings
	 *                      (use "<" to indicate a required argument).
	 * @param[in] function Function to be used when the command is
	 *                     executed.
	 * @param[in] arg_function Function to be used to perform argument
	 *                         completions for this command.
	 */
	void add_command(unsigned int context, unsigned int flags,
			const flash_string_vector &name, const flash_string_vector &arguments,
			command_function function, argument_completion_function arg_function);
	/**
	 * Execute a command for a Shell if it exists in the specified
	 * context and with the specified flags.
	 *
	 * @param[in] shell Shell that is executing the command.
	 * @param[in] command_line Command line as a space-delimited
	 *                         list of strings.
	 * @return An object describing the result of the command execution
	 *         operation.
	 */
	Execution execute_command(Shell &shell, const std::list<std::string> &command_line);
	/**
	 * Complete a partial command for a Shell if it exists in the
	 * specified context and with the specified flags.
	 *
	 * @param[in] shell Shell that is completing the command.
	 * @param[in] command_line Command line as a space-delimited list
	 *                         of strings.
	 * @return An object describing the result of the command
	 *         completion operation.
	 */
	Completion complete_command(Shell &shell, const std::list<std::string> &command_line);

private:
	/**
	 * Command for execution on a Shell.
	 */
	class Command {
	public:
		/**
		 * Create a command for execution on a Shell.
		 *
		 * @param[in] context Shell context in which this command is
		 *                    available.
		 * @param[in] flags Shell flags that must be set for this command
		 *                  to be available.
		 * @param[in] name Name of the command as a std::vector of flash
		 *                 strings.
		 * @param[in] arguments Help text for arguments that the command
		 *                      accepts as a std::vector of flash strings
		 *                      (use "<" to indicate a required argument).
		 * @param[in] function Function to be used when the command is
		 *                     executed.
		 * @param[in] arg_function Function to be used to perform argument
		 *                         completions for this command.
		 */
		Command(unsigned int context, unsigned int flags,
				const flash_string_vector name, const flash_string_vector arguments,
				command_function function, argument_completion_function arg_function);
		~Command();

		/**
		 * Determine the minimum number of arguments for this command
		 * based on the help text for the arguments that begin with the
		 * "<" character.
		 *
		 * @return The minimum number of arguments for this command.
		 */
		size_t minimum_arguments() const;
		/**
		 * Determine the maximum number of arguments for this command
		 * based on the length of help text for the arguments.
		 *
		 * @return The maximum number of arguments for this command.
		 */
		size_t maximum_arguments() const;

		unsigned int context_; /*!< Shell context in which this command is available. */
		unsigned int flags_; /*!< Shell flags that must be set for this command to be available. */
		const flash_string_vector name_; /*!< Name of the command as a std::vector of flash strings. */
		const flash_string_vector arguments_; /*!< Help text for arguments that the command accepts as a std::vector of flash strings. */
		command_function function_; /*!< Function to be used when the command is executed. */
		argument_completion_function arg_function_; /*!< Function to be used to perform argument completions for this command. */
	};

	/**
	 * Result of a command find operation.
	 *
	 * Each matching command is returned in a map grouped by size.
	 */
	struct Match {
		std::multimap<size_t,const Command*> exact; /*!< Commands that match the command line exactly, grouped by the size of the command names. */
		std::multimap<size_t,const Command*> partial; /*!< Commands that the command line partially matches, grouped by the size of the command names. */
	};

	/**
	 * Find commands by matching them against the command line.
	 *
	 * @param[in] shell Shell that is accessing commands.
	 * @param[in] command_line Command line as a space-delimited list
	 *                         of strings.
	 * @return An object describing the result of the command find operation.
	 */
	Match find_command(Shell &shell, const std::list<std::string> &command_line);

	std::list<Command> commands_; /*!< Commands stored in this container. */
};

class Shell: public std::enable_shared_from_this<Shell>, public uuid::log::Handler, public ::Print {
public:
	static constexpr size_t MAX_COMMAND_LINE_LENGTH = 80;
	static constexpr size_t MAX_LOG_MESSAGES = 20;

	using password_function = std::function<void(Shell &shell, bool completed, const std::string &password)>;
	using delay_function = std::function<void(Shell &shell)>;

	~Shell() override;

	static void loop_all();

	void start();
	virtual void operator<<(std::shared_ptr<uuid::log::Message> message);
	uuid::log::Level get_log_level() const;
	void set_log_level(uuid::log::Level level);
	void loop_one();
	bool running() const;
	void stop();

	static inline const uuid::log::Logger& logger() { return logger_; }

	inline unsigned int context() const {
		if (!context_.empty()) {
			return context_.back();
		} else {
			return 0;
		}
	}
	inline void enter_context(unsigned int context) {
		context_.emplace_back(context);
	}
	virtual bool exit_context() {
		if (context_.size() > 1) {
			context_.pop_back();
			return true;
		} else {
			return false;
		}
	}

	inline void add_flags(unsigned int flags) { flags_ |= flags; }
	inline bool has_flags(unsigned int flags) const { return (flags_ & flags) == flags; }
	inline void remove_flags(unsigned int flags) { flags_ &= ~flags; }

	void enter_password(const __FlashStringHelper *prompt, password_function function);

	/**
	 * Stop executing anything on this shell for a period of time.
	 *
	 * There is an assumption that 2^64 milliseconds uptime will always
	 * be enough time for this delay process.
	 *
	 * @param[in] ms Time in milliseconds to delay execution for.
	 * @param[in] function Function to be executed at a future time,
	 *                     prior to resuming normal execution.
	 */
	void delay_for(unsigned long ms, delay_function function);

	/**
	 * Stop executing anything on this shell until a future time is
	 * reached.
	 *
	 * There is an assumption that 2^64 milliseconds uptime will always
	 * be enough time for this delay process.
	 *
	 * The reference time is uuid::get_uptime_ms().
	 *
	 * @param[in] ms Uptime in the future (in milliseconds) when the
	 *               function should be executed.
	 * @param[in] function Function to be executed at a future time,
	 *                     prior to resuming normal execution.
	 */
	void delay_until(uint64_t ms, delay_function function);

	using ::Print::print;
	size_t print(const std::string &data);
	using ::Print::println;
	size_t println(const std::string &data);
	size_t printf(const char *format, ...) /* __attribute__((format(printf, 2, 3))) */;
	size_t printf(const __FlashStringHelper *format, ...) /* __attribute__((format(printf, 2, 3))) */;
	size_t printfln(const char *format, ...) /* __attribute__((format (printf, 2, 3))) */;
	size_t printfln(const __FlashStringHelper *format, ...) /* __attribute__((format(printf, 2, 3))) */;

protected:
	Shell() = default;
	Shell(std::shared_ptr<Commands> commands, unsigned int context, unsigned int flags = 0);

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
	std::string format_line(const std::list<std::string> &items);

private:
	enum class Mode : uint8_t {
		NORMAL,
		PASSWORD,
		DELAY,
	};

	class ModeData {
	public:
		virtual ~ModeData() = default;

	protected:
		ModeData() = default;
	};

	class PasswordData: public ModeData {
	public:
		PasswordData(const __FlashStringHelper *password_prompt, password_function password_function);
		~PasswordData() override = default;

		const __FlashStringHelper *password_prompt_;
		password_function password_function_;
	};

	class DelayData: public ModeData {
	public:
		DelayData(uint64_t delay_time, delay_function delay_function);
		~DelayData() override = default;

		uint64_t delay_time_;
		delay_function delay_function_;
	};

	class QueuedLogMessage {
	public:
		QueuedLogMessage(unsigned long id, std::shared_ptr<uuid::log::Message> content);
		~QueuedLogMessage() = default;

		const unsigned long id_;
		const std::shared_ptr<const uuid::log::Message> content_;
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

	size_t vprintf(const char *format, va_list ap);
	size_t vprintf(const __FlashStringHelper *format, va_list ap);

	static const uuid::log::Logger logger_;
	static std::set<std::shared_ptr<Shell>> shells_;

	std::shared_ptr<Commands> commands_;
	std::deque<unsigned int> context_;
	unsigned int flags_ = 0;
	unsigned long log_message_id_ = 0;
	std::list<QueuedLogMessage> log_messages_;
	std::string line_buffer_;
	unsigned char previous_ = 0;
	Mode mode_ = Mode::NORMAL;
	std::unique_ptr<ModeData> mode_data_ = nullptr;
	bool stopped_ = false;
	bool prompt_displayed_ = false;
};

class StreamConsole: virtual public Shell {
public:
	StreamConsole(std::shared_ptr<Commands> commands, Stream &stream, unsigned int context, unsigned int flags = 0);
	~StreamConsole() override = default;

	size_t write(uint8_t data) override;
	size_t write(const uint8_t *buffer, size_t size) override;

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
