.PHONY: all build native

all: build native
	python3 version_check.py

build:
	rm -rf build/*/.pio

	# Workaround for https://github.com/platformio/platformio-core/issues/2573
	cd build/example-DigitalIO && pio lib install uuid-common uuid-log
	platformio run -d build/example-DigitalIO
	build/example-DigitalIO/.pio/build/native/program <../examples/DigitalIO_input.txt >../examples/DigitalIO_output.txt

	# Workaround for https://github.com/platformio/platformio-core/issues/2573
	cd build/example-WiFi && pio lib install uuid-common uuid-log
	platformio run -d build/example-WiFi
	build/example-WiFi/.pio/build/native/program <../examples/WiFi_input.txt >../examples/WiFi_output.txt

native:
	rm -rf native/.pio
	platformio test -d native
