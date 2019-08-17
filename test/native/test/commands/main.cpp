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
	TEST_ASSERT_EQUAL_INT(10, completion.help.size());
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
 * commands should return the longer commands.
 */
static void test_completion1c() {
	auto completion = commands.complete_command(shell, shell.parse_line("show "));

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
 * should complete as far as possible (FIXME) and return the longer commands.
 */
static void test_completion1d() {
	auto completion = commands.complete_command(shell, shell.parse_line("show th"));

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
 * Commands are not completed before being executed.
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
 * Commands are not completed before being executed.
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

int main(int argc, char *argv[]) {
	commands.add_command(0, 0, flash_string_vector{F("help")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "help";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("show")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing1")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing1";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing2")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing2";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing3")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing3";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("set")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "set";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("set"), F("hostname")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "set hostname";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("err")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log err";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("warning")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log warning";
	}, Commands::no_argument_completion());

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("info")}, Commands::no_arguments(),
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log info";
	}, Commands::no_argument_completion());

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

	// FIXME test arguments
	RUN_TEST(test_completion4a);
	RUN_TEST(test_completion4b);
	RUN_TEST(test_completion4c);
	RUN_TEST(test_execution4a);
	RUN_TEST(test_execution4b);
	RUN_TEST(test_execution4c);

	return UNITY_END();
}
