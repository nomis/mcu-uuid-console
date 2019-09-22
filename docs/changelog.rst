Change log
==========

Unreleased_
-----------

0.7.3_ |--| 2019-09-22
----------------------

Bug fixes.

Fixed
~~~~~

* Output an error message if the shell has no commands.
* Avoid running a shell loop if it has already stopped.

0.7.2_ |--| 2019-09-17
----------------------

Logout improvements on remote shells.

Changed
~~~~~~~

* Automatically stop the shell on end of transmission character if an
  idle timeout is set.

0.7.1_ |--| 2019-09-16
----------------------

Tab completion bug fixes.

Fixed
~~~~~

* Problem with tab completion when the partial match commands have
  arguments and the longest common prefix is returned.
* Incorrect partial tab completion matches when the command line has a
  trailing space.

0.7.0_ |--| 2019-09-15
----------------------

Add idle timeout.

Added
~~~~~

* Configurable idle timeout.

Fixed
~~~~~

* Use move constructors on rvalues.

0.6.0_ |--| 2019-09-03
----------------------

Bug fixes and additional configuration options.

Changed
~~~~~~~

* Remove ``get_`` and ``set_`` from function names.
* Move maximum command line length and maximum log messages to
  getter/setter functions.

Fixed
~~~~~

* Remove messages from the log queue before processing them.
* Problems with tab completion of commands and arguments when there are
  multiple exact matches or there is a single shortest partial match
  with multiple longer partial matches.

0.5.0_ |--| 2019-08-31
----------------------

Fix escaping of command line argument help text.

Changed
~~~~~~~

* Avoid copying command line arguments when executing commands.
* Executed commands can now modify their arguments.
* Use ``std::vector`` instead of ``std::list`` for most containers to
  reduce memory usage.

Fixed
~~~~~

* Don't escape command line argument help text.

0.4.0_ |--| 2019-08-30
----------------------

Support for printing all currently available commands.

Added
~~~~~

* Support for printing all currently available commands.

Changed
~~~~~~~

* Move trailing space handling into instances of the ``CommandLine``
  class.

Fixed
~~~~~

* Support tab completion of empty arguments.

0.3.0_ |--| 2019-08-28
----------------------

Support for empty arguments using quotes.

Added
~~~~~

* Support for empty arguments using quotes (``""`` or ``''``).
* Move command line parsing/formatting to a ``CommandLine`` utility
  class.

0.2.0_ |--| 2019-08-27
----------------------

Support blocking commands that execute asynchronously.

Added
~~~~~

* Support for blocking commands that execute asynchronously and can
  read from the underlying input stream.
* Example serial console for ESP8266/ESP32 WiFi features.

Changed
~~~~~~~

* The default context is now optional when creating a ``Shell`` (it
  defaults to 0).
* Commands can now be created with a default context and flags of 0.

Fixed
~~~~~

* Don't set private member ``prompt_displayed_`` from virtual function
  ``erase_current_line()``.
* Don't try to write empty strings to the shell output.
* Workaround incorrect definition of ``FPSTR()`` on ESP32
  (`#1371 <https://github.com/espressif/arduino-esp32/issues/1371>`_).
* Create a copy of ``va_list`` when outputting with a format string so
  that it can be used twice.

0.1.0_ |--| 2019-08-23
----------------------

Initial development release.

Added
~~~~~

* Reusable container of multi-word commands that can be executed,
  with a fixed list of required/optional arguments per command.
* Shell context to support multiple layers of commands.
* Shell flags to support multiple access levels.
* Minimal line editing support (backspace, delete word, delete line).
* Text input in the US-ASCII character set.
* Support for entry of spaces in arguments using backslashes or quotes.
* Support for CR, CRLF and LF line endings on input.
* Tab completion for recognised commands/arguments.
* Logging handler to output log messages without interrupting the entry
  of commands at a prompt.
* Password entry prompt.
* Customisable ``Shell`` class:

  * Replaceable prompt text.
  * Optional banner, hostname and context text.
  * Support for the ``^D`` (end of transmission) character with implied
    command execution (e.g. ``logout``).

* Support for ``Stream`` (``Serial``) consoles.
* Loop function to consolidate the execution of all active shells.
* Example serial console for Arduino Digital I/O features.

.. |--| unicode:: U+2013 .. EN DASH

.. _Unreleased: https://github.com/nomis/mcu-uuid-console/compare/0.7.3...HEAD
.. _0.7.3: https://github.com/nomis/mcu-uuid-console/compare/0.7.2...0.7.3
.. _0.7.2: https://github.com/nomis/mcu-uuid-console/compare/0.7.1...0.7.2
.. _0.7.1: https://github.com/nomis/mcu-uuid-console/compare/0.7.0...0.7.1
.. _0.7.0: https://github.com/nomis/mcu-uuid-console/compare/0.6.0...0.7.0
.. _0.6.0: https://github.com/nomis/mcu-uuid-console/compare/0.5.0...0.6.0
.. _0.5.0: https://github.com/nomis/mcu-uuid-console/compare/0.4.0...0.5.0
.. _0.4.0: https://github.com/nomis/mcu-uuid-console/compare/0.3.0...0.4.0
.. _0.3.0: https://github.com/nomis/mcu-uuid-console/compare/0.2.0...0.3.0
.. _0.2.0: https://github.com/nomis/mcu-uuid-console/compare/0.1.0...0.2.0
.. _0.1.0: https://github.com/nomis/mcu-uuid-console/commits/0.1.0
