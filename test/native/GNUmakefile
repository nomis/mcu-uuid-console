.PHONY: lcov

lcov:
	lcov -o .pio/build/lcov.info -z -d .pio/build/native/
	platformio test
	lcov -o .pio/build/lcov.info -c -d .pio/build/native/
	rm -rf .pio/build/native/lcov-html
	genhtml -o .pio/build/native/lcov-html --ignore-errors source .pio/build/lcov.info
