Change log
==========

Unreleased_
-----------

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
* Support for ``CR``, ``CRLF`` and ``LF`` line endings on input.
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
