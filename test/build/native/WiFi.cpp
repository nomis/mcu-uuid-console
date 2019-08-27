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
#ifdef ENV_NATIVE

#include <WiFi.h>

static unsigned long __iterations = 0;

WiFiClass WiFi;

int8_t WiFiClass::scanNetworks(bool async) {
	if (async) {
		__iterations = 1;
		return WIFI_SCAN_RUNNING;
	} else {
		return 3;
	}
}

int8_t WiFiClass::scanComplete() {
	if (__iterations == 0) {
		return WIFI_SCAN_FAILED;
	} else if (++__iterations < 100) {
		return WIFI_SCAN_RUNNING;
	} else {
		return 3;
	}
}

String WiFiClass::SSID(uint8_t i) {
	if (__iterations == 0) {
		return "";
	}

	switch (i) {
	case 0:
		return "Free Public WiFi";

	case 1:
		return "Hacklab";

	case 2:
		return "ALL YOUR BASE ARE BELONG TO US";

	default:
		return "";
	}
}

int32_t WiFiClass::RSSI(uint8_t i) {
	if (__iterations == 0) {
		return 0;
	}

	switch (i) {
	case 0:
		return -87;

	case 1:
		return -30;

	case 2:
		return -44;

	default:
		return 0;
	}
}

void WiFiClass::scanDelete() {
	__iterations = 0;
}
#endif
