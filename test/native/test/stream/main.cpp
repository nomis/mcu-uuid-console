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
using ::uuid::console::StreamConsole;

class TestStream: public Stream {
public:
	TestStream(bool supports_peek)
		: supports_peek_(supports_peek) {

	}
	~TestStream() override = default;

	void operator<<(const std::string &input) {
		input_data_.insert(input_data_.end(), input.begin(), input.end());
	}

	bool empty() {
		return input_data_.empty();
	}

	std::string input() {
		return std::string(input_data_.begin(), input_data_.end());
	}

	std::string output() {
		std::string copy = output_data_;
		output_data_.clear();
		return copy;
	}

protected:
	int available() override {
		return input_data_.size();
	}

	int read() override {
		if (input_data_.empty()) {
			return -1;
		} else {
			unsigned char c = input_data_.front();

			input_data_.pop_front();
			return c;
		}
	};

	int peek() override {
		if (!supports_peek_ || input_data_.empty()) {
			return -1;
		} else {
			return input_data_.front();
		}
	};

	size_t write(uint8_t data) override {
		output_data_ += data;
		return 1;
	}

	size_t write(const uint8_t *buffer, size_t size) override {
		output_data_ += std::string(reinterpret_cast<const char*>(buffer), size);
		return size;
	}

private:
	std::list<unsigned char> input_data_;
	std::string output_data_;
	bool supports_peek_;
};

/**
 * First call to make in blocking functions.
 */
enum class BlockingTestMode {
	AVAILABLE,
	PEEK,
	READ,
};

namespace uuid {

uint64_t get_uptime_ms() {
	static uint64_t millis = 0;
	return ++millis;
}

} // namespace uuid

static std::shared_ptr<Commands> commands = std::make_shared<Commands>();
static uuid::console::Shell::blocking_function test_fn;

/**
 * Test with CR line endings.
 */
static void test_blocking_cr(BlockingTestMode mode, bool stream_supports_peek, bool with_data = false) {
	TestStream stream{stream_supports_peek};
	size_t executions = 0;
	auto console = std::make_shared<StreamConsole>(commands, stream);

	console->start();

	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	console->loop_one();

	stream << "test\r";
	if (with_data) {
		stream << "x\n";
	}
	switch (mode) {
	case BlockingTestMode::AVAILABLE:
		test_fn = [executions, stream_supports_peek, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_TRUE(shell.available());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('x', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('x', shell.read());
				TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('\n', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_INT('\n', shell.read());
			}
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_FALSE(shell.available());
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;

	case BlockingTestMode::PEEK:
		test_fn = [executions, stream_supports_peek, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('x', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('x', shell.read());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('\n', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_INT('\n', shell.read());
			}
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;

	case BlockingTestMode::READ:
		test_fn = [executions, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('x', shell.read());
				TEST_ASSERT_EQUAL_INT('\n', shell.read());
			}
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;
	}

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("test\r\n", stream.output().c_str());

	if (!with_data) {
		console->loop_one();
		TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());
	}

	stream << "A";
	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_TRUE(shell.available());
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
		}
		TEST_ASSERT_TRUE(shell.available());
		TEST_ASSERT_EQUAL_INT('A', shell.read());
		TEST_ASSERT_FALSE(shell.available());
		TEST_ASSERT_EQUAL_INT(-1, shell.read());
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return stop;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	stream << "BCD\rnoop\r";
	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('B', shell.peek());
		} else {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return stop;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("BCD\rnoop\r", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_EQUAL_INT('B', shell.read());
		TEST_ASSERT_EQUAL_INT('C', shell.read());
		TEST_ASSERT_EQUAL_INT('D', shell.read());
		TEST_ASSERT_EQUAL_INT('\r', shell.read());
		TEST_ASSERT_TRUE(shell.available());

		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('n', shell.peek());
		} else {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return true;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("noop\r", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	test_fn = [executions] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_FAIL();
		return true;
	};

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("noop\r\n$ ", stream.output().c_str());

	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

static void test_blocking_cr_available_peek() {
	test_blocking_cr(BlockingTestMode::AVAILABLE, true);
}

static void test_blocking_cr_available_no_peek() {
	test_blocking_cr(BlockingTestMode::AVAILABLE, false);
}

static void test_blocking_cr_available_peek_with_data() {
	test_blocking_cr(BlockingTestMode::AVAILABLE, true, true);
}

static void test_blocking_cr_available_no_peek_with_data() {
	test_blocking_cr(BlockingTestMode::AVAILABLE, false, true);
}

static void test_blocking_cr_peek_peek() {
	test_blocking_cr(BlockingTestMode::PEEK, true);
}

static void test_blocking_cr_peek_no_peek() {
	test_blocking_cr(BlockingTestMode::PEEK, false);
}

static void test_blocking_cr_peek_peek_with_data() {
	test_blocking_cr(BlockingTestMode::PEEK, true, true);
}

static void test_blocking_cr_peek_no_peek_with_data() {
	test_blocking_cr(BlockingTestMode::PEEK, false, true);
}

static void test_blocking_cr_read_peek() {
	test_blocking_cr(BlockingTestMode::READ, true);
}

static void test_blocking_cr_read_no_peek() {
	test_blocking_cr(BlockingTestMode::READ, false);
}

static void test_blocking_cr_read_peek_with_data() {
	test_blocking_cr(BlockingTestMode::READ, true, true);
}

static void test_blocking_cr_read_no_peek_with_data() {
	test_blocking_cr(BlockingTestMode::READ, false, true);
}

/**
 * Test with CRLF line endings.
 */
static void test_blocking_crlf(BlockingTestMode mode, bool stream_supports_peek, bool with_data = false) {
	TestStream stream{stream_supports_peek};
	size_t executions = 0;
	auto console = std::make_shared<StreamConsole>(commands, stream);

	console->start();

	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	console->loop_one();

	stream << "test\r\n";
	if (with_data) {
		stream << "x\n";
	}
	switch (mode) {
	case BlockingTestMode::AVAILABLE:
		test_fn = [executions, stream_supports_peek, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("\nx\n", stream.input().c_str());
				TEST_ASSERT_TRUE(shell.available());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT('x', shell.peek());
					TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT('x', shell.read());
					TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT('\n', shell.peek());
					TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT('\n', shell.read());
					TEST_ASSERT_FALSE(shell.available());
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
					TEST_ASSERT_EQUAL_INT(-1, shell.read());
				} else {
					TEST_ASSERT_EQUAL_STRING("\nx\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
					TEST_ASSERT_TRUE(shell.available());
					TEST_ASSERT_EQUAL_INT('x', shell.read());
					TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
					TEST_ASSERT_TRUE(shell.available());
					TEST_ASSERT_EQUAL_INT('\n', shell.read());
					TEST_ASSERT_FALSE(shell.available());
				}
				TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			} else {
				TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
				if (stream_supports_peek) {
					TEST_ASSERT_FALSE(shell.available());
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
					TEST_ASSERT_EQUAL_INT(-1, shell.read());
				} else {
					TEST_ASSERT_TRUE(shell.available());
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
					TEST_ASSERT_TRUE(shell.available());
					TEST_ASSERT_EQUAL_INT(-1, shell.read());
					TEST_ASSERT_FALSE(shell.available());
				}
			}
			TEST_ASSERT_FALSE(stop);
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			return stop;
		};
		break;

	case BlockingTestMode::PEEK:
		test_fn = [executions, stream_supports_peek, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("\nx\n", stream.input().c_str());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('x', shell.peek());
					TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT('x', shell.read());
					TEST_ASSERT_EQUAL_INT('\n', shell.peek());
					TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT('\n', shell.read());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
					TEST_ASSERT_EQUAL_STRING("\nx\n", stream.input().c_str());
					TEST_ASSERT_TRUE(shell.available());
					TEST_ASSERT_EQUAL_STRING("\nx\n", stream.input().c_str());
					TEST_ASSERT_EQUAL_INT('x', shell.read());
					TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
					TEST_ASSERT_TRUE(shell.available());
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
					TEST_ASSERT_EQUAL_INT('\n', shell.read());
				}
				TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
				TEST_ASSERT_FALSE(shell.available());
			} else {
				TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
					TEST_ASSERT_FALSE(shell.available());
				} else {
					TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
					TEST_ASSERT_TRUE(shell.available());
					TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
				}
			}
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_FALSE(shell.available());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;

	case BlockingTestMode::READ:
		test_fn = [executions, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("\nx\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('x', shell.read());
				TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('\n', shell.read());
				TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			} else {
				TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
			}
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_FALSE(shell.available());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;
	}

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("test\r\n", stream.output().c_str());

	stream << "A";
	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_TRUE(shell.available());
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
		}
		TEST_ASSERT_TRUE(shell.available());
		TEST_ASSERT_EQUAL_INT('A', shell.read());
		TEST_ASSERT_FALSE(shell.available());
		TEST_ASSERT_EQUAL_INT(-1, shell.read());
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return stop;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	stream << "BCD\r\nnoop\r\n";
	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('B', shell.peek());
		} else {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return stop;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("BCD\r\nnoop\r\n", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_EQUAL_INT('B', shell.read());
		TEST_ASSERT_EQUAL_INT('C', shell.read());
		TEST_ASSERT_EQUAL_INT('D', shell.read());
		TEST_ASSERT_EQUAL_INT('\r', shell.read());
		TEST_ASSERT_TRUE(shell.available());

		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('\n', shell.peek());
		} else {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return true;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("\nnoop\r\n", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	test_fn = [executions] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_FAIL();
		return true;
	};

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("noop\r\n$ ", stream.output().c_str());

	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

static void test_blocking_crlf_available_peek() {
	test_blocking_crlf(BlockingTestMode::AVAILABLE, true);
}

static void test_blocking_crlf_available_no_peek() {
	test_blocking_crlf(BlockingTestMode::AVAILABLE, false);
}

static void test_blocking_crlf_available_peek_with_data() {
	test_blocking_crlf(BlockingTestMode::AVAILABLE, true, true);
}

static void test_blocking_crlf_available_no_peek_with_data() {
	test_blocking_crlf(BlockingTestMode::AVAILABLE, false, true);
}

static void test_blocking_crlf_peek_peek() {
	test_blocking_crlf(BlockingTestMode::PEEK, true);
}

static void test_blocking_crlf_peek_no_peek() {
	test_blocking_crlf(BlockingTestMode::PEEK, false);
}

static void test_blocking_crlf_peek_peek_with_data() {
	test_blocking_crlf(BlockingTestMode::PEEK, true, true);
}

static void test_blocking_crlf_peek_no_peek_with_data() {
	test_blocking_crlf(BlockingTestMode::PEEK, false, true);
}

static void test_blocking_crlf_read_peek() {
	test_blocking_crlf(BlockingTestMode::READ, true);
}

static void test_blocking_crlf_read_no_peek() {
	test_blocking_crlf(BlockingTestMode::READ, false);
}

static void test_blocking_crlf_read_peek_with_data() {
	test_blocking_crlf(BlockingTestMode::READ, true, true);
}

static void test_blocking_crlf_read_no_peek_with_data() {
	test_blocking_crlf(BlockingTestMode::READ, false, true);
}

/**
 * Test with LF line endings.
 */
static void test_blocking_lf(BlockingTestMode mode, bool stream_supports_peek, bool with_data = false) {
	TestStream stream{stream_supports_peek};
	size_t executions = 0;
	auto console = std::make_shared<StreamConsole>(commands, stream);

	console->start();

	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	console->loop_one();

	stream << "test\n";
	if (with_data) {
		stream << "x\n";
	}
	switch (mode) {
	case BlockingTestMode::AVAILABLE:
		test_fn = [executions, stream_supports_peek, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_TRUE(shell.available());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('x', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('x', shell.read());
				TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('\n', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_INT('\n', shell.read());
			}
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_FALSE(shell.available());
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;

	case BlockingTestMode::PEEK:
		test_fn = [executions, stream_supports_peek, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('x', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('x', shell.read());
				if (stream_supports_peek) {
					TEST_ASSERT_EQUAL_INT('\n', shell.peek());
				} else {
					TEST_ASSERT_EQUAL_INT(-1, shell.peek());
				}
				TEST_ASSERT_EQUAL_INT('\n', shell.read());
			}
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;

	case BlockingTestMode::READ:
		test_fn = [executions, with_data, &stream] (Shell &shell, bool stop) mutable -> bool {
			TEST_ASSERT_EQUAL(1, ++executions);
			if (with_data) {
				TEST_ASSERT_EQUAL_STRING("x\n", stream.input().c_str());
				TEST_ASSERT_EQUAL_INT('x', shell.read());
				TEST_ASSERT_EQUAL_INT('\n', shell.read());
			}
			TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
			TEST_ASSERT_EQUAL_INT(-1, shell.read());
			TEST_ASSERT_FALSE(stop);
			return stop;
		};
		break;
	}

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("test\r\n", stream.output().c_str());

	if (!with_data) {
		console->loop_one();
		TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());
	}

	stream << "A";
	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_TRUE(shell.available());
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
			TEST_ASSERT_EQUAL_INT('A', shell.peek());
		}
		TEST_ASSERT_TRUE(shell.available());
		TEST_ASSERT_EQUAL_INT('A', shell.read());
		TEST_ASSERT_FALSE(shell.available());
		TEST_ASSERT_EQUAL_INT(-1, shell.read());
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return stop;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	stream << "BCD\nnoop\n";
	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('B', shell.peek());
		} else {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return stop;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("BCD\nnoop\n", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	test_fn = [executions, stream_supports_peek] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_EQUAL_INT('B', shell.read());
		TEST_ASSERT_EQUAL_INT('C', shell.read());
		TEST_ASSERT_EQUAL_INT('D', shell.read());
		TEST_ASSERT_EQUAL_INT('\n', shell.read());
		TEST_ASSERT_TRUE(shell.available());

		if (stream_supports_peek) {
			TEST_ASSERT_EQUAL_INT('n', shell.peek());
		} else {
			TEST_ASSERT_EQUAL_INT(-1, shell.peek());
		}
		TEST_ASSERT_FALSE(stop);
		return true;
	};
	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("noop\n", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	test_fn = [executions] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_FAIL();
		return true;
	};

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("noop\r\n$ ", stream.output().c_str());

	console->loop_one();
	TEST_ASSERT_EQUAL_STRING("", stream.output().c_str());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

static void test_blocking_lf_available_peek() {
	test_blocking_lf(BlockingTestMode::AVAILABLE, true);
}

static void test_blocking_lf_available_no_peek() {
	test_blocking_lf(BlockingTestMode::AVAILABLE, false);
}

static void test_blocking_lf_available_peek_with_data() {
	test_blocking_lf(BlockingTestMode::AVAILABLE, true, true);
}

static void test_blocking_lf_available_no_peek_with_data() {
	test_blocking_lf(BlockingTestMode::AVAILABLE, false, true);
}

static void test_blocking_lf_peek_peek() {
	test_blocking_lf(BlockingTestMode::PEEK, true);
}

static void test_blocking_lf_peek_no_peek() {
	test_blocking_lf(BlockingTestMode::PEEK, false);
}

static void test_blocking_lf_peek_peek_with_data() {
	test_blocking_lf(BlockingTestMode::PEEK, true, true);
}

static void test_blocking_lf_peek_no_peek_with_data() {
	test_blocking_lf(BlockingTestMode::PEEK, false, true);
}

static void test_blocking_lf_read_peek() {
	test_blocking_lf(BlockingTestMode::READ, true);
}

static void test_blocking_lf_read_no_peek() {
	test_blocking_lf(BlockingTestMode::READ, false);
}

static void test_blocking_lf_read_peek_with_data() {
	test_blocking_lf(BlockingTestMode::READ, true, true);
}

static void test_blocking_lf_read_no_peek_with_data() {
	test_blocking_lf(BlockingTestMode::READ, false, true);
}

/**
 * Test that the shell will not stop until the blocking function returns true.
 */
static void test_blocking_stop() {
	TestStream stream{true};
	size_t executions = 0;
	auto console = std::make_shared<StreamConsole>(commands, stream);

	console->start();

	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	console->loop_one();

	stream << "test\n";

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_FALSE(stop);
		TEST_ASSERT_TRUE(shell.running());
		return false;
	};

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("test\r\n", stream.output().c_str());

	TEST_ASSERT_TRUE(console->running());
	console->stop();
	TEST_ASSERT_TRUE(console->running());

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_TRUE(stop);
		TEST_ASSERT_TRUE(shell.running());
		return false;
	};
	console->loop_one();
	TEST_ASSERT_TRUE(console->running());

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_TRUE(stop);
		TEST_ASSERT_TRUE(shell.running());
		return false;
	};
	console->loop_one();
	TEST_ASSERT_TRUE(console->running());
	console->stop();
	TEST_ASSERT_TRUE(console->running());

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_TRUE(stop);
		TEST_ASSERT_TRUE(shell.running());
		return false;
	};
	console->loop_one();
	TEST_ASSERT_TRUE(console->running());

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_TRUE(stop);
		TEST_ASSERT_TRUE(shell.running());
		return true;
	};
	console->loop_one();
	TEST_ASSERT_FALSE(console->running());

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_FAIL();
		return true;
	};
	console->loop_one();
}

/**
 * Test that the shell will not allow access to the stream if a blocking function is not running.
 */
static void test_no_stream() {
	TestStream stream{true};
	size_t executions = 0;
	auto console = std::make_shared<StreamConsole>(commands, stream);

	console->start();

	TEST_ASSERT_EQUAL_STRING("$ ", stream.output().c_str());

	console->loop_one();

	stream << "test\n";

	TEST_ASSERT_FALSE(console->available());
	TEST_ASSERT_EQUAL_INT(-1, console->read());
	TEST_ASSERT_EQUAL_INT(-1, console->peek());

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_FALSE(stop);
		return stop;
	};

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING("test\r\n", stream.output().c_str());

	stream << "more";

	TEST_ASSERT_TRUE(console->available());
	TEST_ASSERT_EQUAL_INT('m', console->read());
	TEST_ASSERT_TRUE(console->available());
	TEST_ASSERT_EQUAL_INT('o', console->peek());
	TEST_ASSERT_TRUE(console->available());
	TEST_ASSERT_EQUAL_INT('o', console->read());
	TEST_ASSERT_TRUE(console->available());
	TEST_ASSERT_EQUAL_INT('r', console->peek());
	TEST_ASSERT_TRUE(console->available());

	test_fn = [executions, &stream] (Shell &shell, bool stop) mutable -> bool {
		TEST_ASSERT_EQUAL(1, ++executions);
		TEST_ASSERT_FALSE(stop);
		return true;
	};
	console->loop_one();

	TEST_ASSERT_FALSE(console->available());
	TEST_ASSERT_EQUAL_INT(-1, console->read());
	TEST_ASSERT_EQUAL_INT(-1, console->peek());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

int main(int argc, char *argv[]) {
	commands->add_command(0, 0, flash_string_vector{F("test")},
			[&] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.block_with([&] (Shell &shell, bool stop) -> bool {
			return test_fn(shell, stop);
		});
	});
	commands->add_command(0, 0, flash_string_vector{F("noop")},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});

	UNITY_BEGIN();
	RUN_TEST(test_blocking_cr_available_peek);
	RUN_TEST(test_blocking_cr_available_no_peek);
	RUN_TEST(test_blocking_cr_available_peek_with_data);
	RUN_TEST(test_blocking_cr_available_no_peek_with_data);
	RUN_TEST(test_blocking_cr_peek_peek);
	RUN_TEST(test_blocking_cr_peek_no_peek);
	RUN_TEST(test_blocking_cr_peek_peek_with_data);
	RUN_TEST(test_blocking_cr_peek_no_peek_with_data);
	RUN_TEST(test_blocking_cr_read_peek);
	RUN_TEST(test_blocking_cr_read_no_peek);
	RUN_TEST(test_blocking_cr_read_peek_with_data);
	RUN_TEST(test_blocking_cr_read_no_peek_with_data);
	RUN_TEST(test_blocking_crlf_available_peek);
	RUN_TEST(test_blocking_crlf_available_no_peek);
	RUN_TEST(test_blocking_crlf_available_peek_with_data);
	RUN_TEST(test_blocking_crlf_available_no_peek_with_data);
	RUN_TEST(test_blocking_crlf_peek_peek);
	RUN_TEST(test_blocking_crlf_peek_no_peek);
	RUN_TEST(test_blocking_crlf_peek_peek_with_data);
	RUN_TEST(test_blocking_crlf_peek_no_peek_with_data);
	RUN_TEST(test_blocking_crlf_read_peek);
	RUN_TEST(test_blocking_crlf_read_no_peek);
	RUN_TEST(test_blocking_crlf_read_peek_with_data);
	RUN_TEST(test_blocking_crlf_read_no_peek_with_data);
	RUN_TEST(test_blocking_lf_available_peek);
	RUN_TEST(test_blocking_lf_available_no_peek);
	RUN_TEST(test_blocking_lf_available_peek_with_data);
	RUN_TEST(test_blocking_lf_available_no_peek_with_data);
	RUN_TEST(test_blocking_lf_peek_peek);
	RUN_TEST(test_blocking_lf_peek_no_peek);
	RUN_TEST(test_blocking_lf_peek_peek_with_data);
	RUN_TEST(test_blocking_lf_peek_no_peek_with_data);
	RUN_TEST(test_blocking_lf_read_peek);
	RUN_TEST(test_blocking_lf_read_no_peek);
	RUN_TEST(test_blocking_lf_read_peek_with_data);
	RUN_TEST(test_blocking_lf_read_no_peek_with_data);
	RUN_TEST(test_blocking_stop);
	RUN_TEST(test_no_stream);

	return UNITY_END();
}
