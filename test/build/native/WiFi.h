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

#ifndef WIFI_H_
#define WIFI_H_

#include <Arduino.h>

#define WIFI_SCAN_RUNNING -1
#define WIFI_SCAN_FAILED -2

class WiFiClass {
public:
		static int8_t scanNetworks(bool async);
		static int8_t scanComplete();
		static String SSID(uint8_t i);
		static int32_t RSSI(uint8_t i);
		static void scanDelete();
};

extern WiFiClass WiFi;

#endif
