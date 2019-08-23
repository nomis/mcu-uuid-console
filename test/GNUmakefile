.PHONY: all build native

all: build native
	python3 version_check.py

build:
	rm -rf build/*/.pio
	# Workaround for https://github.com/platformio/platformio-core/issues/2573
	cd build/example-DigitalIO && pio lib install uuid-common uuid-log
	platformio run -d build/example-DigitalIO

native:
	rm -rf native/.pio
	platformio test -d native
