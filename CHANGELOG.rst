Change log
==========

Unreleased
----------

Added
~~~~~

* Reusable container of multi-word commands that can be executed,
  with a fixed list of required/optional arguments per command.
* Shell context to support multiple layers of commands.
* Shell flags to support multiple access levels.
* Minimal line editing support (backspace, delete word, delete line).
* Text input in the US-ASCII character set.
* Support for ``CR``, ``CRLF`` and ``LF`` line endings on input.
* Tab completion to whole words for recognised commands/arguments.
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

.. |--| unicode:: U+2013 .. EN DASH
