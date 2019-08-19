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

/**
 * Console shell.
 *
 * - <a href="https://github.com/nomis/mcu-uuid-console/">Git Repository</a>
 */
namespace console {

class Shell;

/**
 * Container of commands for use by a Shell.
 *
 * These should normally be stored in a std::shared_ptr and reused.
 *
 * @since 0.1.0
 */
class Commands {
public:
	/**
	 * Result of a command completion operation.
	 *
	 * Each space-delimited parameter is a separate string.
	 *
	 * @since 0.1.0
	 */
	struct Completion {
		std::list<std::list<std::string>> help; /*!< Suggestions for matching commands. @since 0.1.0 */
		std::list<std::string> replacement; /*!< Replacement matching full or partial command string. @since 0.1.0 */
	};

	/**
	 * Result of a command execution operation.
	 *
	 * @since 0.1.0
	 */
	struct Execution {
		const __FlashStringHelper *error; /*!< Error message if the command could not be executed. @since 0.1.0 */
	};

	/**
	 * Function to handle a command.
	 *
	 * @param[in] shell Shell instance that is executing the command.
	 * @param[in] arguments Command line arguments.
	 * @since 0.1.0
	 */
	using command_function = std::function<void(Shell &shell, const std::vector<std::string> &arguments)>;
	/**
	 * Function to obtain completions for a command line.
	 *
	 * This is a list instead of set so that a custom sort order can be
	 * used. It should normally be sorted lexicographically so that the
	 * list of options is not confusing.
	 *
	 * @param[in] shell Shell instance that has a command line matching
	 *                  this command.
	 * @param[in] arguments Command line arguments prior to (but
	 *                      excluding) the argument being completed.
	 * @return List of possible values for the next argument on the
	 *         command line.
	 * @since 0.1.0
	 */
	using argument_completion_function = std::function<const std::list<std::string>(Shell &shell, const std::vector<std::string> &arguments)>;

	/**
	 * Construct a new container of commands for use by a Shell.
	 *
	 * This should normally be stored in a std::shared_ptr and reused.
	 *
	 * @since 0.1.0
	 */
	Commands() = default;
	~Commands() = default;

	/**
	 * Add a command with no arguments to the list of commands in this
	 * container.
	 *
	 * @param[in] context Shell context in which this command is
	 *                    available.
	 * @param[in] flags Shell flags that must be set for this command
	 *                  to be available.
	 * @param[in] name Name of the command as a std::vector of flash
	 *                 strings.
	 * @param[in] function Function to be used when the command is
	 *                     executed.
	 * @since 0.1.0
	 */
	void add_command(unsigned int context, unsigned int flags,
			const flash_string_vector &name, command_function function);
	/**
	 * Add a command with arguments to the list of commands in this
	 * container.
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
	 * @since 0.1.0
	 */
	void add_command(unsigned int context, unsigned int flags,
			const flash_string_vector &name, const flash_string_vector &arguments,
			command_function function);
	/**
	 * Add a command with arguments and automatic argument completion
	 * to the list of commands in this container.
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
	 * @since 0.1.0
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
	 * @since 0.1.0
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
	 * @since 0.1.0
	 */
	Completion complete_command(Shell &shell, const std::list<std::string> &command_line);

private:
	/**
	 * Command for execution on a Shell.
	 * @since 0.1.0
	 */
	class Command {
	public:
		/**
		 * Create a command for execution on a Shell.
		 *
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
		 * @since 0.1.0
		 */
		Command(unsigned int flags,
				const flash_string_vector name, const flash_string_vector arguments,
				command_function function, argument_completion_function arg_function);
		~Command();

		/**
		 * Determine the minimum number of arguments for this command
		 * based on the help text for the arguments that begin with the
		 * "<" character.
		 *
		 * @return The minimum number of arguments for this command.
		 * @since 0.1.0
		 */
		size_t minimum_arguments() const;
		/**
		 * Determine the maximum number of arguments for this command
		 * based on the length of help text for the arguments.
		 *
		 * @return The maximum number of arguments for this command.
		 * @since 0.1.0
		 */
		inline size_t maximum_arguments() const { return arguments_.size(); }

		unsigned int flags_; /*!< Shell flags that must be set for this command to be available. @since 0.1.0 */
		const flash_string_vector name_; /*!< Name of the command as a std::vector of flash strings. @since 0.1.0 */
		const flash_string_vector arguments_; /*!< Help text for arguments that the command accepts as a std::vector of flash strings. @since 0.1.0 */
		command_function function_; /*!< Function to be used when the command is executed. @since 0.1.0 */
		argument_completion_function arg_function_; /*!< Function to be used to perform argument completions for this command. @since 0.1.0 */
	};

	/**
	 * Result of a command find operation.
	 *
	 * Each matching command is returned in a map grouped by size.
	 * @since 0.1.0
	 */
	struct Match {
		std::multimap<size_t,const Command*> exact; /*!< Commands that match the command line exactly, grouped by the size of the command names. @since 0.1.0 */
		std::multimap<size_t,const Command*> partial; /*!< Commands that the command line partially matches, grouped by the size of the command names. @since 0.1.0 */
	};

	/**
	 * Find commands by matching them against the command line.
	 *
	 * @param[in] shell Shell that is accessing commands.
	 * @param[in] command_line Command line as a space-delimited list
	 *                         of strings.
	 * @return An object describing the result of the command find
	 *         operation.
	 * @since 0.1.0
	 */
	Match find_command(Shell &shell, const std::list<std::string> &command_line);

	/**
	 * Find the longest common prefix from a shortest match of commands.
	 *
	 * @param[in] commands All commands that matched (at least 2).
	 * @param[in] shortest_match The length of the shortest match.
	 * @param[out] longest_name The longest common prefix as a list of
	 *                          strings.
	 * @return True if the longest common prefix is made up of whole
	 *          components, false if the last part is constructed from
	 *          a partial component.
	 * @since 0.1.0
	 */
	bool find_longest_common_prefix(const std::multimap<size_t,const Command*> &commands, size_t shortest_match, std::list<std::string> &longest_name);

	/**
	 * Find the longest common prefix from a list of potential arguments.
	 *
	 * @param[in] arguments All potential arguments (at least 2).
	 * @return The longest common prefix, which could be empty.
	 * @since 0.1.0
	 */
	std::string find_longest_common_prefix(const std::list<std::string> &arguments);

	std::multimap<unsigned int,Command> commands_; /*!< Commands stored in this container, separated by context. @since 0.1.0 */
};

/**
 * Base class for a command shell.
 *
 * Must be constructed within a std::shared_ptr.
 *
 * Requires a derived class to provide input/output. Derived classes
 * should use virtual inheritance to allow the behaviour to be further
 * extended.
 *
 * @since 0.1.0
 */
class Shell: public std::enable_shared_from_this<Shell>, public uuid::log::Handler, public ::Print {
public:
	static constexpr size_t MAX_COMMAND_LINE_LENGTH = 80; /*!< Maximum length of a command line. @since 0.1.0 */
	static constexpr size_t MAX_LOG_MESSAGES = 20; /*!< Maximum number of log messages to buffer before they are output. @since 0.1.0 */

	/**
	 * Function to handle the response to a password entry prompt.
	 *
	 * @param[in] shell Shell instance where the password entry prompt
	 *                  was requested.
	 * @param[in] completed Password entry at the prompt was either
	 *                      completed (true) or aborted (false).
	 * @param[in] password Password entered at the prompt (may be
	 *                     empty).
	 * @since 0.1.0
	 */
	using password_function = std::function<void(Shell &shell, bool completed, const std::string &password)>;
	/**
	 * Function to handle the end of an execution delay.
	 *
	 * @param[in] shell Shell instance where execution was delayed.
	 * @since 0.1.0
	 */
	using delay_function = std::function<void(Shell &shell)>;

	~Shell() override;

	/**
	 * Loop through all registered shell objects.
	 *
	 * Call loop_one() on every Shell (if it has not been stopped).
	 * Any Shell that is stopped is then unregistered.
	 *
	 * @since 0.1.0
	 */
	static void loop_all();

	/**
	 * Perform startup process for this shell.
	 *
	 * Register as a uuid::log::Handler at the uuid::log::Level::NOTICE
	 * log level, output the banner and	register this Shell with the
	 * loop_all() set.
	 *
	 * The started() function will be called after startup is complete.
	 *
	 * Do not call this function more than once.
	 * Do not call this function from a static initializer.
	 *
	 * @since 0.1.0
	 */
	void start();
	/**
	 * Add a new log message.
	 *
	 * This will be put in a queue for output at the next loop_one()
	 * process. The queue has a maximum size of maximum_log_messages()
	 * and will discard the oldest message first.
	 *
	 * @param[in] message New log message, shared by all handlers.
	 * @since 0.1.0
	 */
	virtual void operator<<(std::shared_ptr<uuid::log::Message> message);
	/**
	 * Get the current log level.
	 *
	 * This only affects newly received log messages, not messages that
	 * have already been queued.
	 *
	 * @return The current log level.
	 * @since 0.1.0
	 */
	uuid::log::Level get_log_level() const;
	/**
	 * Set the current log level.
	 *
	 * This only affects newly received log messages, not messages that
	 * have already been queued.
	 *
	 * @param[in] level Minimum log level that the shell will receive
	 *                  messages for.
	 * @since 0.1.0
	 */
	void set_log_level(uuid::log::Level level);
	/**
	 * Perform one execution step of this shell.
	 *
	 * Depending on the current mode, either read input characters and
	 * process them or check if an execution delay has passed.
	 *
	 * @since 0.1.0
	 */
	void loop_one();
	/**
	 * Determine if this shell is still running.
	 *
	 * @return Running status of this Shell, false if the Shell has
	 *         been stopped using stop().
	 * @since 0.1.0
	 */
	bool running() const;
	/**
	 * Stop this shell from running.
	 *
	 * It is not possible to restart the Shell, which must be destroyed
	 * after it has been stopped.
	 *
	 * @since 0.1.0
	 */
	void stop();

	/**
	 * Get the built-in uuid::log::Logger instance for shells.
	 *
	 * @return Logger instance.
	 * @since 0.1.0
	 */
	static inline const uuid::log::Logger& logger() { return logger_; }

	/**
	 * Get the context at the top of the stack.
	 *
	 * The current context affects which commands are available.
	 *
	 * @return Current shell context.
	 * @since 0.1.0
	 */
	inline unsigned int context() const {
		if (!context_.empty()) {
			return context_.back();
		} else {
			return 0;
		}
	}
	/**
	 * Push a new context onto the stack.
	 *
	 * The current context affects which commands are available.
	 *
	 * @param[in] context New context.
	 * @since 0.1.0
	 */
	inline void enter_context(unsigned int context) {
		context_.emplace_back(context);
	}
	/**
	 * Pop a context off the stack.
	 *
	 * The current context affects which commands are available.
	 *
	 * @return True if there was a context to be exited, false if there
	 *         are no more contexts to exit.
	 * @since 0.1.0
	 */
	virtual bool exit_context() {
		if (context_.size() > 1) {
			context_.pop_back();
			return true;
		} else {
			return false;
		}
	}

	/**
	 * Add one or more flags to the current flags.
	 *
	 * Flags are not affected by execution context. The current flags
	 * affect which commands are available (for access control).
	 *
	 * @param[in] flags Flag bits to add.
	 * @since 0.1.0
	 */
	inline void add_flags(unsigned int flags) { flags_ |= flags; }
	/**
	 * Check if the current flags include all of the specified flags.
	 *
	 * Flags are not affected by execution context. The current flags
	 * affect which commands are available (for access control).
	 *
	 * @param[in] flags Flag bits to check for.
	 * @return True if the current flags includes all of the specified
	 *         flags, otherwise false.
	 * @since 0.1.0
	 */
	inline bool has_flags(unsigned int flags) const { return (flags_ & flags) == flags; }
	/**
	 * Remove one or more flags from the current flags.
	 *
	 * Flags are not affected by execution context. The current flags
	 * affect which commands are available (for access control).
	 *
	 * @param[in] flags Flag bits to remove.
	 * @since 0.1.0
	 */
	inline void remove_flags(unsigned int flags) { flags_ &= ~flags; }

	/**
	 * Prompt for a password to be entered on this shell.
	 *
	 * Password entry is not visible and can be interrupted by the
	 * user.
	 *
	 * @param[in] prompt Message to display prompting for password
	 *                   input.
	 * @param[in] function Function to be executed after the password
	 *                     has been entered prior to resuming normal
	 *                     execution.
	 * @since 0.1.0
	 */
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
	 * @since 0.1.0
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
	 * @since 0.1.0
	 */
	void delay_until(uint64_t ms, delay_function function);

	using ::Print::print; /*!< Include standard Arduino print() functions. */
	/**
	 * Output a string.
	 *
	 * @param[in] data String to be output.
	 * @return The number of bytes that were output.
	 * @since 0.1.0
	 */
	size_t print(const std::string &data);
	using ::Print::println; /*!< Include standard Arduino println() functions. */
	/**
	 * Output a string followed by CRLF end of line characters.
	 *
	 * @param[in] data String to be output.
	 * @return The number of bytes that were output, including CRLF.
	 * @since 0.1.0
	 */
	size_t println(const std::string &data);
	/**
	 * Output a message.
	 *
	 * @param[in] format Format string.
	 * @param[in] ... Format string arguments.
	 * @return The number of bytes that were output.
	 * @since 0.1.0
	 */
	size_t printf(const char *format, ...) /* __attribute__((format(printf, 2, 3))) */;
	/**
	 * Output a message.
	 *
	 * @param[in] format Format string (flash string).
	 * @param[in] ... Format string arguments.
	 * @return The number of bytes that were output.
	 * @since 0.1.0
	 */
	size_t printf(const __FlashStringHelper *format, ...) /* __attribute__((format(printf, 2, 3))) */;
	/**
	 * Output a message followed by CRLF end of line characters.
	 *
	 * @param[in] format Format string.
	 * @param[in] ... Format string arguments.
	 * @return The number of bytes that were output, including CRLF.
	 * @since 0.1.0
	 */
	size_t printfln(const char *format, ...) /* __attribute__((format (printf, 2, 3))) */;
	/**
	 * Output a message followed by CRLF end of line characters.
	 *
	 * @param[in] format Format string (flash string).
	 * @param[in] ... Format string arguments.
	 * @return The number of bytes that were output, including CRLF.
	 * @since 0.1.0
	 */
	size_t printfln(const __FlashStringHelper *format, ...) /* __attribute__((format(printf, 2, 3))) */;

protected:
	/**
	 * Default constructor used by intermediate derived classes for
	 * multiple inheritance.
	 *
	 * This does not initialise the shell completely so the outer
	 * derived class must call the public constructor or there will be
	 * no commands. Does not put any default context on the stack.
	 *
	 * @since 0.1.0
	 */
	Shell() = default;
	/**
	 * Create a new Shell with the given commands, default context and
	 * initial flags.
	 *
	 * The default context is put on the stack and cannot be removed.
	 *
	 * @param[in] commands Commands available for execution in this shell.
	 * @param[in] context Default context for the shell.
	 * @param[in] flags Initial flags for the shell.
	 * @since 0.1.0
	 */
	Shell(std::shared_ptr<Commands> commands, unsigned int context, unsigned int flags = 0);

	/**
	 * Get the maximum length of a command line.
	 *
	 * Defaults to Shell::MAX_COMMAND_LINE_LENGTH.
	 *
	 * @return The maximum length of a command line in bytes.
	 * @since 0.1.0
	 */
	virtual size_t maximum_command_line_length() const;
	/**
	 * Get the maximum number of queued log messages.
	 *
	 * Defaults to Shell::MAX_LOG_MESSAGES.
	 *
	 * @return The maximum number of queued log messages.
	 * @since 0.1.0
	 */
	virtual size_t maximum_log_messages() const;
	/**
	 * Output ANSI escape sequence to erase the current line.
	 *
	 * @since 0.1.0
	 */
	virtual void erase_current_line();
	/**
	 * Output ANSI escape sequence to erase characters.
	 *
	 * @param[in] count The number of characters to erase.
	 * @since 0.1.0
	 */
	virtual void erase_characters(size_t count);

	/**
	 * Startup complete event.
	 *
	 * The shell is ready to start executing commands.
	 * @since 0.1.0
	 */
	virtual void started();
	/**
	 * Output the startup banner.
	 *
	 * There is no default banner.
	 * @since 0.1.0
	 */
	virtual void display_banner();
	/**
	 * Get the hostname to be included in the command prompt.
	 *
	 * Defaults to "".
	 *
	 * @return The current hostname of the device, or an empty string
	 *         for no hostname.
	 * @since 0.1.0
	 */
	virtual std::string hostname_text();
	/**
	 * Get the text indicating the current context, to be included in
	 * the command prompt.
	 *
	 * Defaults to "".
	 *
	 * @return Text indicating the current context, or an empty string
	 *         for no context description.
	 * @since 0.1.0
	 */
	virtual std::string context_text();
	/**
	 * Get the prefix to be used at the beginning of the command
	 * prompt.
	 *
	 * Defaults to "".
	 *
	 * @return Text for the beginning of the command prompt, or an
	 *         empty string.
	 * @since 0.1.0
	 */
	virtual std::string prompt_prefix();
	/**
	 * Get the prefix to be used at the end of the command prompt.
	 *
	 * Defaults to "$".
	 *
	 * @return Text for the end of the command prompt, or an empty
	 *         string.
	 * @since 0.1.0
	 */
	virtual std::string prompt_suffix();
	/**
	 * The end of transmission character has been received.
	 *
	 * A command can be invoked using invoke_command(). All other
	 * actions may result in an inconsistent display of the command
	 * prompt. If the shell is stopped without invoking a command then
	 * a blank line should usually be printed first.
	 *
	 * There is no default action.
	 * @since 0.1.0
	 */
	virtual void end_of_transmission();
	/**
	 * Stop request event.
	 *
	 * The shell is going to stop executing.
	 * @since 0.1.0
	 */
	virtual void stopped();

	/**
	 * Invoke a command on the shell.
	 *
	 * This will output a prompt with the provided command line and
	 * then try to execute it.
	 *
	 * Intended for use from end_of_transmission() to execute an "exit"
	 * or "logout" command.
	 *
	 * @param[in] line The command line to be executed.
	 * @since 0.1.0
	 */
	void invoke_command(std::string line);

	/**
	 * Parse a command line into separate parameters using built-in
	 * escaping rules.
	 *
	 * @param[in] line Command line to parse.
	 * @return A list of strings, one per command line parameter.
	 * @since 0.1.0
	 */
	std::list<std::string> parse_line(const std::string &line);
	/**
	 * Format a command line from separate parameters using built-in
	 * escaping rules.
	 *
	 * @param[in] items Command line parameters.
	 * @return A command line, with escaping of characters sufficient
	 *         to reproduce the same command line parameters when
	 *         parsed.
	 * @since 0.1.0
	 */
	std::string format_line(const std::list<std::string> &items);

private:
	/**
	 * Current mode of the shell.
	 *
	 * @since 0.1.0
	 */
	enum class Mode : uint8_t {
		NORMAL, /*!< Normal command execution. @since 0.1.0 */
		PASSWORD, /*!< Password entry prompt. @since 0.1.0 */
		DELAY, /*!< Delay execution until a future time. @since 0.1.0 */
	};

	/**
	 * Base class of data for a shell mode.
	 *
	 * @since 0.1.0
	 */
	class ModeData {
	public:
		virtual ~ModeData() = default;

	protected:
		ModeData() = default;
	};

	/**
	 * Data for the Mode::PASSWORD shell mode.
	 *
	 * @since 0.1.0
	 */
	class PasswordData: public ModeData {
	public:
		/**
		 * Create Mode::PASSWORD shell mode data.
		 *
		 * @param[in] password_prompt Message to display prompting for
		 *                            password input.
		 * @param[in] password_function Function to be executed after
		 *                              the password has been entered
		 *                              prior to resuming normal
		 *                              execution.
		 * @since 0.1.0
		 */
		PasswordData(const __FlashStringHelper *password_prompt, password_function password_function);
		~PasswordData() override = default;

		const __FlashStringHelper *password_prompt_; /*!< Prompt requesting password input. */
		password_function password_function_; /*!< Function to execute after password entry. */
	};

	/**
	 * Data for the Mode::DELAY shell mode.
	 *
	 * @since 0.1.0
	 */
	class DelayData: public ModeData {
	public:
		/**
		 * Create Mode::DELAY shell mode data.
		 *
		 * @param[in] delay_time Uptime in the future (in milliseconds)
		 *                       when the function should be executed.
		 * @param[in] delay_function Function to be executed at a
		 *                           future time, prior to resuming
		 *                           normal execution.
		 * @since 0.1.0
		 */
		DelayData(uint64_t delay_time, delay_function delay_function);
		~DelayData() override = default;

		uint64_t delay_time_; /*!< Future uptime to resume execution (in milliseconds). */
		delay_function delay_function_; /*!< Function execute after delay. */
	};

	/**
	 * Log message that has been queued.
	 *
	 * Contains an identifier sequence to indicate when log messages
	 * could not be output because the queue discarded one or more
	 * messages.
	 *
	 * @since 0.1.0
	 */
	class QueuedLogMessage {
	public:
		/**
		 * Create a queued log message.
		 *
		 * @param[in] id Identifier to use for the log message on the queue.
		 * @param[in] content Log message content.
		 * @since 0.1.0
		 */
		QueuedLogMessage(unsigned long id, std::shared_ptr<uuid::log::Message> content);
		~QueuedLogMessage() = default;

		const unsigned long id_; /*!< Sequential identifier for this log message. @since 0.1.0 */
		const std::shared_ptr<const uuid::log::Message> content_; /*!< Log message content. @since 0.1.0 */
	};

	Shell(const Shell&) = delete;
	Shell& operator=(const Shell&) = delete;

	/**
	 * Perform one execution step in Mode::NORMAL mode.
	 *
	 * Read characters and execute commands or invoke tab completion.
	 *
	 * @since 0.1.0
	 */
	void loop_normal();
	/**
	 * Perform one execution step in Mode::PASSWORD mode.
	 *
	 * Read characters until interrupted or password entry is complete.
	 *
	 * @since 0.1.0
	 */
	void loop_password();
	/**
	 * Perform one execution step in Mode::DELAY mode.
	 *
	 * Wait until the delay uptime is reached.
	 *
	 * @since 0.1.0
	 */
	void loop_delay();

	/**
	 * Read one character from the available input.
	 *
	 * @return An unsigned char if input is available, otherwise -1.
	 * @since 0.1.0
	 */
	virtual int read_one_char() = 0;

	/**
	 * Output a prompt on the shell.
	 *
	 * Based on the current mode this will output the appropriate text,
	 * e.g. the command prompt with the current command line (for
	 * Mode::NORMAL mode).
	 *
	 * @since 0.1.0
	 */
	void display_prompt();
	/**
	 * Output queued log messages for this shell.
	 *
	 * @since 0.1.0
	 */
	void output_logs();
	/**
	 * Try to execute a command with the current command line.
	 *
	 * @since 0.1.0
	 */
	void process_command();
	/**
	 * Try to complete a command from the current command line.
	 *
	 * @since 0.1.0
	 */
	void process_completion();
	/**
	 * Finish password entry.
	 *
	 * The entered password is in the command line buffer.
	 *
	 * @param[in] completed Password entry at the prompt was either
	 *                      completed (true) or aborted (false).
	 * @since 0.1.0
	 */
	void process_password(bool completed);

	/**
	 * Delete a word from the command line buffer.
	 *
	 * @param[in] display True if output to erased characters is
	 *                    required, false for no output.
	 * @since 0.1.0
	 */
	void delete_buffer_word(bool display);

	/**
	 * Output a message.
	 *
	 * @param[in] format Format string.
	 * @param[in] ap Variable arguments pointer for format string.
	 * @return The number of bytes that were output.
	 * @since 0.1.0
	 */
	size_t vprintf(const char *format, va_list ap);
	/**
	 * Output a message.
	 *
	 * @param[in] format Format string (flash string).
	 * @param[in] ap Variable arguments pointer for format string.
	 * @return The number of bytes that were output.
	 * @since 0.1.0
	 */
	size_t vprintf(const __FlashStringHelper *format, va_list ap);

	static const uuid::log::Logger logger_; /*!< uuid::log::Logger instance for shells. @since 0.1.0 */
	static std::set<std::shared_ptr<Shell>> shells_; /*!< Registered running shells to be executed. @since 0.1.0 */

	std::shared_ptr<Commands> commands_; /*!< Commands available for execution in this shell. @since 0.1.0 */
	std::deque<unsigned int> context_; /*!< Context stack for this shell. Should never be empty. @since 0.1.0 */
	unsigned int flags_ = 0; /*!< Current flags for this shell. Affects which commands are available. @since 0.1.0 */
	unsigned long log_message_id_ = 0; /*!< The next identifier to use for queued log messages. @since 0.1.0 */
	std::list<QueuedLogMessage> log_messages_; /*!< Queued log messages, in the order they were received. @since 0.1.0 */
	std::string line_buffer_; /*!< Command line buffer. Limited to maximum_command_line_length() bytes. @since 0.1.0 */
	unsigned char previous_ = 0; /*!< Previous character that was entered on the command line. Used to detect CRLF line endings. @since 0.1.0 */
	Mode mode_ = Mode::NORMAL; /*!< Current execution mode. @since 0.1.0 */
	std::unique_ptr<ModeData> mode_data_ = nullptr; /*!< Data associated with the current execution mode. @since 0.1.0 */
	bool stopped_ = false; /*!< Indicates that the shell has been stopped. @since 0.1.0 */
	bool prompt_displayed_ = false; /*!< Indicates that a command prompt has been displayed, so that the output of invoke_command() is correct. @since 0.1.0 */
};

/**
 * A command shell console using a Stream for input/output.
 *
 * Must be constructed within a std::shared_ptr.
 *
 * @since 0.1.0
 */
class StreamConsole: virtual public Shell {
public:
	/**
	 * Create a new StreamConsole shell with the given commands,
	 * default context and initial flags.
	 *
	 * The default context is put on the stack and cannot be removed.
	 *
	 * @param[in] commands Commands available for execution in this shell.
	 * @param[in] stream Stream used for the input/output of this shell.
	 * @param[in] context Default context for the shell.
	 * @param[in] flags Initial flags for the shell.
	 * @since 0.1.0
	 */
	StreamConsole(std::shared_ptr<Commands> commands, Stream &stream, unsigned int context, unsigned int flags = 0);
	~StreamConsole() override = default;

	/**
	 * Write one byte to the output stream.
	 *
	 * @param[in] data Data to be output.
	 * @return The number of bytes that were output.
	 * @since 0.1.0
	 */
	size_t write(uint8_t data) override;
	/**
	 * Write an array of bytes to the output stream.
	 *
	 * @param[in] buffer Buffer to be output.
	 * @param[in] size Length of the buffer.
	 * @return The number of bytes that were output.
	 * @since 0.1.0
	 */
	size_t write(const uint8_t *buffer, size_t size) override;

protected:
	/**
	 * Constructor used by intermediate derived classes for multiple
	 * inheritance.
	 *
	 * This does not initialise the shell completely so the outer
	 * derived class must call the public constructor or there will be
	 * no commands. Does not put any default context on the stack.
	 *
	 * @since 0.1.0
	 */
	StreamConsole(Stream &stream);

private:
	StreamConsole(const StreamConsole&) = delete;
	StreamConsole& operator=(const StreamConsole&) = delete;

	/**
	 * Read one character from the input stream.
	 *
	 * @return An unsigned char if input is available, otherwise -1.
	 * @since 0.1.0
	 */
	int read_one_char() override;

	Stream &stream_; /*!< Stream used for the input/output of this shell. @since 0.1.0 */
};

} // namespace console

} // namespace uuid

#endif
