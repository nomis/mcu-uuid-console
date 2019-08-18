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

#include <Arduino.h>
#include <unity.h>

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <uuid/console.h>

using ::uuid::flash_string_vector;
using ::uuid::console::Commands;
using ::uuid::console::Shell;

class DummyShell: public Shell {
public:
	DummyShell() : Shell(std::make_shared<Commands>(), 0, 0) {};
	~DummyShell() override = default;

	using Shell::parse_line;
	using Shell::format_line;

protected:
	int read_one_char() { return '\n'; };
	size_t write(uint8_t data __attribute__((unused))) override { return 1; }
	size_t write(const uint8_t *buffer __attribute__((unused)), size_t size) override { return size; }
};

namespace uuid {

uint64_t get_uptime_ms() {
	static uint64_t millis = 0;
	return ++millis;
}

} // namespace uuid

static Commands commands;
static DummyShell shell;
static std::string run;

/**
 * Completion with an empty command line returns all commands (but the shell does not allow this).
 */
static void test_completion0() {
	run = "";
	auto completion = commands.complete_command(shell, shell.parse_line(""));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(17, completion.help.size());
}

/**
 * An empty command line is not executed.
 */
static void test_execution0() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line(""));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with only one potential match (that is a prefix for multiple longer commands)
 * should be completed up to that point and no further.
 */
static void test_completion1a() {
	auto completion = commands.complete_command(shell, shell.parse_line("sh"));

	TEST_ASSERT_EQUAL_STRING("show", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution1a() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("sh"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * An exact matching command that is a prefix for multiple longer commands should
 * append a space and return them.
 */
static void test_completion1b() {
	auto completion = commands.complete_command(shell, shell.parse_line("show"));

	TEST_ASSERT_EQUAL_STRING("show ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed.
 */
static void test_execution1b() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("show"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show", run.c_str());
}

/**
 * An exact matching command that is a prefix (with a space) for multiple longer
 * commands should complete as far as possible and return the longer commands.
 */
static void test_completion1c() {
	auto completion = commands.complete_command(shell, shell.parse_line("show "));

	TEST_ASSERT_EQUAL_STRING("show thing", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution1c() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("show "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show", run.c_str());
}

/**
 * A partial matching command that is a prefix for multiple longer commands
 * should complete as far as possible and return the longer commands.
 */
static void test_completion1d() {
	auto completion = commands.complete_command(shell, shell.parse_line("show th"));

	TEST_ASSERT_EQUAL_STRING("show thing", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands that have longer matches cannot have arguments so they
 * will fail to find a command if arguments are used.
 */
static void test_execution1d() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("show th"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial matching command that is a prefix for multiple longer commands
 * and is already complete as far as possible will return the longer commands.
 */
static void test_completion1e() {
	auto completion = commands.complete_command(shell, shell.parse_line("show thing"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands that have longer matches cannot have arguments so they
 * will fail to find a command if arguments are used.
 */
static void test_execution1e() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("show thing"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion1f() {
	auto completion = commands.complete_command(shell, shell.parse_line("show thing1"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution1f() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("show thing1"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show thing1", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion1g() {
	auto completion = commands.complete_command(shell, shell.parse_line("show thing1 "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}
/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution1g() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("show thing1 "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show thing1", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2a() {
	auto completion = commands.complete_command(shell, shell.parse_line("cons"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2a() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("cons"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2b() {
	auto completion = commands.complete_command(shell, shell.parse_line("console"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2b() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2c() {
	auto completion = commands.complete_command(shell, shell.parse_line("console "));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2c() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console "));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2d() {
	auto completion = commands.complete_command(shell, shell.parse_line("console l"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2d() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console l"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2e() {
	auto completion = commands.complete_command(shell, shell.parse_line("console log"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2e() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console log"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command (with a trailing space) and multiple potential longer matches
 * where the command line is the common prefix (and is not itself a command) should
 * return the other longer commands.
 */
static void test_completion2f() {
	auto completion = commands.complete_command(shell, shell.parse_line("console log "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("err", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("warning", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("info", shell.format_line(*it++).c_str());
	}
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2f() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console log "));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command that doesn't match anything returns no replacements or help.
 */
static void test_completion2g() {
	auto completion = commands.complete_command(shell, shell.parse_line("console log a"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2g() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console log a"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A single partial match should be auto-completed to the end of the command
 * (with no trailing space).
 */
static void test_completion2h() {
	auto completion = commands.complete_command(shell, shell.parse_line("console log in"));

	TEST_ASSERT_EQUAL_STRING("console log info", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2h() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console log in"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion2i() {
	auto completion = commands.complete_command(shell, shell.parse_line("console log info"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution2i() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console log info"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("console log info", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion2j() {
	auto completion = commands.complete_command(shell, shell.parse_line("console log info "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution2j() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("console log info "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("console log info", run.c_str());
}

/**
 * A single partial match should be auto-completed to the end of the command
 * (with no trailing space).
 */
static void test_completion3a() {
	auto completion = commands.complete_command(shell, shell.parse_line("h"));

	TEST_ASSERT_EQUAL_STRING("help", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution3a() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("h"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion3b() {
	auto completion = commands.complete_command(shell, shell.parse_line("help"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution3b() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("help"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("help", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion3c() {
	auto completion = commands.complete_command(shell, shell.parse_line("help "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution3c() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("help "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("help", run.c_str());
}

/**
 * A partial command with only one potential match (that is a prefix for multiple longer commands)
 * should be completed up to that point and no further.
 */
static void test_completion4a() {
	auto completion = commands.complete_command(shell, shell.parse_line("se"));

	TEST_ASSERT_EQUAL_STRING("set", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution4a() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("se"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * An exact matching command that is a prefix for one longer command (that has
 * no arguments or longer commands) should complete to that longer command.
 */
static void test_completion4b() {
	auto completion = commands.complete_command(shell, shell.parse_line("set"));

	TEST_ASSERT_EQUAL_STRING("set hostname", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution4b() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("set"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("set", run.c_str());
}

/**
 * An exact matching command that is a prefix (with a space) for one longer
 * command (that has no arguments or longer commands) should complete to that
 * longer command without a space.
 */
static void test_completion4c() {
	auto completion = commands.complete_command(shell, shell.parse_line("set "));

	TEST_ASSERT_EQUAL_STRING("set hostname", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution4c() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("set "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("set", run.c_str());
}

/**
 * Partial matches of commands with arguments should complete the command,
 * add a space and provide a list of all the remaining command line arguments.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5a() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a"));

	TEST_ASSERT_EQUAL_STRING("test_a0 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_b"));

	TEST_ASSERT_EQUAL_STRING("test_b1 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> [two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_c"));

	TEST_ASSERT_EQUAL_STRING("test_c2 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_d"));

	TEST_ASSERT_EQUAL_STRING("test_d3 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> <three>", shell.format_line(*it++).c_str());
	}
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution5a() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (without a space) with arguments should add a space
 * and provide a list of all the remaining command line arguments, appending a
 * space if there are arguments remaining.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5b() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0"));

	TEST_ASSERT_EQUAL_STRING("test_a0 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_b1"));

	TEST_ASSERT_EQUAL_STRING("test_b1 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> [two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_c2"));

	TEST_ASSERT_EQUAL_STRING("test_c2 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_d3"));

	TEST_ASSERT_EQUAL_STRING("test_d3 ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> <three>", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5b() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (with a space) with arguments should provide a list of
 * all the remaining command line arguments, appending a space if there are arguments
 * remaining.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5c() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_b1 "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> [two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_c2 "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_d3 "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> <three>", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5c() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (with a space) with arguments should provide a list of
 * all the remaining command line arguments, appending a space if there are arguments
 * remaining.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5d() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 un"));

	TEST_ASSERT_EQUAL_STRING("test_a0 un ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_b1 un"));

	TEST_ASSERT_EQUAL_STRING("test_b1 un ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_c2 un"));

	TEST_ASSERT_EQUAL_STRING("test_c2 un ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_d3 un"));

	TEST_ASSERT_EQUAL_STRING("test_d3 un ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> <three>", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5d() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 un"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 un"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 un"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 un"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (with a space) with arguments should provide a list of
 * all the remaining command line arguments, appending a space if there are arguments
 * remaining.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5e() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 un "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_b1 un "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_c2 un "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> [three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_d3 un "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> <three>", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5e() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 un "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 un "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 un "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 un "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (with a space) with arguments should provide a list of
 * all the remaining command line arguments, appending a space if there are arguments
 * remaining.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5f() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 un deux"));

	TEST_ASSERT_EQUAL_STRING("test_a0 un deux ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_b1 un deux"));

	TEST_ASSERT_EQUAL_STRING("test_b1 un deux ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_c2 un deux"));

	TEST_ASSERT_EQUAL_STRING("test_c2 un deux ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_d3 un deux"));

	TEST_ASSERT_EQUAL_STRING("test_d3 un deux ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<three>", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5f() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 un deux"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 un deux"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 un deux"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 un deux"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (with a space) with arguments should provide a list of
 * all the remaining command line arguments, appending a space if there are arguments
 * remaining.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5g() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 un deux "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_b1 un deux "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_c2 un deux "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", shell.format_line(*it++).c_str());
	}

	completion = commands.complete_command(shell, shell.parse_line("test_d3 un deux "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<three>", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5g() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 un deux "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 un deux "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 un deux "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 un deux "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (with a space) with maximum arguments should do nothing.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5h() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_b1 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_c2 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_d3 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5h() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_d3 un deux trois", run.c_str());
}

/**
 * Exact matches of commands (with a space) with maximum arguments should do nothing,
 * even if there's a space at the end.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5i() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_b1 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_c2 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_d3 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5i() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_d3 un deux trois", run.c_str());
}

/**
 * Exact matches of commands with more than the maximum arguments should do nothing.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5j() {
	auto completion = commands.complete_command(shell, shell.parse_line("test_a0 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_b1 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_c2 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, shell.parse_line("test_d3 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are not executed if they have more than the maximum arguments.
 */
static void test_execution5j() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("test_a0 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_b1 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_c2 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, shell.parse_line("test_d3 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with only one exact match (that is a prefix for multiple longer commands)
 * should be completed up to that point and no further.
 */
static void test_completion6a() {
	auto completion = commands.complete_command(shell, shell.parse_line("ge"));

	TEST_ASSERT_EQUAL_STRING("get", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution6a() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("ge"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * An exact matching command that is a prefix for multiple different longer commands should
 * add a space and return those commands.
 */
static void test_completion6b() {
	auto completion = commands.complete_command(shell, shell.parse_line("get"));

	TEST_ASSERT_EQUAL_STRING("get ", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("hostname", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("uptime", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands are executed.
 */
static void test_execution6b() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("get"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("get", run.c_str());
}

/**
 * An exact matching command with a space that is a prefix for multiple different longer
 * commands should return those commands.
 */
static void test_completion6c() {
	auto completion = commands.complete_command(shell, shell.parse_line("get "));

	TEST_ASSERT_EQUAL_STRING("", shell.format_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("hostname", shell.format_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("uptime", shell.format_line(*it++).c_str());
	}
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution6c() {
	run = "";
	auto execution = commands.execute_command(shell, shell.parse_line("get "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("get", run.c_str());
}

int main(int argc, char *argv[]) {
	commands.add_command(0, 0, flash_string_vector{F("help")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "help";
	});

	commands.add_command(0, 0, flash_string_vector{F("show")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show";
	});

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing1")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing1";
	});

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing2")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing2";
	});

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing3")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing3";
	});

	commands.add_command(0, 0, flash_string_vector{F("get")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "get";
	});

	commands.add_command(0, 0, flash_string_vector{F("get"), F("hostname")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "get hostname";
	});

	commands.add_command(0, 0, flash_string_vector{F("get"), F("uptime")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "get uptime";
	});

	commands.add_command(0, 0, flash_string_vector{F("set")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "set";
	});

	commands.add_command(0, 0, flash_string_vector{F("set"), F("hostname")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "set hostname";
	});

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("err")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log err";
	});

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("warning")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log warning";
	});

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("info")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log info";
	});

	commands.add_command(0, 0, flash_string_vector{F("test_a0")}, flash_string_vector{F("[one]"), F("[two]"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_a0";
		for (auto& argument : arguments) {
			run += " " + argument;
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_b1")}, flash_string_vector{F("<one>"), F("[two]"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_b1";
		for (auto& argument : arguments) {
			run += " " + argument;
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_c2")}, flash_string_vector{F("<one>"), F("<two>"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_c2";
		for (auto& argument : arguments) {
			run += " " + argument;
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_d3")}, flash_string_vector{F("<one>"), F("<two>"), F("<three>")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_d3";
		for (auto& argument : arguments) {
			run += " " + argument;
		}
	});

	UNITY_BEGIN();
	RUN_TEST(test_completion0);
	RUN_TEST(test_execution0);

	RUN_TEST(test_completion1a);
	RUN_TEST(test_completion1b);
	RUN_TEST(test_completion1c);
	RUN_TEST(test_completion1d);
	RUN_TEST(test_completion1e);
	RUN_TEST(test_completion1f);
	RUN_TEST(test_completion1g);
	RUN_TEST(test_execution1a);
	RUN_TEST(test_execution1b);
	RUN_TEST(test_execution1d);
	RUN_TEST(test_execution1c);
	RUN_TEST(test_execution1e);
	RUN_TEST(test_execution1f);
	RUN_TEST(test_execution1g);

	RUN_TEST(test_completion2a);
	RUN_TEST(test_completion2b);
	RUN_TEST(test_completion2c);
	RUN_TEST(test_completion2d);
	RUN_TEST(test_completion2e);
	RUN_TEST(test_completion2f);
	RUN_TEST(test_completion2g);
	RUN_TEST(test_completion2h);
	RUN_TEST(test_completion2i);
	RUN_TEST(test_completion2j);
	RUN_TEST(test_execution2a);
	RUN_TEST(test_execution2b);
	RUN_TEST(test_execution2c);
	RUN_TEST(test_execution2d);
	RUN_TEST(test_execution2e);
	RUN_TEST(test_execution2f);
	RUN_TEST(test_execution2g);
	RUN_TEST(test_execution2h);
	RUN_TEST(test_execution2i);
	RUN_TEST(test_execution2j);

	RUN_TEST(test_completion3a);
	RUN_TEST(test_completion3b);
	RUN_TEST(test_completion3c);
	RUN_TEST(test_execution3a);
	RUN_TEST(test_execution3b);
	RUN_TEST(test_execution3c);

	RUN_TEST(test_completion4a);
	RUN_TEST(test_completion4b);
	RUN_TEST(test_completion4c);
	RUN_TEST(test_execution4a);
	RUN_TEST(test_execution4b);
	RUN_TEST(test_execution4c);

	RUN_TEST(test_completion5a);
	RUN_TEST(test_completion5b);
	RUN_TEST(test_completion5c);
	RUN_TEST(test_completion5d);
	RUN_TEST(test_completion5e);
	RUN_TEST(test_completion5f);
	RUN_TEST(test_completion5g);
	RUN_TEST(test_completion5h);
	RUN_TEST(test_completion5i);
	RUN_TEST(test_completion5j);
	RUN_TEST(test_execution5a);
	RUN_TEST(test_execution5b);
	RUN_TEST(test_execution5c);
	RUN_TEST(test_execution5d);
	RUN_TEST(test_execution5e);
	RUN_TEST(test_execution5f);
	RUN_TEST(test_execution5g);
	RUN_TEST(test_execution5h);
	RUN_TEST(test_execution5i);
	RUN_TEST(test_execution5j);

	RUN_TEST(test_completion6a);
	RUN_TEST(test_completion6b);
	RUN_TEST(test_completion6c);
	RUN_TEST(test_execution6a);
	RUN_TEST(test_execution6b);
	RUN_TEST(test_execution6c);

	return UNITY_END();
}
