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

using ::uuid::console::CommandLine;

namespace uuid {

uint64_t get_uptime_ms() {
	static uint64_t millis = 0;
	return ++millis;
}

} // namespace uuid

/**
 * Empty string.
 */
static void test_empty() {
	CommandLine command_line("");

	TEST_ASSERT_EQUAL_INT(0, command_line->size());
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("", command_line.to_string().c_str());
}

/**
 * Preceding spaces are ignored.
 */
static void test_spaces1() {
	CommandLine command_line(" ");

	TEST_ASSERT_EQUAL_INT(0, command_line->size());
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("", command_line.to_string().c_str());
}

/**
 * Preceding spaces are ignored.
 */
static void test_spaces2() {
	CommandLine command_line("  ");

	TEST_ASSERT_EQUAL_INT(0, command_line->size());
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("", command_line.to_string().c_str());
}

/**
 * Preceding spaces are ignored.
 */
static void test_spaces3() {
	CommandLine command_line("   ");

	TEST_ASSERT_EQUAL_INT(0, command_line->size());
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("", command_line.to_string().c_str());
}

/**
 * No escape characters or characters needing to be escaped.
 */
static void test_simple1() {
	CommandLine command_line("Hello World!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello World!", command_line.to_string().c_str());
}

/**
 * Preceding spaces are ignored.
 */
static void test_space1a() {
	CommandLine command_line(" Hello World!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello World!", command_line.to_string().c_str());
}

/**
 * Trailing spaces are considered another parameter.
 */
static void test_space1b() {
	CommandLine command_line("Hello World! ");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_TRUE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello World! ", command_line.to_string().c_str());
}

/**
 * Multiple preceding spaces are ignored.
 */
static void test_space2a() {
	CommandLine command_line("  Hello World!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello World!", command_line.to_string().c_str());
}

/**
 * Multiple spaces are collapsed to one.
 */
static void test_space2b() {
	CommandLine command_line("Hello World!  ");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_TRUE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello World! ", command_line.to_string().c_str());
}

/**
 * Multiple spaces are collapsed to one.
 */
static void test_space2c() {
	CommandLine command_line("Hello  World!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello World!", command_line.to_string().c_str());
}

/**
 * Spaces can be escaped with a backslash.
 */
static void test_backslash_escaped1() {
	CommandLine command_line("Hello Escaped\\ World!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ World!", command_line.to_string().c_str());
}

/**
 * Double quotes can be escaped with a backslash.
 */
static void test_backslash_escaped2() {
	CommandLine command_line("Hello Escaped\\\" World!");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped\"", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\\" World!", command_line.to_string().c_str());
}

/**
 * Single quotes can be escaped with a backslash.
 */
static void test_backslash_escaped3() {
	CommandLine command_line("Hello Escaped\\' World!");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped'", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\' World!", command_line.to_string().c_str());
}

/**
 * Trailing backslashes are ignored.
 */
static void test_backslash_escaped4() {
	CommandLine command_line("Hello World!\\");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello World!", command_line.to_string().c_str());
}

/**
 * Backslash escapes of characters other than space or quotes are interpreted literally.
 */
static void test_backslash_escaped5() {
	CommandLine command_line("\\H\\e\\l\\l\\o\\ \\n\\e\\w\\l\\i\\n\\e\\ \\W\\o\\r\\l\\d\\!");

	TEST_ASSERT_EQUAL_INT(1, command_line->size());
	if (command_line->size() == 1) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("\\H\\e\\l\\l\\o \\n\\e\\w\\l\\i\\n\\e \\W\\o\\r\\l\\d\\!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\\\\H\\\\e\\\\l\\\\l\\\\o\\ \\\\n\\\\e\\\\w\\\\l\\\\i\\\\n\\\\e\\ \\\\W\\\\o\\\\r\\\\l\\\\d\\\\!", command_line.to_string().c_str());
}

/**
 * Spaces can be escaped by double quotes.
 */
static void test_double_quote_escaped1() {
	CommandLine command_line("Hello \"Escaped World!\"");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ World!", command_line.to_string().c_str());
}

/**
 * Single quotes can be escaped by double quotes.
 */
static void test_double_quote_escaped2() {
	CommandLine command_line("Hello \"Escaped 'World'!\"");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped 'World'!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\'World\\'!", command_line.to_string().c_str());
}

/**
 * Double quote escapes are implicitly ended at the end of the command line.
 */
static void test_double_quote_escaped3a() {
	CommandLine command_line("Hello \"Escaped 'World'!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped 'World'!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\'World\\'!", command_line.to_string().c_str());
}

/**
 * Double quote escapes are implicitly ended at the end of the command line, even if there are trailing spaces.
 */
static void test_double_quote_escaped3b() {
	CommandLine command_line("Hello \"Escaped 'World'!     ");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped 'World'!     ", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\'World\\'!\\ \\ \\ \\ \\ ", command_line.to_string().c_str());
}

/**
 * Backslash escapes of characters other than space or quotes are interpreted literally, even inside double quotes.
 */
static void test_double_quote_escaped4() {
	CommandLine command_line("Hello \"\\E\\s\\c\\a\\p\\e\\d\\ \\'\\W\\o\\r\\l\\d\\'\\!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("\\E\\s\\c\\a\\p\\e\\d\\ '\\W\\o\\r\\l\\d'\\!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello \\\\E\\\\s\\\\c\\\\a\\\\p\\\\e\\\\d\\\\\\ \\'\\\\W\\\\o\\\\r\\\\l\\\\d\\'\\\\!", command_line.to_string().c_str());
}

/**
 * Double quotes can be escaped with backslashes inside double quotes.
 */
static void test_double_quote_escaped5() {
	CommandLine command_line("Hello \"Escaped \\\"World\\\"!\"");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped \"World\"!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\\"World\\\"!", command_line.to_string().c_str());
}

/**
 * Spaces can be escaped by single quotes.
 */
static void test_single_quote_escaped1() {
	CommandLine command_line("Hello 'Escaped World!'");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped World!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ World!", command_line.to_string().c_str());
}

/**
 * Double quotes can be escaped by single quotes.
 */
static void test_single_quote_escaped2() {
	CommandLine command_line("Hello 'Escaped \"World\"!'");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped \"World\"!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\\"World\\\"!", command_line.to_string().c_str());
}

/**
 * Single quote escapes are implicitly ended at the end of the command line.
 */
static void test_single_quote_escaped3a() {
	CommandLine command_line("Hello 'Escaped \"World\"!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped \"World\"!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\\"World\\\"!", command_line.to_string().c_str());
}

/**
 * Single quote escapes are implicitly ended at the end of the command line, even if there are trailing spaces.
 */
static void test_single_quote_escaped3b() {
	CommandLine command_line("Hello 'Escaped \"World\"!     ");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped \"World\"!     ", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\\"World\\\"!\\ \\ \\ \\ \\ ", command_line.to_string().c_str());
}

/**
 * Backslash escapes of characters other than space or quotes are interpreted literally, even inside single quotes.
 */
static void test_single_quote_escaped4() {
	CommandLine command_line("Hello '\\E\\s\\c\\a\\p\\e\\d\\ \\\"\\W\\o\\r\\l\\d\\\"\\!");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("\\E\\s\\c\\a\\p\\e\\d\\ \"\\W\\o\\r\\l\\d\"\\!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello \\\\E\\\\s\\\\c\\\\a\\\\p\\\\e\\\\d\\\\\\ \\\"\\\\W\\\\o\\\\r\\\\l\\\\d\\\"\\\\!", command_line.to_string().c_str());
}

/**
 * Single quotes can be escaped with backslashes inside single quotes.
 */
static void test_single_quote_escaped5() {
	CommandLine command_line("Hello 'Escaped \\'World\\'!'");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("Hello", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("Escaped 'World'!", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("Hello Escaped\\ \\'World\\'!", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using double quotes.
 */
static void test_empty_args_double_quotes1() {
	CommandLine command_line("\"\"");

	TEST_ASSERT_EQUAL_INT(1, command_line->size());
	if (command_line->size() == 1) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using double quotes.
 */
static void test_empty_args_double_quotes2() {
	CommandLine command_line("\"\" \"\"");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using double quotes.
 */
static void test_empty_args_double_quotes3() {
	CommandLine command_line("\"\" \"\" \"\"");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\" \"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using double quotes, extra spaces are ignored.
 */
static void test_empty_args_double_quotes4() {
	CommandLine command_line(" \"\" \"\" \"\" ");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_TRUE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\" \"\" ", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using double quotes, extra spaces are ignored.
 */
static void test_empty_args_double_quotes5() {
	CommandLine command_line("  \"\"  \"\"  \"\"  ");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_TRUE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\" \"\" ", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using double quotes.
 */
static void test_empty_args_double_quotes6() {
	CommandLine command_line("command \"\" test \"\"");

	TEST_ASSERT_EQUAL_INT(4, command_line->size());
	if (command_line->size() == 4) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("command", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("test", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("command \"\" test \"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using a double quote at the end of the line.
 */
static void test_empty_args_double_quotes7() {
	CommandLine command_line("command \"");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("command", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("command \"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using single quotes.
 */
static void test_empty_args_single_quotes1() {
	CommandLine command_line("''");

	TEST_ASSERT_EQUAL_INT(1, command_line->size());
	if (command_line->size() == 1) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using single quotes.
 */
static void test_empty_args_single_quotes2() {
	CommandLine command_line("'' ''");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using single quotes.
 */
static void test_empty_args_single_quotes3() {
	CommandLine command_line("'' '' ''");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\" \"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using single quotes, extra spaces are ignored.
 */
static void test_empty_args_single_quotes4() {
	CommandLine command_line(" '' '' '' ");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_TRUE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\" \"\" ", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using single quotes, extra spaces are ignored.
 */
static void test_empty_args_single_quotes5() {
	CommandLine command_line("  ''   ''   ''  ");

	TEST_ASSERT_EQUAL_INT(3, command_line->size());
	if (command_line->size() == 3) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_TRUE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("\"\" \"\" \"\" ", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using single quotes.
 */
static void test_empty_args_single_quotes6() {
	CommandLine command_line("command '' test ''");

	TEST_ASSERT_EQUAL_INT(4, command_line->size());
	if (command_line->size() == 4) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("command", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("test", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("command \"\" test \"\"", command_line.to_string().c_str());
}

/**
 * Empty arguments can be created using a single quote at the end of the line.
 */
static void test_empty_args_single_quotes7() {
	CommandLine command_line("command '");

	TEST_ASSERT_EQUAL_INT(2, command_line->size());
	if (command_line->size() == 2) {
		auto it = command_line->begin();
		TEST_ASSERT_EQUAL_STRING("command", (*it++).c_str());
		TEST_ASSERT_EQUAL_STRING("", (*it++).c_str());
	}
	TEST_ASSERT_FALSE(command_line.trailing_space);

	TEST_ASSERT_EQUAL_STRING("command \"\"", command_line.to_string().c_str());
}

int main(int argc, char *argv[]) {
	UNITY_BEGIN();
	RUN_TEST(test_empty);
	RUN_TEST(test_spaces1);
	RUN_TEST(test_spaces2);
	RUN_TEST(test_spaces3);

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
	RUN_TEST(test_double_quote_escaped3a);
	RUN_TEST(test_double_quote_escaped3b);
	RUN_TEST(test_double_quote_escaped4);
	RUN_TEST(test_double_quote_escaped5);

	RUN_TEST(test_single_quote_escaped1);
	RUN_TEST(test_single_quote_escaped2);
	RUN_TEST(test_single_quote_escaped3a);
	RUN_TEST(test_single_quote_escaped3b);
	RUN_TEST(test_single_quote_escaped4);
	RUN_TEST(test_single_quote_escaped5);

	RUN_TEST(test_empty_args_double_quotes1);
	RUN_TEST(test_empty_args_double_quotes2);
	RUN_TEST(test_empty_args_double_quotes3);
	RUN_TEST(test_empty_args_double_quotes4);
	RUN_TEST(test_empty_args_double_quotes5);
	RUN_TEST(test_empty_args_double_quotes6);
	RUN_TEST(test_empty_args_double_quotes7);

	RUN_TEST(test_empty_args_single_quotes1);
	RUN_TEST(test_empty_args_single_quotes2);
	RUN_TEST(test_empty_args_single_quotes3);
	RUN_TEST(test_empty_args_single_quotes4);
	RUN_TEST(test_empty_args_single_quotes5);
	RUN_TEST(test_empty_args_single_quotes6);
	RUN_TEST(test_empty_args_single_quotes7);

	return UNITY_END();
}
