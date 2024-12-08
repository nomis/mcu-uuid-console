Usage
=====

.. code:: c++

   #include <uuid/console.h>

Create a |std::shared_ptr<uuid::console::Commands>|_ and populate it
with the commands to be available on the shell.

Create a |std::shared_ptr<uuid::console::Shell>|_ referencing the
|Serial|_ stream and the commands list. Call |start()|_ on the
instance and then |uuid::console::Shell::loop_all()|_ regularly. (The
static set of all shells will retain a copy of the |shared_ptr|_ until
the shell is stopped.)

`Log messages <https://mcu-uuid-log.readthedocs.io/>`_ are written as
output to the shell automatically. Call |log_level()|_ on the shell to
change the log level.

Example (Digital I/O)
---------------------

.. literalinclude:: ../examples/DigitalIO.cpp

Output
~~~~~~

.. literalinclude:: ../examples/DigitalIO_output.txt
   :language: console

Example (WiFi network scan)
---------------------------

.. literalinclude:: ../examples/WiFi.cpp

Output
~~~~~~

.. literalinclude:: ../examples/WiFi_output.txt
   :language: console

.. |std::shared_ptr<uuid::console::Commands>| replace:: ``std::shared_ptr<uuid::console::Commands>``
.. _std::shared_ptr<uuid::console::Commands>: https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Commands.html

.. |std::shared_ptr<uuid::console::Shell>| replace:: ``std::shared_ptr<uuid::console::Shell>``
.. _std::shared_ptr<uuid::console::Shell>: https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Shell.html

.. |Serial| replace:: ``Serial``
.. _Serial: https://www.arduino.cc/reference/en/language/functions/communication/serial/

.. |start()| replace:: ``start()``
.. _start(): https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Shell.html#a1d4509d78ab0a55a972c5b8133be75df

.. |log_level()| replace:: ``log_level()``
.. _log_level(): https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Shell.html#ae47bd94c87f13242799bdde67e2f77ed

.. |uuid::console::Shell::loop_all()| replace:: ``uuid::console::Shell::loop_all()``
.. _uuid::console::Shell::loop_all(): https://mcu-doxygen.uuid.uk/classuuid_1_1console_1_1Shell.html#affe5b4812696a9a53eed1f394301354e

.. |shared_ptr| replace:: ``shared_ptr``
.. _shared_ptr: https://en.cppreference.com/w/cpp/memory/shared_ptr
