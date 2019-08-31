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
using ::uuid::console::CommandLine;
using ::uuid::console::Commands;
using ::uuid::console::Shell;

class DummyShell: public Shell {
public:
	DummyShell(unsigned int context = 0) : Shell(std::make_shared<Commands>(), context, 0) {};
	~DummyShell() override = default;

protected:
	bool available_char() override { return true; }
	int read_one_char() override { return '\n'; };
	int peek_one_char() override { return '\n'; };
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
	auto completion = commands.complete_command(shell, CommandLine(""));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(26, completion.help.size());
}

/**
 * An empty command line is not executed.
 */
static void test_execution0() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine(""));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with only one potential match (that is a prefix for multiple longer commands)
 * should be completed up to that point and no further.
 */
static void test_completion1a() {
	auto completion = commands.complete_command(shell, CommandLine("sh"));

	TEST_ASSERT_EQUAL_STRING("show", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution1a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("sh"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * An exact matching command that is a prefix for multiple longer commands should
 * append a space and return them.
 */
static void test_completion1b() {
	auto completion = commands.complete_command(shell, CommandLine("show"));

	TEST_ASSERT_EQUAL_STRING("show ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed.
 */
static void test_execution1b() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("show"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show", run.c_str());
}

/**
 * An exact matching command that is a prefix (with a space) for multiple longer
 * commands should complete as far as possible and return the longer commands.
 */
static void test_completion1c() {
	auto completion = commands.complete_command(shell, CommandLine("show "));

	TEST_ASSERT_EQUAL_STRING("show thing", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution1c() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("show "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show", run.c_str());
}

/**
 * A partial matching command that is a prefix for multiple longer commands
 * should complete as far as possible and return the longer commands.
 */
static void test_completion1d() {
	auto completion = commands.complete_command(shell, CommandLine("show th"));

	TEST_ASSERT_EQUAL_STRING("show thing", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands that have longer matches cannot have arguments so they
 * will fail to find a command if arguments are used.
 */
static void test_execution1d() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("show th"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial matching command that is a prefix for multiple longer commands
 * and is already complete as far as possible will return the longer commands.
 */
static void test_completion1e() {
	auto completion = commands.complete_command(shell, CommandLine("show thing"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands that have longer matches cannot have arguments so they
 * will fail to find a command if arguments are used.
 */
static void test_execution1e() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("show thing"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion1f() {
	auto completion = commands.complete_command(shell, CommandLine("show thing1"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution1f() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("show thing1"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show thing1", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion1g() {
	auto completion = commands.complete_command(shell, CommandLine("show thing1 "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}
/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution1g() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("show thing1 "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show thing1", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2a() {
	auto completion = commands.complete_command(shell, CommandLine("cons"));

	TEST_ASSERT_EQUAL_STRING("console log ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("cons"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2b() {
	auto completion = commands.complete_command(shell, CommandLine("console"));

	TEST_ASSERT_EQUAL_STRING("console log ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2b() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2c() {
	auto completion = commands.complete_command(shell, CommandLine("console "));

	TEST_ASSERT_EQUAL_STRING("console log ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2c() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console "));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2d() {
	auto completion = commands.complete_command(shell, CommandLine("console l"));

	TEST_ASSERT_EQUAL_STRING("console log ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2d() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console l"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with multiple potential matches with a common prefix (that is not
 * itself a command) should be completed up to that point with a trailing space.
 */
static void test_completion2e() {
	auto completion = commands.complete_command(shell, CommandLine("console log"));

	TEST_ASSERT_EQUAL_STRING("console log ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2e() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console log"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command (with a trailing space) and multiple potential longer matches
 * where the command line is the common prefix (and is not itself a command) should
 * return the other longer commands.
 */
static void test_completion2f() {
	auto completion = commands.complete_command(shell, CommandLine("console log "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("err", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("warning", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("info", (*it++).to_string().c_str());
	}
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2f() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console log "));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command that doesn't match anything returns no replacements or help.
 */
static void test_completion2g() {
	auto completion = commands.complete_command(shell, CommandLine("console log a"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2g() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console log a"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A single partial match should be auto-completed to the end of the command
 * (with no trailing space).
 */
static void test_completion2h() {
	auto completion = commands.complete_command(shell, CommandLine("console log in"));

	TEST_ASSERT_EQUAL_STRING("console log info", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution2h() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console log in"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion2i() {
	auto completion = commands.complete_command(shell, CommandLine("console log info"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution2i() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console log info"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("console log info", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion2j() {
	auto completion = commands.complete_command(shell, CommandLine("console log info "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution2j() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("console log info "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("console log info", run.c_str());
}

/**
 * A single partial match should be auto-completed to the end of the command
 * (with no trailing space).
 */
static void test_completion3a() {
	auto completion = commands.complete_command(shell, CommandLine("h"));

	TEST_ASSERT_EQUAL_STRING("help", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution3a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("h"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion3b() {
	auto completion = commands.complete_command(shell, CommandLine("help"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution3b() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("help"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("help", run.c_str());
}

/**
 * Exact matching commands with nothing longer return no replacements or help.
 */
static void test_completion3c() {
	auto completion = commands.complete_command(shell, CommandLine("help "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution3c() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("help "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("help", run.c_str());
}

/**
 * A partial command with only one potential match (that is a prefix for multiple longer commands)
 * should be completed up to that point and no further.
 */
static void test_completion4a() {
	auto completion = commands.complete_command(shell, CommandLine("se"));

	TEST_ASSERT_EQUAL_STRING("set", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution4a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("se"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * An exact matching command that is a prefix for one longer command (that has
 * no arguments or longer commands) should complete to that longer command.
 */
static void test_completion4b() {
	auto completion = commands.complete_command(shell, CommandLine("set"));

	TEST_ASSERT_EQUAL_STRING("set hostname", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed.
 */
static void test_execution4b() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("set"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("set", run.c_str());
}

/**
 * An exact matching command that is a prefix (with a space) for one longer
 * command (that has no arguments or longer commands) should complete to that
 * longer command without a space.
 */
static void test_completion4c() {
	auto completion = commands.complete_command(shell, CommandLine("set "));

	TEST_ASSERT_EQUAL_STRING("set hostname", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution4c() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("set "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("set", run.c_str());
}

/**
 * Partial matches of commands with arguments should complete the command
 * and add a space.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5a() {
	auto completion = commands.complete_command(shell, CommandLine("test_a"));

	TEST_ASSERT_EQUAL_STRING("test_a0 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_b"));

	TEST_ASSERT_EQUAL_STRING("test_b1 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_c"));

	TEST_ASSERT_EQUAL_STRING("test_c2 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_d"));

	TEST_ASSERT_EQUAL_STRING("test_d3 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution5a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands (without a space) with arguments should add
 * a space if there are arguments remaining.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5b() {
	auto completion = commands.complete_command(shell, CommandLine("test_a0"));

	TEST_ASSERT_EQUAL_STRING("test_a0 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_b1"));

	TEST_ASSERT_EQUAL_STRING("test_b1 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_c2"));

	TEST_ASSERT_EQUAL_STRING("test_c2 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_d3"));

	TEST_ASSERT_EQUAL_STRING("test_d3 ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5b() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3"));

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
	auto completion = commands.complete_command(shell, CommandLine("test_a0 "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_b1 "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_c2 "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_d3 "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> <three>", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5c() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands with arguments (without a space) should provide a list of
 * all the remaining command line arguments.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5d() {
	auto completion = commands.complete_command(shell, CommandLine("test_a0 un"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_b1 un"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_c2 un"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_d3 un"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<one> <two> <three>", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5d() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 un"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 un"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 un"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 un"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_a0 \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 \"\""));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 \"\""));

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
	auto completion = commands.complete_command(shell, CommandLine("test_a0 un "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_b1 un "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_c2 un "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_d3 un "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> <three>", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5e() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 un "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 un "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 un "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 un "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_a0 \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 \"\" "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 \"\" "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands with arguments (without a space) should provide a list of
 * all the remaining command line arguments.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5f() {
	auto completion = commands.complete_command(shell, CommandLine("test_a0 un deux"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_b1 un deux"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_c2 un deux"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_d3 un deux"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<two> <three>", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5f() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 un deux"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 un deux"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 un deux"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 un deux"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_a0 \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 \"\" \"\""));

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
	auto completion = commands.complete_command(shell, CommandLine("test_a0 un deux "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_b1 un deux "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_c2 un deux "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_d3 un deux "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<three>", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5g() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 un deux "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 un deux "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 un deux "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 un deux "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_a0 \"\" \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 \"\" \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 \"\" \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 \"\" \"\" "));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact matches of commands with maximum arguments (without a space) should provide a list of
 * all the remaining command line arguments.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5h() {
	auto completion = commands.complete_command(shell, CommandLine("test_a0 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_b1 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_c2 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_d3 un deux trois"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("<three>", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5h() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_d3 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_a0 \"\" \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 \"\" \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 \"\" \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 \"\" \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_d3 <empty> <empty> <empty>", run.c_str());
}

/**
 * Exact matches of commands (with a space) with maximum arguments should do nothing,
 * even if there's a space at the end.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5i() {
	auto completion = commands.complete_command(shell, CommandLine("test_a0 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_b1 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_c2 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_d3 un deux trois "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are executed after checking for minimum arguments.
 */
static void test_execution5i() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 un deux trois "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_d3 un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_a0 \"\" \"\" \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_a0 <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 \"\" \"\" \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_b1 <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 \"\" \"\" \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_c2 <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 \"\" \"\" \"\" "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_d3 <empty> <empty> <empty>", run.c_str());
}

/**
 * Exact matches of commands with more than the maximum arguments should do nothing.
 *
 * The type of arguments (required/optional) is irrelevant.
 */
static void test_completion5j() {
	auto completion = commands.complete_command(shell, CommandLine("test_a0 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_b1 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_c2 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_d3 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact match commands are not executed if they have more than the maximum arguments.
 */
static void test_execution5j() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_a0 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 un deux trois quatre"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_a0 \"\" \"\" \"\" \"\""));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_b1 \"\" \"\" \"\" \"\""));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_c2 \"\" \"\" \"\" \"\""));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_d3 \"\" \"\" \"\" \"\""));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * A partial command with only one exact match (that is a prefix for multiple longer commands)
 * should be completed up to that point and no further.
 */
static void test_completion6a() {
	auto completion = commands.complete_command(shell, CommandLine("ge"));

	TEST_ASSERT_EQUAL_STRING("get", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Commands are not completed before being executed.
 */
static void test_execution6a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("ge"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * An exact matching command that is a prefix for multiple different longer commands should
 * add a space and return those commands.
 */
static void test_completion6b() {
	auto completion = commands.complete_command(shell, CommandLine("get"));

	TEST_ASSERT_EQUAL_STRING("get ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("hostname", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("uptime", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands are executed.
 */
static void test_execution6b() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("get"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("get", run.c_str());
}

/**
 * An exact matching command with a space that is a prefix for multiple different longer
 * commands should return those commands.
 */
static void test_completion6c() {
	auto completion = commands.complete_command(shell, CommandLine("get "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("hostname", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("uptime", (*it++).to_string().c_str());
	}
}

/**
 * Exact match commands with a trailing space are executed.
 */
static void test_execution6c() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("get "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("get", run.c_str());
}

/**
 * Required arguments can appear anywhere in the list of arguments.
 */
static void test_execution7a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_e"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e un"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e un deux"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_e un deux", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e un deux trois"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_e un deux trois", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e un deux trois quatre"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_e un deux trois quatre", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e un deux trois quatre cinq"));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Required arguments can appear anywhere in the list of arguments,
 * and empty arguments are valid arguments.
 */
static void test_execution7b() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_e"));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e \"\""));

	TEST_ASSERT_EQUAL_STRING("Not enough arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_e <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e \"\" \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_e <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e \"\" \"\" \"\" \"\""));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("test_e <empty> <empty> <empty> <empty>", run.c_str());

	run = "";
	execution = commands.execute_command(shell, CommandLine("test_e \"\" \"\" \"\" \"\" \"\""));

	TEST_ASSERT_EQUAL_STRING("Too many arguments for command", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

/**
 * Exact command matches with no arguments should get a trailing space.
 */
static void test_completion8a() {
	auto completion = commands.complete_command(shell, CommandLine("test_f"));

	TEST_ASSERT_EQUAL_STRING("test_f ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_g"));

	TEST_ASSERT_EQUAL_STRING("test_g ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_h"));

	TEST_ASSERT_EQUAL_STRING("test_h ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_i"));

	TEST_ASSERT_EQUAL_STRING("test_i ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact command matches with no arguments but a trailing space should provide
 * a list of possible arguments but not complete to anything (even for a single
 * option).
 */
static void test_completion8b() {
	auto completion = commands.complete_command(shell, CommandLine("test_f "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaaaa [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb1 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc1c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaaaa [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb1 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc1c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("test [two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8c() {
	auto completion = commands.complete_command(shell, CommandLine("test_f a"));

	TEST_ASSERT_EQUAL_STRING("test_f aaaaa", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g a"));

	TEST_ASSERT_EQUAL_STRING("test_g aaaaa", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h a"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i t"));

	TEST_ASSERT_EQUAL_STRING("test_i test", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8d() {
	auto completion = commands.complete_command(shell, CommandLine("test_f b"));

	TEST_ASSERT_EQUAL_STRING("test_f bbb", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("bbb1 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g b"));

	TEST_ASSERT_EQUAL_STRING("test_g bbb", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("bbb1 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h b"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i b"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8e() {
	auto completion = commands.complete_command(shell, CommandLine("test_f c"));

	TEST_ASSERT_EQUAL_STRING("test_f cccc", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("cccc1c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g c"));

	TEST_ASSERT_EQUAL_STRING("test_g cccc", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("cccc1c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a known completion argument should append a space
 * and return the remaining argument list. Unknown arguments don't get a space.
 */
static void test_completion8f() {
	auto completion = commands.complete_command(shell, CommandLine("test_f cccc1c"));

	TEST_ASSERT_EQUAL_STRING("test_f cccc1c ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g cccc2c"));

	TEST_ASSERT_EQUAL_STRING("test_g cccc2c ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h cccc3c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i test"));

	TEST_ASSERT_EQUAL_STRING("test_i test ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with no arguments but a trailing space should provide
 * a list of possible arguments but not complete to anything (even for a single
 * option).
 */
static void test_completion8g() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaAaa [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB1 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB2 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc1c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc2c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc3c [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaaaa [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb1 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc1c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("test [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8h() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd a"));

	TEST_ASSERT_EQUAL_STRING("test_f ddd aaAaa", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd a"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd a"));

	TEST_ASSERT_EQUAL_STRING("test_h ddd aaaaa", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd t"));

	TEST_ASSERT_EQUAL_STRING("test_i ddd test", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8i() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd b"));

	TEST_ASSERT_EQUAL_STRING("test_f ddd bbB", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("bbB1 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB2 [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd b"));

	TEST_ASSERT_EQUAL_STRING("test_h ddd bbb", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("bbb1 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd b"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd b"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8j() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd c"));

	TEST_ASSERT_EQUAL_STRING("test_f ddd ccCc", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("ccCc1c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc2c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc3c [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd c"));

	TEST_ASSERT_EQUAL_STRING("test_h ddd cccc", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("cccc1c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a known completion argument should append a space
 * and return the remaining argument list. Unknown arguments don't get a space.
 */
static void test_completion8k() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd ccCc1c"));

	TEST_ASSERT_EQUAL_STRING("test_f ddd ccCc1c ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd cccc2c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd cccc3c"));

	TEST_ASSERT_EQUAL_STRING("test_h ddd cccc3c ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd test"));

	TEST_ASSERT_EQUAL_STRING("test_i ddd test ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with no arguments but a trailing space should provide
 * a list of possible arguments but not complete to anything (even for a single
 * option).
 */
static void test_completion8l() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd eee "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaAaa", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB2", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc1c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc2c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc3c", (*it++).to_string().c_str());
	}
	completion = commands.complete_command(shell, CommandLine("test_h ddd eee "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaAaa", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB2", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc1c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc2c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc3c", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("test", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8m() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd eee a"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee a"));

	TEST_ASSERT_EQUAL_STRING("test_g ddd eee aaAaa", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_h ddd eee a"));

	TEST_ASSERT_EQUAL_STRING("test_h ddd eee aaAaa", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee t"));

	TEST_ASSERT_EQUAL_STRING("test_i ddd eee test", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8n() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd eee b"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee b"));

	TEST_ASSERT_EQUAL_STRING("test_g ddd eee bbB", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("bbB1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB2", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd eee b"));

	TEST_ASSERT_EQUAL_STRING("test_h ddd eee bbB", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("bbB1", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB2", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee b"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible.
 */
static void test_completion8o() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd eee c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee c"));

	TEST_ASSERT_EQUAL_STRING("test_g ddd eee ccCc", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("ccCc1c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc2c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc3c", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd eee c"));

	TEST_ASSERT_EQUAL_STRING("test_h ddd eee ccCc", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("ccCc1c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc2c", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc3c", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a known completion argument at the end of the
 * argument list should do nothing, and an unknown completion argument should
 * return help.
 */
static void test_completion8p() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd eee ccCc1c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee ccCc2c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee fff"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h ddd eee ccCc3c"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_h ddd eee fff"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee test"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee fff"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with maximum arguments and a space should do nothing.
 */
static void test_completion8q() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd eee fff "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee fff "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_h ddd eee fff "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee fff "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact command matches with too many arguments should do nothing.
 */
static void test_completion8r() {
	auto completion = commands.complete_command(shell, CommandLine("test_f ddd eee fff ggg"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_g ddd eee fff ggg"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_h ddd eee fff ggg"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());

	completion = commands.complete_command(shell, CommandLine("test_i ddd eee fff ggg"));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Exact command matches with unknown completion arguments don't get a space.
 */
static void test_completion8s() {
	auto completion = commands.complete_command(shell, CommandLine("test_f \"\""));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaaaa [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb1 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc1c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g \"\""));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaaaa [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb1 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc1c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [two] [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h \"\""));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[one] [two] [three]", (*it++).to_string().c_str());
	}

	// An empty string is a prefix of the known argument "test"
	completion = commands.complete_command(shell, CommandLine("test_i \"\""));

	TEST_ASSERT_EQUAL_STRING("test_i test", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with no arguments but a trailing space should provide
 * a list of possible arguments but not complete to anything (even for a single
 * option).
 */
static void test_completion8t() {
	auto completion = commands.complete_command(shell, CommandLine("test_f \"\" "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaAaa [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB1 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbB2 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc1c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc2c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("ccCc3c [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_g \"\" "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two] [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_h \"\" "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(6, completion.help.size());
	if (completion.help.size() == 6) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("aaaaa [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb1 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("bbb2 [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc1c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc2c [three]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("cccc3c [three]", (*it++).to_string().c_str());
	}

	completion = commands.complete_command(shell, CommandLine("test_i \"\" "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("test [three]", (*it++).to_string().c_str());
	}
}

/**
 * Exact command matches with a partial argument should try to auto-complete the
 * argument as far as possible, preserving empty arguments.
 */
static void test_completion8u() {
	auto completion = commands.complete_command(shell, CommandLine("test_f \"\" a"));

	TEST_ASSERT_EQUAL_STRING("test_f \"\" aaAaa", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[three]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with an empty string.
 */
static void test_completion9a() {
	auto completion = commands.complete_command(shell, CommandLine("test_j "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("\"\" [two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with an empty string.
 */
static void test_completion9b() {
	auto completion = commands.complete_command(shell, CommandLine("test_j \"\""));

	TEST_ASSERT_EQUAL_STRING("test_j \"\" ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with an empty string.
 */
static void test_completion9c() {
	auto completion = commands.complete_command(shell, CommandLine("test_j \"\" "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with a space.
 */
static void test_completion9d() {
	auto completion = commands.complete_command(shell, CommandLine("test_k "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("\\  [two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with a space.
 */
static void test_completion9e() {
	auto completion = commands.complete_command(shell, CommandLine("test_k \\ "));

	TEST_ASSERT_EQUAL_STRING("test_k \\  ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with a space.
 */
static void test_completion9f() {
	auto completion = commands.complete_command(shell, CommandLine("test_k \\  "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with an empty string and a space.
 */
static void test_completion9g() {
	auto completion = commands.complete_command(shell, CommandLine("test_l "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("\"\" [two]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("\\  [two]", (*it++).to_string().c_str());
	}
}

/**
 * This is a special case because it's possible to end a command line with a quote
 * and try to tab complete the argument. That shouldn't match an empty string unless
 * it's the only possible option.
 */
static void test_completion9h() {
	auto completion = commands.complete_command(shell, CommandLine("test_l \"\""));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(2, completion.help.size());
	if (completion.help.size() == 2) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("\"\" [two]", (*it++).to_string().c_str());
		TEST_ASSERT_EQUAL_STRING("\\  [two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with an empty string and a space.
 */
static void test_completion9i() {
	auto completion = commands.complete_command(shell, CommandLine("test_l \\ "));

	TEST_ASSERT_EQUAL_STRING("test_l \\  ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with an empty string and a space.
 */
static void test_completion9j() {
	auto completion = commands.complete_command(shell, CommandLine("test_k \"\" "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two]", (*it++).to_string().c_str());
	}
}

/**
 * Argument completion should work as normal even with an empty string and a space.
 */
static void test_completion9k() {
	auto completion = commands.complete_command(shell, CommandLine("test_k \\  "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("[two]", (*it++).to_string().c_str());
	}
}

/**
 * Command names should be escaped but argument help should not.
 */
static void test_completion10a() {
	auto completion = commands.complete_command(shell, CommandLine("test_m"));

	TEST_ASSERT_EQUAL_STRING("test_m\\ with\\ spaces ", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

/**
 * Argument completion should be escaped but argument help should not.
 */
static void test_completion10b() {
	auto completion = commands.complete_command(shell, CommandLine("test_m\\ with\\ spaces "));

	TEST_ASSERT_EQUAL_STRING("", completion.replacement.to_string().c_str());
	TEST_ASSERT_EQUAL_INT(1, completion.help.size());
	if (completion.help.size() == 1) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("hello\\ world [another thing]", (*it++).to_string().c_str());
	}
}

/**
 * Execute a command with spaces in its name.
 */
static void test_execution10a() {
	run = "";
	auto execution = commands.execute_command(shell, CommandLine("test_m\\ with\\ spaces hello world"));

	TEST_ASSERT_NULL(execution.error);
	TEST_ASSERT_EQUAL_STRING("test_m with spaces hello world", run.c_str());
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
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_b1")}, flash_string_vector{F("<one>"), F("[two]"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_b1";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_c2")}, flash_string_vector{F("<one>"), F("<two>"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_c2";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_d3")}, flash_string_vector{F("<one>"), F("<two>"), F("<three>")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_d3";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_e")}, flash_string_vector{F("[one]"), F("<two>"), F("[three]"), F("<four>")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_e";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_f")}, flash_string_vector{F("[one]"), F("[two]"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_f";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		std::list<std::string> potential_arguments;

		if (arguments.size() == 0) {
			potential_arguments.emplace_back("aaaaa");
			potential_arguments.emplace_back("bbb1");
			potential_arguments.emplace_back("bbb2");
			potential_arguments.emplace_back("cccc1c");
			potential_arguments.emplace_back("cccc2c");
			potential_arguments.emplace_back("cccc3c");
		} else if (arguments.size() == 1) {
			potential_arguments.emplace_back("aaAaa");
			potential_arguments.emplace_back("bbB1");
			potential_arguments.emplace_back("bbB2");
			potential_arguments.emplace_back("ccCc1c");
			potential_arguments.emplace_back("ccCc2c");
			potential_arguments.emplace_back("ccCc3c");
		}

		return potential_arguments;
	});

	commands.add_command(0, 0, flash_string_vector{F("test_g")}, flash_string_vector{F("[one]"), F("[two]"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_g";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		std::list<std::string> potential_arguments;

		if (arguments.size() == 0) {
			potential_arguments.emplace_back("aaaaa");
			potential_arguments.emplace_back("bbb1");
			potential_arguments.emplace_back("bbb2");
			potential_arguments.emplace_back("cccc1c");
			potential_arguments.emplace_back("cccc2c");
			potential_arguments.emplace_back("cccc3c");
		} else if (arguments.size() == 2) {
			potential_arguments.emplace_back("aaAaa");
			potential_arguments.emplace_back("bbB1");
			potential_arguments.emplace_back("bbB2");
			potential_arguments.emplace_back("ccCc1c");
			potential_arguments.emplace_back("ccCc2c");
			potential_arguments.emplace_back("ccCc3c");
		}

		return potential_arguments;
	});

	commands.add_command(0, 0, flash_string_vector{F("test_h")}, flash_string_vector{F("[one]"), F("[two]"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_h";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		std::list<std::string> potential_arguments;

		if (arguments.size() == 1) {
			potential_arguments.emplace_back("aaaaa");
			potential_arguments.emplace_back("bbb1");
			potential_arguments.emplace_back("bbb2");
			potential_arguments.emplace_back("cccc1c");
			potential_arguments.emplace_back("cccc2c");
			potential_arguments.emplace_back("cccc3c");
		} else if (arguments.size() == 2) {
			potential_arguments.emplace_back("aaAaa");
			potential_arguments.emplace_back("bbB1");
			potential_arguments.emplace_back("bbB2");
			potential_arguments.emplace_back("ccCc1c");
			potential_arguments.emplace_back("ccCc2c");
			potential_arguments.emplace_back("ccCc3c");
		}

		return potential_arguments;
	});

	commands.add_command(0, 0, flash_string_vector{F("test_i")}, flash_string_vector{F("[one]"), F("[two]"), F("[three]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_i";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		return std::list<std::string>{"test"};
	});

	commands.add_command(0, 0, flash_string_vector{F("test_j")}, flash_string_vector{F("[one]"), F("[two]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_j";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		if (arguments.empty()) {
			return std::list<std::string>{""};
		} else {
			return std::list<std::string>{};
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_k")}, flash_string_vector{F("[one]"), F("[two]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_k";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		if (arguments.empty()) {
			return std::list<std::string>{" "};
		} else {
			return std::list<std::string>{};
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_l")}, flash_string_vector{F("[one]"), F("[two]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_l";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		if (arguments.empty()) {
			return std::list<std::string>{"", " "};
		} else {
			return std::list<std::string>{};
		}
	});

	commands.add_command(0, 0, flash_string_vector{F("test_m with spaces")}, flash_string_vector{F("[one thing]"), F("[another thing]")},
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) {
		run = "test_m with spaces";
		for (auto& argument : arguments) {
			run += " " + argument;
			if (argument.empty()) {
				run += "<empty>";
			}
		}
	},
	[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments) -> std::list<std::string> {
		if (arguments.empty()) {
			return std::list<std::string>{"hello world"};
		} else {
			return std::list<std::string>{};
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

	RUN_TEST(test_execution7a);
	RUN_TEST(test_execution7b);

	RUN_TEST(test_completion8a);
	RUN_TEST(test_completion8b);
	RUN_TEST(test_completion8c);
	RUN_TEST(test_completion8d);
	RUN_TEST(test_completion8e);
	RUN_TEST(test_completion8f);
	RUN_TEST(test_completion8g);
	RUN_TEST(test_completion8h);
	RUN_TEST(test_completion8i);
	RUN_TEST(test_completion8j);
	RUN_TEST(test_completion8k);
	RUN_TEST(test_completion8l);
	RUN_TEST(test_completion8m);
	RUN_TEST(test_completion8n);
	RUN_TEST(test_completion8o);
	RUN_TEST(test_completion8p);
	RUN_TEST(test_completion8q);
	RUN_TEST(test_completion8r);
	RUN_TEST(test_completion8s);
	RUN_TEST(test_completion8t);
	RUN_TEST(test_completion8u);

	RUN_TEST(test_completion9a);
	RUN_TEST(test_completion9b);
	RUN_TEST(test_completion9c);
	RUN_TEST(test_completion9d);
	RUN_TEST(test_completion9e);
	RUN_TEST(test_completion9f);
	RUN_TEST(test_completion9g);
	RUN_TEST(test_completion9h);
	RUN_TEST(test_completion9i);
	RUN_TEST(test_completion9j);
	RUN_TEST(test_completion9k);

	RUN_TEST(test_completion10a);
	RUN_TEST(test_completion10b);
	RUN_TEST(test_execution10a);

	return UNITY_END();
}
