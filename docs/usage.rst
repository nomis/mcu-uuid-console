Usage
=====

.. code:: c++

   #include <uuid/console.h>

Create a |std::shared_ptr<uuid::console::Commands>|_ and populate it
with the commands to be available on the shell.

Create a |std::shared_ptr<uuid::console::StreamConsole>|_ referencing
the |Serial|_ stream and the commands list. Call |start()|_ on the
instance and then |uuid::console::Shell::loop_all()|_ regularly. (The
static set of all shells will retain a copy of the |shared_ptr|_ until
the shell is stopped.)

Example
-------

.. literalinclude:: ../examples/DigitalIO.cpp

.. |std::shared_ptr<uuid::console::Commands>| replace:: ``std::shared_ptr<uuid::console::Commands>``
.. _std::shared_ptr<uuid::console::Commands>: https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Commands.html

.. |std::shared_ptr<uuid::console::StreamConsole>| replace:: ``std::shared_ptr<uuid::console::StreamConsole>``
.. _std::shared_ptr<uuid::console::StreamConsole>: https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1StreamConsole.html

.. |Serial| replace:: ``Serial``
.. _Serial: https://www.arduino.cc/reference/en/language/functions/communication/serial/

.. |start()| replace:: ``start()``
.. _start(): https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Shell.html#a1d4509d78ab0a55a972c5b8133be75df

.. |uuid::console::Shell::loop_all()| replace:: ``uuid::console::Shell::loop_all()``
.. _uuid::console::Shell::loop_all(): https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Shell.html#affe5b4812696a9a53eed1f394301354e

.. |shared_ptr| replace:: ``shared_ptr``
.. _shared_ptr: https://en.cppreference.com/w/cpp/memory/shared_ptr

