Usage
=====

.. code:: c++

   #include <uuid/console.h>

Create a ``std::shared<uuid::console::Commands>`` and populate it with
the commands to be available on the shell.

Create a ``std::shared_ptr<uuid::console::StreamConsole>`` referencing
the ``Serial`` stream and the commands list. Call ``start()`` on the
instance and then ``uuid::console::Shell::loop_all()`` regularly. (The
static set of all shells will retain a copy of the ``shared_ptr`` until
the shell is stopped.)

Example
-------

.. literalinclude:: ../examples/DigitalIO.cpp
   :tab-width: 4
