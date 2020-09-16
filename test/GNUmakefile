.PHONY: all build native doxygen

all: build native doxygen
	python3 version_check.py

build:
	rm -rf build/*/.pio

	# Workaround for https://github.com/platformio/platformio-core/issues/2573
	cd build/example-DigitalIO && pio lib install uuid-common uuid-log
	# Workaround for https://github.com/platformio/platformio-core/issues/3673
	git checkout -- build/example-DigitalIO/platformio.ini
	platformio run -d build/example-DigitalIO
	build/example-DigitalIO/.pio/build/native/program <../examples/DigitalIO_input.txt >../examples/DigitalIO_output.txt

	# Workaround for https://github.com/platformio/platformio-core/issues/2573
	cd build/example-WiFi && pio lib install uuid-common uuid-log
	# Workaround for https://github.com/platformio/platformio-core/issues/3673
	git checkout -- build/example-WiFi/platformio.ini
	platformio run -d build/example-WiFi
	build/example-WiFi/.pio/build/native/program <../examples/WiFi_input.txt >../examples/WiFi_output.txt

native:
	rm -rf native/.pio
	platformio test -d native

doxygen:
	wget https://raw.githubusercontent.com/nomis/mcu-uuid-doxygen/master/Doxyfile -O Doxyfile
	rm -rf html
	(cat Doxyfile; echo INPUT += ../src) | doxygen - 1>doxygen.out 2>doxygen.err
	cat doxygen.err
	cmp /dev/null doxygen.err
