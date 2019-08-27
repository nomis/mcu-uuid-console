#ifdef ARDUINO_ARCH_ESP8266
# include <ESP8266WiFi.h>
#else
# include <WiFi.h>
#endif

#include <memory>
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

	commands->add_command(flash_string_vector{F("wifi"), F("scan")},
		[] (Shell &shell, const std::vector<std::string> &arguments
				__attribute__((unused))) {

			int8_t ret = WiFi.scanNetworks(true);
			if (ret == WIFI_SCAN_RUNNING) {
				shell.println(F("Scanning for WiFi networks..."));

				/* This function will be called repeatedly on every
				 * loop until it returns true. It can be used to
				 * wait for the outcome of asynchronous operations
				 * without blocking execution of the main loop.
				 */
				shell.block_with([] (Shell &shell, bool stop) -> bool {
					int8_t ret = WiFi.scanComplete();

					if (ret == WIFI_SCAN_RUNNING) {
						/* Keep running until the scan completes
						 * or the shell is stopped.
						 */
						return stop;
					} else if (ret == WIFI_SCAN_FAILED || ret < 0) {
						shell.println(F("WiFi scan failed"));
						return true; /* stop running */
					} else {
						shell.printfln(F("Found %u networks"), ret);
						shell.println();

						for (uint8_t i = 0; i < (uint8_t)ret; i++) {
							shell.printfln(F("%s (%d dBm)"),
									WiFi.SSID(i).c_str(),
									WiFi.RSSI(i));
						}

						WiFi.scanDelete();
						return true; /* stop running */
					}
				});
			} else {
				shell.println(F("WiFi scan failed"));
			}
		}
	);

	Serial.begin(115200);

	std::shared_ptr<Shell> shell;
	shell = std::make_shared<uuid::console::StreamConsole>(commands, Serial);
	shell->start();
}

void loop() {
	uuid::loop();
	Shell::loop_all();
	yield();
}
