Change log
==========

Unreleased_
-----------

Added
~~~~~

* Support for blocking commands that execute asynchronously and can
  read from the underlying input stream.

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

.. _Unreleased: https://github.com/nomis/mcu-uuid-console/compare/0.1.0...HEAD
.. _0.1.0: https://github.com/nomis/mcu-uuid-console/commits/0.1.0