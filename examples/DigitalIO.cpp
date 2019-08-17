#include <Arduino.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <uuid/common.h>
#include <uuid/console.h>

using uuid::read_flash_string;
using uuid::flash_string_vector;
using uuid::console::Commands;
using uuid::console::Shell;

void setup() {
	std::shared_ptr<Commands> commands = std::make_shared<Commands>();

	commands->add_command(0, 0, flash_string_vector{F("mode")},
			flash_string_vector{F("<pin>"), F("<mode>")},

			[] (Shell &shell, const std::vector<std::string> &arguments) {
				uint8_t pin = String(arguments[0].c_str()).toInt();
				uint8_t mode;

				if (arguments[1] == read_flash_string(F("INPUT"))) {
					mode = INPUT;
				} else if (arguments[1] == read_flash_string(F("OUTPUT"))) {
					mode = OUTPUT;
				} else if (arguments[1] == read_flash_string(F("INPUT_PULLUP"))) {
					mode = INPUT_PULLUP;
				} else {
					shell.println(F("Invalid mode"));
					return;
				}

				pinMode(pin, mode);
				shell.printfln(F("Configured pin %u to mode %s"),
						pin, arguments[1].c_str());
			},

			[] (Shell &shell __attribute__((unused)),
					const std::vector<std::string> &arguments)
					-> const std::set<std::string> {
				if (arguments.size() == 1) {
					/* The first argument has been provided, so return
					 * completion values for the second argument.
					 */
					return std::set<std::string>{
						read_flash_string(F("INPUT")),
						read_flash_string(F("OUTPUT")),
						read_flash_string(F("INPUT_PULLUP"))
					};
				} else {
					return std::set<std::string>{};
				}
			}
	);

	commands->add_command(0, 0, flash_string_vector{F("read")},
			flash_string_vector{F("<pin>")},

			[] (Shell &shell,
					const std::vector<std::string> &arguments) {
				uint8_t pin = String(arguments[0].c_str()).toInt();
				auto value = digitalRead(pin);

				shell.printfln(F("Read value from pin %u: %S"),
						pin, value == HIGH ? F("HIGH") : F("LOW"));
			},

			Commands::no_argument_completion()
	);

	commands->add_command(0, 0, flash_string_vector{F("write")},
			flash_string_vector{F("<pin>"), F("<value>")},

			[] (Shell &shell, const std::vector<std::string> &arguments) {
				uint8_t pin = String(arguments[0].c_str()).toInt();
				uint8_t value;

				if (arguments[1] == read_flash_string(F("HIGH"))) {
					value = HIGH;
				} else if (arguments[1] == read_flash_string(F("LOW"))) {
					value = LOW;
				} else {
					shell.println(F("Invalid value"));
					return;
				}

				digitalWrite(pin, value);
				shell.printfln(F("Wrote %s value to pin %u"),
						arguments[1].c_str(), pin);
			},

			[] (Shell &shell __attribute__((unused)),
					const std::vector<std::string> &arguments)
					-> const std::set<std::string> {
				if (arguments.size() == 1) {
					/* The first argument has been provided, so return
					 * completion values for the second argument.
					 */
					return std::set<std::string>{
						read_flash_string(F("HIGH")),
						read_flash_string(F("LOW"))
					};
				} else {
					return std::set<std::string>{};
				}
			}
	);

	Serial.begin(115200);

	std::shared_ptr<Shell> shell;
	shell = std::make_shared<uuid::console::StreamConsole>(commands, Serial, 0, 0);
	shell->start();
}

void loop() {
	uuid::loop();
	Shell::loop_all();
	yield();
}
