.PHONY: all build native coverage doxygen registry
SHELL=/bin/bash

all: build native doxygen
	python3 version_check.py

build:
	rm -rf build/*/.pio

	# Workaround for https://github.com/platformio/platformio-core/issues/2573
	cd build/example-DigitalIO && pio lib install --no-save nomis/uuid-common nomis/uuid-log
	platformio run -d build/example-DigitalIO
	build/example-DigitalIO/.pio/build/native/program <../examples/DigitalIO_input.txt >../examples/DigitalIO_output.txt

	# Workaround for https://github.com/platformio/platformio-core/issues/2573
	cd build/example-WiFi && pio lib install --no-save nomis/uuid-common nomis/uuid-log
	platformio run -d build/example-WiFi
	build/example-WiFi/.pio/build/native/program <../examples/WiFi_input.txt >../examples/WiFi_output.txt

native:
	rm -rf native/.pio
	platformio test -d native

coverage: native
	lcov -d native/.pio/build/native/ -c -o native/.pio/coverage.info
	rm -rf coverage/
	genhtml -o coverage/ --demangle-cpp --ignore-errors source native/.pio/coverage.info

doxygen:
	wget https://raw.githubusercontent.com/nomis/mcu-uuid-doxygen/main/Doxyfile -O Doxyfile
	rm -rf html
	(cat Doxyfile; echo INPUT += ../src) | doxygen - 1>doxygen.out 2>doxygen.err
	cat doxygen.err
	cmp /dev/null doxygen.err

registry:
	diff -U0 <(git tag | sort -n) <(platformio pkg show nomis/uuid-console | grep -A 1000000 -E ^Version | tail -n +3 | grep -vE '^$$' | awk '{print $$1}' | sort -n)
