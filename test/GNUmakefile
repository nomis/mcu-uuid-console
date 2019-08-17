.PHONY: all build native

all: build native

build:
	rm -rf build/*/.pio
	platformio run -d build/example-DigitalIO

native:
	rm -rf native/.pio
	platformio test -d native
