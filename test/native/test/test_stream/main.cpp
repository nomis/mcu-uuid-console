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

class TestConsole;

static std::shared_ptr<Commands> commands = std::make_shared<Commands>();
static Shell::blocking_function test_fn;
static std::function<void(TestConsole &shell)> eot_fn;

class TestConsole: public StreamConsole {
public:
	TestConsole(std::shared_ptr<Commands> commands, Stream &stream)
			: uuid::console::Shell(std::move(commands)), StreamConsole(stream) {

	}

	using StreamConsole::invoke_command;

protected:
	void end_of_transmission() override {
		eot_fn(*this);
	}
};

static size_t recursion_count = 0;

class RecursionConsole: public StreamConsole {
public:
	RecursionConsole(std::shared_ptr<Commands> commands, Stream &stream, int level)
			: uuid::console::Shell(std::move(commands)), StreamConsole(stream), level_(level) {

	}

	const int count_ = recursion_count++;
	const int level_;

protected:
	void display_banner() override {
		printfln("Recursion console %u started (level %d)", count_, level_);
	}

	void stopped() override {
		printfln("Recursion console %u stopped (level %d)", count_, level_);
	}
};

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

/**
 * Test help output.
 */
static void test_help() {
	TestStream stream{true};
	auto console = std::make_shared<StreamConsole>(commands, stream);

	console->start();
	stream << "help\n";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ help\r\n"
			"test\r\n"
			"noop\r\n"
			"sh\r\n"
			"exit\r\n"
			"command\\ with\\ spaces and\\ more\\ spaces <argument with spaces> [and more spaces] don't do this it's confusing\r\n"
			"help\r\n"
			"$ ", stream.output().c_str());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with no-op commands.
 */
static void test_end_of_transmission1() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("noop");
	};

	console->start();
	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\r\n"
			"$ ", stream.output().c_str());

	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"noop\r\n"
			"$ ", stream.output().c_str());

	stream << "noop\r\n";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"noop\r\n"
			"$ ", stream.output().c_str());

	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"noop\r\n"
			"$ ", stream.output().c_str());

	stream << "noop\r\n";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"noop\r\n"
			"$ ", stream.output().c_str());

	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"noop\r\n"
			"$ ", stream.output().c_str());

	stream << "\r\n";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"\r\n"
			"$ ", stream.output().c_str());

	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"noop\r\n"
			"$ ", stream.output().c_str());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with no prior input and stopping command.
 */
static void test_end_of_transmission2a() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("exit");
	};

	console->start();
	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ exit\r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with prior input and stopping command.
 */
static void test_end_of_transmission2b() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("exit");
	};

	console->start();
	stream << "noop\r\n\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\r\n"
			"$ exit\r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with prior input and stopping command.
 */
static void test_end_of_transmission2c() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("exit");
	};

	console->start();
	stream << "noop\r\n\r\n\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\r\n"
			"$ \r\n"
			"$ exit\r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with interrupted input and stopping command.
 */
static void test_end_of_transmission2d() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("exit");
	};

	console->start();
	stream << "noop\x03\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\r\n"
			"$ exit\r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with no prior input and immediate stop.
 */
static void test_end_of_transmission3a() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.stop();
	};

	console->start();
	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ ", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with no prior input and immediate stop (with newline).
 */
static void test_end_of_transmission3b() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.println();
		shell.stop();
	};

	console->start();
	stream << "\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ \r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with a non-empty buffer.
 */
static void test_end_of_transmission4() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.stop();
	};

	console->start();
	stream << "noop\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop", stream.output().c_str());

	TEST_ASSERT_TRUE(console->running());
	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and with no-op command.
 */
static void test_end_of_transmission5a() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("noop");
	};

	console->start();
	stream << "noop\x15\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ noop\r\n"
			"$ ", stream.output().c_str());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and stopping command.
 */
static void test_end_of_transmission5b() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("exit");
	};

	console->start();
	stream << "noop\x15\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ exit\r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and immediate stop.
 */
static void test_end_of_transmission5c() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.stop();
	};

	console->start();
	stream << "noop\x15\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ ", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and immediate stop (with newline).
 */
static void test_end_of_transmission5d() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.println();
		shell.stop();
	};

	console->start();
	stream << "noop\x15\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ \r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and with no-op command.
 */
static void test_end_of_transmission5e() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("noop");
	};

	console->start();
	stream << "noop\x17\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ noop\r\n"
			"$ ", stream.output().c_str());

	console->stop();
	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and stopping command.
 */
static void test_end_of_transmission5f() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.invoke_command("exit");
	};

	console->start();
	stream << "noop\x17\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ exit\r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and immediate stop.
 */
static void test_end_of_transmission5g() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.stop();
	};

	console->start();
	stream << "noop\x17\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ ", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test end of transmission with deleted input and immediate stop (with newline).
 */
static void test_end_of_transmission5h() {
	TestStream stream{true};
	auto console = std::make_shared<TestConsole>(commands, stream);

	eot_fn = [] (TestConsole &shell) {
		shell.println();
		shell.stop();
	};

	console->start();
	stream << "noop\x17\x04";

	while (!stream.empty()) {
		console->loop_one();
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"$ noop\x1B[0G\x1B[K"
			"$ \r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test shell recursion using exit commands to stop.
 */
static void test_recursion1() {
	TestStream stream{true};
	recursion_count = 0;
	auto console = std::make_shared<RecursionConsole>(commands, stream, 0);

	console->start();
	stream << "sh\r\n"; /* In 1 */
	stream << "exit\r\n"; /* Out 1 */
	stream << "sh\r\nsh\r\n"; /* In 2 */
	stream << "exit\r\nexit\r\n"; /* Out 2 */
	stream << "sh\r\nsh\r\nsh\r\n"; /* In 3 */
	stream << "exit\r\nexit\r\nexit\r\n"; /* Out 3 */
	stream << "sh\r\nsh\r\nsh\r\nsh\r\n"; /* In 4 */
	stream << "exit\r\nexit\r\nexit\r\nexit\r\n"; /* Out 4 */
	stream << "sh\r\nsh\r\nsh\r\nsh\r\nsh\r\n"; /* In 5 */
	stream << "exit\r\nexit\r\nexit\r\nexit\r\nexit\r\n"; /* Out 5 */

	stream << "sh\r\nsh\r\nsh\r\nsh\r\nsh\r\n"; /* In 5 */
	stream << "exit\r\n"; /* Out 1 */
	stream << "sh\r\n"; /* In 1 */
	stream << "exit\r\nexit\r\n"; /* Out 2 */
	stream << "sh\r\nsh\r\n"; /* In 2 */
	stream << "exit\r\nexit\r\nexit\r\n"; /* Out 3 */
	stream << "sh\r\nsh\r\nsh\r\n"; /* In 3 */
	stream << "exit\r\nexit\r\nexit\r\nexit\r\nexit\r\n"; /* Out 5 */

	stream << "exit\r\n";

	while (stream.input().length() > 1) {
		Shell::loop_all(); /* This is unsafe if any of the earlier tests failed */
	}
	Shell::loop_all();
	TEST_ASSERT_EQUAL_STRING("\n", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"Recursion console 0 started (level 0)\r\n"
			"$ sh\r\n"
			"Recursion console 1 started (level 1)\r\n"
			"$ exit\r\n"
			"Recursion console 1 stopped (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 2 started (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 3 started (level 2)\r\n"
			"$ exit\r\n"
			"Recursion console 3 stopped (level 2)\r\n"
			"$ exit\r\n"
			"Recursion console 2 stopped (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 4 started (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 5 started (level 2)\r\n"
			"$ sh\r\n"
			"Recursion console 6 started (level 3)\r\n"
			"$ exit\r\n"
			"Recursion console 6 stopped (level 3)\r\n"
			"$ exit\r\n"
			"Recursion console 5 stopped (level 2)\r\n"
			"$ exit\r\n"
			"Recursion console 4 stopped (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 7 started (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 8 started (level 2)\r\n"
			"$ sh\r\n"
			"Recursion console 9 started (level 3)\r\n"
			"$ sh\r\n"
			"Recursion console 10 started (level 4)\r\n"
			"$ exit\r\n"
			"Recursion console 10 stopped (level 4)\r\n"
			"$ exit\r\n"
			"Recursion console 9 stopped (level 3)\r\n"
			"$ exit\r\n"
			"Recursion console 8 stopped (level 2)\r\n"
			"$ exit\r\n"
			"Recursion console 7 stopped (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 11 started (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 12 started (level 2)\r\n"
			"$ sh\r\n"
			"Recursion console 13 started (level 3)\r\n"
			"$ sh\r\n"
			"Recursion console 14 started (level 4)\r\n"
			"$ sh\r\n"
			"Recursion console 15 started (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 15 stopped (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 14 stopped (level 4)\r\n"
			"$ exit\r\n"
			"Recursion console 13 stopped (level 3)\r\n"
			"$ exit\r\n"
			"Recursion console 12 stopped (level 2)\r\n"
			"$ exit\r\n"
			"Recursion console 11 stopped (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 16 started (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 17 started (level 2)\r\n"
			"$ sh\r\n"
			"Recursion console 18 started (level 3)\r\n"
			"$ sh\r\n"
			"Recursion console 19 started (level 4)\r\n"
			"$ sh\r\n"
			"Recursion console 20 started (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 20 stopped (level 5)\r\n"
			"$ sh\r\n"
			"Recursion console 21 started (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 21 stopped (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 19 stopped (level 4)\r\n"
			"$ sh\r\n"
			"Recursion console 22 started (level 4)\r\n"
			"$ sh\r\n"
			"Recursion console 23 started (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 23 stopped (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 22 stopped (level 4)\r\n"
			"$ exit\r\n"
			"Recursion console 18 stopped (level 3)\r\n"
			"$ sh\r\n"
			"Recursion console 24 started (level 3)\r\n"
			"$ sh\r\n"
			"Recursion console 25 started (level 4)\r\n"
			"$ sh\r\n"
			"Recursion console 26 started (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 26 stopped (level 5)\r\n"
			"$ exit\r\n"
			"Recursion console 25 stopped (level 4)\r\n"
			"$ exit\r\n"
			"Recursion console 24 stopped (level 3)\r\n"
			"$ exit\r\n"
			"Recursion console 17 stopped (level 2)\r\n"
			"$ exit\r\n"
			"Recursion console 16 stopped (level 1)\r\n"
			"$ exit\r\n"
			"Recursion console 0 stopped (level 0)\r\n", stream.output().c_str());

	TEST_ASSERT_FALSE(console->running());
}

/**
 * Test shell recursion using stop function.
 */
static void test_recursion2() {
	TestStream stream{true};
	recursion_count = 0;
	auto console = std::make_shared<RecursionConsole>(commands, stream, 0);

	console->start();
	stream << "sh\r\nsh\r\nsh\r\nsh\r\nsh\r\nsh\r\nsh\r\nsh\r\nsh\r\nsh\r\n"; /* In 10 */

	while (!stream.empty()) {
		Shell::loop_all(); /* This is unsafe if any of the earlier tests failed */
	}
	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"Recursion console 0 started (level 0)\r\n"
			"$ sh\r\n"
			"Recursion console 1 started (level 1)\r\n"
			"$ sh\r\n"
			"Recursion console 2 started (level 2)\r\n"
			"$ sh\r\n"
			"Recursion console 3 started (level 3)\r\n"
			"$ sh\r\n"
			"Recursion console 4 started (level 4)\r\n"
			"$ sh\r\n"
			"Recursion console 5 started (level 5)\r\n"
			"$ sh\r\n"
			"Recursion console 6 started (level 6)\r\n"
			"$ sh\r\n"
			"Recursion console 7 started (level 7)\r\n"
			"$ sh\r\n"
			"Recursion console 8 started (level 8)\r\n"
			"$ sh\r\n"
			"Recursion console 9 started (level 9)\r\n"
			"$ sh\r\n"
			"Recursion console 10 started (level 10)\r\n"
			"$ ", stream.output().c_str());

	console->stop();
	TEST_ASSERT_TRUE(console->running());

	size_t maximum_iterations = 100;
	while (console->running() && maximum_iterations-- != 0) {
		Shell::loop_all(); /* This is unsafe if any of the earlier tests failed */
	}

	TEST_ASSERT_EQUAL_STRING("", stream.input().c_str());
	TEST_ASSERT_EQUAL_STRING(
			"Recursion console 10 stopped (level 10)\r\n"
			"Recursion console 9 stopped (level 9)\r\n"
			"Recursion console 8 stopped (level 8)\r\n"
			"Recursion console 7 stopped (level 7)\r\n"
			"Recursion console 6 stopped (level 6)\r\n"
			"Recursion console 5 stopped (level 5)\r\n"
			"Recursion console 4 stopped (level 4)\r\n"
			"Recursion console 3 stopped (level 3)\r\n"
			"Recursion console 2 stopped (level 2)\r\n"
			"Recursion console 1 stopped (level 1)\r\n"
			"Recursion console 0 stopped (level 0)\r\n", stream.output().c_str());

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
	commands->add_command(0, 0, flash_string_vector{F("sh")},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		auto console = std::make_shared<RecursionConsole>(commands, shell, dynamic_cast<RecursionConsole&>(shell).level_ + 1);
		console->start();
		shell.block_with([console] (Shell &shell, bool stop) -> bool {
			if (stop) {
				console->stop();
			}
			return !console->running();
		});
	});
	commands->add_command(0, 0, flash_string_vector{F("exit")},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.stop();
	});
	commands->add_command(0, 0, flash_string_vector{F("command with spaces"), F("and more spaces")},
			flash_string_vector{F("<argument with spaces>"), F("[and more spaces]"), F("don't do this it's confusing")},
			[] (Shell &shell __attribute__((unused)), const std::vector<std::string> &arguments __attribute__((unused))) {

	});
	commands->add_command(0, 0, flash_string_vector{F("help")},
			[] (Shell &shell, const std::vector<std::string> &arguments __attribute__((unused))) {
		shell.print_all_available_commands();
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
	RUN_TEST(test_help);
	RUN_TEST(test_end_of_transmission1);
	RUN_TEST(test_end_of_transmission2a);
	RUN_TEST(test_end_of_transmission2b);
	RUN_TEST(test_end_of_transmission2c);
	RUN_TEST(test_end_of_transmission2d);
	RUN_TEST(test_end_of_transmission3a);
	RUN_TEST(test_end_of_transmission3b);
	RUN_TEST(test_end_of_transmission4);
	RUN_TEST(test_end_of_transmission5a);
	RUN_TEST(test_end_of_transmission5b);
	RUN_TEST(test_end_of_transmission5c);
	RUN_TEST(test_end_of_transmission5d);
	RUN_TEST(test_end_of_transmission5e);
	RUN_TEST(test_end_of_transmission5f);
	RUN_TEST(test_end_of_transmission5g);
	RUN_TEST(test_end_of_transmission5h);
	RUN_TEST(test_recursion1);
	RUN_TEST(test_recursion2);

	return UNITY_END();
}
