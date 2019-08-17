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

static DummyShell shell;

static void test_simple1() {
	auto command_line = shell.parse_line("Hello World!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello World!", shell.format_line(command_line).c_str());
}

/**
 * Preceding spaces are ignored.
 */
static void test_space1a() {
	auto command_line = shell.parse_line(" Hello World!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello World!", shell.format_line(command_line).c_str());
}

/**
 * Trailing spaces are considered another parameter.
 */
static void test_space1b() {
	auto command_line = shell.parse_line("Hello World! ");

	TEST_ASSERT_EQUAL_INT(3, command_line.size());
	if (command_line.size() == 3) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello World! ", shell.format_line(command_line).c_str());
}

/**
 * Multiple preceding spaces are ignored.
 */
static void test_space2a() {
	auto command_line = shell.parse_line("  Hello World!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello World!", shell.format_line(command_line).c_str());
}

/**
 * Multiple spaces are collapsed to one.
 */
static void test_space2b() {
	auto command_line = shell.parse_line("Hello World!  ");

	TEST_ASSERT_EQUAL_INT(3, command_line.size());
	if (command_line.size() == 3) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello World! ", shell.format_line(command_line).c_str());
}

/**
 * Multiple spaces are collapsed to one.
 */
static void test_space2c() {
	auto command_line = shell.parse_line("Hello  World!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello World!", shell.format_line(command_line).c_str());
}

static void test_backslash_escaped1() {
	auto command_line = shell.parse_line("Hello Escaped\\ World!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ World!", shell.format_line(command_line).c_str());
}

static void test_backslash_escaped2() {
	auto command_line = shell.parse_line("Hello Escaped\\\" World!");

	TEST_ASSERT_EQUAL_INT(3, command_line.size());
	if (command_line.size() == 3) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped\"", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\\" World!", shell.format_line(command_line).c_str());
}

static void test_backslash_escaped3() {
	auto command_line = shell.parse_line("Hello Escaped\\' World!");

	TEST_ASSERT_EQUAL_INT(3, command_line.size());
	if (command_line.size() == 3) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped'", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\' World!", shell.format_line(command_line).c_str());
}

static void test_backslash_escaped4() {
	auto command_line = shell.parse_line("Hello World!\\");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello World!", shell.format_line(command_line).c_str());
}

static void test_backslash_escaped5() {
	auto command_line = shell.parse_line("\\H\\e\\l\\l\\o\\ \\n\\e\\w\\l\\i\\n\\e\\ \\W\\o\\r\\l\\d\\!");

	TEST_ASSERT_EQUAL_INT(1, command_line.size());
	if (command_line.size() == 1) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("\\H\\e\\l\\l\\o \\n\\e\\w\\l\\i\\n\\e \\W\\o\\r\\l\\d\\!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("\\\\H\\\\e\\\\l\\\\l\\\\o\\ \\\\n\\\\e\\\\w\\\\l\\\\i\\\\n\\\\e\\ \\\\W\\\\o\\\\r\\\\l\\\\d\\\\!", shell.format_line(command_line).c_str());
}

static void test_double_quote_escaped1() {
	auto command_line = shell.parse_line("Hello \"Escaped World!\"");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ World!", shell.format_line(command_line).c_str());
}

static void test_double_quote_escaped2() {
	auto command_line = shell.parse_line("Hello \"Escaped 'World'!\"");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped 'World'!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\'World\\'!", shell.format_line(command_line).c_str());
}

static void test_double_quote_escaped3() {
	auto command_line = shell.parse_line("Hello \"Escaped 'World'!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped 'World'!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\'World\\'!", shell.format_line(command_line).c_str());
}

static void test_double_quote_escaped4() {
	auto command_line = shell.parse_line("Hello \"\\E\\s\\c\\a\\p\\e\\d\\ \\'\\W\\o\\r\\l\\d\\'\\!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("\\E\\s\\c\\a\\p\\e\\d\\ '\\W\\o\\r\\l\\d'\\!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello \\\\E\\\\s\\\\c\\\\a\\\\p\\\\e\\\\d\\\\\\ \\'\\\\W\\\\o\\\\r\\\\l\\\\d\\'\\\\!", shell.format_line(command_line).c_str());
}

static void test_double_quote_escaped5() {
	auto command_line = shell.parse_line("Hello \"Escaped \\\"World\\\"!\"");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped \"World\"!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\\"World\\\"!", shell.format_line(command_line).c_str());
}

static void test_single_quote_escaped1() {
	auto command_line = shell.parse_line("Hello 'Escaped World!'");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped World!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ World!", shell.format_line(command_line).c_str());
}

static void test_single_quote_escaped2() {
	auto command_line = shell.parse_line("Hello 'Escaped \"World\"!'");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped \"World\"!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\\"World\\\"!", shell.format_line(command_line).c_str());
}

static void test_single_quote_escaped3() {
	auto command_line = shell.parse_line("Hello 'Escaped \"World\"!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped \"World\"!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\\"World\\\"!", shell.format_line(command_line).c_str());
}

static void test_single_quote_escaped4() {
	auto command_line = shell.parse_line("Hello '\\E\\s\\c\\a\\p\\e\\d\\ \\\"\\W\\o\\r\\l\\d\\\"\\!");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("\\E\\s\\c\\a\\p\\e\\d\\ \"\\W\\o\\r\\l\\d\"\\!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello \\\\E\\\\s\\\\c\\\\a\\\\p\\\\e\\\\d\\\\\\ \\\"\\\\W\\\\o\\\\r\\\\l\\\\d\\\"\\\\!", shell.format_line(command_line).c_str());
}

static void test_single_quote_escaped5() {
	auto command_line = shell.parse_line("Hello 'Escaped \\'World\\'!'");

	TEST_ASSERT_EQUAL_INT(2, command_line.size());
	if (command_line.size() == 2) {
		auto it = command_line.begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped 'World'!", (*it++).c_str());
	}

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\'World\\'!", shell.format_line(command_line).c_str());
}

int main(int argc, char *argv[]) {
	UNITY_BEGIN();
	RUN_TEST(test_simple1);
	RUN_TEST(test_space1a);
	RUN_TEST(test_space1b);
	RUN_TEST(test_space2a);
	RUN_TEST(test_space2b);
	RUN_TEST(test_space2c);

	RUN_TEST(test_backslash_escaped1);
	RUN_TEST(test_backslash_escaped2);
	RUN_TEST(test_backslash_escaped3);
	RUN_TEST(test_backslash_escaped4);
	RUN_TEST(test_backslash_escaped5);

	RUN_TEST(test_double_quote_escaped1);
	RUN_TEST(test_double_quote_escaped2);
	RUN_TEST(test_double_quote_escaped3);
	RUN_TEST(test_double_quote_escaped4);
	RUN_TEST(test_double_quote_escaped5);

	RUN_TEST(test_single_quote_escaped1);
	RUN_TEST(test_single_quote_escaped2);
	RUN_TEST(test_single_quote_escaped3);
	RUN_TEST(test_single_quote_escaped4);
	RUN_TEST(test_single_quote_escaped5);

	return UNITY_END();
}
