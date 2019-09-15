Features
========

A static loop function consolidates the execution of all active shells.

Flexible command definition
---------------------------

Shells support a context stack so that multiple layers of commands can
be implemented, and flags to support multiple access levels.

Commands can be composed of multiple words and have a fixed list of
required/optional arguments per command.

Command line prompt
-------------------

Text encoded using the US-ASCII character set can be entered with basic
basic line editing (backspace, delete word, delete line). All standard
line endings (CR, CRLF and LF) are supported.

Both command names and arguments (where the command returns a list of
potential arguments) can be tab completed and spaces can be escaped
using backslashes or quotes.

Password entry (without echo) can be performed using a callback function
process.

Blocking commands can be performed using a callback function to execute
asynchronously and can read from the underlying input stream.

The ``Shell`` class is customisable to allow the prompt, banner,
hostname and context text to be replaced. The ``^D`` (end of
transmission) character can be made to execute implied commands (e.g.
``logout``).

Logging
-------

Acts as a `log handler <https://mcu-uuid-log.readthedocs.io/>`_ in order
to output log messages without interrupting the entry of commands at a
prompt.

Session
-------

An idle timeout can be configured to automatically stop the shell if it
is waiting at a prompt for too long.
