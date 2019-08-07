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
	using Shell::unparse_line;

protected:
	int read() { return '\n'; };
	void print(char data __attribute__((unused))) {};
	void print(const char *data __attribute__((unused))) {};
	void print(const std::string &data __attribute__((unused))) {};
	void print(const __FlashStringHelper *data __attribute__((unused))) {};
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

static void test_execution0() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line(""));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion1a() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("sh"));

	TEST_ASSERT_EQUAL_STRING("show", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution1a() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("sh"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion1b() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("show"));

	TEST_ASSERT_EQUAL_STRING("show ", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.unparse_line(*it++).c_str());
	}
}

static void test_execution1b() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("show"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show", run.c_str());
}

static void test_completion1c() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("show "));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.unparse_line(*it++).c_str());
	}
}

static void test_execution1c() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("show "));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show", run.c_str());
}

static void test_completion1d() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("show th"));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.unparse_line(*it++).c_str());
	}
}

static void test_execution1d() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("show th"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion1e() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("show thing"));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("thing1", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing2", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("thing3", shell.unparse_line(*it++).c_str());
	}
}

static void test_execution1e() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("show thing"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion1f() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("show thing1"));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution1f() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("show thing1"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("show thing1", run.c_str());
}

static void test_completion2a() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("cons"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2a() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("cons"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2b() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2b() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2c() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console "));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2c() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console "));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2d() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console l"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2d() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console l"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2e() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console log"));

	TEST_ASSERT_EQUAL_STRING("console log ", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2e() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console log"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2f() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console log "));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(3, completion.help.size());
	if (completion.help.size() == 3) {
		auto it = completion.help.begin();
		TEST_ASSERT_EQUAL_STRING("err", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("warning", shell.unparse_line(*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("info", shell.unparse_line(*it++).c_str());
	}
}

static void test_execution2f() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console log "));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2g() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console log a"));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2g() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console log a"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2h() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console log in"));

	TEST_ASSERT_EQUAL_STRING("console log info", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2h() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console log in"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion2i() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("console log info"));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution2i() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("console log info"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("console log info", run.c_str());
}

static void test_completion3a() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("h"));

	TEST_ASSERT_EQUAL_STRING("help", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution3a() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("h"));

	TEST_ASSERT_EQUAL_STRING("Command not found", execution.error);
	TEST_ASSERT_EQUAL_STRING("", run.c_str());
}

static void test_completion3b() {
	auto completion = commands.complete_command(shell, 0, 0, shell.parse_line("help"));

	TEST_ASSERT_EQUAL_STRING("", shell.unparse_line(completion.replacement).c_str());
	TEST_ASSERT_EQUAL_INT(0, completion.help.size());
}

static void test_execution3b() {
	run = "";
	auto execution = commands.execute_command(shell, 0, 0, shell.parse_line("help"));

	TEST_ASSERT_NULL_MESSAGE(execution.error, (const char *)execution.error);
	TEST_ASSERT_EQUAL_STRING("help", run.c_str());
}

int main(int argc, char *argv[]) {
	commands.add_command(0, 0, flash_string_vector{F("help")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "help";
	}, Commands::no_argument_completion);

	commands.add_command(0, 0, flash_string_vector{F("show")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show";
	}, Commands::no_argument_completion);

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing1")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing1";
	}, Commands::no_argument_completion);

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing2")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing2";
	}, Commands::no_argument_completion);

	commands.add_command(0, 0, flash_string_vector{F("show"), F("thing3")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "show thing3";
	}, Commands::no_argument_completion);

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("err")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log err";
	}, Commands::no_argument_completion);

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("warning")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log warning";
	}, Commands::no_argument_completion);

	commands.add_command(0, 0, flash_string_vector{F("console"), F("log"), F("info")}, Commands::no_arguments,
			[&] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {
		run = "console log info";
	}, Commands::no_argument_completion);

	UNITY_BEGIN();
	RUN_TEST(test_execution0);

	RUN_TEST(test_completion1a);
	RUN_TEST(test_completion1b);
	RUN_TEST(test_completion1c);
	RUN_TEST(test_completion1d);
	RUN_TEST(test_completion1e);
	RUN_TEST(test_completion1f);
	RUN_TEST(test_execution1a);
	RUN_TEST(test_execution1b);
	RUN_TEST(test_execution1d);
	RUN_TEST(test_execution1c);
	RUN_TEST(test_execution1e);
	RUN_TEST(test_execution1f);

	RUN_TEST(test_completion2a);
	RUN_TEST(test_completion2b);
	RUN_TEST(test_completion2c);
	RUN_TEST(test_completion2d);
	RUN_TEST(test_completion2e);
	RUN_TEST(test_completion2f);
	RUN_TEST(test_completion2g);
	RUN_TEST(test_completion2h);
	RUN_TEST(test_completion2i);
	RUN_TEST(test_execution2a);
	RUN_TEST(test_execution2b);
	RUN_TEST(test_execution2c);
	RUN_TEST(test_execution2d);
	RUN_TEST(test_execution2e);
	RUN_TEST(test_execution2f);
	RUN_TEST(test_execution2g);
	RUN_TEST(test_execution2h);
	RUN_TEST(test_execution2i);

	RUN_TEST(test_completion3a);
	RUN_TEST(test_completion3b);
	RUN_TEST(test_execution3a);
	RUN_TEST(test_execution3b);

	// FIXME verify executions
	// FIXME test arguments
	return UNITY_END();
}
